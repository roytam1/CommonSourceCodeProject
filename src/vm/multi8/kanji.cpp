/*
	MITSUBISHI Elec. MULTI8 Emulator 'EmuLTI8'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.15 -

	[ kanji rom ]
*/

#include "kanji.h"
#include "../../fileio.h"

void KANJI::initialize()
{
	// load rom images
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sKANJI.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(rom, sizeof(rom), 1);
		fio->Fclose();
		
		// 8255 Port A, bit6 = 0 (kanji rom status)
		dev->write_signal(did, 0, 0x40);
	}
	else
		// 8255 Port A, bit6 = 1 (kanji rom not status)
		dev->write_signal(did, 0x40, 0x40);
	delete fio;
}

void KANJI::reset()
{
	ptr = 0;
}

void KANJI::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0x40:
		ptr = (ptr & 0xff00) | data;
		break;
	case 0x41:
		ptr = (ptr & 0x00ff) | (data << 8);
		break;
	}
}

uint32 KANJI::read_io8(uint32 addr)
{
	switch(addr & 0xff)
	{
	case 0x40:
		return rom[(ptr << 1) | 0];
	case 0x41:
		return rom[(ptr << 1) | 1];
	}
	return 0xff;
}
