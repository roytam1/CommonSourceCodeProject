/*
	NEC PC-6001 Emulator 'yaPC-6001'

	Author : Takeda.Toshiya
	Date   : 2013.08.22-

	[ display ]
*/

#include "display.h"
#include "../mc6847.h"

void DISPLAY::initialize()
{
	// register event
	register_vline_event(this);
}

void DISPLAY::reset()
{
	vram_ptr = ram_ptr + 0xe000;
	counter = 0;
}

void DISPLAY::write_io8(uint32 addr, uint32 data)
{
	unsigned int VRAMHead[4] = { 0xc000, 0xe000, 0x8000, 0xa000 };

	if ((addr & 0xff)==0xB0) {
		vram_ptr=(ram_ptr+VRAMHead[(data&0x06)>>1]);
	}
}

void DISPLAY::event_vline(int v, int clock)
{
	if(counter++ > 30) {
		counter = 0;
		d_cpu->write_signal(SIG_CPU_IRQ, 0x06, 0xff);
	}
}

void DISPLAY::draw_screen()
{
	d_vdp->write_signal(SIG_MC6847_AG, *vram_ptr, 0x80);
	d_vdp->write_signal(SIG_MC6847_GM, *vram_ptr >> 4, 1);
	d_vdp->write_signal(SIG_MC6847_GM, *vram_ptr >> 2, 2);
	d_vdp->write_signal(SIG_MC6847_GM, *vram_ptr >> 0, 4);
	d_vdp->write_signal(SIG_MC6847_CSS, *vram_ptr, 0x02);
	if(*vram_ptr & 0x80) {
		d_vdp->set_vram_ptr(vram_ptr + 0x200, 0x1800);
	} else {
		d_vdp->set_vram_ptr(vram_ptr, 0x1800);
	}
	d_vdp->draw_screen();
}
