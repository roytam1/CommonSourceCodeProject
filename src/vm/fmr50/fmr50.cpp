/*
	FUJITSU FMR-50 Emulator 'eFMR-50'
	FUJITSU FMR-60 Emulator 'eFMR-60'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.28 -

	[ virtual machine ]
*/

#include "fmr50.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../beep.h"
#include "../hd46505.h"
#ifdef _FMR60
#include "../hd63484.h"
#endif
#include "../i8251.h"
#include "../i8253.h"
#include "../i8259.h"
#include "../i86.h"
#include "../i386.h"
#include "../io.h"
#include "../mb8877.h"
#include "../rtc58321.h"
#include "../upd71071.h"

#include "bios.h"
#include "cmos.h"
#include "floppy.h"
#include "keyboard.h"
#include "memory.h"
#include "scsi.h"
//#include "serial.h"
#include "timer.h"

#include "../../fileio.h"
#include "../../config.h"

// ----------------------------------------------------------------------------
// initialize
// ----------------------------------------------------------------------------

VM::VM(EMU* parent_emu) : emu(parent_emu)
{
	// check ipl
	uint8 machine_id = 0;
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sIPL.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		uint8 ipl[0x4000];
		fio->Fread(ipl, sizeof(ipl), 1);
		fio->Fclose();
		uint32 crc32 = emu->getcrc32(ipl, sizeof(ipl));
		for(int i = 0;; i++) {
			if(machine_ids[i][0] == -1) {
				break;
			}
			if(machine_ids[i][0] == crc32) {
				machine_id = machine_ids[i][1];
				break;
			}
		}
	}
#ifdef _FMRCARD
	machine_id = 0x70;
#else
	if(!machine_id) {
		_stprintf(file_path, _T("%sMACHINE.ID"), app_path);
		if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
			machine_id = fio->Fgetc();
			fio->Fclose();
		}
		else
			machine_id = 0xf8;
	}
#endif
	is_i286 = ((machine_id & 7) == 0);
	
	// create devices
	first_device = last_device = NULL;
	dummy = new DEVICE(this, emu);	// must be 1st device
	event = new EVENT(this, emu);	// must be 2nd device
	event->initialize();		// must be initialized first
	
	beep = new BEEP(this, emu);
	crtc = new HD46505(this, emu);
#ifdef _FMR60
	acrtc = new HD63484(this, emu);
#endif
	sio = new I8251(this, emu);
	pit0 = new I8253(this, emu);
	pit1 = new I8253(this, emu);
	pic = new I8259(this, emu);
	if(is_i286) {
		i286 = new I86(this, emu);
	}
	else {
		i386 = new I386(this, emu);
	}
	io = new IO(this, emu);
	fdc = new MB8877(this, emu);
	rtc = new RTC58321(this, emu);
	dma = new UPD71071(this, emu);
	
	bios = new BIOS(this, emu);
	cmos = new CMOS(this, emu);
	floppy = new FLOPPY(this, emu);
	keyboard = new KEYBOARD(this, emu);
	memory = new MEMORY(this, emu);
	scsi = new SCSI(this, emu);
//	serial = new SERIAL(this, emu);
	timer = new TIMER(this, emu);
	
	// set contexts
	if(is_i286) {
		event->set_context_cpu(i286);
	}
	else {
		event->set_context_cpu(i386);
	}
	event->set_context_sound(beep);
	
/*	pic	0	timer
		1	keyboard
		2	rs-232c
		3	ex rs-232c
		4	(option)
		5	(option)
		6	floppy drive or dma ???
		7	(slave)
		8	scsi
		9	(option)
		10	(option)
		11	(option)
		12	printer
		13	(option)
		14	(option)
		15	(reserve)

	dma	0	floppy drive
		1	hard drive
		2	(option)
		3	(reserve)
*/
	crtc->set_context_disp(memory, SIG_MEMORY_DISP, 1);
	crtc->set_context_vsync(memory, SIG_MEMORY_VSYNC, 1);
#ifdef _FMR60
	acrtc->set_vram_ptr((uint16*)memory->get_vram(), 0x80000);
#endif
	pit0->set_context_ch0(timer, SIG_TIMER_CH0, 1);
	pit0->set_context_ch1(timer, SIG_TIMER_CH1, 1);
	pit0->set_context_ch2(beep, SIG_BEEP_PULSE, 1);
	pit0->set_constant_clock(0, 307200);
	pit0->set_constant_clock(1, 307200);
	pit0->set_constant_clock(2, 307200);
	pit1->set_constant_clock(1, 1228800);
	if(is_i286) {
		pic->set_context(i286);
	}
	else {
		pic->set_context(i386);
	}
	fdc->set_context_drq(dma, SIG_UPD71071_CH0, 1);
	fdc->set_context_irq(floppy, SIG_FLOPPY_IRQ, 1);
	dma->set_context_memory(memory);
	dma->set_context_ch0(fdc);
