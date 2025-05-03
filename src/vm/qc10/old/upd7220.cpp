/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.08

	[ uPD7220 ]
*/

#include <math.h>
#include "upd7220.h"
#include "fifo.h"
#include "../fileio.h"

void UPD7220::initialize()
{
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
#if 0
	for(int i = 0; i < 8; i++)
		palette_pc[i] = RGB_COLOR((i & 2) ? 0x1f : 0, (i & 4) ? 0x1f : 0, (i & 1) ? 0x1f : 0);
#else
	// green monitor
	for(int i = 1; i < 8; i++) {
		palette_pc[i + 0] = RGB_COLOR(0, 0x14, 0);
		palette_pc[i + 8] = RGB_COLOR(0, 0x1f, 0);
	}
	palette_pc[0] = palette_pc[8] = 0;
#endif
	
	// init regs
	fi = new FIFO(16);
	fo = new FIFO(0x10000);	// read command
	ft = new FIFO(16);
	
	cmdreg = -1; // no command
	statreg = 0;
	
	// init params
	hsync = vsync = start = false;
	blink = 0;
	
	ra[0] = ra[1] = ra[2] = 0; ra[3] = 0x19;
	cs[0] = cs[1] = cs[2] = 0;
	sync[6] = 0x90; sync[7] = 0x01;
	
	zr = zw = zoom = 0;
	ead = dad = 0;
	maskl = maskh = 0xff;
	
	// init vectc
	for(int i = 0; i <= RT_TABLEMAX; i++)
		rt[i] = (int)((double)(1 << RT_MULBIT) * (1 - sqrt(1 - pow((0.70710678118654 * i) / RT_TABLEMAX, 2))));
}

void UPD7220::release()
{
	if(fi)
		delete fi;
	if(fo)
		delete fo;
	if(ft)
		delete ft;
}

void UPD7220::write_data8(uint16 addr, uint8 data)
{
	// for dma access
	switch(cmdreg & 0x1f)
	{
		case 0x00:
			// low and high
			if(low_high)
				cmd_write_sub((ead++) * 2 + 1, data & maskh);
			else
				cmd_write_sub(ead * 2 + 0, data & maskl);
			low_high = !low_high;
			break;
		case 0x10:
			// low byte
			cmd_write_sub((ead++) * 2 + 0, data & maskl);
			break;
		case 0x11:
			// high byte
			cmd_write_sub((ead++) * 2 + 1, data & maskh);
			break;
	}
}

uint8 UPD7220::read_data8(uint16 addr)
{
	// for dma access
	switch(cmdreg & 0x1f)
	{
		case 0x00:
		{
			// low and high
			uint8 val = 0xff;
			if(low_high)
				val = cmd_read_sub((ead++) * 2 + 1);
			else
				val = cmd_read_sub(ead * 2 + 0);
			low_high = !low_high;
			return val;
		}
		case 0x10:
			// low byte
			return cmd_read_sub((ead++) * 2 + 0);
		case 0x11:
			// high byte
			return cmd_read_sub((ead++) * 2 + 1);
	}
	return 0xff;
}

void UPD7220::write_io8(uint16 addr, uint8 data)
{
	switch(addr & 0xff)
	{
		case 0x38:
			// set parameter
			if(cmdreg != -1) {
				fi->write(data);
				check_cmd();
			}
			break;
		case 0x39:
			// process prev command if not finished
			if(cmdreg != -1)
				process_cmd();
			// set new command
			cmdreg = data;
			ft->init();	// for vectw
			check_cmd();
			break;
		case 0x3a:
			// set zoom
			zoom = data;
			break;
		case 0x3b:
			// light pen request
			break;
	}
}

