/*
	SHARP X1twin Emulator 'eX1twin'
	SHARP X1turbo Emulator 'eX1turbo'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ memory ]
*/

#include "memory.h"
#ifdef _X1TURBO
#include "../i8255.h"
#else
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

#ifdef _X1TURBO
#define ROM_FILE_SIZE	0x8000
#define ROM_FILE_NAME	_T("IPLROM.X1T")
#else
#define ROM_FILE_SIZE	0x1000
#define ROM_FILE_NAME	_T("IPLROM.X1")
#endif

void MEMORY::initialize()
{
	// init memory
	memset(rom, 0xff, sizeof(rom));
	memset(ram, 0, sizeof(ram));
	
	// load ipl
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sIPL.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(rom, ROM_FILE_SIZE, 1);
		fio->Fclose();
	}
	else {
		// xmillenium rom
		_stprintf(file_path, _T("%s%s"), app_path, ROM_FILE_NAME);
		if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
			fio->Fread(rom, ROM_FILE_SIZE, 1);
			fio->Fclose();
		}
	}
	delete fio;
#ifndef _X1TURBO
	for(int ofs = 0x1000; ofs < 0x8000; ofs += 0x1000) {
		memcpy(rom + ofs, rom, 0x1000);
	}
#endif
}

void MEMORY::reset()
{
	SET_BANK(0x0000, 0x7fff, ram, rom);
	SET_BANK(0x8000, 0xffff, ram + 0x8000, ram + 0x8000);
	romsel = 1;
#ifdef _X1TURBO
	bank = 0x10;
	d_pio->write_signal(SIG_I8255_PORT_B, 0x00, 0x10);
#else
	d_cpu->write_signal(SIG_Z80_M1_CYCLE_WAIT, 1, 1);
#endif
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xffff;
	wbank[addr >> 12][addr & 0xfff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xffff;
	return rbank[addr >> 12][addr & 0xfff];
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	bool update_map_required = false;
	
	switch(addr & 0xff00) {
#ifdef _X1TURBO
	case 0xb00:
		if((bank & 0x1f) != (data & 0x1f)) {
			update_map_required = true;
		}
		bank = data;
		break;
#endif
	case 0x1d00:
		if(!romsel) {
			romsel = 1;
			update_map_required = true;
#ifdef _X1TURBO
			d_pio->write_signal(SIG_I8255_PORT_B, 0x00, 0x10);
#else
			d_cpu->write_signal(SIG_Z80_M1_CYCLE_WAIT, 1, 1);
#endif
		}
		break;
	case 0x1e00:
		if(romsel) {
			romsel = 0;
			update_map_required = true;
#ifdef _X1TURBO
			d_pio->write_signal(SIG_I8255_PORT_B, 0x10, 0x10);
#else
			d_cpu->write_signal(SIG_Z80_M1_CYCLE_WAIT, 0, 0);
#endif
		}
		break;
	}
	if(update_map_required) {
#ifdef _X1TURBO
		if(!(bank & 0x10)) {
			uint8 *ptr = extram + 0x8000 * (bank & 0x0f);
			SET_BANK(0x0000, 0x7fff, ptr, ptr);
		}
		else
#endif
		if(romsel) {
			SET_BANK(0x0000, 0x7fff, ram, rom);
		}
		else {
			SET_BANK(0x0000, 0x7fff, ram, ram);
		}
	}
}

#ifdef _X1TURBO
uint32 MEMORY::read_io8(uint32 addr)
{
	switch(addr & 0xff00) {
	case 0xb00:
		return bank;
	}
	return 0xff;
}
#endif

