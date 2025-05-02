/*
	SHARP MZ-80B Emulator 'EmuZ-80B'
	SHARP MZ-2200 Emulator 'EmuZ-2200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2013.03.17-

	[ MZ-1R12 (32KB SRAM) ]
*/

#include "mz1r12.h"

void MZ1R12::initialize()
{
	memset(sram, 0, sizeof(sram));
	read_only = false;
	
	FILEIO* fio = new FILEIO();
#ifndef _MZ80B
	if(fio->Fopen(emu->bios_path(_T("MZ-1E18.ROM")), FILEIO_READ_BINARY)) {
		fio->Fread(sram, sizeof(sram), 1);
		fio->Fclose();
		read_only = true;
	} else
#endif
	if(fio->Fopen(emu->bios_path(_T("MZ-1R12.BIN")), FILEIO_READ_BINARY)) {
		fio->Fread(sram, sizeof(sram), 1);
		fio->Fclose();
	}
	delete fio;
	
	address = 0;
	crc32 = getcrc32(sram, sizeof(sram));
}

void MZ1R12::release()
{
	if(!read_only && crc32 != getcrc32(sram, sizeof(sram))) {
		FILEIO* fio = new FILEIO();
		if(fio->Fopen(emu->bios_path(_T("MZ-1R12.BIN")), FILEIO_WRITE_BINARY)) {
			fio->Fwrite(sram, sizeof(sram), 1);
			fio->Fclose();
		}
		delete fio;
	}
}

void MZ1R12::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
	case 0xf8:
		address = (address & 0x00ff) | (data << 8);
		break;
	case 0xf9:
		address = (address & 0xff00) | (data << 0);
		break;
	case 0xfa:
		if(!read_only) {
			sram[address & 0x7fff] = data;
		}
		address++;
		break;
	}
}

uint32 MZ1R12::read_io8(uint32 addr)
{
	switch(addr & 0xff) {
	case 0xf8:
		address = 0;
		break;
	case 0xf9:
		return sram[(address++) & 0x7fff];
	}
	return 0xff;
}

