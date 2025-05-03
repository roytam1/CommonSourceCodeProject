/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2007.02.09 -

	[ 1bit PCM ]
*/

#ifndef _PCM1BIT_H_
#define _PCM1BIT_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_PCM1BIT_SIGNAL	0
#define SIG_PCM1BIT_ON		1
#define SIG_PCM1BIT_MUTE	2

class PCM1BIT : public DEVICE
{
private:
	bool signal, on, mute;
	int vol, update, count;
	bool samples[256];
	
public:
	PCM1BIT(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~PCM1BIT() {}
	
	// common functions
	void initialize();
	void write_signal(int id, uint32 data, uint32 mask);
	void event_frame();
	void event_callback(int event_id, int err);
	void mix(int32* buffer, int cnt);
	
	// unique function
	void init(int rate, int volume);
};

#endif

