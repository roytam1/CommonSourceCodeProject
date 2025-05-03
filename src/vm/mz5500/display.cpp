/*
	SHARP MZ-5500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.10 -

	[ display ]
*/

#include <math.h>
#include "display.h"

void DISPLAY::initialize()
{
	// init pallete
	for(int i = 0; i < 8; i++)
		palette_pc[i] = RGB_COLOR(i & 1 ? 0x1f : 0, i & 2 ? 0x1f : 0, i & 4 ? 0x1f : 0);
}

void DISPLAY::write_io8(uint32 addr, uint32 data)
{
}

uint32 DISPLAY::read_io8(uint32 addr)
{
	return 0xff;
}

void DISPLAY::draw_screen()
{
	uint8 cg = sync[0] & 0x22;
	int al = (sync[6] | (sync[7] << 8)) & 0x3ff;
	
	for(int i = 0, total = 0; i < 4 && total < al; i++) {
		uint32 tmp = ra[4 * i];
		tmp |= ra[4 * i + 1] << 8;
		tmp |= ra[4 * i + 2] << 16;
		tmp |= ra[4 * i + 3] << 24;
		int ptr = (tmp << 1) & 0xfffe;
		int line = (tmp >> 20) & 0x3ff;
		bool wide = ((tmp & 0x80000000) != 0);
		
		for(int y = total; y < total + line && y < 400; y++) {
			if(wide) {
				for(int x = 0; x < 640; x+= 16) {
					uint8 b = vram_b[ptr];
					uint8 r = vram_r[ptr];
					uint8 g = vram_g[ptr++];
					ptr &= 0xffff;
					
					screen[y][x +  0] = screen[y][x +  1] = ((r & 0x01) ? 1 : 0) | ((g & 0x01) ? 2 : 0) | ((b & 0x01) ? 4 : 0);
					screen[y][x +  2] = screen[y][x +  3] = ((r & 0x02) ? 1 : 0) | ((g & 0x02) ? 2 : 0) | ((b & 0x02) ? 4 : 0);
					screen[y][x +  4] = screen[y][x +  5] = ((r & 0x04) ? 1 : 0) | ((g & 0x04) ? 2 : 0) | ((b & 0x04) ? 4 : 0);
					screen[y][x +  6] = screen[y][x +  7] = ((r & 0x08) ? 1 : 0) | ((g & 0x08) ? 2 : 0) | ((b & 0x08) ? 4 : 0);
					screen[y][x +  8] = screen[y][x +  9] = ((r & 0x10) ? 1 : 0) | ((g & 0x10) ? 2 : 0) | ((b & 0x10) ? 4 : 0);
					screen[y][x + 10] = screen[y][x + 11] = ((r & 0x20) ? 1 : 0) | ((g & 0x20) ? 2 : 0) | ((b & 0x20) ? 4 : 0);
					screen[y][x + 12] = screen[y][x + 13] = ((r & 0x40) ? 1 : 0) | ((g & 0x40) ? 2 : 0) | ((b & 0x40) ? 4 : 0);
					screen[y][x + 14] = screen[y][x + 15] = ((r & 0x80) ? 1 : 0) | ((g & 0x80) ? 2 : 0) | ((b & 0x80) ? 4 : 0);
				}
			}
			else {
				for(int x = 0; x < 640; x+= 8) {
					uint8 b = vram_b[ptr];
					uint8 r = vram_r[ptr];
					uint8 g = vram_g[ptr++];
					ptr &= 0xffff;
					
					screen[y][x + 0] = ((r & 0x01) ? 1 : 0) | ((g & 0x01) ? 2 : 0) | ((b & 0x01) ? 4 : 0);
					screen[y][x + 1] = ((r & 0x02) ? 1 : 0) | ((g & 0x02) ? 2 : 0) | ((b & 0x02) ? 4 : 0);
					screen[y][x + 2] = ((r & 0x04) ? 1 : 0) | ((g & 0x04) ? 2 : 0) | ((b & 0x04) ? 4 : 0);
					screen[y][x + 3] = ((r & 0x08) ? 1 : 0) | ((g & 0x08) ? 2 : 0) | ((b & 0x08) ? 4 : 0);
					screen[y][x + 4] = ((r & 0x10) ? 1 : 0) | ((g & 0x10) ? 2 : 0) | ((b & 0x10) ? 4 : 0);
					screen[y][x + 5] = ((r & 0x20) ? 1 : 0) | ((g & 0x20) ? 2 : 0) | ((b & 0x20) ? 4 : 0);
					screen[y][x + 6] = ((r & 0x40) ? 1 : 0) | ((g & 0x40) ? 2 : 0) | ((b & 0x40) ? 4 : 0);
					screen[y][x + 7] = ((r & 0x80) ? 1 : 0) | ((g & 0x80) ? 2 : 0) | ((b & 0x80) ? 4 : 0);
				}
			}
		}
		total += line;
	}
	
	// copy to pc screen
	for(int y = 0; y < 400; y++) {
		uint16* dest = emu->screen_buffer(y);
		uint8* src = screen[y];
		
		for(int x = 0; x < 640; x++)
			dest[x] = palette_pc[src[x] & 7];
	}
	
	// access lamp
	uint32 stat_f = d_fdc->read_signal(0);
	if(stat_f) {
		uint16 col = (stat_f & (1 | 4)) ? RGB_COLOR(31, 0, 0) :
		             (stat_f & (2 | 8)) ? RGB_COLOR(0, 31, 0) : 0;
		for(int y = 400 - 8; y < 400; y++) {
			uint16 *dest = emu->screen_buffer(y);
			for(int x = 640 - 8; x < 640; x++)
				dest[x] = col;
		}
	}
}

