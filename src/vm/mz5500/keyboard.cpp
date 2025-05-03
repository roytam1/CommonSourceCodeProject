/*
	SHARP MZ-5500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.10 -

	[ keyboard ]
*/

#include "keyboard.h"
#include "../fifo.h"

#define BIT_DK	8
#define BIT_SRK	0x10
#define BIT_DC	1
#define BIT_STC	2

#define PHASE_IDLE	0

#define PHASE_SEND_EB_L	11
#define PHASE_SEND_D7_H	12
#define PHASE_SEND_D7_L	13
#define PHASE_SEND_D6_H	14
#define PHASE_SEND_D6_L	15
#define PHASE_SEND_D5_H	16
#define PHASE_SEND_D5_L	17
#define PHASE_SEND_D4_H	18
#define PHASE_SEND_D4_L	19
#define PHASE_SEND_D3_H	20
#define PHASE_SEND_D3_L	21
#define PHASE_SEND_D2_H	22
#define PHASE_SEND_D2_L	23
#define PHASE_SEND_D1_H	24
#define PHASE_SEND_D1_L	25
#define PHASE_SEND_D0_H	26
#define PHASE_SEND_D0_L	27
#define PHASE_SEND_PB_H	28
#define PHASE_SEND_PB_L	29
#define PHASE_SEND_RE_H	30
#define PHASE_SEND_RE_L	31
#define PHASE_SEND_END	32

#define PHASE_RECV_D4_H	41
#define PHASE_RECV_D4_L	42
#define PHASE_RECV_D3_H	43
#define PHASE_RECV_D3_L	44
#define PHASE_RECV_D2_H	45
#define PHASE_RECV_D2_L	46
#define PHASE_RECV_D1_H	47
#define PHASE_RECV_D1_L	48
#define PHASE_RECV_D0_H	49
#define PHASE_RECV_D0_L	50
#define PHASE_RECV_PB_H	51
#define PHASE_RECV_PB_L	52
#define PHASE_RECV_RE_H	53
#define PHASE_RECV_RE_L	54
#define PHASE_RECV_END	55

#define TIMEOUT_500MSEC	30
#define TIMEOUT_100MSEC	6

#define SET_DK(v) { \
	d_pio->write_signal(did_pio, (dk = (v) ? 1 : 0) ? 0 : BIT_DK, BIT_DK); \
}
#define SET_SRK(v) { \
	d_pio->write_signal(did_pio, (srk = (v) ? 1 : 0) ? 0 : BIT_SRK, BIT_SRK); \
	d_pic->write_signal(did_pic, srk ? 0 : 1, 1); \
}

void KEYBOARD::initialize()
{
	key_stat = emu->key_buffer();
	mouse_stat = emu->mouse_buffer();
	key_buf = new FIFO(64);
	rsp_buf = new FIFO(16);
	caps = kana = graph = false;
	
	vm->regist_frame_event(this);
}

void KEYBOARD::reset()
{
	key_buf->clear();
	rsp_buf->clear();
	SET_DK(1);
	SET_SRK(1);
	dc = stc = 1;
	phase = PHASE_IDLE;
	timeout = 0;
}

void KEYBOARD::write_signal(int id, uint32 data, uint32 mask)
{
	// from 8255 port c
	dc = (data & BIT_DC) ? 0 : 1;
	stc = (data & BIT_STC) ? 0 : 1;
	drive();
}

void KEYBOARD::event_frame()
{
	if(timeout > 0)
		timeout--;
	drive();
}

void KEYBOARD::key_down(int code)
{
	if(code == 0x1d) {
		// muhenkan->graph
		if(graph)
			graph = false;
		else {
			graph = true;
			kana = false;
		}
		return;
	}
	else if(code == 0xf0) {
		// caps
		caps = !caps;
		return;
	}
	else if(code == 0xf2) {
		// kana
		if(kana)
			kana = false;
		else {
			kana = true;
			graph = false;
		}
		return;
	}
	int shift = key_stat[0x10];
	int ctrl = key_stat[0x11];
	int algo = key_stat[0x12];
	
	if(kana) {
		if(shift)
			code = key_table_kana_shift[code];
		else
			code = key_table_kana[code];
	}
	else if(graph) {
		if(shift)
			code = key_table_graph_shift[code];
		else
			code = key_table_graph[code];
	}
	else {
		if(shift)
			code = key_table_shift[code];
		else
			code = key_table[code];
	}
	if(!code)
		return;
	if(caps) {
		if(0x41 <= code && code <= 0x5a)
			code += 0x20;
		else if(0x61 <= code && code <= 0x7a)
			code -= 0x20;
	}
	if(ctrl)
		key_buf->write(2);
	else if(algo)
		key_buf->write(4);
	key_buf->write(code);
//	drive();
}

