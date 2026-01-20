// project.h
#pragma once

#include <windows.h>

// Centers a window on the monitor work area (avoids taskbar).
// width/height are the intended window size in pixels.
void CenterWindow(HWND hWnd, int width, int height);

