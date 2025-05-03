/*
	Skelton for retropc emulator

	Origin : Ootake (joypad, timer)
	       : xpce (psg, vdc)
	Author : Takeda.Toshiya
	Date   : 2009.03.11-

	[ PC-Eninge ]
*/

#include <math.h>
#include "pce.h"

#define INT_IRQ2	0x01
#define INT_IRQ1	0x02
#define INT_TIRQ	0x04
#define INT_NMI		0x08

#define VDC_MAWR	0
#define VDC_MARR	1
#define VDC_VWR		2
#define VDC_VRR		2
#define VDC_CR		5
#define VDC_RCR		6
#define VDC_BXR		7
#define VDC_BYR		8
#define VDC_MWR		9
#define VDC_HSR		10
#define VDC_HDR		11
#define VDC_VPR		12
#define VDC_VDW		13
#define VDC_VCR		14
#define VDC_DCR		15
#define VDC_SOUR	16
#define VDC_DISTR	17
#define VDC_LENR	18
#define VDC_SATB	19

#define VDC_STAT_CR	0x01
#define VDC_STAT_OR	0x02
#define VDC_STAT_RR	0x04
#define VDC_STAT_DS	0x08
#define VDC_STAT_DV	0x10
#define VDC_STAT_VD	0x20
#define VDC_STAT_BSY	0x40

//#define VDC_SpHitON	(vdc[VDC_CR].w & 0x01)
//#define VDC_OverON	(vdc[VDC_CR].w & 0x02)
#define VDC_RasHitON	(vdc[VDC_CR].w & 0x04)
#define VDC_VBlankON	(vdc[VDC_CR].w & 0x08)
#define VDC_SpriteON	(vdc[VDC_CR].w & 0x40)
#define VDC_ScreenON	(vdc[VDC_CR].w & 0x80)
#define VDC_SATBIntON	(vdc[VDC_DCR].w & 1)
//#define VDC_DMAIntON	(vdc[VDC_DCR].w & 2)

void PCE::initialize()
{
	// get context
	joy_stat = emu->joy_buffer();
	key_stat = emu->key_buffer();
	
	// regist event
	vm->pce_regist_vline_event(this);
	int id;
	vm->pce_regist_event_by_clock(this, 0, 1024, true, &id);
}

void PCE::reset()
{
	// reset memory bus
	_memset(ram, 0, sizeof(ram));
	bank = 0x80000;
	buffer = 0xff;	// ???
	
	// reset devices
	vdc_reset();
	psg_reset();
	timer_reset();
	joy_reset();
	
	// reset screen
	_memset(screen, 0, sizeof(screen));
}

void PCE::event_callback(int event_id, int err)
{
	// update timer
	if(timer_const) {
		if(--timer_count <= 0) {
			if(timer_run)
				int_request(INT_TIRQ);
			timer_count = timer_const;
		}
	}
}

void PCE::event_vline(int v, int clock)
{
	int dispmin = (vdc_maxline - vdc_minline > 227) ? vdc_minline + ((vdc_maxline - vdc_minline - 227 + 1) >> 1) : vdc_minline;
	int dispmax = (vdc_maxline - vdc_minline > 227) ? vdc_maxline - ((vdc_maxline - vdc_minline - 227 + 1) >> 1) : vdc_maxline;
	vdc_scanline = v;
	
	// init screen
	if(!vdc_scanline) {
		for(int y = vdc_minline; y < vdc_maxline; y++) {
			for(int x = 8; x < PCE_WIDTH + 8; x++)
				screen[y][x] = palette_bg[0];
		}
		_memset(mask_spr[vdc_minline], 0, (vdc_maxline - vdc_minline) * PCE_WIDTH);
	}
	
	// update status
	bool intr = false;
	vdc_status &= ~VDC_STAT_RR;
	if(vdc_scanline > vdc_maxline)
		vdc_status |= VDC_STAT_VD;
	if(vdc_scanline == vdc_minline) {
		vdc_status &= ~VDC_STAT_VD;
		prv_scanline = dispmin;
		vdc_scroll_ydiff = prv_scroll_ydiff = 0;
	}
	else if(vdc_scanline == vdc_maxline) {
		// check sprites
		bool hit = false;
		sprtype *spr = (sprtype*)spram;
		int x0 = spr->x;
		int y0 = spr->y;
		int w0 = (((spr->atr >>  8) & 1) + 1) * 16;
		int h0 = (((spr->atr >> 12) & 3) + 1) * 16;
		spr++;
		for(int i = 1; i < 64; i++, spr++) {
			int x = spr->x;
			int y = spr->y;
			int w = (((spr->atr >>  8) & 1) + 1) * 16;
			int h = (((spr->atr >> 12) & 3) + 1) * 16;
			if((x < x0 + w0) && (x + w > x0) && (y < y0 + h0) && (y + h > y0)) {
				hit = true;
				break;
			}
		}
		if(hit)
			vdc_status |= VDC_STAT_CR;
		else
			vdc_status &= ~VDC_STAT_CR;
		if(prv_scanline < dispmax) {
			if(VDC_SpriteON)
				vdc_refresh_sprite(prv_scanline, dispmax, 0);
			vdc_refresh_line(prv_scanline, dispmax);
			if(VDC_SpriteON)
				vdc_refresh_sprite(prv_scanline, dispmax, 1);
		}
		prv_scanline = dispmax + 1;
		// store display range
		vdc_dispwidth = vdc_screen_w;
		vdc_dispmin = dispmin;
		vdc_dispmax = dispmax;
		// refresh sprite mask
//		_memset(mask_spr, 0, sizeof(mask_spr));
	}
	if(vdc_minline <= vdc_scanline && vdc_scanline <= vdc_maxline) {
		if(vdc_scanline == (vdc[VDC_RCR].w & 0x3ff) - 64) {
			if(VDC_RasHitON && dispmin <= vdc_scanline && vdc_scanline <= dispmax) {
				if(prv_scanline < dispmax) {
					if(VDC_SpriteON)
						vdc_refresh_sprite(prv_scanline, vdc_scanline, 0);
					vdc_refresh_line(prv_scanline, vdc_scanline);
					if(VDC_SpriteON)
						vdc_refresh_sprite(prv_scanline, vdc_scanline, 1);
				}
				prv_scanline = vdc_scanline;
			}
			vdc_status |= VDC_STAT_RR;
			if(VDC_RasHitON)
				intr = true;
		}
		else if(vdc_scroll) {
			if(vdc_scanline - 1 > prv_scanline) {
				int tmpx = vdc[VDC_BXR].w;
				int tmpy = vdc[VDC_BYR].w;
				int tmpd = vdc_scroll_ydiff;
				vdc[VDC_BXR].w = prv_scroll_x;
				vdc[VDC_BYR].w = prv_scroll_y;
				vdc_scroll_ydiff = prv_scroll_ydiff;
				if(VDC_SpriteON)
					vdc_refresh_sprite(prv_scanline, vdc_scanline - 1, 0);
				vdc_refresh_line(prv_scanline, vdc_scanline - 1);
				if(VDC_SpriteON)
					vdc_refresh_sprite(prv_scanline, vdc_scanline - 1, 1);
				prv_scanline = vdc_scanline - 1;
				vdc[VDC_BXR].w = tmpx;
				vdc[VDC_BYR].w = tmpy;
				vdc_scroll_ydiff = tmpd;
			}
		}
	}
	else {
		int rcr = (vdc[VDC_RCR].w & 0x3ff) - 64;
		if(vdc_scanline == rcr) {
			if(VDC_RasHitON) {
				vdc_status |= VDC_STAT_RR;
				intr = true;
			}
		}
	}
	vdc_scroll = false;
	if(vdc_scanline == vdc_maxline + 1) {
		// VRAM to SATB DMA
		if(vdc_satb || (vdc[VDC_DCR].w & 0x10)) {
			_memcpy(spram, &vram[vdc[VDC_SATB].w * 2], 512);
			vdc_satb = true;
			vdc_status &= ~VDC_STAT_DS;
		}
		if(intr)
			vdc_pendvsync = true;
		else if(VDC_VBlankON)
			intr = true;
	}
	else if(vdc_scanline == min(vdc_maxline + 5, PCE_LINES_PER_FRAME - 1)) {
		if(vdc_satb) {
			vdc_status |= VDC_STAT_DS;
			vdc_satb = false;
			if(VDC_SATBIntON)
				intr = true;
		}
	}
	else if(vdc_pendvsync && !intr) {
		vdc_pendvsync = false;
		if(VDC_VBlankON)
			intr = true;
	}
	if(intr)
		int_request(INT_IRQ1);
//	else
//		int_cancel(INT_IRQ1);
}

