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
	
#ifdef PCM1BIT_HIGH_QUALITY
	sample_count = 0;
	prev_clock = 0;
	prev_vol = 0;
#endif
	update = 0;
	
	register_frame_event(this);
}

void PCM1BIT::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_PCM1BIT_SIGNAL) {
		bool next = ((data & mask) != 0);
		if(signal != next) {
#ifdef PCM1BIT_HIGH_QUALITY
			if(sample_count < 1024) {
				samples_signal[sample_count] = signal;
				samples_out[sample_count] = (on && !mute);
				samples_clock[sample_count] = current_clock();
				sample_count++;
			}
#endif
			// mute if signal is not changed in 2 frames
			update = 2;
			signal = next;
		}
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
	if(update && --update == 0) {
#ifdef PCM1BIT_HIGH_QUALITY
		prev_vol = 0;
#endif
	}
}

void PCM1BIT::mix(int32* buffer, int cnt)
{
#ifdef PCM1BIT_HIGH_QUALITY
	uint32 cur_clock = current_clock();
	if(update) {
		if(sample_count < 1024) {
			samples_signal[sample_count] = signal;
			samples_out[sample_count] = (on && !mute);
			samples_clock[sample_count] = cur_clock;
			sample_count++;
		}
		uint32 start_clock = prev_clock;
		int start_index = 0;
		for(int i = 0; i < cnt; i++) {
			uint32 end_clock = prev_clock + ((cur_clock - prev_clock) * (i + 1)) / cnt;
			int on_clocks = 0, off_clocks = 0;
			for(int s = start_index; s < sample_count; s++) {
				uint32 clock = samples_clock[s];
				if(clock <= end_clock) {
					if(samples_out[s]) {
						if(samples_signal[s]) {
							on_clocks += clock - start_clock;
						}
						else {
							off_clocks += clock - start_clock;
						}
					}
					start_clock = clock;
					start_index = s + 1;
				}
				else {
					if(samples_out[s]) {
						if(samples_signal[s]) {
							on_clocks += end_clock - start_clock;
						}
						else {
							off_clocks += end_clock - start_clock;
						}
					}
					start_clock = end_clock;
					start_index = s;
					break;
				}
			}
			int clocks = on_clocks + off_clocks;
			if(clocks) {
				prev_vol = max_vol * (on_clocks - off_clocks) / clocks;
			}
			*buffer++ += prev_vol; // L
			*buffer++ += prev_vol; // R
		}
	}
	prev_clock = cur_clock;
	sample_count = 0;
#else
	if(on && !mute && update) {
		int cur_vol = signal ? max_vol : -max_vol;
		for(int i = 0; i < cnt; i++) {
			*buffer++ += cur_vol; // L
			*buffer++ += cur_vol; // R
		}
	}
#endif
}

void PCM1BIT::init(int rate, int volume)
{
	max_vol = volume;
}

