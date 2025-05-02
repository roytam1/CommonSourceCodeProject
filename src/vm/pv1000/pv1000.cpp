/*
	CASIO PV-1000 Emulator 'ePV-1000'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.16 -

	[ virtual machine ]
*/

#include "pv1000.h"
#include "../../emu.h"
#include "../device.h"

#include "../io8.h"
#include "../z80.h"

#include "joystick.h"
#include "memory.h"
#include "sound.h"
#include "vdp.h"

#include "../../config.h"

extern config_t config;

// ----------------------------------------------------------------------------
// initialize
// ----------------------------------------------------------------------------

VM::VM(EMU* parent_emu) : emu(parent_emu)
{
	// load config
	if(!(0 <= config.cpu_power && config.cpu_power <= 4))
		config.cpu_power = 0;
	power = config.cpu_power;
	
	// generate clocks per line
	int sum = (int)((float)CPU_CLOCKS / (float)FRAMES_PER_SEC + 0.5);
	int remain = sum;
	
	for(int i = 0; i < LINES_PER_FRAME; i++) {
		clocks[i] = (int)(sum / LINES_PER_FRAME);
		remain -= clocks[i];
	}
	for(int i = 0; i < remain; i++)
		clocks[(int)(LINES_PER_FRAME * i / remain)]++;
	
	// initialize event manager
	initialize_event();
	
	// create devices
	first_device = last_device = NULL;
	dummy = new DEVICE(this, emu);
	
	io = new IO8(this, emu);
	cpu = new Z80(this, emu);
	
	joystick = new JOYSTICK(this, emu);
	memory = new MEMORY(this, emu);
	sound = new SOUND(this, emu);
	vdp = new VDP(this, emu);
	
	// set contexts
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_int(dummy);
	vdp->set_context(cpu);
	vdp->set_memory_ptr(memory->get_memory());
	
	io->set_iomap_range_w(0xfc, 0xfd, joystick);
	io->set_iomap_range_w(0xfe, 0xff, vdp);
	
	io->set_iomap_range_r(0xfc, 0xfd, joystick);
	
	// initialize and reset all devices
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->initialize();
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->reset();
}

VM::~VM()
{
	// delete all devices
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->release();
	release_sound();
}

DEVICE* VM::get_device(int id)
{
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id == id)
			return device;
	}
	return NULL;
}

// ----------------------------------------------------------------------------
// drive virtual machine
// ----------------------------------------------------------------------------

void VM::reset()
{
	// reset all devices
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->reset();
	reset_event();
	reset_sound();
}

void VM::run()
{
	// run virtual machine for 1 frame period
	for(int i = 0; i < frame_event_cnt; i++)
		frame_event[i]->event_frame();
	for(int v = 0; v < LINES_PER_HBLANK; v++) {
		for(int i = 0; i < vsync_event_cnt; i++)
			vsync_event[i]->event_vsync(v, clocks[v]);
//		update_event(clocks[v]);
		cpu->run(CLOCKS_PER_HBLANK << power);
		update_sound();
	}
	for(int v = LINES_PER_HBLANK; v < LINES_PER_FRAME; v++) {
		for(int i = 0; i < vsync_event_cnt; i++)
			vsync_event[i]->event_vsync(v, clocks[v]);
//		update_event(clocks[v]);
		cpu->run(clocks[v] << power);
		update_sound();
	}
}

// ----------------------------------------------------------------------------
// event manager
// ----------------------------------------------------------------------------

void VM::initialize_event()
{
	for(int i = 0; i < MAX_EVENT; i++) {
		event[i].enable = false;
		event[i].device = NULL;
	}
	next = -1;
	past = event_cnt = frame_event_cnt = vsync_event_cnt = 0;
}

void VM::reset_event()
{
	// clear events (except loop event)
	for(int i = 0; i < event_cnt; i++) {
		if(!(event[i].enable && event[i].loop)) {
			event[i].enable = false;
			event[i].device = NULL;
		}
	}
	// get next event clock
	next = -1;
	for(int i = 0; i < event_cnt; i++) {
		if(event[i].enable && (event[i].clock < next || next == -1))
			next = event[i].clock;
	}
	if(next == -1)
		past = 0;
}

void VM::update_event(int clock)
{
	while(clock) {
		if(clock < next || next == -1) {
			// run cpu
			cpu->run(clock << power);
			
			// update next event clock
			if(next != -1) {
				next -= clock;
				past += clock;
			}
			return;
		}
		
		// run cpu until next event is occured
		cpu->run(next << power);
		clock -= next;
		past += next;
		
		for(int i = 0; i < event_cnt; i++) {
			if(!event[i].enable)
				continue;
			// event is active
			event[i].clock -= past;
			while(event[i].clock <= 0) {
				event[i].device->event_callback(event[i].event_id);
				if(event[i].loop) {
					// loop event
					event[i].clock += event[i].loop;
				}
				else {
					// not loop event
					event[i].device = NULL;
					event[i].enable = false;
					break;
				}
			}
		}
		
		// get next event clock
		past = 0;
		next = -1;
		for(int i = 0; i < event_cnt; i++) {
			if(event[i].enable && (event[i].clock < next || next == -1))
				next = event[i].clock;
		}
	}
}

