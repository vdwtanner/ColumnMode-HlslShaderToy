#include "pch.h"
#include "VS_ScreenSpacePassthrough.h"
#include "PS_Magenta.h"

using namespace CM_HlslShaderToy;

Renderer::Renderer(ShaderToyWindow* pShaderToyWindow)
{
	assert(pShaderToyWindow != nullptr);
	h_pShaderToyWindow = pShaderToyWindow;
}

bool Renderer::Init()
{
#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			m_dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	SILENT_RETURN_FALSE(CreateDevice());
	
	//CommandQueue
	D3D12_COMMAND_QUEUE_DESC cqDesc = {}; //default settings to create a DirectCommandQueue
	BAIL_ON_FAIL(m_pDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&m_pCommandQueue)),
		L"Failed to create CommandQueue");

	SILENT_RETURN_FALSE(CreateSwapChain());
	SILENT_RETURN_FALSE(CreateRTVs());
	SILENT_RETURN_FALSE(CreateCommandList());
	SILENT_RETURN_FALSE(CreateFences());
	SILENT_RETURN_FALSE(PrepareShaderPreviewResources());

	m_isInitialized = true;
	return true;
}

bool Renderer::CreateDevice()
{
	ComPtr<IDXGIFactory4> pDxgiFactory;
	VerifyHR(CreateDXGIFactory2(m_dxgiFactoryFlags, IID_PPV_ARGS(&pDxgiFactory)));
	
	ComPtr<IDXGIAdapter1> pAdapter;
	int adapterIndex = 0;
	bool adapterFound = false;

	//TODO - Find most performant GPU?
	while (pDxgiFactory->EnumAdapters1(adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// skip that software adapter noise
			adapterIndex++;
			continue;
		}

		// Look for the adapter index that points to a valid d3d12 compatible device
		if (SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
		{
			adapterFound = true;
			break;
		}

		adapterIndex++;
	}

	MESSAGE_RETURN_FALSE(adapterFound, L"No valid D3D12 adapter found");

	BAIL_ON_FAIL(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice)), L"Failed to create D3D12 device.");

	return true;
}

bool Renderer::CreateSwapChain()
{
	assert(m_pCommandQueue != nullptr);
	ComPtr<IDXGIFactory4> pDxgiFactory;
	VerifyHR(CreateDXGIFactory2(m_dxgiFactoryFlags, IID_PPV_ARGS(&pDxgiFactory)));

	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1; // TODO: Support MSAA?
	sampleDesc.Quality = 0;

	HWND hwnd;
	MESSAGE_RETURN_FALSE(h_pShaderToyWindow->TryGetD3D12Hwnd(hwnd), L"Can't create a swapchain without a valid HWND");

	//DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = m_frameBufferCount;
	swapChainDesc.Width = m_width; // Can pass 0 to use the output window dimensions and later fetch via IDXGISwapChain1::GetDesc1
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = m_rtvFormat;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // pipeline will render to this swapchain
	swapChainDesc.SampleDesc = sampleDesc;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.Stereo = false;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = 0;

	ComPtr<IDXGISwapChain1> tempSwapChain;
	BAIL_ON_FAIL(pDxgiFactory->CreateSwapChainForHwnd(m_pCommandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, tempSwapChain.GetAddressOf()), L"Failed to create SwapChain")

	// convert to SwapChain3 and handle ref counting
	VerifyHR(tempSwapChain.As(&m_pSwapChain));
	m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	return true;
}

