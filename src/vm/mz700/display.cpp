/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	SHARP MZ-1500 Emulator 'EmuZ-1500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.05 -

	[ display ]
*/

#include "display.h"

void DISPLAY::initialize()
{
#ifdef _MZ1500
	// init palette
	for(int i = 0; i < 8; i++) {
		palette[i] = i;
	}
#endif
	
	// create pc palette
	for(int i = 0; i < 8; i++) {
		palette_pc[i] = RGB_COLOR((i & 2) ? 255 : 0, (i & 4) ? 255 : 0, (i & 1) ? 255 : 0);
	}
	
	// regist event
	vm->regist_vline_event(this);
}

#ifdef _MZ1500
void DISPLAY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
	case 0xf0:
		priority = data;
		break;
	case 0xf1:
		palette[data & 7] = (data >> 4) & 7;
		break;
	}
}
#endif

void DISPLAY::event_vline(int v, int clock)
{
	if(0 <= v && v < 200) {
		// draw one line
		int ptr = (v >> 3) * 40;
		for(int x = 0; x < 320; x += 8) {
			uint8 attr = vram_attr[ptr];
			uint16 code = (vram_char[ptr] << 3) | ((attr & 0x80) << 4);
			uint8 col_b = attr & 7;
			uint8 col_f = (attr >> 4) & 7;
			uint8 pat_t = font[code | (v & 7)];
			uint8* dest = &screen[v][x];
			
#ifdef _MZ1500
			if(priority & 2) {
				uint16 pcg_code = (vram_pcg_char[ptr] << 3) | ((vram_pcg_attr[ptr] & 0xc0) << 5);
				uint8 pcg_dot[7];
				uint8 pat_b = pcg_b[pcg_code | (v & 7)];
				uint8 pat_r = pcg_r[pcg_code | (v & 7)];
				uint8 pat_g = pcg_g[pcg_code | (v & 7)];
				pcg_dot[0] = ((pat_b & 0x80) >> 7) | ((pat_r & 0x80) >> 6) | ((pat_g & 0x80) >> 5);
				pcg_dot[1] = ((pat_b & 0x40) >> 6) | ((pat_r & 0x40) >> 5) | ((pat_g & 0x40) >> 4);
				pcg_dot[2] = ((pat_b & 0x20) >> 5) | ((pat_r & 0x20) >> 4) | ((pat_g & 0x20) >> 3);
				pcg_dot[3] = ((pat_b & 0x10) >> 4) | ((pat_r & 0x10) >> 3) | ((pat_g & 0x10) >> 2);
				pcg_dot[4] = ((pat_b & 0x08) >> 3) | ((pat_r & 0x08) >> 2) | ((pat_g & 0x08) >> 1);
				pcg_dot[5] = ((pat_b & 0x04) >> 2) | ((pat_r & 0x04) >> 1) | ((pat_g & 0x04) >> 0);
				pcg_dot[6] = ((pat_b & 0x02) >> 1) | ((pat_r & 0x02) >> 0) | ((pat_g & 0x02) << 1);
				pcg_dot[7] = ((pat_b & 0x01) >> 0) | ((pat_r & 0x01) << 1) | ((pat_g & 0x01) << 2);
				
				if(priority & 1) {
					// pcg > text
					dest[0] = pcg_dot[0] ? pcg_dot[0] : (pat_t & 0x80) ? col_f : col_b;
					dest[1] = pcg_dot[1] ? pcg_dot[1] : (pat_t & 0x40) ? col_f : col_b;
					dest[2] = pcg_dot[2] ? pcg_dot[2] : (pat_t & 0x20) ? col_f : col_b;
					dest[3] = pcg_dot[3] ? pcg_dot[3] : (pat_t & 0x10) ? col_f : col_b;
					dest[4] = pcg_dot[4] ? pcg_dot[4] : (pat_t & 0x08) ? col_f : col_b;
					dest[5] = pcg_dot[5] ? pcg_dot[5] : (pat_t & 0x04) ? col_f : col_b;
					dest[6] = pcg_dot[6] ? pcg_dot[6] : (pat_t & 0x02) ? col_f : col_b;
					dest[7] = pcg_dot[7] ? pcg_dot[7] : (pat_t & 0x01) ? col_f : col_b;
				}
				else {
					// text_fore > pcg > text_back
					dest[0] = (pat_t & 0x80) ? col_f : pcg_dot[0] ? pcg_dot[0] : col_b;
					dest[1] = (pat_t & 0x40) ? col_f : pcg_dot[1] ? pcg_dot[1] : col_b;
					dest[2] = (pat_t & 0x20) ? col_f : pcg_dot[2] ? pcg_dot[2] : col_b;
					dest[3] = (pat_t & 0x10) ? col_f : pcg_dot[3] ? pcg_dot[3] : col_b;
					dest[4] = (pat_t & 0x08) ? col_f : pcg_dot[4] ? pcg_dot[4] : col_b;
					dest[5] = (pat_t & 0x04) ? col_f : pcg_dot[5] ? pcg_dot[5] : col_b;
					dest[6] = (pat_t & 0x02) ? col_f : pcg_dot[6] ? pcg_dot[6] : col_b;
					dest[7] = (pat_t & 0x01) ? col_f : pcg_dot[7] ? pcg_dot[7] : col_b;
				}
			}
			else {
#endif
				// text only
				dest[0] = (pat_t & 0x80) ? col_f : col_b;
				dest[1] = (pat_t & 0x40) ? col_f : col_b;
				dest[2] = (pat_t & 0x20) ? col_f : col_b;
				dest[3] = (pat_t & 0x10) ? col_f : col_b;
				dest[4] = (pat_t & 0x08) ? col_f : col_b;
				dest[5] = (pat_t & 0x04) ? col_f : col_b;
				dest[6] = (pat_t & 0x02) ? col_f : col_b;
				dest[7] = (pat_t & 0x01) ? col_f : col_b;
#ifdef _MZ1500
			}
#endif
			ptr++;
		}
	}
}

void DISPLAY::draw_screen()
{
	// copy to real screen
	for(int y = 0; y < 200; y++) {
		scrntype* dest = emu->screen_buffer(y);
		uint8* src = screen[y];
		
		for(int x = 0; x < 320; x++) {
#ifdef _MZ1500
			dest[x] = palette_pc[palette[src[x] & 7]];
#else
			dest[x] = palette_pc[src[x] & 7];
#endif
		}
	}
}

