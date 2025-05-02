/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ SN76489AN ]
*/

#ifndef _SN76489AN_H_
#define _SN76489AN_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_SN76489AN_MUTE	0

#define FB_WNOISE 0x14002
#define FB_PNOISE 0x08000
#define NG_PRESET 0x00f35

class SN76489AN : public DEVICE
{
private:
	// register
	uint16 regs[8];
	int index;
	
	// sound info
	typedef struct {
		int count;
		int period;
		int volume;
		bool signal;
	} channel_t;
	channel_t ch[4];
	uint32 noise_fb, noise_gen;
	int volume_table[16];
	int diff;
	bool mute;
	
public:
	SN76489AN(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~SN76489AN() {}
	
	// common functions
	void initialize();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	void write_signal(int id, uint32 data, uint32 mask);
	void mix(int32* buffer, int cnt);
	
	// unique function
	void init(int rate, int clock, int volume);
};

#endif

