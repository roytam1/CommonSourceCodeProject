/*
	Skelton for Z-80 PC Emulator

	Author : Takeda.Toshiya
	Date   : 2004.04.26 -

	[ device base class ]
*/

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "vm.h"
#include "../emu.h"

class DEVICE
{
protected:
	VM* vm;
	EMU* emu;
public:
	DEVICE(VM* parent_vm, EMU* parent_emu) : vm(parent_vm), emu(parent_emu)
	{
		prev_device = vm->last_device;
		next_device = NULL;
		if(!vm->first_device)
			vm->first_device = this;
		if(vm->last_device)
			vm->last_device->next_device = this;
		vm->last_device = this;
	}
	~DEVICE(void) {}
	
	virtual void initialize() {}
	virtual void release() {}
	
	virtual void update_config() {}
//	virtual void save_state(FILEIO* fio) {}
//	virtual void load_state(FILEIO* fio) {}
	
	// control
	virtual void reset() {}
	virtual void ipl_reset() { reset(); }
	
	// memory bus
	virtual void write_data8(uint16 addr, uint8 data) {}
	virtual uint8 read_data8(uint16 addr) { return 0xff; }
	virtual void write_data16(uint16 addr, uint16 data) { write_data8(addr, data & 0xff); write_data8(addr + 1, data >> 8); }
	virtual uint16 read_data16(uint16 addr) { return read_data8(addr) | (read_data8(addr + 1) << 8); }
	
	// i/o bus
	virtual void write_io8(uint16 addr, uint8 data) {}
	virtual uint8 read_io8(uint16 addr) { return 0xff; }
	virtual void write_io16(uint16 addr, uint16 data) { write_io8(addr, data & 0xff); write_io8(addr + 1, data >> 8); }
	virtual uint16 read_io16(uint16 addr) { return read_io8(addr) | (read_io8(addr + 1) << 8); }
	
	virtual int iomap_write(int index) { return -1; }
	virtual int iomap_read(int index) { return -1; }
	
	// cpu to device
	virtual void do_reti() {}
	virtual void do_ei() {}
	
	// event callback
	virtual void event_callback(int event_id, int err) {}
	
	DEVICE* prev_device;
	DEVICE* next_device;
};

#endif
