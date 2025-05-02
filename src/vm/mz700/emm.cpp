/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	SHARP MZ-800 Emulator 'EmuZ-800'
	SHARP MZ-1500 Emulator 'EmuZ-1500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.09.02 -

	[ emm ]
*/

#include "emm.h"
#include "../../fileio.h"

#define DATA_SIZE	0x1000000
#define ADDR_MASK	(DATA_SIZE - 1)

void EMM::initialize()
{
	// init memory
	data_buffer = (uint8 *)malloc(DATA_SIZE);
	memset(data_buffer, 0xff, DATA_SIZE);
	
	// load emm image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sEMM.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(data_buffer, DATA_SIZE, 1);
		fio->Fclose();
	}
	delete fio;
}

void EMM::release()
{
	// release memory
	free(data_buffer);
}

void EMM::reset()
{
	data_addr = 0;
}

void EMM::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
	case 0x00:
		data_addr = (data_addr & 0xffff00) | data;
		break;
	case 0x01:
		data_addr = (data_addr & 0xff00ff) | (data << 8);
		break;
	case 0x02:
		data_addr = (data_addr & 0x00ffff) | (data << 16);
		break;
	case 0x03:
		data_buffer[(data_addr++) & ADDR_MASK] = data;
		break;
	}
}

uint32 EMM::read_io8(uint32 addr)
{
	switch(addr & 0xff) {
	case 0x00:
		return data_addr & 0xff;
	case 0x01:
		return (data_addr >> 8) & 0xff;
	case 0x02:
		return (data_addr >> 16) & 0xff;
	case 0x03:
		return data_buffer[(data_addr++) & ADDR_MASK];
	}
	return 0xff;
}

