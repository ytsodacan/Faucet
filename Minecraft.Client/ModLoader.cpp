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
#include "Registry.h"
#include "IdRegistry.h"
#include "GameBridge.h"

#pragma comment(lib, "gdiplus.lib")

// ---------------------------------------------------------------------------
// Splash state
// ---------------------------------------------------------------------------
static HWND         g_SplashWnd = NULL;
static HBITMAP      g_hLogoBmp = NULL;
static int          g_LogoW = 0;
static int          g_LogoH = 0;
static int          g_TotalMods = 0;
static int          g_LoadedMods = 0;
static int          g_FailedMods = 0;
static std::wstring g_StatusText = L"Preparing...";
static ULONG_PTR    g_gdiplusToken;

static HFONT g_hFontTitle = NULL;
static HFONT g_hFontStatus = NULL;
static HFONT g_hFontCount = NULL;

static const int SW = 600;
static const int SH = 400;

#define SC_BG     RGB(24,  24,  24)
#define SC_WHITE  RGB(255, 255, 255)
#define SC_GREY   RGB(160, 160, 160)
#define SC_RED    RGB(220,  60,  60)
#define SC_ACCENT RGB(80,  140, 220)

// ---------------------------------------------------------------------------
// Splash paint
// ---------------------------------------------------------------------------
static void PaintSplash(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    HDC     mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, SW, SH);
    HBITMAP obmp = (HBITMAP)SelectObject(mem, bmp);

    RECT rc = { 0, 0, SW, SH };
    HBRUSH bgBr = CreateSolidBrush(SC_BG);
    FillRect(mem, &rc, bgBr);
    DeleteObject(bgBr);

    SetBkMode(mem, TRANSPARENT);

    HFONT of = (HFONT)SelectObject(mem, g_hFontTitle);
    SetTextColor(mem, SC_WHITE);
    RECT titleR = { 0, 18, SW, 58 };
    DrawTextW(mem, L"FAUCET", -1, &titleR, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    HPEN linePen = CreatePen(PS_SOLID, 1, SC_ACCENT);
    HPEN oldPen = (HPEN)SelectObject(mem, linePen);
    MoveToEx(mem, 40, 62, NULL);
    LineTo(mem, SW - 40, 62);
    SelectObject(mem, oldPen);
    DeleteObject(linePen);

    if (g_hLogoBmp)
    {
        HDC logoDC = CreateCompatibleDC(mem);
        HBITMAP old = (HBITMAP)SelectObject(logoDC, g_hLogoBmp);
        int lx = (SW - g_LogoW) / 2;
        int ly = 74;
        BitBlt(mem, lx, ly, g_LogoW, g_LogoH, logoDC, 0, 0, SRCCOPY);
        SelectObject(logoDC, old);
        DeleteDC(logoDC);
    }

    SelectObject(mem, g_hFontStatus);
    SetTextColor(mem, SC_GREY);
    RECT statR = { 20, 240, SW - 20, 282 };
    DrawTextW(mem, g_StatusText.c_str(), -1, &statR, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    int barX = 20, barY = 292, barW = SW - 40, barH = 6;
    HBRUSH trackBr = CreateSolidBrush(RGB(50, 50, 50));
    RECT trackR = { barX, barY, barX + barW, barY + barH };
    FillRect(mem, &trackR, trackBr);
    DeleteObject(trackBr);

    if (g_TotalMods > 0)
    {
        int fillW = (int)((float)g_LoadedMods / g_TotalMods * barW);
        HBRUSH fillBr = CreateSolidBrush(SC_ACCENT);
        RECT fillR = { barX, barY, barX + fillW, barY + barH };
        FillRect(mem, &fillR, fillBr);
        DeleteObject(fillBr);
    }

    SelectObject(mem, g_hFontCount);

    std::wstring countText = std::to_wstring(g_LoadedMods) + L"/" + std::to_wstring(g_TotalMods) + L" MODS LOADED";
    SetTextColor(mem, SC_GREY);
    RECT countR = { barX, barY + 14, barX + barW / 2, barY + 34 };
    DrawTextW(mem, countText.c_str(), -1, &countR, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    if (g_FailedMods > 0)
    {
        std::wstring failText = std::to_wstring(g_FailedMods) + L" FAILED";
        SetTextColor(mem, SC_RED);
        RECT failR = { barX + barW / 2, barY + 14, barX + barW, barY + 34 };
        DrawTextW(mem, failText.c_str(), -1, &failR, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
    }

    SelectObject(mem, of);

    BitBlt(hdc, 0, 0, SW, SH, mem, 0, 0, SRCCOPY);
    SelectObject(mem, obmp);
    DeleteObject(bmp);
    DeleteDC(mem);

    EndPaint(hwnd, &ps);
}

// ---------------------------------------------------------------------------
// Splash window proc
// ---------------------------------------------------------------------------
LRESULT CALLBACK SplashWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_PAINT) { PaintSplash(hwnd); return 0; }
    if (msg == WM_ERASEBKGND) { return 1; }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Splash creation
// ---------------------------------------------------------------------------
void CreateSplash()
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);

    g_hFontTitle = CreateFontW(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_hFontStatus = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_hFontCount = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    WNDCLASSW wc = {};
    wc.lpfnWndProc = SplashWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ModLoaderSplash";
    wc.hbrBackground = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    const int x = (GetSystemMetrics(SM_CXSCREEN) - SW) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - SH) / 2;

    g_SplashWnd = CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName, L"Faucet",
        WS_POPUP | WS_BORDER | WS_VISIBLE, x, y, SW, SH, NULL, NULL, wc.hInstance, NULL);

    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring exeDir = std::wstring(buffer);
    exeDir = exeDir.substr(0, exeDir.find_last_of(L"\\/"));
    std::wstring imagePath = exeDir + L"\\Common\\Media\\Faucet.png";

    Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(imagePath.c_str());
    if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok)
    {
        g_LogoW = (int)bitmap->GetWidth();
        g_LogoH = (int)bitmap->GetHeight();

        const int maxW = 400, maxH = 150;
        if (g_LogoW > maxW || g_LogoH > maxH)
        {
            float scale = min((float)maxW / g_LogoW, (float)maxH / g_LogoH);
            g_LogoW = (int)(g_LogoW * scale);
            g_LogoH = (int)(g_LogoH * scale);
        }

        Gdiplus::Bitmap* scaled = new Gdiplus::Bitmap(g_LogoW, g_LogoH, PixelFormat32bppARGB);
        Gdiplus::Graphics g(scaled);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.DrawImage(bitmap, 0, 0, g_LogoW, g_LogoH);

        HDC hdc = GetDC(g_SplashWnd);
        HDC memDC = CreateCompatibleDC(hdc);
        g_hLogoBmp = CreateCompatibleBitmap(hdc, g_LogoW, g_LogoH);
        HBITMAP old = (HBITMAP)SelectObject(memDC, g_hLogoBmp);
        Gdiplus::Graphics gdc(memDC);
        gdc.DrawImage(scaled, 0, 0, g_LogoW, g_LogoH);
        SelectObject(memDC, old);
        DeleteDC(memDC);
        ReleaseDC(g_SplashWnd, hdc);

        delete scaled;
    }
    delete bitmap;

    UpdateWindow(g_SplashWnd);
}

