/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ data recorder ]
*/

#ifndef _DREC_H_
#define _DREC_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_DATAREC_OUT		0
#define SIG_DATAREC_REMOTE	1

class FILEIO;

#define BUFFER_SIZE 0x20000

static uint8 header[44] = {
	'R' , 'I' , 'F' , 'F' , 0x00, 0x00, 0x00, 0x00, 'W' , 'A' , 'V' , 'E' , 'f' , 'm' , 't' , ' ' ,
	0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x80, 0xbb, 0x00, 0x00, 0x80, 0xbb, 0x00, 0x00,
	0x01, 0x00, 0x08, 0x00, 'd' , 'a' , 't' , 'a' , 0x00, 0x00, 0x00, 0x00
};

class DATAREC : public DEVICE
{
private:
	DEVICE* dev[MAX_OUTPUT];
	int did[MAX_OUTPUT], dcount;
	uint32 dmask[MAX_OUTPUT];
	
	// data recorder
	FILEIO* fio;
	int regist_id;
	bool play, rec, is_wave;
	bool in, out, remote;
	
	int bufcnt, samples;
	uint32 remain;
	uint8 buffer[BUFFER_SIZE];
	
	void update_event();
	bool check_extension(_TCHAR* filename);
	
public:
	DATAREC(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount = 0;
	}
	~DATAREC() {}
	
	// common functions
	void initialize();
	void reset();
	void release();
	void write_signal(int id, uint32 data, uint32 mask);
	uint32 read_signal(int ch) {
		return in ? 1 : 0;
	}
	void event_callback(int event_id, int err);
	
	// unique functions
	void set_context(DEVICE* device, int id, uint32 mask) {
		int c = dcount++;
		dev[c] = device; did[c] = id; dmask[c] = mask;
	}
	void play_datarec(_TCHAR* filename);
	void rec_datarec(_TCHAR* filename);
	void close_datarec();
	bool skip() {
		return remote && ((play && remain > 0) || rec);
	}
};

#endif

