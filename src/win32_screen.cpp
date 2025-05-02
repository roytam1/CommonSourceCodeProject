/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 screen ]
*/

#include "emu.h"
#include "vm/vm.h"
#include "config.h"

void EMU::initialize_screen()
{
	screen_width = SCREEN_WIDTH;
	screen_height = SCREEN_HEIGHT;
	screen_width_aspect = SCREEN_WIDTH_ASPECT;
	screen_height_aspect = SCREEN_HEIGHT_ASPECT;
	window_width = WINDOW_WIDTH;
	window_height = WINDOW_HEIGHT;
	
	stretch_pow_x = stretch_pow_y = -1;
	stretch_screen = FALSE;
	
	// create dib sections
	HDC hdc = GetDC(main_window_handle);
	create_dib_section(hdc, screen_width, screen_height, &hdcDib, &hBmp, &lpBuf, &lpBmp, &lpDib);
	
#ifdef USE_SCREEN_ROTATE
	create_dib_section(hdc, screen_height, screen_width, &hdcDibRotate, &hBmpRotate, &lpBufRotate, &lpBmpRotate, &lpDibRotate);
#endif
	
	hdcDibStretch1 = hdcDibStretch2 = NULL;
	hBmpStretch1 = hBmpStretch2 = NULL;
	lpBufStretch1 = lpBufStretch2 = NULL;
	
	// initialize video recording
	now_rec_vid = FALSE;
	pAVIStream = NULL;
	pAVICompressed = NULL;
	pAVIFile = NULL;
	
	// initialize update flags
	first_draw_screen = FALSE;
	first_invalidate = self_invalidate = FALSE;
}

#define release_dib_section(hdcdib, hbmp, lpbuf) { \
	if(hdcdib != NULL) { \
		DeleteDC(hdcdib); \
		hdcdib = NULL; \
	} \
	if(hbmp != NULL) { \
		DeleteObject(hbmp); \
		hbmp = NULL; \
	} \
	if(lpbuf != NULL) { \
		GlobalFree(lpbuf); \
		lpbuf = NULL; \
	} \
}

void EMU::release_screen()
{
	// stop video recording
	stop_rec_video();
	
	// release dib sections
	release_dib_section(hdcDib, hBmp, lpBuf);
#ifdef USE_SCREEN_ROTATE
	release_dib_section(hdcDibRotate, hBmpRotate, lpBufRotate);
#endif
	release_dib_section(hdcDibStretch1, hBmpStretch1, lpBufStretch1);
	release_dib_section(hdcDibStretch2, hBmpStretch2, lpBufStretch2);
}

