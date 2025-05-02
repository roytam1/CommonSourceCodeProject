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

#include "../hd46505.h"
#ifdef _FMR60
#include "../hd63484.h"
#endif
#include "../i8251.h"
#include "../i8253.h"
#include "../i8259.h"
#include "../i286.h"
#include "../i386.h"
#include "../io.h"
#include "../mb8877.h"
#include "../msm58321.h"
#include "../pcm1bit.h"
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
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(emu->bios_path(_T("IPL.ROM")), FILEIO_READ_BINARY)) {
		uint8 ipl[0x4000];
		fio->Fread(ipl, sizeof(ipl), 1);
		fio->Fclose();
		uint32 crc32 = getcrc32(ipl, sizeof(ipl));
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
		if(fio->Fopen(emu->bios_path(_T("MACHINE.ID")), FILEIO_READ_BINARY)) {
			machine_id = fio->Fgetc();
			fio->Fclose();
		}
		else {
			machine_id = 0xf8;
		}
	}
#endif
	delete fio;
	is_i286 = ((machine_id & 7) == 0);
	
	// create devices
	first_device = last_device = NULL;
	dummy = new DEVICE(this, emu);	// must be 1st device
	event = new EVENT(this, emu);	// must be 2nd device
	
	if(is_i286) {
		i286 = new I286(this, emu);
	}
	else {
		i386 = new I386(this, emu);
	}
	crtc = new HD46505(this, emu);
#ifdef _FMR60
	acrtc = new HD63484(this, emu);
#endif
	sio = new I8251(this, emu);
	pit0 = new I8253(this, emu);
	pit1 = new I8253(this, emu);
	pic = new I8259(this, emu);
	io = new IO(this, emu);
	fdc = new MB8877(this, emu);
	rtc = new MSM58321(this, emu);
	pcm = new PCM1BIT(this, emu);
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
	event->set_context_sound(pcm);
	
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
	pit0->set_context_ch2(pcm, SIG_PCM1BIT_SIGNAL, 1);
	pit0->set_constant_clock(0, 307200);
	pit0->set_constant_clock(1, 307200);
	pit0->set_constant_clock(2, 307200);
	pit1->set_constant_clock(1, 1228800);
	if(is_i286) {
		pic->set_context_cpu(i286);
	}
	else {
		pic->set_context_cpu(i386);
	}
	fdc->set_context_drq(dma, SIG_UPD71071_CH0, 1);
	fdc->set_context_irq(floppy, SIG_FLOPPY_IRQ, 1);
	rtc->set_context_data(timer, SIG_TIMER_RTC, 0x0f, 0);
	rtc->set_context_busy(timer, SIG_TIMER_RTC, 0x80);
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
	memory->set_context_crtc(crtc);
	memory->set_chregs_ptr(crtc->get_regs());
//	scsi->set_context_dma(dma);
//	scsi->set_context_pic(pic);
	timer->set_context_pcm(pcm);
	timer->set_context_pic(pic);
	timer->set_context_rtc(rtc);
	
	// cpu bus
	if(is_i286) {
		i286->set_context_mem(memory);
		i286->set_context_io(io);
		i286->set_context_intr(pic);
		i286->set_context_bios(bios);
#ifdef SINGLE_MODE_DMA
		i286->set_context_dma(dma);
#endif
	}
	else {
		i386->set_context_mem(memory);
		i386->set_context_io(io);
		i386->set_context_intr(pic);
		i386->set_context_bios(bios);
#ifdef SINGLE_MODE_DMA
		i386->set_context_dma(dma);
#endif
	}
	
	// i/o bus
	io->set_iomap_alias_rw(0x00, pic, I8259_ADDR_CHIP0 | 0);
	io->set_iomap_alias_rw(0x02, pic, I8259_ADDR_CHIP0 | 1);
	io->set_iomap_alias_rw(0x10, pic, I8259_ADDR_CHIP1 | 0);
	io->set_iomap_alias_rw(0x12, pic, I8259_ADDR_CHIP1 | 1);
	io->set_iomap_single_rw(0x20, memory);	// reset
	io->set_iomap_single_r(0x21, memory);	// cpu misc
	io->set_iomap_single_r(0x30, memory);	// cpu id
	io->set_iomap_alias_rw(0x40, pit0, 0);
	io->set_iomap_alias_rw(0x42, pit0, 1);
	io->set_iomap_alias_rw(0x44, pit0, 2);
	io->set_iomap_alias_rw(0x46, pit0, 3);
	io->set_iomap_alias_rw(0x50, pit1, 0);
	io->set_iomap_alias_rw(0x52, pit1, 1);
	io->set_iomap_alias_rw(0x54, pit1, 2);
	io->set_iomap_alias_rw(0x56, pit1, 3);
	io->set_iomap_single_rw(0x60, timer);
	io->set_iomap_single_rw(0x70, timer);
	io->set_iomap_single_w(0x80, timer);
