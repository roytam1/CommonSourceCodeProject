/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.06.01-

	[ i8253 ]
*/

#include "i8253.h"

void I8253::initialize()
{
	for(int i = 0; i < 3; i++) {
		counter[i].count = 0x10000;
		counter[i].count_reg = 0;
		counter[i].ctrl_reg = 0x34;
		counter[i].mode = 3;
		counter[i].mode0_flag = false;
		counter[i].r_cnt = 0;
		counter[i].delay = 0;
		counter[i].start = false;
	}
}

#define COUNT_VALUE(n) ((!counter[n].count_reg) ? 0x10000 : (counter[n].mode == 3 && counter[n].count_reg == 1) ? 0x10001 : counter[n].count_reg)

void I8253::write_io8(uint32 addr, uint32 data)
{
	int ch = addr & 3;
	
	switch(ch)
	{
	case 0:
	case 1:
	case 2:
		if((counter[ch].ctrl_reg & 0x30) == 0x10)
			counter[ch].count_reg = data;
		else if((counter[ch].ctrl_reg & 0x30) == 0x20)
			counter[ch].count_reg = data << 8;
		else
			counter[ch].count_reg = ((counter[ch].count_reg & 0xff00) >> 8) | (data << 8);
		// start count now if mode is not 1 or 5
		if(!(counter[ch].mode == 1 || counter[ch].mode == 5))
			counter[ch].start = true;
		// copy to counter at 1st clock, start count at 2nd clock
		counter[ch].delay = 1;
		counter[ch].mode0_flag = false;
		break;
		
	case 3: // ctrl reg
		if((data & 0xc0) == 0xc0)
			break;
		ch = (data >> 6) & 3;
		
		if((data & 0x30) == 0x00) {
			// counter latch
			if((counter[ch].ctrl_reg & 0x30) == 0x10) {
				// lower byte
				counter[ch].latch = (uint16)counter[ch].count;
				counter[ch].r_cnt = 1;
			}
			else if((counter[ch].ctrl_reg & 0x30) == 0x20) {
				// upper byte
				counter[ch].latch = (uint16)(counter[ch].count >> 8);
				counter[ch].r_cnt = 1;
			}
			else {
				// lower -> upper
				counter[ch].latch = (uint16)counter[ch].count;
				counter[ch].r_cnt = 2;
			}
		}
		else {
			counter[ch].ctrl_reg = data;
			counter[ch].mode = ((data & 0xe) == 0xc) ? 2 : ((data & 0xe) == 0xe) ? 3 : (data & 0xe) >> 1;
			counter[ch].count_reg = 0;
		}
		break;
	}
}

uint32 I8253::read_io8(uint32 addr)
{
	int ch = addr & 3;
	
	switch(ch)
	{
	case 0:
	case 1:
	case 2:
		if(!counter[ch].r_cnt) {
			// not latched (through current count)
			if((counter[ch].ctrl_reg & 0x30) == 0x10) {
				// lower byte
				counter[ch].latch = (uint16)counter[ch].count;
				counter[ch].r_cnt = 1;
			}
			else if((counter[ch].ctrl_reg & 0x30) == 0x20) {
				// upper byte
				counter[ch].latch = (uint16)(counter[ch].count >> 8);
				counter[ch].r_cnt = 1;
			}
			else {
				// lower -> upper
				counter[ch].latch = (uint16)counter[ch].count;
				counter[ch].r_cnt = 2;
			}
		}
		uint32 val = counter[ch].latch & 0xff;
		counter[ch].latch >>= 8;
		counter[ch].r_cnt--;
		return val;
	}
	return 0xff;
}

void I8253::write_signal(int id, uint32 data, uint32 mask)
{
	switch(id)
	{
	case SIG_I8253_CLOCK_0:
		input_clock(0, data & mask);
		break;
	case SIG_I8253_CLOCK_1:
		input_clock(1, data & mask);
		break;
	case SIG_I8253_CLOCK_2:
		input_clock(2, data & mask);
		break;
	case SIG_I8253_GATE_0:
		input_gate(0, (data & mask) ? true : false);
		break;
	case SIG_I8253_GATE_1:
		input_gate(1, (data & mask) ? true : false);
		break;
	case SIG_I8253_GATE_2:
		input_gate(2, (data & mask) ? true : false);
		break;
	}
}

void I8253::input_clock(int ch, int clock)
{
	if(!counter[ch].start)
		return;
	if(counter[ch].delay & clock) {
		clock -= counter[ch].delay;
		counter[ch].delay = 0;
		counter[ch].count = COUNT_VALUE(ch);
	}
	
	int carry = 0;
	counter[ch].count -= clock;
	while(counter[ch].count <= 0) {
		if(counter[ch].mode == 0 || counter[ch].mode == 2 || counter[ch].mode == 3)
			counter[ch].count += COUNT_VALUE(ch);
		else {
			counter[ch].count = 0;
			counter[ch].start = false;
		}
		if(!(counter[ch].mode == 0 && counter[ch].mode0_flag))
			carry++;
		counter[ch].mode0_flag = true;
	}
	for(int i = 0; i < carry; i++) {
		for(int j = 0; j < dev_cnt[ch]; j++)
			dev[ch][j]->write_signal(dev_id[ch][j], 1, 0xffffffff);
	}
}

void I8253::input_gate(int ch, bool signal)
{
	if(!(counter[ch].mode == 0 || counter[ch].mode == 4))
		counter[ch].count = COUNT_VALUE(ch);
	counter[ch].start = true;
	counter[ch].mode0_flag = false;
}

