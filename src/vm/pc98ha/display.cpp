/*
	NEC PC-98LT Emulator 'ePC-98LT'
	NEC PC-98HA Emulator 'eHANDY98'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.10 -

	[ display ]
*/

#include "display.h"

void DISPLAY::draw_screen()
{
	// draw to real screen
	uint16 cd = RGB_COLOR(6, 7, 2);
	uint16 cb = RGB_COLOR(20, 21, 20);
	int ptr = 0;
	
	for(int y = 0; y < 400; y++) {
		uint16* dest = emu->screen_buffer(y);
		for(int x = 0; x < 640; x += 8) {
			uint8 pat = vram[ptr++];
			dest[x + 0] = (pat & 0x80) ? cd : cb;
			dest[x + 1] = (pat & 0x40) ? cd : cb;
			dest[x + 2] = (pat & 0x20) ? cd : cb;
			dest[x + 3] = (pat & 0x10) ? cd : cb;
			dest[x + 4] = (pat & 0x08) ? cd : cb;
			dest[x + 5] = (pat & 0x04) ? cd : cb;
			dest[x + 6] = (pat & 0x02) ? cd : cb;
			dest[x + 7] = (pat & 0x01) ? cd : cb;
		}
	}
	
	// access lamp
	uint32 stat_f = d_fdc->read_signal(0);
	if(stat_f) {
		uint16 col = (stat_f & (1 | 4)) ? RGB_COLOR(31, 0, 0) :
		             (stat_f & (2 | 8)) ? RGB_COLOR(0, 31, 0) : 0;
		for(int y = 400 - 8; y < 400; y++) {
			uint16 *dest = emu->screen_buffer(y);
			for(int x = 640 - 8; x < 640; x++)
				dest[x] = col;
		}
	}
}

