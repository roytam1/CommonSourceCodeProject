/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.11-

	[ virtual machine ]
*/

#ifndef _X1TWIN_H_
#define _X1TWIN_H_

#define DEVICE_NAME		"SHARP X1twin"
#define CONFIG_NAME		"x1twin"
#define CONFIG_VERSION		0x02

// device informations for virtual machine (x1)
#define FRAMES_PER_10SECS	600
#define FRAMES_PER_SEC		60
#define LINES_PER_FRAME 	266
#define CHARS_PER_LINE		114
// 250*266*60
#define CPU_CLOCKS		3990000
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#define MAX_DRIVE		4
#define IO_ADDR_MAX		0x10000
#define HAS_AY_3_8912
#ifndef _X1TURBO
#define Z80_M1_CYCLE_WAIT	1
#endif

#ifdef _X1TWIN
// device informations for virtual machine (pce)
#define PCE_FRAMES_PER_SEC	60
#define PCE_LINES_PER_FRAME 	263
#define PCE_CPU_CLOCKS		7159090
#endif

// device informations for win32
#define USE_SPECIAL_RESET
#define USE_FD1
#define USE_FD2
#define USE_DATAREC
#define USE_DATAREC_BUTTON
#define DATAREC_TAP
#ifdef _X1TWIN
#define USE_CART
#endif
#define NOTIFY_KEY_DOWN
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6
#define USE_SCANLINE

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class DATAREC;
class HD46505;
class I8255;
class MB8877;
class YM2151;
class YM2203;
class Z80;
class Z80CTC;
#ifdef _X1TURBO
class Z80DMA;
class Z80SIO;
#endif

class DISPLAY;
class EMM;
class FLOPPY;
class IO;
class JOYSTICK;
class KANJI;
class MEMORY;
class SUB;

#ifdef _X1TWIN
class HUC6260;
class PCE;
#endif

class VM
{
protected:
	EMU* emu;
	
	// devices for x1
	EVENT* event;
	
	DATAREC* drec;
	HD46505* crtc;
	I8255* pio;
	MB8877* fdc;
	YM2151* opm;
	YM2203* psg;
	Z80* cpu;
	Z80CTC* ctc;
#ifdef _X1TURBO
	Z80DMA* dma;
	Z80SIO* sio;
#endif
	
	DISPLAY* display;
	EMM* emm;
	FLOPPY* floppy;
	IO* io;
	JOYSTICK* joy;
	KANJI* kanji;
	MEMORY* memory;
	SUB* sub;
	
	// device for pce
	EVENT* pceevent;
	
#ifdef _X1TWIN
	HUC6260* pcecpu;
	PCE* pce;
#endif
	
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
	void special_reset();
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
	void push_play();
	void push_stop();
	bool now_skip();
#ifdef _X1TWIN
	void open_cart(_TCHAR* filename);
	void close_cart();
#endif
	
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
#ifdef _X1TWIN
	void pce_regist_event(DEVICE* device, int event_id, int usec, bool loop, int* regist_id);
	void pce_regist_event_by_clock(DEVICE* device, int event_id, int clock, bool loop, int* regist_id);
	void pce_cancel_event(int regist_id);
	void pce_regist_frame_event(DEVICE* dev);
	void pce_regist_vline_event(DEVICE* dev);
#endif
	
	// clock
	uint32 current_clock();
	uint32 passed_clock(uint32 prev);
	uint32 get_prv_pc();
#ifdef _X1TWIN
	uint32 pce_current_clock();
	uint32 pce_passed_clock(uint32 prev);
	uint32 pce_get_prv_pc();
#endif
	
	// devices
	DEVICE* get_device(int id);
	DEVICE* dummy;
	DEVICE* first_device;
	DEVICE* last_device;
	
#ifdef _X1TWIN
	bool pce_running;
#endif
};

#endif
