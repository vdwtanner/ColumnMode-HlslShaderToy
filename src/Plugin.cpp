#include "pch.h"

using namespace CM_HlslShaderToy;

Plugin::Plugin(ColumnMode::OpenPluginArgs* args)
{

	memcpy(&m_callbacks, args->pCallbacks, sizeof(ColumnMode::OpenPluginArgs));
	args->hPlugin = this;
	args->pPluginFuncs->pfnOnOpen = OnOpen;
	//args->pPluginFuncs->pfnOnSave = OnSave;
	args->pPluginFuncs->pfnOnSaveAs = OnSaveAs;
	args->pPluginFuncs->pfnOnLoadCompleted = OnLoadCompleted;

	m_pShaderToyWindow = new ShaderToyWindow(this);
}

Plugin::~Plugin()
{
	delete(m_pShaderToyWindow);
}

HRESULT Plugin::Init()
{
	return m_pShaderToyWindow->Init();
}

HRESULT Plugin::OnOpen(HANDLE handle, LPCWSTR filepath)
{
	Plugin* pThis = reinterpret_cast<Plugin*>(handle);
	return pThis->HandleFileChange(filepath);
}

HRESULT Plugin::OnSave(HANDLE handle, LPCWSTR filepath)
{
	return E_NOTIMPL;
}

HRESULT Plugin::OnSaveAs(HANDLE handle, LPCWSTR filepath)
{
	Plugin* pThis = reinterpret_cast<Plugin*>(handle);
	return pThis->HandleFileChange(filepath);
}

HRESULT Plugin::OnLoadCompleted(HANDLE handle)
{
	Plugin* pThis = reinterpret_cast<Plugin*>(handle);
	return pThis->Init();
}

HRESULT Plugin::HandleFileChange(LPCWSTR filepath)
{
	std::filesystem::path path;
	path.assign(filepath);

	auto ext = path.extension();
	LPCWSTR validExtensions[] = { L".hlsl"};
	bool valid = false;
	for (auto extension : validExtensions)
	{
		if (ext.compare(extension) == 0)
		{
			valid = true;
			break;
		}
	}
	if (!valid)
	{
		return S_FALSE;
	}
	m_callbacks.pfnRecommendEditMode(this, ColumnMode::EDIT_MODE::TextMode);
	int response = MessageBox(NULL, L"Would you like to preview this file?", PLUGIN_NAME, MB_YESNO | MB_ICONQUESTION);

	if (response == IDYES)
	{
		m_pShaderToyWindow->SetActiveFilepath(path);
		if (!m_pShaderToyWindow->Show()) { return E_FAIL; }
	}
	return S_OK;
}