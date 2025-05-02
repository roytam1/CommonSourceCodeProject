/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 screen ]
*/

#include "emu.h"
#include "vm/vm.h"
#include "config.h"

#define SCREEN_BUFFER_RENDER	0
#define SCREEN_BUFFER_ROTATE	1
#define SCREEN_BUFFER_STRETCH	2

void EMU::initialize_screen()
{
	screen_width = SCREEN_WIDTH;
	screen_height = SCREEN_HEIGHT;
	screen_width_aspect = SCREEN_WIDTH_ASPECT;
	screen_height_aspect = SCREEN_HEIGHT_ASPECT;
	window_width = WINDOW_WIDTH;
	window_height = WINDOW_HEIGHT;
	
	// create dib sections
	int buffer1_width = screen_width * 2;
	int buffer1_height = screen_height * 2;
	int buffer2_width = screen_width_aspect;
	int buffer2_height = screen_height_aspect;
#ifdef USE_SCREEN_ROTATE
	if(buffer1_width > buffer1_height) {
		buffer1_height = buffer1_width;
	}
	else {
		buffer1_width = buffer1_height;
	}
	if(buffer2_width > buffer2_height) {
		buffer2_height = buffer2_width;
	}
	else {
		buffer2_width = buffer2_height;
	}
#endif
	HDC hdc = GetDC(main_window_handle);
	create_dib_section(hdc, screen_width, screen_height, &hdcDib, &hBmp, &lpBuf, &lpBmp, &lpDib);
#ifdef USE_SCREEN_ROTATE
	create_dib_section(hdc, screen_height, screen_width, &hdcDibRotate, &hBmpRotate, &lpBufRotate, &lpBmpRotate, &lpDibRotate);
#endif
#ifdef USE_SCREEN_STRETCH_HIGH_QUALITY
	create_dib_section(hdc, buffer1_width, buffer1_height, &hdcDibStretch1, &hBmpStretch1, &lpBufStretch1, &lpBmpStretch1, &lpDibStretch1);
	create_dib_section(hdc, buffer2_width, buffer2_height, &hdcDibStretch2, &hBmpStretch2, &lpBufStretch2, &lpBmpStretch2, &lpDibStretch2);
	SetStretchBltMode(hdcDibStretch2, HALFTONE);
#endif
	
	// initialize video recording
	now_recv = FALSE;
	pAVIStream = NULL;
	pAVICompressed = NULL;
	pAVIFile = NULL;
	
	// initialize update flags
	first_draw_screen = FALSE;
	first_invalidate = self_invalidate = FALSE;
}

void EMU::release_screen()
{
	// stop video recording
	stop_rec_video();
	
	// release dib sections
	release_dib_section(&hdcDib, &hBmp, &lpBuf);
#ifdef USE_SCREEN_ROTATE
	release_dib_section(&hdcDibRotate, &hBmpRotate, &lpBufRotate);
#endif
#ifdef USE_SCREEN_STRETCH_HIGH_QUALITY
	release_dib_section(&hdcDibStretch1, &hBmpStretch1, &lpBufStretch1);
	release_dib_section(&hdcDibStretch2, &hBmpStretch2, &lpBufStretch2);
#endif
}

void EMU::create_dib_section(HDC hdc, int width, int height, HDC *hdcDib, HBITMAP *hBmp, LPBYTE *lpBuf, scrntype **lpBmp, LPBITMAPINFO *lpDib)
{
	*lpBuf = (LPBYTE)GlobalAlloc(GPTR, sizeof(BITMAPINFO));
	*lpDib = (LPBITMAPINFO)(*lpBuf);
	(*lpDib)->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	(*lpDib)->bmiHeader.biWidth = width;
	(*lpDib)->bmiHeader.biHeight = height;
	(*lpDib)->bmiHeader.biPlanes = 1;
#if defined(_RGB555)
	(*lpDib)->bmiHeader.biBitCount = 16;
	(*lpDib)->bmiHeader.biCompression = BI_RGB;
#elif defined(_RGB565)
	(*lpDib)->bmiHeader.biBitCount = 16;
	(*lpDib)->bmiHeader.biCompression = BI_BITFIELDS;
	LPDWORD lpBf = (LPDWORD)*lpDib->bmiColors;
	lpBf[0] = 0x1f << 11;
	lpBf[1] = 0x3f << 5;
	lpBf[2] = 0x1f << 0;
#elif defined(_RGB888)
	(*lpDib)->bmiHeader.biBitCount = 32;
	(*lpDib)->bmiHeader.biCompression = BI_RGB;
#endif
	(*lpDib)->bmiHeader.biSizeImage = 0;
	(*lpDib)->bmiHeader.biXPelsPerMeter = 0;
	(*lpDib)->bmiHeader.biYPelsPerMeter = 0;
	(*lpDib)->bmiHeader.biClrUsed = 0;
	(*lpDib)->bmiHeader.biClrImportant = 0;
	*hBmp = CreateDIBSection(hdc, *lpDib, DIB_RGB_COLORS, (PVOID*)&(*lpBmp), NULL, 0);
	*hdcDib = CreateCompatibleDC(hdc);
	SelectObject(*hdcDib, *hBmp);
}

