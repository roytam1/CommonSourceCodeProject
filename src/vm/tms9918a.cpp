/*
	Skelton for retropc emulator

	Origin : MESS
	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ TMS9918A ]
*/

#include "tms9918a.h"

void TMS9918A::initialize()
{
	// initialize
	_memset(regs, 0, sizeof(regs));
	status_reg = latch_reg = vram_latch = 0;
	vram_addr = 0;
	latch = intstat = false;
	
	// regist event
	vm->regist_vsync_event(this);
}

void TMS9918A::reset()
{
	_memset(regs, 0, sizeof(regs));
	status_reg = latch_reg = vram_latch = 0;
	vram_addr = 0;
	latch = intstat = false;
}

void TMS9918A::write_io8(uint32 addr, uint32 data)
{
	if(addr & 1) {
		// control reg
		if(latch) {
			if(data & 0x80) {
				// write register
				int num = data & 7;
				uint8 prev = regs[num];
				regs[num] = latch_reg & mask[num];
				
				// check interrupt
				if(num == 1 && !(prev & 0x20) && (regs[1] & 0x20) && (status_reg & 0x80)) {
					for(int i = 0; i < dev_cnt; i++)
						dev[i]->write_signal(dev_id[i], 0xffffffff, dev_mask[i]);
				}
			}
			else {
				// write address
				vram_addr = (((uint16)data << 8) | latch_reg) & (TMS9918A_VRAM_SIZE - 1);
				// read latch
				if(!(data & 0x40)) {
					vram_latch = vram[vram_addr];
					vram_addr = (vram_addr + 1) & (TMS9918A_VRAM_SIZE - 1);
				}
			}
			latch = false;
		}
		else {
			latch_reg = data;
			latch = true;
		}
	}
	else {
		// vram
		vram[vram_addr] = data;
		vram_addr = (vram_addr + 1) & (TMS9918A_VRAM_SIZE - 1);
		vram_latch = data;
		latch = false;
	}
}

uint32 TMS9918A::read_io8(uint32 addr)
{
	uint8 val = 0xff;
	
	if(addr & 1) {
		// status reg
		val = status_reg;
		status_reg &= 0x1f;
		latch = false;
	}
	else {
		// vram
		val = vram_latch;
		vram_latch = vram[vram_addr];
		vram_addr = (vram_addr + 1) & (TMS9918A_VRAM_SIZE - 1);
		latch = false;
	}
	return val;
}

void TMS9918A::draw_screen()
{
	// update screen buffer
	for(int y = 0; y < 192; y++) {
		uint16* dest = emu->screen_buffer(y);
		uint8* src = screen[y];
		for(int x = 0; x < 256; x++)
			dest[x] = palette_pc[src[x] & 0xf];
	}
}

void TMS9918A::event_vsync(int v, int clock)
{
	if(v == 192) {
		// create virtual screen
		if(regs[1] & 0x40) {
			// not blank
			
			// draw character plane
			int mode = ((regs[1] & 0x8) >> 1) | (regs[0] & 0x2) | ((regs[2] & 0x10) >> 1);
			switch(mode)
			{
			case 0: draw_mode0(); break;
			case 1: draw_mode1(); break;
			case 2: draw_mode2(); break;
			case 3: draw_mode12(); break;
			case 4: draw_mode3(); break;
			case 5: draw_mode13(); break;
			case 6: draw_mode23(); break;
			case 7: draw_mode123(); break;
			}
			
			// draw sprite plane
			if((regs[1] & 0x50) == 0x40)
				draw_sprites();
		}
		else {
			// blank
			_memset(screen, 0, sizeof(screen));
		}
		
		// do interrupt
		if((regs[1] & 0x20) && !(status_reg & 0x80)) {
			for(int i = 0; i < dev_cnt; i++)
				dev[i]->write_signal(dev_id[i], 0xffffffff, dev_mask[i]);
		}
		status_reg |= 0x80;
	}
}

