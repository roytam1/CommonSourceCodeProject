/*
	SORD m5 Emulator 'Emu5'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ virtual machine ]
*/

#ifndef _M5_H_
#define _M5_H_

// device informations for win32
#define DEVICE_NAME		"SORD m5"
#define CONFIG_NAME		"m5"

#define WINDOW_WIDTH1		256
#define WINDOW_HEIGHT1		192
#define WINDOW_WIDTH2		512
#define WINDOW_HEIGHT2		384

#define USE_CART
#define USE_DATAREC
#define USE_JOYKEY
#define USE_ALT_F10_KEY
#define USE_SCREEN_X2

// device informations for virtual machine
#define FRAMES_PER_10SECS	600
#define FRAMES_PER_SEC		60
#define LINES_PER_FRAME		262
#define CHARS_PER_LINE		1
#define CPU_CLOCKS		3800000
#define SCREEN_WIDTH		256
#define SCREEN_HEIGHT		192
#define TMS9918A_VRAM_SIZE	0x4000

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class DATAREC;
class IO8;
class SN76489AN;
class TMS9918A;
class Z80;
class Z80CTC;
class Z80PIC;

class CMT;
class KEYBOARD;
class MEMORY;

class VM
{
	// define friend
	friend IO8;
protected:
	EMU* emu;
	
	// devices
	DEVICE* dummy;
	EVENT* event;
	
	DATAREC* drec;
	IO8* io;
	SN76489AN* psg;
	TMS9918A* vdp;
	Z80* cpu;
	Z80CTC* ctc;
	Z80PIC* pic;
	
	CMT* cmt;
	KEYBOARD* key;
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
	
	// devices
	DEVICE* get_device(int id);
	DEVICE* first_device;
	DEVICE* last_device;
};

#endif
