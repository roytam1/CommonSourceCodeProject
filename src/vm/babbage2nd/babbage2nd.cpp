/*
	Gijutsu-Hyoron-Sha Babbage-2nd Emulator 'eBabbage-2nd'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.12.26 -

	[ virtual machine ]
*/

#include "babbage2nd.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../io.h"
#include "../z80.h"
#include "../z80ctc.h"
#include "../z80pio.h"

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
	
	io = new IO(this, emu);
	cpu = new Z80(this, emu);
	ctc = new Z80CTC(this, emu);
	pio1 = new Z80PIO(this, emu);
	pio2 = new Z80PIO(this, emu);
	
	display = new DISPLAY(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	
	pio2->set_context_port_b(display, SIG_DISPLAY_7SEG_LED, 0xff, 0);
	keyboard->set_context_pio(pio2);
	// p.145, fig.3-4
	ctc->set_context_zc2(ctc, SIG_Z80CTC_TRIG_1, 1);
	ctc->set_context_zc1(ctc, SIG_Z80CTC_TRIG_0, 1);
	// p.114, fig.2-52
	pio1->set_context_port_b(display, SIG_DISPLAY_8BIT_LED, 0xff, 0);
	//pio1->set_context_port_b(pio1, SIG_Z80PIO_PORT_A, 0xff, 0);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(ctc);
	
	// z80 family daisy chain
	ctc->set_context_intr(cpu, 0);
	ctc->set_context_child(pio1);
	pio1->set_context_intr(cpu, 1);
	pio1->set_context_child(pio2);
	pio2->set_context_intr(cpu, 2);
	
	// i/o bus
	io->set_iomap_range_w(0x00, 0x03, ctc);
	io->set_iomap_range_w(0x10, 0x13, pio1);
	io->set_iomap_range_w(0x20, 0x23, pio2);
	
	io->set_iomap_range_r(0x00, 0x03, ctc);
	io->set_iomap_range_r(0x10, 0x13, pio1);
	io->set_iomap_range_r(0x20, 0x23, pio2);
	
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
	keyboard->key_down(code);
}

void VM::key_up(int code)
{
	//keyboard->key_up(code);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::load_ram(_TCHAR* filename)
{
	memory->load_ram(filename);
}

void VM::save_ram(_TCHAR* filename)
{
	memory->save_ram(filename);
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

