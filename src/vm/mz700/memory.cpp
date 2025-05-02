/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	SHARP MZ-800 Emulator 'EmuZ-800'
	SHARP MZ-1500 Emulator 'EmuZ-1500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.05 -

	[ memory ]
*/

#include "memory.h"
#if defined(_MZ800)
#include "display.h"
#endif
#include "../i8253.h"
#include "../i8255.h"
#if defined(_MZ800)
#include "../z80pio.h"
#include "../../config.h"
#endif
#include "../../fileio.h"

#define EVENT_TEMPO		0
#define EVENT_BLINK		1
#define EVENT_HBLANK		2
#define EVENT_HSYNC_S		3
#define EVENT_HSYNC_E		4
#if defined(_MZ1500)
#define EVENT_HBLANK_PCG	5
#endif

#define MEM_BANK_MON_L		0x01
#define MEM_BANK_MON_H		0x02
#if defined(_MZ800)
#define MEM_BANK_CGROM_R	0x04
#define MEM_BANK_CGROM_W	0x08
#define MEM_BANK_CGROM		(MEM_BANK_CGROM_R | MEM_BANK_CGROM_W)
#define MEM_BANK_VRAM		0x10
#endif
#if defined(_MZ800) || defined(_MZ1500)
#define MEM_BANK_PCG		0x20
#endif

#if defined(_MZ800)
#define MZ700_MODE	(dmd & 8)
#endif

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

#if defined(_MZ800)
#define IPL_FILE_NAME	"MZ700IPL.ROM"
#define EXT_FILE_NAME	"MZ800IPL.ROM"
#else
#define IPL_FILE_NAME	"IPL.ROM"
#define EXT_FILE_NAME	"EXT.ROM"
#endif

void MEMORY::initialize()
{
	// init memory
	_memset(ipl, 0xff, sizeof(ipl));
	_memset(ram, 0, sizeof(ram));
	_memset(vram, 0, sizeof(vram));
#if defined(_MZ700) || defined(_MZ1500)
	_memset(vram + 0x800, 0x71, 0x400);
#endif
#if defined(_MZ800) || defined(_MZ1500)
	_memset(ext, 0xff, sizeof(ext));
#endif
#if defined(_MZ1500)
	_memset(pcg, 0, sizeof(pcg));
#endif
	_memset(font, 0, sizeof(font));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	// load rom image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%s%s"), app_path, _T(IPL_FILE_NAME));
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ipl, sizeof(ipl), 1);
		fio->Fclose();
	}
#if defined(_MZ800) || defined(_MZ1500)
	_stprintf(file_path, _T("%s%s"), app_path, _T(EXT_FILE_NAME));
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ext, sizeof(ext), 1);
		fio->Fclose();
	}
#endif
	_stprintf(file_path, _T("%sFONT.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(font, sizeof(font), 1);
		fio->Fclose();
	}
	delete fio;
	
	// init memory map
	SET_BANK(0x0000, 0xffff, ram, ram);
	
	// regist event
	vm->register_vline_event(this);
	vm->register_event_by_clock(this, EVENT_TEMPO, CPU_CLOCKS / 64, true, NULL);	// 32hz * 2
	vm->register_event_by_clock(this, EVENT_BLINK, CPU_CLOCKS / 3, true, NULL);	// 1.5hz * 2
}


void MEMORY::reset()
{
#if defined(_MZ800)
	// check dip-switch
	is_mz800 = (config.boot_mode == 0);
#endif
	
	// reset memory map
	mem_bank = MEM_BANK_MON_L | MEM_BANK_MON_H;
#if defined(_MZ800)
	// TODO: check initial params
	wf = rf = 0x01;
	dmd = 0x00;
	vram_addr_top = 0x9fff;
#elif defined(_MZ1500)
	pcg_bank = 0;
#endif
	update_map_low();
	update_map_middle();
	update_map_high();
	
	blink = tempo = false;
	vblank = vsync = true;
	hblank = hsync = true;
	
#if defined(_MZ700) || defined(_MZ1500)
	hblank_vram = true;
#if defined(_MZ1500)
	hblank_pcg = true;
#endif
#endif
	
	// motor is always rotating...
	d_pio->write_signal(SIG_I8255_PORT_C, 0xff, 0x10);
}

