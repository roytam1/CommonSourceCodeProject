/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.08

	[ uPD7220 - draw ]
*/

#include "upd7220.h"

void UPD7220::draw_vectl()
{
	// init pattern mask
	pattern = ra[8] | (ra[9] << 8);
	
	if(dc == 0)
		pset(dx, dy);
	else {
		int x = dx, y = dy;
		
		switch(dir)
		{
			case 0:
				for(int i = 0; i <= dc; i++) {
					int step = (int)((((d1 * i) / dc) + 1) >> 1);
					pset(x + step, y++);
				}
				break;
			case 1:
				for(int i = 0; i <= dc; i++) {
					int step = (int)((((d1 * i) / dc) + 1) >> 1);
					pset(x++, y + step);
				}
				break;
			case 2:
				for(int i = 0; i <= dc; i++) {
					int step = (int)((((d1 * i) / dc) + 1) >> 1);
					pset(x++, y - step);
				}
				break;
			case 3:
				for(int i = 0; i <= dc; i++) {
					int step = (int)((((d1 * i) / dc) + 1) >> 1);
					pset(x + step, y--);
				}
				break;
			case 4:
				for(int i = 0; i <= dc; i++) {
					int step = (int)((((d1 * i) / dc) + 1) >> 1);
					pset(x - step, y--);
				}
				break;
			case 5:
				for(int i = 0; i <= dc; i++) {
					int step = (int)((((d1 * i) / dc) + 1) >> 1);
					pset(x--, y - step);
				}
				break;
			case 6:
				for(int i = 0; i <= dc; i++) {
					int step = (int)((((d1 * i) / dc) + 1) >> 1);
					pset(x--, y + step);
				}
				break;
			case 7:
				for(int i = 0; i <= dc; i++) {
					int step = (int)((((d1 * i) / dc) + 1) >> 1);
					pset(x - step, y++);
				}
				break;
		}
	}
}

void UPD7220::draw_vectt()
{
	// init pattern mask
	pattern = 0xffff;
	uint16 draw = ra[8] | (ra[9] << 8);
	if(sl) // reverse
		draw = (draw & 0x0001 ? 0x8000 : 0) | (draw & 0x0002 ? 0x4000 : 0) | 
		       (draw & 0x0004 ? 0x2000 : 0) | (draw & 0x0008 ? 0x1000 : 0) | 
		       (draw & 0x0010 ? 0x0800 : 0) | (draw & 0x0020 ? 0x0400 : 0) | 
		       (draw & 0x0040 ? 0x0200 : 0) | (draw & 0x0080 ? 0x0100 : 0) | 
		       (draw & 0x0100 ? 0x0080 : 0) | (draw & 0x0200 ? 0x0040 : 0) | 
		       (draw & 0x0400 ? 0x0020 : 0) | (draw & 0x0800 ? 0x0010 : 0) | 
		       (draw & 0x1000 ? 0x0008 : 0) | (draw & 0x2000 ? 0x0004 : 0) | 
		       (draw & 0x8000 ? 0x0002 : 0) | (draw & 0x8000 ? 0x0001 : 0);
	
	int vx1 = vectdir[dir][0], vy1 = vectdir[dir][1];
	int vx2 = vectdir[dir][2], vy2 = vectdir[dir][3];
	int muly = zw + 1;
	
	while(muly--) {
		int cx = dx, cy = dy;
		int xrem = d;//((d - 1) & 0x3fff) + 1;
		while(xrem--) {
			int mulx = zw + 1;
			if(draw & 1) {
				draw >>= 1;
				draw |= 0x8000;
				while(mulx--) {
					pset(cx, cy);
					cx += vx1;
					cy += vy1;
				}
			}
			else {
				draw >>= 1;
				while(mulx--) {
					cx += vx1;
					cy += vy1;
				}
			}
		}
		dx += vx2;
		dy += vy2;
	}
}

