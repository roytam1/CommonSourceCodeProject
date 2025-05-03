/*
	FUJITSU FM-16pi Emulator 'eFM-16pi'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.10.10 -

	[ virtual machine ]
*/

#ifndef _FM16PI_H_
#define _FM16PI_H_

// device informations for win32
#define DEVICE_NAME		"FUJITSU FM-16pi"
#define CONFIG_NAME		"fm16pi"
#define CONFIG_VERSION		0x01

#define WINDOW_WIDTH1		640
#define WINDOW_HEIGHT1		200
#define WINDOW_WIDTH2		640
#define WINDOW_HEIGHT2		200

#define USE_FD1
#define NOTIFY_KEY_DOWN
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6

// device informations for virtual machine
#define FRAMES_PER_10SECS	554
#define FRAMES_PER_SEC		55.4
#define LINES_PER_FRAME 	262
#define CHARS_PER_LINE		108
#define CPU_CLOCKS		4915200
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		200
#define MAX_DRIVE		4
#define HAS_I86
#define I8259_MAX_CHIPS		1
#define IO_ADDR_MAX		0x10000

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class BEEP;
class I8253;
class I8259;
class I86;
class IO;
class LS393;
class MB8877;
class RTC58321;

class DISPLAY;
class KEYBOARD;
class MEMORY;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	BEEP* beep;
	I8253* pit;
	I8259* pic;
	I86* cpu;
	IO* io;
	LS393* ls74;	// 74LS74
	MB8877* fdc;
	RTC58321* rtc;
	
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
	uint16* create_sound(int samples, bool fill);
	
	// notify key
	void key_down(int code);
	void key_up(int code);
	
	// user interface
	void open_disk(_TCHAR* filename, int drv);
	void close_disk(int drv);
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
