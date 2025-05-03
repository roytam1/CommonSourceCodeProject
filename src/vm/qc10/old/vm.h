/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ virtual machine ]
*/


#ifndef _VM_H_
#define _VM_H_

#include "../common.h"

// event callback
#define CALLBACK_MAX	8
// fdc event
#define EVENT_FDC_PHASE	0
#define EVENT_FDC_TC	1
#define EVENT_FDC_SEEK	2
#define EVENT_FDC_LOST	3
#define EVENT_SIO_SEND	4
// 5	ch.1
#define EVENT_SIO_RECV	6
// 7	ch.1

class EMU;
class DEVICE;

class Z80;
class MEMORY;
class IO;

class HD146818P;
class I8237;
class I8253;
class I8255;
class I8259;
class SOUND;
class UPD7201;
class UPD7220;
class UPD765A;

class VM
{
	// define friend
	friend Z80;
	friend MEMORY;
	friend IO;

	friend HD146818P;
	friend I8237;
	friend I8253;
	friend I8255;
	friend I8259;
	friend SOUND;
	friend UPD7201;
	friend UPD7220;
	friend UPD765A;
protected:
	EMU* emu;
	
	Z80* cpu;
	MEMORY* memory;
	IO* io;
	
	HD146818P* rtc;
	I8237* dmac;
	I8253* pit;
	I8255* pio;
	I8259* pic;
	SOUND* sound;
	UPD7201* sio;
	UPD7220* crtc;
	UPD765A* fdc;
	// i/o non connected
	DEVICE* dummy;
	
private:
	// event
	void drive_vm(int clock);
	
	typedef struct {
		bool enable;
		DEVICE* device;
		int event_id;
		int clock;
		int loop;
	} event_t;
	event_t event[CALLBACK_MAX];
	int event_clock;
	int past_clock;
	int clocks[CHARS_PER_LINE];
	
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
	void run();
	void reset();
	
	// draw screen
	void draw_screen();
	
	// sound generation
	void initialize_sound(int rate, int samples);
	uint16* create_sound(int samples, bool fill);
	
	// input
	void key_down(int code);
	void key_up(int code);
	
	// user interface
	void insert_disk(_TCHAR* filename, int drv);
	void eject_disk(int drv);
	
	bool now_skip();
	void update_config();
	
	// ----------------------------------------
	// for each device
	// ----------------------------------------
	
	// event callback
	void regist_callback(DEVICE* device, int event_id, int usec, bool loop, int* regist_id);
	void regist_callback_by_clock(DEVICE* device, int event_id, int clock, bool loop, int* regist_id);
	void cancel_callback(int regist_id);
	
	// devices
	DEVICE* first_device;
	DEVICE* last_device;
};

#endif
