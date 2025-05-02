/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 main ]
*/

#include <windows.h>
#include <windowsx.h>
#include <winsock.h>
#include <commdlg.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <stdio.h>
#include "res/resource.h"
#include "config.h"
#include "emu.h"

// emulation core
EMU* emu;

// buttons
#ifdef USE_BUTTON
#define MAX_FONT_SIZE 32
HFONT hFont[MAX_FONT_SIZE];
HWND hButton[MAX_BUTTONS];
WNDPROC buttonWndProc[MAX_BUTTONS];
#endif

// menu
BOOL now_menu = FALSE;
BOOL now_menuloop = FALSE;

void update_menu(HWND hWnd, HMENU hMenu, int pos);

void show_menu_bar(HWND hWnd)
{
	if(!now_menu) {
		HMENU hMenu = LoadMenu((HINSTANCE)GetModuleHandle(0), MAKEINTRESOURCE(IDR_MENU1));
		SetMenu(hWnd, hMenu);
		now_menu = TRUE;
	}
}

void hide_menu_bar(HWND hWnd)
{
	if(now_menu) {
		HMENU hMenu = GetMenu(hWnd);
		SetMenu(hWnd, NULL);
		DestroyMenu(hMenu);
		now_menu = FALSE;
	}
}

// dialog
#ifdef USE_CART
void open_cart(HWND hWnd);
#endif
#ifdef USE_FD1
void open_disk(HWND hWnd, int drv);
#endif
#ifdef USE_QUICKDISK
void open_quickdisk(HWND hWnd);
#endif
#ifdef USE_DATAREC
void open_datarec(HWND hWnd, BOOL play);
#endif
#ifdef USE_MEDIA
void open_media(HWND hWnd);
#endif
#ifdef USE_RAM
void open_ram(HWND hWnd, BOOL load);
#endif

void get_long_full_path_name(_TCHAR* src, _TCHAR* dst)
{
	_TCHAR tmp[_MAX_PATH];
	
	if(GetFullPathName(src, _MAX_PATH, tmp, NULL) == 0) {
		_tcscpy(dst, src);
	}
	else if(GetLongPathName(tmp, dst, _MAX_PATH) == 0) {
		_tcscpy(dst, tmp);
	}
}

_TCHAR* get_parent_dir(_TCHAR* file)
{
	static _TCHAR path[_MAX_PATH];
	
	_tcscpy(path, file);
	int pt = _tcslen(path);
	while(pt >= 0 && path[pt] != _T('\\')) {
		pt--;
	}
	path[pt + 1] = _T('\0');
	return path;
}

_TCHAR* get_open_file_name(HWND hWnd, _TCHAR* filter, _TCHAR* title, _TCHAR* dir)
{
	static _TCHAR path[_MAX_PATH];
	_TCHAR tmp[_MAX_PATH] = _T("");
	OPENFILENAME OpenFileName;
	
	memset(&OpenFileName, 0, sizeof(OpenFileName));
	OpenFileName.lStructSize = sizeof(OPENFILENAME);
	OpenFileName.hwndOwner = hWnd;
	OpenFileName.lpstrFilter = filter;
	OpenFileName.lpstrFile = tmp;
	OpenFileName.nMaxFile = _MAX_PATH;
	OpenFileName.lpstrTitle = title;
	if(dir[0]) {
		OpenFileName.lpstrInitialDir = dir;
	}
	else {
		_TCHAR app[_MAX_PATH];
		GetModuleFileName(NULL, app, _MAX_PATH);
		OpenFileName.lpstrInitialDir = get_parent_dir(app);
	}
	if(GetOpenFileName(&OpenFileName)) {
		get_long_full_path_name(OpenFileName.lpstrFile, path);
		_tcscpy(dir, get_parent_dir(path));
		return path;
	}
	return NULL;
}

#define UPDATE_HISTORY(path, recent) { \
	int no = 7; \
	for(int i = 0; i < 8; i++) { \
		if(_tcscmp(recent[i], path) == 0) { \
			no = i; \
			break; \
		} \
	} \
	for(int i = no; i > 0; i--) { \
		_tcscpy(recent[i], recent[i - 1]); \
	} \
	_tcscpy(recent[0], path); \
}

// screen
int desktop_width;
int desktop_height;
int desktop_bpp;
int prev_window_mode = 0;
BOOL now_fullscreen = FALSE;

int window_mode_count;
int screen_mode_count;
int screen_mode_width[20];
int screen_mode_height[20];

void set_window(HWND hWnd, int mode);

// timing control
#define MIN_SKIP_FRAMES 0
#define MAX_SKIP_FRAMES 10
DWORD rec_next_time, rec_accum_time;
int rec_delay[3];

int get_interval()
{
	static int accum = 0;
	accum += emu->frame_interval();
	int interval = accum >> 10;
	accum -= interval << 10;
	return interval;
}

