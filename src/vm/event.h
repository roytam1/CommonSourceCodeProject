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
	
	// cpu clock
	int hclocks[LINES_PER_FRAME][CHARS_PER_LINE];
	int vclocks[LINES_PER_FRAME];
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
	DEVICE* vsync_event[MAX_EVENT];
	DEVICE* hsync_event[MAX_EVENT];
	int next, past, next_id;
	int event_cnt, frame_event_cnt, vsync_event_cnt, hsync_event_cnt;
	bool get_nextevent;
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
		dcount_cpu = dcount_sound = 0;
		get_nextevent = true;
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
	void regist_event(DEVICE* device, int event_id, int usec, bool loop, int* regist_id);
	void regist_event_by_clock(DEVICE* device, int event_id, int clock, bool loop, int* regist_id);
	void cancel_event(int regist_id);
	void regist_frame_event(DEVICE* dev);
	void regist_vsync_event(DEVICE* dev);
	void regist_hsync_event(DEVICE* dev);
	
	void initialize_sound(int rate, int samples);
	uint16* create_sound(int samples, bool fill);
	
	void set_context_cpu(DEVICE* device) {
		d_cpu[dcount_cpu++] = device;
	}
	void set_context_sound(DEVICE* device) {
		d_sound[dcount_sound++] = device;
	}
};

#endif

