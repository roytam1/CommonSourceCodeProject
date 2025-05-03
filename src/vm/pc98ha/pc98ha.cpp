/*
	NEC PC-98HA Emulator 'eHandy98'
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
#include "../io8.h"
#include "../upd4991a.h"
#include "../upd71071.h"
#include "../upd765a.h"
#include "../x86.h"

#include "display.h"
#include "floppy.h"
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
	
	beep = new BEEP(this, emu);
	sio_r = new I8251(this, emu);	// for rs232c
	sio_k = new I8251(this, emu);	// for keyboard
	pit = new I8253(this, emu);	// V50 internal
	pio_s = new I8255(this, emu);	// for system port
	pio_p = new I8255(this, emu);	// for printer
	pic = new I8259(this, emu);	// V50 internal
	io = new IO8(this, emu);
	rtc = new UPD4991A(this, emu);
	dma = new UPD71071(this, emu);	// V50 internal
	fdc = new UPD765A(this, emu);
	cpu = new X86(this, emu);	// V50
	
	display = new DISPLAY(this, emu);
	floppy = new FLOPPY(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(beep);
	
/*
	IR0	timer ch0
	IR1	key 8251a
	IR2	timer ch1
	IR3	-
	IR4	rs232c 8251a
	IR5	-
	IR6	fdc
	IR7	pic•sŠ®‘SŠ„ž‚Ý

MEMMAP
	0E8E
	1E8E

0812	ch=dat;
0810	reg[ch]=dat;

0F8E
	memcard‚È‚µ	04
	memcard‚ ‚è	0e


8810
	40

be


1F8E
4810
8810
C810

OUT	c810, 0
OUT	4810, 0
OUT	2f8e, 1
IN	 f8e =  0	4
OUT	4810, 0
OUT	 f8e, 0
IN	 f8e =  0
IN	  be = ff
OUT	  be, 3
IN	 f8e =  0
IN	  be = ff
OUT	1e8e,81
OUT	 e8e,50
OUT	1e8e,81
IN	c810 = ff
OUT	 e8e,50
OUT	1e8e,80
IN	  be = ff
IN	 f8e =  0
IN	  be = ff
IN	 f8e =  0
*/
	sio_r->set_context_rxrdy(pic, SIG_I8259_IR4, 1);
	sio_k->set_context_rxrdy(pic, SIG_I8259_IR1, 1);
	sio_k->set_context_out(keyboard, SIG_KEYBOARD_RECV);
	pit->set_context_ch0(pic, SIG_I8259_IR0);
	pit->set_context_ch1(pic, SIG_I8259_IR2);
	pit->set_constant_clock(0, 1996800);
	pit->set_constant_clock(1, 1996800);
	pit->set_constant_clock(2, 1996800);
	pio_s->set_context_port_c(beep, SIG_BEEP_MUTE, 8, 0);
	pic->set_context(cpu);
	dma->set_context_memory(memory);
	dma->set_context_ch2(fdc);	// 1MB
	dma->set_context_ch3(fdc);	// 640KB
	fdc->set_context_intr(pic, SIG_I8259_IR6, 1);
	fdc->set_context_drq(floppy, SIG_FLOPPY_DRQ, 1);
	
	display->set_context_fdc(fdc);
	display->set_vram_ptr(memory->get_vram());
	floppy->set_context_fdc(fdc);
	floppy->set_context_dma(dma, SIG_UPD71071_CH2, SIG_UPD71071_CH3);
	keyboard->set_context_sio(sio_k, SIG_I8251_RECV);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(pic);
	
	// i/o bus
	io->set_iomap_alias_w(0x00, pic, 0);
	io->set_iomap_alias_w(0x02, pic, 1);
	io->set_iomap_range_w(0x22, 0x23, rtc);
	io->set_iomap_alias_w(0x30, sio_r, 0);
	io->set_iomap_alias_w(0x32, sio_r, 1);
	io->set_iomap_alias_w(0x31, pio_s, 0);
	io->set_iomap_alias_w(0x33, pio_s, 1);
	io->set_iomap_alias_w(0x35, pio_s, 2);
	io->set_iomap_alias_w(0x37, pio_s, 3);
	io->set_iomap_alias_w(0x40, pio_p, 0);
	io->set_iomap_alias_w(0x42, pio_p, 1);
	io->set_iomap_alias_w(0x44, pio_p, 2);
	io->set_iomap_alias_w(0x46, pio_p, 3);
	io->set_iomap_alias_w(0x41, sio_k, 0);
	io->set_iomap_alias_w(0x43, sio_k, 1);
	io->set_iomap_alias_w(0x71, pit, 0);
	io->set_iomap_alias_w(0x73, pit, 1);
	io->set_iomap_alias_w(0x75, pit, 2);
	io->set_iomap_alias_w(0x77, pit, 3);
