/*
	CASIO PV-1000 Emulator 'ePV-1000'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.16 -

	[ video processor ]
*/

#include "vdp.h"

void VDP::initialize()
{
	// regist event to interrupt
	vm->regist_vsync_event(this);
}

void VDP::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0xfe:
		vram = base + (data << 8);
		pcg = base + (data << 8) + 0x400;
		break;
	case 0xff:
		pattern = base + (data << 8);
		break;
	}
}

void VDP::event_vsync(int v, int clock)
{
	if(v == LINES_PER_HBLANK && dev->accept_int())
		dev->write_signal(SIG_CPU_DO_INT, 0xff, 0xffffffff);
}

void VDP::draw_screen()
{
	_memset(bg, 0, sizeof(bg));
	
	for(int y = 0; y < 24; y++) {
		int y8 = y << 3, y32 = y << 5;
		
		for(int x = 0; x < 32; x++) {
			int x8 = x << 3;
			uint8 code = vram[y32 + x];
			
			if(code < 0xe0)
				draw_pattern(x8, y8, code << 5);
			else
				draw_pcg(x8, y8, (code & 0x1f) << 5);
		}
	}
	for(int y = 0; y < 192; y++) {
		uint16* dest = emu->screen_buffer(y);
		for(int x = 0; x < 256; x++)
			dest[x] = palette_pc[bg[y][x] & 7];
	}
}

void VDP::draw_pattern(int x8, int y8, uint16 top)
{
	// draw pattern on rom
	for(int p = 1; p < 4; p++) {
		uint8 col = plane[p];
		uint16 p8 = top + (p << 3);
		
		for(int l = 0; l < 8; l++) {
			uint8* dest = &bg[y8 + l][x8];
			uint8 pat = pattern[p8 + l];
			
			if(pat & 0x80) dest[0] |= col;
			if(pat & 0x40) dest[1] |= col;
			if(pat & 0x20) dest[2] |= col;
			if(pat & 0x10) dest[3] |= col;
			if(pat & 0x08) dest[4] |= col;
			if(pat & 0x04) dest[5] |= col;
			if(pat & 0x02) dest[6] |= col;
			if(pat & 0x01) dest[7] |= col;
		}
	}
}

void VDP::draw_pcg(int x8, int y8, uint16 top)
{
	// draw pattern on ram
	for(int p = 1; p < 4; p++) {
		uint8 col = plane[p];
		uint16 p8 = top + (p << 3);
		
		for(int l = 0; l < 8; l++) {
			uint8* dest = &bg[y8 + l][x8];
			uint8 pat = pcg[p8 + l];
			
			if(pat & 0x80) dest[0] |= col;
			if(pat & 0x40) dest[1] |= col;
			if(pat & 0x20) dest[2] |= col;
			if(pat & 0x10) dest[3] |= col;
			if(pat & 0x08) dest[4] |= col;
			if(pat & 0x04) dest[5] |= col;
			if(pat & 0x02) dest[6] |= col;
			if(pat & 0x01) dest[7] |= col;
		}
	}
}

