/*
	NEC TK-80BS (COMPO BS/80) Emulator 'eTK-80BS'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.08.26 -

	[ virtual machine ]
*/

#include "tk80bs.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../i8080.h"
#include "../i8251.h"
#include "../i8255.h"
#include "../pcm1bit.h"

#include "cmt.h"
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
	
	sio_b = new I8251(this, emu);	// on TK-80BS
	pio_b = new I8255(this, emu);
	pio_t = new I8255(this, emu);	// on TK-80
	pcm0 = new PCM1BIT(this, emu);
	pcm1 = new PCM1BIT(this, emu);
	cpu = new I8080(this, emu);
	
	cmt = new CMT(this, emu);
	display = new DISPLAY(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(pcm0);
	event->set_context_sound(pcm1);
	
/*	8255 on TK-80
	
	PA	key matrix
	PB0	serial in
	PC0	serial out
	PC1	sound #1
	PC2	sound #2
	PC4-6	key column
	PC7	dma disable
*/
	sio_b->set_context_out(cmt, SIG_CMT_OUT);
	pio_b->set_context_port_c(display, SIG_DISPLAY_MODE, 3, 0);
	pio_t->set_context_port_c(pcm0, SIG_PCM1BIT_SIGNAL, 2, 0);
	pio_t->set_context_port_c(pcm1, SIG_PCM1BIT_SIGNAL, 4, 0);
	pio_t->set_context_port_c(keyboard, SIG_KEYBOARD_COLUMN, 0x70, 0);
	pio_t->set_context_port_c(display, SIG_DISPLAY_DMA, 0x80, 0);
	
	cmt->set_context_sio(sio_b, SIG_I8251_RECV, SIG_I8251_CLEAR);
	display->set_context_key(keyboard);
	display->set_vram_ptr(memory->get_vram());
	display->set_led_ptr(memory->get_led());
	keyboard->set_context_pio_b(pio_b, SIG_I8255_PORT_A);
	keyboard->set_context_pio_t(pio_t, SIG_I8255_PORT_A);
	keyboard->set_context_cpu(cpu);
	memory->set_context_sio(sio_b);
	memory->set_context_pio(pio_b);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(pio_t);
	cpu->set_context_intr(keyboard);
	
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
	
	// init 8255 on TK-80
	pio_t->write_io8(0xfb, 0x92);
	pio_t->write_signal(SIG_I8255_PORT_A, 0xff, 0xff);
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
	pcm0->init(rate, 8000);
	pcm1->init(rate, 8000);
}

uint16* VM::create_sound(int samples, bool fill)
{
	return event->create_sound(samples, fill);
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
	keyboard->key_up(code);
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

void VM::play_datarec(_TCHAR* filename)
{
	cmt->play_datarec(filename);
}

void VM::rec_datarec(_TCHAR* filename)
{
	cmt->rec_datarec(filename);
}

void VM::close_datarec()
{
	cmt->close_datarec();
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

