/*
	EPSON QC-10 Emulator 'eQC-10'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.12 -

	[ virtual machine ]
*/

#include "multi8.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../beep.h"
#include "../hd146818p.h"
#include "../i8237.h"
#include "../i8253.h"
#include "../i8255.h"
#include "../i8259.h"
#include "../io8.h"
#include "../not.h"
#include "../upd7201.h"
#include "../upd7220.h"
#include "../upd765a.h"
#include "../z80.h"

#include "memory.h"
#include "timer.h"

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
	rtc = new HD146818P(this, emu);
	dma0 = new I8237(this, emu);
	dma1 = new I8237(this, emu);
	pit0 = new I8253(this, emu);
	pit1 = new I8253(this, emu);
	pio = new I8255(this, emu);
	pic = new I8259(this, emu);
	io = new IO8(this, emu);
	not = new NOT(this, emu);
	sio = new UPD7201(this, emu);
	crtc = new UPD7220(this, emu);
	fdc = new UPD765A(this, emu);
	cpu = new Z80(this, emu);
	
	memory = new MEMORY(this, emu);
	timer = new TIMER(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(beep);
	
	dma0->set_context_memory(memory);
	dma0->set_context_ch0(fdc):
	dma0->set_context_ch0(crtc):
	dma1->set_context_memory(memory);
	pit0->set_context_ch0(not, SIG_NOT_INPUT);
	pit0->set_context_ch1(pic, SIG_I8259_IR5 | SIG_I8259_CHIP1);
	pit0->set_context_ch2(pic, SIG_I8259_IR1 | SIG_I8259_CHIP0);
	pit1->set_context_ch0(beep, SIG_BEEP_PULSE);
	pit1->set_context_ch1(pit0, SIG_I8253_CLOCK_0);
	pit1->set_context_ch1(pit0, SIG_I8253_CLOCK_1);
	not->set_context(beep, SIG_BEEP_ON, 1);
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_int(pic);
	
	timer->set_context_pit0(pit0, SIG_I8253_CLOCK_2);
	timer->set_context_pit1(pit1, SIG_I8253_CLOCK_0, SIG_I8253_CLOCK_1, SIG_I8253_CLOCK_2);
	
	io->set_iomap_range_w(0x00, 0x03, pit0);
	io->set_iomap_range_w(0x04, 0x07, pit1);
	io->set_iomap_alias_w(0x08, pic, 0);
	io->set_iomap_alias_w(0x09, pic, 1);
	io->set_iomap_alias_w(0x0c, pic, 2);
	io->set_iomap_alias_w(0x0d, pic, 3);
	io->set_iomap_range_w(0x10, 0x13, sio);
	io->set_iomap_range_w(0x14, 0x17, pio);
	io->set_iomap_range_w(0x18, 0x23, memory);
	//30-33 motor
	io->set_iomap_single_w(0x35, fdc);
	io->set_iomap_range_w(0x38, 0x3b, crtc);
	io->set_iomap_range_w(0x3c, 0x3d, rtc);
	io->set_iomap_range_w(0x40, 0x4f, dma0);
	io->set_iomap_range_w(0x50, 0x5f, dma1);
	
	io->set_iomap_range_r(0x00, 0x02, pit0);
	io->set_iomap_range_r(0x04, 0x06, pit1);
	io->set_iomap_alias_r(0x08, pic, 0);
	io->set_iomap_alias_r(0x09, pic, 1);
	io->set_iomap_alias_r(0x0c, pic, 2);
	io->set_iomap_alias_r(0x0d, pic, 3);
	io->set_iomap_range_r(0x10, 0x13, sio);
	io->set_iomap_range_r(0x14, 0x16, pio);
	io->set_iomap_range_r(0x18, 0x1b, memory);
	//2c display type
	io->set_iomap_range_r(0x30, 0x33, memory);
	io->set_iomap_range_r(0x34, 0x35, fdc);
	io->set_iomap_range_r(0x38, 0x39, crtc);
	io->set_iomap_single_r(0x3c, rtc);
	io->set_iomap_range_r(0x40, 0x4f, dma0);
	io->set_iomap_range_r(0x50, 0x5f, dma1);
	
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
//	crtc->draw_screen();
}

// ----------------------------------------------------------------------------
// soud manager
// ----------------------------------------------------------------------------

void VM::initialize_sound(int rate, int samples)
{
	// init sound manager
	event->initialize_sound(rate, samples);
	
	// init sound gen
//	opn->init(rate, 3579545, samples);
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

bool VM::now_skip()
{
	return false;
}

void VM::update_config()
{
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->update_config();
}

