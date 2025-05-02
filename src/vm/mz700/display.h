/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	SHARP MZ-800 Emulator 'EmuZ-800'
	SHARP MZ-1500 Emulator 'EmuZ-1500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.05 -

	[ display ]
*/

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class DISPLAY : public DEVICE
{
private:
#if defined(_MZ800) || defined(_MZ1500)
	DEVICE *d_fdc, *d_qd;
#endif
	
#if defined(_MZ800)
	uint8 screen[200][640];
	scrntype palette_mz800_pc[16];
	bool scanline;
#else
	uint8 screen[200][320];
#endif
	scrntype palette_pc[8];
	
	uint8 *vram;
	uint8 *font;
#if defined(_MZ800)
	uint8 *vram_mz800;
	uint8 dmd;
	uint16 sof;
	uint8 sw, ssa, sea, bcol, cksw;
	uint8 palette_sw, palette[4], palette16[16];
#elif defined(_MZ1500)
	uint8 *pcg;
	uint8 priority, palette[8];
#endif
	void draw_line_mz700(int v);
#if defined(_MZ800)
	void draw_line_320x200_2bpp(int v);
	void draw_line_320x200_4bpp(int v);
	void draw_line_640x200_1bpp(int v);
	void draw_line_640x200_2bpp(int v);
#endif
	
public:
	DISPLAY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~DISPLAY() {}
	
	// common functions
	void initialize();
#if defined(_MZ800)
	void update_config();
#endif
	void reset();
	
#if defined(_MZ800) || defined(_MZ1500)
	void write_io8(uint32 addr, uint32 data);
#endif
	void event_vline(int v, int clock);
	
	// unique function
	void set_vram_ptr(uint8* ptr) {
		vram = ptr;
	}
	void set_font_ptr(uint8* ptr) {
		font = ptr;
	}
#if defined(_MZ800)
	void set_vram_mz800_ptr(uint8* ptr) {
		vram_mz800 = ptr;
	}
	void write_dmd(uint8 val) {
		dmd = val;
	}
#elif defined(_MZ1500)
	void set_pcg_ptr(uint8* ptr) {
		pcg = ptr;
	}
#endif
#if defined(_MZ800) || defined(_MZ1500)
	void set_context_fdc(DEVICE* device) {
		d_fdc = device;
	}
	void set_context_qd(DEVICE* device) {
		d_qd = device;
	}
#endif
	void draw_screen();
};

#endif

