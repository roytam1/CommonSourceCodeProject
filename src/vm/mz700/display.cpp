/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	SHARP MZ-800 Emulator 'EmuZ-800'
	SHARP MZ-1500 Emulator 'EmuZ-1500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.05 -

	[ display ]
*/

#include "display.h"
#if defined(_MZ800)
#include "../../config.h"
#endif

void DISPLAY::initialize()
{
#if defined(_MZ800)
	scanline = config.scan_line;
#endif
	
	// create pc palette
	for(int i = 0; i < 8; i++) {
		palette_pc[i] = RGB_COLOR((i & 2) ? 255 : 0, (i & 4) ? 255 : 0, (i & 1) ? 255 : 0);
	}
#if defined(_MZ800)
	for(int i = 0; i < 16; i++) {
		int val = (i & 8) ? 255 : 127;
		palette_mz800_pc[i] = RGB_COLOR((i & 2) ? val : 0, (i & 4) ? val : 0, (i & 1) ? val : 0);
	}
	palette_mz800_pc[8] = palette_mz800_pc[7];
#endif
	
	// regist event
	vm->register_vline_event(this);
}

#if defined(_MZ800)
void DISPLAY::update_config()
{
	scanline = config.scan_line;
}
#endif

void DISPLAY::reset()
{
#if defined(_MZ800)
	sof = 0;
	sw = 125;
	ssa = 0;
	sea = 125;
	bcol = cksw = 0;
	
	// init palette
	palette_sw = 0;
	for(int i = 0; i < 4; i++) {
		palette[i] = i;
	}
	for(int i = 0; i < 16; i++) {
		palette16[i] = i;
	}
#elif defined(_MZ1500)
	// init palette
	for(int i = 0; i < 8; i++) {
		palette[i] = i;
	}
#endif
}

#if defined(_MZ800) || defined(_MZ1500)
void DISPLAY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
#if defined(_MZ800)
	// MZ-800
	case 0xcf:
		switch(addr & 0x7ff) {
		case 0x1cf:
			sof = (sof & 0xf00) | data;
			break;
		case 0x2cf:
			sof = (sof & 0x0ff) | ((data & 3) << 8);
			break;
		case 0x3cf:
			sw = data & 0x7f;
			break;
		case 0x4cf:
			ssa = data & 0x7f;
			break;
		case 0x5cf:
			sea = data & 0x7f;
			break;
		case 0x6cf:
			bcol = data & 0x0f;
			break;
		case 0x7cf:
			cksw = data & 0x80;
			break;
		}
		break;
	case 0xf0:
		if(data & 0x40) {
			palette_sw = (data & 3) << 2;
		}
		else {
			palette[(data >> 4) & 3] = data & 0x0f;
		}
		for(int i = 0; i < 16; i++) {
			palette16[i] = ((i & 0x0c) == palette_sw) ? palette[i & 3] : i;
		}
		break;
#else
	// MZ-1500
	case 0xf0:
		priority = data;
		break;
	case 0xf1:
		palette[(data >> 4) & 7] = data & 7;
		break;
#endif
	}
}
#endif

void DISPLAY::event_vline(int v, int clock)
{
	if(0 <= v && v < 200) {
		// draw one line
#if defined(_MZ800)
		switch(dmd & 0x0f) {
		case 0x00:	// 320x200,4col
		case 0x01:
			draw_line_320x200_2bpp(v);
			break;
		case 0x02:	// 320x200,16col
			draw_line_320x200_4bpp(v);
			break;
		case 0x04:	// 640x200,2col
		case 0x05:
			draw_line_640x200_1bpp(v);
			break;
		case 0x06:	// 640x200,4col
			draw_line_640x200_2bpp(v);
			break;
		case 0x08:	// MZ-700
			draw_line_mz700(v);
			break;
		}
#else
		draw_line_mz700(v);
#endif
	}
}

