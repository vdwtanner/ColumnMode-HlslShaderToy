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
#include <mutex>

//DirectX
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <dxcapi.h>
#include "directx/d3dx12.h"

#include "util.h"
#include "ColumnModePluginAPI.h"
#include "ShaderCompiler.h"

#include "ResourceManager.h"
#include "Renderer.h"
#include "UIElements.h"
#include "UILayouts.h"
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

inline std::string to_utf8(const std::wstring& wide_string)
{
    if (wide_string.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_string[0], (int)wide_string.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wide_string[0], (int)wide_string.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// decoding function
inline std::wstring from_utf8(const std::string& utf8_string)
{
    if (utf8_string.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8_string[0], (int)utf8_string.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &utf8_string[0], (int)utf8_string.size(), &wstrTo[0], size_needed);
    return wstrTo;
}