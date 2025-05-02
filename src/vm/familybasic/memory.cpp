/*
	Nintendo Family BASIC Emulator 'eFamilyBASIC'
	Skelton for retropc emulator

	Origin : nester
	Author : Takeda.Toshiya
	Date   : 2010.08.11-

	[ memory ]
*/

#include "memory.h"
#include "../datarec.h"
#include "../../fileio.h"

#define EVENT_DMA_DONE	0

void MEMORY::initialize()
{
	uint8 header[16];
	
	memset(ram, 0, sizeof(ram));
	memset(save_ram, 0, sizeof(save_ram));
	memset(rom, 0xff, sizeof(rom));
	memset(header, 0, sizeof(header));
	
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sBASIC.NES"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		// read header
		fio->Fread(header, sizeof(header), 1);
		// read program rom (max 32kb)
		fio->Fread(rom, 0x4000, 1);
		memcpy(rom + 0x4000, rom, 0x4000);
		fio->Fread(rom + 0x4000, 0x4000, 1);
		fio->Fclose();
	}
	delete fio;
	
	key_stat = emu->key_buffer();
	joy_stat = emu->joy_buffer();
	
	// register event
	register_vline_event(this);
}

void MEMORY::release()
{
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sBACKUP.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(save_ram, sizeof(save_ram), 1);
		fio->Fclose();
	}
	delete fio;
}

