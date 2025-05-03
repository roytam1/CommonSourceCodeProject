/*
	SHARP MZ-3500 Emulator 'EmuZ-3500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.17 -

	[ virtual machine ]
*/

#include "mz3500.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../beep.h"
#include "../i8251.h"
#include "../i8253.h"
#include "../i8255.h"
#include "../io.h"
#include "../ls244.h"
#include "../upd1990a.h"
#include "../upd7220.h"
#include "../upd765a.h"
#include "../z80.h"

#include "display.h"
#include "keyboard.h"
#include "memory.h"
#include "mfd.h"
#include "submemory.h"

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
	
	// main
	io = new IO(this, emu);
	fdc = new UPD765A(this, emu);
	cpu = new Z80(this, emu);
	
	memory = new MEMORY(this, emu);
	mfd = new MFD(this, emu);
	
	// sub
	beep = new BEEP(this, emu);
	sio = new I8251(this, emu);
	ctc = new I8253(this, emu);
	pio = new I8255(this, emu);
	subio = new IO(this, emu);
	ls244 = new LS244(this, emu);
	rtc = new UPD1990A(this, emu);
	gdc_chr = new UPD7220(this, emu);
	gdc_gfx = new UPD7220(this, emu);
	subcpu = new Z80(this, emu);
	
	display = new DISPLAY(this, emu);
	keyboard = new KEYBOARD(this, emu);
	submemory = new SUBMEMORY(this, emu);
	
	// set event contexts
	event->set_context_cpu(cpu);
	event->set_context_cpu(subcpu);
	event->set_context_sound(beep);
	
	// set contexts main
	fdc->set_context_intr(memory, SIG_MEMORY_INTMFD, 1);
	
	memory->set_context_cpu(cpu);
	memory->set_context_sub(subcpu);
	mfd->set_context_fdc(fdc, SIG_UPD765A_SELECT, SIG_UPD765A_TC, SIG_UPD765A_MOTOR);
	
	// set contexts sub
	sio->set_context_rxrdy(subcpu, SIG_CPU_NMI, 1);
	ctc->set_constant_clock(0, 2450000);	// 2.45MHz
	ctc->set_constant_clock(1, 2450000);	// 2.45MHz
	ctc->set_constant_clock(2, 2450000);	// 2.45MHz
	// OUT0 -> TXC,RXC of 8251
	ctc->set_context_ch1(beep, SIG_BEEP_PULSE);
	ctc->set_context_ch2(ctc, SIG_I8253_GATE_1);
	pio->set_context_port_b(rtc, SIG_UPD1990A_STB, 1, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_C0, 2, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_C1, 4, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_C2, 8, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_DIN, 0x10, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_CLK, 0x20, 0);
	pio->set_context_port_b(memory, SIG_MEMORY_SRDY, 0x40, 0);
	pio->set_context_port_b(display, SIG_DISPLAY_PM, 0x80, 0);
	pio->set_context_port_c(keyboard, SIG_KEYBOARD_DC, 1, 0);
	pio->set_context_port_c(keyboard, SIG_KEYBOARD_STC, 2, 0);
	pio->set_context_port_c(keyboard, SIG_KEYBOARD_ACKC, 4, 0);
	pio->set_context_port_c(ls244, SIG_LS244_INPUT, 0x80, -6);	// PC7 -> bit1
	rtc->set_context_dout(ls244, SIG_LS244_INPUT, 1);
	gdc_chr->set_vram_ptr(display->get_vram_chr(), VRAM_SIZE_CHR);
	gdc_gfx->set_vram_ptr(display->get_vram_gfx(), VRAM_SIZE_GFX);
	subcpu->set_context_busack(memory, SIG_MEMORY_SACK, 1);
	
	display->set_context_fdc(fdc);
	display->set_sync_ptr(gdc_chr->get_sync(), gdc_gfx->get_sync());
	display->set_ra_ptr(gdc_chr->get_ra(), gdc_gfx->get_ra());
	display->set_cs_ptr(gdc_chr->get_cs(), gdc_gfx->get_cs());
	display->set_ead_ptr(gdc_chr->get_ead(), gdc_gfx->get_ead());
	keyboard->set_context_dk(ls244, SIG_LS244_INPUT, 0x20);
	keyboard->set_context_stk(ls244, SIG_LS244_INPUT, 0x40);
	submemory->set_context_mem(memory, SIG_MEMORY_INT0);
	submemory->set_ipl(memory->get_ipl());
	submemory->set_common(memory->get_common());
	
	// cpu bus main
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(memory);
	
	// cpu bus main
	subcpu->set_context_mem(submemory);
	subcpu->set_context_io(subio);
	subcpu->set_context_intr(submemory);
	
	// i/o bus main
	io->set_iomap_range_w(0xec, 0xef, memory);
	io->set_iomap_range_w(0xf4, 0xf7, fdc);
	io->set_iomap_range_w(0xf8, 0xfb, mfd);
	io->set_iomap_range_w(0xfc, 0xff, memory);
	
	io->set_iomap_range_r(0xf4, 0xf7, fdc);
	io->set_iomap_range_r(0xf8, 0xfb, mfd);
	io->set_iomap_range_r(0xfc, 0xff, memory);
	
	// i/o bus sub
	subio->set_iomap_range_w(0x00, 0x0f, submemory);
	subio->set_iomap_range_w(0x10, 0x1f, sio);
	subio->set_iomap_range_w(0x20, 0x2f, ctc);
	subio->set_iomap_range_w(0x30, 0x3f, pio);
	subio->set_iomap_range_w(0x50, 0x5f, display);
	subio->set_iomap_range_w(0x60, 0x6f, gdc_gfx);
	subio->set_iomap_range_w(0x70, 0x7f, gdc_chr);
	
	subio->set_iomap_range_r(0x10, 0x1f, sio);
	subio->set_iomap_range_r(0x20, 0x2f, ctc);
	subio->set_iomap_range_r(0x30, 0x3f, pio);
	subio->set_iomap_range_r(0x40, 0x4f, ls244);
	subio->set_iomap_range_r(0x50, 0x5f, display);
	subio->set_iomap_range_r(0x60, 0x6f, gdc_gfx);
	subio->set_iomap_range_r(0x70, 0x7f, gdc_chr);
	
	// initialize and reset all devices except the event manager
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id)
			device->initialize();
	}
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id)
			device->reset();
	}
	// set busreq of sub cpu
	subcpu->write_signal(SIG_CPU_BUSREQ, 1, 1);
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
// notify key
// ----------------------------------------------------------------------------

void VM::key_down(int code)
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
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->update_config();
}

