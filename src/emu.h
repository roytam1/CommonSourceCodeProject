/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 emulation i/f ]
*/

#ifndef _EMU_H_
#define _EMU_H_

// DirectX
#define DIRECTSOUND_VERSION 0x900
#define DIRECT3D_VERSION 0x900

// for debug
//#define _DEBUG_LOG
#ifdef _DEBUG_LOG
	// output debug log to console
//	#define _DEBUG_CONSOLE
	// output debug log to file
	#define _DEBUG_FILE
	
	// output cpu debug log
//	#define _CPU_DEBUG_LOG
	// output fdc debug log
//	#define _FDC_DEBUG_LOG
	// output i/o debug log
//	#define _IO_DEBUG_LOG
#endif

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <stdio.h>
#include "common.h"
#include "vm/vm.h"

#define WM_RESIZE  (WM_USER + 1)
#define WM_SOCKET0 (WM_USER + 2)
#define WM_SOCKET1 (WM_USER + 3)
#define WM_SOCKET2 (WM_USER + 4)
#define WM_SOCKET3 (WM_USER + 5)

#ifndef SCREEN_WIDTH_ASPECT
#define SCREEN_WIDTH_ASPECT SCREEN_WIDTH
#endif
#ifndef SCREEN_HEIGHT_ASPECT
#define SCREEN_HEIGHT_ASPECT SCREEN_HEIGHT
#endif
#ifndef WINDOW_WIDTH
#define WINDOW_WIDTH SCREEN_WIDTH_ASPECT
#endif
#ifndef WINDOW_HEIGHT
#define WINDOW_HEIGHT SCREEN_HEIGHT_ASPECT
#endif

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#include <d3d9.h>
#include <d3dx9.h>
#include <d3d9types.h>

#include <dsound.h>
#include <vfw.h>

#ifdef USE_SOCKET
#include <winsock.h>
#endif

// check memory leaks
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define malloc(s) _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#ifdef USE_SOCKET
#define SOCKET_MAX 4
#define SOCKET_BUFFER_MAX 0x100000
#endif

class FIFO;
class FILEIO;

class EMU
{
protected:
	VM* vm;
private:
	// ----------------------------------------
	// input
	// ----------------------------------------
	void initialize_input();
	void release_input();
	void update_input();
	
	uint8 keycode_conv[256];
	uint8 key_status[256];	// windows key code mapping
#ifdef USE_SHIFT_NUMPAD_KEY
	uint8 key_converted[256];
	bool key_shift_pressed, key_shift_released;
#endif
	bool lost_focus;
	
	uint32 joy_status[2];	// joystick #1, #2 (b0 = up, b1 = down, b2 = left, b3 = right, b4- = buttons
	int joy_num;
	uint32 joy_mask[2];
	
	int mouse_status[3];	// x, y, button (b0 = left, b1 = right)
	bool mouse_enabled;
	
#ifdef USE_AUTO_KEY
	FIFO* autokey_buffer;
	int autokey_phase, autokey_shift;
#endif
	
	// ----------------------------------------
	// screen
	// ----------------------------------------
	void initialize_screen();
	void release_screen();
	void create_dib_section(HDC hdc, int width, int height, HDC *hdcDib, HBITMAP *hBmp, HBITMAP *hOldBmp, LPBYTE *lpBuf, scrntype **lpBmp, LPBITMAPINFO *lpDib);
	
	HWND main_window_handle;
	HINSTANCE instance_handle;
	
	// screen settings
	int screen_width, screen_height;
	int screen_width_aspect, screen_height_aspect;
	int window_width, window_height;
	int display_width, display_height;
	bool screen_size_changed;
	
	HDC hdcDibSource;
	scrntype* lpBmpSource;
	LPBITMAPINFO lpDibSource;
	LPBITMAPINFOHEADER pbmInfoHeader;
	
	int source_width, source_height;
	int source_width_aspect, source_height_aspect;
	int stretched_width, stretched_height;
	int stretch_pow_x, stretch_pow_y;
	int screen_dest_x, screen_dest_y;
	bool stretch_screen;
	
	// update flags
	bool first_draw_screen;
	bool first_invalidate;
	bool self_invalidate;
	
