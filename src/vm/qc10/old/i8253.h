/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.09-

	[ i8253 * 2chips ]
*/

#ifndef _I8253_H_
#define _I8253_H_

#include "vm.h"
#include "../emu.h"

#include "device.h"

class I8253 : public DEVICE
{
private:
	typedef struct {
		int32 count;
		uint16 latch;
		uint8 ctrl_reg;
		uint16 count_reg;
		uint8 mode;
		bool mode0_flag;
		uint8 r_cnt;
		uint8 delay;
		bool gate;
		bool start;
	} counter_t;
	counter_t counter[6];
	
	int update_clock(int ch, int clock);
	int clocks;
	
public:
	I8253(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~I8253() {}
	
	// common functions
	void initialize();
	
	void write_io8(uint16 addr, uint8 data);
	uint8 read_io8(uint16 addr);
	
	int iomap_write(int index) {
		static const int map[9] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, -1 };
		return map[index];
	}
	int iomap_read(int index) {
		static const int map[7] = { 0x00, 0x01, 0x02, 0x04, 0x05, 0x06, -1 };
		return map[index];
	}
	
	// unique functions
	void input_gate(int ch, bool signal);
	void input_clock(int clock);
};

#endif

