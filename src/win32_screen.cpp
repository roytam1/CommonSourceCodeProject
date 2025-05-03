/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 screen ]
*/

#include "emu.h"
#include "vm/vm.h"
#include "config.h"

extern config_t config;

void EMU::initialize_screen()
{
	// init screen and window size
	screen_width = SCREEN_WIDTH;
	screen_height = SCREEN_HEIGHT;
	screen_width_aspect = SCREEN_WIDTH_ASPECT;
	window_width1 = WINDOW_WIDTH1;
	window_height1 = WINDOW_HEIGHT1;
	window_width2 = WINDOW_WIDTH2;
	window_height2 = WINDOW_HEIGHT2;
	
	// create dib section for render
	HDC hdc = GetDC(main_window_handle);
	lpBuf = (LPBYTE)GlobalAlloc(GPTR, sizeof(BITMAPINFO));
	lpDib = (LPBITMAPINFO)lpBuf;
	lpDib->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpDib->bmiHeader.biWidth = SCREEN_BUFFER_WIDTH;
	lpDib->bmiHeader.biHeight = SCREEN_BUFFER_HEIGHT;
	lpDib->bmiHeader.biPlanes = 1;
	lpDib->bmiHeader.biBitCount = 16;
#ifdef _WIN32_WCE
	// RGB565
	lpDib->bmiHeader.biCompression = BI_BITFIELDS;
	LPDWORD lpBf = (LPDWORD)lpDib->bmiColors;
	lpBf[0] = 0x1f << 11;
	lpBf[1] = 0x3f << 5;
	lpBf[2] = 0x1f << 0;
#else
	lpDib->bmiHeader.biCompression = BI_RGB;
#endif
	lpDib->bmiHeader.biSizeImage = 0;
	lpDib->bmiHeader.biXPelsPerMeter = 0;
	lpDib->bmiHeader.biYPelsPerMeter = 0;
	lpDib->bmiHeader.biClrUsed = 0;
	lpDib->bmiHeader.biClrImportant = 0;
	hBmp = CreateDIBSection(hdc, lpDib, DIB_RGB_COLORS, (PVOID*)&lpBmp, NULL, 0);
	hdcDib = CreateCompatibleDC(hdc);
	SelectObject(hdcDib, hBmp);
#ifdef STRETCH_SCREEN
	// create dib section for stretch
	lpBufOut = (LPBYTE)GlobalAlloc(GPTR, sizeof(BITMAPINFO));
	lpDibOut = (LPBITMAPINFO)lpBufOut;
	lpDibOut->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpDibOut->bmiHeader.biWidth = STRETCH_WIDTH;
	lpDibOut->bmiHeader.biHeight = STRETCH_HEIGHT;
	lpDibOut->bmiHeader.biPlanes = 1;
	lpDibOut->bmiHeader.biBitCount = 16;
#ifdef _WIN32_WCE
	// RGB565
	lpDibOut->bmiHeader.biCompression = BI_BITFIELDS;
	LPDWORD lpBfOut = (LPDWORD)lpDibOut->bmiColors;
	lpBfOut[0] = 0x1f << 11;
	lpBfOut[1] = 0x3f << 5;
	lpBfOut[2] = 0x1f << 0;
#else
	lpDibOut->bmiHeader.biCompression = BI_RGB;
#endif
	lpDibOut->bmiHeader.biSizeImage = 0;
	lpDibOut->bmiHeader.biXPelsPerMeter = 0;
	lpDibOut->bmiHeader.biYPelsPerMeter = 0;
	lpDibOut->bmiHeader.biClrUsed = 0;
	lpDibOut->bmiHeader.biClrImportant = 0;
	hBmpOut = CreateDIBSection(hdc, lpDibOut, DIB_RGB_COLORS, (PVOID*)&lpBmpOut, NULL, 0);
	hdcDibOut = CreateCompatibleDC(hdc);
	SelectObject(hdcDibOut, hBmpOut);
#endif
#ifdef _USE_D3D9
	// init d3d9
	lpd3d = NULL;
	lpd3d9dev = NULL;
	lpSurface = NULL;
	lpDDSBack = NULL;
#endif
	// init video recording
	now_recv = false;
#ifdef SUPPORT_VFW
	pAVIStream = NULL;
	pAVICompressed = NULL;
	pAVIFile = NULL;
#endif
}

#ifdef _USE_D3D9
#define RELEASE_D3D9() { \
	if(lpDDSBack) { \
		lpDDSBack->Release(); \
		lpDDSBack = NULL; \
	} \
	if(lpSurface) { \
		lpSurface->Release(); \
		lpSurface = NULL; \
	} \
	if(lpd3d9dev) { \
		lpd3d9dev->Release(); \
		lpd3d9dev = NULL; \
	} \
	if(lpd3d) { \
		lpd3d->Release(); \
		lpd3d = NULL; \
	} \
}
#endif

