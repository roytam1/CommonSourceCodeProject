/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 emulation i/f ]
*/

#ifndef _EMU_H_
#define _EMU_H_

// DirectX
#define DIRECTDRAW_VERSION 0x300
#define DIRECTSOUND_VERSION 0x500
//#define DIRECT3D_VERSION 0x900

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

#if defined(SCREEN_WIDTH_ASPECT) || defined(SCREEN_HEIGHT_ASPECT)
#define USE_SCREEN_STRETCH_HIGH_QUALITY
#endif
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

#include <dsound.h>
#ifdef USE_SOCKET
#include <winsock.h>
#endif
#include <vfw.h>

#ifdef USE_MEDIA
#define MEDIA_MAX 64
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
	
	uint8 key_status[256];	// windows key code mapping
#ifdef USE_SHIFT_NUMPAD_KEY
	uint8 key_converted[256];
	BOOL key_shift_pressed, key_shift_released;
#endif
	uint8 joy_status[2];	// joystick #1, #2 (b0 = up, b1 = down, b2 = left, b3 = right, b4-b7 = trigger #1-#4
	int joy_num;
	uint32 joy_xmin[2], joy_xmax[2];
	uint32 joy_ymin[2], joy_ymax[2];
	
	int mouse_status[3];	// x, y, button (b0 = left, b1 = right)
	BOOL mouse_enabled;
	
#ifdef USE_AUTO_KEY
	FIFO* autokey_buffer;
	int autokey_phase, autokey_shift;
#endif
	
	// ----------------------------------------
	// screen
	// ----------------------------------------
	void initialize_screen();
	void release_screen();
	void create_dib_section(HDC hdc, int width, int height, HDC *hdcDib, HBITMAP *hBmp, LPBYTE *lpBuf, scrntype **lpBmp, LPBITMAPINFO *lpDib);
	void release_dib_section(HDC *hdcDib, HBITMAP *hBmp, LPBYTE *lpBuf);
	
	HWND main_window_handle;
	HINSTANCE instance_handle;
	
	int screen_width, screen_height;
	int screen_width_aspect, screen_height_aspect;
	int window_width, window_height;
	int display_width, display_height;
	int stretch_width, stretch_height;
	BOOL stretch_screen;
#ifdef USE_SCREEN_STRETCH_HIGH_QUALITY
	BOOL stretch_screen_high_quality;
#endif
	int dest_x, dest_y;
	
	// update flags
	int source_buffer;
	BOOL first_draw_screen;
	BOOL first_invalidate;
	BOOL self_invalidate;
	
	// screen buffer
	HDC hdcDib;
	HBITMAP hBmp;
	LPBYTE lpBuf;
	scrntype* lpBmp;
	LPBITMAPINFO lpDib;
	LPBITMAPINFOHEADER pbmInfoHeader;
	
#ifdef USE_SCREEN_ROTATE
	// rotate buffer
	HDC hdcDibRotate;
	HBITMAP hBmpRotate;
	LPBYTE lpBufRotate;
	scrntype* lpBmpRotate;
	LPBITMAPINFO lpDibRotate;
#endif
#ifdef USE_SCREEN_STRETCH_HIGH_QUALITY
	// stretch buffer
	HDC hdcDibStretch1;
	HBITMAP hBmpStretch1;
	LPBYTE lpBufStretch1;
	scrntype* lpBmpStretch1;
	LPBITMAPINFO lpDibStretch1;
	
	HDC hdcDibStretch2;
	HBITMAP hBmpStretch2;
	LPBYTE lpBufStretch2;
	scrntype* lpBmpStretch2;
	LPBITMAPINFO lpDibStretch2;
#endif
	
	// record video
	BOOL now_recv;
	int rec_frames, rec_fps;
	PAVIFILE pAVIFile;
	PAVISTREAM pAVIStream;
	PAVISTREAM pAVICompressed;
	AVICOMPRESSOPTIONS opts;
	
	// ----------------------------------------
	// sound
	// ----------------------------------------
	void initialize_sound(int rate, int samples);
	void release_sound();
	void update_sound(int* extra_frames);
	
	int sound_rate, sound_samples;
	BOOL sound_ok, now_mute;
	
	// direct sound
	LPDIRECTSOUND lpds;
	LPDIRECTSOUNDBUFFER lpdsb, lpdsp;
	BOOL first_half;
	
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
	BOOL now_recs;
	
	// ----------------------------------------
	// media
	// ----------------------------------------
