/*
	TOSHIBA PASOPIA Emulator 'EmuPIA'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.12.28 -

	[ virtual machine ]
*/

#include "pasopia.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../beep.h"
#include "../datarec.h"
#include "../hd46505.h"
#include "../i8255.h"
#include "../io.h"
#include "../not.h"
#include "../z80.h"
#include "../z80ctc.h"
#include "../z80pio.h"

#include "display.h"
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
	io = new IO(this, emu);
	not = new NOT(this, emu);
	cpu = new Z80(this, emu);
	ctc = new Z80CTC(this, emu);
	pio = new Z80PIO(this, emu);
	
	display = new DISPLAY(this, emu);
	key = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	pac2 = new PAC2(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(beep);
	
	drec->set_context_out(pio2, SIG_I8255_PORT_B, 0x20);
	crtc->set_context_disp(pio1, SIG_I8255_PORT_B, 8);
	crtc->set_context_vsync(pio1, SIG_I8255_PORT_B, 0x20);
	crtc->set_context_hsync(pio1, SIG_I8255_PORT_B, 0x40);
	pio0->set_context_port_a(memory, SIG_MEMORY_I8255_0_A, 0xff, 0);
	pio0->set_context_port_b(memory, SIG_MEMORY_I8255_0_B, 0xff, 0);
	pio1->set_context_port_a(display, SIG_DISPLAY_I8255_1_A, 0xff, 0);
	pio1->set_context_port_c(memory, SIG_MEMORY_I8255_1_C, 0xff, 0);
	pio2->set_context_port_a(beep, SIG_BEEP_MUTE, 0x2, 0);
	pio2->set_context_port_a(drec, SIG_DATAREC_OUT, 0x10, 0);
	pio2->set_context_port_a(not, SIG_NOT_INPUT, 0x20, 0);
	not->set_context(drec, SIG_DATAREC_REMOTE, 1);
	ctc->set_context_zc0(ctc, SIG_Z80CTC_TRIG_1);
	ctc->set_context_zc1(beep, SIG_BEEP_PULSE);
	ctc->set_context_zc2(ctc, SIG_Z80CTC_TRIG_3);
	ctc->set_constant_clock(0, CPU_CLOCKS);
	ctc->set_constant_clock(2, CPU_CLOCKS);
	pio->set_context_port_a(beep, SIG_BEEP_ON, 0x80, 0);
	pio->set_context_port_a(key, SIG_KEYBOARD_Z80PIO_A, 0xff, 0);
	display->set_context(crtc);
	display->set_vram_ptr(memory->get_vram());
	display->set_attr_ptr(memory->get_attr());
	display->set_regs_ptr(crtc->get_regs());
	key->set_context(pio, SIG_Z80PIO_PORT_B);
	memory->set_context_pio0(pio0, SIG_I8255_PORT_C);
	memory->set_context_pio1(pio1, SIG_I8255_PORT_B);
	memory->set_context_pio2(pio2, SIG_I8255_PORT_C);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(pio);
	
	// z80 family daisy chain
	pio->set_context_intr(cpu, 0);
	pio->set_context_child(ctc);
	ctc->set_context_intr(cpu, 1);
/*
pio0	0	8255	vram laddr
	1	8255	vram data
	2	8255
	3	8255
pio1	8	8255	crtc mode
	9	8255	
	a	8255	vram addr, attrib & strobe
	b	8255
	10	crtc	index
	11	crtc	regs
	18	pac2
	19	pac2
	1a	pac2
	1b	pac2
pio2	20	8255	out cmt, sound
	21	8255	in cmt
	22	8255	in memory map
	23	8255
	28	z80ctc
	29	z80ctc
	2a	z80ctc
	2b	z80ctc
	30	z80pio
	31	z80pio
	32	z80pio
	33	z80pio
	38	printer
	3c	memory	out memory map
*/
	// i/o bus
	io->set_iomap_range_w(0x00, 0x03, pio0);
	io->set_iomap_range_w(0x08, 0x0b, pio1);
	io->set_iomap_range_w(0x10, 0x11, display);
	io->set_iomap_range_w(0x18, 0x1b, pac2);
	io->set_iomap_range_w(0x20, 0x23, pio2);
	io->set_iomap_range_w(0x28, 0x2b, ctc);
	io->set_iomap_alias_w(0x30, pio, 0);
	io->set_iomap_alias_w(0x31, pio, 2);
	io->set_iomap_alias_w(0x32, pio, 1);
	io->set_iomap_alias_w(0x33, pio, 3);
	io->set_iomap_single_w(0x3c, memory);
	
	io->set_iomap_range_r(0x00, 0x02, pio0);
	io->set_iomap_range_r(0x08, 0x0a, pio1);
	io->set_iomap_range_r(0x10, 0x11, crtc);
	io->set_iomap_range_r(0x18, 0x1b, pac2);
	io->set_iomap_range_r(0x20, 0x22, pio2);
	io->set_iomap_range_r(0x28, 0x2b, ctc);
	io->set_iomap_alias_r(0x30, pio, 0);
	io->set_iomap_alias_r(0x31, pio, 2);
	
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
	pio1->write_signal(SIG_I8255_PORT_B, 0, 0x10);
#else
	pio1->write_signal(SIG_I8255_PORT_B, 0xffffffff, 0x10);
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
	beep->init(rate, -1, 2, 8000);
}

uint16* VM::create_sound(int samples, bool fill)
{
	return event->create_sound(samples, fill);
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
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->update_config();
}

