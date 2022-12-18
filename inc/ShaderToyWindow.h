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
		std::thread* m_pRenderThread;
		std::mutex m_renderMutex;

		bool m_shutdownRequested = false;
	};
}