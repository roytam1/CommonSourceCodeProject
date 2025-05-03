/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.19-

	[ uPD1990A ]
*/

#ifndef _UPD1990A_H_
#define _UPD1990A_H_

#define SIG_UPD1990A_CLK	0
#define SIG_UPD1990A_STB	1
#define SIG_UPD1990A_C0		2
#define SIG_UPD1990A_C1		3
#define SIG_UPD1990A_C2		4
#define SIG_UPD1990A_DIN	5

#include "vm.h"
#include "../emu.h"
#include "device.h"

class UPD1990A : public DEVICE
{
private:
	DEVICE *dev[MAX_OUTPUT];
	int did[MAX_OUTPUT];
	uint32 dmask[MAX_OUTPUT];
	int dcount;
	
	uint8 cmd, mode;
	uint32 srl, srh;
	bool clk, stb, din;
	
public:
	UPD1990A(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount = 0;
	}
	~UPD1990A() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_dout(DEVICE* device, int id, uint32 mask) {
		int c = dcount++;
		dev[c] = device; did[c] = id; dmask[c] = mask;
	}
};

#endif

