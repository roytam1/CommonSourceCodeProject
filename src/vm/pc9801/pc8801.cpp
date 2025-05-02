/*
	NEC PC-98DO Emulator 'ePC-98DO'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2011.12.29-

	[ PC-8801 ]
*/

#include "pc8801.h"
#include "../beep.h"
#include "../event.h"
#include "../pcm1bit.h"
#include "../upd1990a.h"
#include "../../config.h"

#define EVENT_TIMER	0

#define IRQ_SERIAL	0
#define IRQ_VRTC	1
#define IRQ_TIMER	2
#define IRQ_SOUND	4

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 10, eb = (e) >> 10; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) { \
			wbank[i] = wdmy; \
		} \
		else { \
			wbank[i] = (w) + 0x400 * (i - sb); \
		} \
		if((r) == rdmy) { \
			rbank[i] = rdmy; \
		} \
		else { \
			rbank[i] = (r) + 0x400 * (i - sb); \
		} \
	} \
}

#define SET_BANK_W(s, e, w) { \
	int sb = (s) >> 10, eb = (e) >> 10; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) { \
			wbank[i] = wdmy; \
		} \
		else { \
			wbank[i] = (w) + 0x400 * (i - sb); \
		} \
	} \
}

#define SET_BANK_R(s, e, r) { \
	int sb = (s) >> 10, eb = (e) >> 10; \
	for(int i = sb; i <= eb; i++) { \
		if((r) == rdmy) { \
			rbank[i] = rdmy; \
		} \
		else { \
			rbank[i] = (r) + 0x400 * (i - sb); \
		} \
	} \
}

