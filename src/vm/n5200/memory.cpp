/*
	NEC N5200 Emulator 'eN5200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.06.03-

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 11, eb = (e) >> 11; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) { \
			wbank[i] = wdmy; \
		} \
		else { \
			wbank[i] = (w) + 0x800 * (i - sb); \
		} \
		if((r) == rdmy) { \
			rbank[i] = rdmy; \
		} \
		else { \
			rbank[i] = (r) + 0x800 * (i - sb); \
		} \
	} \
}

void MEMORY::initialize()
{
	// init memory
	_memset(ram, 0, sizeof(ram));
	_memset(exram, 0, sizeof(exram));
	_memset(vram, 0, sizeof(vram));
	_memset(tvram, 0, sizeof(tvram));
	_memset(backup, 0, sizeof(backup));
	_memset(ipl, 0xff, sizeof(ipl));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	// load rom/ram image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sIPL.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ipl, sizeof(ipl), 1);
		fio->Fclose();

for(int i = 0xa8e; i < 0xafc; i++) ipl[i] = 0x90;

	}
	delete fio;
}

void MEMORY::release()
{
	// save ram image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sTVRAM.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(tvram, sizeof(tvram), 1);
		fio->Fclose();
	}
	delete fio;
}

void MEMORY::reset()
{
	SET_BANK(0x000000, 0xffffff, wdmy, rdmy);
	SET_BANK(0x000000, 0x0bffff, ram, ram);
	SET_BANK(0x0c0000, 0x0dffff, vram, vram);	// ???
	SET_BANK(0x0e0000, 0x0e77ff, tvram, tvram);
	SET_BANK(0x0e7800, 0x0effff, backup, backup);
	SET_BANK(0x0f0000, 0x0fffff, wdmy, ipl);
	SET_BANK(0x100000, 0x1fffff, exram, exram);
	SET_BANK(0xff0000, 0xffffff, wdmy, ipl);
	
	protect = true;
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xffffff;
//emu->out_debug("PC=%6x: WM %6x,%2x\n",vm->get_prv_pc(),addr,data);
	if(0xe7800 <= addr && addr < 0xf0000 && protect) {
		return;
	}
	wbank[addr >> 11][addr & 0x7ff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xffffff;
//if(addr<0xf0000)emu->out_debug("PC=%6x: RM %6x\n",vm->get_prv_pc(),addr);
	return rbank[addr >> 11][addr & 0x7ff];
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	switch(addr) {
	case 0x74:
		protect = ((data & 1) != 0);
		break;
	}
}

uint32 MEMORY::read_io8(uint32 addr)
{
	return 0xff;
}

