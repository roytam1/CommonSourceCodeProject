/*
	TOSHIBA PASOPIA 7 Emulator 'EmuPIA7'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.20 -

	[ virtual machine ]
*/

#include "pasopia7.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../beep.h"
#include "../datarec.h"
#include "../hd46505.h"
#include "../i8255.h"
#include "../not.h"
#include "../sn76489an.h"
#include "../upd765a.h"
#include "../z80.h"
#include "../z80ctc.h"
#include "../z80pio.h"

#include "floppy.h"
#include "display.h"
#include "io8.h"
#include "iotrap.h"
#include "keyboard.h"
#include "memory.h"
#include "pac2.h"

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
	drec = new DATAREC(this, emu);
	crtc = new HD46505(this, emu);
	pio0 = new I8255(this, emu);
	pio1 = new I8255(this, emu);
	pio2 = new I8255(this, emu);
	not = new NOT(this, emu);
	psg0 = new SN76489AN(this, emu);
	psg1 = new SN76489AN(this, emu);
	fdc = new UPD765A(this, emu);
	cpu = new Z80(this, emu);
	ctc = new Z80CTC(this, emu);
	pio = new Z80PIO(this, emu);
	
	floppy = new FLOPPY(this, emu);
	display = new DISPLAY(this, emu);
	io = new IO8(this, emu);
	iotrap = new IOTRAP(this, emu);
	key = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	pac2 = new PAC2(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(beep);
	event->set_context_sound(psg0);
	event->set_context_sound(psg1);
	
	drec->set_context_out(pio2, SIG_I8255_PORT_B, 0x20);
	crtc->set_context_disp(pio0, SIG_I8255_PORT_B, 8);
	crtc->set_context_vsync(pio0, SIG_I8255_PORT_B, 0x20);
	pio0->set_context_port_a(display, SIG_DISPLAY_I8255_0_A, 0xff, 0);
	pio1->set_context_port_a(memory, SIG_MEMORY_I8255_1_A, 0xff, 0);
	pio1->set_context_port_b(display, SIG_DISPLAY_I8255_1_B, 0xff, 0);
	pio1->set_context_port_b(memory, SIG_MEMORY_I8255_1_B, 0xff, 0);
	pio1->set_context_port_c(display, SIG_DISPLAY_I8255_1_C, 0xff, 0);
	pio1->set_context_port_c(memory, SIG_MEMORY_I8255_1_C, 0xff, 0);
	pio2->set_context_port_a(beep, SIG_BEEP_MUTE, 0x2, 0);
	pio2->set_context_port_a(psg0, SIG_SN76489AN_MUTE, 0x2, 0);
	pio2->set_context_port_a(psg1, SIG_SN76489AN_MUTE, 0x2, 0);
	pio2->set_context_port_a(drec, SIG_DATAREC_OUT, 0x10, 0);
	pio2->set_context_port_a(not, SIG_NOT_INPUT, 0x20, 0);
	pio2->set_context_port_a(iotrap, SIG_IOTRAP_I8255_2_A, 0xff, 0);
	pio2->set_context_port_c(iotrap, SIG_IOTRAP_I8255_2_C, 0xff, 0);
	not->set_context(drec, SIG_DATAREC_REMOTE, 1);
	fdc->set_context_intr(floppy, SIG_FLOPPY_INTR, 1);
	ctc->set_context_zc0(ctc, SIG_Z80CTC_TRIG_1);
	ctc->set_context_zc1(beep, SIG_BEEP_PULSE);
	ctc->set_context_zc2(ctc, SIG_Z80CTC_TRIG_3);
	ctc->set_constant_clock(0, CPU_CLOCKS);
	ctc->set_constant_clock(2, CPU_CLOCKS);
	pio->set_context_port_a(beep, SIG_BEEP_ON, 0x80, 0);
	pio->set_context_port_a(key, SIG_KEYBOARD_Z80PIO_A, 0xff, 0);
	
	display->set_context(fdc);
	display->set_vram_ptr(memory->get_vram());
	display->set_pal_ptr(memory->get_pal());
	display->set_regs_ptr(crtc->get_regs());
	floppy->set_context(fdc, SIG_UPD765A_TC, SIG_UPD765A_MOTOR);
	io->set_ram_ptr(memory->get_ram());
	iotrap->set_context_cpu(cpu);
	iotrap->set_context_pio2(pio2, SIG_I8255_PORT_B);
	key->set_context(pio, SIG_Z80PIO_PORT_B);
	memory->set_context_io(io, SIG_IO8_MIO);
	memory->set_context_pio0(pio0, SIG_I8255_PORT_B);
	memory->set_context_pio2(pio2, SIG_I8255_PORT_C);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(pio);
	
	// z80 family daisy chain
	pio->set_context_intr(cpu, 0);
	pio->set_context_child(ctc);
	ctc->set_context_intr(cpu, 1);
	
	// i/o bus
	io->set_iomap_range_w(0x08, 0x0b, pio0);
	io->set_iomap_range_w(0x0c, 0x0f, pio1);
	io->set_iomap_range_w(0x10, 0x11, crtc);
	io->set_iomap_range_w(0x18, 0x1b, pac2);
	io->set_iomap_range_w(0x20, 0x23, pio2);
	io->set_iomap_range_w(0x28, 0x2b, ctc);
	io->set_iomap_alias_w(0x30, pio, 0);
	io->set_iomap_alias_w(0x31, pio, 2);
	io->set_iomap_alias_w(0x32, pio, 1);
	io->set_iomap_alias_w(0x33, pio, 3);
	io->set_iomap_single_w(0x3a, psg0);
	io->set_iomap_single_w(0x3b, psg1);
	io->set_iomap_single_w(0x3c, memory);
	io->set_iomap_single_w(0xe0, floppy);
	io->set_iomap_single_w(0xe2, floppy);
	io->set_iomap_single_w(0xe5, fdc);
	io->set_iomap_single_w(0xe6, floppy);
	
	io->set_iomap_range_r(0x08, 0x0a, pio0);
	io->set_iomap_range_r(0x0c, 0x0e, pio1);
	io->set_iomap_single_r(0x11, crtc);
	io->set_iomap_range_r(0x18, 0x1b, pac2);
	io->set_iomap_range_r(0x20, 0x22, pio2);
	io->set_iomap_range_r(0x28, 0x2b, ctc);
	io->set_iomap_alias_r(0x30, pio, 0);
	io->set_iomap_alias_r(0x31, pio, 2);
	io->set_iomap_range_r(0xe4, 0xe5, fdc);
	io->set_iomap_single_r(0xe6, floppy);
	
	// initialize and reset all devices except the event manager
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id)
			device->initialize();
	}
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id)
			device->reset();
	}
	
	// set initial port status
#ifdef _LCD
	pio0->write_signal(SIG_I8255_PORT_B, 0, (0x10 | 0x40));
#else
	pio0->write_signal(SIG_I8255_PORT_B, 0x10, (0x10 | 0x40));
#endif
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
//	for(DEVICE* device = first_device; device; device = device->next_device)
//		device->reset();
	event->reset();
	memory->reset();
	iotrap->do_reset();
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
	return (current > prev) ? current - prev : current + (uint32)~prev + 1;
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
	beep->init(rate, -1, 2, 3600);
	psg0->init(rate, 1996800, 3600);
	psg1->init(rate, 1996800, 3600);
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
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->update_config();
}