void PCE::write_data8(uint32 addr, uint32 data)
{
	uint8 mpr = (addr >> 13) & 0xff;
	uint16 ofs = addr & 0x1fff;
	
	switch(mpr)
	{
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
		// populous
		cart[addr & 0xfffff] = data;
		return;
	case 0xf7:
//		if(ofs < 0x800)
//			backup[ofs] = data;
		return;
	case 0xf8:
		ram[ofs] = data;
		return;
	case 0xf9:
	case 0xfa:
	case 0xfb:
//		ram[addr & 0x7fff] = data;
		return;
	case 0xff:
		switch(addr & 0x1c00)
		{
		case 0x0000:	// vdc
			vdc_write(ofs, data);
			break;
		case 0x0400:	// vce
			vce_write(ofs, data);
			break;
		case 0x0800:	// psg
			buffer = data;
			psg_write(ofs, data);
			break;
		case 0x0c00:	// timer
			buffer = data;
			timer_write(ofs, data);
			break;
		case 0x1000:	// joypad
			buffer = data;
			joy_write(ofs, data);
			break;
		case 0x1400:	// interrupt control
			buffer = data;
			int_write(ofs, data);
			break;
		}
		return;
	}
	// bank switch for sf2d
	if((addr & 0x1ffc) == 0x1ff0)
		bank = 0x80000 * ((addr & 3) + 1);
}

uint32 PCE::read_data8(uint32 addr)
{
	uint8 mpr = (addr >> 13) & 0xff;
	uint16 ofs = addr & 0x1fff;
	
	if(mpr <= 0x3f)
		return cart[addr & 0x7ffff];
	if(mpr <= 0x7f)
		return cart[bank | (addr & 0x7ffff)];
	switch(mpr)
	{
//	case 0xf7:
//		if(ofs < 0x800)
//			return backup[ofs];
//		return 0xff;
	case 0xf8:
		return ram[ofs];
//	case 0xf9:
//	case 0xfa:
//	case 0xfb:
//		return ram[addr & 0x7fff];
	case 0xff:
		switch (addr & 0x1c00)
		{
		case 0x0000: // vdc
			return vdc_read(ofs);
		case 0x0400: // vce
			return vce_read(ofs);
		case 0x0800: // psg
			return psg_read(ofs);
//			return buffer;
		case 0x0c00: // timer
			buffer = (buffer & 0x80) | timer_read(ofs);
			return buffer;
		case 0x1000: // joypad
			if(joy_nibble)
				joy_count = (joy_count + 1) & 15;
			buffer = joy_read(ofs);
			return buffer;
		case 0x1400: // interrupt control
			buffer = (buffer & 0xf8) | int_read(ofs);
			return buffer;
		}
		return 0xff;
	}
	return 0xff;
}

void PCE::draw_screen()
{
	int dx = (SCREEN_WIDTH - vdc_dispwidth) / 2;
	int dy = (SCREEN_HEIGHT - (vdc_dispmax - vdc_dispmin)) / 2;
	
	for(int y = vdc_dispmin; y < vdc_dispmax; y++, dy++) {
		scrntype* src = &screen[y][8];
		scrntype* dst = emu->screen_buffer(dy) + dx;
		for(int x = 0; x < vdc_dispwidth; x++)
			dst[x] = src[x];
	}
}

void PCE::open_cart(_TCHAR* filename)
{
	FILEIO* fio = new FILEIO();
	
	if(fio->Fopen(filename, FILEIO_READ_BINARY)) {
		_memset(cart, 0xff, sizeof(cart));
		fio->Fseek(0, FILEIO_SEEK_END);
		int head = fio->Ftell() % 1024;
		fio->Fseek(head, FILEIO_SEEK_SET);
		fio->Fread(cart, sizeof(cart), 1);
		fio->Fclose();
		vm->pce_running = true;
	}
	delete fio;
}

void PCE::close_cart()
{
	_memset(cart, 0xff, sizeof(cart));
	vm->pce_running = false;
}

// vdc

