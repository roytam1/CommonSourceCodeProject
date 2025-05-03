/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.09-

	[ i8253 * 2chips ]
*/

#include "i8253.h"
#include "i8259.h"
#include "sound.h"

// clock #0 of 1	<- out #1 of 2
// clock #1 of 1	<- out #1 of 2
// clock #2 of 1	<- 1.9968MHz
// clock #0 of 2	<- 1.9968MHz
// clock #1 of 2	<- 1.9968MHz
// clock #2 of 2	<- 1.9968MHz

// gate #0 of 1		<- memory bank
// gate #1 of 1		<- +5V
// gate #2 of 1		<- memory bank
// gate #0 of 2		<- +5V
// gate #1 of 2		<- +5V
// gate #2 of 2		<- +5V

// out #0 of 1		-> speaker timer
// out #1 of 1		-> IR5 of 8259 #1
// out #2 of 1		-> IR1 of 8259 #0
// out #0 of 2		-> speaker freq
// out #1 of 2		-> clock #0,#1 of 1
// out #2 of 2		-> rs232c

void I8253::initialize()
{
	for(int i = 0; i < 6; i++) {
		counter[i].count = 0x10000;
		counter[i].count_reg = 0;
		counter[i].ctrl_reg = 0x34;
		counter[i].mode = 3;
		counter[i].mode0_flag = false;
		counter[i].r_cnt = 0;
		counter[i].delay = 0;
		counter[i].gate = true;
		counter[i].start = false;
	}
	clocks = 0;
}

#define COUNT_VALUE(n) ((!counter[n].count_reg) ? 0x10000 : (counter[n].mode == 3 && counter[n].count_reg == 1) ? 0x10001 : counter[n].count_reg)

void I8253::write_io8(uint16 addr, uint8 data)
{
	switch(addr & 0xff)
	{
		case 0x00: // count reg #0 of 1
		case 0x01: // count reg #1 of 1
		case 0x02: // count reg #2 of 1
		case 0x04: // count reg #0 of 1
		case 0x05: // count reg #1 of 1
		case 0x06: // count reg #2 of 1
		{
			int ch = (addr & 3) + (addr & 4 ? 3 : 0);
			if((counter[ch].ctrl_reg & 0x30) == 0x10)
				counter[ch].count_reg = data;
			else if((counter[ch].ctrl_reg & 0x30) == 0x20)
				counter[ch].count_reg = data << 8;
			else
				counter[ch].count_reg = ((counter[ch].count_reg & 0xff00) >> 8) | (data << 8);
			// start count ?
			if(counter[ch].mode == 1 || counter[ch].mode == 5)
				counter[ch].start = false;
			else
				counter[ch].start = counter[ch].gate;
			// copy to counter at 1st clock, start count at 2nd clock
			counter[ch].delay = 1;
			counter[ch].mode0_flag = false;

			// beep frequency
			if(ch == 3)
				vm->sound->set_ctc(2000000 / (counter[3].count_reg + 1));
			break;
		}
		case 0x03: // ctrl reg of 1
		case 0x07: // ctrl reg of 2
		{
			if((data & 0xc0) == 0xc0)
				break;
			uint8 ch = ((data >> 6) & 3) + (addr & 4 ? 3 : 0);
			
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
}

uint8 I8253::read_io8(uint16 addr)
{
	switch(addr & 0xff)
	{
		case 0x00: // count reg #0 of 1
		case 0x01: // count reg #1 of 1
		case 0x02: // count reg #2 of 1
		case 0x04: // count reg #0 of 1
		case 0x05: // count reg #1 of 1
		case 0x06: // count reg #2 of 1
		{
			int ch = (addr & 3) + (addr & 4 ? 3 : 0);
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
			uint8 val = (uint8)(counter[ch].latch & 0xff);
			counter[ch].latch >>= 8;
			counter[ch].r_cnt--;
			return val;
		}
	}
	return 0xff;
}

void I8253::input_gate(int ch, bool signal)
{
	if(counter[ch].mode == 1 || counter[ch].mode == 5) {
		// 立ち上がりで開始、開始後はターミナルカウントまで停止しない
		if(!counter[ch].gate && signal)
			counter[ch].start = true;
	}
	else
		counter[ch].start = signal;
	counter[ch].gate = signal;
}

void I8253::input_clock(int clock)
{
	// CPUのクロックは3.9936MHz
	// これを2分周した1.9968MHzがch.2-5に入力される
	clocks += clock;
	int out2 = update_clock(2, clocks >> 1);
	int out3 = update_clock(3, clocks >> 1);
	int out4 = update_clock(4, clocks >> 1);
	int out5 = update_clock(5, clocks >> 1);
	int out0 = update_clock(0, out4);
	int out1 = update_clock(1, out4);
	clocks &= 1;
	
	// out #0 of 1		-> speaker timer
	// out #1 of 1		-> IR5 of 8259 #1
	// out #2 of 1		-> IR1 of 8259 #0
	// out #0 of 2		-> speaker freq
	// out #1 of 2		-> clock #0,#1 of 1
	// out #2 of 2		-> rs232c
	vm->sound->beep_timer = counter[0].start;
	if(out1)
		vm->pic->request_int(5+8, true);
	if(out2)
		vm->pic->request_int(1+0, true);
}

int I8253::update_clock(int ch, int clock)
{
	if(!counter[ch].start)
		return 0;
	
	if(counter[ch].delay & clock) {
		clock -= counter[ch].delay;
		counter[ch].delay = 0;
		counter[ch].count = COUNT_VALUE(ch);
	}
	
	int carry = 0;
	counter[ch].count -= clock;
	while(counter[ch].count <= 0) {
		if(counter[ch].mode == 0 || counter[ch].mode == 2 || counter[ch].mode == 3) {
			counter[ch].count += COUNT_VALUE(ch);
			carry++;
		}
		else {
			counter[ch].count = 0;
			counter[ch].start = false;
			return carry;
		}
//		if(!(counter[ch].mode == 0 && counter[ch].mode0_flag))
//			carry++;
		counter[ch].mode0_flag = true;
	}
	return carry;
}