bool Renderer::CreateRTVs()
{
	assert(m_pDevice != nullptr);
	assert(m_pSwapChain != nullptr);

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = m_frameBufferCount;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // It is illegal to create a RTV/DSV heap as shader visible

	BAIL_ON_FAIL(m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRtvDescHeap)),
		L"Failed to create RTV descriptor heap");

	m_rtvDescSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescHandle(m_pRtvDescHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < m_frameBufferCount; i++)
	{
		// Get the Render Target resource from the swapchain
		BAIL_ON_FAIL(m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pRenderTargets[i])), L"Failed to get RT buffer from swapchain");

		// Create a Render Target View for the resource and store it at the descriptor handle's specified location
		m_pDevice->CreateRenderTargetView(
			m_pRenderTargets[i].Get(),
			nullptr, // Not using subresources, so can pass nullptr here
			rtvDescHandle); // Offset for each desc

		rtvDescHandle.Offset(1, m_rtvDescSize); //Offset isn't a pure function and actually mutates the desc handle to the offset value :(
	}

	return true;
}

bool Renderer::CreateCommandList()
{
	assert(m_pDevice != nullptr);

	// Need to create one allocator per frame buffer...
	for (int i = 0; i < m_frameBufferCount; i++)
	{
		BAIL_ON_FAIL(m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocators[i])),
			L"Failed to create CommandAllocator");
	}

	//but only one command list.
	BAIL_ON_FAIL(m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocators[m_frameIndex].Get(), nullptr /*TODO: PSO goes here. Using default for now*/, IID_PPV_ARGS(&m_pCommandList)),
		L"Failed to create CommandList");

	// CommandLists are created in the open state, need to close so that we can safely reopen in main loop
	m_pCommandList->Close();

	return true;
}

bool Renderer::CreateFences()
{
	assert(m_pDevice != nullptr);

	for (int i = 0; i < m_frameBufferCount; i++)
	{
		BAIL_ON_FAIL(m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFences[i])), L"Failed to create fence");
		m_fenceValues[i] = 0;
	}

	// single thread, so only need one fence event
	m_fenceEvent = CreateEvent(nullptr /*default*/,
		false, //automatically reset after waiting for the event
		false,
		L"FrameFence");

	MESSAGE_RETURN_FALSE(m_fenceEvent != NULL, L"Failed to create Fence Event");

	return true;
}

bool Renderer::PrepareShaderPreviewResources()
{
	assert(m_pDevice != nullptr);
	assert(m_pCommandList != nullptr);
	assert(m_pCommandQueue != nullptr);
	//TODO - refactor preview viewport rendering to another class

	// First create RootSig
	CD3DX12_ROOT_PARAMETER1 RP[1];
	RP[0].InitAsConstants(2, 0); // Time, deltaTime @ b0

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc(_countof(RP), RP, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	BAIL_ON_FAIL(D3D12SerializeVersionedRootSignature(&rootSigDesc, &signature, nullptr), L"Failed to serialize RootSignature");
	BAIL_ON_FAIL(m_pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSig)), L"Failed to create RootSignature");

	//Generate a PSO using the default pixelShader
	SILENT_RETURN_FALSE(UpdatePso());

	// Create VertexBuffer
	// verts in NDC
	Vertex verts[] = {
		{{ -1.0f, 1.0f, .5f}},
		{{ 1.0f, 1.0f, .5f}},
		{{ -1.0f, -1.0f, .5f}},
		{{ 1.0f, 1.0f, .5f}},
		{{ 1.0f, -1.0f, .5f}},
		{{ -1.0f, -1.0f, .5f}}
	};
	size_t vertsSize = sizeof(verts);

	//Create the default heap in the copy dest state so that we can push some data into it
	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferResDesc = CD3DX12_RESOURCE_DESC::Buffer(vertsSize);
	BAIL_ON_FAIL(m_pDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferResDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_pVertexBuffer)),
		L"Failed to create default heap for VertexBuffer");
	m_pVertexBuffer->SetName(L"VertexBuffer Resource Heap");

	//now make the upload heap
	ComPtr<ID3D12Resource> pUploadHeap;
	heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	BAIL_ON_FAIL(m_pDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pUploadHeap)),
		L"Failed to create VertexBuffer UploadHeap");
	pUploadHeap->SetName(L"VertexBuffer upload heap");

	// store verts in upload heap
	D3D12_SUBRESOURCE_DATA vertData = {};
	vertData.pData = reinterpret_cast<BYTE*>(verts);
	vertData.RowPitch = vertsSize;
	vertData.SlicePitch = vertsSize;

	//upload
	VerifyHR(m_pCommandList->Reset(m_pCommandAllocators[m_frameIndex].Get(), m_pPSO.Get()));
	UpdateSubresources(m_pCommandList.Get(), m_pVertexBuffer.Get(), pUploadHeap.Get(), 0, 0, 1, &vertData);
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	m_pCommandList->ResourceBarrier(1, &barrier);
	m_pCommandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(1, ppCommandLists);
	m_fenceValues[m_frameIndex]++; // increment fence value so that we can guarentee that the buffer is uploaded when we start drawing
	BAIL_ON_FAIL(m_pCommandQueue->Signal(m_pFences[m_frameIndex].Get(), m_fenceValues[m_frameIndex]), L"Failed to place fence signal on command queue");

	//Create vert buffer view for verts. 
	m_vertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertsSize;

	//Viewport
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = m_width;
	m_viewport.Height = m_height;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	//scissor rect
	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = m_width;
	m_scissorRect.bottom = m_height;

	//Wait for upload to complete before freeing upload heap
	WaitForPreviousFrame(false);

	return true;
}

