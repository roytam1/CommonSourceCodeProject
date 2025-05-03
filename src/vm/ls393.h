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
	uint32 dmask[8][MAX_OUTPUT];
	
	uint32 count;
	bool prev_in;
	
public:
	LS393(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		for(int i = 0; i < 8; i++)
			dcount[i] = 0;
		count = 0;
		prev_in = false;
	}
	~LS393() {}
	
	// common functions
	void write_signal(int id, uint32 data, uint32 mask) {
		bool signal = ((data & mask) != 0);
		if(prev_in && !signal) {
			int prev_count = count++;
			for(int i = 0; i < 8; i++) {
				if(dcount[i]) {
					int bit = 1 << i;
					if((prev_count & bit) != (count & bit)) {
						uint32 val = (count & bit) ? 0xffffffff : 0;
						for(int j = 0; j < dcount[i]; j++)
							dev[i][j]->write_signal(did[i][j], val, dmask[i][j]);
					}
				}
			}
		}
		prev_in = signal;
	}
	
	// unique functions
	void set_context_1qa(DEVICE* device, int id, uint32 mask) {
		int c = dcount[0]++;
		dev[0][c] = device; did[0][c] = id; dmask[0][c] = mask;
	}
	void set_context_1qb(DEVICE* device, int id, uint32 mask) {
		int c = dcount[1]++;
		dev[1][c] = device; did[1][c] = id; dmask[1][c] = mask;
	}
	void set_context_1qc(DEVICE* device, int id, uint32 mask) {
		int c = dcount[2]++;
		dev[2][c] = device; did[2][c] = id; dmask[2][c] = mask;
	}
	void set_context_1qd(DEVICE* device, int id, uint32 mask) {
		int c = dcount[3]++;
		dev[3][c] = device; did[3][c] = id; dmask[3][c] = mask;
	}
	void set_context_2qa(DEVICE* device, int id, uint32 mask) {
		int c = dcount[4]++;
		dev[4][c] = device; did[4][c] = id; dmask[4][c] = mask;
	}
	void set_context_2qb(DEVICE* device, int id, uint32 mask) {
		int c = dcount[5]++;
		dev[5][c] = device; did[5][c] = id; dmask[5][c] = mask;
	}
	void set_context_2qc(DEVICE* device, int id, uint32 mask) {
		int c = dcount[6]++;
		dev[6][c] = device; did[6][c] = id; dmask[6][c] = mask;
	}
	void set_context_2qd(DEVICE* device, int id, uint32 mask) {
		int c = dcount[7]++;
		dev[7][c] = device; did[7][c] = id; dmask[7][c] = mask;
	}
};

#endif

