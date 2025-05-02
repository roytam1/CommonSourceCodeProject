/*
	NEC PC-9801 Emulator 'ePC-9801'
	NEC PC-9801E/F/M Emulator 'ePC-9801E'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.09.15-

	[ virtual machine ]
*/

#include "pc9801.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../beep.h"
#include "../disk.h"
#include "../i8237.h"
#include "../i8251.h"
#include "../i8253.h"
#include "../i8255.h"
#include "../i8259.h"
#include "../i86.h"
#include "../io.h"
#include "../ls244.h"
#include "../memory.h"
#include "../not.h"
#include "../upd1990a.h"
#include "../upd7220.h"
#include "../upd765a.h"
#include "../ym2203.h"

#include "cmt.h"
#include "display.h"
#include "floppy.h"
#include "joystick.h"
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
	event->initialize();		// must be initialized first
	
	beep = new BEEP(this, emu);
	dma = new I8237(this, emu);
	sio_cmt = new I8251(this, emu);	// for cmt
	sio_rs = new I8251(this, emu);	// for rs232c
	sio_kbd = new I8251(this, emu);	// for keyboard
	pit = new I8253(this, emu);
	pio_sys = new I8255(this, emu);	// for system port
	pio_prn = new I8255(this, emu);	// for printer
	pio_fdd = new I8255(this, emu);	// for 320kb fdd i/f
	pic = new I8259(this, emu);
	cpu = new I86(this, emu);
	io = new IO(this, emu);
	dmareg1 = new LS244(this, emu);
	dmareg2 = new LS244(this, emu);
	dmareg3 = new LS244(this, emu);
	dmareg0 = new LS244(this, emu);
	memory = new MEMORY(this, emu);
	not = new NOT(this, emu);
	rtc = new UPD1990A(this, emu);
	gdc_chr = new UPD7220(this, emu);
	gdc_gfx = new UPD7220(this, emu);
	fdc_2hd = new UPD765A(this, emu);
	fdc_2dd = new UPD765A(this, emu);
	opn = new YM2203(this, emu);
	
	cmt = new CMT(this, emu);
	display = new DISPLAY(this, emu);
	floppy = new FLOPPY(this, emu);
	joystick = new JOYSTICK(this, emu);
	keyboard = new KEYBOARD(this, emu);
	
	/* IRQ	0  PIT
		1  KEYBOARD
		2  CRTV
		3  
		4  RS-232C
		5  
		6  
		7  SLAVE PIC
		8  PRINTER
		9  
		10 FDC (640KB I/F)
		11 FDC (1MB I/F)
		12 PC-9801-26(K)
		13 MOUSE
		14 
		15 (RESERVED)
	*/

	// set contexts
	event->set_context_cpu(cpu);
	event->set_context_sound(beep);
	event->set_context_sound(opn);
	
	dma->set_context_memory(memory);
	// dma ch.0: sasi
	// dma ch.1: memory refresh
	dma->set_context_ch2(fdc_2hd);	// 1MB
	dma->set_context_ch3(fdc_2dd);	// 640KB
	sio_cmt->set_context_out(cmt, SIG_CMT_OUT);
//	sio_cmt->set_context_txrdy(cmt, SIG_CMT_TXRDY, 1);
//	sio_cmt->set_context_rxrdy(cmt, SIG_CMT_RXRDY, 1);
//	sio_cmt->set_context_txe(cmt, SIG_CMT_TXEMP, 1);
//	sio_rs->set_context_rxrdy(pic, SIG_I8259_CHIP0 | SIG_I8259_IR4, 1);
	sio_kbd->set_context_rxrdy(pic, SIG_I8259_CHIP0 | SIG_I8259_IR1, 1);
	pit->set_context_ch0(pic, SIG_I8259_CHIP0 | SIG_I8259_IR0, 1);
	// pit ch.1: memory refresh
	// pit ch.2: rs-232c
#ifdef _PC9801
	pit->set_constant_clock(0, 2457600);
	pit->set_constant_clock(1, 2457600);
	pit->set_constant_clock(2, 2457600);
#else
	pit->set_constant_clock(0, 1996800);
	pit->set_constant_clock(1, 1996800);
	pit->set_constant_clock(2, 1996800);
