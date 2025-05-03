/*
	SHARP MZ-3500 Emulator 'EmuZ-3500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.17 -

	[ virtual machine ]
*/

#ifndef _MZ3500_H_
#define _MZ3500_H_

// device informations for win32
#define DEVICE_NAME		"SHARP MZ-3500"
#define CONFIG_NAME		"mz3500"
#define CONFIG_VERSION		0x01

#define WINDOW_WIDTH1		640
#define WINDOW_HEIGHT1		400
#define WINDOW_WIDTH2		640
#define WINDOW_HEIGHT2		400

#define USE_FD1
#define USE_FD2
#define USE_FD3
#define USE_FD4
#define NOTIFY_KEY_DOWN
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6

// device informations for virtual machine
#define FRAMES_PER_10SECS	473
#define FRAMES_PER_SEC		47.3
#define LINES_PER_FRAME 	440
#define CHARS_PER_LINE		108
#define CPU_CLOCKS		4000000
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#define MAX_DRIVE		4
#define I8259_MAX_CHIPS		2
#define UPD765A_DMA_MODE
#define UPD765A_WAIT_SEEK
#define UPD765A_STRICT_ID
#define IO8_ADDR_MAX		0x100

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class BEEP;
class I8251;
class I8253;
class I8255;
class IO8;
class LS244;
class UPD1990A;
class UPD7220;
class UPD765A;
class Z80;

class DISPLAY;
class KEYBOARD;
class MEMORY;
class MFD;
class SUBMEMORY;

class VM
{
	// define friend
	friend IO8;
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	IO8* io;
	UPD765A* fdc;
	Z80* cpu;
	
	MEMORY* memory;
	MFD* mfd;
	
	BEEP* beep;
	I8251* sio;
	I8253* ctc;
	I8255* pio;
	IO8* subio;
	LS244* ls244;
	UPD1990A* rtc;
	UPD7220* gdc_chr;
	UPD7220* gdc_gfx;
	Z80* subcpu;
	
	DISPLAY* display;
	KEYBOARD* keyboard;
	SUBMEMORY* submemory;
	
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
