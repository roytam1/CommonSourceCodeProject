/*
	NEC N5200 Emulator 'eN5200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.10 -

	[ display ]
*/

#include "display.h"
#include "../i8259.h"

void DISPLAY::initialize()
{
	vm->regist_vline_event(this);
}

void DISPLAY::reset()
{
	vsync_enb = true;
}

void DISPLAY::write_io8(uint32 addr, uint32 data)
{
	switch(addr) {
	case 0x64:
		vsync_enb = true;
		break;
	}
}

void DISPLAY::event_vline(int v, int clock)
{
	if(v == 400 && vsync_enb) {
		d_pic->write_signal(SIG_I8259_CHIP0 | SIG_I8259_IR2, 1, 1);
		vsync_enb = false;
	}
}

void DISPLAY::draw_screen()
{
	// access lamp
	uint32 stat_f = d_fdc->read_signal(0);
	if(stat_f) {
		scrntype col = (stat_f & (1 | 4)) ? RGB_COLOR(255, 0, 0) :
		               (stat_f & (2 | 8)) ? RGB_COLOR(0, 255, 0) : 0;
		for(int y = 400 - 8; y < 400; y++) {
			scrntype *dest = emu->screen_buffer(y);
			for(int x = 640 - 8; x < 640; x++) {
				dest[x] = col;
			}
		}
	}
}

