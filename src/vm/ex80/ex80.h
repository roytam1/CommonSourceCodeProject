/*
	TOSHIBA EX-80 Emulator 'eEX-80'

	Author : Takeda.Toshiya
	Date   : 2015.12.10-

	[ virtual machine ]
*/

#ifndef _EX80_H_
#define _EX80_H_

#define DEVICE_NAME		"TOSHIBA EX-80"
#define CONFIG_NAME		"ex80"

// device informations for virtual machine
#define FRAMES_PER_SEC		59.94
#define LINES_PER_FRAME 	525
#define CPU_CLOCKS		2048000
#define HAS_I8080
#define SCREEN_WIDTH		(24 * 12 + 662)
#define SCREEN_HEIGHT		(16 * 29)
#define MEMORY_ADDR_MAX		0x10000
#define MEMORY_BANK_SIZE	0x400
#define IO_ADDR_MAX		0x100

// device informations for win32
#define ONE_BOARD_MICRO_COMPUTER
#define MAX_BUTTONS		25
#define MAX_DRAW_RANGES		9
#define BITMAP_OFFSET_X		(24 * 12)
/*
SW1	ON = STEP / OFF = AUTO
SW2	ON = CHAR / OFF = BIT
SW3-1/2	ON ,ON  = 8000H-81FFH
	OFF,ON  = 8200H-83FFH
	ON ,OFF = 8400H-85FFH
	OFF,OFF = 8600H-87FFH
*/
#define USE_DIPSWITCH
#define DIPSWITCH_DEFAULT	0x0e
#define USE_TAPE
#define TAPE_BINARY_ONLY
#define USE_BINARY_FILE1
#define USE_ALT_F10_KEY
#define USE_AUTO_KEY		5
#define USE_AUTO_KEY_RELEASE	6
#define USE_AUTO_KEY_NO_CAPS
#define USE_DEBUGGER
#define USE_STATE

#include "../../common.h"
#include "../../fileio.h"

#define LED_WIDTH	21
#define LED_HEIGHT	35

