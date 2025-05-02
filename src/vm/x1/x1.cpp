/*
	SHARP X1twin Emulator 'eX1twin'
	SHARP X1turbo Emulator 'eX1turbo'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.11-

	[ virtual machine ]
*/

#include "x1.h"
#include "../../emu.h"
#include "../device.h"
#include "../event.h"

#include "../datarec.h"
#include "../disk.h"
#include "../hd46505.h"
#include "../i8255.h"
#include "../mb8877.h"
#include "../ym2151.h"
#include "../ym2203.h"
#include "../z80.h"
#include "../z80ctc.h"
#ifdef _X1TURBO
#include "../z80dma.h"
#include "../z80sio.h"
#endif

#include "display.h"
#include "emm.h"
#include "floppy.h"
#include "io.h"
#include "joystick.h"
#include "memory.h"
#include "sub.h"

#ifdef _X1TWIN
#include "../huc6280.h"
#include "../pcengine/pce.h"
#endif

#include "../../config.h"

// ----------------------------------------------------------------------------
// initialize
// ----------------------------------------------------------------------------

VM::VM(EMU* parent_emu) : emu(parent_emu)
{
	sound_device_type = config.sound_device_type;
	
	// create devices
	first_device = last_device = NULL;
	dummy = new DEVICE(this, emu);	// must be 1st device
	event = new EVENT(this, emu);	// must be 2nd device
	
	drec = new DATAREC(this, emu);
	crtc = new HD46505(this, emu);
	pio = new I8255(this, emu);
	fdc = new MB8877(this, emu);
	if(sound_device_type >= 1) {
		opm1 = new YM2151(this, emu);
	}
	if(sound_device_type == 2) {
		opm2 = new YM2151(this, emu);
	}
	psg = new YM2203(this, emu);
	cpu = new Z80(this, emu);
	if(sound_device_type >= 1) {
		ctc1 = new Z80CTC(this, emu);
	}
	if(sound_device_type == 2) {
		ctc2 = new Z80CTC(this, emu);
	}
#ifdef _X1TURBO
	ctc = new Z80CTC(this, emu);
	dma = new Z80DMA(this, emu);
	sio = new Z80SIO(this, emu);
#endif
	
	display = new DISPLAY(this, emu);
	emm = new EMM(this, emu);
	floppy = new FLOPPY(this, emu);
	io = new IO(this, emu);
	joy = new JOYSTICK(this, emu);
	memory = new MEMORY(this, emu);
	sub = new SUB(this, emu);
	
	// set contexts
	event->set_context_cpu(cpu);
	if(sound_device_type >= 1) {
		event->set_context_sound(opm1);
	}
	if(sound_device_type == 2) {
		event->set_context_sound(opm2);
	}
	event->set_context_sound(psg);
	
	drec->set_context_out(pio, SIG_I8255_PORT_B, 0x02);
	drec->set_context_end(sub, SIG_SUB_TAPE_END, 1);
	crtc->set_context_vblank(display, SIG_DISPLAY_VBLANK, 1);
	crtc->set_context_vblank(pio, SIG_I8255_PORT_B, 0x80);
	crtc->set_context_vsync(pio, SIG_I8255_PORT_B, 0x04);
	pio->set_context_port_c(drec, SIG_DATAREC_OUT, 0x01, 0);
	pio->set_context_port_c(display, SIG_DISPLAY_COLUMN, 0x40, 0);
	pio->set_context_port_c(io, SIG_IO_MODE, 0x20, 0);
#ifdef _X1TURBO
	fdc->set_context_drq(dma, SIG_Z80DMA_READY, 1);
#endif
#ifdef _FDC_DEBUG_LOG
	fdc->set_context_cpu(cpu);
#endif
	if(sound_device_type >= 1) {
		ctc1->set_context_zc0(ctc1, SIG_Z80CTC_TRIG_3, 1);
		ctc1->set_constant_clock(1, CPU_CLOCKS >> 1);
		ctc1->set_constant_clock(2, CPU_CLOCKS >> 1);
	}
	if(sound_device_type == 2) {
		ctc2->set_context_zc0(ctc2, SIG_Z80CTC_TRIG_3, 1);
		ctc2->set_constant_clock(1, CPU_CLOCKS >> 1);
		ctc2->set_constant_clock(2, CPU_CLOCKS >> 1);
	}
#ifdef _X1TURBO
	ctc->set_context_zc0(ctc, SIG_Z80CTC_TRIG_3, 1);
	ctc->set_constant_clock(1, CPU_CLOCKS >> 1);
	ctc->set_constant_clock(2, CPU_CLOCKS >> 1);
	dma->set_context_memory(memory);
	dma->set_context_io(io);
#endif
	
#ifdef _X1TURBO
	display->set_context_cpu(cpu);
	display->set_context_crtc(crtc);
#endif
	display->set_vram_ptr(io->get_vram());
	display->set_regs_ptr(crtc->get_regs());
	floppy->set_context_fdc(fdc);
#ifdef _X1TURBO
	floppy->set_context_dma(dma);
#endif
	joy->set_context_psg(psg);
#ifdef _X1TURBO
	memory->set_context_pio(pio);
#endif
	sub->set_context_pio(pio);
	sub->set_context_datarec(drec);
	
	// cpu bus
	cpu->set_context_mem(memory);
	cpu->set_context_io(io);
#if defined(_X1TURBO) && defined(SINGLE_MODE_DMA)
	cpu->set_context_dma(dma);
#endif
	
	// z80 family daisy chain
	DEVICE* parent_dev = NULL;
	int level = 0;
	
	#define Z80_DAISY_CHAIN(dev) { \
		if(parent_dev == NULL) { \
			cpu->set_context_intr(dev); \
		} \
		else { \
			parent_dev->set_context_child(dev); \
		} \
		dev->set_context_intr(cpu, level++); \
		parent_dev = dev; \
	}
	if(sound_device_type >= 1) {
		Z80_DAISY_CHAIN(ctc1);
	}
	if(sound_device_type == 2) {
		Z80_DAISY_CHAIN(ctc2);
	}
