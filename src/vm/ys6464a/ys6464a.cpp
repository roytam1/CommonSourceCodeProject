/*
	SHINKO SANGYO YS-6464A Emulator 'eYS-6464A'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.12.30 -

	[ virtual machine ]
*/

#include "ys6464a.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../io.h"
#include "../i8255.h"
#include "../memory.h"
#include "../pcm1bit.h"
#include "../z80.h"

#include "display.h"
#include "keyboard.h"

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
	
	io = new IO(this, emu);
	pio = new I8255(this, emu);
	memory = new MEMORY(this, emu);
//	pcm = new PCM1BIT(this, emu);
	cpu = new Z80(this, emu);
	
	display = new DISPLAY(this, emu);
	keyboard = new KEYBOARD(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
//	event->set_context_sound(pcm);
	
//	pio->set_context_port_b(pcm, SIG_PCM1BIT_SIGNAL, 0x01, 0);
	pio->set_context_port_b(display, SIG_DISPLAY_PORT_B, 0xf0, 0);
	pio->set_context_port_c(display, SIG_DISPLAY_PORT_C, 0xf0, 0);
	pio->set_context_port_c(keyboard, SIG_KEYBOARD_PORT_C, 0xf0, 0);
	keyboard->set_context_pio(pio);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(dummy);
	
	// memory bus
	_memset(ram, 0, sizeof(ram));
	_memset(rom, 0xff, sizeof(rom));
	
	memory->read_bios(_T("MON.ROM"), rom, sizeof(rom));
	
	memory->set_memory_r(0x0000, 0x1fff, rom);
	memory->set_memory_r(0x2000, 0x3fff, rom);
	memory->set_memory_r(0x4000, 0x5fff, rom);
	memory->set_memory_r(0x6000, 0x7fff, rom);
	memory->set_memory_rw(0x8000, 0x9fff, ram);
	memory->set_memory_rw(0xa000, 0xbfff, ram);
	memory->set_memory_rw(0xc000, 0xdfff, ram);
	memory->set_memory_rw(0xe000, 0xffff, ram);
	
	// i/o bus
	io->set_iomap_range_w(0xf8, 0xfb, pio);
	io->set_iomap_range_r(0xf8, 0xfb, pio);
	
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
//	pcm->init(rate, 8000);
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::load_ram(_TCHAR* file_path)
{
	memory->read_image(file_path, ram, sizeof(ram));
}

void VM::save_ram(_TCHAR* file_path)
{
	memory->write_image(file_path, ram, sizeof(ram));
}

bool VM::now_skip()
{
	return false;
}

void VM::update_config()
{
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->update_config();
	}
}

