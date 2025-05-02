/*
	TOSHIBA PASOPIA Emulator 'EmuPIA'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.02.08 -

	[ display ]
*/

#include "display.h"
#include "../../fileio.h"
#include "../../config.h"

void DISPLAY::initialize()
{
	scanline = config.scan_line;
	
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
#ifdef _LCD
	for(int i = 1; i < 8; i++) {
		palette_pc[i] = RGB_COLOR(48, 56, 16);
	}
	palette_pc[0] = RGB_COLOR(160, 168, 160);
#else
	for(int i = 0; i < 8; i++) {
		palette_pc[i] = RGB_COLOR((i & 2) ? 255 : 0, (i & 4) ? 255 : 0, (i & 1) ? 255 : 0);
	}
#endif
	
	// init pasopia own
	mode = 0;
	cblink = 0;
	
	// register event
	vm->register_frame_event(this);
}

void DISPLAY::update_config()
{
	scanline = config.scan_line;
}

void DISPLAY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
	case 0x10:
		d_crtc->write_io8(addr, data);
		ch = data;
		break;
	case 0x11:
		d_crtc->write_io8(addr, data);
		if(ch == 0) {
			// XXX: Fixed Me !
			_memset(vram, 0, 0x8000);
			_memset(attr, 0, 0x8000);
		}
		break;
	}
}

void DISPLAY::write_signal(int id, uint32 data, uint32 mask)
{
	// from 8255-1 port.a
	mode = data;
}

void DISPLAY::event_frame()
{
	cblink = (cblink + 1) & 0x1f;
}

void DISPLAY::draw_screen()
{
	if((regs[8] & 0x30) != 0x30) {
		uint16 src = ((regs[12] << 8) | regs[13]) & 0x7ff;
		if((regs[8] & 0xc0) == 0xc0) {
			cursor = -1;
		}
		else {
			cursor = ((regs[14] << 8) | regs[15]) & 0x7ff;
		}
		
		// render screen
		_memset(screen, mode & 7, sizeof(screen));
		
		switch(mode & 0xe0) {
		case 0x00:	// screen 0, wide
			draw_screen0_wide(src);
			break;
		case 0x20:	// screen 0, normal
			draw_screen0_normal(src);
			break;
		case 0x40:	// screen 1, wide
			draw_screen1_wide(src);
			break;
		case 0x60:	// screen 1, normal
			draw_screen1_normal(src);
			break;
		case 0x80:	// screen 2, wide
		case 0xc0:
			draw_screen2_wide(src);
			break;
		case 0xa0:	// screen 2, normal
		case 0xe0:
			draw_screen2_normal(src);
			break;
		}
	}
	else {
		_memset(screen, 0, sizeof(screen));
	}
	
	// copy to real screen
	uint16 bcol = palette_pc[mode & 7];
	for(int y = 0; y < 200; y++) {
		scrntype* dest0 = emu->screen_buffer(y * 2 + 0);
		scrntype* dest1 = emu->screen_buffer(y * 2 + 1);
		uint8* src = screen[y];
		
		for(int x = 0; x < 640; x++) {
			dest0[x] = palette_pc[src[x] & 7];
		}
		if(scanline) {
			_memset(dest1, 0, 640 * sizeof(scrntype));
		}
		else {
			_memcpy(dest1, dest0, 640 * sizeof(scrntype));
		}
	}
}

