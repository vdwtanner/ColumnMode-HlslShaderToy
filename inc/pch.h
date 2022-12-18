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

//STL
#include <array>


//DirectX
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "directx/d3dx12.h"

#include "ColumnModePluginAPI.h"
#include "Renderer.h"
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

// if result == false, display an error message and then return false. Intended to provide info for why we bailed from a function
#define MESSAGE_RETURN_FALSE(result, msg) if(!result) { MessageBox(0, msg, L"Error", MB_OK); return false; }
//if(FAILED(hr)) { /*display msg then return false*/ }
#define BAIL_ON_FAIL(hr, msg) MESSAGE_RETURN_FALSE(SUCCEEDED(hr), msg)
#define SILENT_RETURN_FALSE(result) if(!result) {return false;}