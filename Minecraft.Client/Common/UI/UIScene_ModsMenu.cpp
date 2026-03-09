#include "stdafx.h"
#include "UI.h"
#include "UIScene_ModsMenu.h"
#include "ModLoader.h"
#include <windows.h>
#include <commctrl.h>
#include <string>

#pragma comment(lib, "comctl32.lib")

#define IDC_MODLIST     1001
#define IDC_CLOSE       1002
#define IDC_MOD_NAME    1003
#define IDC_MOD_ID      1004
#define IDC_MOD_AUTHOR  1005
#define IDC_MOD_VERSION 1006
#define IDC_MOD_DESC    1007
#define IDC_MINI_X      1008

#define MC_BLACK     RGB(0, 0, 0)
#define MC_DARK_GRY  RGB(33, 33, 33)
#define MC_LIGHT_GRY RGB(198, 198, 198)
#define MC_SEL_BLU   RGB(60, 60, 60)

static HWND g_hModsWindow = nullptr;
static HWND g_hList = nullptr;
static UIScene_ModsMenu* g_pScene = nullptr;
static HBRUSH g_hBackBrush = nullptr;
static HBRUSH g_hSelBrush = nullptr;

static LRESULT CALLBACK ModsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_CLOSE || LOWORD(wParam) == IDC_MINI_X)
        {
            DestroyWindow(hWnd);
        }
        else if (LOWORD(wParam) == IDC_MODLIST && HIWORD(wParam) == LBN_SELCHANGE)
        {
            int sel = (int)SendMessage(g_hList, LB_GETCURSEL, 0, 0);
            if (g_pScene && sel >= 0)
                g_pScene->ShowModInfoInWindow(sel);
        }
        break;

    case WM_MEASUREITEM:
    {
        LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
        if (lpmis->CtlID == IDC_MODLIST)
        {
            lpmis->itemHeight = 32;
            return TRUE;
        }
        break;
    }

    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
        if (lpdis->CtlID == IDC_MODLIST && lpdis->itemID != (UINT)-1)
        {
            char text[256];
            SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM)text);

            if (lpdis->itemState & ODS_SELECTED)
            {
                FillRect(lpdis->hDC, &lpdis->rcItem, g_hSelBrush);
                SetTextColor(lpdis->hDC, RGB(255, 255, 255));
            }
            else
            {
                FillRect(lpdis->hDC, &lpdis->rcItem, g_hBackBrush);
                SetTextColor(lpdis->hDC, MC_LIGHT_GRY);
            }

            SetBkMode(lpdis->hDC, TRANSPARENT);
            RECT textRect = lpdis->rcItem;
            textRect.left += 5;
            DrawTextA(lpdis->hDC, text, -1, &textRect, DT_SINGLELINE | DT_VCENTER);

            if (lpdis->itemState & ODS_FOCUS)
                DrawFocusRect(lpdis->hDC, &lpdis->rcItem);

            return TRUE;
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, MC_LIGHT_GRY);
        SetBkColor(hdc, MC_DARK_GRY);
        return (LRESULT)g_hBackBrush;
    }

    case WM_ERASEBKGND:
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect((HDC)wParam, &rc, g_hBackBrush);
        return 1;
    }

    case WM_DESTROY:
        if (g_pScene)
            g_pScene->m_bWindowClosed = true;

        if (g_hSelBrush)
        {
            DeleteObject(g_hSelBrush);
            g_hSelBrush = nullptr;
        }

        g_hModsWindow = nullptr;
        g_hList = nullptr;
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static void CreateModsWindow(UIScene_ModsMenu* pScene)
{
    if (g_hModsWindow) return;

    g_pScene = pScene;
    if (!g_hBackBrush) g_hBackBrush = CreateSolidBrush(MC_DARK_GRY);
    if (!g_hSelBrush) g_hSelBrush = CreateSolidBrush(MC_SEL_BLU);

    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    if (!GetClassInfoEx(GetModuleHandle(nullptr), "ModsMenuWindow", &wc))
    {
        wc.lpfnWndProc = ModsWndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hbrBackground = g_hBackBrush;
        wc.lpszClassName = "ModsMenuWindow";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassEx(&wc);
    }

    HWND hParent = GetActiveWindow();
    LONG style = GetWindowLong(hParent, GWL_STYLE);
    if (!(style & WS_CLIPCHILDREN)) SetWindowLong(hParent, GWL_STYLE, style | WS_CLIPCHILDREN);

    RECT rcParent;
    GetClientRect(hParent, &rcParent);
    int sw = rcParent.right - rcParent.left;
    int sh = rcParent.bottom - rcParent.top;

    int ww = 1200, wh = 600;
    int wx = (sw - ww) / 2;
    int wy = (sh - 520) / 2;

    g_hModsWindow = CreateWindowEx(
        WS_EX_NOPARENTNOTIFY | WS_EX_CONTROLPARENT,
        "ModsMenuWindow", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS,
        wx, wy, ww, wh,
        hParent,
        nullptr, GetModuleHandle(nullptr), nullptr
    );

    auto CreateLabel = [](const char* txt, int x, int y, int w, int id = -1) {
        return CreateWindow("STATIC", txt, WS_CHILD | WS_VISIBLE, x, y, w, 20, g_hModsWindow, (HMENU)id, nullptr, nullptr);
        };

    g_hList = CreateWindow(
        "LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS | LBS_OWNERDRAWFIXED,
        20, 20, 300, 480,
        g_hModsWindow, (HMENU)IDC_MODLIST, GetModuleHandle(nullptr), nullptr
    );

    CreateLabel("Name:", 340, 20, 60);
    CreateLabel("", 410, 20, 400, IDC_MOD_NAME);
    CreateLabel("ID:", 340, 50, 60);
    CreateLabel("", 410, 50, 400, IDC_MOD_ID);
    CreateLabel("Author:", 340, 80, 60);
    CreateLabel("", 410, 80, 400, IDC_MOD_AUTHOR);
    CreateLabel("Version:", 340, 110, 60);
    CreateLabel("", 410, 110, 400, IDC_MOD_VERSION);
    CreateLabel("Description:", 340, 140, 100);

    CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        340, 165, 480, 335, g_hModsWindow, (HMENU)IDC_MOD_DESC, nullptr, nullptr);

    CreateWindow("BUTTON", "Done",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        340, 520, 200, 40,
        g_hModsWindow, (HMENU)IDC_CLOSE, GetModuleHandle(nullptr), nullptr
    );


    const auto& mods = ModLoader::Get().GetMods();
    for (int i = 0; i < (int)mods.size(); ++i)
    {
        if (!mods[i].instance) continue;
        std::string displayName = mods[i].instance->GetInfo()->name;
        if (!mods[i].healthy) displayName += " (disabled)";
        SendMessageA(g_hList, LB_ADDSTRING, 0, (LPARAM)displayName.c_str());
    }

    if (SendMessage(g_hList, LB_GETCOUNT, 0, 0) == 0)
    {
        SendMessageA(g_hList, LB_ADDSTRING, 0, (LPARAM)"No mods installed");
    }
}

