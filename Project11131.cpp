// Project11131.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Project11131.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include "clic.h"

#define MAX_LOADSTRING 100

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна
// Animation targets and state for smooth resize
static double g_animCurW = 20.0;
static double g_animCurH = 30.0;
static double g_animStepW = 0.0;
static double g_animStepH = 0.0;
static int g_animTargetW = 800;
static int g_animTargetH = 600;
static bool g_animating = false;

// Centers a window on the monitor work area (avoids taskbar).
// width/height are the intended window size in pixels.
void CenterWindow(HWND hWnd, int width, int height)
{
    MONITORINFO mi = { 0 };
    mi.cbSize = sizeof(mi);
    HMONITOR hm = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    if (hm && GetMonitorInfoW(hm, &mi)) {
        int workW = mi.rcWork.right - mi.rcWork.left;
        int workH = mi.rcWork.bottom - mi.rcWork.top;
        int x = mi.rcWork.left + (workW - width) / 2;
        int y = mi.rcWork.top + (workH - height) / 2;
        SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    else {
        int sx = GetSystemMetrics(SM_CXSCREEN);
        int sy = GetSystemMetrics(SM_CYSCREEN);
        int x = (sx - width) / 2;
        int y = (sy - height) / 2;
        SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Разместите код здесь.

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PROJECT11131, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PROJECT11131));

    MSG msg;

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}



//
//  ФУНКЦИЯ: MyRegisterClass()
//
//  ЦЕЛЬ: Регистрирует класс окна.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PROJECT11131));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)NULL;
    // Disable the standard menu to avoid white non-client/menu backgrounds
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//
//   ЦЕЛЬ: Сохраняет маркер экземпляра и создает главное окно
//
//   КОММЕНТАРИИ:
//
//        В этой функции маркер экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

    // Create a frameless/ caption-less window so the titlebar (white area)
    // does not appear while keeping resizable frame behavior.
    // Create window initially at size ~20x30 so we can animate to target size
    HWND hWnd = CreateWindowExW(0, szWindowClass, L"",
        WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU),
        CW_USEDEFAULT, 0, 20, 30, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    // Center the window on the monitor work area using the target size
    CenterWindow(hWnd, g_animTargetW, g_animTargetH);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Ensure blur/ composition is applied after the window is visible and
    // force a frame/client repaint to avoid initial white flash.
    // Apply accent policy (same logic as in WM_CREATE)
    {
        typedef enum _ACCENT_STATE {
            ACCENT_DISABLED = 0,
            ACCENT_ENABLE_GRADIENT = 1,
            ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
            ACCENT_ENABLE_BLURBEHIND = 3,
            ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
            ACCENT_INVALID_STATE = 5
        } ACCENT_STATE;

        struct ACCENT_POLICY {
            int AccentState;
            int AccentFlags;
            unsigned int GradientColor;
            int AnimationId;
        };

        enum WINDOW_COMPOSITION_ATTRIB {
            WCA_UNDEFINED = 0,
            WCA_ACCENT_POLICY = 19
        };

        struct WINDOWCOMPOSITIONATTRIBDATA {
            int Attrib;
            PVOID pvData;
            SIZE_T cbData;
        };

        HMODULE hUser = LoadLibraryW(L"user32.dll");
        if (hUser) {
            typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);
            pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");
            if (SetWindowCompositionAttribute) {
                ACCENT_POLICY policy = { 0 };
                policy.AccentState = ACCENT_ENABLE_BLURBEHIND;
                policy.AccentFlags = 0;
                policy.GradientColor = 0;

                WINDOWCOMPOSITIONATTRIBDATA data;
                data.Attrib = WCA_ACCENT_POLICY;
                data.pvData = &policy;
                data.cbData = sizeof(policy);

                SetWindowCompositionAttribute(hWnd, &data);
            }
            else {
                DWM_BLURBEHIND bb = { 0 };
                bb.dwFlags = DWM_BB_ENABLE;
                bb.fEnable = TRUE;
                bb.hRgnBlur = NULL;
                DwmEnableBlurBehindWindow(hWnd, &bb);
            }
            FreeLibrary(hUser);
        }
        else {
            DWM_BLURBEHIND bb = { 0 };
            bb.dwFlags = DWM_BB_ENABLE;
            bb.fEnable = TRUE;
            bb.hRgnBlur = NULL;
            DwmEnableBlurBehindWindow(hWnd, &bb);
        }
    }

    // Force a frame change and immediate redraw to eliminate white artifacts.
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_UPDATENOW);

    // Start animation: compute per-tick steps and set a timer.
    // Animate over ~100 steps (~1000ms with 10ms timer).
    {
        const int steps = 100;
        g_animCurW = 20.0;
        g_animCurH = 30.0;
        g_animStepW = (double)(g_animTargetW - (int)g_animCurW) / (double)steps;
        g_animStepH = (double)(g_animTargetH - (int)g_animCurH) / (double)steps;
        g_animating = true;
        SetTimer(hWnd, 1, 10, NULL); // 10ms timer
    }

    return TRUE;
}

