/*
	NEC PC-98LT Emulator 'ePC-98LT'
	NEC PC-98HA Emulator 'eHANDY98'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.09 -

	[ virtual machine ]
*/

#ifndef _PC98HA_H_
#define _PC98HA_H_

// device informations for win32
#ifdef _PC98HA
#define DEVICE_NAME		"NEC PC-98HA"
#define CONFIG_NAME		"pc98ha"
#else
#define DEVICE_NAME		"NEC PC-98LT"
#define CONFIG_NAME		"pc98lt"
#endif
#define CONFIG_VERSION		0x01

#define WINDOW_WIDTH1		640
#define WINDOW_HEIGHT1		400
#define WINDOW_WIDTH2		640
#define WINDOW_HEIGHT2		400

#define USE_FD1
#define NOTIFY_KEY_DOWN
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6

// device informations for virtual machine
#define FRAMES_PER_10SECS	554
#define FRAMES_PER_SEC		55.4
#define LINES_PER_FRAME 	440
#define CHARS_PER_LINE		108
#ifdef _PC98HA
#define CPU_CLOCKS		10000000
#else
#define CPU_CLOCKS		8000000
#endif
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#define MAX_DRIVE		1
#define HAS_V30
#define I86_BIOS_CALL
#define I8259_MAX_CHIPS		1
#define UPD765A_DMA_MODE
//#define UPD765A_WAIT_SEEK
#define UPD765A_STRICT_ID
#define UPD765A_MEDIA_CHANGE
#define IO_ADDR_MAX		0x10000
#define IOBUS_RETURN_ADDR
#define EVENT_PRECISE	40
#ifdef _PC98HA
//#define DOCKING_STATION
#endif

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class BEEP;
class I8251;
class I8253;
class I8255;
class I8259;
class I86;
class IO;
#ifdef _PC98HA
class UPD4991A;
#else
class UPD1990A;
#endif
class UPD71071;
class UPD765A;

class BIOS;
#ifdef _PC98HA
class CALENDAR;
#endif
class DISPLAY;
class FLOPPY;
class KEYBOARD;
class MEMORY;
class NOTE;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	BEEP* beep;
	I8251* sio_r;
	I8251* sio_k;
	I8253* pit;
	I8255* pio_s;
	I8255* pio_p;
	I8259* pic;
	I86* cpu;
	IO* io;
#ifdef _PC98HA
	UPD4991A* rtc;
#else
	UPD1990A* rtc;
#endif
	UPD71071* dma;
	UPD765A* fdc;
	
	BIOS* bios;
#ifdef _PC98HA
	CALENDAR* calendar;
#endif
	DISPLAY* display;
	FLOPPY* floppy;
	KEYBOARD* keyboard;
	MEMORY* memory;
	NOTE* note;
	
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
