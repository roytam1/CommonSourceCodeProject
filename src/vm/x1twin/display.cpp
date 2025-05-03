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
	for(int i = 0; i < 8; i++)
		palette_pc[i] = RGB_COLOR((i & 2) ? 255 : 0, (i & 4) ? 255 : 0, (i & 1) ? 255 : 0);
	
	// initialize regs
	pal[0] = 0xaa;
	pal[1] = 0xcc;
	pal[2] = 0xf0;
	priority = 0;
	update_pal();
	column = 0x40;
	
	// regist event
	vm->regist_frame_event(this);
	vm->regist_vline_event(this);
}

void DISPLAY::reset()
{
	_memset(vram_t, 0, sizeof(vram_t));
	_memset(vram_a, 0, sizeof(vram_a));
}

void DISPLAY::update_config()
{
	scanline = config.scan_line;
}

void DISPLAY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff00)
	{
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
		get_cgnum();
		pcg_b[cgnum][vline & 7] = data;
		break;
	case 0x1600:
		get_cgnum();
		pcg_r[cgnum][vline & 7] = data;
		break;
	case 0x1700:
		get_cgnum();
		pcg_g[cgnum][vline & 7] = data;
		break;
	case 0x2000:
	case 0x2100:
	case 0x2200:
	case 0x2300:
	case 0x2400:
	case 0x2500:
	case 0x2600:
	case 0x2700:
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
	}
}