#endif
	// sysport port.c bit6: printer strobe
	pio_sys->set_context_port_c(beep, SIG_BEEP_MUTE, 8, 0);
	// sysport port.c bit2: enable txrdy interrupt
	// sysport port.c bit1: enable txempty interrupt
	// sysport port.c bit0: enable rxrdy interrupt
	pio_prn->set_context_port_c(not, SIG_NOT_INPUT, 8, 0);
	dmareg1->set_context_output(dma, SIG_I8237_BANK1, 0x0f, 0);
	dmareg2->set_context_output(dma, SIG_I8237_BANK2, 0x0f, 0);
	dmareg3->set_context_output(dma, SIG_I8237_BANK3, 0x0f, 0);
	dmareg0->set_context_output(dma, SIG_I8237_BANK0, 0x0f, 0);
	not->set_context_out(pic, SIG_I8259_CHIP1 | SIG_I8259_IR0, 1);
	pic->set_context_cpu(cpu);
	rtc->set_context_dout(pio_sys, SIG_I8255_PORT_B, 1);
	fdc_2hd->set_context_irq(pic, SIG_I8259_CHIP1 | SIG_I8259_IR3, 1);
	fdc_2hd->set_context_drq(dma, SIG_I8237_CH2, 1);
	fdc_2dd->set_context_irq(pic, SIG_I8259_CHIP1 | SIG_I8259_IR2, 1);
	fdc_2dd->set_context_drq(dma, SIG_I8237_CH3, 1);
	opn->set_context_irq(pic, SIG_I8259_CHIP1 | SIG_I8259_IR4, 1);
	opn->set_context_port_b(joystick, SIG_JOYSTICK_SELECT, 0xc0, 0);
	
	cmt->set_context_sio(sio_cmt);
	display->set_context_fdc_2hd(fdc_2hd);
	display->set_context_fdc_2dd(fdc_2dd);
	display->set_context_pic(pic);
	display->set_context_gdc_chr(gdc_chr, gdc_chr->get_ra());
	display->set_context_gdc_gfx(gdc_gfx, gdc_gfx->get_ra(), gdc_gfx->get_cs());
	floppy->set_context_fdc_2hd(fdc_2hd);
	floppy->set_context_fdc_2dd(fdc_2dd);
	floppy->set_context_pic(pic);
	joystick->set_context_opn(opn);
	keyboard->set_context_sio(sio_kbd);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
	cpu->set_context_intr(pic);
#ifdef SINGLE_MODE_DMA
	cpu->set_context_dma(dma);
