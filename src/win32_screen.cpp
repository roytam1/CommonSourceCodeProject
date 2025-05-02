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
	screen_width = SCREEN_WIDTH;
	screen_height = SCREEN_HEIGHT;
	screen_width_aspect = SCREEN_WIDTH_ASPECT;
	screen_height_aspect = SCREEN_HEIGHT_ASPECT;
	window_width = WINDOW_WIDTH;
	window_height = WINDOW_HEIGHT;
	screen_size_changed = true;
	
	source_width = source_height = -1;
	source_width_aspect = source_height_aspect = -1;
	stretch_pow_x = stretch_pow_y = -1;
	stretch_screen = false;
	
	// create dib sections
	HDC hdc = GetDC(main_window_handle);
	create_dib_section(hdc, screen_width, screen_height, &hdcDib, &hBmp, &hOldBmp, &lpBuf, &lpBmp, &lpDib);
#ifdef USE_SCREEN_ROTATE
	create_dib_section(hdc, screen_height, screen_width, &hdcDibRotate, &hBmpRotate, &hOldBmpRotate, &lpBufRotate, &lpBmpRotate, &lpDibRotate);
#endif
	ReleaseDC(main_window_handle, hdc);
	
	hdcDibStretch1 = hdcDibStretch2 = NULL;
	hBmpStretch1 = hOldBmpStretch1 = hBmpStretch2 = hOldBmpStretch2 = NULL;
	lpBufStretch1 = lpBufStretch2 = NULL;
	
	// initialize d3d9
	lpd3d9 = NULL;
	lpd3d9Device = NULL;
	lpd3d9Surface = NULL;
	lpd3d9OffscreenSurface = NULL;
	lpd3d9Buffer = NULL;
	render_to_d3d9Buffer = false;
	use_d3d9 = config.use_d3d9;
	wait_vsync = config.wait_vsync;
	
	// initialize video recording
	now_rec_vid = false;
	pAVIStream = NULL;
	pAVICompressed = NULL;
	pAVIFile = NULL;
	
	// initialize update flags
	first_draw_screen = false;
	first_invalidate = self_invalidate = false;
}

#define release_dib_section(hdcdib, hbmp, holdbmp, lpbuf) { \
	if(hdcdib != NULL && holdbmp != NULL) { \
		SelectObject(hdcdib, holdbmp); \
	} \
	if(hbmp != NULL) { \
		DeleteObject(hbmp); \
		hbmp = NULL; \
	} \
	if(lpbuf != NULL) { \
		GlobalFree(lpbuf); \
		lpbuf = NULL; \
	} \
	if(hdcdib != NULL) { \
		DeleteDC(hdcdib); \
		hdcdib = NULL; \
	} \
}

#define release_d3d9() { \
	if(lpd3d9OffscreenSurface != NULL) { \
		lpd3d9OffscreenSurface->Release(); \
		lpd3d9OffscreenSurface = NULL; \
	} \
	if(lpd3d9Surface != NULL) { \
		lpd3d9Surface->Release(); \
		lpd3d9Surface = NULL; \
	} \
	if(lpd3d9Device != NULL) { \
		lpd3d9Device->Release(); \
		lpd3d9Device = NULL; \
	} \
	if(lpd3d9 != NULL) { \
		lpd3d9->Release(); \
		lpd3d9 = NULL; \
	} \
}

#define release_d3d9_surface() { \
	if(lpd3d9OffscreenSurface != NULL) { \
		lpd3d9OffscreenSurface->Release(); \
		lpd3d9OffscreenSurface = NULL; \
	} \
	if(lpd3d9Surface != NULL) { \
		lpd3d9Surface->Release(); \
		lpd3d9Surface = NULL; \
	} \
}

void EMU::release_screen()
{
	// stop video recording
	stop_rec_video();
	
	// release dib sections
	release_dib_section(hdcDib, hBmp, hOldBmp, lpBuf);
#ifdef USE_SCREEN_ROTATE
	release_dib_section(hdcDibRotate, hBmpRotate, hOldBmpRotate, lpBufRotate);
#endif
	release_dib_section(hdcDibStretch1, hBmpStretch1, hOldBmpStretch1, lpBufStretch1);
	release_dib_section(hdcDibStretch2, hBmpStretch2, hOldBmpStretch2, lpBufStretch2);
	
	// release d3d9
	release_d3d9();
}

