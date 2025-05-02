/*
	Skelton for retropc emulator

	Origin : Neko Project 2
	Author : Takeda.Toshiya
	Date   : 2006.12.06 -

	[ uPD7220 ]
*/

#include <math.h>
#include "upd7220.h"
#include "fifo.h"

void UPD7220::initialize()
{
	fi = new FIFO(16);
	fo = new FIFO(0x10000);	// read command
	ft = new FIFO(16);
	
	cmdreg = -1; // no command
	statreg = 0;
	hsync = vsync = start = false;
	blink = 0;
	ra[0] = ra[1] = ra[2] = 0; ra[3] = 0x19;
	cs[0] = cs[1] = cs[2] = 0;
	sync[6] = 0x90; sync[7] = 0x01;
	zr = zw = zoom = 0;
	ead = dad = 0;
	maskl = maskh = 0xff;
	
	for(int i = 0; i <= RT_TABLEMAX; i++)
		rt[i] = (int)((double)(1 << RT_MULBIT) * (1 - sqrt(1 - pow((0.70710678118654 * i) / RT_TABLEMAX, 2))));
	
	vm->regist_vsync_event(this);
	vm->regist_hsync_event(this);
}

void UPD7220::release()
{
	delete fi;
	delete fo;
	delete ft;
}

void UPD7220::write_data8(uint32 addr, uint32 data)
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

uint32 UPD7220::read_data8(uint32 addr)
{
	uint32 val = 0xff;
	
	// for dma access
	switch(cmdreg & 0x1f)
	{
	case 0x00:
		// low and high
		if(low_high)
			val = cmd_read_sub((ead++) * 2 + 1);
		else
			val = cmd_read_sub(ead * 2 + 0);
		low_high = !low_high;
		return val;
	case 0x10:
		// low byte
		return cmd_read_sub((ead++) * 2 + 0);
	case 0x11:
		// high byte
		return cmd_read_sub((ead++) * 2 + 1);
	}
	return 0xff;
}

void UPD7220::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 3)
	{
	case 0:
		// set parameter
		if(cmdreg != -1) {
			fi->write(data);
			check_cmd();
		}
		break;
	case 1:
		// process prev command if not finished
		if(cmdreg != -1)
			process_cmd();
		// set new command
		cmdreg = data;
		ft->init();	// for vectw
		check_cmd();
		break;
	case 2:
		// set zoom
		zoom = data;
		break;
	case 3:
		// light pen request
		break;
	}
}

uint32 UPD7220::read_io8(uint32 addr)
{
	uint32 val;
	
	switch(addr & 3)
	{
	case 0:
		// status
		val = statreg;
		val |= hsync ? STAT_HBLANK : 0;
		val |= vsync ? STAT_VSYNC : 0;
		val |= fi->empty() ? STAT_EMPTY : 0;
		val |= fi->full() ? STAT_FULL : 0;
		val |= fo->count() ? STAT_DRDY : 0;
		// clear busy stat
		statreg &= ~(STAT_DMA | STAT_DRAW);
		return val;
	case 1:
		// data
		if(fo->count())
			return fo->read();
		return 0xff;
	}
	return 0xff;
}

void UPD7220::void event_vsync(int v, int clock)
{
	vsync = (v < 400) ? false : true;
}

void UPD7220::void event_hsync(int v, int h, int clock)
{
	hsync = (h < 80) ? false : true;
}

// command process

