


#include <Windows.h> 
#include "hooks.h"   
#include <iostream>  
#include <string>    
#include <stdexcept> 



HMODULE g_hModule = nullptr;
bool g_ConsoleCreated = false;


void log_debug(const std::wstring& msg) {
    OutputDebugStringW((L"[ImGuiHook] " + msg + L"\n").c_str());
}



void create_debug_console() {
    if (AllocConsole()) {
        FILE* pFile = nullptr;

        if (freopen_s(&pFile, "CONOUT$", "w", stdout) == 0) {
            g_ConsoleCreated = true;
        }
        else {


            pFile = nullptr;
        }


        if (pFile && freopen_s(&pFile, "CONIN$", "r", stdin) != 0) {

        }
        std::cout << "debug console initialized lets go" << std::endl;
        log_debug(L"debug console created and stdio redirected (stdout success: " + std::wstring(g_ConsoleCreated ? L"true" : L"false") + L").");
    }
    else {
        log_debug(L"failed to create debug console or already exists.");
    }
}



DWORD WINAPI main_thread(LPVOID lpReserved) {


    log_debug(L"main_thread started. attempting to initialize hooks...");

    try {

        if (hooks::init_hooks()) {
            log_debug(L"hooks initialized successfully. press DELETE to unhook and eject.");



            while (!(GetAsyncKeyState(VK_DELETE) & 0x8000)) {
                Sleep(150);
            }
            log_debug(L"DELETE key pressed. initiating shutdown...");
        }
        else {
            log_debug(L"failed to initialize hooks. check previous logs for errors.");


            while (!(GetAsyncKeyState(VK_DELETE) & 0x8000)) {
                Sleep(150);
            }
            log_debug(L"DELETE key pressed after failed init. initiating shutdown...");
        }
    }
    catch (const std::exception& e) {
        std::string error_msg_str = "!!! unhandled C++ exception in main_thread: ";
        error_msg_str += e.what();

        std::wstring error_msg_wstr;
        for (char c : error_msg_str) { error_msg_wstr += static_cast<wchar_t>(c); }
        log_debug(error_msg_wstr);
    }
    catch (...) {
        log_debug(L"!!! unhandled unknown exception in main_thread. this is bad.");
    }


    log_debug(L"calling hooks::shutdown_hooks()...");
    hooks::shutdown_hooks();
    log_debug(L"hooks shutdown complete. ejecting dll. bye bye!");

    if (g_ConsoleCreated) {


        FreeConsole();
        log_debug(L"debug console freed.");
        g_ConsoleCreated = false;
    }



    HMODULE hMod = g_hModule;
    g_hModule = nullptr;
    FreeLibraryAndExitThread(hMod, 0);

}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);

        log_debug(L"DLL_PROCESS_ATTACH received. creating main_thread...");

        HANDLE hThread = CreateThread(nullptr, 0, main_thread, hModule, 0, nullptr);
        if (hThread) {
            CloseHandle(hThread);
            log_debug(L"main_thread created successfully.");
        }
        else {
            log_debug(L"!!! failed to create main_thread. DLL will not function.");
        }
    }
    break;
    case DLL_PROCESS_DETACH:
        log_debug(L"DLL_PROCESS_DETACH received.");
        if (g_hModule != nullptr) {
            log_debug(L"DLL_PROCESS_DETACH: g_hModule was not null. this might indicate an unclean shutdown if main_thread didn't complete FreeLibraryAndExitThread.");
        }


        if (g_ConsoleCreated) {
            FreeConsole();
            g_ConsoleCreated = false;
        }
        break;
    case DLL_THREAD_ATTACH:

        break;
    case DLL_THREAD_DETACH:

        break;
    }
    return TRUE;
}
