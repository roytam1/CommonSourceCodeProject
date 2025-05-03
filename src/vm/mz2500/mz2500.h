/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.24 -

	[ virtual machine ]
*/

#ifndef _MZ2500_H_
#define _MZ2500_H_

// device informations for win32
#define DEVICE_NAME		"SHARP MZ-2500"
#define CONFIG_NAME		"mz2500"

#define WINDOW_WIDTH1		640
#define WINDOW_HEIGHT1		400
#define WINDOW_WIDTH2		640
#define WINDOW_HEIGHT2		400

#define USE_IPL_RESET
#define USE_FD1
#define USE_FD2
#define USE_FD3
#define USE_FD4
#define USE_MEDIA
#define USE_SOCKET
#define USE_CAPTURE
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6
#define USE_SCANLINE
#define USE_MONITOR_TYPE

// device informations for virtual machine
#define FRAMES_PER_10SECS	554
#define FRAMES_PER_SEC		55.4
#define LINES_PER_FRAME 	440
#define CHARS_PER_LINE		108
#define CPU_CLOCKS		6000000
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#define MAX_DRIVE		4
#define MB8876
#define TIMER_FREQ

// memory wait
#define Z80_M1_CYCLE_WAIT	1
#define CPU_MEMORY_WAIT
#define CPU_IO_WAIT
#define VRAM_WAIT

// irq priority
#define IRQ_Z80PIO	0
#define IRQ_Z80SIO	1
#define IRQ_CRTC	2
#define IRQ_I8253	3
#define IRQ_PRINTER	4
#define IRQ_RP5C15	5

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class I8253;
class I8255;
class IO8;
class MB8877;
class PCM1BIT;
class RP5C15;
class W3100A;
class YM2203;
class Z80;
class Z80PIC;
class Z80PIO;
class Z80SIO;

class CALENDAR;
class CASSETTE;
class CRTC;
class EMM;
class EXTROM;
class FLOPPY;
class INTERRUPT;
class JOYSTICK;
class KANJI;
class KEYBOARD;
class MEMORY;
class MOUSE;
class RESET;
class ROMFILE;
class SASI;
class TIMER;
class VOICE;

class VM
{
	// define friend
	friend IO8;
protected:
	EMU* emu;
	
	// devices
	DEVICE* dummy;
	EVENT* event;
	
	I8253* pit;
	I8255* pio0;
	IO8* io;
	MB8877* fdc;
	PCM1BIT* pcm;
	RP5C15* rtc;
	W3100A* w3100a;
	YM2203* opn;
	Z80* cpu;
	Z80PIC* pic;
	Z80PIO* pio1;
	Z80SIO* sio;
	
	CALENDAR* calendar;
	CASSETTE* cassette;
	CRTC* crtc;
	EMM* emm;
	EXTROM* extrom;
	FLOPPY* floppy;
	INTERRUPT* interrupt;
	JOYSTICK* joystick;
	KANJI* kanji;
	KEYBOARD* keyboard;
	MEMORY* memory;
	MOUSE* mouse;
	RESET* rst;
	ROMFILE* romfile;
	SASI* sasi;
	TIMER* timer;
	VOICE* voice;
	
public:
	// ----------------------------------------
	// initialize
	// ----------------------------------------
	
	VM(EMU* parent_emu);
	~VM();
	
	// ----------------------------------------
	// for emulation class
	// ----------------------------------------
	
	// drive virtual machine
	void reset();
	void ipl_reset();
	void run();
	
	// draw screen
	void draw_screen();
	
	// sound generation
	void initialize_sound(int rate, int samples);
	uint16* create_sound(int samples, bool fill);
	
	// socket
	void network_connected(int ch);
	void network_disconnected(int ch);
	uint8* get_sendbuffer(int ch, int* size);
	void inc_sendbuffer_ptr(int ch, int size);
	uint8* get_recvbuffer0(int ch, int* size0, int* size1);
	uint8* get_recvbuffer1(int ch);
	void inc_recvbuffer_ptr(int ch, int size);
	
	// user interface
	void open_disk(_TCHAR* filename, int drv);
	void close_disk(int drv);
	bool now_skip();
	
	void update_config();
	
	// ----------------------------------------
	// for each device
	// ----------------------------------------
	
	// event callbacks
	void regist_event(DEVICE* device, int event_id, int usec, bool loop, int* regist_id);
	void regist_event_by_clock(DEVICE* device, int event_id, int clock, bool loop, int* regist_id);
	void cancel_event(int regist_id);
	void regist_frame_event(DEVICE* dev);
	void regist_vsync_event(DEVICE* dev);
	void regist_hsync_event(DEVICE* dev);
	
	// clock
	uint32 current_clock();
	uint32 passed_clock(uint32 prev);
	
	// devices
	DEVICE* get_device(int id);
	DEVICE* first_device;
	DEVICE* last_device;
};

#endif
