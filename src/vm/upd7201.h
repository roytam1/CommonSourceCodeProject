/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.12
	
	[ uPD7201 ]
*/

#ifndef _UPD7201_H_
#define _UPD7201_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_UPD7201_RECV_CH0	0
#define SIG_UPD7201_RECV_CH1	1

#define EVENT_SEND	0
#define EVENT_RECV	2
#define DELAY_SEND	10
#define DELAY_RECV	10

class FIFO;

class UPD7201 : public DEVICE
{
private:
	DEVICE* dev_send[2];
	DEVICE* dev_intr[MAX_OUTPUT];
	int dev_send_id[2];
	int dev_intr_id[MAX_OUTPUT], dev_intr_cnt;
	uint32 dev_intr_mask[MAX_OUTPUT];
	
	// serial
	typedef struct {
		int ch;
		uint8 wr[8];
		uint8 vector;
		// buffer
		FIFO* recv;
		FIFO* rtmp;
		int send_id;
		int recv_id;
	} port_t;
	port_t port[2];
	
public:
	UPD7201(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dev_send[0] = dev_send[1] = NULL;
		dev_intr_cnt = 0;
	}
	~UPD7201() {}
	
	// common functions
	void initialize();
	void release();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	void event_callback(int event_id, int err);
	
	// unique function
	void set_context_ch0(DEVICE* device, int id) {
		dev_send[0] = device;
		dev_send_id[0] = id;
	}
	void set_context_ch1(DEVICE* device, int id) {
		dev_send[1] = device;
		dev_send_id[1] = id;
	}
	void set_context_intr(DEVICE* device, int id, uint32 mask) {
		int c = dev_intr_cnt++;
		dev_intr[c] = device;
		dev_intr_id[c] = id;
		dev_intr_mask[c] = mask;
	}
};

#endif

