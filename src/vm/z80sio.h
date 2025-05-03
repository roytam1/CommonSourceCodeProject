/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.31 -

	[ Z80SIO ]
*/

#ifndef _Z80SIO_H_
#define _Z80SIO_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_Z80SIO_RECV_CH0	0
#define SIG_Z80SIO_RECV_CH1	1
#define SIG_Z80SIO_CLEAR_CH0	2
#define SIG_Z80SIO_CLEAR_CH1	3

#define EVENT_SEND	2
#define EVENT_RECV	4
#define DELAY_SEND	2000
#define DELAY_RECV	2000

class FIFO;

class Z80SIO : public DEVICE
{
private:
	DEVICE *d_rts[2][MAX_OUTPUT], *d_dtr[2][MAX_OUTPUT], *d_send[2][MAX_OUTPUT];
	int did_rts[2][MAX_OUTPUT], did_dtr[2][MAX_OUTPUT], did_send[2][MAX_OUTPUT];
	uint32 dmask_rts[2][MAX_OUTPUT], dmask_dtr[2][MAX_OUTPUT];
	int dcount_rts[2], dcount_dtr[2], dcount_send[2];
	DEVICE* d_pic;
	int pri;
	
	typedef struct {
		int ch;
		uint8 wr[8];
		uint8 vector;
		bool nextrecv_intr;
		bool first_data;
		bool send_intr;
		int recv_intr;
		bool over_flow;
		bool under_flow;
		// buffer
		FIFO* send;
		FIFO* recv;
		FIFO* rtmp;
		int send_id;
		int recv_id;
	} port_t;
	port_t port[2];
	
public:
	Z80SIO(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_rts[0] = dcount_rts[1] = 0;
		dcount_dtr[0] = dcount_dtr[1] = 0;
		d_pic = NULL;
	}
	~Z80SIO() {}
	
	// common functions
	void initialize();
	void reset();
	void release();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_callback(int event_id, int err);
	void do_reti();
	
	// unique functions
	void set_context_rts0(DEVICE* device, int id, uint32 mask) {
		int c = dcount_rts[0]++;
		d_rts[0][c] = device; did_rts[0][c] = id; dmask_rts[0][c] = mask;
	}
	void set_context_dtr0(DEVICE* device, int id, uint32 mask) {
		int c = dcount_dtr[0]++;
		d_dtr[0][c] = device; did_dtr[0][c] = id; dmask_dtr[0][c] = mask;
	}
	void set_context_rts1(DEVICE* device, int id, uint32 mask) {
		int c = dcount_rts[1]++;
		d_rts[1][c] = device; did_rts[1][c] = id; dmask_rts[1][c] = mask;
	}
	void set_context_dtr1(DEVICE* device, int id, uint32 mask) {
		int c = dcount_dtr[1]++;
		d_dtr[1][c] = device; did_dtr[1][c] = id; dmask_dtr[1][c] = mask;
	}
	void set_context_int(DEVICE* device, int priority) {
		d_pic = device; pri = priority;
	}
};

#endif

