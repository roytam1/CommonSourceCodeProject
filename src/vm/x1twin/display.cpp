/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

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
	
	_stprintf(file_path, _T("%sANK8.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(font, sizeof(font), 1);
		fio->Fclose();
	}
	// xmillenium rom
	_stprintf(file_path, _T("%sFNT0808.X1"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(font, sizeof(font), 1);
		fio->Fclose();
	}
	delete fio;
	
	// create pc palette
	for(int i = 0; i < 8; i++) {
		palette_pc[i] = RGB_COLOR((i & 2) ? 255 : 0, (i & 4) ? 255 : 0, (i & 1) ? 255 : 0);
	}
	
	// initialize regs
	pal[0] = 0xaa;
	pal[1] = 0xcc;
	pal[2] = 0xf0;
	priority = 0;
	update_pal();
	column = 0x40;
	
	_memset(vram_t, 0, sizeof(vram_t));
	_memset(vram_a, 0, sizeof(vram_a));
#ifdef _X1TURBO
	_memset(vram_k, 0, sizeof(vram_k));
#endif
	_memset(pcg_b, 0, sizeof(pcg_b));
	_memset(pcg_r, 0, sizeof(pcg_r));
	_memset(pcg_g, 0, sizeof(pcg_g));
	
	// regist event
	vm->regist_frame_event(this);
	vm->regist_vline_event(this);
}

void DISPLAY::reset()
{
#ifdef _X1TURBO
	mode1 = mode2 = 0;
#endif
	vram_b = vram_ptr + 0x0000;
	vram_r = vram_ptr + 0x4000;
	vram_g = vram_ptr + 0x8000;
}

void DISPLAY::update_config()
{
	scanline = config.scan_line;
}

void DISPLAY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff00) {
	case 0x1000:
		pal[0] = data;
		update_pal();
		break;
	case 0x1100:
		pal[1] = data;
		update_pal();
		break;
	case 0x1200:
		pal[2] = data;
		update_pal();
		break;
	case 0x1300:
		priority = data;
		update_pal();
		break;
	case 0x1500:
		get_cur_code();
//		if(cur_attr & 0x20) {
			pcg_b[cur_code][cur_line & 7] = data;
//		}
		break;
	case 0x1600:
		get_cur_code();
//		if(cur_attr & 0x20) {
			pcg_r[cur_code][cur_line & 7] = data;
//		}
		break;
	case 0x1700:
		get_cur_code();
//		if(cur_attr & 0x20) {
			pcg_g[cur_code][cur_line & 7] = data;
//		}
		break;
#ifdef _X1TURBO
	case 0x1fd0:
		if((mode1 & 8) != (data & 8)) {
			int ofs = (data & 8) ? 0xc000 : 0;
			vram_b = vram_ptr + 0x0000 + ofs;
			vram_r = vram_ptr + 0x4000 + ofs;
			vram_g = vram_ptr + 0x8000 + ofs;
		}
		mode1 = data;
		break;
	case 0x1fe0:
		mode2 = data;
		break;
#endif
	case 0x2000:
	case 0x2100:
	case 0x2200:
	case 0x2300:
	case 0x2400:
	case 0x2500:
	case 0x2600:
	case 0x2700:
	case 0x2800:
	case 0x2900:
	case 0x2a00:
	case 0x2b00:
	case 0x2c00:
	case 0x2d00:
	case 0x2e00:
	case 0x2f00:
		vram_a[addr & 0x7ff] = data;
		break;
	case 0x3000:
	case 0x3100:
	case 0x3200:
	case 0x3300:
	case 0x3400:
	case 0x3500:
	case 0x3600:
	case 0x3700:
		vram_t[addr & 0x7ff] = data;
		break;
	case 0x3800:
	case 0x3900:
	case 0x3a00:
	case 0x3b00:
	case 0x3c00:
	case 0x3d00:
	case 0x3e00:
	case 0x3f00:
#ifdef _X1TURBO
		vram_k[addr & 0x7ff] = data;
#else
		vram_t[addr & 0x7ff] = data;
#endif
		break;
	}
}

