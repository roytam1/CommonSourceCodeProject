/*
	NEC PC-6001 Emulator 'yaPC-6001'
	NEC PC-6001mk2 Emulator 'yaPC-6201'
	NEC PC-6601 Emulator 'yaPC-6601'
	PC-6801 Emulator 'PC-6801'

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
#include "../ym2203.h"
#include "../z80.h"
#include "system.h"
#ifdef _PC6001
#include "../mc6847.h"
#include "display.h"
#else
#include "../pd7752.h"
#endif
#include "joystick.h"
#include "memory.h"
#include "keyboard.h"

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
	psg = new YM2203(this, emu);
	cpu = new Z80(this, emu);
	joystick = new JOYSTICK(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(psg);
	system = new SYSTEM(this, emu);
	system->set_context_pio(pio_f);
#ifdef _PC6001
	display = new DISPLAY(this, emu);
	vdp = new MC6847(this, emu);
	display->set_context_vdp(vdp);
	display->set_vram_ptr(memory->get_vram());
	display->set_context_cpu(cpu);
	display->set_context_key(keyboard);
#else
	voice = new PD7752(this, emu);
	event->set_context_sound(voice);
#endif
	joystick->set_context_psg(psg);
	memory->set_context_cpu(cpu);
	keyboard->set_context_cpu(cpu);
	keyboard->set_context_pio(pio_k);
	keyboard->set_context_mem(memory);
	pio_k->set_context_port_a(keyboard, SIG_DATAREC_REMOTE, 0x08, 0);
	pio_k->set_context_port_a(keyboard, SIG_DATAREC_OUT, 0x10, 0);
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(keyboard);
	// i/o bus
	io->set_iomap_range_rw(0x90, 0x92, keyboard);
	io->set_iomap_single_rw(0x93, memory);			// CGSW93
	io->set_iomap_alias_w(0xa0, psg, 0);			// PSG ch
	io->set_iomap_alias_w(0xa1, psg, 1);			// PSG data
	io->set_iomap_alias_r(0xa2, psg, 1);			// PSG data
#ifdef _PC6801
	io->set_iomap_alias_r(0xa3, psg, 1);			// FM status
	io->set_iomap_range_rw(0x40, 0x6f, memory);		// VRAM addr
	io->set_iomap_single_rw(0xbc, keyboard);		// VRTC
#endif
#ifdef _PC6001
	vdp->load_font_image(emu->bios_path(_T("CGROM60.60")));
	vdp->set_context_cpu(cpu);
	io->set_iomap_single_w(0xb0, display);			// VRAM addr
#else
	io->set_iomap_single_w(0xb0, memory);			// VRAM addr
	io->set_iomap_range_rw(0xc0, 0xcf, memory);		// VRAM addr
	io->set_iomap_range_rw(0xe0, 0xe3, voice);		// VOICE
#endif
	io->set_iomap_range_rw(0xb1, 0xb3, system);		// DISK DRIVE
	io->set_iomap_range_rw(0xd0, 0xde, system);		// DISK DRIVE
	io->set_iomap_range_rw(0xf0, 0xf2, memory);		// MEMORY MAP
	io->set_iomap_range_rw(0xf3, 0xfb, keyboard);	// VRAM addr
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
#ifdef _PC6001
	display->draw_screen();
#else
	memory->draw_screen();
#endif
}
// ----------------------------------------------------------------------------
// soud manager
// ----------------------------------------------------------------------------

void VM::initialize_sound(int rate, int samples)
{
	// init sound manager
	event->initialize_sound(rate, samples);
	
	// init sound gen
	psg->init(rate, 4000000, samples, 0, 0);
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

int VM::access_lamp()
{
	uint32 status = 0; /// fdc->read_signal(0);
	return (status & (1 | 4)) ? 1 : (status & (2 | 8)) ? 2 : 0;
}

void VM::open_disk(int drv, _TCHAR* file_path, int offset)
{
	system->open_disk(drv, file_path, offset);
}

void VM::close_disk(int drv)
{
	system->close_disk(drv);
}

bool VM::disk_inserted(int drv)
{
	return system->disk_inserted(drv);
}

void VM::play_tape(_TCHAR* file_path)
{
	keyboard->play_tape(file_path);
}

void VM::rec_tape(_TCHAR* file_path)
{
	keyboard->rec_tape(file_path);
}

void VM::close_tape()
{
	keyboard->close_tape();
}

bool VM::tape_inserted()
{
	return keyboard->tape_inserted();
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
