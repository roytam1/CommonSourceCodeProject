/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.10-

	[ uPD4991A ]
*/

#include "upd4991a.h"

#define YEAR		time[0]
#define MONTH		time[1]
#define DAY		time[2]
#define DAY_OF_WEEK	time[3]
#define HOUR		time[4]
#define MINUTE		time[5]
#define SECOND		time[6]

void UPD4991A::initialize()
{
	_memset(cur, 0, sizeof(cur));
	_memset(tp1, 0, sizeof(tp1));
	_memset(tp2, 0, sizeof(tp2));
	ch = ctrl1 = ctrl2 = mode = 0;
	
	// regist event
	vm->regist_frame_event(this);
}

void UPD4991A::write_io8(uint32 addr, uint32 data)
{
	data &= 0xf;
	
	if(addr & 1) {
		if(ch < 13) {
			if(mode == 1)
				tp1[ch] = data;
			else if(mode == 2)
				tp2[ch] = data;
//			else
//				cur[ch] = data;
		}
		else if(ch == 13)
			ctrl1 = data;
		else if(ch == 14)
			ctrl2 = data;
		else if(ch == 15)
			mode = data & 0xb;
	}
	else
		ch = data & 0xf;
}

uint32 UPD4991A::read_io8(uint32 addr)
{
	if(addr & 1) {
		if(ch < 13) {
			if(mode == 1)
				return tp1[ch];
			else if(mode == 2)
				return tp2[ch];
			else
				return cur[ch];
		}
		else if(ch == 14)
			return ctrl2;
	}
	return 0xff;
}

void UPD4991A::event_frame()
{
	// update clock
	if(!(ctrl1 & 8)) {
		emu->get_timer(time);
		uint8 day = (!(tp2[12] & 8)) ? (DAY % 12) : DAY;
		uint8 ampm = (!(tp2[12] & 8) && DAY >= 12) ? 4 : 0;
		
		cur[ 0] = SECOND % 10;
		cur[ 1] = (uint8)(SECOND / 10);
		cur[ 2] = MINUTE % 10;
		cur[ 3] = (uint8)(MINUTE / 10);
		cur[ 4] = HOUR % 10;
		cur[ 5] = (uint8)(HOUR / 10);
		cur[ 6] = DAY_OF_WEEK;
		cur[ 7] = day % 10;
		cur[ 8] = (uint8)(day / 10) | ampm;
		cur[ 9] = MONTH % 10;
		cur[10] = (uint8)(MONTH / 10);
		cur[11] = YEAR % 10;
		cur[12] = (uint8)(YEAR / 10);
	}
	
	// check alarm
}

