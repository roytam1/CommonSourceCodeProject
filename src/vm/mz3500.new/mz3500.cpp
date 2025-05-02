/*
	SHARP MZ-3500 Emulator 'EmuZ-3500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2010.08.31-

	[ virtual machine ]
*/

#include "mz3500.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../i8251.h"
#include "../i8253.h"
#include "../i8255.h"
#include "../io.h"
#include "../upd1990a.h"
#include "../upd7220.h"
#include "../upd765a.h"
#include "../z80.h"

#include "main.h"
#include "sub.h"

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
	
	// for main cpu
	io = new IO(this, emu);
	fdc = new UPD765A(this, emu);
	cpu = new Z80(this, emu);
	main = new MAIN(this, emu);
	
	// for sub cpu
	sio = new I8251(this, emu);
	pit = new I8253(this, emu);
	pio = new I8255(this, emu);
	subio = new IO(this, emu);
	pcm = new PCM1BIT(this, emu);
	rtc = new UPD1990A(this, emu);
	gdc_g = new UPD7220(this, emu);
	gdc_c = new UPD7220(this, emu);
	subcpu = new Z80(this, emu);
	sub = new SUB(this, emu);
	
	fdc->set_context_irq(main, SIG_MAIN_INTFD, 1);
	fdc->set_context_drq(main, SIG_MAIN_DRQ, 1);
	fdc->set_context_index(main, SIG_MAIN_INDEX, 1);
	
	sio->set_context_rxrdy(subcpu, SIG_CPU_NMI, 1);
	pit->set_context_ch1(pcm, SIG_PCM1BIT_SIGNAL, 1);
	pit->set_context_ch2(pit, SIG_I8253_GATE_1, 1);
	pit->set_constant_clock(0, 2450000);
	pit->set_constant_clock(1, 2450000);
	pit->set_constant_clock(2, 2450000);
	
	// pa0-pa7: printer data
	pio->set_context_port_b(rtc, SIG_UPD1990A_STB, 0x01, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_C0,  0x02, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_C1,  0x04, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_C2,  0x08, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_DIN, 0x10, 0);
	pio->set_context_port_b(rtc, SIG_UPD1990A_CLK, 0x20, 0);
	pio->set_context_port_b(main, SIG_MAIN_SRDY, 0x40, 0);
	pio->set_context_port_b(sub, SIG_SUB_PM, 0x80, 0);	// CG Selection
	pio->set_context_port_c(sub, SIG_SUB_KEYBOARD_DC, 0x01, 0);
	pio->set_context_port_c(sub, SIG_SUB_KEYBOARD_STC, 0x02, 0);
	pio->set_context_port_c(sub, SIG_SUB_KEYBOARD_ACKC, 0x04, 0);
	// pc3: intr (not use?)
	pio->set_context_port_c(pcm, SIG_PCM1BIT_MUTE, 0x10, 0);
	// pc5: printer strobe (out)
	// pc6: printer ack (in)
	pio->set_context_port_c(sub, SIG_SUB_PIO_OBF, 0x80, 0);
	rtc->set_context_dout(sub, SIG_SUB_RTC_DOUT, 1);
	gdc_t->set_vram_ptr(sub->get_vram_t(), 0x1000);
	gdc_g->set_vram_ptr(sub->get_vram_g(), 0x20000);
	subcpu->set_context_busack(main, SIG_MAIN_SACK);
	
	sub->set_context_main(main);
	sub->set_ipl(main->get_ipl());
	sub->set_common(main->get_common());
	
	// main i/o bus
	io->set_iomap_range_w(0xec, 0xef, main);	// reset int0
	io->set_iomap_single_w(0xf5, fdc);
	io->set_iomap_single_w(0xf7, fdc);
	io->set_iomap_range_w(0xf8, 0xfb, main);	// mfd interface
	io->set_iomap_range_w(0xfc, 0xff, main);	// memory mpaper
	
	io->set_iomap_range_r(0xec, 0xef, main);	// reset int0
	io->set_iomap_range_r(0xf4, 0xf7, fdc);	
	io->set_iomap_range_r(0xf8, 0xfb, main);	// mfd interface
	io->set_iomap_range_r(0xfc, 0xff, main);	// memory mpaper
	
	// sub i/o bus
	subio->set_iomap_range_w(0x00, 0x0f, sub);	// int0 to main (set flipflop)
	subio->set_iomap_range_w(0x10, 0x1f, sio);
	subio->set_iomap_range_w(0x20, 0x2f, pit);
	subio->set_iomap_range_w(0x30, 0x3f, pio);
	subio->set_iomap_range_w(0x50, 0x5f, sub);	// crt
	subio->set_iomap_range_w(0x60, 0x6f, gdc_g);
	subio->set_iomap_range_w(0x70, 0x7f, gdc_c);
	
	subio->set_iomap_range_r(0x10, 0x1f, sio);
	subio->set_iomap_range_r(0x20, 0x2f, pit);
	subio->set_iomap_range_r(0x30, 0x3f, pio);
	subio->set_iomap_range_r(0x40, 0x4f, sub);
	// 40-4f
	// bit0	timer output
	// bit1	8255 pc7
	// bit2	printer busy
	// bit3	printer pe
	// bit4	printer pdtr
	// bit5	keyboard dk
	// bit6	keyboard stk
	// bit7	halt key
	subio->set_iomap_range_r(0x60, 0x6f, gdc_g);
	subio->set_iomap_range_r(0x70, 0x7f, gdc_c);

