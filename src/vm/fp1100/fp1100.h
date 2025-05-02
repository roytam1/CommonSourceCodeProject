/*
	CASIO FP-1100 Emulator 'eFP-1100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.06.18-

	[ virtual machine ]
*/

#ifndef _FP1100_H_
#define _FP1100_H_

#define DEVICE_NAME		"CASIO FP-1100"
#define CONFIG_NAME		"fp1100"
#define CONFIG_VERSION		0x01

// device informations for virtual machine
#define FRAMES_PER_10SECS	554
#define FRAMES_PER_SEC		55.4
#define LINES_PER_FRAME 	440
#define CHARS_PER_LINE		108
#define CPU_CLOCKS		3993600
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#define MAX_DRIVE		4
#define UPD765A_DMA_MODE
#define UPD765A_WAIT_SEEK

// device informations for win32
#define USE_DATAREC
#define DATAREC_BINARY_ONLY
#define USE_FD1
#define USE_FD2
//#define USE_FD3
//#define USE_FD4
#define NOTIFY_KEY_DOWN
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6
#define USE_AUTO_KEY_CAPS
#define USE_SCANLINE

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class BEEP;
class HD46505;
class UPD765A;
class UPD7801;
class Z80;

class MAIN;
class SUB;
class RAMPACK;
class ROMPACK;
class FDCPACK;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	BEEP* beep;
	HD46505* crtc;
	UPD765A* fdc;
	UPD7801* subcpu;
	Z80* cpu;
	
	MAIN* main;
	SUB* sub;
	RAMPACK* rampack1;
	RAMPACK* rampack2;
	RAMPACK* rampack3;
	RAMPACK* rampack4;
	RAMPACK* rampack5;
	RAMPACK* rampack6;
	ROMPACK* rompack;
	FDCPACK* fdcpack;
	
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
	
	// notify key
	void key_down(int code, bool repeat);
	void key_up(int code);
	
	// user interface
	void open_disk(_TCHAR* filename, int drv);
	void close_disk(int drv);
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
	uint32 get_sub_prv_pc();
	
	// devices
	DEVICE* get_device(int id);
	DEVICE* dummy;
	DEVICE* first_device;
	DEVICE* last_device;
};

#endif