void EMU::create_dib_section(HDC hdc, int width, int height, HDC *hdcDib, HBITMAP *hBmp, LPBYTE *lpBuf, scrntype **lpBmp, LPBITMAPINFO *lpDib)
{
	*lpBuf = (LPBYTE)GlobalAlloc(GPTR, sizeof(BITMAPINFO));
	*lpDib = (LPBITMAPINFO)(*lpBuf);
	memset(&(*lpDib)->bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	(*lpDib)->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	(*lpDib)->bmiHeader.biWidth = width;
	(*lpDib)->bmiHeader.biHeight = height;
	(*lpDib)->bmiHeader.biPlanes = 1;
#if defined(_RGB555)
	(*lpDib)->bmiHeader.biBitCount = 16;
	(*lpDib)->bmiHeader.biCompression = BI_RGB;
	(*lpDib)->bmiHeader.biSizeImage = width * height * 2;
#elif defined(_RGB565)
	(*lpDib)->bmiHeader.biBitCount = 16;
	(*lpDib)->bmiHeader.biCompression = BI_BITFIELDS;
	LPDWORD lpBf = (LPDWORD)*lpDib->bmiColors;
	lpBf[0] = 0x1f << 11;
	lpBf[1] = 0x3f << 5;
	lpBf[2] = 0x1f << 0;
	(*lpDib)->bmiHeader.biSizeImage = width * height * 2;
#elif defined(_RGB888)
	(*lpDib)->bmiHeader.biBitCount = 32;
	(*lpDib)->bmiHeader.biCompression = BI_RGB;
	(*lpDib)->bmiHeader.biSizeImage = width * height * 4;
#endif
	(*lpDib)->bmiHeader.biXPelsPerMeter = 0;
	(*lpDib)->bmiHeader.biYPelsPerMeter = 0;
	(*lpDib)->bmiHeader.biClrUsed = 0;
	(*lpDib)->bmiHeader.biClrImportant = 0;
	*hBmp = CreateDIBSection(hdc, *lpDib, DIB_RGB_COLORS, (PVOID*)&(*lpBmp), NULL, 0);
	*hdcDib = CreateCompatibleDC(hdc);
	SelectObject(*hdcDib, *hBmp);
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
	BOOL stretch_changed = FALSE;
	
	if(width != -1 && (display_width != width || display_height != height)) {
		display_width = width;
		display_height = height;
		stretch_changed = true;
	}
	
	int source_width_aspect, source_height_aspect;
#ifdef USE_SCREEN_ROTATE
	if(config.monitor_type) {
		hdcDibSource = hdcDibRotate;
		lpBmpSource = lpBmpRotate;
		lpDibSource = lpDibRotate;
		pbmInfoHeader = &lpDibRotate->bmiHeader;
		
		source_width = screen_height;
		source_height = screen_width;
		source_width_aspect = screen_height_aspect;
		source_height_aspect = screen_width_aspect;
	}
	else {
#endif
		hdcDibSource = hdcDib;
		lpBmpSource = lpBmp;
		lpDibSource = lpDib;
		pbmInfoHeader = &lpDib->bmiHeader;
		
		source_width = screen_width;
		source_height = screen_height;
		source_width_aspect = screen_width_aspect;
		source_height_aspect = screen_height_aspect;
#ifdef USE_SCREEN_ROTATE
	}
#endif
	
	int new_pow_x = 1, new_pow_y = 1;
	if(!window_mode && config.stretch_screen) {
		// fit to full screen
		stretched_width = (display_height * source_width_aspect) / source_height_aspect;
		stretched_height = display_height;
		if(stretched_width > display_width) {
			stretched_width = display_width;
			stretched_height = (display_width * source_height_aspect) / source_width_aspect;
		}
#if 0
		// NOTE: high quality but too slow
		while(stretched_width > source_width * new_pow_x) {
			new_pow_x++;
		}
		while(stretched_height > source_height * new_pow_y) {
			new_pow_y++;
		}
		if(stretched_width == source_width * new_pow_x && stretched_height == source_height * new_pow_y) {
			new_pow_x = new_pow_y = 1;
		}
#endif
	}
	else {
		int tmp_pow_x = display_width / source_width_aspect;
		int tmp_pow_y = display_height / source_height_aspect;
		int tmp_pow = 1;
		if(tmp_pow_y >= tmp_pow_x && tmp_pow_x > 1) {
			tmp_pow = tmp_pow_x;
		}
		else if(tmp_pow_x >= tmp_pow_y && tmp_pow_y > 1) {
			tmp_pow = tmp_pow_y;
		}
		stretched_width = source_width_aspect * tmp_pow;
		stretched_height = source_height_aspect * tmp_pow;
		
#if !(SCREEN_WIDTH_ASPECT == SCREEN_WIDTH && SCREEN_HEIGHT_ASPECT == SCREEN_HEIGHT)
		while(stretched_width > source_width * new_pow_x) {
			new_pow_x++;
		}
		while(stretched_height > source_height * new_pow_y) {
			new_pow_y++;
		}
		if(new_pow_x > 1 && new_pow_y > 1) {
			new_pow_x = new_pow_y = 1;
		}
#endif
	}
	screen_dest_x = (display_width - stretched_width) / 2;
	screen_dest_y = (display_height - stretched_height) / 2;
	
	if(stretch_pow_x != new_pow_x || stretch_pow_y != new_pow_y) {
		stretch_pow_x = new_pow_x;
		stretch_pow_y = new_pow_y;
		stretch_changed = TRUE;
	}
	
	if(stretch_changed) {
		release_dib_section(hdcDibStretch1, hBmpStretch1, lpBufStretch1);
		release_dib_section(hdcDibStretch2, hBmpStretch2, lpBufStretch2);
		stretch_screen = FALSE;
		
		if(stretch_pow_x != 1 || stretch_pow_y != 1) {
			HDC hdc = GetDC(main_window_handle);
			create_dib_section(hdc, source_width * stretch_pow_x, source_height * stretch_pow_y, &hdcDibStretch1, &hBmpStretch1, &lpBufStretch1, &lpBmpStretch1, &lpDibStretch1);
			SetStretchBltMode(hdcDibStretch1, COLORONCOLOR);
			create_dib_section(hdc, stretched_width, stretched_height, &hdcDibStretch2, &hBmpStretch2, &lpBufStretch2, &lpBmpStretch2, &lpDibStretch2);
			SetStretchBltMode(hdcDibStretch2, HALFTONE);
			stretch_screen = TRUE;
		}
	}
	
	first_draw_screen = FALSE;
	first_invalidate = TRUE;
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
		
		// re-create dib sections
		HDC hdc = GetDC(main_window_handle);
		release_dib_section(hdcDib, hBmp, lpBuf);
		create_dib_section(hdc, screen_width, screen_height, &hdcDib, &hBmp, &lpBuf, &lpBmp, &lpDib);
		
#ifdef USE_SCREEN_ROTATE
		release_dib_section(hdcDibRotate, hBmpRotate, lpBufRotate);
		create_dib_section(hdc, screen_height, screen_width, &hdcDibRotate, &hBmpRotate, &lpBufRotate, &lpBmpRotate, &lpDibRotate);
#endif
		
		// stop recording
		if(now_rec_vid) {
			stop_rec_video();
			stop_rec_sound();
		}
		
		// change the window size
		PostMessage(main_window_handle, WM_RESIZE, 0L, 0L);
	}
}

