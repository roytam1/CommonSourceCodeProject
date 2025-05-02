/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.29-

	[ event manager ]
*/

#ifndef _EVENT_H_
#define _EVENT_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define MAX_CPU		8
#define MAX_SOUND	8
#define MAX_LINES	1024
#define MAX_EVENT	64
#define NO_EVENT	-1
//#ifndef EVENT_PRECISE
//#define EVENT_PRECISE	40
//#endif

class EVENT : public DEVICE
{
private:
	DEVICE* d_cpu[MAX_CPU];
	DEVICE* d_sound[MAX_SOUND];
	int dcount_cpu, dcount_sound;
	
	// machine setting
	int cpu_clocks;
	double frames_per_sec;
	int lines_per_frame;
	
	// cpu clock
	int vclocks[MAX_LINES];
	int power;
	uint32 accum;
	
	// event manager
	typedef struct {
		bool enable;
		DEVICE* device;
		int event_id;
		int clock;
		int loop;
	} event_t;
	event_t event[MAX_EVENT];
	DEVICE* frame_event[MAX_EVENT];
	DEVICE* vline_event[MAX_EVENT];
	int next, past, next_id;
	int event_cnt, frame_event_cnt, vline_event_cnt;
	bool get_nextevent;
	void update_event(int clock);
	
	// sound manager
	uint16* sound_buffer;
	int32* sound_tmp;
	int buffer_ptr;
	int sound_samples;
	int sound_tmp_samples;
	int accum_samples, update_samples;
	void mix_sound(int samples);
	void update_sound();
	
	bool first_reset;
	
public:
	EVENT(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_cpu = dcount_sound = 0;
		event_cnt = frame_event_cnt = vline_event_cnt = 0;
		get_nextevent = true;
		cpu_clocks = CPU_CLOCKS;
		frames_per_sec = FRAMES_PER_SEC;
		lines_per_frame = LINES_PER_FRAME;
	}
	~EVENT() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void update_config();
	
	// unique functions
	void drive();
	uint32 current_clock();
	void register_event(DEVICE* device, int event_id, int usec, bool loop, int* register_id);
	void register_event_by_clock(DEVICE* device, int event_id, int clock, bool loop, int* register_id);
	void cancel_event(int register_id);
	void register_frame_event(DEVICE* dev);
	void register_vline_event(DEVICE* dev);
	
	void initialize_sound(int rate, int samples);
	uint16* create_sound(int* extra_frames);
	
	void set_context_cpu(DEVICE* device) {
		d_cpu[dcount_cpu++] = device;
	}
	void set_context_sound(DEVICE* device) {
		d_sound[dcount_sound++] = device;
	}
	void set_cpu_clocks(int clocks) {
		cpu_clocks = clocks;
	}
	void set_frames_per_sec(int frames) {
		frames_per_sec = frames;
	}
	void set_lines_per_frame(int lines) {
		lines_per_frame = lines;
	}
};

#endif

