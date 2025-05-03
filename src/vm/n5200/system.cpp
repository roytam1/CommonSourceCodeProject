/*
	NEC N5200 Emulator 'eN5200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.06.04-

	[ system i/o ]
*/

#include "system.h"

void SYSTEM::reset()
{
	mode = 0;
	nmi_enb = false;
}

void SYSTEM::write_io8(uint32 addr, uint32 data)
{
	switch(addr)
	{
	case 0x21:
		d_dma->write_signal(did_bank + 1, data, 0xff);
		break;
	case 0x23:
		d_dma->write_signal(did_bank + 2, data, 0xff);
		break;
	case 0x25:
		d_dma->write_signal(did_bank + 3, data, 0xff);
		break;
	case 0x27:
		d_dma->write_signal(did_bank + 0, data, 0xff);
		break;
	case 0x29:
		if((data & 0xc) == 0)
			d_dma->write_signal(did_mask + (data & 3), 0, 0xff);
		else if((data & 0xc) == 4)
			d_dma->write_signal(did_mask + (data & 3), 0xf, 0xff);
		else if((data & 0xc) == 0xc)
			d_dma->write_signal(did_mask + (data & 3), 0xff, 0xff);
	case 0x3b:
		mode = data;
		break;
	case 0x50:
		nmi_enb = false;
		break;
	case 0x52:
		nmi_enb = true;
		break;
	}
}

uint32 SYSTEM::read_io8(uint32 addr)
{
	switch(addr)
	{
	case 0x39:
		return mode;
	}
	return 0xff;
}

