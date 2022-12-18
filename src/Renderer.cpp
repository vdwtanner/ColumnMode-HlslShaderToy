#include "pch.h"

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
	MESSAGE_RETURN_FALSE(h_pShaderToyWindow->TryGetHwnd(hwnd), L"Can't create a swapchain without a valid HWND");

	//DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = m_frameBufferCount;
	swapChainDesc.Width = m_width; // Can pass 0 to use the output window dimensions and later fetch via IDXGISwapChain1::GetDesc1
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

	UINT rtvDescSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
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

		rtvDescHandle.Offset(1, rtvDescSize); //Offset isn't a pure function and actually mutates the desc handle to the offset value :(
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
	}

	// single thread, so only need one fence event
	m_fenceEvent = CreateEvent(nullptr /*default*/,
		false, //automatically reset after waiting for the event
		false,
		L"FrameFence");

	MESSAGE_RETURN_FALSE(m_fenceEvent != NULL, L"Failed to create Fence Event");

	return true;
}

void Renderer::Update()
{
}

void Renderer::UpdatePipeline()
{
}

void Renderer::Render()
{
}

void Renderer::Cleanup()
{
}

void Renderer::WaitForPreviousFrame()
{
}

Renderer::~Renderer()
{
	WaitForPreviousFrame();
	CloseHandle(m_fenceEvent);
}