/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.17 -

	[ Z80PIO ]
*/

#ifndef _Z80PIO_H_
#define _Z80PIO_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_Z80PIO_PORT_A	0
#define SIG_Z80PIO_PORT_B	1

class Z80PIO : public DEVICE
{
private:
	DEVICE* dev[2][MAX_OUTPUT];
	int did[2][MAX_OUTPUT], dshift[2][MAX_OUTPUT], dcount[2];
	uint32 dmask[2][MAX_OUTPUT];
	
	typedef struct {
		uint8 wreg;
		uint8 rreg;
		uint8 mode;
		uint8 ctrl1;
		uint8 ctrl2;
		uint8 dir;
		uint8 mask;
		uint8 vector;
		bool set_dir;
		bool set_mask;
		bool first;
		// interrupt
		bool enb_intr;
		bool req_intr;
		bool in_service;
	} port_t;
	port_t port[2];
	
	// interrupt
	DEVICE *d_cpu, *d_child;
	bool iei, oei, intr;
	uint32 intr_bit;
	void check_mode3_intr(int ch);
	void update_intr();
	
public:
	Z80PIO(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount[0] = dcount[1] = 0;
		d_cpu = d_child = NULL;
		port[0].wreg = port[1].wreg = port[0].rreg = port[1].rreg = 0;//0xff;
	}
	~Z80PIO() {}
	
	// common functions
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// interrupt common functions
	void set_intr_iei(bool val);
	uint32 intr_ack();
	void intr_reti();
	
	// unique function
	void set_context_intr(DEVICE* device, uint32 bit) {
		d_cpu = device;
		intr_bit = bit;
	}
	void set_context_child(DEVICE* device) {
		d_child = device;
	}
	void set_context_port_a(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount[0]++;
		dev[0][c] = device; did[0][c] = id; dmask[0][c] = mask; dshift[0][c] = shift;
	}
	void set_context_port_b(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount[1]++;
		dev[1][c] = device; did[1][c] = id; dmask[1][c] = mask; dshift[1][c] = shift;
	}
};

#endif

