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

#define SIG_CASSETTE_CONTROL	0
#define SIG_CASSETTE_RESET	1

#define EVENT_PRE	0
#define EVENT_SIGNAL	1
#define EVENT_AFTER	2

class CASSETTE : public DEVICE
{
private:
	DEVICE* dev;
	int dev_id;
	
	// play sound tape
	uint8 areg, creg;
	int id_pre, id_signal, id_after;
	int fw_rw;
	bool signal, playing;
	float track;
	
	void play_media();
	void stop_media();
	void set_fw_rw(int val);
	void set_signal(bool val);
	
public:
	CASSETTE(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~CASSETTE() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_signal(int id, uint32 data, uint32 mask);
	void event_callback(int event_id);
	
	// unique function
	void set_context(DEVICE* device, int id) {
		dev = device;
		dev_id = id;
	}
};

#endif

