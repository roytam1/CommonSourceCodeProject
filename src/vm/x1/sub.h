/*
	SHARP X1twin Emulator 'eX1twin'
	SHARP X1turbo Emulator 'eX1turbo'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.15-

	[ sub cpu ]
*/

#ifndef _SUB_H_
#define _SUB_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_SUB_TAPE_END	0

class DATAREC;
class FIFO;

class SUB : public DEVICE
{
private:
	DEVICE *d_cpu, *d_pio;
	DATAREC *d_drec;
	
	uint8 databuf[32][8], *datap;
	uint8 mode, inbuf, outbuf;
	bool ibf, obf;
	int cmdlen, datalen;
	
	FIFO* key_buf;
	uint8* key_stat;
	int key_prev, key_break, key_repeat;
	bool key_converted[256];
	bool key_shift, key_ctrl, key_graph;
	bool key_caps, key_caps_locked;
	bool key_shift_released;
	bool key_kana, key_kana_locked;
	
	bool play, rec, eot;
	
	bool iei, intr;
	uint32 intr_bit;
	
	void update_intr();
	void process_cmd();
	void set_ibf(bool val);
	void set_obf(bool val);
	uint8 get_key_low();
	uint16 get_key(int code, bool repeat);
	
public:
	SUB(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~SUB() {}
	
	// common functions
	void initialize();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_frame();
	void event_callback(int event_id, int err);
	
	// interrupt common functions
	void set_context_intr(DEVICE* device, uint32 bit) {
		d_cpu = device;
		intr_bit = bit;
	}
	void set_intr_iei(bool val);
	uint32 intr_ack();
	void intr_reti();
	
	// unique functions
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
	void set_context_datarec(DATAREC* device) {
		d_drec = device;
	}
	void key_down(int code, bool repeat);
	void key_up(int code);
	void play_datarec(bool value);
	void rec_datarec(bool value);
	void close_datarec();
	void push_play();
	void push_stop();
};

#endif

