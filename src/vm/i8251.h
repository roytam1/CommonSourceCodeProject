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

// max 256kbytes
#define BUFFER_SIZE	0x40000
// 100usec/byte
#define RECV_DELAY	100

#define TXRDY		0x01
#define RXRDY		0x02
#define TXE		0x04
#define PE		0x08
#define OE		0x10
#define FE		0x20
#define SYNDET		0x40
#define DSR		0x80

#define MODE_CLEAR	0
#define MODE_SYNC	1
#define MODE_ASYNC	2
#define MODE_SYNC1	3
#define MODE_SYNC2	4

class FIFO;

class I8251 : public DEVICE
{
private:
	DEVICE* dev[MAX_OUTPUT];
	int did[MAX_OUTPUT], dcount;
	
	// i8251
	uint8 recv, status, mode;
	bool txen, rxen, dsr;
	// recv buffer
	FIFO* fifo;
	int regist_id;
	
public:
	I8251(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount = 0;
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
	void set_context(DEVICE* device, int id) {
		int c = dcount++;
		dev[c] = device; did[c] = id;
	}
};

#endif

