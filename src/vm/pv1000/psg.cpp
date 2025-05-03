/*
	CASIO PV-1000 Emulator 'ePV-1000'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.16 -

	[ psg ]
*/

#include <math.h>
#include "psg.h"

void PSG::reset()
{
}

void PSG::write_io8(uint32 addr, uint32 data)
{
}

void PSG::init(int rate)
{
}

void PSG::mix(int32* buffer, int cnt)
{
	// create sound buffer
	for(int i = 0; i < cnt; i++) {
		int vol = 0;
		buffer[i] = vol;
	}
}
