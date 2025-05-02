/*
	SHARP MZ-80K Emulator 'EmuZ-80K'
	SHARP MZ-1200 Emulator 'EmuZ-1200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.18-

	[ virtual machine ]
*/

#ifndef _MZ80K_H_
#define _MZ80K_H_

#ifdef _MZ1200
#define DEVICE_NAME		"SHARP MZ-1200"
#define CONFIG_NAME		"mz1200"
#else
#define DEVICE_NAME		"SHARP MZ-80K"
#define CONFIG_NAME		"mz80k"
#endif
#define CONFIG_VERSION		0x02

// device informations for virtual machine
#define FRAMES_PER_SEC		60
#define LINES_PER_FRAME		262
#define CPU_CLOCKS		2000000
#define SCREEN_WIDTH		320
#define SCREEN_HEIGHT		200
#define PCM1BIT_HIGH_QUALITY
//#define LOW_PASS_FILTER

// device informations for win32
#define USE_DATAREC
#define USE_DATAREC_BUTTON
#define DATAREC_MZT
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6
#define USE_AUTO_KEY_CAPS

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

#ifdef _MZ1200
class AND;
#endif
class DATAREC;
class I8253;
class I8255;
class LS393;
class PCM1BIT;
class Z80;

class DISPLAY;
class KEYBOARD;
class MEMORY;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
#ifdef _MZ1200
	AND* and;
#endif
	DATAREC* drec;
	I8253* ctc;
	I8255* pio;
	LS393* counter;
	PCM1BIT* pcm;
	Z80* cpu;
	
	DISPLAY* display;
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
	
	// user interface
	void play_datarec(_TCHAR* filename);
	void rec_datarec(_TCHAR* filename);
	void close_datarec();
	void push_play();
	void push_stop();
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
