/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ Z80CTC ]
*/

#include "z80ctc.h"

void Z80CTC::reset()
{
	for(int ch = 0; ch < 4; ch++) {
		counter[ch].count = counter[ch].constant = 256;
		counter[ch].clocks = 0;
		counter[ch].control = 0;
		counter[ch].slope = false;
		counter[ch].prescaler = 256;
		counter[ch].freeze = counter[ch].start = counter[ch].latch = false;
		counter[ch].clock_id = counter[ch].sysclock_id = -1;
		// interrupt
		counter[ch].req_intr = false;
		counter[ch].in_service = false;
	}
	iei = oei = true;
	intr = false;
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
			counter[ch].latch = ((data & 4) != 0);
			counter[ch].freeze = ((data & 2) != 0);
			counter[ch].start = (counter[ch].freq || !(data & 8));
			counter[ch].control = data;
			counter[ch].slope = ((data & 0x10) != 0);
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
		uint32 input = sysclock * passed / CPU_CLOCKS;
		if(counter[ch].input <= input)
			input = counter[ch].input - 1;
		if(input > 0) {
			input_sysclock(ch, input);
			// cancel and re-regist event
			vm->cancel_event(counter[ch].sysclock_id);
			counter[ch].input -= input;
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
#if 1
	if(data & mask) {
		input_clock(ch, 1);
		update_event(ch, 0);
	}
#else
	// more correct implements...
	bool next = ((data & mask) != 0);
	if(counter[ch].prev_in != next) {
		if(counter[ch].slope == next) {
			input_clock(ch, 1);
			update_event(ch, 0);
		}
		counter[ch].prev_in = next;
	}
#endif
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
	while(counter[ch].count <= 0) {
		counter[ch].count += counter[ch].constant;
		if(counter[ch].control & 0x80) {
			counter[ch].req_intr = true;
			update_intr();
		}
		for(int i = 0; i < dcount_zc[ch]; i++) {
			d_zc[ch][i]->write_signal(did_zc[ch][i], 0xffffffff, dmask_zc[ch][i]);
			d_zc[ch][i]->write_signal(did_zc[ch][i], 0, dmask_zc[ch][i]);
		}
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
	while(counter[ch].count <= 0) {
		counter[ch].count += counter[ch].constant;
		if(counter[ch].control & 0x80) {
			counter[ch].req_intr = true;
			update_intr();
		}
		for(int i = 0; i < dcount_zc[ch]; i++) {
			d_zc[ch][i]->write_signal(did_zc[ch][i], 0xffffffff, dmask_zc[ch][i]);
			d_zc[ch][i]->write_signal(did_zc[ch][i], 0, dmask_zc[ch][i]);
		}
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
			counter[ch].period = CPU_CLOCKS * counter[ch].input / counter[ch].freq + err;
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
			counter[ch].period = CPU_CLOCKS * counter[ch].input / sysclock + err;
			counter[ch].prev = vm->current_clock() + err;
			vm->regist_event_by_clock(this, EVENT_TIMER + ch, counter[ch].period, false, &counter[ch].sysclock_id);
		}
	}
}

void Z80CTC::set_intr_iei(bool val)
{
	if(iei != val) {
		iei = val;
		update_intr();
	}
}

#define set_intr_oei(val) { \
	if(oei != val) { \
		oei = val; \
		if(d_child) \
			d_child->set_intr_iei(oei); \
	} \
}

void Z80CTC::update_intr()
{
	bool next;
	
	// set oei
	if(next = iei) {
		for(int ch = 0; ch < 4; ch++) {
			if(counter[ch].in_service) {
				next = false;
				break;
			}
		}
	}
	set_intr_oei(next);
	
	// set intr
	if(next = iei) {
		next = false;
		for(int ch = 0; ch < 4; ch++) {
			if(counter[ch].in_service)
				break;
			if(counter[ch].req_intr) {
				next = true;
				break;
			}
		}
	}
	if(next != intr) {
		intr = next;
		if(d_cpu)
			d_cpu->set_intr_line(intr, true, intr_bit);
	}
}

uint32 Z80CTC::intr_ack()
{
	// ack (M1=IORQ=L)
	if(intr) {
		for(int ch = 0; ch < 4; ch++) {
			if(counter[ch].in_service)
				break;
			if(counter[ch].req_intr) {
				counter[ch].req_intr = false;
				counter[ch].in_service = true;
				update_intr();
				return counter[ch].vector;
			}
		}
		// invalid interrupt status
//		emu->out_debug(_T("Z80CTC : intr_ack()\n"));
		return 0xff;
	}
	if(d_child)
		return d_child->intr_ack();
	return 0xff;
}

void Z80CTC::intr_reti()
{
	// detect RETI
	for(int ch = 0; ch < 4; ch++) {
		if(counter[ch].in_service) {
			counter[ch].in_service = false;
			update_intr();
			return;
		}
	}
	if(d_child)
		d_child->intr_reti();
}

