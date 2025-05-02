/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.04 -

	[ cassette ]
*/

#ifndef _CASSETTE_H_
#define _CASSETTE_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_CASSETTE_PIO_PA	0
#define SIG_CASSETTE_PIO_PC	1
#define SIG_CASSETTE_OUT	2
#define SIG_CASSETTE_REMOTE	3
#define SIG_CASSETTE_END	4
#define SIG_CASSETTE_TOP	5

#define EVENT_PRE	0
#define EVENT_SIGNAL	1
#define EVENT_AFTER	2

class DATAREC;

class CASSETTE : public DEVICE
{
private:
	DEVICE* d_pio;
	DATAREC *d_drec;
	
	uint8 pa, pb, pc;
	bool play, rec;
	bool now_play, now_rewind, now_apss;
	int register_id;
	
public:
	CASSETTE(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~CASSETTE() {}
	
	// common functions
	void initialize();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_callback(int event_id, int err);
	
	// unique function
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
	void set_context_datarec(DATAREC* device) {
		d_drec = device;
	}
	void play_datarec(bool value);
	void rec_datarec(bool value);
	void close_datarec();
};

#endif

