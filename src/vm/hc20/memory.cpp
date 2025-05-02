/*
	EPSON HC-20 Emulator 'eHC-20'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2011.05.23-

	[ memory ]
*/

#include "memory.h"
#include "../mc6800.h"
#include "../tf20.h"
#include "../../config.h"
#include "../../fifo.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 13, eb = (e) >> 13; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) { \
			wbank[i] = wdmy; \
		} \
		else { \
			wbank[i] = (w) + 0x2000 * (i - sb); \
		} \
		if((r) == rdmy) { \
			rbank[i] = rdmy; \
		} \
		else { \
			rbank[i] = (r) + 0x2000 * (i - sb); \
		} \
	} \
}

#define INT_KEYBOARD	1
#define INT_CLOCK	2
#define INT_POWER	4

static int key_table[8][10] = {
	// PAUSE=F6, MENU=F7, BREAK=F8 NUM=F9 CLR=F10 SCRN=F11 PRINT=PgUp PAPER=PgDn
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x70, 0x00,
	0x38, 0x39, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0x71, 0x00,
	0xc0, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x72, 0x00,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x73, 0x00,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x74, 0x00,
	0x58, 0x59, 0x5a, 0xdb, 0xdd, 0xdc, 0x27, 0x25, 0x22, 0x10,
	0x0d, 0x20, 0x09, 0x00, 0x00, 0x78, 0x00, 0x34, 0x00, 0x11,
	0x79, 0x7a, 0x77, 0x75, 0x2e, 0x76, 0x00, 0x00, 0x00, 0x21
};

void MEMORY::initialize()
{
	// initialize memory
	_memset(ram, 0, sizeof(ram));
	_memset(rom, 0, sizeof(rom));
	_memset(ext, 0, sizeof(ext));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	// load backuped ram / rom images
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sBACKUP.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sBASIC.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(rom, sizeof(rom), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sEXT.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ext, sizeof(ext), 1);
		fio->Fclose();
	}
	delete fio;
	
//	SET_BANK(0x0000, 0x3fff, ram, ram);
//	SET_BANK(0x4000, 0x7fff, wdmy, rdmy);
	SET_BANK(0x0000, 0x7fff, ram, ram);
	SET_BANK(0x8000, 0xffff, wdmy, rom);
	
	// init command buffer
	cmd_buf = new FIFO(16);
	
	// init keyboard
	key_stat = emu->key_buffer();
	
	// init lcd
	pd = RGB_COLOR(48, 56, 16);
	pb = RGB_COLOR(160, 168, 160);
	memset(lcd, 0, sizeof(lcd));
	
	// register event
	vm->register_frame_event(this);
}

void MEMORY::release()
{
	// save battery backuped ram
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sBACKUP.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	
	// release command buffer
	cmd_buf->release();
}

