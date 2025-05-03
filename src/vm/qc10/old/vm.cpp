/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ vitusl machine ]
*/

#include "vm.h"
#include "../emu.h"
#include "device.h"

#include "z80.h"
#include "memory.h"
#include "io.h"

#include "hd146818p.h"
#include "i8237.h"
#include "i8253.h"
#include "i8255.h"
#include "i8259.h"
#include "sound.h"
#include "upd7201.h"
#include "upd7220.h"
#include "upd765a.h"

// ----------------------------------------------------------------------------
// initialize
// ----------------------------------------------------------------------------

VM::VM(EMU* parent_emu) : emu(parent_emu)
{
	// create clocks
	int remain = CLOCKS_PER_LINE;
	for(int i = 0; i < CHARS_PER_LINE; i++) {
		clocks[i] = (int)(CLOCKS_PER_LINE / CHARS_PER_LINE);
		remain -= clocks[i];
	}
	for(int i = 0; i < remain; i++)
		clocks[(int)(CHARS_PER_LINE * i / remain)]++;
	
	// clear all event
	for(int i = 0; i < CALLBACK_MAX; i++) {
		event[i].enable = false;
		event[i].device = NULL;
	}
	event_clock = -1;
	past_clock = 0;
	
	// create devices
	first_device = last_device = NULL;
	
	cpu = new Z80(this, emu);
	memory = new MEMORY(this, emu);
	io = new IO(this, emu);

	rtc = new HD146818P(this, emu);
	dmac = new I8237(this, emu);
	pit = new I8253(this, emu);
	pio = new I8255(this, emu);
	pic = new I8259(this, emu);
	sound = new SOUND(this, emu);
	sio = new UPD7201(this, emu);
	crtc = new UPD7220(this, emu);
	fdc = new UPD765A(this, emu);
	
	dummy = new DEVICE(this, emu);
	
	// initialize all devices
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->initialize();
	
	// set context
	cpu->memory = memory;
	cpu->io = io;
	cpu->interrupt = pic;
	
	for(DEVICE* device = first_device; device; device = device->next_device)
		io->regist_iomap(device);
}

VM::~VM()
{
	for(int i = 0; i < CALLBACK_MAX; i++)
		event[i].device = NULL;
	
	// delete all devices
	for(DEVICE* device = first_device; device;) {
		DEVICE* next = device->next_device;
		device->release();
		delete device;
		device = next;
	}
}

// ----------------------------------------------------------------------------
// drive virtual machine
// ----------------------------------------------------------------------------

void VM::reset()
{
	// clear events (except loop event)
	for(int i = 0; i < CALLBACK_MAX; i++) {
		if(!(event[i].enable && event[i].loop)) {
			event[i].enable = false;
			event[i].device = NULL;
		}
	}
	// get next event clock
	event_clock = -1;
	for(int i = 0; i < CALLBACK_MAX; i++) {
		if(event[i].enable && (event[i].clock < event_clock || event_clock < 0))
			event_clock = event[i].clock;
	}
	if(event_clock < 0)
		past_clock = 0;

	// reset all devices
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->reset();
}

void VM::run()
{
	// run virtual machine for 1 frame period
	for(int v = 0; v < LINES_PER_FRAME; v++) {
		crtc->set_vsync(v);
		for(int h = 0; h < CHARS_PER_LINE; h++) {
			crtc->set_hsync(h);
			drive_vm(clocks[h]);
#if 0
			pit->input_clock(clocks[h]);
#endif
		}
#if 1
		pit->input_clock(CLOCKS_PER_LINE);
#endif
		sound->update_sound();
	}
	crtc->set_blink();
//	keyboard->update_input();
	rtc->update_clock();
}

void VM::drive_vm(int clock)
{
	// run cpu
	cpu->run(clock);
	
	// check next event clock
	if(event_clock < 0)
		return;
	
	event_clock -= clock;
	past_clock += clock;
	
	if(event_clock > 0)
		return;
	
	// run events
	for(int i = 0; i < CALLBACK_MAX; i++) {
		if(!event[i].enable)
			continue;
		// event is active
		event[i].clock -= past_clock;
		while(event[i].clock <= 0 && event[i].enable) {
			int usec = (int)(event[i].clock * 1000000. / CPU_CLOCKS + 0.5);
			if(event[i].loop) // loop event
				event[i].clock += event[i].loop;
			else
				event[i].enable = false;
			event[i].device->event_callback(event[i].event_id, usec);
		}
	}
	past_clock = 0;
	
	// get next event clock
	event_clock = -1;
	for(int i = 0; i < CALLBACK_MAX; i++) {
		if(event[i].enable && (event[i].clock < event_clock || event_clock < 0))
			event_clock = event[i].clock;
	}
}

