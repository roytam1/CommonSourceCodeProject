/*
	BANDAI RX-78 Emulator 'eRX-78'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.21 -

	[ virtual machine ]
*/

#ifndef _RX78_H_
#define _RX78_H_

// device informations for win32
#define DEVICE_NAME		"BANDAI RX-78"
#define CONFIG_NAME		"rx78"
#define CONFIG_VERSION		0x01

#define WINDOW_WIDTH1		288
#define WINDOW_HEIGHT1		184
#define WINDOW_WIDTH2		576
#define WINDOW_HEIGHT2		368

#define USE_CART
#define USE_DATAREC
#define USE_KEY_TO_JOY
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		6
#define USE_AUTO_KEY_RELEASE	10
#define USE_AUTO_KEY_CAPS
#define USE_SCREEN_X2

// device informations for virtual machine
#define FRAMES_PER_10SECS	600
#define FRAMES_PER_SEC		60
#define LINES_PER_FRAME		262
#define CHARS_PER_LINE		1
#define CPU_CLOCKS		4090909
#define SCREEN_WIDTH		192
#define SCREEN_WIDTH_ASPECT	288
#define SCREEN_HEIGHT		184

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class DATAREC;
class IO8;
class SN76489AN;
class Z80;

class CMT;
class KEYBOARD;
class MEMORY;
class PRINTER;
class VDP;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	DATAREC* drec;
	IO8* io;
	SN76489AN* psg;
	Z80* cpu;
	
	CMT* cmt;
	KEYBOARD* key;
	MEMORY* memory;
	PRINTER* prt;
	VDP* vdp;
	
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
	uint16* create_sound(int samples, bool fill);
	
	// user interface
	void open_cart(_TCHAR* filename);
	void close_cart();
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
	void regist_vsync_event(DEVICE* dev);
	void regist_hsync_event(DEVICE* dev);
	
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