void PCE::vdc_reset()
{
	_memset(&vdc, 0, sizeof(vdc));
	_memset(&vce, 0, sizeof(vce));
	_memset(&vce_reg, 0, sizeof(vce_reg));
	_memset(vchange, 1, sizeof(vchange));
	_memset(vchanges, 1, sizeof(vchanges));
	
	vdc_inc = 1;
	vdc_raster_count = 0;
	vdc_ch = vdc_status = vdc_ratch = vce_ratch = 0;
	vdc_satb = vdc_pendvsync = false;
	vdc_bg_w = vdc_bg_h = 0;
	vdc_screen_w = vdc_screen_h = 0;
	vdc_minline = 0;
	vdc_maxline = 255;
	vdc_scanline = 0;
	vdc_scroll = false;
	vdc_spbg = false;
}

void PCE::vdc_write(uint16 addr, uint8 data)
{
	uint8 bgw[] = {32, 64, 128, 128};
	uint8 incsize[] = {1, 32, 64, 128};
	
	switch(addr & 3)
	{
	case 0:
		vdc_ch = data & 31;
		break;
	case 1:
		break;
	case 2:
		switch(vdc_ch)
		{
		case VDC_VWR:
			vdc_ratch = data;
			return;
		case VDC_HDR:
			vdc_screen_w = (data + 1) * 8;
			break;
		case VDC_MWR:
			vdc_bg_h = (data & 0x40) ? 64 : 32;
			vdc_bg_w = bgw[(data >> 4) & 3];
			break;
		case VDC_BYR:
			if(!vdc_scroll) {
				prv_scroll_x = vdc[VDC_BXR].w;
				prv_scroll_y = vdc[VDC_BYR].w;
				prv_scroll_ydiff = vdc_scroll_ydiff;
			}
			vdc[VDC_BYR].b.l = data;
			vdc_scroll = true;
			vdc_scroll_ydiff = vdc_scanline - 1;
			return;
		case VDC_BXR:
			if(!vdc_scroll) {
				prv_scroll_x = vdc[VDC_BXR].w;
				prv_scroll_y = vdc[VDC_BYR].w;
				prv_scroll_ydiff = vdc_scroll_ydiff;
			}
			vdc[VDC_BXR].b.l = data;
			vdc_scroll = true;
			return;
		}
		vdc[vdc_ch].b.l = data;
		break;
	case 3:
		switch(vdc_ch)
		{
		case VDC_VWR:
			vram[vdc[VDC_MAWR].w * 2 + 0] = vdc_ratch;
			vram[vdc[VDC_MAWR].w * 2 + 1] = data;
			vchange[vdc[VDC_MAWR].w >> 4] = 1;
			vchanges[vdc[VDC_MAWR].w >> 6] = 1;
			vdc[VDC_MAWR].w += vdc_inc;
			vdc_ratch = 0;
			return;
		case VDC_VDW:
			vdc[VDC_VDW].b.h = data;
			vdc_screen_h = (vdc[VDC_VDW].w & 0x1ff) + 1;
			vdc_maxline = vdc_screen_h - 1;
			return;
		case VDC_LENR:
			vdc[VDC_LENR].b.h = data;
			// vram to vram
			_memcpy(&vram[vdc[VDC_DISTR].w * 2], &vram[vdc[VDC_SOUR].w * 2], (vdc[VDC_LENR].w + 1) * 2);
			_memset(&vchange[vdc[VDC_DISTR].w >> 4], 1, (vdc[VDC_LENR].w + 1) >> 4);
			_memset(&vchange[vdc[VDC_DISTR].w >> 6], 1, (vdc[VDC_LENR].w + 1) >> 6);
			vdc[VDC_DISTR].w += vdc[VDC_LENR].w + 1;
			vdc[VDC_SOUR].w += vdc[VDC_LENR].w + 1;
			vdc_status |= VDC_STAT_DV;
			return;
		case VDC_CR :
			vdc_inc = incsize[(data >> 3) & 3];
			break;
		case VDC_HDR:
			break;
		case VDC_BYR:
			if(!vdc_scroll) {
				prv_scroll_x = vdc[VDC_BXR].w;
				prv_scroll_y = vdc[VDC_BYR].w;
				prv_scroll_ydiff = vdc_scroll_ydiff;
			}
			vdc[VDC_BYR].b.h = data & 1;
			vdc_scroll = true;
			vdc_scroll_ydiff = vdc_scanline - 1;
			return;
		case VDC_SATB:
			vdc[VDC_SATB].b.h = data;
			vdc_satb = true;
			vdc_status &= ~VDC_STAT_DS;
			return;
		case VDC_BXR:
			if(!vdc_scroll) {
				prv_scroll_x = vdc[VDC_BXR].w;
				prv_scroll_y = vdc[VDC_BYR].w;
				prv_scroll_ydiff = vdc_scroll_ydiff;
			}
			vdc[VDC_BXR].b.h = data & 3;
			vdc_scroll = true;
			return;
		}
		vdc[vdc_ch].b.h = data;
		break;
	}
}

uint8 PCE::vdc_read(uint16 addr)
{
	uint8 val;
	
	switch(addr & 3 )
	{
	case 0:
		val = vdc_status;
		vdc_status = 0;
		return val;
	case 1:
		return 0;
	case 2:
		if(vdc_ch == VDC_VRR)
			return vram[vdc[VDC_MARR].w * 2];
		else
			return vdc[vdc_ch].b.l;
	case 3:
		if(vdc_ch == VDC_VRR) {
			val = vram[vdc[VDC_MARR].w * 2 + 1];
			vdc[VDC_MARR].w += vdc_inc;
			return val;
		}
		else
			return vdc[vdc_ch].b.h;
	}
	return 0xff;
}

void PCE::vce_write(uint16 addr, uint8 data)
{
	switch(addr & 7)
	{
	case 2:
		vce_reg.b.l = data;
		break;
	case 3:
		vce_reg.b.h = data & 1;
		break;
	case 4:
		vce[vce_reg.w].b.l = data;
		vce_update_pal(vce_reg.w);
		break;
	case 5:
		vce[vce_reg.w].b.h = data;
		vce_update_pal(vce_reg.w);
		vce_reg.w = (vce_reg.w + 1) & 0x1ff;
		break;
	}
}

uint8 PCE::vce_read(uint16 addr)
{
	uint8 val;
	
	switch(addr & 7)
	{
	case 4:
		return vce[vce_reg.w & 0x1ff].b.l;
	case 5:
		val = vce[vce_reg.w].b.h;
		vce_reg.w = (vce_reg.w + 1) & 0x1ff;
		return val;
	}
	return 0xff;
}

