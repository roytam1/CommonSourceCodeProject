/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.04 -

	[ cassette ]
*/

#include "cassette.h"
#include "../i8255.h"

// pre-silent (0.2sec)
#define REGISTER_PRE() { \
	set_signal(false); \
	vm->register_event(this, EVENT_PRE, 200000, false, &id_pre); \
}
// signal (0.5sec)
#define REGISTER_SIGNAL() { \
	set_signal(true); \
	vm->register_event(this, EVENT_SIGNAL, 500000, false, &id_signal); \
}
// after-silent (0.3sec)
#define REGISTER_AFTER() { \
	set_signal(false); \
	vm->register_event(this, EVENT_AFTER, 300000, false, &id_after); \
}
#define CANCEL_EVENT() { \
	if(id_pre != -1) { \
		vm->cancel_event(id_pre); \
	} \
	if(id_signal != -1) { \
		vm->cancel_event(id_signal); \
	} \
	if(id_after != -1) { \
		vm->cancel_event(id_after); \
	} \
	id_pre = id_signal = id_after = -1; \
}

void CASSETTE::initialize()
{
	prev = 0xff;
	signal = playing = true;
	set_signal(false);
	fw_rw = 1;
	set_fw_rw(0);
	track = 0.0;
	id_pre = id_signal = id_after = -1;
}

void CASSETTE::release()
{
	stop_media();
}

void CASSETTE::reset()
{
	id_pre = id_signal = id_after = -1;
}

void CASSETTE::write_signal(int id, uint32 data, uint32 mask)
{
	// from i8255 port a
	if((prev & 1) && !(data & 1)) {
		// rewind
		stop_media();
		set_fw_rw(-1);
		CANCEL_EVENT();
		REGISTER_PRE();
	}
	if((prev & 2) && !(data & 2)) {
		// forward
		stop_media();
		set_fw_rw(1);
		CANCEL_EVENT();
		REGISTER_PRE();
	}
	if((prev & 4) && !(data & 4)) {
		// start play
		play_media();
		set_fw_rw(1);
		CANCEL_EVENT();
	}
	if((prev & 8) && !(data & 8)) {
		// stop
		stop_media();
		set_fw_rw(0);
		CANCEL_EVENT();
	}
	prev = data & mask;
}

void CASSETTE::event_callback(int event_id, int err)
{
	if(event_id == EVENT_PRE) {
		id_pre = -1;
		REGISTER_SIGNAL();
	}
	else if(event_id == EVENT_SIGNAL) {
		id_signal = -1;
		REGISTER_AFTER();
	}
	else if(event_id == EVENT_AFTER) {
		id_after = -1;
		if(fw_rw == 1) {
			track = (float)((int)track + 1) + 0.1F;
			if((int)(track + 0.5) > emu->media_count()) {
				// reach last
				if(prev & 0x20) {
					set_fw_rw(0); // stop now
				}
				else {
					set_fw_rw(-1); // auto rewind
					REGISTER_PRE();
				}
				playing = false;
			}
			else {
				if(playing) {
					play_media(); // play next track
				}
				else {
					REGISTER_PRE();
				}
			}
		}
		else if(fw_rw == -1) {
			track = (float)((int)track - 1) + 0.9F;
			if(track < 1.0) {
				// reach top
				if(prev & 0x40) {
					set_fw_rw(0); // stop now
				}
				else {
					set_fw_rw(1); // auto play
					play_media();
				}
			}
			else {
				REGISTER_PRE();
			}
		}
	}
}

void CASSETTE::play_media()
{
	if((int)(track + 0.5) <= emu->media_count() && (int)(track + 0.5) <= track) {
		// current position is after signal
		track = (track < 1.0) ? 1.1F : (int)(track + 0.5) + 0.1F;
		emu->play_media((int)track);
	}
	else {
		// current position is pre signal
		REGISTER_PRE();
	}
	playing = true;
}

void CASSETTE::stop_media()
{
	emu->stop_media();
	playing = false;
}

void CASSETTE::set_fw_rw(int val)
{
	if(fw_rw != val) {
		d_pio->write_signal(SIG_I8255_PORT_B, val ? 0 : 8, 8);
		fw_rw = val;
	}
}

void CASSETTE::set_signal(bool val)
{
	if(signal != val) {
		d_pio->write_signal(SIG_I8255_PORT_B, val ? 0x40 : 0, 0x40);
		signal = val;
	}
}

