/*
	MITSUBISHI Electric MULTI8 Emulator 'EmuLTI8'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.15 -

	[ virtual machine ]
*/

#include "multi8.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../hd46505.h"
#include "../i8251.h"
#include "../i8253.h"
#include "../i8255.h"
#include "../i8259.h"
#include "../io.h"
#include "../upd765a.h"
#include "../ym2203.h"
#include "../z80.h"

#include "cmt.h"
#include "display.h"
#include "floppy.h"
#include "kanji.h"
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
	
	crtc = new HD46505(this, emu);
	sio = new I8251(this, emu);
	pit = new I8253(this, emu);
	pio = new I8255(this, emu);
	pic = new I8259(this, emu);
	io = new IO(this, emu);
	fdc = new UPD765A(this, emu);
	opn = new YM2203(this, emu);
	cpu = new Z80(this, emu);
	
	cmt = new CMT(this, emu);
	display = new DISPLAY(this, emu);
	floppy = new FLOPPY(this, emu);
	kanji = new KANJI(this, emu);
	key = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(opn);
	
	crtc->set_context_vsync(pio, SIG_I8255_PORT_A, 0x20);
	sio->set_context_out(cmt, SIG_CMT_OUT);
	pit->set_context_ch1(pit, SIG_I8253_CLOCK_2, 1);
	pit->set_context_ch1(pic, SIG_I8259_CHIP0 | SIG_I8259_IR5, 1);
	pit->set_context_ch2(pic, SIG_I8259_CHIP0 | SIG_I8259_IR6, 1);
	pit->set_constant_clock(0, CPU_CLOCKS >> 1);
	pit->set_constant_clock(1, CPU_CLOCKS >> 1);
	pio->set_context_port_b(display, SIG_DISPLAY_I8255_B, 0xff, 0);
	pio->set_context_port_c(memory, SIG_MEMORY_I8255_C, 0xff, 0);
	pic->set_context(cpu);
	fdc->set_context_irq(pic, SIG_I8259_CHIP0 | SIG_I8259_IR0, 1);
	fdc->set_context_drq(floppy, SIG_FLOPPY_DRQ, 1);
	opn->set_context_port_a(cmt, SIG_CMT_REMOTE, 0x2, 0);
	opn->set_context_port_a(pio, SIG_I8255_PORT_A, 0x2, 1);
	
	cmt->set_context_sio(sio);
	display->set_context_fdc(fdc);
	display->set_vram_ptr(memory->get_vram());
	display->set_regs_ptr(crtc->get_regs());
	floppy->set_context_fdc(fdc);
	kanji->set_context_pio(pio);
	memory->set_context_pio(pio);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(pic);
	
	// i/o bus
	io->set_iomap_range_w(0x00, 0x01, key);
	io->set_iomap_alias_w(0x18, opn, 0);
	io->set_iomap_alias_w(0x19, opn, 1);
	io->set_iomap_range_w(0x1c, 0x1d, crtc);
	io->set_iomap_range_w(0x20, 0x21, sio);
	io->set_iomap_range_w(0x24, 0x27, pit);
	io->set_iomap_range_w(0x28, 0x2b, pio);
	io->set_iomap_alias_w(0x2c, pic, 0);
	io->set_iomap_alias_w(0x2d, pic, 1);
	io->set_iomap_range_w(0x30, 0x37, display);
	io->set_iomap_range_w(0x40, 0x41, kanji);
	io->set_iomap_range_w(0x71, 0x74, floppy);
	io->set_iomap_single_w(0x78, memory);
	
	io->set_iomap_range_r(0x00, 0x01, key);
	io->set_iomap_range_r(0x1c, 0x1d, crtc);
	io->set_iomap_alias_r(0x18, opn, 0);
	io->set_iomap_alias_r(0x1a, opn, 1);
	io->set_iomap_range_r(0x20, 0x21, sio);
	io->set_iomap_range_r(0x24, 0x27, pit);
	io->set_iomap_range_r(0x28, 0x2a, pio);
	io->set_iomap_alias_r(0x2c, pic, 0);
	io->set_iomap_alias_r(0x2d, pic, 1);
	io->set_iomap_range_r(0x30, 0x37, display);
	io->set_iomap_range_r(0x40, 0x41, kanji);
	io->set_iomap_range_r(0x70, 0x73, floppy);
	
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
	opn->init(rate, 3579545, samples, 0, 0);
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
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