void VM::regist_callback(DEVICE* dev, int event_id, int usec, bool loop, int* regist_id)
{
	// regist_id = -1 when failed to regist event
	*regist_id = -1;
	
	// check if events exist or not
	bool no_exist = true;
	for(int i = 0; i < CALLBACK_MAX; i++) {
		if(event[i].enable) {
			no_exist = false;
			break;
		}
	}
	if(no_exist)
		past_clock = 0;
	
	// regist event
	for(int i = 0; i < CALLBACK_MAX; i++) {
		if(!event[i].enable) {
			int clock = (int)(CPU_CLOCKS * usec / 1000000. + 0.5);
			event[i].enable = true;
			event[i].device = dev;
			event[i].event_id = event_id;
			event[i].clock = clock + past_clock;
			event[i].loop = loop ? clock : 0;
			
			*regist_id = i;
			break;
		}
	}
	
	// get next event clock
	event_clock = -1;
	for(int i = 0; i < CALLBACK_MAX; i++) {
		if(event[i].enable && (event[i].clock < event_clock || event_clock < 0))
			event_clock = event[i].clock;
	}
	if(event_clock < 0)
		past_clock = 0;
}

void VM::regist_callback_by_clock(DEVICE* dev, int event_id, int clock, bool loop, int* regist_id)
{
	// regist_id = -1 when failed to regist event
	*regist_id = -1;
	
	// check if events exist or not
	bool no_exist = true;
	for(int i = 0; i < CALLBACK_MAX; i++) {
		if(event[i].enable) {
			no_exist = false;
			break;
		}
	}
	if(no_exist)
		past_clock = 0;
	
	// regist event
	for(int i = 0; i < CALLBACK_MAX; i++) {
		if(!event[i].enable) {
			event[i].enable = true;
			event[i].device = dev;
			event[i].event_id = event_id;
			event[i].clock = clock + past_clock;
			event[i].loop = loop ? clock : 0;
			
			*regist_id = i;
			break;
		}
	}
	
	// get next event clock
	event_clock = -1;
	for(int i = 0; i < CALLBACK_MAX; i++) {
		if(event[i].enable && (event[i].clock < event_clock || event_clock < 0))
			event_clock = event[i].clock;
	}
	if(event_clock < 0)
		past_clock = 0;
}

void VM::cancel_callback(int regist_id)
{
	// cancel registered event
	if(0 <= regist_id && regist_id < CALLBACK_MAX) {
		event[regist_id].device = NULL;
		event[regist_id].enable = false;
	}
	
	// get next event clock
	event_clock = -1;
	for(int i = 0; i < CALLBACK_MAX; i++) {
		if(event[i].enable && (event[i].clock < event_clock || event_clock < 0))
			event_clock = event[i].clock;
	}
	if(event_clock < 0)
		past_clock = 0;
}

// ----------------------------------------------------------------------------
// draw screen
// ----------------------------------------------------------------------------

void VM::draw_screen()
{
	crtc->draw_screen();
}

// ----------------------------------------------------------------------------
// soud generation
// ----------------------------------------------------------------------------

void VM::initialize_sound(int rate, int samples)
{
	sound->initialize_sound(rate, samples);
}

uint16* VM::create_sound(int samples, bool fill)
{
	return sound->create_sound(samples, fill);
}

// ----------------------------------------------------------------------------
// input
// ----------------------------------------------------------------------------

void VM::key_down(int code)
{
	sio->key_down(code);
}

void VM::key_up(int code)
{
	sio->key_up(code);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::insert_disk(_TCHAR* filename, int drv)
{
	fdc->insert_disk(filename, drv);
}

void VM::eject_disk(int drv)
{
	fdc->eject_disk(drv);
}

bool VM::now_skip()
{
	return false;//pio->now_skip();
}

void VM::update_config()
{
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->update_config();
}