void PCE::vce_update_pal(int num)
{
	scrntype* pal = (num & 0x100) ? palette_spr : palette_bg;
	uint16 g = (vce[num].w >> 6) & 7;
	uint16 r = (vce[num].w >> 3) & 7;
	uint16 b = (vce[num].w >> 0) & 7;
	scrntype col = RGB_COLOR(r << 5, g << 5, b << 5);
	
	if(!num) {
		for(int i = 0; i < 256; i += 16)
			pal[i] = col;
	}
	else if(num & 0xf)
		pal[num & 0xff] = col;
}

void PCE::vdc_refresh_line(int sy, int ey)
{
	if(VDC_ScreenON) {
		int xofs = 8 - (vdc[VDC_BXR].w & 7);
		int yy = sy + vdc[VDC_BYR].w - vdc_scroll_ydiff;
		int yofs = yy & 7;
		int h = min(ey - sy, 8 - yofs);
		yy >>= 3;
		int xw = (vdc_screen_w >> 3) + 1;
		for(int y = sy; y < ey; yy++) {
			int xx = vdc[VDC_BXR].w >> 3;
			yy &= vdc_bg_h - 1;
			for(int x = 0; x < xw; x++, xx++) {
				xx &= vdc_bg_w - 1;
				int no = ((uint16*)vram)[xx + yy * vdc_bg_w];
				scrntype* pal = &palette_bg[(no >> 8) & ~0xf];
				no &= 0xfff;
				if(vchange[no]) {
					vchange[no] = 0;
					vdc_plane2pixel(no);
				}
				uint32* pixel = &pixel_bg[no * 8 + yofs];
				uint8* src = &vram[no * 32 + yofs * 2];
				for(int i = 0; i < h; i++, pixel++, src+=2) {
					uint8 pat = src[0] | src[1] | src[16] | src[17];
					if(!pat)
						continue;
					scrntype* dst = &screen[y + i][x * 8 + xofs];
					uint32 col = pixel[0];
					if(pat & 0x80) dst[0] = pal[(col >>  4) & 0xf];
					if(pat & 0x40) dst[1] = pal[(col >> 12) & 0xf];
					if(pat & 0x20) dst[2] = pal[(col >> 20) & 0xf];
					if(pat & 0x10) dst[3] = pal[(col >> 28) & 0xf];
					if(pat & 0x08) dst[4] = pal[(col      ) & 0xf];
					if(pat & 0x04) dst[5] = pal[(col >>  8) & 0xf];
					if(pat & 0x02) dst[6] = pal[(col >> 16) & 0xf];
					if(pat & 0x01) dst[7] = pal[(col >> 24) & 0xf];
				}
			}
			y += h;
			yofs = 0;
			h = min(ey - y, 8);
		}
	}
}

void PCE::vdc_refresh_sprite(int sy, int ey, int bg)
{
	sprtype* spr = (sprtype*)spram + 63;
	if(!bg)
		vdc_spbg = false;
	for(int n = 0; n < 64; n++, spr--){
		int atr = spr->atr;
		int spbg = (atr >> 7) & 1;
		if(spbg != bg)
			continue;
		int x = (spr->x & 0x3ff) - 32;
		int y = (spr->y & 0x3ff) - 64;
		int no = spr->no & 0x7ff;
		int cgx = (atr >> 8) & 1;
		int cgy = (atr >> 12) & 3;
		cgy |= cgy >> 1;
		no = (no >> 1) & ~(cgy * 2 + cgx);
		if(y >= ey || y + (cgy + 1) * 16 < sy || x >= vdc_screen_w || x + (cgx + 1) *16 < 0)
			continue;
		
		scrntype* pal = &palette_spr[(atr & 0xf) * 16];
		for(int i = 0; i < cgy * 2 + cgx + 1; i++) {
			if(vchanges[no + i]) {
				vchanges[no + i] = 0;
				vdc_sp2pixel(no + i);
			}
			if(!cgx)
				i++;
		}
		uint8* src = &vram[no * 128];
		uint32* pixel = &pixel_spr[no * 32];
		int yy = y;
		int inc = 2;
		if(atr & 0x8000) {
			inc = -2;
			src += 15 * 2 + cgy * 256;
			pixel += 15 * 2 + cgy * 64;
		}
		int ysum = 0;
		for(int i = 0; i <= cgy; i++) {
			int t = sy - y - ysum;
			int h = 16;
			if(t > 0) {
				src += t * inc;
				pixel += t * inc;
				h -= t;
				yy += t;
			}
			if(h > ey - y - ysum)
				h = ey - y - ysum;
			if(!spbg) {
				vdc_spbg = true;
				if(atr & 0x800) {
					for(int j = 0; j <= cgx; j++)
						vdc_put_sprite_hflip_makemask(&screen[yy][x + (cgx - j) * 16 + 8], src + j * 128, pixel + j * 32, pal, h, inc, &mask_spr[yy][x + (cgx - j) * 16], n);
				}
				else {
					for(int j = 0; j <= cgx; j++)
						vdc_put_sprite_makemask(&screen[yy][x + j * 16 + 8], src + j * 128, pixel + j * 32, pal, h, inc, &mask_spr[yy][x + j * 16], n);
				}
			}
			else if(vdc_spbg) {
				if(atr & 0x800) {
					for(int j = 0; j <= cgx; j++)
						vdc_put_sprite_hflip_mask(&screen[yy][x + (cgx - j) * 16 + 8], src + j * 128, pixel + j * 32, pal, h, inc, &mask_spr[yy][x + (cgx - j) * 16], n);
				}
				else {
					for(int j = 0; j <= cgx; j++)
						vdc_put_sprite_mask(&screen[yy][x + j * 16 + 8], src + j * 128, pixel + j * 32, pal, h, inc, &mask_spr[yy][x + j * 16], n);
				}
			}
			else {
				if(atr & 0x800){
					for(int j = 0; j <= cgx; j++)
						vdc_put_sprite_hflip(&screen[yy][x + (cgx - j) * 16 + 8], src + j * 128, pixel + j * 32, pal, h, inc);
				}
				else {
					for(int j = 0; j <= cgx; j++)
						vdc_put_sprite(&screen[yy][x + j * 16 + 8], src + j * 128, pixel + j * 32, pal, h, inc);
				}
			}
			yy += h;
			src += h * inc + 16 * 7 * inc;
			pixel += h * inc + 16 * inc;
			ysum += 16;
		}
	}
}