void EMU::release_screen()
{
	// release dib sections
	DeleteDC(hdcDib);
	DeleteObject(hBmp);
	GlobalFree(lpBuf);
#ifdef STRETCH_SCREEN
	DeleteDC(hdcDibOut);
	DeleteObject(hBmpOut);
	GlobalFree(lpBufOut);
#endif
#ifdef _USE_D3D9
	// release d3d9
	RELEASE_D3D9();
#endif
	// stop video recording
	stop_rec_video();
}

void EMU::set_window_size(int width, int height)
{
	if(width != -1) {
		window_width = width;
		window_height = height;
	}
#ifdef _USE_D3D9
	// release and init d3d9 again
	RELEASE_D3D9();
	lpd3d = Direct3DCreate9(D3D_SDK_VERSION);
	if(lpd3d) {
		// init present params
		D3DPRESENT_PARAMETERS d3dpp;
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
		if(config.d3d9_interval == 0)
			d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		else
			d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
		// create device
		HRESULT hr;
		if(config.d3d9_device == 0) {
			hr = lpd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, main_window_handle, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &lpd3d9dev);
			if(hr != D3D_OK) {
				hr = lpd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, main_window_handle, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &lpd3d9dev);
				if(hr != D3D_OK)
					hr = lpd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, main_window_handle, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &lpd3d9dev);
			}
		}
		else if(config.d3d9_device == 1)
			hr = lpd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, main_window_handle, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &lpd3d9dev);
		else if(config.d3d9_device == 2)
			hr = lpd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, main_window_handle, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &lpd3d9dev);
		else
			hr = lpd3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, main_window_handle, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &lpd3d9dev);
		// create surface
		if(hr == D3D_OK) {
			hr = lpd3d9dev->CreateOffscreenPlainSurface(screen_width, screen_height, D3DFMT_X1R5G5B5, D3DPOOL_DEFAULT, &lpSurface, NULL);
			if(hr != D3D_OK) {
				hr = lpd3d9dev->CreateOffscreenPlainSurface(screen_width, screen_height, D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &lpSurface, NULL);
				if(hr != D3D_OK)
					hr = lpd3d9dev->CreateOffscreenPlainSurface(screen_width, screen_height, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &lpSurface, NULL);
			}
			hr = lpd3d9dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &lpDDSBack);
			if(hr == D3D_OK) {
				// adjust rect to keep aspect
				D3DSURFACE_DESC d3dsd;
				ZeroMemory(&d3dsd, sizeof(d3dsd));
				lpDDSBack->GetDesc(&d3dsd);
				UINT rect_w = d3dsd.Width, w = (d3dsd.Height  * WINDOW_WIDTH1) / WINDOW_HEIGHT1;
				UINT rect_h = d3dsd.Height, h = (d3dsd.Width * WINDOW_HEIGHT1) / WINDOW_WIDTH1;
				if(w < d3dsd.Width) rect_w = w;
				if(h < d3dsd.Height) rect_h = h;
				UINT rect_l = (d3dsd.Width - rect_w) >> 1;
				UINT rect_t = (d3dsd.Height - rect_h) >> 1;
				SetRect(&DstRect, rect_l, rect_t, rect_l + rect_w, rect_t +rect_h);
				if(config.d3d9_filter == 0) {
					if(screen_width >= 640)
						filter = (width == WINDOW_WIDTH1) ? D3DTEXF_POINT : D3DTEXF_LINEAR;
					else if(screen_width != screen_width_aspect)
						filter = (width == WINDOW_WIDTH1) ? D3DTEXF_LINEAR : D3DTEXF_POINT;
					else
						filter = D3DTEXF_POINT;
				}
				else if(config.d3d9_filter == 1)
					filter = D3DTEXF_POINT;
				else
					filter = D3DTEXF_LINEAR;
			}
		}
	}
#else
#ifdef STRETCH_SCREEN
	int stretch_width = window_width > STRETCH_WIDTH ? STRETCH_WIDTH : window_width;
	int stretch_height = window_height > STRETCH_HEIGHT ? STRETCH_HEIGHT : window_height;
	stretch_x = stretch_width / screen_width_aspect; if(stretch_x < 1) stretch_x = 1;
	stretch_y = stretch_height / screen_height; if(stretch_y < 1) stretch_y = 1;
	if(stretch_x < stretch_y) stretch_y = stretch_x; else stretch_x = stretch_y;
	stretch_x = stretch_x * screen_width_aspect / screen_width;
	dest_x = (window_width - screen_width * stretch_x) >> 1;
	dest_y = (window_height - screen_height * stretch_y) >> 1;