void TMS9918A::draw_mode0()
{
	// mode 0
	int name = 0;
	int name_base = ((regs[2] & 0xf) << 10) & (TMS9918A_VRAM_SIZE - 1);
	int pattern_base = (regs[0] & 0x2) ? ((regs[4] & 0x4) << 11) & (TMS9918A_VRAM_SIZE - 1) : ((regs[4] & 0x7) << 11) & (TMS9918A_VRAM_SIZE - 1);
	int color_base = (regs[0] & 0x2) ? ((regs[3] & 0x80) << 6) & (TMS9918A_VRAM_SIZE - 1) : (regs[3] << 6) & (TMS9918A_VRAM_SIZE - 1);
	uint8 backdrop_color = regs[7] & 0xf;
	
	for(int y = 0; y < 24; y++) {
		for(int x = 0; x < 32; x++, name++) {
			int code = vram[name_base + name];
			uint8* pattern_ptr = &vram[pattern_base + code * 8];
			uint8 color = vram[color_base + (code >> 3)];
			uint8 fore_color = (color >> 4) ? (color >> 4) : backdrop_color;
			uint8 back_color = (color & 0xf) ? (color & 0xf) : backdrop_color;
			for(int yy = 0; yy < 8; yy++) {
				uint8* buffer = screen[y * 8 + yy] + x * 8;
				uint8 pattern = pattern_ptr[yy];
				buffer[0] = (pattern & 0x80) ? fore_color : back_color;
				buffer[1] = (pattern & 0x40) ? fore_color : back_color;
				buffer[2] = (pattern & 0x20) ? fore_color : back_color;
				buffer[3] = (pattern & 0x10) ? fore_color : back_color;
				buffer[4] = (pattern & 0x08) ? fore_color : back_color;
				buffer[5] = (pattern & 0x04) ? fore_color : back_color;
				buffer[6] = (pattern & 0x02) ? fore_color : back_color;
				buffer[7] = (pattern & 0x01) ? fore_color : back_color;
			}
		}
	}
}

void TMS9918A::draw_mode1()
{
	// mode 1
	int name = 0;
	int name_base = ((regs[2] & 0xf) << 10) & (TMS9918A_VRAM_SIZE - 1);
	int pattern_base = (regs[0] & 0x2) ? ((regs[4] & 0x4) << 11) & (TMS9918A_VRAM_SIZE - 1) : ((regs[4] & 0x7) << 11) & (TMS9918A_VRAM_SIZE - 1);
	uint8 backdrop_color = regs[7] & 0xf;
	uint8 fore_color = (regs[7] >> 4) ? (regs[7] >> 4) : backdrop_color;
	uint8 back_color = (regs[7] & 0xf) ? (regs[7] & 0xf) : backdrop_color;
	
	for(int y = 0; y < 24; y++) {
		for(int x = 0; x < 40; x++, name++) {
			int code = vram[name_base + name];
			uint8* pattern_ptr = &vram[pattern_base + code * 8];
			for(int yy = 0; yy < 8; yy++) {
				uint8* buffer = screen[y * 8 + yy] + x * 6 + 8;
				uint8 pattern = pattern_ptr[yy];
				buffer[0] = (pattern & 0x80) ? fore_color : back_color;
				buffer[1] = (pattern & 0x40) ? fore_color : back_color;
				buffer[2] = (pattern & 0x20) ? fore_color : back_color;
				buffer[3] = (pattern & 0x10) ? fore_color : back_color;
				buffer[4] = (pattern & 0x08) ? fore_color : back_color;
				buffer[5] = (pattern & 0x04) ? fore_color : back_color;
			}
		}
	}
}

