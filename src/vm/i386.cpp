/*
	Skelton for retropc emulator

	Origin : PCem
	Author : Takeda.Toshiya
	Date  : 2009.04.10-

	[ i386 ]
*/

#include "i386.h"

// interrupt vector
#define DIVIDE_FAULT			0
#define NMI_INT_VECTOR			2
#define OVERFLOW_TRAP			4
#define BOUNDS_CHECK_FAULT		5
#define ILLEGAL_INSTRUCTION		6
#define DOUBLE_FAULT			8
#define NOT_PRESENT_FAULT		11
#define GENERAL_PROTECTION_FAULT	13
#define PAGE_FAULT			14
#define ALIGNMENT_CHECK_FAULT		17

#define INT_REQ_BIT	1
#define NMI_REQ_BIT	2

// regs
#define EAX	regs[0].l
#define ECX	regs[1].l
#define EDX	regs[2].l
#define EBX	regs[3].l
#define ESP	regs[4].l
#define EBP	regs[5].l
#define ESI	regs[6].l
#define EDI	regs[7].l
#define AX	regs[0].w
#define CX	regs[1].w
#define DX	regs[2].w
#define BX	regs[3].w
#define SP	regs[4].w
#define BP	regs[5].w
#define SI	regs[6].w
#define DI	regs[7].w
#define AL	regs[0].b.l
#define AH	regs[0].b.h
#define CL	regs[1].b.l
#define CH	regs[1].b.h
#define DL	regs[2].b.l
#define DH	regs[2].b.h
#define BL	regs[3].b.l
#define BH	regs[3].b.h

#define CS	_cs.seg
#define DS	_ds.seg
#define ES	_es.seg
#define SS	_ss.seg
#define FS	_fs.seg
#define GS	_gs.seg
#define cs	_cs.base
#define ds	_ds.base
#define es	_es.base
#define ss	_ss.base
#define fs	_fs.base
#define gs	_gs.base

#define cr0	CR0.l
#define msw	CR0.w

// flags
#define C_FLAG	0x0001
#define P_FLAG	0x0004
#define A_FLAG	0x0010
#define Z_FLAG	0x0040
#define N_FLAG	0x0080
#define I_FLAG	0x0200
#define D_FLAG	0x0400
#define V_FLAG	0x0800
#define NT_FLAG	0x4000
#define VM_FLAG	0x0002

#define CPL	((_cs.access >> 5) & 3)
#define IOPL	((flags >> 12) & 3)
#define IOPLp	((!(msw & 1)) || (CPL <= IOPL))

// optype
#define JMP	1
#define CALL	2
#define IRET	3

// sub

inline uint8 I386::fetch8()
{
	return RM8(cs + (pc++));
}
inline uint16 I386::fetch16()
{
	uint16 val = RM16(cs + pc);
	pc += 2;
	return val;
}
inline uint32 I386::fetch32()
{
	uint32 val = RM32(cs + pc);
	pc += 4;
	return val;
}

void I386::fetchea32()
{
	if(op32 & 0x200) {
		/*rmdat = RM16(cs, pc);*/ pc++;
		reg = (rmdat >> 3) & 7;
		mod = (rmdat >> 6) & 3;
		rm = rmdat & 7;
		if(mod != 3) {
			easeg = ds;
			if(rm == 4) {
				// fetcheal32sib();
				uint8 sib = rmdat >> 8;
				pc++; 
				switch(mod)
				{
				case 0:
					eaaddr = regs[sib & 7].l;
					break;
				case 1:
					eaaddr = ((uint32)(int8)(rmdat >> 16)) + regs[sib & 7].l;
					pc++;
					break;
				case 2:
					eaaddr = fetch32() + regs[sib & 7].l;
					break;
				}
				if((sib & 7) == 5 && !mod)
					eaaddr = fetch32();
				else if((sib & 6) == 4)
					easeg = ss;
				if(((sib >> 3) & 7) != 4)
					eaaddr += regs[(sib >> 3) & 7].l << (sib >> 6);
			}
			else {
				eaaddr = regs[rm].l;
				if(mod) {
					// fetcheal32nosib();
					if(rm == 5)
						easeg = ss;
					if(mod == 1) {
						eaaddr += ((uint32)(int8)(rmdat >> 8));
						pc++;
					}
					else
						eaaddr += fetch32();
				}
				else if(rm == 5)
					eaaddr = fetch32();
			}
		}
	}
	else {
		rmdat = fetch8();
		reg = (rmdat >> 3) & 7;
		mod = rmdat >> 6;
		rm = rmdat & 7;
		if(mod != 3) {
			// fetcheal();
			if(!mod && rm == 6) {
				eaaddr = fetch16();
				easeg = ds;
			}
			else {
				switch(mod)
				{
				case 0:
					eaaddr = 0;
					break;
				case 1:
					eaaddr = (uint16)(int8)fetch8();
					break;
				case 2:
					eaaddr = fetch16();
					break;
				}
				eaaddr += (*mod1add[0][rm]) + (*mod1add[1][rm]);
				easeg = *mod1seg[rm];
				eaaddr &= 0xFFFF;
			}
		}
	}
}

#define fetchea() if(op32 & 0x200) { rmdat = fetchdat >> 8; } fetchea32()
#define fetchea2() if(op32 & 0x200) { rmdat = RM32(cs + pc); } fetchea32()

inline uint8 I386::getea8()
{
	if(mod == 3)
		return (rm & 4) ? regs[rm & 3].b.h : regs[rm & 3].b.l;
	cycles -= 3;
	return RM8(easeg + eaaddr);
}
inline uint16 I386::getea16()
{
	if(mod == 3)
		return regs[rm].w;
	cycles -= 3;
	return RM16(easeg, eaaddr);
}
inline void I386::setea8(uint8 val)
{
	if(mod == 3) {
		if(rm & 4)
			regs[rm & 3].b.h = val;
		else
			regs[rm & 3].b.l = val;
	}
	else {
		cycles -= 2;
		WM8(easeg + eaaddr, val);
	}
}
inline void I386::setea16(uint16 val)
{
	if(mod == 3)
		regs[rm].w = val;
	else {
		cycles -= 2;
		WM16(easeg, eaaddr, val);
	}
}
inline uint32 I386::getea32()
{
	if(mod == 3)
		return regs[rm].l;
	cycles -= 3;
	return RM32(easeg, eaaddr);
}
inline void I386::setea32(uint32 val)
{
	if(mod == 3)
		regs[rm].l = val;
	else {
		cycles -= 2;
		WM32(easeg, eaaddr, val);
	}
}

#define getr8(r) ((r & 4) ? regs[r & 3].b.h : regs[r & 3].b.l)
#define setr8(r, v) { \
	if(r & 4) \
		regs[r & 3].b.h = v; \
	else \
		regs[r & 3].b.l = v; \
}

