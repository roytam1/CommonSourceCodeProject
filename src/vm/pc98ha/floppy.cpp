/*
	NEC PC-98LT Emulator 'ePC-98LT'
	NEC PC-98HA Emulator 'eHANDY98'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.11 -

	[ floppy ]
*/

#include "floppy.h"

void FLOPPY::reset()
{
	chgreg = 3;
}

void FLOPPY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xffff)
	{
	case 0x92:
	case 0xca:
//		if(((addr >> 4) ^ chgreg) & 1)
//			break;
		d_fdc->write_io8(1, data);
		break;
	case 0x94:
	case 0xcc:
//		if(((addr >> 4) ^ chgreg) & 1)
//			break;
		if((ctrlreg ^ data) & 0x10) {
			// mode chang
//			fdcstatusreset();
//			fdc_dmaready(0);
//			dmac_check();
		}
		ctrlreg = data;
		break;
	case 0xbe:
		chgreg = data;
		break;
	}
}

uint32 FLOPPY::read_io8(uint32 addr)
{
	switch(addr & 0xffff)
	{
	case 0x90:
	case 0xc8:
//		if(((addr >> 4) ^ chgreg) & 1)
//			break;
		return d_fdc->read_io8(0);
	case 0x92:
	case 0xca:
//		if(((addr >> 4) ^ chgreg) & 1)
//			break;
		return d_fdc->read_io8(1);
	case 0x94:
	case 0xcc:
//		if(((addr >> 4) ^ chgreg) & 1)
//			break;
		return (addr & 0x10) ? 0x40 : 0x70;
	case 0xbe:
		return (chgreg & 3) | 8;
	}
	return addr & 0xff;
}

void FLOPPY::write_signal(int id, uint32 data, uint32 mask)
{
	// drq from fdc
	d_dma->write_signal(did_dma[chgreg & 1], data, mask);
}

