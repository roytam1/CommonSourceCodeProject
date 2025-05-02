/*
	TOSHIBA PASOPIA 7 Emulator 'EmuPIA7'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20 -

	[ memory ]
*/

#include "memory.h"
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
	// load ipl
	_memset(bios, 0xff, sizeof(bios));
	_memset(basic, 0xff, sizeof(basic));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sBIOS.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(bios, sizeof(bios), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sBASIC.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(basic, sizeof(basic), 1);
		fio->Fclose();
	}
	delete fio;
	
	SET_BANK(0x0000, 0x3fff, wdmy, bios);
	SET_BANK(0x4000, 0x7fff, wdmy, bios);
	SET_BANK(0x8000, 0xbfff, wdmy, bios);
	SET_BANK(0xc000, 0xffff, wdmy, bios);
	
	plane = 0;
	vram_sel = pal_sel = attr_wrap = false;
}

void MEMORY::reset()
{
	_memset(vram, 0, sizeof(vram));
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	if(vram_sel && (addr & 0xc000) == 0x8000) {
		if(pal_sel && !(plane & 0x70)) {
			pal[addr & 0xf] = data & 0xf;
			return;
		}
		uint32 laddr = addr & 0x3fff;
		if(plane & 0x10)
			vram[0x0000 | laddr] = (plane & 0x1) ? data : 0xff;
		if(plane & 0x20)
			vram[0x4000 | laddr] = (plane & 0x2) ? data : 0xff;
		if(plane & 0x40) {
			vram[0x8000 | laddr] = (plane & 0x4) ? data : 0xff;
			attr_latch = attr_wrap ? attr_latch : attr_data;
			vram[0xc000 | laddr] = attr_latch;
			// 8255-0, Port B
			dev_pio0->write_signal(dev_pio0_id, (attr_latch << 4) | (attr_latch & 7), 0x87);
		}
		return;
	}
	wbank[addr >> 12][addr & 0xfff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	if(vram_sel && (addr & 0xc000) == 0x8000) {
		if(pal_sel && !(plane & 0x70))
			return pal[addr & 0xf];
		uint32 laddr = addr & 0x3fff, val = 0xff;
		if((plane & 0x11) == 0x11)
			val &= vram[0x0000 | laddr];
		if((plane & 0x22) == 0x22)
			val &= vram[0x4000 | laddr];
		if((plane & 0x44) == 0x44) {
			attr_latch = vram[0xc000 | laddr];
			val &= vram[0x8000 | laddr];
			// 8255-0, Port B
			dev_pio0->write_signal(dev_pio0_id, (attr_latch << 4) | (attr_latch & 7), 0x87);
		}
		return val;
	}
	return rbank[addr >> 12][addr & 0xfff];
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	if(data & 1) {
		SET_BANK(0x0000, 0x3fff, ram + 0x0000, basic + 0x0000);
		SET_BANK(0x4000, 0x7fff, ram + 0x4000, bios + 0x0000);
	}
	else if(data & 2) {
		SET_BANK(0x0000, 0x3fff, ram + 0x0000, ram + 0x0000);
		SET_BANK(0x4000, 0x7fff, ram + 0x4000, ram + 0x4000);
	}
	else {
		SET_BANK(0x0000, 0x3fff, ram + 0x0000, basic + 0x0000);
		SET_BANK(0x4000, 0x7fff, ram + 0x4000, basic + 0x4000);
	}
	if(data & 4) {
		SET_BANK(0x8000, 0xbfff, wdmy, rdmy);
	}
	else {
		SET_BANK(0x8000, 0xbfff, ram + 0x8000, ram + 0x8000);
	}
	SET_BANK(0xc000, 0xffff, ram + 0xc000, ram + 0xc000);
	
	vram_sel = (data & 4) ? true : false;
	// I/O memory access
	if(data & 8)
		dev_io->write_signal(dev_io_id, 0xffffffff, 1);
	// 8255-2, Port C
	dev_pio2->write_signal(dev_pio2_id, data, 3);
}

void MEMORY::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_MEMORY_I8255_1_A)
		plane = data;
	else if(id == SIG_MEMORY_I8255_1_B)
		attr_data = data & 0xf;
	else if(id == SIG_MEMORY_I8255_1_C) {
		attr_wrap = (data & 0x10) ? true : false;
		pal_sel = (data & 0xc) ? true : false;
	}
}

