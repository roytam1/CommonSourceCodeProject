/*
	Skelton for retropc emulator

	Origin : PCem
	Author : Takeda.Toshiya
	Date  : 2009.04.10-

	[ i386 ]
*/

#ifndef _I386_H_ 
#define _I386_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_I386_A20	0

class I386 : public DEVICE
{
private:
	/* ---------------------------------------------------------------------------
	contexts
	--------------------------------------------------------------------------- */
	
	DEVICE *d_mem, *d_io, *d_pic, *d_bios;
	
	/* ---------------------------------------------------------------------------
	registers
	--------------------------------------------------------------------------- */

	// cycles
	int cycles, base_cycles;
	bool busreq;
	
	// registers
	pair regs[8];
	typedef struct {
		uint32 base;
		uint16 limit;
		uint8 access;
		uint16 seg;
	} x86seg;
	x86seg gdt, ldt, idt, tr;
	x86seg _cs, _ds, _es, _ss, _fs, _gs;
	union {
		uint32 l;
		uint16 w;
	} CR0;
	uint32 cr2, cr3;
	uint32 pc, prev_pc, a20mask;
	
	// flags
	uint16 flags, eflags;
	uint8 znptable8[256];
	uint16 znptable16[65536];
	
	// mmu
	uint32 mmucache[0x100000];
	int mmunext, mmucaches[64];
	
	// status
	int use32, stack32, optype;
	int intstat;
	int ssegs, abrt;
	int intgatesize, notpresent;
	uint16 notpresent_error;
	
	uint32 oldds, oldss, oldpc, oxpc;
	uint16 oldcs;
	int oldcpl, cflag;
	
	// segment
	uint32 easeg, eaaddr, fetchdat, rmdat;
	int rm, reg, mod;
	uint16 op32;
	
	// modrm
	uint16 zero;
	uint16 *mod1add[2][8];
	uint32 *mod1seg[8];
	
	/* ---------------------------------------------------------------------------
	virtual machine interfaces
	--------------------------------------------------------------------------- */
	
	// memory
	#define mmutranslate(addr, rw) ((mmucache[addr >> 12] != 0xFFFFFFFF) ? (mmucache[addr >> 12] + (addr & 0xFFF)) : mmutranslate2(addr, rw))
	
	uint8 RM8(uint32 addr) {
		uint32 addr2 = addr;
		if(CR0.l >> 31) {
			addr2 = mmutranslate(addr, 0);
			if(addr2 == 0xFFFFFFFF)
				return 0xFF;
		}
		return d_mem->read_data8(addr2 & a20mask);
	}
	void WM8(uint32 addr, uint8 val) {
		uint32 addr2 = addr;
		if(CR0.l >> 31) {
			addr2 = mmutranslate(addr, 1);
			if(addr2 == 0xFFFFFFFF)
				return;
		}
		d_mem->write_data8(addr2 & a20mask, val);
	}
	uint16 RM16(uint32 addr) {
		uint32 addr2 = addr;
		if(CR0.l >> 31) {
			addr2 = mmutranslate(addr2, 0);
			if(addr2 == 0xFFFFFFFF)
				return 0xFFFF;
		}
		return d_mem->read_data16(addr2 & a20mask);
	}
	void WM16(uint32 addr, uint16 val) {
		uint32 addr2 = addr;
		if(CR0.l >> 31) {
			addr2 = mmutranslate(addr2, 1);
			if(addr2 == 0xFFFFFFFF)
				return;
		}
		d_mem->write_data16(addr2 & a20mask, val);
	}
	uint32 RM32(uint32 addr) {
		uint32 addr2 = addr;
		if(CR0.l >> 31) {
			addr2 = mmutranslate(addr2, 1);
			if(addr2 == 0xFFFFFFFF)
				return 0xFFFFFFFF;
		}
		return d_mem->read_data32(addr2 & a20mask);
	}
	void WM32(uint32 addr, uint32 val) {
		uint32 addr2 = addr;
		if(CR0.l >> 31) {
			addr2 = mmutranslate(addr2, 1);
			if(addr2 == 0xFFFFFFFF)
				return;
		}
		d_mem->write_data16(addr2 & a20mask, val);
	}
	uint8 RM8(uint32 seg, uint32 addr) {
		if(seg == -1) {
			general_protection_fault(0);
			return -1;
		}
		uint32 addr2 = seg + addr;
		if(CR0.l >> 31) {
			addr2 = mmutranslate(addr, 0);
			if(addr2 == 0xFFFFFFFF)
				return 0xFF;
		}
		return d_mem->read_data8(addr2 & a20mask);
	}
	void WM8(uint32 seg, uint32 addr, uint8 val) {
		if(seg == -1) {
			general_protection_fault(0);
			return;
		}
		uint32 addr2 = seg + addr;
		if(CR0.l >> 31) {
			addr2 = mmutranslate(addr, 1);
			if(addr2 == 0xFFFFFFFF)
				return;
		}
		d_mem->write_data8(addr2 & a20mask, val);
	}
	uint16 RM16(uint32 seg, uint32 addr) {
		if(seg == -1) {
			general_protection_fault(0);
			return -1;
		}
		uint32 addr2 = seg + addr;
		if(CR0.l >> 31) {
			addr2 = mmutranslate(addr2, 0);
			if(addr2 == 0xFFFFFFFF)
				return 0xFFFF;
		}
		return d_mem->read_data16(addr2 & a20mask);
	}
	void WM16(uint32 seg, uint32 addr, uint16 val) {
		if(seg == -1) {
			general_protection_fault(0);
			return;
		}
		uint32 addr2 = seg + addr;
		if(CR0.l >> 31) {
			addr2 = mmutranslate(addr2, 1);
			if(addr2 == 0xFFFFFFFF)
				return;
		}
		d_mem->write_data16(addr2 & a20mask, val);
	}
	uint32 RM32(uint32 seg, uint32 addr) {
		if(seg == -1) {
			general_protection_fault(0);
			return -1;
		}
		uint32 addr2 = seg + addr;
		if(CR0.l >> 31) {
			addr2 = mmutranslate(addr2, 1);
			if(addr2 == 0xFFFFFFFF)
				return 0xFFFFFFFF;
		}
		return d_mem->read_data32(addr2 & a20mask);
	}
	void WM32(uint32 seg, uint32 addr, uint32 val) {
		if(seg == -1) {
			general_protection_fault(0);
			return;
		}
		uint32 addr2 = seg + addr;
		if(CR0.l >> 31) {
			addr2 = mmutranslate(addr2, 1);
			if(addr2 == 0xFFFFFFFF)
				return;
		}
		d_mem->write_data32(addr2 & a20mask, val);
	}
	