void PCE::vdc_put_sprite(scrntype *dst, uint8 *src, uint32 *pixel, scrntype *pal, int h, int inc)
{
	for(int i = 0; i < h; i++, src += inc, pixel += inc, dst += PCE_WIDTH + 8) {
		uint16 pat = ((uint16*)src)[0] | ((uint16*)src)[16] | ((uint16*)src)[32] | ((uint16*)src)[48];
		if(!pat)
			continue;
		uint32 col = pixel[1];
		if(pat & 0x8000) dst[ 0] = pal[(col >>  4) & 0xf];
		if(pat & 0x4000) dst[ 1] = pal[(col >> 12) & 0xf];
		if(pat & 0x2000) dst[ 2] = pal[(col >> 20) & 0xf];
		if(pat & 0x1000) dst[ 3] = pal[(col >> 28) & 0xf];
		if(pat & 0x0800) dst[ 4] = pal[(col      ) & 0xf];
		if(pat & 0x0400) dst[ 5] = pal[(col >>  8) & 0xf];
		if(pat & 0x0200) dst[ 6] = pal[(col >> 16) & 0xf];
		if(pat & 0x0100) dst[ 7] = pal[(col >> 24) & 0xf];
		col = pixel[0];
		if(pat & 0x0080) dst[ 8] = pal[(col >>  4) & 0xf];
		if(pat & 0x0040) dst[ 9] = pal[(col >> 12) & 0xf];
		if(pat & 0x0020) dst[10] = pal[(col >> 20) & 0xf];
		if(pat & 0x0010) dst[11] = pal[(col >> 28) & 0xf];
		if(pat & 0x0008) dst[12] = pal[(col      ) & 0xf];
		if(pat & 0x0004) dst[13] = pal[(col >>  8) & 0xf];
		if(pat & 0x0002) dst[14] = pal[(col >> 16) & 0xf];
		if(pat & 0x0001) dst[15] = pal[(col >> 24) & 0xf];
	}
}

void PCE::vdc_put_sprite_hflip(scrntype *dst, uint8 *src, uint32 *pixel, scrntype *pal, int h, int inc)
{
	for(int i = 0; i < h; i++, src += inc, pixel += inc, dst += PCE_WIDTH + 8) {
		uint16 pat = ((uint16*)src)[0] | ((uint16*)src)[16] | ((uint16*)src)[32] | ((uint16*)src)[48];
		if(!pat)
			continue;
		uint32 col = pixel[1];
		if(pat & 0x8000) dst[15] = pal[(col >>  4) & 0xf];
		if(pat & 0x4000) dst[14] = pal[(col >> 12) & 0xf];
		if(pat & 0x2000) dst[13] = pal[(col >> 20) & 0xf];
		if(pat & 0x1000) dst[12] = pal[(col >> 28) & 0xf];
		if(pat & 0x0800) dst[11] = pal[(col      ) & 0xf];
		if(pat & 0x0400) dst[10] = pal[(col >>  8) & 0xf];
		if(pat & 0x0200) dst[ 9] = pal[(col >> 16) & 0xf];
		if(pat & 0x0100) dst[ 8] = pal[(col >> 24) & 0xf];
		col = pixel[0];
		if(pat & 0x0080) dst[ 7] = pal[(col >>  4) & 0xf];
		if(pat & 0x0040) dst[ 6] = pal[(col >> 12) & 0xf];
		if(pat & 0x0020) dst[ 5] = pal[(col >> 20) & 0xf];
		if(pat & 0x0010) dst[ 4] = pal[(col >> 28) & 0xf];
		if(pat & 0x0008) dst[ 3] = pal[(col      ) & 0xf];
		if(pat & 0x0004) dst[ 2] = pal[(col >>  8) & 0xf];
		if(pat & 0x0002) dst[ 1] = pal[(col >> 16) & 0xf];
		if(pat & 0x0001) dst[ 0] = pal[(col >> 24) & 0xf];
	}
}

void PCE::vdc_put_sprite_mask(scrntype *dst, uint8 *src, uint32 *pixel, scrntype *pal, int h, int inc, uint8 *mask, uint8 pr)
{
	for(int i = 0; i < h; i++, src += inc, pixel += inc, dst += PCE_WIDTH + 8, mask += PCE_WIDTH) {
		uint16 pat = ((uint16*)src)[0] | ((uint16*)src)[16] | ((uint16*)src)[32] | ((uint16*)src)[48];
		if(!pat)
			continue;
		uint32 col = pixel[1];
		if((pat & 0x8000) && mask[ 0] <= pr) dst[ 0] = pal[(col >>  4) & 0xf];
		if((pat & 0x4000) && mask[ 1] <= pr) dst[ 1] = pal[(col >> 12) & 0xf];
		if((pat & 0x2000) && mask[ 2] <= pr) dst[ 2] = pal[(col >> 20) & 0xf];
		if((pat & 0x1000) && mask[ 3] <= pr) dst[ 3] = pal[(col >> 28) & 0xf];
		if((pat & 0x0800) && mask[ 4] <= pr) dst[ 4] = pal[(col      ) & 0xf];
		if((pat & 0x0400) && mask[ 5] <= pr) dst[ 5] = pal[(col >>  8) & 0xf];
		if((pat & 0x0200) && mask[ 6] <= pr) dst[ 6] = pal[(col >> 16) & 0xf];
		if((pat & 0x0100) && mask[ 7] <= pr) dst[ 7] = pal[(col >> 24) & 0xf];
		col = pixel[0];
		if((pat & 0x0080) && mask[ 8] <= pr) dst[ 8] = pal[(col >>  4) & 0xf];
		if((pat & 0x0040) && mask[ 9] <= pr) dst[ 9] = pal[(col >> 12) & 0xf];
		if((pat & 0x0020) && mask[10] <= pr) dst[10] = pal[(col >> 20) & 0xf];
		if((pat & 0x0010) && mask[11] <= pr) dst[11] = pal[(col >> 28) & 0xf];
		if((pat & 0x0008) && mask[12] <= pr) dst[12] = pal[(col      ) & 0xf];
		if((pat & 0x0004) && mask[13] <= pr) dst[13] = pal[(col >>  8) & 0xf];
		if((pat & 0x0002) && mask[14] <= pr) dst[14] = pal[(col >> 16) & 0xf];
		if((pat & 0x0001) && mask[15] <= pr) dst[15] = pal[(col >> 24) & 0xf];
	}
}

