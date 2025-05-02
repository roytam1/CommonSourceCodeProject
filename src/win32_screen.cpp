/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 screen ]
*/

#include "emu.h"
#include "vm/vm.h"

void EMU::initialize_screen()
{
	// create dib section
	HDC hdc = GetDC(main_window_handle);
	lpBuf = (LPBYTE)GlobalAlloc(GPTR, sizeof(BITMAPINFO));
	lpDIB = (LPBITMAPINFO)lpBuf;
	lpDIB->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpDIB->bmiHeader.biWidth = SCREEN_WIDTH;
	lpDIB->bmiHeader.biHeight = SCREEN_HEIGHT;
	lpDIB->bmiHeader.biPlanes = 1;
	lpDIB->bmiHeader.biBitCount = 16;
#ifdef _WIN32_WCE
	// RGB565
	lpDIB->bmiHeader.biCompression = BI_BITFIELDS;
	LPDWORD lpBf = (LPDWORD)lpDIB->bmiColors;
	lpBf[0] = 0x1f << 11;
	lpBf[1] = 0x3f << 5;
	lpBf[2] = 0x1f << 0;
#else
	lpDIB->bmiHeader.biCompression = BI_RGB;
#endif
	lpDIB->bmiHeader.biSizeImage = 0;
	lpDIB->bmiHeader.biXPelsPerMeter = 0;
	lpDIB->bmiHeader.biYPelsPerMeter = 0;
	lpDIB->bmiHeader.biClrUsed = 0;
	lpDIB->bmiHeader.biClrImportant = 0;
	hBMP = CreateDIBSection(hdc, lpDIB, DIB_RGB_COLORS, (PVOID*)&lpBMP, NULL, 0);
	hdcDIB = CreateCompatibleDC(hdc);
	SelectObject(hdcDIB, hBMP);
}

void EMU::release_screen()
{
	// release dib section
	DeleteDC(hdcDIB);
	DeleteObject(hBMP);
	GlobalFree(lpBuf);
}

void EMU::set_screen_size(int width, int height)
{
#ifdef _WIN32_WCE
	// always fullscreen
	window_width = GetSystemMetrics(SM_CXSCREEN);
	window_height = GetSystemMetrics(SM_CYSCREEN);
#else
	window_width = width;
	window_height = height;
#endif
}

void EMU::draw_screen()
{
	// draw screen if required
	vm->draw_screen();
	
	// invalidate window
	InvalidateRect(main_window_handle, NULL, FALSE);
	UpdateWindow(main_window_handle);
}

void EMU::update_screen(HDC hdc)
{
	int power_x = window_width / SCREEN_WIDTH; if(power_x < 1) power_x = 1;
	int power_y = window_height / SCREEN_HEIGHT; if(power_y < 1) power_y = 1;
#ifndef DONT_KEEP_ASPECT
	if(power_x < power_y) power_y = power_x; else power_x = power_y;
#endif
	int dest_x = (window_width - SCREEN_WIDTH * power_x) >> 1;
	int dest_y = (window_height - SCREEN_HEIGHT * power_y) >> 1;
	
	if(power_x > 1 || power_y > 1)
		StretchBlt(hdc, dest_x, dest_y, SCREEN_WIDTH * power_x, SCREEN_HEIGHT * power_y, hdcDIB, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SRCCOPY);
	else
		BitBlt(hdc, dest_x, dest_y, SCREEN_WIDTH, SCREEN_HEIGHT, hdcDIB, 0, 0, SRCCOPY);
}

