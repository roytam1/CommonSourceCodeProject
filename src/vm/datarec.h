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
#define SIG_DATAREC_REMOTE_NEG	2
#define SIG_DATAREC_TRIG	3
#define SIG_DATAREC_REWIND	4

class FILEIO;

#define DATAREC_BUFFER_SIZE 0x800000

class DATAREC : public DEVICE
{
private:
	// output signals
	outputs_t outputs_out;
	outputs_t outputs_remote;
	outputs_t outputs_end;
	
	// data recorder
	FILEIO* fio;
	int register_id;
	bool play, rec;
	bool in, out, change, remote, trig;
	bool is_wav, is_tap, is_mzt;
	
	int bufcnt, samples;
	int ch, sample_rate, sample_bits;
	int remain;
	uint8 buffer[DATAREC_BUFFER_SIZE];
	
	void update_event();
	bool check_file_extension(_TCHAR* file_path, _TCHAR* ext);
	
	void load_image();
	
	int load_wav_image();
	void save_wav_image();
	uint8 get_wav_sample();
	
	int load_tap_image();
	int load_mzt_image();
	
public:
	DATAREC(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		init_output_signals(&outputs_out);
		init_output_signals(&outputs_remote);
		init_output_signals(&outputs_end);
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
		register_output_signal(&outputs_out, device, id, mask);
	}
	void set_context_remote(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_remote, device, id, mask);
	}
	void set_context_end(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_end, device, id, mask);
	}
	bool play_datarec(_TCHAR* file_path);
	bool rec_datarec(_TCHAR* file_path);
	void close_datarec();
	bool skip();
};

#endif

