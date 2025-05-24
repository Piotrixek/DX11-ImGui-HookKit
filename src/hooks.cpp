



#include "hooks.h"  
#include "menu.h"   


#include <d3d11.h> 
#include <dxgi.h>  
#include "imgui.h"
#include "imgui_impl_win32.h" 
#include "imgui_impl_dx11.h"  


#include "MinHook.h" 
#include <string>    
#include <vector>    
#include <algorithm> 


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


typedef HRESULT(APIENTRY* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(APIENTRY* ResizeBuffers_t)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);


HWND g_hWnd = nullptr;
WNDPROC g_oWndProc = nullptr;

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain_hook = nullptr;          
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

Present_t g_oPresent = nullptr;
ResizeBuffers_t g_oResizeBuffers = nullptr;

bool g_ShowMenu = true;
bool g_ImGuiInitialized = false;
bool g_HooksInitialized = false;


std::wstring char_to_wstring(const char* char_array) {
    if (!char_array) return L"";
    std::wstring result_str;
    int len = MultiByteToWideChar(CP_ACP, 0, char_array, -1, NULL, 0);
    if (len > 0 && len - 1 > 0) {
        result_str.resize(static_cast<size_t>(len - 1));
        MultiByteToWideChar(CP_ACP, 0, char_array, -1, &result_str[0], len);
    }
    return result_str;
}

void log_hook_debug(const std::wstring& msg) {
    OutputDebugStringW((L"[ImGuiHook-Hooks] " + msg + L"\n").c_str());
}


void create_render_target() {
    if (!g_pSwapChain_hook || !g_pd3dDevice) return;

    log_hook_debug(L"create_render_target: attempting to get back buffer and create rtv.");
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = g_pSwapChain_hook->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (SUCCEEDED(hr) && pBackBuffer) {
        hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
        if (SUCCEEDED(hr)) {
            log_hook_debug(L"create_render_target: rtv created successfully.");
        }
        else {
            log_hook_debug(L"create_render_target: failed to create rtv. hr=" + std::to_wstring(hr));
        }
        pBackBuffer->Release();
    }
    else {
        log_hook_debug(L"create_render_target: failed to get back buffer. hr=" + std::to_wstring(hr) + (pBackBuffer ? L"" : L" (pBackBuffer was null)"));
    }
}

void cleanup_render_target() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
        log_hook_debug(L"cleanup_render_target: rtv released.");
    }
}

