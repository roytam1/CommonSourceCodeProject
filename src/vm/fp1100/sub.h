/*
	CASIO FP-1100 Emulator 'eFP-1100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.06.09-

	[ sub pcb ]
*/

#ifndef _SUB_H_
#define _SUB_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_SUB_INT2	0
#define SIG_SUB_COMM	1
#define SIG_SUB_HSYNC	2

class SUB : public DEVICE
{
private:
	// to sub cpu
	DEVICE *d_cpu;
	int did_int0, did_int2, did_wait;
	// to main pcb
	DEVICE *d_main;
	int did_ints, did_comm;
	// to beep
	DEVICE *d_beep;
	int did_beep;
	// from/to crtc
	DEVICE *d_crtc;
	uint8 *regs;
	
	uint8 *wbank[0x200];
	uint8 *rbank[0x200];
	uint8 wdmy[0x80];
	uint8 rdmy[0x80];
	
	uint8 ram[0x80];
	uint8 vram_b[0x4000];
	uint8 vram_r[0x4000];
	uint8 vram_g[0x4000];
	uint8 sub1[0x1000];
	uint8 sub2[0x1000];
	uint8 sub3[0x1000];
	
	uint8 pa, pb, pc;
	uint8 comm_data;
	uint8 *key_stat;
	uint8 key_sel, key_data;
	uint8 color;
	bool hsync, wait;
	uint8 cblink;
	uint8 screen[400][640];
	scrntype palette_pc[8];
	
	void key_update();
	
public:
	SUB(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~SUB() {}
	
	// common functions
	void initialize();
void release();//patch
	void reset();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_data16(uint32 addr, uint32 data) {
		write_data8(addr, data & 0xff); write_data8(addr + 1, data >> 8);
	}
	uint32 read_data16(uint32 addr) {
		return read_data8(addr) | (read_data8(addr + 1) << 8);
	}
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_frame();
	
	// unique functions
	void set_context_cpu(DEVICE *device, int id_int0, int id_int2, int id_wait) {
		d_cpu = device;
		did_int0 = id_int0; did_int2 = id_int2; did_wait = id_wait;
	}
	void set_context_main(DEVICE *device, int id_ints, int id_comm) {
		d_main = device;
		did_ints = id_ints; did_comm = id_comm;
	}
	void set_context_beep(DEVICE *device, int id_beep) {
		d_beep = device;
		did_beep = id_beep;
	}
	void set_context_crtc(DEVICE *device, uint8* ptr) {
		d_crtc = device;
		regs = ptr;
	}
	void key_down(int code);
	void key_up(int code);
	void draw_screen();
	void play_datarec(_TCHAR* filename);
	void rec_datarec(_TCHAR* filename);
	void close_datarec();
};

#endif
