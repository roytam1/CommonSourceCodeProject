/*
	SHARP X1twin Emulator 'eX1twin'
	SHARP X1turbo Emulator 'eX1turbo'
	Skelton for retropc emulator

	Origin : X1EMU by KM (kanji rom)
	         X-millenium by Yui (ank16 patch)
	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ display ]
*/

#include "display.h"
#include "../i8255.h"
#include "../../fileio.h"
#include "../../config.h"

// from X-millenium

static const uint16 ANKFONT7f_af[0x21 * 8] = {
	0x0000, 0x3000, 0x247f, 0x6c24, 0x484c, 0xce4b, 0x0000, 0x0000,

	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff,
	0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff,
	0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0x0000, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	0x8080, 0x8080, 0x8080, 0x8080, 0x8080, 0x8080, 0x8080, 0x8080,
	0xc0c0, 0xc0c0, 0xc0c0, 0xc0c0, 0xc0c0, 0xc0c0, 0xc0c0, 0xc0c0,
	0xe0e0, 0xe0e0, 0xe0e0, 0xe0e0, 0xe0e0, 0xe0e0, 0xe0e0, 0xe0e0,
	0xf0f0, 0xf0f0, 0xf0f0, 0xf0f0, 0xf0f0, 0xf0f0, 0xf0f0, 0xf0f0,
	0xf8f8, 0xf8f8, 0xf8f8, 0xf8f8, 0xf8f8, 0xf8f8, 0xf8f8, 0xf8f8,
	0xfcfc, 0xfcfc, 0xfcfc, 0xfcfc, 0xfcfc, 0xfcfc, 0xfcfc, 0xfcfc,
	0xfefe, 0xfefe, 0xfefe, 0xfefe, 0xfefe, 0xfefe, 0xfefe, 0xfefe,
	0x0101, 0x0202, 0x0404, 0x0808, 0x1010, 0x2020, 0x4040, 0x8080,

	0x0000, 0x0000, 0x0000, 0x0000, 0x00ff, 0x0000, 0x0000, 0x0000,
	0x1010, 0x1010, 0x1010, 0x1010, 0x1010, 0x1010, 0x1010, 0x1010,
	0x1010, 0x1010, 0x1010, 0x1010, 0x00ff, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x10ff, 0x1010, 0x1010, 0x1010,
	0x1010, 0x1010, 0x1010, 0x1010, 0x10f0, 0x1010, 0x1010, 0x1010,
	0x1010, 0x1010, 0x1010, 0x1010, 0x101f, 0x1010, 0x1010, 0x1010,
	0x1010, 0x1010, 0x1010, 0x1010, 0x10ff, 0x1010, 0x1010, 0x1010,
	0x0000, 0x0000, 0x0000, 0x0000, 0x10f0, 0x1010, 0x1010, 0x1010,
	0x1010, 0x1010, 0x1010, 0x1010, 0x00f0, 0x0000, 0x0000, 0x0000,
	0x1010, 0x1010, 0x1010, 0x1010, 0x001f, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x101f, 0x1010, 0x1010, 0x1010,
	0x0000, 0x0000, 0x0000, 0x0000, 0x4080, 0x2020, 0x1010, 0x1010,
	0x1010, 0x1010, 0x0810, 0x0408, 0x0003, 0x0000, 0x0000, 0x0000,
	0x1010, 0x1010, 0x2010, 0x4020, 0x0080, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0403, 0x0808, 0x1010, 0x1010,
	0x8080, 0x4040, 0x2020, 0x1010, 0x0808, 0x0404, 0x0202, 0x0101
};