void VM::regist_event(DEVICE* dev, int event_id, int usec, bool loop, int* regist_id)
{
	// regist_id = -1 when failed to regist event
	*regist_id = -1;
	
	// check if events exist or not
	bool no_exist = true;
	for(int i = 0; i < event_cnt; i++) {
		if(event[i].enable) {
			no_exist = false;
			break;
		}
	}
	if(no_exist)
		past = 0;
	
	// regist event
	for(int i = 0; i < MAX_EVENT; i++) {
		if(!event[i].enable) {
			if(event_cnt < i + 1)
				event_cnt = i + 1;
			int clock = (int)(CPU_CLOCKS / 1000000. * usec + 0.5);
			event[i].enable = true;
			event[i].device = dev;
			event[i].event_id = event_id;
			event[i].clock = clock + past;
			event[i].loop = loop ? clock : 0;
			*regist_id = i;
			break;
		}
	}
	
	// get next event clock
	next = -1;
	for(int i = 0; i < event_cnt; i++) {
		if(event[i].enable && (event[i].clock < next || next == -1))
			next = event[i].clock;
	}
	if(next == -1)
		past = 0;
}

void VM::regist_event_by_clock(DEVICE* dev, int event_id, int clock, bool loop, int* regist_id)
{
	// regist_id = -1 when failed to regist event
	*regist_id = -1;
	
	// check if events exist or not
	bool no_exist = true;
	for(int i = 0; i < event_cnt; i++) {
		if(event[i].enable) {
			no_exist = false;
			break;
		}
	}
	if(no_exist)
		past = 0;
	
	// regist event
	for(int i = 0; i < MAX_EVENT; i++) {
		if(!event[i].enable) {
			if(event_cnt < i + 1)
				event_cnt = i + 1;
			event[i].enable = true;
			event[i].device = dev;
			event[i].event_id = event_id;
			event[i].clock = clock + past;
			event[i].loop = loop ? clock : 0;
			*regist_id = i;
			break;
		}
	}
	
	// get next event clock
	next = -1;
	for(int i = 0; i < event_cnt; i++) {
		if(event[i].enable && (event[i].clock < next || next == -1))
			next = event[i].clock;
	}
	if(next == -1)
		past = 0;
}

void VM::cancel_event(int regist_id)
{
	// cancel registered event
	if(0 <= regist_id && regist_id < MAX_EVENT) {
		event[regist_id].device = NULL;
		event[regist_id].enable = false;
	}
	
	// get next event clock
	next = -1;
	for(int i = 0; i < event_cnt; i++) {
		if(event[i].enable && (event[i].clock < next || next == -1))
			next = event[i].clock;
	}
	if(next == -1)
		past = 0;
}

void VM::regist_frame_event(DEVICE* dev)
{
	frame_event[frame_event_cnt++] = dev;
}

void VM::regist_vsync_event(DEVICE* dev)
{
	vsync_event[vsync_event_cnt++] = dev;
}

// ----------------------------------------------------------------------------
// soud manager
// ----------------------------------------------------------------------------

void VM::initialize_sound(int rate, int samples)
{
	// init sound manager
	sound_samples = samples;
	update_samples = (int)(1024. * rate / FRAMES_PER_SEC / LINES_PER_FRAME + 0.5);
	
	sound_buffer = (uint16*)malloc(samples * sizeof(uint16));
	_memset(sound_buffer, 0, samples * sizeof(uint16));
	sound_tmp = (int32*)malloc(samples * sizeof(int32));
	_memset(sound_tmp, 0, samples * sizeof(int32));
	buffer_ptr = accum_samples = 0;
	
	// init sound gen
	sound->init(rate);
}

void VM::release_sound()
{
	if(sound_buffer)
		free(sound_buffer);
	if(sound_tmp)
		free(sound_tmp);
}

void VM::reset_sound()
{
	if(sound_buffer)
		_memset(sound_buffer, 0, sound_samples * sizeof(uint16));
	if(sound_tmp)
		_memset(sound_tmp, 0, sound_samples * sizeof(int32));
	buffer_ptr = 0;
}

void VM::update_sound()
{
	accum_samples += update_samples;
	int samples = accum_samples >> 10;
	accum_samples -= samples << 10;
	create_sound(samples, false);
}

uint16* VM::create_sound(int samples, bool fill)
{
	// get samples to be created
	int cnt = 0;
	if(fill)
		cnt = sound_samples - buffer_ptr;
	else
		cnt = (sound_samples - buffer_ptr < samples) ? sound_samples - buffer_ptr : samples;
	
	// create sound buffer
	if(cnt) {
		_memset(&sound_tmp[buffer_ptr], 0, cnt * sizeof(int32));
		sound->mix(&sound_tmp[buffer_ptr], cnt);
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

// ----------------------------------------------------------------------------
// draw screen
// ----------------------------------------------------------------------------

void VM::draw_screen()
{
	vdp->draw_screen();
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::open_cart(_TCHAR* filename)
{
	memory->open_cart(filename);
	reset();
}

void VM::close_cart()
{
	memory->close_cart();
	reset();
}

bool VM::now_skip()
{
	return false;
}

void VM::update_config()
{
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->update_config();
	power = config.cpu_power;
}