void EMU::create_dib_section(HDC hdc, int width, int height, HDC *hdcDib, HBITMAP *hBmp, HBITMAP *hOldBmp, LPBYTE *lpBuf, scrntype **lpBmp, LPBITMAPINFO *lpDib)
{
	*hdcDib = CreateCompatibleDC(hdc);
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
	*hOldBmp = (HBITMAP)SelectObject(*hdcDib, *hBmp);
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

int EMU::get_window_height(int mode)
{
#ifdef USE_SCREEN_ROTATE
	if(config.monitor_type) {
		return window_width + screen_width_aspect * mode;
	}
#endif
	return window_height + screen_height_aspect * mode;
}

void EMU::set_display_size(int width, int height, bool window_mode)
{
RETRY:
	bool display_size_changed = false;
	bool stretch_changed = false;
	
	if(width != -1 && (display_width != width || display_height != height)) {
		display_width = width;
		display_height = height;
		display_size_changed = stretch_changed = true;
	}
	if(use_d3d9 != config.use_d3d9) {
		if(!(use_d3d9 = config.use_d3d9)) {
			release_d3d9();
		}
		display_size_changed = stretch_changed = true;
	}
	if(wait_vsync != config.wait_vsync) {
		wait_vsync = config.wait_vsync;
		display_size_changed = stretch_changed = true;
	}
	
	// virtual machine renders to d3d9 buffer directly???
	render_to_d3d9Buffer = use_d3d9;
	
#ifdef USE_SCREEN_ROTATE
	if(config.monitor_type) {
		hdcDibSource = hdcDibRotate;
		lpBmpSource = lpBmpRotate;
		lpDibSource = lpDibRotate;
		pbmInfoHeader = &lpDibRotate->bmiHeader;
		
		stretch_changed |= (source_width != screen_height);
		stretch_changed |= (source_height != screen_width);
		stretch_changed |= (source_width_aspect != screen_height_aspect);
		stretch_changed |= (source_height_aspect != screen_width_aspect);
		
		source_width = screen_height;
		source_height = screen_width;
		source_width_aspect = screen_height_aspect;
		source_height_aspect = screen_width_aspect;
		
		render_to_d3d9Buffer = false;
	}
	else {
#endif
		hdcDibSource = hdcDib;
		lpBmpSource = lpBmp;
		lpDibSource = lpDib;
		pbmInfoHeader = &lpDib->bmiHeader;
		
		stretch_changed |= (source_width != screen_width);
		stretch_changed |= (source_height != screen_height);
		stretch_changed |= (source_width_aspect != screen_width_aspect);
		stretch_changed |= (source_height_aspect != screen_height_aspect);
		
		source_width = screen_width;
		source_height = screen_height;
		source_width_aspect = screen_width_aspect;
		source_height_aspect = screen_height_aspect;
#ifdef USE_SCREEN_ROTATE
	}
#endif
	
	if(config.stretch_screen && !window_mode) {
		// fit to full screen
		stretched_width = (display_height * source_width_aspect) / source_height_aspect;
		stretched_height = display_height;
		if(stretched_width > display_width) {
			stretched_width = display_width;
			stretched_height = (display_width * source_height_aspect) / source_width_aspect;
		}
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
	}
	screen_dest_x = (display_width - stretched_width) / 2;
	screen_dest_y = (display_height - stretched_height) / 2;
	
	int new_pow_x = 1, new_pow_y = 1;
	while(stretched_width > source_width * new_pow_x) {
		new_pow_x++;
	}
	while(stretched_height > source_height * new_pow_y) {
		new_pow_y++;
	}
	if(!use_d3d9 && new_pow_x > 1 && new_pow_y > 1) {
		// support high quality stretch only for x1 window size in gdi mode
		new_pow_x = new_pow_y = 1;
	}
	if(stretch_pow_x != new_pow_x || stretch_pow_y != new_pow_y) {
		stretch_pow_x = new_pow_x;
		stretch_pow_y = new_pow_y;
		stretch_changed = true;
	}
	if(stretch_pow_x != 1 || stretch_pow_y != 1) {
		render_to_d3d9Buffer = false;
	}
	
	if(stretch_changed) {
		release_dib_section(hdcDibStretch1, hBmpStretch1, hOldBmpStretch1, lpBufStretch1);
		release_dib_section(hdcDibStretch2, hBmpStretch2, hOldBmpStretch2, lpBufStretch2);
		stretch_screen = false;
		
		if(stretch_pow_x != 1 || stretch_pow_y != 1) {
			HDC hdc = GetDC(main_window_handle);
			create_dib_section(hdc, source_width * stretch_pow_x, source_height * stretch_pow_y, &hdcDibStretch1, &hBmpStretch1, &hOldBmpStretch1, &lpBufStretch1, &lpBmpStretch1, &lpDibStretch1);
			SetStretchBltMode(hdcDibStretch1, COLORONCOLOR);
			if(!use_d3d9) {
				create_dib_section(hdc, stretched_width, stretched_height, &hdcDibStretch2, &hBmpStretch2, &hOldBmpStretch2, &lpBufStretch2, &lpBmpStretch2, &lpDibStretch2);
				SetStretchBltMode(hdcDibStretch2, HALFTONE);
			}
			ReleaseDC(main_window_handle, hdc);
			stretch_screen = true;
		}
		
		if(use_d3d9 && display_size_changed) {
			// release and initialize d3d9
			release_d3d9();
			
			if((lpd3d9 = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) {
				MessageBox(main_window_handle, _T("Failed to initialize Direct3D9"), _T(DEVICE_NAME), MB_OK | MB_ICONWARNING);
				config.use_d3d9 = false;
				goto RETRY;
			}
			else {
				// initialize present params
				D3DPRESENT_PARAMETERS d3dpp;
				ZeroMemory(&d3dpp, sizeof(d3dpp));
				d3dpp.BackBufferWidth = display_width;
				d3dpp.BackBufferHeight = display_height;
				d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
				d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
				d3dpp.hDeviceWindow = main_window_handle;
				d3dpp.Windowed = TRUE;
				d3dpp.PresentationInterval = config.wait_vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
				
				// create d3d9 device
				HRESULT hr = lpd3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, main_window_handle, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &lpd3d9Device);
				if(hr != D3D_OK) {
					hr = lpd3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, main_window_handle, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &lpd3d9Device);
					if(hr != D3D_OK) {
						hr = lpd3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, main_window_handle, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &lpd3d9Device);
					}
				}
				if(hr != D3D_OK) {
					MessageBox(main_window_handle, _T("Failed to create a Direct3D9 device"), _T(DEVICE_NAME), MB_OK | MB_ICONWARNING);
					config.use_d3d9 = false;
					goto RETRY;
				}
			}
		}
		if(use_d3d9 && lpd3d9Device != NULL) {
			// release and create d3d9 surfaces
			release_d3d9_surface();
			
			HRESULT hr = lpd3d9Device->CreateOffscreenPlainSurface(source_width * stretch_pow_x, source_height * stretch_pow_y, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &lpd3d9Surface, NULL);
			if(hr == D3D_OK) {
				hr = lpd3d9Device->CreateOffscreenPlainSurface(source_width * stretch_pow_x, source_height * stretch_pow_y, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &lpd3d9OffscreenSurface, NULL);
			}
			if(hr == D3D_OK) {
				lpd3d9Device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 0.0, 0);
			}
			else {
				MessageBox(main_window_handle, _T("Failed to create a Direct3D9 offscreen surface"), _T(DEVICE_NAME), MB_OK | MB_ICONWARNING);
				config.use_d3d9 = false;
				goto RETRY;
			}
		}
		if(stretch_screen) {
			render_to_d3d9Buffer = false;
		}
	}
	
	first_draw_screen = false;
	first_invalidate = true;
	screen_size_changed = false;
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
		screen_size_changed = true;
		
		// re-create dib sections
		HDC hdc = GetDC(main_window_handle);
		release_dib_section(hdcDib, hBmp, hOldBmp, lpBuf);
		create_dib_section(hdc, screen_width, screen_height, &hdcDib, &hBmp, &hOldBmp, &lpBuf, &lpBmp, &lpDib);