void TMS9918A::draw_mode2()
{
	// mode 2
	int name = 0;
	int name_base = ((regs[2] & 0xf) << 10) & (TMS9918A_VRAM_SIZE - 1);
	int pattern_base = (regs[0] & 0x2) ? ((regs[4] & 0x4) << 11) & (TMS9918A_VRAM_SIZE - 1) : ((regs[4] & 0x7) << 11) & (TMS9918A_VRAM_SIZE - 1);
	int color_base = (regs[0] & 0x2) ? ((regs[3] & 0x80) << 6) & (TMS9918A_VRAM_SIZE - 1) : (regs[3] << 6) & (TMS9918A_VRAM_SIZE - 1);
	int color_mask = ((regs[3] & 0x7f) << 3) | 0x7;
	int pattern_mask = ((regs[4] & 0x3) << 8) | (color_mask & 0xff);
	uint8 backdrop_color = regs[7] & 0xf;
	
	for(int y = 0; y < 24; y++) {
		for(int x = 0; x < 32; x++, name++) {
			int code = vram[name_base + name] + (y & 0xf8) * 32;
			uint8* pattern_ptr = &vram[pattern_base + (code & color_mask) * 8];
			uint8* color_ptr = &vram[color_base + (code & pattern_mask) * 8];
			for(int yy = 0; yy < 8; yy++) {
				uint8* buffer = screen[y * 8 + yy] + x * 8;
				uint8 pattern = pattern_ptr[yy];
				uint8 color = color_ptr[yy];
				uint8 fore_color = (color >> 4) ? (color >> 4) : backdrop_color;
				uint8 back_color = (color & 0xf) ? (color & 0xf) : backdrop_color;
				buffer[0] = (pattern & 0x80) ? fore_color : back_color;
				buffer[1] = (pattern & 0x40) ? fore_color : back_color;
				buffer[2] = (pattern & 0x20) ? fore_color : back_color;
				buffer[3] = (pattern & 0x10) ? fore_color : back_color;
				buffer[4] = (pattern & 0x08) ? fore_color : back_color;
				buffer[5] = (pattern & 0x04) ? fore_color : back_color;
				buffer[6] = (pattern & 0x02) ? fore_color : back_color;
				buffer[7] = (pattern & 0x01) ? fore_color : back_color;
			}
		}
	}
}

void TMS9918A::draw_mode12()
{
	// mode 1 + mode 2
	int name = 0;
	int name_base = ((regs[2] & 0xf) << 10) & (TMS9918A_VRAM_SIZE - 1);
	int pattern_base = (regs[0] & 0x2) ? ((regs[4] & 0x4) << 11) & (TMS9918A_VRAM_SIZE - 1) : ((regs[4] & 0x7) << 11) & (TMS9918A_VRAM_SIZE - 1);
	int color_mask = ((regs[3] & 0x7f) << 3) | 0x7;
	int pattern_mask = ((regs[4] & 0x3) << 8) | (color_mask & 0xff);
	uint8 backdrop_color = regs[7] & 0xf;
	uint8 fore_color = (regs[7] >> 4) ? (regs[7] >> 4) : backdrop_color;
	uint8 back_color = (regs[7] & 0xf) ? (regs[7] & 0xf) : backdrop_color;
	
	for(int y = 0; y < 24; y++) {
		for(int x = 0; x < 40; x++, name++) {
			int code = (vram[name_base + name] + (y & 0xf8) * 32) & pattern_mask;
			uint8* pattern_ptr = &vram[pattern_base + code * 8];
			for(int yy = 0; yy < 8; yy++) {
				uint8* buffer = screen[y * 8 + yy] + x * 6 + 8;
				uint8 pattern = pattern_ptr[yy];
				buffer[0] = (pattern & 0x80) ? fore_color : back_color;
				buffer[1] = (pattern & 0x40) ? fore_color : back_color;
				buffer[2] = (pattern & 0x20) ? fore_color : back_color;
				buffer[3] = (pattern & 0x10) ? fore_color : back_color;
				buffer[4] = (pattern & 0x08) ? fore_color : back_color;
				buffer[5] = (pattern & 0x04) ? fore_color : back_color;
			}
		}
	}
}

void TMS9918A::draw_mode3()
{
	// mode 3
	int name = 0;
	int name_base = ((regs[2] & 0xf) << 10) & (TMS9918A_VRAM_SIZE - 1);
	int pattern_base = (regs[0] & 0x2) ? ((regs[4] & 0x4) << 11) & (TMS9918A_VRAM_SIZE - 1) : ((regs[4] & 0x7) << 11) & (TMS9918A_VRAM_SIZE - 1);
	uint8 backdrop_color = regs[7] & 0xf;
	
	for(int y = 0; y < 24; y++) {
		for(int x = 0; x < 32; x++, name++) {
			int code = vram[name_base + name];
			uint8* pattern_ptr = &vram[pattern_base + code * 8 + (y & 0x3) * 2];
			for(int yy = 0; yy < 2; yy++) {
				uint8 color = pattern_ptr[yy];
				uint8 fore_color = (color >> 4) ? (color >> 4) : backdrop_color;
				uint8 back_color = (color & 0xf) ? (color & 0xf) : backdrop_color;
				for(int yyy = 0; yyy < 4; yyy++) {
					uint8* buffer = screen[y * 8 + yy * 4 + yyy] + x * 8;
					buffer[0] = buffer[1] = buffer[2] = buffer[3] = fore_color;
					buffer[4] = buffer[5] = buffer[6] = buffer[7] = back_color;
				}
			}
		}
	}
}

