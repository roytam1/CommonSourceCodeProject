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
	_memset(ipl, 0xff, sizeof(ipl));
	_memset(ram, 0, sizeof(ram));
	_memset(common, 0, sizeof(common));
	_memset(basic, 0xff, sizeof(basic));
	_memset(ext, 0xff, sizeof(ext));
	_memset(rdmy, 0xff, sizeof(rdmy));
	knj = false;
	
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
		// MZ-1R06
		fio->Fread(ram + 0x20000, 0x20000, 1);
		fio->Fclose();
		knj = true;
	}
	_stprintf(file_path, _T("%sBASIC.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(basic, sizeof(basic), 1);
		fio->Fclose();
	}
	delete fio;
}

void MEMORY::reset()
{
	ms = ma = mo = 0;
	update_bank();
	e1 = 0x80;
	intmfd = int0 = int1 = int2 = int3 = int4 = false;
	srdy = sack = false;
	inp = 0;
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xffff;
	wbank[addr >> 11][addr & 0x7ff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xffff;
	return rbank[addr >> 11][addr & 0x7ff];
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0xec:
	case 0xed:
	case 0xee:
	case 0xef:
		// reset interrupt from sub cpu
		int0 = false;
		break;
	case 0xfc:
		e1 = data;
		update_intr();
		// set or reset busreq of sub cpu
		emu->out_debug("e1=%2x, ms=%2x\n",e1,ms);
		if(!(e1 & 2) && (ms & 0x80)) {
			emu->out_debug(_T("SUB BUSREQ=OFF\n"));
			d_sub->write_signal(SIG_CPU_BUSREQ, 0, 1);
		}
		else {
			emu->out_debug(_T("SUB BUSREQ=ON\n"));
			d_sub->write_signal(SIG_CPU_BUSREQ, 1, 1);
		}
		break;
	case 0xfd:
		ms = data;
		update_bank();
		// reset sub cpu
		emu->out_debug("e1=%2x, ms=%2x\n",e1,ms);
		if(!(data & 0x80)) {
			emu->out_debug(_T("SUB RESET\n"));
			d_sub->reset();
		}
		// set or reset busreq of sub cpu
		if(!(e1 & 2) && (ms & 0x80)) {
			emu->out_debug(_T("SUB BUSREQ=OFF\n"));
			d_sub->write_signal(SIG_CPU_BUSREQ, 0, 1);
		}
		else {
			emu->out_debug(_T("SUB BUSREQ=ON\n"));
			d_sub->write_signal(SIG_CPU_BUSREQ, 1, 1);
		}
		break;
	case 0xfe:
		mo = data & 7;
		ma = data >> 4;
		update_bank();
		break;
	case 0xff:
		// ME1, ME2
		me = data & 3;
		break;
	}
}

uint32 MEMORY::read_io8(uint32 addr)
{
	switch(addr & 0xff)
	{
	case 0xfd:
		return ms;
	case 0xfe:
		// dipswitch:
		//	bit4	L	SW4: Period for decimal point
		//	bit3	L	SW3: High Resolution CRT
		//	bit2	H	SW2: MZ1P02
		//	bit1	L	SW1: MZ1P02
		//	bit0	L	SEC: Optional double-side minifloppy
		return 0xe4;
	case 0xff:
		// dipswitch:
		//	bit7	L	FD3: Optional double-side minifloppy
		//	bit6	H	FD2: Optional double-side minifloppy
		//	bit5	H	SW8: Small Letter
		return 0x60 | (srdy ? 0x10 : 0) | (sack ? 0 : 8) | (inp & 7);
	}
	return 0xff;
}

void MEMORY::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_MEMORY_SRDY) {
		srdy = ((data & mask) != 0);
if(srdy)
emu->out_debug("\tSRDY = ON\n");
else
emu->out_debug("\tSRDY = OFF\n");
		return;
	}
	else if(id == SIG_MEMORY_SACK) {
		sack = ((data & mask) != 0);
		return;
	}
	else if(id == SIG_MEMORY_INTMFD)
		intmfd = ((data & mask) != 0);
	else if(id == SIG_MEMORY_INT0)
{
		int0 = ((data & mask) != 0);
if(int0)
emu->out_debug("\tINT0 = ON\n");
else
emu->out_debug("\tINT0 = OFF\n");
}
	else if(id == SIG_MEMORY_INT1)
		int1 = ((data & mask) != 0);
	else if(id == SIG_MEMORY_INT2)
		int2 = ((data & mask) != 0);
	else if(id == SIG_MEMORY_INT3)
		int3 = ((data & mask) != 0);
	else if(id == SIG_MEMORY_INT4)
		int4 = ((data & mask) != 0);
	else
		return;
	update_intr();
}

