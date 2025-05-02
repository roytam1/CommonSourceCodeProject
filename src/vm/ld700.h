/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2014.02.12-

	[ Pioneer LD-700 ]
*/

#ifndef _LD700_H_
#define _LD700_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_LD700_LREMO	0

#define MAX_TRACKS	1024
#define MAX_PAUSES	1024

class LD700 : public DEVICE
{
private:
	// output signals
	outputs_t outputs_drec_remote;
	outputs_t outputs_exv;
	outputs_t outputs_ack;
	
	bool prev_signal;
	uint32 prev_time;
	uint32 command, num_bits;
	
	int phase, status;
	int seek_mode, seek_num;
	bool accepted;
	int cur_frame_raw, cur_track;
	
	_TCHAR disc_path[MAX_PATH];
	int num_tracks, track_frame_raw[MAX_TRACKS];
	int num_pauses, pause_frame_raw[MAX_PAUSES];
	
	void set_status(int value);
	void set_ack(bool value);
	void set_cur_frame(int frame, bool relative);
	int get_cur_frame();
	int get_cur_frame_raw();
	void set_cur_track(int track);
	int get_cur_track();
	
public:
	LD700(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		init_output_signals(&outputs_drec_remote);
		init_output_signals(&outputs_exv);
		init_output_signals(&outputs_ack);
	}
	~LD700() {}
	
	// common functions
	void initialize();
	void write_signal(int id, uint32 data, uint32 mask);
	void event_frame();
	void event_callback(int event_id, int err);
	
	// unique functions
	void set_context_drec_remote(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_drec_remote, device, id, mask);
	}
	void set_context_exv(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_exv, device, id, mask);
	}
	void set_context_ack(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_ack, device, id, mask);
	}
	void open_disc(_TCHAR* file_path);
	void close_disc();
	bool disc_inserted();
};

#endif

