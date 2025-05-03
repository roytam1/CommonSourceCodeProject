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

#define SIG_I8251_RECV		0
#define SIG_I8251_DSR		1
#define SIG_I8251_CLEAR		2
#define SIG_I8251_LOOPBACK	3

class FIFO;

class I8251 : public DEVICE
{
private:
	DEVICE *d_out[MAX_OUTPUT], *d_rxrdy[MAX_OUTPUT], *d_txrdy[MAX_OUTPUT], *d_txe[MAX_OUTPUT], *d_dtr[MAX_OUTPUT];
	int did_out[MAX_OUTPUT], did_rxrdy[MAX_OUTPUT], did_txrdy[MAX_OUTPUT], did_txe[MAX_OUTPUT], did_dtr[MAX_OUTPUT];
	uint32 dmask_rxrdy[MAX_OUTPUT], dmask_txrdy[MAX_OUTPUT], dmask_txe[MAX_OUTPUT], dmask_dtr[MAX_OUTPUT];
	int dcount_out, dcount_rxrdy, dcount_txrdy, dcount_txe, dcount_dtr;
	
	// i8251
	uint8 recv, status, mode;
	bool txen, rxen, loopback;
	
	// buffer
	FIFO *recv_buffer;
	FIFO *send_buffer;
	int recv_id, send_id;
	
public:
	I8251(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_out = dcount_rxrdy = dcount_txrdy = dcount_txe = dcount_dtr = 0;
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
	void set_context_out(DEVICE* device, int id) {
		int c = dcount_out++;
		d_out[c] = device; did_out[c] = id;
	}
	void set_context_rxrdy(DEVICE* device, int id, uint32 mask) {
		int c = dcount_rxrdy++;
		d_rxrdy[c] = device; did_rxrdy[c] = id; dmask_rxrdy[c] = mask;
	}
	void set_context_txrdy(DEVICE* device, int id, uint32 mask) {
		int c = dcount_txrdy++;
		d_txrdy[c] = device; did_txrdy[c] = id; dmask_txrdy[c] = mask;
	}
	void set_context_txe(DEVICE* device, int id, uint32 mask) {
		int c = dcount_txe++;
		d_txe[c] = device; did_txe[c] = id; dmask_txe[c] = mask;
	}
	void set_context_dtr(DEVICE* device, int id, uint32 mask) {
		int c = dcount_dtr++;
		d_dtr[c] = device; did_dtr[c] = id; dmask_dtr[c] = mask;
	}
};

#endif

