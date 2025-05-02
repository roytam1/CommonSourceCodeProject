/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.29-

	[ event manager ]
*/

#include "event.h"
#include "../config.h"

void EVENT::initialize()
{
	// load config
	if(!(0 <= config.cpu_power && config.cpu_power <= 4)) {
		config.cpu_power = 0;
	}
	power = config.cpu_power;
	
	// generate clocks per line and char
	int sum = (int)((float)cpu_clocks / frames_per_sec + 0.5);
	int remain = sum;
	
	for(int i = 0; i < lines_per_frame; i++) {
		vclocks[i] = (int)(sum / lines_per_frame);
		remain -= vclocks[i];
	}
	for(int i = 0; i < remain; i++) {
		int index = (int)((float)lines_per_frame * (float)i / (float)remain);
		vclocks[index]++;
	}
	accum = 0;
	
	// initialize event
	for(int i = 0; i < MAX_EVENT; i++) {
		event[i].enable = false;
	}
	next_id = NO_EVENT;
	next = past = 0;
	event_cnt = frame_event_cnt = vline_event_cnt = 0;
	
	// initialize sound buffer
	sound_buffer = NULL;
	sound_tmp = NULL;
	
	first_reset = true;
}

void EVENT::initialize_sound(int rate, int samples)
{
	// initialize sound
	sound_samples = samples;
	update_samples = (int)(1024. * rate / frames_per_sec / lines_per_frame + 0.5);
	
	sound_buffer = (uint16*)malloc(samples * sizeof(uint16));
	_memset(sound_buffer, 0, samples * sizeof(uint16));
	sound_tmp = (int32*)malloc(samples * sizeof(int32));
	_memset(sound_tmp, 0, samples * sizeof(int32));
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
	for(int i = 0; i < event_cnt; i++) {
		if(!(event[i].enable && event[i].loop)) {
			event[i].enable = false;
		}
	}
	
	// get next event clock
	next_id = NO_EVENT;
	for(int i = 0; i < event_cnt; i++) {
		if(event[i].enable && (event[i].clock < next || next_id == NO_EVENT)) {
			next = event[i].clock;
			next_id = i;
		}
	}
	
	// reset sound
	if(sound_buffer) {
		_memset(sound_buffer, 0, sound_samples * sizeof(uint16));
	}
	if(sound_tmp) {
		_memset(sound_tmp, 0, sound_samples * sizeof(int32));
	}
	buffer_ptr = 0;
}

void EVENT::drive()
{
	// run virtual machine for 1 frame period
	for(int i = 0; i < frame_event_cnt; i++) {
		frame_event[i]->event_frame();
	}
	for(int v = 0; v < lines_per_frame; v++) {
		// run virtual machine per line
		for(int i = 0; i < vline_event_cnt; i++) {
			vline_event[i]->event_vline(v, vclocks[v]);
		}
		update_event(vclocks[v]);
		update_sound();
	}
}

void EVENT::update_event(int clock)
{
	int cpu_power = 1 << power;
	while(clock) {
		past = 0;
		while(clock && (next > past || next_id == NO_EVENT)) {
			// run cpu
#ifdef EVENT_PRECISE
			int remain = next - past;
			int tmp = (clock < remain || next_id == NO_EVENT) ? clock : remain;
			int eventclock = tmp > EVENT_PRECISE ? EVENT_PRECISE : tmp;
#else
			int remain = next - past;
			int eventclock = (clock < remain || next_id == NO_EVENT) ? clock : remain;
#endif
			int cpuclock = eventclock * cpu_power;
			
			for(int i = 0; i < dcount_cpu; i++) {
				d_cpu[i]->run(cpuclock);
			}
			clock -= eventclock;
			accum += eventclock;
			past += eventclock;
		}
		// update event_clock
		if(past) {
			for(int i = 0; i < event_cnt; i++) {
				event[i].clock -= past;
			}
			next -= past;
			past = 0;
		}
		// run event
		get_nextevent = false;
		while(!(next > 0 || next_id == NO_EVENT)) {
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
			next_id = NO_EVENT;
			for(int i = 0; i < event_cnt; i++) {
				if(event[i].enable && (event[i].clock < next || next_id == NO_EVENT)) {
					next = event[i].clock;
					next_id = i;
				}
			}
		}
		get_nextevent = true;
	}
}

