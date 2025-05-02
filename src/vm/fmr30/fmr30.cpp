/*
	FUJITSU FMR-30 Emulator 'eFMR-30'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.12.29 -

	[ virtual machine ]
*/

#include "fmr30.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../i8237.h"
#include "../i8251.h"
#include "../i8253.h"
#include "../i8259.h"
#include "../i86.h"
#include "../io.h"
#include "../mb8877.h"
#include "../sn76489an.h"

#include "../fmr50/bios.h"
#include "cmos.h"
#include "floppy.h"
#include "keyboard.h"
#include "memory.h"
#include "rtc.h"
//#include "scsi.h"
#include "serial.h"
#include "system.h"
#include "timer.h"

#include "../../config.h"

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
	sio_kb = new I8251(this, emu);	// keyboard
	sio_sub = new I8251(this, emu);	// sub display
	sio_ch1 = new I8251(this, emu);	// RS-232C ch.1
	sio_ch2 = new I8251(this, emu);	// RS-232C ch.2
	pit = new I8253(this, emu);
	pic = new I8259(this, emu);
	cpu = new I86(this, emu);
	io = new IO(this, emu);
	fdc = new MB8877(this, emu);
	psg = new SN76489AN(this, emu);
	
	bios = new BIOS(this, emu);
	cmos = new CMOS(this, emu);
	floppy = new FLOPPY(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	rtc = new RTC(this, emu);
//	scsi = new SCSI(this, emu);
	serial = new SERIAL(this, emu);
	system = new SYSTEM(this, emu);
	timer = new TIMER(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(psg);
	
	dma->set_context_memory(memory);
	dma->set_context_ch0(fdc);
//	dma->set_context_ch1(scsi);
	sio_kb->set_context_rxrdy(serial, SIG_SERIAL_RXRDY_KB, 1);
	sio_kb->set_context_txrdy(serial, SIG_SERIAL_TXRDY_KB, 1);
	sio_sub->set_context_rxrdy(serial, SIG_SERIAL_RXRDY_SUB, 1);
	sio_sub->set_context_txrdy(serial, SIG_SERIAL_TXRDY_SUB, 1);
	sio_ch1->set_context_rxrdy(serial, SIG_SERIAL_RXRDY_CH1, 1);
	sio_ch1->set_context_txrdy(serial, SIG_SERIAL_TXRDY_CH1, 1);
	sio_ch2->set_context_rxrdy(serial, SIG_SERIAL_RXRDY_CH2, 1);
	sio_ch2->set_context_txrdy(serial, SIG_SERIAL_TXRDY_CH2, 1);
	pit->set_context_ch0(timer, SIG_TIMER_CH0, 1);
	pit->set_context_ch1(timer, SIG_TIMER_CH1, 1);
	pit->set_constant_clock(0, 1000000);
	pit->set_constant_clock(1, 1000000);
	pic->set_context_cpu(cpu);
//	fdc->set_context_drq(dma, SIG_I8237_CH0, 1);
	fdc->set_context_irq(floppy, SIG_FLOPPY_IRQ, 1);
	
	bios->set_context_mem(memory);
	bios->set_context_io(io);
	bios->set_cmos_ptr(cmos->get_cmos());
	bios->set_vram_ptr(memory->get_vram());
	bios->set_cvram_ptr(memory->get_cvram());
	bios->set_kvram_ptr(memory->get_kvram());
	floppy->set_context_fdc(fdc);
	floppy->set_context_pic(pic);
	keyboard->set_context_sio(sio_kb);
	memory->set_context_cpu(cpu);
	memory->set_context_dma(dma);
	rtc->set_context_pic(pic);
//	scsi->set_context_dma(dma, SIG_I8237_CH1);
//	scsi->set_context_pic(pic, SIG_I8259_CHIP1 | SIG_I8259_IR2);
	serial->set_context_pic(pic);
	serial->set_context_sio(sio_kb, sio_sub, sio_ch1, sio_ch2);
	timer->set_context_pic(pic);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(pic);
	cpu->set_context_bios(bios);
#ifdef SINGLE_MODE_DMA
	cpu->set_context_dma(dma);
#endif
	
	// i/o bus
	io->set_iomap_range_rw(0x00, 0x07, rtc);
	io->set_iomap_range_rw(0x08, 0x09, sio_kb);
	io->set_iomap_range_rw(0x0a, 0x0b, serial);
	io->set_iomap_range_rw(0x10, 0x11, sio_sub);
	io->set_iomap_range_rw(0x12, 0x13, serial);
	io->set_iomap_single_r(0x18, system);
	io->set_iomap_range_rw(0x1d, 0x1e, memory);
	io->set_iomap_range_r(0x20, 0x21, system);
	io->set_iomap_single_rw(0x26, memory);
	io->set_iomap_range_rw(0x30, 0x33, fdc);
	io->set_iomap_range_rw(0x34, 0x36, floppy);
	io->set_iomap_single_w(0x40, psg);
	io->set_iomap_range_rw(0x42, 0x43, timer);
	io->set_iomap_range_rw(0x46, 0x47, system);
	io->set_iomap_range_rw(0x60, 0x61, sio_ch1);
	io->set_iomap_range_rw(0x62, 0x66, serial);
	io->set_iomap_range_rw(0x70, 0x71, sio_ch2);
	io->set_iomap_range_rw(0x72, 0x76, serial);
	io->set_iomap_alias_rw(0x100, pic, I8259_ADDR_CHIP0 | 0);
	io->set_iomap_alias_rw(0x101, pic, I8259_ADDR_CHIP0 | 1);
	io->set_iomap_alias_rw(0x108, pic, I8259_ADDR_CHIP1 | 0);
	io->set_iomap_alias_rw(0x10a, pic, I8259_ADDR_CHIP1 | 1);
	io->set_iomap_range_rw(0x110, 0x11f, dma);
	io->set_iomap_range_w(0x120, 0x123, memory);
	io->set_iomap_range_rw(0x130, 0x133, pit);
//	io->set_iomap_range_rw(0x2f0, 0x2f3, scsi);
	io->set_iomap_range_rw(0x300, 0x30f, memory);
	io->set_iomap_range_rw(0xc000, 0xdfff, cmos);
	io->set_iomap_single_rw(0xff00, system);
	
	// initialize all devices
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id) {
			device->initialize();
		}
	}
	for(int i = 0; i < MAX_DRIVE; i++) {
		bios->set_disk_handler(i, fdc->get_disk_handler(i));
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
	// temporary fix...
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->reset();
	}
	
	// set devices
	sio_kb->write_signal(SIG_I8251_DSR, 1, 1);
	sio_sub->write_signal(SIG_I8251_DSR, 0, 0);
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
	memory->draw_screen();
}

int VM::access_lamp()
{
	uint32 status = fdc->read_signal(0) | bios->read_signal(0);
	return (status & 0x10) ? 4 : (status & (1 | 4)) ? 1 : (status & (2 | 8)) ? 2 : 0;
}

// ----------------------------------------------------------------------------
// soud manager
// ----------------------------------------------------------------------------

void VM::initialize_sound(int rate, int samples)
{
	// init sound manager
	event->initialize_sound(rate, samples);
	
	// init sound gen
	psg->init(rate, 125000, 10000);
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

void VM::open_disk(_TCHAR* filename, int drv)
{
	fdc->open_disk(filename, drv);
	floppy->change_disk(drv);
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

