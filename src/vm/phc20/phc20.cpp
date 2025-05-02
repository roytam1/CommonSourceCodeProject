/*
	SANYO PHC-20 Emulator 'ePHC-20'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.09.03-

	[ virtual machine ]
*/

#include "phc20.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../datarec.h"
#include "../mc6847.h"
#include "../z80.h"

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
	vdp = new MC6847(this, emu);
	cpu = new Z80(this, emu);
	
	memory = new MEMORY(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	
	drec->set_context_out(memory, SIG_MEMORY_SYSPORT, 1);
	vdp->set_context_vsync(memory, SIG_MEMORY_SYSPORT, 2);	// vsync / hsync?
	
	vdp->set_vram_ptr(memory->get_vram(), 0x400);
	vdp->write_signal(SIG_MC6847_AG, 0, 0);		// force character mode
	vdp->write_signal(SIG_MC6847_AS, 0, 0);
	vdp->write_signal(SIG_MC6847_INTEXT, 0, 0);	// force internal character ???
	vdp->write_signal(SIG_MC6847_CSS, 0, 0);
	
	memory->set_context_drec(drec);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(dummy);
	cpu->set_context_intr(dummy);
	
	// initialize all devices
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id) {
			device->initialize();
		}
	}
}

VM::~VM()
{
	// delete all devices
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->release();
	}
}

DEVICE* VM::get_device(int id)
{
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id == id) {
			return device;
		}
	}
	return NULL;
}

// ----------------------------------------------------------------------------
// drive virtual machine
// ----------------------------------------------------------------------------

void VM::reset()
{
	// reset all devices
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->reset();
	}
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

void VM::regist_vline_event(DEVICE* dev)
{
	event->regist_vline_event(dev);
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

uint32 VM::get_prv_pc()
{
	return cpu->get_prv_pc();
}

// ----------------------------------------------------------------------------
// draw screen
// ----------------------------------------------------------------------------

void VM::draw_screen()
{
	vdp->draw_screen();
}

// ----------------------------------------------------------------------------
// soud manager
// ----------------------------------------------------------------------------

void VM::initialize_sound(int rate, int samples)
{
	// init sound manager
	event->initialize_sound(rate, samples);
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::play_datarec(_TCHAR* filename)
{
	drec->play_datarec(filename);
	drec->write_signal(SIG_DATAREC_REMOTE, 1, 1);
}

void VM::rec_datarec(_TCHAR* filename)
{
	drec->rec_datarec(filename);
	drec->write_signal(SIG_DATAREC_REMOTE, 1, 1);
}

void VM::close_datarec()
{
	drec->close_datarec();
	drec->write_signal(SIG_DATAREC_REMOTE, 0, 0);
}

bool VM::now_skip()
{
	return false;//drec->skip();
}

void VM::update_config()
{
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->update_config();
	}
}