void DISPLAY::draw_screen0_normal(uint16 src)
{
	// screen 0, normal char (80chars)
	uint16 src_t = src & 0x7ff;
	uint8 c_b = mode & 7;
	int width = regs[1] - 1;
	
	for(int y = 0; y < 200; y += 8) {
		uint8 c_t = vram[src_t] & 7;
		src_t = (src_t & 0x3800) | ((src_t + 1) & 0x7ff);
		
		for(int x = 0; x < width; x++) {
			uint8 code = vram[src_t];
			uint8* font_base = &font[code << 3];
			
			for(int l = 0; l < 8; l++) {
				uint8 p = font_base[l];
				uint8* d = &screen[y + l][x << 3];
				
				d[0] = (p & 0x80) ? c_t : c_b;
				d[1] = (p & 0x40) ? c_t : c_b;
				d[2] = (p & 0x20) ? c_t : c_b;
				d[3] = (p & 0x10) ? c_t : c_b;
				d[4] = (p & 0x08) ? c_t : c_b;
				d[5] = (p & 0x04) ? c_t : c_b;
				d[6] = (p & 0x02) ? c_t : c_b;
				d[7] = (p & 0x01) ? c_t : c_b;
			}
			if(src_t == cursor) {
				int bp = regs[10] & 0x60;
				if(bp == 0 || (bp == 0x40 && (cblink & 8)) || (bp == 0x60 && (cblink & 0x10))) {
					for(int i = (regs[10] & 7); i < 8; i++) {
						_memset(&screen[y + i][x << 3], 7, 8);
					}
				}
			}
			src_t = (src_t & 0x3800) | ((src_t + 1) & 0x7ff);
		}
	}
}

void DISPLAY::draw_screen0_wide(uint16 src)
{
	// screen 0, normal char (80chars)
	uint16 src_t = src & 0x7ff;
	uint8 c_b = mode & 7;
	int width = regs[1] - 1;
	
	for(int y = 0; y < 192; y += 8) {
		uint8 c_t = vram[src_t] & 7;
		src_t = (src_t & 0x3800) | ((src_t + 1) & 0x7ff);
		
		for(int x = 0; x < width; x++) {
			uint8 code = vram[src_t];
			uint8* font_base = &font[code << 3];
			
			for(int l = 0; l < 8; l++) {
				uint8 p = font_base[l];
				uint8* d = &screen[y + l][x << 4];
				
				d[ 0] = d[ 1] = (p & 0x80) ? c_t : c_b;
				d[ 2] = d[ 3] = (p & 0x40) ? c_t : c_b;
				d[ 4] = d[ 5] = (p & 0x20) ? c_t : c_b;
				d[ 6] = d[ 7] = (p & 0x10) ? c_t : c_b;
				d[ 8] = d[ 9] = (p & 0x08) ? c_t : c_b;
				d[10] = d[11] = (p & 0x04) ? c_t : c_b;
				d[12] = d[13] = (p & 0x02) ? c_t : c_b;
				d[14] = d[15] = (p & 0x01) ? c_t : c_b;
			}
			if(src_t == cursor) {
				int bp = regs[10] & 0x60;
				if(bp == 0 || (bp == 0x40 && (cblink & 8)) || (bp == 0x60 && (cblink & 0x10))) {
					for(int i = (regs[10] & 7); i < 8; i++) {
						_memset(&screen[y + i][x << 4], 7, 16);
					}
				}
			}
			src_t = (src_t & 0x3800) | ((src_t + 1) & 0x7ff);
		}
	}
}

void DISPLAY::draw_screen1_normal(uint16 src)
{
	// screen 1, normal char (80chars)
	uint16 src_t = src & 0x7ff;
	uint8 c_b = mode & 7;
	int width = regs[1] - 1;
	
	for(int y = 0; y < 200; y += 8) {
		uint8 c_t = vram[src_t] & 7;
		src_t = (src_t & 0x3800) | ((src_t + 1) & 0x7ff);
		
		for(int x = 0; x < width; x++) {
			uint16 src_g = src_t, src_a = src_t;
			bool graph = false;
			for(int l = 0; l < 8; l++) {
				graph |= attr[src_a & 0x37ff] ? true : false;
				src_a = (src_a + 0x800) & 0x3fff;
			}
			uint8 code = vram[src_t];
			uint8* font_base = &font[code << 3];
			
			for(int l = 0; l < 8; l++) {
				uint8 c_l = c_t, c_r = c_t, p = font_base[l];
				if(graph) {
					if(attr[src_g & 0x37ff]) {
						p = vram[src_g & 0x37ff];
						c_l = (p >> 4) & 7;
						c_r = p & 7;
						p = (p & 0xf0 ? 0xf0 : 0) | (p & 0x0f ? 0x0f : 0);
					}
					else {
						p = 0;
					}
				}
				src_g = (src_g + 0x800) & 0x3fff;
				uint8* d = &screen[y + l][x << 3];
					
				d[0] = (p & 0x80) ? c_l : c_b;
				d[1] = (p & 0x40) ? c_l : c_b;
				d[2] = (p & 0x20) ? c_l : c_b;
				d[3] = (p & 0x10) ? c_l : c_b;
				d[4] = (p & 0x08) ? c_r : c_b;
				d[5] = (p & 0x04) ? c_r : c_b;
				d[6] = (p & 0x02) ? c_r : c_b;
				d[7] = (p & 0x01) ? c_r : c_b;
			}
			if(src_t == cursor) {
				int bp = regs[10] & 0x60;
				if(bp == 0 || (bp == 0x40 && (cblink & 8)) || (bp == 0x60 && (cblink & 0x10))) {
					for(int i = (regs[10] & 7); i < 8; i++) {
						_memset(&screen[y + i][x << 3], 7, 8);
					}
				}
			}
			src_t = (src_t & 0x3800) | ((src_t + 1) & 0x7ff);
		}
	}
}