void MEMORY::event_vline(int v, int clock)
{
	// vblank / vsync
	set_vblank(v >= 200);
#if defined(_MZ800)
	vsync = (v >= 240 && v <= 242);
#else
	vsync = (v >= 221 && v <= 223);
#endif
	
	// hblank / hsync
	set_hblank(false);
#if defined(_MZ800)
	vm->register_event_by_clock(this, EVENT_HBLANK, 128, false, NULL);	// PAL 50Hz
	vm->register_event_by_clock(this, EVENT_HSYNC_S, 161, false, NULL);
	vm->register_event_by_clock(this, EVENT_HSYNC_E, 177, false, NULL);
#else
	vm->register_event_by_clock(this, EVENT_HBLANK, 165, false, NULL);	// NTSC 60Hz
//	vm->register_event_by_clock(this, EVENT_HSYNC_S, 180, false, NULL);
//	vm->register_event_by_clock(this, EVENT_HSYNC_E, 194, false, NULL);
#endif
	
#if defined(_MZ700) || defined(_MZ1500)
	// memory wait for vram
	hblank_vram = false;
#if defined(_MZ1500)
	// memory wait for pcg
	vm->register_event_by_clock(this, EVENT_HBLANK_PCG, 170, false, NULL);
	hblank_pcg = false;
#endif
#endif

#if defined(_MZ700) || defined(_MZ1500)
#endif
}