void I386::setznp8(uint8 val)
{
	flags &= ~0xC4;
	flags |= znptable8[val];
}
void I386::setznp16(uint16 val)
{
	flags &= ~0xC4;
	flags |= znptable16[val];
}
void I386::setznp32(uint32 val)
{
	flags &= ~0xC4;
	flags |= ((val & 0x80000000) ? N_FLAG : ((!val) ? Z_FLAG : 0));
	flags |= znptable8[val & 0xFF] & P_FLAG;
}
void I386::setadd8(uint8 a, uint8 b)
{
	uint16 c = (uint16)a + (uint16)b;
	flags &= ~0x8D5;
	flags |= znptable8[c & 0xFF];
	if(c & 0x100)
		flags |= C_FLAG;
	if(!((a ^ b) & 0x80) && ((a ^ c) & 0x80))
		flags |= V_FLAG;
	if(((a & 0xF) + (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setadd8nc(uint8 a, uint8 b)
{
	uint16 c = (uint16)a + (uint16)b;
	flags &= ~0x8D4;
	flags |= znptable8[c & 0xFF];
	if(!((a ^ b) & 0x80) && ((a ^ c) & 0x80))
		flags |= V_FLAG;
	if(((a & 0xF) + (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setadc8(uint8 a, uint8 b)
{
	uint16 c = (uint16)a + (uint16)b + cflag;
	flags &= ~0x8D5;
	flags |= znptable8[c & 0xFF];
	if(c & 0x100)
		flags |= C_FLAG;
	if(!((a ^ b) & 0x80) && ((a ^ c) & 0x80))
		flags |= V_FLAG;
	if(((a & 0xF) + (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setadd16(uint16 a, uint16 b)
{
	uint32 c = (uint32)a + (uint32)b;
	flags &= ~0x8D5;
	flags |= znptable16[c & 0xFFFF];
	if(c & 0x10000)
		flags |= C_FLAG;
	if(!((a ^ b) & 0x8000) && ((a ^ c) & 0x8000))
		flags |= V_FLAG;
	if(((a & 0xF) + (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setadd16nc(uint16 a, uint16 b)
{
	uint32 c = (uint32)a + (uint32)b;
	flags &= ~0x8D4;
	flags |= znptable16[c & 0xFFFF];
	if(!((a ^ b) & 0x8000) && ((a ^ c) & 0x8000))
		flags |= V_FLAG;
	if(((a & 0xF) + (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setadc16(uint16 a, uint16 b)
{
	uint32 c = (uint32)a + (uint32)b + cflag;
	flags &= ~0x8D5;
	flags |= znptable16[c & 0xFFFF];
	if(c & 0x10000)
		flags |= C_FLAG;
	if(!((a ^ b) & 0x8000) && ((a ^ c) & 0x8000))
		flags |= V_FLAG;
	if(((a & 0xF) + (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setadd32(uint32 a, uint32 b)
{
	uint32 c = (uint32)a + (uint32)b;
	flags &= ~0x8D5;
	flags |= ((c & 0x80000000) ? N_FLAG : ((!c) ? Z_FLAG : 0));
	flags |= (znptable8[c & 0xFF] & P_FLAG);
	if(c < a)
		flags |= C_FLAG;
	if(!((a ^ b) & 0x80000000) && ((a ^ c) & 0x80000000))
		flags |= V_FLAG;
	if(((a & 0xF) + (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setadd32nc(uint32 a, uint32 b)
{
	uint32 c = (uint32)a + (uint32)b;
	flags &= ~0x8D4;
	flags |= ((c & 0x80000000) ? N_FLAG : ((!c) ? Z_FLAG : 0));
	flags |= (znptable8[c & 0xFF] & P_FLAG);
	if(!((a ^ b) & 0x80000000) && ((a ^ c) & 0x80000000))
		flags |= V_FLAG;
	if(((a & 0xF) + (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setadc32(uint32 a, uint32 b)
{
	uint32 c = (uint32)a + (uint32)b + cflag;
	flags &= ~0x8D5;
	flags |= ((c & 0x80000000) ? N_FLAG : ((!c) ? Z_FLAG : 0));
	flags |= (znptable8[c & 0xFF] & P_FLAG);
	if((c < a) || (c == a && cflag))
		flags |= C_FLAG;
	if(!((a ^ b) & 0x80000000) && ((a ^ c) & 0x80000000))
		flags |= V_FLAG;
	if(((a & 0xF) + (b & 0xF) + cflag) & 0x10)
		flags |= A_FLAG;
}
void I386::setsub8(uint8 a, uint8 b)
{
	uint16 c = (uint16)a - (uint16)b;
	flags &= ~0x8D5;
	flags |= znptable8[c & 0xFF];
	if(c & 0x100)
		flags |= C_FLAG;
	if((a ^ b) & (a ^ c) & 0x80)
		flags |= V_FLAG;
	if(((a & 0xF) - (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setsub8nc(uint8 a, uint8 b)
{
	uint16 c = (uint16)a - (uint16)b;
	flags &= ~0x8D4;
	flags |= znptable8[c & 0xFF];
	if((a ^ b) & (a ^ c) & 0x80)
		flags |= V_FLAG;
	if(((a & 0xF) - (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setsbc8(uint8 a, uint8 b)
{
	uint16 c = (uint16)a - (((uint16)b) + cflag);
	flags &= ~0x8D5;
	flags |= znptable8[c & 0xFF];
	if(c & 0x100)
		flags |= C_FLAG;
	if((a ^ b) & (a ^ c) & 0x80)
		flags |= V_FLAG;
	if(((a & 0xF) - (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setsub16(uint16 a, uint16 b)
{
	uint32 c = (uint32)a - (uint32)b;
	flags &= ~0x8D5;
	flags |= znptable16[c & 0xFFFF];
	if(c & 0x10000) flags |= C_FLAG;
	if((a ^ b) & (a ^ c) & 0x8000)
		flags |= V_FLAG;
	if(((a & 0xF) - (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setsub16nc(uint16 a, uint16 b)
{
	uint32 c = (uint32)a - (uint32)b;
	flags &= ~0x8D4;
	flags |= (znptable16[c & 0xFFFF] & ~4);
	flags |= (znptable8[c & 0xFF] & 4);
	if((a ^ b) & (a ^ c) & 0x8000)
		flags |= V_FLAG;
	if(((a & 0xF) - (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setsbc16(uint16 a, uint16 b)
{
	uint32 c = (uint32)a - (((uint32)b) + cflag);
	flags &= ~0x8D5;
	flags |= (znptable16[c & 0xFFFF] & ~4);
	flags |= (znptable8[c & 0xFF] & 4);
	if(c & 0x10000)
		flags |= C_FLAG;
	if((a ^ b) & (a ^ c) & 0x8000)
		flags |= V_FLAG;
	if(((a & 0xF) - (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setsub32(uint32 a, uint32 b)
{
	uint32 c = (uint32)a - (uint32)b;
	flags &= ~0x8D5;
	flags |= ((c & 0x80000000) ? N_FLAG : ((!c) ? Z_FLAG : 0));
	flags |= (znptable8[c & 0xFF] & P_FLAG);
	if(c > a)
		flags |= C_FLAG;
	if((a ^ b) & (a ^ c) & 0x80000000)
		flags |= V_FLAG;
	if(((a & 0xF) - (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setsub32nc(uint32 a, uint32 b)
{
	uint32 c = (uint32)a - (uint32)b;
	flags &= ~0x8D4;
	flags |= ((c & 0x80000000) ? N_FLAG : ((!c) ? Z_FLAG : 0));
	flags |= (znptable8[c & 0xFF] & P_FLAG);
	if((a ^ b) & (a ^ c) & 0x80000000)
		flags |= V_FLAG;
	if(((a & 0xF) - (b & 0xF)) & 0x10)
		flags |= A_FLAG;
}
void I386::setsbc32(uint32 a, uint32 b)
{
	uint32 c = (uint32)a - (((uint32)b) + cflag);
	flags &= ~0x8D5;
	flags |= ((c & 0x80000000) ? N_FLAG : ((!c) ? Z_FLAG : 0));
	flags |= (znptable8[c & 0xFF] & P_FLAG);
	if((c > a) || (c == a && cflag))
		flags |= C_FLAG;
	if((a ^ b) & (a ^ c) & 0x80000000)
		flags |= V_FLAG;
	if(((a & 0xF) - ((b & 0xF) + cflag)) & 0x10)
		flags |= A_FLAG;
}

// main

void I386::initialize()
{
	// init mod1 table
	zero = 0;
	mod1add[0][0] = &BX;
	mod1add[0][1] = &BX;
	mod1add[0][2] = &BP;
	mod1add[0][3] = &BP;
	mod1add[0][4] = &SI;
	mod1add[0][5] = &DI;
	mod1add[0][6] = &BP;
	mod1add[0][7] = &BX;
	mod1add[1][0] = &SI;
	mod1add[1][1] = &DI;
	mod1add[1][2] = &SI;
	mod1add[1][3] = &DI;
	mod1add[1][4] = &zero;
	mod1add[1][5] = &zero;
	mod1add[1][6] = &zero;
	mod1add[1][7] = &zero;
	mod1seg[0] = &ds;
	mod1seg[1] = &ds;
	mod1seg[2] = &ss;
	mod1seg[3] = &ss;
	mod1seg[4] = &ds;
	mod1seg[5] = &ds;
	mod1seg[6] = &ss;
	mod1seg[7] = &ds;
	
	// init znptable
	for(int c = 0; c < 256; c++) {
		int d = 0;
		if(c & 0x01) d++;
		if(c & 0x02) d++;
		if(c & 0x04) d++;
		if(c & 0x08) d++;
		if(c & 0x10) d++;
		if(c & 0x20) d++;
		if(c & 0x40) d++;
		if(c & 0x80) d++;
		if(d & 1)
			znptable8[c] = 0;
		else
			znptable8[c] = P_FLAG;
		if(!c)
			znptable8[c] |= Z_FLAG;
		if(c & 0x80)
			znptable8[c] |= N_FLAG;
	}
	for(int c = 0; c < 65536; c++) {
		int d = 0;
		if(c & 0x01) d++;
		if(c & 0x02) d++;
		if(c & 0x04) d++;
		if(c & 0x08) d++;
		if(c & 0x10) d++;
		if(c & 0x20) d++;
		if(c & 0x40) d++;
		if(c & 0x80) d++;
		if(d & 1)
			znptable16[c] = 0;
		else
			znptable16[c] = P_FLAG;
		if(!c)
			znptable16[c] |= Z_FLAG;
		if(c & 0x8000)
			znptable16[c] |= N_FLAG;
	}
}

void I386::reset()
{
	cycles = base_cycles = 0;
	ssegs = abrt = notpresent = 0;
	softreset();
	
	// init mmucache
	memset(mmucache, 0xFF, sizeof(mmucache));
	for(int c = 0; c < 64; c++)
		mmucaches[c] = 0xFFFFFFFF;
	mmunext = 0;
}

void I386::softreset()
{
	_memset(regs, 0, sizeof(regs));
	CS = DS = ES = SS = FS = GS = 0;
	cs = ds = es = ss = fs = gs = 0;
	use32 = stack32 = 0;
	intstat = 0;
	pc = 0;
	a20mask = ~0;
	msw = 0xfff0;
	flags = 2;
	eflags = 0;
	loadcs(0xFFFF);
}

void I386::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_CPU_NMI) {
		if(data & mask)
			intstat |= NMI_REQ_BIT;
		else
			intstat &= ~NMI_REQ_BIT;
	}
	else if(id == SIG_CPU_BUSREQ) {
		busreq = ((data & mask) != 0);
		if(busreq)
			cycles = base_cycles = 0;
	}
	else if(id == SIG_I386_A20) {
		if(data & mask)
			a20mask = ~0;
		else
			a20mask = ~(1 << 20);
	}
}

void I386::set_intr_line(bool line, bool pending, uint32 bit)
{
	if(line)
		intstat |= INT_REQ_BIT;
	else
		intstat &= ~INT_REQ_BIT;
}

void I386::general_protection_fault(uint16 error)
{
	CS = oldcs;
	pc = oldpc;
	_cs.access = oldcpl << 5;
	pmodeint(GENERAL_PROTECTION_FAULT, 0);
	if(intgatesize == 16) {
		if(stack32) {
			WM16(ss, ESP - 2, error);
			ESP -= 2;
		}
		else {
			WM16(ss, ((SP - 2) & 0xFFFF), error);
			SP -= 2;
		}
	}
	else {
		if(stack32) {
			WM32(ss, ESP - 4, error);
			ESP -= 4;
		}
		else {
			WM32(ss, ((SP - 4) & 0xFFFF), error);
			SP -= 4;
		}
	}
}

void I386::not_present_fault()
{
	pmodeint(NOT_PRESENT_FAULT, 0);
	if(stack32) {
		WM16(ss, ESP - 2, notpresent_error);
		ESP -= 2;
	}
	else {
		WM16(ss, ((SP - 2) & 0xFFFF), notpresent_error);
		SP -= 2;
	}
}

void I386::loadseg(uint16 seg, x86seg *s)
{
	if((msw & 1) && !(eflags & VM_FLAG)) {
		if(!(seg & ~3)) {
			if(s == &_ss) {
				general_protection_fault(seg & 0xFFFC);
				return;
			}
			s->seg = 0;
//			s->base = 0;	// Sim City ???
			s->base = -1;
			return;
		}
		uint32 addr = seg & ~7;
		if(seg & 4) {
			if(addr >= ldt.limit) {
				general_protection_fault(seg & 0xFFFC);
				return;
			}
			addr += ldt.base;
		}
		else {
			if(addr >= gdt.limit) {
				general_protection_fault(seg & 0xFFFC);
				return;
			}
			addr += gdt.base;
		}
		uint16 segdat[4];
		segdat[0] = RM16(addr + 0);
		segdat[1] = RM16(addr + 2);
		segdat[2] = RM16(addr + 4);
		segdat[3] = RM16(addr + 6);
		if(s == &_ss) {
			if(segdat[3] & 0x40)
				stack32 = 1;
			else
				stack32 = 0;
		}
		if((s == &_cs)) {
			if(segdat[3] & 0x40)
				use32 = 0x300;
			else
				use32 = 0;
		}
		if(!(segdat[2] & 0x800) || !(segdat[2] & 0x400)) {
			if(!(segdat[2] & 0x8000)) {
				notpresent = 1;
				notpresent_error = seg & 0xFFFC;
				return;
			}
			s->seg = seg;
			s->limit = segdat[0];
			s->base = segdat[1];
			s->base |= ((segdat[2] & 0xFF) << 16);
			s->base |= ((segdat[3] >> 8) << 24);
			s->access = segdat[2] >> 8;
		}
	}
	else {
		s->base = seg << 4;
		s->limit = 0xFFFF;
		s->seg = seg;
		if(eflags & VM_FLAG)
			s->access = 3 << 5;
		else
			s->access = 0 << 5;
		use32 = 0;
		if(s == &_ss)
			stack32 = 0;
	}
}

void I386::loadcs(uint16 seg)
{
	if((msw & 1) && !(eflags & VM_FLAG)) {
		if(!(seg & ~3)) {
			general_protection_fault(seg & 0xFFFC);
			return;
		}
		uint32 addr = seg & ~7;
		if(seg & 4) {
			if(addr >= ldt.limit) {
				general_protection_fault(seg & 0xFFFC);
				return;
			}
			addr += ldt.base;
		}
		else {
			if(addr >= gdt.limit) {
				general_protection_fault(seg & 0xFFFC);
				return;
			}
			addr += gdt.base;
		}
		uint16 segdat[4];
		segdat[0] = RM16(addr + 0);
		segdat[1] = RM16(addr + 2);
		segdat[2] = RM16(addr + 4);
		segdat[3] = RM16(addr + 6);
		if(segdat[2] & 0x1000) {
			if(segdat[3] & 0x40)
				use32 = 0x300;
			else
				use32 = 0;
			CS = seg;
			_cs.limit = segdat[0];
			_cs.base = segdat[1];
			_cs.base |= ((segdat[2] & 0xFF) << 16);
			_cs.base |= ((segdat[3] >> 8) << 24);
			_cs.access = segdat[2] >> 8;
			if(!(segdat[2] & 0x8000)) {
				notpresent = 1;
				notpresent_error = seg & 0xFFFC;
				return;
			}
		}
		else {
			if(!(segdat[2] & 0x8000)) {
				notpresent = 1;
				notpresent_error = seg & 0xFFFC;
				return;
			}
			switch(segdat[2] & 0xF00)
			{
			case 0x400: /*Call gate*/
				if(segdat[2] & 31) {
					general_protection_fault(seg & 0xFFFC);
					return;
				}
				CS &= ~3;
				loadseg(segdat[1], &_cs);
				pc = segdat[0];
				return;
			case 0x900: /*386 Task gate*/
				taskswitch386(seg, segdat);
				return;
			default:
				general_protection_fault(seg & 0xFFFC);
				return;
			}
		}
	}
	else {
		_cs.base = seg << 4;
		_cs.limit = 0xFFFF;
		CS = seg;
		if(eflags & VM_FLAG)
			_cs.access = 3 << 5;
		else
			_cs.access = 0 << 5;
	}
}

void I386::loadcscall(uint16 seg)
{
	if((msw & 1) && !(eflags & VM_FLAG)) {
		uint16 oldcs = CPL;
		if(!(seg & ~3)) {
			general_protection_fault(seg & 0xFFFC);
			return;
		}
		uint32 addr = seg & ~7;
		if(seg & 4) {
			if(addr >= ldt.limit) {
				general_protection_fault(seg & 0xFFFC);
				return;
			}
			addr += ldt.base;
		}
		else {
			if(addr >= gdt.limit) {
				general_protection_fault(seg & 0xFFFC);
				return;
			}
			addr += gdt.base;
		}
		uint16 segdat[4];
		segdat[0] = RM16(addr + 0);
		segdat[1] = RM16(addr + 2);
		segdat[2] = RM16(addr + 4);
		segdat[3] = RM16(addr + 6);
		if(segdat[2] & 0x1000) {
			if(segdat[3] & 0x40)
				use32 = 0x300;
			else
				use32 = 0;
			CS = seg;
			_cs.limit = segdat[0];
			_cs.base = segdat[1];
			_cs.base |= ((segdat[2] & 0xFF) << 16);
			_cs.base |= ((segdat[3] >> 8) << 24);
			_cs.access = segdat[2] >> 8;
			if(!(segdat[2] & 0x8000)) {
				notpresent = 1;
				notpresent_error = seg & 0xFFFC;
				return;
			}
		}
		else {
			switch(segdat[2] & 0xF00)
			{
			case 0x400: /*Call gate*/
				if(!(segdat[2] & 0x8000)) {
					notpresent = 1;
					notpresent_error = seg & 0xFFFC;
					return;
				}
				CS &= ~3;
				loadseg(segdat[1], &_cs);
				if(CPL < oldcs) {
					if(stack32) {
						general_protection_fault(seg & 0xFFFC);
						return;
					}
					if(segdat[2] & 31) {
						general_protection_fault(seg & 0xFFFC);
						return;
					}
					uint16 oldss = SS;
					uint16 oldsp = SP;
					if(tr.access & 8) {
						addr = 4 + tr.base;
						loadseg(RM16(addr + 4), &_ss);
						ESP = RM32(addr);
					}
					else {
						addr = 2 + tr.base;
						loadseg(RM16(addr + 2), &_ss);
						SP = RM16(addr);
					}
					WM16(ss, ((SP - 2) & 0xFFFF), oldss);
					WM16(ss, ((SP - 4) & 0xFFFF), oldsp);
					SP -= 4;
				}
				pc = segdat[0];
				break;
			default:
				general_protection_fault(seg & 0xFFFC);
				return;
			}
		}
	}
	else {
		_cs.base = seg << 4;
		_cs.limit = 0xFFFF;
		CS = seg;
		if(eflags & VM_FLAG)
			_cs.access = 3 << 5;
		else
			_cs.access = 0 << 5;
	}
}

void I386::interrupt(int num, int soft)
{
	if(msw & 1) {
		pmodeint(num, soft);
		return;
	}
#if 1
	cycles -= 33;
	if(ssegs)
		ss = oldss;
	if(stack32) {
		WM16(ss, ESP - 2, flags);
		WM16(ss, ESP - 4, CS);
		WM16(ss, ESP - 6, pc);
		ESP -= 6;
	}
	else {
		WM16(ss, ((SP - 2) & 0xFFFF), flags);
		WM16(ss, ((SP - 4) & 0xFFFF), CS);
		WM16(ss, ((SP - 6) & 0xFFFF), pc);
		SP -= 6;
	}
#else
	WM16(ss, ((SP - 2) & 0xFFFF), flags);
	WM16(ss, ((SP - 4) & 0xFFFF), CS);
	WM16(ss, ((SP - 6) & 0xFFFF), pc);
	SP -= 6;
#endif
	flags &= ~I_FLAG;
	oxpc = pc;
	uint32 addr = num << 2;
	pc = RM16(addr);
	loadcs(RM16(addr + 2));
}

void I386::pmodeint(int num, int soft)
{
	uint16 segdat[4], temp[5];
	uint16 oldcs = CPL;
	uint16 oldss, oldsp;
	int v86int = eflags & VM_FLAG;
	
	cycles -= 59;
	if((eflags & VM_FLAG) && IOPL != 3) {
		general_protection_fault((num * 8) + 2);
		return;
	}
	uint32 addr = (num << 3);
	if(addr >= idt.limit) {
		if(num == 8)
			softreset();
		else if(num == 0xD)
			pmodeint(DOUBLE_FAULT, 0);
		else
			general_protection_fault((num * 8) + 2 + (soft) ? 0 : 1);
		return;
	}
	addr += idt.base;
	segdat[0] = RM16(addr + 0);
	segdat[1] = RM16(addr + 2);
	segdat[2] = RM16(addr + 4);
	segdat[3] = RM16(addr + 6);
	int dpl = (segdat[2] >> 13) & 3;
	if(!(segdat[2] & 0x1F00)) {
		if(num == 0xD) {
			pmodeint(DOUBLE_FAULT, 0);
			return;
		}
		if(num == 0x8) {
			softreset();
			return;
		}
		general_protection_fault((num * 8) + 2);
		return;
	}
	if(dpl < CPL && soft) {
		general_protection_fault((num * 8) + 2);
		return;
	}
	if(eflags & VM_FLAG) {
		uint8 oldaccess = _cs.access;
		_cs.access = 4;
		uint16 oldcs2 = CS;
		CS = 0;
		oldss = SS; oldsp = SP;
		eflags &= ~VM_FLAG;
		if(tr.access & 8) {
			addr = 4 + tr.base;
			loadseg(RM16(addr + 4), &_ss);
			ESP = RM32(addr);
		}
		else {
			addr = 2 + tr.base;
			loadseg(RM16(addr + 2), &_ss);
			SP = RM16(addr);
		}
		if(stack32) {
			WM32(ss, ESP - 4, GS);
			WM32(ss, ESP - 8, FS);
			WM32(ss, ESP - 12, DS);
			WM32(ss, ESP - 16, ES);
			ESP -= 16;
		}
		else {
			WM32(ss, (SP - 4) & 0xFFFF, GS);  
			WM32(ss, (SP - 8) & 0xFFFF, FS);  
			WM32(ss, (SP - 12) & 0xFFFF, DS); 
			WM32(ss, (SP - 16) & 0xFFFF, ES); 
			SP -= 16;
		}
		loadseg(0, &_ds);
		loadseg(0, &_es);
		loadseg(0, &_fs);
		loadseg(0, &_gs);
		if(stack32) {
			WM32(ss, ESP - 4, oldss);
			WM32(ss, ESP - 8, oldsp);
			ESP -= 8;
		}
		else {
			WM32(ss, (SP - 4) & 0xFFFF, oldss); 
			WM32(ss, (SP - 8) & 0xFFFF, oldsp); 
			SP -= 8;
		}
		eflags |= VM_FLAG;
		CS = oldcs2;
		_cs.access = oldaccess;
		oldcs = 0;
	}
	switch(segdat[2] & 0xF00)
	{
	case 0x600: /*Interrupt gate*/
		temp[0] = flags;
		temp[1] = CS;
		temp[2] = pc;
		flags &= ~I_FLAG;
		eflags &= ~VM_FLAG;
		CS = 0;
		loadseg(segdat[1], &_cs);
		pc = segdat[0];
		if(!(_cs.access & 4) && (CPL < oldcs)) {
			if(stack32) {
				general_protection_fault((num * 8) + 2);
				return;
			}
			oldss = SS;
			oldsp = SP;
			if(tr.access & 8) {
				addr = 4 + tr.base;
				loadseg(RM16(addr + 4), &_ss);
				ESP = RM32(addr);
			}
			else {
				addr = 2 + tr.base;
				loadseg(RM16(addr + 2), &_ss);
				SP = RM16(addr);
			}
			WM16(ss, ((SP - 2) & 0xFFFF), oldss);
			WM16(ss, ((SP - 4) & 0xFFFF), oldsp);
			SP -= 4;
		}
		if(stack32) {
			WM16(ss, ESP - 2, temp[0]);
			WM16(ss, ESP - 4, temp[1]);
			WM16(ss, ESP - 6, temp[2]);
			ESP -= 6;
		}
		else {
			WM16(ss, ((SP - 2) & 0xFFFF), temp[0]);
			WM16(ss, ((SP - 4) & 0xFFFF), temp[1]);
			WM16(ss, ((SP - 6) & 0xFFFF), temp[2]);
			SP -= 6;
		}
		intgatesize = 16;
		return;
	case 0xE00: /*386 Interrupt gate*/
	case 0xF00: /*386 Trap gate*/
		temp[0] = eflags;
		temp[1] = flags;
		temp[2] = CS;
		temp[3] = pc >> 16;
		temp[4] = pc;
		if((segdat[2] & 0xF00) == 0xE00)
			flags &= ~I_FLAG;
		eflags &= ~VM_FLAG;
		CS = 0;
		loadseg(segdat[1], &_cs);
		pc = segdat[0] | (segdat[3] << 16);
		if(!(_cs.access & 4) && (CPL < oldcs) && !v86int) {
//			exit(-1);
			oldss = SS;
			oldsp = SP;
			addr = 2 + ((CS & 3) << 2) + tr.base;
			loadseg(RM16(addr + 2), &_ss);
			SP = RM16(addr);
			WM16(ss, ((SP - 2) & 0xFFFF), oldss);
			WM16(ss, ((SP - 4) & 0xFFFF), oldsp);
			SP -= 4;
		}
		if(stack32) {
			WM16(ss, ESP - 2, temp[0]); 
			WM16(ss, ESP - 4, temp[1]); 
			WM16(ss, ESP - 6, 0);       
			WM16(ss, ESP - 8, temp[2]); 
			WM16(ss, ESP - 10, temp[3]);
			WM16(ss, ESP - 12, temp[4]);
			ESP -= 12;
		}
		else {
			WM16(ss, ((SP - 2) & 0xFFFF), temp[0]);
			WM16(ss, ((SP - 4) & 0xFFFF), temp[1]);
			WM16(ss, ((SP - 6) & 0xFFFF), 0);
			WM16(ss, ((SP - 8) & 0xFFFF), temp[2]);
			WM16(ss, ((SP - 10) & 0xFFFF), temp[3]);
			WM16(ss, ((SP - 12) & 0xFFFF), temp[4]);
			SP -= 12;
		}
		intgatesize = 32;
		return;
//	case 0x000: /*Invalid*/
	default:
		general_protection_fault((num * 8) + 2);
		return;
	}
}

void I386::pmodeiret()
{
	uint16 oldcs = CPL;
	uint16 tempw, tempw2;
	
	if(eflags & VM_FLAG) {
		if(IOPL != 3) {
			general_protection_fault(0);
			return;
		}
		oxpc = pc;
		if(stack32) {
			pc = RM16(ss, ESP);
			tempw2 = RM16(ss, ESP + 2);
			tempw = RM16(ss, ESP + 4);
			flags = (flags & 0x3000) | (tempw & 0xCFFF);
			ESP += 6;
		}
		else {
			pc = RM16(ss, SP);
			tempw2 = RM16(ss, (SP + 2) & 0xFFFF);
			tempw = RM16(ss, ((SP + 4) & 0xFFFF));
			flags = (flags & 0x3000) | (tempw & 0xCFFF);
			SP += 6;
		}
		loadcs(tempw2);
		return;
	}
	oxpc = pc;
	if(stack32) {
		pc = RM16(ss, ESP);
		tempw = RM16(ss, ESP + 2);
		flags = RM16(ss, ESP + 4);
		flags &= 0xFFF;
		ESP += 6;
	}
	else {
		pc = RM16(ss, SP);
		tempw = RM16(ss, ((SP + 2) & 0xFFFF));
		flags = RM16(ss, ((SP + 4) & 0xFFFF));
		flags &= 0xFFF;
		SP += 6;
	}
	loadcs(tempw);
	if(CPL > oldcs) {
		/*Return to outer level*/
		if(stack32) {
			general_protection_fault(0);
			return;
		}
		uint16 nsp = RM16(ss, SP);
		uint16 nss = RM16(ss, ((SP + 2) & 0xFFFF));
		loadseg(nss, &_ss);
		SP = nsp;
		if(CPL > ((_ds.access >> 5) & 3)) {
			_ds.seg = 0;
			_ds.base = -1;
		}
		if(CPL > ((_es.access >> 5) & 3)) {
			_es.seg = 0;
			_es.base = -1;
		}
	}
}
void I386::pmodeiretd()
{
	uint16 oldcs = CPL;
	uint16 tempw;
	
	if(eflags & VM_FLAG) {
		if(IOPL != 3) {
			general_protection_fault(0);
			return;
		}
//		exit(-1);
	}
	oxpc = pc;
	if(stack32) {
		pc = RM32(ss, ESP);
		tempw = RM16(ss, ESP + 4);
		flags = RM16(ss, ESP + 8);
		eflags = RM16(ss, ESP + 10) & 3;
		ESP += 12;
	}
	else {
		pc = RM32(ss, SP);
		tempw = RM16(ss, ((SP + 4) & 0xFFFF));
		flags = RM16(ss, ((SP + 8) & 0xFFFF));
		eflags = RM16(ss, ((SP + 10) & 0xFFFF)) & 3;
		SP += 12;
	}
	loadcs(tempw);
	if(eflags & VM_FLAG) {
		if(stack32) {
			uint32 nesp = RM32(ss, ESP);
			uint16 nss = RM32(ss, ESP + 4);
			loadseg(RM16(ss, ESP + 8), &_es);
			loadseg(RM16(ss, ESP + 12), &_ds);
			loadseg(RM16(ss, ESP + 16), &_fs);
			loadseg(RM16(ss, ESP + 20), &_gs);
			ESP = nesp;
			loadseg(nss, &_ss);
		}
		else {
			uint32 nesp = RM32(ss, SP);
			uint16 nss = RM32(ss, (SP + 4) & 0xFFFF);
			loadseg(RM16(ss, (SP + 8) & 0xFFFF), &_es);
			loadseg(RM16(ss, (SP + 12) & 0xFFFF), &_ds); 
			loadseg(RM16(ss, (SP + 16) & 0xFFFF), &_fs);
			loadseg(RM16(ss, (SP + 20) & 0xFFFF), &_gs);
			SP = nesp;
			loadseg(nss, &_ss);
		}
		use32 = 0;
		return;
	}
	if(CPL > oldcs) {
		general_protection_fault(0);
		return;
		// return to outer level
		uint16 nsp = RM16(ss, SP);
		uint16 nss = RM16(ss, ((SP + 2) & 0xFFFF));
		loadseg(nss, &_ss);
		SP = nsp;
		if(CPL > ((_ds.access >> 5) & 3)) {
			_ds.seg = 0;
			_ds.base = -1;
		}
		if(CPL > ((_es.access >> 5) & 3)) {
			_es.seg = 0;
			_es.base = -1;
		}
	}
}

void I386::taskswitch386(uint16 seg, uint16 *segdat)
{
	uint32 new_cr3 = 0;
	uint32 new_eax, new_ebx, new_ecx, new_edx, new_esp, new_ebp, new_esi, new_edi;
	uint32 new_es, new_cs, new_ss, new_ds, new_fs, new_gs;
	uint32 new_ldt, new_eip, new_eflags;
	
	uint32 base = segdat[1] | ((segdat[2] & 0xFF) << 16) | ((segdat[3] >> 8) << 24);
	uint32 limit = segdat[0] | ((segdat[3] & 0xF) << 16);
	uint16 x386 = segdat[2] & 0x800;
	
	if(x386) {
		new_cr3 = RM32(base, 0x1C);
		new_eip = RM32(base, 0x20);
		new_eflags = RM32(base, 0x24);
		new_eax = RM32(base, 0x28);
		new_ecx = RM32(base, 0x2C);
		new_edx = RM32(base, 0x30);
		new_ebx = RM32(base, 0x34);
		new_esp = RM32(base, 0x38);
		new_ebp = RM32(base, 0x3C);
		new_esi = RM32(base, 0x40);
		new_edi = RM32(base, 0x44);

		new_es = RM16(base, 0x48);
		new_cs = RM16(base, 0x4C);
		new_ss = RM16(base, 0x50);
		new_ds = RM16(base, 0x54);
		new_fs = RM16(base, 0x58);
		new_gs = RM16(base, 0x5C);
		new_ldt = RM16(base, 0x60);
	}
	else {
		// tss fault???
//		exit(-1);
	}
	if(optype == JMP || optype == IRET) {
		tr.access &= ~2; /*Clear busy*/
		WM8((tr.seg & ~7) + gdt.base + 5, tr.access);
	}
	uint32 oldeflags = flags | (eflags << 16);
	if(optype == IRET)
		oldeflags &= ~NT_FLAG;
	if(x386) {
		WM32(tr.base, 0x1C, cr3);
		WM32(tr.base, 0x20, oxpc);
		WM32(tr.base, 0x24, oldeflags);
		WM32(tr.base, 0x28, EAX);
		WM32(tr.base, 0x2C, ECX);
		WM32(tr.base, 0x30, EDX);
		WM32(tr.base, 0x34, EBX);
		WM32(tr.base, 0x38, ESP);
		WM32(tr.base, 0x3C, EBP);
		WM32(tr.base, 0x40, ESI);
		WM32(tr.base, 0x44, EDI);
		WM32(tr.base, 0x48, ES);
		WM32(tr.base, 0x4C, CS);
		WM32(tr.base, 0x50, SS);
		WM32(tr.base, 0x54, DS);
		WM32(tr.base, 0x58, FS);
		WM32(tr.base, 0x5C, GS);
		WM32(tr.base, 0x60, ldt.seg);
	}
	if(optype == CALL)
		WM32(base, 0, tr.seg);
	if(optype == JMP || optype == CALL) {
		segdat[2] |= 0x200;
		WM16(gdt.base, (seg & ~7) + 4, segdat[2]);
	}
	if((seg & ~7) == (tr.seg & ~7)) {
		new_eip = pc;
		new_cs = CS;
		new_ds = DS;
		new_es = ES;
		new_fs = FS;
		new_gs = GS;
		new_ss = SS;
	}
	else {
		cr3 = new_cr3 & ~0xFFF;
		pc = new_eip;
		flags = new_eflags & 0xFFFF;
		eflags = new_eflags >> 16;
		EAX = new_eax;
		EBX = new_ebx;
		ECX = new_ecx;
		EDX = new_edx;
		EBP = new_ebp;
		ESP = new_esp;
		ESI = new_esi;
		EDI = new_edi;
		if(new_ldt & 7) {
			uint32 templ = (ldt.seg & ~7) + gdt.base;
			ldt.limit = RM16(templ);
			ldt.base = (RM16(templ + 2)) | (RM8(templ + 4) << 16) | (RM8(templ + 7) << 24);
		}
	}
	loadseg(new_cs, &_cs);
	loadseg(new_ds, &_ds);
	loadseg(new_es, &_es);
	loadseg(new_fs, &_fs);
	loadseg(new_gs, &_gs);
	loadseg(new_ss, &_ss);
	
	tr.seg = seg;
	tr.base = base;
	tr.limit = limit;
	tr.access = segdat[2] >> 8;
}

uint32 I386::mmutranslate2(uint32 addr, int rw)
{
	if(mmucache[addr >> 12] != 0xFFFFFFFF)
		return mmucache[addr >> 12] + (addr & 0xFFF);
	uint32 temp = RM32(cr3 + ((addr >> 20) & 0xFFC));
	uint32 temp2 = temp;
	if(!(temp & 1)) {
		cr2 = addr;
		temp = 0;
		if(CS & 3)
			temp |= 4;
		if(rw)
			temp |= 2;
		abrt = 1 | (temp << 8);
		return -1;
	}
	temp = RM32((temp & ~0xFFF) + ((addr >> 10) & 0xFFC));
	if(!(temp & 1)) {
		cr2 = addr;
		temp = 0;
		if(CS & 3)
			temp |= 4;
		if(rw)
			temp |= 2;
		abrt = 1 | (temp << 8);
		return -1;
	}
	if(mmucaches[mmunext] != 0xFFFFFFFF)
		mmucache[mmucaches[mmunext]] = 0xFFFFFFFF;
	mmucache[addr >> 12] = temp & ~0xFFF;
	mmucaches[mmunext++] = addr >> 12;
	mmunext &= 63;
	uint32 addr2 = (temp2 & ~0xFFF) + ((addr >> 10) & 0xFFC);
	WM32(addr2, RM32(addr2) | 0x60);
	return (temp & ~0xFFF) + (addr & 0xFFF);
}

void I386::invalid()
{
	pc = oldpc;
	interrupt(ILLEGAL_INSTRUCTION, 0);
}

void I386::rep(int fv)
{
	uint8 temp2;
	uint16 tempw, tempw2;
	uint32 templ, templ2;
	
	uint32 ipc = oldpc;
	int changeds = 0;
	uint32 oldds;
	uint16 rep32 = op32;
startrep:
	uint8 temp = fetch8();
	uint32 c = (rep32 & 0x200) ? ECX : CX;
	
	switch(temp | rep32)
	{
	case 0x08:
		pc = ipc + 1;
		break;
	case 0x26: case 0x126: case 0x226: case 0x326: /*ES:*/
		oldds = ds;
		ds = es;
		changeds = 1;
		goto startrep;
		break;
	case 0x2E: case 0x12E: case 0x22E: case 0x32E: /*CS:*/
		oldds = ds;
		ds = cs;
		changeds = 1;
		goto startrep;
		break;
	case 0x36: case 0x136: case 0x236: case 0x336: /*SS:*/
		oldds = ds;
		ds = ss;
		changeds = 1;
		goto startrep;
		break;
	case 0x3E: case 0x13E: case 0x23E: case 0x33E: /*DS:*/
		oldds = ds;
		ds = ds;
		changeds = 1;
		goto startrep;
		break;
	case 0x64: case 0x164: case 0x264: case 0x364: /*FS:*/
		oldds = ds;
		ds = fs;
		changeds = 1;
		goto startrep;
	case 0x65: case 0x165: case 0x265: case 0x365: /*GS:*/
		oldds = ds;
		ds = gs;
		changeds = 1;
		goto startrep;
	case 0x66: case 0x166: case 0x266: case 0x366: /*Data size prefix*/
		rep32 ^=0x100;
		goto startrep;
	case 0x67: case 0x167: case 0x267: case 0x367:  /*Address size prefix*/
		rep32 ^=0x200;
		goto startrep;
	case 0x6C: case 0x16C: /*REP INSB*/
		if(c > 0) {
			temp2 = IN8(DX);
			WM8(ds, DI, temp2);
			if(flags & D_FLAG)
				DI--;
			else
				DI++;
			c--;
			cycles -= 15;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x26C: case 0x36C: /*REP INSB*/
		if(c > 0) {
			temp2 = IN8(DX);
			WM8(ds, EDI, temp2);
			if(flags & D_FLAG)
				EDI--;
			else
				EDI++;
			c--;
			cycles -= 15;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x6E: case 0x16E: /*REP OUTSB*/
		if(c > 0) {
			temp2 = RM8(ds, SI);
			OUT8(DX, temp2);
			if(flags & D_FLAG) SI--;
			else		   SI++;
			c--;
			cycles -= 14;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x26E: case 0x36E: /*REP OUTSB*/
		if(c > 0) {
			temp2 = RM8(ds, ESI);
			OUT8(DX, temp2);
			if(flags & D_FLAG) ESI--;
			else		   ESI++;
			c--;
			cycles -= 14;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x26F: /*REP OUTSW*/
		if(c > 0) {
			tempw = RM16(ds, ESI);
			OUT16(DX, tempw);
			if(flags & D_FLAG) ESI -= 2;
			else		   ESI += 2;
			c--;
			cycles -= 14;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0xA4: case 0x1A4: /*REP MOVSB*/
		if(c > 0) {
			temp2 = RM8(ds, SI);
			WM8(es, DI, temp2);
			if(flags & D_FLAG) {
				DI--;
				SI--;
			}
			else {
				DI++;
				SI++;
			}
			c--;
			cycles -= 4;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x2A4: case 0x3A4: /*REP MOVSB*/
		if(c > 0) {
			temp2 = RM8(ds, ESI);
			WM8(es, EDI, temp2);
			if(flags & D_FLAG) {
				EDI--;
				ESI--;
			}
			else {
				EDI++;
				ESI++;
			}
			c--;
			cycles -= 4;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0xA5: /*REP MOVSW*/
		if(c > 0) {
			tempw = RM16(ds, SI);
			WM16(es, DI, tempw);
			if(flags & D_FLAG) {
				DI -= 2; SI -= 2; }
			else {
				DI += 2; SI += 2; }
			c--;
			cycles -= 4;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x1A5: /*REP MOVSL*/
		if(c > 0) {
			templ = RM32(ds, SI);
			WM32(es, DI, templ);
			if(flags & D_FLAG) {
				DI -= 4; SI -= 4; }
			else {
				DI += 4; SI += 4; }
			c--;
			cycles -= 4;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x2A5: /*REP MOVSW*/
		if(c > 0) {
			tempw = RM16(ds, ESI);
			WM16(es, EDI, tempw);
			if(flags & D_FLAG) {
				EDI -= 2; ESI -= 2; }
			else {
				EDI += 2; ESI += 2; }
			c--;
			cycles -= 4;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x3A5: /*REP MOVSL*/
		if(c > 0) {
			templ = RM32(ds, ESI);
			if((EDI & 0xFFFF0000) == 0xA0000) cycles -= 12;
			WM32(es, EDI, templ);
			if(flags & D_FLAG) {
				EDI -= 4; ESI -= 4; }
			else {
				EDI += 4; ESI += 4; }
			c--;
			cycles -= 4;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0xA6: case 0x1A6: /*REP CMPSB*/
		if(fv)
			flags |= Z_FLAG;
		else
			flags &= ~Z_FLAG;
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			temp = RM8(ds, SI);
			temp2 = RM8(es, DI);
			if(flags & D_FLAG) {
				DI--;
				SI--;
			}
			else {
				DI++;
				SI++;
			}
			c--;
			cycles -= 9;
			setsub8(temp, temp2);
		}
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x2A6: case 0x3A6: /*REP CMPSB*/
		if(fv)
			flags |= Z_FLAG;
		else
			flags &= ~Z_FLAG;
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			temp = RM8(ds, ESI);
			temp2 = RM8(es, EDI);
			if(flags & D_FLAG) {
				EDI--;
				ESI--;
			}
			else {
				EDI++;
				ESI++;
			}
			c--;
			cycles -= 9;
			setsub8(temp, temp2);
		}
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0xA7: /*REP CMPSW*/
		if(fv)
			flags |= Z_FLAG;
		else
			flags &= ~Z_FLAG;
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			tempw = RM16(ds, SI);
			tempw2 = RM16(es, DI);
			if(flags & D_FLAG) {
				DI -= 2; SI -= 2; }
			else {
				DI += 2; SI += 2; }
			c--;
			cycles -= 9;
			setsub16(tempw, tempw2);
		}
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x1A7: /*REP CMPSL*/
		if(fv)
			flags |= Z_FLAG;
		else
			flags &= ~Z_FLAG;
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			templ = RM32(ds, SI);
			templ2 = RM32(es, DI);
			if(flags & D_FLAG) {
				DI -= 4; SI -= 4; }
			else {
				DI += 4; SI += 4; }
			c--;
			cycles -= 9;
			setsub32(templ, templ2);
		}
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x2A7: /*REP CMPSW*/
		if(fv)
			flags |= Z_FLAG;
		else
			flags &= ~Z_FLAG;
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			tempw = RM16(ds, ESI);
			tempw2 = RM16(es, EDI);
			if(flags & D_FLAG) {
				EDI -= 2; ESI -= 2; }
			else {
				EDI += 2; ESI += 2; }
			c--;
			cycles -= 9;
			setsub16(tempw, tempw2);
		}
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x3A7: /*REP CMPSL*/
		if(fv)
			flags |= Z_FLAG;
		else
			flags &= ~Z_FLAG;
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			templ = RM32(ds, ESI);
			templ2 = RM32(es, EDI);
			if(flags & D_FLAG) {
				EDI -= 4; ESI -= 4; }
			else {
				EDI += 4; ESI += 4; }
			c--;
			cycles -= 9;
			setsub32(templ, templ2);
		}
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0xAA: case 0x1AA: /*REP STOSB*/
		if(c > 0) {
			WM8(es, DI, AL);
			if(flags & D_FLAG) DI--;
			else		   DI++;
			c--;
			cycles -= 5;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x2AA: case 0x3AA: /*REP STOSB*/
		if(c > 0) {
			WM8(es, EDI, AL);
			if(flags & D_FLAG) EDI--;
			else		   EDI++;
			c--;
			cycles -= 5;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0xAB: /*REP STOSW*/
		if(c > 0) {
			WM16(es, DI, AX);
			if(flags & D_FLAG) DI -= 2;
			else		   DI += 2;
			c--;
			cycles -= 5;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x2AB: /*REP STOSW*/
		if(c > 0) {
			WM16(es, EDI, AX);
			if(flags & D_FLAG) EDI -= 2;
			else		   EDI += 2;
			c--;
			cycles -= 5;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x1AB: /*REP STOSL*/
		if(c > 0) {
			WM32(es, DI, EAX);
			if(flags & D_FLAG) DI -= 4;
			else		   DI += 4;
			c--;
			cycles -= 5;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x3AB: /*REP STOSL*/
		if(c > 0) {
			WM32(es, EDI, EAX);
			if(flags & D_FLAG) EDI -= 4;
			else		   EDI += 4;
			c--;
			cycles -= 5;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0xAC: case 0x1AC: /*REP LODSB*/
		if(c > 0) {
			AL = RM8(ds, SI);
			if(flags & D_FLAG) SI--;
			else		   SI++;
			c--;
			cycles -= 5;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x2AC: case 0x3AC: /*REP LODSB*/
		if(c > 0) {
			AL = RM8(ds, ESI);
			if(flags & D_FLAG) ESI--;
			else		   ESI++;
			c--;
			cycles -= 5;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0xAD: /*REP LODSW*/
		if(c > 0) {
			AX = RM16(ds, SI);
			if(flags & D_FLAG) SI -= 2;
			else		   SI += 2;
			c--;
			cycles -= 5;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x1AD: /*REP LODSL*/
		if(c > 0) {
			EAX = RM32(ds, SI);
			if(flags & D_FLAG) SI -= 4;
			else		   SI += 4;
			c--;
			cycles -= 5;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x2AD: /*REP LODSW*/
		if(c > 0) {
			AX = RM16(ds, ESI);
			if(flags & D_FLAG) ESI -= 2;
			else		   ESI += 2;
			c--;
			cycles -= 5;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x3AD: /*REP LODSL*/
		if(c > 0) {
			EAX = RM32(ds, ESI);
			if(flags & D_FLAG) ESI -= 4;
			else		   ESI += 4;
			c--;
			cycles -= 5;
		}
		if(c > 0) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0xAE: case 0x1AE: /*REP SCASB*/
		if(fv)
			flags |= Z_FLAG;
		else
			flags &= ~Z_FLAG;
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			temp2 = RM8(es, DI);
			setsub8(AL, temp2);
			if(flags & D_FLAG) DI--;
			else		   DI++;
			c--;
			cycles -= 8;
		}
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x2AE: case 0x3AE: /*REP SCASB*/
		if(fv)
			flags |= Z_FLAG;
		else
			flags &= ~Z_FLAG;
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			temp2 = RM8(es, EDI);
			setsub8(AL, temp2);
			if(flags & D_FLAG) EDI--;
			else		   EDI++;
			c--;
			cycles -= 8;
		}
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0xAF: /*REP SCASW*/
		if(fv)
			flags |= Z_FLAG;
		else
			flags &= ~Z_FLAG;
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			tempw = RM16(es, DI);
			setsub16(AX, tempw);
			if(flags & D_FLAG) DI -= 2;
			else		   DI += 2;
			c--;
			cycles -= 8;
		}
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x1AF: /*REP SCASL*/
		if(fv)
			flags |= Z_FLAG;
		else
			flags &= ~Z_FLAG;
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			templ = RM32(es, DI);
			setsub32(EAX, templ);
			if(flags & D_FLAG) DI -= 4;
			else		   DI += 4;
			c--;
			cycles -= 8;
		}
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x2AF: /*REP SCASW*/
		if(fv)
			flags |= Z_FLAG;
		else
			flags &= ~Z_FLAG;
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			tempw = RM16(es, EDI);
			setsub16(AX, tempw);
			if(flags & D_FLAG) EDI -= 2;
			else		   EDI += 2;
			c--;
			cycles -= 8;
		}
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	case 0x3AF: /*REP SCASL*/
		if(fv)
			flags |= Z_FLAG;
		else
			flags &= ~Z_FLAG;
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			templ = RM32(es, EDI);
			setsub32(EAX, templ);
			if(flags & D_FLAG) EDI -= 4;
			else		   EDI += 4;
			c--;
			cycles -= 8;
		}
		if(c > 0 && fv == ((flags & Z_FLAG) ? 1 : 0)) {
			pc = ipc;
			if(ssegs)
				ssegs++;
		}
		break;
	default:
		invalid();
		break;
	}
	if(rep32 & 0x200)
		ECX = c;
	else
		CX = c;
	if(changeds)
		ds = oldds;
}

void I386::run(int clock)
{
	uint8 temp, temp2;
	uint16 tempw, tempw2, tempw3, tempw4;
	uint32 templ, templ2, addr;
	uint64 temp64;
	int tempws, tempi;
	int64 temp64i;
	int c, low, high;
	int8 offset;
	int inhlt, noint;
	
	if(busreq) {
//		tsc += clock;
		base_cycles = cycles = 0;
		return;
	}
	
	// run cpu while given clocks
	cycles += clock;
	base_cycles = cycles;
	while(cycles > 0) {
		oldcs = CS;
		oldpc = pc;
		oldcpl = CPL;
		op32 = use32;
		optype = 0;
opcodestart:
		prev_pc = cs + pc;
		fetchdat = RM32(cs + pc);
		pc++;
		uint8 opcode = fetchdat & 0xFF;
		cflag = flags & C_FLAG;
		inhlt = noint = 0;
		
		switch(opcode | op32)
		{
		case 0x00: case 0x100: case 0x200: case 0x300: /*ADD 8, reg*/
			fetchea();
			temp = getea8();
			setadd8(temp, getr8(reg));
			temp += getr8(reg);
			setea8(temp);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x01: case 0x201: /*ADD 16, reg*/
			fetchea();
			tempw = getea16();
			setadd16(tempw, regs[reg].w);
			tempw += regs[reg].w;
			setea16(tempw);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x101: case 0x301: /*ADD 32, reg*/
			fetchea();
			templ = getea32();
			setadd32(templ, regs[reg].l);
			templ += regs[reg].l;
			setea32(templ);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x02: case 0x102: case 0x202: case 0x302: /*ADD reg, 8*/
			fetchea();
			temp = getea8();
			setadd8(getr8(reg), temp);
			setr8(reg, getr8(reg) + temp);
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x03: case 0x203: /*ADD reg, 16*/
			fetchea();
			tempw = getea16();
			setadd16(regs[reg].w, tempw);
			regs[reg].w += tempw;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x103: case 0x303: /*ADD reg, 32*/
			fetchea();
			templ = getea32();
			setadd32(regs[reg].l, templ);
			regs[reg].l += templ;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x04: case 0x104: case 0x204: case 0x304: /*ADD AL, #8*/
			temp = fetchdat >> 8; pc++;
			setadd8(AL, temp);
			AL += temp;
			cycles -= 2;
			break;
		case 0x05: case 0x205: /*ADD AX, #16*/
			tempw = fetchdat >> 8; pc += 2;
			setadd16(AX, tempw);
			AX += tempw;
			cycles -= 2;
			break;
		case 0x105: case 0x305: /*ADD EAX, #32*/
			templ = fetch32();
			setadd32(EAX, templ);
			EAX += templ;
			cycles -= 2;
			break;

		case 0x06: case 0x206: /*PUSH ES*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM16(ss, ESP - 2, ES);
				ESP -= 2;
			}
			else {
				WM16(ss, ((SP - 2) & 0xFFFF), ES);
				SP -= 2;
			}
			cycles -= 2;
			break;
		case 0x106: case 0x306: /*PUSH ES*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM32(ss, ESP - 4, ES);
				ESP -= 4;
			}
			else {
				WM32(ss, ((SP - 4) & 0xFFFF), ES);
				SP -= 4;
			}
			cycles -= 2;
			break;
		case 0x07: case 0x207: /*POP ES*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				tempw = RM16(ss, ESP);
				ESP += 2;
			}
			else {
				tempw = RM16(ss, SP);
				SP += 2;
			}
			loadseg(tempw, &_es);
			cycles -= 7;
			break;
		case 0x107: case 0x307: /*POP ES*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				tempw = RM16(ss, ESP);
				ESP += 4;
			}
			else {
				tempw = RM16(ss, SP);
				SP += 4;
			}
			loadseg(tempw, &_es);
			cycles -= 7;
			break;

		case 0x08: case 0x108: case 0x208: case 0x308: /*OR 8, reg*/
			fetchea();
			temp = getea8();
			temp |= getr8(reg);
			setznp8(temp);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			setea8(temp);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x09: case 0x209: /*OR 16, reg*/
			fetchea();
			tempw = getea16();
			tempw |= regs[reg].w;
			setznp16(tempw);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			setea16(tempw);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x109: case 0x309: /*OR 32, reg*/
			fetchea();
			templ = getea32();
			templ |= regs[reg].l;
			setznp32(templ);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			setea32(templ);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x0A: case 0x10A: case 0x20A: case 0x30A: /*OR reg, 8*/
			fetchea();
			temp = getea8();
			temp |= getr8(reg);
			setznp8(temp);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			setr8(reg, temp);
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x0B: case 0x20B: /*OR reg, 16*/
			fetchea();
			tempw = getea16();
			tempw |= regs[reg].w;
			setznp16(tempw);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			regs[reg].w = tempw;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x10B: case 0x30B: /*OR reg, 32*/
			fetchea();
			templ = getea32();
			templ |= regs[reg].l;
			setznp32(templ);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			regs[reg].l = templ;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x0C: case 0x10C: case 0x20C: case 0x30C: /*OR AL, #8*/
			AL |= fetch8();
			setznp8(AL);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= 2;
			break;
		case 0x0D: case 0x20D: /*OR AX, #16*/
			AX |= fetch16();
			setznp16(AX);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= 2;
			break;
		case 0x10D: case 0x30D: /*OR AX, #32*/
			EAX |= fetch32();
			setznp32(EAX);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= 2;
			break;

		case 0x0E: case 0x20E: /*PUSH CS*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM16(ss, ESP - 2, CS);
				ESP -= 2;
			}
			else {
				WM16(ss, ((SP - 2) & 0xFFFF), CS);
				SP -= 2;
			}
			cycles -= 2;
			break;
		case 0x10E: case 0x30E: /*PUSH CS*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM32(ss, ESP - 4, CS);
				ESP -= 4;
			}
			else {
				WM32(ss, ((SP - 4) & 0xFFFF), CS);
				SP -= 4;
			}
			cycles -= 2;
			break;

		case 0x0F: case 0x20F:
			temp = fetchdat >> 8; pc++;
			switch(temp)
			{
			case 0:
				if(!(cr0 & 1) || (eflags & VM_FLAG)) {
					invalid();
					break;
				}
				fetchea2();
				switch(rmdat & 0x38) {
				case 0x00: /*SLDT*/
					setea16(ldt.seg);
					cycles -= 4;
					break;
				case 0x08: /*STR*/
					setea16(tr.seg);
					cycles -= 4;
					break;
				case 0x10: /*LLDT*/
					ldt.seg = getea16();
					templ = (ldt.seg & ~7) + gdt.base;
					ldt.limit = RM16(templ);
					ldt.base = (RM16(templ + 2)) | (RM8(templ + 4) << 16) | (RM8(templ + 7) << 24);
					cycles -= 20;
					break;
				case 0x18: /*LTR*/
					tr.seg = getea16();
					templ = (tr.seg & ~7) + gdt.base;
					tr.limit = RM16(templ);
					tr.base = (RM16(templ + 2)) | (RM8(templ + 4) << 16) | (RM8(templ + 7) << 24);
					tr.access = RM8(templ + 5);
					cycles -= 20;
					break;
				case 0x20: /*VERR*/
					tempw = getea16();
					flags &= ~Z_FLAG;
					if(!(tempw & 0xFFFC)) break; /*Null selector*/
					tempi = (tempw & ~7) < ((tempw & 4) ? ldt.limit:gdt.limit);
					tempw2 = RM16(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7) + 4);
					if(!(tempw2 & 0x1000)) tempi = 0;
					if((tempw2 & 0xC00) != 0xC00) {
						/*Exclude conforming code segments*/
						tempw3 = (tempw2 >> 13) & 3; /*Check permissions*/
						if(tempw3 < CPL || tempw3 < (tempw & 3)) tempi = 0;
					}
					if((tempw2 & 0x0800) && !(tempw2 & 0x0200))
						tempi = 0;
					if(tempi)
						flags |= Z_FLAG;
					cycles -= 20;
					break;
				case 0x28: /*VERW*/
					tempw = getea16();
					flags &= ~Z_FLAG;
					if(!(tempw & 0xFFFC)) break; /*Null selector*/
					tempi = (tempw & ~7) < ((tempw & 4) ? ldt.limit:gdt.limit);
					tempw2 = RM16(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7) + 4);
					if(!(tempw2 & 0x1000)) tempi = 0;
					tempw3 = (tempw2 >> 13) & 3; /*Check permissions*/
					if(tempw3 < CPL || tempw3 < (tempw & 3)) tempi = 0;
					if(tempw2 & 0x0800) tempi = 0; /*Code*/
					else if(!(tempw2 & 0x0200)) tempi = 0; /*Read - only data*/
					if(tempi) flags |= Z_FLAG;
					cycles -= 20;
					break;
				default:
					invalid();
					break;
				}
				break;
			case 1:
				fetchea2();
				switch(rmdat & 0x38)
				{
				case 0x00: /*SGDT*/
					setea16(gdt.limit);
					WM16(easeg, eaaddr + 2, gdt.base);
					WM8(easeg, eaaddr + 4, gdt.base >> 16);
					cycles -= 7; 
					/*else*/       WM8(easeg, eaaddr + 5, 0xFF);
					break;
				case 0x08: /*SIDT*/
					setea16(idt.limit);
					WM16(easeg, eaaddr + 2, idt.base);
					WM8(easeg, eaaddr + 4, idt.base >> 16);
					if(msw & 1)
						WM8(easeg, eaaddr + 5, idt.access);
					else
						WM8(easeg, eaaddr + 5, 0xFF);
					cycles -= 7;
					break;
				case 0x10: /*LGDT*/
					if((CPL || eflags & VM_FLAG) && (cr0 & 1)) {
						general_protection_fault(0);
						break;
					}
					gdt.limit = getea16();
					gdt.base = RM32(easeg + eaaddr + 2) & 0xFFFFFF;
					cycles -= 11;
					break;
				case 0x18: /*LIDT*/
					if((CPL || eflags & VM_FLAG) && (cr0 & 1)) {
						general_protection_fault(0);
						break;
					}
					idt.limit = getea16();
					idt.base = RM32(easeg + eaaddr + 2) & 0xFFFFFF;
					cycles -= 11;
					break;

				case 0x20: /*SMSW*/
					setea16(msw | 0xFF00);
					cycles -= 2;
					break;
				case 0x30: /*LMSW*/
					if((CPL || eflags & VM_FLAG) && (msw & 1)) {
						general_protection_fault(0);
						break;
					}
					tempw = getea16();
					if(msw & 1) tempw |= 1;
					msw = tempw;
					break;
				default:
					invalid();
					break;
				}
				break;
			case 2: /*LAR*/
				fetchea2();
				tempw = getea16();
				flags &= ~Z_FLAG;
				if(!(tempw & 0xFFFC)) break; /*Null selector*/
				tempi = (tempw & ~7) < ((tempw & 4) ? ldt.limit:gdt.limit);
				tempw2 = RM16(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7) + 4);
				if((tempw2 & 0xE00) == 0x600) tempi = 0; /*Interrupt or trap gate*/
				if((tempw2 & 0xC00) != 0xC00) /*Exclude conforming code segments*/
				{
					tempw3 = (tempw2 >> 13) & 3;
					if(tempw3 < CPL || tempw3 < (tempw & 3)) tempi = 0;
				}
				if(tempi) {
					flags |= Z_FLAG;
					regs[reg].w = RM8(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7) + 5) << 8;
				}
				cycles -= 11;
				break;

			case 3: /*LSL*/
				fetchea2();
				tempw = getea16();
				flags &= ~Z_FLAG;
				if(!(tempw & 0xFFFC)) break; /*Null selector*/
				tempi = (tempw & ~7) < ((tempw & 4) ? ldt.limit:gdt.limit);
				tempw2 = RM16(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7) + 4);
				if((tempw2 & 0xE00) == 0x600) tempi = 0; /*Interrupt or trap gate*/
				if((tempw2 & 0xC00) != 0xC00) /*Exclude conforming code segments*/
				{
					tempw3 = (tempw2 >> 13) & 3;
					if(tempw3 < CPL || tempw3 < (tempw & 3)) tempi = 0;
				}
				if(tempi) {
					flags |= Z_FLAG;
					regs[reg].w = RM16(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7));
				}
				cycles -= 10;
				break;

			case 5: /*LOADALL*/
				flags = RM16(0x818);
				pc = RM16(0x81A);
				DS = RM16(0x81E);
				SS = RM16(0x820);
				CS = RM16(0x822);
				ES = RM16(0x824);
				DI = RM16(0x826);
				SI = RM16(0x828);
				BP = RM16(0x82A);
				SP = RM16(0x82C);
				BX = RM16(0x82E);
				DX = RM16(0x830);
				CX = RM16(0x832);
				AX = RM16(0x834);
				if(RM16(0x806)) {
//					exit(-1);
				}
				es = RM16(0x836) | (RM8(0x838) << 16);
				cs = RM16(0x83C) | (RM8(0x83E) << 16);
				ss = RM16(0x842) | (RM8(0x844) << 16);
				ds = RM16(0x848) | (RM8(0x84A) << 16);
				cycles -= 195;
				break;
				
			case 6: /*CLTS*/
				cr0 &= ~8;
				cycles -= 5;
				break;

			case 0x20: /*MOV reg32, CRx*/
				if((CPL || (eflags & VM_FLAG)) && (cr0 & 1)) {
					general_protection_fault(0);
					break;
				}
				fetchea2();
				switch(reg) {
				case 0:
					regs[rm].l = cr0;
					break;
				case 2:
					regs[rm].l = cr2;
					break;
				case 3:
					regs[rm].l = cr3;
					break;
				default:
					invalid();
					break;
				}
				cycles -= 6;
				break;
			case 0x21: /*MOV reg32, DRx*/
				if((CPL || (eflags & VM_FLAG)) && (cr0 & 1)) {
					general_protection_fault(0);
					break;
				}
				fetchea2();
				regs[rm].l = 0;
				cycles -= 6;
				break;
			case 0x22: /*MOV CRx, reg32*/
				if((CPL || (eflags & VM_FLAG)) && (cr0 & 1)) {
					general_protection_fault(0);
					break;
				}
				fetchea2();
				switch(reg) {
				case 0:
					cr0 = regs[rm].l;
					if(cr0 & 0x80000000) {
					}
					break;
				case 2:
					cr2 = regs[rm].l;
					break;
				case 3:
					cr3 = regs[rm].l & ~0xFFF;
					break;
				default:
					invalid();
					break;
				}
				cycles -= 10;
				break;
			case 0x23: /*MOV DRx, reg32*/
				if((CPL || (eflags & VM_FLAG)) && (cr0 & 1)) {
					general_protection_fault(0);
					break;
				}
				fetchea2();
				cycles -= 6;
				break;

			case 0x82: /*JB*/
				tempw = fetch16();
				if(flags & C_FLAG) {
					pc += (int16)tempw;
					cycles -= 4;
				}
				cycles -= 3;
				break;
			case 0x83: /*JNB*/
				tempw = fetch16();
				if(!(flags & C_FLAG)) {
					pc += (int16)tempw;
					cycles -= 4;
				}
				cycles -= 3;
				break;
			case 0x84: /*JE*/
				tempw = fetch16();
				if(flags & Z_FLAG)
					pc += (int16)tempw;
				cycles -= 4;
				break;
			case 0x85: /*JNE*/
				tempw = fetch16();
				if(!(flags & Z_FLAG))
					pc += (int16)tempw;
				cycles -= 4;
				break;
			case 0x86: /*JBE*/
				tempw = fetch16();
				if(flags & (C_FLAG | Z_FLAG)) {
					pc += (int16)tempw;
					cycles -= 4;
				}
				cycles -= 3;
				break;
			case 0x87: /*JNBE*/
				tempw = fetch16();
				if(!(flags & (C_FLAG | Z_FLAG))) {
					pc += (int16)tempw;
					cycles -= 4;
				}
				cycles -= 3;
				break;
			case 0x88: /*JS*/
				tempw = fetch16();
				if(flags & N_FLAG)
					pc += (int16)tempw;
				cycles -= 4;
				break;
			case 0x89: /*JNS*/
				tempw = fetch16();
				if(!(flags & N_FLAG))
					pc += (int16)tempw;
				cycles -= 4;
				break;
			case 0x8A: /*JP*/
				tempw = fetch16();
				if(flags & P_FLAG) {
					pc += (int16)tempw;
					cycles -= 4;
				}
				cycles -= 3;
				break;
			case 0x8B: /*JNP*/
				tempw = fetch16();
				if(!(flags & P_FLAG)) {
					pc += (int16)tempw;
					cycles -= 4;
				}
				cycles -= 3;
				break;
			case 0x8C: /*JL*/
				tempw = fetch16();
				temp = (flags & N_FLAG) ? 1 : 0;
				temp2 = (flags & V_FLAG) ? 1 : 0;
				if(temp != temp2)  {
					pc += (int16)tempw;
					cycles -= 4;
				}
				cycles -= 3;
				break;
			case 0x8D: /*JNL*/
				tempw = fetch16();
				temp = (flags & N_FLAG) ? 1 : 0;
				temp2 = (flags & V_FLAG) ? 1 : 0;
				if(temp == temp2)  {
					pc += (int16)tempw;
					cycles -= 4;
				}
				cycles -= 3;
				break;
			case 0x8E: /*JLE*/
				tempw = fetch16();
				temp = (flags & N_FLAG) ? 1 : 0;
				temp2 = (flags & V_FLAG) ? 1 : 0;
				if((flags & Z_FLAG) || (temp != temp2)) {
					pc += (int16)tempw;
					cycles -= 4;
				}
				cycles -= 3;
				break;
			case 0x8F: /*JNLE*/
				tempw = fetch16();
				temp = (flags & N_FLAG) ? 1 : 0;
				temp2 = (flags & V_FLAG) ? 1 : 0;
				if(!((flags & Z_FLAG) || (temp != temp2))) {
					pc += (int16)tempw;
					cycles -= 4;
				}
				cycles -= 3;
				break;
			case 0x92: /*SETC*/
				fetchea2();
				setea8((flags & C_FLAG) ? 1 : 0);
				cycles -= 4;
				break;
			case 0x93: /*SETAE*/
				fetchea2();
				setea8((flags & C_FLAG) ? 0 : 1);
				cycles -= 4;
				break;
			case 0x94: /*SETZ*/
				fetchea2();
				setea8((flags & Z_FLAG) ? 1 : 0);
				cycles -= 4;
				break;
			case 0x95: /*SETNZ*/
				fetchea2();
				setea8((flags & Z_FLAG) ? 0 : 1);
				cycles -= 4;
				break;
			case 0x96: /*SETBE*/
				fetchea2();
				setea8((flags & (C_FLAG | Z_FLAG)) ? 1 : 0);
				cycles -= 4;
				break;
			case 0x97: /*SETNBE*/
				fetchea2();
				setea8((flags & (C_FLAG | Z_FLAG)) ? 0 : 1);
				cycles -= 4;
				break;
			case 0x98: /*SETS*/
				fetchea2();
				setea8((flags & N_FLAG) ? 1 : 0);
				cycles -= 4;
				break;

			case 0xA0: /*PUSH FS*/
				if(ssegs)
					ss = oldss;
				if(stack32) {
					WM16(ss, ESP - 2, FS);
					ESP -= 2;
				}
				else {
					WM16(ss, ((SP - 2) & 0xFFFF), FS);
					SP -= 2;
				}
				cycles -= 2;
				break;
			case 0xA1: /*POP FS*/
				if(ssegs)
					ss = oldss;
				if(stack32) {
					tempw = RM16(ss, ESP);
					ESP += 2;
				}
				else {
					tempw = RM16(ss, SP);
					SP += 2;
				}
				loadseg(tempw, &_fs);
				cycles -= 7;
				break;
			case 0xA3: /*BT r16*/
				fetchea2();
				tempw = getea16();
				if(tempw & (1 << (regs[reg].w & 15)))
					flags |= C_FLAG;
				else
					flags &= ~C_FLAG;
				cycles -= 3;
				break;
			case 0xA4: /*SHLD imm*/
				fetchea2();
				temp = fetch8();
				if(temp && temp < 16) {
					tempw = getea16();
					if((tempw << (temp - 1)) & 0x8000)
						flags |= C_FLAG;
					else
						flags &= ~C_FLAG;
					tempw = (tempw << temp) | (regs[reg].w >> (16 - temp));
					setea16(tempw);
					setznp16(tempw);
				}
				cycles -= 3;
				break;
			case 0xA5: /*SHLD CL*/
				fetchea2();
				temp = CL;
				if(temp && temp < 16) {
					tempw = getea16();
					if((tempw << (temp - 1)) & 0x8000)
						flags |= C_FLAG;
					else
						flags &= ~C_FLAG;
					tempw = (tempw << temp) | (regs[reg].w >> (16 - temp));
					setea16(tempw);
					setznp16(tempw);
				}
				cycles -= 3;
				break;
			case 0xA8: /*PUSH GS*/
				if(ssegs)
					ss = oldss;
				if(stack32) {
					WM16(ss, ESP - 2, GS);
					ESP -= 2;
				}
				else {
					WM16(ss, ((SP - 2) & 0xFFFF), GS);
					SP -= 2;
				}
				cycles -= 2;
				break;
			case 0xA9: /*POP GS*/
				if(ssegs)
					ss = oldss;
				if(stack32) {
					tempw = RM16(ss, ESP);
					ESP += 2;
				}
				else {
					tempw = RM16(ss, SP);
					SP += 2;
				}
				loadseg(tempw, &_gs);
				cycles -= 7;
				break;
			case 0xAB: /*BTS r16*/
				fetchea2();
				tempw = getea16();
				if(tempw & (1 << (regs[reg].w & 15)))
					flags |= C_FLAG;
				else
					flags &= ~C_FLAG;
				tempw |= (1 << (regs[reg].w & 15));
				setea16(tempw);
				cycles -= 6;
				break;
			case 0xAC: /*SHRD imm*/
				fetchea2();
				temp = fetch8();
				if(temp && temp < 16) {
					tempw = getea16();
					if((tempw >> (temp - 1)) & 1)
						flags |= C_FLAG;
					else
						flags &= ~C_FLAG;
					tempw = (tempw >> temp) | (regs[reg].w << (16 - temp));
					setea16(tempw);
					setznp16(tempw);
				}
				cycles -= 3;
				break;
			case 0xAD: /*SHRD CL*/
				fetchea2();
				temp = CL;
				if(temp && temp < 16) {
					tempw = getea16();
					if((tempw >> (temp - 1)) & 1)
						flags |= C_FLAG;
					else
						flags &= ~C_FLAG;
					tempw = (tempw >> temp) | (regs[reg].w << (16 - temp));
					setea16(tempw);
					setznp16(tempw);
				}
				cycles -= 3;
				break;
			case 0xAF: /*IMUL reg16, rm16*/
				fetchea2();
				temp64i = (int64)(int16)regs[reg].w*(int64)(int16)getea16();
				regs[reg].w = temp64i & 0xFFFF;
				if((temp64i >> 16) && (temp64i >> 16) != -1)
					flags |= C_FLAG | V_FLAG;
				else
					flags &= ~(C_FLAG | V_FLAG);
				cycles -= 18;
				break;
			case 0xB3: /*BTR r16*/
				fetchea2();
				tempw = getea16();
				if(tempw & (1 << (regs[reg].w & 15)))
					flags |= C_FLAG;
				else
					flags &= ~C_FLAG;
				tempw &= ~(1 << (regs[reg].w & 15));
				setea16(tempw);
				cycles -= 6;
				break;
			case 0xB2: /*LSS*/
				fetchea2();
				regs[reg].w = RM16(easeg, eaaddr); 
				tempw = RM16(easeg, (eaaddr + 2)); 
				loadseg(tempw, &_ss);
				cycles -= 7;
				break;
			case 0xB4: /*LFS*/
				fetchea2();
				regs[reg].w = RM16(easeg, eaaddr); 
				tempw = RM16(easeg, (eaaddr + 2)); 
				loadseg(tempw, &_fs);
				cycles -= 7;
				break;
			case 0xB5: /*LGS*/
				fetchea2();
				regs[reg].w = RM16(easeg, eaaddr); 
				tempw = RM16(easeg, (eaaddr + 2)); 
				loadseg(tempw, &_gs);
				cycles -= 7;
				break;
			case 0xB6: /*MOVZX b*/
				fetchea2();
				regs[reg].w = getea8();
				cycles -= 3;
				break;
			case 0xBE: /*MOVSX b*/
				fetchea2();
				regs[reg].w = getea8();
				if(regs[reg].w & 0x80)
					regs[reg].w |= 0xFF00;
				cycles -= 3;
				break;
			case 0xBA: /*MORE?!?!?!*/
				fetchea2();
				switch(rmdat & 0x38) {
				case 0x20: /*BT w, imm*/
					tempw = getea16();
					temp = fetch8();
					if(tempw & (1 << temp))
						flags |= C_FLAG;
					else
						flags &= ~C_FLAG;
					cycles -= 6;
					break;
				case 0x28: /*BTS w, imm*/
					tempw = getea16();
					temp = fetch8();
					if(tempw & (1 << temp))
						flags |= C_FLAG;
					else
						flags &= ~C_FLAG;
					tempw |= (1 << temp);
					setea16(tempw);
					cycles -= 6;
					break;
				case 0x30: /*BTR w, imm*/
					tempw = getea16();
					temp = fetch8();
					if(tempw & (1 << temp)) flags |= C_FLAG;
					else		 flags &= ~C_FLAG;
					tempw &= ~(1 << temp);
					setea16(tempw);
					cycles -= 6;
					break;
				case 0x38: /*BTC w, imm*/
					tempw = getea16();
					temp = fetch8();
					if(tempw & (1 << temp)) flags |= C_FLAG;
					else		 flags &= ~C_FLAG;
					tempw ^=(1 << temp);
					setea16(tempw);
					cycles -= 6;
					break;

				default:
					invalid();
					break;
				}
				break;

			case 0xBC: /*BSF w*/
				fetchea2();
				tempw = getea16();
				if(!tempw) {
					flags |= Z_FLAG;
				}
				else {
					for(tempi = 0;tempi < 16;tempi++) {
						cycles -= 3;
						if(tempw & (1 << tempi)) {
							flags &= ~Z_FLAG;
							regs[reg].w = tempi;
							break;
						}
					}
				}
				cycles -= 10;
				break;
			case 0xBD: /*BSR w*/
				fetchea2();
				tempw = getea16();
				if(!tempw)
					flags |= Z_FLAG;
				else {
					for(tempi = 15;tempi >= 0;tempi--) {
						cycles -= 3;
						if(tempw & (1 << tempi)) {
							flags &= ~Z_FLAG;
							regs[reg].w = tempi;
							break;
						}
					}
				}
				cycles -= 10;
				break;
//			case 0xA6: /*XBTS / CMPXCHG486*/
//			case 0xFF: /*Invalid  -  Windows 3.1 syscall trap?*/
			default:
				invalid();
				break;
			}
			break;

		case 0x10F: case 0x30F:
			temp = fetchdat >> 8; pc++;
			switch(temp)
			{
			case 0:
				if(!(cr0 & 1) || (eflags & VM_FLAG)) {
					invalid();
					break;
				}
				fetchea2();
				switch(rmdat & 0x38)
				{
				case 0x00: /*SLDT*/
					setea32(ldt.seg);
					cycles -= 4;
					break;
				case 0x08: /*STR*/
					setea32(tr.seg);
					cycles -= 4;
					break;
				case 0x10: /*LLDT*/
					templ = (ldt.seg & ~7) + gdt.base;
					ldt.limit = RM16(templ);
					ldt.base = (RM16(templ + 2)) | (RM8(templ + 4) << 16) | (RM8(templ + 7) << 24);
					cycles -= 20;
					break;
				case 0x18: /*LTR*/
					tr.seg = getea16();
					templ = (tr.seg & ~7) + gdt.base;
					tr.limit = RM16(templ);
					tr.base = (RM16(templ + 2)) | (RM8(templ + 4) << 16) | (RM8(templ + 7) << 24);
					tr.access = RM8(templ + 5);
					cycles -= 20;
					break;
#if 0
				case 0x20: /*VERR*/
					tempw = getea16();
					flags &= ~Z_FLAG;
					if(!(tempw & 0xFFFC)) break; /*Null selector*/
					tempi = (tempw & ~7) < ((tempw & 4) ? ldt.limit:gdt.limit);
					tempw2 = RM16(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7) + 4);
					if(!(tempw2 & 0x1000)) tempi = 0;
					if((tempw2 & 0xC00) != 0xC00) /*Exclude conforming code segments*/
					{
						tempw3 = (tempw2 >> 13) & 3; /*Check permissions*/
						if(tempw3 < CPL || tempw3 < (tempw & 3)) tempi = 0;
					}
					if((tempw2 & 0x0800) && !(tempw2 & 0x0200)) tempi = 0; /*Non - readable code*/
					if(tempi) flags |= Z_FLAG;
					cycles -= 20;
					break;
				case 0x28: /*VERW*/
					tempw = getea16();
					flags &= ~Z_FLAG;
					if(!(tempw & 0xFFFC)) break; /*Null selector*/
					tempi = (tempw & ~7) < ((tempw & 4) ? ldt.limit:gdt.limit);
					tempw2 = RM16(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7) + 4);
					if(!(tempw2 & 0x1000)) tempi = 0;
					tempw3 = (tempw2 >> 13) & 3; /*Check permissions*/
					if(tempw3 < CPL || tempw3 < (tempw & 3)) tempi = 0;
					if(tempw2 & 0x0800) tempi = 0; /*Code*/
					else if(!(tempw2 & 0x0200)) tempi = 0; /*Read - only data*/
					if(tempi) flags |= Z_FLAG;
					cycles -= 20;
					break;
#endif
				default:
					invalid();
					break;
				}
				break;
			case 1:
				fetchea2();
				switch(rmdat & 0x38) {
				case 0x00: /*SGDT*/
					setea16(gdt.limit);
					WM32(easeg, eaaddr + 2, gdt.base);
					cycles -= 7;
					break;
				case 0x08: /*SIDT*/
					setea16(idt.limit);
					WM32(easeg, eaaddr + 2, idt.base);
					cycles -= 7;
					break;
				case 0x10: /*LGDT*/
					if((CPL || eflags & VM_FLAG) && (cr0 & 1)) {
						general_protection_fault(0);
						break;
					}
					gdt.limit = getea16();
					gdt.base = RM32(easeg, eaaddr + 2);
					break;
				case 0x18: /*LIDT*/
					if((CPL || eflags & VM_FLAG) && (cr0 & 1)) {
						general_protection_fault(0);
						break;
					}
					idt.limit = getea16();
					idt.base = RM32(easeg, eaaddr + 2);
					cycles -= 11;
					break;
				case 0x20: /*SMSW*/
					setea32(cr0); /*Apparently this is the case!*/
					cycles -= 2;
					break;
				default:
					invalid();
					break;
				}
				break;

			case 2: /*LAR*/
				fetchea2();
				tempw = getea16();
				flags &= ~Z_FLAG;
				if(!(tempw & 0xFFFC)) break; /*Null selector*/
				tempi = (tempw & ~7) < ((tempw & 4) ? ldt.limit:gdt.limit);
				tempw2 = RM16(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7) + 4);
				if((tempw2 & 0xE00) == 0x600) tempi = 0; /*Interrupt or trap gate*/
				if((tempw2 & 0xC00) != 0xC00) /*Exclude conforming code segments*/
				{
					tempw3 = (tempw2 >> 13) & 3;
					if(tempw3 < CPL || tempw3 < (tempw & 3)) tempi = 0;
				}
				if(tempi) {
					flags |= Z_FLAG;
					regs[reg].l = RM32(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7) + 4) & 0xFFFF00;
				}
				cycles -= 11;
				break;

			case 3: /*LSL*/
				fetchea2();
				tempw = getea16();
				flags &= ~Z_FLAG;
				if(!(tempw & 0xFFFC)) break; /*Null selector*/
				tempi = (tempw & ~7) < ((tempw & 4) ? ldt.limit:gdt.limit);
				tempw2 = RM16(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7) + 4);
				if((tempw2 & 0xE00) == 0x600) tempi = 0; /*Interrupt or trap gate*/
				if((tempw2 & 0xC00) != 0xC00) /*Exclude conforming code segments*/
				{
					tempw3 = (tempw2 >> 13) & 3;
					if(tempw3 < CPL || tempw3 < (tempw & 3)) tempi = 0;
				}
				if(tempi) {
					flags |= Z_FLAG;
					regs[reg].l = RM16(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7));
					regs[reg].l |= (RM8(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7) + 6) & 0xF) << 16;
					if(RM8(((tempw & 4) ? ldt.base:gdt.base) + (tempw & ~7) + 6) & 0x80) regs[reg].l <<= 12;
				}
				cycles -= 10;
				break;

			case 6: /*CLTS*/
				cr0 &= ~8;
				cycles -= 5;
				break;

			case 0x20: /*MOV reg32, CRx*/
				if((CPL || (eflags & VM_FLAG)) && (cr0 & 1)) {
					general_protection_fault(0);
					break;
				}
				fetchea2();
				switch(reg) {
				case 0:
					regs[rm].l = cr0;
					break;
				case 2:
					regs[rm].l = cr2;
					break;
				case 3:
					regs[rm].l = cr3;
					break;
				default:
					invalid();
					break;
				}
				cycles -= 6;
				break;
			case 0x21: /*MOV reg32, DRx*/
				if((CPL || (eflags & VM_FLAG)) && (cr0 & 1)) {
					general_protection_fault(0);
					break;
				}
				fetchea2();
				regs[rm].l = 0;
				cycles -= 6;
				break;
			case 0x22: /*MOV CRx, reg32*/
				if((CPL || (eflags & VM_FLAG)) && (cr0 & 1)) {
					general_protection_fault(0);
					break;
				}
				fetchea2();
				switch(reg) {
				case 0:
					cr0 = regs[rm].l;
					break;
				case 2:
					cr2 = regs[rm].l;
					break;
				case 3:
					cr3 = regs[rm].l & ~0xFFF;
					break;
				default:
					invalid();
					break;
				}
				cycles -= 10;
				break;
			case 0x23: /*MOV DRx, reg32*/
				if((CPL || (eflags & VM_FLAG)) && (cr0 & 1)) {
					general_protection_fault(0);
					break;
				}
				fetchea2();
				cycles -= 6;
				break;

			case 0x82: /*JB*/
				templ = fetch32();
				if(flags & C_FLAG) { pc += templ; cycles -= 4; }
				cycles -= 3;
				break;
			case 0x83: /*JNB*/
				templ = fetch32();
				if(!(flags & C_FLAG)) { pc += templ; cycles -= 4; }
				cycles -= 3;
				break;
			case 0x84: /*JE*/
				templ = fetch32();
				if(flags & Z_FLAG) pc += templ;
				cycles -= 4;
				break;
			case 0x85: /*JNE*/
				templ = fetch32();
				if(!(flags & Z_FLAG)) pc += templ;
				cycles -= 4;
				break;
			case 0x86: /*JBE*/
				templ = fetch32();
				if(flags & (C_FLAG | Z_FLAG)) { pc += templ; cycles -= 4; }
				cycles -= 3;
				break;
			case 0x87: /*JNBE*/
				templ = fetch32();
				if(!(flags & (C_FLAG | Z_FLAG))) { pc += templ; cycles -= 4; }
				cycles -= 3;
				break;
			case 0x88: /*JS*/
				templ = fetch32();
				if(flags & N_FLAG) pc += templ;
				cycles -= 4;
				break;
			case 0x89: /*JNS*/
				templ = fetch32();
				if(!(flags & N_FLAG)) pc += templ;
				cycles -= 4;
				break;
			case 0x8A: /*JP*/
				templ = fetch32();
				if(flags & P_FLAG) pc += templ;
				cycles -= 4;
				break;
			case 0x8B: /*JNP*/
				templ = fetch32();
				if(!(flags & P_FLAG)) pc += templ;
				cycles -= 4;
				break;
			case 0x8C: /*JL*/
				templ = fetch32();
				temp = (flags & N_FLAG) ? 1 : 0;
				temp2 = (flags & V_FLAG) ? 1 : 0;
				if(temp != temp2)  { pc += templ; cycles -= 4; }
				cycles -= 3;
				break;
			case 0x8D: /*JNL*/
				templ = fetch32();
				temp = (flags & N_FLAG) ? 1 : 0;
				temp2 = (flags & V_FLAG) ? 1 : 0;
				if(temp == temp2)  { pc += templ; cycles -= 4; }
				cycles -= 3;
				break;
			case 0x8E: /*JLE*/
				templ = fetch32();
				temp = (flags & N_FLAG) ? 1 : 0;
				temp2 = (flags & V_FLAG) ? 1 : 0;
				if((flags & Z_FLAG) || (temp != temp2))  { pc += templ; cycles -= 4; }
				cycles -= 3;
				break;
			case 0x8F: /*JNLE*/
				templ = fetch32();
				temp = (flags & N_FLAG) ? 1 : 0;
				temp2 = (flags & V_FLAG) ? 1 : 0;
				if(!((flags & Z_FLAG) || (temp != temp2)))  { pc += templ; cycles -= 4; }
				cycles -= 3;
				break;

			case 0x90: /*SETO*/
				fetchea2();
				setea8((flags & V_FLAG) ? 1 : 0);
				cycles -= 4;
				break;
			case 0x91: /*SETNO*/
				fetchea2();
				setea8((flags & V_FLAG) ? 0 : 1);
				cycles -= 4;
				break;
			case 0x92: /*SETC*/
				fetchea2();
				setea8((flags & C_FLAG) ? 1 : 0);
				cycles -= 4;
				break;
			case 0x93: /*SETAE*/
				fetchea2();
				setea8((flags & C_FLAG) ? 0 : 1);
				cycles -= 4;
				break;
			case 0x94: /*SETZ*/
				fetchea2();
				setea8((flags & Z_FLAG) ? 1 : 0);
				cycles -= 4;
				break;
			case 0x95: /*SETNZ*/
				fetchea2();
				setea8((flags & Z_FLAG) ? 0 : 1);
				cycles -= 4;
				break;
			case 0x96: /*SETBE*/
				fetchea2();
				setea8((flags & (C_FLAG | Z_FLAG)) ? 1 : 0);
				cycles -= 4;
				break;
			case 0x97: /*SETNBE*/
				fetchea2();
				setea8((flags & (C_FLAG | Z_FLAG)) ? 0 : 1);
				cycles -= 4;
				break;
			case 0x98: /*SETS*/
				fetchea2();
				setea8((flags & N_FLAG) ? 1 : 0);
				cycles -= 4;
				break;
			case 0x99: /*SETNS*/
				fetchea2();
				setea8((flags & N_FLAG) ? 0 : 1);
				cycles -= 4;
				break;
			case 0x9A: /*SETP*/
				fetchea2();
				setea8((flags & P_FLAG) ? 1 : 0);
				cycles -= 4;
				break;
			case 0x9B: /*SETNP*/
				fetchea2();
				setea8((flags & P_FLAG) ? 0 : 1);
				cycles -= 4;
				break;
			case 0x9C: /*SETL*/
				fetchea2();
				temp = (flags & N_FLAG) ? 1 : 0;
				temp2 = (flags & V_FLAG) ? 1 : 0;
				setea8(temp^temp2);
				cycles -= 4;
				break;
			case 0x9D: /*SETGE*/
				fetchea2();
				temp = (flags & N_FLAG) ? 1 : 0;
				temp2 = (flags & V_FLAG) ? 1 : 0;
				setea8((temp^temp2) ? 0 : 1);
				cycles -= 4;
				break;
			case 0x9E: /*SETLE*/
				fetchea2();
				temp = (flags & N_FLAG) ? 1 : 0;
				temp2 = (flags & V_FLAG) ? 1 : 0;
				setea8(((temp^temp2) || (flags & Z_FLAG)) ? 1 : 0);
				cycles -= 4;
				break;
			case 0x9F: /*SETNLE*/
				fetchea2();
				temp = (flags & N_FLAG) ? 1 : 0;
				temp2 = (flags & V_FLAG) ? 1 : 0;
				setea8(((temp^temp2) || (flags & Z_FLAG)) ? 0 : 1);
				cycles -= 4;
				break;

			case 0xA0: /*PUSH FS*/
				if(ssegs)
					ss = oldss;
				if(stack32) {
					WM32(ss, ESP - 4, FS);
					ESP -= 4;
				}
				else {
					WM32(ss, ((SP - 4) & 0xFFFF), FS);
					SP -= 4;
				}
				cycles -= 2;
				break;
			case 0xA1: /*POP FS*/
				if(ssegs)
					ss = oldss;
				if(stack32) {
					tempw = RM16(ss, ESP);
					ESP += 4;
				}
				else {
					tempw = RM16(ss, SP);
					SP += 4;
				}
				loadseg(tempw, &_fs);
				cycles -= 7;
				break;
			case 0xA8: /*PUSH GS*/
				if(ssegs)
					ss = oldss;
				if(stack32) {
					WM32(ss, ESP - 4, GS);
					ESP -= 4;
				}
				else {
					WM32(ss, ((SP - 4) & 0xFFFF), GS);
					SP -= 4;
				}
				cycles -= 2;
				break;
			case 0xA9: /*POP GS*/
				if(ssegs)
					ss = oldss;
				if(stack32) {
					tempw = RM16(ss, ESP);
					ESP += 4;
				}
				else {
					tempw = RM16(ss, SP);
					SP += 4;
				}
				loadseg(tempw, &_gs);
				cycles -= 7;
				break;

			case 0xA3: /*BT r32*/
				fetchea2();
				templ = getea32();
				if(templ&(1 << (regs[reg].l & 31))) flags |= C_FLAG;
				else				  flags &= ~C_FLAG;
				cycles -= 3;
				break;
			case 0xA4: /*SHLD imm*/
				fetchea2();
				temp = fetch8();
				if(temp && temp < 32) {
					templ = getea32();
					if((templ << (temp - 1)) & 0x80000000) flags |= C_FLAG;
					else				   flags &= ~C_FLAG;
					templ = (templ << temp) | (regs[reg].l >> (32 - temp));
					setea32(templ);
					setznp32(templ);
				}
				cycles -= 3;
				break;
			case 0xA5: /*SHLD CL*/
				fetchea2();
				temp = CL;
				if(temp && temp < 32) {
					templ = getea32();
					if((templ << (temp - 1)) & 0x80000000) flags |= C_FLAG;
					else				   flags &= ~C_FLAG;
					templ = (templ << temp) | (regs[reg].l >> (32 - temp));
					setea32(templ);
					setznp32(templ);
				}
				cycles -= 3;
				break;
			case 0xAB: /*BTS r32*/
				fetchea2();
				templ = getea32();
				if(templ&(1 << (regs[reg].l & 31))) flags |= C_FLAG;
				else				  flags &= ~C_FLAG;
				templ |= (1 << (regs[reg].l & 31));
				setea32(templ);
				cycles -= 6;
				break;
			case 0xAC: /*SHRD imm*/
				fetchea2();
				temp = fetch8();
				if(temp && temp < 32) {
					templ = getea32();
					if((templ >> (temp - 1)) & 1) flags |= C_FLAG;
					else			  flags &= ~C_FLAG;
					templ = (templ >> temp) | (regs[reg].l << (32 - temp));
					setea32(templ);
					setznp32(templ);
				}
				cycles -= 3;
				break;
			case 0xAD: /*SHRD CL*/
				fetchea2();
				temp = CL;
				if(temp && temp < 32) {
					templ = getea32();
					if((templ >> (temp - 1)) & 1) flags |= C_FLAG;
					else			  flags &= ~C_FLAG;
					templ = (templ >> temp) | (regs[reg].l << (32 - temp));
					setea32(templ);
					setznp32(templ);
				}
				cycles -= 3;
				break;

			case 0xAF: /*IMUL reg32, rm32*/
				fetchea2();
				temp64i = (int64)(int32)regs[reg].l*(int64)(int32)getea32();
				regs[reg].l = temp64i & 0xFFFFFFFF;
				if((temp64i >> 32) && (temp64i >> 32) != -1) flags |= C_FLAG | V_FLAG;
				else					 flags &= ~(C_FLAG | V_FLAG);
				cycles -= 30;
				break;

			case 0xB3: /*BTR r16*/
				fetchea2();
				templ = getea32();
				if(templ&(1 << (regs[reg].l & 31))) flags |= C_FLAG;
				else				  flags &= ~C_FLAG;
				templ &= ~(1 << (regs[reg].l & 31));
				setea32(templ);
				cycles -= 6;
				break;

			case 0xB2: /*LSS*/
				fetchea2();
				regs[reg].l = RM32(easeg, eaaddr); 
				tempw = RM16(easeg, (eaaddr + 4)); 
				loadseg(tempw, &_ss);
				cycles -= 7;
				break;
			case 0xB4: /*LFS*/
				fetchea2();
				regs[reg].l = RM32(easeg, eaaddr); 
				tempw = RM16(easeg, (eaaddr + 4)); 
				loadseg(tempw, &_fs);
				cycles -= 7;
				break;
			case 0xB5: /*LGS*/
				fetchea2();
				regs[reg].l = RM32(easeg, eaaddr); 
				tempw = RM16(easeg, (eaaddr + 4)); 
				loadseg(tempw, &_gs);
				cycles -= 7;
				break;

			case 0xB6: /*MOVZX b*/
				fetchea2();
				regs[reg].l = getea8();
				cycles -= 3;
				break;
			case 0xB7: /*MOVZX w*/
				fetchea2();
				regs[reg].l = getea16();
				cycles -= 3;
				break;

			case 0xBA: /*MORE?!?!?!*/
				fetchea2();
				switch(rmdat & 0x38) {
				case 0x20: /*BT l, imm*/
					templ = getea32();
					temp = fetch8();
					if(templ&(1 << temp)) flags |= C_FLAG;
					else		 flags &= ~C_FLAG;
					cycles -= 6;
					break;
				case 0x28: /*BTS l, imm*/
					templ = getea32();
					temp = fetch8();
					if(templ&(1 << temp)) flags |= C_FLAG;
					else		 flags &= ~C_FLAG;
					templ |= (1 << temp);
					setea32(templ);
					cycles -= 6;
					break;
				case 0x30: /*BTR l, imm*/
					templ = getea32();
					temp = fetch8();
					if(templ&(1 << temp)) flags |= C_FLAG;
					else		 flags &= ~C_FLAG;
					templ &= ~(1 << temp);
					setea32(templ);
					cycles -= 6;
					break;
				case 0x38: /*BTC l, imm*/
					templ = getea32();
					temp = fetch8();
					if(templ&(1 << temp)) flags |= C_FLAG;
					else		 flags &= ~C_FLAG;
					templ ^=(1 << temp);
					setea32(templ);
					cycles -= 6;
					break;

				default:
					invalid();
					break;
				}
				break;

			case 0xBB: /*BTC r32*/
				fetchea2();
				templ = getea32();
				if(templ&(1 << (regs[reg].l & 31))) flags |= C_FLAG;
				else				  flags &= ~C_FLAG;
				templ ^=(1 << (regs[reg].l & 31));
				setea32(templ);
				cycles -= 6;
				break;

			case 0xBC: /*BSF l*/
				fetchea2();
				templ = getea32();
				if(!templ) {
					flags |= Z_FLAG;
				}
				else {
					for(tempi = 0;tempi < 32;tempi++) {
						cycles -= 3;
						if(templ&(1 << tempi)) {
							flags &= ~Z_FLAG;
							regs[reg].l = tempi;
							break;
						}
					}
				}
				cycles -= 10;
				break;
			case 0xBD: /*BSR l*/
				fetchea2();
				templ = getea32();
				if(!templ) {
					flags |= Z_FLAG;
				}
				else {
					for(tempi = 31;tempi >= 0;tempi--) {
						cycles -= 3;
						if(templ&(1 << tempi)) {
							flags &= ~Z_FLAG;
							regs[reg].l = tempi;
							break;
						}
					}
				}
				cycles -= 10;
				break;

			case 0xBE: /*MOVSX b*/
				fetchea2();
				regs[reg].l = getea8();
				if(regs[reg].l & 0x80) regs[reg].l |= 0xFFFFFF00;
				cycles -= 3;
				break;
			case 0xBF: /*MOVSX w*/
				fetchea2();
				regs[reg].l = getea16();
				if(regs[reg].l & 0x8000) regs[reg].l |= 0xFFFF0000;
				cycles -= 3;
				break;

			case 0xC8: case 0xC9: case 0xCA: case 0xCB: /*BSWAP*/
			case 0xCC: case 0xCD: case 0xCE: case 0xCF: /*486!!!*/
				regs[opcode & 7].l = (regs[opcode & 7].l >> 24) | ((regs[opcode & 7].l >> 8) & 0xFF00) | ((regs[opcode & 7].l << 8) & 0xFF0000) | ((regs[opcode & 7].l << 24) & 0xFF000000);
				cycles -= 3;
				break;

			default:
				invalid();
				break;
			}
			break;

		case 0x10: case 0x110: case 0x210: case 0x310: /*ADC 8, reg*/
			fetchea();
			temp = getea8();
			temp2 = getr8(reg);
			setadc8(temp, temp2);
			temp += temp2 + cflag;
			setea8(temp);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x11: case 0x211: /*ADC 16, reg*/
			fetchea();
			tempw = getea16();
			tempw2 = regs[reg].w;
			setadc16(tempw, tempw2);
			tempw += tempw2 + cflag;
			setea16(tempw);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x111: case 0x311: /*ADC 32, reg*/
			fetchea();
			templ = getea32();
			templ2 = regs[reg].l;
			setadc32(templ, templ2);
			templ += templ2 + cflag;
			setea32(templ);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x12: case 0x112: case 0x212: case 0x312: /*ADC reg, 8*/
			fetchea();
			temp = getea8();
			setadc8(getr8(reg), temp);
			setr8(reg, getr8(reg) + temp + cflag);
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x13: case 0x213: /*ADC reg, 16*/
			fetchea();
			tempw = getea16();
			setadc16(regs[reg].w, tempw);
			regs[reg].w += tempw + cflag;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x113: case 0x313: /*ADC reg, 32*/
			fetchea();
			templ = getea32();
			setadc32(regs[reg].l, templ);
			regs[reg].l += templ + cflag;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x14: case 0x114: case 0x214: case 0x314: /*ADC AL, #8*/
			temp = fetch8();
			setadc8(AL, temp);
			AL += temp + cflag;
			cycles -= 2;
			break;
		case 0x15: case 0x215: /*ADC AX, #16*/
			tempw = fetch16();
			setadc16(AX, tempw);
			AX += tempw + cflag;
			cycles -= 2;
			break;
		case 0x115: case 0x315: /*ADC EAX, #32*/
			templ = fetch32();
			setadc32(EAX, templ);
			EAX += templ + cflag;
			cycles -= 2;
			break;

		case 0x16: case 0x216: /*PUSH SS*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM16(ss, ESP - 2, SS);
				ESP -= 2;
			}
			else {
				WM16(ss, ((SP - 2) & 0xFFFF), SS);
				SP -= 2;
			}
			cycles -= 2;
			break;
		case 0x116: case 0x316: /*PUSH SS*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM32(ss, ESP - 4, SS);
				ESP -= 4;
			}
			else {
				WM32(ss, ((SP - 4) & 0xFFFF), SS);
				SP -= 4;
			}
			cycles -= 2;
			break;
		case 0x17: case 0x217: /*POP SS*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				tempw = RM16(ss, ESP);
				ESP += 2;
			}
			else {
				tempw = RM16(ss, SP);
				SP += 2;
			}
			loadseg(tempw, &_ss);
			cycles -= 7;
			break;
		case 0x117: case 0x317: /*POP SS*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				tempw = RM16(ss, ESP);
				ESP += 4;
			}
			else {
				tempw = RM16(ss, SP);
				SP += 4;
			}
			loadseg(tempw, &_ss);
			cycles -= 7;
			break;

		case 0x18: case 0x118: case 0x218: case 0x318: /*SBB 8, reg*/
			fetchea();
			temp = getea8();
			temp2 = getr8(reg);
			setsbc8(temp, temp2);
			temp -= (temp2 + cflag);
			setea8(temp);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x19: case 0x219: /*SBB 16, reg*/
			fetchea();
			tempw = getea16();
			tempw2 = regs[reg].w;
			setsbc16(tempw, tempw2);
			tempw -= (tempw2 + cflag);
			setea16(tempw);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x119: case 0x319: /*SBB 32, reg*/
			fetchea();
			templ = getea32();
			templ2 = regs[reg].l;
			setsbc32(templ, templ2);
			templ -= (templ2 + cflag);
			setea32(templ);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x1A: case 0x11A: case 0x21A: case 0x31A: /*SBB reg, 8*/
			fetchea();
			temp = getea8();
			setsbc8(getr8(reg), temp);
			setr8(reg, getr8(reg) - (temp + cflag));
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x1B: case 0x21B: /*SBB reg, 16*/
			fetchea();
			tempw = getea16();
			tempw2 = regs[reg].w;
			setsbc16(tempw2, tempw);
			tempw2 -= (tempw + cflag);
			regs[reg].w = tempw2;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x11B: case 0x31B: /*SBB reg, 32*/
			fetchea();
			templ = getea32();
			templ2 = regs[reg].l;
			setsbc32(templ2, templ);
			templ2 -= (templ + cflag);
			regs[reg].l = templ2;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x1C: case 0x11C: case 0x21C: case 0x31C: /*SBB AL, #8*/
			temp = fetch8();
			setsbc8(AL, temp);
			AL -= (temp + cflag);
			cycles -= 2;
			break;
		case 0x1D: case 0x21D: /*SBB AX, #16*/
			tempw = fetch16();
			setsbc16(AX, tempw);
			AX -= (tempw + cflag);
			cycles -= 2;
			break;
		case 0x11D: case 0x31D: /*SBB AX, #32*/
			templ = fetch32();
			setsbc32(EAX, templ);
			EAX -= (templ + cflag);
			cycles -= 2;
			break;

		case 0x1E: case 0x21E: /*PUSH DS*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM16(ss, ESP - 2, DS);
				ESP -= 2;
			}
			else {
				WM16(ss, ((SP - 2) & 0xFFFF), DS);
				SP -= 2;
			}
			cycles -= 2;
			break;
		case 0x11E: case 0x31E: /*PUSH DS*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM32(ss, ESP - 4, DS);
				ESP -= 4;
			}
			else {
				WM32(ss, ((SP - 4) & 0xFFFF), DS);
				SP -= 4;
			}
			cycles -= 2;
			break;
		case 0x1F: case 0x21F: /*POP DS*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				tempw = RM16(ss, ESP);
				ESP += 2;
			}
			else {
				tempw = RM16(ss, SP);
				SP += 2;
			}
			loadseg(tempw, &_ds);
			cycles -= 7;
			break;
		case 0x11F: case 0x31F: /*POP DS*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				tempw = RM16(ss, ESP);
				ESP += 4;
			}
			else {
				tempw = RM16(ss, SP);
				SP += 4;
			}
			loadseg(tempw, &_ds);
			cycles -= 7;
			break;

		case 0x20: case 0x120: case 0x220: case 0x320: /*AND 8, reg*/
			fetchea();
			temp = getea8();
			temp &= getr8(reg);
			setznp8(temp);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			setea8(temp);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x21: case 0x221: /*AND 16, reg*/
			fetchea();
			tempw = getea16();
			tempw &= regs[reg].w;
			setznp16(tempw);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			setea16(tempw);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x121: case 0x321: /*AND 32, reg*/
			fetchea();
			templ = getea32();
			templ &= regs[reg].l;
			setznp32(templ);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			setea32(templ);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x22: case 0x122: case 0x222: case 0x322: /*AND reg, 8*/
			fetchea();
			temp = getea8();
			temp &= getr8(reg);
			setznp8(temp);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			setr8(reg, temp);
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x23: case 0x223: /*AND reg, 16*/
			fetchea();
			tempw = getea16();
			tempw &= regs[reg].w;
			setznp16(tempw);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			regs[reg].w = tempw;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x123: case 0x323: /*AND reg, 32*/
			fetchea();
			templ = getea32();
			templ &= regs[reg].l;
			setznp32(templ);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			regs[reg].l = templ;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x24: case 0x124: case 0x224: case 0x324: /*AND AL, #8*/
			AL &= fetch8();
			setznp8(AL);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= 2;
			break;
		case 0x25: case 0x225: /*AND AX, #16*/
			AX &= fetch16();
			setznp16(AX);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= 2;
			break;
		case 0x125: case 0x325: /*AND EAX, #32*/
			EAX &= fetch32();
			setznp32(EAX);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= 2;
			break;

		case 0x26: case 0x126: case 0x226: case 0x326: /*ES:*/
			oldss = ss;
			oldds = ds;
			ds = ss = es;
			ssegs = 2;
			cycles -= 4;
			goto opcodestart;

		case 0x27: case 0x127: case 0x227: case 0x327: /*DAA*/
			if((flags & A_FLAG) || ((AL & 0xF) > 9)) {
				tempi = ((uint16)AL) + 6;
				AL += 6;
				flags |= A_FLAG;
				if(tempi & 0x100) flags |= C_FLAG;
			}
			if((flags & C_FLAG) || (AL > 0x9F)) {
				AL += 0x60;
				flags |= C_FLAG;
			}
			setznp8(AL);
			cycles -= 4;
			break;

		case 0x28: case 0x128: case 0x228: case 0x328: /*SUB 8, reg*/
			fetchea();
			temp = getea8();
			setsub8(temp, getr8(reg));
			temp -= getr8(reg);
			setea8(temp);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x29: case 0x229: /*SUB 16, reg*/
			fetchea();
			tempw = getea16();
			setsub16(tempw, regs[reg].w);
			tempw -= regs[reg].w;
			setea16(tempw);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x129: case 0x329: /*SUB 32, reg*/
			fetchea();
			templ = getea32();
			setsub32(templ, regs[reg].l);
			templ -= regs[reg].l;
			setea32(templ);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x2A: case 0x12A: case 0x22A: case 0x32A: /*SUB reg, 8*/
			fetchea();
			temp = getea8();
			setsub8(getr8(reg), temp);
			setr8(reg, getr8(reg) - temp);
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x2B: case 0x22B: /*SUB reg, 16*/
			fetchea();
			tempw = getea16();
			setsub16(regs[reg].w, tempw);
			regs[reg].w -= tempw;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x12B: case 0x32B: /*SUB reg, 32*/
			fetchea();
			templ = getea32();
			setsub32(regs[reg].l, templ);
			regs[reg].l -= templ;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x2C: case 0x12C: case 0x22C: case 0x32C: /*SUB AL, #8*/
			temp = fetch8();
			setsub8(AL, temp);
			AL -= temp;
			cycles -= 2;
			break;
		case 0x2D: case 0x22D: /*SUB AX, #16*/
			tempw = fetch16();
			setsub16(AX, tempw);
			AX -= tempw;
			cycles -= 2;
			break;
		case 0x12D: case 0x32D: /*SUB EAX, #32*/
			templ = fetch32();
			setsub32(EAX, templ);
			EAX -= templ;
			cycles -= 2;
			break;
		case 0x2E: case 0x12E: case 0x22E: case 0x32E: /*CS:*/
			oldss = ss;
			oldds = ds;
			ds = ss = cs;
			ssegs = 2;
			cycles -= 4;
			goto opcodestart;
		case 0x2F: case 0x12F: case 0x22F: case 0x32F: /*DAS*/
			if((flags & A_FLAG) || ((AL & 0xF) > 9)) {
				tempi = ((uint16)AL) - 6;
				AL -= 6;
				flags |= A_FLAG;
				if(tempi & 0x100) flags |= C_FLAG;
			}
			if((flags & C_FLAG) || (AL > 0x9F)) {
				AL -= 0x60;
				flags |= C_FLAG;
			}
			setznp8(AL);
			cycles -= 4;
			break;
		case 0x30: case 0x130: case 0x230: case 0x330: /*XOR 8, reg*/
			fetchea();
			temp = getea8();
			temp ^=getr8(reg);
			setznp8(temp);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			setea8(temp);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x31: case 0x231: /*XOR 16, reg*/
			fetchea();
			tempw = getea16();
			tempw ^=regs[reg].w;
			setznp16(tempw);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			setea16(tempw);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x131: case 0x331: /*XOR 32, reg*/
			fetchea();
			templ = getea32();
			templ ^=regs[reg].l;
			setznp32(templ);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			setea32(templ);
			cycles -= ((mod == 3) ? 2:7);
			break;
		case 0x32: case 0x132: case 0x232: case 0x332: /*XOR reg, 8*/
			fetchea();
			temp = getea8();
			temp ^=getr8(reg);
			setznp8(temp);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			setr8(reg, temp);
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x33: case 0x233: /*XOR reg, 16*/
			fetchea();
			tempw = getea16();
			tempw ^=regs[reg].w;
			setznp16(tempw);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			regs[reg].w = tempw;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x133: case 0x333: /*XOR reg, 32*/
			fetchea();
			templ = getea32();
			templ ^=regs[reg].l;
			setznp32(templ);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			regs[reg].l = templ;
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x34: case 0x134: case 0x234: case 0x334: /*XOR AL, #8*/
			AL ^=fetch8();
			setznp8(AL);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= 2;
			break;
		case 0x35: case 0x235: /*XOR AX, #16*/
			AX ^=fetch16();
			setznp16(AX);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= 2;
			break;
		case 0x135: case 0x335: /*XOR EAX, #32*/
			EAX ^=fetch32();
			setznp32(EAX);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= 2;
			break;

		case 0x36: case 0x136: case 0x236: case 0x336: /*SS:*/
			oldss = ss;
			oldds = ds;
			ds = ss = ss;
			ssegs = 2;
			cycles -= 4;
			goto opcodestart;

		case 0x37: case 0x137: case 0x237: case 0x337: /*AAA*/
			if((flags & A_FLAG) || ((AL & 0xF) > 9)) {
				AL += 6;
				AH++;
				flags |= (A_FLAG | C_FLAG);
			}
			else
				flags &= ~(A_FLAG | C_FLAG);
			AL &= 0xF;
			cycles -= 4;
			break;

		case 0x38: case 0x138: case 0x238: case 0x338: /*CMP 8, reg*/
			fetchea();
			temp = getea8();
			setsub8(temp, getr8(reg));
			cycles -= ((mod == 3) ? 2 : 5);
			break;
		case 0x39: case 0x239: /*CMP 16, reg*/
			fetchea();
			tempw = getea16();
			setsub16(tempw, regs[reg].w);
			cycles -= ((mod == 3) ? 2 : 5);
			break;
		case 0x139: case 0x339: /*CMP 32, reg*/
			fetchea();
			templ = getea32();
			setsub32(templ, regs[reg].l);
			cycles -= ((mod == 3) ? 2 : 5);
			break;
		case 0x3A: case 0x13A: case 0x23A: case 0x33A: /*CMP reg, 8*/
			fetchea();
			temp = getea8();
			setsub8(getr8(reg), temp);
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x3B: case 0x23B: /*CMP reg, 16*/
			fetchea();
			tempw = getea16();
			setsub16(regs[reg].w, tempw);
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x13B: case 0x33B: /*CMP reg, 32*/
			fetchea();
			templ = getea32();
			setsub32(regs[reg].l, templ);
			cycles -= ((mod == 3) ? 2:6);
			break;
		case 0x3C: case 0x13C: case 0x23C: case 0x33C: /*CMP AL, #8*/
			temp = fetch8();
			setsub8(AL, temp);
			cycles -= 2;
			break;
		case 0x3D: case 0x23D: /*CMP AX, #16*/
			tempw = fetch16();
			setsub16(AX, tempw);
			cycles -= 2;
			break;
		case 0x13D: case 0x33D: /*CMP EAX, #32*/
			templ = fetch32();
			setsub32(EAX, templ);
			cycles -= 2;
			break;

		case 0x3E: case 0x13E: case 0x23E: case 0x33E: /*DS:*/
			oldss = ss;
			oldds = ds;
			ds = ss = ds;
			ssegs = 2;
			cycles -= 4;
			goto opcodestart;

		case 0x3F: case 0x13F: case 0x23F: case 0x33F: /*AAS*/
			if((flags & A_FLAG) || ((AL & 0xF) > 9)) {
				AL -= 6;
				AH--;
				flags |= (A_FLAG | C_FLAG);
			}
			else
				flags &= ~(A_FLAG | C_FLAG);
			AL &= 0xF;
			cycles -= 4;
			break;

		case 0x40: case 0x41: case 0x42: case 0x43: /*INC r16*/
		case 0x44: case 0x45: case 0x46: case 0x47:
		case 0x240: case 0x241: case 0x242: case 0x243:
		case 0x244: case 0x245: case 0x246: case 0x247:
			setadd16nc(regs[opcode & 7].w, 1);
			regs[opcode & 7].w++;
			cycles -= 2;
			break;
		case 0x140: case 0x141: case 0x142: case 0x143: /*INC r32*/
		case 0x144: case 0x145: case 0x146: case 0x147:
		case 0x340: case 0x341: case 0x342: case 0x343:
		case 0x344: case 0x345: case 0x346: case 0x347:
			setadd32nc(regs[opcode & 7].l, 1);
			regs[opcode & 7].l++;
			cycles -= 2;
			break;
		case 0x48: case 0x49: case 0x4A: case 0x4B: /*DEC r16*/
		case 0x4C: case 0x4D: case 0x4E: case 0x4F:
		case 0x248: case 0x249: case 0x24A: case 0x24B:
		case 0x24C: case 0x24D: case 0x24E: case 0x24F:
			setsub16nc(regs[opcode & 7].w, 1);
			regs[opcode & 7].w--;
			cycles -= 2;
			break;
		case 0x148: case 0x149: case 0x14A: case 0x14B: /*DEC r32*/
		case 0x14C: case 0x14D: case 0x14E: case 0x14F:
		case 0x348: case 0x349: case 0x34A: case 0x34B:
		case 0x34C: case 0x34D: case 0x34E: case 0x34F:
			setsub32nc(regs[opcode & 7].l, 1);
			regs[opcode & 7].l--;
			cycles -= 2;
			break;

		case 0x50: case 0x51: case 0x52: case 0x53: /*PUSH r16*/
		case 0x54: case 0x55: case 0x56: case 0x57:
		case 0x250: case 0x251: case 0x252: case 0x253:
		case 0x254: case 0x255: case 0x256: case 0x257:
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM16(ss, ESP - 2, regs[opcode & 7].w);
				ESP -= 2;
			}
			else {
				WM16(ss, (SP - 2) & 0xFFFF, regs[opcode & 7].w);
				SP -= 2;
			}
			cycles -= 2;
			break;
		case 0x150: case 0x151: case 0x152: case 0x153: /*PUSH r32*/
		case 0x154: case 0x155: case 0x156: case 0x157:
		case 0x350: case 0x351: case 0x352: case 0x353:
		case 0x354: case 0x355: case 0x356: case 0x357:
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM32(ss, ESP - 4, regs[opcode & 7].l);
				ESP -= 4;
			}
			else {
				WM32(ss, (SP - 4) & 0xFFFF, regs[opcode & 7].l);
				SP -= 4;
			}
			cycles -= 2;
			break;
		case 0x58: case 0x59: case 0x5A: case 0x5B: /*POP r16*/
		case 0x5C: case 0x5D: case 0x5E: case 0x5F:
		case 0x258: case 0x259: case 0x25A: case 0x25B:
		case 0x25C: case 0x25D: case 0x25E: case 0x25F:
			if(ssegs)
				ss = oldss;
			if(stack32) {
				ESP += 2;
				regs[opcode & 7].w = RM16(ss, ESP - 2);
			}
			else {
				SP += 2;
				regs[opcode & 7].w = RM16(ss, (SP - 2) & 0xFFFF);
			}
			cycles -= 5;
			break;
		case 0x158: case 0x159: case 0x15A: case 0x15B: /*POP r32*/
		case 0x15C: case 0x15D: case 0x15E: case 0x15F:
		case 0x358: case 0x359: case 0x35A: case 0x35B:
		case 0x35C: case 0x35D: case 0x35E: case 0x35F:
			if(ssegs)
				ss = oldss;
			if(stack32) {
				ESP += 4;
				regs[opcode & 7].l = RM32(ss, ESP - 4);
			}
			else {
				SP += 4;
				regs[opcode & 7].l = RM32(ss, (SP - 4) & 0xFFFF);
			}
			cycles -= 5;
			break;

		case 0x60: case 0x260: /*PUSHA*/
			if(stack32) {
				WM16(ss, ESP - 2, AX);
				WM16(ss, ESP - 4, CX);
				WM16(ss, ESP - 6, DX);
				WM16(ss, ESP - 8, BX);
				WM16(ss, ESP - 10, SP);
				WM16(ss, ESP - 12, BP);
				WM16(ss, ESP - 14, SI);
				WM16(ss, ESP - 16, DI);
				ESP -= 16;
			}
			else {
				WM16(ss, ((SP - 2) & 0xFFFF), AX);
				WM16(ss, ((SP - 4) & 0xFFFF), CX);
				WM16(ss, ((SP - 6) & 0xFFFF), DX);
				WM16(ss, ((SP - 8) & 0xFFFF), BX);
				WM16(ss, ((SP - 10) & 0xFFFF), SP);
				WM16(ss, ((SP - 12) & 0xFFFF), BP);
				WM16(ss, ((SP - 14) & 0xFFFF), SI);
				WM16(ss, ((SP - 16) & 0xFFFF), DI);
				SP -= 16;
			}
			cycles -= 18;
			break;
		case 0x61: case 0x261: /*POPA*/
			if(stack32) {
				DI = RM16(ss, ESP);
				SI = RM16(ss, ESP + 2);
				BP = RM16(ss, ESP + 4);
				BX = RM16(ss, ESP + 8);
				DX = RM16(ss, ESP + 10);
				CX = RM16(ss, ESP + 12);
				AX = RM16(ss, ESP + 14);
				ESP += 16;
			}
			else {
				DI = RM16(ss, ((SP) & 0xFFFF));
				SI = RM16(ss, ((SP + 2) & 0xFFFF));
				BP = RM16(ss, ((SP + 4) & 0xFFFF));
				BX = RM16(ss, ((SP + 8) & 0xFFFF));
				DX = RM16(ss, ((SP + 10) & 0xFFFF));
				CX = RM16(ss, ((SP + 12) & 0xFFFF));
				AX = RM16(ss, ((SP + 14) & 0xFFFF));
				SP += 16;
			}
			cycles -= 24;
			break;
		case 0x160: case 0x360: /*PUSHA*/
			if(stack32) {
				WM32(ss, ESP - 4, EAX);
				WM32(ss, ESP - 8, ECX);
				WM32(ss, ESP - 12, EDX);
				WM32(ss, ESP - 16, EBX);
				WM32(ss, ESP - 20, ESP);
				WM32(ss, ESP - 24, EBP);
				WM32(ss, ESP - 28, ESI);
				WM32(ss, ESP - 32, EDI);
				ESP -= 32;
			}
			else {
				WM32(ss, ((SP - 4) & 0xFFFF), EAX);
				WM32(ss, ((SP - 8) & 0xFFFF), ECX);
				WM32(ss, ((SP - 12) & 0xFFFF), EDX);
				WM32(ss, ((SP - 16) & 0xFFFF), EBX);
				WM32(ss, ((SP - 20) & 0xFFFF), ESP);
				WM32(ss, ((SP - 24) & 0xFFFF), EBP);
				WM32(ss, ((SP - 28) & 0xFFFF), ESI);
				WM32(ss, ((SP - 32) & 0xFFFF), EDI);
				SP -= 32;
			}
			cycles -= 18;
			break;
		case 0x161: case 0x361: /*POPA*/
			if(stack32) {
				EDI = RM32(ss, ESP);
				ESI = RM32(ss, ESP + 4);
				EBP = RM32(ss, ESP + 8);
				EBX = RM32(ss, ESP + 16);
				EDX = RM32(ss, ESP + 20);
				ECX = RM32(ss, ESP + 24);
				EAX = RM32(ss, ESP + 28);
				ESP += 32;
			}
			else {
				EDI = RM32(ss, ((SP) & 0xFFFF));
				ESI = RM32(ss, ((SP + 4) & 0xFFFF));
				EBP = RM32(ss, ((SP + 8) & 0xFFFF));
				EBX = RM32(ss, ((SP + 16) & 0xFFFF));
				EDX = RM32(ss, ((SP + 20) & 0xFFFF));
				ECX = RM32(ss, ((SP + 24) & 0xFFFF));
				EAX = RM32(ss, ((SP + 28) & 0xFFFF));
				SP += 32;
			}
			cycles -= 24;
			break;
		case 0x62: case 0x262: /*BOUND r16 m16 m16*/
			fetchea();
			low = (int)(int16)getea16();
			eaaddr += 2;
			high = (int)(int16)getea16();
			tempws = (int)(int16)regs[reg].w;
			if(tempws < low || tempws > high) {
				pc -= 2;
				interrupt(BOUNDS_CHECK_FAULT, 0);
				cycles -= 11;
			}
			else
				cycles -= 10;
			break;
		case 0x162: case 0x362: /*BOUND r32 m32 m32*/
			fetchea();
			low = (int)(int32)getea32();
			eaaddr += 4;
			high = (int)(int32)getea32();
			tempws = (int)(int32)regs[reg].l;
			if(tempws < low || tempws > high) {
				pc -= 2;
				interrupt(BOUNDS_CHECK_FAULT, 0);
				cycles -= 11;
			}
			else
				cycles -= 10;
			break;
		case 0x63: /*ARPL*/
			if(msw & 1) {
				fetchea();
				tempw = getea16();
				if((tempw & 3) < (regs[reg].w & 3)) {
					tempw = (tempw & 0xFFFC) | (regs[reg].w & 3);
					setea16(tempw);
					flags |= Z_FLAG;
				}
				else
					flags &= ~Z_FLAG;
				cycles -= 20;
			}
			else
				invalid();
			break;
		case 0x64: case 0x164: case 0x264: case 0x364: /*FS:*/
			oldss = ss;
			oldds = ds;
			ds = ss = fs;
			ssegs = 2;
			cycles -= 4;
			goto opcodestart;
		case 0x65: case 0x165: case 0x265: case 0x365: /*GS:*/
			oldss = ss;
			oldds = ds;
			ds = ss = gs;
			ssegs = 2;
			cycles -= 4;
			goto opcodestart;

		case 0x66: case 0x166: case 0x266: case 0x366: /*Data size select*/
			op32 = ((use32 & 0x100) ^ 0x100) | (op32 & 0x200);
			cycles -= 2;
			goto opcodestart;
		case 0x67: case 0x167: case 0x267: case 0x367: /*Address size select*/
			op32 = ((use32 & 0x200) ^ 0x200) | (op32 & 0x100);
			cycles -= 2;
			goto opcodestart;

		case 0x68: case 0x268: /*PUSH #w*/
			tempw = fetch16();
			if(stack32) {
				WM16(ss, ESP - 2, tempw);
				ESP -= 2;
			}
			else {
				WM16(ss, ((SP - 2) & 0xFFFF), tempw);
				SP -= 2;
			}
			cycles -= 2;
			break;
		case 0x168: case 0x368: /*PUSH #l*/
			templ = fetch32();
			if(stack32) {
				WM32(ss, ESP - 4, templ);
				ESP -= 4;
			}
			else {
				WM32(ss, ((SP - 4) & 0xFFFF), templ);
				SP -= 4;
			}
			cycles -= 2;
			break;
		case 0x69: case 0x269: /*IMUL r16*/
			fetchea();
			tempw = getea16();
			tempw2 = fetch16();
			templ = ((int)(int16)tempw)*((int)(int16)tempw2);
			if((templ >> 16) != 0 && (templ >> 16) != 0xFFFF) flags |= C_FLAG | V_FLAG;
			else					    flags &= ~(C_FLAG | V_FLAG);
			regs[reg].w = templ & 0xFFFF;
			cycles -= ((mod == 3) ? 14 : 17);
			break;
		case 0x169: case 0x369: /*IMUL r32*/
			fetchea();
			templ = getea32();
			templ2 = fetch32();
			temp64i = ((int)(int32)templ)*((int)(int32)templ2);
			if((temp64i >> 32) != 0 && (temp64i >> 32) != -1) flags |= C_FLAG | V_FLAG;
			else					    flags &= ~(C_FLAG | V_FLAG);
			regs[reg].l = temp64i & 0xFFFFFFFF;
			cycles -= 25;
			break;
		case 0x6A: case 0x26A:/*PUSH #eb*/
			tempw = fetch8();
			if(tempw & 0x80) tempw |= 0xFF00;
			if(stack32) {
				WM16(ss, ESP - 2, tempw);
				ESP -= 2;
			}
			else {
				WM16(ss, ((SP - 2) & 0xFFFF), tempw);
				SP -= 2;
			}
			cycles -= 2;
			break;
		case 0x16A: case 0x36A:/*PUSH #eb*/
			templ = fetch8();
			if(templ & 0x80) templ |= 0xFFFFFF00;
			if(stack32) {
				WM32(ss, ESP - 4, templ);
				ESP -= 4;
			}
			else {
				WM32(ss, ((SP - 4) & 0xFFFF), templ);
				SP -= 4;
			}
			cycles -= 2;
			break;
		case 0x6B: case 0x26B: /*IMUL r8*/
			fetchea();
			tempw = getea16();
			tempw2 = fetch8();
			if(tempw2 & 0x80) tempw2 |= 0xFF00;
			templ = ((int)(int16)tempw)*((int)(int16)tempw2);
			if((templ >> 16) != 0 && ((templ >> 16) & 0xFFFF) != 0xFFFF) flags |= C_FLAG | V_FLAG;
			else						flags &= ~(C_FLAG | V_FLAG);
			regs[reg].w = templ & 0xFFFF;
			cycles -= ((mod == 3) ? 14 : 17);
			break;
		case 0x16B: case 0x36B: /*IMUL r8*/
			fetchea();
			templ = getea32();
			templ2 = fetch8();
			if(templ2 & 0x80) templ2 |= 0xFFFFFF00;
			temp64i = ((int64)(int32)templ)*((int64)(int32)templ2);
			if((temp64i >> 32) != 0 && (temp64i >> 32) != -1) flags |= C_FLAG | V_FLAG;
			else					    flags &= ~(C_FLAG | V_FLAG);
			regs[reg].l = temp64i & 0xFFFFFFFF;
			cycles -= 20;
			break;
		case 0x6C: case 0x16C: /*INSB*/
			temp = IN8(DX);
			WM8(es, DI, temp);
			if(flags & D_FLAG) DI--;
			else		   DI++;
			cycles -= 15;
			break;
		case 0x26C: case 0x36C: /*INSB*/
			temp = IN8(DX);
			WM8(es, EDI, temp);
			if(flags & D_FLAG) EDI--;
			else		   EDI++;
			cycles -= 15;
			break;
		case 0x6E: case 0x16E: /*OUTSB*/
			temp = RM8(ds, SI);
			if(flags & D_FLAG) SI--;
			else		   SI++;
			OUT8(DX, temp);
			cycles -= 14;
			break;
		case 0x26E: case 0x36E: /*OUTSB*/
			temp = RM8(ds, ESI);
			if(flags & D_FLAG) ESI--;
			else		   ESI++;
			OUT8(DX, temp);
			cycles -= 14;
			break;
		case 0x6F: /*OUTSW*/
			tempw = RM16(ds, SI);
			if(flags & D_FLAG) SI -= 2;
			else		   SI += 2;
			OUT16(DX, tempw);
			cycles -= 14;
			break;
		case 0x26F: /*OUTSW*/
			tempw = RM16(ds, ESI);
			if(flags & D_FLAG) ESI -= 2;
			else		   ESI += 2;
			OUT16(DX, tempw);
			cycles -= 14;
			break;


		case 0x70: case 0x170: case 0x270: case 0x370: /*JO*/
			offset = (int8)fetch8();
			if(flags & V_FLAG) { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x71: case 0x171: case 0x271: case 0x371: /*JNO*/
			offset = (int8)fetch8();
			if(!(flags & V_FLAG)) { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x72: case 0x172: case 0x272: case 0x372: /*JB*/
			offset = (int8)fetch8();
			if(flags & C_FLAG) { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x73: case 0x173: case 0x273: case 0x373: /*JNB*/
			offset = (int8)fetch8();
			if(!(flags & C_FLAG)) { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x74: case 0x174: case 0x274: case 0x374: /*JZ*/
			offset = (int8)fetch8();
			if(flags & Z_FLAG) { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x75: case 0x175: case 0x275: case 0x375: /*JNZ*/
			offset = (int8)fetch8();
			if(!(flags & Z_FLAG)) { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x76: case 0x176: case 0x276: case 0x376: /*JBE*/
			offset = (int8)fetch8();
			if(flags & (C_FLAG | Z_FLAG)) { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x77: case 0x177: case 0x277: case 0x377: /*JNBE*/
			offset = (int8)fetch8();
			if(!(flags & (C_FLAG | Z_FLAG))) { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x78: case 0x178: case 0x278: case 0x378: /*JS*/
			offset = (int8)fetch8();
			if(flags & N_FLAG)  { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x79: case 0x179: case 0x279: case 0x379: /*JNS*/
			offset = (int8)fetch8();
			if(!(flags & N_FLAG))  { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x7A: case 0x17A: case 0x27A: case 0x37A: /*JP*/
			offset = (int8)fetch8();
			if(flags & P_FLAG)  { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x7B: case 0x17B: case 0x27B: case 0x37B: /*JNP*/
			offset = (int8)fetch8();
			if(!(flags & P_FLAG))  { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x7C: case 0x17C: case 0x27C: case 0x37C: /*JL*/
			offset = (int8)fetch8();
			temp = (flags & N_FLAG) ? 1 : 0;
			temp2 = (flags & V_FLAG) ? 1 : 0;
			if(temp != temp2)  { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x7D: case 0x17D: case 0x27D: case 0x37D: /*JNL*/
			offset = (int8)fetch8();
			temp = (flags & N_FLAG) ? 1 : 0;
			temp2 = (flags & V_FLAG) ? 1 : 0;
			if(temp == temp2)  { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x7E: case 0x17E: case 0x27E: case 0x37E: /*JLE*/
			offset = (int8)fetch8();
			temp = (flags & N_FLAG) ? 1 : 0;
			temp2 = (flags & V_FLAG) ? 1 : 0;
			if((flags & Z_FLAG) || (temp != temp2))  { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;
		case 0x7F: case 0x17F: case 0x27F: case 0x37F: /*JNLE*/
			offset = (int8)fetch8();
			temp = (flags & N_FLAG) ? 1 : 0;
			temp2 = (flags & V_FLAG) ? 1 : 0;
			if(!((flags & Z_FLAG) || (temp != temp2)))  { pc += offset; cycles -= 4; }
			cycles -= 3;
			break;

		case 0x80: case 0x180: case 0x280: case 0x380:
		case 0x82: case 0x182: case 0x282: case 0x382:
			fetchea();
			temp = getea8();
			temp2 = fetch8();
			switch(rmdat & 0x38) {
			case 0x00: /*ADD b, #8*/
				setadd8(temp, temp2);
				setea8(temp + temp2);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x08: /*OR b, #8*/
				temp |= temp2;
				setznp8(temp);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				setea8(temp);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x10: /*ADC b, #8*/
				setadc8(temp, temp2);
				setea8(temp + temp2 + cflag);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x18: /*SBB b, #8*/
				setsbc8(temp, temp2);
				setea8(temp - (temp2 + cflag));
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x20: /*AND b, #8*/
				temp &= temp2;
				setznp8(temp);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				setea8(temp);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x28: /*SUB b, #8*/
				setsub8(temp, temp2);
				setea8(temp - temp2);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x30: /*XOR b, #8*/
				temp ^=temp2;
				setznp8(temp);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				setea8(temp);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x38: /*CMP b, #8*/
				setsub8(temp, temp2);
				cycles -= ((mod == 3) ? 2:7);
				break;

			default:
//				invalid();
				break;
			}
			break;

		case 0x81: case 0x281:
			fetchea();
			tempw = getea16();
			tempw2 = fetch16();
			switch(rmdat & 0x38) {
			case 0x00: /*ADD w, #16*/
				setadd16(tempw, tempw2);
				tempw += tempw2;
				setea16(tempw);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x08: /*OR w, #16*/
				tempw |= tempw2;
				setznp16(tempw);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				setea16(tempw);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x10: /*ADC w, #16*/
				setadc16(tempw, tempw2);
				tempw += tempw2 + cflag;
				setea16(tempw);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x20: /*AND w, #16*/
				tempw &= tempw2;
				setznp16(tempw);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				setea16(tempw);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x18: /*SBB w, #16*/
				setsbc16(tempw, tempw2);
				setea16(tempw - (tempw2 + cflag));
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x28: /*SUB w, #16*/
				setsub16(tempw, tempw2);
				tempw -= tempw2;
				setea16(tempw);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x30: /*XOR w, #16*/
				tempw ^=tempw2;
				setznp16(tempw);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				setea16(tempw);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x38: /*CMP w, #16*/
				setsub16(tempw, tempw2);
				cycles -= ((mod == 3) ? 2:7);
				break;

			default:
//				invalid();
				break;
			}
			break;
		case 0x181: case 0x381:
			fetchea();
			templ = getea32();
			templ2 = fetch32();
			switch(rmdat & 0x38)
			{
			case 0x00: /*ADD l, #32*/
				setadd32(templ, templ2);
				templ += templ2;
				setea32(templ);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x08: /*OR l, #32*/
				templ |= templ2;
				setznp32(templ);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				setea32(templ);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x10: /*ADC l, #32*/
				setadc32(templ, templ2);
				templ += templ2 + cflag;
				setea32(templ);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x20: /*AND l, #32*/
				templ &= templ2;
				setznp32(templ);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				setea32(templ);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x18: /*SBB l, #32*/
				setsbc32(templ, templ2);
				setea32(templ - (templ2 + cflag));
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x28: /*SUB l, #32*/
				setsub32(templ, templ2);
				templ -= templ2;
				setea32(templ);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x30: /*XOR l, #32*/
				templ ^=templ2;
				setznp32(templ);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				setea32(templ);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x38: /*CMP l, #32*/
				setsub32(templ, templ2);
				cycles -= ((mod == 3) ? 2:7);
				break;

			default:
//				invalid();
				break;
			}
			break;

		case 0x83: case 0x283:
			fetchea();
			tempw = getea16();
			tempw2 = fetch8();
			if(tempw2 & 0x80) tempw2 |= 0xFF00;
			switch(rmdat & 0x38) {
			case 0x00: /*ADD w, #8*/
				setadd16(tempw, tempw2);
				tempw += tempw2;
				setea16(tempw);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x08: /*OR w, #8*/
				tempw |= tempw2;
				setznp16(tempw);
				setea16(tempw);
				flags &= ~(C_FLAG | A_FLAG | V_FLAG);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x10: /*ADC w, #8*/
				setadc16(tempw, tempw2);
				tempw += tempw2 + cflag;
				setea16(tempw);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x18: /*SBB w, #8*/
				setsbc16(tempw, tempw2);
				tempw -= (tempw2 + cflag);
				setea16(tempw);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x20: /*AND w, #8*/
				tempw &= tempw2;
				setznp16(tempw);
				setea16(tempw);
				flags &= ~(C_FLAG | A_FLAG | V_FLAG);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x28: /*SUB w, #8*/
				setsub16(tempw, tempw2);
				tempw -= tempw2;
				setea16(tempw);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x30: /*XOR w, #8*/
				tempw ^=tempw2;
				setznp16(tempw);
				setea16(tempw);
				flags &= ~(C_FLAG | A_FLAG | V_FLAG);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x38: /*CMP w, #8*/
				setsub16(tempw, tempw2);
				cycles -= ((mod == 3) ? 2:7);
				break;

			default:
//				invalid();
				break;
			}
			break;
		case 0x183: case 0x383:
			fetchea();
			templ = getea32();
			templ2 = fetch8();
			if(templ2 & 0x80) templ2 |= 0xFFFFFF00;
			switch(rmdat & 0x38) {
			case 0x00: /*ADD l, #32*/
				setadd32(templ, templ2);
				templ += templ2;
				setea32(templ);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x08: /*OR l, #32*/
				templ |= templ2;
				setznp32(templ);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				setea32(templ);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x10: /*ADC l, #32*/
				setadc32(templ, templ2);
				templ += templ2 + cflag;
				setea32(templ);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x20: /*AND l, #32*/
				templ &= templ2;
				setznp32(templ);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				setea32(templ);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x18: /*SBB l, #32*/
				setsbc32(templ, templ2);
				setea32(templ - (templ2 + cflag));
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x28: /*SUB l, #32*/
				setsub32(templ, templ2);
				templ -= templ2;
				setea32(templ);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x30: /*XOR l, #32*/
				templ ^=templ2;
				setznp32(templ);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				setea32(templ);
				cycles -= ((mod == 3) ? 2:7);
				break;
			case 0x38: /*CMP l, #32*/
				setsub32(templ, templ2);
				cycles -= ((mod == 3) ? 2:7);
				break;

			default:
//				invalid();
				break;
			}
			break;

		case 0x84: case 0x184: case 0x284: case 0x384: /*TEST b, reg*/
			fetchea();
			temp = getea8();
			temp2 = getr8(reg);
			setznp8(temp&temp2);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= ((mod == 3) ? 2 : 5);
			break;
		case 0x85: case 0x285: /*TEST w, reg*/
			fetchea();
			tempw = getea16();
			tempw2 = regs[reg].w;
			setznp16(tempw & tempw2);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= ((mod == 3) ? 2 : 5);
			break;
		case 0x185: case 0x385: /*TEST l, reg*/
			fetchea();
			templ = getea32();
			templ2 = regs[reg].l;
			setznp32(templ&templ2);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= ((mod == 3) ? 2 : 5);
			break;
		case 0x86: case 0x186: case 0x286: case 0x386: /*XCHG b, reg*/
			fetchea();
			temp = getea8();
			setea8(getr8(reg));
			setr8(reg, temp);
			cycles -= (mod == 3) ? 3 : 5;
			break;
		case 0x87: case 0x287: /*XCHG w, reg*/
			fetchea();
			tempw = getea16();
			setea16(regs[reg].w);
			regs[reg].w = tempw;
			cycles -= (mod == 3) ? 3 : 5;
			break;
		case 0x187: case 0x387: /*XCHG l, reg*/
			fetchea();
			templ = getea32();
			setea32(regs[reg].l);
			regs[reg].l = templ;
			cycles -= (mod == 3) ? 3 : 5;
			break;

		case 0x88: case 0x188: case 0x288: case 0x388: /*MOV b, reg*/
			fetchea();
			setea8(getr8(reg));
			cycles -= ((mod == 3) ? 2:2);
			break;
		case 0x89: case 0x289: /*MOV w, reg*/
			fetchea();
			setea16(regs[reg].w);
			cycles -= ((mod == 3) ? 2:2);
			break;
		case 0x189: case 0x389: /*MOV l, reg*/
			fetchea();
			setea32(regs[reg].l);
			cycles -= ((mod == 3) ? 2:2);
			break;
		case 0x8A: case 0x18A: case 0x28A: case 0x38A: /*MOV reg, b*/
			fetchea();
			temp = getea8();
			setr8(reg, temp);
			cycles -= ((mod == 3) ? 2:4);
			break;
		case 0x8B: case 0x28B: /*MOV reg, w*/
			fetchea();
			tempw = getea16();
			regs[reg].w = tempw;
			cycles -= ((mod == 3) ? 2:4);
			break;
		case 0x18B: case 0x38B: /*MOV reg, l*/
			fetchea();
			templ = getea32();
			regs[reg].l = templ;
			cycles -= ((mod == 3) ? 2:4);
			break;

		case 0x8C: case 0x28C: /*MOV w, sreg*/
			fetchea();
			switch(rmdat & 0x38) {
			case 0x00: /*ES*/
				setea16(ES);
				break;
			case 0x08: /*CS*/
				setea16(CS);
				break;
			case 0x18: /*DS*/
				if(ssegs)
					ds = oldds;
				setea16(DS);
				break;
			case 0x10: /*SS*/
				if(ssegs)
					ss = oldss;
				setea16(SS);
				break;
			case 0x20: /*FS*/
				setea16(FS);
				break;
			case 0x28: /*GS*/
				setea16(GS);
				break;
			}
			cycles -= ((mod == 3) ? 2:3);
			break;
		case 0x18C: case 0x38C: /*MOV l, sreg*/
			fetchea();
			switch(rmdat & 0x38) {
			case 0x00: /*ES*/
				setea32(ES);
				break;
			case 0x08: /*CS*/
				setea32(CS);
				break;
			case 0x18: /*DS*/
				if(ssegs)
					ds = oldds;
				setea32(DS);
				break;
			case 0x10: /*SS*/
				if(ssegs)
					ss = oldss;
				setea32(SS);
				break;
			case 0x20: /*FS*/
				setea32(FS);
				break;
			case 0x28: /*GS*/
				setea32(GS);
				break;
			}
			cycles -= ((mod == 3) ? 2 : 3);
			break;

		case 0x8D: case 0x28D: /*LEA*/
			fetchea();
			regs[reg].w = eaaddr;
			cycles -= 2;
			break;
		case 0x18D: /*LEA*/
			fetchea();
			regs[reg].l = eaaddr & 0xFFFF;
			cycles -= 2;
			break;
		case 0x38D: /*LEA*/
			fetchea();
			regs[reg].l = eaaddr;
			cycles -= 2;
			break;

		case 0x8E: case 0x18E: case 0x28E: case 0x38E: /*MOV sreg, w*/
			fetchea();
			switch(rmdat & 0x38) {
			case 0x00: /*ES*/
				tempw = getea16();
				loadseg(tempw, &_es);
				break;
			case 0x18: /*DS*/
				tempw = getea16();
				loadseg(tempw, &_ds);
				if(ssegs)
					oldds = ds;
				break;
			case 0x10: /*SS*/
				tempw = getea16();
				loadseg(tempw, &_ss);
				if(ssegs)
					oldss = ss;
				noint = 1;
				break;
			case 0x20: /*FS*/
				tempw = getea16();
				loadseg(tempw, &_fs);
				break;
			case 0x28: /*GS*/
				tempw = getea16();
				loadseg(tempw, &_gs);
				break;
			}
			cycles -= ((mod == 3) ? 2 : 5);
			break;

		case 0x8F: case 0x28F: /*POPW*/
			if(ssegs)
				templ2 = oldss;
			else
				templ2 = ss;
			if(stack32) {
				tempw = RM16(templ2, ESP);
				ESP += 2;
			}
			else {
				tempw = RM16(templ2, SP);
				SP += 2;
			}
			fetchea();
			if(ssegs)
				ss = oldss;
			setea16(tempw);
			cycles -= ((mod == 3) ? 4 : 5);
			break;
		case 0x18F: case 0x38F: /*POPL*/
			if(ssegs)
				templ2 = oldss;
			else
				templ2 = ss;
			if(stack32) {
				templ = RM32(templ2, ESP);
				ESP += 4;
			}
			else {
				templ = RM32(templ2, SP);
				SP += 4;
			}
			fetchea();
			if(ssegs)
				ss = oldss;
			setea32(templ);
			cycles -= ((mod == 3) ? 4 : 5);
			break;

		case 0x90: case 0x190: case 0x290: case 0x390: /*NOP*/
			cycles -= 3;
			break;

		case 0x91: case 0x92: case 0x93: /*XCHG AX*/
		case 0x94: case 0x95: case 0x96: case 0x97:
		case 0x291: case 0x292: case 0x293:
		case 0x294: case 0x295: case 0x296: case 0x297:
			tempw = AX;
			AX = regs[opcode & 7].w;
			regs[opcode & 7].w = tempw;
			cycles -= 3;
			break;
		case 0x191: case 0x192: case 0x193: /*XCHG EAX*/
		case 0x194: case 0x195: case 0x196: case 0x197:
		case 0x391: case 0x392: case 0x393: /*XCHG EAX*/
		case 0x394: case 0x395: case 0x396: case 0x397:
			templ = EAX;
			EAX = regs[opcode & 7].l;
			regs[opcode & 7].l = templ;
			cycles -= 3;
			break;

		case 0x98: case 0x298: /*CBW*/
			AH = (AL & 0x80) ? 0xFF : 0;
			cycles -= 3;
			break;
		case 0x198: case 0x398: /*CWDE*/
			EAX = (AX & 0x8000) ? (0xFFFF0000 | AX):AX;
			cycles -= 3;
			break;
		case 0x99: case 0x299: /*CWD*/
			DX = (AX & 0x8000) ? 0xFFFF : 0;
			cycles -= 2;
			break;
		case 0x199: case 0x399: /*CDQ*/
			EDX = (EAX & 0x80000000) ? 0xFFFFFFFF : 0;
			cycles -= 2;
			break;
		case 0x9A: /*CALL FAR*/
			tempw = fetch16();
			tempw2 = fetch16();
#ifdef I386_BIOS_CALL
			if(d_bios) {
				uint16 _regs[8], _sregs[6];
				int32 _zf, _cf;
				for(int i = 0; i < 8; i++)
					_regs[i] = regs[i].w;
				_sregs[0] = CS;
				_sregs[1] = DS;
				_sregs[2] = ES;
				_sregs[3] = SS;
				_sregs[4] = FS;
				_sregs[5] = GS;
				_zf = ((flags & Z_FLAG) != 0);
				_cf = ((flags & C_FLAG) != 0);
				uint32 newpc = tempw + (tempw2 << 4);
				if(d_bios->bios_call(newpc, _regs, _sregs, &_zf, &_cf)) {
					for(int i = 0; i < 8; i++)
						regs[i].w = _regs[i];
					CS = _sregs[0];
					DS = _sregs[1];
					ES = _sregs[2];
					SS = _sregs[3];
					FS = _sregs[4];
					GS = _sregs[5];
					flags &= ~(Z_FLAG | C_FLAG);
					if(_zf)
						flags |= Z_FLAG;
					if(_cf)
						flags |= C_FLAG;
					cycles -= 100;
					break;
				}
			}
#endif
			tempw3 = CS;
			tempw4 = pc;
			if(ssegs)
				ss = oldss;
			oxpc = pc;
			pc = tempw;
			optype = CALL;
			if(msw & 1)
				loadcscall(tempw2);
			else
				loadcs(tempw2);
			if(notpresent)
				break;
			if(stack32) {
				WM16(ss, ESP - 2, tempw3);
				WM16(ss, ESP - 4, tempw4);
				ESP -= 4;
			}
			else {
				WM16(ss, (SP - 2) & 0xFFFF, tempw3);
				WM16(ss, (SP - 4) & 0xFFFF, tempw4);
				SP -= 4;
			}
			cycles -= 17;
			break;
		case 0x9B: case 0x19B: case 0x29B: case 0x39B: /*WAIT*/
			cycles -= 4;
			break;
		case 0x9C: case 0x29C: /*PUSHF*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM16(ss, ESP - 2, flags);
				ESP -= 2;
			}
			else {
				WM16(ss, ((SP - 2) & 0xFFFF), flags);
				SP -= 2;
			}
			cycles -= 4;
			break;
		case 0x19C: case 0x39C: /*PUSHFD*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM16(ss, ESP - 2, eflags & 3); ESP -= 2;
				WM16(ss, ESP - 2, flags);    ESP -= 2;
			}
			else {
				WM16(ss, ((SP - 2) & 0xFFFF), eflags & 3); SP -= 2;
				WM16(ss, ((SP - 2) & 0xFFFF), flags);  SP -= 2;
			}
			cycles -= 4;
			break;
		case 0x9D: case 0x29D: /*POPF*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				tempw = RM16(ss, ESP);
				ESP += 2;
			}
			else {
				tempw = RM16(ss, SP);
				SP += 2;
			}
			if(!(CPL) || !(msw & 1)) flags = tempw;
			else if(IOPLp) flags = (flags & 0x3000) | (tempw & 0xCFFF);
			else		 flags = (flags & 0xF300) | (tempw & 0x0CFF);
			cycles -= 5;
			break;
		case 0x19D: case 0x39D: /*POPFD*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				tempw = RM16(ss, ESP); ESP += 2;
				/*eflags = RM16(ss, ESP) & 3; */ESP += 2;
			}
			else {
				tempw = RM16(ss, SP); SP += 2;
				/*eflags = RM16(ss, SP) & 3; */SP += 2;
			}
			if(!(CPL) || !(msw & 1))
				flags = tempw;
			else if(IOPLp)
				flags = (flags & 0x3000) | (tempw & 0xCFFF);
			else
				flags = (flags & 0xF300) | (tempw & 0x0CFF);
			cycles -= 5;
			break;
		case 0x9E: case 0x19E: case 0x29E: case 0x39E: /*SAHF*/
			flags = (flags & 0xFF00) | AH;
			cycles -= 3;
			break;
		case 0x9F: case 0x19F: case 0x29F: case 0x39F: /*LAHF*/
			AH = flags & 0xFF;
			cycles -= 3;
			break;

		case 0xA0: case 0x1A0: /*MOV AL, (w)*/
			addr = fetch16();
			AL = RM8(ds, addr);
			cycles -= 4;
			break;
		case 0x2A0: case 0x3A0: /*MOV AL, (l)*/
			addr = fetch32();
			AL = RM8(ds, addr);
			cycles -= 4;
			break;
		case 0xA1: /*MOV AX, (w)*/
			addr = fetch16();
			AX = RM16(ds, addr);
			cycles -= 4;
			break;
		case 0x1A1: /*MOV EAX, (w)*/
			addr = fetch16();
			EAX = RM32(ds, addr);
			cycles -= 4;
			break;
		case 0x2A1: /*MOV AX, (l)*/
			addr = fetch32();
			AX = RM16(ds, addr);
			cycles -= 4;
			break;
		case 0x3A1: /*MOV EAX, (l)*/
			addr = fetch32();
			EAX = RM32(ds, addr);
			cycles -= 4;
			break;
		case 0xA2: case 0x1A2: /*MOV (w), AL*/
			addr = fetch16();
			WM8(ds, addr, AL);
			cycles -= 2;
			break;
		case 0x2A2: case 0x3A2: /*MOV (l), AL*/
			addr = fetch32();
			WM8(ds, addr, AL);
			cycles -= 2;
			break;
		case 0xA3: /*MOV (w), AX*/
			addr = fetch16();
			WM16(ds, addr, AX);
			cycles -= 2;
			break;
		case 0x1A3: /*MOV (w), EAX*/
			addr = fetch16();
			WM32(ds, addr, EAX);
			cycles -= 4;
			break;
		case 0x2A3: /*MOV (l), AX*/
			addr = fetch32();
			WM16(ds, addr, AX);
			cycles -= 4;
			break;
		case 0x3A3: /*MOV (l), EAX*/
			addr = fetch32();
			WM32(ds, addr, EAX);
			cycles -= 4;
			break;

		case 0xA4: case 0x1A4: /*MOVSB*/
			temp = RM8(ds, SI);
			WM8(es, DI, temp);
			if(flags & D_FLAG) {
				DI--;
				SI--;
			}
			else {
				DI++;
				SI++;
			}
			cycles -= 7;
			break;
		case 0x2A4: case 0x3A4: /*MOVSB*/
			temp = RM8(ds, ESI);
			WM8(es, EDI, temp);
			if(flags & D_FLAG) {
				EDI--;
				ESI--;
			}
			else {
				EDI++;
				ESI++;
			}
			cycles -= 7;
			break;
		case 0xA5: /*MOVSW*/
			tempw = RM16(ds, SI);
			WM16(es, DI, tempw);
			if(flags & D_FLAG) {
				DI -= 2; SI -= 2; }
			else {
				DI += 2; SI += 2; }
			cycles -= 7;
			break;
		case 0x2A5: /*MOVSW*/
			tempw = RM16(ds, ESI);
			WM16(es, EDI, tempw);
			if(flags & D_FLAG) {
				EDI -= 2; ESI -= 2; }
			else {
				EDI += 2; ESI += 2; }
			cycles -= 7;
			break;
		case 0x1A5: /*MOVSL*/
			templ = RM32(ds, SI);
			WM32(es, DI, templ);
			if(flags & D_FLAG) {
				DI -= 4; SI -= 4; }
			else {
				DI += 4; SI += 4; }
			cycles -= 7;
			break;
		case 0x3A5: /*MOVSL*/
			templ = RM32(ds, ESI);
			WM32(es, EDI, templ);
			if(flags & D_FLAG) {
				EDI -= 4; ESI -= 4; }
			else {
				EDI += 4; ESI += 4; }
			cycles -= 7;
			break;
		case 0xA6: case 0x1A6: /*CMPSB*/
			temp  = RM8(ds, SI);
			temp2 = RM8(es, DI);
			setsub8(temp, temp2);
			if(flags & D_FLAG) {
				DI--;
				SI--;
			}
			else {
				DI++;
				SI++;
			}
			cycles -= 10;
			break;
		case 0x2A6: case 0x3A6: /*CMPSB*/
			temp  = RM8(ds, ESI);
			temp2 = RM8(es, EDI);
			setsub8(temp, temp2);
			if(flags & D_FLAG) {
				EDI--;
				ESI--;
			}
			else {
				EDI++;
				ESI++;
			}
			cycles -= 10;
			break;
		case 0xA7: /*CMPSW*/
			tempw  = RM16(ds, SI);
			tempw2 = RM16(es, DI);
			setsub16(tempw, tempw2);
			if(flags & D_FLAG) {
				DI -= 2; SI -= 2; }
			else {
				DI += 2; SI += 2; }
			cycles -= 10;
			break;
		case 0x1A7: /*CMPSL*/
			templ  = RM32(ds, SI);
			templ2 = RM32(es, DI);
			setsub32(templ, templ2);
			if(flags & D_FLAG) {
				DI -= 4; SI -= 4; }
			else {
				DI += 4; SI += 4; }
			cycles -= 10;
			break;
		case 0x2A7: /*CMPSW*/
			tempw  = RM16(ds, ESI);
			tempw2 = RM16(es, EDI);
			setsub16(tempw, tempw2);
			if(flags & D_FLAG) {
				EDI -= 2; ESI -= 2; }
			else {
				EDI += 2; ESI += 2; }
			cycles -= 10;
			break;
		case 0x3A7: /*CMPSL*/
			templ  = RM32(ds, ESI);
			templ2 = RM32(es, EDI);
			setsub32(templ, templ2);
			if(flags & D_FLAG) {
				EDI -= 4; ESI -= 4; }
			else {
				EDI += 4; ESI += 4; }
			cycles -= 10;
			break;
		case 0xA8: case 0x1A8: case 0x2A8: case 0x3A8: /*TEST AL, #8*/
			temp = fetch8();
			setznp8(AL&temp);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= 2;
			break;
		case 0xA9: case 0x2A9: /*TEST AX, #16*/
			tempw = fetch16();
			setznp16(AX & tempw);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= 2;
			break;
		case 0x1A9: case 0x3A9: /*TEST EAX, #32*/
			templ = fetch32();
			setznp32(EAX & templ);
			flags &= ~(C_FLAG | V_FLAG | A_FLAG);
			cycles -= 2;
			break;
		case 0xAA: case 0x1AA: /*STOSB*/
			WM8(es, DI, AL);
			if(flags & D_FLAG) DI--;
			else		   DI++;
			cycles -= 4;
			break;
		case 0x2AA: case 0x3AA: /*STOSB*/
			WM8(es, EDI, AL);
			if(flags & D_FLAG) EDI--;
			else		   EDI++;
			cycles -= 4;
			break;
		case 0xAB: /*STOSW*/
			WM16(es, DI, AX);
			if(flags & D_FLAG) DI -= 2;
			else		   DI += 2;
			cycles -= 4;
			break;
		case 0x1AB: /*STOSL*/
			WM32(es, DI, EAX);
			if(flags & D_FLAG) DI -= 4;
			else		   DI += 4;
			cycles -= 4;
			break;
		case 0x2AB: /*STOSW*/
			WM16(es, EDI, AX);
			if(flags & D_FLAG) EDI -= 2;
			else		   EDI += 2;
			cycles -= 4;
			break;
		case 0x3AB: /*STOSL*/
			WM32(es, EDI, EAX);
			if(flags & D_FLAG) EDI -= 4;
			else		   EDI += 4;
			cycles -= 4;
			break;
		case 0xAC: case 0x1AC: /*LODSB*/
			AL = RM8(ds, SI);
			if(flags & D_FLAG) SI--;
			else		   SI++;
			cycles -= 5;
			break;
		case 0x2AC: case 0x3AC: /*LODSB*/
			AL = RM8(ds, ESI);
			if(flags & D_FLAG) ESI--;
			else		   ESI++;
			cycles -= 5;
			break;
		case 0xAD: /*LODSW*/
			AX = RM16(ds, SI);
			if(flags & D_FLAG) SI -= 2;
			else		   SI += 2;
			cycles -= 5;
			break;
		case 0x1AD: /*LODSL*/
			EAX = RM32(ds, SI);
			if(flags & D_FLAG) SI -= 4;
			else		   SI += 4;
			cycles -= 5;
			break;
		case 0x2AD: /*LODSW*/
			AX = RM16(ds, ESI);
			if(flags & D_FLAG) ESI -= 2;
			else		   ESI += 2;
			cycles -= 5;
			break;
		case 0x3AD: /*LODSL*/
			EAX = RM32(ds, ESI);
			if(flags & D_FLAG) ESI -= 4;
			else		   ESI += 4;
			cycles -= 5;
			break;
		case 0xAE: case 0x1AE: /*SCASB*/
			temp = RM8(es, DI);
			setsub8(AL, temp);
			if(flags & D_FLAG) DI--;
			else		   DI++;
			cycles -= 7;
			break;
		case 0x2AE: case 0x3AE: /*SCASB*/
			temp = RM8(es, EDI);
			setsub8(AL, temp);
			if(flags & D_FLAG) EDI--;
			else		   EDI++;
			cycles -= 7;
			break;
		case 0xAF: /*SCASW*/
			tempw = RM16(es, DI);
			setsub16(AX, tempw);
			if(flags & D_FLAG) DI -= 2;
			else		   DI += 2;
			cycles -= 7;
			break;
		case 0x1AF: /*SCASL*/
			templ = RM32(es, DI);
			setsub32(EAX, templ);
			if(flags & D_FLAG) DI -= 4;
			else		   DI += 4;
			cycles -= 7;
			break;
		case 0x2AF: /*SCASW*/
			tempw = RM16(es, EDI);
			setsub16(AX, tempw);
			if(flags & D_FLAG) EDI -= 2;
			else		   EDI += 2;
			cycles -= 7;
			break;
		case 0x3AF: /*SCASL*/
			templ = RM32(es, EDI);
			setsub32(EAX, templ);
			if(flags & D_FLAG) EDI -= 4;
			else		   EDI += 4;
			cycles -= 7;
			break;

		case 0xB0: case 0x1B0: case 0x2B0: case 0x3B0: /*MOV AL, #8*/
			AL = fetch8();
			cycles -= 2;
			break;
		case 0xB1: case 0x1B1: case 0x2B1: case 0x3B1: /*MOV CL, #8*/
			CL = fetch8();
			cycles -= 2;
			break;
		case 0xB2: case 0x1B2: case 0x2B2: case 0x3B2: /*MOV DL, #8*/
			DL = fetch8();
			cycles -= 2;
			break;
		case 0xB3: case 0x1B3: case 0x2B3: case 0x3B3: /*MOV BL, #8*/
			BL = fetch8();
			cycles -= 2;
			break;
		case 0xB4: case 0x1B4: case 0x2B4: case 0x3B4: /*MOV AH, #8*/
			AH = fetch8();
			cycles -= 2;
			break;
		case 0xB5: case 0x1B5: case 0x2B5: case 0x3B5: /*MOV CH, #8*/
			CH = fetch8();
			cycles -= 2;
			break;
		case 0xB6: case 0x1B6: case 0x2B6: case 0x3B6: /*MOV DH, #8*/
			DH = fetch8();
			cycles -= 2;
			break;
		case 0xB7: case 0x1B7: case 0x2B7: case 0x3B7: /*MOV BH, #8*/
			BH = fetch8();
			cycles -= 2;
			break;
		case 0xB8: case 0xB9: case 0xBA: case 0xBB: /*MOV reg, #16*/
		case 0xBC: case 0xBD: case 0xBE: case 0xBF:
		case 0x2B8: case 0x2B9: case 0x2BA: case 0x2BB:
		case 0x2BC: case 0x2BD: case 0x2BE: case 0x2BF:
			regs[opcode & 7].w = fetch16();
			cycles -= 2;
			break;
		case 0x1B8: case 0x1B9: case 0x1BA: case 0x1BB: /*MOV reg, #32*/
		case 0x1BC: case 0x1BD: case 0x1BE: case 0x1BF:
		case 0x3B8: case 0x3B9: case 0x3BA: case 0x3BB:
		case 0x3BC: case 0x3BD: case 0x3BE: case 0x3BF:
			regs[opcode & 7].l = fetch32();
			cycles -= 2;
			break;

		case 0xC0: case 0x1C0: case 0x2C0: case 0x3C0:
			fetchea();
			c = fetch8();
			temp = getea8();
			c &= 31;
			if(!c) break;
			switch(rmdat & 0x38) {
			case 0x00: /*ROL b, CL*/
				while(c > 0) {
					temp2 = (temp & 0x80) ? 1 : 0;
					temp = (temp << 1) | temp2;
					c--;
				}
				if(temp2) flags |= C_FLAG;
				else       flags &= ~C_FLAG;
				setea8(temp);
				if((flags & C_FLAG)^(temp >> 7)) flags |= V_FLAG;
				else			  flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x08: /*ROR b, CL*/
				while(c > 0) {
					temp2 = temp & 1;
					temp >>= 1;
					if(temp2) temp |= 0x80;
					c--;
				}
				if(temp2) flags |= C_FLAG;
				else       flags &= ~C_FLAG;
				setea8(temp);
				if((temp^(temp >> 1)) & 0x40) flags |= V_FLAG;
				else			    flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x10: /*RCL b, CL*/
				while(c > 0) {
					cflag = (flags & C_FLAG) ? 1 : 0;
					if(temp & 0x80) flags |= C_FLAG;
					else		flags &= ~C_FLAG;
					temp = (temp << 1) | cflag;
					c--;
				}
				setea8(temp);
				if((flags & C_FLAG)^(temp >> 7)) flags |= V_FLAG;
				else			  flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 9 : 10);
				break;
			case 0x18: /*RCR b, CL*/
				while(c > 0) {
					cflag = (flags & C_FLAG) ? 0x80 : 0;
					if(temp & 1) flags |= C_FLAG;
					else	flags &= ~C_FLAG;
					temp = (temp >> 1) | cflag;
					c--;
				}
				setea8(temp);
				if((temp^(temp >> 1)) & 0x40) flags |= V_FLAG;
				else			    flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 9 : 10);
				break;
			case 0x20: case 0x30: /*SHL b, CL*/
				if((temp << (c - 1)) & 0x80) flags |= C_FLAG;
				else			 flags &= ~C_FLAG;
				temp <<= c;
				setea8(temp);
				setznp8(temp);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;
			case 0x28: /*SHR b, CL*/
				if((temp >> (c - 1)) & 1) flags |= C_FLAG;
				else		 flags &= ~C_FLAG;
				temp >>= c;
				setea8(temp);
				setznp8(temp);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;
			case 0x38: /*SAR b, CL*/
				if((temp >> (c - 1)) & 1) flags |= C_FLAG;
				else		 flags &= ~C_FLAG;
				while(c > 0) {
					temp >>= 1;
					if(temp & 0x40) temp |= 0x80;
					c--;
				}
				setea8(temp);
				setznp8(temp);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			default:
//				invalid();
				break;
			}
			break;

		case 0xC1: case 0x2C1:
			fetchea();
			c = fetch8() & 31;
			tempw = getea16();
			if(!c) break;
			switch(rmdat & 0x38) {
			case 0x00: /*ROL w, CL*/
				while(c > 0) {
					temp = (tempw & 0x8000) ? 1 : 0;
					tempw = (tempw << 1) | temp;
					c--;
				}
				if(temp) flags |= C_FLAG;
				else      flags &= ~C_FLAG;
				setea16(tempw);
				if((flags & C_FLAG)^(tempw >> 15)) flags |= V_FLAG;
				else				 flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x08: /*ROR w, CL*/
				while(c > 0) {
					tempw2 = (tempw & 1) ? 0x8000 : 0;
					tempw = (tempw >> 1) | tempw2;
					c--;
				}
				if(tempw2) flags |= C_FLAG;
				else	flags &= ~C_FLAG;
				setea16(tempw);
				if((tempw^(tempw >> 1)) & 0x4000) flags |= V_FLAG;
				else				flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x10: /*RCL w, CL*/
				while(c > 0) {
					cflag = (flags & C_FLAG) ? 1 : 0;
					if(tempw & 0x8000) flags |= C_FLAG;
					else		   flags &= ~C_FLAG;
					tempw = (tempw << 1) | cflag;
					c--;
				}
				setea16(tempw);
				if((flags & C_FLAG)^(tempw >> 15)) flags |= V_FLAG;
				else				 flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 9 : 10);
				break;
			case 0x18: /*RCR w, CL*/
				while(c > 0) {
					cflag = (flags & C_FLAG) ? 0x8000 : 0;
					if(tempw & 1) flags |= C_FLAG;
					else	 flags &= ~C_FLAG;
					tempw = (tempw >> 1) | cflag;
					c--;
				}
				setea16(tempw);
				if((tempw^(tempw >> 1)) & 0x4000) flags |= V_FLAG;
				else				flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 9 : 10);
				break;

			case 0x20: case 0x30: /*SHL w, CL*/
				if((tempw << (c - 1)) & 0x8000) flags |= C_FLAG;
				else			    flags &= ~C_FLAG;
				tempw <<= c;
				setea16(tempw);
				setznp16(tempw);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			case 0x28:		 /*SHR w, CL*/
				if((tempw >> (c - 1)) & 1) flags |= C_FLAG;
				else		  flags &= ~C_FLAG;
				tempw >>= c;
				setea16(tempw);
				setznp16(tempw);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			case 0x38:		 /*SAR w, CL*/
				tempw2 = tempw & 0x8000;
				if((tempw >> (c - 1)) & 1) flags |= C_FLAG;
				else		  flags &= ~C_FLAG;
				while(c > 0) {
					tempw = (tempw >> 1) | tempw2;
					c--;
				}
				setea16(tempw);
				setznp16(tempw);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			default:
//				invalid();
				break;
			}
			break;
		case 0x1C1: case 0x3C1:
			fetchea();
			c = fetch8();
			c &= 31;
			templ = getea32();
			if(!c) break;
			switch(rmdat & 0x38) {
			case 0x00: /*ROL l, CL*/
				while(c > 0) {
					temp = (templ & 0x80000000) ? 1 : 0;
					templ = (templ << 1) | temp;
					c--;
				}
				if(temp) flags |= C_FLAG;
				else      flags &= ~C_FLAG;
				setea32(templ);
				if((flags & C_FLAG)^(templ >> 31)) flags |= V_FLAG;
				else				 flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 9 : 10);
				break;
			case 0x08: /*ROR l, CL*/
				while(c > 0) {
					templ2 = (templ & 1) ? 0x80000000 : 0;
					templ = (templ >> 1) | templ2;
					c--;
				}
				if(templ2) flags |= C_FLAG;
				else	flags &= ~C_FLAG;
				setea32(templ);
				if((templ^(templ >> 1)) & 0x40000000) flags |= V_FLAG;
				else				    flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 9 : 10);
				break;
			case 0x10: /*RCL l, CL*/
				while(c > 0) {
					cflag = (flags & C_FLAG) ? 1 : 0;
					if(templ & 0x80000000) flags |= C_FLAG;
					else		  flags &= ~C_FLAG;
					templ = (templ << 1) | cflag;
					c--;
				}
				setea32(templ);
				if((flags & C_FLAG)^(templ >> 31)) flags |= V_FLAG;
				else				 flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 9 : 10);
				break;
			case 0x18: /*RCR l, CL*/
				while(c > 0) {
					cflag = (flags & C_FLAG) ? 0x80000000 : 0;
					if(templ & 1) flags |= C_FLAG;
					else	 flags &= ~C_FLAG;
					templ = (templ >> 1) | cflag;
					c--;
				}
				setea32(templ);
				if((templ^(templ >> 1)) & 0x40000000) flags |= V_FLAG;
				else				    flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 9 : 10);
				break;

			case 0x20: case 0x30: /*SHL l, CL*/
				if((templ << (c - 1)) & 0x80000000) flags |= C_FLAG;
				else				flags &= ~C_FLAG;
				templ <<= c;
				setea32(templ);
				setznp32(templ);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			case 0x28:		 /*SHR l, CL*/
				if((templ >> (c - 1)) & 1) flags |= C_FLAG;
				else		  flags &= ~C_FLAG;
				templ >>= c;
				setea32(templ);
				setznp32(templ);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			case 0x38:		 /*SAR l, CL*/
				templ2 = templ & 0x80000000;
				if((templ >> (c - 1)) & 1) flags |= C_FLAG;
				else		  flags &= ~C_FLAG;
				while(c > 0) {
					templ = (templ >> 1) | templ2;
					c--;
				}
				setea32(templ);
				setznp32(templ);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			default:
//				invalid();
				break;
			}
			break;

		case 0xC2: case 0x2C2: /*RET*/
			tempw = fetch16();
			if(ssegs)
				ss = oldss;
			if(stack32) {
				pc = RM16(ss, ESP);
				ESP += 2 + tempw;
			}
			else {
				pc = RM16(ss, SP);
				SP += 2 + tempw;
			}
			cycles -= 10;
			break;
		case 0x1C2: case 0x3C2: /*RET*/
			tempw = fetch16();
			if(ssegs)
				ss = oldss;
			if(stack32) {
				pc = RM32(ss, ESP);
				ESP += 4 + tempw;
			}
			else {
				pc = RM32(ss, SP);
				SP += 4 + tempw;
			}
			cycles -= 10;
			break;
		case 0xC3: case 0x2C3: /*RET*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				pc = RM16(ss, ESP);
				ESP += 2;
			}
			else {
				pc = RM16(ss, SP);
				SP += 2;
			}
			cycles -= 10;
			break;
		case 0x1C3: case 0x3C3: /*RET*/
			if(ssegs)
				ss = oldss;
			if(stack32) {
				pc = RM32(ss, ESP);
				ESP += 4;
			}
			else {
				pc = RM32(ss, SP);
				SP += 4;
			}
			cycles -= 10;
			break;
		case 0xC4: case 0x2C4: /*LES*/
			fetchea();
			regs[reg].w = RM16(easeg, eaaddr);
			tempw = RM16(easeg, eaaddr + 2);
			loadseg(tempw, &_es);
			cycles -= 7;
			break;
		case 0x1C4: case 0x3C4: /*LES*/
			fetchea();
			regs[reg].l = RM32(easeg, eaaddr);
			tempw = RM16(easeg, eaaddr + 4);
			loadseg(tempw, &_es);
			cycles -= 7;
			break;
		case 0xC5: case 0x2C5: /*LDS*/
			fetchea();
			regs[reg].w = RM16(easeg, eaaddr);
			tempw = RM16(easeg, eaaddr + 2);
			loadseg(tempw, &_ds);
			if(ssegs)
				oldds = ds;
			cycles -= 7;
			break;
		case 0x1C5: case 0x3C5: /*LDS*/
			fetchea();
			regs[reg].l = RM32(easeg, eaaddr);
			tempw = RM16(easeg, eaaddr + 4);
			loadseg(tempw, &_ds);
			if(ssegs)
				oldds = ds;
			cycles -= 7;
			break;
		case 0xC6: case 0x1C6: case 0x2C6: case 0x3C6: /*MOV b, #8*/
			fetchea();
			temp = fetch8();
			setea8(temp);
			cycles -= 2;
			break;
		case 0xC7: case 0x2C7: /*MOV w, #16*/
			fetchea();
			tempw = fetch16();
			setea16(tempw);
			cycles -= 2;
			break;
		case 0x1C7: case 0x3C7: /*MOV l, #32*/
			fetchea();
			templ = fetch32();
			setea32(templ);
			cycles -= 2;
			break;
		case 0xC8: case 0x2C8: /*ENTER*/
			tempw2 = fetch16();
			tempi = fetch8();
			if(stack32) {
				WM16(ss, (ESP - 2), BP);
				ESP -= 2;
			}
			else {
				WM16(ss, ((SP - 2) & 0xFFFF), BP);
				SP -= 2;
			}
			templ2 = ESP;
			if(tempi > 0) {
				while(--tempi) {
					EBP -= 2;
					if(stack32)
						tempw = RM16(ss, EBP);
					else
						tempw = RM16(ss, BP);
					if(stack32) {
						WM16(ss, (ESP - 2), tempw);
						ESP -= 2;
					}
					else {
						WM16(ss, ((SP - 2) & 0xFFFF), tempw);
						SP -= 2;
					}
					cycles -= 4;
				}
				if(stack32) {
					WM16(ss, (ESP - 2), templ2);
					ESP -= 2;
				}
				else {
					WM16(ss, ((SP - 2) & 0xFFFF), templ2);
					SP -= 2;
				}
				cycles -= 5;
			}
			if(stack32) {
				EBP = templ2;
				ESP -= tempw2;
			}
			else {
				BP = templ2;
				SP -= tempw2;
			}
			cycles -= 10;
			break;
			break;
		case 0x1C8: case 0x3C8: /*ENTER*/
			tempw = fetch16();
			tempi = fetch8();
			if(stack32) { WM32(ss, (ESP - 4), EBP);	ESP -= 4; }
			else	 { WM32(ss, ((SP - 4) & 0xFFFF), EBP); SP -= 4; }
			templ2 = ESP;
			if(tempi > 0) {
				while(--tempi) {
					EBP -= 4;
					if(stack32) templ = RM32(ss, EBP);
					else	 templ = RM32(ss, BP);
					if(stack32) { WM32(ss, (ESP - 4), templ);	ESP -= 4; }
					else	 { WM32(ss, ((SP - 4) & 0xFFFF), templ); SP -= 4; }
					cycles -= 4;
				}
				if(stack32) { WM32(ss, (ESP - 4), templ2);	ESP -= 4; }
				else	 { WM32(ss, ((SP - 4) & 0xFFFF), templ2); SP -= 4; }
				cycles -= 5;
			}
			if(stack32) { EBP = templ2; ESP -= tempw; }
			else	 {  BP = templ2;  SP -= tempw; }
			cycles -= 10;
			break;
		case 0xC9: case 0x2C9: /*LEAVE*/
			SP = BP;
			if(stack32) { BP = RM16(ss, ESP); ESP += 2; }
			else	 { BP = RM16(ss, SP);   SP += 2; }
			cycles -= 4;
			break;
		case 0x3C9: case 0x1C9: /*LEAVE*/
			ESP = EBP;
			if(stack32) {
				EBP = RM32(ss, ESP);
				ESP += 4;
			}
			else {
				EBP = RM32(ss, SP);
				SP += 4;
			}
			cycles -= 4;
			break;
		case 0xCA: case 0x2CA: /*RETF*/
			tempw = fetch16();
			tempw2 = CPL;
			if(ssegs)
				ss = oldss;
			oxpc = pc;
			if(stack32) {
				pc = RM16(ss, ESP);
				loadcs(RM16(ss, ESP + 2));
			}
			else {
				pc = RM16(ss, SP);
				loadcs(RM16(ss, SP + 2));
			}
			if(notpresent) break;
			if(stack32) ESP += 4 + tempw;
			else	 SP += 4 + tempw;
			if((msw & 1) && CPL > tempw2) {
				if(stack32) {
					general_protection_fault(0);
					break;
				}
				tempw = RM16(ss, SP);
				loadseg(RM16(ss, SP + 2), &_ss);
				SP = tempw;
			}
			cycles -= 18;
			break;
		case 0x1CA: case 0x3CA: /*RETF*/
			tempw = fetch16();
			tempw2 = CPL;
			if(ssegs)
				ss = oldss;
			oxpc = pc;
			if(stack32) {
				pc = RM32(ss, ESP);
				loadcs(RM32(ss, ESP + 4) & 0xFFFF);
			}
			else {
				pc = RM32(ss, SP);
				loadcs(RM32(ss, SP + 4) & 0xFFFF);
			}
			if(notpresent)
				break;
			if(stack32) ESP += 8 + tempw;
			else	 SP += 8 + tempw;
			if((msw & 1) && CPL > tempw2) {
				general_protection_fault(0);
				break;
				// ???
				tempw = RM16(ss, SP);
				loadseg(RM16(ss, SP + 2), &_ss);
				SP = tempw;
			}
			cycles -= 18;
			break;
		case 0xCB: case 0x2CB: /*RETF*/
			tempw2 = CPL;
			if(ssegs)
				ss = oldss;
			oxpc = pc;
			if(stack32) {
				pc = RM16(ss, ESP);
				loadcs(RM16(ss, ESP + 2));
			}
			else {
				pc = RM16(ss, SP);
				loadcs(RM16(ss, SP + 2));
			}
			if(notpresent) break;
			if(stack32) ESP += 4;
			else	 SP += 4;
			if((msw & 1) && CPL > tempw2) {
				if(stack32) {
					general_protection_fault(0);
					break;
				}
				tempw = RM16(ss, SP);
				loadseg(RM16(ss, SP + 2), &_ss);
				SP = tempw;
			}
			cycles -= 18;
			break;
		case 0x1CB: case 0x3CB: /*RETF*/
			tempw2 = CPL;
			if(ssegs)
				ss = oldss;
			oxpc = pc;
			if(stack32) {
				pc = RM32(ss, ESP);
				loadcs(RM16(ss, ESP + 4));
			}
			else {
				pc = RM32(ss, SP);
				loadcs(RM16(ss, SP + 4));
			}
			if(notpresent) break;
			if(stack32) ESP += 8;
			else	 SP += 8;
			if((msw & 1) && CPL > tempw2) {
				if(stack32) {
					templ = RM32(ss, ESP);
					loadseg(RM32(ss, ESP + 4), &_ss);
					ESP = templ;
				}
				else {
					templ = RM32(ss, SP);
					loadseg(RM32(ss, SP + 4), &_ss);
					ESP = templ;
				}
			}
			cycles -= 18;
			break;
		case 0xCC: case 0x1CC: case 0x2CC: case 0x3CC: /*INT 3*/
			interrupt(3, 1);
			break;
		case 0xCD: case 0x1CD: case 0x2CD: case 0x3CD: /*INT*/
			temp = fetch8();
#ifdef I386_BIOS_CALL
			if(d_bios) {
				uint16 _regs[8], _sregs[6];
				int32 _zf, _cf;
				for(int i = 0; i < 8; i++)
					_regs[i] = regs[i].w;
				_sregs[0] = CS;
				_sregs[1] = DS;
				_sregs[2] = ES;
				_sregs[3] = SS;
				_sregs[4] = FS;
				_sregs[5] = GS;
				_zf = ((flags & Z_FLAG) != 0);
				_cf = ((flags & C_FLAG) != 0);
				if(d_bios->bios_call(temp, _regs, _sregs, &_zf, &_cf)) {
					// set regs and flags
					for(int i = 0; i < 8; i++)
						regs[i].w = _regs[i];
					CS = _sregs[0];
					DS = _sregs[1];
					ES = _sregs[2];
					SS = _sregs[3];
					FS = _sregs[4];
					GS = _sregs[5];
					flags &= ~(Z_FLAG | C_FLAG);
					if(_zf)
						flags |= Z_FLAG;
					if(_cf)
						flags |= C_FLAG;
					cycles -= 100;
					break;
				}
			}
#endif
			interrupt(temp, 1);
			cycles -= 4;
			break;
		case 0xCE: /*INTO*/
			if(flags & V_FLAG)
				interrupt(OVERFLOW_TRAP, 1);
			cycles -= 3;
			break;
		case 0xCF: case 0x2CF: /*IRET*/
			optype = IRET;
			if(ssegs)
				ss = oldss;
			if(msw & 1)
				pmodeiret();
			else {
				tempw = CS;
				tempw2 = pc;
				oxpc = pc;
				if(stack32) {
					pc = RM16(ss, ESP);
					loadcs(RM16(ss, ESP + 2));
				}
				else {
					pc = RM16(ss, SP);
					loadcs(RM16(ss, ((SP + 2) & 0xFFFF)));
				}
				if(notpresent)
					break;
				if(stack32) {
					flags = RM16(ss, ESP + 4);
					ESP += 6;
				}
				else {
					flags = RM16(ss, ((SP + 4) & 0xFFFF));
					SP += 6;
				}
			}
			cycles -= 22;
			break;
		case 0x1CF: case 0x3CF: /*IRETD*/
			optype = IRET;
			if(ssegs)
				ss = oldss;
			if(msw & 1)
				pmodeiretd();
			else {
				tempw = CS;
				tempw2 = pc;
				oxpc = pc;
				if(stack32) {
					pc = RM32(ss, ESP);
					templ = RM32(ss, ESP + 4);
				}
				else {
					pc = RM32(ss, SP);
					templ = RM32(ss, ((SP + 4) & 0xFFFF));
				}
				if(notpresent) break;
				if(stack32) {
					flags = RM16(ss, ESP + 8);
					eflags = RM16(ss, ESP + 10);
					ESP += 12;
				}
				else {
					flags = RM16(ss, (SP + 8) & 0xFFFF);
					eflags = RM16(ss, (SP + 10) & 0xFFFF);
					SP += 12;
				}
				loadcs(templ);
			}
			cycles -= 22;
			break;
		case 0xD0: case 0x1D0: case 0x2D0: case 0x3D0:
			fetchea();
			temp = getea8();
			switch(rmdat & 0x38)
			{
			case 0x00: /*ROL b, 1*/
				if(temp & 0x80) flags |= C_FLAG;
				else		flags &= ~C_FLAG;
				temp <<= 1;
				if(flags & C_FLAG) temp |= 1;
				setea8(temp);
				if((flags & C_FLAG)^(temp >> 7)) flags |= V_FLAG;
				else			  flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x08: /*ROR b, 1*/
				if(temp & 1) flags |= C_FLAG;
				else	flags &= ~C_FLAG;
				temp >>= 1;
				if(flags & C_FLAG) temp |= 0x80;
				setea8(temp);
				if((temp^(temp >> 1)) & 0x40) flags |= V_FLAG;
				else			    flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x10: /*RCL b, 1*/
				temp2 = flags & C_FLAG;
				if(temp & 0x80) flags |= C_FLAG;
				else		flags &= ~C_FLAG;
				temp <<= 1;
				if(temp2) temp |= 1;
				setea8(temp);
				if((flags & C_FLAG)^(temp >> 7)) flags |= V_FLAG;
				else			  flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x18: /*RCR b, 1*/
				temp2 = flags & C_FLAG;
				if(temp & 1) flags |= C_FLAG;
				else	flags &= ~C_FLAG;
				temp >>= 1;
				if(temp2) temp |= 0x80;
				setea8(temp);
				if((temp^(temp >> 1)) & 0x40) flags |= V_FLAG;
				else			    flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x20: /*SHL b, 1*/
				if(temp & 0x80) flags |= C_FLAG;
				else		flags &= ~C_FLAG;
				if((temp^(temp << 1)) & 0x80) flags |= V_FLAG;
				else			    flags &= ~V_FLAG;
				temp <<= 1;
				setea8(temp);
				setznp8(temp);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;
			case 0x28: /*SHR b, 1*/
				if(temp & 1) flags |= C_FLAG;
				else	flags &= ~C_FLAG;
				if(temp & 0x80) flags |= V_FLAG;
				else		flags &= ~V_FLAG;
				temp >>= 1;
				setea8(temp);
				setznp8(temp);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;
			case 0x38: /*SAR b, 1*/
				if(temp & 1) flags |= C_FLAG;
				else	flags &= ~C_FLAG;
				temp >>= 1;
				if(temp & 0x40) temp |= 0x80;
				setea8(temp);
				setznp8(temp);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				flags &= ~V_FLAG;
				break;

			default:
//				invalid();
				break;
			}
			break;

		case 0xD1: case 0x2D1:
			fetchea();
			tempw = getea16();
			switch(rmdat & 0x38) {
			case 0x00: /*ROL w, 1*/
				if(tempw & 0x8000) flags |= C_FLAG;
				else		   flags &= ~C_FLAG;
				tempw <<= 1;
				if(flags & C_FLAG) tempw |= 1;
				setea16(tempw);
				if((flags & C_FLAG)^(tempw >> 15)) flags |= V_FLAG;
				else				 flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x08: /*ROR w, 1*/
				if(tempw & 1) flags |= C_FLAG;
				else	 flags &= ~C_FLAG;
				tempw >>= 1;
				if(flags & C_FLAG) tempw |= 0x8000;
				setea16(tempw);
				if((tempw^(tempw >> 1)) & 0x4000) flags |= V_FLAG;
				else				flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x10: /*RCL w, 1*/
				temp2 = flags & C_FLAG;
				if(tempw & 0x8000) flags |= C_FLAG;
				else		   flags &= ~C_FLAG;
				tempw <<= 1;
				if(temp2) tempw |= 1;
				setea16(tempw);
				if((flags & C_FLAG)^(tempw >> 15)) flags |= V_FLAG;
				else				 flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x18: /*RCR w, 1*/
				temp2 = flags & C_FLAG;
				if(tempw & 1) flags |= C_FLAG;
				else	 flags &= ~C_FLAG;
				tempw >>= 1;
				if(temp2) tempw |= 0x8000;
				setea16(tempw);
				if((tempw^(tempw >> 1)) & 0x4000) flags |= V_FLAG;
				else				flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x20: /*SHL w, 1*/
				if(tempw & 0x8000) flags |= C_FLAG;
				else		   flags &= ~C_FLAG;
				if((tempw^(tempw << 1)) & 0x8000) flags |= V_FLAG;
				else				flags &= ~V_FLAG;
				tempw <<= 1;
				setea16(tempw);
				setznp16(tempw);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;
			case 0x28: /*SHR w, 1*/
				if(tempw & 1) flags |= C_FLAG;
				else	 flags &= ~C_FLAG;
				if(tempw & 0x8000) flags |= V_FLAG;
				else		   flags &= ~V_FLAG;
				tempw >>= 1;
				setea16(tempw);
				setznp16(tempw);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			case 0x38: /*SAR w, 1*/
				if(tempw & 1) flags |= C_FLAG;
				else	 flags &= ~C_FLAG;
				tempw >>= 1;
				if(tempw & 0x4000) tempw |= 0x8000;
				setea16(tempw);
				setznp16(tempw);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				flags &= ~V_FLAG;
				break;

			default:
//				invalid();
				break;
			}
			break;
		case 0x1D1: case 0x3D1:
			fetchea();
			templ = getea32();
			switch(rmdat & 0x38) {
			case 0x00: /*ROL l, 1*/
				if(templ & 0x80000000) flags |= C_FLAG;
				else		  flags &= ~C_FLAG;
				templ <<= 1;
				if(flags & C_FLAG) templ |= 1;
				setea32(templ);
				if((flags & C_FLAG)^(templ >> 31)) flags |= V_FLAG;
				else				 flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x08: /*ROR l, 1*/
				if(templ & 1) flags |= C_FLAG;
				else	 flags &= ~C_FLAG;
				templ >>= 1;
				if(flags & C_FLAG) templ |= 0x80000000;
				setea32(templ);
				if((templ^(templ >> 1)) & 0x40000000) flags |= V_FLAG;
				else				    flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x10: /*RCL l, 1*/
				temp2 = flags & C_FLAG;
				if(templ & 0x80000000) flags |= C_FLAG;
				else		  flags &= ~C_FLAG;
				templ <<= 1;
				if(temp2) templ |= 1;
				setea32(templ);
				if((flags & C_FLAG)^(templ >> 31)) flags |= V_FLAG;
				else				 flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x18: /*RCR l, 1*/
				temp2 = flags & C_FLAG;
				if(templ & 1) flags |= C_FLAG;
				else	 flags &= ~C_FLAG;
				templ >>= 1;
				if(temp2) templ |= 0x80000000;
				setea32(templ);
				if((templ^(templ >> 1)) & 0x40000000) flags |= V_FLAG;
				else				    flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x20: /*SHL l, 1*/
				if(templ & 0x80000000) flags |= C_FLAG;
				else		  flags &= ~C_FLAG;
				if((templ^(templ << 1)) & 0x80000000) flags |= V_FLAG;
				else				    flags &= ~V_FLAG;
				templ <<= 1;
				setea32(templ);
				setznp32(templ);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;
			case 0x28: /*SHR l, 1*/
				if(templ & 1) flags |= C_FLAG;
				else	 flags &= ~C_FLAG;
				if(templ & 0x80000000) flags |= V_FLAG;
				else		  flags &= ~V_FLAG;
				templ >>= 1;
				setea32(templ);
				setznp32(templ);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			case 0x38: /*SAR l, 1*/
				if(templ & 1) flags |= C_FLAG;
				else	 flags &= ~C_FLAG;
				templ >>= 1;
				if(templ & 0x40000000) templ |= 0x80000000;
				setea32(templ);
				setznp32(templ);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				flags &= ~V_FLAG;
				break;

			default:
//				invalid();
				break;
			}
			break;

		case 0xD2: case 0x1D2: case 0x2D2: case 0x3D2:
			fetchea();
			temp = getea8();
			c = CL & 31;
			if(!c) break;
			switch(rmdat & 0x38) {
			case 0x00: /*ROL b, CL*/
				while(c > 0) {
					temp2 = (temp & 0x80) ? 1 : 0;
					temp = (temp << 1) | temp2;
					c--;
				}
				if(temp2) flags |= C_FLAG;
				else       flags &= ~C_FLAG;
				setea8(temp);
				if((flags & C_FLAG)^(temp >> 7)) flags |= V_FLAG;
				else			  flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x08: /*ROR b, CL*/
				while(c > 0) {
					temp2 = temp & 1;
					temp >>= 1;
					if(temp2) temp |= 0x80;
					c--;
				}
				if(temp2) flags |= C_FLAG;
				else       flags &= ~C_FLAG;
				setea8(temp);
				if((temp^(temp >> 1)) & 0x40) flags |= V_FLAG;
				else			    flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x10: /*RCL b, CL*/
				while(c > 0) {
					templ = flags & C_FLAG;
					temp2 = temp & 0x80;
					temp <<= 1;
					if(temp2) flags |= C_FLAG;
					else       flags &= ~C_FLAG;
					if(templ) temp |= 1;
					c--;
				}
				setea8(temp);
				if((flags & C_FLAG)^(temp >> 7)) flags |= V_FLAG;
				else			  flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x18: /*RCR b, CL*/
				while(c > 0) {
					templ = flags & C_FLAG;
					temp2 = temp & 1;
					temp >>= 1;
					if(temp2) flags |= C_FLAG;
					else       flags &= ~C_FLAG;
					if(templ) temp |= 0x80;
					c--;
				}
				setea8(temp);
				if((temp^(temp >> 1)) & 0x40) flags |= V_FLAG;
				else			    flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x20: case 0x30: /*SHL b, CL*/
				if((temp << (c - 1)) & 0x80) flags |= C_FLAG;
				else			 flags &= ~C_FLAG;
				temp <<= c;
				setea8(temp);
				setznp8(temp);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;
			case 0x28: /*SHR b, CL*/
				if((temp >> (c - 1)) & 1) flags |= C_FLAG;
				else		 flags &= ~C_FLAG;
				temp >>= c;
				setea8(temp);
				setznp8(temp);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;
			case 0x38: /*SAR b, CL*/
				if((temp >> (c - 1)) & 1) flags |= C_FLAG;
				else		 flags &= ~C_FLAG;
				while(c > 0) {
					temp >>= 1;
					if(temp & 0x40) temp |= 0x80;
					c--;
				}
				setea8(temp);
				setznp8(temp);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			default:
//				invalid();
				break;
			}
			break;

		case 0xD3: case 0x2D3:
			fetchea();
			tempw = getea16();
			c = CL & 31;
			if(!c) break;
			switch(rmdat & 0x38)
			{
			case 0x00: /*ROL w, CL*/
				while(c > 0) {
					temp = (tempw & 0x8000) ? 1 : 0;
					tempw = (tempw << 1) | temp;
					c--;
				}
				if(temp) flags |= C_FLAG;
				else      flags &= ~C_FLAG;
				setea16(tempw);
				if((flags & C_FLAG)^(tempw >> 15)) flags |= V_FLAG;
				else				 flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x08: /*ROR w, CL*/
				while(c > 0) {
					tempw2 = (tempw & 1) ? 0x8000 : 0;
					tempw = (tempw >> 1) | tempw2;
					c--;
				}
				if(tempw2) flags |= C_FLAG;
				else	flags &= ~C_FLAG;
				setea16(tempw);
				if((tempw^(tempw >> 1)) & 0x4000) flags |= V_FLAG;
				else				flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x10: /*RCL w, CL*/
				while(c > 0) {
					templ = flags & C_FLAG;
					if(tempw & 0x8000) flags |= C_FLAG;
					else		   flags &= ~C_FLAG;
					tempw = (tempw << 1) | templ;
					c--;
				}
				if(temp) flags |= C_FLAG;
				else      flags &= ~C_FLAG;
				setea16(tempw);
				if((flags & C_FLAG)^(tempw >> 15)) flags |= V_FLAG;
				else				 flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x18: /*RCR w, CL*/
				while(c > 0) {
					templ = flags & C_FLAG;
					tempw2 = (templ & 1) ? 0x8000 : 0;
					if(tempw & 1) flags |= C_FLAG;
					else	 flags &= ~C_FLAG;
					tempw = (tempw >> 1) | tempw2;
					c--;
				}
				if(tempw2) flags |= C_FLAG;
				else	flags &= ~C_FLAG;
				setea16(tempw);
				if((tempw^(tempw >> 1)) & 0x4000) flags |= V_FLAG;
				else				flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;

			case 0x20: case 0x30: /*SHL w, CL*/
				if(c > 16) {
					tempw = 0;
					flags &= ~C_FLAG;
				}
				else {
					if((tempw << (c - 1)) & 0x8000) flags |= C_FLAG;
					else			    flags &= ~C_FLAG;
					tempw <<= c;
				}
				setea16(tempw);
				setznp16(tempw);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			case 0x28:		 /*SHR w, CL*/
				if((tempw >> (c - 1)) & 1) flags |= C_FLAG;
				else		  flags &= ~C_FLAG;
				tempw >>= c;
				setea16(tempw);
				setznp16(tempw);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			case 0x38:		 /*SAR w, CL*/
				tempw2 = tempw & 0x8000;
				if((tempw >> (c - 1)) & 1) flags |= C_FLAG;
				else		  flags &= ~C_FLAG;
				while(c > 0) {
					tempw = (tempw >> 1) | tempw2;
					c--;
				}
				setea16(tempw);
				setznp16(tempw);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			default:
//				invalid();
				break;
			}
			break;
		case 0x1D3: case 0x3D3:
			fetchea();
			templ = getea32();
			c = CL & 31;
			if(!c) break;
			switch(rmdat & 0x38) {
			case 0x00: /*ROL l, CL*/
				while(c > 0) {
					temp = (templ & 0x80000000) ? 1 : 0;
					templ = (templ << 1) | temp;
					c--;
				}
				if(temp) flags |= C_FLAG;
				else      flags &= ~C_FLAG;
				setea32(templ);
				if((flags & C_FLAG)^(templ >> 31)) flags |= V_FLAG;
				else				 flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x08: /*ROR l, CL*/
				while(c > 0) {
					templ2 = (templ & 1) ? 0x80000000 : 0;
					templ = (templ >> 1) | templ2;
					c--;
				}
				if(templ2) flags |= C_FLAG;
				else	flags &= ~C_FLAG;
				setea32(templ);
				if((templ^(templ >> 1)) & 0x40000000) flags |= V_FLAG;
				else				    flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x10: /*RCL l, CL*/
				while(c > 0) {
					templ2 = flags & C_FLAG;
					if(templ & 0x80000000) flags |= C_FLAG;
					else		  flags &= ~C_FLAG;
					templ = (templ << 1) | templ2;
					c--;
				}
				setea32(templ);
				if((flags & C_FLAG)^(templ >> 31)) flags |= V_FLAG;
				else				 flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;
			case 0x18: /*RCR l, CL*/
				while(c > 0) {
					templ2 = (flags & C_FLAG) ? 0x80000000 : 0;
					if(templ & 1) flags |= C_FLAG;
					else	 flags &= ~C_FLAG;
					templ = (templ >> 1) | templ2;
					c--;
				}
				setea32(templ);
				if((templ^(templ >> 1)) & 0x40000000) flags |= V_FLAG;
				else				    flags &= ~V_FLAG;
				cycles -= ((mod == 3) ? 3:7);
				break;

			case 0x20: case 0x30: /*SHL l, CL*/
				if((templ << (c - 1)) & 0x80000000) flags |= C_FLAG;
				else				flags &= ~C_FLAG;
				templ <<= c;
				setea32(templ);
				setznp32(templ);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			case 0x28:		 /*SHR l, CL*/
				if((templ >> (c - 1)) & 1) flags |= C_FLAG;
				else		  flags &= ~C_FLAG;
				templ >>= c;
				setea32(templ);
				setznp32(templ);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			case 0x38:		 /*SAR w, CL*/
				templ2 = templ & 0x80000000;
				if((templ >> (c - 1)) & 1) flags |= C_FLAG;
				else		  flags &= ~C_FLAG;
				while(c > 0) {
					templ = (templ >> 1) | templ2;
					c--;
				}
				setea32(templ);
				setznp32(templ);
				cycles -= ((mod == 3) ? 3:7);
				flags |= A_FLAG;
				break;

			default:
//				invalid();
				break;
			}
			break;

		case 0xD4: case 0x1D4: case 0x2D4: case 0x3D4: /*AAM*/
			tempws = fetch8();
			AH = AL / tempws;
			AL %= tempws;
			setznp16(AX);
			cycles -= 17;
			break;
		case 0xD5: case 0x1D5: case 0x2D5: case 0x3D5: /*AAD*/
			tempws = fetch8();
			AL = (AH*tempws) + AL;
			AH = 0;
			setznp16(AX);
			cycles -= 19;
			break;
		case 0xD7: case 0x1D7: /*XLAT*/
			addr = (BX + AL) & 0xFFFF;
			AL = RM8(ds, addr);
			cycles -= 5;
			break;
		case 0x2D7: case 0x3D7: /*XLAT*/
			addr = EBX + AL;
			AL = RM8(ds, addr);
			cycles -= 5;
			break;
		case 0xD9: case 0xDA: case 0xDB: case 0xDD:     /*ESCAPE*/
		case 0x1D9: case 0x1DA: case 0x1DB: case 0x1DD: /*ESCAPE*/
		case 0x2D9: case 0x2DA: case 0x2DB: case 0x2DD: /*ESCAPE*/
		case 0x3D9: case 0x3DA: case 0x3DB: case 0x3DD: /*ESCAPE*/
		case 0xD8: case 0x1D8: case 0x2D8: case 0x3D8:
		case 0xDC: case 0x1DC: case 0x2DC: case 0x3DC:
		case 0xDE: case 0x1DE: case 0x2DE: case 0x3DE:
		case 0xDF: case 0x1DF: case 0x2DF: case 0x3DF:
			if(cr0 & 1) {
				if((cr0&5) == 5) {
					pc--;
					pmodeint(7, 0);
					cycles -= 59;
				}
				else {
					fetchea();
					getea8();
				}
			}
			else {
				fetchea();
				getea8();
			}
			break;

		case 0xE0: case 0x1E0: /*LOOPNE*/
			offset = (int8)fetch8();
			CX--;
			if(CX && !(flags & Z_FLAG)) { pc += offset; }
			cycles -= 11;
			break;
		case 0x2E0: case 0x3E0: /*LOOPNE*/
			offset = (int8)fetch8();
			ECX--;
			if(ECX && !(flags & Z_FLAG)) { pc += offset; }
			cycles -= 11;
			break;
		case 0xE1: case 0x1E1: /*LOOPE*/
			offset = (int8)fetch8();
			CX--;
			if(CX && (flags & Z_FLAG)) { pc += offset; }
			cycles -= 11;
			break;
		case 0x2E1: case 0x3E1: /*LOOPE*/
			offset = (int8)fetch8();
			ECX--;
			if(ECX && (flags & Z_FLAG)) { pc += offset; }
			cycles -= 11;
			break;
		case 0xE2: case 0x1E2: /*LOOP*/
			offset = (int8)fetch8();
			CX--;
			if(CX) { pc += offset; }
			cycles -= 11;
			break;
		case 0x2E2: case 0x3E2: /*LOOP*/
			offset = (int8)fetch8();
			ECX--;
			if(ECX) { pc += offset; }
			cycles -= 11;
			break;
		case 0xE3: case 0x1E3: /*JCXZ*/
			offset = (int8)fetch8();
			if(!CX) { pc += offset; cycles -= 4; }
			cycles -= 5;
			break;
		case 0x2E3: case 0x3E3: /*JECXZ*/
			offset = (int8)fetch8();
			if(!ECX) { pc += offset; cycles -= 4; }
			cycles -= 5;
			break;

		case 0xE4: case 0x1E4: case 0x2E4: case 0x3E4: /*IN AL*/
			temp = fetch8();
			AL = IN8(temp);
			cycles -= 12;
			break;
		case 0xE5: case 0x2E5: /*IN AX*/
			temp = fetch8();
			AX = IN16(temp);
			cycles -= 12;
			break;
		case 0x1E5: case 0x3E5: /*IN EAX*/
			temp = fetch8();
			EAX = IN32(temp);
			cycles -= 12;
			break;
		case 0xE6: case 0x1E6: case 0x2E6: case 0x3E6: /*OUT AL*/
			temp = fetch8();
			OUT8(temp, AL);
			cycles -= 10;
			break;
		case 0xE7: case 0x2E7: /*OUT AX*/
			temp = fetch8();
			OUT16(temp, AX);
			cycles -= 10;
			break;
		case 0x1E7: case 0x3E7: /*OUT EAX*/
			temp = fetch8();
			OUT32(temp, EAX);
			cycles -= 10;
			break;

		case 0xE8: /*CALL rel 16*/
			tempw = fetch16();
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM16(ss, ESP - 2, pc);
				ESP -= 2;
			}
			else {
				WM16(ss, ((SP - 2) & 0xFFFF), pc);
				SP -= 2;
			}
			pc += (int16)tempw;
			cycles -= 7;
			break;
		case 0x3E8: /*CALL rel 16*/
			templ = fetch32();
			if(ssegs)
				ss = oldss;
			if(stack32) {
				WM32(ss, ESP - 4, pc);
				ESP -= 4;
			}
			else {
				WM32(ss, ((SP - 4) & 0xFFFF), pc);
				SP -= 4;
			}
			pc += templ;
			cycles -= 7;
			break;
		case 0xE9: case 0x2E9: /*JMP rel 16*/
			pc += (int16)fetch16();
			cycles -= 7;
			break;
		case 0x1E9: case 0x3E9: /*JMP rel 32*/
			pc += fetch32();
			cycles -= 7;
			break;
		case 0xEA: case 0x2EA: /*JMP far*/
			addr = fetch16();
			tempw = fetch16();
			oxpc = pc;
			pc = addr;
			optype = JMP;
			loadcs(tempw);
			cycles -= 12;
			break;
		case 0x1EA: case 0x3EA: /*JMP far*/
			templ = fetch32();
			tempw = fetch16();
			oxpc = pc;
			pc = templ;
			optype = JMP;
			loadcs(tempw);
			cycles -= 12;
			break;
		case 0xEB: case 0x1EB: case 0x2EB: case 0x3EB: /*JMP rel*/
			offset = (int8)fetch8();
			pc += offset;
			cycles -= 7;
			break;
		case 0xEC: case 0x1EC: case 0x2EC: case 0x3EC: /*IN AL, DX*/
			AL = IN8(DX);
			cycles -= 13;
			break;
		case 0xED: case 0x2ED: /*IN AX, DX*/
			AX = IN16(DX);
			cycles -= 13;
			break;
		case 0x1ED: case 0x3ED: /*IN EAX, DX*/
			EAX = IN32(DX);
			cycles -= 13;
			break;
		case 0xEE: case 0x1EE: case 0x2EE: case 0x3EE: /*OUT DX, AL*/
			OUT8(DX, AL);
			cycles -= 11;
			break;
		case 0xEF: case 0x2EF: /*OUT DX, AX*/
			OUT16(DX, AX);
			cycles -= 11;
			break;
		case 0x1EF: case 0x3EF: /*OUT DX, EAX*/
			OUT32(DX, EAX);
			cycles -= 11;
			break;

		case 0xF0: /*LOCK*/
			cycles -= 4;
			break;
		case 0xF2: case 0x1F2: case 0x2F2: case 0x3F2: /*REPNE*/
			rep(0);
			break;
		case 0xF3: case 0x1F3: case 0x2F3: case 0x3F3: /*REPE*/
			rep(1);
			break;
		case 0xF4: case 0x1F4: case 0x2F4: case 0x3F4: /*HLT*/
			inhlt = 1;
			pc--;
			cycles -= 5;
			break;
		case 0xF5: case 0x1F5: case 0x2F5: case 0x3F5: /*CMC*/
			flags ^=C_FLAG;
			cycles -= 2;
			break;

		case 0xF6: case 0x1F6: case 0x2F6: case 0x3F6:
			fetchea();
			temp = getea8();
			switch(rmdat & 0x38) {
			case 0x00: /*TEST b, #8*/
				temp2 = fetch8();
				temp &= temp2;
				setznp8(temp);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				cycles -= ((mod == 3) ? 2 : 5);
				break;
			case 0x10: /*NOT b*/
				temp = ~temp;
				setea8(temp);
				cycles -= ((mod == 3) ? 2:6);
				break;
			case 0x18: /*NEG b*/
				setsub8(0, temp);
				temp = 0 - temp;
				setea8(temp);
				cycles -= ((mod == 3) ? 2:6);
				break;
			case 0x20: /*MUL AL, b*/
				AX = AL*temp;
				if(AH) flags |= (C_FLAG | V_FLAG);
				else    flags &= ~(C_FLAG | V_FLAG);
				cycles -= 13;
				break;
			case 0x28: /*IMUL AL, b*/
				tempws = (int)((int8)AL)*(int)((int8)temp);
				AX = tempws & 0xFFFF;
				if(AH && AH != 0xFF) flags |= (C_FLAG | V_FLAG);
				else		flags &= ~(C_FLAG | V_FLAG);
				cycles -= 14;
				break;
			case 0x30: /*DIV AL, b*/
				tempw = AX;
				if(temp) {
					tempw2 = tempw % temp;
					AH = tempw2 & 0xFF;
					tempw /= temp;
					AL = tempw & 0xFF;
				}
				else
					interrupt(DIVIDE_FAULT, 1);
				cycles -= 14;
				break;
			case 0x38: /*IDIV AL, b*/
				tempws = (int)(int16)AX;
				if(temp) {
					tempw2 = tempws % (int)((int8)temp);
					AH = tempw2 & 0xFF;
					tempws /= (int)((int8)temp);
					AL = tempws & 0xFF;
				}
				else
					interrupt(DIVIDE_FAULT, 1);
				cycles -= 19;
				break;

			default:
//				invalid();
				break;
			}
			break;

		case 0xF7: case 0x2F7:
			fetchea();
			tempw = getea16();
			switch(rmdat & 0x38) {
			case 0x00: /*TEST w*/
				tempw2 = fetch16();
				setznp16(tempw & tempw2);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				cycles -= ((mod == 3) ? 2 : 5);
				break;
			case 0x10: /*NOT w*/
				setea16(~tempw);
				cycles -= ((mod == 3) ? 2:6);
				break;
			case 0x18: /*NEG w*/
				setsub16(0, tempw);
				tempw = 0 - tempw;
				setea16(tempw);
				cycles -= ((mod == 3) ? 2:6);
				break;
			case 0x20: /*MUL AX, w*/
				templ = AX*tempw;
				AX = templ & 0xFFFF;
				DX = templ >> 16;
				if(DX)    flags |= (C_FLAG | V_FLAG);
				else       flags &= ~(C_FLAG | V_FLAG);
				cycles -= 21;
				break;
			case 0x28: /*IMUL AX, w*/
				tempws = (int)((int16)AX)*(int)((int16)tempw);
				if((tempws >> 15) && ((tempws >> 15) != -1)) flags |= (C_FLAG | V_FLAG);
				else					 flags &= ~(C_FLAG | V_FLAG);
				AX = tempws & 0xFFFF;
				tempws = (uint16)(tempws >> 16);
				DX = tempws & 0xFFFF;
				cycles -= 22;
				break;
			case 0x30: /*DIV AX, w*/
				templ = (DX << 16) | AX;
				if(tempw) {
					tempw2 = templ%tempw;
					DX = tempw2;
					templ /= tempw;
					AX = templ & 0xFFFF;
				}
				else
					interrupt(DIVIDE_FAULT, 1);
				cycles -= 22;
				break;
			case 0x38: /*IDIV AX, w*/
				tempws = (int)((DX << 16) | AX);
				if(tempw) {
					tempw2 = tempws % (int)((int16)tempw);
					DX = tempw2;
					tempws /= (int)((int16)tempw);
					AX = tempws & 0xFFFF;
				}
				else
					interrupt(DIVIDE_FAULT, 1);
				cycles -= 27;
				break;

			default:
//				invalid();
				break;
			}
			break;
		case 0x1F7: case 0x3F7:
			fetchea();
			templ = getea32();
			switch(rmdat & 0x38) {
			case 0x00: /*TEST l*/
				templ2 = fetch32();
				setznp32(templ&templ2);
				flags &= ~(C_FLAG | V_FLAG | A_FLAG);
				cycles -= ((mod == 3) ? 2 : 5);
				break;
			case 0x10: /*NOT l*/
				setea32(~templ);
				cycles -= ((mod == 3) ? 2:6);
				break;
			case 0x18: /*NEG l*/
				setsub32(0, templ);
				templ = 0 - templ;
				setea32(templ);
				cycles -= ((mod == 3) ? 2:6);
				break;
			case 0x20: /*MUL EAX, l*/
				temp64 = (uint64)EAX*(uint64)templ;
				EAX = temp64 & 0xFFFFFFFF;
				EDX = temp64 >> 32;
				if(EDX) flags |=  (C_FLAG | V_FLAG);
				else     flags &= ~(C_FLAG | V_FLAG);
				cycles -= 21;
				break;
			case 0x28: /*IMUL EAX, l*/
				temp64i = (int64)(int32)EAX*(int64)(int32)templ;
				EAX = temp64i & 0xFFFFFFFF;
				EDX = temp64i >> 32;
				if((temp64i >> 31) && ((temp64i >> 31) != -1)) flags |= (C_FLAG | V_FLAG);
				else					   flags &= ~(C_FLAG | V_FLAG);
				cycles -= 38;
				break;
			case 0x30: /*DIV EAX, l*/
				temp64 = ((uint64)EDX << 32) | EAX;
				if(templ) {
					templ2 = temp64%templ;
					EDX = templ2;
					temp64 /= templ;
					EAX = temp64 & 0xFFFFFFFF;
				}
				else
					interrupt(DIVIDE_FAULT, 1);
				cycles -= 38;
				break;
			case 0x38: /*IDIV EAX, l*/
				temp64i = (int64)((uint64)EDX << 32) | EAX;
				if(templ) {
					templ2 = temp64i % (int)((int32)templ);
					EDX = templ2;
					temp64i /= (int)((int32)templ);
					EAX = temp64i & 0xFFFFFFFF;
				}
				else
					interrupt(DIVIDE_FAULT, 1);
				cycles -= 43;
				break;

			default:
//				invalid();
				break;
			}
			break;

		case 0xF8: case 0x1F8: case 0x2F8: case 0x3F8: /*CLC*/
			flags &= ~C_FLAG;
			cycles -= 2;
			break;
		case 0xF9: case 0x1F9: case 0x2F9: case 0x3F9: /*STC*/
			flags |= C_FLAG;
			cycles -= 2;
			break;
		case 0xFA: case 0x1FA: case 0x2FA: case 0x3FA: /*CLI*/
			if(!IOPLp) {
				general_protection_fault(0);
			}
			else
				flags &= ~I_FLAG;
			cycles -= 3;
			break;
		case 0xFB: case 0x1FB: case 0x2FB: case 0x3FB: /*STI*/
			if(!IOPLp)
				general_protection_fault(0);
			else
				flags |= I_FLAG;
			cycles -= 2;
			break;
		case 0xFC: case 0x1FC: case 0x2FC: case 0x3FC: /*CLD*/
			flags &= ~D_FLAG;
			cycles -= 2;
			break;
		case 0xFD: case 0x1FD: case 0x2FD: case 0x3FD: /*STD*/
			flags |= D_FLAG;
			cycles -= 2;
			break;

		case 0xFE: case 0x1FE: case 0x2FE: case 0x3FE: /*INC / DEC b*/
			fetchea();
			temp = getea8();
			flags &= ~V_FLAG;
			if(rmdat & 0x38) {
				setsub8nc(temp, 1);
				temp2 = temp - 1;
				if((temp & 0x80) && !(temp2 & 0x80)) flags |= V_FLAG;
			}
			else {
				setadd8nc(temp, 1);
				temp2 = temp + 1;
				if((temp2 & 0x80) && !(temp & 0x80)) flags |= V_FLAG;
			}
			setea8(temp2);
			cycles -= ((mod == 3) ? 2:6);
			break;

		case 0xFF: case 0x2FF:
			fetchea();
			switch(rmdat & 0x38) {
			case 0x00: /*INC w*/
				tempw = getea16();
				setadd16nc(tempw, 1);
				setea16(tempw + 1);
				cycles -= ((mod == 3) ? 2:6);
				break;
			case 0x08: /*DEC w*/
				tempw = getea16();
				setsub16nc(tempw, 1);
				setea16(tempw - 1);
				cycles -= ((mod == 3) ? 2:6);
				break;
			case 0x10: /*CALL*/
				tempw = getea16();
				if(ssegs)
					ss = oldss;
				if(stack32) {
					WM16(ss, ESP - 2, pc);
					ESP -= 2;
				}
				else {
					WM16(ss, (SP - 2) & 0xFFFF, pc);
					SP -= 2;
				}
				pc = tempw;
				cycles -= ((mod == 3) ? 7 : 10);
				break;
			case 0x18: /*CALL far*/
				tempw = RM16(easeg, eaaddr);
				tempw2 = RM16(easeg, (eaaddr + 2)); 
#ifdef I386_BIOS_CALL
				if(d_bios) {
					uint16 _regs[8], _sregs[6];
					int32 _zf, _cf;
					for(int i = 0; i < 8; i++)
						_regs[i] = regs[i].w;
					_sregs[0] = CS;
					_sregs[1] = DS;
					_sregs[2] = ES;
					_sregs[3] = SS;
					_sregs[4] = FS;
					_sregs[5] = GS;
					_zf = ((flags & Z_FLAG) != 0);
					_cf = ((flags & C_FLAG) != 0);
					uint32 newpc = tempw + (tempw2 << 4);
					if(d_bios->bios_call(newpc, _regs, _sregs, &_zf, &_cf)) {
						for(int i = 0; i < 8; i++)
							regs[i].w = _regs[i];
						CS = _sregs[0];
						DS = _sregs[1];
						ES = _sregs[2];
						SS = _sregs[3];
						FS = _sregs[4];
						GS = _sregs[5];
						flags &= ~(Z_FLAG | C_FLAG);
						if(_zf)
							flags |= Z_FLAG;
						if(_cf)
							flags |= C_FLAG;
						cycles -= 100;
						break;
					}
				}
#endif
				tempw3 = CS;
				tempw4 = pc;
				if(ssegs) ss = oldss;
				oxpc = pc;
				pc = tempw;
				optype = CALL;
				if(msw & 1) loadcscall(tempw2);
				else       loadcs(tempw2);
				if(notpresent) break;
				if(stack32) {
					WM16(ss, ESP - 2, tempw3);
					WM16(ss, ESP - 4, tempw4);
					ESP -= 4;
				}
				else {
					WM16(ss, (SP - 2) & 0xFFFF, tempw3);
					WM16(ss, ((SP - 4) & 0xFFFF), tempw4);
					SP -= 4;
				}
				cycles -= 22;
				break;
			case 0x20: /*JMP*/
				pc = getea16();
				cycles -= ((mod == 3) ? 7 : 10);
				break;
			case 0x28: /*JMP far*/
				oxpc = pc;
				pc = RM16(easeg, eaaddr); 
				optype = JMP;
				loadcs(RM16(easeg, (eaaddr + 2))); 
				cycles -= 12;
				break;
			case 0x30: /*PUSH w*/
				tempw = getea16();
				if(ssegs) ss = oldss;
				if(stack32) {
					WM16(ss, ESP - 2, tempw);
					ESP -= 2;
				}
				else {
					WM16(ss, ((SP - 2) & 0xFFFF), tempw);
					SP -= 2;
				}
				cycles -= ((mod == 3) ? 2 : 5);
				break;

			default:
				invalid();
				break;
			}
			break;
		case 0x1FF: case 0x3FF:
			fetchea();
			switch(rmdat & 0x38) {
			case 0x00: /*INC l*/
				templ = getea32();
				setadd32nc(templ, 1);
				setea32(templ + 1);
				cycles -= ((mod == 3) ? 2:6);
				break;
			case 0x08: /*DEC l*/
				templ = getea32();
				setsub32nc(templ, 1);
				setea32(templ - 1);
				cycles -= ((mod == 3) ? 2:6);
				break;
			case 0x10: /*CALL*/
				templ = getea32();
				if(ssegs)
					ss = oldss;
				if(stack32) {
					WM32(ss, ESP - 4, pc);
					ESP -= 4;
				}
				else {
					WM32(ss, (SP - 4) & 0xFFFF, pc);
					SP -= 4;
				}
				pc = templ;
				cycles -= ((mod == 3) ? 7 : 10);
				break;
			case 0x18: /*CALL far*/
				templ = RM32(easeg, eaaddr);
				tempw2 = RM16(easeg, (eaaddr + 4)); 
#ifdef I386_BIOS_CALL
				if(d_bios) {
					uint16 _regs[8], _sregs[6];
					int32 _zf, _cf;
					for(int i = 0; i < 8; i++)
						_regs[i] = regs[i].w;
					_sregs[0] = CS;
					_sregs[1] = DS;
					_sregs[2] = ES;
					_sregs[3] = SS;
					_sregs[4] = FS;
					_sregs[5] = GS;
					_zf = ((flags & Z_FLAG) != 0);
					_cf = ((flags & C_FLAG) != 0);
					uint32 newpc = tempw + (tempw2 << 4);
					if(d_bios->bios_call(newpc, _regs, _sregs, &_zf, &_cf)) {
						for(int i = 0; i < 8; i++)
							regs[i].w = _regs[i];
						CS = _sregs[0];
						DS = _sregs[1];
						ES = _sregs[2];
						SS = _sregs[3];
						FS = _sregs[4];
						GS = _sregs[5];
						flags &= ~(Z_FLAG | C_FLAG);
						if(_zf)
							flags |= Z_FLAG;
						if(_cf)
							flags |= C_FLAG;
						cycles -= 100;
						break;
					}
				}
#endif
				tempw3 = CS;
				templ2 = pc;
				if(ssegs)
					ss = oldss;
				oxpc = pc;
				pc = templ;
				optype = CALL;
				if(msw & 1) loadcscall(tempw2);
				else       loadcs(tempw2);
				if(notpresent) break;
				if(stack32) {
					WM32(ss, ESP - 4, tempw3);
					WM32(ss, ESP - 8, templ2);
					ESP -= 8;
				}
				else {
					WM32(ss, (SP - 4) & 0xFFFF, tempw3);
					WM32(ss, (SP - 8) & 0xFFFF, templ2);
					SP -= 8;
				}
				cycles -= 22;
				break;
			case 0x20: /*JMP*/
				pc = getea32();
				cycles -= ((mod == 3) ? 7 : 12);
				break;
			case 0x28: /*JMP far*/
				oxpc = pc;
				pc = RM32(easeg, eaaddr);
				optype = JMP;
				loadcs(RM16(easeg, (eaaddr + 4)));
				cycles -= 12;
				break;
			case 0x30: /*PUSH l*/
				templ = getea32();
				if(ssegs)
					ss = oldss;
				if(stack32) {
					WM32(ss, ESP - 4, templ);
					ESP -= 4;
				}
				else {
					WM32(ss, ((SP - 4) & 0xFFFF), templ);
					SP -= 4;
				}
				cycles -= ((mod == 3) ? 2 : 5);
				break;
			default:
				invalid();
				break;
			}
			break;
		default:
			invalid();
			break;
		}
		if(!use32)
			pc &= 0xFFFF;
		if(ssegs) {
			ds = oldds;
			ss = oldss;
			ssegs = 0;
		}
		if(abrt) {
			pc = oldpc;
			pmodeint(PAGE_FAULT, 0);
			if(stack32) {
				WM32(ss, (ESP - 4), (uint32)(abrt >> 8));
				ESP -= 4;
			}
			else {
				WM32(ss, ((SP - 4) & 0xFFFF), (uint32)(abrt >> 8));
				SP -= 4;
			}
			abrt = 0;
		}
		if(notpresent) {
			CS = oldcs;
			pc = oldpc;
			_cs.access = oldcpl << 5;
			notpresent = 0;
			not_present_fault();
		}
		
		// interrupt
		if(intstat & NMI_REQ_BIT) {
			if(inhlt)
				pc++;
			intstat &= ~NMI_REQ_BIT;
			interrupt(NMI_INT_VECTOR, 1);
		}
		else if((intstat & INT_REQ_BIT) && (flags & I_FLAG) && !ssegs && !noint) {
			if(inhlt)
				pc++;
			int intnum = ACK_INTR() & 0xff;
			intstat &= ~INT_REQ_BIT;
			interrupt(intnum, 1);
		}
	}
//	tsc += base_cycles - cycles;
	base_cycles = cycles;
}

