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
	memset(regs, 0, sizeof(regs));
	wreg = rreg = cmdreg = regnum = 0;
	busy = false;
	
	// register event
	register_frame_event(this);
}

void RTC58321::write_io8(uint32 addr, uint32 data)
{
	// for FMR-50
	if(addr & 1) {
		// command
		if(data & RTC58321_BIT_CS) {
			if(!(cmdreg & 4) && (data & 4)) {
				rreg = regs[regnum];
			}
			if((cmdreg & 2) && !(data & 2)) {
				if(regnum == 5) {
					regs[5] = (regs[5] & 7) | (wreg & 8);
				}
				else if(regnum == 8) {
					regs[8] = (regs[8] & 3) | (wreg & 0x0c);
				}
			}
			if((cmdreg & 1) && !(data & 1)) {
				regnum = wreg & 0x0f;
			}
		}
		cmdreg = data;
	}
	else {
		wreg = data;
	}
}

uint32 RTC58321::read_io8(uint32 addr)
{
	// for FMR-50
	if(addr & 1) {
		return cmdreg;
	}
	else {
		return rreg | (busy ? 0 : RTC58321_BIT_READY);
	}
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
	regs[ 8] = (uint8)(DAY / 10) | (regs[8] & 0x0c);
	regs[ 9] = MONTH % 10;
	regs[10] = (uint8)(MONTH / 10);
	regs[11] = YEAR % 10;
	regs[12] = (uint8)((YEAR % 100) / 10);
}

void RTC58321::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_RTC58321_DATA) {
		wreg = data;
	}
	else if(id == SIG_RTC58321_SELECT) {
		if(data & mask) {
			regnum = wreg & 0x0f;
		}
	}
	else if(id == SIG_RTC58321_WRITE) {
		if(data & mask) {
			if(regnum == 5) {
				regs[5] = (regs[5] & 7) | (wreg & 8);
			}
			else if(regnum == 8) {
				regs[8] = (regs[8] & 3) | (wreg & 0x0c);
			}
		}
	}
	else if(id == SIG_RTC58321_READ) {
		if(data & mask) {
			rreg = regs[regnum];
			write_signals(&outputs_data, rreg);
		}
	}
}

void RTC58321::set_busy(bool val)
{
	if(busy != val) {
		write_signals(&outputs_busy, val ? 0xffffffff : 0);
	}
	busy = val;
}

