/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.05.03-

	[ MSM5832 ]
*/

#include "msm5832.h"

#define YEAR		time[0]
#define MONTH		time[1]
#define DAY		time[2]
#define DAY_OF_WEEK	time[3]
#define HOUR		time[4]
#define MINUTE		time[5]
#define SECOND		time[6]

#define EVENT_1024HZ	0
#define EVENT_3600HZ	1
#define EVENT_PULSE_OFF	2

#define BIT_1024HZ	1
#define BIT_1HZ		2
#define BIT_60HZ	4
#define BIT_3600HZ	8

void MSM5832::initialize()
{
	memset(regs, 0, sizeof(regs));
	regs[15] = 0x0f;
	wreg = regnum = 0;
	cs = true;
	hold = rd = wr = addr_wr = false;
	cnt1 = cnt2 = 0;
	
	// register event
	register_event_by_clock(this, EVENT_1024HZ, CPU_CLOCKS / 2048, true, NULL);
	register_event_by_clock(this, EVENT_3600HZ, CPU_CLOCKS / 3600, true, NULL);
}

void MSM5832::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0x0f) {
	case 5:
		regs[5] = (regs[5] & 7) | (data & 8);
		break;
	case 8:
		regs[8] = (regs[8] & 3) | (data & 4);
		break;
	}
}

uint32 MSM5832::read_io8(uint32 addr)
{
	return regs[addr & 0x0f] | 0xf0;
}

void MSM5832::event_callback(int event_id, int err)
{
	if(event_id == EVENT_1024HZ) {
		if(!hold) {
			regs[15] ^= BIT_1024HZ;
			if(regnum == 15) {
				output();
			}
		}
	}
	else if(event_id == EVENT_3600HZ) {
		if(!hold) {
			// 3600Hz
			regs[15] |= BIT_3600HZ;
			// 60Hz
			if(!cnt1) {
				regs[15] &= ~BIT_60HZ;
			}
			// 1Hz
			if(!cnt2) {
				regs[15] &= ~BIT_1HZ;
				write_signals(&outputs_busy, 0xffffffff);
				
				// update clock
				emu->get_timer(time);
				int hour = (regs[5] & 8) ? HOUR : (HOUR % 12);
				int ampm = (HOUR > 11) ? 4 : 0;
				
				regs[ 0] = SECOND % 10;
				regs[ 1] = (uint8)(SECOND / 10);
				regs[ 2] = MINUTE % 10;
				regs[ 3] = (uint8)(MINUTE / 10);
				regs[ 4] = hour % 10;
				regs[ 5] = (uint8)(hour / 10) | ampm | (regs[5] & 8);
				regs[ 6] = DAY_OF_WEEK;
				regs[ 7] = DAY % 10;
				regs[ 8] = (uint8)(DAY / 10) | (regs[8] & 4);
				regs[ 9] = MONTH % 10;
				regs[10] = (uint8)(MONTH / 10);
				regs[11] = YEAR % 10;
				regs[12] = (uint8)((YEAR % 100) / 10);
			}
			if(regnum == 15) {
				output();
			}
			// register event
			register_event_by_clock(this, EVENT_PULSE_OFF, CPU_CLOCKS / 8192, false, NULL);
		}
		if(++cnt1 == 60) {
			cnt1 = 0;
		}
		if(++cnt2 == 3600) {
			cnt2 = 0;
			write_signals(&outputs_busy, 0);
		}
	}
	else if(event_id == EVENT_PULSE_OFF) {
		// 122.1usec (1/8192sec)
		if(!hold) {
			regs[15] |= (BIT_1HZ | BIT_60HZ);
			regs[15] &= ~BIT_3600HZ;
			if(regnum == 15) {
				output();
			}
		}
	}
}

void MSM5832::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_MSM5832_DATA) {
		wreg = (data & mask) | (wreg & ~mask);
	}
	else if(id == SIG_MSM5832_ADDR) {
		regnum = (data & mask) | (regnum & ~mask);
		output();
	}
	else if(id == SIG_MSM5832_CS) {
		cs = ((data & mask) != 0);
		output();
	}
	else if(id == SIG_MSM5832_HOLD) {
		hold = ((data & mask) != 0);
	}
	else if(id == SIG_MSM5832_READ) {
		rd = ((data & mask) != 0);
		output();
	}
	else if(id == SIG_MSM5832_WRITE) {
		bool next = ((data & mask) != 0);
		if(wr && !next && cs) {
			if(regnum == 5) {
				regs[5] = (regs[5] & 7) | (wreg & 8);
			}
			else if(regnum == 8) {
				regs[8] = (regs[8] & 3) | (wreg & 4);
			}
		}
		wr = next;
	}
	else if(id == SIG_MSM5832_ADDR_WRITE) {
		bool next = ((data & mask) != 0);
		if(addr_wr && !next && cs) {
			regnum = wreg;
			output();
		}
		addr_wr = next;
	}
}

void MSM5832::output()
{
	if(rd & cs) {
		uint8 rreg = regs[regnum & 0x0f];
		write_signals(&outputs_data, rreg);
	}
}
