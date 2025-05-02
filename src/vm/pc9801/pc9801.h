/*
	NEC PC-9801 Emulator 'ePC-9801'
	NEC PC-9801E/F/M Emulator 'ePC-9801E'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.09.15-

	[ virtual machine ]
*/

#ifndef _PC9801_H_
#define _PC9801_H_

#ifdef _PC9801
#define DEVICE_NAME		"NEC PC-9801"
#define CONFIG_NAME		"pc9801"
#else
#define DEVICE_NAME		"NEC PC-9801E/F/M"
#define CONFIG_NAME		"pc9801e"
#endif
#define CONFIG_VERSION		0x01

// device informations for virtual machine
#define FRAMES_PER_10SECS	554
#define FRAMES_PER_SEC		55.4
#define LINES_PER_FRAME 	440
#define CHARS_PER_LINE		108
#ifdef _PC9801
#define CPU_CLOCKS		5000000
#else
#define CPU_CLOCKS		8000000
#endif
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#define MAX_DRIVE		4
//#define UPD765A_DMA_MODE
//#define UPD765A_DRQ_DELAY
//#define UPD765A_WAIT_SEEK
#define UPD765A_STRICT_ID
#define UPD765A_NO_ST1_EN_OR_FOR_RESULT7
#define UPD765A_MEDIA_CHANGE
#define UPD7220_MSB_FIRST
#define HAS_I86
#define I8259_MAX_CHIPS		2
#define MEMORY_ADDR_MAX		0x100000
#define MEMORY_BANK_SIZE	0x1000
#define IO_ADDR_MAX		0x10000
//#define EVENT_PRECISE		10

// device informations for win32
#define USE_FD1
#define USE_FD2
#define USE_FD3
#define USE_FD4
#define NOTIFY_KEY_DOWN
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class BEEP;
class I8237;
class I8251;
class I8253;
class I8255;
class I8259;
class I86;
class IO;
class LS244;
class MEMORY;
class NOT;
class UPD1990A;
class UPD7220;
class UPD765A;
class YM2203;

class DISPLAY;
class FLOPPY;
class JOYSTICK;
class KEYBOARD;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	BEEP* beep;
	I8237* dma;
	I8251* sio_rs;
	I8251* sio_kbd;
	I8253* pit;
	I8255* pio_sys;
	I8255* pio_prn;
	I8255* pio_fdd;
	I8259* pic;
	I86* cpu;
	IO* io;
	LS244* dmareg1;
	LS244* dmareg2;
	LS244* dmareg3;
	LS244* dmareg0;
	MEMORY* memory;
	NOT* not;
	UPD1990A* rtc;
	UPD7220* gdc_chr;
	UPD7220* gdc_gfx;
	UPD765A* fdc_2hd;
	UPD765A* fdc_2dd;
	YM2203* opn;
	
	DISPLAY* display;
	FLOPPY* floppy;
	JOYSTICK* joystick;
	KEYBOARD* keyboard;
	
	// memory
	uint8 ram[0xa0000];
	uint8 ipl[0x18000];
	uint8 sound_bios[0x2000];
	uint8 fd_bios_2hd[0x1000];
	uint8 fd_bios_2dd[0x1000];
	
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
	void open_disk(_TCHAR* file_path, int drv);
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