const struct {
	const _TCHAR* caption;
	int x, y;
	int width, height;
	int font_size;
	int code;
} buttons[] = {
	// virtual key codes 0x80-0x8f and 0x98-0x9f are not used in pc keyboard
	{_T("0"),	24 * 12 + 524 + 25 * 0,	330 + 23 * 4,	24,	16,	12,	0x80},
	{_T("1"),	24 * 12 + 524 + 25 * 1,	330 + 23 * 4,	24,	16,	12,	0x81},
	{_T("2"),	24 * 12 + 524 + 25 * 2,	330 + 23 * 4,	24,	16,	12,	0x82},
	{_T("3"),	24 * 12 + 524 + 25 * 3,	330 + 23 * 4,	24,	16,	12,	0x83},
	{_T("4"),	24 * 12 + 524 + 25 * 0,	330 + 23 * 3,	24,	16,	12,	0x84},
	{_T("5"),	24 * 12 + 524 + 25 * 1,	330 + 23 * 3,	24,	16,	12,	0x85},
	{_T("6"),	24 * 12 + 524 + 25 * 2,	330 + 23 * 3,	24,	16,	12,	0x86},
	{_T("7"),	24 * 12 + 524 + 25 * 3,	330 + 23 * 3,	24,	16,	12,	0x87},
	{_T("8"),	24 * 12 + 524 + 25 * 0,	330 + 23 * 2,	24,	16,	12,	0x88},
	{_T("9"),	24 * 12 + 524 + 25 * 1,	330 + 23 * 2,	24,	16,	12,	0x89},
	{_T("A"),	24 * 12 + 524 + 25 * 2,	330 + 23 * 2,	24,	16,	12,	0x8a},
	{_T("B"),	24 * 12 + 524 + 25 * 3,	330 + 23 * 2,	24,	16,	12,	0x8b},
	{_T("C"),	24 * 12 + 524 + 25 * 0,	330 + 23 * 1,	24,	16,	12,	0x8c},
	{_T("D"),	24 * 12 + 524 + 25 * 1,	330 + 23 * 1,	24,	16,	12,	0x8d},
	{_T("E"),	24 * 12 + 524 + 25 * 2,	330 + 23 * 1,	24,	16,	12,	0x8e},
	{_T("F"),	24 * 12 + 524 + 25 * 3,	330 + 23 * 1,	24,	16,	12,	0x8f},
	{_T("RET"),	24 * 12 + 524 + 25 * 0,	330 + 23 * 0,	24,	16,	8,	0x98},
	{_T("RUN"),	24 * 12 + 524 + 25 * 1,	330 + 23 * 0,	24,	16,	8,	0x99},
	{_T("SDA"),	24 * 12 + 524 + 25 * 2,	330 + 23 * 0,	24,	16,	8,	0x9a},
	{_T("LDA"),	24 * 12 + 524 + 25 * 3,	330 + 23 * 0,	24,	16,	8,	0x9b},
	{_T("RST"),	24 * 12 + 524 + 25 * 4,	330 + 23 * 0,	24,	16,	8,	0x00},
	{_T("ADR"),	24 * 12 + 524 + 25 * 4,	330 + 23 * 1,	24,	16,	8,	0x9c},
	{_T("RIC"),	24 * 12 + 524 + 25 * 4,	330 + 23 * 2,	24,	16,	8,	0x9d},
	{_T("RDC"),	24 * 12 + 524 + 25 * 4,	330 + 23 * 3,	24,	16,	8,	0x9e},
	{_T("WIC"),	24 * 12 + 524 + 25 * 4,	330 + 23 * 4,	24,	16,	8,	0x9f},
};
const struct {
	int x, y;
	int width, height;
} ranges[] = {
	{24 * 12 + 459 + 24 * 0, 204, LED_WIDTH, LED_HEIGHT }, // 7-seg LEDs
	{24 * 12 + 459 + 24 * 1, 204, LED_WIDTH, LED_HEIGHT },
	{24 * 12 + 459 + 24 * 2, 204, LED_WIDTH, LED_HEIGHT },
	{24 * 12 + 459 + 24 * 3, 204, LED_WIDTH, LED_HEIGHT },
	{24 * 12 + 568 + 24 * 0, 204, LED_WIDTH, LED_HEIGHT },
	{24 * 12 + 568 + 24 * 1, 204, LED_WIDTH, LED_HEIGHT },
	{24 * 12 + 568 + 24 * 2, 204, LED_WIDTH, LED_HEIGHT },
	{24 * 12 + 568 + 24 * 3, 204, LED_WIDTH, LED_HEIGHT },
	{0, 0, 24 * 12, 16 * 29}, // CRT
};

class EMU;
class DEVICE;
class EVENT;

class I8251;
class I8255;
class IO;
class PCM1BIT;
class I8080;

class CMT;
class DISPLAY;
class KEYBOARD;
class MEMORY;

class VM
{
protected:
	EMU* emu;
	
	// devices
	EVENT* event;
	
	I8251* sio;
	I8255* pio;
	IO* io;
	PCM1BIT* pcm;
	I8080* cpu;
	
	CMT* cmt;
	DISPLAY* display;
	KEYBOARD* keyboard;
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
	
#ifdef USE_DEBUGGER
	// debugger
	DEVICE *get_cpu(int index);
#endif
	
	// draw screen
	void draw_screen();
	
	// sound generation
	void initialize_sound(int rate, int samples);
	uint16* create_sound(int* extra_frames);
	int sound_buffer_ptr();
	
	// user interface
	void load_binary(int drv, const _TCHAR* file_path);
	void save_binary(int drv, const _TCHAR* file_path);
	void play_tape(const _TCHAR* file_path);
	void rec_tape(const _TCHAR* file_path);
	void close_tape();
	bool tape_inserted();
	bool now_skip();
	
	void update_config();
	void save_state(FILEIO* state_fio);
	bool load_state(FILEIO* state_fio);
	
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