void PCE::vdc_put_sprite_hflip_mask(scrntype *dst, uint8 *src, uint32 *pixel, scrntype *pal, int h, int inc, uint8 *mask, uint8 pr)
{
	for(int i = 0; i < h; i++, src += inc, pixel += inc, dst += PCE_WIDTH + 8, mask += PCE_WIDTH) {
		uint16 pat = ((uint16*)src)[0] | ((uint16*)src)[16] | ((uint16*)src)[32] | ((uint16*)src)[48];
		if(!pat)
			continue;
		uint32 col = pixel[1];
		if((pat & 0x8000) && mask[15] <= pr) dst[15] = pal[(col >>  4) & 0xf];
		if((pat & 0x4000) && mask[14] <= pr) dst[14] = pal[(col >> 12) & 0xf];
		if((pat & 0x2000) && mask[13] <= pr) dst[13] = pal[(col >> 20) & 0xf];
		if((pat & 0x1000) && mask[12] <= pr) dst[12] = pal[(col >> 28) & 0xf];
		if((pat & 0x0800) && mask[11] <= pr) dst[11] = pal[(col      ) & 0xf];
		if((pat & 0x0400) && mask[10] <= pr) dst[10] = pal[(col >>  8) & 0xf];
		if((pat & 0x0200) && mask[ 9] <= pr) dst[ 9] = pal[(col >> 16) & 0xf];
		if((pat & 0x0100) && mask[ 8] <= pr) dst[ 8] = pal[(col >> 24) & 0xf];
		col = pixel[0];
		if((pat & 0x0080) && mask[ 7] <= pr) dst[ 7] = pal[(col >>  4) & 0xf];
		if((pat & 0x0040) && mask[ 6] <= pr) dst[ 6] = pal[(col >> 12) & 0xf];
		if((pat & 0x0020) && mask[ 5] <= pr) dst[ 5] = pal[(col >> 20) & 0xf];
		if((pat & 0x0010) && mask[ 4] <= pr) dst[ 4] = pal[(col >> 28) & 0xf];
		if((pat & 0x0008) && mask[ 3] <= pr) dst[ 3] = pal[(col      ) & 0xf];
		if((pat & 0x0004) && mask[ 2] <= pr) dst[ 2] = pal[(col >>  8) & 0xf];
		if((pat & 0x0002) && mask[ 1] <= pr) dst[ 1] = pal[(col >> 16) & 0xf];
		if((pat & 0x0001) && mask[ 0] <= pr) dst[ 0] = pal[(col >> 24) & 0xf];
	}
}

void PCE::vdc_put_sprite_makemask(scrntype *dst, uint8 *src, uint32 *pixel, scrntype *pal, int h, int inc, uint8 *mask, uint8 pr)
{
	for(int i = 0; i < h; i++, src += inc, pixel += inc, dst += PCE_WIDTH + 8, mask += PCE_WIDTH) {
		uint16 pat = ((uint16*)src)[0] | ((uint16*)src)[16] | ((uint16*)src)[32] | ((uint16*)src)[48];
		if(!pat)
			continue;
		uint32 col = pixel[1];
		if(pat & 0x8000) {dst[ 0] = pal[(col >>  4) & 0xf]; mask[ 0] = pr;}
		if(pat & 0x4000) {dst[ 1] = pal[(col >> 12) & 0xf]; mask[ 1] = pr;}
		if(pat & 0x2000) {dst[ 2] = pal[(col >> 20) & 0xf]; mask[ 2] = pr;}
		if(pat & 0x1000) {dst[ 3] = pal[(col >> 28) & 0xf]; mask[ 3] = pr;}
		if(pat & 0x0800) {dst[ 4] = pal[(col      ) & 0xf]; mask[ 4] = pr;}
		if(pat & 0x0400) {dst[ 5] = pal[(col >>  8) & 0xf]; mask[ 5] = pr;}
		if(pat & 0x0200) {dst[ 6] = pal[(col >> 16) & 0xf]; mask[ 6] = pr;}
		if(pat & 0x0100) {dst[ 7] = pal[(col >> 24) & 0xf]; mask[ 7] = pr;}
		col = pixel[0];
		if(pat & 0x0080) {dst[ 8] = pal[(col >>  4) & 0xf]; mask[ 8] = pr;}
		if(pat & 0x0040) {dst[ 9] = pal[(col >> 12) & 0xf]; mask[ 9] = pr;}
		if(pat & 0x0020) {dst[10] = pal[(col >> 20) & 0xf]; mask[10] = pr;}
		if(pat & 0x0010) {dst[11] = pal[(col >> 28) & 0xf]; mask[11] = pr;}
		if(pat & 0x0008) {dst[12] = pal[(col      ) & 0xf]; mask[12] = pr;}
		if(pat & 0x0004) {dst[13] = pal[(col >>  8) & 0xf]; mask[13] = pr;}
		if(pat & 0x0002) {dst[14] = pal[(col >> 16) & 0xf]; mask[14] = pr;}
		if(pat & 0x0001) {dst[15] = pal[(col >> 24) & 0xf]; mask[15] = pr;}
	}
}

void PCE::vdc_put_sprite_hflip_makemask(scrntype *dst,uint8 *src,uint32 *pixel,scrntype *pal,int h,int inc,uint8 *mask,uint8 pr)
{
	for(int i = 0; i < h; i++, src += inc, pixel += inc, dst += PCE_WIDTH + 8, mask += PCE_WIDTH) {
		uint16 pat = ((uint16*)src)[0] | ((uint16*)src)[16] | ((uint16*)src)[32] | ((uint16*)src)[48];
		if(!pat)
			continue;
		uint32 col = pixel[1];
		if(pat & 0x8000) {dst[15] = pal[(col >>  4) & 0xf]; mask[15] = pr;}
		if(pat & 0x4000) {dst[14] = pal[(col >> 12) & 0xf]; mask[14] = pr;}
		if(pat & 0x2000) {dst[13] = pal[(col >> 20) & 0xf]; mask[13] = pr;}
		if(pat & 0x1000) {dst[12] = pal[(col >> 28) & 0xf]; mask[12] = pr;}
		if(pat & 0x0800) {dst[11] = pal[(col      ) & 0xf]; mask[11] = pr;}
		if(pat & 0x0400) {dst[10] = pal[(col >>  8) & 0xf]; mask[10] = pr;}
		if(pat & 0x0200) {dst[ 9] = pal[(col >> 16) & 0xf]; mask[ 9] = pr;}
		if(pat & 0x0100) {dst[ 8] = pal[(col >> 24) & 0xf]; mask[ 8] = pr;}
		col = pixel[0];
		if(pat & 0x0080) {dst[ 7] = pal[(col >>  4) & 0xf]; mask[ 7] = pr;}
		if(pat & 0x0040) {dst[ 6] = pal[(col >> 12) & 0xf]; mask[ 6] = pr;}
		if(pat & 0x0020) {dst[ 5] = pal[(col >> 20) & 0xf]; mask[ 5] = pr;}
		if(pat & 0x0010) {dst[ 4] = pal[(col >> 28) & 0xf]; mask[ 4] = pr;}
		if(pat & 0x0008) {dst[ 3] = pal[(col      ) & 0xf]; mask[ 3] = pr;}
		if(pat & 0x0004) {dst[ 2] = pal[(col >>  8) & 0xf]; mask[ 2] = pr;}
		if(pat & 0x0002) {dst[ 1] = pal[(col >> 16) & 0xf]; mask[ 1] = pr;}
		if(pat & 0x0001) {dst[ 0] = pal[(col >> 24) & 0xf]; mask[ 0] = pr;}
	}
}

