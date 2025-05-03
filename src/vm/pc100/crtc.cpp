/*
	NEC PC-100 Emulator 'ePC-100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.07.14 -

	[ crtc ]
*/

#include "crtc.h"

void CRTC::initialize()
{
	// init pallete
	for(int i = 1; i < 16; i++) {
		palette[i] = 0x1ff;
		update_palette(i);
	}
	palette[0] = 0;
	update_palette(0);
	
	// regist events
	vm->regist_vsync_event(this);
}

void CRTC::event_vsync(int v, int clock)
{
	if(v == 512)
		d_pic->write_signal(did_pic, 1, 1);
}

void CRTC::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0x38:
		sel = data;
		break;
	case 0x3a:
		regs[sel & 7] = data;
		break;
	case 0x3c:
		vs = (vs & 0xff00) | data;
		break;
	case 0x3e:
		vs = (vs & 0xff) | (data << 8);
		break;
	case 0x40:
	case 0x42:
	case 0x44:
	case 0x46:
	case 0x48:
	case 0x4a:
	case 0x4c:
	case 0x4e:
	case 0x50:
	case 0x52:
	case 0x54:
	case 0x56:
	case 0x58:
	case 0x5a:
	case 0x5c:
	case 0x5e:
		palette[(addr >> 1) & 0xf] = (palette[(addr >> 1) & 0xf] & 0xff00) | data;
		update_palette((addr >> 1) & 0xf);
		break;
	case 0x41:
	case 0x43:
	case 0x45:
	case 0x47:
	case 0x49:
	case 0x4b:
	case 0x4d:
	case 0x4f:
	case 0x51:
	case 0x53:
	case 0x55:
	case 0x57:
	case 0x59:
	case 0x5b:
	case 0x5d:
	case 0x5f:
		palette[(addr >> 1) & 0xf] = (palette[(addr >> 1) & 0xf] & 0xff) | (data << 8);
		update_palette((addr >> 1) & 0xf);
		break;
	case 0x60:
		cmd = (cmd & 0xff00) | data;
		break;
	case 0x61:
		cmd = (cmd & 0xff) | (data << 8);
		break;
	}
}

uint32 CRTC::read_io8(uint32 addr)
{
	uint32 val = 0xff;
	
	switch(addr & 0x3ff)
	{
	case 0x38:
		return sel;
	case 0x3a:
		return regs[sel & 7];
	case 0x3c:
		return vs & 0xff;
	case 0x3e:
		return vs >> 8;
	case 0x40:
	case 0x42:
	case 0x44:
	case 0x46:
	case 0x48:
	case 0x4a:
	case 0x4c:
	case 0x4e:
	case 0x50:
	case 0x52:
	case 0x54:
	case 0x56:
	case 0x58:
	case 0x5a:
	case 0x5c:
	case 0x5e:
		return palette[(addr >> 1) & 0xf] & 0xff;
	case 0x41:
	case 0x43:
	case 0x45:
	case 0x47:
	case 0x49:
	case 0x4b:
	case 0x4d:
	case 0x4f:
	case 0x51:
	case 0x53:
	case 0x55:
	case 0x57:
	case 0x59:
	case 0x5b:
	case 0x5d:
	case 0x5f:
		return palette[(addr >> 1) & 0xf] >> 8;
	case 0x60:
		return cmd & 0xff;
	case 0x61:
		return cmd >> 8;
	}
	return 0xff;
}

