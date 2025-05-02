/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.01 -

	[ extended rom ]
*/

#include "extrom.h"
#include "../../fileio.h"

void EXTROM::initialize()
{
	// init image
	_memset(rom, 0, sizeof(rom));
	
	// load rom image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sEXT.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(rom, sizeof(rom), 1);
		fio->Fclose();
	}
	delete fio;
	
	ptr = 0;
}

void EXTROM::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
	case 0xf8:
		// high addr
		ptr = (ptr & 0x00ff) | (data << 8);
		break;
	case 0xf9:
		// low addr
		ptr = (ptr & 0xff00) | data;
		break;
	}
}

uint32 EXTROM::read_io8(uint32 addr)
{
	switch(addr & 0xff) {
	case 0xf8:
		return rom[ptr];
	}
	return 0xff;
}

