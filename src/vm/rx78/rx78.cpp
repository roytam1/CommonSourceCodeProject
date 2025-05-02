/*
	BANDAI RX-78 Emulator 'eRX-78'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.21 -

	[ virtual machine ]
*/

#include "rx78.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../datarec.h"
#include "../io.h"
#include "../sn76489an.h"
#include "../z80.h"

#include "cmt.h"
#include "keyboard.h"
#include "memory.h"
#include "printer.h"
#include "vdp.h"

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
	io = new IO(this, emu);
	psg = new SN76489AN(this, emu);
	cpu = new Z80(this, emu);
	
	cmt = new CMT(this, emu);
	key = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	prt = new PRINTER(this, emu);
	vdp = new VDP(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(psg);
	
	drec->set_context_out(cmt, SIG_CMT_IN, 1);
	cmt->set_context_drec(drec);
	vdp->set_context_cpu(cpu);
	vdp->set_vram_ptr(memory->get_vram());
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(dummy);
	
	// i/o bus
	io->set_iomap_range_rw(0xe2, 0xe3, prt);
	io->set_iomap_single_rw(0xf0, cmt);
	io->set_iomap_range_w(0xf1, 0xf2, memory);
	io->set_iomap_single_rw(0xf4, key);
	io->set_iomap_range_w(0xf5, 0xfc, vdp);
	io->set_iomap_single_w(0xfe, vdp);
	io->set_iomap_single_w(0xff, psg);
	
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

void VM::run()
{
	event->drive();
}

// ----------------------------------------------------------------------------
// draw screen
// ----------------------------------------------------------------------------

void VM::draw_screen()
{
	vdp->draw_screen();
}

// ----------------------------------------------------------------------------
// soud manager
// ----------------------------------------------------------------------------

void VM::initialize_sound(int rate, int samples)
{
	// init sound manager
	event->initialize_sound(rate, samples);
	
	// init sound gen
	psg->init(rate, 3579545, 8000);
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::open_cart(_TCHAR* file_path)
{
	memory->open_cart(file_path);
	reset();
}

void VM::close_cart()
{
	memory->close_cart();
	reset();
}

bool VM::cart_inserted()
{
	return memory->cart_inserted();
}

void VM::play_tape(_TCHAR* file_path)
{
	drec->play_tape(file_path);
}

void VM::rec_tape(_TCHAR* file_path)
{
	drec->rec_tape(file_path);
}

void VM::close_tape()
{
	drec->close_tape();
}

bool VM::tape_inserted()
{
	return drec->tape_inserted();
}

bool VM::now_skip()
{
	return event->now_skip();
}

void VM::update_config()
{
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->update_config();
	}
}

