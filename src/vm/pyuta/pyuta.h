/*
	TOMY PyuTa Emulator 'ePyuTa'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.07.15 -

	[ virtual machine ]
*/

#ifndef _PYUTA_H_
#define _PYUTA_H_

// device informations for win32
#define DEVICE_NAME		"TOMY PyuTa"
#define CONFIG_NAME		"pyuta"

#define WINDOW_WIDTH1		256
#define WINDOW_HEIGHT1		192
#define WINDOW_WIDTH2		512
#define WINDOW_HEIGHT2		384

#define USE_CART
#define USE_DATAREC
#define DATAREC_BINARY_ONLY
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6
#define USE_AUTO_KEY_CAPS
#define USE_SCREEN_X2

// device informations for virtual machine
#define FRAMES_PER_10SECS	600
#define FRAMES_PER_SEC		60
#define LINES_PER_FRAME 	262
#define CHARS_PER_LINE		1
#define CPU_CLOCKS		10700000
#define SCREEN_WIDTH		256
#define SCREEN_HEIGHT		192
#define TMS9918A_VRAM_SIZE	0x4000
//#define TMS9918A_LIMIT_SPRITES

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class DATAREC;
class SN76489AN;
class TMS9918A;
class TMS9995;

class MEMORY;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	DATAREC* drec;
	SN76489AN* psg;
	TMS9918A* vdp;
	TMS9995* cpu;
	
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
	
	// devices
	DEVICE* get_device(int id);
	DEVICE* dummy;
	DEVICE* first_device;
	DEVICE* last_device;
};

#endif
