/*
	NEC PC-98DO Emulator 'ePC-98DO'
	NEC PC-8801MA Emulator 'ePC-8801MA'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2011.12.29-

	[ PC-8801 ]
*/

#ifndef _PC88_H_
#define _PC88_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_PC88_USART_IRQ	0
#define SIG_PC88_SOUND_IRQ	1
#define SIG_PC88_USART_OUT	2

#define DATAREC_BUFFER_SIZE	0x40000

class PC88 : public DEVICE
{
private:
	DEVICE *d_cpu, *d_beep, *d_opn, *d_pcm, *d_pio, *d_rtc, *d_sio;
	
	uint8* rbank[16];
	uint8* wbank[16];
	uint8 wdmy[0x1000];
	uint8 rdmy[0x1000];
	
	uint8 ram[0x10000];
#ifdef PC88_EXRAM_BANKS
	uint8 exram[0x8000 * PC88_EXRAM_BANKS];
#endif
	uint8 gvram[0xc000];
	uint8 tvram[0x1000];
	uint8 n88rom[0x8000];
	uint8 n88exrom[0x8000];
	uint8 n80rom[0x8000];
	uint8 kanji1[0x20000];
	uint8 kanji2[0x20000];
#ifdef SUPPORT_PC88_DICTIONARY
	uint8 dicrom[0x80000];
#endif
	
	// memory mapper
	uint8 rm_mode;
	uint8 exrom_sel, exrom_bank;
	uint8 exram_sel, exram_bank;
	uint8 text_window;
	uint8 gvram_plane, gvram_sel;
	uint8 tvram_sel;
#ifdef SUPPORT_PC88_DICTIONARY
	uint8 dicrom_sel, dicrom_bank;
#endif
	
	void update_gvram_wait();
	void update_gvram_sel();
	void update_low_memmap();
	void update_tvram_memmap();
	
	// misc
	uint8 port32, portE2;
	uint8 alu_ctrl1, alu_ctrl2, alu_reg[3];
	uint8 ghs_mode;
	
	bool cpu_clock_low;
	int mem_wait_clocks;
	int gvram_wait_clocks;
	int busreq_clocks;
	
	bool usart_dcd;
	bool opn_busy;
	
	// crtc
	uint8 crtc_cmd, crtc_ptr;
	uint8 crtc_status;
	
	uint8 text_attrib;
	int attrib_num;
	
	int text_width, text_height, char_height;
	bool skip_line;
	
	bool cursor_on, cursor_blink;
	int cursor_x, cursor_y, cursor_line;
	
	bool blink_on;
	int blink_rate, blink_counter;
	
	uint8 crtc_buffer[120 * 200];
	int crtc_buffer_ptr;
	
	uint8 disp_ctrl, text_mode, graph_mode;
	bool line200, vblank;
	
	typedef struct {
		uint8 b, r, g;
	} palette_t;
	palette_t palette[9];
	bool update_palette;
	
	uint8 sg_pattern[0x800];
	uint8 text[200][640];
	uint8 attribs[200][80];
	uint8 graph[400][640];
	scrntype palette_text_pc[9];	// 8 = non transparent black
	scrntype palette_graph_pc[9];	// 8 = non dot in hireso screen
	
	uint8 get_crtc_buffer(int ofs);
	void expand_attribs();
	void draw_text();
	void draw_color_graph();
	void draw_color_hires_graph();
	void draw_mono_graph();
	
	// dma (temporary)
	typedef struct {
		pair start, length;
	} dma_t;
	dma_t dma_reg[4];
	uint8 dma_mode, dma_status;
	bool dma_hl;
	
	// keyboard
	uint8 key_status[256];
	uint8 key_caps, key_kana;
	
	// joystick & mouse
#ifdef SUPPORT_PC88_JOYSTICK
	uint8 *joystick_status;
	int* mouse_status;
	uint8 mouse_strobe;
	uint32 mouse_strobe_clock;
	uint32 mouse_strobe_clock_lim;
	int mouse_phase;
	int mouse_dx, mouse_dy;
	int mouse_lx, mouse_ly;
#endif
	uint8 opn_ch;
	
	// kanji rom
	pair kanji1_addr, kanji2_addr;
	
	// intterrupt
	uint8 intr_req;
	bool intr_req_sound;
	uint8 intr_mask1, intr_mask2, intr_mask_sound;
	void request_intr(int level, bool status);
	void update_intr();
	
	// data recorder
	FILEIO *cmt_fio;
	int cmt_bufptr, cmt_bufcnt;
	uint8 cmt_buffer[DATAREC_BUFFER_SIZE];
	bool cmt_play, cmt_rec;
	int cmt_register_id;
	
	void release_datarec();
	bool check_data_carrier(uint8* p);
	
public:
	PC88(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~PC88() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	
#ifdef Z80_MEMORY_WAIT
	void write_data8w(uint32 addr, uint32 data, int* wait);
	uint32 read_data8w(uint32 addr, int* wait);
#else
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
#endif
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	
	void write_dma_data8(uint32 addr, uint32 data);
	uint32 read_dma_data8(uint32 addr);
	void write_dma_io8(uint32 addr, uint32 data);
	
	void write_signal(int id, uint32 data, uint32 mask);
	void event_callback(int event_id, int err);
	void event_frame();
	void event_vline(int v, int clock);
	uint32 intr_ack();
	void intr_ei();
	
	// unique functions
	void set_context_beep(DEVICE* device) {
		d_beep = device;
	}
	void set_context_cpu(DEVICE* device) {
		d_cpu = device;
	}
	void set_context_opn(DEVICE* device) {
		d_opn = device;
	}
	void set_context_pcm(DEVICE* device) {
		d_pcm = device;
	}
	void set_context_pio(DEVICE* device) {
		d_pio = device;
	}
	void set_context_rtc(DEVICE* device) {
		d_rtc = device;
	}
	void set_context_sio(DEVICE* device) {
		d_sio = device;
	}
	void key_down(int code, bool repeat);
	
	void play_datarec(_TCHAR* file_path);
	void rec_datarec(_TCHAR* file_path);
	void close_datarec();
	bool now_skip();
	
	void draw_screen();
};

#endif

