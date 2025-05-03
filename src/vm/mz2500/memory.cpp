/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.24 -

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 11, eb = (e) >> 11; \
	for(int i = sb; i <= eb; i++) { \
		wbank[i] = (w) + 0x800 * (i - sb); \
		rbank[i] = (r) + 0x800 * (i - sb); \
	} \
}

void MEMORY::initialize()
{
	// init memory
	_memset(ram, 0, sizeof(ram));
	_memset(vram, 0, sizeof(vram));
	_memset(tvram, 0, sizeof(tvram));
	_memset(pcg, 0, sizeof(pcg));
	_memset(ipl, 0xff, sizeof(ipl));
	_memset(kanji, 0xff, sizeof(kanji));
	_memset(dic, 0xff, sizeof(dic));
	_memset(phone, 0xff, sizeof(phone));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	// load rom image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sIPL.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ipl, sizeof(ipl), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sKANJI.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(kanji, sizeof(kanji), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sDICT.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(dic, sizeof(dic), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sPHONE.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(phone, sizeof(phone), 1);
		fio->Fclose();
	}
	delete fio;
	
	blank = hblank = vblank = busreq = false;
}

void MEMORY::reset()
{
	// init memory map
	bank = 0;
	set_map(0x00);
	set_map(0x01);
	set_map(0x02);
	set_map(0x03);
	set_map(0x04);
	set_map(0x05);
	set_map(0x06);
	set_map(0x07);
}

void MEMORY::ipl_reset()
{
	// init memory map
	bank = 0;
	set_map(0x34);
	set_map(0x35);
	set_map(0x36);
	set_map(0x37);
	set_map(0x04);
	set_map(0x05);
	set_map(0x06);
	set_map(0x07);
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	int b = addr >> 13;
#ifdef VRAM_WAIT
	if(is_vram[b] && !blank) {
		// vram wait
		d_cpu->write_signal(SIG_CPU_BUSREQ, 0xffffffff, 1);
		busreq = true;
	}
#endif
	if(page_type[b] == PAGE_TYPE_MODIFY) {
		// read write modify
		if(page[b] == 0x30)
			d_crtc->write_data8((addr & 0x1fff) + 0x0000, data);
		else if(page[b] == 0x31)
			d_crtc->write_data8((addr & 0x1fff) + 0x2000, data);
		else if(page[b] == 0x32)
			d_crtc->write_data8((addr & 0x1fff) + 0x4000, data);
		else
			d_crtc->write_data8((addr & 0x1fff) + 0x6000, data);
		return;
	}
	wbank[addr >> 11][addr & 0x7ff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	int b = addr >> 13;
#ifdef VRAM_WAIT
	if(is_vram[b] && !blank) {
		// vram wait
		d_cpu->write_signal(SIG_CPU_BUSREQ, 0xffffffff, 1);
		busreq = true;
	}
#endif
	if(page_type[b] == PAGE_TYPE_MODIFY) {
		// read write modify
		if(page[b] == 0x30)
			return d_crtc->read_data8((addr & 0x1fff) + 0x0000);
		else if(page[b] == 0x31)
			return d_crtc->read_data8((addr & 0x1fff) + 0x2000);
		else if(page[b] == 0x32)
			return d_crtc->read_data8((addr & 0x1fff) + 0x4000);
		else
			return d_crtc->read_data8((addr & 0x1fff) + 0x6000);
	}
	return rbank[addr >> 11][addr & 0x7ff];
}

void MEMORY::write_data16(uint32 addr, uint32 data)
{
	write_data8(addr, data & 0xff);
	write_data8(addr + 1, data >> 8);
}

uint32 MEMORY::read_data16(uint32 addr)
{
	return read_data8(addr) | (read_data8(addr + 1) << 8);
}

void MEMORY::write_data8w(uint32 addr, uint32 data, int* wait)
{
	*wait = page_wait[addr >> 13];
	write_data8(addr, data);
}

uint32 MEMORY::read_data8w(uint32 addr, int* wait)
{
	*wait = page_wait[addr >> 13];
	return read_data8(addr);
}

void MEMORY::write_data16w(uint32 addr, uint32 data, int* wait)
{
	*wait = page_wait[addr >> 13] << 1;
	write_data8(addr, data & 0xff);
	write_data8(addr + 1, data >> 8);
}

uint32 MEMORY::read_data16w(uint32 addr, int* wait)
{
	*wait = page_wait[addr >> 13] << 1;
	return read_data8(addr) | (read_data8(addr + 1) << 8);
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0xb4:
		// map bank
		bank = data & 7;
		break;
	case 0xb5:
		// map reg
		set_map(data & 0x3f);
		break;
	case 0xce:
		// dictionary bank
		dic_bank = data & 0x1f;
		for(int i = 0; i < 8; i++) {
			if(page_type[i] == PAGE_TYPE_DIC) {
				SET_BANK(i * 0x2000,  i * 0x2000 + 0x1fff, wdmy, dic + dic_bank * 0x2000);
			}
		}
		break;
	case 0xcf:
		// kanji bank
		kanji_bank = data;
		for(int i = 0; i < 8; i++) {
			if(page_type[i] == PAGE_TYPE_KANJI) {
				if(kanji_bank & 0x80) {
					SET_BANK(i * 0x2000,  i * 0x2000 + 0x7ff, wdmy, kanji + (kanji_bank & 0x7f) * 0x800);
				}
				else {
					SET_BANK(i * 0x2000,  i * 0x2000 + 0x7ff, pcg, pcg);
				}
			}
		}
		break;
	}
}

