/*
	Japan Electronics College MYCOMZ-80A Emulator 'eMYCOMZ-80A'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.05.13-

	[ virtual machine ]
*/

#include "mycomz80a.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../datarec.h"
#include "../hd46505.h"
#include "../i8255.h"
#include "../io.h"
#include "../msm5832.h"
#include "../sn76489an.h"
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
	crtc = new HD46505(this, emu);
	pio1 = new I8255(this, emu);
	pio2 = new I8255(this, emu);
	pio3 = new I8255(this, emu);
	io = new IO(this, emu);
	rtc = new MSM5832(this, emu);
	psg = new SN76489AN(this, emu);
	cpu = new Z80(this, emu);
	
	display = new DISPLAY(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(psg);
	
	//	00	out	system control
	//	01	in/out	vram data
	//	02	out	crtc addr
	//	03	in/out	crtc data
	//	04-07	in/ou	pio1
	//		pa0-7	out	vram addr low, psg data
	//		pb0-7	in	keyboard data
	//		pc0-2	out	vram addr high
	//		pc3	out	fdc format write
	//		pc4	in	keyboard s2
	//		pc5	in	keyboard s3
	//		pc6	in	keyboard s4 (motor on/off)
	//		pc7	in	keyboard s5 (slow)
	//	08-0b	in/ou	pio2
	//		pa0	in	keyboard strobe (1=pressed)
	//		pa1	in	keyboard shift (1=pressed)
	//		pa2	in	cmt in
	//		pa3-6	in	printer ctrl
	//		pa7	in	crtc disptmg
	//		pb0-7	out	printer data
	//		pc0	out	printer strobe
	//		pc1	out	printer reset
	//		pc2	out	cmt out
	//		pc3	out	cmt remote
	//		pc4	out	psg we
	//		pc5	out	psg cs
	//		pc6	out	display chr/cg (0=chr,1=cg)
	//		pc7	out	display freq (0=80column,1=40column)
	//	0c-0f	in/ou	pio3
	//		pa0-6	in	fdc control
	//		pb0-3	in/out	rtc data
	//		pc0-3	out	rtc address
	//		pc4	out	rtc hold
	//		pc5	out	rtc rd
	//		pc6	out	rtc wr
	//		pc7	out	rtc cs
	drec->set_context_out(pio2, SIG_I8255_PORT_A, 4);
	crtc->set_context_disp(pio2, SIG_I8255_PORT_A, 0x80);
	pio1->set_context_port_a(display, SIG_DISPLAY_ADDR_L, 0xff, 0);
	pio1->set_context_port_a(psg, SIG_SN76489AN_DATA, 0xff, 0);
	pio1->set_context_port_c(display, SIG_DISPLAY_ADDR_H, 7, 0);
	pio2->set_context_port_c(drec, SIG_DATAREC_OUT, 4, 0);
	pio2->set_context_port_c(drec, SIG_DATAREC_REMOTE, 8, 0);
	pio2->set_context_port_c(psg, SIG_SN76489AN_WE, 0x10, 0);
	pio2->set_context_port_c(psg, SIG_SN76489AN_CS, 0x20, 0);
	pio2->set_context_port_c(display, SIG_DISPLAY_MODE, 0xc0, 0);
	pio3->set_context_port_b(rtc, SIG_MSM5832_DATA, 0xf, 0);
	pio3->set_context_port_c(rtc, SIG_MSM5832_ADDR, 0xf, 0);
	pio3->set_context_port_c(rtc, SIG_MSM5832_HOLD, 0x10, 0);
	pio3->set_context_port_c(rtc, SIG_MSM5832_READ, 0x20, 0);
	pio3->set_context_port_c(rtc, SIG_MSM5832_WRITE, 0x40, 0);
	pio3->set_context_port_c(rtc, SIG_MSM5832_CS, 0x80, 0);
	rtc->set_context_data(pio3, SIG_I8255_PORT_B, 0xf, 0);
	
	display->set_regs_ptr(crtc->get_regs());
	keyboard->set_context_cpu(cpu);
	keyboard->set_context_pio1(pio1);
	keyboard->set_context_pio2(pio2);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(dummy);
	
	// i/o bus
	io->set_iomap_single_w(0x0, memory);
	io->set_iomap_single_w(0x1, display);
	io->set_iomap_range_w(0x2, 0x3, crtc);
	io->set_iomap_range_w(0x4, 0x7, pio1);
	io->set_iomap_range_w(0x8, 0xb, pio2);
	io->set_iomap_range_w(0xc, 0xf, pio3);
	
	io->set_iomap_single_r(0x1, display);
	io->set_iomap_range_r(0x2, 0x3, crtc);
	io->set_iomap_range_r(0x4, 0x6, pio1);
	io->set_iomap_range_r(0x8, 0xa, pio2);
	io->set_iomap_range_r(0xc, 0xe, pio3);
	
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

void VM::register_event(DEVICE* dev, int event_id, int usec, bool loop, int* register_id)
{
	event->register_event(dev, event_id, usec, loop, register_id);
}

void VM::register_event_by_clock(DEVICE* dev, int event_id, int clock, bool loop, int* register_id)
{
	event->register_event_by_clock(dev, event_id, clock, loop, register_id);
}

void VM::cancel_event(int register_id)
{
	event->cancel_event(register_id);
}

void VM::register_frame_event(DEVICE* dev)
{
	event->register_frame_event(dev);
}

void VM::register_vline_event(DEVICE* dev)
{
	event->register_vline_event(dev);
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
	psg->init(rate, 2500800, 10000);
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
}

// ----------------------------------------------------------------------------
// notify key
// ----------------------------------------------------------------------------

void VM::key_down(int code, bool repeat)
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

void VM::play_datarec(_TCHAR* filename)
{
	drec->play_datarec(filename);
//	drec->write_signal(SIG_DATAREC_REMOTE, 1, 1);
}

void VM::rec_datarec(_TCHAR* filename)
{
	drec->rec_datarec(filename);
//	drec->write_signal(SIG_DATAREC_REMOTE, 1, 1);
}

void VM::close_datarec()
{
	drec->close_datarec();
//	drec->write_signal(SIG_DATAREC_REMOTE, 0, 1);
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

