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
#ifndef _WIN32_WCE
	pAVIStream = NULL;
	pAVICompressed = NULL;
	pAVIFile = NULL;
#endif
	now_recv = false;
	
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
	
	// stop recording
	stop_rec_video();
}

void EMU::set_screen_size(int width, int height)
{
#ifdef _WIN32_WCE
	// always fullscreen and don't stretch
	window_width = GetSystemMetrics(SM_CXSCREEN);
	window_height = GetSystemMetrics(SM_CYSCREEN);
	power_x = power_y = 1;
#else
	window_width = width;
	window_height = height;
	power_x = window_width / SCREEN_WIDTH; if(power_x < 1) power_x = 1;
	power_y = window_height / SCREEN_HEIGHT; if(power_y < 1) power_y = 1;
#ifndef DONT_KEEP_ASPECT
	if(power_x < power_y) power_y = power_x; else power_x = power_y;
#endif
#endif
	dest_x = (window_width - SCREEN_WIDTH * power_x) >> 1;
	dest_y = (window_height - SCREEN_HEIGHT * power_y) >> 1;
}

void EMU::draw_screen()
{
#ifdef _USE_GAPI
	// draw screen to vram directly
	if(lpGapi = (LPWORD)GXBeginDraw())
		vm->draw_screen();
	GXEndDraw();
#else
	// draw screen if required
	vm->draw_screen();
	
	// record picture
#ifndef _WIN32_WCE
	if(now_recv) {
		if(AVIStreamWrite(pAVICompressed, rec_frames++, 1, 
		                  (LPBYTE)lpBMP, 
		                  SCREEN_WIDTH * SCREEN_HEIGHT * 2, AVIIF_KEYFRAME, NULL, NULL) != AVIERR_OK)
			stop_rec_video();
	}
#endif
	// invalidate window
	InvalidateRect(main_window_handle, NULL, FALSE);
	UpdateWindow(main_window_handle);
#endif
}

void EMU::update_screen(HDC hdc)
{
#ifndef _USE_GAPI
	if(power_x == 1 && power_y == 1)
		BitBlt(hdc, dest_x, dest_y, SCREEN_WIDTH, SCREEN_HEIGHT, hdcDIB, 0, 0, SRCCOPY);
	else
		StretchBlt(hdc, dest_x, dest_y, SCREEN_WIDTH * power_x, SCREEN_HEIGHT * power_y,
		           hdcDIB, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SRCCOPY);
#endif
}

uint16* EMU::screen_buffer(int y)
{
#ifdef _USE_GAPI
	return lpGapi + window_width * (dest_y + y) + dest_x;
#else
	return lpBMP + SCREEN_WIDTH * (SCREEN_HEIGHT - y - 1);
#endif
}

void EMU::start_rec_video(int fps, bool show_dialog)
{
#ifndef _WIN32_WCE
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	application_path(app_path);
	_stprintf(file_path, _T("%svideo.avi"), app_path);
	
	// init vfw
	AVIFileInit();
	if(AVIFileOpen(&pAVIFile, file_path, OF_WRITE | OF_CREATE, NULL) != AVIERR_OK)
		return;
	
	// stream header
	AVISTREAMINFO strhdr;
	_memset(&strhdr, 0, sizeof(strhdr));
	strhdr.fccType = streamtypeVIDEO;	// vids
	strhdr.fccHandler = 0;
	strhdr.dwScale = 1;
	strhdr.dwRate = fps;
	strhdr.dwSuggestedBufferSize = SCREEN_WIDTH * SCREEN_HEIGHT * 2;
	SetRect(&strhdr.rcFrame, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	if(AVIFileCreateStream(pAVIFile, &pAVIStream, &strhdr) != AVIERR_OK) {
		stop_rec_video();
		return;
	}
	
	// compression
	AVICOMPRESSOPTIONS FAR * pOpts[1];
	pOpts[0] = &opts;
	if(show_dialog && !AVISaveOptions(main_window_handle, ICMF_CHOOSE_KEYFRAME | ICMF_CHOOSE_DATARATE, 1, &pAVIStream, (LPAVICOMPRESSOPTIONS FAR *)&pOpts)) {
		AVISaveOptionsFree(1, (LPAVICOMPRESSOPTIONS FAR *)&pOpts);
		stop_rec_video();
		return;
	}
	if(AVIMakeCompressedStream(&pAVICompressed, pAVIStream, &opts, NULL) != AVIERR_OK) {
		stop_rec_video();
		return;
	}
	if(AVIStreamSetFormat(pAVICompressed, 0, &lpDIB->bmiHeader, lpDIB->bmiHeader.biSize + lpDIB->bmiHeader.biClrUsed * sizeof(RGBQUAD)) != AVIERR_OK) {
		stop_rec_video();
		return;
	}
	rec_frames = 0;
	rec_fps = fps;
	now_recv = true;
#endif
}

void EMU::stop_rec_video()
{
#ifndef _WIN32_WCE
	// release vfw
	if(pAVIStream)
		AVIStreamClose(pAVIStream);
	if(pAVICompressed)
		AVIStreamClose(pAVICompressed);
	if(pAVIFile) {
		AVIFileClose(pAVIFile);
		AVIFileExit();
	}
	pAVIStream = NULL;
	pAVICompressed = NULL;
	pAVIFile = NULL;
	
	// repair header
	if(now_recv) {
		_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
		application_path(app_path);
		_stprintf(file_path, _T("%svideo.avi"), app_path);
		
		FILE* fp = _tfopen(file_path, _T("r+b"));
		if(fp != NULL) {
			// copy fccHandler
			uint8 buf[4];
			fseek(fp, 0xbc, SEEK_SET);
			if(ftell(fp) == 0xbc) {
				fread(buf, 4, 1, fp);
				fseek(fp, 0x70, SEEK_SET);
				fwrite(buf, 4, 1, fp);
			}
			fclose(fp);
		}
	}
	now_recv = false;
#endif
}

void EMU::restart_rec_video()
{
#ifndef _WIN32_WCE
	if(now_recv) {
		// release vfw
		if(pAVIStream)
			AVIStreamClose(pAVIStream);
		if(pAVICompressed)
			AVIStreamClose(pAVICompressed);
		if(pAVIFile) {
			AVIFileClose(pAVIFile);
			AVIFileExit();
		}
		pAVIStream = NULL;
		pAVICompressed = NULL;
		pAVIFile = NULL;
		
		start_rec_video(rec_fps, false);
	}
#endif
}

