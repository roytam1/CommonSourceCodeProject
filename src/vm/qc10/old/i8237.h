/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.13-

	[ i8237 * 2 ]
*/

#ifndef _I8237_H_
#define _I8237_H_

#include "vm.h"
#include "../emu.h"

#include "device.h"

class I8237 : public DEVICE
{
private:
	typedef struct {
		bool low_high;	// false=low, true=high
		DEVICE* dev[4];
		uint16 areg[4];
		uint16 creg[4];
		uint16 addr[4];
		uint16 count[4];
		uint8 mode[4];
		uint8 cmd;
		uint8 mask;
		uint8 req;
		uint8 run;
		uint8 stat;
	} dma_t;
	dma_t dma[2];
	
	void do_dma();
	
public:
	I8237(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~I8237() {}
	
	// common functions
	void initialize();
	void reset();
	
	void write_io8(uint16 addr, uint8 data);
	uint8 read_io8(uint16 addr);
	
	int iomap_write(int index) {
		static const int map[33] = {
			0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
			0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, -1
		};
		return map[index];
	}
	int iomap_read(int index) {
		static const int map[33] = {
			0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
			0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, -1
		};
		return map[index];
	}
	
	// unique function
	void request_dma(int ch, bool signal);
};

#endif

