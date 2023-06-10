#include "pch.h"

//include libs
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxcompiler.lib")
#pragma comment (lib, "dxguid.lib")
//#pragma comment (lib, "d2d1.lib")
//#pragma comment (lib, "windowscodecs.lib")

using namespace CM_HlslShaderToy;
HMODULE hRichEdit = NULL;

HRESULT APIENTRY OnShutdown(HANDLE handle)
{
    Plugin* p = reinterpret_cast<Plugin*>(handle);
    delete(p);
    return S_OK;
}

HRESULT WINAPI OpenColumnModePlugin(_Inout_ ColumnMode::OpenPluginArgs* args)
{
    if (hRichEdit == NULL)
    {
        hRichEdit = LoadLibrary(L"Msftedit.dll"); //Not next to plugin dll, so can't use the other approach
        assert(hRichEdit != NULL);
    }
    Plugin* p = new Plugin(args);
    args->pPluginFuncs->pfnOnShutdown = OnShutdown;
    return S_OK;
}

HRESULT WINAPI QueryColumnModePluginDependencies(_Inout_ UINT* pCount, _Inout_opt_count_(*pCount) ColumnMode::PluginDependency* pDependencies)
{
    std::wstring deps[] = { L"dxil.dll" };

    if (pDependencies == nullptr)
    {
        // First call sets the numder of runtime dependencies
        *pCount = _countof(deps);
        return S_OK;
    }
    else if (*pCount != _countof(deps))
    {
        return E_INVALIDARG;
    }

    // now we know pDependencies is valid and pCount == 1
    bool returnSOK = false;
    for (int i = 0; i < _countof(deps); i++)
    {
        if (pDependencies[i].pName == nullptr)
        {
            // second call sets the size of the dependency name

            pDependencies[i].length = deps[i].length();
            returnSOK = true;
        }
        else if (pDependencies[i].length != deps[i].length())
        {
            return E_INVALIDARG;
        }

        
    }
    if (returnSOK)
    {
        return S_OK;
    }

    for (int i = 0; i < _countof(deps); i++)
    {
        // now we know that pDependencies[i].pName is valid and the correct size
        if (memcpy_s(pDependencies[i].pName, pDependencies[i].length * sizeof(WCHAR), deps[i].c_str(), deps[i].length() * sizeof(WCHAR)) != 0)
        {
            return E_FAIL;
        }
    }
    

    return S_OK;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}