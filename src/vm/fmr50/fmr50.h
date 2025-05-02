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

#if defined(_FMR50)
#define DEVICE_NAME		"FUJITSU FMR-50"
#define CONFIG_NAME		"fmr50"
#elif defined(_FMR60)
#define DEVICE_NAME		"FUJITSU FMR-60"
#define CONFIG_NAME		"fmr60"
#elif defined(_FMRCARD)
#define DEVICE_NAME		"FUJITSU FMR-CARD"
#define CONFIG_NAME		"fmrcard"
#elif defined(_OASYS30)
#define DEVICE_NAME		"FUJITSU OASYS 30"
#define CONFIG_NAME		"oasys30"
#elif defined(_OASYSPOCKET3)
#define DEVICE_NAME		"FUJITSU OASYS Pocket3"
#define CONFIG_NAME		"oasyspocket3"
#endif
#define CONFIG_VERSION		0x01

// device informations for virtual machine
#define FRAMES_PER_SEC		55.4
#if defined(_FMR60)
#define LINES_PER_FRAME 	784
#define CHARS_PER_LINE		98
#else
#define LINES_PER_FRAME 	440
#define CHARS_PER_LINE		54
#endif
//#if defined(_FMRCARD) || defined(_OASYS30) || defined(_OASYSPOCKET3)
#define CPU_CLOCKS		8000000
//#else
//#define CPU_CLOCKS		12000000
//#endif
#if defined(_FMR60)
#define SCREEN_WIDTH		1120
#define SCREEN_HEIGHT		750
#elif defined(_OASYSPOCKET3)
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		200
#else
#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400
#endif
#define MAX_DRIVE		4
#define MAX_SCSI		8
#define MAX_MEMCARD		2
#define HAS_I286
#define I86_BIOS_CALL
#define HAS_I386
#define I386_BIOS_CALL
#define I8259_MAX_CHIPS		2
//#define SINGLE_MODE_DMA
#define IO_ADDR_MAX		0x10000

// device informations for win32
#define USE_FD1
#define USE_FD2
#define USE_FD3
#define USE_FD4
#define NOTIFY_KEY_DOWN
#define USE_SHIFT_NUMPAD_KEY
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6
#define USE_ACCESS_LAMP

#include "../../common.h"

static uint32 machine_ids[][2] = {
	{0x6a868423, 0xf8},	// FMR-50FD/HD		87/05/22 V12 L10
	{0xb6e51659, 0xf8},	// FMR-60FD/HD		87/01/27 V12 L09
	{0x3777a7cb, 0xe0},	// FMR-50FX/HX		89/01/24 V12 L12
				// FMR-60FX/HX		
	{0xde3044c7, 0xd8},	// FMR-50LT5/LT6	88/10/04 V40 L12
	{0x6eaeaef1, 0x0a},	// FMR-50NE/T3		94/12/01 V70 L01
	{0xafc9606e, 0xf8},	// Panacom M500HD	89/04/14 V12 L12
	{0x78a920e9, 0x70},	// FMR-CARD		92/02/10 V63 L13
//	{0x86072415, 0x0a},	// FMR-250L4		94/10/20 V90 L12
//	{0xe77c3518, 0x0a}.	// FMR-250N/T3		96/04/10 V69 L15
//	{0x06b44c36, 0xba},	// OASYS 30AX401	92/10/20
//	{0x7cf02e08, 0xba},	// OASYS Pocket 3	??/??/??
	{-1, -1}
};

class EMU;
class DEVICE;
class EVENT;

class HD46505;
#ifdef _FMR60
class HD63484;
#endif
class I8251;
class I8253;
class I8259;
class I286;
class I386;
class IO;
class MB8877;
class MSM58321;
class PCM1BIT;
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
	
	HD46505* crtc;
#if defined(_FMR60)
	HD63484* acrtc;
#endif
	I8251* sio;
	I8253* pit0;
	I8253* pit1;
	I8259* pic;
	I286* i286;
	I386* i386;
	IO* io;
	MB8877* fdc;
	MSM58321* rtc;
	PCM1BIT* pcm;
	UPD71071* dma;
	
	BIOS* bios;
	CMOS* cmos;
	FLOPPY* floppy;
	KEYBOARD* keyboard;
	MEMORY* memory;
	SCSI* scsi;
//	SERIAL* serial;
	TIMER* timer;
	
private:
	bool is_i286;
	
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
	void open_disk(int drv, _TCHAR* file_path, int offset);
	void close_disk(int drv);
	bool disk_inserted(int drv);
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
