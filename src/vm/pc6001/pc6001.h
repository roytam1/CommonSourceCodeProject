/*
	NEC PC-6001 Emulator 'yaPC-6001'
	NEC PC-6001mk2 Emulator 'yaPC-6201'
	NEC PC-6601 Emulator 'yaPC-6601'

	Author : tanam
	Date   : 2013.07.15-

	[ virtual machine ]
*/

#ifndef _PC6001_H_
#define _PC6001_H_

#ifdef _PC6601
#define DEVICE_NAME		"NEC PC-6601"
#define CONFIG_NAME		"pc6601"
#define SCREEN_WIDTH		320
#define SCREEN_HEIGHT		200
#define CPU_CLOCKS			4000000
#endif
#ifdef _PC6001MK2
#define DEVICE_NAME		"NEC PC-6001mk2"
#define CONFIG_NAME		"pc6001mk2"
#define SCREEN_WIDTH		320
#define SCREEN_HEIGHT		200
#define CPU_CLOCKS			4000000
#endif
#ifdef _PC6001
#define DEVICE_NAME		"NEC PC-6001"
#define CONFIG_NAME		"pc6001"
#define SCREEN_WIDTH		256
#define SCREEN_HEIGHT		192
#define CPU_CLOCKS			3993600
#endif

// device informations for virtual machine
#define FRAMES_PER_SEC		60
#define LINES_PER_FRAME		262
#define MAX_DRIVE			4
#define HAS_AY_3_8910
#define MC6847_ATTR_OFS		0
#define MC6847_VRAM_OFS		0x200
#define MC6847_ATTR_AG		0x80
#define MC6847_ATTR_AS		0x40
#define MC6847_ATTR_INTEXT	0x20
#define MC6847_ATTR_GM0		0x10
#define MC6847_ATTR_GM1		0x08
#define MC6847_ATTR_GM2		0x04
#define MC6847_ATTR_CSS		0x02
#define MC6847_ATTR_INV		0x01

// device informations for win32
#define MIN_WINDOW_WIDTH	320
#define USE_CART1
#define USE_FD1
///#define USE_FD2
///#define USE_FD3
///#define USE_FD4
#define USE_ACCESS_LAMP
#define USE_TAPE
#define TAPE_PC6001
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		6
#define USE_AUTO_KEY_RELEASE	10
#define USE_AUTO_KEY_CAPS
#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class I8255;
class IO;
class SYSTEM;
#ifdef _PC6001
class MC6847;
#else
class PD7752;
#endif
class DATAREC;
class YM2203;
class Z80;
class DISPLAY;
class JOYSTICK;
class KEYBOARD;
class MEMORY;

class VM
{
protected:
	EMU* emu;
	int vdata;
	
	// devices
	EVENT* event;
	
	I8255* pio_k;
	I8255* pio_f;
	IO* io;
	DATAREC* drec;
	YM2203* psg;
	Z80* cpu;
	SYSTEM* system;
#ifdef _PC6001
	MC6847* vdp;
#else
	PD7752* voice;
#endif	
	DISPLAY* display;
	JOYSTICK* joystick;
	KEYBOARD* keyboard;
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
	int sound_buffer_ptr();
	
	// user interface
	void open_cart(int drv,_TCHAR* file_path);
	void close_cart(int drv);
	bool cart_inserted(int drv);
	int access_lamp();
	void open_disk(int drv, _TCHAR* file_path, int offset);
	void close_disk(int drv);
	bool disk_inserted(int drv);
	void play_tape(_TCHAR* file_path);
	void rec_tape(_TCHAR* file_path);
	void close_tape();
	bool tape_inserted();
	bool now_skip();
	
	void update_config();
	
	// ----------------------------------------
	// for each device
	// ----------------------------------------
	
	// devices
	DEVICE* get_device(int id);
	DEVICE* dummy;
	DEVICE* first_device;
	DEVICE* last_device;
};
#endif
