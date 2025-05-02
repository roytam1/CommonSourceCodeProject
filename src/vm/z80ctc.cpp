/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ Z80CTC ]
*/

#include "z80ctc.h"
#include "z80pic.h"

void Z80CTC::reset()
{
	for(int i = 0; i < 4; i++) {
		counter[i].count = counter[i].constant = 0xff;
		counter[i].control = 0;
		counter[i].clock = 256;
		counter[i].prescaler = 256;
		counter[i].freeze = counter[i].start = counter[i].latch = false;
	}
}

void Z80CTC::write_io8(uint32 addr, uint32 data)
{
	int c = addr & 3;
	
	if(counter[c].latch) {
		// time constant
		counter[c].constant = data ? data : 256;
		if(counter[c].control & 2)
			counter[c].count = counter[c].constant;
		counter[c].latch = false;
	}
	else {
		if(data & 1) {
			// control word
			if(!(data & 0x40)) {
				counter[c].prescaler = (data & 0x20) ? 256 : 16;
				if(counter[c].control & 0x40)
					counter[c].clock = counter[c].prescaler;
			}
			counter[c].latch = (data & 4) ? true : false;
			counter[c].freeze = ((data & 6) == 2) ? true : false;
			counter[c].start = (data & 8) ? false : true;
			counter[c].control = data;
		}
		else {
			// vector
			counter[0].vector = (data & 0xf8) | 0;
			counter[1].vector = (data & 0xf8) | 2;
			counter[2].vector = (data & 0xf8) | 4;
			counter[3].vector = (data & 0xf8) | 6;
		}
	}
}

uint32 Z80CTC::read_io8(uint32 addr)
{
	return counter[addr & 3].count & 0xff;
}

void Z80CTC::event_callback(int event_id)
{
	write_signal(SIG_Z80CTC_CLOCK, eventclock, 0xffffffff);
}

void Z80CTC::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_Z80CTC_TRIG_0 || 
	   id == SIG_Z80CTC_TRIG_1 || 
	   id == SIG_Z80CTC_TRIG_2 || 
	   id == SIG_Z80CTC_TRIG_3) {
		int c = (id == SIG_Z80CTC_TRIG_0) ? 0 : (id == SIG_Z80CTC_TRIG_1) ? 1 : (id == SIG_Z80CTC_TRIG_2) ? 2 : 3;
		if(counter[c].control & 0x40) {
			// counter mode
			counter[c].count -= (int)(data & mask);
			while(counter[c].count <= 0) {
				// reach zero
				if((counter[c].control & 0x80) && intr)
					intr->request_int(pri + c, counter[c].vector, true);
				counter[c].count += counter[c].constant;
				// output signal
				for(int i = 0; i < zc_cnt[c]; i++)
					zc[c][i]->write_signal(zc_id[c][i], 1, 0xffffffff);
			}
		}
		else
			counter[c].start = true;
	}
	else if(id == SIG_Z80CTC_CLOCK) {
		// input cpu clock
		for(int c = 0; c < 4; c++) {
			if(!(counter[c].control & 0x40) && counter[c].start) {
				// timer mode, timer start
				counter[c].clock -= (int)(data & mask);
				while(counter[c].clock < 0) {
					counter[c].clock += counter[c].prescaler;
					
					// count down
					if(counter[c].count > 0) {
						// decliment counter
						if(--counter[c].count == 0) {
							// reach zero
							if((counter[c].control & 0x80) && intr)
								intr->request_int(pri + c, counter[c].vector, true);
							counter[c].count = counter[c].constant;
							// output signal
							for(int i = 0; i < zc_cnt[c]; i++)
								zc[c][i]->write_signal(zc_id[c][i], 1, 0xffffffff);
						}
					}
					else
						counter[c].count = counter[c].constant;
				}
			}
		}
	}
}

void Z80CTC::set_event(int clock)
{
	eventclock = clock;
	int id;
	vm->regist_event_by_clock(this, 0, clock, true, &id);
}
