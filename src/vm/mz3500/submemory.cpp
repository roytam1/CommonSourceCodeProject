/*
	SHARP MZ-5500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.23 -

	[ sub memory ]
*/

#include "submemory.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 11, eb = (e) >> 11; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) \
			wbank[i] = wdmy; \
		else \
			wbank[i] = (w) + 0x800 * (i - sb); \
		if((r) == rdmy) \
			rbank[i] = rdmy; \
		else \
			rbank[i] = (r) + 0x800 * (i - sb); \
	} \
}

void SUBMEMORY::initialize()
{
	_memset(ram, 0, sizeof(ram));
	
	SET_BANK(0x0000, 0x1fff, wdmy, ipl);
	SET_BANK(0x2000, 0x27ff, common, common);
	SET_BANK(0x2800, 0x3fff, wdmy, rdmy);
	SET_BANK(0x4000, 0x5fff, ram, ram);
	SET_BANK(0x6000, 0xffff, wdmy, rdmy);
}

void SUBMEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xffff;
	wbank[addr >> 11][addr & 0x7ff] = data;
}

uint32 SUBMEMORY::read_data8(uint32 addr)
{
	addr &= 0xffff;
	return rbank[addr >> 11][addr & 0x7ff];
}

void SUBMEMORY::write_io8(uint32 addr, uint32 data)
{
	// $00: main cpu int
	d_mem->write_signal(did_mem, 1, 1);
}

