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
#define MAX_EVENT	64

class EVENT : public DEVICE
{
private:
	DEVICE* cpu[MAX_CPU];
	DEVICE* sound[MAX_SOUND];
	int cpu_cnt, sound_cnt;
	
	// cpu clock
	int hclocks[LINES_PER_FRAME][CHARS_PER_LINE];
	int vclocks[LINES_PER_FRAME];
	int power;
	
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
	DEVICE* vsync_event[MAX_EVENT];
	DEVICE* hsync_event[MAX_EVENT];
	int next, past;
	int event_cnt, frame_event_cnt, vsync_event_cnt, hsync_event_cnt;
	
	void update_event(int clock);
	
	// sound manager
	uint16* sound_buffer;
	int32* sound_tmp;
	int buffer_ptr;
	int sound_samples;
	int accum_samples, update_samples;
	
	void update_sound();
	
public:
	EVENT(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		cpu_cnt = sound_cnt = 0;
	}
	~EVENT() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void update_config();
	
	// unique functions
	void initialize_sound(int rate, int samples);
	void drive();
	void regist_event(DEVICE* device, int event_id, int usec, bool loop, int* regist_id);
	void regist_event_by_clock(DEVICE* device, int event_id, int clock, bool loop, int* regist_id);
	void cancel_event(int regist_id);
	void regist_frame_event(DEVICE* dev);
	void regist_vsync_event(DEVICE* dev);
	void regist_hsync_event(DEVICE* dev);
	uint16* create_sound(int samples, bool fill);
	
	void set_context_cpu(DEVICE* device) {
		cpu[cpu_cnt++] = device;
	}
	void set_context_sound(DEVICE* device) {
		sound[sound_cnt++] = device;
	}
};

#endif

