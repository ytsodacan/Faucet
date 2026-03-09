#include "stdafx.h"
#include "ModLoader.h"
#include "ServerLevel.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <windows.h>
#include <thread>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

static HWND g_SplashWnd = NULL;
static HWND g_StatusWnd = NULL;
static HWND g_CountWnd = NULL;
static HWND g_FailWnd = NULL;
static int g_TotalModsFound = 0;
static int g_ModsLoadedCount = 0;
static int g_ModsFailedCount = 0;

static ULONG_PTR g_gdiplusToken;

LRESULT CALLBACK SplashWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
    } return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CreateSplash() {

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = SplashWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ModLoaderSplash";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    int w = 600;
    int h = 400;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    g_SplashWnd = CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName, L"Faucet",
        WS_POPUP | WS_BORDER | WS_VISIBLE, x, y, w, h, NULL, NULL, wc.hInstance, NULL);

    CreateWindowW(L"STATIC", L"FAUCET", WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 20, w, 40, g_SplashWnd, NULL, wc.hInstance, NULL);

    HWND hLogo = CreateWindowW(L"STATIC", NULL,
        WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
        100, 70, 400, 150,
        g_SplashWnd, NULL, wc.hInstance, NULL);

    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring exePath = buffer;
    std::wstring exeDir = exePath.substr(0, exePath.find_last_of(L"\\/"));
    std::wstring fullPath = exeDir + L"\\Common\\Media\\Faucet.png";

    DWORD dwAttrib = GetFileAttributesW(fullPath.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
        std::wstring errorMsg = L"Image not found at:\n" + fullPath;
        MessageBoxW(g_SplashWnd, errorMsg.c_str(), L"File Error", MB_ICONERROR);
    }

    Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(fullPath.c_str());
    if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok) {
        HBITMAP hBmp = NULL;
        Gdiplus::Status status = bitmap->GetHBITMAP(Gdiplus::Color(255, 255, 255, 255), &hBmp);

        if (status == Gdiplus::Ok && hBmp != NULL) {
            SendMessage(hLogo, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
        }
        delete bitmap;
    }

    g_StatusWnd = CreateWindowW(L"STATIC", L"Preparing...", WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 240, 580, 40, g_SplashWnd, NULL, wc.hInstance, NULL);

    g_CountWnd = CreateWindowW(L"STATIC", L"0/0 MODS LOADED", WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 350, 200, 20, g_SplashWnd, NULL, wc.hInstance, NULL);

    g_FailWnd = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
        380, 350, 200, 20, g_SplashWnd, NULL, wc.hInstance, NULL);

    UpdateWindow(g_SplashWnd);
}