	// i/o
	uint8 IN8(uint32 addr) {
		return d_io->read_io8(addr);
	}
	void OUT8(uint32 addr, uint8 val) {
		d_io->write_io8(addr, val);
	}
	uint16 IN16(uint32 addr) {
		return d_io->read_io16(addr);
	}
	void OUT16(uint32 addr, uint16 val) {
		d_io->write_io16(addr, val);
	}
	uint32 IN32(uint32 addr) {
		return d_io->read_io32(addr);
	}
	void OUT32(uint32 addr, uint32 val) {
		d_io->write_io32(addr, val);
	}
	
	// interrupt
	uint32 ACK_INTR() {
		return d_pic->intr_ack();
	}
	
	/* ---------------------------------------------------------------------------
	opecode
	--------------------------------------------------------------------------- */
	
	uint8 fetch8();
	uint16 fetch16();
	uint32 fetch32();
	
	void fetchea32();
	uint8 getea8();
	uint16 getea16();
	void setea8(uint8 val);
	void setea16(uint16 val);
	uint32 getea32();
	void setea32(uint32 val);
	
	void setznp8(uint8 val);
	void setznp16(uint16 val);
	void setznp32(uint32 val);
	void setadd8(uint8 a, uint8 b);
	void setadd8nc(uint8 a, uint8 b);
	void setadc8(uint8 a, uint8 b);
	void setadd16(uint16 a, uint16 b);
	void setadd16nc(uint16 a, uint16 b);
	void setadc16(uint16 a, uint16 b);
	void setadd32(uint32 a, uint32 b);
	void setadd32nc(uint32 a, uint32 b);
	void setadc32(uint32 a, uint32 b);
	void setsub8(uint8 a, uint8 b);
	void setsub8nc(uint8 a, uint8 b);
	void setsbc8(uint8 a, uint8 b);
	void setsub16(uint16 a, uint16 b);
	void setsub16nc(uint16 a, uint16 b);
	void setsbc16(uint16 a, uint16 b);
	void setsub32(uint32 a, uint32 b);
	void setsub32nc(uint32 a, uint32 b);
	void setsbc32(uint32 a, uint32 b);
	
	void softreset();
	void general_protection_fault(uint16 error);
	void not_present_fault();
	void loadseg(uint16 seg, x86seg *s);
	void loadcs(uint16 seg);
	void loadcscall(uint16 seg);
	void interrupt(int num, int soft);
	void pmodeint(int num, int soft);
	void pmodeiret();
	void pmodeiretd();
	void taskswitch386(uint16 seg, uint16 *segdat);
	uint32 mmutranslate2(uint32 addr, int rw);
	
	void invalid();
	void rep(int fv);
	
public:
	I386(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		d_bios = NULL;
		cycles = base_cycles = 0;	// passed_clock must be zero at initialize
		busreq = false;
	}
	~I386() {}
	
	// common functions
	void initialize();
	void reset();
	void run(int clock);
	void write_signal(int id, uint32 data, uint32 mask);
	void set_intr_line(bool line, bool pending, uint32 bit);
	int passed_clock() {
		return base_cycles - cycles;
	}
	uint32 get_prv_pc() {
		return prev_pc;
	}
	
	// unique function
	void set_context_mem(DEVICE* device) {
		d_mem = device;
	}
	void set_context_io(DEVICE* device) {
		d_io = device;
	}
	void set_context_intr(DEVICE* device) {
		d_pic = device;
	}
	void set_context_bios(DEVICE* device) {
		d_bios = device;
	}
};

#endif
