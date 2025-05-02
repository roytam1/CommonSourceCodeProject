/*
	NEC PC-100 Emulator 'ePC-100'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.07.12 -

	[ virtual machine ]
*/

#include "pc100.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../and.h"
#include "../beep.h"
#include "../i8251.h"
#include "../i8255.h"
#include "../i8259.h"
#include "../i86.h"
#include "../io.h"
#include "../pcm1bit.h"
#include "../rtc58321.h"
#include "../upd765a.h"

#include "crtc.h"
#include "ioctrl.h"
#include "kanji.h"
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
	
	and = new AND(this, emu);
	beep = new BEEP(this, emu);
	sio = new I8251(this, emu);
	pio0 = new I8255(this, emu);
	pio1 = new I8255(this, emu);
	pic = new I8259(this, emu);
	cpu = new I86(this, emu);
	io = new IO(this, emu);
	pcm = new PCM1BIT(this, emu);
	rtc = new RTC58321(this, emu);
	fdc = new UPD765A(this, emu);
	
	crtc = new CRTC(this, emu);
	ioctrl = new IOCTRL(this, emu);
	kanji = new KANJI(this, emu);
	memory = new MEMORY(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(beep);
	event->set_context_sound(pcm);
	
	and->set_context_out(cpu, SIG_CPU_NMI, 1);
	and->set_mask(SIG_AND_BIT_0 | SIG_AND_BIT_1);
	sio->set_context_rxrdy(pic, SIG_I8259_IR1, 1);
	pio0->set_context_port_a(rtc, SIG_RTC58321_WRITE, 1, 0);
	pio0->set_context_port_a(rtc, SIG_RTC58321_READ, 2, 0);
	pio0->set_context_port_a(rtc, SIG_RTC58321_SELECT, 4, 0);
	pio0->set_context_port_c(rtc, SIG_RTC58321_DATA, 0xf, 0);
	pio1->set_context_port_a(memory, SIG_MEMORY_BITMASK_LOW, 0xff, 0);
	pio1->set_context_port_b(memory, SIG_MEMORY_BITMASK_HIGH, 0xff, 0);
	pio1->set_context_port_c(memory, SIG_MEMORY_VRAM_PLANE, 0x3f, 0);
	pio1->set_context_port_c(and, SIG_AND_BIT_0, 0x80, 0);
	pio1->set_context_port_c(ioctrl, SIG_IOCTRL_RESET, 0x40, 0);
	pic->set_context(cpu);
	rtc->set_context_data(pio0, SIG_I8255_PORT_C, 0xf, 0);
	rtc->set_context_busy(pio0, SIG_I8255_PORT_C, 0x10);
	fdc->set_context_irq(cpu, SIG_CPU_NMI, 1);
	fdc->set_context_drq(and, SIG_AND_BIT_1, 1);
	
	crtc->set_context_pic(pic);
	crtc->set_context_fdc(fdc);
	crtc->set_vram_ptr(memory->get_vram());
	ioctrl->set_context_pic(pic);
	ioctrl->set_context_fdc(fdc);
	ioctrl->set_context_beep(beep);
	ioctrl->set_context_pcm(pcm);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(pic);
	
	// i/o bus
	io->set_iomap_alias_w(0x00, pic, 0);
	io->set_iomap_alias_w(0x02, pic, 1);
//	io->set_iomap_alias_w(0x04, dma, 0);
//	io->set_iomap_alias_w(0x06, dma, 1);
	io->set_iomap_alias_w(0x0a, fdc, 1);
	io->set_iomap_alias_w(0x10, pio0, 0);
	io->set_iomap_alias_w(0x12, pio0, 1);
	io->set_iomap_alias_w(0x14, pio0, 2);
	io->set_iomap_alias_w(0x16, pio0, 3);
	io->set_iomap_alias_w(0x18, pio1, 0);
	io->set_iomap_alias_w(0x1a, pio1, 1);
	io->set_iomap_alias_w(0x1c, pio1, 2);
	io->set_iomap_alias_w(0x1e, pio1, 3);
	io->set_iomap_single_w(0x22, ioctrl);
	io->set_iomap_single_w(0x24, ioctrl);
	io->set_iomap_alias_w(0x28, sio, 0);
	io->set_iomap_alias_w(0x2a, sio, 1);
	io->set_iomap_single_w(0x30, memory);
	io->set_iomap_single_w(0x38, crtc);
	io->set_iomap_single_w(0x3a, crtc);
	io->set_iomap_single_w(0x3c, crtc);
	io->set_iomap_single_w(0x3e, crtc);
	for(int i = 0x40; i < 0x62; i++) {
		io->set_iomap_single_w(i, crtc);
	}
	io->set_iomap_single_w(0x80, kanji);
	io->set_iomap_single_w(0x81, kanji);
	io->set_iomap_single_w(0x84, kanji);
	io->set_iomap_single_w(0x86, kanji);
	
	io->set_iomap_alias_r(0x00, pic, 0);
	io->set_iomap_alias_r(0x02, pic, 1);
//	io->set_iomap_alias_r(0x04, dma, 0);
//	io->set_iomap_alias_r(0x06, dma, 1);
	io->set_iomap_alias_r(0x08, fdc, 0);
	io->set_iomap_alias_r(0x0a, fdc, 1);
	io->set_iomap_alias_r(0x10, pio0, 0);
	io->set_iomap_alias_r(0x12, pio0, 1);
	io->set_iomap_alias_r(0x14, pio0, 2);
	io->set_iomap_alias_r(0x18, pio1, 0);
	io->set_iomap_alias_r(0x1a, pio1, 1);
	io->set_iomap_alias_r(0x1c, pio1, 2);
	io->set_iomap_single_r(0x20, ioctrl);
	io->set_iomap_single_r(0x22, ioctrl);
	io->set_iomap_alias_r(0x28, sio, 0);
	io->set_iomap_alias_r(0x2a, sio, 1);
	io->set_iomap_single_r(0x30, memory);
	io->set_iomap_single_r(0x38, crtc);
	io->set_iomap_single_r(0x3a, crtc);
	io->set_iomap_single_r(0x3c, crtc);
	io->set_iomap_single_r(0x3e, crtc);
	for(int i = 0x40; i < 0x62; i++) {
		io->set_iomap_single_r(i, crtc);
	}
	io->set_iomap_single_r(0x80, kanji);
	io->set_iomap_single_r(0x81, kanji);
	
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
	crtc->draw_screen();
}

// ----------------------------------------------------------------------------
// soud manager
// ----------------------------------------------------------------------------

void VM::initialize_sound(int rate, int samples)
{
	// init sound manager
	event->initialize_sound(rate, samples);
	
	// init sound gen
	beep->init(rate, 2400, 2, 8000);
	pcm->init(rate, 8000);
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
}

// ----------------------------------------------------------------------------
// notify key
// ----------------------------------------------------------------------------

void VM::key_down(int code)
{
	ioctrl->key_down(code);
}

void VM::key_up(int code)
{
	ioctrl->key_up(code);
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