void PCE::vdc_plane2pixel(int no)
{
	uint8 pat, *src = &vram[no * 32];
	uint32 col, *pixel = &pixel_bg[no * 8];
	for(int i = 0; i < 8; i++, src += 2, pixel++) {
		pat = src[0];
		col  = ((pat & 0x88) >> 3) | ((pat & 0x44) << 6) | ((pat & 0x22) << 15) | ((pat & 0x11) << 24);
		pat = src[1];
		col |= ((pat & 0x88) >> 2) | ((pat & 0x44) << 7) | ((pat & 0x22) << 16) | ((pat & 0x11) << 25);
		pat = src[16];
		col |= ((pat & 0x88) >> 1) | ((pat & 0x44) << 8) | ((pat & 0x22) << 17) | ((pat & 0x11) << 26);
		pat = src[17];
		col |= ((pat & 0x88)     ) | ((pat & 0x44) << 9) | ((pat & 0x22) << 18) | ((pat & 0x11) << 27);
		pixel[0] = col;
	}
}

void PCE::vdc_sp2pixel(int no)
{
	uint8 pat, *src = &vram[no * 128];
	uint32 col, *pixel = &pixel_spr[no * 32];
	for(int i = 0; i < 32; i++, src++, pixel++) {
		pat = src[0];
		col  = ((pat & 0x88) >> 3) | ((pat & 0x44) << 6) | ((pat & 0x22) << 15) | ((pat & 0x11) << 24);
		pat = src[32];
		col |= ((pat & 0x88) >> 2) | ((pat & 0x44) << 7) | ((pat & 0x22) << 16) | ((pat & 0x11) << 25);
		pat = src[64];
		col |= ((pat & 0x88) >> 1) | ((pat & 0x44) << 8) | ((pat & 0x22) << 17) | ((pat & 0x11) << 26);
		pat = src[96];
		col |= ((pat & 0x88)     ) | ((pat & 0x44) << 9) | ((pat & 0x22) << 18) | ((pat & 0x11) << 27);
		pixel[0] = col;
	}
}

// psg

void PCE::psg_reset()
{
	_memset(psg, 0, sizeof(psg));
	for (int i = 0; i < 6; i++)
		psg[i].regs[4] = 0x80;
	psg[4].randval = psg[5].randval = 0x51f631e4;
	
	psg_ch = 0;
	psg_vol = psg_lfo_freq = psg_lfo_ctrl = 0;
}

void PCE::psg_write(uint16 addr, uint8 data)
{
	switch(addr & 0x1f)
	{
	case 0:
		psg_ch = data & 7;
		break;
	case 1:
		psg_vol = data;
		break;
	case 2:
		psg[psg_ch].regs[2] = data;
		break;
	case 3:
//		psg[psg_ch].regs[3] = data & 0x1f;
		psg[psg_ch].regs[3] = data & 0xf;
		break;
	case 4:
		psg[psg_ch].regs[4] = data;
		break;
	case 5:
		psg[psg_ch].regs[5] = data;
		break;
	case 6:
		if(psg[psg_ch].regs[4] & 0x40)
			psg[psg_ch].wav[0] =data & 0x1f;
		else {
			psg[psg_ch].wav[psg[psg_ch].wavptr] = data & 0x1f;
			psg[psg_ch].wavptr = (psg[psg_ch].wavptr + 1) & 0x1f;
		}
		break;
	case 7:
		psg[psg_ch].regs[7] = data;
		break;
	case 8:
		psg_lfo_freq = data;
		break;
	case 9:
		psg_lfo_ctrl = data;
		break;
	}
}

uint8 PCE::psg_read(uint16 addr)
{
	int ptr;
	
	switch(addr & 0x1f)
	{
	case 0:
		return psg_ch;
	case 1:
		return psg_vol;
	case 2:
		return psg[psg_ch].regs[2];
	case 3:
		return psg[psg_ch].regs[3];
	case 4:
		return psg[psg_ch].regs[4];
	case 5:
		return psg[psg_ch].regs[5];
	case 6:
		ptr = psg[psg_ch].wavptr;
		psg[psg_ch].wavptr = (psg[psg_ch].wavptr + 1) & 0x1f;
		return psg[psg_ch].wav[ptr];
	case 7:
		return psg[psg_ch].regs[7];
	case 8:
		return psg_lfo_freq;
	case 9:
		return psg_lfo_ctrl;
	}
	return 0xff;
}