// windows main
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR szCmdLine, int iCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
#ifdef USE_BUTTON
LRESULT CALLBACK ButtonWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR szCmdLine, int iCmdShow)
{
	// load config
	load_config();
	
	// create window
	WNDCLASS wndclass;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = (WNDPROC)WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wndclass.hCursor = 0;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wndclass.lpszClassName = _T("CWINDOW");
	RegisterClass(&wndclass);
	
	// get window position
	RECT rect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
	AdjustWindowRectEx(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_VISIBLE, TRUE, 0);
	HDC hdcScr = GetDC(NULL);
	desktop_width = GetDeviceCaps(hdcScr, HORZRES);
	desktop_height = GetDeviceCaps(hdcScr, VERTRES);
	desktop_bpp = GetDeviceCaps(hdcScr, BITSPIXEL);
	ReleaseDC(NULL, hdcScr);
	int dest_x = (int)((desktop_width - (rect.right - rect.left)) / 2);
	int dest_y = (int)((desktop_height - (rect.bottom - rect.top)) / 2);
	//dest_x = (dest_x < 0) ? 0 : dest_x;
	dest_y = (dest_y < 0) ? 0 : dest_y;
	
	// show window
	HWND hWnd = CreateWindow(_T("CWINDOW"), _T(DEVICE_NAME), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_MINIMIZEBOX,
	                         dest_x, dest_y, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);
	ShowWindow(hWnd, iCmdShow);
	UpdateWindow(hWnd);
	
	// show menu
	show_menu_bar(hWnd);
	
	// enumerate screen mode
	screen_mode_count = 0;
	for(int i = 0;; i++) {
		DEVMODE dev;
		ZeroMemory(&dev, sizeof(dev));
		dev.dmSize = sizeof(dev);
		if(EnumDisplaySettings(NULL, i, &dev) == 0) {
			break;
		}
		if(dev.dmPelsWidth >= WINDOW_WIDTH && dev.dmPelsHeight >= WINDOW_HEIGHT && dev.dmPelsWidth >= 640 && dev.dmPelsHeight >= 480) {
			BOOL found = FALSE;
			for(int j = 0; j < screen_mode_count; j++) {
				if(screen_mode_width[j] == dev.dmPelsWidth && screen_mode_height[j] == dev.dmPelsHeight) {
					found = TRUE;
					break;
				}
			}
			if(!found) {
				screen_mode_width[screen_mode_count] = dev.dmPelsWidth;
				screen_mode_height[screen_mode_count] = dev.dmPelsHeight;
				if(++screen_mode_count == 20) {
					break;
				}
			}
		}
	}
	for(int i = 0; i < screen_mode_count - 1; i++) {
		for(int j = i + 1; j < screen_mode_count; j++) {
			if(screen_mode_width[i] > screen_mode_width[j] || (screen_mode_width[i] == screen_mode_width[j] && screen_mode_height[i] > screen_mode_height[j])) {
				int width = screen_mode_width[i];
				screen_mode_width[i] = screen_mode_width[j];
				screen_mode_width[j] = width;
				int height = screen_mode_height[i];
				screen_mode_height[i] = screen_mode_height[j];
				screen_mode_height[j] = height;
			}
		}
	}
	if(screen_mode_count == 0) {
		screen_mode_width[0] = desktop_width;
		screen_mode_height[0] = desktop_height;
		screen_mode_count = 1;
	}
	
	// restore screen mode
	if(config.window_mode >= 0 && config.window_mode < 8) {
		PostMessage(hWnd, WM_COMMAND, ID_SCREEN_WINDOW1 + config.window_mode, 0L);
	}
	else if(config.window_mode >= 8 && config.window_mode < screen_mode_count + 8) {
		PostMessage(hWnd, WM_COMMAND, ID_SCREEN_FULLSCREEN1 + config.window_mode - 8, 0L);
	}
	else {
		config.window_mode = 0;
		PostMessage(hWnd, WM_COMMAND, ID_SCREEN_WINDOW1, 0L);
	}
	
	// accelerator
	HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
	
	// disenable ime
	ImmAssociateContext(hWnd, 0);
	
	// initialize emulation core
	emu = new EMU(hWnd, hInstance);
	emu->set_display_size(WINDOW_WIDTH, WINDOW_HEIGHT, TRUE);
	
	// open command line path
	if(szCmdLine[0]) {
		if(szCmdLine[0] == _T('"')) {
			int len = _tcslen(szCmdLine);
			szCmdLine[len - 1] = _T('\0');
			szCmdLine++;
		}
		_TCHAR path[_MAX_PATH];
		get_long_full_path_name(szCmdLine, path);
#ifdef USE_CART
		UPDATE_HISTORY(path, config.recent_cart_path);
		emu->open_cart(path);
		_tcscpy(config.initial_cart_path, get_parent_dir(path));
#elif defined(USE_FD1)
		UPDATE_HISTORY(path, config.recent_disk_path[0]);
		emu->open_disk(path, 0);
		_tcscpy(config.initial_disk_path, get_parent_dir(path));
#elif defined(USE_QUICKDISK)
		UPDATE_HISTORY(path, config.recent_quickdisk_path);
		emu->open_quickdisk(path);
		_tcscpy(config.initial_quickdisk_path, get_parent_dir(path));
#endif
	}
	
	// main loop
	int current_interval = get_interval(), next_interval;
	int skip_frames = 0, rec_cnt = 0, fps = 0, total = 0;
	DWORD next_time = timeGetTime();
	DWORD fps_time = next_time + 1000;
	MSG msg;
	
	while(1) {
		// check window message
		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			if(!GetMessage(&msg, NULL, 0, 0)) {
				ExitProcess(0);	// trick
				return msg.wParam;
			}
			if(!TranslateAccelerator(hWnd, hAccel, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else if(emu) {
			// get next period
			next_time += emu->now_skip() ? 0 : current_interval;
			rec_next_time += current_interval;
			next_interval = get_interval();
			
			// drive machine
			emu->run();
			total++;
			
			if(emu->now_rec_video()) {
				while(rec_next_time >= rec_accum_time) {
					// rec pictures 15/30/60 frames per 1 second
					emu->draw_screen();
					fps++;
					rec_accum_time += rec_delay[rec_cnt];
					rec_cnt = (rec_cnt == 2) ? 0 : rec_cnt + 1;
				}
				
				DWORD tmp = timeGetTime();
				if(next_time > tmp) {
					skip_frames = 0;
					
					// sleep 1 frame priod if need
					if((int)(next_time - tmp) >= next_interval) {
						Sleep(next_interval);
					}
				}
				else if(++skip_frames > MAX_SKIP_FRAMES) {
					skip_frames = 0;
					next_time = tmp;
				}
			}
			else {
				if(next_time > timeGetTime()) {
					if(skip_frames >= MIN_SKIP_FRAMES) {
						// update window if enough time
						emu->draw_screen();
						skip_frames = 0;
						fps++;
					}
					else {
						skip_frames++;
					}
					// sleep 1 frame priod if need
					if((int)(next_time - timeGetTime()) >= next_interval) {
						Sleep(next_interval);
					}
				}
				else if(++skip_frames > MAX_SKIP_FRAMES) {
					// update window at least once per 10 frames
					emu->draw_screen();
					skip_frames = 0;
					fps++;
					next_time = timeGetTime();
				}
			}
			current_interval = next_interval;
			Sleep(0);
			
			// calc frame rate
			if(fps_time <= timeGetTime()) {
				_TCHAR buf[32];
				int ratio = (int)(100 * fps / total + 0.5);
				_stprintf(buf, _T("%s - %d fps (%d %%)"), _T(DEVICE_NAME), fps, ratio);
				SetWindowText(hWnd, buf);
				fps_time += 1000;
				fps = total = 0;
			}
		}
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	_TCHAR path[_MAX_PATH];
	int no;
	
	switch(iMsg) {
	case WM_CREATE:
#ifdef USE_BUTTON
		memset(hFont, 0, sizeof(hFont));
		for(int i = 0; i < MAX_BUTTONS; i++) {
			hButton[i] = CreateWindow(_T("BUTTON"), buttons[i].caption,
			                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE,
			                          buttons[i].x, buttons[i].y,
			                          buttons[i].width, buttons[i].height,
			                          hWnd, (HMENU)(ID_BUTTON + i), (HINSTANCE)GetModuleHandle(0), NULL);
			buttonWndProc[i] = (WNDPROC)(LONG_PTR)GetWindowLong(hButton[i], GWL_WNDPROC);
			SetWindowLong(hButton[i], GWL_WNDPROC, (LONG)(LONG_PTR)ButtonWndProc);
			//HFONT hFont = GetWindowFont(hButton[i]);
			if(!hFont[buttons[i].font_size]) {
				LOGFONT logfont;
				logfont.lfEscapement = 0;
				logfont.lfOrientation = 0;
				logfont.lfWeight = FW_NORMAL;
				logfont.lfItalic = FALSE;
				logfont.lfUnderline = FALSE;
				logfont.lfStrikeOut = FALSE;
				logfont.lfCharSet = DEFAULT_CHARSET;
				logfont.lfOutPrecision = OUT_TT_PRECIS;
				logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
				logfont.lfQuality = DEFAULT_QUALITY;
				logfont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
				_tcscpy(logfont.lfFaceName, _T("Arial"));
				logfont.lfHeight = buttons[i].font_size;
				logfont.lfWidth = buttons[i].font_size >> 1;
				hFont[buttons[i].font_size] = CreateFontIndirect(&logfont);
			}
			SetWindowFont(hButton[i], hFont[buttons[i].font_size], TRUE);
		}
#endif
		break;
	case WM_CLOSE:
		// release emulation core
		if(emu) {
#ifdef USE_POWER_OFF
			// notify power off
			static int notified = 0;
			if(!notified) {
				emu->notify_power_off();
				notified = 1;
				return 0;
			}
#endif
			delete emu;
		}
		emu = NULL;
		save_config();
		// quit fullscreen mode
		if(now_fullscreen) {
			ChangeDisplaySettings(NULL, 0);
		}
		now_fullscreen = FALSE;
#ifdef USE_BUTTON
		for(int i = 0; i < MAX_FONT_SIZE; i++) {
			if(hFont[i]) {
				DeleteObject(hFont[i]);
			}
		}
#endif
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
#ifdef USE_BITMAP
	case WM_SIZE:
		if(emu) {
			emu->reload_bitmap();
		}
		break;
#endif
	case WM_PAINT:
		if(emu) {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			emu->update_screen(hdc);
			EndPaint(hWnd, &ps);
		}
		return 0;
	case WM_MOVING:
		if(emu) {
			emu->mute_sound();
		}
		break;
	case WM_KEYDOWN:
		if(emu) {
			bool repeat = ((HIWORD(lParam) & 0x4000) != 0);
			emu->key_down(LOBYTE(wParam), repeat);
		}
		break;
	case WM_KEYUP:
		if(emu) {
			emu->key_up(LOBYTE(wParam));
		}
		break;
	case WM_SYSKEYDOWN:
		if(emu) {
			bool repeat = ((HIWORD(lParam) & 0x4000) != 0);
			emu->key_down(LOBYTE(wParam), repeat);
		}
#ifdef USE_ALT_F10_KEY
		return 0;	// not activate menu when hit ALT/F10
#endif
		break;
	case WM_SYSKEYUP:
		if(emu) {
			emu->key_up(LOBYTE(wParam));
		}
#ifdef USE_ALT_F10_KEY
		return 0;	// not activate menu when hit ALT/F10
#endif
		break;
	case WM_SYSCHAR:
#ifdef USE_ALT_F10_KEY
		return 0;	// not activate menu when hit ALT/F10
#endif
		break;
	case WM_INITMENUPOPUP:
		if(emu) {
			emu->mute_sound();
		}
		update_menu(hWnd, (HMENU)wParam, LOWORD(lParam));
		break;
	case WM_ENTERMENULOOP:
		now_menuloop = TRUE;
		break;
	case WM_EXITMENULOOP:
		if(now_fullscreen && now_menuloop) {
			hide_menu_bar(hWnd);
		}
		now_menuloop = FALSE;
		break;
	case WM_MOUSEMOVE:
		if(now_fullscreen && !now_menuloop) {
			POINTS p = MAKEPOINTS(lParam);
			if(p.y == 0) {
				show_menu_bar(hWnd);
			}
			else if(p.y > 32) {
				hide_menu_bar(hWnd);
			}
		}
		break;
	case WM_RESIZE:
		if(emu) {
			if(now_fullscreen) {
				emu->set_display_size(-1, -1, FALSE);
			}
			else {
				set_window(hWnd, config.window_mode);
			}
		}
		break;
#ifdef USE_SOCKET
	case WM_SOCKET0:
	case WM_SOCKET1:
	case WM_SOCKET2:
	case WM_SOCKET3:
		no = iMsg - WM_SOCKET0;
		if(!emu) {
			break;
		}
		if(WSAGETSELECTERROR(lParam) != 0) {
			emu->disconnect_socket(no);
			emu->socket_disconnected(no);
			break;
		}
		if(emu->get_socket(no) != (int)wParam) {
			break;
		}
		switch(WSAGETSELECTEVENT(lParam)) {
		case FD_CONNECT:
			emu->socket_connected(no);
			break;
		case FD_CLOSE:
			emu->socket_disconnected(no);
			break;
		case FD_WRITE:
			emu->send_data(no);
			break;
		case FD_READ:
			emu->recv_data(no);
			break;
		}
		break;
#endif
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case ID_RESET:
			if(emu) {
				emu->reset();
			}
			rec_next_time = rec_accum_time = 0;
			break;
#ifdef USE_SPECIAL_RESET
		case ID_SPECIAL_RESET:
			if(emu) {
				emu->special_reset();
			}
			rec_next_time = rec_accum_time = 0;
			break;
#endif
#ifdef USE_DIPSWITCH
		case ID_DIPSWITCH1:
		case ID_DIPSWITCH2:
		case ID_DIPSWITCH3:
		case ID_DIPSWITCH4:
		case ID_DIPSWITCH5:
		case ID_DIPSWITCH6:
		case ID_DIPSWITCH7:
		case ID_DIPSWITCH8:
			config.dipswitch ^= (1 << (LOWORD(wParam) - ID_DIPSWITCH1));
			break;
#endif
#ifdef _HC80
		case ID_HC80_RAMDISK0:
		case ID_HC80_RAMDISK1:
		case ID_HC80_RAMDISK2:
			config.ramdisk_type = LOWORD(wParam) - ID_HC80_RAMDISK0;
			break;
#endif
#ifdef _MZ800
		case ID_MZ800_BOOT_MODE_MZ800:
		case ID_MZ800_BOOT_MODE_MZ700:
			config.boot_mode = LOWORD(wParam) - ID_MZ800_BOOT_MODE_MZ800;
			if(emu) {
				emu->update_config();
			}
			break;
#endif
#ifdef _PC98DO
		case ID_PC98DO_BOOT_MODE_PC98:
		case ID_PC98DO_BOOT_MODE_PC88_V1S:
		case ID_PC98DO_BOOT_MODE_PC88_V1H:
		case ID_PC98DO_BOOT_MODE_PC88_V2:
		case ID_PC98DO_BOOT_MODE_PC88_N:
			config.boot_mode = LOWORD(wParam) - ID_PC98DO_BOOT_MODE_PC98;
			if(emu) {
				emu->update_config();
			}
			break;
#endif
		case ID_CPU_POWER0:
		case ID_CPU_POWER1:
		case ID_CPU_POWER2:
		case ID_CPU_POWER3:
		case ID_CPU_POWER4:
			config.cpu_power = LOWORD(wParam) - ID_CPU_POWER0;
			if(emu) {
				emu->update_config();
			}
			break;
#ifdef USE_AUTO_KEY
		case ID_AUTOKEY_START:
			if(emu) {
				emu->start_auto_key();
			}
			break;
		case ID_AUTOKEY_STOP:
			if(emu) {
				emu->stop_auto_key();
			}
			break;
#endif
		case ID_EXIT:
			SendMessage(hWnd, WM_CLOSE, 0, 0L);
			break;
#ifdef USE_CART
		case ID_OPEN_CART:
			if(emu) {
				open_cart(hWnd);
			}
			break;
		case ID_CLOSE_CART:
			if(emu) {
				emu->close_cart();
			}
			break;
		case ID_RECENT_CART1:
		case ID_RECENT_CART2:
		case ID_RECENT_CART3:
		case ID_RECENT_CART4:
		case ID_RECENT_CART5:
		case ID_RECENT_CART6:
		case ID_RECENT_CART7:
		case ID_RECENT_CART8:
			no = LOWORD(wParam) - ID_RECENT_CART1;
			_tcscpy(path, config.recent_cart_path[no]);
			for(int i = no; i > 0; i--) {
				_tcscpy(config.recent_cart_path[i], config.recent_cart_path[i - 1]);
			}
			_tcscpy(config.recent_cart_path[0], path);
			if(emu) {
				emu->open_cart(path);
			}
			break;
#endif
#ifdef USE_FD1
		case ID_OPEN_FD1:
			if(emu) {
				open_disk(hWnd, 0);
			}
			break;
		case ID_CLOSE_FD1:
			if(emu) {
				emu->close_disk(0);
			}
			break;
		case ID_RECENT_FD11:
		case ID_RECENT_FD12:
		case ID_RECENT_FD13:
		case ID_RECENT_FD14:
		case ID_RECENT_FD15:
		case ID_RECENT_FD16:
		case ID_RECENT_FD17:
		case ID_RECENT_FD18:
			no = LOWORD(wParam) - ID_RECENT_FD11;
			_tcscpy(path, config.recent_disk_path[0][no]);
			for(int i = no; i > 0; i--) {
				_tcscpy(config.recent_disk_path[0][i], config.recent_disk_path[0][i - 1]);
			}
			_tcscpy(config.recent_disk_path[0][0], path);
			if(emu) {
				emu->open_disk(path, 0);
			}
			break;
#endif
#ifdef USE_FD2
		case ID_OPEN_FD2:
			if(emu) {
				open_disk(hWnd, 1);
			}
			break;
		case ID_CLOSE_FD2:
			if(emu) {
				emu->close_disk(1);
			}
			break;
		case ID_RECENT_FD21:
		case ID_RECENT_FD22:
		case ID_RECENT_FD23:
		case ID_RECENT_FD24:
		case ID_RECENT_FD25:
		case ID_RECENT_FD26:
		case ID_RECENT_FD27:
		case ID_RECENT_FD28:
			no = LOWORD(wParam) - ID_RECENT_FD21;
			_tcscpy(path, config.recent_disk_path[1][no]);
			for(int i = no; i > 0; i--) {
				_tcscpy(config.recent_disk_path[1][i], config.recent_disk_path[1][i - 1]);
			}
			_tcscpy(config.recent_disk_path[1][0], path);
			if(emu) {
				emu->open_disk(path, 1);
			}
			break;
#endif
#ifdef USE_FD3
		case ID_OPEN_FD3:
			if(emu) {
				open_disk(hWnd, 2);
			}
			break;
		case ID_CLOSE_FD3:
			if(emu) {
				emu->close_disk(2);
			}
			break;
		case ID_RECENT_FD31:
		case ID_RECENT_FD32:
		case ID_RECENT_FD33:
		case ID_RECENT_FD34:
		case ID_RECENT_FD35:
		case ID_RECENT_FD36:
		case ID_RECENT_FD37:
		case ID_RECENT_FD38:
			no = LOWORD(wParam) - ID_RECENT_FD31;
			_tcscpy(path, config.recent_disk_path[2][no]);
			for(int i = no; i > 0; i--) {
				_tcscpy(config.recent_disk_path[2][i], config.recent_disk_path[2][i - 1]);
			}
			_tcscpy(config.recent_disk_path[2][0], path);
			if(emu) {
				emu->open_disk(path, 2);
			}
			break;
#endif
#ifdef USE_FD4
		case ID_OPEN_FD4:
			if(emu) {
				open_disk(hWnd, 3);
			}
			break;
		case ID_CLOSE_FD4:
			if(emu) {
				emu->close_disk(3);
			}
			break;
		case ID_RECENT_FD41:
		case ID_RECENT_FD42:
		case ID_RECENT_FD43:
		case ID_RECENT_FD44:
		case ID_RECENT_FD45:
		case ID_RECENT_FD46:
		case ID_RECENT_FD47:
		case ID_RECENT_FD48:
			no = LOWORD(wParam) - ID_RECENT_FD41;
			_tcscpy(path, config.recent_disk_path[3][no]);
			for(int i = no; i > 0; i--) {
				_tcscpy(config.recent_disk_path[3][i], config.recent_disk_path[3][i - 1]);
			}
			_tcscpy(config.recent_disk_path[3][0], path);
			if(emu) {
				emu->open_disk(path, 3);
			}
			break;
#endif
#ifdef USE_FD5
		case ID_OPEN_FD5:
			if(emu) {
				open_disk(hWnd, 4);
			}
			break;
		case ID_CLOSE_FD5:
			if(emu) {
				emu->close_disk(4);
			}
			break;
		case ID_RECENT_FD51:
		case ID_RECENT_FD52:
		case ID_RECENT_FD53:
		case ID_RECENT_FD54:
		case ID_RECENT_FD55:
		case ID_RECENT_FD56:
		case ID_RECENT_FD57:
		case ID_RECENT_FD58:
			no = LOWORD(wParam) - ID_RECENT_FD51;
			_tcscpy(path, config.recent_disk_path[4][no]);
			for(int i = no; i > 0; i--) {
				_tcscpy(config.recent_disk_path[4][i], config.recent_disk_path[4][i - 1]);
			}
			_tcscpy(config.recent_disk_path[4][0], path);
			if(emu) {
				emu->open_disk(path, 4);
			}
			break;
#endif
#ifdef USE_FD6
		case ID_OPEN_FD6:
			if(emu) {
				open_disk(hWnd, 5);
			}
			break;
		case ID_CLOSE_FD6:
			if(emu) {
				emu->close_disk(5);
			}
			break;
		case ID_RECENT_FD61:
		case ID_RECENT_FD62:
		case ID_RECENT_FD63:
		case ID_RECENT_FD64:
		case ID_RECENT_FD65:
		case ID_RECENT_FD66:
		case ID_RECENT_FD67:
		case ID_RECENT_FD68:
			no = LOWORD(wParam) - ID_RECENT_FD61;
			_tcscpy(path, config.recent_disk_path[5][no]);
			for(int i = no; i > 0; i--) {
				_tcscpy(config.recent_disk_path[5][i], config.recent_disk_path[5][i - 1]);
			}
			_tcscpy(config.recent_disk_path[5][0], path);
			if(emu) {
				emu->open_disk(path, 5);
			}
			break;
#endif
#ifdef USE_QUICKDISK
		case ID_OPEN_QUICKDISK:
			if(emu) {
				open_quickdisk(hWnd);
			}
			break;
		case ID_CLOSE_QUICKDISK:
			if(emu) {
				emu->close_quickdisk();
			}
			break;
		case ID_RECENT_QUICKDISK1:
		case ID_RECENT_QUICKDISK2:
		case ID_RECENT_QUICKDISK3:
		case ID_RECENT_QUICKDISK4:
		case ID_RECENT_QUICKDISK5:
		case ID_RECENT_QUICKDISK6:
		case ID_RECENT_QUICKDISK7:
		case ID_RECENT_QUICKDISK8:
			no = LOWORD(wParam) - ID_RECENT_QUICKDISK1;
			_tcscpy(path, config.recent_quickdisk_path[no]);
			for(int i = no; i > 0; i--) {
				_tcscpy(config.recent_quickdisk_path[i], config.recent_quickdisk_path[i - 1]);
			}
			_tcscpy(config.recent_quickdisk_path[0], path);
			if(emu) {
				emu->open_quickdisk(path);
			}
			break;
#endif
#ifdef USE_DATAREC
		case ID_PLAY_DATAREC:
			if(emu) {
				open_datarec(hWnd, TRUE);
			}
			break;
		case ID_REC_DATAREC:
			if(emu) {
				open_datarec(hWnd, FALSE);
			}
			break;
		case ID_CLOSE_DATAREC:
			if(emu) {
				emu->close_datarec();
			}
			break;
		case ID_RECENT_DATAREC1:
		case ID_RECENT_DATAREC2:
		case ID_RECENT_DATAREC3:
		case ID_RECENT_DATAREC4:
		case ID_RECENT_DATAREC5:
		case ID_RECENT_DATAREC6:
		case ID_RECENT_DATAREC7:
		case ID_RECENT_DATAREC8:
			no = LOWORD(wParam) - ID_RECENT_DATAREC1;
			_tcscpy(path, config.recent_datarec_path[no]);
			for(int i = no; i > 0; i--) {
				_tcscpy(config.recent_datarec_path[i], config.recent_datarec_path[i - 1]);
			}
			_tcscpy(config.recent_datarec_path[0], path);
			if(emu) {
				emu->play_datarec(path);
			}
			break;
#endif
#ifdef USE_DATAREC_BUTTON
		case ID_PLAY_BUTTON:
			if(emu) {
				emu->push_play();
			}
			break;
		case ID_STOP_BUTTON:
			if(emu) {
				emu->push_stop();
			}
			break;
#endif
#ifdef USE_MEDIA
		case ID_OPEN_MEDIA:
			if(emu) {
				open_media(hWnd);
			}
			break;
		case ID_CLOSE_MEDIA:
			if(emu) {
				emu->close_media();
			}
			break;
		case ID_RECENT_MEDIA1:
		case ID_RECENT_MEDIA2:
		case ID_RECENT_MEDIA3:
		case ID_RECENT_MEDIA4:
		case ID_RECENT_MEDIA5:
		case ID_RECENT_MEDIA6:
		case ID_RECENT_MEDIA7:
		case ID_RECENT_MEDIA8:
			no = LOWORD(wParam) - ID_RECENT_MEDIA1;
			_tcscpy(path, config.recent_media_path[no]);
			for(int i = no; i > 0; i--) {
				_tcscpy(config.recent_media_path[i], config.recent_media_path[i - 1]);
			}
			_tcscpy(config.recent_media_path[0], path);
			if(emu) {
				emu->open_media(path);
			}
			break;
#endif
#ifdef USE_RAM
		case ID_LOAD_RAM:
			if(emu) {
				open_ram(hWnd, TRUE);
			}
			break;
		case ID_SAVE_RAM:
			if(emu) {
				open_ram(hWnd, FALSE);
			}
			break;
		case ID_RECENT_RAM1:
		case ID_RECENT_RAM2:
		case ID_RECENT_RAM3:
		case ID_RECENT_RAM4:
		case ID_RECENT_RAM5:
		case ID_RECENT_RAM6:
		case ID_RECENT_RAM7:
		case ID_RECENT_RAM8:
			no = LOWORD(wParam) - ID_RECENT_RAM1;
			_tcscpy(path, config.recent_ram_path[no]);
			for(int i = no; i > 0; i--) {
				_tcscpy(config.recent_ram_path[i], config.recent_ram_path[i - 1]);
			}
			_tcscpy(config.recent_ram_path[0], path);
			if(emu) {
				emu->load_ram(path);
			}
			break;
#endif
		case ID_SCREEN_REC60:
		case ID_SCREEN_REC30:
		case ID_SCREEN_REC15:
			if(emu) {
				static int fps[3] = {60, 30, 15};
				static int delay[3][3] = {{16, 33, 66}, {17, 33, 67}, {17, 34, 67}};
				no = LOWORD(wParam) - ID_SCREEN_REC60;
				emu->start_rec_video(fps[no], TRUE);
				emu->start_rec_sound();
				rec_delay[0] = delay[0][no];
				rec_delay[1] = delay[1][no];
				rec_delay[2] = delay[2][no];
				rec_next_time = rec_accum_time = 0;
			}
			break;
		case ID_SCREEN_STOP:
			if(emu) {
				emu->stop_rec_video();
				emu->stop_rec_sound();
			}
			break;
		case ID_SCREEN_CAPTURE:
			if(emu) {
				emu->capture_screen();
			}
			break;
		case ID_SCREEN_WINDOW1:
		case ID_SCREEN_WINDOW2:
		case ID_SCREEN_WINDOW3:
		case ID_SCREEN_WINDOW4:
		case ID_SCREEN_WINDOW5:
		case ID_SCREEN_WINDOW6:
		case ID_SCREEN_WINDOW7:
		case ID_SCREEN_WINDOW8:
			if(emu) {
				set_window(hWnd, LOWORD(wParam) - ID_SCREEN_WINDOW1);
			}
			break;
		case ID_SCREEN_FULLSCREEN1:
		case ID_SCREEN_FULLSCREEN2:
		case ID_SCREEN_FULLSCREEN3:
		case ID_SCREEN_FULLSCREEN4:
		case ID_SCREEN_FULLSCREEN5:
		case ID_SCREEN_FULLSCREEN6:
		case ID_SCREEN_FULLSCREEN7:
		case ID_SCREEN_FULLSCREEN8:
		case ID_SCREEN_FULLSCREEN9:
		case ID_SCREEN_FULLSCREEN10:
		case ID_SCREEN_FULLSCREEN11:
		case ID_SCREEN_FULLSCREEN12:
		case ID_SCREEN_FULLSCREEN13:
		case ID_SCREEN_FULLSCREEN14:
		case ID_SCREEN_FULLSCREEN15:
		case ID_SCREEN_FULLSCREEN16:
		case ID_SCREEN_FULLSCREEN17:
		case ID_SCREEN_FULLSCREEN18:
		case ID_SCREEN_FULLSCREEN19:
		case ID_SCREEN_FULLSCREEN20:
			if(emu && !now_fullscreen) {
				set_window(hWnd, LOWORD(wParam) - ID_SCREEN_FULLSCREEN1 + 8);
			}
			break;
		case ID_SCREEN_STRETCH:
			config.stretch_screen = !config.stretch_screen;
			if(emu) {
				emu->set_display_size(-1, -1, !now_fullscreen);
			}
			break;
		// accelerator
		case ID_ACCEL_SCREEN:
			if(emu) {
				emu->mute_sound();
				set_window(hWnd, now_fullscreen ? prev_window_mode : -1);
			}
			break;
		case ID_ACCEL_MOUSE:
			if(emu) {
				emu->toggle_mouse();
			}
			break;
#if defined(USE_MONITOR_TYPE) || defined(USE_SCREEN_ROTATE)
		case ID_SCREEN_MONITOR_TYPE0:
		case ID_SCREEN_MONITOR_TYPE1:
		case ID_SCREEN_MONITOR_TYPE2:
		case ID_SCREEN_MONITOR_TYPE3:
			config.monitor_type = LOWORD(wParam) - ID_SCREEN_MONITOR_TYPE0;
			if(emu) {
				emu->update_config();
#ifdef USE_SCREEN_ROTATE
				if(now_fullscreen) {
					emu->set_display_size(-1, -1, FALSE);
				}
				else {
					set_window(hWnd, prev_window_mode);
				}
#endif
			}
			break;
#endif
#ifdef USE_SCANLINE
		case ID_SCREEN_SCANLINE:
			config.scan_line = !config.scan_line;
			if(emu) {
				emu->update_config();
			}
			break;
#endif
		case ID_SOUND_REC:
			if(emu) {
				emu->start_rec_sound();
			}
			break;
		case ID_SOUND_STOP:
			if(emu) {
				emu->stop_rec_sound();
			}
			break;
		case ID_SOUND_FREQ0:
		case ID_SOUND_FREQ1:
		case ID_SOUND_FREQ2:
		case ID_SOUND_FREQ3:
		case ID_SOUND_FREQ4:
		case ID_SOUND_FREQ5:
		case ID_SOUND_FREQ6:
		case ID_SOUND_FREQ7:
			config.sound_frequency = LOWORD(wParam) - ID_SOUND_FREQ0;
			if(emu) {
				emu->update_config();
			}
			break;
		case ID_SOUND_LATE0:
		case ID_SOUND_LATE1:
		case ID_SOUND_LATE2:
		case ID_SOUND_LATE3:
		case ID_SOUND_LATE4:
			config.sound_latency = LOWORD(wParam) - ID_SOUND_LATE0;
			if(emu) {
				emu->update_config();
			}
			break;
#ifdef USE_SOUND_DEVICE_TYPE
		case ID_SOUND_DEVICE_TYPE0:
		case ID_SOUND_DEVICE_TYPE1:
		case ID_SOUND_DEVICE_TYPE2:
		case ID_SOUND_DEVICE_TYPE3:
			config.sound_device_type = LOWORD(wParam) - ID_SOUND_DEVICE_TYPE0;
			//if(emu) {
			//	emu->update_config();
			//}
			break;
#endif
#ifdef USE_BUTTON
		case ID_BUTTON +  0:
		case ID_BUTTON +  1:
		case ID_BUTTON +  2:
		case ID_BUTTON +  3:
		case ID_BUTTON +  4:
		case ID_BUTTON +  5:
		case ID_BUTTON +  6:
		case ID_BUTTON +  7:
		case ID_BUTTON +  8:
		case ID_BUTTON +  9:
		case ID_BUTTON + 10:
		case ID_BUTTON + 11:
		case ID_BUTTON + 12:
		case ID_BUTTON + 13:
		case ID_BUTTON + 14:
		case ID_BUTTON + 15:
		case ID_BUTTON + 16:
		case ID_BUTTON + 17:
		case ID_BUTTON + 18:
		case ID_BUTTON + 19:
		case ID_BUTTON + 20:
		case ID_BUTTON + 21:
		case ID_BUTTON + 22:
		case ID_BUTTON + 23:
		case ID_BUTTON + 24:
		case ID_BUTTON + 25:
		case ID_BUTTON + 26:
		case ID_BUTTON + 27:
		case ID_BUTTON + 28:
		case ID_BUTTON + 29:
		case ID_BUTTON + 30:
		case ID_BUTTON + 31:
			if(emu) {
				emu->press_button(LOWORD(wParam) - ID_BUTTON);
			}
			break;
#endif
		}
		break;
	}
	return DefWindowProc(hWnd, iMsg, wParam, lParam) ;
}

#ifdef USE_BUTTON
LRESULT CALLBACK ButtonWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	for(int i = 0; i < MAX_BUTTONS; i++) {
		if(hWnd == hButton[i]) {
			switch(iMsg) {
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				if(emu) {
					emu->key_down(LOBYTE(wParam), false);
				}
				return 0;
			case WM_KEYUP:
			case WM_SYSKEYUP:
				if(emu) {
					emu->key_up(LOBYTE(wParam));
				}
				return 0;
			}
			return CallWindowProc(buttonWndProc[i], hWnd, iMsg, wParam, lParam);
		}
	}
	return 0;
}
#endif

void update_menu(HWND hWnd, HMENU hMenu, int pos)
{
#ifdef MENU_POS_CONTROL
	if(pos == MENU_POS_CONTROL) {
		// control menu
#ifdef USE_DIPSWITCH
		for(int i = 0; i < 8; i++) {
			CheckMenuItem(hMenu, ID_DIPSWITCH1 + i, !(config.dipswitch & (1 << i)) ? MF_CHECKED : MF_UNCHECKED);
		}
#endif
#ifdef _HC80
		if(config.ramdisk_type >= 0 && config.ramdisk_type < 3) {
			CheckMenuRadioItem(hMenu, ID_HC80_RAMDISK0, ID_HC80_RAMDISK2, ID_HC80_RAMDISK0 + config.ramdisk_type, MF_BYCOMMAND);
		}
#endif
#ifdef _MZ800
		if(config.boot_mode >= 0 && config.boot_mode < 2) {
			CheckMenuRadioItem(hMenu, ID_MZ800_BOOT_MODE_MZ800, ID_MZ800_BOOT_MODE_MZ700, ID_MZ800_BOOT_MODE_MZ800 + config.boot_mode, MF_BYCOMMAND);
		}
#endif
#ifdef _PC98DO
		if(config.boot_mode >= 0 && config.boot_mode < 5) {
			CheckMenuRadioItem(hMenu, ID_PC98DO_BOOT_MODE_PC98, ID_PC98DO_BOOT_MODE_PC88_N, ID_PC98DO_BOOT_MODE_PC98 + config.boot_mode, MF_BYCOMMAND);
		}
#endif
		if(config.cpu_power >= 0 && config.cpu_power < 5) {
			CheckMenuRadioItem(hMenu, ID_CPU_POWER0, ID_CPU_POWER4, ID_CPU_POWER0 + config.cpu_power, MF_BYCOMMAND);
		}
#ifdef USE_AUTO_KEY
		// auto key
		BOOL now_paste = TRUE, now_stop = TRUE;
		if(emu) {
			now_paste = emu->now_auto_key();
			now_stop = !now_paste;
		}
		EnableMenuItem(hMenu, ID_AUTOKEY_START, now_paste ? MF_GRAYED : MF_ENABLED);
		EnableMenuItem(hMenu, ID_AUTOKEY_STOP, now_stop ? MF_GRAYED : MF_ENABLED);
#endif
	}
#endif
#ifdef MENU_POS_CART
	else if(pos == MENU_POS_CART) {
		// cartridge
		BOOL flag = FALSE;
		for(int i = 0; i < 8; i++) {
			DeleteMenu(hMenu, ID_RECENT_CART1 + i, MF_BYCOMMAND);
		}
		for(int i = 0; i < 8; i++) {
			if(_tcscmp(config.recent_cart_path[i], _T(""))) {
				AppendMenu(hMenu, MF_STRING, ID_RECENT_CART1 + i, config.recent_cart_path[i]);
				flag = TRUE;
			}
		}
		if(!flag) {
			AppendMenu(hMenu, MF_GRAYED | MF_STRING, ID_RECENT_CART1, _T("None"));
		}
	}
#endif
#ifdef MENU_POS_FD1
	else if(pos == MENU_POS_FD1) {
		// floppy drive #1
		BOOL flag = FALSE;
		for(int i = 0; i < 8; i++) {
			DeleteMenu(hMenu, ID_RECENT_FD11 + i, MF_BYCOMMAND);
		}
		for(int i = 0; i < 8; i++) {
			if(_tcscmp(config.recent_disk_path[0][i], _T(""))) {
				AppendMenu(hMenu, MF_STRING, ID_RECENT_FD11 + i, config.recent_disk_path[0][i]);
				flag = TRUE;
			}
		}
		if(!flag) {
			AppendMenu(hMenu, MF_GRAYED | MF_STRING, ID_RECENT_FD11, _T("None"));
		}
	}
#endif
#ifdef MENU_POS_FD2
	else if(pos == MENU_POS_FD2) {
		// floppy drive #2
		BOOL flag = FALSE;
		for(int i = 0; i < 8; i++) {
			DeleteMenu(hMenu, ID_RECENT_FD21 + i, MF_BYCOMMAND);
		}
		for(int i = 0; i < 8; i++) {
			if(_tcscmp(config.recent_disk_path[1][i], _T(""))) {
				AppendMenu(hMenu, MF_STRING, ID_RECENT_FD21 + i, config.recent_disk_path[1][i]);
				flag = TRUE;
			}
		}
		if(!flag) {
			AppendMenu(hMenu, MF_GRAYED | MF_STRING, ID_RECENT_FD21, _T("None"));
		}
	}
#endif
#ifdef MENU_POS_FD3
	else if(pos == MENU_POS_FD3) {
		// floppy drive #3
		BOOL flag = FALSE;
		for(int i = 0; i < 8; i++) {
			DeleteMenu(hMenu, ID_RECENT_FD31 + i, MF_BYCOMMAND);
		}
		for(int i = 0; i < 8; i++) {
			if(_tcscmp(config.recent_disk_path[2][i], _T(""))) {
				AppendMenu(hMenu, MF_STRING, ID_RECENT_FD31 + i, config.recent_disk_path[2][i]);
				flag = TRUE;
			}
		}
		if(!flag) {
			AppendMenu(hMenu, MF_GRAYED | MF_STRING, ID_RECENT_FD31, _T("None"));
		}
	}
#endif
#ifdef MENU_POS_FD4
	else if(pos == MENU_POS_FD4) {
		// floppy drive #4
		BOOL flag = FALSE;
		for(int i = 0; i < 8; i++) {
			DeleteMenu(hMenu, ID_RECENT_FD41 + i, MF_BYCOMMAND);
		}
		for(int i = 0; i < 8; i++) {
			if(_tcscmp(config.recent_disk_path[3][i], _T(""))) {
				AppendMenu(hMenu, MF_STRING, ID_RECENT_FD41 + i, config.recent_disk_path[3][i]);
				flag = TRUE;
			}
		}
		if(!flag) {
			AppendMenu(hMenu, MF_GRAYED | MF_STRING, ID_RECENT_FD41, _T("None"));
		}
	}
#endif
#ifdef MENU_POS_FD5
	else if(pos == MENU_POS_FD5) {
		// floppy drive #5
		BOOL flag = FALSE;
		for(int i = 0; i < 8; i++) {
			DeleteMenu(hMenu, ID_RECENT_FD51 + i, MF_BYCOMMAND);
		}
		for(int i = 0; i < 8; i++) {
			if(_tcscmp(config.recent_disk_path[4][i], _T(""))) {
				AppendMenu(hMenu, MF_STRING, ID_RECENT_FD51 + i, config.recent_disk_path[4][i]);
				flag = TRUE;
			}
		}
		if(!flag) {
			AppendMenu(hMenu, MF_GRAYED | MF_STRING, ID_RECENT_FD51, _T("None"));
		}
	}
#endif
#ifdef MENU_POS_FD6
	else if(pos == MENU_POS_FD6) {
		// floppy drive #6
		BOOL flag = FALSE;
		for(int i = 0; i < 8; i++) {
			DeleteMenu(hMenu, ID_RECENT_FD61 + i, MF_BYCOMMAND);
		}
		for(int i = 0; i < 8; i++) {
			if(_tcscmp(config.recent_disk_path[5][i], _T(""))) {
				AppendMenu(hMenu, MF_STRING, ID_RECENT_FD61 + i, config.recent_disk_path[5][i]);
				flag = TRUE;
			}
		}
		if(!flag) {
			AppendMenu(hMenu, MF_GRAYED | MF_STRING, ID_RECENT_FD61, _T("None"));
		}
	}
#endif
#ifdef MENU_POS_QUICKDISK
	else if(pos == MENU_POS_QUICKDISK) {
		// quick disk drive
		BOOL flag = FALSE;
		for(int i = 0; i < 8; i++) {
			DeleteMenu(hMenu, ID_RECENT_QUICKDISK1 + i, MF_BYCOMMAND);
		}
		for(int i = 0; i < 8; i++) {
			if(_tcscmp(config.recent_quickdisk_path[i], _T(""))) {
				AppendMenu(hMenu, MF_STRING, ID_RECENT_QUICKDISK1 + i, config.recent_quickdisk_path[i]);
				flag = TRUE;
			}
		}
		if(!flag) {
			AppendMenu(hMenu, MF_GRAYED | MF_STRING, ID_RECENT_QUICKDISK1, _T("None"));
		}
	}
#endif
#ifdef MENU_POS_DATAREC
	else if(pos == MENU_POS_DATAREC) {
		// data recorder
		BOOL flag = FALSE;
		for(int i = 0; i < 8; i++) {
			DeleteMenu(hMenu, ID_RECENT_DATAREC1 + i, MF_BYCOMMAND);
		}
		for(int i = 0; i < 8; i++) {
			if(_tcscmp(config.recent_datarec_path[i], _T(""))) {
				AppendMenu(hMenu, MF_STRING, ID_RECENT_DATAREC1 + i, config.recent_datarec_path[i]);
				flag = TRUE;
			}
		}
		if(!flag) {
			AppendMenu(hMenu, MF_GRAYED | MF_STRING, ID_RECENT_DATAREC1, _T("None"));
		}
	}
#endif
#ifdef MENU_POS_MEDIA
	else if(pos == MENU_POS_MEDIA) {
		// media
		BOOL flag = FALSE;
		for(int i = 0; i < 8; i++) {
			DeleteMenu(hMenu, ID_RECENT_MEDIA1 + i, MF_BYCOMMAND);
		}
		for(int i = 0; i < 8; i++) {
			if(_tcscmp(config.recent_media_path[i], _T(""))) {
				AppendMenu(hMenu, MF_STRING, ID_RECENT_MEDIA1 + i, config.recent_media_path[i]);
				flag = TRUE;
			}
		}
		if(!flag) {
			AppendMenu(hMenu, MF_GRAYED | MF_STRING, ID_RECENT_MEDIA1, _T("None"));
		}
	}
#endif
#ifdef MENU_POS_RAM
	else if(pos == MENU_POS_RAM) {
		// ram
		BOOL flag = FALSE;
		for(int i = 0; i < 8; i++) {
			DeleteMenu(hMenu, ID_RECENT_RAM1 + i, MF_BYCOMMAND);
		}
		for(int i = 0; i < 8; i++) {
			if(_tcscmp(config.recent_ram_path[i], _T(""))) {
				AppendMenu(hMenu, MF_STRING, ID_RECENT_RAM1 + i, config.recent_ram_path[i]);
				flag = TRUE;
			}
		}
		if(!flag) {
			AppendMenu(hMenu, MF_GRAYED | MF_STRING, ID_RECENT_RAM1, _T("None"));
		}
	}
#endif
#ifdef MENU_POS_SCREEN
	else if(pos == MENU_POS_SCREEN) {
		// recording
		BOOL now_rec = TRUE, now_stop = TRUE;
		if(emu) {
			now_rec = emu->now_rec_video();
			now_stop = !now_rec;
		}
		EnableMenuItem(hMenu, ID_SCREEN_REC60, now_rec ? MF_GRAYED : MF_ENABLED);
		EnableMenuItem(hMenu, ID_SCREEN_REC30, now_rec ? MF_GRAYED : MF_ENABLED);
		EnableMenuItem(hMenu, ID_SCREEN_REC15, now_rec ? MF_GRAYED : MF_ENABLED);
		EnableMenuItem(hMenu, ID_SCREEN_STOP, now_stop ? MF_GRAYED : MF_ENABLED);
		
		// screen mode
		UINT last = ID_SCREEN_WINDOW1;
		for(int i = 1; i < 8; i++) {
			DeleteMenu(hMenu, ID_SCREEN_WINDOW1 + i, MF_BYCOMMAND);
		}
		for(int i = 1; i < 8; i++) {
			if(emu && emu->get_window_width(i) <= desktop_width && emu->get_window_height(i) <= desktop_height) {
				_TCHAR buf[16];
				_stprintf(buf, _T("Window x%d"), i + 1);
				InsertMenu(hMenu, ID_SCREEN_FULLSCREEN1, MF_BYCOMMAND | MF_STRING, ID_SCREEN_WINDOW1 + i, buf);
				last = ID_SCREEN_WINDOW1 + i;
			}
		}
		for(int i = 0; i < 20; i++) {
			if(i < screen_mode_count) {
				MENUITEMINFO info;
				ZeroMemory(&info, sizeof(info));
				info.cbSize = sizeof(info);
				_TCHAR buf[64];
				_stprintf(buf, _T("Fullscreen %dx%d"), screen_mode_width[i], screen_mode_height[i]);
				info.fMask = MIIM_TYPE;
				info.fType = MFT_STRING;
				info.dwTypeData = buf;
				SetMenuItemInfo(hMenu, ID_SCREEN_FULLSCREEN1 + i, FALSE, &info);
				EnableMenuItem(hMenu, ID_SCREEN_FULLSCREEN1 + i, now_fullscreen ? MF_GRAYED : MF_ENABLED);
				last = ID_SCREEN_FULLSCREEN1 + i;
			}
			else {
				DeleteMenu(hMenu, ID_SCREEN_FULLSCREEN1 + i, MF_BYCOMMAND);
			}
		}
		if(config.window_mode >= 0 && config.window_mode < 8) {
			CheckMenuRadioItem(hMenu, ID_SCREEN_WINDOW1, last, ID_SCREEN_WINDOW1 + config.window_mode, MF_BYCOMMAND);
		}
		else if(config.window_mode >= 8 && config.window_mode < screen_mode_count + 8) {
			CheckMenuRadioItem(hMenu, ID_SCREEN_WINDOW1, last, ID_SCREEN_FULLSCREEN1 + config.window_mode - 8, MF_BYCOMMAND);
		}
		// stretch screen
		CheckMenuItem(hMenu, ID_SCREEN_STRETCH, config.stretch_screen ? MF_CHECKED : MF_UNCHECKED);
		
#ifdef USE_MONITOR_TYPE
		if(config.monitor_type >= 0 && config.monitor_type < USE_MONITOR_TYPE) {
			CheckMenuRadioItem(hMenu, ID_SCREEN_MONITOR_TYPE0, ID_SCREEN_MONITOR_TYPE0 + USE_MONITOR_TYPE - 1, ID_SCREEN_MONITOR_TYPE0 + config.monitor_type, MF_BYCOMMAND);
		}
#elif defined(USE_SCREEN_ROTATE)
		if(config.monitor_type >= 0 && config.monitor_type < 2) {
			CheckMenuRadioItem(hMenu, ID_SCREEN_MONITOR_TYPE0, ID_SCREEN_MONITOR_TYPE1, ID_SCREEN_MONITOR_TYPE0 + config.monitor_type, MF_BYCOMMAND);
		}
#endif
#ifdef USE_SCANLINE
		// scanline
		CheckMenuItem(hMenu, ID_SCREEN_SCANLINE, config.scan_line ? MF_CHECKED : MF_UNCHECKED);
#endif
	}
#endif
#ifdef MENU_POS_SOUND
	else if(pos == MENU_POS_SOUND) {
		// sound menu
		BOOL now_rec = FALSE, now_stop = FALSE;
		if(emu) {
			now_rec = emu->now_rec_sound();
			now_stop = !now_rec;
		}
		EnableMenuItem(hMenu, ID_SOUND_REC, now_rec ? MF_GRAYED : MF_ENABLED);
		EnableMenuItem(hMenu, ID_SOUND_STOP, now_stop ? MF_GRAYED : MF_ENABLED);
		
		if(config.sound_frequency >= 0 && config.sound_frequency < 8) {
			CheckMenuRadioItem(hMenu, ID_SOUND_FREQ0, ID_SOUND_FREQ7, ID_SOUND_FREQ0 + config.sound_frequency, MF_BYCOMMAND);
		}
		if(config.sound_latency >= 0 && config.sound_latency < 5) {
			CheckMenuRadioItem(hMenu, ID_SOUND_LATE0, ID_SOUND_LATE4, ID_SOUND_LATE0 + config.sound_latency, MF_BYCOMMAND);
		}
#ifdef USE_SOUND_DEVICE_TYPE
		if(config.sound_device_type >= 0 && config.sound_device_type < USE_SOUND_DEVICE_TYPE) {
			CheckMenuRadioItem(hMenu, ID_SOUND_DEVICE_TYPE0, ID_SOUND_DEVICE_TYPE0 + USE_SOUND_DEVICE_TYPE - 1, ID_SOUND_DEVICE_TYPE0 + config.sound_device_type, MF_BYCOMMAND);
		}
#endif
	}
#endif
	DrawMenuBar(hWnd);
}

#ifdef USE_CART
void open_cart(HWND hWnd)
{
	_TCHAR* path = get_open_file_name(
		hWnd,
#ifdef _X1TWIN
		_T("Supported Files (*.pce)\0*.pce\0All Files (*.*)\0*.*\0\0"),
		_T("HuCARD"),
#else
		_T("Supported Files (*.rom;*.bin)\0*.rom;*.bin\0All Files (*.*)\0*.*\0\0"), 
		_T("Game Cartridge"),
#endif
		config.initial_cart_path
	);
	if(path) {
		UPDATE_HISTORY(path, config.recent_cart_path);
		emu->open_cart(path);
	}
}
#endif

#ifdef USE_FD1
void open_disk(HWND hWnd, int drv)
{
	_TCHAR* path = get_open_file_name(
		hWnd,
		_T("Supported Files (*.d88;*.td0;*.imd;*.dsk;*.fdi;*.tfd;*.2d;*.sf7)\0*.d88;*.td0;*.imd;*.dsk;*.fdi;*.tfd;*.2d;*.sf7\0All Files (*.*)\0*.*\0\0"),
		_T("Floppy Disk"),
		config.initial_disk_path
	);
	if(path) {
		UPDATE_HISTORY(path, config.recent_disk_path[drv]);
		emu->open_disk(path, drv);
	}
}
#endif

#ifdef USE_QUICKDISK
void open_quickdisk(HWND hWnd)
{
	_TCHAR* path = get_open_file_name(
		hWnd,
		_T("Supported Files (*.mzt)\0*.mzt\0All Files (*.*)\0*.*\0\0"),
		_T("Quick Disk"),
		config.initial_quickdisk_path
	);
	if(path) {
		UPDATE_HISTORY(path, config.recent_quickdisk_path);
		emu->open_quickdisk(path);
	}
}
#endif

#ifdef USE_DATAREC
void open_datarec(HWND hWnd, BOOL play)
{
	_TCHAR* path = get_open_file_name(
		hWnd,
#if defined(DATAREC_BINARY_ONLY)
		_T("Supported Files (*.cas)\0*.cas\0All Files (*.*)\0*.*\0\0"),
#elif defined(DATAREC_TAP)
		play ? _T("Supported Files (*.wav;*.cas;*.tap)\0*.wav;*.cas;*.tap\0All Files (*.*)\0*.*\0\0") : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
#elif defined(DATAREC_MZT)
		play ? _T("Supported Files (*.wav;*.cas;*.mzt;*.m12)\0*.wav;*.cas;*.mzt;*.m12\0All Files (*.*)\0*.*\0\0") : _T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
#else
		_T("Supported Files (*.wav;*.cas)\0*.wav;*.cas\0All Files (*.*)\0*.*\0\0"),
#endif
		play ? _T("Data Recorder Tape [Play]") : _T("Data Recorder Tape [Rec]"),
		config.initial_datarec_path
	);
	if(path) {
		UPDATE_HISTORY(path, config.recent_datarec_path);
		if(play) {
			emu->play_datarec(path);
		}
		else {
			emu->rec_datarec(path);
		}
	}
}
#endif

#ifdef USE_MEDIA
void open_media(HWND hWnd)
{
	_TCHAR* path = get_open_file_name(
		hWnd,
		_T("Supported Files (*.m3u)\0*.m3u\0All Files (*.*)\0*.*\0\0"),
		_T("Sound Cassette Tape"),
		config.initial_media_path
	);
	if(path) {
		UPDATE_HISTORY(path, config.recent_media_path);
		emu->open_media(path);
	}
}
#endif

#ifdef USE_RAM
void open_ram(HWND hWnd, BOOL load)
{
	_TCHAR* path = get_open_file_name(
		hWnd,
		_T("Supported Files (*.ram)\0*.ram\0All Files (*.*)\0*.*\0\0"),
		_T("Memory Dump"),
		config.initial_ram_path
	);
	if(path) {
		UPDATE_HISTORY(path, config.recent_ram_path);
		if(load) {
			emu->load_ram(path);
		}
		else {
			emu->save_ram(path);
		}
	}
}
#endif

void set_window(HWND hWnd, int mode)
{
	static LONG style = WS_VISIBLE;
	WINDOWPLACEMENT place;
	place.length = sizeof(WINDOWPLACEMENT);
	
	if(mode >= 0 && mode < 8) {
		// window
		int width = emu->get_window_width(mode);
		int height = emu->get_window_height(mode);
		RECT rect = {0, 0, width, height};
		AdjustWindowRectEx(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_VISIBLE, TRUE, 0);
		int dest_x = (int)((desktop_width - (rect.right - rect.left)) / 2);
		int dest_y = (int)((desktop_height - (rect.bottom - rect.top)) / 2);
		//dest_x = (dest_x < 0) ? 0 : dest_x;
		dest_y = (dest_y < 0) ? 0 : dest_y;
		
		if(now_fullscreen) {
			ChangeDisplaySettings(NULL, 0);
			SetWindowLong(hWnd, GWL_STYLE, style);
			SetWindowPos(hWnd, HWND_TOP, dest_x, dest_y, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW);
			now_fullscreen = FALSE;
			
			// show menu
			show_menu_bar(hWnd);
		}
		else {
			SetWindowPos(hWnd, NULL, dest_x, dest_y, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
		}
		config.window_mode = prev_window_mode = mode;
		
		// set screen size to emu class
		emu->set_display_size(width, height, TRUE);
	}
	else if(!now_fullscreen) {
		// fullscreen
		int width = (mode == -1) ? desktop_width : screen_mode_width[mode - 8];
		int height = (mode == -1) ? desktop_height : screen_mode_height[mode - 8];
		
		DEVMODE dev;
		ZeroMemory(&dev, sizeof(dev));
		dev.dmSize = sizeof(dev);
		dev.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		dev.dmBitsPerPel = desktop_bpp;
		dev.dmPelsWidth = width;
		dev.dmPelsHeight = height;
		
		if(ChangeDisplaySettings(&dev, CDS_TEST) == DISP_CHANGE_SUCCESSFUL) {
			GetWindowPlacement(hWnd, &place);
			ChangeDisplaySettings(&dev, CDS_FULLSCREEN);
			style = GetWindowLong(hWnd, GWL_STYLE);
			SetWindowLong(hWnd, GWL_STYLE, WS_VISIBLE);
			SetWindowPos(hWnd, HWND_TOP, 0, 0, width, height, SWP_SHOWWINDOW);
			SetCursorPos(width / 2, height / 2);
			now_fullscreen = TRUE;
			if(mode == -1) {
				for(int i = 0; i < screen_mode_count; i++) {
					if(screen_mode_width[i] == desktop_width && screen_mode_height[i] == desktop_height) {
						mode = i + 8;
						break;
					}
				}
			}
			config.window_mode = mode;
			
			// remove menu
			hide_menu_bar(hWnd);
			
			// set screen size to emu class
			emu->set_display_size(width, height, FALSE);
		}
	}
}

