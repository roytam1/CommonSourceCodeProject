/*
	CASIO PV-1000 Emulator 'ePV-1000'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.16 -

	[ virtual machine ]
*/

#ifndef _PV1000_H_
#define _PV1000_H_

// device informations for win32
#define DEVICE_NAME		"CASIO PV-1000"
#define CONFIG_NAME		"pv1000"

#define WINDOW_WIDTH1		256
#define WINDOW_HEIGHT1		192
#define WINDOW_WIDTH2		512
#define WINDOW_HEIGHT2		384

#define USE_CART
#define USE_KEY_TO_JOY
#define USE_SCREEN_X2

// device informations for virtual machine
#define FRAMES_PER_10SECS	592
#define FRAMES_PER_SEC		59
#define LINES_PER_FRAME		67
#define CHARS_PER_LINE		1
#define CPU_CLOCKS		3579545
#define SCREEN_WIDTH		256
#define SCREEN_HEIGHT		192

#define LINES_PER_HBLANK 	51
#define CLOCKS_PER_HBLANK	800

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class IO8;
class Z80;

class JOYSTICK;
class MEMORY;
class PSG;
class VDP;

class VM
{
	// define friend
	friend IO8;
protected:
	EMU* emu;
	
	// devices
	DEVICE* dummy;
	EVENT* event;
	
	IO8* io;
	Z80* cpu;
	
	JOYSTICK* joystick;
	MEMORY* memory;
	PSG* psg;
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