#ifdef _FMRCARD
	io->set_iomap_single_w(0x90, cmos);
#endif
	io->set_iomap_range_rw(0xa0, 0xaf, dma);
	io->set_iomap_alias_rw(0x200, fdc, 0);
	io->set_iomap_alias_rw(0x202, fdc, 1);
	io->set_iomap_alias_rw(0x204, fdc, 2);
	io->set_iomap_alias_rw(0x206, fdc, 3);
	io->set_iomap_single_rw(0x208, floppy);
	io->set_iomap_single_rw(0x20c, floppy);
	io->set_iomap_single_rw(0x400, memory);	// crtc
	io->set_iomap_single_rw(0x402, memory);	// crtc
	io->set_iomap_single_rw(0x404, memory);	// crtc
	io->set_iomap_single_w(0x408, memory);	// crtc
	io->set_iomap_single_rw(0x40a, memory);	// crtc
	io->set_iomap_single_rw(0x40c, memory);	// crtc
	io->set_iomap_single_rw(0x40e, memory);	// crtc
	io->set_iomap_alias_rw(0x500, crtc, 0);
	io->set_iomap_alias_rw(0x502, crtc, 1);
#ifdef _FMR60
	io->set_iomap_range_rw(0x520, 0x523, acrtc);
#endif
	io->set_iomap_single_rw(0x600, keyboard);
	io->set_iomap_single_rw(0x602, keyboard);
	io->set_iomap_single_rw(0x604, keyboard);
	io->set_iomap_alias_rw(0xa00, sio, 0);
	io->set_iomap_alias_rw(0xa02, sio, 1);
//	io->set_iomap_single_r(0xa04, serial);
//	io->set_iomap_single_r(0xa06, serial);
//	io->set_iomap_single_w(0xa08, serial);
	io->set_iomap_single_rw(0xc30, scsi);
	io->set_iomap_single_rw(0xc32, scsi);
	io->set_iomap_range_rw(0x3000, 0x3fff, cmos);
	io->set_iomap_single_w(0xfd90, memory);		// crtc
	io->set_iomap_single_rw(0xfd92, memory);	// crtc
	io->set_iomap_single_rw(0xfd94, memory);	// crtc
	io->set_iomap_single_rw(0xfd96, memory);	// crtc
	io->set_iomap_range_rw(0xfd98, 0xfd9f, memory);	// crtc
	io->set_iomap_single_rw(0xfda0, memory);	// crtc
	
	// initialize all devices
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->initialize();
	}
	for(int i = 0; i < MAX_DRIVE; i++) {
		bios->set_disk_handler(i, fdc->get_disk_handler(i));
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
// draw screen
// ----------------------------------------------------------------------------

void VM::draw_screen()
{
	memory->draw_screen();
}

int VM::access_lamp()
{
	uint32 status = fdc->read_signal(0) | bios->read_signal(0);
	return (status & 0x10) ? 4 : (status & (1 | 4)) ? 1 : (status & (2 | 8)) ? 2 : 0;
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
}

uint16* VM::create_sound(int* extra_frames)
{
	return event->create_sound(extra_frames);
}

// ----------------------------------------------------------------------------
// notify key
// ----------------------------------------------------------------------------

void VM::key_down(int code, bool repeat)
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

void VM::open_disk(int drv, _TCHAR* file_path, int offset)
{
	fdc->open_disk(drv, file_path, offset);
	floppy->change_disk(drv);
}

void VM::close_disk(int drv)
{
	fdc->close_disk(drv);
}

bool VM::disk_inserted(int drv)
{
	return fdc->disk_inserted(drv);
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

