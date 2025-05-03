/*
	NEC PC-100 Emulator 'ePC-100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.07.14 -

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
	delete fio;
	
	// set memory map
	SET_BANK(0x00000, 0xbffff, ram, ram);
	SET_BANK(0xc0000, 0xdffff, vram, vram);
	SET_BANK(0xe0000, 0xf7fff, wdmy, rdmy);
	SET_BANK(0xf8000, 0xfffff, wdmy, ipl);
	
	// init bit control
	shift = 0;
	maskl = maskh = busl = bush = 0;
	
	// init vram plane
	write_plane = 1;
	read_plane = 0;
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xfffff;
	if(addr & 1)
		bush = data;
	else
		busl = data;
	if(0xc0000 <= addr && addr < 0xe0000) {
		uint32 bus = busl | (bush << 8) | (busl << 16) | (bush << 24);
		bus >>= shift;
		if(addr & 1) {
			uint32 h = (bus >> 8) & 0xff;
			for(int pl = 0; pl < 4; pl++) {
				if(write_plane & (1 << pl)) {
					int ofsh = (addr & 0x1ffff) | (0x20000 * pl);
					vram[ofsh] = (vram[ofsh] & maskh) | (h & ~maskh);
				}
			}
		}
		else {
			uint32 l = bus & 0xff;
			for(int pl = 0; pl < 4; pl++) {
				if(write_plane & (1 << pl)) {
					int ofsl = (addr & 0x1ffff) | (0x20000 * pl);
					vram[ofsl] = (vram[ofsl] & maskl) | (l & ~maskl);
				}
			}
		}
		return;
	}
	wbank[addr >> 14][addr & 0x3fff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xfffff;
	if(0xc0000 <= addr && addr < 0xe0000)
		return vram[(addr & 0x1ffff) | (0x20000 * read_plane)];
	return rbank[addr >> 14][addr & 0x3fff];
}

void MEMORY::write_data16(uint32 addr, uint32 data)
{
	addr &= 0xfffff;
	busl = addr & 1 ? data >> 8 : data & 0xff;
	bush = addr & 1 ? data & 0xff : data >> 8;
	if(0xc0000 <= addr && addr < 0xe0000) {
		uint32 bus = busl | (bush << 8) | (busl << 16) | (bush << 24);
		bus >>= shift;
		uint32 l = bus & 0xff;
		uint32 h = (bus >> 8) & 0xff;
		for(int pl = 0; pl < 4; pl++) {
			if(write_plane & (1 << pl)) {
				int ofsl = ((addr & 1 ? addr + 1 : addr) & 0x1ffff) | (0x20000 * pl);
				int ofsh = ((addr & 1 ? addr : addr + 1) & 0x1ffff) | (0x20000 * pl);
				vram[ofsl] = (vram[ofsl] & maskl) | (l & ~maskl);
				vram[ofsh] = (vram[ofsh] & maskh) | (h & ~maskh);
			}
		}
		return;
	}
	wbank[addr >> 14][addr & 0x3fff] = data & 0xff;
	addr = (addr + 1) & 0xfffff;
	wbank[addr >> 14][addr & 0x3fff] = data >> 8;
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	// $30: BIT SHIFT
	shift = data & 0xf;
}

uint32 MEMORY::read_io8(uint32 addr)
{
	// $30: BIT SHIFT
	return shift;
}

void MEMORY::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_MEMORY_BITMASK_LOW) {
		// $18: 8255 PA
		maskl = data & 0xff;
	}
	else if(id == SIG_MEMORY_BITMASK_HIGH) {
		// $1A: 8255 PB
		maskh = data & 0xff;
	}
	else if(id == SIG_MEMORY_VRAM_PLANE) {
		// $1C: 8255 PC
		write_plane = data & 0xf;
		read_plane = (data >> 4) & 3;
	}
}
