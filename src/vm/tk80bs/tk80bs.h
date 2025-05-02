/*
	NEC TK-80BS (COMPO BS/80) Emulator 'eTK-80BS'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.08.26 -

	[ virtual machine ]
*/

#ifndef _TK80BS_H_
#define _TK80BS_H_

#define DEVICE_NAME		"NEC TK-80BS"
#define CONFIG_NAME		"tk80bs"
#define CONFIG_VERSION		0x01

// device informations for virtual machine
#define FRAMES_PER_10SECS	599
#define FRAMES_PER_SEC		59.9
#define LINES_PER_FRAME 	262
#define CHARS_PER_LINE		1
#define CPU_CLOCKS		2048000
//#define CPU_START_ADDR		0xf000
#define HAS_I8080
#define SCREEN_WIDTH		256
#define SCREEN_HEIGHT		164
#define MEMORY_ADDR_MAX		0x10000
#define MEMORY_BANK_SIZE	0x200
#define IO_ADDR_MAX		0x10000

// device informations for win32
#define USE_DATAREC
#define DATAREC_BINARY_ONLY
#define USE_RAM
#define NOTIFY_KEY_DOWN
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6
#define USE_AUTO_KEY_NO_CAPS

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class I8251;
class I8255;
class IO;
class MEMORY;
class PCM1BIT;
class I8080;

class CMT;
class DISPLAY;
class KEYBOARD;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	I8251* sio_b;
	I8255* pio_b;
	I8255* pio_t;
	IO* memio;
	MEMORY* memory;
	PCM1BIT* pcm0;
	PCM1BIT* pcm1;
	I8080* cpu;
	
	CMT* cmt;
	DISPLAY* display;
	KEYBOARD* keyboard;
	
	// memory
	uint8 mon[0x800];
	uint8 ext[0x7000];
	uint8 basic[0x2000];
	uint8 bsmon[0x1000];
	uint8 ram[0x5000];	// with TK-M20K
	uint8 vram[0x200];
	
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
	void key_down(int code);
	void key_up(int code);
	
	// user interface
	void load_ram(_TCHAR* file_path);
	void save_ram(_TCHAR* file_path);
	void play_datarec(_TCHAR* file_path);
	void rec_datarec(_TCHAR* file_path);
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
