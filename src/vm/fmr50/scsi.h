/*
	FUJITSU FMR-50 Emulator 'eFMR-50'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.05.02 -

	[ scsi ]
*/

#ifndef _SCSI_H_
#define _SCSI_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class SCSI : public DEVICE
{
private:
	DEVICE *d_drq, *d_irq;
	int did_drq, did_irq;
	uint32 dmask_drq, dmask_irq;
	
	int phase;
	uint8 ctrlreg, datareg, statreg;
	
public:
	SCSI(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~SCSI() {}
	
	// common functions
	void initialize();
	void release();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_dma8(uint32 addr, uint32 data);
	uint32 read_dma8(uint32 addr);
	
	// unique function
	void set_context_drq(DEVICE* device, int id, uint32 mask) {
		d_drq = device; did_drq = id; dmask_drq = mask;
	}
	void set_context_irq(DEVICE* device, int id, uint32 mask) {
		d_irq = device; did_irq = id; dmask_irq = mask;
	}
};

#endif

