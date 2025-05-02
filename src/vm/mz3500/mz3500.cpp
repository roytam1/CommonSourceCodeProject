/*
	SHARP MZ-3500 Emulator 'EmuZ-3500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.31-

	[ virtual machine ]
*/

#include "mz3500.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../i8251.h"
#include "../i8253.h"
#include "../i8255.h"
#include "../io.h"
#include "../pcm1bit.h"
#include "../upd1990a.h"
#include "../upd7220.h"
#include "../upd765a.h"
#include "../z80.h"

#include "main.h"
#include "sub.h"

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
	
	// for main cpu
	io = new IO(this, emu);
	fdc = new UPD765A(this, emu);
	cpu = new Z80(this, emu);
	main = new MAIN(this, emu);
	
	// for sub cpu
	sio = new I8251(this, emu);
	pit = new I8253(this, emu);
	pio = new I8255(this, emu);
	subio = new IO(this, emu);
	pcm = new PCM1BIT(this, emu);
	rtc = new UPD1990A(this, emu);
	gdc_chr = new UPD7220(this, emu);
	gdc_gfx = new UPD7220(this, emu);
	subcpu = new Z80(this, emu);
	sub = new SUB(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_cpu(subcpu);
	event->set_context_sound(pcm);
	
	// mz3500sm p.59
	fdc->set_context_irq(main, SIG_MAIN_INTFD, 1);
	fdc->set_context_drq(main, SIG_MAIN_DRQ, 1);
	fdc->set_context_index(main, SIG_MAIN_INDEX, 1);
	
	// mz3500sm p.72,77
	sio->set_context_rxrdy(subcpu, SIG_CPU_NMI, 1);
	
	// mz3500sm p.77
	// i8253 ch.0 -> i8251 txc,rxc
	pit->set_context_ch1(pcm, SIG_PCM1BIT_SIGNAL, 1);
	pit->set_context_ch2(pit, SIG_I8253_GATE_1, 1);
	pit->set_constant_clock(0, 2450000);
	pit->set_constant_clock(1, 2450000);
	pit->set_constant_clock(2, 2450000);
	
	// mz3500sm p.78,80,81
	// i8255 pa0-pa7 -> printer data
	pio->set_context_port_b(rtc, SIG_UPD1990A_STB, 0x01, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_C0,  0x02, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_C1,  0x04, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_C2,  0x08, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_DIN, 0x10, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_CLK, 0x20, 0);
	pio->set_context_port_b(main, SIG_MAIN_SRDY, 0x40, 0);
	pio->set_context_port_b(sub, SIG_SUB_PIO_PM, 0x80, 0);	// P/M: CG Selection
	pio->set_context_port_c(sub, SIG_SUB_KEYBOARD_DC, 0x01, 0);
	pio->set_context_port_c(sub, SIG_SUB_KEYBOARD_STC, 0x02, 0);
	pio->set_context_port_c(sub, SIG_SUB_KEYBOARD_ACKC, 0x04, 0);
	// i8255 pc3 <- intr (not use ???)
	pio->set_context_port_c(pcm, SIG_PCM1BIT_MUTE, 0x10, 0);
	// i8255 pc5 -> printer strobe
	// i8255 pc6 <- printer ack
	pio->set_context_port_c(sub, SIG_SUB_PIO_OBF, 0x80, 0);
	
	// mz3500sm p.80,81
	rtc->set_context_dout(sub, SIG_SUB_RTC_DOUT, 1);
	
	gdc_chr->set_vram_ptr(sub->get_vram_chr(), 0x1000);
	sub->set_sync_ptr_chr(gdc_chr->get_sync());
	sub->set_ra_ptr_chr(gdc_chr->get_ra());
	sub->set_cs_ptr_chr(gdc_chr->get_cs());
	sub->set_ead_ptr_chr(gdc_chr->get_ead());
	
	gdc_gfx->set_vram_ptr(sub->get_vram_gfx(), 0x20000);
	sub->set_sync_ptr_gfx(gdc_gfx->get_sync());
	sub->set_ra_ptr_gfx(gdc_gfx->get_ra());
	sub->set_cs_ptr_gfx(gdc_gfx->get_cs());
	sub->set_ead_ptr_gfx(gdc_gfx->get_ead());
	
	// mz3500sm p.23
	subcpu->set_context_busack(main, SIG_MAIN_SACK, 1);
	
	main->set_context_cpu(cpu);
	main->set_context_subcpu(subcpu);
	main->set_context_fdc(fdc);
	
	sub->set_context_main(main);
	sub->set_context_fdc(fdc);
	sub->set_ipl(main->get_ipl());
	sub->set_common(main->get_common());
	
	// mz3500sm p.17
	io->set_iomap_range_rw(0xec, 0xef, main);	// reset int0
	io->set_iomap_range_rw(0xf4, 0xf7, fdc);	// fdc: f4h,f6h = status, f5h,f7h = data
	io->set_iomap_range_rw(0xf8, 0xfb, main);	// mfd interface
	io->set_iomap_range_rw(0xfc, 0xff, main);	// memory mpaper
	
	// mz3500sm p.18
	subio->set_iomap_range_rw(0x00, 0x0f, sub);	// int0 to main (set flipflop)
	subio->set_iomap_range_rw(0x10, 0x1f, sio);
	subio->set_iomap_range_rw(0x20, 0x2f, pit);
	subio->set_iomap_range_rw(0x30, 0x3f, pio);
	subio->set_iomap_range_r(0x40, 0x4f, sub);	// input port
	subio->set_iomap_range_w(0x50, 0x5f, sub);	// crt control i/o
	subio->set_iomap_range_rw(0x60, 0x6f, gdc_gfx);
	subio->set_iomap_range_rw(0x70, 0x7f, gdc_chr);
	
	// cpu bus
	cpu->set_context_mem(main);
	cpu->set_context_io(io);
	cpu->set_context_intr(main);
	
	subcpu->set_context_mem(sub);
	subcpu->set_context_io(subio);
	subcpu->set_context_intr(sub);
	
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
	
	// set busreq of sub cpu
	subcpu->write_signal(SIG_CPU_BUSREQ, 1, 1);
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

uint32 VM::get_sub_prv_pc()
{
	return subcpu->get_prv_pc();
}

// ----------------------------------------------------------------------------
// draw screen
// ----------------------------------------------------------------------------

void VM::draw_screen()
{
	sub->draw_screen();
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
// notify key
// ----------------------------------------------------------------------------

void VM::key_down(int code, bool repeat)
{
	sub->key_down(code);
}

void VM::key_up(int code)
{
	sub->key_up(code);
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

