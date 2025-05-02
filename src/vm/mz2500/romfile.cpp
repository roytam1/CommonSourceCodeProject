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
	_TCHAR app_path[_MAX_PATH], file_path[2][_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path[0], _T("%sFILE.ROM"), app_path);
	_stprintf(file_path[1], _T("%sSASI.ROM"), app_path);
	if(fio->Fopen(file_path[0], FILEIO_READ_BINARY)) {
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
	else if(fio->Fopen(file_path[1], FILEIO_READ_BINARY)) {
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