uint32 DISPLAY::read_io8(uint32 addr)
{
	switch(addr & 0xff00)
	{
	case 0x1000:
		return pal[0];
	case 0x1100:
		return pal[1];
	case 0x1200:
		return pal[2];
	case 0x1300:
		return priority;
	case 0x1400:
		get_cgnum();
		return font[(cgnum << 3) | (vline & 7)];
	case 0x1500:
		get_cgnum();
		return pcg_b[cgnum][vline & 7];
	case 0x1600:
		get_cgnum();
		return pcg_b[cgnum][vline & 7];
	case 0x1700:
		get_cgnum();
		return pcg_b[cgnum][vline & 7];
	case 0x2000:
	case 0x2100:
	case 0x2200:
	case 0x2300:
	case 0x2400:
	case 0x2500:
	case 0x2600:
	case 0x2700:
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
}

void DISPLAY::update_pal()
{
	if(!(pal[0] || pal[1] || pal[2])) {
		pal[0] = 0xaa;
		pal[1] = 0xcc;
		pal[2] = 0xf0;
	}
	uint8 pal2[8];
	for(int i = 0; i < 8; i++) {
		uint8 bit = 1 << i;
		pal2[i] = ((pal[0] & bit) ? 1 : 0) | ((pal[1] & bit) ? 2 : 0) | ((pal[2] & bit) ? 4 : 0);
	}
	for(int c = 0; c < 8; c++) {
		uint8 bit = 1 << c;
		for(int t = 0; t < 8; t++) {
			if(priority & bit)
				pri[c][t] = pal2[c];
			else if(t)
				pri[c][t] = pal2[t];
			else
				pri[c][t] = pal2[c];
		}
	}
}

void DISPLAY::get_cgnum()
{
	int ofs = (regs[0] + 1) * vm->passed_clock(vclock) / 250;	// 250clocks/line
	if(ofs >= regs[1])
		ofs = regs[1] - 1;
	int ht = ((regs[9] <= 9) ? regs[9] : 9) + 1;
	ofs += regs[1] * (int)(vline / ht);
	ofs += (regs[12] << 8) | regs[13];
	cgnum = vram_t[ofs & 0x7ff];
}

void DISPLAY::draw_screen()
{
	// clear screen buffer
	_memset(text, 0, sizeof(text));
	_memset(cg, 0, sizeof(cg));
	
	if(column & 0x40) {
		// column 40
		if((regs[8] & 0x30) != 0x30) {
			// render screen
			draw_text(40);
			draw_cg(40);
		}
		
		// copy to real screen
		for(int y = 0; y < 200; y++) {
			scrntype* dest0 = emu->screen_buffer(y * 2 + 0);
			scrntype* dest1 = emu->screen_buffer(y * 2 + 1);
			uint8* src_text = text[y];
			uint8* src_cg = cg[y];
			
			for(int x = 0, x2 = 0; x < 320; x++, x2 += 2)
				dest0[x2] = dest0[x2 + 1] = palette_pc[pri[src_cg[x]][src_text[x]]];
			if(scanline) {
//				for(int x = 0; x < 640; x++)
//					dest1[x] = palette_pc[0];
				_memset(dest1, 0, 640 * sizeof(scrntype));
			}
			else
				_memcpy(dest1, dest0, 640 * sizeof(scrntype));
		}
	}
	else {
		// column 80
		if((regs[8] & 0x30) != 0x30) {
			// render screen
			draw_text(80);
			draw_cg(80);
		}
		
		// copy to real screen
		for(int y = 0; y < 200; y++) {
			scrntype* dest0 = emu->screen_buffer(y * 2 + 0);
			scrntype* dest1 = emu->screen_buffer(y * 2 + 1);
			uint8* src_text = text[y];
			uint8* src_cg = cg[y];
			
			for(int x = 0; x < 640; x++)
				dest0[x] = palette_pc[pri[src_cg[x]][src_text[x]]];
			if(scanline) {
//				for(int x = 0; x < 640; x++)
//					dest1[x] = palette_pc[0];
				_memset(dest1, 0, 640 * sizeof(scrntype));
			}
			else
				_memcpy(dest1, dest0, 640 * sizeof(scrntype));
		}
	}
	
	// access lamp
	uint32 stat_f = d_fdc->read_signal(0);
	if(stat_f) {
		scrntype col = (stat_f & (1 | 4)) ? RGB_COLOR(255, 0, 0) :
		               (stat_f & (2 | 8)) ? RGB_COLOR(0, 255, 0) : 0;
		for(int y = 400 - 8; y < 400; y++) {
			scrntype *dest = emu->screen_buffer(y);
			for(int x = 640 - 8; x < 640; x++)
				dest[x] = col;
		}
	}
}

void DISPLAY::draw_text(int width)
{
	uint16 src = ((regs[12] << 8) | regs[13]) & 0x7ff;
	int hz = (regs[1] <= width) ? regs[1] : width;
	int vt = (regs[6] <= 25) ? regs[6] : 25;
	int ht = ((regs[9] <= 9) ? regs[9] : 9) + 1;
	
	for(int y = 0; y < vt; y++) {
		for(int x = 0; x < hz; x++) {
			int x8 = x << 3;
			uint8 code = vram_t[src];
			uint8 attr = vram_a[src];
			src = (src + 1) & 0x7ff;
			
			uint8 col = ((attr & 0x10) && (cblink & 8)) ? 0 : (attr & 7);
			if(attr & 0x20) {
				// pcg
				if(attr & 0x80) {
					// wide
					for(int l = 0; l < 8; l++) {
						uint8 b = (col & 1) ? pcg_b[code][l] : 0;
						uint8 r = (col & 2) ? pcg_r[code][l] : 0;
						uint8 g = (col & 4) ? pcg_g[code][l] : 0;
						b = (attr & 8) ? ~b : b;
						r = (attr & 8) ? ~r : r;
						g = (attr & 8) ? ~g : g;
						int yy = y * ht + l;
						if(yy >= 200)
							break;
						uint8* d = &text[yy][x8];
						
						d[ 0] = d[ 1] = ((b & 0x80) >> 7) | ((r & 0x80) >> 6) | ((g & 0x80) >> 5);
						d[ 2] = d[ 3] = ((b & 0x40) >> 6) | ((r & 0x40) >> 5) | ((g & 0x40) >> 4);
						d[ 4] = d[ 5] = ((b & 0x20) >> 5) | ((r & 0x20) >> 4) | ((g & 0x20) >> 3);
						d[ 6] = d[ 7] = ((b & 0x10) >> 4) | ((r & 0x10) >> 3) | ((g & 0x10) >> 2);
						d[ 8] = d[ 9] = ((b & 0x08) >> 3) | ((r & 0x08) >> 2) | ((g & 0x08) >> 1);
						d[10] = d[11] = ((b & 0x04) >> 2) | ((r & 0x04) >> 1) | ((g & 0x04) >> 0);
						d[12] = d[13] = ((b & 0x02) >> 1) | ((r & 0x02) >> 0) | ((g & 0x02) << 1);
						d[14] = d[15] = ((b & 0x01) >> 0) | ((r & 0x01) << 1) | ((g & 0x01) << 2);
					}
					src = (src + 1) & 0x7ff;
					x++;
				}
				else {
					// normal
					for(int l = 0; l < 8; l++) {
						uint8 b = (col & 1) ? pcg_b[code][l] : 0;
						uint8 r = (col & 2) ? pcg_r[code][l] : 0;
						uint8 g = (col & 4) ? pcg_g[code][l] : 0;
						b = (attr & 8) ? ~b : b;
						r = (attr & 8) ? ~r : r;
						g = (attr & 8) ? ~g : g;
						int yy = y * ht + l;
						if(yy >= 200)
							break;
						uint8* d = &text[yy][x8];
						
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
			}
			else {
				// ank
				if(attr & 0x80) {
					// wide
					for(int l = 0; l < 8; l++) {
						uint8 pat = font[(code << 3) + l];
						pat = (attr & 8) ? ~pat : pat;
						int yy = y * ht + l;
						if(yy >= 200)
							break;
						uint8* d = &text[yy][x8];
						
						d[ 0] = d[ 1] = (pat & 0x80) ? col : 0;
						d[ 2] = d[ 3] = (pat & 0x40) ? col : 0;
						d[ 4] = d[ 5] = (pat & 0x20) ? col : 0;
						d[ 6] = d[ 7] = (pat & 0x10) ? col : 0;
						d[ 8] = d[ 9] = (pat & 0x08) ? col : 0;
						d[10] = d[11] = (pat & 0x04) ? col : 0;
						d[12] = d[13] = (pat & 0x02) ? col : 0;
						d[14] = d[15] = (pat & 0x01) ? col : 0;
					}
					src = (src + 1) & 0x7ff;
					x++;
				}
				else {
					// normal
					for(int l = 0; l < 8; l++) {
						uint8 pat = font[(code << 3) + l];
						pat = (attr & 8) ? ~pat : pat;
						int yy = y * ht + l;
						if(yy >= 200)
							break;
						uint8* d = &text[yy][x8];
						
						d[0] = (pat & 0x80) ? col : 0;
						d[1] = (pat & 0x40) ? col : 0;
						d[2] = (pat & 0x20) ? col : 0;
						d[3] = (pat & 0x10) ? col : 0;
						d[4] = (pat & 0x08) ? col : 0;
						d[5] = (pat & 0x04) ? col : 0;
						d[6] = (pat & 0x02) ? col : 0;
						d[7] = (pat & 0x01) ? col : 0;
					}
				}
			}
		}
	}
}

void DISPLAY::draw_cg(int width)
{
	uint16 src = ((regs[12] << 8) | regs[13]) & 0x7ff;
	
	for(int l = 0; l < 8; l++) {
		uint16 src = ((regs[12] << 8) | regs[13]) & 0x7ff;
		uint16 ofs = 0x800 * l;
		for(int y = 0; y < 200; y += 8) {
			for(int x = 0; x < width; x++) {
				uint8 b = vram_b[ofs | src];
				uint8 r = vram_r[ofs | src];
				uint8 g = vram_g[ofs | src];
				src = (src + 1) & 0x7ff;
				uint8* d = &cg[y | l][x << 3];
				
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
	}
}