void PCE::mix(int32* buffer, int cnt)
{
	int vol_tbl[32] = {
		 100, 451, 508, 573, 646, 728, 821, 925,1043,1175,1325, 1493, 1683, 1898, 2139, 2411,
		2718,3064,3454,3893,4388,4947,5576,6285,7085,7986,9002,10148,11439,12894,14535,16384
	};
	
	for(int ch = 0; ch < 6; ch++) {
		if(!(psg[ch].regs[4] & 0x80)) {
			// mute
			psg[ch].genptr = psg[ch].remain = 0;
		}
		else if(psg[ch].regs[4] & 0x40) {
			// dda
			int32 wav = ((int32)psg[ch].wav[0] - 16) * 702;
			int32 vol = max((psg_vol >> 3) & 0x1e, (psg_vol << 1) & 0x1e) + (psg[ch].regs[4] & 0x1f) + max((psg[ch].regs[5] >> 3) & 0x1e, (psg[ch].regs[5] << 1) & 0x1e) - 60;
			vol = (vol < 0) ? 0 : (vol > 31) ? 31 : vol;
			vol = wav * vol_tbl[vol] / 16384;
			for(int i = 0; i < cnt; i++)
				buffer[i] += vol;
		}
		else if(ch >= 4 && (psg[ch].regs[7] & 0x80)) {
			// noise
			uint16 freq = (psg[ch].regs[7] & 0x1f);
			int32 vol = max((psg_vol >> 3) & 0x1e, (psg_vol << 1) & 0x1e) + (psg[ch].regs[4] & 0x1f) + max((psg[ch].regs[5] >> 3) & 0x1e, (psg[ch].regs[5] << 1) & 0x1e) - 60;
			vol = (vol < 0) ? 0 : (vol > 31) ? 31 : vol;
			vol = vol_tbl[vol];
			for(int i = 0; i < cnt; i++) {
				psg[ch].remain += 3000 + freq * 512;
				uint32 t = psg[ch].remain / sample_rate;
				if(t >= 1) {
					if(psg[ch].randval & 0x80000) {
						psg[ch].randval = ((psg[ch].randval ^ 4) << 1) + 1;
						psg[ch].noise = true;
					}
					else {
						psg[ch].randval <<= 1;
						psg[ch].noise = false;
					}
					psg[ch].remain -= sample_rate * t;
				}
				buffer[i] += (int32)((psg[ch].noise ? 10 * 702 : -10 * 702) * vol / 16384);
			}
		}
		else {
			int32 wav[32];
			for(int i = 0; i < 32; i++)
				wav[i] = ((int32)psg[ch].wav[i] - 16) * 702;
			uint32 freq = psg[ch].regs[2] + ((uint32)psg[ch].regs[3] << 8);
			if(freq) {
				int32 vol = max((psg_vol >> 3) & 0x1e, (psg_vol << 1) & 0x1e) + (psg[ch].regs[4] & 0x1f) + max((psg[ch].regs[5] >> 3) & 0x1e, (psg[ch].regs[5] << 1) & 0x1e) - 60;
				vol = (vol < 0) ? 0 : (vol > 31) ? 31 : vol;
				vol = vol_tbl[vol];
				for(int i = 0; i < cnt; i++) {
					buffer[i] += wav[psg[ch].genptr] * vol / 16384;
					psg[ch].remain += 32 * 1118608 / freq;
					uint32 t = psg[ch].remain / (10 * sample_rate);
					psg[ch].genptr = (psg[ch].genptr + t) & 0x1f;
					psg[ch].remain -= 10 * sample_rate * t;
				}
			}
		}
	}
}

// timer

void PCE::timer_reset()
{
	timer_const = timer_count = timer_run = 0;
}

void PCE::timer_write(uint16 addr, uint8 data)
{
	switch(addr & 1)
	{
	case 0:
		timer_const = (data & 0x7f) + 1;
		break;
	case 1:
		if(!timer_run)
			timer_count = timer_const;
		timer_run = data & 1;
		break;
	}
}

uint8 PCE::timer_read(uint16 addr)
{
	return timer_count & 0x7f;
}

// joypad (non multipad)

void PCE::joy_reset()
{
	joy_count = joy_nibble = joy_second = 0;
}

void PCE::joy_write(uint16 addr, uint8 data)
{
	joy_nibble = data & 1;
	if(data & 2) {
		// clear flag
		joy_count = 0;
		joy_nibble = 0;
		joy_second ^= 1;
	}
}

uint8 PCE::joy_read(uint16 addr)
{
	uint8 val = 0xf;
	uint16 stat = 0;
	
	if(!joy_count)
		return 0;
	if(joy_count > 5)
		return 0xf;
	
	if(joy_count == 1) {
		stat = joy_stat[0];
		if(key_stat[0x26]) stat |= 0x001;	// up
		if(key_stat[0x28]) stat |= 0x002;	// down
		if(key_stat[0x25]) stat |= 0x004;	// left
		if(key_stat[0x27]) stat |= 0x008;	// right
		if(key_stat[0x44]) stat |= 0x010;	// d (1)
		if(key_stat[0x53]) stat |= 0x020;	// s (2)
		if(key_stat[0x20]) stat |= 0x040;	// space (select)
		if(key_stat[0x0d]) stat |= 0x080;	// enter (run)
		if(key_stat[0x41]) stat |= 0x100;	// a (3)
		if(key_stat[0x51]) stat |= 0x200;	// q (4)
		if(key_stat[0x57]) stat |= 0x400;	// w (5)
		if(key_stat[0x45]) stat |= 0x800;	// e (6)
	}
	else if(joy_count == 2)
		stat = joy_stat[1];
	
	if(joy_second) {
		if(joy_nibble) {
			if(stat & 0x001) val &= ~1;	// up
			if(stat & 0x008) val &= ~2;	// right
			if(stat & 0x002) val &= ~4;	// down
			if(stat & 0x004) val &= ~8;	// left
		}
		else {
			if(stat & 0x010) val &= ~1;	// b1
			if(stat & 0x020) val &= ~2;	// b2
			if(stat & 0x040) val &= ~4;	// sel
			if(stat & 0x080) val &= ~8;	// run
		}
	}
	else {
		if(joy_nibble)
			val = 0;
		else {
			if(stat & 0x100) val &= ~1;	// b3
			if(stat & 0x200) val &= ~2;	// b4
			if(stat & 0x400) val &= ~4;	// b5
			if(stat & 0x800) val &= ~8;	// b6
		}
	}
	if(joy_count == 5)
		joy_second = 0;
	return val;
}

// interrupt control

void PCE::int_request(uint8 val)
{
	if(val & INT_IRQ2)
		d_cpu->write_signal(did_irq2, 1, 1);
	if(val & INT_IRQ1)
		d_cpu->write_signal(did_irq1, 1, 1);
	if(val & INT_TIRQ)
		d_cpu->write_signal(did_tirq, 1, 1);
}

void PCE::int_cancel(uint8 val)
{
	if(val & INT_IRQ2)
		d_cpu->write_signal(did_irq2, 0, 0);
	if(val & INT_IRQ1)
		d_cpu->write_signal(did_irq1, 0, 0);
	if(val & INT_TIRQ)
		d_cpu->write_signal(did_tirq, 0, 0);
}

void PCE::int_write(uint16 addr, uint8 data)
{
	switch(addr & 3)
	{
	case 2:
		d_cpu->write_signal(did_intmask, data, 7);
		break;
	case 3:
		int_cancel(INT_TIRQ);
		break;
	}
}

uint8 PCE::int_read(uint16 addr)
{
	switch(addr & 3)
	{
	case 2:
		return d_cpu->read_signal(did_intmask);
	case 3:
		return d_cpu->read_signal(did_intstat);
	}
	return 0;
}