#endif
	
	// memory bus
	_memset(ram, 0, sizeof(ram));
	_memset(ipl, 0xff, sizeof(ipl));
	_memset(sound_bios, 0xff, sizeof(sound_bios));
	_memset(fd_bios_2hd, 0xff, sizeof(fd_bios_2hd));
	_memset(fd_bios_2dd, 0xff, sizeof(fd_bios_2dd));
	
	memory->read_bios(_T("IPL.ROM"), ipl, sizeof(ipl));
	int sound_bios_ok = memory->read_bios(_T("SOUND.ROM"), sound_bios, sizeof(sound_bios));
	memory->read_bios(_T("2HDIF.ROM"), fd_bios_2hd, sizeof(fd_bios_2hd));
	memory->read_bios(_T("2DDIF.ROM"), fd_bios_2dd, sizeof(fd_bios_2dd));
	
	memory->set_memory_rw(0x00000, 0x9ffff, ram);
	// A0000h - A1FFFh: TEXT VRAM
	// A2000h - A3FFFh: ATTRIBUTE
	memory->set_memory_mapped_io_rw(0xa0000, 0xa3fff, display);
	// A8000h - BFFFFh: VRAM
	memory->set_memory_mapped_io_rw(0xa8000, 0xbffff, display);
	memory->set_memory_r(0xcc000, 0xcdfff, sound_bios);
	memory->set_memory_r(0xd6000, 0xd6fff, fd_bios_2dd);
	memory->set_memory_r(0xd7000, 0xd7fff, fd_bios_2hd);
	memory->set_memory_r(0xe8000, 0xfffff, ipl);
	
	display->sound_bios_ok = (sound_bios_ok != 0);	// memory switch
	
	// i/o bus
	io->set_iomap_alias_rw(0x00, pic, 0);
	io->set_iomap_alias_rw(0x02, pic, 1);
	io->set_iomap_alias_rw(0x08, pic, 2);
	io->set_iomap_alias_rw(0x0a, pic, 3);
	
	io->set_iomap_alias_rw(0x01, dma, 0x00);
	io->set_iomap_alias_rw(0x03, dma, 0x01);
	io->set_iomap_alias_rw(0x05, dma, 0x02);
	io->set_iomap_alias_rw(0x07, dma, 0x03);
	io->set_iomap_alias_rw(0x09, dma, 0x04);
	io->set_iomap_alias_rw(0x0b, dma, 0x05);
	io->set_iomap_alias_rw(0x0d, dma, 0x06);
	io->set_iomap_alias_rw(0x0f, dma, 0x07);
	io->set_iomap_alias_rw(0x11, dma, 0x08);
	io->set_iomap_alias_w(0x13, dma, 0x09);
	io->set_iomap_alias_w(0x15, dma, 0x0a);
	io->set_iomap_alias_w(0x17, dma, 0x0b);
	io->set_iomap_alias_w(0x19, dma, 0x0c);
	io->set_iomap_alias_rw(0x1b, dma, 0x0d);
	io->set_iomap_alias_w(0x1d, dma, 0x0e);
	io->set_iomap_alias_w(0x1f, dma, 0x0f);
	io->set_iomap_single_w(0x21, dmareg1);
	io->set_iomap_single_w(0x23, dmareg2);
	io->set_iomap_single_w(0x25, dmareg3);
	io->set_iomap_single_w(0x27, dmareg0);
	
	io->set_iomap_single_w(0x20, rtc);
	
	io->set_iomap_alias_rw(0x30, sio_rs, 0);
	io->set_iomap_alias_rw(0x32, sio_rs, 1);
	
	io->set_iomap_alias_rw(0x31, pio_sys, 0);
	io->set_iomap_alias_rw(0x33, pio_sys, 1);
	io->set_iomap_alias_rw(0x35, pio_sys, 2);
	io->set_iomap_alias_w(0x37, pio_sys, 3);
	
	io->set_iomap_alias_rw(0x40, pio_prn, 0);
	io->set_iomap_alias_rw(0x42, pio_prn, 1);
	io->set_iomap_alias_rw(0x44, pio_prn, 2);
	io->set_iomap_alias_w(0x46, pio_prn, 3);
	
	io->set_iomap_alias_rw(0x41, sio_kbd, 0);
	io->set_iomap_alias_rw(0x43, sio_kbd, 1);
	
	// 50h, 52h: NMI Flip Flop
	
	io->set_iomap_alias_rw(0x51, pio_fdd, 0);
	io->set_iomap_alias_rw(0x53, pio_fdd, 1);
	io->set_iomap_alias_rw(0x55, pio_fdd, 2);
	io->set_iomap_alias_w(0x57, pio_fdd, 3);
	
	io->set_iomap_alias_rw(0x60, gdc_chr, 0);
	io->set_iomap_alias_rw(0x62, gdc_chr, 1);
	
	io->set_iomap_single_w(0x64, display);
	io->set_iomap_single_w(0x68, display);
	io->set_iomap_single_w(0x6c, display);
	io->set_iomap_single_w(0x6e, display);
	
	io->set_iomap_single_w(0x70, display);
	io->set_iomap_single_w(0x72, display);
	io->set_iomap_single_w(0x74, display);
	io->set_iomap_single_w(0x76, display);
	io->set_iomap_single_w(0x78, display);
	io->set_iomap_single_w(0x7a, display);
	
	io->set_iomap_alias_rw(0x71, pit, 0);
	io->set_iomap_alias_rw(0x73, pit, 1);
	io->set_iomap_alias_rw(0x75, pit, 2);
	io->set_iomap_alias_w(0x77, pit, 3);
	
	// 80h, 82h: SASI
	
	io->set_iomap_alias_rw(0x90, fdc_2hd, 0);
	io->set_iomap_alias_rw(0x92, fdc_2hd, 1);
