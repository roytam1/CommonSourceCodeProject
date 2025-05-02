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
#define MAX_OUTPUT	16

// common signal id
#define SIG_CPU_IRQ	101
#define SIG_CPU_NMI	102
#define SIG_CPU_BUSREQ	103
#define SIG_CPU_DEBUG	201

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
	virtual void special_reset() {
		reset();
	}
	
	// memory bus
	virtual void write_data8(uint32 addr, uint32 data) {}
	virtual uint32 read_data8(uint32 addr) {
		return 0xff;
	}
	virtual void write_data16(uint32 addr, uint32 data) {
		write_data8(addr, data & 0xff);
		write_data8(addr + 1, (data >> 8) & 0xff);
	}
	virtual uint32 read_data16(uint32 addr) {
		uint32 val = read_data8(addr);
		val |= read_data8(addr + 1) << 8;
		return val;
	}
	virtual void write_data32(uint32 addr, uint32 data) {
		write_data8(addr, data & 0xff);
		write_data8(addr + 1, (data >> 8) & 0xff);
		write_data8(addr + 2, (data >> 16) & 0xff);
		write_data8(addr + 3, (data >> 24) & 0xff);
	}
	virtual uint32 read_data32(uint32 addr) {
		uint32 val = read_data8(addr);
		val |= read_data8(addr + 1) << 8;
		val |= read_data8(addr + 2) << 16;
		val |= read_data8(addr + 3) << 24;
		return val;
	}
	virtual void write_data8w(uint32 addr, uint32 data, int* wait) {
		*wait = 0;
		write_data8(addr, data);
	}
	virtual uint32 read_data8w(uint32 addr, int* wait) {
		*wait = 0;
		return read_data8(addr);
	}
	virtual void write_data16w(uint32 addr, uint32 data, int* wait) {
		int wait0, wait1;
		write_data8w(addr, data & 0xff, &wait0);
		write_data8w(addr + 1, (data >> 8) & 0xff, &wait1);
		*wait = wait0 + wait1;
	}
	virtual uint32 read_data16w(uint32 addr, int* wait) {
		int wait0, wait1;
		uint32 val = read_data8w(addr, &wait0);
		val |= read_data8w(addr + 1, &wait1) << 8;
		*wait = wait0 + wait1;
		return val;
	}
	virtual void write_data32w(uint32 addr, uint32 data, int* wait) {
		int wait0, wait1, wait2, wait3;
		write_data8w(addr, data & 0xff, &wait0);
		write_data8w(addr + 1, (data >> 8) & 0xff, &wait1);
		write_data8w(addr + 2, (data >> 16) & 0xff, &wait2);
		write_data8w(addr + 3, (data >> 24) & 0xff, &wait3);
		*wait = wait0 + wait1 + wait2 + wait3;
	}
	virtual uint32 read_data32w(uint32 addr, int* wait) {
		int wait0, wait1, wait2, wait3;
		uint32 val = read_data8w(addr, &wait0);
		val |= read_data8w(addr + 1, &wait1) << 8;
		val |= read_data8w(addr + 2, &wait2) << 16;
		val |= read_data8w(addr + 3, &wait3) << 24;
		*wait = wait0 + wait1 + wait2 + wait3;
		return val;
	}
	virtual void write_dma_data8(uint32 addr, uint32 data) {
		write_data8(addr, data);
	}
	virtual uint32 read_dma_data8(uint32 addr) {
		return read_data8(addr);
	}
	virtual void write_dma_data16(uint32 addr, uint32 data) {
		write_data16(addr, data);
	}
	virtual uint32 read_dma_data16(uint32 addr) {
		return read_data16(addr);
	}
	virtual void write_dma_data32(uint32 addr, uint32 data) {
		write_data32(addr, data);
	}
	virtual uint32 read_dma_data32(uint32 addr) {
		return read_data32(addr);
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
	virtual void write_io16(uint32 addr, uint32 data) {
		write_io8(addr, data & 0xff);
		write_io8(addr + 1, (data >> 8) & 0xff);
	}
	virtual uint32 read_io16(uint32 addr) {
		uint32 val = read_io8(addr);
		val |= read_io8(addr + 1) << 8;
		return val;
	}
	virtual void write_io32(uint32 addr, uint32 data) {
		write_io8(addr, data & 0xff);
		write_io8(addr + 1, (data >> 8) & 0xff);
		write_io8(addr + 2, (data >> 16) & 0xff);
		write_io8(addr + 3, (data >> 24) & 0xff);
	}
	virtual uint32 read_io32(uint32 addr) {
		uint32 val = read_io8(addr);
		val |= read_io8(addr + 1) << 8;
		val |= read_io8(addr + 2) << 16;
		val |= read_io8(addr + 3) << 24;
		return val;
	}
	virtual void write_io8w(uint32 addr, uint32 data, int* wait) {
		*wait = 0;
		write_io8(addr, data);
	}
	virtual uint32 read_io8w(uint32 addr, int* wait) {
		*wait = 0;
		return read_io8(addr);
	}
	virtual void write_io16w(uint32 addr, uint32 data, int* wait) {
		int wait0, wait1;
		write_io8w(addr, data & 0xff, &wait0);
		write_io8w(addr + 1, (data >> 8) & 0xff, &wait1);
		*wait = wait0 + wait1;
	}
	virtual uint32 read_io16w(uint32 addr, int* wait) {
		int wait0, wait1;
		uint32 val = read_io8w(addr, &wait0);
		val |= read_io8w(addr + 1, &wait1) << 8;
		*wait = wait0 + wait1;
		return val;
	}
	virtual void write_io32w(uint32 addr, uint32 data, int* wait) {
		int wait0, wait1, wait2, wait3;
		write_io8w(addr, data & 0xff, &wait0);
		write_io8w(addr + 1, (data >> 8) & 0xff, &wait1);
		write_io8w(addr + 2, (data >> 16) & 0xff, &wait2);
		write_io8w(addr + 3, (data >> 24) & 0xff, &wait3);
		*wait = wait0 + wait1 + wait2 + wait3;
	}
	virtual uint32 read_io32w(uint32 addr, int* wait) {
		int wait0, wait1, wait2, wait3;
		uint32 val = read_io8w(addr, &wait0);
		val |= read_io8w(addr + 1, &wait1) << 8;
		val |= read_io8w(addr + 2, &wait2) << 16;
		val |= read_io8w(addr + 3, &wait3) << 24;
		*wait = wait0 + wait1 + wait2 + wait3;
		return val;
	}
	virtual void write_dma_io8(uint32 addr, uint32 data) {
		write_io8(addr, data);
	}
	virtual uint32 read_dma_io8(uint32 addr) {
		return read_io8(addr);
	}
	virtual void write_dma_io16(uint32 addr, uint32 data) {
		write_io16(addr, data);
	}
	virtual uint32 read_dma_io16(uint32 addr) {
		return read_io16(addr);
	}
	virtual void write_dma_io32(uint32 addr, uint32 data) {
		write_io32(addr, data);
	}
	virtual uint32 read_dma_io32(uint32 addr) {
		return read_io32(addr);
	}
	
	// memory mapped i/o
	virtual void write_memory_mapped_io8(uint32 addr, uint32 data) {
		write_io8(addr, data);
	}
	virtual uint32 read_memory_mapped_io8(uint32 addr) {
		return read_io8(addr);
	}
	virtual void write_memory_mapped_io16(uint32 addr, uint32 data) {
		write_memory_mapped_io8(addr, data & 0xff);
		write_memory_mapped_io8(addr + 1, (data >> 8) & 0xff);
	}
	virtual uint32 read_memory_mapped_io16(uint32 addr) {
		uint32 val = read_memory_mapped_io8(addr);
		val |= read_memory_mapped_io8(addr + 1) << 8;
		return val;
	}
	virtual void write_memory_mapped_io32(uint32 addr, uint32 data) {
		write_memory_mapped_io8(addr, data & 0xff);
		write_memory_mapped_io8(addr + 1, (data >> 8) & 0xff);
		write_memory_mapped_io8(addr + 2, (data >> 16) & 0xff);
		write_memory_mapped_io8(addr + 3, (data >> 24) & 0xff);
	}
	virtual uint32 read_memory_mapped_io32(uint32 addr) {
		uint32 val = read_memory_mapped_io8(addr);
		val |= read_memory_mapped_io8(addr + 1) << 8;
		val |= read_memory_mapped_io8(addr + 2) << 16;
		val |= read_memory_mapped_io8(addr + 3) << 24;
		return val;
	}
	virtual void write_memory_mapped_io8w(uint32 addr, uint32 data, int* wait) {
		*wait = 0;
		write_memory_mapped_io8(addr, data);
	}
	virtual uint32 read_memory_mapped_io8w(uint32 addr, int* wait) {
		*wait = 0;
		return read_memory_mapped_io8(addr);
	}
	virtual void write_memory_mapped_io16w(uint32 addr, uint32 data, int* wait) {
		int wait0, wait1;
		write_memory_mapped_io8w(addr, data & 0xff, &wait0);
		write_memory_mapped_io8w(addr + 1, (data >> 8) & 0xff, &wait1);
		*wait = wait0 + wait1;
	}
	virtual uint32 read_memory_mapped_io16w(uint32 addr, int* wait) {
		int wait0, wait1;
		uint32 val = read_memory_mapped_io8w(addr, &wait0);
		val |= read_memory_mapped_io8w(addr + 1, &wait1) << 8;
		*wait = wait0 + wait1;
		return val;
	}
	virtual void write_memory_mapped_io32w(uint32 addr, uint32 data, int* wait) {
		int wait0, wait1, wait2, wait3;
		write_memory_mapped_io8w(addr, data & 0xff, &wait0);
		write_memory_mapped_io8w(addr + 1, (data >> 8) & 0xff, &wait1);
		write_memory_mapped_io8w(addr + 2, (data >> 16) & 0xff, &wait2);
		write_memory_mapped_io8w(addr + 3, (data >> 24) & 0xff, &wait3);
		*wait = wait0 + wait1 + wait2 + wait3;
	}
	virtual uint32 read_memory_mapped_io32w(uint32 addr, int* wait) {
		int wait0, wait1, wait2, wait3;
		uint32 val = read_memory_mapped_io8w(addr, &wait0);
		val |= read_memory_mapped_io8w(addr + 1, &wait1) << 8;
		val |= read_memory_mapped_io8w(addr + 2, &wait2) << 16;
		val |= read_memory_mapped_io8w(addr + 3, &wait3) << 24;
		*wait = wait0 + wait1 + wait2 + wait3;
		return val;
	}
	
	// device to device
	typedef struct {
		DEVICE *device;
		int id;
		uint32 mask;
		int shift;
	} output_t;
	
	typedef struct {
		int count;
		output_t item[MAX_OUTPUT];
	} outputs_t;
	
	virtual void init_output_signals(outputs_t *items) {
		items->count = 0;
	}
	virtual void register_output_signal(outputs_t *items, DEVICE *device, int id, uint32 mask, int shift) {
		int c = items->count++;
		items->item[c].device = device;
		items->item[c].id = id;
		items->item[c].mask = mask;
		items->item[c].shift = shift;
	}
	virtual void register_output_signal(outputs_t *items, DEVICE *device, int id, uint32 mask) {
		int c = items->count++;
		items->item[c].device = device;
		items->item[c].id = id;
		items->item[c].mask = mask;
		items->item[c].shift = 0;
	}
	virtual void write_signals(outputs_t *items, uint32 data) {
		for(int i = 0; i < items->count; i++) {
			output_t *item = &items->item[i];
			int shift = item->shift;
			uint32 val = (shift < 0) ? (data >> (-shift)) : (data << shift);
			uint32 mask = (shift < 0) ? (item->mask >> (-shift)) : (item->mask << shift);
			item->device->write_signal(item->id, val, mask);
		}
	};
	virtual void write_signal(int id, uint32 data, uint32 mask) {}
	virtual uint32 read_signal(int ch) {
		return 0;
	}
	
	// z80 daisy chain
	virtual void set_context_intr(DEVICE* device, uint32 bit) {}
	virtual void set_context_child(DEVICE* device) {}
	
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
	virtual void set_pc(uint32 pc) {
	}
	
	// bios
	virtual bool bios_call(uint32 PC, uint16 regs[], uint16 sregs[], int32* ZeroFlag, int32* CarryFlag) {
		return false;
	}
	virtual bool bios_int(int intnum, uint16 regs[], uint16 sregs[], int32* ZeroFlag, int32* CarryFlag) {
		return false;
	}
	
	// sound
	virtual void mix(int32* buffer, int cnt) {}
	
	// event callback
	virtual void event_callback(int event_id, int err) {}
	virtual void event_frame() {}
	virtual void event_vline(int v, int clock) {}
	virtual void event_hsync(int v, int h, int clock) {}
	
	DEVICE* prev_device;
	DEVICE* next_device;
	int this_device_id;
};

#endif
