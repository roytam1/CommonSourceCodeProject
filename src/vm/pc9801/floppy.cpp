/*
	NEC PC-9801 Emulator 'ePC-9801'
	NEC PC-9801E/F/M Emulator 'ePC-9801E'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.09.15-

	[ floppy ]
*/

#include "floppy.h"
#include "../i8259.h"
#include "../upd765a.h"

#define EVENT_TIMER	0

void FLOPPY::reset()
{
	ctrlreg_2hd = ctrlreg_2dd = 0x80;
	timer_id = -1;
}

void FLOPPY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xffff) {
	case 0x94:
		if(!(ctrlreg_2hd & 0x80) && (data & 0x80)) {
			d_fdc_2hd->reset();
		}
		d_fdc_2hd->write_signal(SIG_UPD765A_FREADY, data, 0x40);
		ctrlreg_2hd = data;
		break;
	case 0xcc:
		if(!(ctrlreg_2dd & 0x80) && (data & 0x80)) {
			d_fdc_2dd->reset();
		}
		if(data & 1) {
			if(timer_id != -1) {
				vm->cancel_event(timer_id);
			}
			vm->register_event(this, EVENT_TIMER, 100000, false, &timer_id);
		}
		// FDC RDY is pulluped
		d_fdc_2dd->write_signal(SIG_UPD765A_MOTOR, data, 0x08);
		ctrlreg_2dd = data;
		break;
	}
}

uint32 FLOPPY::read_io8(uint32 addr)
{
	switch(addr & 0xffff) {
	case 0x94:
		return 0x5f;	// 0x40 ???
	case 0xcc:
		return (d_fdc_2dd->disk_inserted() ? 0x10 : 0) | 0x6f;	// 0x60 ???
	}
	return addr & 0xff;
}

void FLOPPY::event_callback(int event_id, int err)
{
	if(ctrlreg_2dd & 4) {
		d_pic->write_signal(SIG_I8259_CHIP1 | SIG_I8259_IR2, 1, 1);
	}
	timer_id = -1;
}
