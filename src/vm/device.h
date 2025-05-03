/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ device base class ]
*/

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "vm.h"
#include "../emu.h"
#include "../fileio.h"

// max devices connected to the output port
#define MAX_OUTPUT	8

// common signal id
#define SIG_CPU_NMI	101
#define SIG_CPU_BUSREQ	102

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
		if(!vm->first_device) {
			// this is the first device
			vm->first_device = this;
			this_device_id = 0;
		}
		else {
			// this is not the first device
			vm->last_device->next_device = this;
			this_device_id = vm->last_device->this_device_id + 1;
		}
		vm->last_device = this;
	}
	~DEVICE(void) {}
	
	virtual void initialize() {}
	virtual void release() {}
	
	virtual void update_config() {}
	virtual void save_state(FILEIO* fio) {}
	virtual void load_state(FILEIO* fio) {}
	
	// control
	virtual void reset() {}
	virtual void soft_reset() {
		reset();
	}
	virtual void ipl_reset() {
		reset();
	}
	
	// memory bus
	virtual void write_data8(uint32 addr, uint32 data) {}
	virtual uint32 read_data8(uint32 addr) {
		return 0xff;
	}
	virtual void write_data16(uint32 addr, uint32 data) {
		write_data8(addr, data & 0xff); write_data8(addr + 1, data >> 8);
	}
	virtual uint32 read_data16(uint32 addr) {
		return read_data8(addr) | (read_data8(addr + 1) << 8);
	}
	virtual void write_data8w(uint32 addr, uint32 data, int* wait) {
		*wait = 0;
	}
	virtual uint32 read_data8w(uint32 addr, int* wait) {
		*wait = 0;
		return 0xff;
	}
	virtual void write_data16w(uint32 addr, uint32 data, int* wait) {
		*wait = 0;
		write_data8(addr, data & 0xff); write_data8(addr + 1, data >> 8);
	}
	virtual uint32 read_data16w(uint32 addr, int* wait) {
		*wait = 0;
		return read_data8(addr) | (read_data8(addr + 1) << 8);
	}
	virtual void write_dma8(uint32 addr, uint32 data) {
		write_data8(addr, data);
	}
	virtual uint32 read_dma8(uint32 addr) {
		return read_data8(addr);
	}
	
	// i/o bus
	virtual void write_io8(uint32 addr, uint32 data) {}
	virtual uint32 read_io8(uint32 addr) {
#ifdef IOBUS_RETURN_ADDR
		return (addr & 1 ? addr >> 8 : addr) & 0xff;
#else
		return 0xff;
#endif
	}
	virtual void write_io8w(uint32 addr, uint32 data, int* wait) {
		*wait = 0;
	}
	virtual uint32 read_io8w(uint32 addr, int* wait) {
		*wait = 0;
#ifdef IOBUS_RETURN_ADDR
		return (addr & 1 ? addr >> 8 : addr) & 0xff;
#else
		return 0xff;
#endif
	}
	
	// device to device
	virtual void write_signal(int id, uint32 data, uint32 mask) {}
	virtual uint32 read_signal(int ch) {
		return 0;
	}
	
	// interrupt device to device
	virtual void set_intr_iei(bool val) {}
	
	// interrupt device to cpu
	virtual void set_intr_line(bool line, bool pending, uint32 bit) {}
	
	// interrupt cpu to device
	virtual uint32 intr_ack() {
		return 0xff;
	}
	virtual void intr_reti() {}
	
	// cpu
	virtual void run(int clock) {}
	virtual int passed_clock() {
		return 0;
	}
	virtual uint32 get_prv_pc() {
		return 0;
	}
	
	// sound
	virtual void mix(int32* buffer, int cnt) {}
	
	// event callback
	virtual void event_callback(int event_id, int err) {}
	virtual void event_frame() {}
	virtual void event_vsync(int v, int clock) {}
	virtual void event_hsync(int v, int h, int clock) {}
	
	DEVICE* prev_device;
	DEVICE* next_device;
	int this_device_id;
};

#endif
