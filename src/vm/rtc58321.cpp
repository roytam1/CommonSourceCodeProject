/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.05.02-

	[ RTC58321 ]
*/

#include "rtc58321.h"

#define YEAR		time[0]
#define MONTH		time[1]
#define DAY		time[2]
#define DAY_OF_WEEK	time[3]
#define HOUR		time[4]
#define MINUTE		time[5]
#define SECOND		time[6]

// for FMR-50
#ifndef RTC58321_BIT_CS
#define RTC58321_BIT_CS		0x80
#endif
#ifndef RTC58321_BIT_READY
#define RTC58321_BIT_READY	0x80
#endif

void RTC58321::initialize()
{
	_memset(regs, 0, sizeof(regs));
	wreg = rreg = cmdreg = regnum = 0;
	busy = false;
	
	// regist event
	vm->regist_frame_event(this);
}

void RTC58321::write_io8(uint32 addr, uint32 data)
{
	// for FMR-50
	if(addr & 1) {
		// command
		if(data & RTC58321_BIT_CS) {
			if(!(cmdreg & 4) && (data & 4))
				rreg = regs[regnum];
			if((cmdreg & 2) && !(data & 2)) {
				if(regnum == 5)
					regs[5] = (regs[5] & 7) | (wreg & 8);
				else if(regnum == 8)
					regs[8] = (regs[8] & 3) | (wreg & 0xc);
			}
			if((cmdreg & 1) && !(data & 1))
				regnum = wreg & 0xf;
		}
		cmdreg = data;
	}
	else
		wreg = data;
}

uint32 RTC58321::read_io8(uint32 addr)
{
	// for FMR-50
	if(addr & 1)
		return cmdreg;
	else
		return rreg | (busy ? 0 : RTC58321_BIT_READY);
}

void RTC58321::event_frame()
{
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
	regs[ 8] = (uint8)(DAY / 10) | (regs[8] & 0xc);
	regs[ 9] = MONTH % 10;
	regs[10] = (uint8)(MONTH / 10);
	regs[11] = YEAR % 10;
	regs[12] = (uint8)((YEAR % 100) / 10);
}

void RTC58321::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_RTC58321_DATA)
		wreg = data;
	else if(id == SIG_RTC58321_SELECT) {
		if(data & mask)
			regnum = wreg & 0xf;
	}
	else if(id == SIG_RTC58321_WRITE) {
		if(data & mask) {
			if(regnum == 5)
				regs[5] = (regs[5] & 7) | (wreg & 8);
			else if(regnum == 8)
				regs[8] = (regs[8] & 3) | (wreg & 0xc);
		}
	}
	else if(id == SIG_RTC58321_READ) {
		if(data & mask) {
			rreg = regs[regnum];
			for(int i = 0; i < dcount_data; i++) {
				int shift = dshift_data[i];
				uint32 val = (shift < 0) ? (rreg >> (-shift)) : (rreg << shift);
				uint32 mask = (shift < 0) ? (dmask_data[i] >> (-shift)) : (dmask_data[i] << shift);
				d_data[i]->write_signal(did_data[i], val, mask);
			}
		}
	}
}

void RTC58321::set_busy(bool val)
{
	if(busy != val) {
		for(int i = 0; i < dcount_busy; i++)
			d_busy[i]->write_signal(did_busy[i], val ? 0xffffffff : 0, dmask_busy[i]);
	}
	busy = val;
}

