/*
	Skelton for retropc emulator

	Origin : Ootake (joypad, timer)
	       : xpce (psg, vdc)
	Author : Takeda.Toshiya
	Date   : 2009.03.11-

	[ PC-Eninge ]
*/

#ifndef _PCE_H_
#define _PCE_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define	PCE_WIDTH	(360+64)
#define	PCE_HEIGHT	256

class PCE : public DEVICE
{
private:
	DEVICE* d_cpu;
	int did_irq2, did_irq1, did_tirq, did_intmask, did_intstat;
	
	// memory
	uint8 ram[0x2000];	// ram 8kb
	uint8 cart[0x400000];	// max 4mb
	uint32 bank;
	uint8 buffer;
	
	// vdc
	typedef struct {
		int16 y, x, no, atr;
	} sprtype;
	pair vdc[32];
	pair vce[0x200];
	pair vce_reg;
	uint16 vdc_inc, vdc_raster_count;
	uint8 vdc_ch, vdc_status, vdc_ratch, vce_ratch;
	bool vdc_satb;
	bool vdc_pendvsync;
	int vdc_bg_h, vdc_bg_w;
	int vdc_screen_w, vdc_screen_h;
	int vdc_minline, vdc_maxline;
	int vdc_dispwidth, vdc_dispmin, vdc_dispmax;
	int vdc_scanline;
	int prv_scanline;
	bool vdc_scroll;
	int vdc_scroll_ydiff;
	int prv_scroll_x, prv_scroll_y, prv_scroll_ydiff;
	uint8 mask_spr[PCE_HEIGHT][PCE_WIDTH];
	uint32 pixel_bg[0x20000], pixel_spr[0x20000];
	uint8 vchange[0x1000], vchanges[0x400];
	bool vdc_spbg;
	scrntype screen[PCE_HEIGHT][PCE_WIDTH + 8];	// 8 for xofs
	scrntype palette_bg[256], palette_spr[256];
	uint8 vram[0x20000];
	uint16 spram[256];
	void vdc_reset();
	void vdc_write(uint16 addr, uint8 data);
	uint8 vdc_read(uint16 addr);
	void vce_write(uint16 addr, uint8 data);
	uint8 vce_read(uint16 addr);
	void vce_update_pal(int num);
	void vdc_refresh_line(int sy, int ey);
	void vdc_refresh_sprite(int sy, int ey, int bg);
	void vdc_put_sprite(scrntype *dst, uint8 *src, uint32 *pixel, scrntype *pal, int h, int inc);
	void vdc_put_sprite_hflip(scrntype *dst, uint8 *src, uint32 *pixel, scrntype *pal, int h, int inc);
	void vdc_put_sprite_mask(scrntype *dst, uint8 *src, uint32 *pixel, scrntype *pal, int h, int inc, uint8 *mask, uint8 pr);
	void vdc_put_sprite_hflip_mask(scrntype *dst, uint8 *src, uint32 *pixel, scrntype *pal, int h, int inc, uint8 *mask, uint8 pr);
	void vdc_put_sprite_makemask(scrntype *dst, uint8 *src, uint32 *pixel, scrntype *pal, int h, int inc, uint8 *mask, uint8 pr);
	void vdc_put_sprite_hflip_makemask(scrntype *dst,uint8 *src,uint32 *pixel,scrntype *pal,int h,int inc,uint8 *mask,uint8 pr);
	void vdc_plane2pixel(int no);
	void vdc_sp2pixel(int no);
	
	// psg
	typedef struct {
		// registers
		uint8 regs[8];
		uint8 wav[32];
		uint8 wavptr;
		// sound gen
		uint32 genptr;
		uint32 remain;
		bool noise;
		uint32 randval;
	} psg_t;
	psg_t psg[8];
	uint8 psg_ch, psg_vol, psg_lfo_freq, psg_lfo_ctrl;
	int sample_rate;
	void psg_reset();
	void psg_write(uint16 addr, uint8 data);
	uint8 psg_read(uint16 addr);
	
	// timer
	int timer_const, timer_count, timer_run;
	void timer_reset();
	void timer_write(uint16 addr, uint8 data);
	uint8 timer_read(uint16 addr);
	
	// joypad
	uint8 *joy_stat, *key_stat;
	uint8 joy_count, joy_nibble, joy_second;
	void joy_reset();
	void joy_write(uint16 addr, uint8 data);
	uint8 joy_read(uint16 addr);
	
	// interrupt control
	void int_request(uint8 val);
	void int_cancel(uint8 val);
	void int_write(uint16 addr, uint8 data);
	uint8 int_read(uint16 addr);
	
public:
	PCE(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~PCE() {}
	
	// common functions
	void initialize();
	void reset();
	void event_callback(int event_id, int err);
	void event_vline(int v, int clock);
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void mix(int32* buffer, int cnt);
	
	// unique functions
	void set_context_cpu(DEVICE* device, int id_irq2, int id_irq1, int id_tirq, int id_intmask, int id_intstat) {
		d_cpu = device;
		did_irq2 = id_irq2; did_irq1 = id_irq1; did_tirq = id_tirq;
		did_intmask = id_intmask; did_intstat = id_intstat;
	}
	void initialize_sound(int rate) {
		sample_rate = rate;
	}
	void open_cart(_TCHAR* filename);
	void close_cart();
	void draw_screen();
};

#endif
