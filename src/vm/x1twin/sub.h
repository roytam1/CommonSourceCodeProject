/*
	SHARP X1twin Emulator 'eX1twin'
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

class FIFO;

class SUB : public DEVICE
{
private:
	DEVICE *d_cpu, *d_pio;
	
	uint8 databuf[32][8], *datap;
	uint8 mode, cmdlen, datalen;
	uint8 inbuf, outbuf;
	bool ibf, obf;
	
	FIFO* key_buf;
	uint8* key_stat;
	int key_prev, key_break;
	bool caps, kana;
	
	bool iei, intr;
	uint32 intr_bit;
	
	void update_intr();
	void process_cmd();
	void set_ibf(bool val);
	void set_obf(bool val);
	uint16 get_key(int code);
	
public:
	SUB(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~SUB() {}
	
	// common functions
	void initialize();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void set_intr_iei(bool val);
	uint32 intr_ack();
	void intr_reti();
	void event_callback(int event_id, int err);
	
	// unique functions
	void set_context_intr(DEVICE* device, uint32 bit) {
		d_cpu = device;
		intr_bit = bit;
	}
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
	void key_down(int code);
	void key_up(int code);
};

#endif

