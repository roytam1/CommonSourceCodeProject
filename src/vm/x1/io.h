/*
	SHARP X1twin Emulator 'eX1twin'
	SHARP X1turbo Emulator 'eX1turbo'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ 8bit i/o bus ]
*/

#ifndef _IO_H_
#define _IO_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_IO_MODE	0

#ifndef IO_ADDR_MAX
#define IO_ADDR_MAX 0x10000
#endif
#define IO_ADDR_MASK (IO_ADDR_MAX - 1)

class IO : public DEVICE
{
private:
	// i/o map
	typedef struct {
		DEVICE* dev;
		uint32 addr;
	} write_t;
	
	typedef struct {
		DEVICE* dev;
		uint32 addr;
		bool value_registered;
		uint32 value;
	} read_t;
	
	write_t write_table[IO_ADDR_MAX];
	read_t read_table[IO_ADDR_MAX];
	
	// for debug
	uint32 prv_waddr, prv_wdata;
	uint32 prv_raddr, prv_rdata;
	
	// vram
#ifdef _X1TURBO
	uint8 vram[0x18000];
#else
	uint8 vram[0xc000];
#endif
	bool vram_mode, signal;
	uint8* vram_b;
	uint8* vram_r;
	uint8* vram_g;
	
	void write_port(uint32 addr, uint32 data, bool is_dma);
	uint32 read_port(uint32 addr, bool is_dma);
	
public:
	IO(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		// vm->dummy must be generated first !
		for(int i = 0; i < IO_ADDR_MAX; i++) {
			write_table[i].dev = read_table[i].dev = vm->dummy;
			write_table[i].addr = read_table[i].addr = i;
		}
		prv_waddr = prv_raddr = -1;
	}
	~IO() {}
	
	// common functions
	void reset();
	void write_signal(int id, uint32 data, uint32 mask);
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_dma_io8(uint32 addr, uint32 data);
	uint32 read_dma_io8(uint32 addr);
	
	// unique functions
	uint8* get_vram() {
		return vram;
	}
	void set_iomap_single_r(uint32 addr, DEVICE* device);
	void set_iomap_single_w(uint32 addr, DEVICE* device);
	void set_iomap_single_rw(uint32 addr, DEVICE* device);
	void set_iomap_alias_r(uint32 addr, DEVICE* device, uint32 alias);
	void set_iomap_alias_w(uint32 addr, DEVICE* device, uint32 alias);
	void set_iomap_alias_rw(uint32 addr, DEVICE* device, uint32 alias);
	void set_iomap_range_r(uint32 s, uint32 e, DEVICE* device);
	void set_iomap_range_w(uint32 s, uint32 e, DEVICE* device);
	void set_iomap_range_rw(uint32 s, uint32 e, DEVICE* device);
	void set_iovalue_single_r(uint32 addr, uint32 value);
	void set_iovalue_range_r(uint32 s, uint32 e, uint32 value);
};

#endif