void CRTC::draw_screen()
{
	// display region
	int hd = (regs[2] >> 1) & 0x3f;
	int vd = (regs[6] >> 1) & 0x3f;
	int hs = (int)(int8)((regs[0] & 0x40) ? (regs[0] | 0x80) : (regs[0] & 0x3f));
//	int hs = (int)(int8)regs[0];
	int vs_tmp = (int)(int16)((vs & 0x400) ? (vs | 0xf800) : (vs & 0x3ff));
	int sa = (hs + hd + 1) * 2 + (vs_tmp + vd) * 0x80;
//	int sa = (hs + hd + 1) * 2 + ((vs & 0x3ff) + vd) * 0x80;
	
	if(cmd != 0xffff) {
		// mono
		uint16 col = RGB_COLOR(31, 31, 31);
		for(int y = 0; y < 512; y++) {
			int ptr = sa & 0x1ffff;
			sa += 0x80;
			uint16 *dest = emu->screen_buffer(y);
			
			for(int x = 0; x < 720; x += 8) {
				uint8 pat = vram0[ptr++];
				ptr &= 0x1ffff;
				
				dest[x + 0] = pat & 0x01 ? col : 0;
				dest[x + 1] = pat & 0x02 ? col : 0;
				dest[x + 2] = pat & 0x04 ? col : 0;
				dest[x + 3] = pat & 0x08 ? col : 0;
				dest[x + 4] = pat & 0x10 ? col : 0;
				dest[x + 5] = pat & 0x20 ? col : 0;
				dest[x + 6] = pat & 0x40 ? col : 0;
				dest[x + 7] = pat & 0x80 ? col : 0;
			}
		}
	}
	else {
		// color
		for(int y = 0; y < 512; y++) {
			int ptr = sa & 0x1ffff;
			sa += 0x80;
			uint16 *dest = emu->screen_buffer(y);
			
			for(int x = 0; x < 720; x += 8) {
				uint8 p0 = vram0[ptr];
				uint8 p1 = vram1[ptr];
				uint8 p2 = vram2[ptr];
				uint8 p3 = vram3[ptr++];
				ptr &= 0x1ffff;
				
				dest[x + 0] = palette_pc[((p0 & 0x01) << 0) | ((p1 & 0x01) << 1) | ((p2 & 0x01) << 2) | ((p3 & 0x01) << 3)];
				dest[x + 1] = palette_pc[((p0 & 0x02) >> 1) | ((p1 & 0x02) << 0) | ((p2 & 0x02) << 1) | ((p3 & 0x02) << 2)];
				dest[x + 2] = palette_pc[((p0 & 0x04) >> 2) | ((p1 & 0x04) >> 1) | ((p2 & 0x04) << 0) | ((p3 & 0x04) << 1)];
				dest[x + 3] = palette_pc[((p0 & 0x08) >> 3) | ((p1 & 0x08) >> 2) | ((p2 & 0x08) >> 1) | ((p3 & 0x08) << 0)];
				dest[x + 4] = palette_pc[((p0 & 0x10) >> 4) | ((p1 & 0x10) >> 3) | ((p2 & 0x10) >> 2) | ((p3 & 0x10) >> 1)];
				dest[x + 5] = palette_pc[((p0 & 0x20) >> 5) | ((p1 & 0x20) >> 4) | ((p2 & 0x20) >> 3) | ((p3 & 0x20) >> 2)];
				dest[x + 6] = palette_pc[((p0 & 0x40) >> 6) | ((p1 & 0x40) >> 5) | ((p2 & 0x40) >> 4) | ((p3 & 0x40) >> 3)];
				dest[x + 7] = palette_pc[((p0 & 0x80) >> 7) | ((p1 & 0x80) >> 6) | ((p2 & 0x80) >> 5) | ((p3 & 0x80) >> 4)];
			}
		}
	}
	
	// access lamp
	uint32 stat_f = d_fdc->read_signal(0);
	if(stat_f) {
		uint16 col = (stat_f & (1 | 4)) ? RGB_COLOR(31, 0, 0) :
		             (stat_f & (2 | 8)) ? RGB_COLOR(0, 31, 0) : 0;
		for(int y = 512 - 8; y < 512; y++) {
			uint16 *dest = emu->screen_buffer(y);
			for(int x = 720 - 8; x < 720; x++)
				dest[x] = col;
		}
	}
}

void CRTC::update_palette(int num)
{
	int r = (palette[num] >> 0) & 7; r = r ? (r << 2) | 3 : 0;
	int g = (palette[num] >> 3) & 7; g = g ? (g << 2) | 3 : 0;
	int b = (palette[num] >> 6) & 7; b = b ? (b << 2) | 3 : 0;
	palette_pc[num] = RGB_COLOR(r, g, b);
}

