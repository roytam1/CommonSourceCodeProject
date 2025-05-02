/*
	NEC PC-6001 Emulator 'yaPC-6001'

	Author : tanam
	Date   : 2013.07.15-

	[ keyboard ]
*/

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define STICK0_SPACE 0x80
#define STICK0_LEFT  0x20
#define STICK0_RIGHT 0x10
#define STICK0_DOWN  0x08
#define STICK0_UP    0x04
#define STICK0_STOP  0x02
#define STICK0_SHIFT 0x01

#define SIG_DATAREC_OUT		0
#define SIG_DATAREC_REMOTE	1
#define SIG_DATAREC_TRIG	2

class FILEIO;

class KEYBOARD : public DEVICE
{
private:
	DEVICE *d_cpu, *d_pio, *d_mem;
	// output signals
	outputs_t outputs_out;
	outputs_t outputs_remote;
	outputs_t outputs_rotate;
	outputs_t outputs_end;
	outputs_t outputs_top;
	outputs_t outputs_apss;
	
	// data recorder
	FILEIO* fio;
	bool play, rec, remote, trigger;
	int ff_rew;
	bool in_signal, out_signal;
	uint32 prev_clock;
	int positive_clocks, negative_clocks;
	int changed;
	int register_id;
	int sample_rate;
	uint8 *buffer, *buffer_bak;
	int buffer_ptr, buffer_length;
	bool is_wav;
	bool *apss_buffer;
	int apss_ptr, apss_count, apss_remain;
	bool apss_signals;
	void update_event();
	void close_file();	
	int load_cas_image();
	int load_wav_image(int offset);
	void save_wav_image();
	int load_tap_image();
	int load_mzt_image();

	int CasMode;
	int CasIndex;
	uint8 CasData[0x10000];

	uint8* key_stat;
   	byte stick0;
	int kbFlagFunc;
	int kbFlagGraph;
	int kbFlagCtrl;
	int kanaMode;
	int katakana;
	void update_keyboard();
	uint8 counter;
	int p6key;
public:
	KEYBOARD(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		init_output_signals(&outputs_out);
		init_output_signals(&outputs_remote);
		init_output_signals(&outputs_rotate);
		init_output_signals(&outputs_end);
		init_output_signals(&outputs_top);
		init_output_signals(&outputs_apss);
	}
	~KEYBOARD() {}
	
	// common functions
	void initialize();
	void event_frame();
	void reset();
	void release();
	void write_signal(int id, uint32 data, uint32 mask);
	uint32 read_signal(int ch) {
		return in_signal ? 1 : 0;
	}
	void event_callback(int event_id, int err);

	// unique functions
	void set_context_cpu(DEVICE* device) {
		d_cpu = device;
	}
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
	void set_context_memory(DEVICE* device) {
		d_mem = device;
	}

	void set_context_out(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_out, device, id, mask);
	}
	void set_context_remote(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_remote, device, id, mask);
	}
	void set_context_rotate(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_rotate, device, id, mask);
	}
	void set_context_end(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_end, device, id, mask);
	}
	void set_context_top(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_top, device, id, mask);
	}
	void set_context_apss(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_apss, device, id, mask);
	}
	bool play_tape(_TCHAR* file_path);
	bool rec_tape(_TCHAR* file_path);
	void close_tape();
	bool tape_inserted() {
		return (play || rec);
	}
	void set_remote(bool value);
	void set_ff_rew(int value);
	bool do_apss(int value);

	// interrupt cpu to device
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	uint32 intr_ack();
};
#endif