void KEYBOARD::key_up(int code)
{
	// dont check key break
}

#define NEXTPHASE() { \
	phase++; \
	timeout = TIMEOUT_100MSEC; \
}

void KEYBOARD::drive()
{
	switch(phase)
	{
	case PHASE_IDLE:
		if(dc && (!key_buf->empty() || !rsp_buf->empty())) {
			if(!rsp_buf->empty())
				send = rsp_buf->read();
			else
				send = key_buf->read();
			send = ~send & 0x1ff;
			int parity = 0;
			for(int i = 0; i < 9; i++)
				parity += (send & (1 << i)) ? 1 : 0;
			send = (send << 1) | (parity & 1);
			
			SET_DK(0);
			SET_SRK(0);
			phase = PHASE_SEND_EB_L;
			// 500msec
			timeout = TIMEOUT_500MSEC;
		}
		else if(!dc && !stc) {
			recv = 0;
			SET_DK(0);
			phase = PHASE_RECV_D4_H;
			// 500msec
			timeout = TIMEOUT_500MSEC;
		}
		break;
	case PHASE_SEND_EB_L:
		if(!stc) {
			SET_DK(send & 0x200);
			SET_SRK(1);
			NEXTPHASE();
		}
		break;
	case PHASE_SEND_D7_H:
	case PHASE_SEND_D6_H:
	case PHASE_SEND_D5_H:
	case PHASE_SEND_D4_H:
	case PHASE_SEND_D3_H:
	case PHASE_SEND_D2_H:
	case PHASE_SEND_D1_H:
	case PHASE_SEND_D0_H:
	case PHASE_SEND_PB_H:
		if(stc) {
			SET_DK(send & 0x100);
			send <<= 1;
			NEXTPHASE();
		}
		break;
	case PHASE_SEND_D7_L:
	case PHASE_SEND_D6_L:
	case PHASE_SEND_D5_L:
	case PHASE_SEND_D4_L:
	case PHASE_SEND_D3_L:
	case PHASE_SEND_D2_L:
	case PHASE_SEND_D1_L:
	case PHASE_SEND_D0_L:
	case PHASE_SEND_PB_L:
		if(!stc) {
			NEXTPHASE();
		}
		break;
	case PHASE_SEND_RE_H:
		if(stc) {
			SET_DK(0);
			NEXTPHASE();
		}
		break;
	case PHASE_SEND_RE_L:
		if(!stc) {
			NEXTPHASE();
		}
		break;
	case PHASE_SEND_END:
		if(stc) {
			SET_DK(1);
			phase = PHASE_IDLE;
		}
		break;
	case PHASE_RECV_D4_H:
	case PHASE_RECV_D3_H:
	case PHASE_RECV_D2_H:
	case PHASE_RECV_D1_H:
	case PHASE_RECV_D0_H:
	case PHASE_RECV_PB_H:
		if(stc) {
			NEXTPHASE();
		}
		break;
	case PHASE_RECV_D4_L:
	case PHASE_RECV_D3_L:
	case PHASE_RECV_D2_L:
	case PHASE_RECV_D1_L:
	case PHASE_RECV_D0_L:
	case PHASE_RECV_PB_L:
		if(!stc) {
			recv = (recv << 1) | (dc ? 1 : 0);
			NEXTPHASE();
		}
		break;
	case PHASE_RECV_RE_H:
		if(stc) {
			SET_DK(1);
			NEXTPHASE();
		}
		break;
	case PHASE_RECV_RE_L:
		if(!stc) {
			NEXTPHASE();
		}
		break;
	case PHASE_RECV_END:
		if(stc) {
			recv >>= 1;
			recv = ~recv & 0x1f;
			process(recv);
			phase = PHASE_IDLE;
		}
		break;
	}
	
	// timeout
	if(phase != PHASE_IDLE && !(timeout > 0)) {
		SET_DK(1);
		SET_SRK(1);
		phase = PHASE_IDLE;
	}
}

void KEYBOARD::process(int cmd)
{
	int mx, my, mb;
	
	switch(cmd)
	{
	case 1:
		// mouse ???
		mx = mouse_stat[0]; mx = (mx > 126) ? 126 : (mx < -128) ? -128 : mx;
		my = mouse_stat[1]; my = (my > 126) ? 126 : (my < -128) ? -128 : my;
		mb = mouse_stat[2];
//		rsp_buf->clear();
		rsp_buf->write(0x140 | (mx & 0x3f));
		rsp_buf->write(0x140 | (my & 0x3f));
		rsp_buf->write(0x140 | ((my >> 2) & 0x30) | ((mx >> 4) & 0xc) | (mb & 3));
		break;
	case 25:
		// clear buffer ?
//		key_buf->clear();
		break;
	case 30:
		// version ???
//		rsp_buf->write(0x110);
		break;
	}
}

