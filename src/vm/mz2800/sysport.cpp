/*
	SHARP MZ-2800 Emulator 'EmuZ-2800'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.08.14 -

	[ system poty ]
*/

#include "sysport.h"

void SYSPORT::initialize()
{
//	int id;
//	vm->regist_event(this, 0, 2000, true, &id);
	shut = 0;
}

void SYSPORT::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0x7fff)
	{
	case 0x8f:
		// shut
		shut = data;
		break;
	}
}

uint32 SYSPORT::read_io8(uint32 addr)
{
	switch(addr & 0x7fff)
	{
	case 0x8e:
		// dipswitch
		return 0xff;
	case 0x8f:
		// shut
		return shut;
	case 0xbe:
		// z80sio ack
		return d_sio->intr_ack();
	case 0xca:
		// voice communication ???
		return 0x7f;
	}
	return 0xff;
}

void SYSPORT::event_callback(int event_id, int err)
{
	// memory reshresh
	d_dma->write_signal(did_dma, 1, 1);
}