// ---------------------------------------------------------------------------
// Splash update
// ---------------------------------------------------------------------------
void UpdateSplashStatus(const std::wstring& text)
{
    if (!g_SplashWnd) return;

    g_StatusText = text;

    InvalidateRect(g_SplashWnd, NULL, FALSE);
    UpdateWindow(g_SplashWnd);

    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// ---------------------------------------------------------------------------
// ModLoader
// ---------------------------------------------------------------------------
ModLoader& ModLoader::Get()
{
    static ModLoader instance;
    return instance;
}

void ModLoader::Initialize()
{
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

    if (elapsed < 1000)
    {
        UpdateSplashStatus(L"Finalizing...");
        Sleep(static_cast<DWORD>(1000 - elapsed));
    }

    if (g_SplashWnd)
    {
        DestroyWindow(g_SplashWnd);
        g_SplashWnd = NULL;
        if (g_hLogoBmp) { DeleteObject(g_hLogoBmp);    g_hLogoBmp = NULL; }
        if (g_hFontTitle) { DeleteObject(g_hFontTitle);  g_hFontTitle = NULL; }
        if (g_hFontStatus) { DeleteObject(g_hFontStatus); g_hFontStatus = NULL; }
        if (g_hFontCount) { DeleteObject(g_hFontCount);  g_hFontCount = NULL; }
        UnregisterClassW(L"ModLoaderSplash", GetModuleHandle(NULL));
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
    }
}

// ---------------------------------------------------------------------------
// Mod lifecycle
// ---------------------------------------------------------------------------
void ModLoader::NotifyInit()
{
    for (auto& mod : m_mods)
    {
        if (!mod.healthy) continue;
        try
        {
            if (!mod.instance->OnInit())
            {
                Log("OnInit() returned false for: " + std::string(mod.instance->GetInfo()->id));
                mod.healthy = false;
            }
        }
        catch (...)
        {
            Log("OnInit() threw for: " + std::string(mod.instance->GetInfo()->id));
            mod.healthy = false;
        }
    }
}

void ModLoader::NotifyRegister()
{
    Log(L"REGISTER");
    for (auto& mod : m_mods)
    {
        if (!mod.healthy) continue;
        try { mod.instance->OnRegister(); }
        catch (...) {
            Log("OnRegister() threw for: " + std::string(mod.instance->GetInfo()->id));
            mod.healthy = false;
        }
    }

    const auto& pending = Registry::Internal::GetPendingCreativeItems();
    size_t count = pending.size();

    for (const auto& item : pending)
    {

        GameBridge::AddToCreativeGroup(item.itemId, item.count, item.auxValue, item.groupIndex);
    }

    Registry::Internal::ClearPendingCreativeItems();

    Log("Registry flush complete — " + std::to_string(count) + " creative item(s) injected");
}

void ModLoader::OnLevelLoad()
{
    if (m_levelLoaded) return;
    m_levelLoaded = true;
    Log(L"Level loaded");
    for (auto& mod : m_mods)
    {
        if (!mod.healthy) continue;
        try { mod.instance->OnLevelLoad(); }
        catch (...) { mod.healthy = false; }
    }
}

void ModLoader::OnLevelUnload()
{
    if (!m_levelLoaded) return;
    m_levelLoaded = false;
    Log(L"Level unloaded");
    for (auto& mod : m_mods)
    {
        if (!mod.healthy) continue;
        try { mod.instance->OnLevelUnload(); }
        catch (...) { mod.healthy = false; }
    }
}

void ModLoader::NotifyUpdate(float deltaTime)
{
    for (auto& mod : m_mods)
    {
        if (!mod.healthy) continue;
        if (!mod.instance) { mod.healthy = false; continue; }
        try
        {
            if (!mod.instance->OnUpdate(deltaTime))
            {
                Log("OnUpdate() returned false for: " + std::string(mod.instance->GetInfo()->id));
                mod.healthy = false;
            }
        }
        catch (...)
        {
            Log("OnUpdate() threw for: " + std::string(mod.instance->GetInfo()->id));
            mod.healthy = false;
        }
    }
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------
void ModLoader::Shutdown()
{
    Log("Shutting down ModLoader");
    for (int i = static_cast<int>(m_mods.size()) - 1; i >= 0; --i)
        UnloadOneMod(m_mods[i]);
    m_mods.clear();

    if (m_logFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_logFile);
        m_logFile = INVALID_HANDLE_VALUE;
    }
}

// ---------------------------------------------------------------------------
// Mod lookup
// ---------------------------------------------------------------------------
IMod* ModLoader::FindMod(const std::string& id) const
{
    for (const auto& mod : m_mods)
    {
        if (mod.healthy && mod.instance &&
            std::string(mod.instance->GetInfo()->id) == id)
            return mod.instance;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Scanning & loading
// ---------------------------------------------------------------------------
void ModLoader::ScanAndLoadMods(const std::wstring& modsDir)
{
    DWORD dwAttrib = GetFileAttributesW(modsDir.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES || !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        CreateDirectoryW(modsDir.c_str(), NULL);
        Log("Mods directory not found — created it");
        return;
    }

    std::vector<std::wstring> dllPaths;
    std::wstring searchPath = modsDir + L"\\*.dll";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                dllPaths.push_back(modsDir + L"\\" + fd.cFileName);
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }

    g_TotalMods = static_cast<int>(dllPaths.size());
    Log("Found " + std::to_string(g_TotalMods) + " mod(s) to load");

    for (const auto& path : dllPaths)
    {
        std::wstring filename = path.substr(path.find_last_of(L"\\/") + 1);
        UpdateSplashStatus(L"Loading: " + filename);
        LoadOneMod(path);
    }
}

bool ModLoader::LoadOneMod(const std::wstring& dllPath)
{
    std::string narrowPath(dllPath.begin(), dllPath.end());
    std::string filename = narrowPath.substr(narrowPath.find_last_of("\\/") + 1);

    auto t0 = std::chrono::steady_clock::now();

    HMODULE hMod = LoadLibraryW(dllPath.c_str());
    if (!hMod)
    {
        Log("FAIL [" + filename + "] LoadLibrary failed (error " + std::to_string(GetLastError()) + ")");
        ++g_FailedMods;
        return false;
    }

    using CreateModFn = IMod * (*)();
    auto createFn = reinterpret_cast<CreateModFn>(GetProcAddress(hMod, "CreateMod"));
    if (!createFn)
    {
        Log("FAIL [" + filename + "] missing CreateMod export");
        FreeLibrary(hMod);
        ++g_FailedMods;
        return false;
    }

    IMod* instance = nullptr;
    try { instance = createFn(); }
    catch (...)
    {
        Log("FAIL [" + filename + "] CreateMod() threw");
        FreeLibrary(hMod);
        ++g_FailedMods;
        return false;
    }

    if (!instance)
    {
        Log("FAIL [" + filename + "] CreateMod() returned null");
        FreeLibrary(hMod);
        ++g_FailedMods;
        return false;
    }

    const ModInfo* info = instance->GetInfo();
    std::string modId = info ? info->id : "(unknown)";
    std::string modVer = info
        ? std::to_string(info->version.major) + "." + std::to_string(info->version.minor) + "." + std::to_string(info->version.patch)
        : "(unknown)";

    LoadedMod record;
    record.module = hMod;
    record.instance = instance;
    record.path = narrowPath;

    try
    {
        if (!instance->OnLoad())
        {
            Log("WARN [" + modId + " " + modVer + "] OnLoad() returned false — marking unhealthy");
            record.healthy = false;
        }
    }
    catch (...)
    {
        Log("FAIL [" + modId + " " + modVer + "] OnLoad() threw — marking unhealthy");
        record.healthy = false;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    if (record.healthy)
    {
        Log("OK   [" + modId + " " + modVer + "] loaded in " + std::to_string(elapsed) + "ms");
        ++g_LoadedMods;
    }
    else
    {
        ++g_FailedMods;
    }

    m_mods.push_back(std::move(record));
    return record.healthy;
}

void ModLoader::UnloadOneMod(LoadedMod& mod)
{
    if (!mod.instance) return;

    if (mod.instance->GetInfo())
        Log("Unloading: " + std::string(mod.instance->GetInfo()->id));

    delete mod.instance;
    mod.instance = nullptr;

    if (mod.module)
    {
        FreeLibrary(mod.module);
        mod.module = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------
void ModLoader::OpenLogFile()
{
    std::wstring logPath = GetModsDirectory() + L"\\modloader.log";
    CreateDirectoryW(GetModsDirectory().c_str(), NULL);
    m_logFile = CreateFileW(logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
}

void ModLoader::Log(const std::string& msg)
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    struct tm buf;
    localtime_s(&buf, &t);
    char ts[16];
    std::strftime(ts, sizeof(ts), "[%H:%M:%S] ", &buf);

    std::string line = std::string(ts) + msg + "\r\n";

    if (m_logFile != INVALID_HANDLE_VALUE)
    {
        DWORD written;
        WriteFile(m_logFile, line.c_str(), static_cast<DWORD>(line.size()), &written, nullptr);
    }

    OutputDebugStringA(line.c_str());
}

void ModLoader::Log(const std::wstring& msg)
{
    Log(std::string(msg.begin(), msg.end()));
}

// ---------------------------------------------------------------------------
// Paths
// ---------------------------------------------------------------------------
std::wstring ModLoader::GetModsDirectory() const
{
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring dir = exePath;
    dir = dir.substr(0, dir.find_last_of(L"\\/"));
    return dir + L"\\mods";
}