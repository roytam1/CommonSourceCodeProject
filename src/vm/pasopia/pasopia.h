/*
	TOSHIBA PASOPIA Emulator 'EmuPIA'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.28 -

	[ virtual machine ]
*/

#ifndef _PASOPIA_H_
#define _PASOPIA_H_

#ifdef _LCD
#define DEVICE_NAME		"TOSHIBA PASOPIA with LCD"
#define CONFIG_NAME		"pasopialcd"
#define CONFIG_VERSION		0x01
#else
#define DEVICE_NAME		"TOSHIBA PASOPIA"
#define CONFIG_NAME		"pasopia"
#define CONFIG_VERSION		0x01
#endif

// device informations for virtual machine
#ifdef _LCD
#define FRAMES_PER_SEC		74.38
#define LINES_PER_FRAME 	32
#define CHARS_PER_LINE		94
#define HD46505_HORIZ_FREQ	2380
#else
#define FRAMES_PER_SEC		59.92
#define LINES_PER_FRAME 	262
#define CHARS_PER_LINE		57
#define HD46505_HORIZ_FREQ	15700
#endif
#define CPU_CLOCKS		3993600
#ifdef _LCD
#define SCREEN_WIDTH		320
#define SCREEN_HEIGHT		128
#else
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#endif
#define SUPPORT_VARIABLE_TIMING

// irq priority
#define IRQ_Z80PIO		0
//				1
#define IRQ_Z80CTC		2
//				3-5
#define IRQ_EXTERNAL		6

// device informations for win32
#define USE_DATAREC
#define USE_SHIFT_NUMPAD_KEY
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6
#define USE_SCANLINE

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class DATAREC;
class HD46505;
class I8255;
class IO;
class LS393;
class NOT;
class PCM1BIT;
class Z80;
class Z80CTC;
class Z80PIO;

class DISPLAY;
class KEYBOARD;
class MEMORY;
class PAC2;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	DATAREC* drec;
	HD46505* crtc;
	I8255* pio0;
	I8255* pio1;
	I8255* pio2;
	IO* io;
	LS393* flipflop;
	NOT* not;
	PCM1BIT* pcm;
	Z80* cpu;
	Z80CTC* ctc;
	Z80PIO* pio;
	
	DISPLAY* display;
	KEYBOARD* key;
	MEMORY* memory;
	PAC2* pac2;
	
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
	double frame_rate();
	
	// draw screen
	void draw_screen();
	
	// sound generation
	void initialize_sound(int rate, int samples);
	uint16* create_sound(int* extra_frames);
	
	// user interface
	void play_datarec(_TCHAR* file_path);
	void rec_datarec(_TCHAR* file_path);
	void close_datarec();
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