static const uint16 ANKFONTe0_ff[0x20 * 8] = {
	0x0000, 0x7e3c, 0xffff, 0xdbdb, 0xffff, 0xe7db, 0x7eff, 0x003c,
	0x0000, 0x423c, 0x8181, 0xa5a5, 0x8181, 0x99a5, 0x4281, 0x003c,
	0x0000, 0x3810, 0x7c7c, 0xfefe, 0xfefe, 0x106c, 0x7c38, 0x0000,
	0x0000, 0x6c00, 0xfefe, 0xfefe, 0xfefe, 0x7c7c, 0x1038, 0x0000,
	0x0000, 0x1010, 0x3838, 0x7c7c, 0x7cfe, 0x387c, 0x1038, 0x0010,
	0x0000, 0x3810, 0x7c7c, 0x5438, 0xfefe, 0x6cfe, 0x7c10, 0x0000,
	0x0101, 0x0303, 0x0707, 0x0f0f, 0x1f1f, 0x3f3f, 0x7f7f, 0xffff,
	0x8080, 0xc0c0, 0xe0e0, 0xf0f0, 0xf8f8, 0xfcfc, 0xfefe, 0xffff,
	0x8181, 0x4242, 0x2424, 0x1818, 0x1818, 0x2424, 0x4242, 0x8181,
	0xf0f0, 0xf0f0, 0xf0f0, 0xf0f0, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0xf0f0, 0xf0f0, 0xf0f0, 0xf0f0,
	0x0f0f, 0x0f0f, 0x0f0f, 0x0f0f, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0f0f, 0x0f0f, 0x0f0f, 0x0f0f,
	0x0f0f, 0x0f0f, 0x0f0f, 0x0f0f, 0xf0f0, 0xf0f0, 0xf0f0, 0xf0f0,
	0xf0f0, 0xf0f0, 0xf0f0, 0xf0f0, 0x0f0f, 0x0f0f, 0x0f0f, 0x0f0f,
	0x81ff, 0x8181, 0x8181, 0x8181, 0x8181, 0x8181, 0x8181, 0xff81,

	0x55aa, 0x55aa, 0x55aa, 0x55aa, 0x55aa, 0x55aa, 0x55aa, 0x55aa,
	0x1000, 0x1010, 0xf01e, 0x1010, 0x1010, 0x1010, 0x7e10, 0x00c0,
	0x1000, 0x2418, 0x7c42, 0x1090, 0x781c, 0x5410, 0xfe54, 0x0000,
	0x1000, 0x1010, 0xfe10, 0x1010, 0x3030, 0x565c, 0x9090, 0x0010,
	0x1000, 0x1210, 0xf412, 0x3034, 0x5030, 0x9654, 0x1090, 0x0000,
	0x0800, 0x8808, 0x5292, 0x1454, 0x2020, 0x5020, 0x465c, 0x0040,
	0x0000, 0xe23c, 0x8282, 0x82fa, 0x7a82, 0x4242, 0x0044, 0x0000,
	0x0000, 0x443c, 0x8242, 0xf282, 0x8282, 0x8282, 0x3844, 0x0000,
	0x0800, 0x5e18, 0xa468, 0xe4be, 0xbea4, 0xb2a2, 0x0a5a, 0x0002,
	0x0000, 0x2628, 0x4042, 0xe23c, 0x2222, 0x4222, 0x4442, 0x0004,
	0x0800, 0x0808, 0xda7a, 0x5454, 0x46e4, 0xe442, 0x08c4, 0x0000,
	0x0000, 0x7e40, 0x8848, 0x28be, 0x2828, 0x3e28, 0x08e8, 0x0008,
	0x0000, 0x723c, 0x9252, 0x9292, 0x8292, 0x84fc, 0x8484, 0x0000,
	0x0000, 0x1010, 0x2010, 0x2020, 0x6040, 0x8c50, 0x8286, 0x0000,
	0x0000, 0x4040, 0x784e, 0x88c0, 0x388e, 0x0848, 0x7e08, 0x0000,
	0x0000, 0x7c00, 0x0000, 0x0000, 0x10fe, 0x1010, 0x1010, 0x0010
};

