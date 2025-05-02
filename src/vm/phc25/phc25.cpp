/*
	SANYO PHC-25 Emulator 'ePHC-25'
	SEIKO MAP-1010 Emulator 'eMAP-1010'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.03-

	[ virtual machine ]
*/

#include "phc25.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../datarec.h"
#include "../io.h"
#include "../mc6847.h"
#include "../not.h"
#include "../ym2203.h"
#include "../z80.h"

#include "joystick.h"
#include "keyboard.h"
#include "memory.h"
#include "system.h"

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
	io = new IO(this, emu);
	vdp = new MC6847(this, emu);
	not = new NOT(this, emu);
	psg = new YM2203(this, emu);
	cpu = new Z80(this, emu);
	
	joystick = new JOYSTICK(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	system = new SYSTEM(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(psg);
	
	vdp->set_vram_ptr(memory->get_vram(), 0x1800);
	vdp->set_context_vsync(not, SIG_NOT_INPUT, 1);
	not->set_context_out(cpu, SIG_CPU_IRQ, 1);
	
	vdp->set_context_vsync(system, SIG_SYSTEM_PORT, 0x10);
	drec->set_context_out(system, SIG_SYSTEM_PORT, 0x20);
	// bit6: printer busy
	vdp->set_context_hsync(system, SIG_SYSTEM_PORT, 0x80);
	
	joystick->set_context_psg(psg);
#ifdef _MAP1010
	memory->set_context_keyboard(keyboard);
#endif
	system->set_context_drec(drec);
	system->set_context_vdp(vdp);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(dummy);
	
	// i/o bus
	io->set_iomap_single_w(0x40, system);
	io->set_iomap_alias_w(0xc0, psg, 1);
	io->set_iomap_alias_w(0xc1, psg, 0);
	
	io->set_iomap_single_r(0x40, system);
#ifndef _MAP1010
	io->set_iomap_range_r(0x80, 0x88, keyboard);
#endif
	io->set_iomap_alias_r(0xc0, psg, 1);
	io->set_iomap_alias_r(0xc1, psg, 1);
	
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
	
	// init sound gen
	psg->init(rate, 1996750, samples, 0, 0);
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
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->update_config();
	}
}

