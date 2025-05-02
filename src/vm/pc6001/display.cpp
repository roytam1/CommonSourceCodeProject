/*
	NEC PC-6001 Emulator 'yaPC-6001'

	Author : Takeda.Toshiya
	Date   : 2013.08.22-

	[ display ]
*/

#include "memory.h"
#include "display.h"
#include "../mc6847.h"

void DISPLAY::initialize()
{
	// register event
	register_vline_event(this);
}

void DISPLAY::reset()
{
	counter = 0;
	OldTimerSW = TimerSW = 0; // originally zero
	TimerSW_F3 = 1;
#ifdef _PC6001
	vram_ptr = ram_ptr + 0xe000;
#else
	IntSW_F3 = 1;
	portF3 = 0;
	portF6 = 1;
	portF7 = 0x06;	
#endif
}

void DISPLAY::write_io8(uint32 addr, uint32 data)
{
	unsigned int VRAMHead[4] = { 0xc000, 0xe000, 0x8000, 0xa000 };
	uint16 port=(addr & 0x00ff);
	byte Value=data;
	switch (port)
	{
#ifdef _PC6001
	case 0xB0:
		vram_ptr=(ram_ptr+VRAMHead[(data&0x06)>>1]);
		TimerSW=(data & 0x01)?0:1;
		break;
	}
}
#else
	case 0xF3:                          // wait,int control
		portF3=Value;
		TimerSW_F3=(Value&0x04)?0:1;
		IntSW_F3=(Value&0x01)?0:1;
		break;
	case 0xF4: break;                  // int1 addr set
	case 0xF5: break;                  // int2 addr set
	case 0xF6:                          // timer countup value
		portF6 = Value;
///		SetTimerIntClock(Value);
		break;
	case 0xF7:                          // tmer int addr set
		portF7 = Value;
		break;
	case 0xF8: break;                  // CG access control
	}
}

uint32 DISPLAY::read_io8(uint32 addr)
{
	uint16 port=(addr & 0x00ff);
	byte Value=0xff;

	switch(port)
	{
	case 0xF3: Value=portF3;break;
	case 0xF6: Value=portF6;break;
	case 0xF7: Value=portF7;break;
	}
	return(Value);
}
#endif

#ifdef _PC6001

void DISPLAY::event_vline(int v, int clock)
{
	if(counter++ >= 30) {
		if (!OldTimerSW && TimerSW) {
			counter = 15;
		} else {
			counter = 0;
		}
		OldTimerSW = TimerSW;
		if (TimerSW)
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
#else
void DISPLAY::event_vline(int v, int clock)
{
	if(counter++ >= portF6 * 10) {
		counter = 0;
		TimerSW=d_mem->TimerSW;
		if (TimerSW && TimerSW_F3) {
			d_cpu->write_signal(SIG_CPU_IRQ, 0x06, 0xff);
		}
	}
}

void DISPLAY::draw_screen()
{
	d_mem->draw_screen();
}
#endif
