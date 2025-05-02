/*
	NEC PC-9801 Emulator 'ePC-9801'
	NEC PC-9801E/F/M Emulator 'ePC-9801E'
	NEC PC-98DO Emulator 'ePC-98DO'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.09.15-

	[ virtual machine ]
*/

#ifndef _PC9801_H_
#define _PC9801_H_

#if defined(_PC9801)
#define DEVICE_NAME		"NEC PC-9801"
#define CONFIG_NAME		"pc9801"
#elif defined(_PC9801E)
#define DEVICE_NAME		"NEC PC-9801E/F/M"
#define CONFIG_NAME		"pc9801e"
#elif defined(_PC9801U)
#define DEVICE_NAME		"NEC PC-9801U"
#define CONFIG_NAME		"pc9801u"
#elif defined(_PC9801VM)
#define DEVICE_NAME		"NEC PC-9801VM"
#define CONFIG_NAME		"pc9801vm"
#elif defined(_PC98DO)
#define DEVICE_NAME		"NEC PC-98DO"
#define CONFIG_NAME		"pc98do"
#else
#endif
#define CONFIG_VERSION		0x02

#if defined(_PC9801) || defined(_PC9801E)
#define SUPPORT_CMT_IF
#define SUPPORT_OLD_FDD_IF
#define SUPPORT_OLD_BUZZER
#endif

#if !(defined(_PC9801) || defined(_PC9801U))
#define SUPPORT_2ND_VRAM
#endif
#if !(defined(_PC9801) || defined(_PC9801E))
#define SUPPORT_16_COLORS
#endif

#if defined(_PC98DO)
#define SUPPORT_VARIABLE_TIMING
#define MODE_PC98	0
#define MODE_PC88_V1S	1
#define MODE_PC88_V1H	2
#define MODE_PC88_V2	3
#define MODE_PC88_N	4
#endif

// device informations for virtual machine
#define FRAMES_PER_SEC		56.4
#define LINES_PER_FRAME 	440
#if defined(_PC9801)
#define CPU_CLOCKS		5000000
#define PIT_CLOCK_5MHZ
#elif defined(_PC9801E)
#define CPU_CLOCKS		8000000
#define PIT_CLOCK_8MHZ
#else
#define CPU_CLOCKS		10000000
#define PIT_CLOCK_5MHZ
#endif
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#define MAX_DRIVE		2
//#define UPD765A_WAIT_SEEK
#define UPD765A_NO_ST1_EN_OR_FOR_RESULT7
//#define UPD765A_MEDIA_CHANGE
#define UPD7220_MSB_FIRST
#if defined(_PC9801) || defined(_PC9801E)
#define HAS_I86
#else
#define HAS_V30
#endif
#define I8259_MAX_CHIPS		2
#define SINGLE_MODE_DMA
#define MEMORY_ADDR_MAX		0x100000
#define MEMORY_BANK_SIZE	0x800
#define IO_ADDR_MAX		0x10000
//#define EVENT_PRECISE		10
#if !defined(SUPPORT_OLD_BUZZER)
#define PCM1BIT_HIGH_QUALITY
#endif

// device informations for win32
#define USE_FD1
#define USE_FD2
#if defined(_PC98DO)
// for PC-8801 drives
#define USE_FD3
#define USE_FD4
#elif defined(SUPPORT_OLD_FDD_IF)
// for 2DD drives and inteligent 2D drives
#define USE_FD3
#define USE_FD4
#define USE_FD5
#define USE_FD6
#endif
#if defined(SUPPORT_CMT_IF)
#define USE_DATAREC
#define DATAREC_BINARY_ONLY
#endif
#define NOTIFY_KEY_DOWN
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6
#define USE_ACCESS_LAMP
#if defined(_PC98DO)
#define USE_SCANLINE
#endif

#include "../../common.h"

class EMU;
class DEVICE;
class EVENT;

#if defined(SUPPORT_OLD_BUZZER)
class BEEP;
#endif
class I8237;
class I8251;
class I8253;
class I8255;
class I8259;
class I86;
class IO;
class LS244;
class MEMORY;
#if defined(HAS_I86) || defined(HAS_V30)
class NOT;
#endif
#if !defined(SUPPORT_OLD_BUZZER)
class PCM1BIT;
#endif
class UPD1990A;
class UPD7220;
class UPD765A;
class YM2203;

#if defined(SUPPORT_CMT_IF)
class CMT;
#endif
class DISPLAY;
class FLOPPY;
class JOYSTICK;
class KEYBOARD;
class MOUSE;

#if defined(SUPPORT_OLD_FDD_IF)
// 320kb fdd drives
class PC80S31K;
class Z80;
#endif

#if defined(_PC98DO)
class BEEP;
class PC80S31K;
class PC8801;
class Z80;
#endif

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
#if defined(SUPPORT_OLD_BUZZER)
	BEEP* beep;
#else
	PCM1BIT* beep;
#endif
	I8237* dma;
#if defined(SUPPORT_CMT_IF)
	I8251* sio_cmt;
#endif
	I8251* sio_rs;
	I8251* sio_kbd;
	I8253* pit;
#if defined(SUPPORT_OLD_FDD_IF)
	I8255* pio_fdd;
#endif
	I8255* pio_mouse;
	I8255* pio_sys;
	I8255* pio_prn;
	I8259* pic;
	I86* cpu;
	IO* io;
	LS244* dmareg1;
	LS244* dmareg2;
	LS244* dmareg3;
	LS244* dmareg0;
	MEMORY* memory;
#if defined(HAS_I86) || defined(HAS_V30)
	NOT* not;
#endif
	UPD1990A* rtc;
#if defined(SUPPORT_OLD_FDD_IF)
	UPD765A* fdc_2hd;
	UPD765A* fdc_2dd;
#else
	UPD765A* fdc;
#endif
	UPD7220* gdc_chr;
	UPD7220* gdc_gfx;
	YM2203* opn;
	
#if defined(SUPPORT_CMT_IF)
	CMT* cmt;
#endif
	DISPLAY* display;
	FLOPPY* floppy;
	JOYSTICK* joystick;
	KEYBOARD* keyboard;
	MOUSE* mouse;
	
#if defined(SUPPORT_OLD_FDD_IF)
	// 320kb fdd drives
	I8255* pio_sub;
	PC80S31K *pc80s31k;
	UPD765A* fdc_sub;
	Z80* cpu_sub;
#endif
	
	// memory
	uint8 ram[0xa0000];
	uint8 ipl[0x18000];
	uint8 sound_bios[0x2000];
#if defined(SUPPORT_OLD_FDD_IF)
	uint8 fd_bios_2hd[0x1000];
	uint8 fd_bios_2dd[0x1000];
#endif
	
#if defined(_PC98DO)
	EVENT* pc88event;
	
	PC8801* pc88;
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
	
	int boot_mode;
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
	void run();
	
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
	void open_disk(_TCHAR* file_path, int drv);
	void close_disk(int drv);
#if defined(SUPPORT_CMT_IF)
	void play_datarec(_TCHAR* filename);
	void rec_datarec(_TCHAR* filename);
	void close_datarec();
#endif
	bool now_skip();
	
#if defined(_PC98DO)
	double frame_rate();
#endif
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
	
	// devices
	DEVICE* get_device(int id);
	DEVICE* dummy;
	DEVICE* first_device;
	DEVICE* last_device;
};

#endif
