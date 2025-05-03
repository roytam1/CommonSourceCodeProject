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
	for(int ch = 0; ch < 4; ch++) {
		counter[ch].count = counter[ch].constant = 256;
		counter[ch].clocks = 0;
		counter[ch].control = 0;
		counter[ch].prescaler = 256;
		counter[ch].freeze = counter[ch].start = counter[ch].latch = false;
		counter[ch].clock_id = counter[ch].sysclock_id = -1;
	}
	tmp = 0;
}

void Z80CTC::write_io8(uint32 addr, uint32 data)
{
	int ch = addr & 3;
	if(counter[ch].latch) {
		// time constant
		counter[ch].constant = data ? data : 256;
		counter[ch].latch = false;
		if(counter[ch].control & 2) {
			counter[ch].count = counter[ch].constant;
			counter[ch].clocks = 0;
			counter[ch].freeze = false;
			
			if(counter[ch].clock_id != -1)
				vm->cancel_event(counter[ch].clock_id);
			if(counter[ch].sysclock_id != -1)
				vm->cancel_event(counter[ch].sysclock_id);
			counter[ch].clock_id = counter[ch].sysclock_id = -1;
			update_event(ch, 0);
		}
	}
	else {
		if(data & 1) {
			// control word
			counter[ch].prescaler = (data & 0x20) ? 256 : 16;
			counter[ch].latch = (data & 4) ? true : false;
//			counter[ch].freeze = ((data & 6) == 2) ? true : false;
			counter[ch].freeze = (data & 2) ? true : false;
			counter[ch].start = (counter[ch].freq || !(data & 8)) ? true : false;
			counter[ch].control = data;
			update_event(ch, 0);
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
	int ch = addr & 3;
	if(counter[ch].clock_id != -1) {
		int passed = vm->passed_clock(counter[ch].prev);
		uint32 input = counter[ch].freq * passed / CPU_CLOCKS;
		if(counter[ch].input <= input)
			input = counter[ch].input - 1;
		if(input > 0) {
			input_clock(ch, input);
			// cancel and re-regist event
			vm->cancel_event(counter[ch].clock_id);
			counter[ch].input -= input;
			counter[ch].period -= passed;
			counter[ch].prev = vm->current_clock();
			vm->regist_event_by_clock(this, EVENT_COUNTER + ch, counter[ch].period, false, &counter[ch].clock_id);
		}
	}
	else if(counter[ch].sysclock_id != -1) {
		int passed = vm->passed_clock(counter[ch].prev);
		uint32 input = passed;
		if(counter[ch].input <= input)
			input = counter[ch].input - 1;
		if(input > 0) {
			input_sysclock(ch, input);
			// cancel and re-regist event
			vm->cancel_event(counter[ch].sysclock_id);
			counter[ch].input -= passed;
			counter[ch].period -= passed;
			counter[ch].prev = vm->current_clock();
			vm->regist_event_by_clock(this, EVENT_TIMER + ch, counter[ch].period, false, &counter[ch].sysclock_id);
		}
	}
	return counter[ch].count & 0xff;
}

void Z80CTC::event_callback(int event_id, int err)
{
	int ch = event_id & 3;
	if(event_id & 4) {
		input_sysclock(ch, counter[ch].input);
		counter[ch].sysclock_id = -1;
	}
	else {
		input_clock(ch, counter[ch].input);
		counter[ch].clock_id = -1;
	}
	update_event(ch, err);
}

void Z80CTC::write_signal(int id, uint32 data, uint32 mask)
{
	int ch = id & 3;
	int clock = data & mask;
	input_clock(ch, clock);
	update_event(ch, 0);
}

void Z80CTC::input_clock(int ch, int clock)
{
	if(!(counter[ch].control & 0x40)) {
		counter[ch].start = true;
		return;
	}
	if(counter[ch].freeze)
		return;
	
	// update counter
	counter[ch].count -= clock;
	uint32 carry = 0;
	while(counter[ch].count <= 0) {
		counter[ch].count += counter[ch].constant;
		if((counter[ch].control & 0x80) && d_pic)
			d_pic->request_int(this, pri + ch, counter[ch].vector, true);
		carry++;
	}
	if(carry) {
		for(int i = 0; i < dcount_zc[ch]; i++)
			d_zc[ch][i]->write_signal(did_zc[ch][i], carry, 0xffffffff);
	}
}

void Z80CTC::input_sysclock(int ch, int clock)
{
	if(counter[ch].control & 0x40)
		return;
	if(!counter[ch].start || counter[ch].freeze)
		return;
	counter[ch].clocks += clock;
	int input = counter[ch].clocks >> (counter[ch].prescaler == 256 ? 8 : 4);
	counter[ch].clocks &= counter[ch].prescaler - 1;
	
	// update counter
	counter[ch].count -= input;
	uint32 carry = 0;
	while(counter[ch].count <= 0) {
		counter[ch].count += counter[ch].constant;
		if((counter[ch].control & 0x80) && d_pic)
			d_pic->request_int(this, pri + ch, counter[ch].vector, true);
		carry++;
	}
	if(carry) {
		for(int i = 0; i < dcount_zc[ch]; i++)
			d_zc[ch][i]->write_signal(did_zc[ch][i], carry, 0xffffffff);
	}
}

void Z80CTC::update_event(int ch, int err)
{
	if(counter[ch].control & 0x40) {
		// counter mode
		if(counter[ch].sysclock_id != -1)
			vm->cancel_event(counter[ch].sysclock_id);
		counter[ch].sysclock_id = -1;
		
		if(counter[ch].freeze) {
			if(counter[ch].clock_id != -1)
				vm->cancel_event(counter[ch].clock_id);
			counter[ch].clock_id = -1;
			return;
		}
		if(counter[ch].clock_id == -1 && counter[ch].freq) {
			counter[ch].input = counter[ch].count;
			counter[ch].period = CPU_CLOCKS / counter[ch].freq * counter[ch].input + err;
			counter[ch].prev = vm->current_clock() + err;
			vm->regist_event_by_clock(this, EVENT_COUNTER + ch, counter[ch].period, false, &counter[ch].clock_id);
		}
	}
	else {
		// timer mode
		if(counter[ch].clock_id != -1)
			vm->cancel_event(counter[ch].clock_id);
		counter[ch].clock_id = -1;
		
		if(!counter[ch].start || counter[ch].freeze) {
			if(counter[ch].sysclock_id != -1)
				vm->cancel_event(counter[ch].sysclock_id);
			counter[ch].sysclock_id = -1;
			return;
		}
		if(counter[ch].sysclock_id == -1) {
			counter[ch].input = counter[ch].count * counter[ch].prescaler - counter[ch].clocks;
			counter[ch].period = counter[ch].input + err;
			counter[ch].prev = vm->current_clock() + err;
			vm->regist_event_by_clock(this, EVENT_TIMER + ch, counter[ch].period, false, &counter[ch].sysclock_id);
		}
	}
}