#ifdef USE_MEDIA
	void initialize_media();
	void release_media();
	
	_TCHAR media_path[MEDIA_MAX][_MAX_PATH];
	int media_cnt;
#endif
	
	// ----------------------------------------
	// timer
	// ----------------------------------------
	void update_timer();
	SYSTEMTIME sTime;
	
	// ----------------------------------------
	// socket
	// ----------------------------------------
#ifdef USE_SOCKET
	void initialize_socket();
	void release_socket();
	void update_socket();
	
	int soc[SOCKET_MAX];
	BOOL is_tcp[SOCKET_MAX];
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
	_TCHAR app_path[_MAX_PATH];
	
public:
	// ----------------------------------------
	// initialize
	// ----------------------------------------
	EMU(HWND hwnd, HINSTANCE hinst);
	~EMU();
	
	void application_path(_TCHAR* path);
	
	// ----------------------------------------
	// for windows
	// ----------------------------------------
	
	// drive virtual machine
	int frame_interval();
	void run();
	void reset();
#ifdef USE_SPECIAL_RESET
	void special_reset();
#endif
#ifdef USE_POWER_OFF
	void notify_power_off();
#endif
	
	// user interface
#ifdef USE_CART
	void open_cart(_TCHAR* filename);
	void close_cart();
#endif
#ifdef USE_FD1
	void open_disk(_TCHAR* filename, int drv);
	void close_disk(int drv);
#endif
#ifdef USE_QUICKDISK
	void open_quickdisk(_TCHAR* filename);
	void close_quickdisk();
#endif
#ifdef USE_DATAREC
	void play_datarec(_TCHAR* filename);
	void rec_datarec(_TCHAR* filename);
	void close_datarec();
#endif
#ifdef USE_DATAREC_BUTTON
	void push_play();
	void push_stop();
#endif
#ifdef USE_MEDIA
	void open_media(_TCHAR* filename);
	void close_media();
#endif
#ifdef USE_RAM
	void load_ram(_TCHAR* filename);
	void save_ram(_TCHAR* filename);
#endif
	BOOL now_skip();
	
	void start_rec_sound();
	void stop_rec_sound();
	void restart_rec_sound();
	BOOL now_rec_sound() { return now_recs; }
	
	void capture_screen();
	void start_rec_video(int fps, BOOL show_dialog);
	void stop_rec_video();
	void restart_rec_video();
	BOOL now_rec_video() { return now_recv; }
	
	void update_config();
	
	// input device
	void key_down(int code, bool repeat);
	void key_up(int code);
#ifdef USE_BUTTON
	void press_button(int num);
#endif
	
	void enable_mouse();
	void disenable_mouse();
	void toggle_mouse();
	BOOL get_mouse_enabled() { return mouse_enabled; }
	
#ifdef USE_AUTO_KEY
	void start_auto_key();
	void stop_auto_key();
	BOOL now_auto_key() { return (autokey_phase != 0); }
#endif
	
	// screen
	int get_window_width(int mode);
	int get_window_height(int mode);
	void set_display_size(int width, int height, BOOL window_mode);
	void draw_screen();
	void update_screen(HDC hdc);
#ifdef USE_BITMAP
	void reload_bitmap() { first_invalidate = TRUE; }
#endif
	
	// sound
	void mute_sound();
	
	// socket
#ifdef USE_SOCKET
	int get_socket(int ch) { return soc[ch]; }
	void socket_connected(int ch);
	void socket_disconnected(int ch);
	void send_data(int ch);
	void recv_data(int ch);
#endif
	
	// ----------------------------------------
	// for virtual machine
	// ----------------------------------------
	
	// power off
	void power_off() { PostMessage(main_window_handle, WM_CLOSE, 0, 0L); }
	
	// input device
	uint8* key_buffer() { return key_status; }
	uint8* joy_buffer() { return joy_status; }
	int* mouse_buffer() { return mouse_status; }
	
	// screen
	void change_screen_size(int sw, int sh, int swa, int sha, int ww, int wh);
	scrntype* screen_buffer(int y);
	
	// timer
	void get_timer(int time[]);
	
	// media
#ifdef USE_MEDIA
	int media_count();
	void play_media(int trk);
	void stop_media();
#endif
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