uint32 MEMORY::read_io8(uint32 addr)
{
	switch(addr & 0xff)
	{
	case 0xb4:
		// map bank
		return bank;
	case 0xb5:
		// map reg
		uint32 val = page[bank];
		bank = (bank + 1) & 7;
		return val;
	}
	return 0xff;
}

void MEMORY::write_signal(int id, uint32 data, uint32 mask)
{
#ifdef VRAM_WAIT
	if(id == SIG_MEMORY_HBLANK)
		hblank = (data & mask) ? true : false;
	else if(id == SIG_MEMORY_VBLANK)
		vblank = (data & mask) ? true : false;
	
	// if blank, disable busreq
	bool next = hblank || vblank;
	if(!blank && next && busreq) {
		d_cpu->write_signal(SIG_CPU_BUSREQ, 0, 1);
		busreq = false;
	}
	blank = next;
#endif
}

void MEMORY::set_map(uint8 data)
{
	int base = bank * 0x2000;
	
	page_wait[bank] = 0;
	is_vram[bank] = false;
	if(data <= 0x1f) {
		// main ram
		SET_BANK(base,  base + 0x1fff, ram + data * 0x2000, ram + data * 0x2000);
		page_type[bank] = PAGE_TYPE_NORMAL;
	}
	else if(0x20 <= data && data <= 0x2f) {
		// vram
		int ofs = 0;
		switch (data) {
			case 0x20: ofs = 0x0; break;
			case 0x21: ofs = 0x1; break;
			case 0x22: ofs = 0x4; break;
			case 0x23: ofs = 0x5; break;
			case 0x24: ofs = 0x8; break;
			case 0x25: ofs = 0x9; break;
			case 0x26: ofs = 0xc; break;
			case 0x27: ofs = 0xd; break;
			case 0x28: ofs = 0x2; break;
			case 0x29: ofs = 0x3; break;
			case 0x2a: ofs = 0x6; break;
			case 0x2b: ofs = 0x7; break;
			case 0x2c: ofs = 0xa; break;
			case 0x2d: ofs = 0xb; break;
			case 0x2e: ofs = 0xe; break;
			case 0x2f: ofs = 0xf; break;
		}
		SET_BANK(base,  base + 0x1fff, vram + ofs * 0x2000, vram + ofs * 0x2000);
		page_type[bank] = PAGE_TYPE_VRAM;
		page_wait[bank] = 1;
		is_vram[bank] = true;
	}
	else if(0x30 <= data && data <= 0x33) {
		// read modify write
		SET_BANK(base,  base + 0x1fff, wdmy, rdmy);
		page_type[bank] = PAGE_TYPE_MODIFY;
		page_wait[bank] = 2;
		is_vram[bank] = true;
	}
	else if(0x34 <= data && data <= 0x37) {
		// ipl rom
		SET_BANK(base,  base + 0x1fff, wdmy, ipl + (data - 0x34) * 0x2000);
		page_type[bank] = PAGE_TYPE_NORMAL;
	}
	else if(data == 0x38) {
		// text vram
		SET_BANK(base         ,  base + 0x17ff, tvram, tvram);
		SET_BANK(base + 0x1800,  base + 0x1fff, wdmy, rdmy);
		page_type[bank] = PAGE_TYPE_VRAM;
		page_wait[bank] = 1;
		is_vram[bank] = true;
	}
	else if(data == 0x39) {
		// kanji rom, pcg
		SET_BANK(base,  base + 0x1fff, pcg, pcg);
		if(kanji_bank & 0x80) {
			SET_BANK(base,  base + 0x7ff, wdmy, kanji + (kanji_bank & 0x7f) * 0x800);
		}
		page_type[bank] = PAGE_TYPE_KANJI;
		page_wait[bank] = 2;
	}
	else if(data == 0x3a) {
		// dictionary rom
		SET_BANK(base,  base + 0x1fff, wdmy, dic + dic_bank * 0x2000);
		page_type[bank] = PAGE_TYPE_DIC;
	}
	else if(0x3c <= data && data <= 0x3f) {
		// phone rom
		SET_BANK(base,  base + 0x1fff, wdmy, phone + (data - 0x3c) * 0x2000);
		page_type[bank] = PAGE_TYPE_NORMAL;
	}
	else {
		// n.c
		SET_BANK(base,  base + 0x1fff, wdmy, rdmy);
		page_type[bank] = PAGE_TYPE_NORMAL;
	}
	page[bank] = data;
	bank = (bank + 1) & 7;
}

