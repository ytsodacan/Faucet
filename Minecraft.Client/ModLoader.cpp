#include "stdafx.h"
#include "ModLoader.h"
#include "ServerLevel.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <vector>
#include <windows.h>
#include <thread>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

static HWND g_SplashWnd = NULL;
static HWND g_StatusWnd = NULL;
static HWND g_CountWnd = NULL;
static HWND g_FailWnd = NULL;
static int  g_TotalMods = 0;
static int  g_LoadedMods = 0;
static int  g_FailedMods = 0;

static ULONG_PTR g_gdiplusToken;

LRESULT CALLBACK SplashWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CreateSplash() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = SplashWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ModLoaderSplash";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    const int w = 600, h = 400;
    const int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    g_SplashWnd = CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName, L"Faucet",
        WS_POPUP | WS_BORDER | WS_VISIBLE, x, y, w, h, NULL, NULL, wc.hInstance, NULL);

    CreateWindowW(L"STATIC", L"FAUCET",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 20, w, 40, g_SplashWnd, NULL, wc.hInstance, NULL);

    HWND hLogo = CreateWindowW(L"STATIC", NULL,
        WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
        100, 70, 400, 150, g_SplashWnd, NULL, wc.hInstance, NULL);

    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring exeDir = std::wstring(buffer);
    exeDir = exeDir.substr(0, exeDir.find_last_of(L"\\/"));
    std::wstring imagePath = exeDir + L"\\Common\\Media\\Faucet.png";

    Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(imagePath.c_str());
    if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok) {
        HBITMAP hBmp = NULL;
        if (bitmap->GetHBITMAP(Gdiplus::Color(255, 255, 255, 255), &hBmp) == Gdiplus::Ok && hBmp) {
            HBITMAP hOld = (HBITMAP)SendMessage(hLogo, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
            if (hOld) DeleteObject(hOld);
        }
    }
    delete bitmap;

    g_StatusWnd = CreateWindowW(L"STATIC", L"Preparing...",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 240, 580, 40, g_SplashWnd, NULL, wc.hInstance, NULL);

    g_CountWnd = CreateWindowW(L"STATIC", L"0/0 MODS LOADED",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 350, 200, 20, g_SplashWnd, NULL, wc.hInstance, NULL);

    g_FailWnd = CreateWindowW(L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        380, 350, 200, 20, g_SplashWnd, NULL, wc.hInstance, NULL);

    UpdateWindow(g_SplashWnd);
}

void UpdateSplashStatus(const std::wstring& text) {
    if (!g_StatusWnd) return;

    SendMessageW(g_StatusWnd, WM_SETTEXT, 0, (LPARAM)text.c_str());

    std::wstring countText = std::to_wstring(g_LoadedMods) + L"/"
        + std::to_wstring(g_TotalMods) + L" MODS LOADED";
    SendMessageW(g_CountWnd, WM_SETTEXT, 0, (LPARAM)countText.c_str());

    if (g_FailedMods > 0) {
        std::wstring failText = std::to_wstring(g_FailedMods) + L" FAILED TO LOAD";
        SendMessageW(g_FailWnd, WM_SETTEXT, 0, (LPARAM)failText.c_str());
    }

    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
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

    Log("=== Faucet ModLoader starting ===");

    std::wstring modsDir = GetModsDirectory();
    ScanAndLoadMods(modsDir);

    auto endTime = std::chrono::steady_clock::now();
    long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    Log("Initialization complete — " + std::to_string(g_LoadedMods) + " loaded, "
        + std::to_string(g_FailedMods) + " failed, "
        + std::to_string(elapsed) + "ms total");

    if (elapsed < 1000) {
        UpdateSplashStatus(L"Finalizing...");
        Sleep(static_cast<DWORD>(1000 - elapsed));
    }

    if (g_SplashWnd) {
        DestroyWindow(g_SplashWnd);
        g_SplashWnd = NULL;
        UnregisterClassW(L"ModLoaderSplash", GetModuleHandle(NULL));
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
    }
}

void ModLoader::NotifyInit() {
    for (auto& mod : m_mods) {
        if (!mod.healthy) continue;
        try {
            if (!mod.instance->OnInit()) {
                Log("OnInit() returned false for: " + std::string(mod.instance->GetInfo()->id));
                mod.healthy = false;
            }
        }
        catch (...) {
            Log("OnInit() threw for: " + std::string(mod.instance->GetInfo()->id));
            mod.healthy = false;
        }
    }
}

void ModLoader::OnLevelLoad() {
    Log("Level loaded");
    for (auto& mod : m_mods) {
        if (!mod.healthy) continue;
        try {
            mod.instance->OnLevelLoad();
        }
        catch (...) {
            Log("OnLevelLoad() threw for: " + std::string(mod.instance->GetInfo()->id));
            mod.healthy = false;
        }
    }
}

void ModLoader::OnLevelUnload() {
    Log("Level unloaded");
    for (auto& mod : m_mods) {
        if (!mod.healthy) continue;
        try {
            mod.instance->OnLevelUnload();
        }
        catch (...) {
            Log("OnLevelUnload() threw for: " + std::string(mod.instance->GetInfo()->id));
            mod.healthy = false;
        }
    }
}

void ModLoader::NotifyUpdate(float deltaTime) {
    for (auto& mod : m_mods) {
        if (!mod.healthy) continue;
        if (!mod.instance) {
            mod.healthy = false;
            continue;
        }
        try {
            if (!mod.instance->OnUpdate(deltaTime)) {
                Log("OnUpdate() returned false for: " + std::string(mod.instance->GetInfo()->id));
                mod.healthy = false;
            }
        }
        catch (...) {
            Log("OnUpdate() threw for: " + std::string(mod.instance->GetInfo()->id));
            mod.healthy = false;
        }
    }
}

