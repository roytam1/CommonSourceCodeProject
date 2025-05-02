/*
	CANON X-07 Emulator 'eX-07'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.12.26 -

	[ virtual machine ]
*/

#ifndef _X07_H_
#define _X07_H_

#define DEVICE_NAME		"CANON X-07"
#define CONFIG_NAME		"x07"
#define CONFIG_VERSION		0x01

// device informations for virtual machine
#define FRAMES_PER_10SECS	600
#define FRAMES_PER_SEC		60
#define LINES_PER_FRAME		262
#define CHARS_PER_LINE		1
#define CPU_CLOCKS		3840000
#define CPU_START_ADDR		0xc3c3
#define SCREEN_WIDTH		120
#define SCREEN_HEIGHT		32
#define TV_SCREEN_WIDTH		256
#define TV_SCREEN_HEIGHT	192
#define NSC800

// device informations for win32
#define WINDOW_WIDTH		(SCREEN_WIDTH * 2)
#define WINDOW_HEIGHT		(SCREEN_HEIGHT * 2)
#define TV_WINDOW_WIDTH		TV_SCREEN_WIDTH
#define TV_WINDOW_HEIGHT	TV_SCREEN_HEIGHT

#define USE_DATAREC
#define DATAREC_BINARY_ONLY
#define NOTIFY_KEY_DOWN
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		6
#define USE_AUTO_KEY_RELEASE	10
#define USE_AUTO_KEY_CAPS

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class BEEP;
class Z80;

class IO;
class MEMORY;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	BEEP* beep;
	Z80* cpu;
	
	IO* io;
	MEMORY* memory;
	
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
	void run();
	
	// draw screen
	void draw_screen();
	
	// sound generation
	void initialize_sound(int rate, int samples);
	uint16* create_sound(int* extra_frames);
	
	// notify key
	void key_down(int code);
	void key_up(int code);
	
	// user interface
	void play_datarec(_TCHAR* filename);
	void rec_datarec(_TCHAR* filename);
	void close_datarec();
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
