/*
	NEC PC-98LT Emulator 'ePC-98LT'
	NEC PC-98HA Emulator 'eHANDY98'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.09 -

	[ virtual machine ]
*/

#include "pc98ha.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../beep.h"
#include "../i8251.h"
#include "../i8253.h"
#include "../i8255.h"
#include "../i8259.h"
#include "../i86.h"
#include "../io.h"
#ifdef _PC98HA
#include "../upd4991a.h"
#else
#include "../upd1990a.h"
#endif
#include "../upd71071.h"
#include "../upd765a.h"

#include "bios.h"
#ifdef _PC98HA
#include "calendar.h"
#endif
#include "floppy.h"
#include "keyboard.h"
#include "memory.h"
#include "note.h"

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
	sio_r = new I8251(this, emu);	// for rs232c
	sio_k = new I8251(this, emu);	// for keyboard
	pit = new I8253(this, emu);	// V50 internal
	pio_s = new I8255(this, emu);	// for system port
	pio_p = new I8255(this, emu);	// for printer
	pic = new I8259(this, emu);	// V50 internal
	cpu = new I86(this, emu);	// V50
	io = new IO(this, emu);
#ifdef _PC98HA
	rtc = new UPD4991A(this, emu);
#else
	rtc = new UPD1990A(this, emu);
#endif
	dma = new UPD71071(this, emu);	// V50 internal
	fdc = new UPD765A(this, emu);
	
	bios = new BIOS(this, emu);
#ifdef _PC98HA
	calendar = new CALENDAR(this, emu);
#endif
	floppy = new FLOPPY(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	note = new NOTE(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(beep);
	
//???	sio_r->set_context_rxrdy(pic, SIG_I8259_IR4, 1);
	sio_k->set_context_rxrdy(pic, SIG_I8259_IR1, 1);
//	sio_k->set_context_out(keyboard, SIG_KEYBOARD_RECV);
	pit->set_context_ch0(pic, SIG_I8259_IR0, 1);
	pit->set_context_ch1(pic, SIG_I8259_IR2, 1);
#ifdef _PC98HA
	pit->set_constant_clock(0, 2457600);
	pit->set_constant_clock(1, 2457600);
	pit->set_constant_clock(2, 2457600);
#else
	pit->set_constant_clock(0, 1996800);
	pit->set_constant_clock(1, 300);	// ???
	pit->set_constant_clock(2, 1996800);
#endif
	pio_s->set_context_port_c(beep, SIG_BEEP_MUTE, 8, 0);
	pic->set_context_cpu(cpu);
#ifdef _PC98LT
	rtc->set_context_dout(pio_s, SIG_I8255_PORT_B, 1);
#endif
	dma->set_context_memory(memory);
	dma->set_context_ch2(fdc);	// 1MB
	dma->set_context_ch3(fdc);	// 640KB
	fdc->set_context_irq(pic, SIG_I8259_IR6, 1);
	fdc->set_context_drq(dma, SIG_UPD71071_CH3, 1);
	
	bios->set_context_fdc(fdc);
#ifdef _PC98HA
	calendar->set_context_rtc(rtc);
#endif
	floppy->set_context_fdc(fdc);
	keyboard->set_context_sio(sio_k);
	note->set_context_pic(pic);
	
	// cpu bus
	cpu->set_context_bios(bios);
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(pic);
#ifdef SINGLE_MODE_DMA
	cpu->set_context_dma(dma);
#endif
	
	// i/o bus
	io->set_iomap_alias_rw(0x00, pic, 0);
	io->set_iomap_alias_rw(0x02, pic, 1);
#ifdef _PC98HA
	io->set_iomap_range_rw(0x22, 0x23, calendar);
#else
	io->set_iomap_single_w(0x20, rtc);
#endif
	io->set_iomap_alias_rw(0x30, sio_r, 0);
	io->set_iomap_alias_rw(0x32, sio_r, 1);
	io->set_iomap_alias_rw(0x31, pio_s, 0);
	io->set_iomap_alias_rw(0x33, pio_s, 1);
	io->set_iomap_alias_rw(0x35, pio_s, 2);
	io->set_iomap_alias_rw(0x37, pio_s, 3);
	io->set_iomap_alias_rw(0x40, pio_p, 0);
	io->set_iomap_alias_rw(0x42, pio_p, 1);
	io->set_iomap_alias_rw(0x44, pio_p, 2);
	io->set_iomap_alias_rw(0x46, pio_p, 3);
	io->set_iomap_alias_rw(0x41, sio_k, 0);
	io->set_iomap_alias_rw(0x43, sio_k, 1);
	io->set_iomap_alias_rw(0x71, pit, 0);
	io->set_iomap_alias_rw(0x73, pit, 1);
	io->set_iomap_alias_rw(0x75, pit, 2);
	io->set_iomap_alias_rw(0x77, pit, 3);
#if defined(_PC98LT) || defined(DOCKING_STATION)
	io->set_iomap_single_r(0xc8, floppy);
	io->set_iomap_single_rw(0xca, floppy);
	io->set_iomap_single_rw(0xcc, floppy);
	io->set_iomap_single_rw(0xbe, floppy);
#endif
	io->set_iomap_range_rw(0xe0, 0xef, dma);
	io->set_iomap_single_w(0x8e1, memory);
	io->set_iomap_single_w(0x8e3, memory);
	io->set_iomap_single_w(0x8e5, memory);
	io->set_iomap_single_w(0x8e7, memory);
	io->set_iomap_single_rw(0x0c10, memory);
	io->set_iomap_single_w(0x0e8e, memory);
	io->set_iomap_single_w(0x1e8e, memory);
	io->set_iomap_single_rw(0x4c10, memory);
	io->set_iomap_single_rw(0x8c10, memory);
	io->set_iomap_single_rw(0xcc10, memory);
	io->set_iomap_single_rw(0x0810, note);
	io->set_iomap_single_rw(0x0812, note);
	io->set_iomap_single_r(0x0f8e, note);
	io->set_iomap_single_r(0x5e8e, note);
	io->set_iomap_single_rw(0x8810, note);
	io->set_iomap_single_w(0xc810, note);
	
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
	
	// initial device settings
	pio_s->write_signal(SIG_I8255_PORT_A, 0xe3, 0xff);
	pio_s->write_signal(SIG_I8255_PORT_B, 0xe0, 0xff);
#ifdef _PC98HA
	pio_p->write_signal(SIG_I8255_PORT_B, 0xde, 0xff);
#else
	pio_p->write_signal(SIG_I8255_PORT_B, 0xfc, 0xff);
#endif
	beep->write_signal(SIG_BEEP_ON, 1, 1);
	beep->write_signal(SIG_BEEP_MUTE, 1, 1);
}

void VM::run()
{
	event->drive();
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
	beep->init(rate, 2400, 8000);
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