void MEMORY::event_callback(int event_id, int err)
{
	if(event_id == EVENT_TEMPO) {
		// 32KHz
		tempo = !tempo;
	}
	else if(event_id == EVENT_BLINK) {
		// 556 OUT (1.5KHz) -> 8255:PC6
		d_pio->write_signal(SIG_I8255_PORT_C, (blink = !blink) ? 0xff : 0, 0x40);
	}
	else if(event_id == EVENT_HBLANK) {
		set_hblank(true);
#if defined(_MZ700) || defined(_MZ1500)
		if(hblank_vram) {
			// wait because vram is accessed
			d_cpu->write_signal(SIG_CPU_BUSREQ, 0, 0);
		}
		hblank_vram = true;
#endif
	}
	else if(event_id == EVENT_HSYNC_S) {
		hsync = true;
	}
	else if(event_id == EVENT_HSYNC_E) {
		hsync = false;
	}
#if defined(_MZ1500)
	else if(event_id == EVENT_HBLANK_PCG) {
		if(hblank_pcg) {
			// wait because pcg is accessed
			d_cpu->write_signal(SIG_CPU_BUSREQ, 0, 0);
		}
		hblank_pcg = true;
	}
#endif
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xffff;
#if defined(_MZ800)
	// MZ-800
	if(MZ700_MODE) {
		if(0xe000 <= addr && addr <= 0xe00f && (mem_bank & MEM_BANK_MON_H)) {
			// memory mapped i/o
			switch(addr & 0x0f) {
			case 0: case 1: case 2: case 3:
				d_pio->write_io8(addr & 3, data);
				break;
			case 4: case 5: case 6: case 7:
				d_pit->write_io8(addr & 3, data);
				break;
			case 8:
				// 8253 gate0
//				d_pit->write_signal(SIG_I8253_GATE_0, data, 1);
				break;
			}
			return;
		}
	} else {
		if(0x8000 <= addr && addr <= vram_addr_top && (mem_bank & MEM_BANK_VRAM)) {
			addr &= 0x3fff;
			int page;
			switch(wf & 0xe0) {
			case 0x00:	// single write
				page = (dmd & 4) ? (wf & 5) : wf;
				for(int i = 0, bit = 1; i < 4; i++, bit <<= 1, addr += 0x2000) {
					if(page & bit) {
						vram[addr] = data;
					}
				}
				break;
			case 0x20:	// exor
				page = (dmd & 4) ? (wf & 5) : wf;
				for(int i = 0, bit = 1; i < 4; i++, bit <<= 1, addr += 0x2000) {
					if(page & bit) {
						vram[addr] ^= data;
					}
				}
				break;
			case 0x40:	// or
				page = (dmd & 4) ? (wf & 5) : wf;
				for(int i = 0, bit = 1; i < 4; i++, bit <<= 1, addr += 0x2000) {
					if(page & bit) {
						vram[addr] |= data;
					}
				}
				break;
			case 0x60:	// reset
				page = (dmd & 4) ? (wf & 5) : wf;
				for(int i = 0, bit = 1; i < 4; i++, bit <<= 1, addr += 0x2000) {
					if(page & bit) {
						vram[addr] &= ~data;
					}
				}
				break;
			case 0x80:	// replace
			case 0xa0:
				page = vram_page_mask(wf);
				for(int i = 0, bit = 1; i < 4; i++, bit <<= 1, addr += 0x2000) {
					if(page & bit) {
						if(wf & bit) {
							vram[addr] = data;
						}
						else {
							vram[addr] = 0;
						}
					}
				}
				break;
			case 0xc0:	// pset
			case 0xe0:
				page = vram_page_mask(wf);
				for(int i = 0, bit = 1; i < 4; i++, bit <<= 1, addr += 0x2000) {
					if(page & bit) {
						if(wf & bit) {
							vram[addr] |= data;
						}
						else {
							vram[addr] &= ~data;
						}
					}
				}
				break;
			}
			return;
		}
	}
#else
	// MZ-700/1500
#if defined(_MZ1500)
	if(mem_bank & MEM_BANK_PCG) {
		if(0xd000 <= addr && addr <= 0xefff) {
			// pcg wait
			if(!hblank_pcg) {
				d_cpu->write_signal(SIG_CPU_BUSREQ, 1, 1);
				hblank_pcg = true;
			}
		}
	}
	else {
#endif
		if(mem_bank & MEM_BANK_MON_H) {
			if(0xd000 <= addr && addr <= 0xdfff) {
				// vram wait
				if(!hblank_vram) {
					d_cpu->write_signal(SIG_CPU_BUSREQ, 1, 1);
					hblank_vram = true;
				}
			}
			else if(0xe000 <= addr && addr <= 0xe00f) {
				// memory mapped i/o
				switch(addr & 0x0f) {
				case 0: case 1: case 2: case 3:
					d_pio->write_io8(addr & 3, data);
					break;
				case 4: case 5: case 6: case 7:
					d_pit->write_io8(addr & 3, data);
					break;
				case 8:
					// 8253 gate0
					d_pit->write_signal(SIG_I8253_GATE_0, data, 1);
					break;
				}
				return;
			}
		}
#if defined(_MZ1500)
	}
#endif
#endif
	wbank[addr >> 11][addr & 0x7ff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xffff;
#if defined(_MZ800)
	// MZ-800
	if(MZ700_MODE) {
		if(0xe000 <= addr && addr <= 0xe00f && (mem_bank & MEM_BANK_MON_H)) {
			// memory mapped i/o
			switch(addr & 0x0f) {
			case 0: case 1: case 2: case 3:
				return d_pio->read_io8(addr & 3);
			case 4: case 5: case 6: case 7:
				return d_pit->read_io8(addr & 3);
			case 8:
				return (hblank ? 0 : 0x80) | (tempo ? 1 : 0) | 0x7e;
			}
			return 0xff;
		}
	} else {
		if(0x8000 <= addr && addr <= vram_addr_top && (mem_bank & MEM_BANK_VRAM)) {
			addr &= 0x3fff;
			if(rf & 0x80) {
				int page = vram_page_mask(rf);
				uint32 result = 0xff;
				for(int bit2 = 1; bit2 <= 0x80; bit2 <<= 1) {
					uint32 addr2 = addr;
					for(int i = 0, bit = 1; i < 4; i++, bit <<= 1, addr2 += 0x2000) {
						if((page & bit) && (vram[addr2] & bit2) != ((rf & bit) ? bit2 : 0)) {
							result &= ~bit2;
							break;
						}
					}
				}
				return result;
			}
			else {
				int page = vram_page_mask(rf) & rf;
				for(int i = 0, bit = 1; i < 4; i++, bit <<= 1, addr += 0x2000) {
					if(page & bit) {
						return vram[addr];
					}
				}
			}
			return 0xff;
		}
	}
#else
	// MZ-700/1500
#if defined(_MZ1500)
	if(mem_bank & MEM_BANK_PCG) {
		if(0xd000 <= addr && addr <= 0xefff) {
			// pcg wait
			if(!hblank_pcg) {
				d_cpu->write_signal(SIG_CPU_BUSREQ, 1, 1);
				hblank_pcg = true;
			}
		}
	}
	else {
#endif
		if(mem_bank & MEM_BANK_MON_H) {
			if(0xd000 <= addr && addr <= 0xdfff) {
				// vram wait
				if(!hblank_vram) {
					d_cpu->write_signal(SIG_CPU_BUSREQ, 1, 1);
					hblank_vram = true;
				}
			}
			else if(0xe000 <= addr && addr <= 0xe00f) {
				// memory mapped i/o
				switch(addr & 0x0f) {
				case 0: case 1: case 2: case 3:
					return d_pio->read_io8(addr & 3);
				case 4: case 5: case 6: case 7:
					return d_pit->read_io8(addr & 3);
				case 8:
					return (hblank ? 0 : 0x80) | (tempo ? 1 : 0) | 0x7e;
				}
				return 0xff;
			}
		}
#if defined(_MZ1500)
	}
#endif
#endif
	return rbank[addr >> 11][addr & 0x7ff];
}

void MEMORY::write_data8w(uint32 addr, uint32 data, int* wait)
{
	*wait = ((mem_bank & MEM_BANK_MON_L) && addr < 0x1000) ? 1 : 0;
	write_data8(addr, data);
}

uint32 MEMORY::read_data8w(uint32 addr, int* wait)
{
	*wait = ((mem_bank & MEM_BANK_MON_L) && addr < 0x1000) ? 1 : 0;
	return read_data8(addr);
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
#if defined(_MZ800)
	case 0xcc:
		wf = data;
		break;
	case 0xcd:
		rf = data;
		break;
	case 0xce:
		vram_addr_top = (data & 4) ? 0xbfff : 0x9fff;
		dmd = data;
		d_display->write_dmd(dmd);
		update_map_middle();
		break;
#endif
	case 0xe0:
		mem_bank &= ~MEM_BANK_MON_L;
#if defined(_MZ800)
		mem_bank &= ~MEM_BANK_CGROM;
#endif
		update_map_low();
		update_map_middle();
		break;
	case 0xe1:
		mem_bank &= ~MEM_BANK_MON_H;
		update_map_high();
		break;
	case 0xe2:
		mem_bank |= MEM_BANK_MON_L;
		update_map_low();
		break;
	case 0xe3:
		mem_bank |= MEM_BANK_MON_H;
		update_map_high();
		break;
	case 0xe4:
		mem_bank |= MEM_BANK_MON_L | MEM_BANK_MON_H;
#if defined(_MZ800)
		mem_bank &= ~MEM_BANK_CGROM_R;
		mem_bank |= MEM_BANK_CGROM_W | MEM_BANK_VRAM;
#elif defined(_MZ1500)
		mem_bank &= ~MEM_BANK_PCG;
#endif
		update_map_low();
		update_map_middle();
		update_map_high();
		break;
#if defined(_MZ800) || defined(_MZ1500)
	case 0xe5:
		mem_bank |= MEM_BANK_PCG;
#if defined(_MZ1500)
		pcg_bank = data;
#endif
		update_map_high();
		break;
	case 0xe6:
		mem_bank &= ~MEM_BANK_PCG;
		update_map_high();
		break;
#endif
	}
}

