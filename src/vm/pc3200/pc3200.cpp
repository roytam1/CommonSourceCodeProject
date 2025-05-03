/*
	SHARP PC-3200 Emulator 'ePC-3200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.07.08 -

	[ virtual machine ]
*/

#include "pc3200.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../datarec.h"
#include "../i8255.h"
#include "../io8.h"
#include "../upd1990a.h"
#include "../upd765a.h"
#include "../z80.h"

#include "display.h"
#include "keyboard.h"
#include "memory.h"

// ----------------------------------------------------------------------------
// initialize
// ----------------------------------------------------------------------------

VM::VM(EMU* parent_emu) : emu(parent_emu)
{
	// create devices
	first_device = last_device = NULL;
	dummy = new DEVICE(this, emu);	// must be 1st device
	event = new EVENT(this, emu);	// must be 2nd device
	event->initialize();		// must be initialized first
	
	drec = new DATAREC(this, emu);
	pio = new I8255(this, emu);
	io = new IO8(this, emu);
	rtc = new UPD1990A(this, emu);
	fdc = new UPD765A(this, emu);
	cpu = new Z80(this, emu);
	
	display = new DISPLAY(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(dummy);
	
	// i/o bus
	
	// initialize and reset all devices except the event manager
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id)
			device->initialize();
	}
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id)
			device->reset();
	}
}

VM::~VM()
{
	// delete all devices
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->release();
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
}

void VM::run()
{
	event->drive();
}

// ----------------------------------------------------------------------------
// event manager
// ----------------------------------------------------------------------------

void VM::regist_event(DEVICE* dev, int event_id, int usec, bool loop, int* regist_id)
{
	event->regist_event(dev, event_id, usec, loop, regist_id);
}

void VM::regist_event_by_clock(DEVICE* dev, int event_id, int clock, bool loop, int* regist_id)
{
	event->regist_event_by_clock(dev, event_id, clock, loop, regist_id);
}

void VM::cancel_event(int regist_id)
{
	event->cancel_event(regist_id);
}

void VM::regist_frame_event(DEVICE* dev)
{
	event->regist_frame_event(dev);
}

void VM::regist_vsync_event(DEVICE* dev)
{
	event->regist_vsync_event(dev);
}

void VM::regist_hsync_event(DEVICE* dev)
{
	event->regist_hsync_event(dev);
}

uint32 VM::current_clock()
{
	return event->current_clock();
}

uint32 VM::passed_clock(uint32 prev)
{
	uint32 current = event->current_clock();
	return (current > prev) ? current - prev : current + (0xffffffff - prev) + 1;
}

// ----------------------------------------------------------------------------
// draw screen
// ----------------------------------------------------------------------------

void VM::draw_screen()
{
	display->draw_screen();
}

// ----------------------------------------------------------------------------
// soud manager
// ----------------------------------------------------------------------------

void VM::initialize_sound(int rate, int samples)
{
	// init sound manager
	event->initialize_sound(rate, samples);
	
	// init sound gen
}

uint16* VM::create_sound(int samples, bool fill)
{
	return event->create_sound(samples, fill);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::open_disk(_TCHAR* filename, int drv)
{
	fdc->open_disk(filename, drv);
}

void VM::close_disk(int drv)
{
	fdc->close_disk(drv);
}

void VM::play_datarec(_TCHAR* filename)
{
	drec->play_datarec(filename);
}

void VM::rec_datarec(_TCHAR* filename)
{
	drec->rec_datarec(filename);
}

void VM::close_datarec()
{
	drec->close_datarec();
}

bool VM::now_skip()
{
	return drec->skip();
}

void VM::update_config()
{
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->update_config();
}

