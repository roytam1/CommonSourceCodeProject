/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.02.09 -

	[ 1bit PCM ]
*/

#include "pcm1bit.h"

void PCM1BIT::initialize()
{
	signal = false;
	on = true;
	mute = false;
	update = count = 0;
	
	vm->regist_frame_event(this);
}

void PCM1BIT::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_PCM1BIT_SIGNAL) {
		bool next = ((data & mask) != 0);
		if(signal != next)
			update = 4;
		signal = next;
	}
	else if(id == SIG_PCM1BIT_ON)
		on = ((data & mask) != 0);
	else if(id == SIG_PCM1BIT_MUTE)
		mute = ((data & mask) != 0);
}

void PCM1BIT::event_frame()
{
	if(update)
		update--;
}

void PCM1BIT::event_callback(int event_id, int err)
{
	samples[count++ & 0xff] = signal;
}

void PCM1BIT::mix(int32* buffer, int cnt)
{
	if(on && !mute && update) {
#ifdef PCM1BIT_HIGH_QUALITY
		if(count > cnt) {
			for(int i = 0; i < cnt; i++)
				buffer[i] += samples[count - cnt + i] ? vol : -vol;
		}
		else {
			for(int i = 0; i < cnt; i++)
				buffer[i] += (i < count ? samples[i] : signal) ? vol : -vol;
		}
#else
		for(int i = 0; i < cnt; i++)
			buffer[i] += signal ? vol : -vol;
#endif
	}
#ifdef PCM1BIT_HIGH_QUALITY
	samples[0] = signal;
	count = 1;
#else
	count = 0;
#endif
}

void PCM1BIT::init(int rate, int volume)
{
	// create gain
	vol = volume;
#ifdef PCM1BIT_HIGH_QUALITY
	int id;
	vm->regist_event_by_clock(this, 0, CPU_CLOCKS / rate, true, &id);
#endif
}

