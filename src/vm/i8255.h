/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.14 -

	[ i8255 ]
*/

#ifndef _I8255_H_
#define _I8255_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_I8255_PORT_A	0
#define SIG_I8255_PORT_B	1
#define SIG_I8255_PORT_C	2

class I8255 : public DEVICE
{
private:
	DEVICE* dev[3][MAX_OUTPUT];
	int did[3][MAX_OUTPUT], dshift[3][MAX_OUTPUT], dcount[3];
	uint32 dmask[3][MAX_OUTPUT];
	
	typedef struct {
		uint8 wreg;
		uint8 rreg;
		uint8 rmask;
		bool first;
	} port_t;
	port_t port[3];
	
public:
	I8255(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount[0] = dcount[1] = dcount[2] = 0;
		port[0].wreg = port[1].wreg = port[2].wreg = port[0].rreg = port[1].rreg = port[2].rreg = 0;//0xff;
	}
	~I8255() {}
	
	// common functions
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_port_a(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount[0]++;
		dev[0][c] = device; did[0][c] = id; dmask[0][c] = mask; dshift[0][c] = shift;
	}
	void set_context_port_b(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount[1]++;
		dev[1][c] = device; did[1][c] = id; dmask[1][c] = mask; dshift[1][c] = shift;
	}
	void set_context_port_c(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount[2]++;
		dev[2][c] = device; did[2][c] = id; dmask[2][c] = mask; dshift[2][c] = shift;
	}
};

#endif