void UPD7220::draw_vectc()
{
	// init pattern mask
	pattern = ra[8] | (ra[9] << 8);
	
	int x = dx, y = dy;
	int m = (d * 10000 + 14141) / 14142;
	int t = dc > m ? m : dc;
	
	if(m == 0)
		pset(x, y);
	else {
		switch(dir)
		{
			case 0:
				for(int i = dm; i <= t; i++) {
					int s = (rt[(i << RT_TABLEBIT) / m] * d);
					s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
					pset((x + s), (y + i));
				}
				break;
			case 1:
				for(int i = dm; i <= t; i++) {
					int s = (rt[(i << RT_TABLEBIT) / m] * d);
					s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
					pset((x + i), (y + s));
				}
				break;
			case 2:
				for(int i = dm; i <= t; i++) {
					int s = (rt[(i << RT_TABLEBIT) / m] * d);
					s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
					pset((x + i), (y - s));
				}
				break;
			case 3:
				for(int i = dm; i <= t; i++) {
					int s = (rt[(i << RT_TABLEBIT) / m] * d);
					s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
					pset((x + s), (y - i));
				}
				break;
			case 4:
				for(int i = dm; i <= t; i++) {
					int s = (rt[(i << RT_TABLEBIT) / m] * d);
					s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
					pset((x - s), (y - i));
				}
				break;
			case 5:
				for(int i = dm; i <= t; i++) {
					int s = (rt[(i << RT_TABLEBIT) / m] * d);
					s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
					pset((x - i), (y - s));
				}
				break;
			case 6:
				for(int i = dm; i <= t; i++) {
					int s = (rt[(i << RT_TABLEBIT) / m] * d);
					s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
					pset((x - i), (y + s));
				}
				break;
			case 7:
				for(int i = dm; i <= t; i++) {
					int s = (rt[(i << RT_TABLEBIT) / m] * d);
					s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
					pset((x - s), (y + i));
				}
				break;
		}
	}
}

void UPD7220::draw_vectr()
{
	// init pattern mask
	pattern = ra[8] | (ra[9] << 8);
	
	int vx1 = vectdir[dir][0], vy1 = vectdir[dir][1];
	int vx2 = vectdir[dir][2], vy2 = vectdir[dir][3];
	int x = dx, y = dy;
	
	for(int i = 0; i < d; i++) {
		pset(x, y);
		x += vx1;
		y += vy1;
	}
	for(int i = 0; i < d2; i++) {
		pset(x, y);
		x += vx2;
		y += vy2;
	}
	for(int i = 0; i < d; i++) {
		pset(x, y);
		x -= vx1;
		y -= vy1;
	}
	for(int i = 0; i < d2; i++) {
		pset(x, y);
		x -= vx2;
		y -= vy2;
	}
}

void UPD7220::draw_text()
{
	// init pattern mask
	pattern = 0xffff;
	
	int dir2 = dir + (sl ? 8 : 0);
	int vx1 = vectdir[dir2][0], vy1 = vectdir[dir2][1];
	int vx2 = vectdir[dir2][2], vy2 = vectdir[dir2][3];
	int sx = ((d - 1) & 0x3fff) + 1, sy = dc + 1;
	int index = 15;
	
	while(sy--) {
		int muly = zw + 1;
		while(muly--) {
			int cx = dx, cy = dy;
			uint8 bit = ra[index];
			int xrem = sx;
			
			while(xrem--) {
				int mulx = zw + 1;
				if(bit & 1) {
					bit >>= 1;
					bit |= 0x80;
					while(mulx--) {
						pset(cx, cy);
						cx += vx1;
						cy += vy1;
					}
				}
				else {
					bit >>= 1;
					while(mulx--) {
						cx += vx1;
						cy += vy1;
					}
				}
			}
			dx += vx2;
			dy += vy2;
		}
		index = ((index - 1) & 7) | 8;
	}
}

void UPD7220::pset(int x, int y)
{
	uint16 dot = pattern & 1;
	pattern = (pattern >> 1) | (dot << 15);
	int addr = y * 80 + (x >> 3);
	
	if(!(y < 0 || x < 0 || 640 <= x || VRAM_SIZE <= addr)) {
		uint8 data = 0x80 >> (x & 7);
		if(dot)
			vram[addr] |= data;
//		else
//			vram[addr] &= ~data;
	}
}

