/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.15-

	[ YM2203 ]
*/

#ifndef _YM2203_H_
#define _YM2203_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"
#include "fmgen/opna.h"

#define SIG_YM2203_PORT_A	0
#define SIG_YM2203_PORT_B	1
#define SIG_YM2203_MUTE		2

class YM2203 : public DEVICE
{
private:
	DEVICE* dev[2][MAX_OUTPUT];
	int dev_id[2][MAX_OUTPUT], dev_shift[2][MAX_OUTPUT], dev_cnt[2];
	uint32 dev_mask[2][MAX_OUTPUT];
	
	FM::OPN* opn;
	int usec;
	int32* sound_tmp;
	
	uint8 ch, mode;
	typedef struct {
		uint8 wreg;
		uint8 rreg;
		bool first;
	} port_t;
	port_t port[2];
	bool mute;
	
public:
	YM2203(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dev_cnt[0] = dev_cnt[1] = 0;
		port[0].wreg = port[1].wreg = port[0].rreg = port[1].rreg = 0;//0xff;
	}
	~YM2203() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_vsync(int v, int clock);
	void mix(int32* buffer, int cnt);
	
	// unique functions
	void set_context_port_a(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dev_cnt[0]++;
		dev[0][c] = device; dev_id[0][c] = id; dev_mask[0][c] = mask; dev_shift[0][c] = shift;
	}
	void set_context_port_b(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dev_cnt[1]++;
		dev[1][c] = device; dev_id[1][c] = id; dev_mask[1][c] = mask; dev_shift[1][c] = shift;
	}
	void init(int rate, int clock, int samples);
};

#endif

