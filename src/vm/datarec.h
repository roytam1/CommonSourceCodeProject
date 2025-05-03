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
#define SIG_DATAREC_TRIG	2

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
	DEVICE *d_out[MAX_OUTPUT], *d_remote[MAX_OUTPUT];
	int did_out[MAX_OUTPUT], did_remote[MAX_OUTPUT];
	int dcount_out, dcount_remote;
	uint32 dmask_out[MAX_OUTPUT], dmask_remote[MAX_OUTPUT];
	
	// data recorder
	FILEIO* fio;
	int regist_id;
	bool play, rec, is_wave;
	bool in, out, change, remote, trig;
	
	int bufcnt, samples;
	uint32 remain;
	uint8 buffer[BUFFER_SIZE];
	
	void update_event();
	bool check_extension(_TCHAR* filename);
	
public:
	DATAREC(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_out = dcount_remote = 0;
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
	void set_context_out(DEVICE* device, int id, uint32 mask) {
		int c = dcount_out++;
		d_out[c] = device; did_out[c] = id; dmask_out[c] = mask;
	}
	void set_context_remote(DEVICE* device, int id, uint32 mask) {
		int c = dcount_remote++;
		d_remote[c] = device; did_remote[c] = id; dmask_remote[c] = mask;
	}
	void play_datarec(_TCHAR* filename);
	void rec_datarec(_TCHAR* filename);
	void close_datarec();
	bool skip();
};

#endif

