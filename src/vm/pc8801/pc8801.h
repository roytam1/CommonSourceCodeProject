/*
	NEC PC-8801MA Emulator 'ePC-8801MA'
	NEC PC-8001mkIISR Emulator 'ePC-8001mkIISR'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2012.02.16-

	[ virtual machine ]
*/

#ifndef _PC8801_H_
#define _PC8801_H_

#if defined(_PC8801MA)
#define DEVICE_NAME		"NEC PC-8801MA"
#define CONFIG_NAME		"pc8801ma"
#elif defined(_PC8001SR)
#define DEVICE_NAME		"NEC PC-8001mkIISR"
#define CONFIG_NAME		"pc8801mk2sr"
#endif
#define CONFIG_VERSION		0x02

#if defined(_PC8001SR)
#define MODE_PC80_V1	0
#define MODE_PC80_V2	1
#define MODE_PC80_N	2
#else
#define MODE_PC88_V1S	0
#define MODE_PC88_V1H	1
#define MODE_PC88_V2	2
#define MODE_PC88_N	3
#endif

#if defined(_PC8801MA)
#define SUPPORT_PC88_DICTIONARY
#define SUPPORT_PC88_HIGH_CLOCK
#define SUPPORT_PC88_OPNA
#define PC88_EXRAM_BANKS	4
#define HAS_UPD4990A
#endif
#define SUPPORT_PC88_JOYSTICK

#define Z80_MEMORY_WAIT

// device informations for virtual machine
#define FRAMES_PER_SEC		60
#define LINES_PER_FRAME 	260
#define CPU_CLOCKS		3993600
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#define MAX_DRIVE		2
#define UPD765A_NO_ST1_EN_OR_FOR_RESULT7
#ifdef SUPPORT_PC88_OPNA
#define HAS_YM2608
#endif
#define PCM1BIT_HIGH_QUALITY
#define SUPPORT_SOUND_FREQ_55467HZ
#define SUPPORT_VARIABLE_TIMING

// device informations for win32
#define USE_FD1
#define USE_FD2
#define USE_DATAREC
#define DATAREC_PC8801
#define NOTIFY_KEY_DOWN
#define USE_SHIFT_NUMPAD_KEY
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6
#define USE_ACCESS_LAMP
#define USE_SCANLINE
#define USE_BOOT_MODE
#define USE_CPU_CLOCK_LOW

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

class BEEP;
class I8251;
class I8255;
class PCM1BIT;
class UPD1990A;
class YM2203;
class Z80;

class PC80S31K;
class UPD765A;

class PC88;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* pc88event;
	
	BEEP* pc88beep;
	I8251* pc88sio;
	I8255* pc88pio;
	PCM1BIT* pc88pcm;
	UPD1990A* pc88rtc;
	YM2203* pc88opn;
	Z80* pc88cpu;
	
	PC80S31K* pc88sub;
	I8255* pc88pio_sub;
	UPD765A* pc88fdc_sub;
	Z80* pc88cpu_sub;
	
	PC88* pc88;
	
	int boot_mode;
	
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
	double frame_rate();
	
	// draw screen
	void draw_screen();
	int access_lamp();
	
	// sound generation
	void initialize_sound(int rate, int samples);
	uint16* create_sound(int* extra_frames);
	
	// notify key
	void key_down(int code, bool repeat);
	void key_up(int code);
	
	// user interface
	void open_disk(int drv, _TCHAR* file_path, int offset);
	void close_disk(int drv);
	bool disk_inserted(int drv);
	void play_datarec(_TCHAR* file_path);
	void rec_datarec(_TCHAR* file_path);
	void close_datarec();
	bool now_skip();
	
	void update_config();
	
	// ----------------------------------------
	// for each device
	// ----------------------------------------
	
	// devices
	DEVICE* get_device(int id);
	DEVICE* dummy;
	DEVICE* first_device;
	DEVICE* last_device;
};

#endif
