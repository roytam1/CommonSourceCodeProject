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
	DEVICE* d_pic;
	int pri;
	
	typedef struct {
		uint8 wreg;
		uint8 rreg;
		uint8 mode;
		uint8 ctrl1;
		uint8 ctrl2;
		uint8 dir;
		uint8 mask;
		uint8 vector;
		bool int_enb;
		bool set_dir;
		bool set_mask;
		bool prv_req;
		bool first;
	} port_t;
	port_t port[2];
	void do_interrupt(int ch);
	
public:
	Z80PIO(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount[0] = dcount[1] = 0;
		d_pic = NULL;
		port[0].wreg = port[1].wreg = port[0].rreg = port[1].rreg = 0;//0xff;
	}
	~Z80PIO() {}
	
	// common functions
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique function
	void set_context_port_a(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount[0]++;
		dev[0][c] = device; did[0][c] = id; dmask[0][c] = mask; dshift[0][c] = shift;
	}
	void set_context_port_b(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount[1]++;
		dev[1][c] = device; did[1][c] = id; dmask[1][c] = mask; dshift[1][c] = shift;
	}
	void set_context_int(DEVICE* device, int priority) {
		d_pic = device; pri = priority;
	}
};

#endif

