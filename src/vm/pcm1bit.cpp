/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.02.09 -

	[ 1bit PCM ]
*/

#include "pcm1bit.h"

void PCM1BIT::initialize()
{
	signal = mute = false;
	update = 0;
	vm->regist_frame_event(this);
}

void PCM1BIT::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_PCM1BIT_SIGNAL) {
		bool next = (data & mask) ? true : false;
		if(signal != next)
			update = 4;
		signal = next;
	}
	else if(id == SIG_PCM1BIT_MUTE)
		mute = (data & mask) ? true : false;
}

void PCM1BIT::event_frame()
{
	if(update)
		update--;
}

void PCM1BIT::mix(int32* buffer, int cnt)
{
	if(!mute && update) {
		for(int i = 0; i < cnt; i++)
			buffer[i] += signal ? vol : -vol;
	}
}

void PCM1BIT::init(int volume)
{
	// create gain
	vol = volume;
}

