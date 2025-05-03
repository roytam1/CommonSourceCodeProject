/*
	NEC TK-80BS (COMPO BS/80) Emulator 'eTK-80BS'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.08.26 -

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
}

void DISPLAY::write_signal(int id, uint32 data, uint32 mask)
{
	mode = data & 3;
}

void DISPLAY::draw_screen()
{
	// create screen
	uint16 col_f = (mode & 2) ? RGB_COLOR(31, 31, 31) : 0;
	uint16 col_b = (mode & 2) ? 0 : RGB_COLOR(31, 31, 31);
	int code_ofs = (mode & 1) << 8;
	int ptr = 0;
	
	for(int y = 0; y < 128; y += 8) {
		for(int x = 0; x < 256; x += 8) {
			int code = vram[ptr++] | code_ofs;
			uint8* font_base = &font[code << 3];
			
			for(int l = 0; l < 8; l++) {
				uint8 pat = font_base[l];
				uint16* dest = &screen[l][x];
				
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
		for(int l = 0; l < 8; l++) {
			uint16* dest = emu->screen_buffer(y + l);
			uint16* src = screen[l];
			_memcpy(dest, src, 512);	// 256*sizeof(uint16)
		}
	}
}