UIScene_ModsMenu::UIScene_ModsMenu(int iPad, void* initData, UILayer* parentLayer)
    : UIScene(iPad, parentLayer)
    , m_selectedIndex(-1)
    , m_bWindowClosed(false)
{
    CreateModsWindow(this);
}

UIScene_ModsMenu::~UIScene_ModsMenu()
{
    if (g_hModsWindow)
    {
        DestroyWindow(g_hModsWindow);
        g_hModsWindow = nullptr;
    }
}

wstring UIScene_ModsMenu::getMoviePath()
{
    return L"";
}

void UIScene_ModsMenu::handleGainFocus(bool navBack)
{
}

void UIScene_ModsMenu::tick()
{
    m_hasTickedOnce = true;
    m_bCanHandleInput = true;

    if (g_hModsWindow && IsWindow(g_hModsWindow))
    {
        MSG msg;
        while (PeekMessage(&msg, g_hModsWindow, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (m_bWindowClosed)
    {
        m_bWindowClosed = false;
        ui.NavigateBack(m_iPad);
    }
}

void UIScene_ModsMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool& handled)
{
    if (g_hModsWindow && IsWindow(g_hModsWindow)) handled = true;

    if (key == ACTION_MENU_CANCEL && pressed && m_hasTickedOnce)
    {
        if (g_hModsWindow) DestroyWindow(g_hModsWindow);
    }
}

void UIScene_ModsMenu::handlePress(F64 controlId, F64 childId) {}

void UIScene_ModsMenu::ShowModInfoInWindow(int index)
{
    if (!g_hModsWindow) return;

    const auto& mods = ModLoader::Get().GetMods();
    if (index < 0 || index >= (int)mods.size() || !mods[index].instance) return;

    const ModInfo* info = mods[index].instance->GetInfo();

    SetWindowTextA(GetDlgItem(g_hModsWindow, IDC_MOD_NAME), info->name ? info->name : "");
    SetWindowTextA(GetDlgItem(g_hModsWindow, IDC_MOD_ID), info->id ? info->id : "");
    SetWindowTextA(GetDlgItem(g_hModsWindow, IDC_MOD_AUTHOR), info->author ? info->author : "");

    std::string ver = std::to_string(info->version.major) + "." + std::to_string(info->version.minor) + "." + std::to_string(info->version.patch);
    SetWindowTextA(GetDlgItem(g_hModsWindow, IDC_MOD_VERSION), ver.c_str());

    std::string desc = info->description ? info->description : "";
    if (!mods[index].healthy) desc += "\r\n[This mod has been disabled due to an error.]";
    SetWindowTextA(GetDlgItem(g_hModsWindow, IDC_MOD_DESC), desc.c_str());
}

void UIScene_ModsMenu::PopulateModList() {}
void UIScene_ModsMenu::ShowModInfo(int) {}
void UIScene_ModsMenu::ClearModInfo() {}