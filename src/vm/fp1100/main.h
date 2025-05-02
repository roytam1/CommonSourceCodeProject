/*
	CASIO FP-1100 Emulator 'eFP-1100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.06.17-

	[ main pcb ]
*/

#ifndef _MAIN_H_
#define _MAIN_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_MAIN_INTA	0
#define SIG_MAIN_INTB	1
#define SIG_MAIN_INTC	2
#define SIG_MAIN_INTD	3
#define SIG_MAIN_INTS	4
#define SIG_MAIN_COMM	5

class MAIN : public DEVICE
{
private:
	// to main cpu
	DEVICE *d_cpu;
	// to sub pcb
	DEVICE *d_sub;
	int did_int2, did_comm;
	// to slots
	DEVICE *d_slot[8];
	
	uint8 *wbank[16];
	uint8 *rbank[16];
	uint8 wdmy[0x1000];
	uint8 rdmy[0x1000];
	uint8 ram[0x10000];
	uint8 rom[0x9000];
	
	uint8 comm_data;
	uint8 slot_sel;
	uint8 intr_mask;
	uint8 intr_req;
	
	void update_intr();
	
public:
	MAIN(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		intr_mask = intr_req = 0;
	}
	~MAIN() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_data16(uint32 addr, uint32 data) {
		write_data8(addr, data & 0xff); write_data8(addr + 1, data >> 8);
	}
	uint32 read_data16(uint32 addr) {
		return read_data8(addr) | (read_data8(addr + 1) << 8);
	}
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	uint32 intr_ack();
	void intr_reti();
	
	// unique functions
	void set_context_cpu(DEVICE *device) {
		d_cpu = device;
	}
	void set_context_sub(DEVICE *device, int id_int2, int id_comm) {
		d_sub = device;
		did_int2 = id_int2; did_comm = id_comm;
	}
	void set_context_slot(int slot, DEVICE *device) {
		d_slot[slot] = device;
	}
};

#endif
