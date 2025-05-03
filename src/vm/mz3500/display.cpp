/*
	SHARP MZ-3500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.24 -

	[ display ]
*/

#include <math.h>
#include "display.h"
#include "../upd7220.h"
#include "../../fileio.h"

void DISPLAY::initialize()
{
	_memset(vram_chr, 0, sizeof(vram_chr));
	_memset(vram_gfx, 0, sizeof(vram_gfx));
	
	// load rom images
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sFONT.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(font, sizeof(font), 1);
		fio->Fclose();
	}
	delete fio;
	
	// create pc palette
	for(int i = 0; i < 8; i++)
		palette_pc[i] = RGB_COLOR(i & 1 ? 0x1f : 0, i & 2 ? 0x1f : 0, i & 4 ? 0x1f : 0);
	
	// cursor blinking
	vm->regist_frame_event(this);
	blink = 0;
}

void DISPLAY::write_io8(uint32 addr, uint32 data)
{
}

uint32 DISPLAY::read_io8(uint32 addr)
{
	return 0xff;
}

void DISPLAY::event_frame()
{
	blink++;
}

void DISPLAY::draw_screen()
{
	_memset(screen, 0, sizeof(screen));
	draw_chr();
	draw_gfx();
	
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

void DISPLAY::draw_chr()
{
	int al = (sync_chr[6] | (sync_chr[7] << 8)) & 0x3ff;
	for(int i = 0, total = 0; i < 4 && total < al; i++) {
		uint32 tmp = ra_chr[4 * i];
		tmp |= ra_chr[4 * i + 1] << 8;
		tmp |= ra_chr[4 * i + 2] << 16;
		tmp |= ra_chr[4 * i + 3] << 24;
		
		int ptr = (tmp << 1) & (VRAM_SIZE_CHR - 1);
		int line = (tmp >> 20) & 0x3ff;
		bool wide = ((tmp & 0x80000000) != 0);
		int caddr = ((cs_chr[0] & 0x80) && ((cs_chr[1] & 0x20) || !(blink & 0x10))) ? (*ead_chr << 1) : -1;
		
		for(int y = total; y < total + line;) {
			if(wide) {
				for(int x = 0; x < 640; x += 16) {
					bool cursor = (ptr == caddr);
					uint8 code = vram_chr[ptr++];
					ptr &= VRAM_SIZE_CHR - 1;
					uint8 attrib = vram_chr[ptr++];
					ptr &= VRAM_SIZE_CHR - 1;
					uint8* pattern = &font[0x1000 | (code * 16)];
					
					for(int l = y % 16; l < 16 && (y + l) < 400; l++) {
						uint8 pat = pattern[l];
						// attribute
						if(attrib & 8)
							pat = ~pat;
						uint8 col = 7;//attrib & 7;
						
						if(pat & 0x80) screen[y + l][x +  0] = screen[y + l][x +  1] =  col;
						if(pat & 0x40) screen[y + l][x +  2] = screen[y + l][x +  3] =  col;
						if(pat & 0x20) screen[y + l][x +  4] = screen[y + l][x +  5] =  col;
						if(pat & 0x10) screen[y + l][x +  6] = screen[y + l][x +  7] =  col;
						if(pat & 0x08) screen[y + l][x +  8] = screen[y + l][x +  9] =  col;
						if(pat & 0x04) screen[y + l][x + 10] = screen[y + l][x + 11] =  col;
						if(pat & 0x02) screen[y + l][x + 12] = screen[y + l][x + 13] =  col;
						if(pat & 0x01) screen[y + l][x + 14] = screen[y + l][x + 15] =  col;
					}
					if(cursor) {
						int top = cs_chr[1] & 0x1f, bottom = cs_chr[2] >> 3;
						for(int l = top; l < bottom && l < 16; l++)
							_memset(&screen[y + l][x], 1, 16);
					}
				}
			}
			else{
				for(int x = 0; x < 640; x += 8) {
					bool cursor = (ptr == caddr);
					uint8 code = vram_chr[ptr++];
					ptr &= VRAM_SIZE_CHR - 1;
					uint8 attrib = vram_chr[ptr++];
					ptr &= VRAM_SIZE_CHR - 1;
					uint8* pattern = &font[0x1000 | (code * 16)];
					
					for(int l = y % 16; l < 16 && (y + l) < 400; l++) {
						uint8 pat = pattern[l];
						// attribute
						if(attrib & 8)
							pat = ~pat;
						uint8 col = 7;//attrib & 7;
						
						if(pat & 0x80) screen[y + l][x + 0] = col;
						if(pat & 0x40) screen[y + l][x + 1] = col;
						if(pat & 0x20) screen[y + l][x + 2] = col;
						if(pat & 0x10) screen[y + l][x + 3] = col;
						if(pat & 0x08) screen[y + l][x + 4] = col;
						if(pat & 0x04) screen[y + l][x + 5] = col;
						if(pat & 0x02) screen[y + l][x + 6] = col;
						if(pat & 0x01) screen[y + l][x + 7] = col;
					}
					if(cursor) {
						int top = cs_chr[1] & 0x1f, bottom = cs_chr[2] >> 3;
						for(int l = top; l < bottom && l < 16; l++)
							_memset(&screen[y + l][x], 1, 8);
					}
				}
			}
			y += 16 - (y % 16);
		}
		total += line;
	}
}

void DISPLAY::draw_gfx()
{
#if 0
	int al = (sync_gfx[6] | (sync_gfx[7] << 8)) & 0x3ff;
	for(int i = 0, total = 0; i < 4 && total < al; i++) {
		uint32 tmp = ra_gfx[4 * i];
		tmp |= ra_gfx[4 * i + 1] << 8;
		tmp |= ra_gfx[4 * i + 2] << 16;
		tmp |= ra_gfx[4 * i + 3] << 24;
		
		int ptr = (tmp << 1) & (VRAM_SIZE_GFX - 1);
		int line = (tmp >> 20) & 0x3ff;
		bool wide = ((tmp & 0x80000000) != 0);
		
		for(int y = total; y < total + line && y < 400; y++) {
			if(wide) {
				for(int x = 0; x < 640; x+= 16) {
					uint8 pat = vram[ptr++];
					ptr &= VRAM_SIZE_GFX - 1;
					
					scr_gfx[y][x +  0] = scr_gfx[y][x +  1] = (pat & 0x01) ? 1 : 0;
					scr_gfx[y][x +  2] = scr_gfx[y][x +  3] = (pat & 0x02) ? 1 : 0;
					scr_gfx[y][x +  4] = scr_gfx[y][x +  5] = (pat & 0x04) ? 1 : 0;
					scr_gfx[y][x +  6] = scr_gfx[y][x +  7] = (pat & 0x08) ? 1 : 0;
					scr_gfx[y][x +  8] = scr_gfx[y][x +  9] = (pat & 0x10) ? 1 : 0;
					scr_gfx[y][x + 10] = scr_gfx[y][x + 11] = (pat & 0x20) ? 1 : 0;
					scr_gfx[y][x + 12] = scr_gfx[y][x + 13] = (pat & 0x40) ? 1 : 0;
					scr_gfx[y][x + 14] = scr_gfx[y][x + 15] = (pat & 0x80) ? 1 : 0;
				}
			}
			else {
				for(int x = 0; x < 640; x+= 8) {
					uint8 pat = vram[ptr++];
					ptr &= VRAM_SIZE_GFX - 1;
					
					scr_gfx[y][x + 0] = (pat & 0x01) ? 1 : 0;
					scr_gfx[y][x + 1] = (pat & 0x02) ? 1 : 0;
					scr_gfx[y][x + 2] = (pat & 0x04) ? 1 : 0;
					scr_gfx[y][x + 3] = (pat & 0x08) ? 1 : 0;
					scr_gfx[y][x + 4] = (pat & 0x10) ? 1 : 0;
					scr_gfx[y][x + 5] = (pat & 0x20) ? 1 : 0;
					scr_gfx[y][x + 6] = (pat & 0x40) ? 1 : 0;
					scr_gfx[y][x + 7] = (pat & 0x80) ? 1 : 0;
				}
			}
		}
		total += line;
	}
#endif
}

