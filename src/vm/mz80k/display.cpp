/*
	SHARP MZ-80K Emulator 'EmuZ-80K'
	SHARP MZ-1200 Emulator 'EmuZ-1200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.18-

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
	palette_pc[0] = RGB_COLOR(0, 0, 0);
#ifdef _MZ1200
	palette_pc[1] = RGB_COLOR(0, 255, 0);
#else
	palette_pc[1] = RGB_COLOR(255, 255, 255);
#endif
	
	// register event
	vm->register_vline_event(this);
}

void DISPLAY::reset()
{
	vgate = true;
#ifdef _MZ1200
	reverse = false;
#endif
}

void DISPLAY::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_DISPLAY_VGATE) {
		vgate = ((data & mask) != 0);
	}
#ifdef _MZ1200
	else if(id == SIG_DISPLAY_REVERSE) {
		reverse = ((data & mask) != 0);
	}
#endif
}

void DISPLAY::event_vline(int v, int clock)
{
	if(0 <= v && v < 200) {
		// draw one line
		int ptr = (v >> 3) * 40;
		for(int x = 0; x < 320; x += 8) {
			uint8 code = vram_ptr[ptr++];
			uint8 pat = font[(code << 3) | (v & 7)];
#ifdef _MZ1200
			if(reverse) {
				pat = ~pat;
			}
#endif
			uint8* dest = &screen[v][x];
			
			dest[0] = (pat & 0x80) >> 7;
			dest[1] = (pat & 0x40) >> 6;
			dest[2] = (pat & 0x20) >> 5;
			dest[3] = (pat & 0x10) >> 4;
			dest[4] = (pat & 0x08) >> 3;
			dest[5] = (pat & 0x04) >> 2;
			dest[6] = (pat & 0x02) >> 1;
			dest[7] = (pat & 0x01) >> 0;
		}
	}
}

void DISPLAY::draw_screen()
{
	// copy to real screen
	if(true || vgate) {
		for(int y = 0; y < 200; y++) {
			scrntype* dest = emu->screen_buffer(y);
			uint8* src = screen[y];
			
			for(int x = 0; x < 320; x++) {
				dest[x] = palette_pc[src[x] & 1];
			}
		}
	} else {
		for(int y = 0; y < 200; y++) {
			scrntype* dest = emu->screen_buffer(y);
			memset(dest, 0, sizeof(scrntype) * 320);
		}
	}
}

