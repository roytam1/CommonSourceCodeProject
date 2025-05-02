/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.21 -

	[ timer ]
*/

#include "emu.h"

void EMU::update_timer()
{
	GetLocalTime(&sTime);
}

void EMU::get_timer(int time[])
{
/*
	0	year
	1	month
	2	day
	3	day of week
	4	hour
	5	minute
	6	second
	7	milli seconds
*/
	time[0] = sTime.wYear;
	time[1] = sTime.wMonth;
	time[2] = sTime.wDay;
	time[3] = sTime.wDayOfWeek;
	time[4] = sTime.wHour;
	time[5] = sTime.wMinute;
	time[6] = sTime.wSecond;
	time[7] = sTime.wMilliseconds;
}