void DISPLAY::draw_screen1_wide(uint16 src)
{
	// screen 1, normal char (80chars)
	uint16 src_t = src & 0x7ff;
	uint8 c_b = mode & 7;
	int width = regs[1] - 1;
	
	for(int y = 0; y < 192; y += 8) {
		uint8 c_t = vram[src_t] & 7;
		src_t = (src_t & 0x3800) | ((src_t + 1) & 0x7ff);
		
		for(int x = 0; x < width; x++) {
			uint16 src_g = src_t, src_a = src_t;
			bool graph = false;
			for(int l = 0; l < 8; l++) {
				graph |= attr[src_a & 0x37ff] ? true : false;
				src_a = (src_a + 0x800) & 0x3fff;
			}
			uint8 code = vram[src_t];
			uint8* font_base = &font[code << 3];
			
			for(int l = 0; l < 8; l++) {
				uint8 c_l = c_t, c_r = c_t, p = font_base[l];
				if(graph) {
					if(attr[src_g & 0x37ff]) {
						p = vram[src_g & 0x37ff];
						c_l = (p >> 4) & 7;
						c_r = p & 7;
						p = (p & 0xf0 ? 0xf0 : 0) | (p & 0x0f ? 0x0f : 0);
					}
					else {
						p = 0;
					}
				}
				src_g = (src_g + 0x800) & 0x3fff;
				uint8* d = &screen[y + l][x << 4];
				
				d[ 0] = d[ 1] = (p & 0x80) ? c_l : c_b;
				d[ 2] = d[ 3] = (p & 0x40) ? c_l : c_b;
				d[ 4] = d[ 5] = (p & 0x20) ? c_l : c_b;
				d[ 6] = d[ 7] = (p & 0x10) ? c_l : c_b;
				d[ 8] = d[ 9] = (p & 0x08) ? c_r : c_b;
				d[10] = d[11] = (p & 0x04) ? c_r : c_b;
				d[12] = d[13] = (p & 0x02) ? c_r : c_b;
				d[14] = d[15] = (p & 0x01) ? c_r : c_b;
			}
			if(src_t == cursor) {
				int bp = regs[10] & 0x60;
				if(bp == 0 || (bp == 0x40 && (cblink & 8)) || (bp == 0x60 && (cblink & 0x10))) {
					for(int i = (regs[10] & 7); i < 8; i++) {
						_memset(&screen[y + i][x << 4], 7, 16);
					}
				}
			}
			src_t = (src_t & 0x3800) | ((src_t + 1) & 0x7ff);
		}
	}
}