void MEMORY::reset()
{
	// select internal rom
//	SET_BANK(0x4000, 0x7fff, wdmy, rdmy);
	SET_BANK(0x8000, 0xbfff, wdmy, rom);
	
	key_strobe = 0xff;
	key_data = 0x3ff;
	key_intmask = 0;
	
	lcd_select = 0;
	lcd_clock = 0;
	
	int_status = 0;
	int_mask = 0;
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xffff;
	if(addr < 0x40) {
		switch(addr) {
		case 0x20:
			key_strobe = data;
			update_keyboard();
			break;
		case 0x26:
			lcd_select = data & 0x0f;
			key_intmask = data & 0x10;
			// interrupt mask reset in sleep mode
			if(int_mask) {
				int_mask = 0;
//				update_intr();
			}
			break;
		case 0x2a:
			lcd_data = data;
			lcd_clock = 8;
			break;
		case 0x2c:
			// used for interrupt mask setting in sleep mode
			if(!int_mask) {
				int_mask = 1;
//				update_intr();
			}
			break;
		case 0x30:
//			SET_BANK(0x4000, 0x7fff, ram + 0x4000, ram + 0x4000);
			SET_BANK(0x8000, 0xbfff, wdmy, ext);
			break;
		case 0x32:
		case 0x33:
//			SET_BANK(0x4000, 0x7fff, wdmy, rdmy);
			SET_BANK(0x8000, 0xbfff, wdmy, rom);
			break;
		}
		ram[addr] = data;
	}
	else if(addr < 0x80) {
		d_rtc->write_io8(1, addr & 0x3f);
		d_rtc->write_io8(0, data);
	}
	else {
		wbank[(addr >> 13) & 7][addr & 0x1fff] = data;
	}
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xffff;
	if(addr < 0x40) {
		switch(addr) {
		case 0x20:
			return key_strobe;
		case 0x22:
			return key_data & 0xff;
		case 0x26:
			// interrupt mask reset in sleep mode
			if(int_mask) {
				int_mask = 0;
//				update_intr();
			}
			break;
		case 0x28:
			// bit6: power switch interrupt flag (0=active)
			// bit7: busy signal of lcd controller (0=busy)
			return ((key_data >> 8) & 3) | ((int_status & INT_POWER) ? 0 : 0x40) | 0xa8;
		case 0x2a:
		case 0x2b:
			if(lcd_clock > 0 && --lcd_clock <= 0) {
				int c = lcd_select & 7;
				if(c >= 1 && c <= 6) {
					lcd_t *block = &lcd[c - 1];
					if(lcd_select & 8) {
						block->bank = lcd_data & 0x40 ? 40 : 0;
						block->addr = lcd_data & 0x3f;
					}
					else if(block->addr < 40) {
						block->buffer[block->bank + block->addr] = lcd_data;
						block->addr++;
					}
				}
			}
			break;
		case 0x2c:
			// used for interrupt mask setting in sleep mode
			if(!int_mask) {
				int_mask = 1;
//				update_intr();
			}
			break;
		case 0x30:
//			SET_BANK(0x4000, 0x7fff, ram + 0x4000, ram + 0x4000);
			SET_BANK(0x8000, 0xbfff, ext, rom);
			break;
		case 0x32:
		case 0x33:
//			SET_BANK(0x4000, 0x7fff, wdmy, rdmy);
			SET_BANK(0x8000, 0xbfff, wdmy, rom);
			break;
		}
//		return ram[addr];
		return addr;
	}
	else if(addr < 0x80) {
		d_rtc->write_io8(1, addr & 0x3f);
		return d_rtc->read_io8(0);
	}
	return rbank[(addr >> 13) & 7][addr & 0x1fff];
}

void MEMORY::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_MEMORY_PORT_2) {
		sio_select = data & 0x04;
	}
	else if(id == SIG_MEMORY_SIO) {
		if(!sio_select) {
			d_tf20->write_signal(SIGNAL_TF20_SIO, data, 0xff);
		}
		else {
			send_to_subcpu(data & mask);
		}
	}
	else if(id == SIG_MEMORY_RTC_IRQ) {
		bool cur_int = ((int_status & INT_CLOCK) != 0);
		bool next_int = ((data & mask) != 0);
		
		if(cur_int != next_int) {
			if(next_int) {
				int_status |= INT_CLOCK;
			}
			else {
				int_status &= ~INT_CLOCK;
			}
			update_intr();
		}
	}
}

void MEMORY::event_frame()
{
	update_keyboard();
}

void MEMORY::update_keyboard()
{
	key_data = 0x3ff;
	
	for(int i = 0; i < 8; i++) {
		if(key_strobe & (1 << i)) {
			continue;
		}
		for(int j = 0; j < 10; j++) {
			if(key_stat[key_table[i][j]]) {
				key_data &= ~(1 << j);
			}
		}
		// dip-switch
		if(i < 4 && !(config.dipswitch & (1 << i))) {
			key_data &= ~0x200;
		}
	}
	
	// update interrupt
	bool cur_int = ((int_status & INT_KEYBOARD) != 0);
	bool next_int = ((key_data & 0x1ff) != 0x1ff && key_intmask);
	
	if(cur_int != next_int) {
		if(next_int) {
			int_status |= INT_KEYBOARD;
		}
		else {
			int_status &= ~INT_KEYBOARD;
		}
		d_cpu->write_signal(SIG_MC6801_PORT_1, next_int ? 0 : 0x20, 0x20);
		update_intr();
	}
}

void MEMORY::notify_power_off()
{
	int_status |= INT_POWER;
	update_intr();
}

void MEMORY::update_intr()
{
//	d_cpu->write_signal(SIG_CPU_IRQ, (int_status && !int_mask) ? 1 : 0, 1);
	d_cpu->write_signal(SIG_CPU_IRQ, int_status ? 1 : 0, 1);
}

