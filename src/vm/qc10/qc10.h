/*
	EPSON QC-10 Emulator 'eQC-10'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.02.13 -

	[ virtual machine ]
*/

#ifndef _QC10_H_
#define _QC10_H_

// device informations for win32
#ifdef _COLOR_MONITOR
#define DEVICE_NAME		"EPSON QC-10 with color monitor subboard"
#define CONFIG_NAME		"qc10cms"
#else
#define DEVICE_NAME		"EPSON QC-10"
#define CONFIG_NAME		"qc10"
#endif
#define CONFIG_VERSION		0x02

#define WINDOW_WIDTH1		640
#define WINDOW_HEIGHT1		400
#define WINDOW_WIDTH2		640
#define WINDOW_HEIGHT2		400

#define USE_DIPSWITCH
#define DIPSWITCH_DEFAULT	0xe0
#define USE_FD1
#define USE_FD2
#define NOTIFY_KEY_DOWN
#define USE_ALT_F10_KEY

// device informations for virtual machine
#ifdef _COLOR_MONITOR
#define FRAMES_PER_10SECS	569
#define FRAMES_PER_SEC		56.9
#else
#define FRAMES_PER_10SECS	458
#define FRAMES_PER_SEC		45.8
#endif
#define LINES_PER_FRAME 	421
#define CHARS_PER_LINE		108
#define CPU_CLOCKS		3993600
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#define MAX_DRIVE		4
#define I8259_MAX_CHIPS		2
#define UPD7201
#define UPD7220_FIXED_PITCH
#define UPD765A_DMA_MODE
//#define UPD765A_WAIT_SEEK
//#define UPD765A_STRICT_ID

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class BEEP;
class HD146818P;
class I8237;
class I8253;
class I8255;
class I8259;
class IO;
class UPD7220;
class UPD765A;
class Z80;
class Z80SIO;

class DISPLAY;
class FLOPPY;
class KEYBOARD;
class MEMORY;
class MFONT;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	BEEP* beep;
	HD146818P* rtc;
	I8237* dma0;
	I8237* dma1;
	I8253* pit0;
	I8253* pit1;
	I8255* pio;
	I8259* pic;	// includes 2chips
	IO* io;
	UPD7220* gdc;
	UPD765A* fdc;
	Z80* cpu;
	Z80SIO* sio;
	
	DISPLAY* display;
	FLOPPY* floppy;
	KEYBOARD* keyboard;
	MEMORY* memory;
	MFONT* mfont;
	
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
	uint32 get_prv_pc();
	
	// devices
	DEVICE* get_device(int id);
	DEVICE* dummy;
	DEVICE* first_device;
	DEVICE* last_device;
};

#endif
