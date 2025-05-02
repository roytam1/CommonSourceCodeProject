/*
	SORD m5 Emulator 'Emu5'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ cmt/printer ]
*/

#include "cmt.h"

void CMT::initialize()
{
	// data recorder
	in = out = remote = false;
	
	// printer
	strobe = busy = false;
}

void CMT::write_io8(uint32 addr, uint32 data)
{
	bool signal, motor;
	
	switch(addr & 0xff)
	{
	case 0x40:
		// printer
		pout = data;
		break;
	case 0x50:
		// data recorder
		if((signal = (data & 1) ? false : true) != out) {
			dev->write_signal(out_id, signal ? 0xffffffff : 0, 1);
			out = signal;
		}
		if((motor = (data & 2) ? true : false) != remote) {
			dev->write_signal(rmt_id, motor ? 0xffffffff : 0, 1);
			remote = motor;
		}
		// printer
		strobe = (data & 1) ? true : false;
		break;
	}
}

uint32 CMT::read_io8(uint32 addr)
{
	return (in ? 1 : 0) | (busy ? 2 : 0);
}

void CMT::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_CMT_IN)
		in = (data & mask) ? true : false;
	else if(id == SIG_PRINTER_BUSY)
		busy = (data & mask) ? true : false;
}

