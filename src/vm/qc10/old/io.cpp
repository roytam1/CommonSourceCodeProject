/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ i/o bus ]
*/

#include <stdio.h>
#include "io.h"
#include "memory.h"
#include "z80.h"

void IO::initialize()
{
	for(int i = 0; i < 256; i++) {
		map_w[i] = vm->dummy;
		map_r[i] = vm->dummy;
	}
}

void IO::regist_iomap(DEVICE* dev)
{
	// write
	for(int i = 0; ; i++) {
		int port = dev->iomap_write(i);
		if(port == -1)
			break;
		map_w[port] = dev;
	}
	
	// read
	for(int i = 0; ; i++) {
		int port = dev->iomap_read(i);
		if(port == -1)
			break;
		map_r[port] = dev;
	}
}

void IO::write_io8(uint16 addr, uint8 data)
{
	map_w[addr & 0xff]->write_io8(addr, data);
#if 0
//	if(map_w[addr & 0xff]->iomap_write(0) == -1)
		emu->out_debug(_T("%4x : wr %2x, %2x\n"), vm->cpu->prvPC, addr & 0xff, data);
#endif
}

uint8 IO::read_io8(uint16 addr)
{
	uint8 val = map_r[addr & 0xff]->read_io8(addr);
#if 0
//	if(map_r[addr & 0xff]->iomap_write(0) == -1)
		emu->out_debug(_T("%4x : rd %2x, %2x\n"), vm->cpu->prvPC, addr & 0xff, val);
#endif
	return val;
}
