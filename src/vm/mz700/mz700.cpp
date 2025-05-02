/*
	SHARP MZ-700 Emulator 'EmuZ-700'
	SHARP MZ-1500 Emulator 'EmuZ-1500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.05 -

	[ virtual machine ]
*/

#include "mz700.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../and.h"
#include "../datarec.h"
#include "../i8253.h"
#include "../i8255.h"
#include "../io.h"
#include "../pcm1bit.h"
#ifdef _MZ1500
#include "../sn76489an.h"
#include "../z80pio.h"
#include "../z80sio.h"
#endif
#include "../z80.h"

//#include "cmos.h"
#include "display.h"
#include "emm.h"
#include "keyboard.h"
#include "memory.h"
#ifdef _MZ1500
#include "psg.h"
#endif
#include "ramfile.h"

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
	
	and_int = new AND(this, emu);
	drec = new DATAREC(this, emu);
	pit = new I8253(this, emu);
	pio = new I8255(this, emu);
	io = new IO(this, emu);
	pcm = new PCM1BIT(this, emu);
	cpu = new Z80(this, emu);
	
//	cmos = new CMOS(this, emu);
	display = new DISPLAY(this, emu);
	emm = new EMM(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	ramfile = new RAMFILE(this, emu);
	
#ifdef _MZ1500
	and_snd = new AND(this, emu);
	psg_l = new SN76489AN(this, emu);
	psg_r = new SN76489AN(this, emu);
	pio_int = new Z80PIO(this, emu);
	sio_rs = new Z80SIO(this, emu);
	sio_qd = new Z80SIO(this, emu);
	
	psg = new PSG(this, emu);
#endif
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(pcm);
#ifdef _MZ1500
	event->set_context_sound(psg_l);
	event->set_context_sound(psg_r);
#endif
	
	and_int->set_context_out(cpu, SIG_CPU_IRQ, 1);
	and_int->set_mask(SIG_AND_BIT_0 | SIG_AND_BIT_1);
#ifdef _MZ1500
	and_snd->set_context_out(pcm, SIG_PCM1BIT_SIGNAL, 1);
	and_snd->set_mask(SIG_AND_BIT_0 | SIG_AND_BIT_1);
#endif
	drec->set_context_out(pio, SIG_I8255_PORT_C, 0x20);
	drec->set_context_remote(pio, SIG_I8255_PORT_C, 0x10);
#ifdef _MZ1500
	pit->set_context_ch0(and_snd, SIG_AND_BIT_0, 1);
#else
	pit->set_context_ch0(pcm, SIG_PCM1BIT_SIGNAL, 1);
#endif
	pit->set_context_ch1(pit, SIG_I8253_CLOCK_2, 1);
	pit->set_context_ch2(and_int, SIG_AND_BIT_0, 1);
#ifdef _MZ1500
	pit->set_context_ch0(pio_int, SIG_Z80PIO_PORT_A, 0x10);
	pit->set_context_ch2(pio_int, SIG_Z80PIO_PORT_A, 0x20);
#endif
	pit->set_constant_clock(0, CPU_CLOCKS >> 2);
	pit->set_constant_clock(1, 15700);
	pio->set_context_port_a(keyboard, SIG_KEYBOARD_COLUMN, 0x0f, 0);
#ifdef _MZ1500
	pio->set_context_port_c(and_snd, SIG_AND_BIT_1, 1, 0);
#endif
	pio->set_context_port_c(drec, SIG_DATAREC_OUT, 2, 0);
	pio->set_context_port_c(and_int, SIG_AND_BIT_1, 4, 0);
	pio->set_context_port_c(drec, SIG_DATAREC_TRIG, 8, 0);
	
	display->set_vram_ptr(memory->get_vram());
	display->set_font_ptr(memory->get_font());
#ifdef _MZ1500
	display->set_pcg_ptr(memory->get_pcg());
#endif
	keyboard->set_context_pio(pio);
	memory->set_context_cpu(cpu);
	memory->set_context_pit(pit);
	memory->set_context_pio(pio);
#ifdef _MZ1500
	psg->set_context_psg_l(psg_l);
	psg->set_context_psg_r(psg_r);
#endif
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
#ifdef _MZ1500
	cpu->set_context_intr(pio_int);
#else
	cpu->set_context_intr(dummy);
#endif
	
#ifdef _MZ1500
	// z80 family daisy chain
	// 0=8253 ch.2
	pio_int->set_context_intr(cpu, 1);
	pio_int->set_context_child(sio_rs);
	sio_rs->set_context_intr(cpu, 2);
	sio_rs->set_context_child(sio_qd);
	sio_qd->set_context_intr(cpu, 3);
#endif
	
	// emm
	io->set_iomap_range_w(0x00, 0x03, emm);
	io->set_iomap_range_r(0x00, 0x03, emm);
	// ramfile
	io->set_iomap_range_w(0xea, 0xeb, ramfile);
	io->set_iomap_single_r(0xea, ramfile);
	// cmos
//	io->set_iomap_range_w(0xf8, 0xfa, cmos);
//	io->set_iomap_range_r(0xf8, 0xf9, cmos);
	
#ifdef _MZ1500
	// memory mapper
	io->set_iomap_range_w(0xe0, 0xe6, memory);	// E5h-E6h: PCG
	// psg
	io->set_iomap_single_w(0xe9, psg);
	io->set_iomap_single_w(0xf2, psg_l);
	io->set_iomap_single_w(0xf3, psg_r);
	// display
	io->set_iomap_range_w(0xf0, 0xf1, display);
	// z80pio and z80sio*2
	static int z80_sio_addr[4] = {0, 2, 1, 3};
	static int z80_pio_addr[4] = {1, 3, 0, 2};
	for(int i = 0; i < 4; i++) {
		io->set_iomap_alias_rw(0xb0 + i, sio_rs, z80_sio_addr[i]);
		io->set_iomap_alias_rw(0xf4 + i, sio_qd, z80_sio_addr[i]);
		io->set_iomap_alias_rw(0xfc + i, pio_int, z80_pio_addr[i]);
	}
//	io->set_iovalue_single_r(0xf7, 0x80);	// NOTE: sio status for quick disk
#else
	// memory mapper
	io->set_iomap_range_w(0xe0, 0xe4, memory);
	// printer
	io->set_iovalue_single_r(0xfe, 0xc0);
#endif
	
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
	and_int->write_signal(SIG_AND_BIT_0, 0, 1);	// CLOCK = L
	and_int->write_signal(SIG_AND_BIT_1, 1, 1);	// INTMASK = H
#ifdef _MZ1500
	and_snd->write_signal(SIG_AND_BIT_1, 1, 1);	// SNDMASK = H
#endif
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

void VM::set_pc(uint32 pc)
{
	cpu->set_pc(pc);
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
	pcm->init(rate, 8000);
#ifdef _MZ1500
	psg_l->init(rate, 3579545, 8000);
	psg_r->init(rate, 3579545, 8000);
#endif
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::open_mzt(_TCHAR* filename)
{
	memory->open_mzt(filename);
}

void VM::play_datarec(_TCHAR* filename)
{
	drec->play_datarec(filename);
	drec->write_signal(SIG_DATAREC_REMOTE, 1, 1);
}

void VM::rec_datarec(_TCHAR* filename)
{
	drec->rec_datarec(filename);
	drec->write_signal(SIG_DATAREC_REMOTE, 1, 1);
}

void VM::close_datarec()
{
	drec->close_datarec();
	drec->write_signal(SIG_DATAREC_REMOTE, 0, 0);
}

void VM::push_play()
{
	drec->write_signal(SIG_DATAREC_REMOTE, 1, 1);
}

void VM::push_stop()
{
	drec->write_signal(SIG_DATAREC_REMOTE, 0, 0);
}

bool VM::now_skip()
{
#ifdef _TINYIMAS
	return false;
#else
	return drec->skip();
#endif
}

void VM::update_config()
{
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->update_config();
	}
}