LRESULT CALLBACK hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN && wParam == VK_INSERT) {
        g_ShowMenu = !g_ShowMenu;
        log_hook_debug(g_ShowMenu ? L"hkWndProc: menu toggled ON." : L"hkWndProc: menu toggled OFF.");

        if (g_ImGuiInitialized) {
            ImGuiIO& io = ImGui::GetIO();
            io.MouseDrawCursor = g_ShowMenu;
        }
    }

    if (g_ShowMenu && g_ImGuiInitialized && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
        return true;
    }

    if (g_oWndProc) {
        return CallWindowProc(g_oWndProc, hWnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HRESULT APIENTRY hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    
    if (!g_ImGuiInitialized) {
        log_hook_debug(L"hkPresent: first call imgui not initialized. attempting init...");
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice)) && g_pd3dDevice) {
            g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);
            if (!g_pd3dDeviceContext) {
                log_hook_debug(L"hkPresent: !!! Failed to get device context.");
                g_pd3dDevice->Release(); g_pd3dDevice = nullptr;
                return g_oPresent ? g_oPresent(pSwapChain, SyncInterval, Flags) : E_FAIL;
            }

            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            g_hWnd = sd.OutputWindow;
            g_pSwapChain_hook = pSwapChain; 
            log_hook_debug(L"hkPresent: Main swapchain identified and stored.");

            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            io.IniFilename = "MyCoolHackImGuiLayout.ini";

            ImGui::StyleColorsDark();

            ImGuiStyle& style = ImGui::GetStyle();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                style.WindowRounding = 0.0f;
                style.Colors[ImGuiCol_WindowBg].w = 1.0f;
            }

            if (!ImGui_ImplWin32_Init(g_hWnd)) {
                log_hook_debug(L"hkPresent: !!! ImGui_ImplWin32_Init FAILED.");
                
                return g_oPresent ? g_oPresent(pSwapChain, SyncInterval, Flags) : E_FAIL;
            }
            log_hook_debug(L"hkPresent: ImGui_ImplWin32_Init success.");

            if (!ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext)) {
                log_hook_debug(L"hkPresent: !!! ImGui_ImplDX11_Init FAILED.");
                
                return g_oPresent ? g_oPresent(pSwapChain, SyncInterval, Flags) : E_FAIL;
            }
            log_hook_debug(L"hkPresent: ImGui_ImplDX11_Init success.");

            g_oWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
            if (!g_oWndProc) {
                log_hook_debug(L"hkPresent: !!! WndProc hooking FAILED.");
            }
            else {
                log_hook_debug(L"hkPresent: WndProc hooked successfully.");
            }

            g_ImGuiInitialized = true;
            log_hook_debug(L"hkPresent: ImGui fully initialized.");
            create_render_target();
        }
        else {
            log_hook_debug(L"hkPresent: !!! failed to get D3D11Device from swapchain. ImGui init aborted.");
            if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
            return g_oPresent ? g_oPresent(pSwapChain, SyncInterval, Flags) : E_FAIL;
        }
    }

    
    
    
    
    if (g_ImGuiInitialized && pSwapChain == g_pSwapChain_hook) {

        if (!g_mainRenderTargetView) {
            log_hook_debug(L"hkPresent: RTV null for main swapchain, trying to recreate...");
            create_render_target();
        }

        
        if (g_mainRenderTargetView && g_pd3dDeviceContext) {

            
            ID3D11RenderTargetView* game_rtv_backup = nullptr;
            ID3D11DepthStencilView* game_dsv_backup = nullptr;
            g_pd3dDeviceContext->OMGetRenderTargets(1, &game_rtv_backup, &game_dsv_backup);

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            if (g_ShowMenu) {
                menu::render_menu();
            }

            ImGui::Render();

            
            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, game_dsv_backup); 
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

            ImGuiIO& io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();

                
                
                
                g_pd3dDeviceContext->OMSetRenderTargets(1, &game_rtv_backup, game_dsv_backup);

                
                DXGI_SWAP_CHAIN_DESC sd;
                g_pSwapChain_hook->GetDesc(&sd);
                D3D11_VIEWPORT vp;
                vp.Width = (FLOAT)sd.BufferDesc.Width; vp.Height = (FLOAT)sd.BufferDesc.Height;
                vp.MinDepth = 0.0f; vp.MaxDepth = 1.0f; vp.TopLeftX = 0; vp.TopLeftY = 0;
                g_pd3dDeviceContext->RSSetViewports(1, &vp);
            }

            
            if (game_rtv_backup) game_rtv_backup->Release();
            if (game_dsv_backup) game_dsv_backup->Release();
        }

    }
    else if (g_ImGuiInitialized) {
        
        
        
    }
    

    
    return g_oPresent ? g_oPresent(pSwapChain, SyncInterval, Flags) : E_FAIL;
}


HRESULT APIENTRY hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    
    if (g_pSwapChain_hook == pSwapChain) {
        log_hook_debug(L"hkResizeBuffers called for main swapchain. width: " + std::to_wstring(Width) + L" height: " + std::to_wstring(Height));
        cleanup_render_target();
        if (g_ImGuiInitialized) {
            ImGui_ImplDX11_InvalidateDeviceObjects();
            log_hook_debug(L"hkResizeBuffers: ImGui_ImplDX11_InvalidateDeviceObjects called.");
        }
    }
    else {
        log_hook_debug(L"hkResizeBuffers called for non-main swapchain. Passing through.");
    }

    HRESULT hr = g_oResizeBuffers ?
        g_oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags) :
        E_FAIL;

    if (!SUCCEEDED(hr)) {
        log_hook_debug(L"hkResizeBuffers: !!! original ResizeBuffers call FAILED. hr=" + std::to_wstring(hr));
    }

    return hr;
}



