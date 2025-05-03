/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ SN76489AN ]
*/

#include "sn76489an.h"

#define FB_WNOISE 0x14002
#define FB_PNOISE 0x08000
#define NG_PRESET 0x00f35

void SN76489AN::initialize()
{
	mute = false;
	cs = we = true;
}

void SN76489AN::reset()
{
	for(int i = 0; i < 4; i++) {
		ch[i].count = 0;
		ch[i].period = 1;
		ch[i].volume = 0;
		ch[i].signal = false;
	}
	for(int i = 0; i < 8; i += 2) {
		regs[i + 0] = 0;
		regs[i + 1] = 0xf;	// volume = 0
	}
	noise_gen = NG_PRESET;
	ch[3].signal = (NG_PRESET & 1) ? true : false;
}

void SN76489AN::write_io8(uint32 addr, uint32 data)
{
	if(data & 0x80) {
		index = (data >> 4) & 7;
		int c = index >> 1;
		
		switch(index & 7)
		{
		case 0: case 2: case 4:
			// tone : frequency
			regs[index] = (regs[index] & 0x3f0) | (data & 0xf);
			ch[c].period = regs[index] ? regs[index] : 1;
//			ch[c].count = 0;
			break;
		case 1: case 3: case 5: case 7:
			// tone / noise : volume
			regs[index] = data & 0xf;
			ch[c].volume = volume_table[data & 0xf];
			break;
		case 6:
			// noise : frequency, mode
			regs[6] = data;
			noise_fb = (data & 4) ? FB_WNOISE : FB_PNOISE;
			data &= 3;
			ch[3].period = (data == 3) ? (ch[2].period << 9) : (1 << (data + 5));
//			ch[3].count = 0;
			noise_gen = NG_PRESET;
			ch[3].signal = (NG_PRESET & 1) ? true : false;
			break;
		}
	}
	else {
		int c = index >> 1;
		
		switch(index & 0x7)
		{
		case 0: case 2: case 4:
			// tone : frequency
			regs[index] = (regs[index] & 0xf) | (((uint16)data << 4) & 0x3f0);
			ch[c].period = regs[index] ? regs[index] : 1;
//			ch[c].count = 0;
			// update noise shift frequency
			if(index == 4 && (regs[6] & 3) == 0x3)
				ch[3].period = ch[2].period << 1;
			break;
		}
	}
}

void SN76489AN::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_SN76489AN_MUTE)
		mute = ((data & mask) != 0);
	else if(id == SIG_SN76489AN_DATA)
		val = data & mask;
	else if(id == SIG_SN76489AN_CS) {
		bool next = ((data & mask) != 0);
		if(cs != next) {
			if(!(cs = next) && !we)
				write_io8(0, val);
		}
	}
	else if(id == SIG_SN76489AN_CS) {
		bool next = ((data & mask) != 0);
		if(cs != next) {
			cs = next;
			if(!cs && !we)
				write_io8(0, val);
		}
	}
	else if(id == SIG_SN76489AN_WE) {
		bool next = ((data & mask) != 0);
		if(we != next) {
			we = next;
			if(!cs && !we)
				write_io8(0, val);
		}
	}
}

void SN76489AN::mix(int32* buffer, int cnt)
{
	if(mute)
		return;
	for(int i = 0; i < cnt; i++) {
		int32 vol = buffer[i];
		for(int j = 0; j < 4; j++) {
			if(!ch[j].volume)
				continue;
			
			ch[j].count -= diff;
			if(ch[j].count < 0) {
				ch[j].count += ch[j].period << 8;
				if(j == 3) {
					if(noise_gen & 1)
						noise_gen ^= noise_fb;
					noise_gen >>= 1;
					ch[3].signal = (noise_gen & 1) ? true : false;
				}
				else
					ch[j].signal = !ch[j].signal;
			}
			vol += ch[j].signal ? ch[j].volume : -ch[j].volume;
		}
		buffer[i] = vol;
	}
}

void SN76489AN::init(int rate, int clock, int volume)
{
	// create gain
	double vol = volume;
	for(int i = 0; i < 15; i++) {
		volume_table[i] = (int)vol;
		vol /= 1.258925412;
	}
	volume_table[15] = 0;
	diff = 16 * clock / rate;
}