uint8 UPD7220::read_io8(uint16 addr)
{
	switch(addr & 0xff)
	{
		case 0x2c:
			// display type
			return 0xfe; // monochrome
			//return 0x02; // color
			
		case 0x38:
		{
			// status
			uint8 stat = statreg;
			stat |= hsync ? STAT_HBLANK : 0;
			stat |= vsync ? STAT_VSYNC : 0;
			stat |= fi->empty() ? STAT_EMPTY : 0;
			stat |= fi->full() ? STAT_FULL : 0;
			stat |= fo->count() ? STAT_DRDY : 0;
			// clear busy stat
			statreg &= ~(STAT_DMA | STAT_DRAW);
			return stat;
		}
		case 0x39:
			// data
			if(fo->count())
				return fo->read();
			return 0xff;
	}
	return 0xff;
}

void UPD7220::set_hsync(int h)
{
	hsync = (h < 80) ? false : true;
}

void UPD7220::set_vsync(int v)
{
	vsync = (v < 400) ? false : true;
}

void UPD7220::set_blink()
{
	blink = (blink + 1) & 0x1f;
}

void UPD7220::draw_screen()
{
	uint8 screen[400][640];
	int max = (sync[6] | (sync[7] << 8)) & 0x3ff;
	
	// create screen
	for(int i = 0, total = 0; i < 4 && total < max; i++) {
		int ptr = (ra[4 * i + 0] | (ra[4 * i + 1] << 8) | (ra[4 * i + 2] << 16)) & 0x3ffff;
		if(MODE_CHR)
			ptr &= 0x1fff;
		ptr <<= 1;
		int caddr = ead << 1;
		
		int line = ((ra[4 * i + 2] >> 4) | (ra[4 * i + 3] << 4)) & 0x3ff;
//		bool gfx = MODE_GFX ? true : MODE_CHR ? false : (ra[4 * i + 3] & 0x40) ? true : false;
		bool gfx = (ra[4 * i + 3] & 0x40) ? true : false;
		bool wide =(ra[4 * i + 3] & 0x80) ? true : false;
		
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
						bool cursor = ((cs[0] & 0x80) && ((cs[1] & 0x20) || !(blink & 0x10)) && ptr == caddr) ? true : false;
						uint8 code = vram[ptr++];
						uint8 attrib = vram[ptr++];
						uint8* pattern = &font[code * 16];
						
						for(int l = y % 16; l < 16 && (y + l) < 400; l++) {
							uint8 pat = pattern[l];
							// attribute
							if((attrib & 0x40) || ((attrib & 0x80) && (blink & 0x10)))
								pat = 0;
							if(attrib & 0x8)
								pat = ~pat;
							uint8 col = (attrib & 0x4) ? 9 : 1;
							
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
						bool cursor = ((cs[0] & 0x80) && ((cs[1] & 0x20) || !(blink & 0x10)) && ptr == caddr) ? true : false;
						uint8 code = vram[ptr++];
						uint8 attrib = vram[ptr++];
						uint8* pattern = &font[code * 16];
						
						for(int l = y % 16; l < 16 && (y + l) < 400; l++) {
							uint8 pat = pattern[l];
							// attribute
							if((attrib & 0x40) || ((attrib & 0x80) && (blink & 0x10)))
								pat = 0;
							if(attrib & 0x8)
								pat = ~pat;
							uint8 col = (attrib & 0x4) ? 9 : 1;
							
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
	if(zoom) {
		for(int y = 0, dy = 0; y < 400 && dy < 400; y++) {
			uint16* dest = emu->screen_buffer(dy++);
			uint8* src = screen[y];
			
			for(int x = 0, dx = 0; x < 640 && dx < 640; x++) {
				uint16 col = palette_pc[src[x] & 0xf];
				for(int zx = 0; zx < zoom + 1; zx++) {
					if(dx >= 640)
						break;
					dest[dx++] = col;
				}
			}
			// copy line
			for(int zy = 1; zy < zoom + 1; zy++) {
				if(dy >= 400)
					break;
				uint16* copy = emu->screen_buffer(dy++);
				_memcpy(copy, dest, sizeof(uint16) * 640);
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