#ifdef USE_SCREEN_ROTATE
		release_dib_section(hdcDibRotate, hBmpRotate, hOldBmpRotate, lpBufRotate);
		create_dib_section(hdc, screen_height, screen_width, &hdcDibRotate, &hBmpRotate, &hOldBmpRotate, &lpBufRotate, &lpBmpRotate, &lpDibRotate);
#endif
		ReleaseDC(main_window_handle, hdc);
		
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
	// don't draw screen before new screen size is applied to buffers
	if(screen_size_changed) {
		return;
	}
	
	// lock offscreen surface
	D3DLOCKED_RECT pLockedRect;
	if(use_d3d9 && lpd3d9OffscreenSurface != NULL && lpd3d9OffscreenSurface->LockRect(&pLockedRect, NULL, 0) == D3D_OK) {
		lpd3d9Buffer = (scrntype *)pLockedRect.pBits;
	}
	else {
		lpd3d9Buffer = NULL;
	}
	
	// draw screen
	vm->draw_screen();
	
	// screen size was changed in vm->draw_screen()
	if(screen_size_changed) {
		// unlock offscreen surface
		if(use_d3d9 && lpd3d9Buffer != NULL) {
			lpd3d9Buffer = NULL;
			lpd3d9OffscreenSurface->UnlockRect();
		}
		return;
	}
	
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
		scrntype* src = lpBmpSource + source_width * (source_height - 1);
		scrntype* out = lpBmpStretch1 + source_width * stretch_pow_x * (source_height * stretch_pow_y - 1);
		int data_len = source_width * stretch_pow_x;
		
		for(int y = 0; y < source_height; y++) {
			if(stretch_pow_x != 1) {
				scrntype* out_tmp = out;
				for(int x = 0; x < source_width; x++) {
					scrntype c = src[x];
					for(int px = 0; px < stretch_pow_x; px++) {
						out_tmp[px] = c;
					}
					out_tmp += stretch_pow_x;
				}
			}
			else {
				// faster than memcpy()
				for(int x = 0; x < source_width; x++) {
					out[x] = src[x];
				}
			}
			if(stretch_pow_y != 1) {
				scrntype* src_tmp = out;
				for(int py = 1; py < stretch_pow_y; py++) {
					out -= data_len;
					// about 10% faster than memcpy()
					for(int x = 0; x < data_len; x++) {
						out[x] = src_tmp[x];
					}
				}
			}
			src -= source_width;
			out -= data_len;
		}
		if(!use_d3d9) {
			StretchBlt(hdcDibStretch2, 0, 0, stretched_width, stretched_height, hdcDibStretch1, 0, 0, source_width * stretch_pow_x, source_height * stretch_pow_y, SRCCOPY);
		}
	}
	first_draw_screen = true;
	
	// copy bitmap to d3d9 offscreen surface
	if(use_d3d9 && lpd3d9Buffer != NULL) {
		if(!(render_to_d3d9Buffer && !now_rec_vid)) {
			scrntype *src = stretch_screen ? lpBmpStretch1 : lpBmpSource;
			src += source_width * stretch_pow_x * (source_height * stretch_pow_y - 1);
			scrntype *out = lpd3d9Buffer;
			int data_len = source_width * stretch_pow_x;
			
			for(int y = 0; y < source_height * stretch_pow_y; y++) {
				for(int i = 0; i < data_len; i++) {
					out[i] = src[i];
				}
				src -= data_len;
				out += data_len;
			}
		}
		// unlock offscreen surface
		lpd3d9Buffer = NULL;
		lpd3d9OffscreenSurface->UnlockRect();
	}
	
	// invalidate window
	InvalidateRect(main_window_handle, NULL, first_invalidate);
	UpdateWindow(main_window_handle);
	self_invalidate = true;
	
	// record picture
	if(now_rec_vid) {
		if(AVIStreamWrite(pAVICompressed, rec_frames++, 1, (LPBYTE)lpBmpSource, pbmInfoHeader->biSizeImage, AVIIF_KEYFRAME, NULL, NULL) != AVIERR_OK) {
			stop_rec_video();
		}
	}
}

