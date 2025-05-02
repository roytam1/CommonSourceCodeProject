/*
	SHARP MZ-80K Emulator 'EmuZ-80K'
	SHARP MZ-1200 Emulator 'EmuZ-1200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.18-

	[ virtual machine ]
*/

#include "mz80k.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#ifdef _MZ1200
#include "../and.h"
#endif
#include "../datarec.h"
#include "../i8253.h"
#include "../i8255.h"
#include "../io.h"
#include "../ls393.h"
#include "../pcm1bit.h"
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
	
#ifdef _MZ1200
	and = new AND(this, emu);
#endif
	drec = new DATAREC(this, emu);
	ctc = new I8253(this, emu);
	pio = new I8255(this, emu);
	io = new IO(this, emu);
	counter = new LS393(this, emu);
	pcm = new PCM1BIT(this, emu);
	cpu = new Z80(this, emu);
	
	display = new DISPLAY(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(pcm);
	
#ifdef _MZ1200
	and->set_context_out(cpu, SIG_CPU_IRQ, 1);
	and->set_mask(SIG_AND_BIT_0 | SIG_AND_BIT_1);
#endif
	drec->set_context_out(pio, SIG_I8255_PORT_C, 0x20);
	drec->set_context_remote(pio, SIG_I8255_PORT_C, 0x10);
	ctc->set_context_ch0(counter, SIG_LS393_CLK, 1);
	ctc->set_context_ch1(ctc, SIG_I8253_CLOCK_2, 1);
#ifdef _MZ1200
	ctc->set_context_ch2(and, SIG_AND_BIT_0, 1);
#else
	ctc->set_context_ch2(cpu, SIG_CPU_IRQ, 1);
#endif
	ctc->set_constant_clock(0, 2000000);
	ctc->set_constant_clock(1, 31250);
	pio->set_context_port_a(keyboard, SIG_KEYBOARD_COLUMN, 0x0f, 0);
	pio->set_context_port_c(display, SIG_DISPLAY_VGATE, 1, 0);
	pio->set_context_port_c(drec, SIG_DATAREC_OUT, 2, 0);
#ifdef _MZ1200
	pio->set_context_port_c(and, SIG_AND_BIT_1, 4, 0);
#endif
	pio->set_context_port_c(drec, SIG_DATAREC_TRIG, 8, 0);
	counter->set_context_1qa(pcm, SIG_PCM1BIT_SIGNAL, 1);
	
	display->set_vram_ptr(memory->get_vram());
	keyboard->set_context_pio(pio);
	memory->set_context_ctc(ctc);
	memory->set_context_pio(pio);
#ifdef _MZ1200
	memory->set_context_disp(display);
#endif
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(dummy);
	
	// i/o bus
	
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
#ifdef _MZ1200
	and->write_signal(SIG_AND_BIT_0, 0, 1);	// CLOCK = L
	and->write_signal(SIG_AND_BIT_1, 1, 1);	// INTMASK = H
#endif
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

void VM::set_pc(uint32 pc)
{
	cpu->set_pc(pc);
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
	pcm->init(rate, 8000);
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::open_mzt(_TCHAR* filename)
{
	memory->open_mzt(filename);
}

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

void VM::push_play()
{
	drec->write_signal(SIG_DATAREC_REMOTE, 1, 1);
}

void VM::push_stop()
{
	drec->write_signal(SIG_DATAREC_REMOTE, 0, 0);
}

bool VM::now_skip()
{
#ifdef _TINYIMAS
	return false;
#else
	return drec->skip();
#endif
}

void VM::update_config()
{
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->update_config();
	}
}