#if defined(_MZ800)
uint32 MEMORY::read_io8(uint32 addr)
{
	switch(addr & 0xff) {
	case 0xce:
		return (hblank ? 0 : 0x80) | (vblank ? 0 : 0x40) | (hsync ? 0 : 0x20) | (vsync ? 0 : 0x10) | (is_mz800 ? 2 : 0) | (tempo ? 1 : 0) | 0x0c;
	case 0xe0:
		mem_bank &= ~MEM_BANK_CGROM_W;
		mem_bank |= MEM_BANK_CGROM_R | MEM_BANK_VRAM;
		update_map_middle();
		break;
	case 0xe1:
		mem_bank &= ~(MEM_BANK_CGROM | MEM_BANK_VRAM);
		update_map_middle();
		break;
	}
	return 0xff;
}
#endif

void MEMORY::set_vblank(bool val)
{
	if(vblank != val) {
		// VBLANK -> 8255:PC7
		d_pio->write_signal(SIG_I8255_PORT_C, val ? 0 : 0xff, 0x80);
		vblank = val;
	}
}

void MEMORY::set_hblank(bool val)
{
	if(hblank != val) {
#if defined(_MZ800)
		// HBLANK -> Z80PIO:PA5
		d_pio_int->write_signal(SIG_Z80PIO_PORT_A, val ? 0 : 0xff, 0x20);
#endif
		hblank = val;
	}
}

