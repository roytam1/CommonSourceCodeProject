/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ kanji rom ]
*/

#ifndef _KANJI_H_
#define _KANJI_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class KANJI : public DEVICE
{
private:
	uint8 kanji[0x4bc00];
	int kaddr, ofs, flag;
	uint8* pattern;
	
	uint16 jis2addr(uint16 code);
	void get_kanji_pattern(uint16 addr);
	
public:
	KANJI(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~KANJI() {}
	
	// common functions
	void initialize();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
};

#endif