void TMS9918A::draw_mode13()
{
	// mode 1 + mode 3
	uint8 backdrop_color = regs[7] & 0xf;
	uint8 fore_color = (regs[7] >> 4) ? (regs[7] >> 4) : backdrop_color;
	uint8 back_color = (regs[7] & 0xf) ? (regs[7] & 0xf) : backdrop_color;
	
	for(int y = 0; y < 192; y++) {
		uint8* buffer = screen[y];
		int x = 0;
		for(int i = 0; i < 8; i++)
			buffer[x++] = back_color;
		for(int i = 0; i < 40; i++) {
			for(int j = 0; j < 4; j++)
				buffer[x++] = fore_color;
			for(int j = 0; j < 2; j++)
				buffer[x++] = back_color;
		}
		for(int i = 0; i < 8; i++)
			buffer[x++] = back_color;
	}
}

void TMS9918A::draw_mode23()
{
	// mode 2 + mode 3
	int name = 0;
	int name_base = ((regs[2] & 0xf) << 10) & (TMS9918A_VRAM_SIZE - 1);
	int pattern_base = (regs[0] & 0x2) ? ((regs[4] & 0x4) << 11) & (TMS9918A_VRAM_SIZE - 1) : ((regs[4] & 0x7) << 11) & (TMS9918A_VRAM_SIZE - 1);
	int color_mask = ((regs[3] & 0x7f) << 3) | 0x7;
	int pattern_mask = ((regs[4] & 0x3) << 8) | (color_mask & 0xff);
	uint8 backdrop_color = regs[7] & 0xf;
	
	for(int y = 0; y < 24; y++) {
		for(int x = 0; x < 32; x++, name++) {
			int code = vram[name_base + name];
			uint8* pattern_ptr = &vram[pattern_base + ((code + (y & 0x3) * 2 + (y & 0xf8) * 32) & pattern_mask) * 8];
			for(int yy = 0; yy < 2; yy++) {
				uint8 color = pattern_ptr[yy];
				uint8 fore_color = (color >> 4) ? (color >> 4) : backdrop_color;
				uint8 back_color = (color & 0xf) ? (color & 0xf) : backdrop_color;
				for(int yyy = 0; yyy < 4; yyy++) {
					uint8* buffer = screen[y * 8 + yy * 4 + yyy] + x * 8;
					buffer[0] = buffer[1] = buffer[2] = buffer[3] = fore_color;
					buffer[4] = buffer[5] = buffer[6] = buffer[7] = back_color;
				}
			}
		}
	}
}

void TMS9918A::draw_mode123()
{
	// mode 1 + mode 2 + mode 3
	uint8 backdrop_color = regs[7] & 0xf;
	uint8 fore_color = (regs[7] >> 4) ? (regs[7] >> 4) : backdrop_color;
	uint8 back_color = (regs[7] & 0xf) ? (regs[7] & 0xf) : backdrop_color;
	
	for(int y = 0; y < 192; y++) {
		uint8* buffer = screen[y];
		int x = 0;
		for(int i = 0; i < 8; i++)
			buffer[x++] = back_color;
		for(int i = 0; i < 40; i++) {
			for(int j = 0; j < 4; j++)
				buffer[x++] = fore_color;
			for(int j = 0; j < 2; j++)
				buffer[x++] = back_color;
		}
		for(int i = 0; i < 8; i++)
			buffer[x++] = back_color;
	}
}

