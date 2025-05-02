/*
	NEC PC-6001 Emulator 'yaPC-6001'

	Author : tanam
	Date   : 2013.07.15-

	[ virtual machine ]
*/

#include "pc6001.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../i8255.h"
#include "../io.h"
#include "../upd765a.h"
#include "../mc6847.h"
#include "../ym2203.h"
#include "../z80.h"

#include "display.h"
#include "joystick.h"
#include "keyboard.h"
#include "memory.h"
#include "system.h"

// ----------------------------------------------------------------------------
// initialize
// ----------------------------------------------------------------------------

VM::VM(EMU* parent_emu) : emu(parent_emu)
{
	// create devices
	first_device = last_device = NULL;
	dummy = new DEVICE(this, emu);	// must be 1st device
	event = new EVENT(this, emu);	// must be 2nd device
	
	pio_k = new I8255(this, emu);
	pio_f = new I8255(this, emu);
	io = new IO(this, emu);
	fdc = new UPD765A(this, emu);
	vdp = new MC6847(this, emu);
	psg = new YM2203(this, emu);
	cpu = new Z80(this, emu);
	
	display = new DISPLAY(this, emu);
	joystick = new JOYSTICK(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	system = new SYSTEM(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(psg);
	system->set_context_pio(pio_f);
	system->set_context_fdc(fdc);
	joystick->set_context_psg(psg);
#ifdef _TANAM
	vdp->load_font_image(emu->bios_path(_T("ROM/CGROM60.60")));
#else
	vdp->load_font_image(emu->bios_path(_T("CGROM60.60")));
#endif
	vdp->set_context_cpu(cpu);
	display->set_context_cpu(cpu);
	display->set_context_vdp(vdp);
	display->set_vram_ptr(memory->get_vram());
	keyboard->set_context_cpu(cpu);
	keyboard->set_context_pio(pio_k);
	keyboard->set_context_memory(memory);

	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(keyboard);
	
	// i/o bus
	io->set_iomap_single_rw(0x90, pio_k);
	io->set_iomap_single_rw(0x91, pio_k);
	io->set_iomap_single_w(0x92, pio_k);
	io->set_iovalue_single_r(0x92, 0xff);
	io->set_iomap_single_w(0x93, pio_k);
	io->set_iomap_range_rw(0xd0, 0xd3, system);
	io->set_iomap_range_rw(0xf0, 0xf3, memory);
	io->set_iomap_alias_w(0xa0, psg, 0);	// PSG ch
	io->set_iomap_alias_w(0xa1, psg, 1);	// PSG data
	io->set_iomap_alias_r(0xa2, psg, 1);	// PSG data
	io->set_iomap_single_w(0xb0, display);
	
	// initialize all devices
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->initialize();
	}
#ifdef _TANAM
	fdc->open_disk(0, emu->bios_path(_T("D1/INIT.D88")) ,0);
	system->open_disk(0, emu->bios_path(_T("D1/INIT.D88")) ,0);
#endif
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
	display->draw_screen();
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
	psg->init(rate, 1996750, samples, 0, 0);
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
}

int VM::sound_buffer_ptr()
{
	return event->sound_buffer_ptr();
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::open_cart(int drv, _TCHAR* file_path)
{
	if(drv == 0) {
		memory->open_cart(file_path);
		reset();
	}
}

void VM::close_cart(int drv)
{
	if(drv == 0) {
		memory->close_cart();
		reset();
	}
}

bool VM::cart_inserted(int drv)
{
	if(drv == 0) {
		return memory->cart_inserted();
	} else {
		return false;
	}
}

void VM::open_disk(int drv, _TCHAR* file_path, int offset)
{
	fdc->open_disk(drv, file_path, offset);
	system->open_disk(drv, file_path, offset);
}

void VM::close_disk(int drv)
{
	fdc->close_disk(drv);
	system->close_disk(drv);
}

bool VM::disk_inserted(int drv)
{
	return fdc->disk_inserted(drv);
}

void VM::play_tape(_TCHAR* file_path)
{
	//drec->play_tape(file_path);
}

void VM::rec_tape(_TCHAR* file_path)
{
	//drec->rec_tape(file_path);
}

void VM::close_tape()
{
	//drec->close_tape();
}

bool VM::tape_inserted()
{
	//return drec->tape_inserted();
	return false;
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
