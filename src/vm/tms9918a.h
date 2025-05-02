/*
	Skelton for retropc emulator

	Origin : MAME TMS9928A Core
	Author : Takeda.Toshiya
	Date   : 2006.08.18 -
	         2007.07.21 -

	[ TMS9918A ]
*/

#ifndef _TMS9918A_H_
#define _TMS9918A_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

class TMS9918A : public DEVICE
{
private:
	// output signals
	outputs_t outputs_irq;
	
	uint8 vram[TMS9918A_VRAM_SIZE];
	uint8 screen[192][256];
	uint8 regs[8], status_reg, read_ahead, first_byte;
	uint16 vram_addr;
	bool latch, intstat;
	uint16 color_table, pattern_table, name_table;
	uint16 sprite_pattern, sprite_attrib;
	uint16 color_mask, pattern_mask;
	
	void set_intstat(bool val);
	void draw_mode0();
	void draw_mode1();
	void draw_mode2();
	void draw_mode12();
	void draw_mode3();
	void draw_mode23();
	void draw_modebogus();
	void draw_sprites();
	
public:
	TMS9918A(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		init_output_signals(&outputs_irq);
	}
	~TMS9918A() {}
	
	// common functions
	void initialize();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_vline(int v, int clock);
	
	// unique function
	void set_context_irq(DEVICE* device, int id, uint32 mask) {
		regist_output_signal(&outputs_irq, device, id, mask);
	}
	void draw_screen();
};

#endif

