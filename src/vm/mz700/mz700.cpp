/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.05 -

	[ virtual machine ]
*/

#include "mz700.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../datarec.h"
#include "../i8253.h"
#include "../i8255.h"
#include "../io.h"
#include "../pcm1bit.h"
#include "../z80.h"

#include "display.h"
#include "interrupt.h"
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
	ctc = new I8253(this, emu);
	pio = new I8255(this, emu);
	io = new IO(this, emu);
	pcm0 = new PCM1BIT(this, emu);
//	pcm1 = new PCM1BIT(this, emu);
	cpu = new Z80(this, emu);
	
	display = new DISPLAY(this, emu);
	interrupt = new INTERRUPT(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(pcm0);
//	event->set_context_sound(pcm1);
	
	drec->set_context_out(pio, SIG_I8255_PORT_C, 0x20);
//	drec->set_context_out(pcm1, SIG_PCM1BIT_SIGNAL, 1);
	drec->set_context_remote(pio, SIG_I8255_PORT_C, 0x10);
	ctc->set_context_ch0(pcm0, SIG_PCM1BIT_SIGNAL, 1);
	ctc->set_context_ch1(ctc, SIG_I8253_CLOCK_2, 1);
	ctc->set_context_ch2(interrupt, SIG_INTERRUPT_CLOCK, 1);
	ctc->set_constant_clock(0, CPU_CLOCKS >> 2);
	ctc->set_constant_clock(1, 16000);
	pio->set_context_port_a(keyboard, SIG_KEYBOARD_COLUMN, 0x7f, 0);
	pio->set_context_port_c(drec, SIG_DATAREC_OUT, 2, 0);
	pio->set_context_port_c(interrupt, SIG_INTERRUPT_INTMASK, 4, 0);
	pio->set_context_port_c(drec, SIG_DATAREC_TRIG, 8, 0);
	// pc3: motor rotate control
	
	display->set_vram_ptr(memory->get_vram());
	interrupt->set_context_cpu(cpu);
	keyboard->set_context_pio(pio, SIG_I8255_PORT_B);
	memory->set_context_cpu(cpu);
	memory->set_context_ctc(ctc, SIG_I8253_GATE_0);
	memory->set_context_pio(pio, SIG_I8255_PORT_C);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
//	cpu->set_context_io(memory);
	cpu->set_context_intr(interrupt);
	
	// i/o bus
	io->set_iomap_range_r(0, 3, memory);	// EMM
	io->set_iomap_range_w(0, 3, memory);	// EMM
	io->set_iomap_range_w(0xe0, 0xe6, memory);
	
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
//	pcm1->init(rate, 2000);	// data recorder noise
}

uint16* VM::create_sound(int samples, bool fill)
{
	return event->create_sound(samples, fill);
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
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->update_config();
}

