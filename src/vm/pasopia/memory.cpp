/*
	TOSHIBA PASOPIA Emulator 'EmuPIA'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.28 -

	[ memory ]
*/

#include "memory.h"
#include "../i8255.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 12, eb = (e) >> 12; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) { \
			wbank[i] = wdmy; \
		} \
		else { \
			wbank[i] = (w) + 0x1000 * (i - sb); \
		} \
		if((r) == rdmy) { \
			rbank[i] = rdmy; \
		} \
		else { \
			rbank[i] = (r) + 0x1000 * (i - sb); \
		} \
	} \
}

void MEMORY::initialize()
{
	// load ipl
	memset(rom, 0xff, sizeof(rom));
	memset(rdmy, 0xff, sizeof(rdmy));
	
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sOABASIC.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(rom, sizeof(rom), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sTBASIC.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(rom, sizeof(rom), 1);
		fio->Fclose();
	}
	delete fio;
	
	// init memory map
	SET_BANK(0x0000, 0x7fff, ram + 0x0000, rom + 0x0000);
	SET_BANK(0x8000, 0xffff, ram + 0x8000, ram + 0x8000);
	vram_ptr = 0;
	vram_data = memmap = 0;
}

void MEMORY::reset()
{
	memset(vram, 0, sizeof(vram));
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
	memmap = data;
	
	if(memmap & 4) {
		vm->reset();
	}
	if(memmap & 2) {
		SET_BANK(0x0000, 0x7fff, ram, ram);
	}
	else {
		SET_BANK(0x0000, 0x7fff, ram, rom);
	}
	// to 8255-2 port-c, bit2
	d_pio2->write_signal(SIG_I8255_PORT_C, (memmap & 2) ? 4 : 0, 4);
}

void MEMORY::write_signal(int id, uint32 data, uint32 mask)
{
	// vram control
	if(id == SIG_MEMORY_I8255_0_A) {
		vram_ptr = (vram_ptr & 0xff00) | (data & 0xff);
		// to 8255-0 port-c
		d_pio0->write_signal(SIG_I8255_PORT_C, vram[vram_ptr & 0x3fff], 0xff);
		// to 8255-1 port-b, bit7
		d_pio1->write_signal(SIG_I8255_PORT_B, attr[vram_ptr & 0x3fff] ? 0x80 : 0, 0x80);
	}
	else if(id == SIG_MEMORY_I8255_0_B) {
		vram_data = data & 0xff;
	}
	else if(id == SIG_MEMORY_I8255_1_C) {
		if((data & 0x40) && !(vram_ptr & 0x4000)) {
			vram[vram_ptr & 0x3fff] = vram_data;
			attr[vram_ptr & 0x3fff] = (vram_ptr & 0x8000) ? 1 : 0;
		}
		vram_ptr = (vram_ptr & 0xff) | ((data & 0xff) << 8);
		// to 8255-0 port-c
		d_pio0->write_signal(SIG_I8255_PORT_C, vram[vram_ptr & 0x3fff], 0xff);
		// to 8255-1 port-b, bit7
		d_pio1->write_signal(SIG_I8255_PORT_B, attr[vram_ptr & 0x3fff] ? 0x80 : 0, 0x80);
	}
}