void DISPLAY::initialize()
{
	scanline = config.scan_line;
	
	// load rom images
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	// ank8 (8x8)
	_stprintf(file_path, _T("%sANK8.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(font, sizeof(font), 1);
		fio->Fclose();
	}
	else {
		// xmillenium rom
		_stprintf(file_path, _T("%sFNT0808.X1"), app_path);
		if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
			fio->Fread(font, sizeof(font), 1);
			fio->Fclose();
		}
	}
	
	// ank16 (8x16)
	_stprintf(file_path, _T("%sANK16.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(kanji, 0x1000, 1);
		fio->Fclose();
	}
	else {
		// xmillenium rom
		_stprintf(file_path, _T("%sFNT0816.X1"), app_path);
		if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
			fio->Fread(kanji, 0x1000, 1);
			fio->Fclose();
		}
	}
	_memcpy(kanji + 0x7f * 16, ANKFONT7f_af, sizeof(ANKFONT7f_af));
	_memcpy(kanji + 0xe0 * 16, ANKFONTe0_ff, sizeof(ANKFONTe0_ff));
	
	// kanji (16x16)
	_stprintf(file_path, _T("%sKANJI.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(kanji + 0x1000, 0x4ac00, 1);
		fio->Fclose();
	}
	else {
		// xmillenium rom
		_stprintf(file_path, _T("%sFNT1616.X1"), app_path);
		if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
			fio->Fread(kanji + 0x1000, 0x4ac00, 1);
			fio->Fclose();
		}
	}
	for(int ofs = 0x1000; ofs < 0x4bc00; ofs += 32) {
		// LRLR.. -> LL..RR..
		uint8 buf[32];
		for(int i = 0; i < 16; i++) {
			buf[i     ] = kanji[ofs + i * 2    ];
			buf[i + 16] = kanji[ofs + i * 2 + 1];
		}
		_memcpy(kanji + ofs, buf, 32);
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
#ifdef _X1TURBO
	_memset(gaiji_b, 0, sizeof(gaiji_b));
	_memset(gaiji_r, 0, sizeof(gaiji_r));
	_memset(gaiji_g, 0, sizeof(gaiji_g));
#endif
	
	vblank = vsync = true;
	
	// regist event
	vm->register_frame_event(this);
	vm->register_vline_event(this);
}

void DISPLAY::reset()
{
#ifdef _X1TURBO
	mode1 = 3;
	mode2 = 0;
	hires = false;
#endif
	cur_line = cur_code = 0;
	
	kaddr = kofs = kflag = 0;
	kanji_ptr = &kanji[0];
}

void DISPLAY::update_config()
{
	scanline = config.scan_line;
}

void DISPLAY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff00) {
	case 0x0e00:
		write_kanji(addr, data);
		break;
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
		get_cur_pcg(addr);
		pcg_b[cur_code][cur_line] = data;
#ifdef _X1TURBO
		gaiji_b[cur_code >> 1][(cur_line << 1) | (cur_code & 1)] = data;
#endif
		break;
	case 0x1600:
		get_cur_pcg(addr);
		pcg_r[cur_code][cur_line] = data;
#ifdef _X1TURBO
		gaiji_r[cur_code >> 1][(cur_line << 1) | (cur_code & 1)] = data;
#endif
		break;
	case 0x1700:
		get_cur_pcg(addr);
		pcg_g[cur_code][cur_line] = data;
#ifdef _X1TURBO
		gaiji_g[cur_code >> 1][(cur_line << 1) | (cur_code & 1)] = data;
#endif
		break;
#ifdef _X1TURBO
	case 0x1f00:
		switch(addr) {
		case 0x1fd0:
			mode1 = data;
			break;
		case 0x1fe0:
			mode2 = data;
			update_pal();
			break;
		}
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
		vram_a[addr & 0x7ff] = data;
		break;
	case 0x2800:
	case 0x2900:
	case 0x2a00:
	case 0x2b00:
	case 0x2c00:
	case 0x2d00:
	case 0x2e00:
	case 0x2f00:
		vram_a[addr & 0x7ff] = data; // mirror
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
		vram_t[addr & 0x7ff] = data; // mirror
#endif
		break;
	}
}

