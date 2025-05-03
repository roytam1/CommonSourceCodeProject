/*
	SHARP MZ-2800 Emulator 'EmuZ-2800'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.08.13 -

	[ virtual machine ]
*/

#include "mz2800.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../i8253.h"
#include "../i8255.h"
#include "../i8259.h"
#include "../i86.h"
#include "../io.h"
#include "../mb8877.h"
#include "../pcm1bit.h"
#include "../rp5c15.h"
//#include "../sasi.h"
#include "../upd71071.h"
#include "../ym2203.h"
#include "../z80pio.h"
#include "../z80sio.h"

#include "calendar.h"
#include "crtc.h"
#include "floppy.h"
#include "joystick.h"
#include "keyboard.h"
#include "memory.h"
#include "mouse.h"
#include "reset.h"
#include "sysport.h"
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
	
	pit = new I8253(this, emu);
	pio0 = new I8255(this, emu);
	pic = new I8259(this, emu);
	cpu = new I86(this, emu);
	io = new IO(this, emu);
	fdc = new MB8877(this, emu);
	pcm = new PCM1BIT(this, emu);
	rtc = new RP5C15(this, emu);
//	sasi = new SASI(this, emu);
	dma = new UPD71071(this, emu);
	opn = new YM2203(this, emu);
	pio1 = new Z80PIO(this, emu);
	sio = new Z80SIO(this, emu);
	
	calendar = new CALENDAR(this, emu);
	crtc = new CRTC(this, emu);
	floppy = new FLOPPY(this, emu);
	joystick = new JOYSTICK(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	mouse = new MOUSE(this, emu);
	rst = new RESET(this, emu);
	sysport = new SYSPORT(this, emu);
	timer = new TIMER(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(opn);
	event->set_context_sound(pcm);
	
	pit->set_constant_clock(0, 31250);
	pit->set_constant_clock(2, 31250);
	pit->set_context_ch0(pit, SIG_I8253_CLOCK_1);
	pit->set_context_ch0(pic, SIG_I8259_CHIP0 | SIG_I8259_IR0);
	pit->set_context_ch1(pic, SIG_I8259_CHIP0 | SIG_I8259_IR6);
	pit->set_context_ch2(pic, SIG_I8259_CHIP1 | SIG_I8259_IR0);
	pio0->set_context_port_c(rst, SIG_RESET_CONTROL, 0xff, 0);
	pio0->set_context_port_c(pcm, SIG_PCM1BIT_SIGNAL, 0x04, 0);
	pic->set_context(cpu);
	fdc->set_context_drq(dma, SIG_UPD71071_CH1, 1);
	fdc->set_context_irq(pic, SIG_I8259_CHIP0 | SIG_I8259_IR5, 1);
	rtc->set_context_alarm(pic, SIG_I8259_CHIP1 | SIG_I8259_IR2, 1);
	rtc->set_context_pulse(opn, SIG_YM2203_PORT_B, 8);
//	sasi->set_context_drq(dma, SIG_UPD71071_CH0, 1);
//	sasi->set_context_irq(pic, SIG_I8259_CHIP0 | SIG_I8259_IR4, 1);
	dma->set_context_memory(memory);
//	dma->set_context_ch0(sasi);
	dma->set_context_ch1(fdc);
	dma->set_context_tc(pic, SIG_I8259_CHIP0 | SIG_I8259_IR3, 1);
	opn->set_context_irq(pic, SIG_I8259_CHIP1 | SIG_I8259_IR7, 1);
	opn->set_context_port_a(crtc, SIG_CRTC_PALLETE, 0x04, 0);
	opn->set_context_port_a(mouse, SIG_MOUSE_SEL, 0x08, 0);
	pio1->set_context_port_a(crtc, SIG_CRTC_COLUMN_SIZE, 0x20, 0);
	pio1->set_context_port_a(keyboard, SIG_KEYBOARD_COLUMN, 0xff, 0);
	sio->set_context_intr(pic, SIG_I8259_CHIP0 | SIG_I8259_IR2);
	sio->set_context_dtr1(mouse, SIG_MOUSE_DTR, 1);
	
	calendar->set_context(rtc);
	crtc->set_context_pic(pic, SIG_I8259_CHIP0 | SIG_I8259_IR1);
	crtc->set_context_pio(pio0, SIG_I8255_PORT_B);
	crtc->set_context_fdc(fdc);
	crtc->set_vram_ptr(memory->get_vram());
	crtc->set_tvram_ptr(memory->get_tvram());
	crtc->set_kanji_ptr(memory->get_kanji());
	crtc->set_pcg_ptr(memory->get_pcg());
	floppy->set_context_fdc(fdc, SIG_MB8877_DRIVEREG, SIG_MB8877_SIDEREG, SIG_MB8877_MOTOR);
	keyboard->set_context_pio0(pio0, SIG_I8255_PORT_B);
	keyboard->set_context_pio1(pio1, SIG_Z80PIO_PORT_B);
	memory->set_context(crtc);
	mouse->set_context(sio, SIG_Z80SIO_RECV_CH1, SIG_Z80SIO_CLEAR_CH1);
	sysport->set_context_dma(dma, SIG_UPD71071_CH2);
	sysport->set_context_sio(sio);
	timer->set_context(pit, SIG_I8253_GATE_0, SIG_I8253_GATE_1);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(pic);
	
	// i/o bus
	io->set_iomap_range_w(0x70, 0x7f, dma);
	io->set_iomap_alias_w(0x80, pic, 0);
	io->set_iomap_alias_w(0x81, pic, 1);
	io->set_iomap_alias_w(0x82, pic, 2);
	io->set_iomap_alias_w(0x83, pic, 3);
	io->set_iomap_range_w(0x8c, 0x8d, memory);
	io->set_iomap_single_w(0x8f, sysport);
	io->set_iomap_range_w(0xa0, 0xa3, sio);
	for(uint32 p = 0xae; p <= 0x1fae; p += 0x100)
		io->set_iomap_single_w(p, crtc);
//	io->set_iomap_single_w(0xaf, sasi);
	io->set_iomap_range_w(0xc8, 0xc9, opn);
	for(uint32 p = 0xcc; p <= 0xfcc; p += 0x100)
		io->set_iomap_single_w(p, calendar);
	io->set_iomap_single_w(0xce, memory);
	io->set_iomap_range_w(0xd8, 0xdb, fdc);
	io->set_iomap_range_w(0xdc, 0xdf, floppy);
	io->set_iomap_range_w(0xe0, 0xe3, pio0);
	io->set_iomap_range_w(0xe4, 0xe7, pit);
	io->set_iomap_range_w(0xe8, 0xeb, pio1);
	io->set_iomap_single_w(0xef, joystick);
//	io->set_iomap_range_w(0xf0, 0xf3, timer);
	io->set_iomap_single_w(0x170, crtc);
	io->set_iomap_single_w(0x172, crtc);
	io->set_iomap_single_w(0x174, crtc);
	io->set_iomap_single_w(0x176, crtc);
	io->set_iomap_range_w(0x178, 0x17b, crtc);
	io->set_iomap_single_w(0x270, crtc);
	io->set_iomap_single_w(0x272, crtc);
	io->set_iomap_single_w(0x274, memory);
	
	io->set_iomap_range_r(0x70, 0x7f, dma);
	io->set_iomap_alias_r(0x80, pic, 0);
	io->set_iomap_alias_r(0x81, pic, 1);
	io->set_iomap_alias_r(0x82, pic, 2);
	io->set_iomap_alias_r(0x83, pic, 3);
	io->set_iomap_range_r(0x8c, 0x8d, memory);
	io->set_iomap_range_r(0x8e, 0x8f, sysport);
	io->set_iomap_range_r(0xc8, 0xc9, opn);
	io->set_iomap_range_r(0xa0, 0xa3, sio);
//	io->set_iomap_single_r(0xaf, sasi);
	io->set_iomap_single_r(0xbe, sysport);
	io->set_iomap_single_r(0xca, sysport);
	for(uint32 p = 0xcc; p <= 0xfcc; p += 0x100)
		io->set_iomap_single_r(p, calendar);
	io->set_iomap_single_r(0xce, memory);
	io->set_iomap_range_r(0xd8, 0xdb, fdc);
	io->set_iomap_range_r(0xe0, 0xe2, pio0);
	io->set_iomap_range_r(0xe4, 0xe6, pit);
	io->set_iomap_single_r(0xe8, pio1);
	io->set_iomap_single_r(0xea, pio1);
	io->set_iomap_single_r(0xef, joystick);
	io->set_iomap_single_r(0x274, memory);
	
	// initialize and ipl reset all devices
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id)
			device->initialize();
	}
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id)
			device->reset();
	}
	
	// set initial port status
	pio0->write_signal(SIG_I8255_PORT_B, 0x7c, 0xff);
	opn->write_signal(SIG_YM2203_PORT_B, 0x37, 0xff);
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
	// temporary fix...
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->reset();
}

void VM::cpu_reset()
{
	cpu->reset();
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
	opn->init(rate, 2000000, samples, 0, -8);
	pcm->init(rate, 4096);
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

bool VM::now_skip()
{
	return false;
}

void VM::update_config()
{
	for(DEVICE* device = first_device; device; device = device->next_device)
		device->update_config();
}