void MEMORY::update_map_low()
{
	if(mem_bank & MEM_BANK_MON_L) {
		SET_BANK(0x0000, 0x0fff, wdmy, ipl);
	}
	else {
		SET_BANK(0x0000, 0x0fff, ram, ram);
	}
}

void MEMORY::update_map_middle()
{
#if defined(_MZ800)
	if(MZ700_MODE) {
		if(mem_bank & MEM_BANK_CGROM_R) {
			SET_BANK(0x1000, 0x1fff, wdmy, font);
			SET_BANK(0xc000, 0xcfff, vram + 0x2000, vram + 0x2000);
		}
		else {
			SET_BANK(0x1000, 0x1fff, ram + 0x1000, ram + 0x1000);
			SET_BANK(0xc000, 0xcfff, ram + 0xc000, ram + 0xc000);
		}
	} else {
		if(mem_bank & MEM_BANK_CGROM) {
			SET_BANK(0x1000, 0x1fff, wdmy, font);
		}
		else {
			SET_BANK(0x1000, 0x1fff, ram + 0x1000, ram + 0x1000);
		}
		SET_BANK(0xc000, 0xcfff, ram + 0xc000, ram + 0xc000);
	}
#endif
}

void MEMORY::update_map_high()
{
#if defined(_MZ800)
	// MZ-800
	if(MZ700_MODE) {
		if(mem_bank & MEM_BANK_PCG) {
			SET_BANK(0xd000, 0xffff, wdmy, rdmy);
		}
		else if(mem_bank & MEM_BANK_MON_H) {
			SET_BANK(0xd000, 0xdfff, vram + 0x3000, vram + 0x3000);
			SET_BANK(0xe000, 0xffff, wdmy, ext);
		}
		else {
			SET_BANK(0xd000, 0xffff, ram + 0xd000, ram + 0xd000);
		}
	}
	else {
		SET_BANK(0xd000, 0xdfff, ram + 0xd000, ram + 0xd000);
		if(mem_bank & MEM_BANK_PCG) {
			SET_BANK(0xe000, 0xffff, wdmy, rdmy);
		}
		else if(mem_bank & MEM_BANK_MON_H) {
			SET_BANK(0xe000, 0xffff, wdmy, ext);
		}
		else {
			SET_BANK(0xe000, 0xffff, ram + 0xe000, ram + 0xe000);
		}
	}
#else
	// MZ-700/1500
#if defined(_MZ1500)
	if(mem_bank & MEM_BANK_PCG) {
		if(pcg_bank & 3) {
			uint8 *bank = pcg + ((pcg_bank & 3) - 1) * 0x2000;
			SET_BANK(0xd000, 0xefff, bank, bank);
		}
		else {
			SET_BANK(0xd000, 0xdfff, wdmy, font);	// read only
			SET_BANK(0xe000, 0xefff, wdmy, font);
		}
		SET_BANK(0xf000, 0xffff, wdmy, rdmy);
	}
	else {
#endif
		if(mem_bank & MEM_BANK_MON_H) {
			SET_BANK(0xd000, 0xdfff, vram, vram);
#if defined(_MZ1500)
			SET_BANK(0xe000, 0xe7ff, wdmy, rdmy);
			SET_BANK(0xe800, 0xffff, wdmy, ext );
#else
			SET_BANK(0xe000, 0xffff, wdmy, rdmy);
#endif
		}
		else {
			SET_BANK(0xd000, 0xffff, ram + 0xd000, ram + 0xd000);
		}
#if defined(_MZ1500)
	}
#endif
#endif
}

#if defined(_MZ800)
int MEMORY::vram_page_mask(uint8 f)
{
	switch(dmd & 7) {
	case 0:	// 320x200,4col
	case 1:
		return (f & 0x10) ? (4 + 8) : (1 + 2);
	case 2:	// 320x200,16col
		return (1 + 2 + 4 + 8);
	case 4:	// 640x200,2col
	case 5:
		return (f & 0x10) ? 4 : 1;
	case 6:	// 640x200,4col
		return (1 + 4);
	}
	return 0;
}
#endif

