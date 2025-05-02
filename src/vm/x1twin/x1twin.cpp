/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.11-

	[ virtual machine ]
*/

#include "x1twin.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../hd46505.h"
#include "../i8255.h"
#include "../mb8877.h"
#include "../ym2203.h"
#include "../z80.h"
//#include "../z80ctc.h"

#include "display.h"
#include "floppy.h"
#include "io.h"
#include "joystick.h"
#include "kanji.h"
#include "memory.h"
#include "sub.h"

#include "../huc6260.h"
#include "pce.h"

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
	pio = new I8255(this, emu);
	fdc = new MB8877(this, emu);
	psg = new YM2203(this, emu);
	cpu = new Z80(this, emu);
//	ctc = new Z80CTC(this, emu);
	
	display = new DISPLAY(this, emu);
	floppy = new FLOPPY(this, emu);
	io = new IO(this, emu);
	joy = new JOYSTICK(this, emu);
	kanji = new KANJI(this, emu);
	memory = new MEMORY(this, emu);
	sub = new SUB(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(psg);
	
	crtc->set_context_vblank(pio, SIG_I8255_PORT_B, 0x80);
	crtc->set_context_vsync(pio, SIG_I8255_PORT_B, 4);
	pio->set_context_port_c(display, SIG_DISPLAY_COLUMN, 0x40, 0);
	pio->set_context_port_c(io, SIG_IO_MODE, 0x20, 0);
//	ctc->set_context_zc0(ctc, SIG_Z80CTC_TRIG_3, 1);
//	ctc->set_constant_clock(1, CPU_CLOCKS >> 1);
//	ctc->set_constant_clock(2, CPU_CLOCKS >> 1);
	
	display->set_context_fdc(fdc);
	display->set_vram_ptr(io->get_vram());
	display->set_regs_ptr(crtc->get_regs());
	floppy->set_context_fdc(fdc, SIG_MB8877_DRIVEREG, SIG_MB8877_SIDEREG, SIG_MB8877_MOTOR);
	joy->set_context_psg(psg, SIG_YM2203_PORT_A, SIG_YM2203_PORT_B);
	sub->set_context_pio(pio, SIG_I8255_PORT_B);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
//	cpu->set_context_intr(ctc);
	cpu->set_context_intr(sub);
	
	// z80 family daisy chain
//	ctc->set_context_intr(cpu, 0);
//	ctc->set_context_child(sub);
	sub->set_context_intr(cpu, 1);
	
	// i/o bus
//	io->set_iomap_range_w(0x704, 0x707, ctc);
	io->set_iomap_range_w(0xe80, 0xe82, kanji);
	io->set_iomap_range_w(0xff8, 0xffb, fdc);
	io->set_iomap_single_w(0xffc, floppy);
	io->set_iomap_range_w(0x1000, 0x12ff, display);
	io->set_iomap_single_w(0x1300, display);
	io->set_iomap_range_w(0x1500, 0x17ff, display);
	io->set_iomap_range_w(0x1800, 0x1801, crtc);
	io->set_iomap_single_w(0x1900, sub);
	for(int i = 0x1a00; i <= 0x1af0; i += 0x10) {
		io->set_iomap_range_w(i, i + 3, pio);
	}
	for(int i = 0x1b00; i <= 0x1bff; i++) {
		io->set_iomap_alias_w(i, psg, 1);
	}
	for(int i = 0x1c00; i <= 0x1cff; i++) {
		io->set_iomap_alias_w(i, psg, 0);
	}
	io->set_iomap_range_w(0x1d00, 0x1eff, memory);
	io->set_iomap_range_w(0x2000, 0x27ff, display);	// attr vram 
	io->set_iomap_range_w(0x3000, 0x37ff, display);	// text vram
	
//	io->set_iomap_range_r(0x704, 0x707, ctc);
	io->set_iomap_range_r(0xe80, 0xe81, kanji);
	io->set_iomap_range_r(0xff8, 0xffb, fdc);
	io->set_iomap_range_w(0x1000, 0x12ff, display);
	io->set_iomap_single_r(0x1300, display);
	io->set_iomap_range_r(0x1400, 0x17ff, display);
	io->set_iomap_single_r(0x1900, sub);
	for(int i = 0x1a00; i <= 0x1af0; i += 0x10) {
		io->set_iomap_range_r(i, i + 2, pio);
	}
	for(int i = 0x1b00; i <= 0x1bff; i++) {
		io->set_iomap_alias_r(i, psg, 1);
	}
	io->set_iomap_range_r(0x2000, 0x27ff, display);	// attr vram
	io->set_iomap_range_r(0x3000, 0x37ff, display);	// text vram
	
	// init PC Engine
	pceevent = new EVENT(this, emu);
	pceevent->set_cpu_clocks(PCE_CPU_CLOCKS);
	pceevent->set_frames_per_sec(PCE_FRAMES_PER_SEC);
	pceevent->set_lines_per_frame(PCE_LINES_PER_FRAME);
	pceevent->initialize();
	
	pcecpu = new HUC6260(this, emu);
	pce = new PCE(this, emu);
	
	pceevent->set_context_cpu(pcecpu);
	pceevent->set_context_sound(pce);
	pce->set_context_cpu(pcecpu, SIG_HUC6260_IRQ2, SIG_HUC6260_IRQ1, SIG_HUC6260_TIRQ, SIG_HUC6260_INTMASK, SIG_HUC6260_INTSTAT);
	pcecpu->set_context_mem(pce);
	
	// initialize all devices
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id && device->this_device_id != pceevent->this_device_id) {
			device->initialize();
		}
	}
	pce_running = false;
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
	// nmi reset
	cpu->write_signal(SIG_CPU_NMI, 1, 1);
}