scrntype* EMU::screen_buffer(int y)
{
	if(use_d3d9 && lpd3d9Buffer != NULL && render_to_d3d9Buffer && !now_rec_vid) {
		return lpd3d9Buffer + screen_width * y;
	}
	return lpBmp + screen_width * (screen_height - y - 1);
}

void EMU::update_screen(HDC hdc)
{
#ifdef USE_BITMAP
	if(first_invalidate || !self_invalidate) {
		HDC hmdc = CreateCompatibleDC(hdc);
		HBITMAP hBitmap = LoadBitmap(instance_handle, _T("IDI_BITMAP1"));
		BITMAP bmp;
		GetObject(hBitmap, sizeof(BITMAP), &bmp);
		int w = (int)bmp.bmWidth;
		int h = (int)bmp.bmHeight;
		HBITMAP hOldBitmap = (HBITMAP)SelectObject(hmdc, hBitmap);
		BitBlt(hdc, 0, 0, w, h, hmdc, 0, 0, SRCCOPY);
		SelectObject(hmdc, hOldBitmap);
		DeleteObject(hBitmap);
		DeleteDC(hmdc);
	}
#endif
	if(first_draw_screen) {
#ifdef USE_LED
		// 7-seg LEDs
		for(int i = 0; i < MAX_LEDS; i++) {
			int x = leds[i].x;
			int y = leds[i].y;
			int w = leds[i].width;
			int h = leds[i].height;
			BitBlt(hdc, x, y, w, h, hdcDib, x, y, SRCCOPY);
		}
#else
		// standard screen
		if(use_d3d9) {
			LPDIRECT3DSURFACE9 lpd3d9BackSurface = NULL;
			if(lpd3d9Device != NULL && lpd3d9Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &lpd3d9BackSurface) == D3D_OK && lpd3d9BackSurface != NULL) {
				RECT rectSrc = { 0, 0, source_width * stretch_pow_x, source_height * stretch_pow_y };
				RECT rectDst = { screen_dest_x, screen_dest_y, screen_dest_x + stretched_width, screen_dest_y + stretched_height };
				
				lpd3d9Device->UpdateSurface(lpd3d9OffscreenSurface, NULL, lpd3d9Surface, NULL);
				lpd3d9Device->StretchRect(lpd3d9Surface, &rectSrc, lpd3d9BackSurface, &rectDst, stretch_screen ? D3DTEXF_LINEAR : D3DTEXF_POINT);
				lpd3d9BackSurface->Release();
				lpd3d9Device->Present(NULL, NULL, NULL, NULL);
			}
		}
		else if(stretch_screen) {
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
#ifdef USE_ACCESS_LAMP
		// draw access lamps of drives
		int status = vm->access_lamp() & 7;
		static int prev_status = 0, tmp_status = 0;
		static int remain = 0;
		
		if(status == 0) {
			if(remain > 0) {
				status = prev_status = tmp_status;
				remain--;
			}
		} else {
#ifdef SUPPORT_VARIABLE_TIMING
			remain = (int)(vm->frame_rate() / 4.0 + 0,5);
#else
			remain = (int)(FRAMES_PER_SEC / 4.0 + 0.5);
#endif
			tmp_status = status;
		}
		bool render_in = (status != 0);
		bool render_out = (prev_status != status);
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
#endif
		first_invalidate = self_invalidate = false;
	}
}