void DISPLAY::draw_screen2_normal(uint16 src)
{
	// screen 2, normal char (80chars)
	uint16 src_t = src & 0x7ff;
	uint8 c_b = mode & 7;
	int width = regs[1] - 1;
	
	for(int y = 0; y < 200; y += 8) {
		uint8 c_t = vram[src_t] & 7;
		src_t = (src_t & 0x3800) | ((src_t + 1) & 0x7ff);
		
		for(int x = 0; x < width; x++) {
			uint16 src_g = src_t, src_a = src_t;
			bool graph = false;
			for(int l = 0; l < 8; l++) {
				graph |= attr[src_a] ? true : false;
				src_a = (src_a + 0x800) & 0x3fff;
			}
			uint8 code = vram[src_t];
			uint8* font_base = &font[code << 3];
			
			for(int l = 0; l < 8; l++) {
				uint8 p = graph ? (attr[src_g] ? vram[src_g] : 0) : font_base[l];
				src_g = (src_g + 0x800) & 0x3fff;
				uint8 c_p = graph ? 7 : c_t;
				uint8* d = &screen[y + l][x << 3];
				
				d[0] = (p & 0x80) ? c_p : c_b;
				d[1] = (p & 0x40) ? c_p : c_b;
				d[2] = (p & 0x20) ? c_p : c_b;
				d[3] = (p & 0x10) ? c_p : c_b;
				d[4] = (p & 0x08) ? c_p : c_b;
				d[5] = (p & 0x04) ? c_p : c_b;
				d[6] = (p & 0x02) ? c_p : c_b;
				d[7] = (p & 0x01) ? c_p : c_b;
			}
			if(src_t == cursor) {
				int bp = regs[10] & 0x60;
				if(bp == 0 || (bp == 0x40 && (cblink & 8)) || (bp == 0x60 && (cblink & 0x10))) {
					for(int i = (regs[10] & 7); i < 8; i++) {
						_memset(&screen[y + i][x << 3], 7, 8);
					}
				}
			}
			src_t = (src_t & 0x3800) | ((src_t + 1) & 0x7ff);
		}
	}
}

void DISPLAY::draw_screen2_wide(uint16 src)
{
	// screen 0, normal char (80chars)
	uint16 src_t = src & 0x7ff;
	uint8 c_b = mode & 7;
	int width = regs[1] - 1;
	
	for(int y = 0; y < 192; y += 8) {
		uint8 c_t = vram[src_t] & 7;
		src_t = (src_t & 0x3800) | ((src_t + 1) & 0x7ff);
		
		for(int x = 0; x < width; x++) {
			uint16 src_g = src_t, src_a = src_t;
			bool graph = false;
			for(int l = 0; l < 8; l++) {
				graph |= attr[src_a] ? true : false;
				src_a = (src_a + 0x800) & 0x3fff;
			}
			uint8 code = vram[src_t];
			uint8* font_base = &font[code << 3];
			
			for(int l = 0; l < 8; l++) {
				uint8 p = graph ? (attr[src_g] ? vram[src_g] : 0) : font_base[l];
				src_g = (src_g + 0x800) & 0x3fff;
				uint8 c_p = graph ? 7 : c_t;
				uint8* d = &screen[y + l][x << 4];
				
				d[ 0] = d[ 1] = (p & 0x80) ? c_p : c_b;
				d[ 2] = d[ 3] = (p & 0x40) ? c_p : c_b;
				d[ 4] = d[ 5] = (p & 0x20) ? c_p : c_b;
				d[ 6] = d[ 7] = (p & 0x10) ? c_p : c_b;
				d[ 8] = d[ 9] = (p & 0x08) ? c_p : c_b;
				d[10] = d[11] = (p & 0x04) ? c_p : c_b;
				d[12] = d[13] = (p & 0x02) ? c_p : c_b;
				d[14] = d[15] = (p & 0x01) ? c_p : c_b;
			}
			if(src_t == cursor) {
				int bp = regs[10] & 0x60;
				if(bp == 0 || (bp == 0x40 && (cblink & 8)) || (bp == 0x60 && (cblink & 0x10))) {
					for(int i = (regs[10] & 7); i < 8; i++) {
						_memset(&screen[y + i][x << 4], 7, 16);
					}
				}
			}
			src_t = (src_t & 0x3800) | ((src_t + 1) & 0x7ff);
		}
	}
}
