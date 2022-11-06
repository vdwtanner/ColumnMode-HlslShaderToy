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

	public:
		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	protected:
		void EnsureWindowCreated();
		void UpdateWindowTitle();

	private:
		Plugin* m_plugin;
		ATOM m_windowClassAtom;
		std::optional<HWND> m_hwnd;
		std::filesystem::path m_path;
		bool isNewFile = false;
	};
}