void EMU::draw_screen()
{
	// draw screen
	vm->draw_screen();
	
#ifdef USE_SCREEN_ROTATE
	// rotate screen
	if(config.monitor_type) {
		for(int y = 0; y < screen_height; y++) {
			scrntype* src = lpBmp + screen_width * (screen_height - y - 1);
			scrntype* out = lpBmpRotate + screen_height * (screen_width - 1) + (screen_height - y - 1);
			for(int x = 0; x < screen_width; x++) {
				*out = src[x];
				out -= screen_height;
			}
		}
	}
#endif	
	
	// stretch screen
	if(stretch_screen) {
#if 0
		StretchBlt(hdcDibStretch1, 0, 0, source_width * stretch_pow_x, source_height * stretch_pow_y, hdcDibSource, 0, 0, source_width, source_height, SRCCOPY);
#else		
		scrntype* src = lpBmpSource + source_width * (source_height - 1);
		scrntype* out = lpBmpStretch1 + source_width * stretch_pow_x * (source_height * stretch_pow_y - 1);
		
		int data_len = source_width * stretch_pow_x;
		int data_size = sizeof(scrntype) * data_len;
		
		for(int y = 0; y < source_height; y++) {
			if(stretch_pow_x != 1) {
				for(int x = 0, dx = 0; x < source_width; x++) {
					scrntype c = src[x];
					for(int px = 0; px < stretch_pow_x; px++) {
						out[dx + px] = c;
					}
					dx += stretch_pow_x;
				}
			}
			else {
				memcpy(out, src, data_size);
			}
			if(stretch_pow_y != 1) {
				scrntype* src_tmp = out;
				for(int py = 1; py < stretch_pow_y; py++) {
					out -= data_len;
					memcpy(out, src_tmp, data_size);
				}
			}
			src -= source_width;
			out -= data_len;
		}
#endif
		StretchBlt(hdcDibStretch2, 0, 0, stretched_width, stretched_height, hdcDibStretch1, 0, 0, source_width * stretch_pow_x, source_height * stretch_pow_y, SRCCOPY);
	}
	first_draw_screen = TRUE;
	
	// invalidate window
	InvalidateRect(main_window_handle, NULL, first_invalidate);
	UpdateWindow(main_window_handle);
	self_invalidate = TRUE;
	
	// record picture
	if(now_rec_vid) {
		if(AVIStreamWrite(pAVICompressed, rec_frames++, 1, (LPBYTE)lpBmpSource, pbmInfoHeader->biSizeImage, AVIIF_KEYFRAME, NULL, NULL) != AVIERR_OK) {
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
		if(stretch_screen) {
			BitBlt(hdc, screen_dest_x, screen_dest_y, stretched_width, stretched_height, hdcDibStretch2, 0, 0, SRCCOPY);
		}
		else {
			if(stretched_width == source_width && stretched_height == source_height) {
				BitBlt(hdc, screen_dest_x, screen_dest_y, stretched_width, stretched_height, hdcDibSource, 0, 0, SRCCOPY);
			}
			else {
				StretchBlt(hdc, screen_dest_x, screen_dest_y, stretched_width, stretched_height, hdcDibSource, 0, 0, source_width, source_height, SRCCOPY);
			}
		}
#endif
#ifdef USE_ACCESS_LAMP
		int status = vm->access_lamp() & 7;
		static int prev_status = 0;
		BOOL render_in = (status != 0);
		BOOL render_out = (prev_status != status);
		prev_status = status;
		
		if(render_in || render_out) {
			COLORREF crColor = RGB((status & 1) ? 255 : 0, (status & 2) ? 255 : 0, (status & 4) ? 255 : 0);
			int right_bottom_x = screen_dest_x + stretched_width;
			int right_bottom_y = screen_dest_y + stretched_height;
			
			for(int y = display_height - 6; y < display_height; y++) {
				for(int x = display_width - 6; x < display_width; x++) {
					if((x < right_bottom_x && y < right_bottom_y) ? render_in : render_out) {
						SetPixelV(hdc, x, y, crColor);
					}
				}
			}
		}
#endif
		first_invalidate = self_invalidate = FALSE;
	}
}

scrntype* EMU::screen_buffer(int y)
{
	return lpBmp + screen_width * (screen_height - y - 1);
}

void EMU::capture_screen()
{
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	application_path(app_path);
	_stprintf(file_path, _T("%s%d-%d-%d_%d-%d-%d.bmp"), app_path, sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wSecond);
	
	BITMAPFILEHEADER bmFileHeader = { (WORD)(TEXT('B') | TEXT('M') << 8) };
	bmFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmFileHeader.bfSize = bmFileHeader.bfOffBits + pbmInfoHeader->biSizeImage;
	
	DWORD dwSize;
	HANDLE hFile = CreateFile(file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(hFile, &bmFileHeader, sizeof(BITMAPFILEHEADER), &dwSize, NULL);
	WriteFile(hFile, lpDibSource, sizeof(BITMAPINFOHEADER), &dwSize, NULL);
	WriteFile(hFile, lpBmpSource, pbmInfoHeader->biSizeImage, &dwSize, NULL);
	CloseHandle(hFile);
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
	memset(&strhdr, 0, sizeof(strhdr));
	strhdr.fccType = streamtypeVIDEO;	// vids
	strhdr.fccHandler = 0;
	strhdr.dwScale = 1;
	strhdr.dwRate = fps;
	strhdr.dwSuggestedBufferSize = pbmInfoHeader->biSizeImage;
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
	if(AVIStreamSetFormat(pAVICompressed, 0, &lpDibSource->bmiHeader, lpDibSource->bmiHeader.biSize + lpDibSource->bmiHeader.biClrUsed * sizeof(RGBQUAD)) != AVIERR_OK) {
		stop_rec_video();
		return;
	}
	rec_frames = 0;
	rec_fps = fps;
	now_rec_vid = TRUE;
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
	if(now_rec_vid) {
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
	now_rec_vid = FALSE;
}

void EMU::restart_rec_video()
{
	if(now_rec_vid) {
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