void MEMORY::reset()
{
	dma_addr = 0x80;
	frame_irq_enabled = 0xff;
	
	pad_strobe = false;
	pad1_bits = pad2_bits = 0xff;
	
	kb_out = false;
	kb_scan = 0;
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xffff;

	if(addr < 0x2000) {
		ram[addr & 0x7ff] = data;
	}
	else if(addr < 0x4000) {
		d_ppu->write_data8(addr, data);
	}
	else if(addr == 0x4014) {
		// stop cpu
		d_cpu->write_signal(SIG_CPU_BUSREQ, 1, 1);
		int id;
		register_event_by_clock(this, EVENT_DMA_DONE, 514, false, &id);
		// start dma
		dma_addr = data << 8;
		for(int i = 0; i < 256; i++) {
			spr_ram[i] = read_data8(dma_addr | i);
		}
	}
	else if(addr == 0x4016) {
		if(data & 1) {
			pad_strobe = true;
		}
		else if(pad_strobe) {
			pad_strobe = false;
			// joypad #1
			pad1_bits = 0xff;
			if(joy_stat[0] & 0x10) pad1_bits &= ~0x01;	// A
			if(joy_stat[0] & 0x20) pad1_bits &= ~0x02;	// B
			if(joy_stat[0] & 0x40) pad1_bits &= ~0x04;	// SEL
			if(joy_stat[0] & 0x80) pad1_bits &= ~0x08;	// START
			if(joy_stat[0] & 0x02) pad1_bits &= ~0x10;	// DOWN
			if(joy_stat[0] & 0x01) pad1_bits &= ~0x20;	// UP
			if(joy_stat[0] & 0x04) pad1_bits &= ~0x40;	// LEFT
			if(joy_stat[0] & 0x08) pad1_bits &= ~0x80;	// RIGHT
			// joypad #2
			pad2_bits = 0xff;
			if(joy_stat[1] & 0x10) pad2_bits &= ~0x01;	// A
			if(joy_stat[1] & 0x20) pad2_bits &= ~0x02;	// B
			if(joy_stat[1] & 0x02) pad2_bits &= ~0x10;	// DOWN
			if(joy_stat[1] & 0x01) pad2_bits &= ~0x20;	// UP
			if(joy_stat[1] & 0x04) pad2_bits &= ~0x40;	// LEFT
			if(joy_stat[1] & 0x08) pad2_bits &= ~0x80;	// RIGHT
		}
		// keyboard
		if((data & 0x07) == 0x04) {
			if(++kb_scan > 9) {
				kb_scan = 0;
			}
			kb_out = !kb_out;
		}
		else if((data & 0x07) == 0x05) {
			kb_out = false;
			kb_scan = 0;
		}
		else if((data & 0x07) == 0x06) {
			kb_out = !kb_out;
		}
		// data recorder
		d_drec->write_signal(SIG_DATAREC_OUT, data, 2);
	}
	else if(addr < 0x4018) {
		if(addr == 0x4017) {
			frame_irq_enabled = data;
		}
		d_apu->write_data8(addr, data);
	}
	else if(addr < 0x6000) {
		// mapper independent
	}
	else if(addr < 0x8000) {
		save_ram[addr & 0x1fff] = data;
	}
	else {
		// mapper independent
	}
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xffff;

	if(addr < 0x2000) {
		return ram[addr & 0x7ff];
	}
	else if(addr < 0x4000) {
		return d_ppu->read_data8(addr);
	}
	else if(addr == 0x4014) {
		return dma_addr >> 8;
	}
	else if(addr < 0x4016) {
		uint32 val = d_apu->read_data8(addr);
		if(addr == 0x4015 && !(frame_irq_enabled & 0xc0)) {
			val |= 0x40;
		}
		return val;
	}
	else if(addr == 0x4016) {
		// joypad #1
		uint32 val = pad1_bits & 1;
		pad1_bits >>= 1;
		// data recorder
		val |= d_drec->read_signal(0) ? 2 : 0;
		// mic
		val |= key_stat[0x7b] ? 4 : 0;	// F12
		return val;
	}
	else if(addr == 0x4017) {
		// joypad #2
		uint32 val = 0xfe | (pad2_bits & 1);
		pad2_bits >>= 1;
		// keyboard
		if(kb_out) {
			switch(kb_scan) {
			case 1:
				if(key_stat[0x77]) val &= ~0x02;	// F8
				if(key_stat[0x0d]) val &= ~0x04;	// RETURN
				if(key_stat[0xdb]) val &= ~0x08;	// [
				if(key_stat[0xdd]) val &= ~0x10;	// ]
				break;
			case 2:
				if(key_stat[0x76]) val &= ~0x02;	// F7
				if(key_stat[0xc0]) val &= ~0x04;	// @
				if(key_stat[0xba]) val &= ~0x08;	// :
				if(key_stat[0xbb]) val &= ~0x10;	// ;
				break;
			case 3:
				if(key_stat[0x75]) val &= ~0x02;	// F6
				if(key_stat[0x4f]) val &= ~0x04;	// O
				if(key_stat[0x4c]) val &= ~0x08;	// L
				if(key_stat[0x4b]) val &= ~0x10;	// K
				break;
			case 4:
				if(key_stat[0x74]) val &= ~0x02;	// F5
				if(key_stat[0x49]) val &= ~0x04;	// I
				if(key_stat[0x55]) val &= ~0x08;	// U
				if(key_stat[0x4a]) val &= ~0x10;	// J
				break;
			case 5:
				if(key_stat[0x73]) val &= ~0x02;	// F4
				if(key_stat[0x59]) val &= ~0x04;	// Y
				if(key_stat[0x47]) val &= ~0x08;	// G
				if(key_stat[0x48]) val &= ~0x10;	// H
				break;
			case 6:
				if(key_stat[0x72]) val &= ~0x02;	// F3
				if(key_stat[0x54]) val &= ~0x04;	// T
				if(key_stat[0x52]) val &= ~0x08;	// R
				if(key_stat[0x44]) val &= ~0x10;	// D
				break;
			case 7:
				if(key_stat[0x71]) val &= ~0x02;	// F2
				if(key_stat[0x57]) val &= ~0x04;	// W
				if(key_stat[0x53]) val &= ~0x08;	// S
				if(key_stat[0x41]) val &= ~0x10;	// A
				break;
			case 8:
				if(key_stat[0x70]) val &= ~0x02;	// F1
				if(key_stat[0x1b]) val &= ~0x04;	// ESC
				if(key_stat[0x51]) val &= ~0x08;	// Q
				if(key_stat[0x11]) val &= ~0x10;	// CTRL
				break;
			case 9:
				if(key_stat[0x24]) val &= ~0x02;	// CLS
				if(key_stat[0x26]) val &= ~0x04;	// UP
				if(key_stat[0x27]) val &= ~0x08;	// RIGHT
				if(key_stat[0x25]) val &= ~0x10;	// LEFT
				break;
			}
		}
		else {
			switch(kb_scan) {
			case 1:
				if(key_stat[0x15]) val &= ~0x02;	// KANA
//				if(key_stat[0x10]) val &= ~0x04;	// RSHIFT
				if(key_stat[0xdc]) val &= ~0x08;	// '\\'
				if(key_stat[0x23]) val &= ~0x10;	// STOP
				break;
			case 2:
				if(key_stat[0xe2]) val &= ~0x02;	// _
				if(key_stat[0xbf]) val &= ~0x04;	// /
				if(key_stat[0xbd]) val &= ~0x08;	// -
				if(key_stat[0xde]) val &= ~0x10;	// ^
				break;
			case 3:
				if(key_stat[0xbe]) val &= ~0x02;	// .
				if(key_stat[0xbc]) val &= ~0x04;	// ,
				if(key_stat[0x50]) val &= ~0x08;	// P
				if(key_stat[0x30]) val &= ~0x10;	// 0
				break;
			case 4:
				if(key_stat[0x4d]) val &= ~0x02;	// M
				if(key_stat[0x4e]) val &= ~0x04;	// N
				if(key_stat[0x39]) val &= ~0x08;	// 9
				if(key_stat[0x38]) val &= ~0x10;	// 8
				break;
			case 5:
				if(key_stat[0x42]) val &= ~0x02;	// B
				if(key_stat[0x56]) val &= ~0x04;	// V
				if(key_stat[0x37]) val &= ~0x08;	// 7
				if(key_stat[0x36]) val &= ~0x10;	// 6
				break;
			case 6:
				if(key_stat[0x46]) val &= ~0x02;	// F
				if(key_stat[0x43]) val &= ~0x04;	// C
				if(key_stat[0x35]) val &= ~0x08;	// 5
				if(key_stat[0x34]) val &= ~0x10;	// 4
				break;
			case 7:
				if(key_stat[0x58]) val &= ~0x02;	// X
				if(key_stat[0x5a]) val &= ~0x04;	// Z
				if(key_stat[0x45]) val &= ~0x08;	// E
				if(key_stat[0x33]) val &= ~0x10;	// 3
				break;
			case 8:
				if(key_stat[0x10]) val &= ~0x02;	// LSHIFT
				if(key_stat[0x12]) val &= ~0x04;	// GRAPH
				if(key_stat[0x31]) val &= ~0x08;	// 1
				if(key_stat[0x32]) val &= ~0x10;	// 2
				break;
			case 9:
				if(key_stat[0x28]) val &= ~0x02;	// DOWN
				if(key_stat[0x20]) val &= ~0x04;	// SPACE
				if(key_stat[0x2e]) val &= ~0x08;	// DEL
				if(key_stat[0x2d]) val &= ~0x10;	// INS
				break;
			}
		}
		return val;
	}
	else if(addr < 0x6000) {
		// mapper independent
		return 0xff;
	}
	else if(addr < 0x8000) {
		return save_ram[addr & 0x1fff];
	}
	else {
		return rom[addr & 0x7fff];
	}
}

void MEMORY::event_vline(int v, int clock)
{
	// fram irq
	if(v == 240 && !(frame_irq_enabled & 0xc0)) {
		// pending
		d_cpu->write_signal(SIG_CPU_IRQ, 1, 1);
	}
}

void MEMORY::event_callback(int event_id, int err)
{
	// dma done
	d_cpu->write_signal(SIG_CPU_BUSREQ, 0, 1);
}