//	dma->set_context_ch1(scsi);
	
	bios->set_context_mem(memory);
	bios->set_context_io(io);
	bios->set_cmos_ptr(cmos->get_cmos());
	bios->set_vram_ptr(memory->get_vram());
	bios->set_cvram_ptr(memory->get_cvram());
#ifdef _FMR60
	bios->set_avram_ptr(memory->get_avram());
#else
	bios->set_kvram_ptr(memory->get_kvram());
#endif
	floppy->set_context_fdc(fdc);
	floppy->set_context_pic(pic);
	keyboard->set_context_pic(pic);
	if(is_i286) {
		memory->set_context_cpu(i286);
	}
	else {
		memory->set_context_cpu(i386);
	}
	memory->set_machine_id(machine_id);
	memory->set_context_fdc(fdc);
	memory->set_context_bios(bios);
	memory->set_context_crtc(crtc);
	memory->set_chregs_ptr(crtc->get_regs());
//	scsi->set_context_dma(dma);
//	scsi->set_context_pic(pic);
	timer->set_context_beep(beep);
	timer->set_context_pic(pic);
	
	// cpu bus
	if(is_i286) {
		i286->set_context_mem(memory);
		i286->set_context_io(io);
		i286->set_context_intr(pic);
		i286->set_context_bios(bios);
	}
	else {
		i386->set_context_mem(memory);
		i386->set_context_io(io);
		i386->set_context_intr(pic);
		i386->set_context_bios(bios);
	}
	
	// i/o bus
	io->set_iomap_alias_w(0x00, pic, 0);
	io->set_iomap_alias_w(0x02, pic, 1);
	io->set_iomap_alias_w(0x10, pic, 2);
	io->set_iomap_alias_w(0x12, pic, 3);
	io->set_iomap_single_w(0x20, memory);	// reset
	io->set_iomap_alias_w(0x40, pit0, 0);
	io->set_iomap_alias_w(0x42, pit0, 1);
	io->set_iomap_alias_w(0x44, pit0, 2);
	io->set_iomap_alias_w(0x46, pit0, 3);
	io->set_iomap_alias_w(0x50, pit1, 0);
	io->set_iomap_alias_w(0x52, pit1, 1);
	io->set_iomap_alias_w(0x54, pit1, 2);
	io->set_iomap_alias_w(0x56, pit1, 3);
	io->set_iomap_single_w(0x60, timer);
	io->set_iomap_alias_w(0x70, rtc, 0);
	io->set_iomap_alias_w(0x80, rtc, 1);
#ifdef _FMRCARD
	io->set_iomap_single_w(0x90, cmos);
#endif
	io->set_iomap_range_w(0xa0, 0xaf, dma);
	io->set_iomap_alias_w(0x200, fdc, 0);
	io->set_iomap_alias_w(0x202, fdc, 1);
	io->set_iomap_alias_w(0x204, fdc, 2);
	io->set_iomap_alias_w(0x206, fdc, 3);
	io->set_iomap_single_w(0x208, floppy);
	io->set_iomap_single_w(0x20c, floppy);
	io->set_iomap_single_w(0x400, memory);	// crtc
	io->set_iomap_single_w(0x402, memory);	// crtc
	io->set_iomap_single_w(0x404, memory);	// crtc
	io->set_iomap_single_w(0x408, memory);	// crtc
	io->set_iomap_single_w(0x40a, memory);	// crtc
	io->set_iomap_single_w(0x40c, memory);	// crtc
	io->set_iomap_single_w(0x40e, memory);	// crtc
	io->set_iomap_alias_w(0x500, crtc, 0);
	io->set_iomap_alias_w(0x502, crtc, 1);
#ifdef _FMR60
	io->set_iomap_range_w(0x520, 0x523, acrtc);
#endif
	io->set_iomap_single_w(0x600, keyboard);
	io->set_iomap_single_w(0x602, keyboard);
	io->set_iomap_single_w(0x604, keyboard);
	io->set_iomap_alias_w(0xa00, sio, 0);
	io->set_iomap_alias_w(0xa02, sio, 1);
