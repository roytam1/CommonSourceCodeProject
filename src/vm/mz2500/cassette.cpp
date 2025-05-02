/*
	SHARP MZ-80B Emulator 'EmuZ-80B'
	SHARP MZ-2200 Emulator 'EmuZ-2200'
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.04 -

	[ cassette ]
*/

#include "cassette.h"
#include "../datarec.h"
#include "../i8255.h"

#define EVENT_APSS	0

void CASSETTE::initialize()
{
	pa = pb = pc = 0xff;
	play = rec = false;
	now_play = now_rewind = false;
}

void CASSETTE::reset()
{
#ifndef _MZ80B
	now_apss = false;
	register_id = -1;
#endif
	close_datarec();
}

void CASSETTE::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff) {
	case 0xe1:
		pb = data;
		break;
	}
}

void CASSETTE::fast_forwad()
{
	if(play) {
		d_drec->set_ff_rew(1);
		d_drec->set_remote(true);
	}
	now_play = now_rewind = false;
}

void CASSETTE::fast_rewind()
{
	if(play) {
		d_drec->set_ff_rew(-1);
		d_drec->set_remote(true);
	}
	now_rewind = play;
	now_play = false;
}

void CASSETTE::forward()
{
	if(play || rec) {
		d_drec->set_ff_rew(0);
		d_drec->set_remote(true);
	}
	now_play = (play || rec);
	now_rewind = false;
}

void CASSETTE::stop()
{
	if(play || rec) {
		d_drec->set_remote(false);
	}
	now_play = now_rewind = false;
	d_pio->write_signal(SIG_I8255_PORT_B, 0, 0x40);
}

void CASSETTE::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_CASSETTE_PIO_PA) {
#ifdef _MZ80B
		if(!(pa & 1) && (data & 1)) {
			if(data & 2) {
				fast_forwad();
			} else {
				fast_rewind();
			}
		}
		if(!(pa & 4) && (data & 4)) {
			forward();
		}
		if(!(pa & 8) && (data & 8)) {
			stop();
		}
#else
		if((pa & 1) && !(data & 1)) {
			fast_rewind();
			now_apss = ((pa & 0x80) == 0);
		}
		if((pa & 2) && !(data & 2)) {
			fast_forwad();
			now_apss = ((pa & 0x80) == 0);
		}
		if((pa & 4) && !(data & 4)) {
			forward();
			now_apss = false;
		}
		if((pa & 8) && !(data & 8)) {
			stop();
			// stop apss
			if(register_id != -1) {
				cancel_event(register_id);
				register_id = -1;
			}
			now_apss = false;
		}
#endif
		pa = data;
	} else if(id == SIG_CASSETTE_PIO_PC) {
		if(!(pc & 2) && (data & 2)) {
			vm->special_reset();
		}
		if(!(pc & 8) && (data & 8)) {
			vm->reset();
		}
		if((pc & 0x10) && !(data & 0x10)) {
			// eject
		}
		d_drec->write_signal(SIG_DATAREC_OUT, data, 0x80);
		pc = data;
	} else if(id == SIG_CASSETTE_OUT) {
#ifndef _MZ80B
		if(now_apss) {
			if((data & mask) && register_id == -1) {
				register_event(this, EVENT_APSS, 350000, false, &register_id);
				d_pio->write_signal(SIG_I8255_PORT_B, 0x40, 0x40);
			}
		} else
#endif
		if(now_play) {
			d_pio->write_signal(SIG_I8255_PORT_B, (data & mask) ? 0x40 : 0, 0x40);
		}
	} else if(id == SIG_CASSETTE_REMOTE) {
		d_pio->write_signal(SIG_I8255_PORT_B, (data & mask) ? 0 : 8, 8);
	} else if(id == SIG_CASSETTE_END) {
		if((data & mask) && now_play) {
#ifndef _MZ80B
			if(!(pa & 0x20)) {
				fast_rewind();
			}
#endif
			now_play = false;
		}
	} else if(id == SIG_CASSETTE_TOP) {
		if((data & mask) && now_rewind) {
#ifndef _MZ80B
			if(!(pa & 0x40)) {
				forward();
			}
#endif
			now_rewind = false;
		}
	}
}

#ifndef _MZ80B
void CASSETTE::event_callback(int event_id, int err)
{
	if(event_id == EVENT_APSS) {
		register_id = -1;
		d_pio->write_signal(SIG_I8255_PORT_B, 0, 0x40);
	}
}
#endif

void CASSETTE::play_datarec(bool value)
{
	play = value;
	rec = false;
	d_pio->write_signal(SIG_I8255_PORT_B, play ? 0x10 : 0x30, 0x30);
}

void CASSETTE::rec_datarec(bool value)
{
	play = false;
	rec = value;
	d_pio->write_signal(SIG_I8255_PORT_B, rec ? 0 : 0x30, 0x30);
}

void CASSETTE::close_datarec()
{
	play = rec = false;
	now_play = now_rewind = false;
	d_pio->write_signal(SIG_I8255_PORT_B, 0x30, 0x30);
	d_pio->write_signal(SIG_I8255_PORT_B, 0, 0x40);
	
#ifndef _MZ80B
	if(register_id != -1) {
		cancel_event(register_id);
		register_id = -1;
	}
#endif
}

