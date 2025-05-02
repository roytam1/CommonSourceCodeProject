//
// refresh screen by Koichi Nishida 2006
// based on Marat Fayzullin's fMSX
// and Hiroshi Ishioka's iP6
//

/*
	NEC PC-6001 Emulator 'yaPC-6001'
	NEC PC-6601 Emulator 'yaPC-6601'

	Author : tanam
	Date   : 2013.07.15-

	[ memory ]
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

class MEMORY : public DEVICE
{
private:
	DEVICE *d_cpu;
	// memory
	uint8 RAM[0x10000];
	uint8 EXTROM[0x4000];	// CURRENT EXTEND ROM
	uint8 BASICROM[0x8000];	// BASICROM
	uint8 *CGROM;
	uint8 CGROM1[0x2400];	// CGROM1
	uint8 *EXTROM1;			// EXTEND ROM 1
	uint8 *EXTROM2;			// EXTEND ROM 2
	uint8 *RdMem[8];		// READ  MEMORY MAPPING ADDRESS
	uint8 *WrMem[8];		// WRITE MEMORY MAPPING ADDRESS
	uint8 *VRAM;
	uint8 EmptyRAM[0x2000];
	uint8 EnWrite[4];		// MEMORY MAPPING WRITE ENABLE [N60/N66]
	byte CGSW93;
	bool inserted;
#ifdef _PC6001
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) { inserted = false; }
	~MEMORY() {}
#else
	uint8 VOICEROM[0x4000];
	uint8 KANJIROM[0x8000];
	uint8 *CurKANJIROM;
	uint8 CGROM5[0x2000];	// CGROM5
	byte CRTMode1,CRTMode2,CRTMode3;
	byte CSS1,CSS2,CSS3;
	// for 8255 & Sub CPU
	byte IBF5;
	byte INT3;
	byte WIE6;
	// some port values
	byte portF0;
	byte portF1;
	uint8 screen[200][320];
	uint8* dest;
	int palette[16];
	// color palettes
	int BPal[16],BPal11[4],BPal12[8],BPal13[8],BPal14[4],BPal15[8],BPal53[32];
	uint8 W;
public:
	MEMORY(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {inserted=false;}
	~MEMORY() {}
	// common functions
	void initialize();
	void RefreshScr10();
	void RefreshScr11();
	void RefreshScr13();
	void RefreshScr13a();
	void RefreshScr13b();
	void RefreshScr13c();
	void RefreshScr13d();
	void RefreshScr13e();
	void RefreshScr51();
	void RefreshScr53();
	void RefreshScr54();
	void draw_screen();
	uint32 read_io8(uint32 addr);
#endif
	// common functions
	void reset();
	void write_data8(uint32 addr, uint32 data);
	uint32 read_data8(uint32 addr);
	void write_io8(uint32 addr, uint32 data);
	// unique functions
	void set_context_cpu(DEVICE* device) {
		d_cpu = device;
	}
	void open_cart(_TCHAR* file_path);
	void close_cart();
	bool cart_inserted() {
		return inserted;
	}
	uint8* get_vram() {
		return RAM;
	}
	byte TimerSW;
};
#endif
