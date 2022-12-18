#pragma once

namespace CM_HlslShaderToy 
{
	class ShaderToyWindow;
	class Renderer 
	{
	public:
		Renderer(_In_ ShaderToyWindow* pShaderToyWindow);
		bool Init();
		void Update();
		void UpdatePipeline();
		void Render();
		void Cleanup();
		void WaitForPreviousFrame();

		~Renderer();

	private:
		bool CreateDevice();
		bool CreateSwapChain();
		bool CreateRTVs();
		bool CreateCommandList();
		bool CreateFences();

	private: // Owned members
		static const int m_frameBufferCount = 3;

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

	private: // handles to pointers owned elsewhere
		ShaderToyWindow* h_pShaderToyWindow;
	};
}