void MEMORY::update_bank()
{
	SET_BANK(0x0000, 0xffff, wdmy, rdmy);
	
	if((ms & 3) == 0) {
		// sd0: initialize state
		SET_BANK(0x0000, 0x0fff, wdmy, ipl + 0x1000);
		SET_BANK(0x1000, 0x1fff, wdmy, ipl + 0x1000);
		SET_BANK(0x2000, 0x3fff, wdmy, basic + 0x2000);
		SET_BANK(0x4000, 0xbfff, ram + 0x4000, ram + 0x4000);
		switch(ma)
		{
		case 0x0: SET_BANK(0xc000, 0xffff, ram + 0xc000, ram + 0xc000); break;
		case 0x1: SET_BANK(0xc000, 0xffff, ram + 0x0000, ram + 0x0000); break;
		case 0xf: SET_BANK(0xf800, 0xffff, common, common); break;
		}
	}
	else if((ms & 3) == 1) {
		// sd1: system loading and cp/m
		SET_BANK(0x0000, 0xf7ff, ram, ram);
		SET_BANK(0xf800, 0xffff, common, common);
	}
	else if((ms & 3) == 2) {
		// sd2: rom based basic
		SET_BANK(0x0000, 0x1fff, wdmy, basic);
		switch(mo)
		{
		case 0x0: SET_BANK(0x2000, 0x3fff, wdmy, basic + 0x2000); break;
		case 0x1: SET_BANK(0x2000, 0x3fff, wdmy, basic + 0x4000); break;
		case 0x2: SET_BANK(0x2000, 0x3fff, wdmy, basic + 0x6000); break;
		case 0x3: SET_BANK(0x2000, 0x3fff, wdmy, ext + 0x0000); break;
		case 0x4: SET_BANK(0x2000, 0x3fff, wdmy, ext + 0x2000); break;
		case 0x5: SET_BANK(0x2000, 0x3fff, wdmy, ext + 0x4000); break;
		case 0x6: SET_BANK(0x2000, 0x3fff, wdmy, ext + 0x6000); break;
		}
		SET_BANK(0x4000, 0xbfff, ram + 0x4000, ram + 0x4000);
		switch(ma)
		{
		case 0x0: SET_BANK(0xc000, 0xffff, ram + 0x0c000, ram + 0x0c000); break;
		case 0x1: SET_BANK(0xc000, 0xffff, ram + 0x00000, ram + 0x00000); break;
		case 0x2: SET_BANK(0xc000, 0xffff, ram + 0x10000, ram + 0x10000); break;
		case 0x3: SET_BANK(0xc000, 0xffff, ram + 0x14000, ram + 0x14000); break;
		case 0x4: SET_BANK(0xc000, 0xffff, ram + 0x18000, ram + 0x18000); break;
		case 0x5: SET_BANK(0xc000, 0xffff, ram + 0x1c000, ram + 0x1c000); break;
		case 0x6: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x20000, ram + 0x20000); break;
		case 0x7: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x24000, ram + 0x24000); break;
		case 0x8: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x28000, ram + 0x28000); break;
		case 0x9: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x2c000, ram + 0x2c000); break;
		case 0xa: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x30000, ram + 0x30000); break;
		case 0xb: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x34000, ram + 0x34000); break;
		case 0xc: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x38000, ram + 0x38000); break;
		case 0xd: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x3c000, ram + 0x3c000); break;
		case 0xf: SET_BANK(0xf800, 0xffff, common, common); break;
		}
	}
	else {
		// sd3: ram based basic
		SET_BANK(0x0000, 0x1fff, ram, ram);
		switch(mo)
		{
		case 0x0: SET_BANK(0x2000, 0x3fff, ram + 0x2000, ram + 0x2000); break;
		case 0x1: SET_BANK(0x2000, 0x3fff, ram + 0xc000, ram + 0xc000); break;
		case 0x2: SET_BANK(0x2000, 0x3fff, ram + 0xe000, ram + 0xe000); break;
		case 0x3: SET_BANK(0x2000, 0x3fff, wdmy, ext + 0x0000); break;
		case 0x4: SET_BANK(0x2000, 0x3fff, wdmy, ext + 0x2000); break;
		case 0x5: SET_BANK(0x2000, 0x3fff, wdmy, ext + 0x4000); break;
		case 0x6: SET_BANK(0x2000, 0x3fff, wdmy, ext + 0x6000); break;
		}
		SET_BANK(0x4000, 0xbfff, ram + 0x4000, ram + 0x4000);
		switch(ma)
		{
		case 0x0: SET_BANK(0xc000, 0xffff, ram + 0x10000, ram + 0x10000); break;
		case 0x1: SET_BANK(0xc000, 0xffff, ram + 0x14000, ram + 0x14000); break;
		case 0x2: SET_BANK(0xc000, 0xffff, ram + 0x18000, ram + 0x18000); break;
		case 0x3: SET_BANK(0xc000, 0xffff, ram + 0x1c000, ram + 0x1c000); break;
		case 0x4: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x20000, ram + 0x20000); break;
		case 0x5: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x24000, ram + 0x24000); break;
		case 0x6: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x28000, ram + 0x28000); break;
		case 0x7: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x2c000, ram + 0x2c000); break;
		case 0x8: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x30000, ram + 0x30000); break;
		case 0x9: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x34000, ram + 0x34000); break;
		case 0xa: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x38000, ram + 0x38000); break;
		case 0xb: SET_BANK(0xc000, 0xffff, knj ? wdmy : ram + 0x3c000, ram + 0x3c000); break;
		case 0xf: SET_BANK(0xf800, 0xffff, common, common); break;
		}
	}
}

void MEMORY::update_intr()
{
	bool intr = false;
	if(intmfd) {
		intr = true;
		inp = 0;
	}
	else if(!(e1 & 1)) {
		// interrupt disable
	}
	else if(int0) {
		intr = true;
		inp = 1;
	}
	else if(int1) {
		intr = true;
		inp = 2;
	}
	else if(int2) {
		intr = true;
		inp = 3;
	}
	else if(int3) {
		intr = true;
		inp = 4;
	}
	else if(int4) {
		intr = true;
		inp = 5;
	}
	d_cpu->set_intr_line(intr, true, 0);
}

uint32 MEMORY::intr_ack()
{
	if(intmfd)
		intmfd = false;
	else if(int0)
		int0 = false;
	else if(int1)
		int1 = false;
	else if(int2)
		int2 = false;
	else if(int3)
		int3 = false;
	else if(int4)
		int4 = false;
	return 0xff;
}
