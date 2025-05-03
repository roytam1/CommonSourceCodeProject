/*
	SHARP MZ-5500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.10 -

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 14, eb = (e) >> 14; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) \
			wbank[i] = wdmy; \
		else \
			wbank[i] = (w) + 0x4000 * (i - sb); \
		if((r) == rdmy) \
			rbank[i] = rdmy; \
		else \
			rbank[i] = (r) + 0x4000 * (i - sb); \
	} \
}

void MEMORY::initialize()
{
	// init memory
	_memset(ram, 0, sizeof(ram));
	_memset(vram, 0, sizeof(vram));
	_memset(ipl, 0xff, sizeof(ipl));
	_memset(kanji, 0xff, sizeof(kanji));
	_memset(dic, 0xff, sizeof(dic));
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
	delete fio;
	
	// set memory bank
	SET_BANK(0x00000, 0x7ffff, ram, ram);
	SET_BANK(0x80000, 0x9ffff, wdmy, rdmy);	// aux
	SET_BANK(0xa0000, 0xbffff, wdmy, kanji);
	SET_BANK(0xc0000, 0xeffff, vram, vram);
	SET_BANK(0xf0000, 0xfbfff, wdmy, rdmy);	// aux
	SET_BANK(0xfc000, 0xfffff, wdmy, ipl);
	
	// init dmac
	haddr = 0;
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xfffff;
//	if((0x80000 <= addr && addr < 0xa0000) || (0xf0000 <= addr && addr < 0xfc000))
//		d_cpu->write_signal(SIG_CPU_NMI, 1, 1);
	wbank[addr >> 14][addr & 0x3fff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xfffff;
//	if((0x80000 <= addr && addr < 0xa0000) || (0xf0000 <= addr && addr < 0xfc000))
//		d_cpu->write_signal(SIG_CPU_NMI, 1, 1);
	return rbank[addr >> 14][addr & 0x3fff];
}

void MEMORY::write_dma8(uint32 addr, uint32 data)
{
	addr = (addr & 0xffff) | haddr;
//	if((0x80000 <= addr && addr < 0xa0000) || (0xf0000 <= addr && addr < 0xfc000))
//		d_cpu->write_signal(SIG_CPU_NMI, 1, 1);
	wbank[addr >> 14][addr & 0x3fff] = data;
}

uint32 MEMORY::read_dma8(uint32 addr)
{
	addr = (addr & 0xffff) | haddr;
//	if((0x80000 <= addr && addr < 0xa0000) || (0xf0000 <= addr && addr < 0xfc000))
//		d_cpu->write_signal(SIG_CPU_NMI, 1, 1);
	return rbank[addr >> 14][addr & 0x3fff];
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	// $50: DMAC high-order address latch
	haddr = (data & 0xf0) << 12;
}

void MEMORY::write_signal(int id, uint32 data, uint32 mask)
{
	switch(data & 0xe0)
	{
	case 0xe0:
		SET_BANK(0x0a0000, 0x0bffff, wdmy, kanji);
		break;
	case 0xc0:
		SET_BANK(0x0a0000, 0x0bffff, wdmy, kanji + 0x20000);
		break;
	case 0xa0:
		SET_BANK(0x0a0000, 0x0bffff, wdmy, dic);
		break;
	case 0x80:
		SET_BANK(0x0a0000, 0x0bffff, wdmy, dic + 0x20000);
		break;
	default:
		SET_BANK(0x0a0000, 0x0bffff, wdmy, rdmy);
		break;
	}
}
