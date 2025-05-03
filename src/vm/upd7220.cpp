/*
	Skelton for retropc emulator

	Origin : Neko Project 2
	Author : Takeda.Toshiya
	Date   : 2006.12.06 -

	[ uPD7220 ]
*/

#include <math.h>
#include "upd7220.h"
#include "../fifo.h"

void UPD7220::initialize()
{
	fi = new FIFO(16);
	fo = new FIFO(0x10000);	// read command
	ft = new FIFO(16);
	
	cmdreg = -1; // no command
	statreg = 0;
	sync[6] = 0x90; sync[7] = 0x01;
	zoom = zr = zw = 0;
	ra[0] = ra[1] = ra[2] = 0; ra[3] = 0x19;
	cs[0] = cs[1] = cs[2] = 0;
	ead = dad = 0;
	pitch = 40;	// 640dot
	maskl = maskh = 0xff;
	mod = 0;
	vsync = hblank = start = false;
	// default (QC-10)
	vs = LINES_PER_FRAME * 16 / 421;
	hc = (int)(CPU_CLOCKS * 29 / FRAMES_PER_SEC / LINES_PER_FRAME / 109 + 0.5);
	
	for(int i = 0; i <= RT_TABLEMAX; i++)
		rt[i] = (int)((double)(1 << RT_MULBIT) * (1 - sqrt(1 - pow((0.70710678118654 * i) / RT_TABLEMAX, 2))));
	
	vm->regist_vline_event(this);
}

void UPD7220::release()
{
	delete fi;
	delete fo;
	delete ft;
}

void UPD7220::write_dma8(uint32 addr, uint32 data)
{
	// for dma access
	switch(cmdreg & 0x18)
	{
	case 0x00:
		// low and high
		if(low_high) {
			cmd_write_sub(ead * 2 + 1, data & maskh);
			ead += dif;
		}
		else
			cmd_write_sub(ead * 2 + 0, data & maskl);
		low_high = !low_high;
		break;
	case 0x10:
		// low byte
		cmd_write_sub(ead * 2 + 0, data & maskl);
		ead += dif;
		break;
	case 0x18:
		// high byte
		cmd_write_sub(ead * 2 + 1, data & maskh);
		ead += dif;
		break;
	}
}

uint32 UPD7220::read_dma8(uint32 addr)
{
	uint32 val = 0xff;
	
	// for dma access
	switch(cmdreg & 0x18)
	{
	case 0x00:
		// low and high
		if(low_high) {
			val = cmd_read_sub(ead * 2 + 1);
			ead += dif;
		}
		else
			val = cmd_read_sub(ead * 2 + 0);
		low_high = !low_high;
		break;
	case 0x10:
		// low byte
		val =  cmd_read_sub(ead * 2 + 0);
		ead += dif;
		break;
	case 0x18:
		// high byte
		val =  cmd_read_sub(ead * 2 + 1);
		ead += dif;
		break;
	}
	return val;
}

void UPD7220::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 3)
	{
	case 0:
		// set parameter
//		emu->out_debug("\tPARAM = %2x\n", data);
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
//		emu->out_debug("CMDREG = %2x\n", cmdreg);
		ft->clear();	// for vectw
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
		val |= hblank ? STAT_HBLANK : 0;
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

void UPD7220::event_vline(int v, int clock)
{
	bool next = (v < vs);
	if(vsync != next) {
		for(int i = 0; i < dcount_vsync; i++)
			d_vsync[i]->write_signal(did_vsync[i], next ? 0xffffffff : 0, dmask_vsync[i]);
		vsync = next;
	}
	hblank = true;
	int id;
	vm->regist_event_by_clock(this, 0, hc, false, &id);
}

void UPD7220::event_callback(int event_id, int err)
{
	hblank = false;
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
	case CMD_WRITE + 0x00: case CMD_WRITE + 0x01: case CMD_WRITE + 0x02: case CMD_WRITE + 0x03:
	case CMD_WRITE + 0x08: case CMD_WRITE + 0x09: case CMD_WRITE + 0x0a: case CMD_WRITE + 0x0b:
	case CMD_WRITE + 0x10: case CMD_WRITE + 0x11: case CMD_WRITE + 0x12: case CMD_WRITE + 0x13:
	case CMD_WRITE + 0x18: case CMD_WRITE + 0x19: case CMD_WRITE + 0x1a: case CMD_WRITE + 0x1b:
		// no params ?
		mod = cmdreg & 3;
		break;
	}
}