#else
	dest_x = (window_width - screen_width) >> 1;
	dest_y = (window_height - screen_height) >> 1;
#endif
#endif
}

void EMU::draw_screen()
{
	// draw screen
	vm->draw_screen();
	
#ifdef _USE_D3D9
	// update screen
	if(lpd3d9dev && lpSurface && lpDDSBack) {
		lpd3d9dev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0, 0);
		lpd3d9dev->BeginScene();
		HDC hdc;
		lpSurface->GetDC(&hdc);
		BitBlt(hdc, 0, 0, screen_width, screen_height, hdcDib, 0, 0, SRCCOPY);
		lpSurface->ReleaseDC(hdc);
		lpd3d9dev->StretchRect(lpSurface, NULL, lpDDSBack, &DstRect, filter);
		lpd3d9dev->EndScene();
		lpd3d9dev->Present(NULL, NULL, NULL, NULL);
	}
#else
#ifdef STRETCH_SCREEN
	// stretch screen
	if(!(stretch_x == 1 && stretch_y == 1)) {
		for(int y = 0; y < screen_height; y++) {
			uint16* src = lpBmp + SCREEN_BUFFER_WIDTH * (SCREEN_BUFFER_HEIGHT - y - 1);
			uint16* out = lpBmpOut + STRETCH_WIDTH * (STRETCH_HEIGHT - y * stretch_y - 1);
			if(stretch_x > 1) {
				uint16* outx = out;
				for(int x = 0; x < screen_width; x++) {
					uint16 col = src[x];
					for(int px = 0; px < stretch_x; px++)
						*outx++ = col;
				}
			}
			else
				_memcpy(out, src, screen_width * 2);
			for(int py = 1; py < stretch_y; py++) {
				uint16* outy = lpBmpOut + STRETCH_WIDTH * (STRETCH_HEIGHT - y * stretch_y - py - 1);
				_memcpy(outy, out, screen_width * stretch_x * 2);
			}
		}
	}
#endif
	// invalidate window
	InvalidateRect(main_window_handle, NULL, FALSE);
	UpdateWindow(main_window_handle);
#endif
#ifdef SUPPORT_VFW
	// record picture
	if(now_recv) {
		if(AVIStreamWrite(pAVICompressed, rec_frames++, 1, (LPBYTE)lpBmp, SCREEN_BUFFER_WIDTH * SCREEN_BUFFER_HEIGHT * 2, AVIIF_KEYFRAME, NULL, NULL) != AVIERR_OK)
			stop_rec_video();
	}
#endif
}

void EMU::update_screen(HDC hdc)
{
#ifndef _USE_D3D9
#ifdef STRETCH_SCREEN
	if(stretch_x == 1 && stretch_y == 1)
		BitBlt(hdc, dest_x, dest_y, screen_width, screen_height, hdcDib, 0, 0, SRCCOPY);
	else
		BitBlt(hdc, dest_x, dest_y, screen_width * stretch_x, screen_height * stretch_y, hdcDibOut, 0, 0, SRCCOPY);
#else
	BitBlt(hdc, dest_x, dest_y, screen_width, screen_height, hdcDib, 0, 0, SRCCOPY);
#endif
#endif
}

uint16* EMU::screen_buffer(int y)
{
	return lpBmp + SCREEN_BUFFER_WIDTH * (SCREEN_BUFFER_HEIGHT - y - 1);
}

void EMU::change_screen_size(int sw, int sh, int swa, int ww1, int wh1, int ww2, int wh2)
{
	// virtual machine changes the screen size
	if(screen_width != sw && screen_height != sh) {
		screen_width = sw;
		screen_height = sh;
		screen_width_aspect = (swa > 0) ? swa : sw;
		window_width1 = ww1;
		window_height1 = wh1;
		window_width2 = ww2;
		window_height2 = wh2;
		PostMessage(main_window_handle, WM_RESIZE, 0L, 0L);
	}
}

void EMU::start_rec_video(int fps, bool show_dialog)
{
#ifdef SUPPORT_VFW
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
	strhdr.dwSuggestedBufferSize = SCREEN_BUFFER_WIDTH * SCREEN_BUFFER_HEIGHT * 2;
	SetRect(&strhdr.rcFrame, 0, 0, SCREEN_BUFFER_WIDTH, SCREEN_BUFFER_HEIGHT);
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
	now_recv = true;
#endif
}

void EMU::stop_rec_video()
{
#ifdef SUPPORT_VFW
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
#ifdef SUPPORT_VFW
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

