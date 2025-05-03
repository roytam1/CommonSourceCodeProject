/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.05 -

	[ memory ]
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
//	_memset(vram, 0, sizeof(vram));
	_memset(vram, 0, 0x800);
	_memset(vram + 0x800, 0x71, 0x800);
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
	
	// regist event
	vm->regist_vsync_event(this);
	int id;
	vm->regist_event_by_clock(this, EVENT_TEMPO, CPU_CLOCKS / 64, true, &id);	// 32hz * 2
	vm->regist_event_by_clock(this, EVENT_BLINK, CPU_CLOCKS / 3, true, &id);	// 1.5hz * 2
}

void MEMORY::reset()
{
	inh = inhbak = 3;
	update_map();
	hblank = tempo = 0;
	blink = false;
	// motor is always rotating...
	d_pio->write_signal(did_pio, 0xff, 0x10);
}

void MEMORY::event_vsync(int v, int clock)
{
	// vblank
	if(v == 0)
		d_pio->write_signal(did_pio, 0xff, 0x80);
	else if(v == 200)
		d_pio->write_signal(did_pio, 0, 0x80);
	
	// hblank
	hblank = 0x80;
	int id;
	vm->regist_event_by_clock(this, EVENT_HBLANK, 160, false, &id);
}

void MEMORY::event_callback(int event_id, int err)
{
	if(event_id == EVENT_HBLANK) {
		hblank = 0;
		d_cpu->write_signal(SIG_CPU_BUSREQ, 0, 0);
	}
	else if(event_id == EVENT_TEMPO)	// 32khz
		tempo ^= 1;
	else if(event_id == EVENT_BLINK)	// 1.5khz
		d_pio->write_signal(did_pio, (blink = !blink) ? 0xff : 0, 0x40);
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	if(inh & 2) {
		if(0xd000 <= addr && addr <= 0xdfff) {
			// vram wait
			if(hblank)
				d_cpu->write_signal(SIG_CPU_BUSREQ, 1, 1);
		}
		else if(0xe000 <= addr && addr <= 0xffff) {
			// memory mapped i/o
			switch(addr & 0xf)
			{
			case 0: case 1: case 2: case 3:
				d_pio->write_io8(addr & 3, data);
				break;
			case 4: case 5: case 6: case 7:
				d_ctc->write_io8(addr & 3, data);
				break;
			case 8:
				// 8253 gate0
				d_ctc->write_signal(did_ctc, data, 1);
				break;
			}
			return;
		}
	}
	wbank[addr >> 11][addr & 0x7ff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	if(inh & 2) {
		if(0xd000 <= addr && addr <= 0xdfff) {
			// vram wait
			if(hblank)
				d_cpu->write_signal(SIG_CPU_BUSREQ, 1, 1);
		}
		else if(0xe000 <= addr && addr <= 0xffff) {
			// memory mapped i/o
			switch(addr & 0xf)
			{
			case 0: case 1: case 2: case 3:
				return d_pio->read_io8(addr & 3);
			case 4: case 5: case 6: case 7:
				return d_ctc->read_io8(addr & 3);
			case 8:
				return hblank | tempo | 0x7e;
			}
			return 0xff;
		}
	}
	return rbank[addr >> 11][addr & 0x7ff];
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0xe0:
		inh &= ~1;
		update_map();
		break;
	case 0xe1:
		inh &= ~2;
		update_map();
		break;
	case 0xe2:
		inh |= 1;
		update_map();
		break;
	case 0xe3:
		inh |= 2;
		update_map();
		break;
	case 0xe4:
		inh |= 3;
		update_map();
		break;
	case 0xe5:
		inhbak = inh;
		break;
	case 0xe6:
		inh = inhbak;
		update_map();
		break;
	}
}

void MEMORY::update_map()
{
	if(inh & 1) {
		SET_BANK(0x0000, 0x0fff, wdmy, ipl);
	}
	else {
		SET_BANK(0x0000, 0x0fff, ram, ram);
	}
	SET_BANK(0x1000, 0xcfff, ram + 0x1000, ram + 0x1000);
	if(inh & 2) {
		SET_BANK(0xd000, 0xdfff, vram, vram);
		SET_BANK(0xe000, 0xffff, wdmy, rdmy);
	}
	else {
		SET_BANK(0xd000, 0xffff, ram + 0xd000, ram + 0xd000);
	}
}

