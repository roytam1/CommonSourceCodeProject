/*
	CASIO PV-1000 Emulator 'ePV-1000'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.16 -

	[ sound ]
*/

#include <math.h>
#include "sound.h"

void SOUND::reset()
{
}

void SOUND::write_io8(uint32 addr, uint32 data)
{
}

void SOUND::init(int rate)
{
}

void SOUND::mix(int32* buffer, int cnt)
{
	// create sound buffer
	for(int i = 0; i < cnt; i++) {
		int vol = 0;
		buffer[i] = vol;
	}
}