void VM::run()
{
	event->drive();
	if(pce_running) {
		pceevent->drive();
	}
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

void VM::pce_regist_event(DEVICE* dev, int event_id, int usec, bool loop, int* regist_id)
{
	pceevent->regist_event(dev, event_id, usec, loop, regist_id);
}

void VM::pce_regist_event_by_clock(DEVICE* dev, int event_id, int clock, bool loop, int* regist_id)
{
	pceevent->regist_event_by_clock(dev, event_id, clock, loop, regist_id);
}

void VM::pce_cancel_event(int regist_id)
{
	pceevent->cancel_event(regist_id);
}

void VM::pce_regist_frame_event(DEVICE* dev)
{
	pceevent->regist_frame_event(dev);
}

void VM::pce_regist_vline_event(DEVICE* dev)
{
	pceevent->regist_vline_event(dev);
}

uint32 VM::pce_current_clock()
{
	return pceevent->current_clock();
}

uint32 VM::pce_passed_clock(uint32 prev)
{
	uint32 current = pceevent->current_clock();
	return (current > prev) ? current - prev : current + (0xffffffff - prev) + 1;
}

uint32 VM::pce_get_prv_pc()
{
	return pcecpu->get_prv_pc();
}

// ----------------------------------------------------------------------------
// draw screen
// ----------------------------------------------------------------------------

void VM::draw_screen()
{
	display->draw_screen();
	if(pce_running) {
		pce->draw_screen();
	}
}

// ----------------------------------------------------------------------------
// soud manager
// ----------------------------------------------------------------------------

void VM::initialize_sound(int rate, int samples)
{
	// init sound manager
	event->initialize_sound(rate, samples);
	pceevent->initialize_sound(rate, samples);
	
	// init sound gen
	psg->init(rate, 2000000, samples, 0, 0);
	pce->initialize_sound(rate);
}

uint16* VM::create_sound(int samples, bool fill)
{
	if(pce_running) {
		return pceevent->create_sound(samples, fill);
	}
	return event->create_sound(samples, fill);
}

// ----------------------------------------------------------------------------
// notify key
// ----------------------------------------------------------------------------

void VM::key_down(int code)
{
	if(!pce_running) {
		sub->key_down(code);
	}
}

void VM::key_up(int code)
{
//	if(!pce_running)
//		sub->key_up(code);
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

void VM::open_cart(_TCHAR* filename)
{
	pce->open_cart(filename);
	pce->reset();
	pcecpu->reset();
}

void VM::close_cart()
{
	pce->close_cart();
	pce->reset();
	pcecpu->reset();
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