void DISPLAY::draw_line_mz700(int v)
{
	int ptr = (v >> 3) * 40;
	
	for(int x = 0; x < 320; x += 8) {
		uint8 attr = vram[ptr | 0x800];
#if defined(_MZ1500)
		uint8 pcg_attr = vram[ptr | 0xc00];
#endif
		uint16 code = (vram[ptr] << 3) | ((attr & 0x80) << 4);
		uint8 col_b = attr & 7;
		uint8 col_f = (attr >> 4) & 7;
		uint8 pat_t = font[code | (v & 7)];
		uint8* dest = &screen[v][x];
		
#if defined(_MZ1500)
		if((priority & 1) && (pcg_attr & 8)) {
			uint16 pcg_code = (vram[ptr | 0x400] << 3) | ((pcg_attr & 0xc0) << 5);
			uint8 pcg_dot[8];
			uint8 pat_b = pcg[pcg_code | (v & 7) | 0x0000];
			uint8 pat_r = pcg[pcg_code | (v & 7) | 0x2000];
			uint8 pat_g = pcg[pcg_code | (v & 7) | 0x4000];
			pcg_dot[0] = ((pat_b & 0x80) >> 7) | ((pat_r & 0x80) >> 6) | ((pat_g & 0x80) >> 5);
			pcg_dot[1] = ((pat_b & 0x40) >> 6) | ((pat_r & 0x40) >> 5) | ((pat_g & 0x40) >> 4);
			pcg_dot[2] = ((pat_b & 0x20) >> 5) | ((pat_r & 0x20) >> 4) | ((pat_g & 0x20) >> 3);
			pcg_dot[3] = ((pat_b & 0x10) >> 4) | ((pat_r & 0x10) >> 3) | ((pat_g & 0x10) >> 2);
			pcg_dot[4] = ((pat_b & 0x08) >> 3) | ((pat_r & 0x08) >> 2) | ((pat_g & 0x08) >> 1);
			pcg_dot[5] = ((pat_b & 0x04) >> 2) | ((pat_r & 0x04) >> 1) | ((pat_g & 0x04) >> 0);
			pcg_dot[6] = ((pat_b & 0x02) >> 1) | ((pat_r & 0x02) >> 0) | ((pat_g & 0x02) << 1);
			pcg_dot[7] = ((pat_b & 0x01) >> 0) | ((pat_r & 0x01) << 1) | ((pat_g & 0x01) << 2);
			
			if(priority & 2) {
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
#if defined(_MZ800)
			dest[0] = (pat_t & 0x01) ? col_f : col_b;
			dest[1] = (pat_t & 0x02) ? col_f : col_b;
			dest[2] = (pat_t & 0x04) ? col_f : col_b;
			dest[3] = (pat_t & 0x08) ? col_f : col_b;
			dest[4] = (pat_t & 0x10) ? col_f : col_b;
			dest[5] = (pat_t & 0x20) ? col_f : col_b;
			dest[6] = (pat_t & 0x40) ? col_f : col_b;
			dest[7] = (pat_t & 0x80) ? col_f : col_b;
#else
			dest[0] = (pat_t & 0x80) ? col_f : col_b;
			dest[1] = (pat_t & 0x40) ? col_f : col_b;
			dest[2] = (pat_t & 0x20) ? col_f : col_b;
			dest[3] = (pat_t & 0x10) ? col_f : col_b;
			dest[4] = (pat_t & 0x08) ? col_f : col_b;
			dest[5] = (pat_t & 0x04) ? col_f : col_b;
			dest[6] = (pat_t & 0x02) ? col_f : col_b;
			dest[7] = (pat_t & 0x01) ? col_f : col_b;
#endif
#if defined(_MZ1500)
		}
#endif
		ptr++;
	}
}

#if defined(_MZ800)
void DISPLAY::draw_line_320x200_2bpp(int v)
{
	int l = v >> 3;
	if((int)(ssa / 5) <= l && l < (int)(sea / 5)) {
		l += (int)(sof / 5);
	}
	int ptr = 40 * (l * 8 + (v & 7));
	int ofs1 = (dmd & 1) ? 0x4000 : 0;
	int ofs2 = ofs1 | 0x2000;
	
	for(int x = 0; x < 320; x += 8) {
		ptr &= 0x1fff;
		uint8 pat1 = vram_mz800[ptr | ofs1];
		uint8 pat2 = vram_mz800[ptr | ofs2];
		ptr++;
		uint8* dest = &screen[v][x];
		
		dest[0] = palette[((pat1 & 0x01)     ) | ((pat2 & 0x01) << 1)];
		dest[1] = palette[((pat1 & 0x02) >> 1) | ((pat2 & 0x02)     )];
		dest[2] = palette[((pat1 & 0x04) >> 2) | ((pat2 & 0x04) >> 1)];
		dest[3] = palette[((pat1 & 0x08) >> 3) | ((pat2 & 0x08) >> 2)];
		dest[4] = palette[((pat1 & 0x10) >> 4) | ((pat2 & 0x10) >> 3)];
		dest[5] = palette[((pat1 & 0x20) >> 5) | ((pat2 & 0x20) >> 4)];
		dest[6] = palette[((pat1 & 0x40) >> 6) | ((pat2 & 0x40) >> 5)];
		dest[7] = palette[((pat1 & 0x80) >> 7) | ((pat2 & 0x80) >> 6)];
	}
}

void DISPLAY::draw_line_320x200_4bpp(int v)
{
	int l = v >> 3;
	if((int)(ssa / 5) <= l && l < (int)(sea / 5)) {
		l += (int)(sof / 5);
	}
	int ptr = 40 * (l * 8 + (v & 7));
	
	for(int x = 0; x < 320; x += 8) {
		ptr &= 0x1fff;
		uint8 pat1 = vram_mz800[ptr         ];
		uint8 pat2 = vram_mz800[ptr | 0x2000];
		uint8 pat3 = vram_mz800[ptr | 0x4000];
		uint8 pat4 = vram_mz800[ptr | 0x6000];
		ptr++;
		uint8* dest = &screen[v][x];
		
		dest[0] = palette16[((pat1 & 0x01)     ) | ((pat2 & 0x01) << 1) | ((pat3 & 0x01) << 2) | ((pat4 & 0x01) << 3)];
		dest[1] = palette16[((pat1 & 0x02) >> 1) | ((pat2 & 0x02)     ) | ((pat3 & 0x02) << 1) | ((pat4 & 0x02) << 2)];
		dest[2] = palette16[((pat1 & 0x04) >> 2) | ((pat2 & 0x04) >> 1) | ((pat3 & 0x04)     ) | ((pat4 & 0x04) << 1)];
		dest[3] = palette16[((pat1 & 0x08) >> 3) | ((pat2 & 0x08) >> 2) | ((pat3 & 0x08) >> 1) | ((pat4 & 0x08)     )];
		dest[4] = palette16[((pat1 & 0x10) >> 4) | ((pat2 & 0x10) >> 3) | ((pat3 & 0x10) >> 2) | ((pat4 & 0x10) >> 1)];
		dest[5] = palette16[((pat1 & 0x20) >> 5) | ((pat2 & 0x20) >> 4) | ((pat3 & 0x20) >> 3) | ((pat4 & 0x20) >> 2)];
		dest[6] = palette16[((pat1 & 0x40) >> 6) | ((pat2 & 0x40) >> 5) | ((pat3 & 0x40) >> 4) | ((pat4 & 0x40) >> 3)];
		dest[7] = palette16[((pat1 & 0x80) >> 7) | ((pat2 & 0x80) >> 6) | ((pat3 & 0x80) >> 5) | ((pat4 & 0x80) >> 4)];
	}
}

void DISPLAY::draw_line_640x200_1bpp(int v)
{
	int l = v >> 3;
	if((int)(ssa / 5) <= l && l < (int)(sea / 5)) {
		l += (int)(sof / 5);
	}
	int ptr = 80 * (l * 8 + (v & 7));
	int ofs = (dmd & 1) ? 0x4000 : 0;
	
	for(int x = 0; x < 640; x += 8) {
		ptr &= 0x3fff;
		uint8 pat = vram_mz800[ptr | ofs];
		ptr++;
		uint8* dest = &screen[v][x];
		
		dest[0] = palette[(pat & 0x01)     ];
		dest[1] = palette[(pat & 0x02) >> 1];
		dest[2] = palette[(pat & 0x04) >> 2];
		dest[3] = palette[(pat & 0x08) >> 3];
		dest[4] = palette[(pat & 0x10) >> 4];
		dest[5] = palette[(pat & 0x20) >> 5];
		dest[6] = palette[(pat & 0x40) >> 6];
		dest[7] = palette[(pat & 0x80) >> 7];
	}
}

void DISPLAY::draw_line_640x200_2bpp(int v)
{
	int l = v >> 3;
	if((int)(ssa / 5) <= l && l < (int)(sea / 5)) {
		l += (int)(sof / 5);
	}
	int ptr = 80 * (l * 8 + (v & 7));
	
	for(int x = 0; x < 640; x += 8) {
		ptr &= 0x3fff;
		uint8 pat1 = vram_mz800[ptr         ];
		uint8 pat2 = vram_mz800[ptr | 0x4000];
		ptr++;
		uint8* dest = &screen[v][x];
		
		dest[0] = palette[((pat1 & 0x01)     ) | ((pat2 & 0x01) << 1)];
		dest[1] = palette[((pat1 & 0x02) >> 1) | ((pat2 & 0x02)     )];
		dest[2] = palette[((pat1 & 0x04) >> 2) | ((pat2 & 0x04) >> 1)];
		dest[3] = palette[((pat1 & 0x08) >> 3) | ((pat2 & 0x08) >> 2)];
		dest[4] = palette[((pat1 & 0x10) >> 4) | ((pat2 & 0x10) >> 3)];
		dest[5] = palette[((pat1 & 0x20) >> 5) | ((pat2 & 0x20) >> 4)];
		dest[6] = palette[((pat1 & 0x40) >> 6) | ((pat2 & 0x40) >> 5)];
		dest[7] = palette[((pat1 & 0x80) >> 7) | ((pat2 & 0x80) >> 6)];
	}
}
#endif

void DISPLAY::draw_screen()
{
	// copy to real screen
#if defined(_MZ800)
	for(int y = 0; y < 200; y++) {
		scrntype* dest0 = emu->screen_buffer(2 * y);
		scrntype* dest1 = emu->screen_buffer(2 * y + 1);
		uint8* src = screen[y];
		
		if(dmd & 8) {
			// MZ-700 mode
			for(int x = 0, x2 = 0; x < 320; x++, x2 += 2) {
				dest0[x2] = dest0[x2 + 1] = palette_pc[src[x] & 7];
			}
		}
		else if(dmd & 4) {
			// 640x200
			for(int x = 0; x < 640; x++) {
				dest0[x] = palette_mz800_pc[src[x] & 15];
			}
		}
		else {
			// 320x200
			for(int x = 0, x2 = 0; x < 320; x++, x2 += 2) {
				dest0[x2] = dest0[x2 + 1] = palette_mz800_pc[src[x] & 15];
			}
		}
		if(!scanline) {
			_memcpy(dest1, dest0, 640 * sizeof(scrntype));
		}
		else {
			_memset(dest1, 0, 640 * sizeof(scrntype));
		}
	}
#else
	for(int y = 0; y < 200; y++) {
		scrntype* dest = emu->screen_buffer(y);
		uint8* src = screen[y];
		
		for(int x = 0; x < 320; x++) {
#if defined(_MZ1500)
			dest[x] = palette_pc[palette[src[x] & 7]];
#else
			dest[x] = palette_pc[src[x] & 7];
#endif
		}
	}
#endif
#if defined(_MZ800) || defined(_MZ1500)
	// access lamp
	uint32 stat_f = d_fdc->read_signal(0) | d_qd->read_signal(0);
	if(stat_f) {
		scrntype col = (stat_f & (1 | 4)) ? RGB_COLOR(255, 0, 0) :
		               (stat_f & (2 | 8)) ? RGB_COLOR(0, 255, 0) : 0;
		for(int y = SCREEN_HEIGHT - 8; y < SCREEN_HEIGHT; y++) {
			scrntype *dest = emu->screen_buffer(y);
			for(int x = SCREEN_WIDTH - 8; x < SCREEN_WIDTH; x++) {
				dest[x] = col;
			}
		}
	}
#endif
}