//	io->set_iomap_single_w(0xa08, serial);
	io->set_iomap_single_w(0xc30, scsi);
	io->set_iomap_single_w(0xc32, scsi);
	io->set_iomap_range_w(0x3000, 0x3fff, cmos);
	io->set_iomap_single_w(0xfd90, memory);	// crtc
	io->set_iomap_single_w(0xfd92, memory);	// crtc
	io->set_iomap_single_w(0xfd94, memory);	// crtc
	io->set_iomap_single_w(0xfd96, memory);	// crtc
	io->set_iomap_range_w(0xfd98, 0xfd9f, memory);	// crtc
	io->set_iomap_single_w(0xfda0, memory);	// crtc
	
	io->set_iomap_alias_r(0x00, pic, 0);
	io->set_iomap_alias_r(0x02, pic, 1);
	io->set_iomap_alias_r(0x10, pic, 2);
	io->set_iomap_alias_r(0x12, pic, 3);
	io->set_iomap_single_r(0x20, memory);	// reset
	io->set_iomap_single_r(0x21, memory);	// cpu misc
	io->set_iomap_single_r(0x30, memory);	// cpu id
	io->set_iomap_alias_r(0x40, pit0, 0);
	io->set_iomap_alias_r(0x42, pit0, 1);
	io->set_iomap_alias_r(0x44, pit0, 2);
	io->set_iomap_alias_r(0x46, pit0, 3);
	io->set_iomap_alias_r(0x50, pit1, 0);
	io->set_iomap_alias_r(0x52, pit1, 1);
	io->set_iomap_alias_r(0x54, pit1, 2);
	io->set_iomap_alias_r(0x56, pit1, 3);
	io->set_iomap_single_r(0x60, timer);
	io->set_iomap_alias_r(0x70, rtc, 0);
	io->set_iomap_alias_r(0x80, rtc, 1);
	io->set_iomap_range_r(0xa0, 0xaf, dma);
	io->set_iomap_single_r(0x200, bios);
	io->set_iomap_single_r(0x202, bios);
	io->set_iomap_alias_r(0x200, fdc, 0);
	io->set_iomap_alias_r(0x202, fdc, 1);
	io->set_iomap_alias_r(0x204, fdc, 2);
	io->set_iomap_alias_r(0x206, fdc, 3);
	io->set_iomap_single_r(0x208, floppy);
	io->set_iomap_single_r(0x20c, floppy);
	io->set_iomap_single_r(0x400, memory);	// crtc
	io->set_iomap_single_r(0x402, memory);	// crtc
	io->set_iomap_single_r(0x404, memory);	// crtc
	io->set_iomap_single_r(0x40a, memory);	// crtc
	io->set_iomap_single_r(0x40c, memory);	// crtc
	io->set_iomap_single_r(0x40e, memory);	// crtc
	io->set_iomap_alias_r(0x500, crtc, 0);
	io->set_iomap_alias_r(0x502, crtc, 1);
#ifdef _FMR60
	io->set_iomap_range_r(0x520, 0x523, acrtc);
#endif
	io->set_iomap_single_r(0x600, keyboard);
	io->set_iomap_single_r(0x602, keyboard);
	io->set_iomap_single_r(0x604, keyboard);
	io->set_iomap_alias_r(0xa00, sio, 0);
	io->set_iomap_alias_r(0xa02, sio, 1);
//	io->set_iomap_single_r(0xa04, serial);
//	io->set_iomap_single_r(0xa06, serial);
	io->set_iomap_single_r(0xc30, scsi);
	io->set_iomap_single_r(0xc32, scsi);
	io->set_iomap_range_r(0x3000, 0x3fff, cmos);
	io->set_iomap_single_r(0xfd92, memory);	// crtc
	io->set_iomap_single_r(0xfd94, memory);	// crtc
	io->set_iomap_single_r(0xfd96, memory);	// crtc
	io->set_iomap_range_r(0xfd98, 0xfd9f, memory);	// crtc
	io->set_iomap_single_r(0xfda0, memory);	// crtc
	
	// initialize all devices
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id) {
			device->initialize();
		}
	}
	for(int i = 0; i < MAX_DRIVE; i++) {
		bios->set_disk_handler(i, fdc->get_disk_handler(i));
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
	// temporary fix...
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
	if(is_i286) {
		return i286->get_prv_pc();
	}
	else {
		return i386->get_prv_pc();
	}
}

// ----------------------------------------------------------------------------
// draw screen
// ----------------------------------------------------------------------------

void VM::draw_screen()
{
	memory->draw_screen();
}

// ----------------------------------------------------------------------------
// soud manager
// ----------------------------------------------------------------------------

void VM::initialize_sound(int rate, int samples)
{
	// init sound manager
	event->initialize_sound(rate, samples);
	
	// init sound gen
	beep->init(rate, -1, 2, 8000);
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
	keyboard->key_down(code);
}

void VM::key_up(int code)
{
	keyboard->key_up(code);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void VM::open_disk(_TCHAR* filename, int drv)
{
	fdc->open_disk(filename, drv);
	floppy->change_disk(drv);
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

