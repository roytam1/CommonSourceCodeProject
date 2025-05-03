/*
	EPSON QC-10 Emulator 'eQC-10'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.02.16 -

	[ display ]
*/

#include <math.h>
#include "display.h"
#include "../../fileio.h"

void DISPLAY::initialize()
{
	_memset(vram, 0, sizeof(vram));
	
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
	for(int i = 1; i < 8; i++) {
		palette_pc[i + 0] = RGB_COLOR(0, 0x14, 0);
		palette_pc[i + 8] = RGB_COLOR(0, 0x1f, 0);
	}
	palette_pc[0] = palette_pc[8] = 0;
	
	// cursor blinking
	vm->regist_frame_event(this);
	blink = 0;
}

uint32 DISPLAY::read_io8(uint32 addr)
{
	// display type $2c
	return 0xfe; // monochrome
	//return 0x02; // color
}

void DISPLAY::event_frame()
{
	blink++;
}

void DISPLAY::draw_screen()
{
	int al = (sync[6] | (sync[7] << 8)) & 0x3ff;
	
	for(int i = 0, total = 0; i < 4 && total < al; i++) {
		uint32 tmp = ra[4 * i];
		tmp |= ra[4 * i + 1] << 8;
		tmp |= ra[4 * i + 2] << 16;
		tmp |= ra[4 * i + 3] << 24;
		
		int ptr = tmp & 0x3ffff;
		int line = (tmp >> 20) & 0x3ff;
		bool gfx = ((tmp & 0x40000000) != 0);
		bool wide = ((tmp & 0x80000000) != 0);
		
		if((sync[0] & 0x22) == 0x20)	// char mode
			ptr &= 0x1fff;
		ptr <<= 1;
		int caddr = ((cs[0] & 0x80) && ((cs[1] & 0x20) || !(blink & 0x10))) ? (*ead << 1) : -1;
		
		if(gfx) {
			for(int y = total; y < total + line && y < 400; y++) {
				if(wide) {
					for(int x = 0; x < 640; x+= 16) {
						uint8 pat = vram[ptr++];
						
						screen[y][x +  0] = screen[y][x +  1] = (pat & 0x80) ? 1 : 0;
						screen[y][x +  2] = screen[y][x +  3] = (pat & 0x40) ? 1 : 0;
						screen[y][x +  4] = screen[y][x +  5] = (pat & 0x20) ? 1 : 0;
						screen[y][x +  6] = screen[y][x +  7] = (pat & 0x10) ? 1 : 0;
						screen[y][x +  8] = screen[y][x +  9] = (pat & 0x08) ? 1 : 0;
						screen[y][x + 10] = screen[y][x + 11] = (pat & 0x04) ? 1 : 0;
						screen[y][x + 12] = screen[y][x + 13] = (pat & 0x02) ? 1 : 0;
						screen[y][x + 14] = screen[y][x + 15] = (pat & 0x01) ? 1 : 0;
					}
				}
				else {
					for(int x = 0; x < 640; x+= 8) {
						uint8 pat = vram[ptr++];
						
						screen[y][x + 0] = (pat & 0x80) ? 1 : 0;
						screen[y][x + 1] = (pat & 0x40) ? 1 : 0;
						screen[y][x + 2] = (pat & 0x20) ? 1 : 0;
						screen[y][x + 3] = (pat & 0x10) ? 1 : 0;
						screen[y][x + 4] = (pat & 0x08) ? 1 : 0;
						screen[y][x + 5] = (pat & 0x04) ? 1 : 0;
						screen[y][x + 6] = (pat & 0x02) ? 1 : 0;
						screen[y][x + 7] = (pat & 0x01) ? 1 : 0;
					}
				}
			}
		}
		else {
			for(int y = total; y < total + line;) {
				if(wide) {
					for(int x = 0; x < 640; x += 16) {
						bool cursor = (ptr == caddr);
						uint8 code = vram[ptr++];
						uint8 attrib = vram[ptr++];
						uint8* pattern = &font[code * 16];
						
						for(int l = y % 16; l < 16 && (y + l) < 400; l++) {
							uint8 pat = pattern[l];
							// attribute
							if((attrib & 0x40) || ((attrib & 0x80) && (blink & 0x10)))
								pat = 0;
							if(attrib & 8)
								pat = ~pat;
							uint8 col = (attrib & 4) ? 9 : 1;
							
							screen[y + l][x +  0] = screen[y + l][x +  1] = (pat & 0x01) ? col : 0;
							screen[y + l][x +  2] = screen[y + l][x +  3] = (pat & 0x02) ? col : 0;
							screen[y + l][x +  4] = screen[y + l][x +  5] = (pat & 0x04) ? col : 0;
							screen[y + l][x +  6] = screen[y + l][x +  7] = (pat & 0x08) ? col : 0;
							screen[y + l][x +  8] = screen[y + l][x +  9] = (pat & 0x10) ? col : 0;
							screen[y + l][x + 10] = screen[y + l][x + 11] = (pat & 0x20) ? col : 0;
							screen[y + l][x + 12] = screen[y + l][x + 13] = (pat & 0x40) ? col : 0;
							screen[y + l][x + 14] = screen[y + l][x + 15] = (pat & 0x80) ? col : 0;
						}
						if(cursor) {
							int top = cs[1] & 0x1f, bottom = cs[2] >> 3;
							for(int l = top; l < bottom && l < 16; l++)
								_memset(&screen[y + l][x], 1, 16);
						}
					}
				}
				else{
					for(int x = 0; x < 640; x += 8) {
						bool cursor = (ptr == caddr);
						uint8 code = vram[ptr++];
						uint8 attrib = vram[ptr++];
						uint8* pattern = &font[code * 16];
						
						for(int l = y % 16; l < 16 && (y + l) < 400; l++) {
							uint8 pat = pattern[l];
							// attribute
							if((attrib & 0x40) || ((attrib & 0x80) && (blink & 0x10)))
								pat = 0;
							if(attrib & 8)
								pat = ~pat;
							uint8 col = (attrib & 4) ? 9 : 1;
							
							screen[y + l][x + 0] = (pat & 0x01) ? col : 0;
							screen[y + l][x + 1] = (pat & 0x02) ? col : 0;
							screen[y + l][x + 2] = (pat & 0x04) ? col : 0;
							screen[y + l][x + 3] = (pat & 0x08) ? col : 0;
							screen[y + l][x + 4] = (pat & 0x10) ? col : 0;
							screen[y + l][x + 5] = (pat & 0x20) ? col : 0;
							screen[y + l][x + 6] = (pat & 0x40) ? col : 0;
							screen[y + l][x + 7] = (pat & 0x80) ? col : 0;
						}
						if(cursor) {
							int top = cs[1] & 0x1f, bottom = cs[2] >> 3;
							for(int l = top; l < bottom && l < 16; l++)
								_memset(&screen[y + l][x], 1, 8);
						}
					}
				}
				y += 16 - (y % 16);
			}
		}
		total += line;
	}
	
	// copy to pc screen
	if(*zoom) {
		for(int y = 0, dy = 0; y < 400 && dy < 400; y++) {
			uint8* src = screen[y];
			
			for(int x = 0, dx = 0; x < 640 && dx < 640; x++) {
				uint16 col = palette_pc[src[x] & 0xf];
				for(int zx = 0; zx < *zoom + 1; zx++) {
					if(dx >= 640)
						break;
					tmp[dx++] = col;
				}
			}
			// copy line
			for(int zy = 1; zy < *zoom + 1; zy++) {
				if(dy >= 400)
					break;
				uint16* dest = emu->screen_buffer(dy++);
				_memcpy(dest, tmp, sizeof(uint16) * 640);
			}
		}
	}
	else {
		for(int y = 0; y < 400; y++) {
			uint16* dest = emu->screen_buffer(y);
			uint8* src = screen[y];
			
			for(int x = 0; x < 640; x++)
				dest[x] = palette_pc[src[x] & 0xf];
		}
	}
}

