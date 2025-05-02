/*
	SHARP MZ-3500 Emulator 'EmuZ-3500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.31-

	[ sub ]
*/

#include "sub.h"
#include "main.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 11, eb = (e) >> 11; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) { \
			wbank[i] = wdmy; \
		} \
		else { \
			wbank[i] = (w) + 0x800 * (i - sb); \
		} \
		if((r) == rdmy) { \
			rbank[i] = rdmy; \
		} \
		else { \
			rbank[i] = (r) + 0x800 * (i - sb); \
		}
	} \
}

void SUB::initialize()
{
	_memset(ram, 0, sizeof(ram));
	
	SET_BANK(0x0000, 0x1fff, wdmy, ipl);
	SET_BANK(0x2000, 0x27ff, common, common);
	SET_BANK(0x2800, 0x3fff, wdmy, rdmy);
	SET_BANK(0x4000, 0x5fff, ram, ram);
	SET_BANK(0x6000, 0xffff, wdmy, rdmy);
}

void SUB::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xffff;
	wbank[addr >> 11][addr & 0x7ff] = data;
}

uint32 SUB::read_data8(uint32 addr)
{
	addr &= 0xffff;
	return rbank[addr >> 11][addr & 0x7ff];
}

void SUB::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xf0) {
	case 0x00:
		d_main->write_signal(SIG_MAIN_INT0, 1, 1);
		break;
	case 0x50:
		disp[addr & 0x0f] = data;
		break;
	}
}

uint32 SUB::read_io8(uint32 addr)
{
	switch(addr & 0xf0) {
	case 0x00:
		d_main->write_signal(SIG_MAIN_INT0, 1, 1);
		break;
	case 0x40:
		return (dout ? 1 : 0) | (obf ? 2 : 0) | (dk ? 0x20 : 0) | (stk ? 0x40 : 0) | (hlt ? 0x80 : 0);
	}
	return 0xff;
}



void SUB::draw_text()
{
	

	
	
	
	uint8 code = vram_t[src++ & 0xfff];
	uint8 attr = vram_t[src++ & 0xfff];
	
	// bit0: blink
	// bit1: reverse or green
	// bit2: vertical line or red
	// bit3: horizontal line or blue
	

