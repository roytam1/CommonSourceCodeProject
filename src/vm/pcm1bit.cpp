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

void PCM1BIT::reset()
{
	gen_vol = 0;
}

void PCM1BIT::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_PCM1BIT_SIGNAL) {
		bool next = ((data & mask) != 0);
		if(signal != next) {
			update = 4;
		}
		signal = next;
	}
	else if(id == SIG_PCM1BIT_ON) {
		on = ((data & mask) != 0);
	}
	else if(id == SIG_PCM1BIT_MUTE) {
		mute = ((data & mask) != 0);
	}
}

void PCM1BIT::event_frame()
{
	if(update) {
		if(--update == 0) {
			gen_vol = 0;
		}
	}
}

void PCM1BIT::event_callback(int event_id, int err)
{
	if(count < 256) {
		samples[count++] = signal;
	}
}

void PCM1BIT::mix(int32* buffer, int cnt)
{
	if(on && !mute && update) {
#ifdef PCM1BIT_HIGH_QUALITY
		for(int i = 0; i < cnt; i++) {
			if((i < count) ? samples[i] : signal) {
				if((gen_vol += dif_vol) > max_vol) {
					gen_vol = max_vol;
				}
			}
			else {
				if((gen_vol -= dif_vol) < -max_vol) {
					gen_vol = -max_vol;
				}
			}
			buffer[i] += gen_vol;
		}
#else
		for(int i = 0; i < cnt; i++) {
			buffer[i] += signal ? max_vol : -max_vol;
		}
#endif
	}
	count = 0;
}

void PCM1BIT::init(int rate, int volume)
{
	// create gain
	max_vol = volume;
#ifdef PCM1BIT_HIGH_QUALITY
	dif_vol = max_vol >> 3;
	int id;
	vm->regist_event_by_clock(this, 0, CPU_CLOCKS / rate, true, &id);
#endif
}