void MEMORY::send_to_subcpu(uint8 val)
{
	cmd_buf->write(val);
	uint8 cmd = cmd_buf->read_not_remove(0);
	
//	emu->out_debug("Command = %2x", cmd);
//	for(int i = 1; i < cmd_buf->count(); i++) {
//		emu->out_debug(" %2x", cmd_buf->read_not_remove(i));
//	}
//	emu->out_debug("\n");
	
	switch(cmd) {
	case 0x00: // slave mcpu ready check
	case 0x01: // sets the constants required by slave mcu
	case 0x02: // initialization
	case 0x04: // closes masks for special commands
	case 0x09: // bar-code reader power on
	case 0x0a: // bar-code reader power off
	case 0x22: // turns the external cassette rem terminal on
	case 0x23: // turns the external cassette rem terminal off
	case 0x40: // turns the serial driver on
	case 0x41: // turns the serial driver off
	case 0x51: // turns power of plug-in rom cartridge on
	case 0x52: // turns power of plug-in rom cartridge off
		cmd_buf->read();
		send_to_maincpu(0x01);
		break;
	case 0x0c: // terminate process
		cmd_buf->read();
		send_to_maincpu(0x02);
		break;
	case 0x0d:
		if(cmd_buf->count() == 2) {
			cmd_buf->read();
			if(cmd_buf->read() == 0xaa) {
				emu->power_off();
			}
		}
		send_to_maincpu(0x01);
		break;
	case 0x31:
		if(cmd_buf->count() == 1) {
			send_to_maincpu(0x01);
		}
		else {
			send_to_maincpu(0x31);
		}
		if(cmd_buf->count() == 5) {
			cmd_buf->clear();
		}
		break;
	case 0x50: // identifies the plug-in option
		// bit0 = P46	
		// bit1 = P20	RXD (RS-232C)
		cmd_buf->read();
		send_to_maincpu(0x01);
		break;
	default:
		// unknown command
		emu->out_debug("Unknown Command = %2x\n", cmd);
		send_to_maincpu(0x0f);
		break;
	}
}

void MEMORY::send_to_maincpu(uint8 val)
{
	// send to main cpu
	d_cpu->write_signal(SIG_MC6801_SIO_RECV, val, 0xff);
}

void MEMORY::draw_screen()
{
	static int xtop[12] = {0, 0, 40, 40, 80, 80, 0, 0, 40, 40, 80, 80};
	static int ytop[12] = {0, 8, 0, 8, 0, 8, 16, 24, 16, 24, 16, 24};
	
	for(int c = 0; c < 12; c++) {
		int x = xtop[c];
		int y = ytop[c];
		int ofs = (c & 1) ? 40 : 0;
		
		for(int i = 0; i < 40; i++) {
			uint8 pat = lcd[c >> 1].buffer[ofs + i];
			lcd_render[y + 0][x + i] = (pat & 0x01) ? pd : pb;
			lcd_render[y + 1][x + i] = (pat & 0x02) ? pd : pb;
			lcd_render[y + 2][x + i] = (pat & 0x04) ? pd : pb;
			lcd_render[y + 3][x + i] = (pat & 0x08) ? pd : pb;
			lcd_render[y + 4][x + i] = (pat & 0x10) ? pd : pb;
			lcd_render[y + 5][x + i] = (pat & 0x20) ? pd : pb;
			lcd_render[y + 6][x + i] = (pat & 0x40) ? pd : pb;
			lcd_render[y + 7][x + i] = (pat & 0x80) ? pd : pb;
		}
	}
	for(int y = 0; y < 32; y++) {
		scrntype* dest = emu->screen_buffer(y);
		memcpy(dest, lcd_render[y], sizeof(scrntype) * 120);
	}
	
	// access lamp
	uint32 stat_f = d_tf20->read_signal(0);
	if(stat_f) {
		scrntype col = (stat_f & (1 | 4)) ? RGB_COLOR(255, 0, 0) :
		               (stat_f & (2 | 8)) ? RGB_COLOR(0, 255, 0) : 0;
		for(int y = 32 - 8; y < 32; y++) {
			scrntype *dest = emu->screen_buffer(y);
			for(int x = 120 - 8; x < 120; x++) {
				dest[x] = col;
			}
		}
	}
}

