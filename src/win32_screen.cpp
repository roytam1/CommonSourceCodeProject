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
	create_screen_buffer(hdc);
#ifdef USE_SECOND_BUFFER
	// create dib section for 2nd buffer
	lpBufOut = (LPBYTE)GlobalAlloc(GPTR, sizeof(BITMAPINFO));
	lpDibOut = (LPBITMAPINFO)lpBufOut;
	lpDibOut->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpDibOut->bmiHeader.biWidth = SECOND_BUFFER_WIDTH;
	lpDibOut->bmiHeader.biHeight = SECOND_BUFFER_HEIGHT;
	lpDibOut->bmiHeader.biPlanes = 1;
#if defined(_RGB555)
	lpDibOut->bmiHeader.biBitCount = 16;
	lpDibOut->bmiHeader.biCompression = BI_RGB;
#elif defined(_RGB565)
	lpDibOut->bmiHeader.biBitCount = 16;
	lpDibOut->bmiHeader.biCompression = BI_BITFIELDS;
	LPDWORD lpBfOut = (LPDWORD)lpDibOut->bmiColors;
	lpBfOut[0] = 0x1f << 11;
	lpBfOut[1] = 0x3f << 5;
	lpBfOut[2] = 0x1f << 0;
#elif defined(_RGB888)
	lpDibOut->bmiHeader.biBitCount = 32;
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
	release_screen_buffer();
#ifdef USE_SECOND_BUFFER
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

void EMU::create_screen_buffer(HDC hdc)
{
	lpBuf = (LPBYTE)GlobalAlloc(GPTR, sizeof(BITMAPINFO));
	lpDib = (LPBITMAPINFO)lpBuf;
	lpDib->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpDib->bmiHeader.biWidth = screen_width;
	lpDib->bmiHeader.biHeight = screen_height;
	lpDib->bmiHeader.biPlanes = 1;
#if defined(_RGB555)
	lpDib->bmiHeader.biBitCount = 16;
	lpDib->bmiHeader.biCompression = BI_RGB;
#elif defined(_RGB565)
	lpDib->bmiHeader.biBitCount = 16;
	lpDib->bmiHeader.biCompression = BI_BITFIELDS;
	LPDWORD lpBf = (LPDWORD)lpDib->bmiColors;
	lpBf[0] = 0x1f << 11;
	lpBf[1] = 0x3f << 5;
	lpBf[2] = 0x1f << 0;
#elif defined(_RGB888)
	lpDib->bmiHeader.biBitCount = 32;
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
}

void EMU::release_screen_buffer()
{
	DeleteDC(hdcDib);
	DeleteObject(hBmp);
	GlobalFree(lpBuf);
}

int EMU::get_window_width(int mode)
{
#ifdef USE_SCREEN_ROTATE
	return config.monitor_type ? window_width2 : window_width1;
#else
	return mode ? window_width2 : window_width1;
#endif
}

int EMU::get_window_height(int mode) {
#ifdef USE_SCREEN_ROTATE
	return config.monitor_type ? window_height2 : window_height1;
#else
	return mode ? window_height2 : window_height1;
#endif
}

void EMU::set_window_size(int width, int height)
{
	if(width != -1) {
		window_width = width;
		window_height = height;
	}
#ifdef USE_SCREEN_X2
	bool use_scree_buffer_size = ((window_width > SECOND_BUFFER_WIDTH) || (window_height > SECOND_BUFFER_HEIGHT));
	int stretch_width = use_scree_buffer_size ? SECOND_BUFFER_WIDTH : window_width;
	int stretch_height = use_scree_buffer_size ? SECOND_BUFFER_HEIGHT : window_height;
	stretch_x = stretch_width / screen_width_aspect; if(stretch_x < 1) stretch_x = 1;
	stretch_y = stretch_height / screen_height; if(stretch_y < 1) stretch_y = 1;
	if(stretch_x < stretch_y) stretch_y = stretch_x; else stretch_x = stretch_y;
	stretch_x = stretch_x * screen_width_aspect / screen_width;
	buffer_x = screen_width * stretch_x;
	buffer_y = screen_height * stretch_y;
#elif defined(USE_SCREEN_ROTATE)
	now_rotate = (config.monitor_type != 0);
	buffer_x = now_rotate ? screen_height : screen_width;
	buffer_y = now_rotate ? screen_width : screen_height;
#else
	buffer_x = screen_width;
	buffer_y = screen_height;
#endif
	dest_x = (window_width - buffer_x) >> 1;
	dest_y = (window_height - buffer_y) >> 1;
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
			hr = lpd3d9dev->CreateOffscreenPlainSurface(buffer_x, buffer_y, D3DFMT_X1R5G5B5, D3DPOOL_DEFAULT, &lpSurface, NULL);
			if(hr != D3D_OK) {
				hr = lpd3d9dev->CreateOffscreenPlainSurface(buffer_x, buffer_y, D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &lpSurface, NULL);
				if(hr != D3D_OK)
					hr = lpd3d9dev->CreateOffscreenPlainSurface(buffer_x, buffer_y, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &lpSurface, NULL);
			}
			hr = lpd3d9dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &lpDDSBack);
			if(hr == D3D_OK) {
				// adjust rect to keep aspect
				D3DSURFACE_DESC d3dsd;
				ZeroMemory(&d3dsd, sizeof(d3dsd));
				lpDDSBack->GetDesc(&d3dsd);
				UINT rect_w = d3dsd.Width, w = (d3dsd.Height  * buffer_x) / buffer_y;
				UINT rect_h = d3dsd.Height, h = (d3dsd.Width * buffer_y) / buffer_x;
				if(w < d3dsd.Width) rect_w = w;
				if(h < d3dsd.Height) rect_h = h;
				UINT rect_l = (d3dsd.Width - rect_w) >> 1;
				UINT rect_t = (d3dsd.Height - rect_h) >> 1;
				SetRect(&DstRect, rect_l, rect_t, rect_l + rect_w, rect_t +rect_h);
				if(config.d3d9_filter == 0)
					filter = D3DTEXF_LINEAR;
				else
					filter = D3DTEXF_POINT;
			}
		}
	}
