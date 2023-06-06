#include "pch.h"

using namespace CM_HlslShaderToy;

Plugin::Plugin(ColumnMode::OpenPluginArgs* args)
{

	memcpy(&m_callbacks, args->pCallbacks, sizeof(ColumnMode::OpenPluginArgs));
	args->hPlugin = this;
	args->pPluginFuncs->pfnOnOpen = OnOpen;
	args->pPluginFuncs->pfnOnSave = OnSave;
	args->pPluginFuncs->pfnOnSaveAs = OnSaveAs;
	args->pPluginFuncs->pfnOnLoadCompleted = OnLoadCompleted;

	if (args->apiVersion >= 2)
	{
		args->pPluginFuncs->pfnOnTypingComplete = OnTypingCompleted;
	}

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
	Plugin* pThis = reinterpret_cast<Plugin*>(handle);
	if (pThis->currentFileIsHlsl)
	{
		pThis->m_pShaderToyWindow->TryUpdatePixelShaderFile(filepath);
	}
	return S_OK;
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

HRESULT CM_HlslShaderToy::Plugin::OnTypingCompleted(HANDLE handle, const size_t numChars, const WCHAR* pAllText)
{
	Plugin* pThis = reinterpret_cast<Plugin*>(handle);
	if (pThis->currentFileIsHlsl)
	{
		pThis->m_pShaderToyWindow->TryUpdatePixelShaderText(numChars, pAllText);
	}
	return E_NOTIMPL;
}

bool Plugin::CheckIfFileValidForPlugin(LPCWSTR filepath)
{
	std::filesystem::path path;
	path.assign(filepath);

	auto ext = path.extension();
	LPCWSTR validExtensions[] = { L".hlsl" };
	bool valid = false;
	for (auto extension : validExtensions)
	{
		if (ext.compare(extension) == 0)
		{
			valid = true;
			break;
		}
	}
	return valid;
}

HRESULT Plugin::HandleFileChange(LPCWSTR filepath)
{
	std::filesystem::path path;
	path.assign(filepath);

	currentFileIsHlsl = CheckIfFileValidForPlugin(filepath);
	if (!currentFileIsHlsl)
	{
		return S_FALSE;
	}
	m_callbacks.pfnRecommendEditMode(this, ColumnMode::EDIT_MODE::TextMode);
	int response = MessageBox(NULL, L"Would you like to preview this file?", PLUGIN_NAME, MB_YESNO | MB_ICONQUESTION);

	if (response == IDYES)
	{
		m_pShaderToyWindow->SetActiveFilepath(path);
		if (!m_pShaderToyWindow->Show()) 
		{ 
			return E_FAIL;
		}
		else
		{
			m_pShaderToyWindow->TryUpdatePixelShaderFile(filepath);
		}
	}
	return S_OK;
}