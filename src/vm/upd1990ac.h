/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.19-

	[ uPD1990AC ]
*/

#ifndef _UPD1990AC_H_
#define _UPD1990AC_H_

#define SIG_UPD1990AC_CLK	0
#define SIG_UPD1990AC_STB	1
#define SIG_UPD1990AC_C0	2
#define SIG_UPD1990AC_C1	3
#define SIG_UPD1990AC_C2	4
#define SIG_UPD1990AC_DIN	5

#include "vm.h"
#include "../emu.h"
#include "device.h"

class UPD1990AC : public DEVICE
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
	UPD1990AC(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount = 0;
	}
	~UPD1990AC() {}
	
	// common functions
	void initialize();
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context_dout(DEVICE* device, int id, uint32 mask) {
		int c = dcount++;
		dev[c] = device; did[c] = id; dmask[c] = mask;
	}
};

#endif

