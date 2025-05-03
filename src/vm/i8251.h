/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.14 -

	[ i8251 ]
*/

#ifndef _I8251_H_
#define _I8251_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_I8251_RECV	0
#define SIG_I8251_CLEAR	1

class FIFO;

class I8251 : public DEVICE
{
private:
	DEVICE *d_sio[MAX_OUTPUT], *d_rxrdy[MAX_OUTPUT];
	int did_sio[MAX_OUTPUT], did_rxrdy[MAX_OUTPUT];
	uint32 dmask_rxrdy[MAX_OUTPUT];
	int dcount_sio, dcount_rxrdy;
	
	// i8251
	uint8 recv, status, mode;
	bool txen, rxen, dsr;
	// recv buffer
	FIFO* fifo;
	int regist_id;
	
public:
	I8251(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_sio = dcount_rxrdy = 0;
	}
	~I8251() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_callback(int event_id, int err);
	
	// unique functions
	void set_context_sio(DEVICE* device, int id) {
		int c = dcount_sio++;
		d_sio[c] = device; did_sio[c] = id;
	}
	void set_context_rxrdy(DEVICE* device, int id, uint32 mask) {
		int c = dcount_rxrdy++;
		d_rxrdy[c] = device; did_rxrdy[c] = id; dmask_rxrdy[c] = mask;
	}
};

#endif

