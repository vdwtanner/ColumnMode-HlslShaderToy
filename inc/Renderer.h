#pragma once

namespace CM_HlslShaderToy 
{
	struct Vertex
	{
		DirectX::XMFLOAT3 pos;
	};

	class ShaderToyWindow;
	class Renderer 
	{
	public:
		Renderer(_In_ ShaderToyWindow* pShaderToyWindow);
		bool Init();
		
		
		void Render();
		void Cleanup();
		void WaitForPreviousFrame(bool incrementFenceValue = true);

		bool IsInitialized() { return m_isInitialized; }

		bool TryUpdatePixelShader(IDxcBlob* shader);

		~Renderer();

	private:
		//init
		bool CreateDevice();
		bool CreateSwapChain();
		bool CreateRTVs();
		bool CreateCommandList();
		bool CreateFences();
		bool PrepareShaderPreviewResources();

		//Render
		void UpdateResources();
		void UpdatePipeline();

		bool UpdatePso(IDxcBlob* pixelShader = nullptr);

	private: // Owned members
		static const int m_frameBufferCount = 3;
		static const UINT ROOT_CONSTANT_INDEX = 0;
		const DXGI_FORMAT m_rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		ComPtr<ID3D12Device> m_pDevice;
		ComPtr<ID3D12CommandQueue> m_pCommandQueue;
		ComPtr<IDXGISwapChain3> m_pSwapChain;
		ComPtr<ID3D12DescriptorHeap> m_pRtvDescHeap;
		std::array<ComPtr<ID3D12Resource>, m_frameBufferCount> m_pRenderTargets;
		std::array<ComPtr<ID3D12CommandAllocator>, m_frameBufferCount> m_pCommandAllocators; // num allocators = numBuffers * numThreads (we only have one thread)
		ComPtr<ID3D12GraphicsCommandList> m_pCommandList; // only one thread, so only need one commandList
		std::array<ComPtr<ID3D12Fence>, m_frameBufferCount> m_pFences;
		HANDLE m_fenceEvent;
		std::array<UINT64, m_frameBufferCount> m_fenceValues; // incremented each frame, each fence will have its own value
		UINT m_frameIndex; // current RTV we're on
		UINT m_rtvDescSize; //size of RTV desc on the device (all front and back buffers will be the same size)
		UINT m_dxgiFactoryFlags = 0;

		UINT m_width = 800;
		UINT m_height = 600;

		bool m_isInitialized = false;

		ComPtr<ID3D12PipelineState> m_pPSO;
		ComPtr<ID3D12RootSignature> m_pRootSig;
		D3D12_VIEWPORT m_viewport;
		D3D12_RECT m_scissorRect;
		ComPtr<ID3D12Resource> m_pVertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

		// temporary way of checking that render thread is working correctly
		bool increasing = true;
		float blue = 0.0f;

		std::vector<ComPtr<IUnknown>> m_pendingFreeList[m_frameBufferCount];

	private: // handles to pointers owned elsewhere
		ShaderToyWindow* h_pShaderToyWindow;
	};
}