void UPD7220::check_cmd()
{
	// check fifo buffer and process command if enough params in fifo
	switch(cmdreg)
	{
	case CMD_RESET:
		cmd_reset();
		break;
	case CMD_SYNC + 0x0: case CMD_SYNC + 0x1:
		if(fi->count() >= 8)
			cmd_sync();
		break;
	case CMD_MASTER:
		cmd_master();
		break;
	case CMD_SLAVE:
		cmd_slave();
		break;
	case CMD_START:
		cmd_start();
		break;
	case CMD_BCTRL + 0x0: case CMD_BCTRL + 0x1:
		cmd_bctrl();
		break;
	case CMD_ZOOM:
		if(fi->count())
			cmd_zoom();
		break;
	case CMD_SCROLL + 0x0: case CMD_SCROLL + 0x1: case CMD_SCROLL + 0x2: case CMD_SCROLL + 0x3:
	case CMD_SCROLL + 0x4: case CMD_SCROLL + 0x5: case CMD_SCROLL + 0x6: case CMD_SCROLL + 0x7:
//	case CMD_SCROLL + 0x8: case CMD_SCROLL + 0x9: case CMD_SCROLL + 0xa: case CMD_SCROLL + 0xb:
//	case CMD_SCROLL + 0xc: case CMD_SCROLL + 0xd: case CMD_SCROLL + 0xe: case CMD_SCROLL + 0xf:
	case CMD_TEXTW + 0x0: case CMD_TEXTW + 0x1: case CMD_TEXTW + 0x2: case CMD_TEXTW + 0x3:
	case CMD_TEXTW + 0x4: case CMD_TEXTW + 0x5: case CMD_TEXTW + 0x6: case CMD_TEXTW + 0x7:
		if(fi->count())
			cmd_scroll();
		break;
	case CMD_CSRFORM:
		if(fi->count() >= 3)
			cmd_csrform();
		break;
	case CMD_PITCH:
		if(fi->count())
			cmd_pitch();
		break;
	case CMD_LPEN:
		cmd_lpen();
		break;
	case CMD_VECTW:
		while(fi->count())
			ft->write(fi->read());
		if(ft->count() >= 11)
			cmd_vectw();
		break;
	case CMD_VECTE:
		cmd_vecte();
		break;
	case CMD_TEXTE:
		cmd_texte();
		break;
	case CMD_CSRW:
		while(fi->count())
			ft->write(fi->read());
		if(ft->count() >= 3)
			cmd_csrw();
		break;
	case CMD_CSRR:
		cmd_csrr();
		break;
	case CMD_MASK:
		if(fi->count() >= 2)
			cmd_mask();
		break;
	case CMD_WRITE + 0x00: case CMD_WRITE + 0x01: case CMD_WRITE + 0x02: case CMD_WRITE + 0x03:
	case CMD_WRITE + 0x08: case CMD_WRITE + 0x09: case CMD_WRITE + 0x0a: case CMD_WRITE + 0x0b:
	case CMD_WRITE + 0x10: case CMD_WRITE + 0x11: case CMD_WRITE + 0x12: case CMD_WRITE + 0x13:
	case CMD_WRITE + 0x18: case CMD_WRITE + 0x19: case CMD_WRITE + 0x1a: case CMD_WRITE + 0x1b:
	{
		uint8 lh = cmdreg & 0x18;
		if(lh == 0x00 && fi->count() >= 2)
			cmd_write();
		else if(lh == 0x08)
			cmdreg = -1;
		else if((lh == 0x10 || lh == 0x18) && fi->count())
			cmd_write();
		break;
	}
	case CMD_READ + 0x00: case CMD_READ + 0x01: case CMD_READ + 0x02: case CMD_READ + 0x03:
	case CMD_READ + 0x08: case CMD_READ + 0x09: case CMD_READ + 0x0a: case CMD_READ + 0x0b:
	case CMD_READ + 0x10: case CMD_READ + 0x11: case CMD_READ + 0x12: case CMD_READ + 0x13:
	case CMD_READ + 0x18: case CMD_READ + 0x19: case CMD_READ + 0x1a: case CMD_READ + 0x1b:
		cmd_read();
		break;
	case CMD_DMAW + 0x00: case CMD_DMAW + 0x01: case CMD_DMAW + 0x02: case CMD_DMAW + 0x03:
	case CMD_DMAW + 0x08: case CMD_DMAW + 0x09: case CMD_DMAW + 0x0a: case CMD_DMAW + 0x0b:
	case CMD_DMAW + 0x10: case CMD_DMAW + 0x11: case CMD_DMAW + 0x12: case CMD_DMAW + 0x13:
	case CMD_DMAW + 0x18: case CMD_DMAW + 0x19: case CMD_DMAW + 0x1a: case CMD_DMAW + 0x1b:
		cmd_dmaw();
		break;
	case CMD_DMAR + 0x00: case CMD_DMAR + 0x01: case CMD_DMAR + 0x02: case CMD_DMAR + 0x03:
	case CMD_DMAR + 0x08: case CMD_DMAR + 0x09: case CMD_DMAR + 0x0a: case CMD_DMAR + 0x0b:
	case CMD_DMAR + 0x10: case CMD_DMAR + 0x11: case CMD_DMAR + 0x12: case CMD_DMAR + 0x13:
	case CMD_DMAR + 0x18: case CMD_DMAR + 0x19: case CMD_DMAR + 0x1a: case CMD_DMAR + 0x1b:
		cmd_dmar();
		break;
	}
}

