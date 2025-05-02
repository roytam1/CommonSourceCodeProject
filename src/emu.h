/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 emulation i/f ]
*/

#ifndef _EMU_H_
#define _EMU_H_

#define DIRECTSOUND_VERSION 0x300
#define DIRECTDRAW_VERSION 0x500

#include <windows.h>
#include <windowsx.h>
#include <winsock.h>
#include <dsound.h>
#include <mmsystem.h>
#include <stdio.h>
#include "common.h"
#include "vm/vm.h"

//#define _DEBUG_LOG_

#ifdef USE_MEDIA
#define MEDIA_MAX 64
#endif
#ifdef USE_SOCKET
#define WM_SOCKET0 (WM_USER + 1)
#define WM_SOCKET1 (WM_USER + 2)
#define WM_SOCKET2 (WM_USER + 3)
#define WM_SOCKET3 (WM_USER + 4)
#define SOCKET_MAX 4
#define SOCKET_BUFFER_MAX 0x100000
#endif

class FILEIO;

class EMU
{
protected:
	VM* vm;
private:
	// ----------------------------------------
	// debug log
	// ----------------------------------------
#ifdef _DEBUG_LOG_
	FILE* debug;
#endif
	void open_debug();
	void close_debug();
	
	// ----------------------------------------
	// window
	// ----------------------------------------
	
	HWND main_window_handle;
	int window_width, window_height;
	
	// ----------------------------------------
	// input
	// ----------------------------------------
	
	void initialize_input();
	void release_input();
	void update_input();
	
	uint8 key_status[256];	// windows key code mapping
	uint8 joy_status[2];	// joystick #1, #2 (b0 = up, b1 = down, b2 = left, b3 = right, b4-b7 = trigger #1-#4
	int mouse_status[3];	// x, y, button (b0 = left, b1 = right)
	bool mouse_enable;
	
#ifndef _WIN32_WCE
	int joy_num;
	uint32 joy_xmin[2], joy_xmax[2];
	uint32 joy_ymin[2], joy_ymax[2];
	JOYCAPS joycaps[2];
#endif
	
	// ----------------------------------------
	// screen
	// ----------------------------------------
	
	void initialize_screen();
	void release_screen();
	
	HDC hdcDIB;
	HBITMAP hBMP;
	LPBYTE lpBuf;
	LPWORD lpBMP;
	LPBITMAPINFO lpDIB;
	
	// ----------------------------------------
	// sound
	// ----------------------------------------
	
	void initialize_sound(int rate, int samples);
	void release_sound();
	void update_sound();
	
	int sound_rate, sound_samples;
	DWORD sound_buffer_size;
	bool sound_ok, now_mute;
	
	// dsound
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
	DWORD rec_size;
	bool now_rec;
	
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
	bool is_tcp[SOCKET_MAX];
	struct sockaddr_in udpaddr[SOCKET_MAX];
	int socket_delay[SOCKET_MAX];
	char recv_buffer[SOCKET_MAX][SOCKET_BUFFER_MAX];
	int recv_r_ptr[SOCKET_MAX], recv_w_ptr[SOCKET_MAX];
#endif
	
public:
	// ----------------------------------------
	// initialize
	// ----------------------------------------
	
	EMU(HWND hwnd);
	~EMU();
	
	void application_path(_TCHAR* path);
	
	// ----------------------------------------
	// for windows
	// ----------------------------------------
	
	// drive virtual machine
	void run();
	void reset();
#ifdef USE_IPL_RESET
	void ipl_reset();
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
#ifdef USE_DATAREC
	void play_datarec(_TCHAR* filename);
	void rec_datarec(_TCHAR* filename);
	void close_datarec();
#endif
#ifdef USE_MEDIA
	void open_media(_TCHAR* filename);
	void close_media();
#endif
	bool now_skip();
	
	void start_rec_sound();
	void stop_rec_sound();
	bool now_rec_sound() { return now_rec; }
	
	void update_config();
	
	// input device
	void key_down(int code);
	void key_up(int code);
	
	void enable_mouse();
	void disenable_mouse();
	void toggle_mouse();
	
	// screen
	void set_screen_size(int width, int height);
	void draw_screen();
	void update_screen(HDC hdc);
	
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
	
	// get input device status
	uint8* key_buffer() { return key_status; }
	uint8* joy_buffer() { return joy_status; }
	int* mouse_buffer() { return mouse_status; }
	
	// get screen buffer
	uint16* screen_buffer(int y) { return &lpBMP[SCREEN_WIDTH * (SCREEN_HEIGHT - y - 1)]; }
	
	// get timer
	void get_timer(int time[]);
	
#ifdef USE_MEDIA
	// play media
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