	// screen buffer
	HDC hdcDib;
	HBITMAP hBmp, hOldBmp;
	LPBYTE lpBuf;
	scrntype* lpBmp;
	LPBITMAPINFO lpDib;
	
#ifdef USE_SCREEN_ROTATE
	// rotate buffer
	HDC hdcDibRotate;
	HBITMAP hBmpRotate, hOldBmpRotate;
	LPBYTE lpBufRotate;
	scrntype* lpBmpRotate;
	LPBITMAPINFO lpDibRotate;
#endif
	
	// stretch buffer
	HDC hdcDibStretch1;
	HBITMAP hBmpStretch1, hOldBmpStretch1;
	LPBYTE lpBufStretch1;
	scrntype* lpBmpStretch1;
	LPBITMAPINFO lpDibStretch1;
	
	HDC hdcDibStretch2;
	HBITMAP hBmpStretch2, hOldBmpStretch2;
	LPBYTE lpBufStretch2;
	scrntype* lpBmpStretch2;
	LPBITMAPINFO lpDibStretch2;
	
	// for direct3d9
	LPDIRECT3D9 lpd3d9;
	LPDIRECT3DDEVICE9 lpd3d9Device;
	LPDIRECT3DSURFACE9 lpd3d9Surface;
	LPDIRECT3DSURFACE9 lpd3d9OffscreenSurface;
	scrntype *lpd3d9Buffer;
	bool render_to_d3d9Buffer;
	bool use_d3d9;
	bool wait_vsync;
	
	// record video
	bool now_rec_vid;
	int rec_frames, rec_fps;
	PAVIFILE pAVIFile;
	PAVISTREAM pAVIStream;
	PAVISTREAM pAVICompressed;
	AVICOMPRESSOPTIONS opts;
	
	// ----------------------------------------
	// sound
	// ----------------------------------------
	void initialize_sound();
	void release_sound();
	void update_sound(int* extra_frames);
	
	int sound_rate, sound_samples;
	bool sound_ok, sound_started, now_mute;
	
	// direct sound
	LPDIRECTSOUND lpds;
	LPDIRECTSOUNDBUFFER lpdsb, lpdsp;
	bool first_half;
	
	// record sound
	typedef struct {
		DWORD dwRIFF;
		DWORD dwFileSize;
		DWORD dwWAVE;
		DWORD dwfmt_;
		DWORD dwFormatSize;
		WORD wFormatTag;
		WORD wChannels;
		DWORD dwSamplesPerSec;
		DWORD dwAvgBytesPerSec;
		WORD wBlockAlign;
		WORD wBitsPerSample;
		DWORD dwdata;
		DWORD dwDataLength;
	} wavheader_t;
	FILEIO* rec;
	int rec_bytes;
	bool now_rec_snd;
	
	// ----------------------------------------
	// floppy disk
	// ----------------------------------------
#ifdef USE_FD1
	void initialize_disk_insert();
	void update_disk_insert();
	
	typedef struct {
		_TCHAR path[_MAX_PATH];
		int offset;
		int wait_count;
	} disk_insert_t;
	disk_insert_t disk_insert[8];
#endif
	
	// ----------------------------------------
	// socket
	// ----------------------------------------
#ifdef USE_SOCKET
	void initialize_socket();
	void release_socket();
	void update_socket();
	
	int soc[SOCKET_MAX];
	bool is_tcp[SOCKET_MAX];
	struct sockaddr_in udpaddr[SOCKET_MAX];
	int socket_delay[SOCKET_MAX];
	char recv_buffer[SOCKET_MAX][SOCKET_BUFFER_MAX];
	int recv_r_ptr[SOCKET_MAX], recv_w_ptr[SOCKET_MAX];
#endif
	
	// ----------------------------------------
	// misc
	// ----------------------------------------
	void open_debug();
	void close_debug();
#ifdef _DEBUG_CONSOLE
	HANDLE hConsole;
#endif
#ifdef _DEBUG_FILE
	FILE* debug;
#endif
#ifdef USE_CPU_CLOCK_LOW
	bool cpu_clock_low;
#endif
	_TCHAR app_path[_MAX_PATH];
	
public:
	// ----------------------------------------
	// initialize
	// ----------------------------------------
	EMU(HWND hwnd, HINSTANCE hinst);
	~EMU();
	