bool Renderer::UpdatePso(IDxcBlob* pixelShader)
{
	assert(m_pDevice != nullptr);
	assert(m_pRootSig != nullptr);

	//Vertex Shader loaded from BuiltIn
	D3D12_SHADER_BYTECODE vsByteCode = {};
	vsByteCode.BytecodeLength = sizeof(g_VS_ScreenSpacePassthrough);
	vsByteCode.pShaderBytecode = g_VS_ScreenSpacePassthrough;

	// Pixel Shader provided by user or default shader
	D3D12_SHADER_BYTECODE psByteCode = {};
	if (pixelShader)
	{
		psByteCode.BytecodeLength = pixelShader->GetBufferSize();
		psByteCode.pShaderBytecode = pixelShader->GetBufferPointer();
	}
	else
	{
		psByteCode.BytecodeLength = sizeof(g_PS_Magenta);
		psByteCode.pShaderBytecode = g_PS_Magenta;
	}

	//Create Input Layout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = 1;
	inputLayoutDesc.pInputElementDescs = inputElementDescs;

	//Create PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.VS = vsByteCode;
	psoDesc.PS = psByteCode;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_rtvFormat;
	psoDesc.SampleDesc = { 1 /*count*/,0 /*quality*/ };
	psoDesc.SampleMask = 0xffffffff; // point sample
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	if (m_pPSO != nullptr)
	{
		m_pendingFreeList[m_frameIndex].push_back(ComPtr<IUnknown>(m_pPSO.Detach()));
	}

	BAIL_ON_FAIL(m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO)), L"Failed to create PSO");

	return true;
}

bool Renderer::TryUpdatePixelShader(IDxcBlob* shader)
{
	if (!UpdatePso(shader))
	{
		UpdatePso();
		return false;
	}
	return true;
}

void Renderer::UpdateResources()
{
	// make sure root sig is set
	m_pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	//Update resource data
	auto& resourceData = h_pShaderToyWindow->GetResourceManager().GetData();
	{
		auto lockedData = resourceData.GetLocked();		
		if (lockedData->GetDirtyBits().constants)
		{
			m_pCommandList->SetGraphicsRoot32BitConstants(ROOT_CONSTANT_INDEX, 2, lockedData->GetConstantsPtr(), 0);
		}
		lockedData->ClearDirtyBits();
	}
	

	//TODO: Draw the ShaderToy quad

	//TODO: UI elements? especially since we don't get GDI bc d3d12
}

