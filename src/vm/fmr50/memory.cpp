/*
	Fujitsu FMR-50 Emulator 'eFMR-50'
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
	_memset(vram, 0, sizeof(cvram));
	_memset(vram, 0, sizeof(kvram));
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
	
	// set palette
	for(int i = 0; i < 16; i++) {
		if(i & 8)
			palette_cg[i] = RGB_COLOR(i & 2 ? 0x1f : 0, i & 4 ? 0x1f : 0, i & 1 ? 0x1f : 0);
		else
			palette_cg[i] = RGB_COLOR(i & 2 ? 0x10 : 0, i & 4 ? 0x10 : 0, i & 1 ? 0x10 : 0);
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
	apalsel = cgregsel = 0;
	outctrl = 0xf;
	dispctrl = 0x47;
	mix = 8;
	accaddr = dispaddr = 0;
	blink = 0;
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xffffff;
	if(!mainmem) {
		if(0xc0000 <= addr && addr < 0xc8000) {
			// vram
			uint32 ofs = addr & 0x7fff;
			if(pagesel & 0x10) {
				if(wplane & 1) vram[ofs | 0x20000] = data;
				if(wplane & 2) vram[ofs | 0x28000] = data;
				if(wplane & 4) vram[ofs | 0x30000] = data;
				if(wplane & 8) vram[ofs | 0x38000] = data;
			}
			else {
				if(wplane & 1) vram[ofs | 0x00000] = data;
				if(wplane & 2) vram[ofs | 0x08000] = data;
				if(wplane & 4) vram[ofs | 0x10000] = data;
				if(wplane & 8) vram[ofs | 0x18000] = data;
			}
			return;
		}
		else if(0xcff80 <= addr && addr < 0xcffe0) {
			// memory mapped i/o
			switch(addr & 0xffff)
			{
			case 0xff80:
				// mix register
				mix = data;
				break;
			case 0xff81:
				// update register
				wplane = data & 0xf;
				rplane = (data >> 6) & 3;
				update_bank();
				break;
			case 0xff82:
				// display ctrl register
				dispctrl = data;
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
				cgregsel = data;
				break;
			case 0xff8f:
				// crtc data register
				cgreg[cgregsel] = data;
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
				return (disp ? 0x80 : 0) | (vsync ? 4 : 0);
			case 0xff8e:
				// crtc addr register
				return cgregsel;
			case 0xff8f:
				// crtc data register
				return cgreg[cgregsel];
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
//			emu->power_off();
		}
		if(data & 1) {
			// software reset
			rst |= 1;
			d_cpu->reset();
		}
		// protect mode
		d_cpu->write_signal(did_a20, data, 0x20);
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
		palette_cg[apalsel] = RGB_COLOR(apal[apalsel][1] >> 3, apal[apalsel][2] >> 3, apal[apalsel][0] >> 3);
		break;
	case 0x40c:
		// red level register
		apal[apalsel][1] = data & 0xf0;
		palette_cg[apalsel] = RGB_COLOR(apal[apalsel][1] >> 3, apal[apalsel][2] >> 3, apal[apalsel][0] >> 3);
		break;
	case 0x40e:
		// green level register
		apal[apalsel][2] = data & 0xf0;
		palette_cg[apalsel] = RGB_COLOR(apal[apalsel][1] >> 3, apal[apalsel][2] >> 3, apal[apalsel][0] >> 3);
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
			palette_cg[addr & 7] = RGB_COLOR(data & 2 ? 0x1f : 0, data & 4 ? 0x1f : 0, data & 1 ? 0x1f : 0);
		else
			palette_cg[addr & 7] = RGB_COLOR(data & 2 ? 0x10 : 0, data & 4 ? 0x10 : 0, data & 1 ? 0x10 : 0);
		break;
	case 0xfda0:
		// crt output control register
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
	SET_BANK(0x000000, 0xffffff, wdmy, rdmy);
	SET_BANK(0x000000, sizeof(ram) - 1, ram, ram);
	if(!mainmem) {
		SET_BANK(0x0c0000, 0x0effff, wdmy, rdmy);
		int ofs = (rplane | (pagesel & 0x10 ? 4 : 0)) * 0x8000;
		SET_BANK(0x0c0000, 0x0c7fff, vram + ofs, vram + ofs);
		SET_BANK(0x0c8000, 0x0c8fff, cvram, cvram);
		if(ankcg & 1) {
			SET_BANK(0x0ca000, 0x0ca7ff, wdmy, ank8);
			SET_BANK(0x0cb000, 0x0cbfff, wdmy, ank16);
		}
		else {
			SET_BANK(0x0ca000, 0x0cafff, kvram, kvram);
		}
	}
	if(!(protect & 0x20)) {
//		SET_BANK(0x0f8000, 0x0fbfff, wdmy, rdmy);
//		SET_BANK(0x0fc000, 0x0fffff, wdmy, ipl);
		SET_BANK(0x0f8000, 0x0fbfff, ram + 0xf8000, rdmy);
		SET_BANK(0x0fc000, 0x0fffff, ram + 0xfc000, ipl);
	}
	SET_BANK(0xffc000, 0xffffff, wdmy, ipl);
}

void MEMORY::draw_screen()
{
	// render screen
	_memset(screen_txt, 0, sizeof(screen_txt));
	_memset(screen_cg, 0, sizeof(screen_cg));
	draw_text();
	draw_cg();
	
	for(int y = 0; y < 400; y++) {
		uint16* dest = emu->screen_buffer(y);
		uint8* txt = screen_txt[y];
		uint8* cg = screen_cg[y];
		
		for(int x = 0; x < 640; x++)
			dest[x] = txt[x] ? palette_txt[txt[x]] : palette_cg[cg[x]];
	}
	
	// access lamp
	uint32 stat_f = d_fdc->read_signal(0) | d_bios->read_signal(0);
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

void MEMORY::draw_text()
{
	uint16 src = ((chreg[12] << 9) | (chreg[13] << 1)) & 0xfff;
	
	for(int y = 0; y < 25; y++) {
		for(int x = 0; x < 80; x++) {
			uint8 code = cvram[src];
			uint8 kj_h = kvram[src] & 0x7f;
			src = (src + 1) & 0xfff;
			uint8 attr = cvram[src];
			uint8 kj_l = kvram[src] & 0x7f;
			src = (src + 1) & 0xfff;
			uint8 col = ((attr & 0x20) >> 2) | (attr & 7);
			bool blnk = (blink & 32) && (attr & 0x10);
			bool rev = ((attr & 8) != 0);
			
			if(attr & 0x40) {
				// kanji
				int ofs;
				if(kj_h < 0x30)
					ofs = (((kj_l - 0x00) & 0x1f) <<  5) | (((kj_l - 0x20) & 0x20) <<  9) | (((kj_l - 0x20) & 0x40) <<  7) | (((kj_h - 0x00) & 0x07) << 10) | 0x00000;
				else if(kj_h < 0x70)
					ofs = (((kj_l - 0x00) & 0x1f) <<  5) + (((kj_l - 0x20) & 0x60) <<  9) + (((kj_h - 0x00) & 0x0f) << 10) + (((kj_h - 0x30) & 0x70) * 0xc00) + 0x08000;
				else
					ofs = (((kj_l - 0x00) & 0x1f) <<  5) | (((kj_l - 0x20) & 0x20) <<  9) | (((kj_l - 0x20) & 0x40) <<  7) | (((kj_h - 0x00) & 0x07) << 10) | 0x38000;
				
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
		}
	}
}

void MEMORY::draw_cg()
{
	
}

