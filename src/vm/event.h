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
	// event manager
	typedef struct {
		DEVICE* device;
		int cpu_clocks;
		int update_clocks;
		int accum_clocks;
	} cpu_t;
	cpu_t d_cpu[MAX_CPU];
	int dcount_cpu;
	
	int vclocks[MAX_LINES];
	int power;
	uint32 accum_clocks;
	
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
	
	double frames_per_sec, next_frames_per_sec;
	int lines_per_frame, next_lines_per_frame;
	
	int next_clock, past_clock, next_id;
	int event_count, frame_event_count, vline_event_count;
	bool get_nextevent;
	void update_event(int clock);
	void get_next_event();
	
	// sound manager
	DEVICE* d_sound[MAX_SOUND];
	int dcount_sound;
	
	uint16* sound_buffer;
	int32* sound_tmp;
	int buffer_ptr;
	int sound_rate;
	int sound_samples;
	int sound_tmp_samples;
	int accum_samples, update_samples;
	void mix_sound(int samples);
	void update_sound();
	
	bool first_reset;
	
public:
	EVENT(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_cpu = dcount_sound = 0;
		event_count = frame_event_count = vline_event_count = 0;
		get_nextevent = true;
		
		// force update timing in the first frame
		frames_per_sec = 0.0;
		lines_per_frame = 0;
		next_frames_per_sec = FRAMES_PER_SEC;
		next_lines_per_frame = LINES_PER_FRAME;
	}
	~EVENT() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void update_config();
	
	// common event functions
	int event_manager_id() {
		return this_device_id;
	}
	void set_frames_per_sec(double new_frames_per_sec) {
		next_frames_per_sec = new_frames_per_sec;
	}
	void set_lines_per_frame(int new_lines_per_frame) {
		next_lines_per_frame = new_lines_per_frame;
	}
	void register_event(DEVICE* device, int event_id, double usec, bool loop, int* register_id);
	void register_event_by_clock(DEVICE* device, int event_id, int clock, bool loop, int* register_id);
	void cancel_event(int register_id);
	void register_frame_event(DEVICE* device);
	void register_vline_event(DEVICE* device);
	uint32 current_clock();
	uint32 passed_clock(uint32 prev);
	uint32 get_cpu_pc(int index);
	
	// unique functions
	double frame_rate() {
		return next_frames_per_sec;
	}
	void drive();
	
	void initialize_sound(int rate, int samples);
	uint16* create_sound(int* extra_frames);
	
	void set_context_cpu(DEVICE* device, int clocks) {
		int index = dcount_cpu++;
		d_cpu[index].device = device;
		d_cpu[index].cpu_clocks = clocks;
		d_cpu[index].accum_clocks = 0;
	}
	void set_context_cpu(DEVICE* device) {
		set_context_cpu(device, CPU_CLOCKS);
	}
	void set_context_sound(DEVICE* device) {
		d_sound[dcount_sound++] = device;
	}
};

#endif