	_TCHAR* application_path() {
		return app_path;
	}
	_TCHAR* bios_path(_TCHAR* file_name);
	
	// ----------------------------------------
	// for windows
	// ----------------------------------------
	
	// drive virtual machine
	int frame_interval();
	int run();
	void reset();
#ifdef USE_SPECIAL_RESET
	void special_reset();
#endif
#ifdef USE_POWER_OFF
	void notify_power_off();
#endif
	
	// user interface
#ifdef USE_CART
	void open_cart(_TCHAR* file_path);
	void close_cart();
#endif
#ifdef USE_FD1
	void open_disk(int drv, _TCHAR* file_path, int offset);
	void close_disk(int drv);
#endif
#ifdef USE_QUICKDISK
	void open_quickdisk(_TCHAR* file_path);
	void close_quickdisk();
#endif
#ifdef USE_DATAREC
	void play_datarec(_TCHAR* file_path);
	void rec_datarec(_TCHAR* file_path);
	void close_datarec();
#endif
#ifdef USE_DATAREC_BUTTON
	void push_play();
	void push_stop();
#endif
#ifdef USE_BINARY_FILE1
	void load_binary(int drv, _TCHAR* file_path);
	void save_binary(int drv, _TCHAR* file_path);
#endif
	bool now_skip();
	
	void start_rec_sound();
	void stop_rec_sound();
	void restart_rec_sound();
	bool now_rec_sound() {
		return now_rec_snd;
	}
	
	void capture_screen();
	void start_rec_video(int fps, bool show_dialog);
	void stop_rec_video();
	void restart_rec_video();
	bool now_rec_video() {
		return now_rec_vid;
	}
	
	void update_config();
	
	// input device
	void key_down(int code, bool repeat);
	void key_up(int code);
	void key_lost_focus() {
		lost_focus = true;
	}
#ifdef USE_BUTTON
	void press_button(int num);
#endif
	
	void enable_mouse();
	void disenable_mouse();
	void toggle_mouse();
	bool get_mouse_enabled() {
		return mouse_enabled;
	}
	
#ifdef USE_AUTO_KEY
	void start_auto_key();
	void stop_auto_key();
	bool now_auto_key() {
		return (autokey_phase != 0);
	}
#endif
	
	// screen
	int get_window_width(int mode);
	int get_window_height(int mode);
	void set_display_size(int width, int height, bool window_mode);
	void draw_screen();
	void update_screen(HDC hdc);
#ifdef USE_BITMAP
	void reload_bitmap() {
		first_invalidate = true;
	}
#endif
	
	// sound
	void mute_sound();
	
	// socket
#ifdef USE_SOCKET
	int get_socket(int ch) {
		return soc[ch];
	}
	void socket_connected(int ch);
	void socket_disconnected(int ch);
	void send_data(int ch);
	void recv_data(int ch);
#endif
	
	// ----------------------------------------
	// for virtual machine
	// ----------------------------------------
	
	// power off
	void power_off() {
		PostMessage(main_window_handle, WM_CLOSE, 0, 0L);
	}
	
	// input device
	uint8* key_buffer() {
		return key_status;
	}
	uint32* joy_buffer() {
		return joy_status;
	}
	int* mouse_buffer() {
		return mouse_status;
	}
	
	// screen
	void change_screen_size(int sw, int sh, int swa, int sha, int ww, int wh);
	scrntype* screen_buffer(int y);
	
	// timer
	void get_host_time(cur_time_t* time);
	
	// socket
#ifdef USE_SOCKET
	bool init_socket_tcp(int ch);
	bool init_socket_udp(int ch);
	bool connect_socket(int ch, uint32 ipaddr, int port);
	void disconnect_socket(int ch);
	bool listen_socket(int ch);
	void send_data_tcp(int ch);
	void send_data_udp(int ch, uint32 ipaddr, int port);
#endif
	// debug log
	void out_debug(const _TCHAR* format, ...);
};

#endif
