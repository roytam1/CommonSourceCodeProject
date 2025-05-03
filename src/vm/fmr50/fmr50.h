/*
	FUJITSU FMR-50 Emulator 'eFMR-50'
	FUJITSU FMR-60 Emulator 'eFMR-60'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.28 -

	[ virtual machine ]
*/

#ifndef _FMR50_H_
#define _FMR50_H_

// device informations for win32
#ifdef _FMR60
#define DEVICE_NAME		"FUJITSU FMR-60"
#define CONFIG_NAME		"fmr60"
#else
#define DEVICE_NAME		"FUJITSU FMR-50"
#define CONFIG_NAME		"fmr50"
#endif
#define CONFIG_VERSION		0x01

#ifdef _FMR60
#define WINDOW_WIDTH1		1120
#define WINDOW_HEIGHT1		750
#define WINDOW_WIDTH2		1120
#define WINDOW_HEIGHT2		750
#else
#define WINDOW_WIDTH1		640
#define WINDOW_HEIGHT1		400
#define WINDOW_WIDTH2		640
#define WINDOW_HEIGHT2		400
#endif

//#define USE_IPL_RESET
#define USE_FD1
#define USE_FD2
#define USE_FD3
#define USE_FD4
#define NOTIFY_KEY_DOWN
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6

// device informations for virtual machine
#define FRAMES_PER_10SECS	554
#define FRAMES_PER_SEC		55.4
#ifdef _FMR60
#define LINES_PER_FRAME 	812
#define CHARS_PER_LINE		108
#else
#define LINES_PER_FRAME 	440
#define CHARS_PER_LINE		108
#endif
#define CPU_CLOCKS		8000000
#ifdef _FMR60
#define SCREEN_WIDTH		1120
#define SCREEN_HEIGHT		750
#else
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#endif
#define MAX_DRIVE		4
#define MAX_SCSI		8
#define MAX_MEMCARD		2
#define HAS_I286
#define I86_BIOS_CALL
//#define HAS_I386
//#define I386_BIOS_CALL
#define I8259_MAX_CHIPS		2
#define IO_ADDR_MAX		0x10000

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class BEEP;
class HD46505;
#ifdef _FMR60
class HD63484;
#endif
class I8251;
class I8253;
class I8259;
class I86;
//class I386;
class IO;
class MB8877;
class RTC58321;
class UPD71071;

class BIOS;
class CMOS;
class FLOPPY;
class KEYBOARD;
class MEMORY;
//class SERIAL;
class SCSI;
class TIMER;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	BEEP* beep;
	HD46505* crtc;
#ifdef _FMR60
	HD63484* acrtc;
#endif
	I8251* sio;
	I8253* pit0;
	I8253* pit1;
	I8259* pic;
	I86* cpu;
//	I386* cpu;
	IO* io;
	MB8877* fdc;
	RTC58321* rtc;
	UPD71071* dma;
	
	BIOS* bios;
	CMOS* cmos;
	FLOPPY* floppy;
	KEYBOARD* keyboard;
	MEMORY* memory;
	SCSI* scsi;
//	SERIAL* serial;
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
