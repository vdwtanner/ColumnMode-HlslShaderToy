#pragma once

namespace DX
{
	class StepTimer;
}

namespace CM_HlslShaderToy
{
	class ChildWindowIdFactory
	{
	private:
		const UINT m_firstChildId;
		UINT m_nextId;

	public:
		ChildWindowIdFactory(UINT startingId = 0x8): m_firstChildId(startingId), m_nextId(startingId) {}
		HMENU GetNextId() { if (m_nextId > 0xDFFF) { throw(L"Invalid child ID: Greater than 0xDFFF"); } return (HMENU)m_nextId++; }
		HMENU GetFirstChildId() { return (HMENU)m_firstChildId; }
	};

	class Plugin;
	class ShaderToyWindow
	{
	public:
		ShaderToyWindow(Plugin* plugin);
		~ShaderToyWindow();
		HRESULT Init();
		void SetActiveFilepath(std::filesystem::path path);
		bool TryUpdatePixelShaderFile(LPCWSTR filepath);
		bool TryUpdatePixelShaderText(const size_t numChars, LPCWSTR shaderText);
		bool Show();
		ResourceManager& GetResourceManager() { return *m_pResourceManager; }
		
		bool TryGetHwnd(_Out_ HWND& pHwnd);
		bool TryGetD3D12Hwnd(_Out_ HWND& pHwnd);

		void RenderLoop();

	public:
		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	protected:
		bool EnsureWindowCreated();
		void UpdateWindowTitle(std::wstring message = L"");
		void OnResize(UINT width, UINT height);

		void CreateResourceManagementUI();

	private: //variables
		Plugin* m_plugin;
		ATOM m_windowClassAtom;
		ATOM m_d3d12SurfaceAtom;
		std::optional<HWND> m_hwnd;
		std::optional<HWND> m_d3d12SurfaceHwnd;
		std::filesystem::path m_path;
		bool isNewFile = false;

		std::unique_ptr<Renderer> m_pRenderer;
		std::unique_ptr<ShaderCompiler> m_pShaderCompiler;

		std::thread* m_pRenderThread;
		std::mutex m_renderMutex;

		std::optional<ComPtr<IDxcBlob>> m_newPixelShader;
		std::mutex m_newShaderMutex;

		bool m_shutdownRequested = false;
		std::unique_ptr<DX::StepTimer> m_pTimer;
		std::unique_ptr<ResourceManager> m_pResourceManager;
		
		struct WindowDimension
		{
			UINT width;
			UINT height;
		};
		WindowDimension m_windowDimensions;
		ChildWindowIdFactory m_childWindowIdFactory;

		std::unique_ptr<UIVerticalLayout> m_pVerticalLayout;//valid after CreateResourceManagementUI and until the window is destroyed
		UITextArea* m_hWarningMessageTextArea; //valid after CreateResourceManagementUI and until the window is destroyed

	private: //consts
		static const UINT MIN_WINDOW_WIDTH = 500;
		static const UINT MIN_WINDOW_HEIGHT = 700;
		static const UINT D3D12_SURFACE_HEIGHT = 400;
	};
}