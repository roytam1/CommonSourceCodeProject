/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ sound ]
*/

#include "sound.h"
#include "i8255.h"

void SOUND::initialize()
{
	// initialize
	update_usec = (int)(1000000. / FRAMES_PER_SEC / LINES_PER_FRAME + 0.5);
	sound_buffer = NULL;
	buffer_ptr = 0;
	accum_samples = 0;
	
	beep_cont = beep_timer = false;
}

void SOUND::release()
{
	if(sound_buffer)
		free(sound_buffer);
}

void SOUND::reset()
{
	// reset
	if(sound_buffer)
		_memset(sound_buffer, 0, sound_samples * 2);
	buffer_ptr = 0;
}

void SOUND::initialize_sound(int rate, int samples)
{
	if(sound_buffer)
		free(sound_buffer);
	sound_buffer = NULL;
	
	sound_samples = samples;
	update_samples = (int)(1024. * rate / FRAMES_PER_SEC / LINES_PER_FRAME + 0.5);
	
	sound_buffer = (uint16*)malloc(samples * sizeof(uint16));
	_memset(sound_buffer, 0, samples * 2);
	buffer_ptr = 0;
	
	// initialize beep
	sound_rate = rate;
	dif = sound_rate / 1000 / 2;	// set 1khz
	count = 0;
	signal = false;
}

void SOUND::update_sound()
{
	accum_samples += update_samples;
	int samples = accum_samples >> 10;
	accum_samples -= samples << 10;
	create_sound(samples, false);
}

uint16* SOUND::create_sound(int samples, bool fill)
{
	// get samples to be created
	int cnt = 0;
	if(fill)
		cnt = sound_samples - buffer_ptr;
	else
		cnt = (sound_samples - buffer_ptr < samples) ? sound_samples - buffer_ptr : samples;
	
	// create sound buffer
	if(beep_cont || beep_timer) {
		for(int i = buffer_ptr, j = 0; j < cnt; i++, j++) {
			if(--count < 0) {
				count += dif;
				signal = !signal;
			}
			sound_buffer[i] = signal ? VOLUME : -VOLUME;
		}
	}
	else
		_memset(&sound_buffer[buffer_ptr], 0, sizeof(uint16) * cnt);
	
	buffer_ptr = fill ? 0 : (buffer_ptr + cnt);
	return sound_buffer;
}

