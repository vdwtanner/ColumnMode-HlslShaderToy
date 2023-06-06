#pragma once

namespace CM_HlslShaderToy
{
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
		
		bool TryGetHwnd(_Out_ HWND& pHwnd);

		void RenderLoop();

	public:
		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	protected:
		bool EnsureWindowCreated();
		void UpdateWindowTitle(std::wstring message = L"");
		void OnResize(UINT width, UINT height);

	private:
		Plugin* m_plugin;
		ATOM m_windowClassAtom;
		std::optional<HWND> m_hwnd;
		std::filesystem::path m_path;
		bool isNewFile = false;

		std::unique_ptr<Renderer> m_pRenderer;
		std::unique_ptr<ShaderCompiler> m_pShaderCompiler;

		std::thread* m_pRenderThread;
		std::mutex m_renderMutex;

		std::optional<ComPtr<IDxcBlob>> m_newPixelShader;
		std::mutex m_newShaderMutex;

		bool m_shutdownRequested = false;
	};
}