#include "stdafx.h"
#include "UI.h"
#include "UIScene_ModsMenu.h"
#include "ModLoader.h"
#include <windows.h>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Colours
// ---------------------------------------------------------------------------
#define MC_DARK_GRY  RGB(33,  33,  33)
#define MC_MID_GRY   RGB(50,  50,  50)
#define MC_LIGHT_GRY RGB(198, 198, 198)
#define MC_SEL_BLU   RGB(60,  60,  60)
#define MC_WHITE     RGB(255, 255, 255)
#define MC_BORDER    RGB(80,  80,  80)
#define MC_HOVER     RGB(80,  80,  80)
#define MC_PRESS     RGB(30,  30,  30)
#define MC_SCROLLBAR RGB(100, 100, 100)

// ---------------------------------------------------------------------------
// Layout constants
// ---------------------------------------------------------------------------
#define WW          1200
#define WH          600

#define LIST_X      20
#define LIST_Y      20
#define LIST_W      300
#define LIST_H      480
#define ITEM_H      32

#define INFO_X      340
#define INFO_W      820

#define BTN_X       340
#define BTN_Y       520
#define BTN_W       200
#define BTN_H       40

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
static HWND              g_hWnd = nullptr;
static UIScene_ModsMenu* g_pScene = nullptr;

static HBRUSH g_hBackBrush = nullptr;
static HFONT  g_hFont = nullptr;
static HFONT  g_hFontBold = nullptr;

static std::vector<std::string> g_modNames;

static int  g_selIdx = -1;
static int  g_listScroll = 0;

static std::string g_iName, g_iId, g_iAuthor, g_iVersion, g_iDesc;

static int  g_descScroll = 0;
static int  g_descTextH = 0;

static bool g_btnHover = false;
static bool g_btnPress = false;

static bool g_trackingMouse = false;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void Redraw() { if (g_hWnd) InvalidateRect(g_hWnd, nullptr, FALSE); }

static RECT RectOf(int x, int y, int w, int h) { return { x, y, x + w, y + h }; }

static RECT ListRect() { return RectOf(LIST_X, LIST_Y, LIST_W, LIST_H); }
static RECT BtnRect() { return RectOf(BTN_X, BTN_Y, BTN_W, BTN_H); }

static int MaxListScroll()
{
    int visible = LIST_H / ITEM_H;
    return max(0, (int)g_modNames.size() - visible);
}

static int MeasureDescHeight(HDC hdc, const std::string& text, int width)
{
    if (text.empty()) return 0;
    RECT r = { 0, 0, width, 32000 };
    HFONT old = (HFONT)SelectObject(hdc, g_hFont);
    DrawTextA(hdc, text.c_str(), -1, &r, DT_WORDBREAK | DT_CALCRECT);
    SelectObject(hdc, old);
    return r.bottom;
}

// ---------------------------------------------------------------------------
// GDI drawing primitives
// ---------------------------------------------------------------------------
static void FillRoundRect(HDC hdc, RECT r, int rx, COLORREF fill, COLORREF border)
{
    HBRUSH br = CreateSolidBrush(fill);
    HPEN   pen = CreatePen(PS_SOLID, 1, border);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, br);
    HPEN   op = (HPEN)SelectObject(hdc, pen);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, rx, rx);
    SelectObject(hdc, ob); DeleteObject(br);
    SelectObject(hdc, op); DeleteObject(pen);
}

static void GdiText(HDC hdc, const std::string& s, RECT r, COLORREF col,
    HFONT font, UINT flags = DT_SINGLELINE | DT_VCENTER | DT_LEFT)
{
    SetTextColor(hdc, col);
    SetBkMode(hdc, TRANSPARENT);
    HFONT old = (HFONT)SelectObject(hdc, font);
    DrawTextA(hdc, s.c_str(), -1, &r, flags);
    SelectObject(hdc, old);
}