void TMS9918A::draw_sprites()
{
	int attrib_base = (regs[5] * 128) & (TMS9918A_VRAM_SIZE - 1);
	uint8* attrib_ptr = &vram[attrib_base];
	
	int sprite_size = (regs[1] & 0x2) ? 16 : 8;
	bool sprite_large = (regs[1] & 0x1) ? true : false;
	
	int sprite_limit[192];
	for(int i = 0; i < 192; i++)
		sprite_limit[i] = 4;
	int illegal_sprite_line = 255, illegal_sprite = 0, p;
	
	status_reg &= 0x80;
	_memset(sprite_check, 0, sizeof(sprite_check));
	
	// draw 32 sprites
	for(p = 0; p < 32; p++) {
		// get sprite location
		int y = *attrib_ptr++;
		if(y == 208)
			break;
		y = (y > 208) ? -(~y & 0xff) : y + 1;
		
		int x = *attrib_ptr++;
		int pattern_base = (((regs[6] & 0x7) * 2048) & (TMS9918A_VRAM_SIZE - 1)) + ((sprite_size == 16) ? (*attrib_ptr & 0xfc) : *attrib_ptr) * 8;
		uint8* pattern_ptr = &vram[pattern_base];
		attrib_ptr++;
		
		uint8 color = *attrib_ptr & 0xf;
		if(*attrib_ptr & 0x80)
			x -= 32;
		attrib_ptr++;
		
		if(!sprite_large) {
			// normal size
			for(int yy = y; yy < (y + sprite_size); yy++) {
				if(!(0 <= yy && yy < 192))
					continue;
				
				// check illegal sprite
				if(sprite_limit[yy] == 0) {
					if(yy < illegal_sprite_line) {
						illegal_sprite_line = yy;
						illegal_sprite = p;
					}
					else if(illegal_sprite_line == yy) {
						if(illegal_sprite > p)
							illegal_sprite = p;
					}
					continue;
				}
				else {
					sprite_limit[yy]--;
				}
				
				// draw sprite
				int line = pattern_ptr[yy - y] * 256 + pattern_ptr[yy - y + 16];
				for(int xx = x; xx < (x + sprite_size); xx++) {
					if(line & 0x8000) {
						if(0 <= xx && xx < 256) {
							if(sprite_check[yy * 256 + xx])
								status_reg |= 0x20;
							else
								sprite_check[yy * 256 + xx] = 1;
							if(color && !(sprite_check[yy * 256 + xx] & 0x2)) {
								sprite_check[yy * 256 + xx] |= 0x2;
								screen[yy][xx] = color;
							}
						}
					}
					line *= 2;
				}
			}
		}
		else {
			// large size
			for(int i = 0; i < sprite_size; i++) {
				int yy = y + i * 2;
				int line2 = pattern_ptr[i] * 256 + pattern_ptr[i + 16];
				for(int j = 0; j < 2; j++) {
					if(0 <= yy && y < 192) {
						// check illegal sprite
						if(sprite_limit[yy] == 0) {
							if(yy < illegal_sprite_line) {
								illegal_sprite_line = yy;
								illegal_sprite = p;
							}
							else if(illegal_sprite_line == yy) {
								if(illegal_sprite > p)
									illegal_sprite = p;
							}
							continue;
						}
						else {
							sprite_limit[yy]--;
						}
						
						int line = line2;
						for(int xx = x ; xx < (x + sprite_size * 2); xx += 2) {
							if(line & 0x8000) {
								if(0 <= xx && xx < 256) {
									if(sprite_check[yy * 256 + xx])
										status_reg |= 0x20;
									else
										sprite_check[yy * 256 + xx] = 0x01;
									if(color && !(sprite_check[yy * 256 + xx] & 0x02)) {
										sprite_check[yy * 256 + xx] |= 0x02;
										screen[yy][xx] = color;
									}
								}
								if(0 <= xx + 1 && xx + 1 < 256) {
									if (sprite_check[yy * 256 + xx + 1])
										status_reg |= 0x20;
									else
									 	sprite_check[yy * 256 + xx + 1] = 0x01;
									if(color && !(sprite_check[yy * 256 + xx + 1] & 0x02)) {
										sprite_check[yy * 256 + xx + 1] |= 0x02;
										screen[yy][xx + 1] = color;
									}
								}
							}
							line *= 2;
						}
					}
					yy++;
				}
			}
		}
	}
	
	if(illegal_sprite_line == 255)
		status_reg |= (p > 0x1f) ? 0x1f : p;
	else
		status_reg |= 0x40 + illegal_sprite;
}

