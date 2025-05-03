/*
	EPSON QC-10 Emulator 'eQC-10'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.12 -

	[ virtual machine ]
*/

#ifndef _MULTI8_H_
#define _MULTI8_H_

// device informations for win32
#define DEVICE_NAME		"EPSON QC-10"
#define CONFIG_NAME		"qc10"

#define WINDOW_WIDTH1		640
#define WINDOW_HEIGHT1		400
#define WINDOW_WIDTH2		640
#define WINDOW_HEIGHT2		400

#define USE_FD1
#define USE_FD2
#define USE_FD3
#define USE_FD4
#define USE_ALT_F10_KEY
#define USE_SCANLINE

// device informations for virtual machine
#define FRAMES_PER_10SECS	458
#define FRAMES_PER_SEC		45.8
#define LINES_PER_FRAME 	421
#define CHARS_PER_LINE		108
#define CPU_CLOCKS		3993600
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#define I8259_MAX_CHIPS		2
#define MAX_DRIVE		4
#define UPD765A_DMA_MODE
//#define UPD765A_WAIT_SEEK

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class BEEP;
class HD146818P:
class I8237;
class I8253;
class I8255;
class I8259;
class IO8;
class NOT;
class UPD7201;
class UPD7220;
class UPD765A;
class Z80;

class MEMORY;
class TIMER;

class VM
{
	// define friend
	friend IO8;
protected:
	EMU* emu;
	
	// devices
	DEVICE* dummy;
	EVENT* event;
	
	BEEP* beep;
	HD146818P* rtc;
	I8237* dma0;
	I8237* dma1;
	I8253* pit0;
	I8253* pit1;
	I8255* pio;
	I8259* pic;	// includes 2chips
	IO8* io;
	NOT* not;
	UPD7201* sio;
	UPD7220* crtc;
	UPD765A* fdc;
	Z80* cpu;
	
	MEMORY* memory;
	TIMER* timer;
	
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
	DEVICE* first_device;
	DEVICE* last_device;
};

#endif