void ModLoader::Shutdown() {
    Log("Shutting down ModLoader");
    for (int i = static_cast<int>(m_mods.size()) - 1; i >= 0; --i)
        UnloadOneMod(m_mods[i]);
    m_mods.clear();

    if (m_logFile != INVALID_HANDLE_VALUE) {
        CloseHandle(m_logFile);
        m_logFile = INVALID_HANDLE_VALUE;
    }
}

IMod* ModLoader::FindMod(const std::string& id) const {
    for (const auto& mod : m_mods) {
        if (mod.healthy && mod.instance &&
            std::string(mod.instance->GetInfo()->id) == id)
            return mod.instance;
    }
    return nullptr;
}

void ModLoader::ScanAndLoadMods(const std::wstring& modsDir) {
    DWORD dwAttrib = GetFileAttributesW(modsDir.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES || !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
        CreateDirectoryW(modsDir.c_str(), NULL);
        Log("Mods directory not found — created it");
        return;
    }

    std::vector<std::wstring> dllPaths;
    std::wstring searchPath = modsDir + L"\\*.dll";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                dllPaths.push_back(modsDir + L"\\" + fd.cFileName);
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }

    g_TotalMods = static_cast<int>(dllPaths.size());
    Log("Found " + std::to_string(g_TotalMods) + " mod(s) to load");

    for (const auto& path : dllPaths) {
        std::wstring filename = path.substr(path.find_last_of(L"\\/") + 1);
        UpdateSplashStatus(L"Loading: " + filename);
        LoadOneMod(path);
    }
}

bool ModLoader::LoadOneMod(const std::wstring& dllPath) {
    std::string narrowPath(dllPath.begin(), dllPath.end());
    std::string filename = narrowPath.substr(narrowPath.find_last_of("\\/") + 1);

    auto t0 = std::chrono::steady_clock::now();

    HMODULE hMod = LoadLibraryW(dllPath.c_str());
    if (!hMod) {
        Log("FAIL [" + filename + "] LoadLibrary failed (error " + std::to_string(GetLastError()) + ")");
        ++g_FailedMods;
        return false;
    }

    using CreateModFn = IMod * (*)();
    auto createFn = reinterpret_cast<CreateModFn>(GetProcAddress(hMod, "CreateMod"));
    if (!createFn) {
        Log("FAIL [" + filename + "] missing CreateMod export");
        FreeLibrary(hMod);
        ++g_FailedMods;
        return false;
    }

    IMod* instance = nullptr;
    try {
        instance = createFn();
    }
    catch (...) {
        Log("FAIL [" + filename + "] CreateMod() threw");
        FreeLibrary(hMod);
        ++g_FailedMods;
        return false;
    }

    if (!instance) {
        Log("FAIL [" + filename + "] CreateMod() returned null");
        FreeLibrary(hMod);
        ++g_FailedMods;
        return false;
    }

    const ModInfo* info = instance->GetInfo();
    std::string modId = info ? info->id : "(unknown)";
    std::string modVer = info
        ? std::to_string(info->version.major) + "."
        + std::to_string(info->version.minor) + "."
        + std::to_string(info->version.patch)
        : "(unknown)";

    LoadedMod record;
    record.module = hMod;
    record.instance = instance;
    record.path = narrowPath;

    try {
        if (!instance->OnLoad()) {
            Log("WARN [" + modId + " " + modVer + "] OnLoad() returned false — marking unhealthy");
            record.healthy = false;
        }
    }
    catch (...) {
        Log("FAIL [" + modId + " " + modVer + "] OnLoad() threw — marking unhealthy");
        record.healthy = false;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    if (record.healthy) {
        Log("OK   [" + modId + " " + modVer + "] loaded in " + std::to_string(elapsed) + "ms");
        ++g_LoadedMods;
    }
    else {
        ++g_FailedMods;
    }

    m_mods.push_back(std::move(record));
    return record.healthy;
}

void ModLoader::UnloadOneMod(LoadedMod& mod) {
    if (!mod.instance) return;

    if (mod.instance->GetInfo())
        Log("Unloading: " + std::string(mod.instance->GetInfo()->id));

    delete mod.instance;
    mod.instance = nullptr;

    if (mod.module) {
        FreeLibrary(mod.module);
        mod.module = nullptr;
    }
}

void ModLoader::OpenLogFile() {
    std::wstring logPath = GetModsDirectory() + L"\\modloader.log";
    CreateDirectoryW(GetModsDirectory().c_str(), NULL);
    m_logFile = CreateFileW(logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);


}

void ModLoader::Log(const std::string& msg) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    struct tm buf;
    localtime_s(&buf, &t);
    char ts[16];
    std::strftime(ts, sizeof(ts), "[%H:%M:%S] ", &buf);

    std::string line = std::string(ts) + msg + "\r\n";

    if (m_logFile != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(m_logFile, line.c_str(), static_cast<DWORD>(line.size()), &written, nullptr);
    }

    OutputDebugStringA(line.c_str());
}

void ModLoader::Log(const std::wstring& msg) {
    Log(std::string(msg.begin(), msg.end()));
}

std::wstring ModLoader::GetModsDirectory() const {
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring dir = exePath;
    dir = dir.substr(0, dir.find_last_of(L"\\/"));
    return dir + L"\\mods";
}