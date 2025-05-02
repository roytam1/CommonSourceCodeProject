/*
	EPSON HC-20 Emulator 'eHC-20'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2011.05.23-

	[ virtual machine ]
*/

#include "hc20.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../beep.h"
#include "../hd146818p.h"
#include "../mc6800.h"
#include "../tf20.h"

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
	rtc = new HD146818P(this, emu);
	cpu = new MC6800(this, emu);
	tf20 = new TF20(this, emu);
	
	memory = new MEMORY(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(beep);
	
/*
	memory:
		0002	in	---	port1 (cpu)
		0003	in/out	---	port2 (cpu)
		0006		---	port3 (cpu)
		0007		---	port4 (cpu)
		0020	out	bit0-7	key scan line
		0022	in	bit0-7	key scan result (lo)
		0026	out	bit0-2	selection of lcd driver (0,1-6)
				bit3	output selection for lcd driver (0=data 1=command)
				bit4	key input interrupt mask (0=Mask)
				bit5	pout (serial control line)
				bit6	shift/load select for rom cartridge (0=load 1=shift)
				bit7	clock for rom cartridge
		0028	in	bit0-1	key scan result (hi)
				bit6	power switch interrupt flag (0=active)
				bit7	busy signal of lcd controller (0=busy)
		002a	out	bit0-7	output data to lcd controller
			in	---	serial clock to lcd controller
		002b	in	---	serial clock to lcd controller
		002c	in/out	---	used for interrupt mask setting in sleep mode
		0030	in/out	---	select expansion unit rom (bank1)
		0032	in/out	---	select internal rom (bank0)
		0033	in/out	---	select internal rom (bank0)
		003c	in	---	XXX: unknown

	port1:
		p10	in	dsr (RS-232C)
		p11	in	cts (RS-232C)
		p12	in	error status of slave mcu (P34)
		p13	in	external interrupt flag(0=active)
		p14	in	battery voltage interrupt flag (0=active)
		p15	in	key input inerrupt flag (0=active)
		p16	in	pin (serial control line)
		p17	in	counter status of microcassete / rom data / plug-in option

	port 2:
		p20	in	barcode input signal (1=mark 0=space)
		p21	out	txd (RS-232C)
		p22	out	selection for CPU serial communication (0=slave 1=serial)
*/	
	cpu->set_context_port2(memory, SIG_MEMORY_PORT_2, 0xff, 0);
	cpu->set_context_port3(memory, SIG_MEMORY_PORT_3, 0xff, 0);
	cpu->set_context_port4(memory, SIG_MEMORY_PORT_4, 0xff, 0);
	cpu->set_context_sio(memory, SIG_MEMORY_SIO_MAIN);
	rtc->set_context_intr(memory, SIG_MEMORY_RTC_IRQ, 1);
	tf20->set_context_sio(memory, SIG_MEMORY_SIO_TF20);
	
	memory->set_context_beep(beep);
	memory->set_context_cpu(cpu);
	memory->set_context_rtc(rtc);
	memory->set_context_tf20(tf20);
	
	// cpu bus
	cpu->set_context_mem(memory);
	
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
	cpu->write_signal(SIG_MC6801_PORT_1, 0x78, 0xff);
	cpu->write_signal(SIG_MC6801_PORT_2, 0x9e, 0xff);
}

void VM::notify_power_off()
{
//	emu->out_debug("--- POWER OFF ---\n");
	memory->notify_power_off();
}

void VM::run()
{
	event->drive();
}

// ----------------------------------------------------------------------------
// event manager
// ----------------------------------------------------------------------------

void VM::register_event(DEVICE* dev, int event_id, int usec, bool loop, int* register_id)
{
	event->register_event(dev, event_id, usec, loop, register_id);
}

void VM::register_event_by_clock(DEVICE* dev, int event_id, int clock, bool loop, int* register_id)
{
	event->register_event_by_clock(dev, event_id, clock, loop, register_id);
}

void VM::cancel_event(int register_id)
{
	event->cancel_event(register_id);
}

void VM::register_frame_event(DEVICE* dev)
{
	event->register_frame_event(dev);
}

void VM::register_vline_event(DEVICE* dev)
{
	event->register_vline_event(dev);
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
	memory->draw_screen();
}

int VM::access_lamp()
{
	uint32 status = tf20->read_signal(0);
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
	beep->init(rate, 1000, 8000);
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::open_disk(_TCHAR* filename, int drv)
{
	tf20->open_disk(filename, drv);
}

void VM::close_disk(int drv)
{
	tf20->close_disk(drv);
}

void VM::play_datarec(_TCHAR* filename)
{
	memory->play_datarec(filename);
}

void VM::rec_datarec(_TCHAR* filename)
{
	memory->rec_datarec(filename);
}

void VM::close_datarec()
{
	memory->close_datarec();
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

