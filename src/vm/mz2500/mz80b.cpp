/*
	SHARP MZ-80B Emulator 'EmuZ-80B'
	SHARP MZ-2200 Emulator 'EmuZ-2200'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2013.03.14-

	[ virtual machine ]
*/

#include "mz80b.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../datarec.h"
#include "../i8253.h"
#include "../i8255.h"
#include "../io.h"
#include "../mb8877.h"
#include "../pcm1bit.h"
#include "../z80.h"
#include "../z80pio.h"

#include "cassette.h"
#include "floppy.h"
#include "keyboard.h"
#include "memory80b.h"
#include "mz1r12.h"
#include "mz1r13.h"
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
	
	drec = new DATAREC(this, emu);
	pit = new I8253(this, emu);
	pio_i = new I8255(this, emu);
	io = new IO(this, emu);
	fdc = new MB8877(this, emu);
	pcm = new PCM1BIT(this, emu);
	cpu = new Z80(this, emu);
	pio_z = new Z80PIO(this, emu);
	
	cassette = new CASSETTE(this, emu);
	floppy = new FLOPPY(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	mz1r12 = new MZ1R12(this, emu);
	mz1r13 = new MZ1R13(this, emu);
	timer = new TIMER(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(pcm);
	
	drec->set_context_out(cassette, SIG_CASSETTE_OUT, 1);
	drec->set_context_remote(cassette, SIG_CASSETTE_REMOTE, 1);
	drec->set_context_end(cassette, SIG_CASSETTE_END, 1);
	drec->set_context_top(cassette, SIG_CASSETTE_TOP, 1);
	
	pit->set_context_ch0(pit, SIG_I8253_CLOCK_1, 1);
	pit->set_context_ch1(pit, SIG_I8253_CLOCK_2, 1);
	pit->set_constant_clock(0, 31250);
	pio_i->set_context_port_a(cassette, SIG_CASSETTE_PIO_PA, 0xff, 0);
	pio_i->set_context_port_a(memory, SIG_CRTC_REVERSE, 0x10, 0);
	pio_i->set_context_port_c(cassette, SIG_CASSETTE_PIO_PC, 0xff, 0);
	pio_i->set_context_port_c(pcm, SIG_PCM1BIT_SIGNAL, 0x04, 0);
#ifdef _FDC_DEBUG_LOG
	fdc->set_context_cpu(cpu);
#endif
	pio_z->set_context_port_a(memory, SIG_MEMORY_VRAM_SEL, 0xc0, 0);
	pio_z->set_context_port_a(memory, SIG_CRTC_WIDTH80, 0x20, 0);
	pio_z->set_context_port_a(keyboard, SIG_KEYBOARD_COLUMN, 0x1f, 0);
	
	cassette->set_context_pio(pio_i);
	cassette->set_context_datarec(drec);
	floppy->set_context_fdc(fdc);
	keyboard->set_context_pio_i(pio_i);
	keyboard->set_context_pio_z(pio_z);
	memory->set_context_cpu(cpu);
	memory->set_context_pio(pio_i);
	timer->set_context_pit(pit);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(pio_z);
	
	// z80 family daisy chain
	pio_z->set_context_intr(cpu, 0);
	
	// i/o bus
	io->set_iomap_range_rw(0xb8, 0xb9, mz1r13);
	io->set_iomap_range_rw(0xd8, 0xdb, fdc);
	io->set_iomap_range_w(0xdc, 0xdd, floppy);
	io->set_iomap_single_rw(0xe0, pio_i);
	io->set_iomap_single_r(0xe1, pio_i);
	io->set_iomap_single_w(0xe1, cassette);	// this is not i8255 port-b
	io->set_iomap_single_rw(0xe2, pio_i);
	io->set_iomap_single_w(0xe3, pio_i);
	io->set_iomap_range_rw(0xe4, 0xe7, pit);
	io->set_iomap_range_rw(0xe8, 0xeb, pio_z);
	io->set_iomap_range_w(0xf0, 0xf3, timer);
#ifndef _MZ80B
	io->set_iomap_range_w(0xf4, 0xf7, memory);
#endif
	io->set_iomap_range_rw(0xf8, 0xfa, mz1r12);
	
	io->set_iowait_range_rw(0xd8, 0xdf, 1);
	io->set_iowait_range_rw(0xe8, 0xeb, 1);
	
	// initialize all devices
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->initialize();
	}
}

VM::~VM()
{
	// delete all devices
	for(DEVICE* device = first_device; device;) {
		DEVICE *next_device = device->next_device;
		device->release();
		delete device;
		device = next_device;
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
	// reset all devices
//	for(DEVICE* device = first_device; device; device = device->next_device) {
//		device->special_reset();
//	}
	memory->special_reset();
	cpu->reset();
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
	pcm->init(rate, 4096);
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::open_disk(int drv, _TCHAR* file_path, int offset)
{
	fdc->open_disk(drv, file_path, offset);
}

void VM::close_disk(int drv)
{
	fdc->close_disk(drv);
}

bool VM::disk_inserted(int drv)
{
	return fdc->disk_inserted(drv);
}

void VM::play_datarec(_TCHAR* file_path)
{
	if(check_file_extension(file_path, _T(".dat"))) {
		memory->load_dat_image(file_path);
		return;
	} else if(check_file_extension(file_path, _T(".mzt"))) {
		memory->load_mzt_image(file_path);
		return;
	} else if(check_file_extension(file_path, _T(".mtw"))) {
		memory->load_mzt_image(file_path);
	}
	bool value = drec->play_datarec(file_path);
	cassette->close_datarec();
	cassette->play_datarec(value);
}

void VM::rec_datarec(_TCHAR* file_path)
{
	bool value = drec->rec_datarec(file_path);
	cassette->close_datarec();
	cassette->rec_datarec(value);
}

void VM::close_datarec()
{
	drec->close_datarec();
	cassette->close_datarec();
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
	return drec->skip();
}

void VM::update_config()
{
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->update_config();
	}
}