#ifdef _X1TURBO
	Z80_DAISY_CHAIN(sio);
	Z80_DAISY_CHAIN(dma);
	Z80_DAISY_CHAIN(ctc);
#endif
	Z80_DAISY_CHAIN(sub);
	
	// i/o bus
	if(sound_device_type >= 1) {
		io->set_iomap_single_w(0x700, opm1);
		io->set_iovalue_single_r(0x700, 0x00);
		io->set_iomap_single_rw(0x701, opm1);
		io->set_iomap_range_rw(0x704, 0x707, ctc1);
	}
	if(sound_device_type == 2) {
		io->set_iomap_single_w(0x708, opm2);
		io->set_iovalue_single_r(0x708, 0x00);
		io->set_iomap_single_rw(0x709, opm2);
		io->set_iomap_range_rw(0x70c, 0x70f, ctc2);
	}
#ifdef _X1TURBO
	io->set_iomap_single_rw(0xb00, memory);
#endif
	io->set_iomap_range_rw(0xd00, 0xd03, emm);
	io->set_iomap_range_r(0xe80, 0xe81, display);
	io->set_iomap_range_w(0xe80, 0xe82, display);
	io->set_iomap_range_rw(0xff8, 0xffb, fdc);
	io->set_iomap_single_w(0xffc, floppy);
#ifdef _X1TURBO
	io->set_iomap_range_r(0xffc, 0xfff, floppy);
#endif
	io->set_iomap_range_rw(0x1000, 0x17ff, display);
	for(int i = 0x1800; i <= 0x18ff; i += 0x10) {
		io->set_iomap_range_rw(i, i + 1, crtc);
	}
	io->set_iomap_range_rw(0x1900, 0x19ff, sub);
	for(int i = 0x1a00; i <= 0x1aff; i += 4) {
		io->set_iomap_range_rw(i, i + 3, pio);
	}
	for(int i = 0x1b00; i <= 0x1bff; i++) {
		io->set_iomap_alias_rw(i, psg, 1);
	}
	for(int i = 0x1c00; i <= 0x1cff; i++) {
		io->set_iomap_alias_w(i, psg, 0);
	}
	io->set_iomap_range_w(0x1d00, 0x1eff, memory);
#ifdef _X1TURBO
	io->set_iomap_range_rw(0x1f80, 0x1f8f, dma);
	io->set_iomap_range_rw(0x1f90, 0x1f93, sio);
	io->set_iomap_range_rw(0x1fa0, 0x1fa3, ctc);
#ifdef _X1TURBOZ
	io->set_iomap_single_rw(0x1fd0, display);
	io->set_iomap_single_rw(0x1fe0, display);
#else
	io->set_iomap_single_w(0x1fd0, display);
	io->set_iomap_single_w(0x1fe0, display);
#endif
	// 0x1ff0: dipswitch
//	io->set_iovalue_single_r(0x1ff0, 0x00);
	update_dipswitch();
#endif
	io->set_iomap_range_rw(0x2000, 0x3fff, display);	// tvram
	