uint32 DISPLAY::read_io8(uint32 addr)
{
	switch(addr & 0xff00) {
	case 0x1000:
		return pal[0];
	case 0x1100:
		return pal[1];
	case 0x1200:
		return pal[2];
	case 0x1300:
		return priority;
	case 0x1400:
		get_cur_code();
//		if(cur_attr & 0x20) {
//			return pcg_b[cur_code][cur_line & 7];	// blue ???
//		}
		return font[(cur_code << 3) | (cur_line & 7)];
	case 0x1500:
		get_cur_code();
//		if(cur_attr & 0x20) {
			return pcg_b[cur_code][cur_line & 7];
//		}
//		return font[(cur_code << 3) | (cur_line & 7)];
	case 0x1600:
		get_cur_code();
//		if(cur_attr & 0x20) {
			return pcg_r[cur_code][cur_line & 7];
//		}
//		return font[(cur_code << 3) | (cur_line & 7)];
	case 0x1700:
		get_cur_code();
//		if(cur_attr & 0x20) {
			return pcg_g[cur_code][cur_line & 7];
//		}
//		return font[(cur_code << 3) | (cur_line & 7)];
#ifdef _X1TURBO
	case 0x1fd0:
		return mode1;
	case 0x1fe0:
		return mode2;
#endif
	case 0x2000:
	case 0x2100:
	case 0x2200:
	case 0x2300:
	case 0x2400:
	case 0x2500:
	case 0x2600:
	case 0x2700:
	case 0x2800:
	case 0x2900:
	case 0x2a00:
	case 0x2b00:
	case 0x2c00:
	case 0x2d00:
	case 0x2e00:
	case 0x2f00:
		return vram_a[addr & 0x7ff];
	case 0x3000:
	case 0x3100:
	case 0x3200:
	case 0x3300:
	case 0x3400:
	case 0x3500:
	case 0x3600:
	case 0x3700:
		return vram_t[addr & 0x7ff];
	case 0x3800:
	case 0x3900:
	case 0x3a00:
	case 0x3b00:
	case 0x3c00:
	case 0x3d00:
	case 0x3e00:
	case 0x3f00:
#ifdef _X1TURBO
		retuan vram_k[addr & 0x7ff];
#else
		return vram_t[addr & 0x7ff];
#endif
	}
	return 0xff;
}

void DISPLAY::write_signal(int id, uint32 data, uint32 mask)
{
	column = data & mask;
}

void DISPLAY::event_frame()
{
	cblink = (cblink + 1) & 0x1f;
}

void DISPLAY::event_vline(int v, int clock)
{
	vline = v;
	vclock = vm->current_clock();
	prev_clock = 0;
	
	// render screen
	if(v == 0) {
		_memset(text, 0, sizeof(text));
		_memset(cg, 0, sizeof(cg));
		_memset(prev_top, 0, sizeof(prev_top));
	}
	if(v < 200) {
		if((regs[8] & 0x30) != 0x30) {
			int ht = (regs[9] & 0x1f) + 1;
			if((v % ht) == 0) {
				draw_text(v / ht);
			}
			draw_cg(v);
			_memcpy(&pri_line[v][0][0], &pri[0][0], sizeof(pri));
		}
		else {
			_memset(&pri_line[v][0][0], 0, sizeof(pri));
		}
	}
}

void DISPLAY::update_pal()
{
	uint8 pal2[8];
	for(int i = 0; i < 8; i++) {
		uint8 bit = 1 << i;
		pal2[i] = ((pal[0] & bit) ? 1 : 0) | ((pal[1] & bit) ? 2 : 0) | ((pal[2] & bit) ? 4 : 0);
	}
	for(int c = 0; c < 8; c++) {
		uint8 bit = 1 << c;
		for(int t = 0; t < 8; t++) {
			if(priority & bit) {
				pri[c][t] = pal2[c];
			}
			else if(t) {
				pri[c][t] = t;
			}
			else {
				pri[c][t] = pal2[c];
			}
		}
	}
}

void DISPLAY::get_cur_code()
{
	/*
		NOTE: don't update pcg addr frequently to write r/g/b patterns to one pcg (wibarm)
	
		CLOCKS_PER_LINE = 250
		CLOCKS_PER_LINE / CHARS_PER_LINE * 40 = 87.8
	*/
	if(vm->passed_clock(prev_clock) > 87) {
		int ofs = (regs[0] + 1) * vm->passed_clock(vclock) / 250;
		if(ofs >= regs[1]) {
			ofs = regs[1] - 1;
		}
		int ht = (regs[9] & 0x1f) + 1;
		ofs += regs[1] * (int)(vline / ht);
		ofs += (regs[12] << 8) | regs[13];
		cur_code = vram_t[ofs & 0x7ff];
		cur_attr = vram_a[ofs & 0x7ff];
		cur_line = vline % ht;
		prev_clock = vm->current_clock();
	}
}

