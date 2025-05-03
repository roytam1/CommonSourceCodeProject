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
	int did[2][MAX_OUTPUT], dshift[2][MAX_OUTPUT], dcount[2];
	uint32 dmask[2][MAX_OUTPUT];
	
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
		dcount[0] = dcount[1] = 0;
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
		int c = dcount[0]++;
		dev[0][c] = device; did[0][c] = id; dmask[0][c] = mask; dshift[0][c] = shift;
	}
	void set_context_port_b(DEVICE* device, int id, uint32 mask, int shift) {
		int c = dcount[1]++;
		dev[1][c] = device; did[1][c] = id; dmask[1][c] = mask; dshift[1][c] = shift;
	}
	void init(int rate, int clock, int samples, int volf, int volp);
};

#endif