//	io->set_iomap_alias_w(0x92, fdc, 1);
//	io->set_iomap_single_w(0x94, floppy);
//	io->set_iomap_alias_w(0xca, fdc, 1);
//	io->set_iomap_single_w(0xcc, floppy);
//	io->set_iomap_single_w(0xbe, floppy);
	io->set_iomap_range_w(0xe0, 0xef, dma);
	io->set_iomap_single_w(0x0810, display);
	io->set_iomap_single_w(0x0812, display);
	io->set_iomap_single_w(0x0c10, memory);
	io->set_iomap_single_w(0x0e8e, memory);
	io->set_iomap_single_w(0x1e8e, memory);
	io->set_iomap_single_w(0x4c10, memory);
	io->set_iomap_single_w(0x6e8e, memory);
	io->set_iomap_single_w(0x7e8e, memory);
	io->set_iomap_single_w(0x8810, memory);
	io->set_iomap_single_w(0x8c10, memory);
	io->set_iomap_single_w(0xcc10, memory);
	
	io->set_iomap_alias_r(0x00, pic, 0);
	io->set_iomap_alias_r(0x02, pic, 1);
	io->set_iomap_single_r(0x23, rtc);
	io->set_iomap_alias_r(0x30, sio_r, 0);
	io->set_iomap_alias_r(0x32, sio_r, 1);
	io->set_iomap_alias_r(0x31, pio_s, 0);
	io->set_iomap_alias_r(0x33, pio_s, 1);
	io->set_iomap_alias_r(0x35, pio_s, 2);
	io->set_iomap_alias_r(0x37, pio_s, 3);
	io->set_iomap_alias_r(0x40, pio_p, 0);
	io->set_iomap_alias_r(0x42, pio_p, 1);
	io->set_iomap_alias_r(0x44, pio_p, 2);
	io->set_iomap_alias_r(0x46, pio_p, 3);
	io->set_iomap_alias_r(0x41, sio_k, 0);
	io->set_iomap_alias_r(0x43, sio_k, 1);
	io->set_iomap_alias_r(0x71, pit, 0);
	io->set_iomap_alias_r(0x73, pit, 1);
	io->set_iomap_alias_r(0x75, pit, 2);
//	io->set_iomap_alias_r(0x90, fdc, 0);
//	io->set_iomap_alias_r(0x92, fdc, 1);
	io->set_iomap_single_r(0x90, floppy);
	io->set_iomap_single_r(0x92, floppy);
	io->set_iomap_single_r(0x94, floppy);
//	io->set_iomap_alias_r(0xc8, fdc, 0);
//	io->set_iomap_alias_r(0xca, fdc, 1);
	io->set_iomap_single_r(0xc8, floppy);
	io->set_iomap_single_r(0xca, floppy);
	io->set_iomap_single_r(0xcc, floppy);
	io->set_iomap_single_r(0xbe, floppy);
	io->set_iomap_range_r(0xe0, 0xef, dma);
	io->set_iomap_single_r(0x0810, display);
	io->set_iomap_single_r(0x0812, display);
	io->set_iomap_single_r(0x0c10, memory);
	io->set_iomap_single_r(0x0e8e, memory);
	io->set_iomap_single_r(0x0f8e, memory);
	io->set_iomap_single_r(0x1e8e, memory);
	io->set_iomap_single_r(0x1f8e, memory);
	io->set_iomap_single_r(0x4c10, memory);
	io->set_iomap_single_r(0x8810, memory);
	io->set_iomap_single_r(0x8c10, memory);
	io->set_iomap_single_r(0xc810, memory);
	io->set_iomap_single_r(0xcc10, memory);
	
	// initialize and reset all devices except the event manager
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id)
			device->initialize();
	}
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id)
			device->reset();
	}
	
	// initial device settings
	pio_s->write_signal(SIG_I8255_PORT_A, 0xe3, 0xff);
	pio_s->write_signal(SIG_I8255_PORT_B, 0xe0, 0xff);
	pio_s->write_signal(SIG_I8255_PORT_C, 0x18, 0xff);	//18 or 98
	pio_p->write_signal(SIG_I8255_PORT_B, 0xde, 0xff);
	pio_p->write_signal(SIG_I8255_PORT_C, 0x80, 0xff);
	
	beep->write_signal(SIG_BEEP_ON, 1, 1);
	beep->write_signal(SIG_BEEP_MUTE, 1, 1);
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
	beep->init(rate, 1000, 2, 8000);
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
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->update_config();
}

