/*
	SHARP MZ-5500 Emulator 'EmuZ-5500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.10 -

	[ virtual machine ]
*/

#include "mz5500.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../i8237.h"
#include "../i8255.h"
#include "../i8259.h"
#include "../i86.h"
#include "../io.h"
#include "../ls393.h"
#include "../rp5c01.h"
#include "../upd7220.h"
#include "../upd765a.h"
#include "../ym2203.h"
#include "../z80ctc.h"
#include "../z80sio.h"

#include "display.h"
#include "keyboard.h"
#include "memory.h"
#include "sysport.h"

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
	
	dma = new I8237(this, emu);
	pio = new I8255(this, emu);
	pic = new I8259(this, emu);
	cpu = new I86(this, emu);
	io = new IO(this, emu);
	div = new LS393(this, emu);
	rtc = new RP5C01(this, emu);
	gdc = new UPD7220(this, emu);
	fdc = new UPD765A(this, emu);
	psg = new YM2203(this, emu);	// AY-3-8912
	ctc0 = new Z80CTC(this, emu);
#if defined(_MZ6500) || defined(_MZ6550)
	ctc1 = new Z80CTC(this, emu);
#endif
	sio = new Z80SIO(this, emu);
	
	display = new DISPLAY(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	sysport = new SYSPORT(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(psg);
	
	dma->set_context_memory(memory);
	dma->set_context_ch1(fdc);
	pio->set_context_port_c(keyboard, SIG_KEYBOARD_INPUT, 3, 0);
	pic->set_context_cpu(cpu);
	div->set_context_2qb(ctc0, SIG_Z80CTC_TRIG_3, 1);
#if defined(_MZ6500) || defined(_MZ6550)
	div->set_context_1qb(ctc1, SIG_Z80CTC_TRIG_0, 1);
	div->set_context_2qb(ctc1, SIG_Z80CTC_TRIG_1, 1);
	div->set_context_2qb(ctc1, SIG_Z80CTC_TRIG_2, 1);
	div->set_context_2qd(ctc1, SIG_Z80CTC_TRIG_3, 1);
#endif
	rtc->set_context_alarm(pic, SIG_I8259_IR0 | SIG_I8259_CHIP1, 1);
	gdc->set_vram_ptr(memory->get_vram(), 0x80000);
	gdc->set_context_vsync(pic, SIG_I8259_IR0 | SIG_I8259_CHIP0, 1);
	fdc->set_context_irq(pic, SIG_I8259_IR1 | SIG_I8259_CHIP1, 1);
	fdc->set_context_drq(dma, SIG_I8237_CH1, 1);
	psg->set_context_port_a(pic, SIG_I8259_IR7 | SIG_I8259_CHIP0, 0x20, 0);
	psg->set_context_port_a(pic, SIG_I8259_IR7 | SIG_I8259_CHIP1, 0x40, 0);
	psg->set_context_port_a(memory, SIG_MEMORY_BANK, 0xe0, 0);
	ctc0->set_context_intr(pic, SIG_I8259_IR5 | SIG_I8259_CHIP0);
	ctc0->set_context_zc0(div, SIG_LS393_CLK, 1);
#if defined(_MZ6500) || defined(_MZ6550)
	ctc0->set_context_child(ctc1);
	ctc1->set_context_intr(pic, SIG_I8259_IR5 | SIG_I8259_CHIP0);
#endif
	sio->set_context_intr(pic, SIG_I8259_IR1 | SIG_I8259_CHIP0);
	
	display->set_vram_ptr(memory->get_vram());
	display->set_sync_ptr(gdc->get_sync());
	display->set_ra_ptr(gdc->get_ra());
	display->set_cs_ptr(gdc->get_cs());
	display->set_ead_ptr(gdc->get_ead());
	keyboard->set_context_pio(pio);
	keyboard->set_context_pic(pic);
	memory->set_context_cpu(cpu);
	sysport->set_context_fdc(fdc);
	sysport->set_context_ctc(ctc0);
	sysport->set_context_sio(sio);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(pic);
#ifdef SINGLE_MODE_DMA
	cpu->set_context_dma(dma);
#endif
	
	// i/o bus
	io->set_iomap_range_rw(0x00, 0x0f, dma);
	io->set_iomap_range_rw(0x10, 0x1f, pio);
	io->set_iomap_range_rw(0x20, 0x2f, fdc);
	for(int i = 0x30; i < 0x40; i += 2) {
		io->set_iomap_alias_rw(i, pic, I8259_ADDR_CHIP0 | ((i >> 1) & 1));
	}
	for(int i = 0x40; i < 0x50; i += 2) {
		io->set_iomap_alias_rw(i, pic, I8259_ADDR_CHIP1 | ((i >> 1) & 1));
	}
	io->set_iomap_range_w(0x50, 0x5f, memory);
	io->set_iomap_range_r(0x60, 0x6f, sysport);
	io->set_iomap_range_w(0x70, 0x7f, sysport);
#if defined(_MZ6500) || defined(_MZ6550)
	io->set_iomap_single_rw(0xcd, memory);
#endif
	for(int i = 0x100; i < 0x110; i += 2) {
		io->set_iomap_alias_rw(i, gdc, (i >> 1) & 1);
	}
	io->set_iomap_range_rw(0x110, 0x17f, display);
	io->set_iomap_range_rw(0x200, 0x20f, sio);
	io->set_iomap_range_rw(0x210, 0x21f, ctc0);
	io->set_iomap_range_rw(0x220, 0x22f, rtc);
	for(int i = 0x230; i < 0x240; i++) {
		io->set_iomap_alias_rw(i, psg, ~i & 1);
	}
	io->set_iomap_range_r(0x240, 0x25f, sysport);
	io->set_iomap_range_w(0x260, 0x26f, sysport);
	io->set_iomap_range_r(0x270, 0x27f, sysport);
	
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

void VM::special_reset()
{
	// nmi
	cpu->write_signal(SIG_CPU_NMI, 1, 1);
	sysport->nmi_reset();
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

int VM::access_lamp()
{
	uint32 status = fdc->read_signal(0);
	return (status & (1 | 4)) ? 1 : (status & (2 | 8)) ? 2 : 0;
}

// ----------------------------------------------------------------------------
// soud manager
// ----------------------------------------------------------------------------

void VM::initialize_sound(int rate, int samples)
{
	// init sound manager
	event->initialize_sound(rate, samples);
	
	// init sound gen
	psg->init(rate, 4000000, samples, 0, 0);
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
//	keyboard->key_up(code);
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
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->update_config();
	}
}