void EMU::capture_screen()
{
	if(use_d3d9 && render_to_d3d9Buffer && !now_rec_vid) {
		// virtual machine may render screen to d3d9 buffer directly...
		vm->draw_screen();
	}
	
	SYSTEMTIME sTime;
	GetLocalTime(&sTime);
	
	_TCHAR file_name[_MAX_PATH];
	_stprintf(file_name, _T("%d-%0.2d-%0.2d_%0.2d-%0.2d-%0.2d.bmp"), sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wSecond);
	
	BITMAPFILEHEADER bmFileHeader = { (WORD)(TEXT('B') | TEXT('M') << 8) };
	bmFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmFileHeader.bfSize = bmFileHeader.bfOffBits + pbmInfoHeader->biSizeImage;
	
	DWORD dwSize;
	HANDLE hFile = CreateFile(bios_path(file_name), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(hFile, &bmFileHeader, sizeof(BITMAPFILEHEADER), &dwSize, NULL);
	WriteFile(hFile, lpDibSource, sizeof(BITMAPINFOHEADER), &dwSize, NULL);
	WriteFile(hFile, lpBmpSource, pbmInfoHeader->biSizeImage, &dwSize, NULL);
	CloseHandle(hFile);
}

void EMU::start_rec_video(int fps, bool show_dialog)
{
	// initialize vfw
	AVIFileInit();
	if(AVIFileOpen(&pAVIFile, bios_path(_T("video.avi")), OF_WRITE | OF_CREATE, NULL) != AVIERR_OK) {
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
	now_rec_vid = true;
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
		FILE* fp = _tfopen(bios_path(_T("video.avi")), _T("r+b"));
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
	now_rec_vid = false;
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
		
		start_rec_video(rec_fps, false);
	}
}

