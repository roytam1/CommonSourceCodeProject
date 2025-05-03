/*
	NEC PC-8201 Emulator 'ePC-8201'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.31-

	[ virtual machine ]
*/

#ifndef _PC8201_H_
#define _PC8201_H_

// device informations for win32
#define DEVICE_NAME		"NEC PC-8201"
#define CONFIG_NAME		"pc8201"
#define CONFIG_VERSION		0x01

#define WINDOW_WIDTH1		240
#define WINDOW_HEIGHT1		64
#define WINDOW_WIDTH2		480
#define WINDOW_HEIGHT2		128

#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6
#define USE_AUTO_KEY_CAPS
#define USE_SCREEN_X2

// device informations for virtual machine
#define FRAMES_PER_10SECS	600
#define FRAMES_PER_SEC		60
#define LINES_PER_FRAME		64
#define CHARS_PER_LINE		1
#define CPU_CLOCKS		2400000
#define SCREEN_WIDTH		240
#define SCREEN_HEIGHT		46
#define USE_PCM1BIT
#define PCM1BIT_HIGH_QUALITY
#define HAS_I8085

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class I8080;
class I8155;
class IO;
class PCM1BIT;
class UPD1990A;

class MEMORY;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	I8080* cpu;
	I8155* pio;
	IO* io;
	PCM1BIT* buzzer;
	UPD1990A* rtc;
	
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
