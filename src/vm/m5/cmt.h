/*
	SORD m5 Emulator 'Emu5'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ cmt/printer ]
*/

#ifndef _CMT_H_
#define _CMT_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define SIG_CMT_IN		0
#define SIG_PRINTER_BUSY	1

class CMT : public DEVICE
{
private:
	DEVICE* dev;
	int out_id, rmt_id;
	
	// data recorder
	bool in, out, remote;
	
	// printer
	uint8 pout;
	bool strobe, busy;
	
public:
	CMT(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~CMT() {}
	
	// common functions
	void initialize();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	
	// unique functions
	void set_context(DEVICE* device, int out, int remote) { dev = device; out_id = out; rmt_id = remote; }
};

#endif
