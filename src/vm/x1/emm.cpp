/*
	SHARP X1twin Emulator 'eX1twin'
	SHARP X1turbo Emulator 'eX1turbo'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2011.02.17-

	[ emm ]
*/

#include "emm.h"
#include "../../fileio.h"

void EMM::initialize()
{
	memset(data_buffer, 0, sizeof(data_buffer));
}

void EMM::reset()
{
	data_addr = 0;
}

void EMM::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 3) {
	case 0:
		data_addr = (data_addr & 0xffff00) | data;
		break;
	case 1:
		data_addr = (data_addr & 0xff00ff) | (data << 8);
		break;
	case 2:
		data_addr = (data_addr & 0x00ffff) | (data << 16);
		break;
	case 3:
		data_buffer[(data_addr++) & (EMM_BUFFER_SIZE - 1)] = data;
		break;
	}
}

uint32 EMM::read_io8(uint32 addr)
{
	switch(addr & 3) {
	case 3:
		return data_buffer[(data_addr++) & (EMM_BUFFER_SIZE - 1)];
	}
	return 0xff;
}

