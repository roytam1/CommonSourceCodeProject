/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.22 -

	[ beep ]
*/

#include "beep.h"

void BEEP::initialize()
{
	count = 0;
	diff = 16;
	signal = true;
	on = false;
	mute = false;
}

void BEEP::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_BEEP_ON) {
		bool next = (data & mask) ? true : false;
		if(!on && next)
			count = 0;
		on = next;
	}
	else if(id == SIG_BEEP_PULSE)
		pulse += (data & mask);
	else if(id == SIG_BEEP_MUTE)
		mute = (data & mask) ? true : false;
}

#define myabs(v) ((v) < 0 ? -(v) : (v))

void BEEP::event_frame()
{
	if(myabs(pulse - prv) > 3)
		prv = pulse;
	if(prv) {
		diff = constant / prv;
		diff >>= 8;
	}
	pulse = 0;
}

void BEEP::mix(int32* buffer, int cnt)
{
	if(mute || !on)
		return;
	for(int i = 0; i < cnt; i++) {
		if((count -= 32) < 0) {
			count += diff;
			signal = !signal;
		}
		buffer[i] += signal ? vol : -vol;
	}
}

void BEEP::init(int rate, int frequency, int divide, int volume)
{
	if(frequency != -1)
		diff = 32 * rate / frequency / 2;
	else {
		constant = (int)(32768.0 * rate / FRAMES_PER_SEC * divide + 0.5);
		pulse = prv = 0;
		vm->regist_frame_event(this);
	}
	vol = volume;
}