static void HLine(HDC hdc, int x0, int x1, int y, COLORREF col)
{
    HPEN pen = CreatePen(PS_SOLID, 1, col);
    HPEN old = (HPEN)SelectObject(hdc, pen);
    MoveToEx(hdc, x0, y, nullptr);
    LineTo(hdc, x1, y);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------
static void Paint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT rcClient;
    GetClientRect(hWnd, &rcClient);

    HDC     mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, rcClient.right, rcClient.bottom);
    HBITMAP obmp = (HBITMAP)SelectObject(mem, bmp);

    FillRect(mem, &rcClient, g_hBackBrush);

    // -----------------------------------------------------------------------
    // List panel
    // -----------------------------------------------------------------------
    RECT listRect = ListRect();
    FillRoundRect(mem,
        { listRect.left - 1, listRect.top - 1, listRect.right + 1, listRect.bottom + 1 },
        4, MC_DARK_GRY, MC_BORDER);

    HRGN listClip = CreateRectRgn(listRect.left, listRect.top,
        listRect.right, listRect.bottom);
    SelectClipRgn(mem, listClip);

    if (g_modNames.empty())
    {
        GdiText(mem, "No mods installed", listRect, MC_LIGHT_GRY, g_hFont,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    else
    {
        int visible = LIST_H / ITEM_H;
        for (int i = g_listScroll; i < g_listScroll + visible + 1; ++i)
        {
            if (i >= (int)g_modNames.size()) break;

            int drawY = LIST_Y + (i - g_listScroll) * ITEM_H;
            RECT ir = { LIST_X, drawY, LIST_X + LIST_W, drawY + ITEM_H };

            bool sel = (i == g_selIdx);
            HBRUSH ibr = CreateSolidBrush(sel ? MC_SEL_BLU : MC_DARK_GRY);
            FillRect(mem, &ir, ibr);
            DeleteObject(ibr);

            if (i > g_listScroll)
                HLine(mem, LIST_X, LIST_X + LIST_W, drawY, MC_BORDER);

            RECT tr = ir;
            tr.left += 8;
            GdiText(mem, g_modNames[i], tr, sel ? MC_WHITE : MC_LIGHT_GRY, g_hFont);
        }
    }

    SelectClipRgn(mem, nullptr);
    DeleteObject(listClip);

    if (MaxListScroll() > 0)
    {
        int total = (int)g_modNames.size();
        int visible = LIST_H / ITEM_H;
        int thumbH = max(20, (int)(((float)visible / total) * LIST_H));
        int thumbY = LIST_Y + (int)(((float)g_listScroll / (total - visible)) * (LIST_H - thumbH));
        RECT sb = { LIST_X + LIST_W - 6, thumbY,
                    LIST_X + LIST_W - 2, thumbY + thumbH };
        HBRUSH sbr = CreateSolidBrush(MC_SCROLLBAR);
        FillRect(mem, &sb, sbr);
        DeleteObject(sbr);
    }

    // -----------------------------------------------------------------------
    // Info panel rows
    // -----------------------------------------------------------------------
    int iy = 20;

    auto DrawRow = [&](const std::string& label, const std::string& value)
        {
            RECT lr = { INFO_X,      iy, INFO_X + 75,        iy + 24 };
            RECT vr = { INFO_X + 78, iy, INFO_X + INFO_W,    iy + 24 };
            GdiText(mem, label, lr, MC_LIGHT_GRY, g_hFontBold);
            GdiText(mem, value, vr, MC_WHITE, g_hFont,
                DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);
            iy += 30;
        };

    DrawRow("Name:", g_iName);
    DrawRow("ID:", g_iId);
    DrawRow("Author:", g_iAuthor);
    DrawRow("Version:", g_iVersion);

    RECT dlr = { INFO_X, iy, INFO_X + 200, iy + 22 };
    GdiText(mem, "Description:", dlr, MC_LIGHT_GRY, g_hFontBold);
    iy += 26;

    int descBoxH = BTN_Y - 10 - iy;
    RECT descBox = { INFO_X, iy, INFO_X + INFO_W, iy + descBoxH };
    FillRoundRect(mem, descBox, 4, MC_MID_GRY, MC_BORDER);

    int descInnerW = INFO_W - 14;
    RECT descTextR = { descBox.left + 6,
                       descBox.top + 4 - g_descScroll,
                       descBox.left + 6 + descInnerW,
                       descBox.top + 4 - g_descScroll + 32000 };

    HRGN descClip = CreateRectRgn(descBox.left + 2, descBox.top + 2,
        descBox.right - 2, descBox.bottom - 2);
    SelectClipRgn(mem, descClip);

    if (!g_iDesc.empty())
    {
        SetTextColor(mem, MC_LIGHT_GRY);
        SetBkMode(mem, TRANSPARENT);
        HFONT of = (HFONT)SelectObject(mem, g_hFont);
        DrawTextA(mem, g_iDesc.c_str(), -1, &descTextR, DT_WORDBREAK | DT_LEFT);
        SelectObject(mem, of);
    }

    SelectClipRgn(mem, nullptr);
    DeleteObject(descClip);

    if (g_descTextH > descBoxH)
    {
        int maxDs = g_descTextH - descBoxH;
        int thumbH = max(20, (int)(((float)descBoxH / g_descTextH) * descBoxH));
        int thumbY = descBox.top + (int)(((float)g_descScroll / maxDs) * (descBoxH - thumbH));
        RECT dsb = { descBox.right - 6, thumbY,
                     descBox.right - 2, thumbY + thumbH };
        HBRUSH dsbr = CreateSolidBrush(MC_SCROLLBAR);
        FillRect(mem, &dsb, dsbr);
        DeleteObject(dsbr);
    }

    // -----------------------------------------------------------------------
    // Done button
    // -----------------------------------------------------------------------
    RECT btnR = BtnRect();
    COLORREF bc = g_btnPress ? MC_PRESS : g_btnHover ? MC_HOVER : MC_SEL_BLU;
    FillRoundRect(mem, btnR, 6, bc, MC_BORDER);
    GdiText(mem, "Done", btnR, MC_WHITE, g_hFontBold,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, mem, 0, 0, SRCCOPY);
    SelectObject(mem, obmp);
    DeleteObject(bmp);
    DeleteDC(mem);

    EndPaint(hWnd, &ps);
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------
static LRESULT CALLBACK ModsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        // -----------------------------------------------------------------------
    case WM_PAINT:
        Paint(hWnd);
        return 0;

    case WM_ERASEBKGND:
        return 1;

        // -----------------------------------------------------------------------
    case WM_MOUSEMOVE:
    {
        if (!g_trackingMouse)
        {
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
            TrackMouseEvent(&tme);
            g_trackingMouse = true;
        }

        int mx = (short)LOWORD(lParam), my = (short)HIWORD(lParam);
        RECT btnR = BtnRect();
        bool prev = g_btnHover;
        g_btnHover = (PtInRect(&btnR, { mx, my }) != 0);
        if (prev != g_btnHover) Redraw();
        return 0;
    }

    case WM_MOUSELEAVE:
        g_trackingMouse = false;
        if (g_btnHover) { g_btnHover = false; Redraw(); }
        return 0;

    case WM_LBUTTONDOWN:
    {
        int mx = (short)LOWORD(lParam), my = (short)HIWORD(lParam);
        RECT listR = ListRect();
        RECT btnR = BtnRect();

        if (PtInRect(&listR, { mx, my }))
        {
            int row = (my - LIST_Y) / ITEM_H + g_listScroll;
            if (row >= 0 && row < (int)g_modNames.size())
            {
                g_selIdx = row;
                g_descScroll = 0;
                if (g_pScene) g_pScene->ShowModInfoInWindow(row);
                Redraw();
            }
        }
        else if (PtInRect(&btnR, { mx, my }))
        {
            g_btnPress = true;
            SetCapture(hWnd);
            Redraw();
        }
        return 0;
    }

    case WM_LBUTTONUP:
    {
        int mx = (short)LOWORD(lParam), my = (short)HIWORD(lParam);
        if (g_btnPress)
        {
            g_btnPress = false;
            ReleaseCapture();
            RECT btnR = BtnRect();
            if (PtInRect(&btnR, { mx, my }))
            {
                DestroyWindow(hWnd);
                return 0;
            }
            Redraw();
        }
        return 0;
    }

    case WM_MOUSEWHEEL:
    {
        POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
        ScreenToClient(hWnd, &pt);

        int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;

        RECT listR = ListRect();
        if (PtInRect(&listR, pt))
        {
            g_listScroll -= delta;
            g_listScroll = max(0, min(g_listScroll, MaxListScroll()));
            Redraw();
            return 0;
        }

        int iy = 20 + 4 * 30 + 26;
        int descBoxH = BTN_Y - 10 - iy;
        RECT descR = { INFO_X, iy, INFO_X + INFO_W, iy + descBoxH };
        if (PtInRect(&descR, pt) && g_descTextH > descBoxH)
        {
            int maxDs = g_descTextH - descBoxH;
            g_descScroll -= delta * 20;
            g_descScroll = max(0, min(g_descScroll, maxDs));
            Redraw();
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hWnd);
            return 0;
        }
        if (wParam == VK_UP || wParam == VK_DOWN)
        {
            int next = g_selIdx + (wParam == VK_DOWN ? 1 : -1);
            next = max(0, min(next, (int)g_modNames.size() - 1));
            if (next != g_selIdx)
            {
                g_selIdx = next;
                g_descScroll = 0;

                if (g_selIdx < g_listScroll)
                    g_listScroll = g_selIdx;
                else if (g_selIdx >= g_listScroll + LIST_H / ITEM_H)
                    g_listScroll = g_selIdx - LIST_H / ITEM_H + 1;

                if (g_pScene) g_pScene->ShowModInfoInWindow(g_selIdx);
                Redraw();
            }
            return 0;
        }
        break;

    case WM_DESTROY:
        if (g_pScene) g_pScene->m_bWindowClosed = true;
        g_hWnd = nullptr;
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Window creation
// ---------------------------------------------------------------------------
static void CreateModsWindow(UIScene_ModsMenu* pScene)
{
    if (g_hWnd) return;

    g_pScene = pScene;
    g_selIdx = -1;
    g_listScroll = 0;
    g_descScroll = 0;
    g_descTextH = 0;
    g_iName = g_iId = g_iAuthor = g_iVersion = g_iDesc = "";
    g_btnHover = g_btnPress = false;
    g_trackingMouse = false;

    if (!g_hBackBrush)
        g_hBackBrush = CreateSolidBrush(MC_DARK_GRY);

    if (!g_hFont)
        g_hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    if (!g_hFontBold)
        g_hFontBold = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    if (!GetClassInfoEx(GetModuleHandle(nullptr), "ModsMenuWindow", &wc))
    {
        wc.lpfnWndProc = ModsWndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = "ModsMenuWindow";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassEx(&wc);
    }

    HWND hParent = GetActiveWindow();
    LONG style = GetWindowLong(hParent, GWL_STYLE);
    if (!(style & WS_CLIPCHILDREN))
        SetWindowLong(hParent, GWL_STYLE, style | WS_CLIPCHILDREN);

    RECT rcParent;
    GetClientRect(hParent, &rcParent);
    int wx = (rcParent.right - WW) / 2;
    int wy = (rcParent.bottom - WH) / 2;

    g_hWnd = CreateWindowEx(
        WS_EX_NOPARENTNOTIFY,
        "ModsMenuWindow", nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        wx, wy, WW, WH,
        hParent, nullptr, GetModuleHandle(nullptr), nullptr
    );

    SetFocus(g_hWnd);

    g_modNames.clear();
    const auto& mods = ModLoader::Get().GetMods();
    for (const auto& m : mods)
    {
        if (!m.instance) continue;
        std::string name = m.instance->GetInfo()->name;
        if (!m.healthy) name += " (disabled)";
        g_modNames.push_back(name);
    }
}

// ---------------------------------------------------------------------------
// UIScene_ModsMenu implementation
// ---------------------------------------------------------------------------
UIScene_ModsMenu::UIScene_ModsMenu(int iPad, void* initData, UILayer* parentLayer)
    : UIScene(iPad, parentLayer)
    , m_selectedIndex(-1)
    , m_bWindowClosed(false)
{
    CreateModsWindow(this);
}

UIScene_ModsMenu::~UIScene_ModsMenu()
{
    if (g_hWnd)
    {
        DestroyWindow(g_hWnd);
        g_hWnd = nullptr;
    }
}

wstring UIScene_ModsMenu::getMoviePath() { return L""; }

void UIScene_ModsMenu::handleGainFocus(bool navBack) {}

void UIScene_ModsMenu::tick()
{
    m_hasTickedOnce = true;
    m_bCanHandleInput = true;

    if (g_hWnd && IsWindow(g_hWnd))
    {
        MSG msg;
        while (PeekMessage(&msg, g_hWnd, 0, 0, PM_REMOVE))
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

void UIScene_ModsMenu::handleInput(int iPad, int key, bool repeat,
    bool pressed, bool released, bool& handled)
{
    if (g_hWnd && IsWindow(g_hWnd)) handled = true;

    if (key == ACTION_MENU_CANCEL && pressed && m_hasTickedOnce)
    {
        if (g_hWnd) DestroyWindow(g_hWnd);
    }
}

void UIScene_ModsMenu::handlePress(F64 controlId, F64 childId) {}

void UIScene_ModsMenu::ShowModInfoInWindow(int index)
{
    if (!g_hWnd) return;

    const auto& mods = ModLoader::Get().GetMods();
    if (index < 0 || index >= (int)mods.size() || !mods[index].instance) return;

    const ModInfo* info = mods[index].instance->GetInfo();

    g_iName = info->name ? info->name : "";
    g_iId = info->id ? info->id : "";
    g_iAuthor = info->author ? info->author : "";
    g_iVersion = std::to_string(info->version.major) + "." +
        std::to_string(info->version.minor) + "." +
        std::to_string(info->version.patch);
    g_iDesc = info->description ? info->description : "";
    if (!mods[index].healthy)
        g_iDesc += "\r\n[This mod has been disabled due to an error.]";

    g_descScroll = 0;

    HDC hdc = GetDC(g_hWnd);
    int descInnerW = INFO_W - 14;
    g_descTextH = MeasureDescHeight(hdc, g_iDesc, descInnerW);
    ReleaseDC(g_hWnd, hdc);

    Redraw();
}

void UIScene_ModsMenu::PopulateModList() {}
void UIScene_ModsMenu::ShowModInfo(int) {}
void UIScene_ModsMenu::ClearModInfo() {}