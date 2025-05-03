/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ memory ]
*/

#include "memory.h"
#include "i8253.h"
#include "upd765a.h"
#include "sound.h"
#include "../fileio.h"

#include "../config.h"
extern config_t config;

void MEMORY::initialize()
{
	// init memory
	_memset(ram, 0, sizeof(ram));
	_memset(cmos, 0, sizeof(cmos));
	_memset(ipl, 0xff, sizeof(ipl));
	_memset(dummy_r, 0xff, sizeof(dummy_r));
	
	// load rom images
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sIPL.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ipl, sizeof(ipl), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sCMOS.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(cmos, sizeof(cmos), 1);
		fio->Fclose();
	}
	delete fio;
	
	// init memory map
	bank = 0x10;
	psel = true;
	csel = false;
	update_map();
}

void MEMORY::release()
{
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sCMOS.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(cmos, sizeof(cmos), 1);
		fio->Fclose();
	}
	delete fio;
}

void MEMORY::reset()
{
	bank = 0x10;
	psel = true;
	csel = false;
	update_map();
}

void MEMORY::write_data8(uint16 addr, uint8 data)
{
	bank_w[addr >> 11][addr & 0x7ff] = data;
}

uint8 MEMORY::read_data8(uint16 addr)
{
	return bank_r[addr >> 11][addr & 0x7ff];
}

void MEMORY::write_io8(uint16 addr, uint8 data)
{
	switch(addr & 0xff)
	{
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
			bank = data;
			// to gate of i8253
			vm->pit->input_gate(0, (data & 1) ? true : false); // speaker
			vm->pit->input_gate(2, (data & 2) ? true : false); // software timer
			vm->sound->beep_cont = (data & 4) ? true : false; // beep
			break;
		case 0x1c:
		case 0x1d:
		case 0x1e:
		case 0x1f:
			psel = (data & 1) ? false : true;
			break;
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23:
			csel = (data & 1) ? true : false;
			break;
	}
	update_map();
}

uint8 MEMORY::read_io8(uint16 addr)
{
	switch(addr & 0xff)
	{
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
			return config.dipswitch;
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
//			return bank & 0xf0;
			return (bank & 0xf0) | vm->fdc->fdc_status();
	}
	return 0xff;
}

void MEMORY::update_map()
{
	for(int i = 0; i < 28; i++) {
		if(bank & 0x10) {
			bank_r[i] = ram + 0x00000 + 0x800 * i;
			bank_w[i] = ram + 0x00000 + 0x800 * i;
		}
		else if(bank & 0x20) {
			bank_r[i] = ram + 0x10000 + 0x800 * i;
			bank_w[i] = ram + 0x10000 + 0x800 * i;
		}
		else if(bank & 0x40) {
			bank_r[i] = ram + 0x20000 + 0x800 * i;
			bank_w[i] = ram + 0x20000 + 0x800 * i;
		}
		else if(bank & 0x80) {
			bank_r[i] = ram + 0x30000 + 0x800 * i;
			bank_w[i] = ram + 0x30000 + 0x800 * i;
		}
		else {
			bank_r[i] = dummy_r;
			bank_w[i] = dummy_w;
		}
	}
	for(int i = 28; i < 32; i++) {
		bank_r[i] = ram + 0x00000 + 0x800 * i;
		bank_w[i] = ram + 0x00000 + 0x800 * i;
	}
	if(psel) {
		for(int i = 0; i < 4; i++) {
			bank_r[i] = ipl + 0x800 * i;
			bank_w[i] = dummy_w;
		}
	}
	if(csel) {
		bank_r[16] = cmos;
		bank_w[16] = cmos;
	}
}

