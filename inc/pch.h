#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define UNICODE
// Windows Header Files
#include <windows.h>
#include <windowsx.h>
#include <wrl/client.h>
using namespace ::Microsoft::WRL;

//c runtime headers
#include <filesystem>


//DirectX


#include "ColumnModePluginAPI.h"

#include "ShaderToyWindow.h"
#include "Plugin.h"

constexpr LPCWSTR PLUGIN_NAME = L"HLSL Shader Toy";

inline void VerifyHR(HRESULT hr)
{
	if (FAILED(hr))
		__debugbreak();
}

inline void VerifyBool(bool b)
{
	if (!b)
		__debugbreak();
}