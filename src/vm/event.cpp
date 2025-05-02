/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.29-

	[ event manager ]
*/

#include "event.h"
#include "../config.h"

#ifndef EVENT_CONTINUOUS_SOUND
//#ifdef PCM1BIT_HIGH_QUALITY
#define EVENT_CONTINUOUS_SOUND
//#endif
#endif

void EVENT::initialize()
{
	// load config
	if(!(0 <= config.cpu_power && config.cpu_power <= 4)) {
		config.cpu_power = 0;
	}
	power = config.cpu_power;
	
	// initialize event
	for(int i = 0; i < MAX_EVENT; i++) {
		event[i].enable = false;
	}
	next_id = NO_EVENT;
	next_clock = past_clock = 0;
	
	// initialize sound buffer
	sound_buffer = NULL;
	sound_tmp = NULL;
	
	accum_clocks = 0;
	first_reset = true;
}

void EVENT::initialize_sound(int rate, int samples)
{
	// initialize sound
	sound_samples = samples;
#ifdef EVENT_CONTINUOUS_SOUND
	sound_tmp_samples = samples + (int)(rate / frames_per_sec * 2);
#else
	sound_tmp_samples = samples;
#endif
	update_samples = (int)(1024. * rate / frames_per_sec / lines_per_frame + 0.5);
	
	sound_buffer = (uint16*)malloc(sound_samples * sizeof(uint16) * 2);
	memset(sound_buffer, 0, sound_samples * sizeof(uint16) * 2);
	sound_tmp = (int32*)malloc(sound_tmp_samples * sizeof(int32) * 2);
	memset(sound_tmp, 0, sound_tmp_samples * sizeof(int32) * 2);
	buffer_ptr = accum_samples = 0;
}

void EVENT::release()
{
	// release sound
	if(sound_buffer) {
		free(sound_buffer);
	}
	if(sound_tmp) {
		free(sound_tmp);
	}
}

void EVENT::reset()
{
	// skip the first reset request after the device is initialized
	if(first_reset) {
		first_reset = false;
		return;
	}
	
	// clear events (except loop event)
	for(int i = 0; i < event_count; i++) {
		if(!(event[i].enable && event[i].loop)) {
			event[i].enable = false;
		}
	}
	
	// get next event clock
	get_next_event();
	
	// reset sound
	if(sound_buffer) {
		memset(sound_buffer, 0, sound_samples * sizeof(uint16) * 2);
	}
	if(sound_tmp) {
		memset(sound_tmp, 0, sound_tmp_samples * sizeof(int32) * 2);
	}
	buffer_ptr = 0;
}

void EVENT::drive()
{
	// generate clocks per line
	if(update_timing) {
		int sum = (int)((float)event_base_clocks / frames_per_sec + 0.5);
		int remain = sum;
		
		for(int i = 0; i < lines_per_frame; i++) {
			vclocks[i] = (int)(sum / lines_per_frame);
			remain -= vclocks[i];
		}
		for(int i = 0; i < remain; i++) {
			int index = (int)((float)lines_per_frame * (float)i / (float)remain);
			vclocks[index]++;
		}
		for(DEVICE* device = vm->first_device; device; device = device->next_device) {
			device->update_timing(frames_per_sec, lines_per_frame);
		}
		update_timing = false;
	}
	
	// run virtual machine for 1 frame period
	for(int i = 0; i < frame_event_count; i++) {
		frame_event[i]->event_frame();
	}
	for(int v = 0; v < lines_per_frame; v++) {
		// run virtual machine per line
		for(int i = 0; i < vline_event_count; i++) {
			vline_event[i]->event_vline(v, vclocks[v]);
		}
		update_event(vclocks[v]);
		update_sound();
	}
}

void EVENT::update_event(int clock)
{
	while(clock) {
		past_clock = 0;
		while(clock && (next_id == NO_EVENT || next_clock > past_clock)) {
			// run cpu
#ifdef EVENT_PRECISE
			int remain = next_clock - past_clock;
			int tmp = (next_id == NO_EVENT || clock < remain) ? clock : remain;
			int eventclock = tmp > EVENT_PRECISE ? EVENT_PRECISE : tmp;
#else
			int remain = next_clock - past_clock;
			int eventclock = (next_id == NO_EVENT || clock < remain) ? clock : remain;
#endif
			if(dcount_cpu > 1) {
				// sync cpus
				for(int c = 0; c < eventclock; c++) {
					for(int p = 0; p < (1 << power); p++) {
						d_cpu[0].device->run(1);
						for(int i = 1; i < dcount_cpu; i++) {
							d_cpu[i].accum_clocks += d_cpu[i].update_clocks;
							int clocks = d_cpu[i].accum_clocks >> 10;
							if(clocks) {
								d_cpu[i].accum_clocks -= clocks << 10;
								d_cpu[i].device->run(clocks);
							}
						}
					}
					accum_clocks += 1;
				}
			}
			else {
				d_cpu[0].device->run(eventclock << power);
				accum_clocks += eventclock;
			}
			clock -= eventclock;
			past_clock += eventclock;
		}
		// update event_clock
		if(past_clock) {
			for(int i = 0; i < event_count; i++) {
				event[i].clock -= past_clock;
			}
			next_clock -= past_clock;
			past_clock = 0;
		}
		// run event
		get_nextevent = false;
		while(!(next_id == NO_EVENT || next_clock > 0)) {
			// run event
			int err = event[next_id].clock;
			if(event[next_id].loop) {
				event[next_id].clock += event[next_id].loop;
			}
			else {
				event[next_id].enable = false;
			}
			event[next_id].device->event_callback(event[next_id].event_id, err);
			
			// get next event clock
			get_next_event();
		}
		get_nextevent = true;
	}
}

