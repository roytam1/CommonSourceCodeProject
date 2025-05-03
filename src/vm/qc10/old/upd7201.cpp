/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.15-

	[ uPD7201 ]
*/

#include "upd7201.h"
#include "i8259.h"
#include "fifo.h"

void UPD7201::initialize()
{
	// initialize
	for(int i = 0; i < 2; i++) {
		sio[i].ch = 0;
		sio[i].vector = 0;
		_memset(sio[i].wr, 0, sizeof(sio[i].wr));
		// create fifo
		sio[i].send = new FIFO(16);
		sio[i].recv = new FIFO(16);
		sio[i].rtmp = new FIFO(16);
	}
	for(int i = 0; i < 8; i++)
		key_led[i] = false;
	key_repeat = key_enable = true;
	
	key_stat = emu->key_buffer();
}

void UPD7201::release()
{
	for(int i = 0; i < 2; i++) {
		if(sio[i].send)
			delete sio[i].send;
		if(sio[i].recv)
			delete sio[i].recv;
		if(sio[i].rtmp)
			delete sio[i].rtmp;
	}
}

#define SEND(c, data) { \
	if(sio[c].send->empty()) { \
		int id; \
		vm->regist_callback(this, EVENT_SIO_SEND + (c), 10, false, &id); \
	} \
	sio[c].send->write(data); \
}
#define RECV(c, data) { \
	if(sio[c].rtmp->empty()) { \
		int id; \
		vm->regist_callback(this, EVENT_SIO_RECV + (c), 10, false, &id); \
	} \
	sio[c].rtmp->write(data); \
}
#define RECV_TOP(c, data) { \
	if(sio[c].rtmp->empty()) { \
		int id; \
		vm->regist_callback(this, EVENT_SIO_RECV + (c), 10, false, &id); \
	} \
	sio[c].rtmp->init(); \
	sio[c].rtmp->write(data); \
}

void UPD7201::event_callback(int event_id, int err)
{
	if(event_id == EVENT_SIO_SEND || event_id == EVENT_SIO_SEND + 1) {
		int c = (event_id == EVENT_SIO_SEND) ? 0 : 1;
		
		// keyboard command
		if(c == 0)
			process_cmd(sio[0].send->read());
		// clear buffer
		sio[c].send->init();
		
		// txredy interrupt
		if(sio[c].wr[1] & 2) {
			// vector reg is in ch.b
			sio[1].vector = sio[1].wr[2] & 0xfe;
			if(sio[c].wr[1] & 4) {
				sio[1].vector &= 0xf0;
				sio[1].vector |= c ? 0 : 0x8;
			}
			// interrupt occur
			vm->pic->request_int(4+0, true);
		}
	}
	else if(event_id == EVENT_SIO_RECV || event_id == EVENT_SIO_RECV + 1) {
		int c = (event_id == EVENT_SIO_RECV) ? 0 : 1;
		
		// recieve 1 byte
		if(sio[c].rtmp->count()) {
			sio[c].recv->write(sio[c].rtmp->read());
			
			// rxredy interrupt
			if(sio[c].wr[1] & 0x18) {
				// vector reg is in ch.b
				sio[1].vector = sio[1].wr[2] & 0xfe;
				if(sio[c].wr[1] & 4) {
					sio[1].vector &= 0xf0;
					sio[1].vector |= c ? 0x4 : 0xc;
				}
				// interrupt occur
				vm->pic->request_int(4+0, true);
			}
		}
		// regist event again for next 1 byte
		if(sio[c].rtmp->count()) {
			int id;
			vm->regist_callback(this, event_id, 10 + err, false, &id);
		}
	}
}

void UPD7201::write_io8(uint16 addr, uint8 data)
{
	switch(addr & 0xff)
	{
		case 0x10:
		case 0x11:
			// data for keyboard/rs-232c
			SEND(addr & 1, data);
			break;
		case 0x12:
		case 0x13:
			// command
			if(sio[addr & 1].ch) {
				sio[addr & 1].wr[sio[addr & 1].ch] = data;
				sio[addr & 1].ch = 0;
			}
			else {
				sio[addr & 1].wr[0] = data;
				sio[addr & 1].ch = data & 7;
			}
			break;
	}
}

uint8 UPD7201::read_io8(uint16 addr)
{
	switch(addr & 0xff)
	{
		case 0x10:
		case 0x11:
			// data from keyboard/rs-232c
			return sio[addr & 1].recv->read();
		case 0x12:
		case 0x13:
			// status
			if(sio[addr & 1].ch) {
				// rr1, rr2
				uint8 val = 0xff;
				if(sio[addr & 1].ch == 1)
					val = sio[addr & 1].send->empty() ? 1 : 0;
				else if(sio[addr & 1].ch == 2)
					val = sio[addr & 1].vector;
				sio[addr & 1].ch = 0;
				return val;
			}
			// rr0
			return (sio[addr & 1].recv->count() ? 1 : 0) | (sio[addr & 1].send->empty() ? 4 : 0);
	}
	return 0xff;
}

// keyboard control

void UPD7201::key_down(int code)
{
	// send key code
	if(key_map[code] && key_enable) {
		RECV(0, key_map[code]);
	}
}

void UPD7201::key_up(int code)
{
	// shift break
	if(key_enable) {
		if(code == 0x10) {
			RECV(0, 0x86);	// shift break;
		}
		else if(code == 0x11) {
			RECV(0, 0x8a);	// ctrl break
		}
		else if(code == 0x12) {
			RECV(0, 0x8c);	// graph break
		}
	}
}

void UPD7201::process_cmd(uint8 data)
{
	switch(data & 0xe0)
	{
		case 0x00:
			// repeat starting time set:
			break;
		case 0x20:
			// repeat interval set
			break;
		case 0xa0:
			// repeat control
			key_repeat = (data & 1) ? true : false;
			break;
		case 0x40:
			// key_led control
			key_led[(data >> 1) & 7] = (data & 1) ? true : false;
			break;
		case 0x60:
			// key_led status read
			RECV_TOP(0, 0xc0 | (key_led[0] ? 1 : 0));
			for(int i = 0; i < 8; i++)
				RECV(0, 0xc0 | (i << 1) | (key_led[i] ? 1 : 0));
			break;
			
		case 0x80:
			// key sw status read
			RECV_TOP(0, 0x80);
			RECV(0, 0x82);
			RECV(0, 0x84);
			RECV(0, 0x86 | (key_stat[0x10] ? 1 : 0));
			RECV(0, 0x88);
			RECV(0, 0x8a | (key_stat[0x11] ? 1 : 0));
			RECV(0, 0x8c | (key_stat[0x12] ? 1 : 0));
			RECV(0, 0x8e);
			break;
			
		case 0xc0:
			// keyboard enable
			key_enable = (data & 1) ? true : false;
			break;
			
		case 0xe0:
			// reset
			for(int i = 0; i < 8; i++)
				key_led[i] = false;
			key_repeat = key_enable = true;
			// diagnosis
			if(!(data & 1)) {
				RECV_TOP(0, 0);	// 0 or 0xff
			}
			break;
	}
}