namespace hooks {
    void* get_vtable_func(void* pClassInstance, UINT uiFunctionIndex) {
        if (!pClassInstance) return nullptr;
        void*** pVTable = reinterpret_cast<void***>(pClassInstance);
        if (!pVTable || !*pVTable) return nullptr;
        return (*pVTable)[uiFunctionIndex];
    }

    bool init_hooks() {
        
        log_hook_debug(L"init_hooks: initializing MinHook...");
        MH_STATUS mhStatus = MH_Initialize();
        if (mhStatus != MH_OK) {
            log_hook_debug(L"!!! MinHook MH_Initialize failed: " + char_to_wstring(MH_StatusToString(mhStatus)));
            return false;
        }
        log_hook_debug(L"MinHook initialized successfully.");

        log_hook_debug(L"init_hooks: creating dummy D3D11 device to get VTable addresses...");

        HWND tempHwnd = nullptr;
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, DefWindowProcW, 0L, 0L, GetModuleHandleW(NULL), NULL, NULL, NULL, NULL, L"DummyHookWindowForImGuiUnique", NULL };

        ATOM classAtom = RegisterClassExW(&wc);
        if (!classAtom && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            log_hook_debug(L"!!! Failed to register dummy window class. GetLastError: " + std::to_wstring(GetLastError()));
            MH_Uninitialize();
            return false;
        }

        tempHwnd = CreateWindowW(wc.lpszClassName, L"Dummy DirectX Window", WS_OVERLAPPEDWINDOW, 0, 0, 1, 1, NULL, NULL, wc.hInstance, NULL);

