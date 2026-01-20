// clic.h - create 10 animated owner-draw buttons
#pragma once

#include <windows.h>

// Initialize buttons (call from WM_CREATE)
void Clic_Create(HWND hWnd);
// Cleanup (call from WM_DESTROY)
void Clic_Destroy(HWND hWnd);
// Called from WM_TIMER to update animations
void Clic_OnTimer(HWND hWnd, WPARAM wParam);
// Called from WM_DRAWITEM to render owner-draw buttons
void Clic_OnDrawItem(HWND hWnd, const DRAWITEMSTRUCT* dis);
// Called from WM_SIZE to reposition the button(s)
void Clic_OnResize(HWND hWnd, int newClientW, int newClientH);
// Called when the button is clicked (index)
void Clic_OnClick(HWND hWnd, int index);
