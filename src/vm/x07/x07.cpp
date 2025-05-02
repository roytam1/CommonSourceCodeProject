/*
	CANON X-07 Emulator 'eX-07'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.12.26 -

	[ virtual machine ]
*/

#include "x07.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../beep.h"
#include "../memory.h"
#include "../z80.h"

#include "io.h"

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
	
	beep = new BEEP(this, emu);
	memory = new MEMORY(this, emu);
	cpu = new Z80(this, emu);
	
	io = new IO(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(beep);
	
	io->set_context_beep(beep);
	io->set_context_cpu(cpu);
	io->set_context_mem(memory, ram);
	
	// memory bus
	_memset(ram, 0, sizeof(ram));
	_memset(app, 0xff, sizeof(app));
	_memset(tv, 0xff, sizeof(tv));
	_memset(bas, 0xff, sizeof(bas));
	
	memory->read_bios(_T("APP.ROM"), app, sizeof(app));
	memory->read_bios(_T("TV.ROM"), tv, sizeof(tv));
	memory->read_bios(_T("BASIC.ROM"), bas, sizeof(bas));
	
	memory->set_memory_rw(0x0000, 0x5fff, ram);
	memory->set_memory_r(0x6000, 0x7fff, app);
	memory->set_memory_rw(0x8000, 0x97ff, vram);
	memory->set_memory_r(0xa000, 0xafff, tv);
	memory->set_memory_r(0xb000, 0xffff, bas);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(io);
	
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
	io->draw_screen();
}

// ----------------------------------------------------------------------------
// soud manager
// ----------------------------------------------------------------------------

void VM::initialize_sound(int rate, int samples)
{
	// init sound manager
	event->initialize_sound(rate, samples);
	
	// init sound gen
	beep->init(rate, 1000, 2, 8000);
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
}

// ----------------------------------------------------------------------------
// notify key
// ----------------------------------------------------------------------------

void VM::key_down(int code)
{
	io->key_down(code);
}

void VM::key_up(int code)
{
	io->key_up(code);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::play_datarec(_TCHAR* filename)
{
	io->play_datarec(filename);
}

void VM::rec_datarec(_TCHAR* filename)
{
	io->rec_datarec(filename);
}

void VM::close_datarec()
{
	io->close_datarec();
}

bool VM::now_skip()
{
//	return drec->skip();
	return false;
}

void VM::update_config()
{
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->update_config();
	}
}

