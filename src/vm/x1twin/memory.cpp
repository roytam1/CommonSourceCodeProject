/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ memory ]
*/

#include "memory.h"
#ifndef _X1TURBO
#include "../z80.h"
#endif
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 12, eb = (e) >> 12; \
	for(int i = sb; i <= eb; i++) { \
		wbank[i] = (w) + 0x1000 * (i - sb); \
		rbank[i] = (r) + 0x1000 * (i - sb); \
	} \
}

void MEMORY::initialize()
{
	// init memory
	_memset(rom, 0xff, sizeof(rom));
	_memset(ram, 0, sizeof(ram));
	
	// load ipl
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sIPL.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(rom, sizeof(rom), 1);
		fio->Fclose();
	}
	// xmillenium rom
	_stprintf(file_path, _T("%sIPLROM.X1"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(rom, sizeof(rom), 1);
		fio->Fclose();
	}
	delete fio;
}

void MEMORY::reset()
{
	for(int ofs = 0; ofs < 0x8000; ofs += 0x1000) {
		SET_BANK(ofs, ofs + 0x0fff, ram + ofs, rom);
	}
	SET_BANK(0x8000, 0xffff, ram + 0x8000, ram + 0x8000);
#ifndef _X1TURBO
	d_cpu->write_signal(SIG_Z80_M1_CYCLE_WAIT, 1, 1);
#endif
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	wbank[addr >> 12][addr & 0xfff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	return rbank[addr >> 12][addr & 0xfff];
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff00) {
	case 0x1d00:
		for(int ofs = 0; ofs < 0x8000; ofs += 0x1000) {
			SET_BANK(ofs, ofs + 0x0fff, ram + ofs, rom);
		}
#ifndef _X1TURBO
		d_cpu->write_signal(SIG_Z80_M1_CYCLE_WAIT, 1, 1);
#endif
		break;
	case 0x1e00:
		SET_BANK(0x0000, 0x7fff, ram, ram);
#ifndef _X1TURBO
		d_cpu->write_signal(SIG_Z80_M1_CYCLE_WAIT, 0, 0);
#endif
		break;
	}
}

