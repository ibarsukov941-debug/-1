#include "clic.h"
#include <windowsx.h>
#include <wchar.h>
#include <math.h>

// Simple animated owner-draw buttons implementation.
// Creates 10 buttons and drives animation via timers.

static const int kButtonCount = 1;
static const int kTimerBase = 2000; // timer ids: 2000..2009
static HWND g_buttons[kButtonCount] = { 0 };
static int g_animState[kButtonCount] = { 0 };

// layout constants for the single static button
static const int kMargin = 12;
static const int kButtonW = 140;
static const int kButtonH = 36;

void Clic_Create(HWND hWnd)
{
    // Create 10 buttons in a grid
    const int margin = 12;
    const int bw = 140;
    const int bh = 36;
    int startX = margin;
    int startY = margin;

    // create a single static blue button
    {
        int x = startX;
        int y = startY;
        wchar_t text[32];
        swprintf_s(text, _countof(text), L"Button");
        g_buttons[0] = CreateWindowW(L"BUTTON", text,
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, y, kButtonW, kButtonH, hWnd, (HMENU)(INT_PTR)(1000 + 0), GetModuleHandle(NULL), NULL);
        g_animState[0] = 0;
    }
}

void Clic_Destroy(HWND hWnd)
{
    for (int i = 0; i < kButtonCount; ++i) {
        if (g_buttons[i]) DestroyWindow(g_buttons[i]);
        g_buttons[i] = NULL;
    }
}

void Clic_OnTimer(HWND hWnd, WPARAM wParam)
{
    // No animation timers for static button; ignore
    (void)hWnd; (void)wParam;
}

void Clic_OnResize(HWND hWnd, int newClientW, int newClientH)
{
    // Keep the button at fixed margin from top-left
    if (g_buttons[0]) {
        MoveWindow(g_buttons[0], kMargin, kMargin, kButtonW, kButtonH, TRUE);
    }
}

void Clic_OnClick(HWND hWnd, int index)
{
    // Placeholder for click action. Do not toggle visual state here;
    // visual pressed state is driven by DRAWITEM's ODS_SELECTED.
    (void)hWnd; (void)index;
}

static COLORREF LerpColor(COLORREF a, COLORREF b, double t)
{
    int ra = GetRValue(a), ga = GetGValue(a), ba = GetBValue(a);
    int rb = GetRValue(b), gb = GetGValue(b), bb = GetBValue(b);
    int r = (int)(ra + (rb - ra) * t);
    int g = (int)(ga + (gb - ga) * t);
    int b_ = (int)(ba + (bb - ba) * t);
    return RGB(r, g, b_);
}

void Clic_OnDrawItem(HWND hWnd, const DRAWITEMSTRUCT* dis)
{
    if (!dis) return;
    int ctlId = (int)dis->CtlID;
    if (ctlId < 1000 || ctlId >= 1000 + kButtonCount) return;
    int idx = ctlId - 1000;

    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;

    // Determine visual state: normal (very dark), hover, pressed
    bool hover = (dis->itemState & ODS_HOTLIGHT) != 0;
    bool pressed = (dis->itemState & ODS_SELECTED) != 0;
    COLORREF bg;
    if (pressed) bg = RGB(30, 90, 200);         // slightly lighter on press
    else if (hover) bg = RGB(10, 50, 120);     // hover darker-blue
    else bg = RGB(4, 30, 60);                  // default very dark blue
    HBRUSH br = CreateSolidBrush(bg);
    FillRect(hdc, &rc, br);
    DeleteObject(br);

    // draw a subtle border
    HBRUSH border = CreateSolidBrush(RGB(10, 10, 40));
    FrameRect(hdc, &rc, border);
    DeleteObject(border);

    // draw centered white text, inset when pressed
    wchar_t text[64];
    int len = GetWindowTextW(dis->hwndItem, text, _countof(text));
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    HFONT hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT oldf = (HFONT)SelectObject(hdc, hf);
    RECT tr = rc;
    if (pressed) OffsetRect(&tr, 1, 1);
    DrawTextW(hdc, text, len, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldf);
}
