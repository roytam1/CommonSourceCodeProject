/*
	SEGA SC-3000 Emulator 'eSC-3000'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.17-

	[ virtual machine ]
*/

#include "sc3000.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../datarec.h"
#include "../i8251.h"
#include "../i8255.h"
#include "../io.h"
#include "../sn76489an.h"
#include "../tms9918a.h"
#include "../upd765a.h"
#include "../z80.h"

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
	
	drec = new DATAREC(this, emu);
	sio = new I8251(this, emu);
	pio_k = new I8255(this, emu);
	pio_f = new I8255(this, emu);
	io = new IO(this, emu);
	psg = new SN76489AN(this, emu);
	vdp = new TMS9918A(this, emu);
	fdc = new UPD765A(this, emu);
	cpu = new Z80(this, emu);
	
	key = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(psg);
	
	drec->set_context_out(pio_k, SIG_I8255_PORT_B, 0x80);
	pio_k->set_context_port_c(key, SIG_KEYBOARD_COLUMN, 0x07, 0);
	pio_k->set_context_port_c(drec, SIG_DATAREC_REMOTE, 0x08, 0);
	pio_k->set_context_port_c(drec, SIG_DATAREC_OUT, 0x10, 0);
	pio_f->set_context_port_c(fdc, SIG_UPD765A_MOTOR_NEG, 2, 0);
	pio_f->set_context_port_c(fdc, SIG_UPD765A_TC, 4, 0);
	pio_f->set_context_port_c(fdc, SIG_UPD765A_RESET, 8, 0);
	pio_f->set_context_port_c(memory, SIG_MEMORY_SEL, 0x40, 0);
	vdp->set_context_irq(cpu, SIG_CPU_IRQ, 1);
	fdc->set_context_irq(pio_f, SIG_I8255_PORT_A, 1);
	fdc->set_context_index(pio_f, SIG_I8255_PORT_A, 4);
#ifdef _FDC_DEBUG_LOG
	fdc->set_context_cpu(cpu);
#endif
	
	key->set_context_cpu(cpu);
	key->set_context_pio(pio_k);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(dummy);
	
	// i/o bus
	io->set_iomap_range_rw(0x40, 0x7f, psg);
	io->set_iomap_range_rw(0x80, 0xbf, vdp);
	io->set_iomap_range_rw(0xc0, 0xdf, pio_k);
	io->set_iomap_range_rw(0xe0, 0xe3, fdc);
	io->set_iomap_range_rw(0xe4, 0xe7, pio_f);
	io->set_iomap_range_rw(0xe8, 0xeb, sio);
	
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
// draw screen
// ----------------------------------------------------------------------------

void VM::draw_screen()
{
	vdp->draw_screen();
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
	psg->init(rate, 3579545, 8000);
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::open_cart(_TCHAR* filename)
{
	memory->open_cart(filename);
	reset();
}

void VM::close_cart()
{
	memory->close_cart();
	reset();
}

void VM::open_disk(_TCHAR* filename, int drv)
{
	fdc->open_disk(filename, drv);
}

void VM::close_disk(int drv)
{
	fdc->close_disk(drv);
}

void VM::play_datarec(_TCHAR* filename)
{
	drec->play_datarec(filename);
}

void VM::rec_datarec(_TCHAR* filename)
{
	drec->rec_datarec(filename);
}

void VM::close_datarec()
{
	drec->close_datarec();
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