void UPD7220::cmd_reset()
{
	// init gdc params
	sync[6] = 0x90; sync[7] = 0x01;
	zoom = zr = zw = 0;
	ra[0] = ra[1] = ra[2] = 0; ra[3] = 0x19;
	cs[0] = cs[1] = cs[2] = 0;
	ead = dad = 0;
	maskl = maskh = 0xff;
	
	// init fifo
	fi->clear();
	fo->clear();
	ft->clear();
	
	// stop display and drawing
	start = false;
	statreg &= ~STAT_DRAW;
	
	statreg = 0;
	cmdreg = -1;
}

void UPD7220::cmd_sync()
{
	start = ((cmdreg & 1) != 0);
	for(int i = 0; i < 8 && !fi->empty(); i++)
		sync[i] = fi->read();
	cmdreg = -1;
	
	// calc vsync/hblank timing
	int v1 = sync[2] >> 5;		// VS
	v1 += (sync[3] & 3) << 3;
	int v2 = sync[5] & 0x3f;	// VFP
	v2 += sync[6];			// AL
	v2 += (sync[7] & 3) << 8;
	v2 += sync[7] >> 2;		// VBP
	vs = (int)(LINES_PER_FRAME * v1 / (v1 + v2) + 0.5);
	
	int h1 = sync[1] + 2;		// AW
	int h2 = (sync[2] & 0x1f) + 1;	// HS
	h2 += (sync[3] >> 2) + 1;	// HFP
	h2 += (sync[4] & 0x3f) + 1;	// HBP
	hc = (int)(CPU_CLOCKS * h2 / FRAMES_PER_SEC / LINES_PER_FRAME / (h1 + h2) + 0.5);
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
	start = ((cmdreg & 1) != 0);
	cmdreg = -1;
}

void UPD7220::cmd_zoom()
{
	uint8 tmp = fi->read();
	zr = tmp >> 4;
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
#ifdef UPD7220_FIXED_PITCH
	fi->read();
#else
	pitch = fi->read();
#endif
	cmdreg = -1;
}

void UPD7220::cmd_lpen()
{
	fo->write(lad & 0xff);
	fo->write((lad >> 8) & 0xff);
	fo->write((lad >> 16) & 0x3);
	cmdreg = -1;
}

#define UPDATE_VECT() { \
	dir = vect[0] & 7; \
	dif = vectdir[dir][0] + vectdir[dir][1] * pitch; \
	sl = vect[0] & 0x80; \
	dc = (vect[1] | (vect[ 2] << 8)) & 0x3fff; \
	d  = (vect[3] | (vect[ 4] << 8)) & 0x3fff; \
	d2 = (vect[5] | (vect[ 6] << 8)) & 0x3fff; \
	d1 = (vect[7] | (vect[ 8] << 8)) & 0x3fff; \
	dm = (vect[9] | (vect[10] << 8)) & 0x3fff; \
}

void UPD7220::cmd_vectw()
{
	for(int i = 0; i < 11 && !ft->empty(); i++) {
		vect[i] = ft->read();
//		emu->out_debug("\tVECT[%d] = %2x\n", i, vect[i]);
	}
	UPDATE_VECT();
	cmdreg = -1;
}

void UPD7220::cmd_vecte()
{
	dx = ((ead %  pitch) << 4) | (dad & 0xf);
	dy = ead / pitch;
	
	// execute command
	if(!(vect[0] & 0x78)) {
		pattern = ra[8] | (ra[9] << 8);
		pset(dx, dy);
	}
	if(vect[0] & 0x08)
		draw_vectl();
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
	dx = ((ead % pitch) << 4) | (dad & 0xf);
	dy = ead / pitch;
	
	// execute command
	if(!(vect[0] & 0x78)) {
		pattern = ra[8] | (ra[9] << 8);
		pset(dx, dy);
	}
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
//	emu->out_debug("\tCSRW: X=%d,Y=%d,DOT=%d\n", ead % pitch, (int)(ead / pitch), dad);
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
	int l, h;
	mod = cmdreg & 3;
	
	switch(cmdreg & 0x18)
	{
	case 0x00:
		// low and high
		l = fi->read() & maskl;
		h = fi->read() & maskh;
		for(int i = 0; i < dc + 1; i++) {
			cmd_write_sub(ead * 2 + 0, l);
			cmd_write_sub(ead * 2 + 1, h);
			ead += dif;
		}
		break;
	case 0x10:
		// low byte
		l = fi->read() & maskl;
		for(int i = 0; i < dc + 1; i++) {
			cmd_write_sub(ead * 2 + 0, l);
			ead += dif;
		}
		break;
	case 0x18:
		// high byte
		h = fi->read() & maskh;
		for(int i = 0; i < dc + 1; i++) {
			cmd_write_sub(ead * 2 + 1, h);
			ead += dif;
		}
		break;
	default:
		// invalid
		cmdreg = -1;
		break;
	}
	// ???
	vectreset();
	cmdreg = -1;
}

