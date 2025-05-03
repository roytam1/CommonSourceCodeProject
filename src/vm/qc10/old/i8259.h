/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ i8259 ]
*/

#ifndef _I8259_H_
#define _I8259_H_

#include "vm.h"
#include "../emu.h"

#include "device.h"

class I8259 : public DEVICE
{
private:
	typedef struct {
		uint8 imr, isr, irr, prio;
		uint8 icw1, icw2, icw3, icw4;
		uint8 icw2_r, icw3_r, icw4_r;
		uint8 special, input;
	} pic_t;
	pic_t pic[2];
	
	void do_interrupt();
	
public:
	I8259(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~I8259() {}
	
	// common functions
	void initialize();
	
	void write_io8(uint16 addr, uint8 data);
	uint8 read_io8(uint16 addr);
	
	int iomap_write(int index) {
		static const int map[5] = { 0x08, 0x09, 0x0c, 0x0d, -1 };
		return map[index];
	}
	int iomap_read(int index) {
		static const int map[5] = { 0x08, 0x09, 0x0c, 0x0d, -1 };
		return map[index];
	}
	
	void do_reti() {}
	void do_ei() { do_interrupt(); }
	
	// unique functions
	void request_int(int ch, bool signal);
};

#endif