        if (!tempHwnd) {
            log_hook_debug(L"!!! Failed to create temporary window for D3D11 init. GetLastError: " + std::to_wstring(GetLastError()));
            MH_Uninitialize();
            if (classAtom) UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return false;
        }

        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = tempHwnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        ID3D11Device* pDummyDevice = nullptr;
        ID3D11DeviceContext* pDummyContext = nullptr;
        IDXGISwapChain* pDummySwapChain = nullptr;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION, &sd, &pDummySwapChain, &pDummyDevice, NULL, &pDummyContext
        );

        void* pPresentAddr = nullptr;
        void* pResizeBuffersAddr = nullptr;

        if (SUCCEEDED(hr) && pDummySwapChain && pDummyDevice && pDummyContext) {
            log_hook_debug(L"Dummy D3D11 device and swapchain created successfully.");
            pPresentAddr = get_vtable_func(pDummySwapChain, 8);
            pResizeBuffersAddr = get_vtable_func(pDummySwapChain, 13);

            if (pPresentAddr) log_hook_debug(L"Present VTable address obtained."); else log_hook_debug(L"!!! Failed to get Present VTable address.");
            if (pResizeBuffersAddr) log_hook_debug(L"ResizeBuffers VTable address obtained."); else log_hook_debug(L"!!! Failed to get ResizeBuffers VTable address.");

            pDummySwapChain->Release();
            pDummyContext->Release();
            pDummyDevice->Release();
        }
        else {
            log_hook_debug(L"!!! Failed to create dummy D3D11 device and swapchain. hr=" + std::to_wstring(hr));
            if (pDummySwapChain) pDummySwapChain->Release();
            if (pDummyContext) pDummyContext->Release();
            if (pDummyDevice) pDummyDevice->Release();
            DestroyWindow(tempHwnd);
            if (classAtom) UnregisterClassW(wc.lpszClassName, wc.hInstance);
            MH_Uninitialize();
            return false;
        }
        DestroyWindow(tempHwnd);
        if (classAtom) UnregisterClassW(wc.lpszClassName, wc.hInstance);
        log_hook_debug(L"Dummy D3D11 resources and window cleaned up.");

        if (!pPresentAddr) {
            log_hook_debug(L"!!! Cannot hook Present: address not found.");
            MH_Uninitialize();
            return false;
        }

        log_hook_debug(L"init_hooks: creating hook for Present...");
        mhStatus = MH_CreateHook(pPresentAddr, &hkPresent, reinterpret_cast<LPVOID*>(&g_oPresent));
        if (mhStatus != MH_OK) {
            log_hook_debug(L"!!! MH_CreateHook for Present failed: " + char_to_wstring(MH_StatusToString(mhStatus)));
            MH_Uninitialize();
            return false;
        }

        if (pResizeBuffersAddr) {
            log_hook_debug(L"init_hooks: creating hook for ResizeBuffers...");
            mhStatus = MH_CreateHook(pResizeBuffersAddr, &hkResizeBuffers, reinterpret_cast<LPVOID*>(&g_oResizeBuffers));
            if (mhStatus != MH_OK) {
                log_hook_debug(L"!!! MH_CreateHook for ResizeBuffers failed: " + char_to_wstring(MH_StatusToString(mhStatus)) + L". Continuing without it but resizing might be unstable.");
            }
        }
        else {
            log_hook_debug(L"init_hooks: ResizeBuffers address not found skipping hook. Resizing might be unstable.");
        }

        log_hook_debug(L"init_hooks: enabling hooks...");
        mhStatus = MH_EnableHook(MH_ALL_HOOKS);
        if (mhStatus != MH_OK) {
            log_hook_debug(L"!!! MH_EnableHook(MH_ALL_HOOKS) failed: " + char_to_wstring(MH_StatusToString(mhStatus)));
            MH_RemoveHook(MH_ALL_HOOKS);
            MH_Uninitialize();
            return false;
        }

        log_hook_debug(L"MinHook hooks created and enabled successfully.");
        g_HooksInitialized = true;
        return true;
    }

    void shutdown_hooks() {
        
        log_hook_debug(L"shutdown_hooks called.");
        if (!g_HooksInitialized && !g_ImGuiInitialized) {
            log_hook_debug(L"shutdown_hooks: nothing to shutdown (hooks or imgui not initialized).");
            if (MH_Uninitialize() == MH_OK) {
                log_hook_debug(L"shutdown_hooks: MinHook uninitialized (or was already).");
            }
            return;
        }

        log_hook_debug(L"shutdown_hooks: disabling and removing MinHook hooks...");
        MH_STATUS mhStatus = MH_DisableHook(MH_ALL_HOOKS);
        if (mhStatus != MH_OK && mhStatus != MH_ERROR_NOT_INITIALIZED && mhStatus != MH_ERROR_DISABLED) {
            log_hook_debug(L"!!! MH_DisableHook(MH_ALL_HOOKS) failed: " + char_to_wstring(MH_StatusToString(mhStatus)));
        }
        mhStatus = MH_RemoveHook(MH_ALL_HOOKS);
        if (mhStatus != MH_OK && mhStatus != MH_ERROR_NOT_INITIALIZED && mhStatus != MH_ERROR_NOT_CREATED) {
            log_hook_debug(L"!!! MH_RemoveHook(MH_ALL_HOOKS) failed: " + char_to_wstring(MH_StatusToString(mhStatus)));
        }

        mhStatus = MH_Uninitialize();
        if (mhStatus == MH_OK) {
            log_hook_debug(L"MinHook uninitialized successfully.");
        }
        else if (mhStatus != MH_ERROR_NOT_INITIALIZED) {
            log_hook_debug(L"!!! MH_Uninitialize failed: " + char_to_wstring(MH_StatusToString(mhStatus)));
        }

        if (g_oWndProc && g_hWnd && IsWindow(g_hWnd)) {
            log_hook_debug(L"shutdown_hooks: restoring original WndProc...");
            SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)g_oWndProc);
            g_oWndProc = nullptr;
            log_hook_debug(L"Original WndProc restored.");
        }
        else {
            log_hook_debug(L"shutdown_hooks: skipping WndProc restore (g_oWndProc is null or g_hWnd is invalid/null).");
        }

        if (g_ImGuiInitialized) {
            log_hook_debug(L"shutdown_hooks: shutting down ImGui...");
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            g_ImGuiInitialized = false;
            log_hook_debug(L"ImGui shutdown complete.");
        }
        else {
            log_hook_debug(L"shutdown_hooks: ImGui was not initialized skipping its shutdown.");
        }

        cleanup_render_target();

        if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
        if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

        g_pSwapChain_hook = nullptr;
        g_hWnd = nullptr;

        g_HooksInitialized = false;
        log_hook_debug(L"shutdown_hooks finished.");
    }
} 