#ifdef _X1TWIN
	// init PC Engine
	pceevent = new EVENT(this, emu);
	pceevent->set_frames_per_sec(PCE_FRAMES_PER_SEC);
	pceevent->set_lines_per_frame(PCE_LINES_PER_FRAME);
	
	pcecpu = new HUC6280(this, emu);
	pcecpu->set_context_event_manager(pceevent);
	pce = new PCE(this, emu);
	pce->set_context_event_manager(pceevent);
	
	pceevent->set_context_cpu(pcecpu, PCE_CPU_CLOCKS);
	pceevent->set_context_sound(pce);
	
	pcecpu->set_context_mem(pce);
	pcecpu->set_context_io(pce);
	pce->set_context_cpu(pcecpu);
#endif
	
	// initialize all devices
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->initialize();
	}
	// NOTE: motor seems to be on automatically when fdc command is requested,
	// so motor is always on temporary
	fdc->write_signal(SIG_MB8877_MOTOR, 1, 1);
	for(int i = 0; i < 4; i++) {
#ifdef _X1TURBO
		fdc->set_drive_type(i, DRIVE_TYPE_2DD);
#else
		fdc->set_drive_type(i, DRIVE_TYPE_2D);
#endif
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
	psg->SetReg(0x2e, 0);	// set prescaler
}

void VM::special_reset()
{
	// nmi reset
	cpu->write_signal(SIG_CPU_NMI, 1, 1);
}

void VM::run()
{
	event->drive();
#ifdef _X1TWIN
	if(pce->running) {
		pceevent->drive();
	}
#endif
}

double VM::frame_rate()
{
#ifdef _X1TWIN
	if(pce->running) {
		return pceevent->frame_rate();
	}
#endif
	return event->frame_rate();
}

// ----------------------------------------------------------------------------
// draw screen
// ----------------------------------------------------------------------------

void VM::draw_screen()
{
	display->draw_screen();
#ifdef _X1TWIN
	if(pce->running) {
		pce->draw_screen();
	}
#endif
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
#ifdef _X1TWIN
	pceevent->initialize_sound(rate, samples);
#endif
	
	// init sound gen
	if(sound_device_type >= 1) {
		opm1->init(rate, 4000000, samples, 0);
	}
	if(sound_device_type == 2) {
		opm2->init(rate, 4000000, samples, 0);
	}
	psg->init(rate, 2000000, samples, 0, 0);
#ifdef _X1TWIN
	pce->initialize_sound(rate);
#endif
}

uint16* VM::create_sound(int* extra_frames)
{
#ifdef _X1TWIN
	if(pce->running) {
		uint16* buffer = pceevent->create_sound(extra_frames);
		for(int i = 0; i < *extra_frames; i++) {
			event->drive();
		}
		return buffer;
	}
#endif
	return event->create_sound(extra_frames);
}

// ----------------------------------------------------------------------------
// notify key
// ----------------------------------------------------------------------------

void VM::key_down(int code, bool repeat)
{
#ifdef _X1TWIN
	if(!pce->running && !repeat) {
		sub->key_down(code, false);
	}
#else
	if(!repeat) {
		sub->key_down(code, false);
	}
#endif
}

void VM::key_up(int code)
{
#ifdef _X1TWIN
	if(!pce->running) {
		sub->key_up(code);
	}
#else
	sub->key_up(code);
#endif
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
	bool value = drec->play_datarec(file_path);
	
	sub->close_datarec();
	sub->play_datarec(value);
}

void VM::rec_datarec(_TCHAR* file_path)
{
	bool value = drec->rec_datarec(file_path);
	
	sub->close_datarec();
	sub->rec_datarec(value);
}

void VM::close_datarec()
{
	drec->close_datarec();
	
	sub->close_datarec();
}

void VM::push_play()
{
	sub->push_play();
}

void VM::push_stop()
{
	sub->push_stop();
}

bool VM::now_skip()
{
	return drec->skip();
}

#ifdef _X1TWIN
void VM::open_cart(_TCHAR* file_path)
{
	pce->open_cart(file_path);
	pce->reset();
	pcecpu->reset();
}

void VM::close_cart()
{
	pce->close_cart();
	pce->reset();
	pcecpu->reset();
}
#endif

void VM::update_config()
{
	for(DEVICE* device = first_device; device; device = device->next_device) {
		device->update_config();
	}
#ifdef _X1TURBO
	update_dipswitch();
#endif
}

#ifdef _X1TURBO
void VM::update_dipswitch()
{
	// bit0		0=High 1=Standard
	// bit1-3	0=5"2D 4=5"2HD
	io->set_iovalue_single_r(0x1ff0, config.monitor_type & 1);
}
#endif

#ifdef _X1TWIN
bool VM::pce_running()
{
	return pce->running;
}
#endif

