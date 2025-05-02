/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.01 -

	[ emm ]
*/

#include "emm.h"

void EMM::initialize()
{
	ptr = 0;
	buf = (uint8*)malloc(EMM_SIZE);
	_memset(buf, 0, EMM_SIZE);
}

void EMM::release()
{
	if(buf) {
		free(buf);
	}
}

void EMM::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
	case 0xac:
		// addr
		ptr = ((addr & 0xff00) << 8) | (data << 8) | (ptr & 0x0000ff);
		break;
	case 0xad:
		// data
		ptr = (ptr & 0xffff00) | (addr >> 8);
		if(ptr < EMM_SIZE) {
			buf[ptr] = data;
		}
		break;
	}
}

uint32 EMM::read_io8(uint32 addr)
{
	switch(addr & 0xff) {
	case 0xad:
		// data
		ptr = (ptr & 0xffff00) | (addr >> 8);
		return (ptr < EMM_SIZE) ? buf[ptr] : 0xff;
	}
	return 0xff;
}

