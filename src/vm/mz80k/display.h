/*
	SHARP MZ-80K Emulator 'EmuZ-80K'
	SHARP MZ-1200 Emulator 'EmuZ-1200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.18-

	[ display ]
*/

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_DISPLAY_VGATE	0
#ifdef _MZ1200
#define SIG_DISPLAY_REVERSE	1
#endif

class DISPLAY : public DEVICE
{
private:
	uint8 screen[200][320];
	uint8 font[0x800];
	uint8 *vram_ptr;
	scrntype palette_pc[2];
	bool vgate;
#ifdef _MZ1200
	bool reverse;
#endif
	
public:
	DISPLAY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~DISPLAY() {}
	
	// common functions
	void initialize();
	void reset();
	void write_signal(int id, uint32 data, uint32 mask);
	void event_vline(int v, int clock);
	
	// unique function
	void set_vram_ptr(uint8* ptr) {
		vram_ptr = ptr;
	}
	void draw_screen();
};

#endif

