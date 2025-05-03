/*
	NEC PC-100 Emulator 'ePC-100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.07.14 -

	[ i/o controller ]
*/

#ifndef _IOCTRL_H_
#define _IOCTRL_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_IOCTRL_RESET	0

static int key_table[256] = {
	  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,0x18,0x12,  -1,  -1,  -1,0x38,  -1,  -1,
	  -1,0x04,0x05,0x09,  -1,  -1,  -1,  -1,  -1,  -1,  -1,0x11,  -1,  -1,  -1,  -1,
	0x4A,  -1,  -1,0x5B,0x5A,0x15,0x13,0x16,0x14,  -1,  -1,  -1,  -1,0x17,  -1,  -1,
	0x27,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x26,  -1,  -1,  -1,  -1,  -1,  -1,
	  -1,0x31,0x45,0x43,0x33,0x23,0x34,0x35,0x36,0x30,0x3F,0x40,0x3B,0x47,0x46,0x2B,
	0x29,0x21,0x2C,0x32,0x2D,0x2F,0x44,0x22,0x3A,0x2E,0x39,  -1,  -1,  -1,  -1,  -1,
	0x4B,0x4F,0x50,0x51,0x52,0x53,0x54,0x56,0x57,0x58,0x59,0x55,  -1,0x5C,0x4D,0x5D,
	0x0B,0x0C,0x0D,0x0E,0x0F,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,0x3D,0x3C,0x48,0x28,0x41,0x42,
	0x2A,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,0x37,0x25,0x3E,0x24,  -1,
	  -1,  -1,0x49,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	  -1,  -1,  -1,  -1,0x10,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1
};

class FIFO;

class IOCTRL : public DEVICE
{
private:
	DEVICE *d_pic, *d_fdc, *d_beep, *d_pcm;
	int did_pic_ir2, did_pic_ir3, did_fdc, did_beep, did_pcm_on, did_pcm_sig;
	
	void update_key();
	uint8* key_stat;
	int* mouse_stat;
	bool caps, kana;
	FIFO* key_buf;
	uint32 key_val, key_mouse;
	int key_prev;
	bool key_res, key_done;
	int regist_id;
	uint8 ts;
	
public:
	IOCTRL(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~IOCTRL() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_callback(int event_id, int err);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique function
	void set_context_pic(DEVICE* device, int id0, int id1) {
		d_pic = device; did_pic_ir2 = id0; did_pic_ir3 = id1;
	}
	void set_context_fdc(DEVICE* device, int id) {
		d_fdc = device; did_fdc = id;
	}
	void set_context_beep(DEVICE* device, int id) {
		d_beep = device; did_beep = id;
	}
	void set_context_pcm(DEVICE* device, int id0, int id1) {
		d_pcm = device; did_pcm_on = id0; did_pcm_sig = id1;
	}
	void key_down(int code);
	void key_up(int code);
};

#endif

