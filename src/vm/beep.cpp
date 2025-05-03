/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.22 -

	[ beep ]
*/

#include "beep.h"

#define DELAY_FRAMES	3

void BEEP::reset()
{
	signal = true;
	pulse = prv = lines = 0;
	change = 0;
	
	on = mute = false;
}

void BEEP::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_BEEP_ON) {
		bool next = ((data & mask) != 0);
		if(!on && next && !mute) {
			count = 0;
			pulse = lines = 0;
			change = constant ? 2 : 0;
		}
		on = next;
	}
	else if(id == SIG_BEEP_MUTE) {
		bool next = ((data & mask) != 0);
		if(mute && !next && on) {
			count = 0;
			pulse = lines = 0;
			change = constant ? 2 : 0;
		}
		mute = next;
	}
	else if(id == SIG_BEEP_PULSE)
		pulse += (data & mask);
	else if(id == SIG_BEEP_FREQ) {
		int freq = data & mask;
		diff = (int)(32.0 * gen_rate / (freq ? freq : 1) / 2.0 + 0.5);
	}
}

#define mydiff(p, q) ((p) > (q) ? (p) - (q) : (q) - (p))

void BEEP::event_vsync(int v, int clock)
{
	if(++lines == LINES_PER_FRAME * DELAY_FRAMES) {
		if(change == 1) {
#ifndef DONT_KEEP_BEEP_FREQ
			if(mydiff(pulse, prv) > 4)
#endif
				prv = pulse;
			if(prv) {
				diff = constant / prv;
				diff >>= 8;
			}
		}
		if(change)
			change--;
		pulse = lines = 0;
	}
}

void BEEP::mix(int32* buffer, int cnt)
{
	if(on && !mute && !change && diff >= 32) {
		for(int i = 0; i < cnt; i++) {
			if((count -= 32) < 0) {
				count += diff;
				signal = !signal;
			}
			buffer[i] += signal ? gen_vol : -gen_vol;
		}
	}
}

void BEEP::init(int rate, int frequency, int divide, int volume)
{
	if(frequency != -1) {
//		diff = (int)(32.0 * rate / frequency / 2.0 + 0.5);	// constant frequency
		diff = 32 * (int)(rate / frequency / 2.0 + 0.5);	// constant frequency
		constant = 0;
	}
	else {
/*
		frequency = pulse * FRAMES_PER_SEC / DELAY_FRAMES
		diff = 32 * rate / frequency / 2
		     = 16 * rate / pulse / FRAMES_PER_SEC * DELAY_FRAMES
		     = constant / pulse
		constant = 16 * rate / FRAMES_PER_SEC * DELAY_FRAMES
*/
		constant = (long)((256.0 * 16.0 * DELAY_FRAMES * rate * divide) / FRAMES_PER_SEC + 0.5);
		vm->regist_vsync_event(this);
	}
	gen_rate = rate;
	gen_vol = volume;
}

