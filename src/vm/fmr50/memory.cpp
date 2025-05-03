/*
	FUJITSU FMR-50 Emulator 'eFMR-50'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.29 -

	[ memory and crtc ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 11, eb = (e) >> 11; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) \
			wbank[i] = wdmy; \
		else \
			wbank[i] = (w) + 0x800 * (i - sb); \
		if((r) == rdmy) \
			rbank[i] = rdmy; \
		else \
			rbank[i] = (r) + 0x800 * (i - sb); \
	} \
}

void MEMORY::initialize()
{
	// init memory
	_memset(ram, 0, sizeof(ram));
	_memset(vram, 0, sizeof(vram));
	_memset(cvram, 0, sizeof(cvram));
	_memset(kvram, 0, sizeof(kvram));
	_memset(dummy, 0, sizeof(dummy));
	_memset(ipl, 0xff, sizeof(ipl));
	_memset(ank8, 0xff, sizeof(ank8));
	_memset(ank16, 0xff, sizeof(ank16));
	_memset(kanji16, 0xff, sizeof(kanji16));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	// init machine id (FMR-50/LT3)
	id[0] = 0xd8;
	id[1] = 0xff;
	
	// load rom image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sIPL.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ipl, sizeof(ipl), 1);
		fio->Fclose();
	}
	else {
		// load pseudo ipl
		_memcpy(ipl + 0x0000, bios1, sizeof(bios1));
		_memcpy(ipl + 0x3ff0, bios2, sizeof(bios2));
	}
	_stprintf(file_path, _T("%sANK8.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ank8, sizeof(ank8), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sANK16.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ank16, sizeof(ank16), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sKANJI16.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(kanji16, sizeof(kanji16), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sMACHINE.ID"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(id, sizeof(id), 1);
		fio->Fclose();
	}
	delete fio;
	
	// set memory
	SET_BANK(0x000000, 0xffffff, wdmy, rdmy);
	SET_BANK(0x000000, sizeof(ram) - 1, ram, ram);
	SET_BANK(0xffc000, 0xffffff, wdmy, ipl);
	
	// set palette
	for(int i = 0; i < 8; i++)
		dpal[i] = i;
	for(int i = 0; i < 16; i++) {
		if(i & 8)
			palette_cg[i] = RGB_COLOR(i & 2 ? 255 : 0, i & 4 ? 255 : 0, i & 1 ? 255 : 0);
		else
			palette_cg[i] = RGB_COLOR(i & 2 ? 127 : 0, i & 4 ? 127 : 0, i & 1 ? 127 : 0);
		palette_txt[i] = palette_cg[i];
	}
	
	// regist event
	vm->regist_frame_event(this);
}

void MEMORY::reset()
{
	// reset memory
	protect = rst = 0;
	mainmem = rplane = wplane = pagesel = ankcg = 0;
	update_bank();
	
	// reset crtc
	apalsel = 0;
	outctrl = 0xf;
	dispctrl = 0x47;
	mix = 8;
	accaddr = dispaddr = 0;
	kj_l = kj_h = kj_ofs = kj_row = 0;
	blink = 0;
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xffffff;
	if(!mainmem) {
		if(0xc0000 <= addr && addr < 0xc8000) {
			// vram
			if(dispctrl & 0x40) {
				// 400 line
//				uint32 ofs = (((pagesel >> 4) & 1) * 0x20000) | (addr & 0x7fff);
				uint32 ofs = ((pagesel & 0x10) << 13) | (addr & 0x7fff);
				if(wplane & 1) vram[ofs | 0x00000] = data;
				if(wplane & 2) vram[ofs | 0x08000] = data;
				if(wplane & 4) vram[ofs | 0x10000] = data;
				if(wplane & 8) vram[ofs | 0x18000] = data;
			}
			else {
//				uint32 ofs = (((pagesel >> 3) & 3) * 0x10000) | (addr & 0x7fff);
				uint32 ofs = ((pagesel & 0x18) << 13) | (addr & 0x3fff);
				if(wplane & 1) vram[ofs | 0x00000] = data;
				if(wplane & 2) vram[ofs | 0x04000] = data;
				if(wplane & 4) vram[ofs | 0x08000] = data;
				if(wplane & 8) vram[ofs | 0x10000] = data;
			}
			return;
		}
		else if(0xcff80 <= addr && addr < 0xcffe0) {
#ifdef _DEBUG_LOG
			emu->out_debug("MW\t%4x, %2x\n", addr, data);
#endif
			// memory mapped i/o
			switch(addr & 0xffff)
			{
			case 0xff80:
				// mix register
				mix = data;
				break;
			case 0xff81:
				// update register
				wplane = data & 7;
				rplane = (data >> 6) & 3;
				update_bank();
				break;
			case 0xff82:
				// display ctrl register
				dispctrl = data;
				update_bank();
				break;
			case 0xff83:
				// page select register
				pagesel = data;
				update_bank();
				break;
			case 0xff88:
				// access start register
				accaddr = (accaddr & 0xff) | ((data & 0x7f) << 8);
				break;
			case 0xff89:
				// access start register
				accaddr = (accaddr & 0xff00) | (data & 0xfe);
				break;
			case 0xff8a:
				// display start register
				dispaddr = (dispaddr & 0xff) | ((data & 0x7f) << 8);
				break;
			case 0xff8b:
				// display start register
				dispaddr = (dispaddr & 0xff00) | (data & 0xfe);
				break;
			case 0xff8e:
				// crtc addr register
				d_crtc->write_io8(0, data);
				break;
			case 0xff8f:
				// crtc data register
				d_crtc->write_io8(1, data);
				break;
			case 0xff94:
				kj_h = data & 0x7f;
				break;
			case 0xff95:
				kj_l = data & 0x7f;
				kj_row = 0;
				if(kj_h < 0x30)
					kj_ofs = (((kj_l - 0x00) & 0x1f) <<  5) | (((kj_l - 0x20) & 0x20) <<  9) | (((kj_l - 0x20) & 0x40) <<  7) | (((kj_h - 0x00) & 0x07) << 10);
				else if(kj_h < 0x70)
					kj_ofs = (((kj_l - 0x00) & 0x1f) <<  5) + (((kj_l - 0x20) & 0x60) <<  9) + (((kj_h - 0x00) & 0x0f) << 10) + (((kj_h - 0x30) & 0x70) * 0xc00) + 0x08000;
				else
					kj_ofs = (((kj_l - 0x00) & 0x1f) <<  5) | (((kj_l - 0x20) & 0x20) <<  9) | (((kj_l - 0x20) & 0x40) <<  7) | (((kj_h - 0x00) & 0x07) << 10) | 0x38000;
				break;
			case 0xff96:
				kanji16[(kj_ofs | ((kj_row & 0xf) << 1)) & 0x3ffff] = data;
				break;
			case 0xff97:
				kanji16[(kj_ofs | ((kj_row++ & 0xf) << 1) | 1) & 0x3ffff] = data;
				break;
			case 0xff99:
				ankcg = data;
				update_bank();
				break;
			}
			return;
		}
	}
	if((addr & ~3) == 8 && (protect & 0x80))
		return;
	wbank[addr >> 11][addr & 0x7ff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xffffff;
	if(!mainmem) {
		if(0xcff80 <= addr && addr < 0xcffe0) {
#ifdef _DEBUG_LOG
			emu->out_debug("MR\t%4x\n", addr);
#endif
			// memory mapped i/o
			switch(addr & 0xffff)
			{
			case 0xff80:
				// mix register
				return mix;
			case 0xff81:
				// update register
				return wplane | (rplane << 6);
			case 0xff83:
				// page select register
				return pagesel;
			case 0xff86:
				// status register
				return (disp ? 0x80 : 0) | (vsync ? 4 : 0) | 0x10;
			case 0xff8e:
				// crtc addr register
				return d_crtc->read_io8(0);
			case 0xff8f:
				// crtc data register
				return d_crtc->read_io8(1);
			case 0xff94:
				return 0x80;	// ???
			case 0xff96:
				return kanji16[(kj_ofs | ((kj_row & 0xf) << 1)) & 0x3ffff];
			case 0xff97:
				return kanji16[(kj_ofs | ((kj_row++ & 0xf) << 1) | 1) & 0x3ffff];
			}
			return 0xff;
		}
	}
	return rbank[addr >> 11][addr & 0x7ff];
}

void MEMORY::write_dma8(uint32 addr, uint32 data)
{
	write_data8(addr, data);
}

uint32 MEMORY::read_dma8(uint32 addr)
{
	return read_data8(addr);
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xffff)
	{
	case 0x20:
		// protect and reset
		protect = data;
		update_bank();
		if(data & 0x40) {
			// power off
			emu->power_off();
		}
		if(data & 1) {
			// software reset
			rst |= 1;
			d_cpu->reset();
		}
		// protect mode
		d_cpu->write_signal(did_a20, data, 0x20);
		break;
	case 0x400:
		// video output control
		break;
	case 0x402:
		// update register
		wplane = data & 0xf;
		break;
	case 0x404:
		// read out register
		mainmem = data & 0x80;
		rplane = data & 3;
		update_bank();
		break;
	case 0x408:
		// palette code register
		apalsel = data & 0xf;
		break;
	case 0x40a:
		// blue level register
		apal[apalsel][0] = data & 0xf0;
		palette_cg[apalsel] = RGB_COLOR(apal[apalsel][1], apal[apalsel][2], apal[apalsel][0]);
		break;
	case 0x40c:
		// red level register
		apal[apalsel][1] = data & 0xf0;
		palette_cg[apalsel] = RGB_COLOR(apal[apalsel][1], apal[apalsel][2], apal[apalsel][0]);
		break;
	case 0x40e:
		// green level register
		apal[apalsel][2] = data & 0xf0;
		palette_cg[apalsel] = RGB_COLOR(apal[apalsel][1], apal[apalsel][2], apal[apalsel][0]);
		break;
	case 0xfd90:
		// palette code register
		apalsel = data & 0xf;
		break;
	case 0xfd92:
		// blue level register
		apal[apalsel][0] = data;
		palette_cg[apalsel] = RGB_COLOR(apal[apalsel][1], apal[apalsel][2], apal[apalsel][0]);
		break;
	case 0xfd94:
		// red level register
		apal[apalsel][1] = data;
		palette_cg[apalsel] = RGB_COLOR(apal[apalsel][1], apal[apalsel][2], apal[apalsel][0]);
		break;
	case 0xfd96:
		// green level register
		apal[apalsel][2] = data;
		palette_cg[apalsel] = RGB_COLOR(apal[apalsel][1], apal[apalsel][2], apal[apalsel][0]);
		break;
	case 0xfd98:
	case 0xfd99:
	case 0xfd9a:
	case 0xfd9b:
	case 0xfd9c:
	case 0xfd9d:
	case 0xfd9e:
	case 0xfd9f:
		// digital palette
		dpal[addr & 7] = data;
		if(data & 8)
			palette_cg[addr & 7] = RGB_COLOR(data & 2 ? 255 : 0, data & 4 ? 255 : 0, data & 1 ? 255 : 0);
		else
			palette_cg[addr & 7] = RGB_COLOR(data & 2 ? 127 : 0, data & 4 ? 127 : 0, data & 1 ? 127 : 0);
		break;
	case 0xfda0:
		// video output control
		outctrl = data;
		break;
	case 0xff81:
		// update register
		wplane = data & 0xf;
		rplane = (data >> 6) & 3;
		update_bank();
		break;
	}
}

uint32 MEMORY::read_io8(uint32 addr)
{
	uint32 val = 0xff;
	
	switch(addr & 0xffff)
	{
	case 0x20:
		// reset cause register
		val = rst;
		rst &= ~3;
		return val | 0x7c;
	case 0x30:
		// machine & cpu id
		return id[0];
	case 0x31:
		// machine id
		return id[1];
	case 0x400:
		// system status register
		return 0xfe;
//		return 0xf6;
	case 0x402:
		// update register
		return wplane | 0xf0;
	case 0x404:
		// read out register
		return mainmem | rplane | 0x7c;
	case 0x40a:
		// blue level register
		return apal[apalsel][0] | 0xf;
	case 0x40c:
		// red level register
		return apal[apalsel][1] | 0xf;
	case 0x40e:
		// green level register
		return apal[apalsel][2] | 0xf;
	case 0xfd92:
		// blue level register
		return apal[apalsel][0];
	case 0xfd94:
		// red level register
		return apal[apalsel][1];
	case 0xfd96:
		// green level register
		return apal[apalsel][2];
	case 0xfd98:
	case 0xfd99:
	case 0xfd9a:
	case 0xfd9b:
	case 0xfd9c:
	case 0xfd9d:
	case 0xfd9e:
	case 0xfd9f:
		// digital palette
		return dpal[addr & 7] | 0xf0;
	case 0xfda0:
		// status register
		return (disp ? 2 : 0) | (vsync ? 1 : 0) | 0xfc;
	case 0xff81:
		// update register
		return wplane | (rplane << 6);
	}
	return 0xff;
}

void MEMORY::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_MEMORY_DISP)
		disp = ((data & mask) != 0);
	else if(id == SIG_MEMORY_VSYNC)
		vsync = ((data & mask) != 0);
}

void MEMORY::event_frame()
{
	blink++;
}

void MEMORY::update_bank()
{
	if(!mainmem) {
		SET_BANK(0xc0000, 0xeffff, wdmy, rdmy);
		if(dispctrl & 0x40) {
			// 400 line
			int ofs = (rplane | ((pagesel >> 2) & 4)) * 0x8000;
			SET_BANK(0xc0000, 0xc7fff, vram + ofs, vram + ofs);
		}
		else {
			// 200 line
			int ofs = (rplane | ((pagesel >> 1) & 0xc)) * 0x4000;
			SET_BANK(0xc0000, 0xc3fff, vram + ofs, vram + ofs);
			SET_BANK(0xc4000, 0xc7fff, vram + ofs, vram + ofs);
		}
		SET_BANK(0xc8000, 0xc8fff, cvram, cvram);
		SET_BANK(0xc9000, 0xc9fff, wdmy, rdmy);
		if(ankcg & 1) {
			SET_BANK(0xca000, 0xca7ff, wdmy, ank8);
			SET_BANK(0xca800, 0xcafff, wdmy, rdmy);
			SET_BANK(0xcb000, 0xcbfff, wdmy, ank16);
		}
		else {
			SET_BANK(0xca000, 0xcafff, kvram, kvram);
			SET_BANK(0xcb000, 0xcbfff, wdmy, rdmy);
		}
		SET_BANK(0xcc000, 0xeffff, wdmy, rdmy);
	}
	else {
		SET_BANK(0xc0000, 0xeffff, ram + 0xc0000, ram + 0xc0000);
	}
	if(!(protect & 0x20)) {
		SET_BANK(0xf8000, 0xfbfff, ram + 0xf8000, rdmy);
		SET_BANK(0xfc000, 0xfffff, ram + 0xfc000, ipl);
	}
	else {
		SET_BANK(0xf8000, 0xfffff, ram + 0xf8000, ram + 0xf8000);
	}
}

void MEMORY::draw_screen()
{
	// render screen
	_memset(screen_txt, 0, sizeof(screen_txt));
	_memset(screen_cg, 0, sizeof(screen_cg));
	if(outctrl & 1)
		draw_text();
	if(outctrl & 4)
		draw_cg();
	
	for(int y = 0; y < 400; y++) {
		scrntype* dest = emu->screen_buffer(y);
		uint8* txt = screen_txt[y];
		uint8* cg = screen_cg[y];
		
		for(int x = 0; x < 640; x++)
			dest[x] = txt[x] ? palette_txt[txt[x]] : palette_cg[cg[x]];
	}
	
	// access lamp
	uint32 stat_f = d_fdc->read_signal(0) | d_bios->read_signal(0);
	if(stat_f) {
		scrntype col = (stat_f & 0x10   ) ? RGB_COLOR(0, 0, 255) :
		               (stat_f & (1 | 4)) ? RGB_COLOR(255, 0, 0) :
		               (stat_f & (2 | 8)) ? RGB_COLOR(0, 255, 0) : 0;
		for(int y = 400 - 8; y < 400; y++) {
			scrntype *dest = emu->screen_buffer(y);
			for(int x = 640 - 8; x < 640; x++)
				dest[x] = col;
		}
	}
}

void MEMORY::draw_text()
{
	int src = ((chreg[12] << 9) | (chreg[13] << 1)) & 0xfff;
	int caddr = ((chreg[8] & 0xc0) == 0xc0) ? -1 : (((chreg[14] << 9) | (chreg[15] << 1) | (mix & 0x20 ? 1 : 0)) & 0x7ff);
	
	for(int y = 0; y < 25; y++) {
		for(int x = 0; x < 80; x++) {
			bool cursor = ((src >> 1) == caddr);
			int cx = x;
			uint8 code = cvram[src];
			uint8 h = kvram[src] & 0x7f;
			src = (src + 1) & 0xfff;
			uint8 attr = cvram[src];
			uint8 l = kvram[src] & 0x7f;
			src = (src + 1) & 0xfff;
			uint8 col = ((attr & 0x20) >> 2) | (attr & 7);
			bool blnk = (blink & 32) && (attr & 0x10);
			bool rev = ((attr & 8) != 0);
			
			if(attr & 0x40) {
				// kanji
				int ofs;
				if(h < 0x30)
					ofs = (((l - 0x00) & 0x1f) <<  5) | (((l - 0x20) & 0x20) <<  9) | (((l - 0x20) & 0x40) <<  7) | (((h - 0x00) & 0x07) << 10);
				else if(h < 0x70)
					ofs = (((l - 0x00) & 0x1f) <<  5) + (((l - 0x20) & 0x60) <<  9) + (((h - 0x00) & 0x0f) << 10) + (((h - 0x30) & 0x70) * 0xc00) + 0x08000;
				else
					ofs = (((l - 0x00) & 0x1f) <<  5) | (((l - 0x20) & 0x20) <<  9) | (((l - 0x20) & 0x40) <<  7) | (((h - 0x00) & 0x07) << 10) | 0x38000;
				
				for(int l = 0; l < 16; l++) {
					uint8 pat0 = kanji16[ofs + (l << 1) + 0];
					uint8 pat1 = kanji16[ofs + (l << 1) + 1];
					pat0 = blnk ? 0 : rev ? ~pat0 : pat0;
					pat1 = blnk ? 0 : rev ? ~pat1 : pat1;
					int yy = (y << 4) + l;
					if(yy >= 400)
						break;
					uint8* d = &screen_txt[yy][x << 3];
					
					d[ 0] = (pat0 & 0x80) ? col : 0;
					d[ 1] = (pat0 & 0x40) ? col : 0;
					d[ 2] = (pat0 & 0x20) ? col : 0;
					d[ 3] = (pat0 & 0x10) ? col : 0;
					d[ 4] = (pat0 & 0x08) ? col : 0;
					d[ 5] = (pat0 & 0x04) ? col : 0;
					d[ 6] = (pat0 & 0x02) ? col : 0;
					d[ 7] = (pat0 & 0x01) ? col : 0;
					d[ 8] = (pat1 & 0x80) ? col : 0;
					d[ 9] = (pat1 & 0x40) ? col : 0;
					d[10] = (pat1 & 0x20) ? col : 0;
					d[11] = (pat1 & 0x10) ? col : 0;
					d[12] = (pat1 & 0x08) ? col : 0;
					d[13] = (pat1 & 0x04) ? col : 0;
					d[14] = (pat1 & 0x02) ? col : 0;
					d[15] = (pat1 & 0x01) ? col : 0;
				}
				src = (src + 2) & 0xfff;
				x++;
			}
			else {
				for(int l = 0; l < 16; l++) {
					uint8 pat = ank16[(code << 4) + l];
					pat = blnk ? 0 : rev ? ~pat : pat;
					int yy = (y << 4) + l;
					if(yy >= 400)
						break;
					uint8* d = &screen_txt[yy][x << 3];
					
					d[0] = (pat & 0x80) ? col : 0;
					d[1] = (pat & 0x40) ? col : 0;
					d[2] = (pat & 0x20) ? col : 0;
					d[3] = (pat & 0x10) ? col : 0;
					d[4] = (pat & 0x08) ? col : 0;
					d[5] = (pat & 0x04) ? col : 0;
					d[6] = (pat & 0x02) ? col : 0;
					d[7] = (pat & 0x01) ? col : 0;
				}
			}
			if(cursor) {
				int bp = chreg[10] & 0x60;
				if(bp == 0 || (bp == 0x40 && (blink & 8)) || (bp == 0x60 && (blink & 0x10))) {
					for(int i = (chreg[10] & 15); i < 15; i++)
						_memset(&screen_txt[y * 16 + i][cx << 3], 7, 8);
				}
			}
		}
	}
}

void MEMORY::draw_cg()
{
	if(dispctrl & 0x40) {
		// 400line
		int pofs = ((dispctrl >> 3) & 1) * 0x20000;
		uint8* p0 = (dispctrl & 0x01) ? &vram[pofs | 0x00000] : dummy;
		uint8* p1 = (dispctrl & 0x02) ? &vram[pofs | 0x08000] : dummy;
		uint8* p2 = (dispctrl & 0x04) ? &vram[pofs | 0x10000] : dummy;
		uint8* p3 = (dispctrl & 0x20) ? &vram[pofs | 0x18000] : dummy;	// ???
		int ptr = dispaddr & 0x7ffe;
		
		for(int y = 0; y < 400; y++) {
			for(int x = 0; x < 640; x += 8) {
				uint8 r = p0[ptr];
				uint8 g = p1[ptr];
				uint8 b = p2[ptr];
				uint8 i = p3[ptr++];
				ptr &= 0x7fff;
				uint8* d = &screen_cg[y][x];
				
				d[0] = ((r & 0x80) >> 7) | ((g & 0x80) >> 6) | ((b & 0x80) >> 5) | ((i & 0x80) >> 4);
				d[1] = ((r & 0x40) >> 6) | ((g & 0x40) >> 5) | ((b & 0x40) >> 4) | ((i & 0x40) >> 3);
				d[2] = ((r & 0x20) >> 5) | ((g & 0x20) >> 4) | ((b & 0x20) >> 3) | ((i & 0x20) >> 2);
				d[3] = ((r & 0x10) >> 4) | ((g & 0x10) >> 3) | ((b & 0x10) >> 2) | ((i & 0x10) >> 1);
				d[4] = ((r & 0x08) >> 3) | ((g & 0x08) >> 2) | ((b & 0x08) >> 1) | ((i & 0x08) >> 0);
				d[5] = ((r & 0x04) >> 2) | ((g & 0x04) >> 1) | ((b & 0x04) >> 0) | ((i & 0x04) << 1);
				d[6] = ((r & 0x02) >> 1) | ((g & 0x02) >> 0) | ((b & 0x02) << 1) | ((i & 0x02) << 2);
				d[7] = ((r & 0x01) >> 0) | ((g & 0x01) << 1) | ((b & 0x01) << 2) | ((i & 0x01) << 3);
			}
		}
	}
	else {
		// 200line
		int pofs = ((dispctrl >> 3) & 3) * 0x10000;
		uint8* p0 = (dispctrl & 0x01) ? &vram[pofs | 0x0000] : dummy;
		uint8* p1 = (dispctrl & 0x02) ? &vram[pofs | 0x4000] : dummy;
		uint8* p2 = (dispctrl & 0x04) ? &vram[pofs | 0x8000] : dummy;
		uint8* p3 = (dispctrl & 0x20) ? &vram[pofs | 0xc000] : dummy;	// ???
		int ptr = dispaddr & 0x3ffe;
		
		for(int y = 0; y < 400; y += 2) {
			for(int x = 0; x < 640; x += 8) {
				uint8 r = p0[ptr];
				uint8 g = p1[ptr];
				uint8 b = p2[ptr];
				uint8 i = p3[ptr++];
				ptr &= 0x3fff;
				uint8* d = &screen_cg[y][x];
				
				d[0] = ((r & 0x80) >> 7) | ((g & 0x80) >> 6) | ((b & 0x80) >> 5) | ((i & 0x80) >> 4);
				d[1] = ((r & 0x40) >> 6) | ((g & 0x40) >> 5) | ((b & 0x40) >> 4) | ((i & 0x40) >> 3);
				d[2] = ((r & 0x20) >> 5) | ((g & 0x20) >> 4) | ((b & 0x20) >> 3) | ((i & 0x20) >> 2);
				d[3] = ((r & 0x10) >> 4) | ((g & 0x10) >> 3) | ((b & 0x10) >> 2) | ((i & 0x10) >> 1);
				d[4] = ((r & 0x08) >> 3) | ((g & 0x08) >> 2) | ((b & 0x08) >> 1) | ((i & 0x08) >> 0);
				d[5] = ((r & 0x04) >> 2) | ((g & 0x04) >> 1) | ((b & 0x04) >> 0) | ((i & 0x04) << 1);
				d[6] = ((r & 0x02) >> 1) | ((g & 0x02) >> 0) | ((b & 0x02) << 1) | ((i & 0x02) << 2);
				d[7] = ((r & 0x01) >> 0) | ((g & 0x01) << 1) | ((b & 0x01) << 2) | ((i & 0x01) << 3);
			}
			_memcpy(screen_cg[y + 1], screen_cg[y], 640);
		}
	}
}

