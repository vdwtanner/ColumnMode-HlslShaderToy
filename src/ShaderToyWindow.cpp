#include "pch.h"

using namespace CM_HlslShaderToy;

//Forwards WndProc messages to ShaderToyWindow::WndProc for handling
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LONG_PTR ptr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (ptr)
    {
        ShaderToyWindow* pThis = reinterpret_cast<ShaderToyWindow*>(ptr);
        return pThis->WndProc(hWnd, message, wParam, lParam);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

ShaderToyWindow::ShaderToyWindow(Plugin* plugin) : m_plugin(plugin)
{
    m_path.clear();
    m_pRenderer = std::make_unique<Renderer>(this);
    m_pShaderCompiler = std::make_unique<ShaderCompiler>();
}

ShaderToyWindow::~ShaderToyWindow()
{
    m_shutdownRequested = true;
    if (m_pRenderThread != nullptr)
    {
        m_pRenderThread->join();
    }
    if (m_hwnd.has_value())
    {
        CloseWindow(m_hwnd.value());
    }
    UnregisterClass(MAKEINTATOM(m_windowClassAtom), NULL);
}

HRESULT ShaderToyWindow::Init()
{
    WNDCLASS windowClass{};
    windowClass.lpfnWndProc = ::WndProc;
    windowClass.lpszClassName = L"CM_HlslShaderToy";
    m_windowClassAtom = (*m_plugin->m_callbacks.pfnRegisterWindowClass)(windowClass);

    if (m_windowClassAtom == 0)
    {
        return E_FAIL;
    }

    return S_OK;
}

void ShaderToyWindow::SetActiveFilepath(std::filesystem::path path)
{
    m_path.assign(path);
    isNewFile = true;
}

bool ShaderToyWindow::TryUpdatePixelShaderFile(LPCWSTR filepath)
{
    IDxcBlob* shader;
    ComPtr<ID3DBlob> errorBuffer;
    if (m_pShaderCompiler->CompilePixelShader(filepath, &shader))
    {
        std::lock_guard lock(m_newShaderMutex);
        m_newPixelShader.emplace(ComPtr<IDxcBlob>(shader));
        return true;
    }
    return false;
}

bool ShaderToyWindow::Show()
{
    SILENT_RETURN_FALSE(EnsureWindowCreated());
    UpdateWindowTitle();
    if (!IsWindowVisible(m_hwnd.value()))
    {
        ShowWindowAsync(m_hwnd.value(), 1);
    }
    return true;
}

bool ShaderToyWindow::EnsureWindowCreated()
{
    if (!m_hwnd.has_value())
    {
        ColumnMode::CreateWindowArgs args{};
        args.exWindowStyle = 0;
        args.windowClass = m_windowClassAtom;
        args.windowName = L"HLSL Shader Toy";
        args.height = 500;
        args.width = 500;

        HWND hwnd = 0;
        if (SUCCEEDED((*m_plugin->m_callbacks.pfnOpenWindow)(args, &hwnd)))
        {
            m_hwnd.emplace(std::move(hwnd));
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
        }

        if (!m_pRenderer->Init())
        {
            MessageBox(0, L"Failed to initialize direct3d 12",
                L"Error", MB_OK);
            m_pRenderer->Cleanup();
            return false;
        }

        //Spin up a render thread
        m_pRenderThread = new std::thread([](ShaderToyWindow& stw) { stw.RenderLoop(); }, std::ref(*this));
    }
    return true;
}

bool ShaderToyWindow::ShaderToyWindow::TryGetHwnd(_Out_ HWND& pHwnd)
{
    pHwnd = NULL;
    if (m_hwnd.has_value())
    {
        pHwnd = m_hwnd.value();
        return true;
    }
    return false;
}

void ShaderToyWindow::RenderLoop()
{
    while (!m_shutdownRequested)
    {
        std::unique_lock shaderUpdateLock(m_newShaderMutex, std::defer_lock);
        if (shaderUpdateLock.try_lock())
        {
            if (m_newPixelShader.has_value())
            {
                if (!m_pRenderer->TryUpdatePixelShader(m_newPixelShader->Get()))
                {
                    MessageBox(NULL, L"Error setting the new pixel shader :(", L"ERROR", MB_OK);
                }

                //now wipe it out and free the memory again
                m_newPixelShader.reset();
            }
            shaderUpdateLock.unlock();
        }
        {
            // Take the mutex so there are no interuptions while we render this frame
            std::lock_guard<std::mutex> renderLock(m_renderMutex);
            m_pRenderer->Render();
        }   
    }
}

void ShaderToyWindow::UpdateWindowTitle(std::wstring message)
{
    if (m_hwnd.has_value() && !m_path.empty())
    {
        std::wstring windowTitle = PLUGIN_NAME;
        windowTitle.append(L" - ")
            .append(m_path.filename());
        if (message.size() > 0) {
            windowTitle.append(L" | ")
                .append(message);
        }
        SetWindowText(m_hwnd.value(), windowTitle.c_str());
    }
}

void ShaderToyWindow::OnResize(UINT width, UINT height)
{

}

LRESULT CALLBACK ShaderToyWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!m_hwnd.has_value())
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_DESTROY:
        if (!m_shutdownRequested)
        {
            m_shutdownRequested = true;
            if (m_pRenderThread != nullptr)
            {
                m_pRenderThread->join();
                delete(m_pRenderThread);
                m_pRenderThread = nullptr;
            }
            m_hwnd.reset();
        }
        
        break;
    
    case WM_PAINT:
        break;
    case WM_SIZE:
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        OnResize(width, height);
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}