void DISPLAY::draw_screen()
{
	// copy to real screen
	if(column & 0x40) {
		// column 40
		for(int y = 0; y < 200; y++) {
			scrntype* dest0 = emu->screen_buffer(y * 2 + 0);
			scrntype* dest1 = emu->screen_buffer(y * 2 + 1);
			uint8* src_text = text[y];
			uint8* src_cg = cg[y];
			
			for(int x = 0, x2 = 0; x < 320; x++, x2 += 2) {
				dest0[x2] = dest0[x2 + 1] = palette_pc[pri_line[y][src_cg[x]][src_text[x]]];
			}
			if(scanline) {
				_memset(dest1, 0, 640 * sizeof(scrntype));
			}
			else {
				_memcpy(dest1, dest0, 640 * sizeof(scrntype));
			}
		}
	}
	else {
		// column 80
		for(int y = 0; y < 200; y++) {
			scrntype* dest0 = emu->screen_buffer(y * 2 + 0);
			scrntype* dest1 = emu->screen_buffer(y * 2 + 1);
			uint8* src_text = text[y];
			uint8* src_cg = cg[y];
			
			for(int x = 0; x < 640; x++) {
				dest0[x] = palette_pc[pri_line[y][src_cg[x]][src_text[x]]];
			}
			if(scanline) {
				_memset(dest1, 0, 640 * sizeof(scrntype));
			}
			else {
				_memcpy(dest1, dest0, 640 * sizeof(scrntype));
			}
		}
	}
	
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

void DISPLAY::draw_text(int y)
{
	int width = (column & 0x40) ? 40 : 80;
	int hz = regs[1];
	int vt = regs[6] & 0x7f;
	int ht = (regs[9] & 0x1f) + 1;
	uint16 src = ((regs[12] << 8) | regs[13]) + hz * y;
	
	for(int x = 0; x < hz && x < width; x++) {
		src &= 0x7ff;
		uint8 code = vram_t[src];
		uint8 attr = vram_a[src];
		uint8 col = ((attr & 0x10) && (cblink & 8)) ? 0 : (attr & 7);
		
		// select pcg or ank
		static const uint8 null_pattern[8] = {0, 0, 0, 0, 0, 0, 0, 0};
		const uint8 *pattern_b, *pattern_r, *pattern_g;
		if(attr & 0x20) {
			// pcg
			pattern_b = (col & 1) ? pcg_b[code] : null_pattern;
			pattern_r = (col & 2) ? pcg_r[code] : null_pattern;
			pattern_g = (col & 4) ? pcg_g[code] : null_pattern;
		}
		else {
			// ank
			pattern_b = (col & 1) ? &font[code << 3] : null_pattern;
			pattern_r = (col & 2) ? &font[code << 3] : null_pattern;
			pattern_g = (col & 4) ? &font[code << 3] : null_pattern;
		}
		
		// check vertical doubled char
		uint8 is_top = 0, is_bottom = prev_top[x];
		if(attr & 0x40) {
			if(is_bottom) {
				// bottom 4 rasters of vertical doubled char
				pattern_b += 4;
				pattern_r += 4;
				pattern_g += 4;
			}
			else {
				// check next line
				uint8 next_code = 0, next_attr = 0;
				if(y < vt - 1) {
					uint16 addr = (src + hz) & 0x7ff;
					next_code = vram_t[addr];
					next_attr = vram_a[addr];
				}
				if(code == next_code && attr == next_attr) {
					// top 4 rasters of vertical doubled char
					is_top = 1;
				}
				else {
					// this is not vertical doubled char !!!
					attr &= ~0x40;
				}
			}
		}
		prev_top[x] = is_top;
		
		// render character
		for(int l = 0; l < 8 && l < ht; l++) {
			int line = (attr & 0x40) ? (l >> 1) : l;
			uint8 b = (attr & 8) ? ~pattern_b[line] : pattern_b[line];
			uint8 r = (attr & 8) ? ~pattern_r[line] : pattern_r[line];
			uint8 g = (attr & 8) ? ~pattern_g[line] : pattern_g[line];
			int yy = y * ht + l;
			if(yy >= 200) {
				break;
			}
			uint8* d = &text[yy][x << 3];
			
			if(attr & 0x80) {
				// horizontal doubled char
				d[ 0] = d[ 1] = ((b & 0x80) >> 7) | ((r & 0x80) >> 6) | ((g & 0x80) >> 5);
				d[ 2] = d[ 3] = ((b & 0x40) >> 6) | ((r & 0x40) >> 5) | ((g & 0x40) >> 4);
				d[ 4] = d[ 5] = ((b & 0x20) >> 5) | ((r & 0x20) >> 4) | ((g & 0x20) >> 3);
				d[ 6] = d[ 7] = ((b & 0x10) >> 4) | ((r & 0x10) >> 3) | ((g & 0x10) >> 2);
				d[ 8] = d[ 9] = ((b & 0x08) >> 3) | ((r & 0x08) >> 2) | ((g & 0x08) >> 1);
				d[10] = d[11] = ((b & 0x04) >> 2) | ((r & 0x04) >> 1) | ((g & 0x04) >> 0);
				d[12] = d[13] = ((b & 0x02) >> 1) | ((r & 0x02) >> 0) | ((g & 0x02) << 1);
				d[14] = d[15] = ((b & 0x01) >> 0) | ((r & 0x01) << 1) | ((g & 0x01) << 2);
			}
			else {
				d[0] = ((b & 0x80) >> 7) | ((r & 0x80) >> 6) | ((g & 0x80) >> 5);
				d[1] = ((b & 0x40) >> 6) | ((r & 0x40) >> 5) | ((g & 0x40) >> 4);
				d[2] = ((b & 0x20) >> 5) | ((r & 0x20) >> 4) | ((g & 0x20) >> 3);
				d[3] = ((b & 0x10) >> 4) | ((r & 0x10) >> 3) | ((g & 0x10) >> 2);
				d[4] = ((b & 0x08) >> 3) | ((r & 0x08) >> 2) | ((g & 0x08) >> 1);
				d[5] = ((b & 0x04) >> 2) | ((r & 0x04) >> 1) | ((g & 0x04) >> 0);
				d[6] = ((b & 0x02) >> 1) | ((r & 0x02) >> 0) | ((g & 0x02) << 1);
				d[7] = ((b & 0x01) >> 0) | ((r & 0x01) << 1) | ((g & 0x01) << 2);
			}
		}
		if(attr & 0x80) {
			// skip next one char
			src++;
			x++;
		}
		src++;
	}
}

void DISPLAY::draw_cg(int line)
{
	int width = (column & 0x40) ? 40 : 80;
	int hz = regs[1];
	int vt = regs[6] & 0x7f;
	int ht = (regs[9] & 0x1f) + 1;
	
	int y = line / ht;
	int l = line % ht;
	if(y >= vt) {
		return;
	}
	uint16 src = ((regs[12] << 8) | regs[13]) + hz * y;
	uint16 ofs = 0x800 * l;
	
	for(int x = 0; x < hz && x < width; x++) {
		src &= 0x7ff;
		uint8 b = vram_b[ofs | src];
		uint8 r = vram_r[ofs | src];
		uint8 g = vram_g[ofs | src++];
		uint8* d = &cg[line][x << 3];
		
		d[0] = ((b & 0x80) >> 7) | ((r & 0x80) >> 6) | ((g & 0x80) >> 5);
		d[1] = ((b & 0x40) >> 6) | ((r & 0x40) >> 5) | ((g & 0x40) >> 4);
		d[2] = ((b & 0x20) >> 5) | ((r & 0x20) >> 4) | ((g & 0x20) >> 3);
		d[3] = ((b & 0x10) >> 4) | ((r & 0x10) >> 3) | ((g & 0x10) >> 2);
		d[4] = ((b & 0x08) >> 3) | ((r & 0x08) >> 2) | ((g & 0x08) >> 1);
		d[5] = ((b & 0x04) >> 2) | ((r & 0x04) >> 1) | ((g & 0x04) >> 0);
		d[6] = ((b & 0x02) >> 1) | ((r & 0x02) >> 0) | ((g & 0x02) << 1);
		d[7] = ((b & 0x01) >> 0) | ((r & 0x01) << 1) | ((g & 0x01) << 2);
	}
}

