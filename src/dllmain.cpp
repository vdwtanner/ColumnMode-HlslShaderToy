#include "pch.h"

//include libs
//#pragma comment (lib, "d3d11.lib")
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