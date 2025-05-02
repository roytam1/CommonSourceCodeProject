/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ Z80 mode2 int ]
*/

#include "z80pic.h"
#include "../z80.h"

void Z80PIC::reset()
{
	for(int i = 0; i < MAX_IRQ; i++)
		irq[i].request = irq[i].running = false;
	pri_cnt = 0;
	mask = 0;
}

void Z80PIC::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0xc6:
		mask = data;
		break;
	case 0xc7:
		if(mask & 0x80) vectors[0] = data;	// crtc
		if(mask & 0x40) vectors[1] = data;	// i8253
		if(mask & 0x20) vectors[2] = data;	// printer
		if(mask & 0x10) vectors[3] = data;	// rp5c15
		break;
	}
}

uint32 Z80PIC::read_io8(uint32 addr)
{
	return 0x30; // ?
}

void Z80PIC::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_Z80PIC_CRTC) {
		if(data & mask)
			request_int(IRQ_CRTC, vectors[0], false);
	}
	else if(id == SIG_Z80PIC_I8253) {
		if(data & mask)
			request_int(IRQ_I8253, vectors[1], false);
	}
	else if(id == SIG_Z80PIC_PRINTER) {
		if(data & mask)
			request_int(IRQ_PRINTER, vectors[2], false);
	}
	else if(id == SIG_Z80PIC_RP5C15) {
		if(!(data & mask))
			request_int(IRQ_RP5C15, vectors[3], false);
	}
}

void Z80PIC::request_int(int pri, uint32 vector, bool pending)
{
	if(pri_cnt < pri + 1)
		pri_cnt = pri + 1;
	
	// accept the request when the requested interrupt is not running now
	if(!irq[pri].running) {
		irq[pri].request = true;
		irq[pri].vector = vector;
		do_ei();
	}
	// cancel request when not pending
	if(!pending)
		irq[pri].request = false;
}

void Z80PIC::cancel_int(int pri)
{
	irq[pri].request = false;
}

void Z80PIC::do_reti()
{
	// most high priority request is finished
	for(int i = 0; i < pri_cnt; i++) {
		if(irq[i].running) {
			irq[i].running = false;
			// try next interrupt
			do_ei();
			return;
		}
	}
}

void Z80PIC::do_ei()
{
	// check cpu status
	if(!dev->accept_int())
		return;
	
	// try interrupt when cpu can accept interrupt requst
	for(int i = 0; i < pri_cnt; i++) {
		// quit when more high priority request is running
		if(irq[i].running)
			return;
		// check irq mask
		if(!(mask & 8) && i == IRQ_CRTC) continue;
		if(!(mask & 4) && i == IRQ_I8253) continue;
		if(!(mask & 2) && i == IRQ_PRINTER) continue;
		if(!(mask & 1) && i == IRQ_RP5C15) continue;
		// do interrupt
		if(irq[i].request) {
			irq[i].running = true;
			irq[i].request = false;
			dev->write_signal(SIG_CPU_DO_INT, (uint32)irq[i].vector, 0xffffffff);
			return;
		}
	}
}