void EMU::release_dib_section(HDC *hdcDib, HBITMAP *hBmp, LPBYTE *lpBuf)
{
	DeleteDC(*hdcDib);
	DeleteObject(*hBmp);
	GlobalFree(*lpBuf);
}

int EMU::get_window_width(int mode)
{
#ifdef USE_SCREEN_ROTATE
	if(config.monitor_type) {
		return window_height + screen_height_aspect * mode;
	}
#endif
	return window_width + screen_width_aspect * mode;
}

int EMU::get_window_height(int mode) {
#ifdef USE_SCREEN_ROTATE
	if(config.monitor_type) {
		return window_width + screen_width_aspect * mode;
	}
#endif
	return window_height + screen_height_aspect * mode;
}

void EMU::set_display_size(int width, int height, BOOL window_mode)
{
	if(width != -1) {
		display_width = width;
		display_height = height;
	}
	int src_width = screen_width;
	int src_height = screen_height;
	int src_width_aspect = screen_width_aspect;
	int src_height_aspect = screen_height_aspect;
#ifdef USE_SCREEN_ROTATE
	if(config.monitor_type) {
		src_width = screen_height;
		src_height = screen_width;
		src_width_aspect = screen_height_aspect;
		src_height_aspect = screen_width_aspect;
	}
#endif
	if(window_mode || config.stretch_screen) {
		stretch_width = (display_height * src_width_aspect) / src_height_aspect;
		stretch_height = display_height;
		if(stretch_width > display_width) {
			stretch_width = display_width;
			stretch_height = (display_width * src_height_aspect) / src_width_aspect;
		}
	}
	else {
		int power_x = display_width / src_width_aspect;
		int power_y = display_height / src_height_aspect;
		power_x = (power_x < 1) ? 1 : power_x;
		power_y = (power_y < 1) ? 1 : power_y;
		if(power_x < power_y) {
			power_y = power_x;
		}
		else {
			power_x = power_y;
		}
		power_x = (power_x * src_width_aspect) / src_width;
		power_y = (power_y * src_height_aspect) / src_height;
		stretch_width = src_width * power_x;
		stretch_height = src_height * power_y;
	}
	dest_x = (display_width - stretch_width) / 2;
	dest_y = (display_height - stretch_height) / 2;
	
	stretch_screen = !(stretch_width == screen_width && stretch_height == screen_height);
#ifdef USE_SCREEN_STRETCH_HIGH_QUALITY
	if(window_mode) {
		stretch_screen_high_quality = !((stretch_width % screen_width) == 0 && (stretch_height % screen_height) == 0);
	}
	else {
		stretch_screen_high_quality = FALSE;
	}
#endif
	first_draw_screen = FALSE;
	first_invalidate = TRUE;
}