void Renderer::Render()
{
	// get the new frame index from the swap chain
	m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	// Let the GPU finish doing work with this frame's commandAllocator before we reset it
	WaitForPreviousFrame();
	VerifyHR(m_pCommandAllocators[m_frameIndex]->Reset());
	// now we need to make sure that the commandList is reset and using the current allocator
	VerifyHR(m_pCommandList->Reset(m_pCommandAllocators[m_frameIndex].Get(), m_pPSO.Get()));

	UpdateResources();
	UpdatePipeline();

	// "Build" an array of command lists sorted in the order we want to render
	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(1, ppCommandLists);

	VerifyHR(m_pCommandQueue->Signal(m_pFences[m_frameIndex].Get(), m_fenceValues[m_frameIndex]));
	//Present current backbuffer
	VerifyHR(m_pSwapChain->Present(0, 0));
}

void Renderer::UpdatePipeline()
{
	

	// Transition our RTVs, and clear the new RTV

	// the "frameIndex" RTV needs to go from Present state to Render Target state so we can draw to it
	auto t1 = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pRenderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCommandList->ResourceBarrier(1, &t1);

	//Get our descriptor handle again so we can update the OM
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvDescHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescSize);

	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr /*TODO: depthStencil for doing UI things? Or never depthStencil and do painters alg?*/);


	const float clearColor[] = {0.0f, .2f, .8f, 1.0f};
	m_pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr); // Can clear a subsection of the RTV by passing rects here. Maybe usefull for only doing the preview in a rect and doing UI elsewhere.

	//After Clearing we can draw
	//m_pCommandList->SetPipelineState(m_pPSO.Get()); // don't need to do this since we set it as the initial PSO when resetting
	
	m_pCommandList->RSSetViewports(1, &m_viewport);
	m_pCommandList->RSSetScissorRects(1, &m_scissorRect);
	m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_pCommandList->DrawInstanced(6, 1, 0, 0);

	// Now transition back to present so we can show off the goods
	auto t2 = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pRenderTargets[m_frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	m_pCommandList->ResourceBarrier(1, &t2);

	VerifyHR(m_pCommandList->Close());
}



void Renderer::Cleanup()
{
	m_isInitialized = false;
	//Wait for all outstanding GPU work before shutdown
	for (int i = 0; i < m_frameBufferCount; i++)
	{
		m_frameIndex = i;
		WaitForPreviousFrame();
	}

	if (m_pSwapChain != nullptr)
	{
		//Exit fullscreen if needed before exit
		BOOL fs = false;
		VerifyHR(m_pSwapChain->GetFullscreenState(&fs, nullptr));
		if (fs)
		{
			VerifyHR(m_pSwapChain->SetFullscreenState(false, nullptr));
		}
	}
	
	// Reset ComPtrs
	m_pCommandList.Reset();
	for (int i = 0; i < m_frameBufferCount; i++)
	{
		m_pFences[i].Reset();
		m_pCommandAllocators[i].Reset();
		m_pRenderTargets[i].Detach(); //looks like these maybe shouldn't be ComPtrs? Reset here causes a crash when we try to free the SwapChain because it also has a reference to them that it tries to clear.
	}
	m_pRtvDescHeap.Reset();
	m_pCommandQueue.Reset();
	m_pSwapChain.Reset();
	m_pDevice.Reset();
}

void Renderer::WaitForPreviousFrame(bool incrementFenceValue)
{
	// Check the current fence value. If it is less than the expected value, the GPU is still executing work
	// on the command queue since it hasn't hit the signal() yet.
	if (m_pFences[m_frameIndex]->GetCompletedValue() < m_fenceValues[m_frameIndex])
	{
		// Tell the fence to notify us once the value is m_fenceValues[m_fenceIndex]
		VerifyHR(m_pFences[m_frameIndex]->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));

		// Idle until we get the signal to continue
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_pendingFreeList[m_frameIndex].clear();

	if (incrementFenceValue)
	{
		//increment the fence value for the next frame
		m_fenceValues[m_frameIndex]++;
	}
}

Renderer::~Renderer()
{
	Cleanup();
	CloseHandle(m_fenceEvent);
}