void UPD7220::cmd_read()
{
	mod = cmdreg & 3;
	
	switch(cmdreg & 0x18)
	{
	case 0x00:
		// low and high
		for(int i = 0; i < dc; i++) {
			fo->write(cmd_read_sub(ead * 2 + 0));
			fo->write(cmd_read_sub(ead * 2 + 1));
			ead += dif;
		}
		break;
	case 0x10:
		// low byte
		for(int i = 0; i < dc; i++) {
			fo->write(cmd_read_sub(ead * 2 + 0));
			ead += dif;
		}
		break;
	case 0x18:
		// high byte
		for(int i = 0; i < dc; i++) {
			fo->write(cmd_read_sub(ead * 2 + 1));
			ead += dif;
		}
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
	mod = cmdreg & 3;
	low_high = false;
	for(int i = 0; i < dcount_drq; i++)
		d_drq[i]->write_signal(did_drq[i], 0xffffffff, dmask_drq[i]);
	vectreset();
//	statreg |= STAT_DMA;
	cmdreg = -1;
}

void UPD7220::cmd_dmar()
{
	mod = cmdreg & 3;
	low_high = false;
	for(int i = 0; i < dcount_drq; i++)
		d_drq[i]->write_signal(did_drq[i], 0xffffffff, dmask_drq[i]);
	vectreset();
//	statreg |= STAT_DMA;
	cmdreg = -1;
}

void UPD7220::cmd_write_sub(uint32 addr, uint8 data)
{
	if(vram) {
		switch(mod)
		{
		case 0:
			// replace
			vram[addr & ADDR_MASK] = data;
			break;
		case 1:
			// complement
			vram[addr & ADDR_MASK] ^= data;
			break;
		case 2:
			// reset
			vram[addr & ADDR_MASK] &= ~data;
			break;
		case 3:
			// set
			vram[addr & ADDR_MASK] |= data;
			break;
		}
	}
}

uint8 UPD7220::cmd_read_sub(uint32 addr)
{
	if(vram)
		return vram[addr & ADDR_MASK];
	return 0xff;
}

void UPD7220::vectreset()
{
	vect[ 1] = 0x00;
	vect[ 2] = 0x00;
	vect[ 3] = 0x08;
	vect[ 4] = 0x00;
	vect[ 5] = 0x08;
	vect[ 6] = 0x00;
	vect[ 7] = 0x00;//0xff;
	vect[ 8] = 0x00;//0xff;
	vect[ 9] = 0x00;//0xff;
	vect[10] = 0x00;//0xff;
	UPDATE_VECT();
}

// draw

void UPD7220::draw_vectl()
{
	pattern = ra[8] | (ra[9] << 8);
	
	if(dc) {
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
	else
		pset(dx, dy);
}

void UPD7220::draw_vectt()
{
	uint16 draw = ra[8] | (ra[9] << 8);
	if(sl) {
		// reverse
		draw = (draw & 0x0001 ? 0x8000 : 0) | (draw & 0x0002 ? 0x4000 : 0) | 
		       (draw & 0x0004 ? 0x2000 : 0) | (draw & 0x0008 ? 0x1000 : 0) | 
		       (draw & 0x0010 ? 0x0800 : 0) | (draw & 0x0020 ? 0x0400 : 0) | 
		       (draw & 0x0040 ? 0x0200 : 0) | (draw & 0x0080 ? 0x0100 : 0) | 
		       (draw & 0x0100 ? 0x0080 : 0) | (draw & 0x0200 ? 0x0040 : 0) | 
		       (draw & 0x0400 ? 0x0020 : 0) | (draw & 0x0800 ? 0x0010 : 0) | 
		       (draw & 0x1000 ? 0x0008 : 0) | (draw & 0x2000 ? 0x0004 : 0) | 
		       (draw & 0x8000 ? 0x0002 : 0) | (draw & 0x8000 ? 0x0001 : 0);
	}
	int vx1 = vectdir[dir][0], vy1 = vectdir[dir][1];
	int vx2 = vectdir[dir][2], vy2 = vectdir[dir][3];
	int muly = zw + 1;
	pattern = 0xffff;
	
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
	ead = (dx >> 4) + dy * pitch;
	dad = dx & 0xf;
}

void UPD7220::draw_vectc()
{
	int m = (d * 10000 + 14141) / 14142;
	int t = dc > m ? m : dc;
	pattern = ra[8] | (ra[9] << 8);
	
	if(m) {
		switch(dir)
		{
		case 0:
			for(int i = dm; i <= t; i++) {
				int s = (rt[(i << RT_TABLEBIT) / m] * d);
				s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
				pset((dx + s), (dy + i));
			}
			break;
		case 1:
			for(int i = dm; i <= t; i++) {
				int s = (rt[(i << RT_TABLEBIT) / m] * d);
				s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
				pset((dx + i), (dy + s));
			}
			break;
		case 2:
			for(int i = dm; i <= t; i++) {
				int s = (rt[(i << RT_TABLEBIT) / m] * d);
				s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
				pset((dx + i), (dy - s));
			}
			break;
		case 3:
			for(int i = dm; i <= t; i++) {
				int s = (rt[(i << RT_TABLEBIT) / m] * d);
				s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
				pset((dx + s), (dy - i));
			}
			break;
		case 4:
			for(int i = dm; i <= t; i++) {
				int s = (rt[(i << RT_TABLEBIT) / m] * d);
				s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
				pset((dx - s), (dy - i));
			}
			break;
		case 5:
			for(int i = dm; i <= t; i++) {
				int s = (rt[(i << RT_TABLEBIT) / m] * d);
				s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
				pset((dx - i), (dy - s));
			}
			break;
		case 6:
			for(int i = dm; i <= t; i++) {
				int s = (rt[(i << RT_TABLEBIT) / m] * d);
				s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
				pset((dx - i), (dy + s));
			}
			break;
		case 7:
			for(int i = dm; i <= t; i++) {
				int s = (rt[(i << RT_TABLEBIT) / m] * d);
				s = (s + (1 << (RT_MULBIT - 1))) >> RT_MULBIT;
				pset((dx - s), (dy + i));
			}
			break;
		}
	}
	else
		pset(dx, dy);
}

void UPD7220::draw_vectr()
{
	int vx1 = vectdir[dir][0], vy1 = vectdir[dir][1];
	int vx2 = vectdir[dir][2], vy2 = vectdir[dir][3];
	pattern = ra[8] | (ra[9] << 8);
	
	for(int i = 0; i < d; i++) {
		pset(dx, dy);
		dx += vx1;
		dy += vy1;
	}
	for(int i = 0; i < d2; i++) {
		pset(dx, dy);
		dx += vx2;
		dy += vy2;
	}
	for(int i = 0; i < d; i++) {
		pset(dx, dy);
		dx -= vx1;
		dy -= vy1;
	}
	for(int i = 0; i < d2; i++) {
		pset(dx, dy);
		dx -= vx2;
		dy -= vy2;
	}
	ead = (dx >> 4) + dy * pitch;
	dad = dx & 0xf;
}

void UPD7220::draw_text()
{
	int dir2 = dir + (sl ? 8 : 0);
	int vx1 = vectdir[dir2][0], vy1 = vectdir[dir2][1];
	int vx2 = vectdir[dir2][2], vy2 = vectdir[dir2][3];
	int sx = d, sy = dc + 1;
#ifdef _QC10
	if(dir == 0 && sy == 40) sy = 640;	// patch
#endif
//	emu->out_debug("\tTEXT: dx=%d,dy=%d,sx=%d,sy=%d\n", dx, dy, sx, sy);
	int index = 15;
	
	while(sy--) {
		int muly = zw + 1;
		while(muly--) {
			int cx = dx, cy = dy;
			uint8 bit = ra[index];
			int xrem = sx;
			while(xrem--) {
				pattern = (bit & 1) ? 0xffff : 0;
				bit = (bit >> 1) | ((bit & 1) ? 0x80 : 0);
				int mulx = zw + 1;
				while(mulx--) {
					pset(cx, cy);
					cx += vx1;
					cy += vy1;
				}
			}
			dx += vx2;
			dy += vy2;
		}
		index = ((index - 1) & 7) | 8;
	}
	ead = (dx >> 4) + dy * pitch;
	dad = dx & 0xf;
}

void UPD7220::pset(int x, int y)
{
	if(vram) {
		uint16 dot = pattern & 1;
		pattern = (pattern >> 1) | (dot << 15);
		uint32 addr = (y * 80 + (x >> 3)) & ADDR_MASK;
		uint8 bit = 1 << (x & 7);
		uint8 cur = vram[addr];
		
		switch(mod)
		{
		case 0:
			// replace
			vram[addr] = (cur & ~bit) | (dot ? bit : 0);
			break;
		case 1:
			// complement
			vram[addr] = (cur & ~bit) | ((cur ^ (dot ? 0xff : 0)) & bit);
			break;
		case 2:
			// reset
			vram[addr] &= dot ? ~bit : 0xff;
			break;
		case 3:
			// set
			vram[addr] |= dot ? bit : 0;
			break;
		}
	}
}