void EMU::draw_screen()
{
	// draw screen
	vm->draw_screen();
	source_buffer = SCREEN_BUFFER_RENDER;
	
#ifdef USE_SCREEN_ROTATE
	// rotate screen
	if(config.monitor_type) {
		for(int y = 0; y < screen_height; y++) {
			scrntype* src = lpBmp + screen_width * (screen_height - y - 1);
			//scrntype* out = lpBmpRotate + rotate_width * (rotate_height - 1) + (screen_height - y - 1);
			scrntype* out = lpBmpRotate + screen_height * (screen_width - 1) + (screen_height - y - 1);
			for(int x = 0; x < screen_width; x++) {
				*out = src[x];
				//out -= rotate_width;
				out -= screen_height;
			}
		}
		source_buffer = SCREEN_BUFFER_ROTATE;
	}
#endif
#ifdef USE_SCREEN_STRETCH_HIGH_QUALITY
	// stretch screen
	if(stretch_screen_high_quality) {
		int src_width = screen_width;
		int src_height = screen_height;
		HDC hdcSrc = hdcDib;
#ifdef USE_SCREEN_ROTATE
		if(config.monitor_type) {
			src_width = screen_height;
			src_height = screen_width;
			hdcSrc = hdcDibRotate;
		}
#endif
		StretchBlt(hdcDibStretch1, 0, 0, src_width * 2, src_height * 2, hdcSrc, 0, 0, src_width, src_height, SRCCOPY);
		StretchBlt(hdcDibStretch2, 0, 0, stretch_width, stretch_height, hdcDibStretch1, 0, 0, src_width * 2, src_height * 2, SRCCOPY);
		source_buffer = SCREEN_BUFFER_STRETCH;
	}
#endif
	first_draw_screen = TRUE;
	
	// invalidate window
	InvalidateRect(main_window_handle, NULL, first_invalidate);
	UpdateWindow(main_window_handle);
	self_invalidate = TRUE;
	
	// record picture
	if(now_recv) {
#ifdef _RGB888
		if(AVIStreamWrite(pAVICompressed, rec_frames++, 1, (LPBYTE)lpBmp, screen_width * screen_height * 4, AVIIF_KEYFRAME, NULL, NULL) != AVIERR_OK) {
#else
		if(AVIStreamWrite(pAVICompressed, rec_frames++, 1, (LPBYTE)lpBmp, screen_width * screen_height * 2, AVIIF_KEYFRAME, NULL, NULL) != AVIERR_OK) {
#endif
			stop_rec_video();
		}
	}
}

void EMU::update_screen(HDC hdc)
{
#ifdef USE_BITMAP
	if(first_invalidate || !self_invalidate) {
		HBITMAP hBitmap = LoadBitmap(instance_handle, _T("IDI_BITMAP1"));
		BITMAP bmp;
		GetObject(hBitmap, sizeof(BITMAP), &bmp);
		int w = (int)bmp.bmWidth;
		int h = (int)bmp.bmHeight;
		HDC hmdc = CreateCompatibleDC(hdc);
		SelectObject(hmdc, hBitmap);
		BitBlt(hdc, 0, 0, w, h, hmdc, 0, 0, SRCCOPY);
		DeleteDC(hmdc);
		DeleteObject(hBitmap);
	}
#endif
	if(first_draw_screen) {
#ifdef USE_LED
		for(int i = 0; i < MAX_LEDS; i++) {
			int x = leds[i].x;
			int y = leds[i].y;
			int w = leds[i].width;
			int h = leds[i].height;
			BitBlt(hdc, x, y, w, h, hdcDib, x, y, SRCCOPY);
		}
#else
		if(source_buffer == SCREEN_BUFFER_RENDER) {
			if(stretch_screen) {
				StretchBlt(hdc, dest_x, dest_y, stretch_width, stretch_height, hdcDib, 0, 0, screen_width, screen_height, SRCCOPY);
			}
			else {
				BitBlt(hdc, dest_x, dest_y, stretch_width, stretch_height, hdcDib, 0, 0, SRCCOPY);
			}
		}
#ifdef USE_SCREEN_ROTATE
		else if(source_buffer == SCREEN_BUFFER_ROTATE) {
			if(stretch_screen) {
				StretchBlt(hdc, dest_x, dest_y, stretch_width, stretch_height, hdcDibRotate, 0, 0, screen_height, screen_width, SRCCOPY);
			}
			else {
				BitBlt(hdc, dest_x, dest_y, stretch_width, stretch_height, hdcDibRotate, 0, 0, SRCCOPY);
			}
		}
#endif
#ifdef USE_SCREEN_STRETCH_HIGH_QUALITY
		else if(source_buffer == SCREEN_BUFFER_STRETCH) {
			BitBlt(hdc, dest_x, dest_y, stretch_width, stretch_height, hdcDibStretch2, 0, 0, SRCCOPY);
		}
#endif
#endif
		first_invalidate = self_invalidate = FALSE;
	}
}

scrntype* EMU::screen_buffer(int y)
{
	return lpBmp + screen_width * (screen_height - y - 1);
}

void EMU::change_screen_size(int sw, int sh, int swa, int sha, int ww, int wh)
{
	// virtual machine changes the screen size
	if(screen_width != sw || screen_height != sh) {
		screen_width = sw;
		screen_height = sh;
		screen_width_aspect = (swa != -1) ? swa : sw;
		screen_height_aspect = (sha != -1) ? sha : sh;
		window_width = ww;
		window_height = wh;
		
		// update screen buffer size
		int buffer1_width = screen_width * 2;
		int buffer1_height = screen_height * 2;
		int buffer2_width = screen_width_aspect;
		int buffer2_height = screen_height_aspect;
#ifdef USE_SCREEN_ROTATE
		if(buffer1_width > buffer1_height) {
			buffer1_height = buffer1_width;
		}
		else {
			buffer1_width = buffer1_height;
		}
		if(buffer2_width > buffer2_height) {
			buffer2_height = buffer2_width;
		}
		else {
			buffer2_width = buffer2_height;
		}
#endif
		
		// re-create dib sections
		release_dib_section(&hdcDib, &hBmp, &lpBuf);
#ifdef USE_SCREEN_ROTATE
		release_dib_section(&hdcDibRotate, &hBmpRotate, &lpBufRotate);
#endif
#ifdef USE_SCREEN_STRETCH_HIGH_QUALITY
		release_dib_section(&hdcDibStretch1, &hBmpStretch1, &lpBufStretch1);
		release_dib_section(&hdcDibStretch2, &hBmpStretch2, &lpBufStretch2);
#endif
		
		HDC hdc = GetDC(main_window_handle);
		create_dib_section(hdc, screen_width, screen_height, &hdcDib, &hBmp, &lpBuf, &lpBmp, &lpDib);
#ifdef USE_SCREEN_ROTATE
		create_dib_section(hdc, screen_height, screen_width, &hdcDibRotate, &hBmpRotate, &lpBufRotate, &lpBmpRotate, &lpDibRotate);
#endif
#ifdef USE_SCREEN_STRETCH_HIGH_QUALITY
		create_dib_section(hdc, buffer1_width, buffer1_height, &hdcDibStretch1, &hBmpStretch1, &lpBufStretch1, &lpBmpStretch1, &lpDibStretch1);
		create_dib_section(hdc, buffer2_width, buffer2_height, &hdcDibStretch2, &hBmpStretch2, &lpBufStretch2, &lpBmpStretch2, &lpDibStretch2);
		SetStretchBltMode(hdcDibStretch2, HALFTONE);
#endif
		
		// stop recording
		if(now_recv) {
			stop_rec_video();
			stop_rec_sound();
		}
		
		// change the window size
		PostMessage(main_window_handle, WM_RESIZE, 0L, 0L);
	}
}

void EMU::start_rec_video(int fps, BOOL show_dialog)
{
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	application_path(app_path);
	_stprintf(file_path, _T("%svideo.avi"), app_path);
	
	// initialize vfw
	AVIFileInit();
	if(AVIFileOpen(&pAVIFile, file_path, OF_WRITE | OF_CREATE, NULL) != AVIERR_OK) {
		return;
	}
	
	// stream header
	AVISTREAMINFO strhdr;
	_memset(&strhdr, 0, sizeof(strhdr));
	strhdr.fccType = streamtypeVIDEO;	// vids
	strhdr.fccHandler = 0;
	strhdr.dwScale = 1;
	strhdr.dwRate = fps;
#ifdef _RGB888
	strhdr.dwSuggestedBufferSize = screen_width * screen_height * 4;
#else
	strhdr.dwSuggestedBufferSize = screen_width * screen_height * 2;
#endif
	SetRect(&strhdr.rcFrame, 0, 0, screen_width, screen_height);
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
	if(AVIStreamSetFormat(pAVICompressed, 0, &lpDib->bmiHeader, lpDib->bmiHeader.biSize + lpDib->bmiHeader.biClrUsed * sizeof(RGBQUAD)) != AVIERR_OK) {
		stop_rec_video();
		return;
	}
	rec_frames = 0;
	rec_fps = fps;
	now_recv = TRUE;
}

void EMU::stop_rec_video()
{
	// release vfw
	if(pAVIStream) {
		AVIStreamClose(pAVIStream);
	}
	if(pAVICompressed) {
		AVIStreamClose(pAVICompressed);
	}
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
	now_recv = FALSE;
}

void EMU::restart_rec_video()
{
	if(now_recv) {
		// release vfw
		if(pAVIStream) {
			AVIStreamClose(pAVIStream);
		}
		if(pAVICompressed) {
			AVIStreamClose(pAVICompressed);
		}
		if(pAVIFile) {
			AVIFileClose(pAVIFile);
			AVIFileExit();
		}
		pAVIStream = NULL;
		pAVICompressed = NULL;
		pAVIFile = NULL;
		
		start_rec_video(rec_fps, FALSE);
	}
}

