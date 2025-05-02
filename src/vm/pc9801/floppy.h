/*
	NEC PC-9801 Emulator 'ePC-9801'
	NEC PC-9801E/F/M Emulator 'ePC-9801E'
	NEC PC-98DO Emulator 'ePC-98DO'

	Author : Takeda.Toshiya
	Date   : 2010.09.15-

	[ floppy ]
*/

#ifndef _FLOPPY_H_
#define _FLOPPY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#if defined(SUPPORT_OLD_FDD_IF)
#define SIG_FLOPPY_2HD_IRQ	0
#define SIG_FLOPPY_2HD_DRQ	1
#define SIG_FLOPPY_2DD_IRQ	2
#define SIG_FLOPPY_2DD_DRQ	3
#else
#define SIG_FLOPPY_IRQ	0
#define SIG_FLOPPY_DRQ	1
#endif

class UPD765A;

class FLOPPY : public DEVICE
{
private:
#if defined(SUPPORT_OLD_FDD_IF)
	UPD765A *d_fdc_2hd, *d_fdc_2dd;
	uint8 ctrlreg_2hd, ctrlreg_2dd;
#else
	UPD765A *d_fdc;
	uint8 ctrlreg, modereg;
#endif
	DEVICE *d_dma, *d_pic;
	
	int timer_id;
	
public:
	FLOPPY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~FLOPPY() {}
	
	// common functions
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_callback(int event_id, int err);
	
	// unique functions
#if defined(SUPPORT_OLD_FDD_IF)
	void set_context_fdc_2hd(UPD765A* device) {
		d_fdc_2hd = device;
	}
	void set_context_fdc_2dd(UPD765A* device) {
		d_fdc_2dd = device;
	}
#else
	void set_context_fdc(UPD765A* device) {
		d_fdc = device;
	}
#endif
	void set_context_dma(DEVICE* device) {
		d_dma = device;
	}
	void set_context_pic(DEVICE* device) {
		d_pic = device;
	}
};

#endif