static const int key_table[15][8] = {
	{ 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67 },
	{ 0x68, 0x69, 0x6a, 0x6b, 0x00, 0x6e, 0x00, 0x0d },
	{ 0xc0, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47 },
	{ 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f },
	{ 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57 },
	{ 0x58, 0x59, 0x5a, 0xdb, 0xdc, 0xdd, 0xde, 0xbd },
	{ 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37 },
	{ 0x38, 0x39, 0xba, 0xbb, 0xbc, 0xbe, 0xbf, 0xe2 },
	{ 0x24, 0x26, 0x27, 0x00, 0x12, 0x15, 0x10, 0x11 },
	{ 0x13, 0x70, 0x71, 0x72, 0x73, 0x74, 0x20, 0x1b },
	{ 0x09, 0x28, 0x25, 0x23, 0x7b, 0x6d, 0x6f, 0x14 },
	{ 0x21, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x75, 0x76, 0x77, 0x78, 0x79, 0x08, 0x2d, 0x2e },
	{ 0x1c, 0x1d, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00 },
	{ 0x0d, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

void PC8801::initialize()
{
	memset(rdmy, 0xff, sizeof(rdmy));
	memset(ram, 0, sizeof(ram));
	memset(eram, 0, sizeof(eram));
	memset(gvram, 0, sizeof(gvram));
	memset(tvram, 0, sizeof(tvram));
	memset(n88rom, 0xff, sizeof(n88rom));
	memset(n88erom, 0xff, sizeof(n88erom));
	memset(n80rom, 0xff, sizeof(n80rom));
	memset(kanji1, 0xff, sizeof(kanji1));
	memset(kanji2, 0xff, sizeof(kanji2));
	
	// load font data
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sN88.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(n88rom, 0x8000, 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sN88_0.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(n88erom + 0x0000, 0x2000, 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sN88_1.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(n88erom + 0x2000, 0x2000, 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sN88_2.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(n88erom + 0x4000, 0x2000, 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sN88_3.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(n88erom + 0x6000, 0x2000, 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sN80.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(n80rom, 0x8000, 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sKANJI1.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(kanji1, 0x20000, 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sKANJI2.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(kanji2, 0x20000, 1);
		fio->Fclose();
	}
	delete fio;
	
	// create semi graphics pattern
	for(int i = 0; i < 256; i++) {
		uint8 *dest = sg_pattern + 8 * i;
		dest[0] = dest[1] = ((i & 1) ? 0xf0 : 0) | ((i & 0x10) ? 0x0f : 0);
		dest[2] = dest[3] = ((i & 2) ? 0xf0 : 0) | ((i & 0x20) ? 0x0f : 0);
		dest[4] = dest[5] = ((i & 4) ? 0xf0 : 0) | ((i & 0x40) ? 0x0f : 0);
		dest[6] = dest[7] = ((i & 8) ? 0xf0 : 0) | ((i & 0x80) ? 0x0f : 0);
	}
	
	for(int i = 0; i < 8; i++) {
		palette_text_pc[i] = RGB_COLOR((i & 2) ? 255 : 0, (i & 4) ? 255 : 0, (i & 1) ? 255 : 0);
	}
	palette_text_pc[8] = 0;
	
	key_status = emu->key_buffer();
	line200 = 1;
	
	register_frame_event(this);
	register_vline_event(this);
	register_event(this, EVENT_TIMER, 1000000 / 600, true, NULL);
}

void PC8801::reset()
{
	// memory
	rm_mode = 0; // N-BASIC ... 4
	erom_sel = eram_sel = eram_bank = 0;
	erom_bank = 0xff;
	tw_ofs = 0x80; // ???
	gvram_sel = 0;
	tvram_sel = 0x10;
	
	SET_BANK(0x0000, 0x7fff, ram, n88rom);
	SET_BANK(0x8000, 0xffff, ram + 0x8000, ram + 0x8000);
	
	port32 = 0;	// ???
	alu_ctrl1 = alu_ctrl2 = 0;
	
	// crtc
	memset(crtc_reg, 0, sizeof(crtc_reg));
	crtc_cmd = crtc_ptr = 0;
	crtc_status = 0;
	text_mode = graph_mode = 0;
	disp_ctrl = 0;
	
	if(!line200) {
		((EVENT*)event_manager)->set_frames_per_sec(60);
		((EVENT*)event_manager)->set_lines_per_frame(260);
		line200 = 1;
	}
	cursor_on = blink_on = false;
	blink_counter = 0;
	
//	for(int i = 0; i < 8; i++) {
//		digipal[i] = i;
//	}
	memset(anapal, 0, sizeof(anapal));
	memset(digipal, 0, sizeof(digipal));
	update_palette = true;
	
	// dma
	memset(dma_reg, 0, sizeof(dma_reg));
	dma_mode = dma_status = 0;
	dma_hl = false;
	
	// kanji rom
	kanji1_addr.d = kanji2_addr.d = 0;
	
	// interrupt
	intr_req = intr_mask1 = intr_mask2 = 0;
	
	// fdd i/f
	d_pio->write_io8(1, 0);
	d_pio->write_io8(2, 0);
}

void PC8801::write_data8(uint32 addr, uint32 data)
{
	if((addr & 0xc000) == 0xc000) {
		if((port32 & 0x40) && (alu_ctrl2 & 0x80)) {
			// alu
			addr &= 0x3fff;
			switch(alu_ctrl2 & 0x30) {
			case 0x00:
				for(int i = 0; i < 3; i++) {
					switch((alu_ctrl1 >> i) & 0x11) {
					case 0x00:	// reset
						gvram[addr | (0x4000 * i)] &= ~data;
						break;
					case 0x01:	// set
						gvram[addr | (0x4000 * i)] |= data;
						break;
					case 0x10:	// reverse
						gvram[addr | (0x4000 * i)] ^= data;
						break;
					}
				}
				break;
			case 0x10:
				gvram[addr | 0x0000] = alu_reg[0];
				gvram[addr | 0x4000] = alu_reg[1];
				gvram[addr | 0x8000] = alu_reg[2];
				break;
			case 0x20:
				gvram[addr | 0x0000] = alu_reg[1];
				break;
			case 0x30:
				gvram[addr | 0x4000] = alu_reg[0];
				break;
			}
			return;
		}
		switch(gvram_sel) {
		case 1: gvram[(addr & 0x3fff) | 0x0000] = data; return;
		case 2: gvram[(addr & 0x3fff) | 0x4000] = data; return;
		case 4: gvram[(addr & 0x3fff) | 0x8000] = data; return;
		}
	}
	addr &= 0xffff;
	wbank[addr >> 10][addr & 0x3ff] = data;
}

uint32 PC8801::read_data8(uint32 addr)
{
	if((addr & 0xc000) == 0xc000) {
		if((port32 & 0x40) && (alu_ctrl2 & 0x80)) {
			// alu
			addr &= 0x3fff;
			alu_reg[0] = gvram[addr | 0x0000];
			alu_reg[1] = gvram[addr | 0x4000];
			alu_reg[2] = gvram[addr | 0x8000];
			uint8 b = alu_reg[0]; if(!(alu_ctrl2 & 1)) b ^= 0xff;
			uint8 r = alu_reg[1]; if(!(alu_ctrl2 & 2)) r ^= 0xff;
			uint8 g = alu_reg[2]; if(!(alu_ctrl2 & 4)) g ^= 0xff;
			return b & r & g;
		}
		switch(gvram_sel) {
		case 1: return gvram[(addr & 0x3fff) | 0x0000];
		case 2: return gvram[(addr & 0x3fff) | 0x4000];
		case 4: return gvram[(addr & 0x3fff) | 0x8000];
		}
	}
	addr &= 0xffff;
	return rbank[addr >> 10][addr & 0x3ff];
}

void PC8801::write_dma_io8(uint32 addr, uint32 data)
{
	// to crtc
	crtc_buffer[(crtc_buffer_ptr++) & 0x3fff] = data;
}

uint32 PC8801::read_dma_data8(uint32 addr)
{
	addr &= 0xffff;
	if((addr & 0xf000) == 0xf000 && (config.boot_mode == MODE_PC88_V1H || config.boot_mode == MODE_PC88_V2)) {
		return tvram[addr & 0xfff];
	}
	return ram[addr];
}

void PC8801::write_io8(uint32 addr, uint32 data)
{
	addr &= 0xff;
	switch(addr) {
	case 0x10:
		d_rtc->write_signal(SIG_UPD1990A_C0, data, 1);
		d_rtc->write_signal(SIG_UPD1990A_C1, data, 2);
		d_rtc->write_signal(SIG_UPD1990A_C2, data, 4);
		d_rtc->write_signal(SIG_UPD1990A_DIN, data, 8);
		break;
	case 0x20:
	case 0x21:
		d_sio->write_io8(addr, data);
		break;
	case 0x30:
		text_mode = data;
		break;
	case 0x31:
		if(line200 != (data & 1)) {
			if(data & 1) {
				((EVENT*)event_manager)->set_frames_per_sec(60);
				((EVENT*)event_manager)->set_lines_per_frame(260);
			}
			else {
				((EVENT*)event_manager)->set_frames_per_sec(55.4);
				((EVENT*)event_manager)->set_lines_per_frame(448);
			}
			line200 = data & 1;
		}
		if(rm_mode != (data & 6)) {
			bool tw_update = (rm_mode == 0 || (data & 6) == 0);
			rm_mode = data & 6;
			update_low_memmap();
			if(tw_update) {
				update_tw_memmap();
			}
		}
		if((graph_mode & 0x10) != (data & 0x10)) {
			update_palette = true;
		}
		graph_mode = data;
		break;
	case 0x32:
		if(erom_sel != (data & 3)) {
			erom_sel = data & 3;
			if(erom_bank == 0xfe) {
				update_low_memmap();
			}
		}
		if(tvram_sel != (data & 0x10)) {
			tvram_sel = data & 0x10;
			if(config.boot_mode == MODE_PC88_V1H || config.boot_mode == MODE_PC88_V2) {
				update_tvram_memmap();
			}
		}
		if((port32 & 0x20) != (data & 0x20)) {
			update_palette = true;
		}
		port32 = data;
		break;
	case 0x34:
		alu_ctrl1 = data;
		break;
	case 0x35:
		alu_ctrl2 = data;
		break;
	case 0x40:
		// bit0: printer strobe
		d_rtc->write_signal(SIG_UPD1990A_STB, data, 2);
		d_rtc->write_signal(SIG_UPD1990A_CLK, data, 4);
		// bit3: crtc i/f sync mode
		// bit4: graph high speed mode
		d_beep->write_signal(SIG_BEEP_ON, data, 0x20);
		// bit6: joystick port pin#8
		d_pcm->write_signal(SIG_PCM1BIT_SIGNAL, data, 0x80);
		break;
	case 0x44:
	case 0x45:
		d_opn->write_io8(addr, data);
		break;
	// crtc (from MESS PC-8801 driver)
	case 0x50:
		if(crtc_ptr < 5) {
			crtc_reg[crtc_cmd][crtc_ptr++] = data;
		}
		break;
	case 0x51:
		crtc_cmd = (data >> 5) & 7;
		crtc_ptr = 0;
		switch(crtc_cmd) {
		case 0:	// reset CRTC
			crtc_status = 0;
			break;
		case 1:	// start display
			crtc_status |= 0x10;
			crtc_status &= ~8;
			break;
		case 2:	// set irq mask
			if(!(data & 1)) {
				crtc_status = 0;
			}
			break;
		case 3:	// read light pen
			crtc_status &= ~1;
			break;
		case 4:	// load cursor position ON/OFF
			cursor_on = ((data & 1) != 0);
			break;
		case 5:	// reset IRQ
			break;
		case 6:	// reset counters
			crtc_status = 0;
			break;
		}
		break;
	case 0x52:
		digipal[8] = (data >> 4) & 7;
		update_palette = true;
		break;
	case 0x53:
		disp_ctrl = data;
		break;
	case 0x54:
	case 0x55:
	case 0x56:
	case 0x57:
	case 0x58:
	case 0x59:
	case 0x5a:
	case 0x5b:
		if(data & 0x80) {
			anapal[8][(data >> 6) & 1] = data;
		}
		else {
			anapal[addr - 0x54][(data >> 6) & 1] = data;
		}
		digipal[addr - 0x54] = data;
		update_palette = true;
		break;
	case 0x5c:
		gvram_sel = 1;
		break;
	case 0x5d:
		gvram_sel = 2;
		break;
	case 0x5e:
		gvram_sel = 4;
		break;
	case 0x5f:
		gvram_sel = 0;
		break;
	case 0x60:
	case 0x62:
	case 0x64:
	case 0x66:
		if(!dma_hl) {
			dma_reg[(addr >> 1) & 3].start.b.l = data;
		}
		else {
			dma_reg[(addr >> 1) & 3].start.b.h = data;
		}
		dma_hl = !dma_hl;
		break;
	case 0x61:
	case 0x63:
	case 0x65:
	case 0x67:
		if(!dma_hl) {
			dma_reg[(addr >> 1) & 3].length.b.l = data;
		}
		else {
			dma_reg[(addr >> 1) & 3].length.b.h = data & 0x3f;
		}
		dma_hl = !dma_hl;
		break;
	case 0x68:
		dma_mode = data;
		dma_hl = false;
		break;
	case 0x70:
		if(tw_ofs != data) {
			tw_ofs = data;
			update_tw_memmap();
		}
		break;
	case 0x71:
		if(erom_bank != data) {
			erom_bank = data;
			update_low_memmap();
		}
		break;
	case 0x78:
		tw_ofs++;
		update_tw_memmap();
		break;
	case 0xe2:
		if(eram_sel != (data & 0x11)) {
			eram_sel = data & 0x11;
			update_low_memmap();
		}
		break;
	case 0xe3:
		if(eram_bank != (data & 0x0f)) {
			eram_bank = data & 0x0f;
			update_low_memmap();
		}
		break;
	case 0xe4:
		intr_mask1 = (data < 8) ? ~(0xff << data) : 0xff;
		update_intr();
		break;
	case 0xe6:
		intr_mask2 = 0xf8 | ((data & 1) << 2) | (data & 2) | ((data & 4) >> 2);
		update_intr();
		break;
	case 0xe8:
		kanji1_addr.b.l = data;
		break;
	case 0xe9:
		kanji1_addr.b.h = data;
		break;
	case 0xec:
		kanji2_addr.b.l = data;
		break;
	case 0xed:
		kanji2_addr.b.h = data;
		break;
	case 0xfc:
		d_pio->write_io8(0, data);
		break;
	case 0xfd:
	case 0xfe:
	case 0xff:
		d_pio->write_io8(addr, data);
		break;
	}
}

uint32 PC8801::read_io8(uint32 addr)
{
	uint32 val = 0xff;
	
	addr &= 0xff;
	switch(addr) {
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0c:
	case 0x0d:
	case 0x0e:
		for(int i = 0; i < 8; i++) {
			if(key_status[key_table[addr & 0x0f][i]]) {
				val &= ~(1 << i);
			}
		}
		return val;
	case 0x20:
	case 0x21:
		return d_sio->read_io8(addr);
	case 0x30:
		return (config.boot_mode == MODE_PC88_N ? 0 : 1) | 2;	// 80x25
	case 0x31:
		return (config.boot_mode == MODE_PC88_V2 ? 0 : 0x80) | (config.boot_mode == MODE_PC88_V1S ? 0 : 0x40);
	case 0x32:
		return port32;
	case 0x40:
		return (vdisp ? 0 : 0x20) | (d_rtc->read_signal(0) ? 0x10 : 0) | (line200 ? 2 : 0);
	case 0x44:
	case 0x45:
		return d_opn->read_io8(addr);
	case 0x50:
		return crtc_status;
	case 0x51:
		return 0xff;
	case 0x5c:
		return gvram_sel;
	case 0x68:
		return dma_status;
	case 0x6e:
		return 0;
	case 0x70:
		return tw_ofs;
	case 0x71:
		return erom_bank;
	case 0xe2:
		return eram_sel;
	case 0xe3:
		return eram_bank;
		break;
	case 0xe8:
		return kanji1[kanji1_addr.w.l * 2 + 1];
	case 0xe9:
		return kanji1[kanji1_addr.w.l * 2];
	case 0xec:
		return kanji2[kanji2_addr.w.l * 2 + 1];
	case 0xed:
		return kanji2[kanji2_addr.w.l * 2];
	case 0xfc:
	case 0xfd:
	case 0xfe:
		return d_pio->read_io8(addr);
	}
	return 0xff;
}

void PC8801::update_low_memmap()
{
	// read
	if(eram_sel & 1) {
		if((eram_bank & 0x0c) == 0) {
			SET_BANK_R(0x0000, 0x7fff, eram + 0x8000 * (eram_bank & 3));
		}
		else {
			SET_BANK_R(0x0000, 0x7fff, rdmy);
		}
	}
	else if(rm_mode & 2) {
		// 64K RAM
		SET_BANK_R(0x0000, 0x7fff, ram);
	}
	else if(rm_mode & 4) {
		// N-BASIC
		SET_BANK_R(0x0000, 0x7fff, n80rom);
	}
	else {
		// N-88BASIC
		SET_BANK_R(0x0000, 0x5fff, n88rom);
		if(erom_bank == 0xff) {
			SET_BANK_R(0x6000, 0x7fff, n88rom + 0x6000);
		}
		else if(erom_bank == 0xfe) {
			SET_BANK_R(0x6000, 0x7fff, n88erom + 0x2000 * erom_sel);
		}
		else {
			SET_BANK_R(0x6000, 0x7fff, rdmy);
		}
	}
	
	// write
	if(eram_sel & 0x10) {
		if((eram_bank & 0x0c) == 0) {
			SET_BANK_W(0x0000, 0x7fff, eram + 0x8000 * (eram_bank & 3));
		}
		else {
			SET_BANK_W(0x0000, 0x7fff, wdmy);
		}
	}
	else {
		SET_BANK_W(0x0000, 0x7fff, ram);
	}
}

void PC8801::update_tw_memmap()
{
	int ofs = (rm_mode == 0) ? ((tw_ofs << 8) & 0xff00) : 0x8000;
	SET_BANK(0x8000, 0x83ff, ram + ofs, ram + ofs);
}

void PC8801::update_tvram_memmap()
{
	if(tvram_sel == 0) {
		SET_BANK(0xf000, 0xffff, tvram, tvram);
	}
	else {
		SET_BANK(0xf000, 0xffff, ram + 0xf000, ram + 0xf000);
	}
}

void PC8801::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_PC8801_SOUND_IRQ) {
		if(!(port32 & 0x80)) {
			request_intr(IRQ_SOUND, ((data & mask) != 0));
		}
	}
}

void PC8801::event_callback(int event_id, int err)
{
	request_intr(IRQ_TIMER, true);
}

void PC8801::event_frame()
{
	int blink_rate = 8 * ((crtc_reg[0][1] >> 6) + 1);
	if(!(blink_counter++ < blink_rate)) {
		blink_on = !blink_on;
		blink_counter = 0;
	}
}

void PC8801::event_vline(int v, int clock)
{
	if(v == 0) {
		if(crtc_status & 0x10) {
			// start dma
			if(dma_reg[2].length.sd != 0) {
				dma_status &= ~4;
			}
		}
		memset(crtc_buffer, 0, sizeof(crtc_buffer));
		crtc_buffer_ptr = 0;
		vdisp = true;
	}
	else if(v == (line200 ? 200 : 400)) {
		if(crtc_status & 0x10) {
			// run dma
			if((dma_mode & 4) && !(dma_status & 4)) {
				uint16 addr = dma_reg[2].start.w.l;
				for(int i = 0; i < dma_reg[2].length.sd; i++) {
					write_dma_io8(0, read_dma_data8(addr++));
				}
				dma_status |= 4;
			}
			request_intr(IRQ_VRTC, true);
		}
		vdisp = false;
	}
}

void PC8801::draw_screen()
{
	memset(text, 0, sizeof(text));
	memset(graph, 0, sizeof(graph));
	
	if(!(disp_ctrl & 1) && (crtc_status & 0x10)) {
		draw_text();
	}
	if(graph_mode & 8) {
		if(graph_mode & 0x10) {
			draw_color_graph();
		}
		else if(line200) {
			draw_mono_graph();
		}
		else {
			draw_mono_hires_graph();
		}
	}
	if(update_palette) {
		if(graph_mode & 0x10) {
			if(port32 & 0x20) {
				for(int i = 0; i < 9; i++) {
					uint8 b = anapal[i][0] & 7;
					uint8 r = (anapal[i][0] >> 3) & 7;
					uint8 g = anapal[i][1] & 7;
					palette_graph_pc[i] = RGB_COLOR(r << 5, g << 5, b << 5);
				}
			}
			else {
				for(int i = 0; i < 9; i++) {
					uint8 pal = digipal[i];
					palette_graph_pc[i] = RGB_COLOR((pal & 2) ? 255 : 0, (pal & 4) ? 255 : 0, (pal & 1) ? 255 : 0);
				}
			}
		}
		else {
			palette_graph_pc[0] = 0;
			palette_graph_pc[1] = RGB_COLOR(255, 255, 255);
		}
		update_palette = false;
	}
	if(line200) {
		for(int y = 0; y < 200; y++) {
			scrntype* dest0 = emu->screen_buffer(y * 2);
			scrntype* dest1 = emu->screen_buffer(y * 2 + 1);
			uint8* src_t = text[y];
			uint8* src_g = graph[y];
			
			for(int x = 0; x < 640; x++) {
				uint8 t = src_t[x];
				dest0[x] = t ? palette_text_pc[t] : palette_graph_pc[src_g[x]];
			}
			if(config.scan_line) {
				memset(dest1, 0, 640 * sizeof(scrntype));
			}
			else {
				memcpy(dest1, dest0, 640 * sizeof(scrntype));
			}
		}
	}
	else {
		for(int y = 0; y < 400; y++) {
			scrntype* dest = emu->screen_buffer(y);
			uint8* src_t = text[y >> 1];
			uint8* src_g = graph[y];
			
			for(int x = 0; x < 640; x++) {
				uint8 t = src_t[x];
				dest[x] = t ? palette_text_pc[t] : palette_graph_pc[src_g[x]];
			}
		}
	}
}

uint8 PC8801::get_crtc_buffer(int ofs)
{
	if(ofs < crtc_buffer_ptr) {
		return crtc_buffer[ofs];
	}
	// DMA underrun
//	crtc_status &= ~0x10;
//	crtc_status |= 8;
	return 0;
}

void PC8801::draw_text()
{
	int width = (crtc_reg[0][0] & 0x7f) + 2;
	int height = (crtc_reg[0][1] & 0x3f) + 1;
	int char_lines = (crtc_reg[0][2] & 0x1f) + 1;
	if(char_lines >= 16) {
		char_lines >>= 1;
	}
//	if(!line200 || (crtc_reg[0][1] & 0x80)) {
//		char_lines >>= 1;
//	}
	int attrib_num = (crtc_reg[0][4] & 0x20) ? 0 : ((crtc_reg[0][4] & 0x1f) + 1) * 2;
	uint8 attribs[80], flags[128], cur_attrib = 0;
	if(attrib_num == 0) {
		memset(attribs, 0xe0, sizeof(attribs));
	}
	bool cursor_draw = cursor_on && (blink_on || !(crtc_reg[0][2] & 0x20));
	int cursor_line = (crtc_reg[0][2] & 0x40) ? 0 : 7;
	
	for(int cy = 0, ytop = 0, ofs = 0; cy < height && ytop < 200; cy++, ytop += char_lines, ofs += 120) {
		if(attrib_num != 0) {
			memset(flags, 0, sizeof(flags));
			for(int i = 2 * (attrib_num - 1); i >= 0; i -= 2) {
				flags[get_crtc_buffer(ofs + i + 80) & 0x7f] = 1;
			}
			for(int cx = 0, pos = 0; cx < width && cx < 80; cx++) {
				if(flags[cx]) {
					cur_attrib = get_crtc_buffer(ofs + pos + 81);
					pos += 2;
				}
				attribs[cx] = cur_attrib;
			}
		}
		bool cursor_now_y = (cursor_draw && cy == crtc_reg[4][1]);
		
		for(int x = 0, cx = 0; cx < width && cx < 80; x += 8, cx++) {
			if(!(text_mode & 1) && (cx & 1)) {
				continue;
			}
			uint8 code = get_crtc_buffer(ofs + cx);
			uint8 attrib = attribs[cx];
			
			bool under_line = false, upper_line = false, reverse = false, blink = false, secret = false;
			uint8 color = 7;
			uint8 *pattern = kanji1 + 0x1000 + code * 8;
			
			if(!(text_mode & 2) && (attrib & 8)) {
				if(attrib & 0x10) {
					pattern = sg_pattern + code * 8;
				}
				color = (attrib & 0xe0) ? (attrib >> 5) : 8;
			}
			else {
				if(attrib & 0x80) {
					pattern = sg_pattern + code * 8;
				}
				under_line = ((attrib & 0x20) != 0);
				upper_line = ((attrib & 0x10) != 0);
				reverse = ((attrib & 4) != 0);
				blink = ((attrib & 2) != 0) && blink_on;
				secret = ((attrib & 1) != 0);
			}
			bool cursor_now = (cursor_now_y && cx == crtc_reg[4][0]);
			
			for(int l = 0, y = ytop; l < char_lines && y < 200; l++, y++) {
				uint8 pat = pattern[l];
				if(secret || l >= 8) {
					pat = 0;
				}
				if((upper_line && l == 0) || (under_line && l >= 7)) {
					pat = 0xff;
				}
				if(blink || reverse || (cursor_now && l >= cursor_line)) {
					pat = ~pat;
				}
				
				uint8 *dest = &text[y][x];
				if(text_mode & 1) {
					dest[0] = (pat & 0x80) ? color : 0;
					dest[1] = (pat & 0x40) ? color : 0;
					dest[2] = (pat & 0x20) ? color : 0;
					dest[3] = (pat & 0x10) ? color : 0;
					dest[4] = (pat & 0x08) ? color : 0;
					dest[5] = (pat & 0x04) ? color : 0;
					dest[6] = (pat & 0x02) ? color : 0;
					dest[7] = (pat & 0x01) ? color : 0;
				}
				else {
					dest[ 0] = dest[ 1] = (pat & 0x80) ? color : 0;
					dest[ 2] = dest[ 3] = (pat & 0x40) ? color : 0;
					dest[ 4] = dest[ 5] = (pat & 0x20) ? color : 0;
					dest[ 6] = dest[ 7] = (pat & 0x10) ? color : 0;
					dest[ 8] = dest[ 9] = (pat & 0x08) ? color : 0;
					dest[10] = dest[11] = (pat & 0x04) ? color : 0;
					dest[12] = dest[13] = (pat & 0x02) ? color : 0;
					dest[14] = dest[15] = (pat & 0x01) ? color : 0;
				}
			}
		}
	}
}

void PC8801::draw_color_graph()
{
	int addr = 0;
	for(int y = 0; y < 200; y++) {
		uint8 *dest = graph[y];
		for(int x = 0; x < 640; x += 8) {
			uint8 b = gvram[addr | 0x0000];
			uint8 r = gvram[addr | 0x4000];
			uint8 g = gvram[addr | 0x8000];
			addr++;
			uint8 *dest = &graph[y][x];
			dest[0] = ((b & 0x80) >> 7) | ((r & 0x80) >> 6) | ((g & 0x80) >> 5);
			dest[1] = ((b & 0x40) >> 6) | ((r & 0x40) >> 5) | ((g & 0x40) >> 4);
			dest[2] = ((b & 0x20) >> 5) | ((r & 0x20) >> 4) | ((g & 0x20) >> 3);
			dest[3] = ((b & 0x10) >> 4) | ((r & 0x10) >> 3) | ((g & 0x10) >> 2);
			dest[4] = ((b & 0x08) >> 3) | ((r & 0x08) >> 2) | ((g & 0x08) >> 1);
			dest[5] = ((b & 0x04) >> 2) | ((r & 0x04) >> 1) | ((g & 0x04)     );
			dest[6] = ((b & 0x02) >> 1) | ((r & 0x02)     ) | ((g & 0x02) << 1);
			dest[7] = ((b & 0x01)     ) | ((r & 0x01) << 1) | ((g & 0x01) << 2);
		}
	}
}

void PC8801::draw_mono_graph()
{
	int addr = 0;
	for(int y = 0; y < 200; y++) {
		uint8 *dest = graph[y];
		for(int x = 0; x < 640; x += 8) {
			uint8 b = (disp_ctrl & 2) ? 0 : gvram[addr | 0x0000];
			uint8 r = (disp_ctrl & 4) ? 0 : gvram[addr | 0x4000];
			uint8 g = (disp_ctrl & 8) ? 0 : gvram[addr | 0x8000];
			addr++;
			uint8 *dest = &graph[y][x];
			dest[0] = ((b | r | g) & 0x80) >> 7;
			dest[1] = ((b | r | g) & 0x40) >> 6;
			dest[2] = ((b | r | g) & 0x20) >> 5;
			dest[3] = ((b | r | g) & 0x10) >> 4;
			dest[4] = ((b | r | g) & 0x08) >> 3;
			dest[5] = ((b | r | g) & 0x04) >> 2;
			dest[6] = ((b | r | g) & 0x02) >> 1;
			dest[7] = ((b | r | g) & 0x01)     ;
		}
	}
}

void PC8801::draw_mono_hires_graph()
{
	int addr = 0;
	for(int y = 0; y < 200; y++) {
		uint8 *dest0 = graph[y * 2];
		uint8 *dest1 = graph[y * 2 + 1];
		for(int x = 0; x < 640; x += 8) {
			uint8 b = (disp_ctrl & 2) ? gvram[addr | 0x0000] : 0;
			uint8 r = (disp_ctrl & 4) ? gvram[addr | 0x4000] : 0;
			addr++;
			uint8 *dest0 = &graph[y * 2    ][x];
			uint8 *dest1 = &graph[y * 2 + 1][x];
			dest0[0] = (b & 0x80) >> 7;
			dest0[1] = (b & 0x40) >> 6;
			dest0[2] = (b & 0x20) >> 5;
			dest0[3] = (b & 0x10) >> 4;
			dest0[4] = (b & 0x08) >> 3;
			dest0[5] = (b & 0x04) >> 2;
			dest0[6] = (b & 0x02) >> 1;
			dest0[7] = (b & 0x01)     ;
			dest1[0] = (r & 0x80) >> 7;
			dest1[1] = (r & 0x40) >> 6;
			dest1[2] = (r & 0x20) >> 5;
			dest1[3] = (r & 0x10) >> 4;
			dest1[4] = (r & 0x08) >> 3;
			dest1[5] = (r & 0x04) >> 2;
			dest1[6] = (r & 0x02) >> 1;
			dest1[7] = (r & 0x01)     ;
		}
	}
}

void PC8801::request_intr(int level, bool status)
{
	uint8 bit = 1 << level;
	
	if(status) {
		if(!(intr_req & bit)) {
			intr_req |= bit;
			update_intr();
		}
	}
	else {
		if(intr_req & bit) {
			intr_req &= ~bit;
			update_intr();
		}
	}
}

void PC8801::update_intr()
{
	if(intr_req & intr_mask1 & intr_mask2) {
		d_cpu->write_signal(SIG_CPU_IRQ, 1, 1);
	}
	else {
		d_cpu->write_signal(SIG_CPU_IRQ, 0, 0);
	}
}

uint32 PC8801::intr_ack()
{
	for(int i = 0; i < 8; i++) {
		uint8 bit = 1 << i;
		if(intr_req & intr_mask1 & intr_mask2 & bit) {
			intr_req &= ~bit;
//			intr_mask1 = 0;
			return i * 2;
		}
	}
	return 0;
}

double PC8801::frame_rate()
{
	return FRAMES_PER_SEC;
}

