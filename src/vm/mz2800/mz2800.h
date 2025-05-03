/*
	SHARP MZ-2800 Emulator 'EmuZ-2800'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.08.13 -

	[ virtual machine ]
*/

#ifndef _MZ2800_H_
#define _MZ2800_H_

// device informations for win32
#define DEVICE_NAME		"SHARP MZ-2800"
#define CONFIG_NAME		"mz2800"
#define CONFIG_VERSION		0x01

#define WINDOW_WIDTH1		640
#define WINDOW_HEIGHT1		400
#define WINDOW_WIDTH2		640
#define WINDOW_HEIGHT2		400

//#define USE_IPL_RESET
#define USE_FD1
#define USE_FD2
#define USE_FD3
#define USE_FD4
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
#define CPU_CLOCKS		8000000
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#define MAX_DRIVE		4
#define HAS_I286
#define I8259_MAX_CHIPS		2
#define IO_ADDR_MAX		0x8000

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class I8253;
class I8255;
class I8259;
class I86;
class IO;
class MB8877;
class PCM1BIT;
class RP5C15;
//class SASI;
class UPD71071;
class YM2203;
class Z80PIO;
class Z80SIO;

class CALENDAR;
class CRTC;
class FLOPPY;
class JOYSTICK;
class KEYBOARD;
class MEMORY;
class MOUSE;
class RESET;
class SYSPORT;
class TIMER;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	I8253* pit;
	I8255* pio0;
	I8259* pic;
	I86* cpu;
	IO* io;
	MB8877* fdc;
	PCM1BIT* pcm;
	RP5C15* rtc;
//	SASI* sasi;
	UPD71071* dma;
	YM2203* opn;
	Z80PIO* pio1;
	Z80SIO* sio;
	
	CALENDAR* calendar;
	CRTC* crtc;
	FLOPPY* floppy;
	JOYSTICK* joystick;
	KEYBOARD* keyboard;
	MEMORY* memory;
	MOUSE* mouse;
	RESET* rst;
	SYSPORT* sysport;
	TIMER* timer;
	
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
	void cpu_reset();
	void run();
	
	// draw screen
	void draw_screen();
	
	// sound generation
	void initialize_sound(int rate, int samples);
	uint16* create_sound(int samples, bool fill);
	
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
	void regist_vline_event(DEVICE* dev);
	
	// clock
	uint32 current_clock();
	uint32 passed_clock(uint32 prev);
	uint32 get_prv_pc();
	
	// devices
	DEVICE* get_device(int id);
	DEVICE* dummy;
	DEVICE* first_device;
	DEVICE* last_device;
};

#endif
