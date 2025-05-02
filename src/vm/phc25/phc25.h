/*
	SANYO PHC-25 Emulator 'ePHC-25'
	SEIKO MAP-1010 Emulator 'eMAP-1010'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.03-

	[ virtual machine ]
*/

#ifndef _PHC25_H_
#define _PHC25_H_

#ifdef _MAP1010
#define DEVICE_NAME		"SEIKO MAP-1010"
#define CONFIG_NAME		"map1010"
#else
#define DEVICE_NAME		"SANYO PHC-25"
#define CONFIG_NAME		"phc25"
#endif
#define CONFIG_VERSION		0x01

// device informations for virtual machine
#define FRAMES_PER_10SECS	600
#define FRAMES_PER_SEC		60
#define LINES_PER_FRAME		262
#define CHARS_PER_LINE		1
#define CPU_CLOCKS		4000000
#define SCREEN_WIDTH		256
#define SCREEN_HEIGHT		192

#define MC6847_ATTR_OFS		0x800
#define MC6847_ATTR_INV		0x01
#define MC6847_ATTR_AS		0x02
#define MC6847_ATTR_CSS		0x04
#define HAS_AY_3_8912

// device informations for win32
#define USE_DATAREC
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		6
#define USE_AUTO_KEY_RELEASE	10
#define USE_AUTO_KEY_CAPS

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class DATAREC;
class IO;
class MC6847;
class NOT;
class YM2203;
class Z80;

class JOYSTICK;
class KEYBOARD;
class MEMORY;
class SYSTEM;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	DATAREC* drec;
	IO* io;
	MC6847* vdp;
	NOT* not;
	YM2203* psg;
	Z80* cpu;
	
	JOYSTICK* joystick;
	KEYBOARD* keyboard;
	MEMORY* memory;
	SYSTEM* system;
	
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
	void register_event(DEVICE* device, int event_id, int usec, bool loop, int* register_id);
	void register_event_by_clock(DEVICE* device, int event_id, int clock, bool loop, int* register_id);
	void cancel_event(int register_id);
	void register_frame_event(DEVICE* dev);
	void register_vline_event(DEVICE* dev);
	
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
