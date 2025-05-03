/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ sound ]
*/

#ifndef _SOUND_H_
#define _SOUND_H_

#include "vm.h"
#include "../emu.h"

#include "device.h"

#define VOLUME 30000

class SOUND : public DEVICE
{
private:
	// beep
	int count, dif;
	bool signal;
	
	// sound buffer
	uint16* sound_buffer;
	int sound_rate;
	int buffer_ptr;
	int sound_samples;
	int accum_samples, update_samples;
	int update_usec;
	
public:
	SOUND(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~SOUND() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	
	// unique function
	void initialize_sound(int rate, int samples);
	void update_sound();
	uint16* create_sound(int samples, bool fill);
	
	void set_ctc(int hz) { dif = sound_rate / hz / 2; }
	bool beep_cont, beep_timer;
};

#endif

