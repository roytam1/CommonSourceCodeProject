/*
	CASIO PV-2000 Emulator 'EmuGaki'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ keyboard ]
*/

#include "keyboard.h"

void KEYBOARD::initialize()
{
	key_stat = emu->key_buffer();
	joy_stat = emu->joy_buffer();
	key_no = 0;
	intr_enb = false;
}

void KEYBOARD::write_io8(uint32 addr, uint32 data)
{
	intr_enb = (data == 0xf) ? true : false;
	key_no = data & 0xf;
}

uint32 KEYBOARD::read_io8(uint32 addr)
{
	uint32 val = 0;
	
	switch(addr & 0xff)
	{
	case 0x10:
		if(key_stat[key_map[key_no][7]]) val |= 1;
		if(key_stat[key_map[key_no][6]]) val |= 2;
		if(key_stat[key_map[key_no][5]]) val |= 4;
		if(key_stat[key_map[key_no][4]]) val |= 8;
		return val;
	case 0x20:
		if(key_stat[key_map[key_no][3]]) val |= 1;
		if(key_stat[key_map[key_no][2]]) val |= 2;
		if(key_stat[key_map[key_no][1]]) val |= 4;
		if(key_stat[key_map[key_no][0]]) val |= 8;
		if(key_no == 6) {
			if(joy_stat[0] & 0x02) val |= 1;
			if(joy_stat[0] & 0x08) val |= 2;
			if(joy_stat[1] & 0x02) val |= 4;
			if(joy_stat[1] & 0x08) val |= 8;
		}
		else if(key_no == 7) {
			if(joy_stat[0] & 0x04) val |= 1;
			if(joy_stat[0] & 0x01) val |= 2;
			if(joy_stat[1] & 0x04) val |= 4;
			if(joy_stat[1] & 0x01) val |= 8;
		}
		else if(key_no == 8) {
			if(joy_stat[0] & 0x10) val |= 1;
			if(joy_stat[0] & 0x20) val |= 2;
			if(joy_stat[1] & 0x10) val |= 4;
			if(joy_stat[1] & 0x20) val |= 8;
		}
		return val;
	case 0x40:
		if(key_stat[0x11]) val |= 1;	// COLOR (CTRL)
		if(key_stat[0x09]) val |= 2;	// FUNC (TAB)
		if(key_stat[0x10]) val |= 4;	// SHIFT
		return val;
	}
	return 0xff;
}

void KEYBOARD::key_down()
{
	if(intr_enb) {
		dev->set_intr_line(true, true, 0);
		intr_enb = false;
	}
}