void UPD7220::process_cmd()
{
	switch(cmdreg)
	{
	case CMD_RESET:
		cmd_reset();
		break;
	case CMD_SYNC + 0x0: case CMD_SYNC + 0x1:
		cmd_sync();
		break;
	case CMD_SCROLL + 0x0: case CMD_SCROLL + 0x1: case CMD_SCROLL + 0x2: case CMD_SCROLL + 0x3:
	case CMD_SCROLL + 0x4: case CMD_SCROLL + 0x5: case CMD_SCROLL + 0x6: case CMD_SCROLL + 0x7:
//	case CMD_SCROLL + 0x8: case CMD_SCROLL + 0x9: case CMD_SCROLL + 0xa: case CMD_SCROLL + 0xb:
//	case CMD_SCROLL + 0xc: case CMD_SCROLL + 0xd: case CMD_SCROLL + 0xe: case CMD_SCROLL + 0xf:
	case CMD_TEXTW + 0x0: case CMD_TEXTW + 0x1: case CMD_TEXTW + 0x2: case CMD_TEXTW + 0x3:
	case CMD_TEXTW + 0x4: case CMD_TEXTW + 0x5: case CMD_TEXTW + 0x6: case CMD_TEXTW + 0x7:
		if(fi->count())
			cmd_scroll();
		break;
	case CMD_VECTW:
		cmd_vectw();
		break;
	case CMD_CSRW:
		cmd_csrw();
		break;
	}
}

void UPD7220::cmd_reset()
{
	// init gdc params
	ra[0] = ra[1] = ra[2] = 0; ra[3] = 0x19;
	cs[0] = cs[1] = cs[2] = 0;
	sync[6] = 0x90; sync[7] = 0x01;
	
	zr = zw = zoom = 0;
	ead = dad = 0;
	maskl = maskh = 0xff;
	
	// init fifo
	fi->init();
	fo->init();
	ft->init();
	
	// stop display and drawing
	start = false;
	statreg &= ~STAT_DRAW;
	
	statreg = 0;
	cmdreg = -1;
}

void UPD7220::cmd_sync()
{
	start = (cmdreg & 1) ? true : false;
	for(int i = 0; i < 8; i++)
		sync[i] = fi->read();
	cmdreg = -1;
}

void UPD7220::cmd_master()
{
	cmdreg = -1;
}

void UPD7220::cmd_slave()
{
	cmdreg = -1;
}

void UPD7220::cmd_start()
{
	start = true;
	cmdreg = -1;
}

void UPD7220::cmd_bctrl()
{
	start = (cmdreg & 1) ? true : false;
	cmdreg = -1;
}

void UPD7220::cmd_zoom()
{
	uint8 tmp = fi->read();
	zr = (tmp & 0xf0) >> 4;
	zw = tmp & 0xf;
	cmdreg = -1;
}

void UPD7220::cmd_scroll()
{
	ra[cmdreg & 0xf] = fi->read();
	cmdreg = (cmdreg == 0x7f) ? -1 : cmdreg + 1;
}

void UPD7220::cmd_csrform()
{
	cs[0] = fi->read();
	cs[1] = fi->read();
	cs[2] = fi->read();
	cmdreg = -1;
}

void UPD7220::cmd_pitch()
{
	pitch = fi->read();
	cmdreg = -1;
}

void UPD7220::cmd_lpen()
{
	fo->write(lad & 0xff);
	fo->write((lad >> 8) & 0xff);
	fo->write((lad >> 16) & 0x3);
	cmdreg = -1;
}

void UPD7220::cmd_vectw()
{
	for(int i = 0; i < 11; i++)
		vect[i] = ft->read();
	cmdreg = -1;
}

void UPD7220::cmd_vecte()
{
	// get param
	dx = ((ead %  40) << 4) | (dad & 0xf);
	dy = ead / 40;
	dir = vect[0] & 7;
	sl = vect[0] & 0x80;
	dc = (vect[1] | (vect[ 2] << 8)) & 0x3fff;
	d  = (vect[3] | (vect[ 4] << 8)) & 0x3fff;
	d2 = (vect[5] | (vect[ 6] << 8)) & 0x3fff;
	d1 = (vect[7] | (vect[ 8] << 8)) & 0x3fff;
	dm = (vect[9] | (vect[10] << 8)) & 0x3fff;
	
	// execute command
//	if(vect[0] & 0x08)
//		draw_vectl();
	if(vect[0] & 0x10)
		draw_vectt();
	if(vect[0] & 0x20)
		draw_vectc();
	if(vect[0] & 0x40)
		draw_vectr();
	vectreset();
	
	statreg |= STAT_DRAW;
	cmdreg = -1;
}

