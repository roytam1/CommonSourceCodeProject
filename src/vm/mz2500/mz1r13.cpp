/*
	SHARP MZ-80B Emulator 'EmuZ-80B'
	SHARP MZ-2200 Emulator 'EmuZ-2200'
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.01 -

	[ MZ-1R13 (Kanji ROM) ]
*/

#include "mz1r13.h"
#include "../../fileio.h"

void MZ1R13::initialize()
{
	// load rom image
	memset(rom, 0xff, sizeof(rom));
	
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(emu->bios_path(_T("MZ-1R13.ROM")), FILEIO_READ_BINARY) || 
	   fio->Fopen(emu->bios_path(_T("KANJI2.ROM")), FILEIO_READ_BINARY)) {
		fio->Fread(rom, sizeof(rom), 1);
		fio->Fclose();
	}
	delete fio;
	
	address = 0;
}

void MZ1R13::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
	case 0xb8:
		address = (address & 0xff00) | (data << 0);
		break;
	case 0xb9:
		address = (address & 0x00ff) | (data << 8);
		break;
	}
}

uint32 MZ1R13::read_io8(uint32 addr)
{
	switch(addr & 0xff) {
	case 0xb8:
		return rom[(address << 1) | 0];
	case 0xb9:
		return rom[(address << 1) | 1];
	}
	return 0xff;
}

