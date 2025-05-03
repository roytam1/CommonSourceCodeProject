/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.02.11 -

	[ interrupt ]
*/

#include "interrupt.h"
#include "../../config.h"

extern config_t config;

void INTERRUPT::initialize()
{
	patch = config.pic_patch;
}

void INTERRUPT::reset()
{
	enable = paddr = 0;
}

void INTERRUPT::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0xc6:
//		if((enable & 8) && !(data & 8))
//			d_pic->cancel_int(IRQ_CRTC);
		if((enable & 4) && !(data & 4))
			d_pic->cancel_int(IRQ_I8253);
//		if((enable & 2) && !(data & 2))
//			d_pic->cancel_int(IRQ_PRINTER);
//		if((enable & 1) && !(data & 1))
//			d_pic->cancel_int(IRQ_RP5C15);
		enable = data;
		break;
	case 0xc7:
		if(enable & 0x80)
			vectors[0] = data;	// crtc
		if(enable & 0x40) {
			vectors[1] = data;	// i8253
			paddr = d_cpu->get_prv_pc();
			if((vectors[1] == 0x10 && paddr == 0xe96a) ||	// MULTIPLAN
			   (vectors[1] == 0xe0 && paddr == 0x234d) ||	// RELICS
			   (vectors[1] == 0x02 && paddr == 0xb0de) ||	// SUPERt–]
			   (vectors[1] == 0x00 && paddr == 0x434f) ||	// Wizardly
			   (vectors[1] == 0x00 && paddr == 0x0a28) ||	// ‹ã‹Ê“`
			   (vectors[1] == 0xfe && paddr == 0x0f60))	// ŽEl‹äŠy•”
				d_pic->cancel_int(IRQ_I8253);
		}
		if(enable & 0x20)
			vectors[2] = data;	// printer
		if(enable & 0x10)
			vectors[3] = data;	// rp5c15
		break;
	}
}

void INTERRUPT::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_INTERRUPT_CRTC) {
		if((enable & 8) && (data & mask))
			d_pic->request_int(this, IRQ_CRTC, vectors[0], false);
	}
	else if(id == SIG_INTERRUPT_I8253) {
		if(enable & 4) {
			if((vectors[1] == 0x10 && paddr == 0xe96a) ||	// MULTIPLAN
			   (vectors[1] == 0xe0 && paddr == 0x234d) ||	// RELICS
			   (vectors[1] == 0x02 && paddr == 0xb0de) ||	// SUPERt–]
			   (vectors[1] == 0x00 && paddr == 0x434f) ||	// Wizardly
			   (vectors[1] == 0x00 && paddr == 0x0a28) ||	// ‹ã‹Ê“`
			   (vectors[1] == 0xfe && paddr == 0x0f60) ||	// ŽEl‹äŠy•”
			   patch) {
				if(!(data & mask))
					d_pic->request_int(this, IRQ_I8253, vectors[1], false);
			}
			else {
				if(!(data & mask))
					d_pic->request_int(this, IRQ_I8253, vectors[1], true);
				else
					d_pic->cancel_int(IRQ_I8253);
			}
		}
	}
	else if(id == SIG_INTERRUPT_PRINTER) {
		if((enable & 2) && (data & mask))
			d_pic->request_int(this, IRQ_PRINTER, vectors[2], false);
	}
	else if(id == SIG_INTERRUPT_RP5C15) {
		if((enable & 1) && (data & mask))
			d_pic->request_int(this, IRQ_RP5C15, vectors[3], false);
	}
}

void INTERRUPT::update_config()
{
	if(patch = config.pic_patch)
		d_pic->cancel_int(IRQ_I8253);
}