uint32 DISPLAY::read_io8(uint32 addr)
{
	switch(addr & 0xff00) {
	case 0x0e00:
		return read_kanji(addr);
//	case 0x1000:
//		return pal[0];
//	case 0x1100:
//		return pal[1];
//	case 0x1200:
//		return pal[2];
//	case 0x1300:
//		return priority;
	case 0x1400:
		return get_cur_font(addr);
	case 0x1500:
		get_cur_pcg(addr);
		return pcg_b[cur_code][cur_line];
	case 0x1600:
		get_cur_pcg(addr);
		return pcg_r[cur_code][cur_line];
	case 0x1700:
		get_cur_pcg(addr);
		return pcg_g[cur_code][cur_line];
#ifdef _X1TURBOZ
	case 0x1f00:
		switch(addr) {
		case 0x1fd0:
			return mode1;
		case 0x1fe0:
			return mode2;
		}
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
		return vram_a[addr & 0x7ff];
	case 0x2800:
	case 0x2900:
	case 0x2a00:
	case 0x2b00:
	case 0x2c00:
	case 0x2d00:
	case 0x2e00:
	case 0x2f00:
		return vram_a[addr & 0x7ff]; // mirror
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
		return vram_k[addr & 0x7ff];
#else
		return vram_t[addr & 0x7ff]; // mirror
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
	cblink = (cblink + 1) & 0x3f;
#ifdef _X1TURBO
	hires = ((regs[4] + 1) * ((regs[9] & 0x1f) + 1) + regs[5] > 400);
#endif
}

void DISPLAY::event_vline(int v, int clock)
{
	vline = v;
	vclock = vm->current_clock();
	prev_clock = 0;
	
	int wd = (regs[3] & 0xf0) >> 4;
	int vs = regs[7] * (regs[9] + 1);
	int ve = vs + (wd ? wd : 8);
	int dve = regs[6] * (regs[9] + 1);
	
#ifdef _X1TURBO
	int vv = hires ? (v * 2) : v;
	set_vblank(vv < dve);
	set_vsync(vs <= vv && vv <= ve);
	
	if(v < 200) {
		if(hires) {
			draw_line(vv);	// ugly patch !!!
			draw_line(vv + 1);
		}
		else {
			draw_line(v);
		}
	}
#else
	set_vblank(v < dve);
	set_vsync(vs <= v && v <= ve);
	
	if(v < 200) {
		draw_line(v);
	}
#endif
}

void DISPLAY::set_vblank(bool val)
{
	if(vblank != val) {
		d_pio->write_signal(SIG_I8255_PORT_B, val ? 0x80 : 0, 0x80);
		vblank = val;
	}
}

void DISPLAY::set_vsync(bool val)
{
	if(vsync != val) {
		d_pio->write_signal(SIG_I8255_PORT_B, val ? 0x04 : 0, 0x04);
		vsync = val;
	}
}

void DISPLAY::update_pal()
{
	uint8 pal2[8];
	for(int i = 0; i < 8; i++) {
		uint8 bit = 1 << i;
		pal2[i] = ((pal[0] & bit) ? 1 : 0) | ((pal[1] & bit) ? 2 : 0) | ((pal[2] & bit) ? 4 : 0);
#ifdef _X1TURBO
		if(((mode2 & 0x10) && i == 0) || ((mode2 & 0x20) && i == 1)) {
			pal2[i] = 0;
		}
#endif
	}
	for(int c = 0; c < 8; c++) {
		for(int t = 0; t < 8; t++) {
			if(priority & (1 << c)) {
				pri[c][t] = pal2[c];
			}
			else if(t) {
#ifdef _X1TURBO
				pri[c][t] = ((mode2 & 8) && (mode2 & 7) == t) ? 0 : t;
#else
				pri[c][t] = t;
#endif
			}
			else {
				pri[c][t] = pal2[c];
			}
		}
	}
}

uint8 DISPLAY::get_cur_font(uint32 addr)
{
#ifdef _X1TURBO
	if(mode1 & 0x20) {
		// from X1EMU
		uint16 vaddr;
		if(!(vram_a[0x7ff] & 0x20)) {
			vaddr = 0x7ff;
		}
		else if(!(vram_a[0x3ff] & 0x20)) {
			vaddr = 0x3ff;
		}
		else if(!(vram_a[0x5ff] & 0x20)) {
			vaddr = 0x5ff;
		}
		else if(!(vram_a[0x1ff] & 0x20)) {
			vaddr = 0x1ff;
		}
		else {
			vaddr = 0x3ff;
		}
		uint16 ank = vram_t[vaddr];
		uint16 knj = vram_k[vaddr];
		
		if(knj & 0x80) {
			uint32 ofs = adr2knj_x1t((knj << 8) | ank);
			if(knj & 0x40) {
				ofs += 16; // right
			}
			return kanji[ofs | (addr & 15)];
		}
		else if(mode1 & 0x40) {
			return kanji[(ank << 4) | (addr & 15)];
		}
		else {
			return font[(ank << 3) | ((addr >> 1) & 7)];
		}
	}
#endif
	get_cur_code_line();
	return font[(cur_code << 3) | (cur_line & 7)];
}

void DISPLAY::get_cur_pcg(uint32 addr)
{
#ifdef _X1TURBO
	if(mode1 & 0x20) {
		// from X1EMU
		uint16 vaddr;
		if(vram_a[0x7ff] & 0x20) {
			vaddr = 0x7ff;
		}
		else if(vram_a[0x3ff] & 0x20) {
			vaddr = 0x3ff;
		}
		else if(vram_a[0x5ff] & 0x20) {
			vaddr = 0x5ff;
		}
		else if(vram_a[0x1ff] & 0x20) {
			vaddr = 0x1ff;
		}
		else {
			vaddr = 0x3ff;
		}
		cur_code = vram_t[vaddr];
		cur_line = (addr >> 1) & 7;
		
		if(vram_k[vaddr] & 0x90) {
			cur_code &= 0xfe;
			cur_code += addr & 1;
		}
		// force update in next get_cur_code_line()
		prev_clock = 0;
	}
	else
#endif
	get_cur_code_line();
}

void DISPLAY::get_cur_code_line()
{
	/*
		NOTE: don't update pcg addr frequently to write r/g/b patterns to one pcg (wibarm)
	
		CLOCKS_PER_LINE = 250
		CLOCKS_PER_LINE / CHARS_PER_LINE * 40 = 87.8
		
		XXX: Need to run X1turbo virtual machine with 15KHz mode :-(
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
		cur_line = (vline % ht) & 7;
		prev_clock = vm->current_clock();
	}
}

void DISPLAY::draw_line(int v)
{
	if(v == 0) {
		_memset(text, 0, sizeof(text));
		_memset(cg, 0, sizeof(cg));
		_memset(prev_top, 0, sizeof(prev_top));
	}
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

void DISPLAY::draw_screen()
{
	// copy to real screen
#ifdef _X1TURBO
	if(hires) {
		// 400 lines
		if(column & 0x40) {
			// 40 columns
			for(int y = 0; y < 400; y++) {
				scrntype* dest = emu->screen_buffer(y);
				uint8* src_text = text[y];
				uint8* src_cg = cg[y];
				
				for(int x = 0, x2 = 0; x < 320; x++, x2 += 2) {
					dest[x2] = dest[x2 + 1] = palette_pc[pri_line[y][src_cg[x]][src_text[x]]];
				}
			}
		}
		else {
			// 80 columns
			for(int y = 0; y < 400; y++) {
				scrntype* dest = emu->screen_buffer(y);
				uint8* src_text = text[y];
				uint8* src_cg = cg[y];
				
				for(int x = 0; x < 640; x++) {
					dest[x] = palette_pc[pri_line[y][src_cg[x]][src_text[x]]];
				}
			}
		}
	}
	else {
#endif
		// 200 lines
		if(column & 0x40) {
			// 40 columns
			for(int y = 0; y < 200; y++) {
				scrntype* dest0 = emu->screen_buffer(y * 2 + 0);
				scrntype* dest1 = emu->screen_buffer(y * 2 + 1);
				uint8* src_text = text[y];
				uint8* src_cg = cg[y];
				
				for(int x = 0, x2 = 0; x < 320; x++, x2 += 2) {
					dest0[x2] = dest0[x2 + 1] = palette_pc[pri_line[y][src_cg[x]][src_text[x]]];
				}
				if(!scanline) {
					_memcpy(dest1, dest0, 640 * sizeof(scrntype));
				}
				else {
					_memset(dest1, 0, 640 * sizeof(scrntype));
				}
			}
		}
		else {
			// 80 columns
			for(int y = 0; y < 200; y++) {
				scrntype* dest0 = emu->screen_buffer(y * 2 + 0);
				scrntype* dest1 = emu->screen_buffer(y * 2 + 1);
				uint8* src_text = text[y];
				uint8* src_cg = cg[y];
				
				for(int x = 0; x < 640; x++) {
					dest0[x] = palette_pc[pri_line[y][src_cg[x]][src_text[x]]];
				}
				if(!scanline) {
					_memcpy(dest1, dest0, 640 * sizeof(scrntype));
				}
				else {
					_memset(dest1, 0, 640 * sizeof(scrntype));
				}
			}
		}
#ifdef _X1TURBO
	}
#endif
	
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
#ifdef _X1TURBO
		uint8 knj = vram_k[src];
#endif
		uint8 attr = vram_a[src];
		uint8 col = attr & 7;
		bool reverse = ((attr & 8) != 0);
		bool blink = ((attr & 0x10) && (cblink & 0x20));
		reverse = (reverse != blink);
#ifdef _X1TURBO
		int shift = 0;
#endif
		
		// select pcg or ank
		const uint8 *pattern_b, *pattern_r, *pattern_g;
		if(attr & 0x20) {
			// pcg
#ifdef _X1TURBO
			if(knj & 0x90) {
				pattern_b = gaiji_b[code >> 1];
				pattern_r = gaiji_r[code >> 1];
				pattern_g = gaiji_g[code >> 1];
			}
			else {
#endif
				pattern_b = pcg_b[code];
				pattern_r = pcg_r[code];
				pattern_g = pcg_g[code];
#ifdef _X1TURBO
				shift = hires ? 1 : 0;
			}
#endif
		}
#ifdef _X1TURBO
		else if(hires) {
			// ank 8x16 or kanji
			uint32 ofs = code << 4;
			if(knj & 0x80) {
				ofs = adr2knj_x1t((knj << 8) | code);
				if(knj & 0x40) {
					ofs += 16; // right
				}
			}
			pattern_b = pattern_r = pattern_g = &kanji[ofs];
		}
#endif
		else {
			// ank 8x8
			pattern_b = pattern_r = pattern_g = &font[code << 3];
		}
		
		// check vertical doubled char
		uint8 is_top = 0, is_bottom = prev_top[x];
		if(attr & 0x40) {
			if(is_bottom) {
				// bottom 4 rasters of vertical doubled char
#ifdef _X1TURBO
				int half = (hires && !shift) ? 8 : 4;
#else
				#define half 4
#endif
				pattern_b += half;
				pattern_r += half;
				pattern_g += half;
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
		for(int l = 0; l < ht; l++) {
			int line = (attr & 0x40) ? (l >> 1) : l;
#ifdef _X1TURBO
			line >>= shift;
#endif
			uint8 b = (!(col & 1)) ? 0 : reverse ? ~pattern_b[line] : pattern_b[line];
			uint8 r = (!(col & 2)) ? 0 : reverse ? ~pattern_r[line] : pattern_r[line];
			uint8 g = (!(col & 4)) ? 0 : reverse ? ~pattern_g[line] : pattern_g[line];
			int yy = y * ht + l;
#ifdef _X1TURBO
			if(yy >= 400) {
#else
			if(yy >= 200) {
#endif
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
	int ofs, src = ((regs[12] << 8) | regs[13]) + hz * y;
#ifdef _X1TURBO
	int page = (hires && !(mode1 & 2)) ? (l & 1) : (mode1 & 8);
	int ll = hires ? (l >> 1) : l;
	
	if(mode1 & 4) {
		ofs = (0x400 * (ll & 15)) + (page ? 0xc000 : 0);
	}
	else {
		ofs = (0x800 * (ll & 7)) + (page ? 0xc000 : 0);
	}
#else
	ofs = 0x800 * (l & 7);
#endif
	int ofs_b = ofs + 0x0000;
	int ofs_r = ofs + 0x4000;
	int ofs_g = ofs + 0x8000;
	
	for(int x = 0; x < hz && x < width; x++) {
		src &= 0x7ff;
		uint8 b = vram_ptr[ofs_b | src];
		uint8 r = vram_ptr[ofs_r | src];
		uint8 g = vram_ptr[ofs_g | src++];
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

// kanji rom (from X1EMU by KM)

void DISPLAY::write_kanji(uint32 addr, uint32 data)
{
	switch(addr) {
	case 0xe80:
		kaddr = (kaddr & 0xff00) | data;
		break;
	case 0xe81:
		kaddr = (kaddr & 0xff) | (data << 8);
		break;
	case 0xe82:
		// TODO: bit0 L->H: Latch
		kanji_ptr = &kanji[adr2knj_x1(kaddr & 0xfff0)];
		break;
	}
}

uint32 DISPLAY::read_kanji(uint32 addr)
{
	switch(addr) {
	case 0xe80:
		if(kaddr & 0xff00) {
			uint32 val = kanji_ptr[kofs];
			kflag |= 1;
			if(kflag == 3) {
				kofs = (kofs + 1) & 15;
				kflag = 0;
			}
			return val;
		}
		return jis2adr_x1(kaddr << 8) >> 8;
	case 0xe81:
		if(kaddr & 0xff00) {
			uint32 val = kanji_ptr[kofs + 16];
			kflag |= 2;
			if(kflag == 3) {
				kofs = (kofs + 1) & 15;
				kflag = 0;
			}
			return val;
		}
		return 0;
	}
	return 0xff;
}

uint16 DISPLAY::jis2adr_x1(uint16 jis)
{
	uint16 jh, jl, adr;
	
	jh = jis >> 8;
	jl = jis & 0xff;
	if(jh > 0x28) {
		adr = 0x4000 + (jh - 0x30) * 0x600;
	}
	else {
		adr = 0x0100 + (jh - 0x21) * 0x600;
	}
	if(jl >= 0x20) {
		adr += (jl - 0x20) * 0x10;
	}
	return adr;
}

uint32 DISPLAY::adr2knj_x1(uint16 adr)
{
	uint16 jh, jl, jis;
	
	if(adr < 0x4000) {
		jh = adr - 0x0100;
		jh = 0x21 + jh / 0x600;
	}
	else {
		jh = adr - 0x4000;
		jh = 0x30 + jh / 0x600;
	}
	if(jh > 0x28) {
		adr -= 0x4000 + (jh - 0x30) * 0x600;
	}
	else {
		adr -= 0x0100 + (jh - 0x21) * 0x600;
	}
	jl = 0x20;
	if(adr) {
		jl += adr / 0x10;
	}
	
	jis = (jh << 8) | jl;
	return jis2knj(jis);
}

#ifdef _X1TURBO
uint32 DISPLAY::adr2knj_x1t(uint16 adr)
{
	uint16 j1, j2;
	uint16 rl, rh;
	uint16 jis;
	
	rh = adr >> 8;
	rl = adr & 0xff;
	
	rh &= 0x1f;
	if(!rl && !rh) {
		return jis2knj(0);
	}
	j2 = rl & 0x1f;		// rl4,3,2,1,0
	j1 = (rl / 0x20) & 7;	// rl7,6,5
	
	if(rh < 0x04) {
		// 2121-277e
		j1 |= 0x20;
		switch(rh & 3){
		case 0: j2 |= 0x20; break;
		case 1: j2 |= 0x60; break;
		case 2: j2 |= 0x40; break;
		default: j1 = j2 = 0; break;
		}
	}
	else if(rh > 0x1c) {
		// 7021-777e
		j1 |= 0x70;
		switch(rh & 3) {
		case 0: j2 |= 0x20; break;
		case 1: j2 |= 0x60; break;
		case 2: j2 |= 0x40; break;
		default: j1 = j2 = 0; break;
		}
	}
	else {
		j1 |= (((rh >> 1) + 7) / 3) * 0x10;
		j1 |= (rh & 1) * 8;
		j2 |= ((((rh >> 1) + 1) % 3) + 1) * 0x20;
	}
	
	jis = (j1 << 8) | j2;
	return jis2knj(jis);
}
#endif

uint32 DISPLAY::jis2knj(uint16 jis)
{
	uint32 sjis = jis2sjis(jis);
	
	if(sjis < 0x100){
		return sjis * 16;
	}
	else if(sjis >= 0x8140 && sjis < 0x84c0) {
		return 0x01000 + (sjis - 0x8140) * 32;
	}
	else if(sjis >= 0x8890 && sjis < 0xa000) {
		return 0x08000 + (sjis - 0x8890) * 32;
	}
	else if(sjis >= 0xe040 && sjis < 0xeab0) {
		return 0x36e00 + (sjis - 0xe040) * 32;
	}
	else {
		return 0;
	}
}

uint16 DISPLAY::jis2sjis(uint16 jis)
{
	uint16 c1, c2;
	
	if(!jis) {
		return 0;
	}
	c1 = jis >> 8;
	c2 = jis & 0xff;
	
	if(c1 & 1){
		c2 += 0x1f;
		if(c2 >= 0x7f) {
			c2++;
		}
	}
	else {
		c2 += 0x7e;
	}
	c1 = (c1 - 0x20 - 1) / 2 + 0x81;
	if(c1 >= 0xa0) {
		c1 += 0x40;
	}
	return (c1 << 8) | c2;
}

