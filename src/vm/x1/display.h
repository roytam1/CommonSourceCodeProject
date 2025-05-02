/*
	SHARP X1twin Emulator 'eX1twin'
	SHARP X1turbo Emulator 'eX1turbo'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ display ]
*/

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_DISPLAY_COLUMN	0

class DISPLAY : public DEVICE
{
private:
	DEVICE *d_fdc, *d_pio;
	
	uint8* regs;
	uint8 vram_t[0x800];
	uint8 vram_a[0x800];
#ifdef _X1TURBO
	uint8 vram_k[0x800];
#endif
	uint8* vram_ptr;
	uint8 pcg_b[256][8];
	uint8 pcg_r[256][8];
	uint8 pcg_g[256][8];
#ifdef _X1TURBO
	uint8 gaiji_b[128][16];
	uint8 gaiji_r[128][16];
	uint8 gaiji_g[128][16];
#endif
	uint8 font[0x800];
	uint8 kanji[0x4bc00];
	
	int vline, vclock, prev_clock;
	uint8 cur_code, cur_line;
	
	int kaddr, kofs, kflag;
	uint8* kanji_ptr;
	
	uint8 pal[3];
	uint8 priority, pri[8][8];	// pri[cg][txt]
	
	uint8 column;
#ifdef _X1TURBO
	uint8 mode1, mode2;
	bool hires;
#endif
	
#ifdef _X1TURBO
	uint8 text[400][640+8];	// +8 for wide char
	uint8 cg[400][640];
	uint8 pri_line[400][8][8];
#else
	uint8 text[200][640+8];	// +8 for wide char
	uint8 cg[200][640];
	uint8 pri_line[200][8][8];
#endif
	scrntype palette_pc[8];
	uint8 prev_top[80];
	int cblink;
	bool vblank, vsync;
	bool scanline;
	
	void set_vblank(bool val);
	void set_vsync(bool val);
	void update_pal();
	
	uint8 get_cur_font(uint32 addr);
	void get_cur_pcg(uint32 addr);
	void get_cur_code_line();
	
	void draw_line(int v);
	void draw_text(int y);
	void draw_cg(int line);
	
	// kanji rom (from X1EMU by KM)
	void write_kanji(uint32 addr, uint32 data);
	uint32 read_kanji(uint32 addr);
	
	uint16 jis2adr_x1(uint16 jis);
	uint32 adr2knj_x1(uint16 adr);
#ifdef _X1TURBO
	uint32 adr2knj_x1t(uint16 adr);
#endif
	uint32 jis2knj(uint16 jis);
	uint16 jis2sjis(uint16 jis);
	
public:
	DISPLAY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~DISPLAY() {}
	
	// common functions
	void initialize();
	void reset();
	void update_config();
	
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_frame();
	void event_vline(int v, int clock);
	
	// unique function
	void set_context_fdc(DEVICE* device) {
		d_fdc = device;
	}
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
	void set_vram_ptr(uint8* ptr) {
		vram_ptr = ptr;
	}
	void set_regs_ptr(uint8* ptr) {
		regs = ptr;
	}
	void draw_screen();
};

#endif

