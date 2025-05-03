/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.05 -

	[ display ]
*/

#include "display.h"
#include "../../fileio.h"

void DISPLAY::initialize()
{
	// load rom images
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sFONT.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(font, sizeof(font), 1);
		fio->Fclose();
	}
	delete fio;
	
	// create pc palette
	for(int i = 0; i < 8; i++)
		palette_pc[i] = RGB_COLOR((i & 2) ? 0x1f : 0, (i & 4) ? 0x1f : 0, (i & 1) ? 0x1f : 0);
	
	// regist event
	vm->regist_vsync_event(this);
}

void DISPLAY::event_vsync(int v, int clock)
{
//	v--;
	if(0 <= v && v < 200) {
		// draw one line
		int ptr = (v >> 3) * 40;
		for(int x = 0; x < 320; x += 8) {
			uint8 code = vram_char[ptr];
			uint8 col = vram_col[ptr++];
			
			uint8 col_b = col & 7;
			uint8 col_f = (col >> 4) & 7;
			uint8 pat = font[((col & 0x80) << 4) | (code << 3) | (v & 7)];
			uint8* dest = &screen[v][x];
			
			dest[0] = (pat & 0x80) ? col_f : col_b;
			dest[1] = (pat & 0x40) ? col_f : col_b;
			dest[2] = (pat & 0x20) ? col_f : col_b;
			dest[3] = (pat & 0x10) ? col_f : col_b;
			dest[4] = (pat & 0x08) ? col_f : col_b;
			dest[5] = (pat & 0x04) ? col_f : col_b;
			dest[6] = (pat & 0x02) ? col_f : col_b;
			dest[7] = (pat & 0x01) ? col_f : col_b;
		}
	}
}

void DISPLAY::draw_screen()
{
#if 0
	// create screen
	int ptr = 0;
	for(int y = 0; y < 200; y += 8) {
		for(int x = 0; x < 320; x += 8) {
			uint8 code = vram_char[ptr];
			uint8 col = vram_col[ptr++];
			
			uint8* font_base = &font[((col & 0x80) << 4) | (code << 3)];
			uint8 col_b = col & 7;
			uint8 col_f = (col >> 4) & 7;
			
			for(int l = 0; l < 8; l++) {
				uint8 pat = font_base[l];
				uint8* dest = &screen[y + l][x];
				
				dest[0] = (pat & 0x80) ? col_f : col_b;
				dest[1] = (pat & 0x40) ? col_f : col_b;
				dest[2] = (pat & 0x20) ? col_f : col_b;
				dest[3] = (pat & 0x10) ? col_f : col_b;
				dest[4] = (pat & 0x08) ? col_f : col_b;
				dest[5] = (pat & 0x04) ? col_f : col_b;
				dest[6] = (pat & 0x02) ? col_f : col_b;
				dest[7] = (pat & 0x01) ? col_f : col_b;
			}
		}
	}
#endif
	// copy to real screen
	for(int y = 0; y < 200; y++) {
		uint16* dest = emu->screen_buffer(y);
		uint8* src = screen[y];
		
		for(int x = 0; x < 320; x++)
			dest[x] = palette_pc[src[x] & 7];
	}
}

