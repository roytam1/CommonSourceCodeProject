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

#define DATAREC_BUFFER_SIZE 0x20000

class DATAREC : public DEVICE
{
private:
	// output signals
	outputs_t outputs_out;
	outputs_t outputs_remote;
	
	// data recorder
	FILEIO* fio;
	int regist_id;
	bool play, rec, is_wave;
	bool in, out, change, remote, trig;
	
	int bufcnt, samples;
	uint32 remain;
	uint8 buffer[DATAREC_BUFFER_SIZE];
	
	void update_event();
	bool check_extension(_TCHAR* filename);
	
public:
	DATAREC(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		init_output_signals(&outputs_out);
		init_output_signals(&outputs_remote);
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
		regist_output_signal(&outputs_out, device, id, mask);
	}
	void set_context_remote(DEVICE* device, int id, uint32 mask) {
		regist_output_signal(&outputs_remote, device, id, mask);
	}
	void play_datarec(_TCHAR* filename);
	void rec_datarec(_TCHAR* filename);
	void close_datarec();
	bool skip();
};

#endif

