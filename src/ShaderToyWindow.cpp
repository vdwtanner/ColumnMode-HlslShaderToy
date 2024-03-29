#include "pch.h"
#include "StepTimer.h"

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

LRESULT CALLBACK WndProcD3D12(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    /*LONG_PTR ptr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (ptr)
    {
        ShaderToyWindow* pThis = reinterpret_cast<ShaderToyWindow*>(ptr);
        return pThis->WndProc(hWnd, message, wParam, lParam);
    }*/

    return DefWindowProc(hWnd, message, wParam, lParam);
}

ShaderToyWindow::ShaderToyWindow(Plugin* plugin) : m_plugin(plugin)
{
    m_path.clear();
    m_pRenderer = std::make_unique<Renderer>(this);
    m_pShaderCompiler = std::make_unique<ShaderCompiler>();
    m_pTimer = std::make_unique<DX::StepTimer>(); //throws
    //m_pTimer->SetFixedTimeStep(true);
    m_pResourceManager = std::make_unique<ResourceManager>();
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

    WNDCLASS windowClassd3d12{};
    windowClassd3d12.lpfnWndProc = ::WndProcD3D12;
    windowClassd3d12.lpszClassName = L"CM_HlslShaderToy_D3D12Surface";
    m_d3d12SurfaceAtom = (*m_plugin->m_callbacks.pfnRegisterWindowClass)(windowClassd3d12);

    if (m_d3d12SurfaceAtom == 0)
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
    ComPtr<IDxcBlobUtf8> pErrors = nullptr;
    if (m_pShaderCompiler->CompilePixelShaderFromFile(filepath, &shader, &pErrors))
    {
        std::lock_guard lock(m_newShaderMutex);
        m_newPixelShader.emplace(ComPtr<IDxcBlob>(shader));
        m_hWarningMessageTextArea->SetText(L"Compilation Successful.");
        return true;
    }
    else if (pErrors != nullptr)
    {
        m_hWarningMessageTextArea->SetText(from_utf8(pErrors->GetStringPointer()).c_str());
    }
    return false;
}

bool ShaderToyWindow::TryUpdatePixelShaderText(const size_t numChars, LPCWSTR shaderText)
{
    IDxcBlob* shader;
    ComPtr<ID3DBlob> errorBuffer;
    ComPtr<IDxcBlobUtf8> pErrors = nullptr;
    if (m_pShaderCompiler->CompilePixelShaderFromText(numChars, shaderText, m_path.c_str(), &shader, &pErrors))
    {
        std::lock_guard lock(m_newShaderMutex);
        m_newPixelShader.emplace(ComPtr<IDxcBlob>(shader));
        m_hWarningMessageTextArea->SetText(L"Compilation Successful.");
        return true;
    }
    else if (pErrors != nullptr)
    {   
        m_hWarningMessageTextArea->SetText(from_utf8(pErrors->GetStringPointer()).c_str());
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
        args.height = MIN_WINDOW_HEIGHT;
        args.width = MIN_WINDOW_WIDTH;
        m_windowDimensions.width = MIN_WINDOW_WIDTH;
        m_windowDimensions.height = MIN_WINDOW_HEIGHT;

        HWND hwnd = 0;
        if (SUCCEEDED((*m_plugin->m_callbacks.pfnOpenWindow)(args, &hwnd)))
        {
            m_hwnd.emplace(std::move(hwnd));
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);

            RECT rect;
            if (!GetWindowRect(hwnd, &rect))
            {
                MessageBoxHelper_FormattedBody(MB_ICONERROR | MB_OK, L"Error", L"Failed to fetch main window dimensions.");
                return false;
            }
            //now create a child hwnd to host the d3d12 rendering
            hwnd = CreateWindowEx(
                0,
                MAKEINTATOM(m_d3d12SurfaceAtom),
                L"D3D12 Surface",
                WS_CHILD | WS_VISIBLE | SS_OWNERDRAW | WS_BORDER,
                0, 0,       //x,y
                rect.right - rect.left, D3D12_SURFACE_HEIGHT,   //width, height
                m_hwnd.value(),
                m_childWindowIdFactory.GetNextId(),
                NULL,
                NULL);
            if (hwnd == NULL)
            {
                DWORD err = GetLastError();
                MessageBoxHelper_FormattedBody(MB_ICONERROR | MB_OK, L"Error", L"Failed to initialize D3D12 surface with error code: %d", err);
                return false;
            }
            m_d3d12SurfaceHwnd.emplace(std::move(hwnd));

            CreateResourceManagementUI();
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

bool ShaderToyWindow::ShaderToyWindow::TryGetD3D12Hwnd(_Out_ HWND& pHwnd)
{
    pHwnd = NULL;
    if (m_d3d12SurfaceHwnd.has_value())
    {
        pHwnd = m_d3d12SurfaceHwnd.value();
        return true;
    }
    return false;
}

void ShaderToyWindow::RenderLoop()
{
    while (!m_shutdownRequested)
    {
        m_pTimer->Tick([&](){
            // "update" step where we set ResourceData
            auto& data = m_pResourceManager->GetData();
            auto lockedData = data.GetLocked();
            lockedData->SetConstants(m_pTimer->GetTotalSeconds(), m_pTimer->GetElapsedSeconds());
        });



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
    m_windowDimensions.width = width;
    m_windowDimensions.height = height;

    //TODO - resize d3d rect

    auto rect = m_pVerticalLayout->GetRect();
    rect.right = width-5;
    rect.bottom = height;
    m_pVerticalLayout->Reposition(&rect);
}

void ShaderToyWindow::CreateResourceManagementUI()
{
    RECT rect{};
    rect.left = 5;
    rect.right = m_windowDimensions.width-15;
    rect.top = D3D12_SURFACE_HEIGHT + 10;
    rect.bottom = m_windowDimensions.height;

    m_pVerticalLayout = std::make_unique<UIVerticalLayout>(m_hwnd.value(), m_childWindowIdFactory.GetNextId(), rect);

    HWND vlHwnd = m_pVerticalLayout->GetHwnd();
    m_hWarningMessageTextArea = m_pVerticalLayout->AddElement(new UITextArea(vlHwnd, m_childWindowIdFactory.GetNextId()));
    UIDimension textAreaDim{};
    textAreaDim.width = 200;
    textAreaDim.height = 100;
    m_hWarningMessageTextArea->SetDesiredDimension(textAreaDim, LayoutFlags::FLEX_WIDTH);
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
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // All painting occurs here, between BeginPaint and EndPaint.
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_SIZE:
    {
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        OnResize(width, height);
        break;
    }
        
    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = MIN_WINDOW_WIDTH;
        lpMMI->ptMinTrackSize.y = MIN_WINDOW_HEIGHT;
        break;
    }
        
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}