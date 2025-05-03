/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.11-

	[ 74LS393 ]
*/

#ifndef _LS393_H_
#define _LS393_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_LS393_CLK	0

class LS393 : public DEVICE
{
private:
	DEVICE* dev[8][MAX_OUTPUT];
	int did[8][MAX_OUTPUT], dcount[MAX_OUTPUT];
	
	int accum[8];
	
public:
	LS393(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		for(int i = 0; i < 8; i++)
			dcount[i] = accum[i] = 0;
	}
	~LS393() {}
	
	// common functions
	void write_signal(int id, uint32 data, uint32 mask) {
		for(int i = 0; i < 8; i++) {
			accum[i] += data & mask;
			int c = accum[i] >> (i + 1);
			if(c) {
				for(int j = 0; j < dcount[i]; j++)
					dev[i][j]->write_signal(did[i][j], c, 0xffffffff);
				accum[i] &= (1 << (i + 1)) - 1;
			}
		}
	}
	
	// unique functions
	void set_context_1qa(DEVICE* device, int id) {
		int c = dcount[0]++;
		dev[0][c] = device; did[0][c] = id;
	}
	void set_context_1qb(DEVICE* device, int id) {
		int c = dcount[1]++;
		dev[1][c] = device; did[1][c] = id;
	}
	void set_context_1qc(DEVICE* device, int id) {
		int c = dcount[2]++;
		dev[2][c] = device; did[2][c] = id;
	}
	void set_context_1qd(DEVICE* device, int id) {
		int c = dcount[3]++;
		dev[3][c] = device; did[3][c] = id;
	}
	void set_context_2qa(DEVICE* device, int id) {
		int c = dcount[4]++;
		dev[4][c] = device; did[4][c] = id;
	}
	void set_context_2qb(DEVICE* device, int id) {
		int c = dcount[5]++;
		dev[5][c] = device; did[5][c] = id;
	}
	void set_context_2qc(DEVICE* device, int id) {
		int c = dcount[6]++;
		dev[6][c] = device; did[6][c] = id;
	}
	void set_context_2qd(DEVICE* device, int id) {
		int c = dcount[7]++;
		dev[7][c] = device; did[7][c] = id;
	}
};

#endif