void EVENT::get_next_event()
{
	next_id = NO_EVENT;
	for(int i = 0; i < event_count; i++) {
		if(event[i].enable && (next_id == NO_EVENT || event[i].clock < next_clock)) {
			next_clock = event[i].clock;
			next_id = i;
		}
	}
}

uint32 EVENT::current_clock()
{
	return accum_clocks + (d_cpu[0].device->passed_clock() >> power);
}

uint32 EVENT::passed_clock(uint32 prev)
{
	uint32 current = current_clock();
	return (current > prev) ? current - prev : current + (0xffffffff - prev) + 1;
}

void EVENT::register_event(DEVICE* dev, int event_id, int usec, bool loop, int* register_id)
{
	int clock = (int)(event_base_clocks / 1000000. * usec + 0.5);
	register_event_by_clock(dev, event_id, clock, loop, register_id);
}

uint32 EVENT::get_prv_pc(int index)
{
	return d_cpu[index].device->get_prv_pc();
}

void EVENT::register_event_by_clock(DEVICE* dev, int event_id, int clock, bool loop, int* register_id)
{
	// register event
#ifdef _DEBUG_LOG
	bool registered = false;
#endif
	if(register_id != NULL) {
		*register_id = -1;
	}
	for(int i = 0; i < MAX_EVENT; i++) {
		if(!event[i].enable) {
			if(event_count < i + 1) {
				event_count = i + 1;
			}
			event[i].enable = true;
			event[i].device = dev;
			event[i].event_id = event_id;
			event[i].clock = clock + past_clock + (d_cpu[0].device->passed_clock() >> power);
			event[i].loop = loop ? clock : 0;
			if(register_id != NULL) {
				*register_id = i;
			}
#ifdef _DEBUG_LOG
			registered = true;
#endif
			break;
		}
	}
#ifdef _DEBUG_LOG
	if(!registered) {
		emu->out_debug(_T("EVENT: too many events !!!\n"));
	}
#endif
	
	// get next event clock
	if(get_nextevent) {
		get_next_event();
	}
}

void EVENT::cancel_event(int register_id)
{
	// cancel registered event
	if(0 <= register_id && register_id < MAX_EVENT) {
		event[register_id].enable = false;
		
		// get next event clock
		if(next_id == register_id) {
			get_next_event();
		}
	}
	
}

void EVENT::register_frame_event(DEVICE* dev)
{
	if(frame_event_count < MAX_EVENT) {
		frame_event[frame_event_count++] = dev;
	}
#ifdef _DEBUG_LOG
	else {
		emu->out_debug(_T("EVENT: too many frame events !!!\n"));
	}
#endif
}

void EVENT::register_vline_event(DEVICE* dev)
{
	if(vline_event_count < MAX_EVENT) {
		vline_event[vline_event_count++] = dev;
	}
#ifdef _DEBUG_LOG
	else {
		emu->out_debug(_T("EVENT: too many vline events !!!\n"));
	}
#endif
}

void EVENT::mix_sound(int samples)
{
	if(samples > 0) {
		memset(sound_tmp + buffer_ptr * 2, 0, samples * sizeof(int32) * 2);
		for(int i = 0; i < dcount_sound; i++) {
			d_sound[i]->mix(sound_tmp + buffer_ptr * 2, samples);
		}
		buffer_ptr += samples;
	}
	else {
		// notify to sound devices
		for(int i = 0; i < dcount_sound; i++) {
			d_sound[i]->mix(sound_tmp + buffer_ptr * 2, 0);
		}
	}
}

void EVENT::update_sound()
{
	accum_samples += update_samples;
	int samples = accum_samples >> 10;
	accum_samples -= samples << 10;
	
	// mix sound
	if(sound_tmp_samples - buffer_ptr < samples) {
		samples = sound_tmp_samples - buffer_ptr;
	}
	mix_sound(samples);
}

uint16* EVENT::create_sound(int* extra_frames)
{
	int frames = 0;
	
#ifdef EVENT_CONTINUOUS_SOUND
	// drive extra frames to fill the sound buffer
	while(sound_samples > buffer_ptr) {
		drive();
		frames++;
	}
#else
	// fill sound buffer
	int samples = sound_samples - buffer_ptr;
	mix_sound(samples);
#endif
#ifdef LOW_PASS_FILTER
	// low-pass filter
	for(int i = 0; i < sound_samples - 1; i++) {
		sound_tmp[i * 2    ] = (sound_tmp[i * 2    ] + sound_tmp[i * 2 + 2]) / 2; // L
		sound_tmp[i * 2 + 1] = (sound_tmp[i * 2 + 1] + sound_tmp[i * 2 + 3]) / 2; // R
	}
#endif
	// copy to buffer
	for(int i = 0; i < sound_samples * 2; i++) {
		int dat = sound_tmp[i];
		uint16 highlow = (uint16)(dat & 0x0000ffff);
		
		if((dat > 0) && (highlow >= 0x8000)) {
			sound_buffer[i] = 0x7fff;
			continue;
		}
		if((dat < 0) && (highlow < 0x8000)) {
			sound_buffer[i] = 0x8000;
			continue;
		}
		sound_buffer[i] = highlow;
	}
	if(buffer_ptr > sound_samples) {
		buffer_ptr -= sound_samples;
		memcpy(sound_tmp, sound_tmp + sound_samples * 2, buffer_ptr * sizeof(int32) * 2);
	}
	else {
		buffer_ptr = 0;
	}
	*extra_frames = frames;
	return sound_buffer;
}

void EVENT::update_config()
{
	power = config.cpu_power;
}
