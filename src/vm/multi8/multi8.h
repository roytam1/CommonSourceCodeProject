/*
	MITSUBISHI Elec. MULTI8 Emulator 'EmuLTI8'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.15 -

	[ virtual machine ]
*/

#ifndef _MULTI8_H_
#define _MULTI8_H_

// device informations for win32
#define DEVICE_NAME		"MITSUBISHI Elec. MULTI 8"
#define CONFIG_NAME		"multi8"

#define WINDOW_WIDTH1		640
#define WINDOW_HEIGHT1		400
#define WINDOW_WIDTH2		640
#define WINDOW_HEIGHT2		400

#define USE_DATAREC
#define DATAREC_BINARY_ONLY
#define USE_FD1
#define USE_FD2
//#define USE_FD3
//#define USE_FD4
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6
#define USE_AUTO_KEY_CAPS
#define USE_SCANLINE

// device informations for virtual machine
#define FRAMES_PER_10SECS	599
#define FRAMES_PER_SEC		59.9
#define LINES_PER_FRAME 	256
#define CHARS_PER_LINE		112
#define CPU_CLOCKS		3993600
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#define I8259_MAX_CHIPS		1
#define MAX_DRIVE		4
//#define UPD765A_DMA_MODE
//#define UPD765A_WAIT_SEEK

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class HD46505;
class I8251;
class I8253;
class I8255;
class I8259;
class IO8;
class UPD765A;
class YM2203;
class Z80;

class CMT;
class DISPLAY;
class FLOPPY;
class KANJI;
class KEYBOARD;
class MEMORY;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	HD46505* crtc;
	I8251* sio;
	I8253* pit;
	I8255* pio;
	I8259* pic;
	IO8* io;
	UPD765A* fdc;
	YM2203* opn;
	Z80* cpu;
	
	CMT* cmt;
	DISPLAY* display;
	FLOPPY* floppy;
	KANJI* kanji;
	KEYBOARD* key;
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
