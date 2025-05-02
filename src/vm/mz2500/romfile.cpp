/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.05 -

	[ rom file ]
*/

#include "romfile.h"
#include "../../fileio.h"

void ROMFILE::initialize()
{
	// load rom image
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(emu->bios_path(_T("FILE.ROM")), FILEIO_READ_BINARY)) {
		// get file size (max 16mb)
		fio->Fseek(0, FILEIO_SEEK_END);
		size = fio->Ftell();
		fio->Fseek(0, FILEIO_SEEK_SET);
		
		// create buffer
		size = (size > 0x1000000) ? 0x1000000 : size;
		ptr = 0;
		buf = (uint8*)malloc(size);
		
		// load file
		fio->Fread(buf, size, 1);
		fio->Fclose();
	}
	else if(fio->Fopen(emu->bios_path(_T("SASI.ROM")), FILEIO_READ_BINARY)) {
		// get file size (max 16mb)
		fio->Fseek(0, FILEIO_SEEK_END);
		size = fio->Ftell();
		fio->Fseek(0, FILEIO_SEEK_SET);
		
		// create buffer
		size = (size > 0x1000000) ? 0x1000000 : size;
		ptr = 0;
		buf = (uint8*)malloc(size);
		
		// load file
		fio->Fread(buf, size, 1);
		fio->Fclose();
	}
	else {
		// file.rom not exist
		size = ptr = 0;
		buf = (uint8*)malloc(1);
	}
	delete fio;
}

void ROMFILE::release()
{
	if(buf) {
		free(buf);
	}
}

void ROMFILE::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
	case 0xa8:
		ptr = ((addr & 0xff00) << 8) | (data << 8) | (ptr & 0x0000ff);
		break;
	}
}

uint32 ROMFILE::read_io8(uint32 addr)
{
	switch(addr & 0xff) {
	case 0xa9:
		ptr = (ptr & 0xffff00) | ((addr & 0xff00) >> 8);
		return (ptr < size) ? buf[ptr] : 0xff;
	}
	return 0xff;
}

