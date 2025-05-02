/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.18-

	[ RP-5C15 ]
*/

#include "rp5c15.h"

#define EVENT_1HZ	0
#define EVENT_16HZ	1

#define YEAR		time[0]
#define MONTH		time[1]
#define DAY		time[2]
#define DAY_OF_WEEK	time[3]
#define HOUR		time[4]
#define MINUTE		time[5]
#define SECOND		time[6]

void RP5C15::initialize()
{
	_memset(regs, 0, sizeof(regs));
	regs[0x0a] = 1;
	regs[0x0d] = 8;
	regs[0x0f] = 0xc;
	alarm = pulse_1hz = pulse_16hz = false;
	
	int regist_id;
	vm->regist_event(this, EVENT_1HZ, 500000, true, &regist_id);
	vm->regist_event(this, EVENT_16HZ, 31250, true, &regist_id);
	vm->regist_frame_event(this);
}

void RP5C15::write_io8(uint32 addr, uint32 data)
{
	int ch = addr & 0x0f;
	
	switch(ch) {
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0c:
		if(regs[0xd] & 1) {
			regs[ch] = data;
		}
		break;
	case 0x0d:
	case 0x0e:
	case 0x0f:
		regs[ch] = data;
		break;
	}
}

uint32 RP5C15::read_io8(uint32 addr)
{
	int ch = addr & 0x0f;
	
	switch(ch) {
	case 0x00:
		return (regs[0xd] & 1) ? regs[ch] : SECOND % 10;
	case 0x01:
		return (regs[0xd] & 1) ? regs[ch] : (uint8)(SECOND / 10);
	case 0x02:
		return (regs[0xd] & 1) ? regs[ch] : MINUTE % 10;
	case 0x03:
		return (regs[0xd] & 1) ? regs[ch] : (uint8)(MINUTE / 10);
	case 0x04:
		return (regs[0xd] & 1) ? regs[ch] : ((regs[0xa] & 1) ? HOUR : HOUR % 12) % 10;
	case 0x05:
		return (regs[0xd] & 1) ? regs[ch] : (uint8)((regs[0xa] & 1) ? HOUR / 10 : ((HOUR % 12) / 10) | (HOUR < 12 ? 0 : 2));
	case 0x06:
		return (regs[0xd] & 1) ? regs[ch] : DAY_OF_WEEK;
	case 0x07:
		return (regs[0xd] & 1) ? regs[ch] : DAY % 10;
	case 0x08:
		return (regs[0xd] & 1) ? regs[ch] : (uint8)(DAY / 10);
	case 0x09:
		return (regs[0xd] & 1) ? regs[ch] : MONTH % 10;
	case 0x0a:
		return (regs[0xd] & 1) ? regs[ch] : (uint8)(MONTH / 10);
	case 0x0b:
		if(regs[0xd] & 1) {
			if(((YEAR - 0) % 4) == 0 && (((YEAR - 0) % 100) != 0 || ((YEAR - 0) % 400) == 0)) {
				return 0;
			}
			else if(((YEAR - 1) % 4) == 0 && (((YEAR - 1) % 100) != 0 || ((YEAR - 1) % 400) == 0)) {
				return 1;
			}
			else if(((YEAR - 2) % 4) == 0 && (((YEAR - 2) % 100) != 0 || ((YEAR - 2) % 400) == 0)) {
				return 2;
			}
			else {
				return 3;
			}
		}
		else {
			return YEAR % 10;
		}
	case 0x0c:
		return (regs[0xd] & 1) ? regs[ch] : (uint8)((YEAR % 100) / 10);
	case 0x0d:
	case 0x0e:
	case 0x0f:
		return regs[ch];
	}
	return 0xff;
}

void RP5C15::event_callback(int event_id, int err)
{
	if(event_id == EVENT_1HZ) {
		pulse_1hz = !pulse_1hz;
	}
	else {
		pulse_16hz = !pulse_16hz;
	}
	bool pulse = alarm;
	pulse |= (regs[0x0f] & 8) ? false : pulse_1hz;
	pulse |= (regs[0x0f] & 4) ? false : pulse_16hz;
	
	write_signals(&outputs_pulse, pulse ? 0 : 0xffffffff);
}

void RP5C15::event_frame()
{
	// update clock
	emu->get_timer(time);
	
	int minute, hour, day;
	minute = (regs[2] & 0x0f) + (regs[3] & 7) * 10;
	if(regs[0x0a] & 1) {
		hour = (regs[4] & 0x0f) + (regs[5] & 3) * 10;
	}
	else {
		hour = (regs[4] & 0x0f) + (regs[5] & 1) * 10 + (regs[5] & 2 ? 12 : 0);
	}
	day = (regs[7] & 0x0f) + (regs[8] & 3) * 10;
	bool newval = ((regs[0x0d] & 4) && minute == MINUTE && hour == HOUR && day == DAY) ? true : false;
	
	if(alarm != newval) {
		write_signals(&outputs_alarm, newval ? 0 : 0xffffffff);
		alarm = newval;
	}
}