#ifdef _PC9801
	io->set_iomap_single_w(0x94, floppy);
#else
	io->set_iomap_single_rw(0x94, floppy);
#endif
	
	io->set_iomap_alias_rw(0x91, sio_cmt, 0);
	io->set_iomap_alias_rw(0x93, sio_cmt, 1);
	io->set_iomap_single_w(0x95, cmt);
	io->set_iomap_single_w(0x97, cmt);
	
	io->set_iomap_alias_rw(0xa0, gdc_gfx, 0);
	io->set_iomap_alias_rw(0xa2, gdc_gfx, 1);
	
#ifndef _PC9801
	io->set_iomap_single_w(0xa4, display);
	io->set_iomap_single_w(0xa6, display);
#endif
	io->set_iomap_single_rw(0xa8, display);
	io->set_iomap_single_rw(0xaa, display);
	io->set_iomap_single_rw(0xac, display);
	io->set_iomap_single_rw(0xae, display);
	
	io->set_iomap_single_w(0xa1, display);
	io->set_iomap_single_w(0xa3, display);
	io->set_iomap_single_w(0xa5, display);
	io->set_iomap_single_rw(0xa9, display);
	
	io->set_iomap_alias_rw(0xc8, fdc_2dd, 0);
	io->set_iomap_alias_rw(0xca, fdc_2dd, 1);
	io->set_iomap_single_rw(0xcc, floppy);
	
	io->set_iomap_alias_rw(0x188, opn, 0);
	io->set_iomap_alias_rw(0x18a, opn, 1);
	
	// initialize all devices
	for(DEVICE* device = first_device; device; device = device->next_device) {
		if(device->this_device_id != event->this_device_id) {
			device->initialize();
		}
	}
	for(int i = 0; i < 4; i++) {
		fdc_2hd->set_drive_type(i, DRIVE_TYPE_2HD);
		fdc_2dd->set_drive_type(i, DRIVE_TYPE_2DD);
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
	
	// initial device settings
	pio_sys->write_signal(SIG_I8255_PORT_A, 0xe3, 0xff);
	pio_sys->write_signal(SIG_I8255_PORT_B, 0xe8, 0xff);
#ifdef _PC9801
	pio_prn->write_signal(SIG_I8255_PORT_B, 0x1c, 0xff);
#else
	pio_prn->write_signal(SIG_I8255_PORT_B, 0xbc, 0xff);
#endif
	pio_fdd->write_signal(SIG_I8255_PORT_A, 0xff, 0xff);
	pio_fdd->write_signal(SIG_I8255_PORT_B, 0xff, 0xff);
	pio_fdd->write_signal(SIG_I8255_PORT_C, 0xff, 0xff);
	fdc_2dd->write_signal(SIG_UPD765A_FREADY, 1, 1);	// 2DD FDC RDY is pulluped
	opn->write_signal(SIG_YM2203_PORT_A, 0xff, 0xff);	// PC-9801-26(K) IRQ=12
	beep->write_signal(SIG_BEEP_ON, 1, 1);
	beep->write_signal(SIG_BEEP_MUTE, 1, 1);
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
	beep->init(rate, 2400, 2, 8000);
	opn->init(rate, 3993552, samples, 0, 0);
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

void VM::open_disk(_TCHAR* file_path, int drv)
{
	if(drv == 0 || drv == 1) {
		fdc_2hd->open_disk(file_path, drv);
	}
	else if(drv == 2 || drv == 3) {
		fdc_2dd->open_disk(file_path, drv - 2);
	}
}

void VM::close_disk(int drv)
{
	if(drv == 0 || drv == 1) {
		fdc_2hd->close_disk(drv);
	}
	else if(drv == 2 || drv == 3) {
		fdc_2dd->close_disk(drv - 2);
	}
}

void VM::play_datarec(_TCHAR* filename)
{
	cmt->play_datarec(filename);
}

void VM::rec_datarec(_TCHAR* filename)
{
	cmt->rec_datarec(filename);
}

void VM::close_datarec()
{
	cmt->close_datarec();
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