//
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ЦЕЛЬ: Обрабатывает сообщения в главном окне.
//
//  WM_COMMAND  - обработать меню приложения
//  WM_PAINT    - Отрисовка главного окна
//  WM_DESTROY  - отправить сообщение о выходе и вернуться
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // Enable a simple blurred + transparent background using Accent BlurBehind.
        // Load SetWindowCompositionAttribute dynamically and request blur-behind.
        typedef enum _ACCENT_STATE {
            ACCENT_DISABLED = 0,
            ACCENT_ENABLE_GRADIENT = 1,
            ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
            ACCENT_ENABLE_BLURBEHIND = 3,
            ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
            ACCENT_INVALID_STATE = 5
        } ACCENT_STATE;

        struct ACCENT_POLICY {
            int AccentState;
            int AccentFlags;
            unsigned int GradientColor;
            int AnimationId;
        };

        enum WINDOW_COMPOSITION_ATTRIB {
            WCA_UNDEFINED = 0,
            WCA_ACCENT_POLICY = 19
        };

        struct WINDOWCOMPOSITIONATTRIBDATA {
            int Attrib;
            PVOID pvData;
            SIZE_T cbData;
        };

        HMODULE hUser = LoadLibraryW(L"user32.dll");
        if (hUser) {
            typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);
            pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");
            if (SetWindowCompositionAttribute) {
                ACCENT_POLICY policy = { 0 };
                // Use simple blur-behind (no acrylic tint) for transparent frosted glass
                policy.AccentState = ACCENT_ENABLE_BLURBEHIND;
                policy.AccentFlags = 0;
                policy.GradientColor = 0; // no tint

                WINDOWCOMPOSITIONATTRIBDATA data;
                data.Attrib = WCA_ACCENT_POLICY;
                data.pvData = &policy;
                data.cbData = sizeof(policy);

                SetWindowCompositionAttribute(hWnd, &data);
            }
            else {
                // Fallback: use DWM blur behind
                DWM_BLURBEHIND bb = { 0 };
                bb.dwFlags = DWM_BB_ENABLE;
                bb.fEnable = TRUE;
                bb.hRgnBlur = NULL;
                DwmEnableBlurBehindWindow(hWnd, &bb);
            }
            FreeLibrary(hUser);
        }
        else {
            // Fallback: use DWM blur behind
            DWM_BLURBEHIND bb = { 0 };
            bb.dwFlags = DWM_BB_ENABLE;
            bb.fEnable = TRUE;
            bb.hRgnBlur = NULL;
            DwmEnableBlurBehindWindow(hWnd, &bb);
        }
        // create animated buttons
        Clic_Create(hWnd);
    }
    break;
    case WM_MOVE:
    {
        RECT rc;
        if (GetClientRect(hWnd, &rc)) {
            Clic_OnResize(hWnd, rc.right - rc.left, rc.bottom - rc.top);
        }
    }
    break;
    case WM_SIZING:
    {
        // Keep button fixed while user is interactively sizing the window
        Clic_OnResize(hWnd, 0, 0);
    }
    break;
    case WM_ENTERSIZEMOVE:
    {
        Clic_OnResize(hWnd, 0, 0);
    }
    break;
    case WM_EXITSIZEMOVE:
    {
        RECT rc;
        if (GetClientRect(hWnd, &rc)) Clic_OnResize(hWnd, rc.right - rc.left, rc.bottom - rc.top);
    }
    break;
    case WM_WINDOWPOSCHANGED:
    {
        RECT rc;
        if (GetClientRect(hWnd, &rc)) {
            Clic_OnResize(hWnd, rc.right - rc.left, rc.bottom - rc.top);
        }
    }
    break;
    case WM_SIZE:
    {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        Clic_OnResize(hWnd, w, h);
    }
    break;
    case WM_TIMER:
    {
        if (wParam == 1 && g_animating) {
            // advance animation
            g_animCurW += g_animStepW;
            g_animCurH += g_animStepH;

            int newW = (int)(g_animCurW + 0.5);
            int newH = (int)(g_animCurH + 0.5);

            // clamp to target
            if (newW >= g_animTargetW) newW = g_animTargetW;
            if (newH >= g_animTargetH) newH = g_animTargetH;

            SetWindowPos(hWnd, NULL, 0, 0, newW, newH, SWP_NOMOVE | SWP_NOZORDER);
            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

            if (newW == g_animTargetW && newH == g_animTargetH) {
                KillTimer(hWnd, 1);
                g_animating = false;
            }
        }
        else {
            // forward other timers to button animations
            Clic_OnTimer(hWnd, wParam);
        }
    }
    break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);
        // handle our button click (id 1000)
        if (wmId == 1000 && wmEvent == BN_CLICKED) {
            Clic_OnClick(hWnd, 0);
            break;
        }
        // Разобрать выбор в меню:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Добавьте сюда любой код прорисовки, использующий HDC...
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        // cleanup animated buttons
        Clic_Destroy(hWnd);
        PostQuitMessage(0);
        break;
    case WM_DRAWITEM:
    {
        Clic_OnDrawItem(hWnd, (DRAWITEMSTRUCT*)lParam);
    }
    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Обработчик сообщений для окна "О программе".
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