void UPD7220::cmd_texte()
{
	dx = ((ead %  40) << 4) | (dad & 0xf);
	dy = ead / 40;
	dir = vect[0] & 7;
	sl = vect[0] & 0x80;
	dc = (vect[1] | (vect[ 2] << 8)) & 0x3fff;
	d  = (vect[3] | (vect[ 4] << 8)) & 0x3fff;
	d2 = (vect[5] | (vect[ 6] << 8)) & 0x3fff;
	d1 = (vect[7] | (vect[ 8] << 8)) & 0x3fff;
	dm = (vect[9] | (vect[10] << 8)) & 0x3fff;
	
	// execute command
	if(vect[0] & 0x08)
		draw_vectl();
	if(vect[0] & 0x10)
		draw_text();
	if(vect[0] & 0x20)
		draw_vectc();
	if(vect[0] & 0x40)
		draw_vectr();
	vectreset();
	
	statreg |= STAT_DRAW;
	cmdreg = -1;
}

void UPD7220::cmd_csrw()
{
	ead = ft->read();
	ead |= ft->read() << 8;
	ead |= ft->read() << 16;
	dad = (ead >> 20) & 0xf;
	ead &= 0x3ffff;
	cmdreg = -1;
}

void UPD7220::cmd_csrr()
{
	fo->write(ead & 0xff);
	fo->write((ead >> 8) & 0xff);
	fo->write((ead >> 16) & 0x3);
	fo->write(dad & 0xff);
	fo->write((dad >> 8) & 0xff);
	cmdreg = -1;
}

void UPD7220::cmd_mask()
{
	maskl = fi->read();
	maskh = fi->read();
	cmdreg = -1;
}

void UPD7220::cmd_write()
{
	int l, h, w = (vect[1] | (vect[2] << 8)) & 0x3fff;
	
	switch(cmdreg & 0x18)
	{
	case 0x00:
		// low and high
		l = fi->read() & maskl;
		h = fi->read() & maskh;
		for(int i = 0; i < w + 1; i++) {
			cmd_write_sub(ead * 2 + 0, l);
			cmd_write_sub((ead++) * 2 + 1, h);
		}
		break;
	case 0x10:
		// low byte
		l = fi->read() & maskl;
		for(int i = 0; i < w + 1; i++)
			cmd_write_sub((ead++) * 2 + 0, l);
		break;
	case 0x18:
		// high byte
		h = fi->read() & maskh;
		for(int i = 0; i < w + 1; i++)
			cmd_write_sub((ead++) * 2 + 1, h);
		break;
	default:
		// invalid
		cmdreg = -1;
		break;
	}
	vectreset();
}

void UPD7220::cmd_read()
{
	int w = (vect[1] | (vect[2] << 8)) & 0x3fff;
	
	switch(cmdreg & 0x18)
	{
	case 0x00:
		// low and high
		for(int i = 0; i < w; i++) {
			fo->write(cmd_read_sub(ead * 2 + 0));
			fo->write(cmd_read_sub((ead++) * 2 + 1));
		}
		break;
	case 0x10:
		// low byte
		for(int i = 0; i < w; i++)
			fo->write(cmd_read_sub((ead++) * 2 + 0));
		break;
	case 0x18:
		// high byte
		for(int i = 0; i < w; i++)
			fo->write(cmd_read_sub((ead++) * 2 + 1));
		break;
	default:
		// invalid
		break;
	}
	vectreset();
	cmdreg = -1;
}

void UPD7220::cmd_dmaw()
{
	low_high = false;
	if(dev)
		dev->write_signal(dev_id, 0xffffffff, 1);
	vectreset();
	
//	statreg |= STAT_DMA;
	cmdreg = -1;
}

void UPD7220::cmd_dmar()
{
	low_high = false;
	if(dev)
		dev->write_signal(dev_id, 0xffffffff, 1);
	vectreset();
	
//	statreg |= STAT_DMA;
	cmdreg = -1;
}

void UPD7220::cmd_write_sub(uint32 addr, uint8 data)
{
	switch(cmdreg & 0x3)
	{
	case 0x0:
		// replace
		vram[addr & ADDR_MASK] = data;
		break;
	case 0x1:
		// complement
		vram[addr & ADDR_MASK] ^= data;
		break;
	case 0x2:
		// reset
		vram[addr & ADDR_MASK] &= ~data;
		break;
	case 0x3:
		// set
		vram[addr & ADDR_MASK] |= data;
		break;
	}
}

void UPD7220::vectreset()
{
	vect[ 1] = 0x00;
	vect[ 2] = 0x00;
	vect[ 3] = 0x08;
	vect[ 4] = 0x00;
	vect[ 5] = 0x08;
	vect[ 6] = 0x00;
	vect[ 7] = 0xff;
	vect[ 8] = 0xff;
	vect[ 9] = 0xff;
	vect[10] = 0xff;
}

// draw

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
	
	if(!(y < 0 || x < 0 || 640 <= x || vram_size <= addr)) {
		uint8 data = 0x80 >> (x & 7);
		if(dot)
			vram[addr] |= data;
//		else
//			vram[addr] &= ~data;
	}
}