void UpdateSplashStatus(const std::wstring& text) {
    if (g_StatusWnd) {
        SendMessageW(g_StatusWnd, WM_SETTEXT, 0, (LPARAM)text.c_str());

        std::wstring countText = std::to_wstring(g_ModsLoadedCount) + L"/" + std::to_wstring(g_TotalModsFound) + L" MODS LOADED";
        SendMessageW(g_CountWnd, WM_SETTEXT, 0, (LPARAM)countText.c_str());

        if (g_ModsFailedCount > 0) {
            std::wstring failText = std::to_wstring(g_ModsFailedCount) + L" HAVE FAILED TO LOAD";
            SendMessageW(g_FailWnd, WM_SETTEXT, 0, (LPARAM)failText.c_str());
        }

        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

ModLoader& ModLoader::Get() {
    static ModLoader instance;
    return instance;
}

void ModLoader::Initialize() {
    if (m_initialized) return;
    m_initialized = true;

    auto startTime = std::chrono::steady_clock::now();
    CreateSplash();

    OpenLogFile();
    Log(L"Faucet ModLoader initializing");

    std::wstring modsDir = GetModsDirectory();
    ScanAndLoadMods(modsDir);

    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    if (elapsed < 1000) {
        UpdateSplashStatus(L"Finalizing...");
        Sleep(static_cast<DWORD>(1000 - elapsed));
    }

    if (g_SplashWnd) {
        DestroyWindow(g_SplashWnd);
        UnregisterClassW(L"ModLoaderSplash", GetModuleHandle(NULL));

        Gdiplus::GdiplusShutdown(g_gdiplusToken);
    }
}

void ModLoader::NotifyInit() {
    for (auto& mod : m_mods) {
        if (!mod.healthy) continue;
        const char* id = mod.instance->GetInfo()->id;
        try {
            if (!mod.instance->OnInit()) {
                mod.healthy = false;
            }
        }
        catch (...) {
            mod.healthy = false;
        }
    }
}

void ModLoader::OnLevelLoad() {
    Log(L"Level has loaded");
    for (auto& mod : m_mods) {
        if (!mod.healthy) continue;
        try {
            mod.instance->OnLevelLoad();
        }
        catch (...) {
            mod.healthy = false;
        }
    }
}

void ModLoader::OnLevelUnload() {
    Log(L"Level has unloaded");
    for (auto& mod : m_mods) {
        if (!mod.healthy) continue;
        try {
            mod.instance->OnLevelUnload();
        }
        catch (...) {
            mod.healthy = false;
        }
    }
}

void ModLoader::NotifyUpdate(float deltaTime) {
    for (auto& mod : m_mods) {
        if (!mod.healthy) continue;
        try {
            if (!mod.instance->OnUpdate(deltaTime)) {
                mod.healthy = false;
            }
        }
        catch (...) {
            mod.healthy = false;
        }
    }
}

void ModLoader::Shutdown() {
    for (int i = static_cast<int>(m_mods.size()) - 1; i >= 0; --i) {
        UnloadOneMod(m_mods[i]);
    }
    m_mods.clear();
    if (m_logFile != INVALID_HANDLE_VALUE) {
        CloseHandle(m_logFile);
        m_logFile = INVALID_HANDLE_VALUE;
    }
}

IMod* ModLoader::FindMod(const std::string& id) const {
    for (const auto& mod : m_mods) {
        if (mod.healthy && mod.instance &&
            std::string(mod.instance->GetInfo()->id) == id) {
            return mod.instance;
        }
    }
    return nullptr;
}

void ModLoader::ScanAndLoadMods(const std::wstring& modsDir) {
    DWORD dwAttrib = GetFileAttributesW(modsDir.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES || !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
        CreateDirectoryW(modsDir.c_str(), NULL);
        return;
    }

    std::wstring searchPath = modsDir + L"\\*.dll";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::wstring fullPath = modsDir + L"\\" + fd.cFileName;
                UpdateSplashStatus(L"Loading: " + std::wstring(fd.cFileName));
                LoadOneMod(fullPath);
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
}

bool ModLoader::LoadOneMod(const std::wstring& dllPath) {
    HMODULE hMod = LoadLibraryW(dllPath.c_str());
    if (!hMod) return false;

    using CreateModFn = IMod * (*)();
    auto createFn = reinterpret_cast<CreateModFn>(GetProcAddress(hMod, "CreateMod"));

    if (!createFn) {
        FreeLibrary(hMod);
        return false;
    }

    IMod* instance = nullptr;
    try {
        instance = createFn();
    }
    catch (...) {
        FreeLibrary(hMod);
        return false;
    }

    if (!instance) {
        FreeLibrary(hMod);
        return false;
    }

    LoadedMod record;
    record.module = hMod;
    record.instance = instance;
    record.path = std::string(dllPath.begin(), dllPath.end());

    try {
        if (!instance->OnLoad()) {
            record.healthy = false;
        }
    }
    catch (...) {
        record.healthy = false;
    }

    m_mods.push_back(std::move(record));
    return true;
}

void ModLoader::UnloadOneMod(LoadedMod& mod) {
    if (!mod.instance) return;
    delete mod.instance;
    mod.instance = nullptr;
    if (mod.module) {
        FreeLibrary(mod.module);
        mod.module = nullptr;
    }
}

void ModLoader::OpenLogFile() {
    std::wstring dir = GetModsDirectory();
    std::wstring logPath = dir + L"\\modloader.log";
    CreateDirectoryW(dir.c_str(), NULL);
    m_logFile = CreateFileW(logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
}

void ModLoader::Log(const std::wstring& msg) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    struct tm buf;
    localtime_s(&buf, &t);
    char ts[16];
    std::strftime(ts, sizeof(ts), "[%H:%M:%S] ", &buf);
    std::wstring line = std::wstring(ts, ts + strlen(ts)) + msg + L"\r\n";
    if (m_logFile != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(m_logFile, line.c_str(), static_cast<DWORD>(line.size() * sizeof(wchar_t)), &written, nullptr);
    }
    OutputDebugStringW(line.c_str());
}

void ModLoader::Log(const std::string& msg) {
    Log(std::wstring(msg.begin(), msg.end()));
}

std::wstring ModLoader::GetModsDirectory() const {
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring dir = exePath;
    dir = dir.substr(0, dir.find_last_of(L"\\/"));
    return dir + L"\\mods";
}