uint32 EVENT::current_clock()
{
	return accum + (d_cpu[0]->passed_clock() >> power);
}

void EVENT::update_sound()
{
	accum_samples += update_samples;
	int samples = accum_samples >> 10;
	accum_samples -= samples << 10;
	create_sound(samples, false);
}

void EVENT::regist_event(DEVICE* dev, int event_id, int usec, bool loop, int* regist_id)
{
	// regist event
	*regist_id = -1;
	for(int i = 0; i < MAX_EVENT; i++) {
		if(!event[i].enable) {
			if(event_cnt < i + 1) {
				event_cnt = i + 1;
			}
			int clock = (int)(cpu_clocks / 1000000. * usec + 0.5);
			event[i].enable = true;
			event[i].device = dev;
			event[i].event_id = event_id;
			event[i].clock = clock + past + (d_cpu[0]->passed_clock() >> power);
			event[i].loop = loop ? clock : 0;
			*regist_id = i;
			break;
		}
	}
#ifdef _DEBUG_LOG
	if(*regist_id == -1) {
		emu->out_debug(_T("EVENT: too many events !!!\n"));
	}
#endif
	
	// get next event clock
	if(get_nextevent) {
		next_id = NO_EVENT;
		for(int i = 0; i < event_cnt; i++) {
			if(event[i].enable && (event[i].clock < next || next_id == NO_EVENT)) {
				next = event[i].clock;
				next_id = i;
			}
		}
	}
}

void EVENT::regist_event_by_clock(DEVICE* dev, int event_id, int clock, bool loop, int* regist_id)
{
	// regist event
	*regist_id = -1;
	for(int i = 0; i < MAX_EVENT; i++) {
		if(!event[i].enable) {
			if(event_cnt < i + 1) {
				event_cnt = i + 1;
			}
			event[i].enable = true;
			event[i].device = dev;
			event[i].event_id = event_id;
			event[i].clock = clock + past + (d_cpu[0]->passed_clock() >> power);
			event[i].loop = loop ? clock : 0;
			*regist_id = i;
			break;
		}
	}
#ifdef _DEBUG_LOG
	if(*regist_id == -1) {
		emu->out_debug(_T("EVENT: too many events !!!\n"));
	}
#endif
	
	// get next event clock
	if(get_nextevent) {
		next_id = NO_EVENT;
		for(int i = 0; i < event_cnt; i++) {
			if(event[i].enable && (event[i].clock < next || next_id == NO_EVENT)) {
				next = event[i].clock;
				next_id = i;
			}
		}
	}
}

void EVENT::cancel_event(int regist_id)
{
	// cancel registered event
	if(0 <= regist_id && regist_id < MAX_EVENT) {
		event[regist_id].enable = false;
	}
	
	// get next event clock
	next_id = NO_EVENT;
	for(int i = 0; i < event_cnt; i++) {
		if(event[i].enable && (event[i].clock < next || next_id == NO_EVENT)) {
			next = event[i].clock;
			next_id = i;
		}
	}
}

void EVENT::regist_frame_event(DEVICE* dev)
{
	frame_event[frame_event_cnt++] = dev;
}

void EVENT::regist_vline_event(DEVICE* dev)
{
	vline_event[vline_event_cnt++] = dev;
}

uint16* EVENT::create_sound(int samples, bool fill)
{
	// get samples to be created
	int cnt = 0;
	if(fill) {
		cnt = sound_samples - buffer_ptr;
	}
	else {
		cnt = (sound_samples - buffer_ptr < samples) ? sound_samples - buffer_ptr : samples;
	}
	
	// create sound buffer
	if(cnt) {
		_memset(&sound_tmp[buffer_ptr], 0, cnt * sizeof(int32));
		for(int i = 0; i < dcount_sound; i++) {
			d_sound[i]->mix(&sound_tmp[buffer_ptr], cnt);
		}
	}
	
	if(fill) {
		// low-pass filter
//		for(int i = 0; i < sound_samples - 1; i++)
//			sound_tmp[i] = (sound_tmp[i] + sound_tmp[i + 1]) >> 1;
		// copy to buffer
		for(int i = 0; i < sound_samples; i++) {
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
	}
	buffer_ptr = fill ? 0 : (buffer_ptr + cnt);
	return sound_buffer;
}

void EVENT::update_config()
{
	power = config.cpu_power;
}
