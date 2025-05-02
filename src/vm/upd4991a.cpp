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
	ctrl1 = ctrl2 = mode = 0;
	
	// regist event
	vm->register_frame_event(this);
}

void UPD4991A::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0x0f) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		if(mode == 1) {
			tp1[addr & 0x0f] = data;
		}
		else if(mode == 2) {
			tp2[addr & 0x0f] = data;
		}
//		else {
//			cur[addr & 0x0f] = data;
//		}
		break;
	case 13:
		ctrl1 = data;
		break;
	case 14:
		ctrl2 = data;
		break;
	case 15:
		mode = data & 0x0b;
		break;
	}
}

uint32 UPD4991A::read_io8(uint32 addr)
{
	switch(addr & 0x0f) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		if(mode == 1) {
			return tp1[addr & 0x0f];
		}
		else if(mode == 2) {
			return tp2[addr & 0x0f];
		}
		else {
			return cur[addr & 0x0f];
		}
	case 14:
		return ctrl2;
	}
	return 0x0f;
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
		cur[12] = (uint8)((YEAR % 100) / 10);
	}
	
	// check alarm
}

