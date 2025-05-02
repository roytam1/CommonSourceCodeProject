/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.22 -

	[ beep ]
*/

#include "beep.h"

void BEEP::reset()
{
	signal = true;
	count = 0;
	on = mute = false;
}

void BEEP::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_BEEP_ON) {
		on = ((data & mask) != 0);
	}
	else if(id == SIG_BEEP_MUTE) {
		mute = ((data & mask) != 0);
	}
}

void BEEP::mix(int32* buffer, int cnt)
{
	if(on && !mute) {
		for(int i = 0; i < cnt; i++) {
			if((count -= 1024) < 0) {
				count += diff;
				signal = !signal;
			}
			*buffer++ += signal ? gen_vol : -gen_vol; // L
			*buffer++ += signal ? gen_vol : -gen_vol; // R
		}
	}
}

void BEEP::init(int rate, double frequency, int volume)
{
	gen_rate = rate;
	gen_vol = volume;
	set_frequency(frequency);
}

void BEEP::set_frequency(double frequency)
{
	diff = (int)(1024.0 * gen_rate / frequency / 2.0 + 0.5);
}