#endif
}

void EMU::draw_screen()
{
	// draw screen
	vm->draw_screen();
	
#ifdef USE_SECOND_BUFFER
	use_buffer = false;
#ifdef USE_SCREEN_X2
	// stretch screen
	if(!(stretch_x == 1 && stretch_y == 1)) {
		for(int y = 0; y < screen_height; y++) {
			scrntype* src = lpBmp + screen_width * (screen_height - y - 1);
			scrntype* out = lpBmpOut + SECOND_BUFFER_WIDTH * (SECOND_BUFFER_HEIGHT - y * stretch_y - 1);
			if(stretch_x > 1) {
				scrntype* outx = out;
				for(int x = 0; x < screen_width; x++) {
					scrntype col = src[x];
					for(int px = 0; px < stretch_x; px++)
						*outx++ = col;
				}
			}
			else
				_memcpy(out, src, screen_width * sizeof(scrntype));
			for(int py = 1; py < stretch_y; py++) {
				scrntype* outy = lpBmpOut + SECOND_BUFFER_WIDTH * (SECOND_BUFFER_HEIGHT - y * stretch_y - py - 1);
				_memcpy(outy, out, screen_width * stretch_x * sizeof(scrntype));
			}
		}
		use_buffer = true;
	}
#elif defined(USE_SCREEN_ROTATE)
	// rotate screen
	if(now_rotate) {
		for(int y = 0; y < screen_height; y++) {
			scrntype* src = lpBmp + screen_width * (screen_height - y - 1);
			scrntype* out = lpBmpOut + SECOND_BUFFER_WIDTH * (SECOND_BUFFER_HEIGHT - 1) + (screen_height - y - 1);
			for(int x = 0; x < screen_width; x++) {
				*out = src[x];
				out -= SECOND_BUFFER_WIDTH;
			}
		}
		use_buffer = true;
	}
#endif
#endif
#ifdef _USE_D3D9
	// update screen
	if(lpd3d9dev && lpSurface && lpDDSBack) {
		lpd3d9dev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0, 0);
		lpd3d9dev->BeginScene();
		HDC hdc;
		lpSurface->GetDC(&hdc);
#ifdef USE_SECOND_BUFFER
		BitBlt(hdc, 0, 0, buffer_x, buffer_y, use_buffer ? hdcDibOut : hdcDib, 0, 0, SRCCOPY);
#else
		BitBlt(hdc, 0, 0, buffer_x, buffer_y, hdcDib, 0, 0, SRCCOPY);
#endif
		lpSurface->ReleaseDC(hdc);
		lpd3d9dev->StretchRect(lpSurface, NULL, lpDDSBack, &DstRect, filter);
		lpd3d9dev->EndScene();
		lpd3d9dev->Present(NULL, NULL, NULL, NULL);
	}
#else
	// invalidate window
	InvalidateRect(main_window_handle, NULL, FALSE);
	UpdateWindow(main_window_handle);
#endif
#ifdef SUPPORT_VFW
	// record picture
	if(now_recv) {
		if(AVIStreamWrite(pAVICompressed, rec_frames++, 1, (LPBYTE)lpBmp, screen_width * screen_height * 2, AVIIF_KEYFRAME, NULL, NULL) != AVIERR_OK)
			stop_rec_video();
	}
#endif
}

void EMU::update_screen(HDC hdc)
{
#ifndef _USE_D3D9
#ifdef USE_SECOND_BUFFER
	BitBlt(hdc, dest_x, dest_y, buffer_x, buffer_y, use_buffer ? hdcDibOut : hdcDib, 0, 0, SRCCOPY);
#else
	BitBlt(hdc, dest_x, dest_y, buffer_x, buffer_y, hdcDib, 0, 0, SRCCOPY);
#endif
#endif
}

scrntype* EMU::screen_buffer(int y)
{
	return lpBmp + screen_width * (screen_height - y - 1);
}

void EMU::change_screen_size(int sw, int sh, int swa, int ww1, int wh1, int ww2, int wh2)
{
	// virtual machine changes the screen size
	if(screen_width != sw || screen_height != sh) {
		screen_width = sw;
		screen_height = sh;
		screen_width_aspect = (swa > 0) ? swa : sw;
		window_width1 = ww1;
		window_height1 = wh1;
		window_width2 = ww2;
		window_height2 = wh2;
		
		// stop recording
		if(now_recv) {
			stop_rec_video();
			stop_rec_sound();
		}
		// recreate the screen buffer
		release_screen_buffer();
		HDC hdc = GetDC(main_window_handle);
		create_screen_buffer(hdc);
		// change the window size
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
	strhdr.dwSuggestedBufferSize = screen_width * screen_height * 2;
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

