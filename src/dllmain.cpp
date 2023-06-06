#include "pch.h"

//include libs
#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxcompiler.lib")
#pragma comment (lib, "dxguid.lib")
//#pragma comment (lib, "d2d1.lib")
//#pragma comment (lib, "windowscodecs.lib")

using namespace CM_HlslShaderToy;

HRESULT APIENTRY OnShutdown(HANDLE handle)
{
    Plugin* p = reinterpret_cast<Plugin*>(handle);
    delete(p);
    return S_OK;
}

HRESULT WINAPI OpenColumnModePlugin(_Inout_ ColumnMode::OpenPluginArgs* args)
{
    Plugin* p = new Plugin(args);
    args->pPluginFuncs->pfnOnShutdown = OnShutdown;
    return S_OK;
}

HRESULT WINAPI QueryColumnModePluginDependencies(_Inout_ UINT* pCount, _Inout_opt_count_(*pCount) ColumnMode::PluginDependency* pDependencies)
{
    constexpr WCHAR dxilDll[] = L"dxil.dll";
    constexpr UINT dxilDllLength = _countof(dxilDll);
    if (pDependencies == nullptr)
    {
        // First call sets the numder of runtime dependencies
        *pCount = 1;
        return S_OK;
    }
    else if (*pCount != 1)
    {
        return E_INVALIDARG;
    }

    // now we know pDependencies is valid and pCount == 1
    if (pDependencies[0].pName == nullptr)
    {
        // second call sets the size of the dependency name
        pDependencies[0].length = dxilDllLength;
        return S_OK;
    }
    else if (pDependencies[0].length != dxilDllLength)
    {
        return E_INVALIDARG;
    }

    // now we know that pDependencies[0].pName is valid and the correct size

    if (memcpy_s(pDependencies[0].pName, pDependencies[0].length * sizeof(WCHAR), dxilDll, dxilDllLength * sizeof(WCHAR)) != 0)
    {
        return E_FAIL;
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