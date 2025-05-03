/*
	Skelton for retropc emulator

	Origin : MAME i386 core
	Author : Takeda.Toshiya
	Date  : 2009.06.08-

	[ i386/i486/Pentium/MediaGX ]
*/

#include "i386.h"

#define ES	0
#define CS	1
#define SS	2
#define DS	3
#define FS	4
#define GS	5

#ifdef _BIG_ENDIAN
#define AL	3
#define AH	2
#define CL	7
#define CH	6
#define DL	11
#define DH	10
#define BL	15
#define BH	14
#define AX	1
#define CX	3
#define DX	5
#define BX	7
#define SP	9
#define BP	11
#define SI	13
#define DI	15
#else
#define AL	0
#define AH	1
#define CL	4
#define CH	5
#define DL	8
#define DH	9
#define BL	12
#define BH	13
#define AX	0
#define CX	2
#define DX	4
#define BX	6
#define SP	8
#define BP	10
#define SI	12
#define DI	14
#endif

#define EAX	0
#define ECX	1
#define EDX	2
#define EBX	3
#define ESP	4
#define EBP	5
#define ESI	6
#define EDI	7

#define PROTECTED_MODE		(cr[0] & 1)
#define STACK_32BIT		(sreg[SS].d)
#define V8086_MODE		(eflags & 0x00020000)
#define CPL			((sreg[CS].flags >> 5) & 3)

#define SetOF_Add32(r, s, d)	(OF = (((r) ^ (s)) & ((r) ^ (d)) & 0x80000000) ? 1: 0)
#define SetOF_Add16(r, s, d)	(OF = (((r) ^ (s)) & ((r) ^ (d)) & 0x8000) ? 1 : 0)
#define SetOF_Add8(r, s, d)	(OF = (((r) ^ (s)) & ((r) ^ (d)) & 0x80) ? 1 : 0)
#define SetOF_Sub32(r, s, d)	(OF = (((d) ^ (s)) & ((d) ^ (r)) & 0x80000000) ? 1 : 0)
#define SetOF_Sub16(r, s, d)	(OF = (((d) ^ (s)) & ((d) ^ (r)) & 0x8000) ? 1 : 0)
#define SetOF_Sub8(r, s, d)	(OF = (((d) ^ (s)) & ((d) ^ (r)) & 0x80) ? 1 : 0)
#define SetCF8(x)		{CF = ((x) & 0x100) ? 1 : 0; }
#define SetCF16(x)		{CF = ((x) & 0x10000) ? 1 : 0; }
#define SetCF32(x)		{CF = ((x) & (((uint64)1) << 32)) ? 1 : 0; }
#define SetSF(x)		(SF = (x))
#define SetZF(x)		(ZF = (x))
#define SetAF(x, y, z)		(AF = (((x) ^ ((y) ^ (z))) & 0x10) ? 1 : 0)
#define SetPF(x)		(PF = parity_table[(x) & 0xFF])
#define SetSZPF8(x)		{ZF = ((uint8)(x) == 0);  SF = ((x)&0x80) ? 1 : 0; PF = parity_table[x & 0xFF]; }
#define SetSZPF16(x)		{ZF = ((uint16)(x) == 0);  SF = ((x)&0x8000) ? 1 : 0; PF = parity_table[x & 0xFF]; }
#define SetSZPF32(x)		{ZF = ((uint32)(x) == 0);  SF = ((x)&0x80000000) ? 1 : 0; PF = parity_table[x & 0xFF]; }

#define REG8(x)			(reg.b[x])
#define REG16(x)		(reg.w[x])
#define REG32(x)		(reg.d[x])
#define LOAD_REG8(x)		(reg.b[modrm_table[x].reg.b])
#define LOAD_REG16(x)		(reg.w[modrm_table[x].reg.w])
#define LOAD_REG32(x)		(reg.d[modrm_table[x].reg.d])
#define LOAD_RM8(x)		(reg.b[modrm_table[x].rm.b])
#define LOAD_RM16(x)		(reg.w[modrm_table[x].rm.w])
#define LOAD_RM32(x)		(reg.d[modrm_table[x].rm.d])
#define STORE_REG8(x, v)	(reg.b[modrm_table[x].reg.b] = (v))
#define STORE_REG16(x, v)	(reg.w[modrm_table[x].reg.w] = (v))
#define STORE_REG32(x, v)	(reg.d[modrm_table[x].reg.d] = (v))
#define STORE_RM8(x, v)		(reg.b[modrm_table[x].rm.b] = (v))
#define STORE_RM16(x, v)	(reg.w[modrm_table[x].rm.w] = (v))
#define STORE_RM32(x, v)	(reg.d[modrm_table[x].rm.d] = (v))

#define ST(x)			(fpu.reg[(fpu.top + (x)) & 7])
#define FPU_INFINITY_DOUBLE	U64(0x7ff0000000000000)
#define FPU_INFINITY_SINGLE	(0x7f800000)
#define FPU_SIGN_BIT_DOUBLE	U64(0x8000000000000000)
#define FPU_SIGN_BIT_SINGLE	(0x80000000)

#define FPU_MASK_INVALID_OP		0x0001
#define FPU_MASK_DENORMAL_OP		0x0002
#define FPU_MASK_ZERO_DIVIDE		0x0004
#define FPU_MASK_OVERFLOW		0x0008
#define FPU_MASK_UNDERFLOW		0x0010
#define FPU_MASK_PRECISION		0x0020
#define FPU_BUSY			0x8000
#define FPU_C3				0x4000
#define FPU_STACK_TOP_MASK		0x3800
#define FPU_C2				0x0400
#define FPU_C1				0x0200
#define FPU_C0				0x0100
#define FPU_ERROR_SUMMARY		0x0080
#define FPU_STACK_FAULT			0x0040
#define FPU_EXCEPTION_PRECISION		0x0020
#define FPU_EXCEPTION_UNDERFLOW		0x0010
#define FPU_EXCEPTION_OVERFLOW		0x0008
#define FPU_EXCEPTION_ZERO_DIVIDE	0x0004
#define FPU_EXCEPTION_DENORMAL_OP	0x0002
#define FPU_EXCEPTION_INVALID_OP	0x0001

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
#define U64(x)		(uint64)(x)

#ifdef I386_BIOS_CALL
#define BIOS_INT(num) if(d_bios) { \
	uint16 regs[8], sregs[4]; \
	regs[0] = REG16(AX); regs[1] = REG16(CX); regs[2] = REG16(DX); regs[3] = REG16(BX); \
	regs[4] = REG16(SP); regs[5] = REG16(BP); regs[6] = REG16(SI); regs[7] = REG16(DI); \
	sregs[0] = sreg[ES].selector; sregs[1] = sreg[CS].selector; sregs[2] = sreg[SS].selector; sregs[3] = sreg[DS].selector; \
	if(d_bios->bios_int(num, regs, sregs, &ZF, &CF)) return; \
}
#define BIOS_CALL_NEAR16() if(d_bios) { \
	uint16 regs[8], sregs[4]; \
	regs[0] = REG16(AX); regs[1] = REG16(CX); regs[2] = REG16(DX); regs[3] = REG16(BX); \
	regs[4] = REG16(SP); regs[5] = REG16(BP); regs[6] = REG16(SI); regs[7] = REG16(DI); \
	sregs[0] = sreg[ES].selector; sregs[1] = sreg[CS].selector; sregs[2] = sreg[SS].selector; sregs[3] = sreg[DS].selector; \
	if(d_bios->bios_call(prev_pc, regs, sregs, &ZF, &CF)) ret_near16(); \
}
#define BIOS_CALL_NEAR32() if(d_bios) { \
	uint16 regs[8], sregs[4]; \
	regs[0] = REG16(AX); regs[1] = REG16(CX); regs[2] = REG16(DX); regs[3] = REG16(BX); \
	regs[4] = REG16(SP); regs[5] = REG16(BP); regs[6] = REG16(SI); regs[7] = REG16(DI); \
	sregs[0] = sreg[ES].selector; sregs[1] = sreg[CS].selector; sregs[2] = sreg[SS].selector; sregs[3] = sreg[DS].selector; \
	if(d_bios->bios_call(prev_pc, regs, sregs, &ZF, &CF)) ret_near32(); \
}
#define BIOS_CALL_FAR16() if(d_bios) { \
	uint16 regs[8], sregs[4]; \
	regs[0] = REG16(AX); regs[1] = REG16(CX); regs[2] = REG16(DX); regs[3] = REG16(BX); \
	regs[4] = REG16(SP); regs[5] = REG16(BP); regs[6] = REG16(SI); regs[7] = REG16(DI); \
	sregs[0] = sreg[ES].selector; sregs[1] = sreg[CS].selector; sregs[2] = sreg[SS].selector; sregs[3] = sreg[DS].selector; \
	if(d_bios->bios_call(prev_pc, regs, sregs, &ZF, &CF)) retf16(); \
}
#define BIOS_CALL_FAR32() if(d_bios) { \
	uint16 regs[8], sregs[4]; \
	regs[0] = REG16(AX); regs[1] = REG16(CX); regs[2] = REG16(DX); regs[3] = REG16(BX); \
	regs[4] = REG16(SP); regs[5] = REG16(BP); regs[6] = REG16(SI); regs[7] = REG16(DI); \
	sregs[0] = sreg[ES].selector; sregs[1] = sreg[CS].selector; sregs[2] = sreg[SS].selector; sregs[3] = sreg[DS].selector; \
	if(d_bios->bios_call(prev_pc, regs, sregs, &ZF, &CF)) retf32(); \
}
#endif

void I386::initialize()
{
	const int regs8[8] = {AL, CL, DL, BL, AH, CH, DH, BH};
	const int regs16[8] = {AX, CX, DX, BX, SP, BP, SI, DI};
	const int regs32[8] = {EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI};
	
	for(int i = 0; i < 256; i++) {
		int c = 0;
		for(int j = 0; j < 8; j++) {
			if(i & (1 << j))
				c++;
		}
		parity_table[i] = ~(c & 1) & 1;
	}
	for(int i = 0; i < 256; i++) {
		modrm_table[i].reg.b = regs8[(i >> 3) & 7];
		modrm_table[i].reg.w = regs16[(i >> 3) & 7];
		modrm_table[i].reg.d = regs32[(i >> 3) & 7];
		modrm_table[i].rm.b = regs8[i & 7];
		modrm_table[i].rm.w = regs16[i & 7];
		modrm_table[i].rm.d = regs32[i & 7];
	}
}

void I386::reset()
{
	_memset(&reg, 0, sizeof(reg));
	REG32(EAX) = 0x0308;	// Intel 386, stepping D1
	REG32(EDX) = 0;
	
	_memset(&sreg, 0, sizeof(sreg));
	sreg[CS].selector = 0xf000;
	sreg[CS].base = 0xffff0000;
	sreg[CS].limit = 0xffff;
#if defined(HAS_I486) || defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
	sreg[CS].flags    = 0x009b;
#endif
	sreg[DS].limit = sreg[ES].limit = sreg[FS].limit = sreg[GS].limit = sreg[SS].limit = 0xffff;
	sreg[DS].flags = sreg[ES].flags = sreg[FS].flags = sreg[GS].flags = sreg[SS].flags = 0x0092;
	
	_memset(&fpu, 0, sizeof(fpu));
	
	_memset(cr, 0, sizeof(cr));
	_memset(dr, 0, sizeof(dr));
	_memset(tr, 0, sizeof(tr));
	_memset(&gdtr, 0, sizeof(gdtr));
	_memset(&idtr, 0, sizeof(idtr));
	idtr.limit = 0x3ff;
	
	CF = DF = SF = OF = ZF = PF = AF = IF = TF = 0;
	eflags = 0;
	
	halted = busreq = irq_state = 0;
	operand_size = address_size = segment_prefix = segment_override = 0;
	cycles = base_cycles = 0;
	a20_mask = ~0;
#if defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
	tsc = 0;
#endif
	eip = 0xfff0;
	CHANGE_PC(eip);
}

void I386::run(int clock)
{
	// return now if halted or busreq
	if(busreq || halted) {
		cycles = base_cycles = 0;
#if defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
		tsc += cycles;
#endif
		return;
	}
	
	// run cpu whille given clocks
	cycles += clock;
	base_cycles = cycles;
	CHANGE_PC(eip);
	
	while(cycles > 0) {
//		prev_eip = eip;
//		prev_cs = sreg[CS].selector;
//		prev_cpl = sreg[CS].flags;
		operand_size = sreg[CS].d;
		address_size = sreg[CS].d;
		segment_prefix = 0;
		if(irq_state & NMI_REQ_BIT) {
			halted = 0;
			irq_state &= ~NMI_REQ_BIT;
			trap(NMI_INT_VECTOR, 1);
		}
		else if((irq_state & INT_REQ_BIT) && IF) {
			halted = 0;
			irq_state &= ~INT_REQ_BIT;
			int interrupt = ACK_INTR() & 0xff;
			trap(interrupt, 1);
		}
		decode_opcode();
	}
#if defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
	tsc += (base_cycles - cycles);
#endif
	base_cycles = cycles;
}

void I386::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_CPU_NMI) {
		if(data & mask)
			irq_state |= NMI_REQ_BIT;
		else
			irq_state &= ~NMI_REQ_BIT;
	}
	else if(id == SIG_CPU_BUSREQ) {
		busreq = ((data & mask) != 0);
		if(busreq)
			cycles = base_cycles = 0;
	}
	else if(id == SIG_I386_A20)
		a20_mask = (data & mask) ? ~0 : ~(1 << 20);
}


void I386::set_intr_line(bool line, bool pending, uint32 bit)
{
	if(line)
		irq_state |= INT_REQ_BIT;
	else
		irq_state &= ~INT_REQ_BIT;
}

void I386::load_protected_mode_segment(sreg_c *seg)
{
	uint32 base, limit;
	if(seg->selector & 4) {
		base = ldtr.base;
		limit = ldtr.limit;
	}
	else {
		base = gdtr.base;
		limit = gdtr.limit;
	}
	if(limit == 0 || (uint32)(seg->selector + 7) > limit)
		return;
	int entry = seg->selector & ~7;
	uint32 v1 = RM32(base + entry);
	uint32 v2 = RM32(base + entry + 4);
	
	seg->flags = (v2 >> 8) & 0xf0ff;
	seg->base = (v2 & 0xff000000) | ((v2 & 0xff) << 16) | ((v1 >> 16) & 0xffff);
	seg->limit = (v2 & 0xf0000) | (v1 & 0xffff);
	if(seg->flags & 0x8000)
		seg->limit = (seg->limit << 12) | 0xfff;
	seg->d = (seg->flags & 0x4000) ? 1 : 0;
}

void I386::load_segment_descriptor(int segment)
{
	if(PROTECTED_MODE) {
		if(!V8086_MODE)
			load_protected_mode_segment(&sreg[segment]);
		else {
			sreg[segment].base = sreg[segment].selector << 4;
			sreg[segment].limit = 0xffff;
			sreg[segment].flags = (segment == CS) ? 0x009a : 0x0092;
		}
	}
	else {
		sreg[segment].base = sreg[segment].selector << 4;
		if(segment == CS && !performed_intersegment_jump)
			sreg[segment].base |= 0xfff00000;
	}
}

inline uint32 I386::get_flags()
{
	uint32 f = 0x2;
	f |= CF;
	f |= PF << 2;
	f |= AF << 4;
	f |= ZF << 6;
	f |= SF << 7;
	f |= TF << 8;
	f |= IF << 9;
	f |= DF << 10;
	f |= OF << 11;
	return (eflags & 0xFFFF0000) | (f & 0xFFFF);
}

inline void I386::set_flags(uint32 f)
{
	CF = (f & 0x1) ? 1 : 0;
	PF = (f & 0x4) ? 1 : 0;
	AF = (f & 0x10) ? 1 : 0;
	ZF = (f & 0x40) ? 1 : 0;
	SF = (f & 0x80) ? 1 : 0;
	TF = (f & 0x100) ? 1 : 0;
	IF = (f & 0x200) ? 1 : 0;
	DF = (f & 0x400) ? 1 : 0;
	OF = (f & 0x800) ? 1 : 0;
}

void I386::sib_byte(uint8 mod, uint32* out_ea, uint8* out_segment)
{
	uint32 ea = 0;
	uint8 segment = 0;
	uint8 sib = FETCH8();
	uint8 scale = (sib >> 6) & 3;
	uint8 i = (sib >> 3) & 7;
	uint8 base = sib & 7;
	
	switch(base)
	{
	case 0: ea = REG32(EAX); segment = DS; break;
	case 1: ea = REG32(ECX); segment = DS; break;
	case 2: ea = REG32(EDX); segment = DS; break;
	case 3: ea = REG32(EBX); segment = DS; break;
	case 4: ea = REG32(ESP); segment = SS; break;
	case 5:
		if(mod == 0) {
			ea = FETCH32();
			segment = DS;
		}
		else if(mod == 1) {
			ea = REG32(EBP);
			segment = SS;
		}
		else if(mod == 2) {
			ea = REG32(EBP);
			segment = SS;
		}
		break;
	case 6: ea = REG32(ESI); segment = DS; break;
	case 7: ea = REG32(EDI); segment = DS; break;
	}
	switch(i)
	{
	case 0: ea += REG32(EAX) * (1 << scale); break;
	case 1: ea += REG32(ECX) * (1 << scale); break;
	case 2: ea += REG32(EDX) * (1 << scale); break;
	case 3: ea += REG32(EBX) * (1 << scale); break;
	case 4: break;
	case 5: ea += REG32(EBP) * (1 << scale); break;
	case 6: ea += REG32(ESI) * (1 << scale); break;
	case 7: ea += REG32(EDI) * (1 << scale); break;
	}
	*out_ea = ea;
	*out_segment = segment;
}

void I386::modrm_to_EA(uint8 mod_rm, uint32* out_ea, uint8* out_segment)
{
	uint8 mod = (mod_rm >> 6) & 3;
	uint8 rm = mod_rm & 7;
	uint32 ea;
	uint8 segment;
	
	if(address_size) {
		switch(rm)
		{
		default:
		case 0: ea = REG32(EAX); segment = DS; break;
		case 1: ea = REG32(ECX); segment = DS; break;
		case 2: ea = REG32(EDX); segment = DS; break;
		case 3: ea = REG32(EBX); segment = DS; break;
		case 4: sib_byte(mod, &ea, &segment); break;
		case 5:
			if(mod == 0) {
				ea = FETCH32(); segment = DS;
			}
			else {
				ea = REG32(EBP); segment = SS;
			}
			break;
		case 6: ea = REG32(ESI); segment = DS; break;
		case 7: ea = REG32(EDI); segment = DS; break;
		}
		if(mod == 1) {
			int8 disp8 = FETCH8();
			ea += (int32)disp8;
		}
		else if(mod == 2) {
			int32 disp32 = FETCH32();
			ea += disp32;
		}
		if(segment_prefix)
			segment = segment_override;
		*out_ea = ea;
		*out_segment = segment;
	}
	else {
		switch(rm)
		{
		default:
		case 0: ea = REG16(BX) + REG16(SI); segment = DS; break;
		case 1: ea = REG16(BX) + REG16(DI); segment = DS; break;
		case 2: ea = REG16(BP) + REG16(SI); segment = SS; break;
		case 3: ea = REG16(BP) + REG16(DI); segment = SS; break;
		case 4: ea = REG16(SI); segment = DS; break;
		case 5: ea = REG16(DI); segment = DS; break;
		case 6:
			if(mod == 0) {
				ea = FETCH16(); segment = DS;
			}
			else {
				ea = REG16(BP); segment = SS;
			}
			break;
		case 7: ea = REG16(BX); segment = DS; break;
		}
		if(mod == 1) {
			int8 disp8 = FETCH8();
			ea += (int32)disp8;
		}
		else if(mod == 2) {
			int16 disp16 = FETCH16();
			ea += (int32)disp16;
		}
		if(segment_prefix)
			segment = segment_override;
		*out_ea = ea & 0xffff;
		*out_segment = segment;
	}
}

uint32 I386::GetNonTranslatedEA(uint8 modrm)
{
	uint8 segment;
	uint32 ea;
	modrm_to_EA(modrm, &ea, &segment);
	return ea;
}

uint32 I386::GetEA(uint8 modrm)
{
	uint8 segment;
	uint32 ea;
	modrm_to_EA(modrm, &ea, &segment);
	return translate(segment, ea);
}

inline uint32 I386::translate(int segment, uint32 ip)
{
	// TODO: segment limit
	return sreg[segment].base + ip;
}

inline void I386::CHANGE_PC(uint32 newpc)
{
	pc = translate(CS, newpc);
	uint32 address = pc;
	if(cr[0] & 0x80000000)
		translate_address(&address);
}

inline void I386::NEAR_BRANCH(int32 offs)
{
	/* TODO: limit */
	eip += offs;
	pc += offs;
	uint32 address = pc;
	if(cr[0] & 0x80000000)
		translate_address(&address);
}

void I386::trap(int irq, int irq_gate)
{
	int entry = irq * (sreg[CS].d ? 8 : 4);
	
	/* Check if IRQ is out of IDTR's bounds */
	if(entry > idtr.limit) {
//		fatalerror(_T("I386 Interrupt: IRQ out of IDTR bounds (IRQ: %d, IDTR Limit: %d)\n"), irq, idtr.limit);
	}

	if(!sreg[CS].d) {
		/* 16-bit */
		PUSH16(get_flags() & 0xffff);
		PUSH16(sreg[CS].selector);
		PUSH16(eip);
		
		sreg[CS].selector = RM16(idtr.base + entry + 2);
		eip = RM16(idtr.base + entry);
		
		// Interrupts that vector through either interrupt gates or trap gates cause TF
		// (the trap flag) to be reset after the current value of TF is saved on the stack as part of EFLAGS.
		TF = 0;
		if(irq_gate)
			IF = 0;
	}
	else {
		/* 32-bit */
		PUSH32(get_flags() & 0x00fcffff);
		PUSH32(sreg[CS].selector);
		PUSH32(eip);
		
		uint32 v1 = RM32(idtr.base + entry);
		uint32 v2 = RM32(idtr.base + entry + 4);
		uint32 offset = (v2 & 0xffff0000) | (v1 & 0xffff);
		uint16 segment = (v1 >> 16) & 0xffff;
		int type = (v2>>8) & 0x1F;
		sreg[CS].selector = segment;
		eip = offset;
		
		// Interrupts that vector through either interrupt gates or trap gates cause TF
		// (the trap flag) to be reset after the current value of TF is saved on the stack as part of EFLAGS.
		if((type == 14) || (type==15))
			TF = 0;
		if(type == 14)
			IF = 0;
	}
	load_segment_descriptor(CS);
	CHANGE_PC(eip);
}

//void I386::general_protection_fault(uint16 error)
//{
//	sreg[CS].selector = prev_cs;
//	load_segment_descriptor(CS);
//	sreg[CS].flags = prev_cpl & 0x60;
//	eip = prev_eip;
//	CHANGE_PC(eip);
//	trap(GENERAL_PROTECTION_FAULT, 0);
//	if(!sreg[CS].d)
//		PUSH16(error);
//	else
//		PUSH32(error);
//}

#define CYCLES_NUM(x)	(cycles -= (x))

inline void I386::CYCLES(int x)
{
	if(PROTECTED_MODE)
		cycles -= cycle_table[x][1];
	else
		cycles -= cycle_table[x][0];
}

inline void I386::CYCLES_RM(int modrm, int r, int m)
{
	if(modrm >= 0xc0) {
		if(PROTECTED_MODE)
			cycles -= cycle_table[r][1];
		else
			cycles -= cycle_table[r][0];
	}
	else {
		if(PROTECTED_MODE)
			cycles -= cycle_table[m][1];
		else
			cycles -= cycle_table[m][0];
	}
}

void I386::decode_opcode()
{
	opcode = FETCHOP();
	if(operand_size) {
		switch(opcode)
		{
		case 0x00: add_rm8_r8(); break;
		case 0x01: add_rm32_r32(); break;
		case 0x02: add_r8_rm8(); break;
		case 0x03: add_r32_rm32(); break;
		case 0x04: add_al_i8(); break;
		case 0x05: add_eax_i32(); break;
		case 0x06: push_es32(); break;
		case 0x07: pop_es32(); break;
		case 0x08: or_rm8_r8(); break;
		case 0x09: or_rm32_r32(); break;
		case 0x0A: or_r8_rm8(); break;
		case 0x0B: or_r32_rm32(); break;
		case 0x0C: or_al_i8(); break;
		case 0x0D: or_eax_i32(); break;
		case 0x0E: push_cs32(); break;
		case 0x0F:
			opcode = FETCH8();
			switch(opcode)
			{
			case 0x00: group0F00_32(); break;
			case 0x01: group0F01_32(); break;
			case 0x02: unimplemented(); break;
			case 0x03: unimplemented(); break;
			case 0x06: clts(); break;
#if defined(HAS_I486) || defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
			case 0x08: invd(); break;
			case 0x09: wbinvd(); break;
#endif
			case 0x0B: unimplemented(); break;
			case 0x20: mov_r32_cr(); break;
			case 0x21: mov_r32_dr(); break;
			case 0x22: mov_cr_r32(); break;
			case 0x23: mov_dr_r32(); break;
			case 0x24: mov_r32_tr(); break;
			case 0x26: mov_tr_r32(); break;
#if defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
			case 0x30: wrmsr(); break;
			case 0x31: rdtsc(); break;
			case 0x32: rdmsr(); break;
#endif
#if defined(HAS_MEDIAGX)
			case 0x74: cyrix_unknown(); break;
#endif
			case 0x80: jo_rel32(); break;
			case 0x81: jno_rel32(); break;
			case 0x82: jc_rel32(); break;
			case 0x83: jnc_rel32(); break;
			case 0x84: jz_rel32(); break;
			case 0x85: jnz_rel32(); break;
			case 0x86: jbe_rel32(); break;
			case 0x87: ja_rel32(); break;
			case 0x88: js_rel32(); break;
			case 0x89: jns_rel32(); break;
			case 0x8A: jp_rel32(); break;
			case 0x8B: jnp_rel32(); break;
			case 0x8C: jl_rel32(); break;
			case 0x8D: jge_rel32(); break;
			case 0x8E: jle_rel32(); break;
			case 0x8F: jg_rel32(); break;
			case 0x90: seto_rm8(); break;
			case 0x91: setno_rm8(); break;
			case 0x92: setc_rm8(); break;
			case 0x93: setnc_rm8(); break;
			case 0x94: setz_rm8(); break;
			case 0x95: setnz_rm8(); break;
			case 0x96: setbe_rm8(); break;
			case 0x97: seta_rm8(); break;
			case 0x98: sets_rm8(); break;
			case 0x99: setns_rm8(); break;
			case 0x9A: setp_rm8(); break;
			case 0x9B: setnp_rm8(); break;
			case 0x9C: setl_rm8(); break;
			case 0x9D: setge_rm8(); break;
			case 0x9E: setle_rm8(); break;
			case 0x9F: setg_rm8(); break;
			case 0xA0: push_fs32(); break;
			case 0xA1: pop_fs32(); break;
#if defined(HAS_I486) || defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
			case 0xA2: cpuid(); break;
#endif
			case 0xA3: bt_rm32_r32(); break;
			case 0xA4: shld32_i8(); break;
			case 0xA5: shld32_cl(); break;
			case 0xA8: push_gs32(); break;
			case 0xA9: pop_gs32(); break;
			case 0xAA: unimplemented(); break;
			case 0xAB: bts_rm32_r32(); break;
			case 0xAC: shrd32_i8(); break;
			case 0xAD: shrd32_cl(); break;
			case 0xAE: invalid(); break;
			case 0xAF: imul_r32_rm32(); break;
#if defined(HAS_I486) || defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
			case 0xB0: cmpxchg_rm8_r8(); break;
			case 0xB1: cmpxchg_rm32_r32(); break;
#endif
			case 0xB2: lss32(); break;
			case 0xB3: btr_rm32_r32(); break;
			case 0xB4: lfs32(); break;
			case 0xB5: lgs32(); break;
			case 0xB6: movzx_r32_rm8(); break;
			case 0xB7: movzx_r32_rm16(); break;
			case 0xBA: group0FBA_32(); break;
			case 0xBB: btc_rm32_r32(); break;
			case 0xBC: bsf_r32_rm32(); break;
			case 0xBD: bsr_r32_rm32(); break;
			case 0xBE: movsx_r32_rm8(); break;
			case 0xBF: movsx_r32_rm16(); break;
#if defined(HAS_I486) || defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
			case 0xC0: xadd_rm8_r8(); break;
			case 0xC1: xadd_rm32_r32(); break;
#endif
#if defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
			case 0xC7: cmpxchg8b_m64(); break;
#endif
			default: invalid(); break;
			}
			break;
		case 0x10: adc_rm8_r8(); break;
		case 0x11: adc_rm32_r32(); break;
		case 0x12: adc_r8_rm8(); break;
		case 0x13: adc_r32_rm32(); break;
		case 0x14: adc_al_i8(); break;
		case 0x15: adc_eax_i32(); break;
		case 0x16: push_ss32(); break;
		case 0x17: pop_ss32(); break;
		case 0x18: sbb_rm8_r8(); break;
		case 0x19: sbb_rm32_r32(); break;
		case 0x1A: sbb_r8_rm8(); break;
		case 0x1B: sbb_r32_rm32(); break;
		case 0x1C: sbb_al_i8(); break;
		case 0x1D: sbb_eax_i32(); break;
		case 0x1E: push_ds32(); break;
		case 0x1F: pop_ds32(); break;
		case 0x20: and_rm8_r8(); break;
		case 0x21: and_rm32_r32(); break;
		case 0x22: and_r8_rm8(); break;
		case 0x23: and_r32_rm32(); break;
		case 0x24: and_al_i8(); break;
		case 0x25: and_eax_i32(); break;
		case 0x26: segment_ES(); break;
		case 0x27: daa(); break;
		case 0x28: sub_rm8_r8(); break;
		case 0x29: sub_rm32_r32(); break;
		case 0x2A: sub_r8_rm8(); break;
		case 0x2B: sub_r32_rm32(); break;
		case 0x2C: sub_al_i8(); break;
		case 0x2D: sub_eax_i32(); break;
		case 0x2E: segment_CS(); break;
		case 0x2F: das(); break;
		case 0x30: xor_rm8_r8(); break;
		case 0x31: xor_rm32_r32(); break;
		case 0x32: xor_r8_rm8(); break;
		case 0x33: xor_r32_rm32(); break;
		case 0x34: xor_al_i8(); break;
		case 0x35: xor_eax_i32(); break;
		case 0x36: segment_SS(); break;
		case 0x37: aaa(); break;
		case 0x38: cmp_rm8_r8(); break;
		case 0x39: cmp_rm32_r32(); break;
		case 0x3A: cmp_r8_rm8(); break;
		case 0x3B: cmp_r32_rm32(); break;
		case 0x3C: cmp_al_i8(); break;
		case 0x3D: cmp_eax_i32(); break;
		case 0x3E: segment_DS(); break;
		case 0x3F: aas(); break;
		case 0x40: inc_eax(); break;
		case 0x41: inc_ecx(); break;
		case 0x42: inc_edx(); break;
		case 0x43: inc_ebx(); break;
		case 0x44: inc_esp(); break;
		case 0x45: inc_ebp(); break;
		case 0x46: inc_esi(); break;
		case 0x47: inc_edi(); break;
		case 0x48: dec_eax(); break;
		case 0x49: dec_ecx(); break;
		case 0x4A: dec_edx(); break;
		case 0x4B: dec_ebx(); break;
		case 0x4C: dec_esp(); break;
		case 0x4D: dec_ebp(); break;
		case 0x4E: dec_esi(); break;
		case 0x4F: dec_edi(); break;
		case 0x50: push_eax(); break;
		case 0x51: push_ecx(); break;
		case 0x52: push_edx(); break;
		case 0x53: push_ebx(); break;
		case 0x54: push_esp(); break;
		case 0x55: push_ebp(); break;
		case 0x56: push_esi(); break;
		case 0x57: push_edi(); break;
		case 0x58: pop_eax(); break;
		case 0x59: pop_ecx(); break;
		case 0x5A: pop_edx(); break;
		case 0x5B: pop_ebx(); break;
		case 0x5C: pop_esp(); break;
		case 0x5D: pop_ebp(); break;
		case 0x5E: pop_esi(); break;
		case 0x5F: pop_edi(); break;
		case 0x60: pushad(); break;
		case 0x61: popad(); break;
		case 0x62: bound_r32_m32_m32(); break;
		case 0x63: unimplemented(); break;
		case 0x64: segment_FS(); break;
		case 0x65: segment_GS(); break;
		case 0x66: opsiz(); break;
		case 0x67: adrsiz(); break;
		case 0x68: push_i32(); break;
		case 0x69: imul_r32_rm32_i32(); break;
		case 0x6A: push_i8(); break;
		case 0x6B: imul_r32_rm32_i8(); break;
		case 0x6C: insb(); break;
		case 0x6D: insd(); break;
		case 0x6E: outsb(); break;
		case 0x6F: outsd(); break;
		case 0x70: jo_rel8(); break;
		case 0x71: jno_rel8(); break;
		case 0x72: jc_rel8(); break;
		case 0x73: jnc_rel8(); break;
		case 0x74: jz_rel8(); break;
		case 0x75: jnz_rel8(); break;
		case 0x76: jbe_rel8(); break;
		case 0x77: ja_rel8(); break;
		case 0x78: js_rel8(); break;
		case 0x79: jns_rel8(); break;
		case 0x7A: jp_rel8(); break;
		case 0x7B: jnp_rel8(); break;
		case 0x7C: jl_rel8(); break;
		case 0x7D: jge_rel8(); break;
		case 0x7E: jle_rel8(); break;
		case 0x7F: jg_rel8(); break;
		case 0x80: group80_8(); break;
		case 0x81: group81_32(); break;
		case 0x82: group80_8(); break;
		case 0x83: group83_32(); break;
		case 0x84: test_rm8_r8(); break;
		case 0x85: test_rm32_r32(); break;
		case 0x86: xchg_r8_rm8(); break;
		case 0x87: xchg_r32_rm32(); break;
		case 0x88: mov_rm8_r8(); break;
		case 0x89: mov_rm32_r32(); break;
		case 0x8A: mov_r8_rm8(); break;
		case 0x8B: mov_r32_rm32(); break;
		case 0x8C: mov_rm16_sreg(); break;
		case 0x8D: lea32(); break;
		case 0x8E: mov_sreg_rm16(); break;
		case 0x8F: pop_rm32(); break;
		case 0x90: nop(); break;
		case 0x91: xchg_eax_ecx(); break;
		case 0x92: xchg_eax_edx(); break;
		case 0x93: xchg_eax_ebx(); break;
		case 0x94: xchg_eax_esp(); break;
		case 0x95: xchg_eax_ebp(); break;
		case 0x96: xchg_eax_esi(); break;
		case 0x97: xchg_eax_edi(); break;
		case 0x98: cwde(); break;
		case 0x99: cdq(); break;
		case 0x9A: call_abs32(); break;
		case 0x9B: wait(); break;
		case 0x9C: pushfd(); break;
		case 0x9D: popfd(); break;
		case 0x9E: sahf(); break;
		case 0x9F: lahf(); break;
		case 0xA0: mov_al_m8(); break;
		case 0xA1: mov_eax_m32(); break;
		case 0xA2: mov_m8_al(); break;
		case 0xA3: mov_m32_eax(); break;
		case 0xA4: movsb(); break;
		case 0xA5: movsd(); break;
		case 0xA6: cmpsb(); break;
		case 0xA7: cmpsd(); break;
		case 0xA8: test_al_i8(); break;
		case 0xA9: test_eax_i32(); break;
		case 0xAA: stosb(); break;
		case 0xAB: stosd(); break;
		case 0xAC: lodsb(); break;
		case 0xAD: lodsd(); break;
		case 0xAE: scasb(); break;
		case 0xAF: scasd(); break;
		case 0xB0: mov_al_i8(); break;
		case 0xB1: mov_cl_i8(); break;
		case 0xB2: mov_dl_i8(); break;
		case 0xB3: mov_bl_i8(); break;
		case 0xB4: mov_ah_i8(); break;
		case 0xB5: mov_ch_i8(); break;
		case 0xB6: mov_dh_i8(); break;
		case 0xB7: mov_bh_i8(); break;
		case 0xB8: mov_eax_i32(); break;
		case 0xB9: mov_ecx_i32(); break;
		case 0xBA: mov_edx_i32(); break;
		case 0xBB: mov_ebx_i32(); break;
		case 0xBC: mov_esp_i32(); break;
		case 0xBD: mov_ebp_i32(); break;
		case 0xBE: mov_esi_i32(); break;
		case 0xBF: mov_edi_i32(); break;
		case 0xC0: groupC0_8(); break;
		case 0xC1: groupC1_32(); break;
		case 0xC2: ret_near32_i16(); break;
		case 0xC3: ret_near32(); break;
		case 0xC4: les32(); break;
		case 0xC5: lds32(); break;
		case 0xC6: mov_rm8_i8(); break;
		case 0xC7: mov_rm32_i32(); break;
		case 0xC8: unimplemented(); break;
		case 0xC9: leave32(); break;
		case 0xCA: retf_i32(); break;
		case 0xCB: retf32(); break;
		case 0xCC: int3(); break;
		case 0xCD: intr(); break;
		case 0xCE: into(); break;
		case 0xCF: iret32(); break;
		case 0xD0: groupD0_8(); break;
		case 0xD1: groupD1_32(); break;
		case 0xD2: groupD2_8(); break;
		case 0xD3: groupD3_32(); break;
		case 0xD4: aam(); break;
		case 0xD5: aad(); break;
		case 0xD6: setalc(); break;
		case 0xD7: xlat32(); break;
#if defined(HAS_I486) || defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
		case 0xD8: fpu_group_d8(); break;
		case 0xD9: fpu_group_d9(); break;
		case 0xDA: fpu_group_da(); break;
		case 0xDB: fpu_group_db(); break;
		case 0xDC: fpu_group_dc(); break;
		case 0xDD: fpu_group_dd(); break;
		case 0xDE: fpu_group_de(); break;
		case 0xDF: fpu_group_df(); break;
#else
		case 0xD8: escape(); break;
		case 0xD9: escape(); break;
		case 0xDA: escape(); break;
		case 0xDB: escape(); break;
		case 0xDC: escape(); break;
		case 0xDD: escape(); break;
		case 0xDE: escape(); break;
		case 0xDF: escape(); break;
#endif
		case 0xE0: loopne32(); break;
		case 0xE1: loopz32(); break;
		case 0xE2: loop32(); break;
		case 0xE3: jcxz32(); break;
		case 0xE4: in_al_i8(); break;
		case 0xE5: in_eax_i8(); break;
		case 0xE6: out_al_i8(); break;
		case 0xE7: out_eax_i8(); break;
		case 0xE8: call_rel32(); break;
		case 0xE9: jmp_rel32(); break;
		case 0xEA: jmp_abs32(); break;
		case 0xEB: jmp_rel8(); break;
		case 0xEC: in_al_dx(); break;
		case 0xED: in_eax_dx(); break;
		case 0xEE: out_al_dx(); break;
		case 0xEF: out_eax_dx(); break;
		case 0xF0: lock(); break;
		case 0xF1: invalid(); break;
		case 0xF2: repne(); break;
		case 0xF3: rep(); break;
		case 0xF4: hlt(); break;
		case 0xF5: cmc(); break;
		case 0xF6: groupF6_8(); break;
		case 0xF7: groupF7_32(); break;
		case 0xF8: clc(); break;
		case 0xF9: stc(); break;
		case 0xFA: cli(); break;
		case 0xFB: sti(); break;
		case 0xFC: cld(); break;
		case 0xFD: std(); break;
		case 0xFE: groupFE_8(); break;
		case 0xFF: groupFF_32(); break;
		default: invalid(); break;
		}
	}
	else {
		switch(opcode)
		{
		case 0x00: add_rm8_r8(); break;
		case 0x01: add_rm16_r16(); break;
		case 0x02: add_r8_rm8(); break;
		case 0x03: add_r16_rm16(); break;
		case 0x04: add_al_i8(); break;
		case 0x05: add_ax_i16(); break;
		case 0x06: push_es16(); break;
		case 0x07: pop_es16(); break;
		case 0x08: or_rm8_r8(); break;
		case 0x09: or_rm16_r16(); break;
		case 0x0A: or_r8_rm8(); break;
		case 0x0B: or_r16_rm16(); break;
		case 0x0C: or_al_i8(); break;
		case 0x0D: or_ax_i16(); break;
		case 0x0E: push_cs16(); break;
		case 0x0F:
			opcode = FETCH8();
			switch(opcode)
			{
			case 0x00: group0F00_16(); break;
			case 0x01: group0F01_16(); break;
			case 0x02: unimplemented(); break;
			case 0x03: unimplemented(); break;
			case 0x06: clts(); break;
#if defined(HAS_I486) || defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
			case 0x08: invd(); break;
			case 0x09: wbinvd(); break;
#endif
			case 0x0B: unimplemented(); break;
			case 0x20: mov_r32_cr(); break;
			case 0x21: mov_r32_dr(); break;
			case 0x22: mov_cr_r32(); break;
			case 0x23: mov_dr_r32(); break;
			case 0x24: mov_r32_tr(); break;
			case 0x26: mov_tr_r32(); break;
#if defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
			case 0x30: wrmsr(); break;
			case 0x31: rdtsc(); break;
			case 0x32: rdmsr(); break;
#endif
#if defined(HAS_MEDIAGX)
			case 0x74: cyrix_unknown(); break;
#endif
			case 0x80: jo_rel16(); break;
			case 0x81: jno_rel16(); break;
			case 0x82: jc_rel16(); break;
			case 0x83: jnc_rel16(); break;
			case 0x84: jz_rel16(); break;
			case 0x85: jnz_rel16(); break;
			case 0x86: jbe_rel16(); break;
			case 0x87: ja_rel16(); break;
			case 0x88: js_rel16(); break;
			case 0x89: jns_rel16(); break;
			case 0x8A: jp_rel16(); break;
			case 0x8B: jnp_rel16(); break;
			case 0x8C: jl_rel16(); break;
			case 0x8D: jge_rel16(); break;
			case 0x8E: jle_rel16(); break;
			case 0x8F: jg_rel16(); break;
			case 0x90: seto_rm8(); break;
			case 0x91: setno_rm8(); break;
			case 0x92: setc_rm8(); break;
			case 0x93: setnc_rm8(); break;
			case 0x94: setz_rm8(); break;
			case 0x95: setnz_rm8(); break;
			case 0x96: setbe_rm8(); break;
			case 0x97: seta_rm8(); break;
			case 0x98: sets_rm8(); break;
			case 0x99: setns_rm8(); break;
			case 0x9A: setp_rm8(); break;
			case 0x9B: setnp_rm8(); break;
			case 0x9C: setl_rm8(); break;
			case 0x9D: setge_rm8(); break;
			case 0x9E: setle_rm8(); break;
			case 0x9F: setg_rm8(); break;
			case 0xA0: push_fs16(); break;
			case 0xA1: pop_fs16(); break;
#if defined(HAS_I486) || defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
			case 0xA2: cpuid(); break;
#endif
			case 0xA3: bt_rm16_r16(); break;
			case 0xA4: shld16_i8(); break;
			case 0xA5: shld16_cl(); break;
			case 0xA8: push_gs16(); break;
			case 0xA9: pop_gs16(); break;
			case 0xAA: unimplemented(); break;
			case 0xAB: bts_rm16_r16(); break;
			case 0xAC: shrd16_i8(); break;
			case 0xAD: shrd16_cl(); break;
			case 0xAE: invalid(); break;
			case 0xAF: imul_r16_rm16(); break;
#if defined(HAS_I486) || defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
			case 0xB0: cmpxchg_rm8_r8(); break;
			case 0xB1: cmpxchg_rm16_r16(); break;
#endif
			case 0xB2: lss16(); break;
			case 0xB3: btr_rm16_r16(); break;
			case 0xB4: lfs16(); break;
			case 0xB5: lgs16(); break;
			case 0xB6: movzx_r16_rm8(); break;
			case 0xB7: invalid(); break;
			case 0xBA: group0FBA_16(); break;
			case 0xBB: btc_rm16_r16(); break;
			case 0xBC: bsf_r16_rm16(); break;
			case 0xBD: bsr_r16_rm16(); break;
			case 0xBE: movsx_r16_rm8(); break;
			case 0xBF: invalid(); break;
#if defined(HAS_I486) || defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
			case 0xC0: xadd_rm8_r8(); break;
			case 0xC1: xadd_rm16_r16(); break;
#endif
#if defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
			case 0xC7: cmpxchg8b_m64(); break;
#endif
			default: invalid(); break;
			}
			break;
		case 0x10: adc_rm8_r8(); break;
		case 0x11: adc_rm16_r16(); break;
		case 0x12: adc_r8_rm8(); break;
		case 0x13: adc_r16_rm16(); break;
		case 0x14: adc_al_i8(); break;
		case 0x15: adc_ax_i16(); break;
		case 0x16: push_ss16(); break;
		case 0x17: pop_ss16(); break;
		case 0x18: sbb_rm8_r8(); break;
		case 0x19: sbb_rm16_r16(); break;
		case 0x1A: sbb_r8_rm8(); break;
		case 0x1B: sbb_r16_rm16(); break;
		case 0x1C: sbb_al_i8(); break;
		case 0x1D: sbb_ax_i16(); break;
		case 0x1E: push_ds16(); break;
		case 0x1F: pop_ds16(); break;
		case 0x20: and_rm8_r8(); break;
		case 0x21: and_rm16_r16(); break;
		case 0x22: and_r8_rm8(); break;
		case 0x23: and_r16_rm16(); break;
		case 0x24: and_al_i8(); break;
		case 0x25: and_ax_i16(); break;
		case 0x26: segment_ES(); break;
		case 0x27: daa(); break;
		case 0x28: sub_rm8_r8(); break;
		case 0x29: sub_rm16_r16(); break;
		case 0x2A: sub_r8_rm8(); break;
		case 0x2B: sub_r16_rm16(); break;
		case 0x2C: sub_al_i8(); break;
		case 0x2D: sub_ax_i16(); break;
		case 0x2E: segment_CS(); break;
		case 0x2F: das(); break;
		case 0x30: xor_rm8_r8(); break;
		case 0x31: xor_rm16_r16(); break;
		case 0x32: xor_r8_rm8(); break;
		case 0x33: xor_r16_rm16(); break;
		case 0x34: xor_al_i8(); break;
		case 0x35: xor_ax_i16(); break;
		case 0x36: segment_SS(); break;
		case 0x37: aaa(); break;
		case 0x38: cmp_rm8_r8(); break;
		case 0x39: cmp_rm16_r16(); break;
		case 0x3A: cmp_r8_rm8(); break;
		case 0x3B: cmp_r16_rm16(); break;
		case 0x3C: cmp_al_i8(); break;
		case 0x3D: cmp_ax_i16(); break;
		case 0x3E: segment_DS(); break;
		case 0x3F: aas(); break;
		case 0x40: inc_ax(); break;
		case 0x41: inc_cx(); break;
		case 0x42: inc_dx(); break;
		case 0x43: inc_bx(); break;
		case 0x44: inc_sp(); break;
		case 0x45: inc_bp(); break;
		case 0x46: inc_si(); break;
		case 0x47: inc_di(); break;
		case 0x48: dec_ax(); break;
		case 0x49: dec_cx(); break;
		case 0x4A: dec_dx(); break;
		case 0x4B: dec_bx(); break;
		case 0x4C: dec_sp(); break;
		case 0x4D: dec_bp(); break;
		case 0x4E: dec_si(); break;
		case 0x4F: dec_di(); break;
		case 0x50: push_ax(); break;
		case 0x51: push_cx(); break;
		case 0x52: push_dx(); break;
		case 0x53: push_bx(); break;
		case 0x54: push_sp(); break;
		case 0x55: push_bp(); break;
		case 0x56: push_si(); break;
		case 0x57: push_di(); break;
		case 0x58: pop_ax(); break;
		case 0x59: pop_cx(); break;
		case 0x5A: pop_dx(); break;
		case 0x5B: pop_bx(); break;
		case 0x5C: pop_sp(); break;
		case 0x5D: pop_bp(); break;
		case 0x5E: pop_si(); break;
		case 0x5F: pop_di(); break;
		case 0x60: pusha(); break;
		case 0x61: popa(); break;
		case 0x62: bound_r16_m16_m16(); break;
		case 0x63: unimplemented(); break;
		case 0x64: segment_FS(); break;
		case 0x65: segment_GS(); break;
		case 0x66: opsiz(); break;
		case 0x67: adrsiz(); break;
		case 0x68: push_i16(); break;
		case 0x69: imul_r16_rm16_i16(); break;
		case 0x6A: push_i8(); break;
		case 0x6B: imul_r16_rm16_i8(); break;
		case 0x6C: insb(); break;
		case 0x6D: insw(); break;
		case 0x6E: outsb(); break;
		case 0x6F: outsw(); break;
		case 0x70: jo_rel8(); break;
		case 0x71: jno_rel8(); break;
		case 0x72: jc_rel8(); break;
		case 0x73: jnc_rel8(); break;
		case 0x74: jz_rel8(); break;
		case 0x75: jnz_rel8(); break;
		case 0x76: jbe_rel8(); break;
		case 0x77: ja_rel8(); break;
		case 0x78: js_rel8(); break;
		case 0x79: jns_rel8(); break;
		case 0x7A: jp_rel8(); break;
		case 0x7B: jnp_rel8(); break;
		case 0x7C: jl_rel8(); break;
		case 0x7D: jge_rel8(); break;
		case 0x7E: jle_rel8(); break;
		case 0x7F: jg_rel8(); break;
		case 0x80: group80_8(); break;
		case 0x81: group81_16(); break;
		case 0x82: group80_8(); break;
		case 0x83: group83_16(); break;
		case 0x84: test_rm8_r8(); break;
		case 0x85: test_rm16_r16(); break;
		case 0x86: xchg_r8_rm8(); break;
		case 0x87: xchg_r16_rm16(); break;
		case 0x88: mov_rm8_r8(); break;
		case 0x89: mov_rm16_r16(); break;
		case 0x8A: mov_r8_rm8(); break;
		case 0x8B: mov_r16_rm16(); break;
		case 0x8C: mov_rm16_sreg(); break;
		case 0x8D: lea16(); break;
		case 0x8E: mov_sreg_rm16(); break;
		case 0x8F: pop_rm16(); break;
		case 0x90: nop(); break;
		case 0x91: xchg_ax_cx(); break;
		case 0x92: xchg_ax_dx(); break;
		case 0x93: xchg_ax_bx(); break;
		case 0x94: xchg_ax_sp(); break;
		case 0x95: xchg_ax_bp(); break;
		case 0x96: xchg_ax_si(); break;
		case 0x97: xchg_ax_di(); break;
		case 0x98: cbw(); break;
		case 0x99: cwd(); break;
		case 0x9A: call_abs16(); break;
		case 0x9B: wait(); break;
		case 0x9C: pushf(); break;
		case 0x9D: popf(); break;
		case 0x9E: sahf(); break;
		case 0x9F: lahf(); break;
		case 0xA0: mov_al_m8(); break;
		case 0xA1: mov_ax_m16(); break;
		case 0xA2: mov_m8_al(); break;
		case 0xA3: mov_m16_ax(); break;
		case 0xA4: movsb(); break;
		case 0xA5: movsw(); break;
		case 0xA6: cmpsb(); break;
		case 0xA7: cmpsw(); break;
		case 0xA8: test_al_i8(); break;
		case 0xA9: test_ax_i16(); break;
		case 0xAA: stosb(); break;
		case 0xAB: stosw(); break;
		case 0xAC: lodsb(); break;
		case 0xAD: lodsw(); break;
		case 0xAE: scasb(); break;
		case 0xAF: scasw(); break;
		case 0xB0: mov_al_i8(); break;
		case 0xB1: mov_cl_i8(); break;
		case 0xB2: mov_dl_i8(); break;
		case 0xB3: mov_bl_i8(); break;
		case 0xB4: mov_ah_i8(); break;
		case 0xB5: mov_ch_i8(); break;
		case 0xB6: mov_dh_i8(); break;
		case 0xB7: mov_bh_i8(); break;
		case 0xB8: mov_ax_i16(); break;
		case 0xB9: mov_cx_i16(); break;
		case 0xBA: mov_dx_i16(); break;
		case 0xBB: mov_bx_i16(); break;
		case 0xBC: mov_sp_i16(); break;
		case 0xBD: mov_bp_i16(); break;
		case 0xBE: mov_si_i16(); break;
		case 0xBF: mov_di_i16(); break;
		case 0xC0: groupC0_8(); break;
		case 0xC1: groupC1_16(); break;
		case 0xC2: ret_near16_i16(); break;
		case 0xC3: ret_near16(); break;
		case 0xC4: les16(); break;
		case 0xC5: lds16(); break;
		case 0xC6: mov_rm8_i8(); break;
		case 0xC7: mov_rm16_i16(); break;
		case 0xC8: unimplemented(); break;
		case 0xC9: leave16(); break;
		case 0xCA: retf_i16(); break;
		case 0xCB: retf16(); break;
		case 0xCC: int3(); break;
		case 0xCD: intr(); break;
		case 0xCE: into(); break;
		case 0xCF: iret16(); break;
		case 0xD0: groupD0_8(); break;
		case 0xD1: groupD1_16(); break;
		case 0xD2: groupD2_8(); break;
		case 0xD3: groupD3_16(); break;
		case 0xD4: aam(); break;
		case 0xD5: aad(); break;
		case 0xD6: setalc(); break;
		case 0xD7: xlat16(); break;
#if defined(HAS_I486) || defined(HAS_PENTIUM) || defined(HAS_MEDIAGX)
		case 0xD8: fpu_group_d8(); break;
		case 0xD9: fpu_group_d9(); break;
		case 0xDA: fpu_group_da(); break;
		case 0xDB: fpu_group_db(); break;
		case 0xDC: fpu_group_dc(); break;
		case 0xDD: fpu_group_dd(); break;
		case 0xDE: fpu_group_de(); break;
		case 0xDF: fpu_group_df(); break;
#else
		case 0xD8: escape(); break;
		case 0xD9: escape(); break;
		case 0xDA: escape(); break;
		case 0xDB: escape(); break;
		case 0xDC: escape(); break;
		case 0xDD: escape(); break;
		case 0xDE: escape(); break;
		case 0xDF: escape(); break;
#endif
		case 0xE0: loopne16(); break;
		case 0xE1: loopz16(); break;
		case 0xE2: loop16(); break;
		case 0xE3: jcxz16(); break;
		case 0xE4: in_al_i8(); break;
		case 0xE5: in_ax_i8(); break;
		case 0xE6: out_al_i8(); break;
		case 0xE7: out_ax_i8(); break;
		case 0xE8: call_rel16(); break;
		case 0xE9: jmp_rel16(); break;
		case 0xEA: jmp_abs16(); break;
		case 0xEB: jmp_rel8(); break;
		case 0xEC: in_al_dx(); break;
		case 0xED: in_ax_dx(); break;
		case 0xEE: out_al_dx(); break;
		case 0xEF: out_ax_dx(); break;
		case 0xF0: lock(); break;
		case 0xF1: invalid(); break;
		case 0xF2: repne(); break;
		case 0xF3: rep(); break;
		case 0xF4: hlt(); break;
		case 0xF5: cmc(); break;
		case 0xF6: groupF6_8(); break;
		case 0xF7: groupF7_16(); break;
		case 0xF8: clc(); break;
		case 0xF9: stc(); break;
		case 0xFA: cli(); break;
		case 0xFB: sti(); break;
		case 0xFC: cld(); break;
		case 0xFD: std(); break;
		case 0xFE: groupFE_8(); break;
		case 0xFF: groupFF_16(); break;
		default: invalid(); break;
		}
	}
}

inline uint8 I386::OR8(uint8 dst, uint8 src)
{
	uint8 res = dst | src;
	CF = OF = 0;
	SetSZPF8(res);
	return res;
}
inline uint16 I386::OR16(uint16 dst, uint16 src)
{
	uint16 res = dst | src;
	CF = OF = 0;
	SetSZPF16(res);
	return res;
}
inline uint32 I386::OR32(uint32 dst, uint32 src)
{
	uint32 res = dst | src;
	CF = OF = 0;
	SetSZPF32(res);
	return res;
}
inline uint8 I386::AND8(uint8 dst, uint8 src)
{
	uint8 res = dst & src;
	CF = OF = 0;
	SetSZPF8(res);
	return res;
}
inline uint16 I386::AND16(uint16 dst, uint16 src)
{
	uint16 res = dst & src;
	CF = OF = 0;
	SetSZPF16(res);
	return res;
}
inline uint32 I386::AND32(uint32 dst, uint32 src)
{
	uint32 res = dst & src;
	CF = OF = 0;
	SetSZPF32(res);
	return res;
}
inline uint8 I386::XOR8(uint8 dst, uint8 src)
{
	uint8 res = dst ^ src;
	CF = OF = 0;
	SetSZPF8(res);
	return res;
}
inline uint16 I386::XOR16(uint16 dst, uint16 src)
{
	uint16 res = dst ^ src;
	CF = OF = 0;
	SetSZPF16(res);
	return res;
}
inline uint32 I386::XOR32(uint32 dst, uint32 src)
{
	uint32 res = dst ^ src;
	CF = OF = 0;
	SetSZPF32(res);
	return res;
}
inline uint8 I386::SUB8(uint8 dst, uint8 src)
{
	uint16 res = (uint16)dst - (uint16)src;
	SetCF8(res);
	SetOF_Sub8(res,src,dst);
	SetAF(res,src,dst);
	SetSZPF8(res);
	return (uint8)res;
}
inline uint16 I386::SUB16(uint16 dst, uint16 src)
{
	uint32 res = (uint32)dst - (uint32)src;
	SetCF16(res);
	SetOF_Sub16(res,src,dst);
	SetAF(res,src,dst);
	SetSZPF16(res);
	return (uint16)res;
}
inline uint32 I386::SUB32(uint32 dst, uint32 src)
{
	uint64 res = (uint64)dst - (uint64)src;
	SetCF32(res);
	SetOF_Sub32(res,src,dst);
	SetAF(res,src,dst);
	SetSZPF32(res);
	return (uint32)res;
}
inline uint8 I386::ADD8(uint8 dst, uint8 src)
{
	uint16 res = (uint16)dst + (uint16)src;
	SetCF8(res);
	SetOF_Add8(res,src,dst);
	SetAF(res,src,dst);
	SetSZPF8(res);
	return (uint8)res;
}
inline uint16 I386::ADD16(uint16 dst, uint16 src)
{
	uint32 res = (uint32)dst + (uint32)src;
	SetCF16(res);
	SetOF_Add16(res,src,dst);
	SetAF(res,src,dst);
	SetSZPF16(res);
	return (uint16)res;
}
inline uint32 I386::ADD32(uint32 dst, uint32 src)
{
	uint64 res = (uint64)dst + (uint64)src;
	SetCF32(res);
	SetOF_Add32(res,src,dst);
	SetAF(res,src,dst);
	SetSZPF32(res);
	return (uint32)res;
}
inline uint8 I386::INC8(uint8 dst)
{
	uint16 res = (uint16)dst + 1;
	SetOF_Add8(res,1,dst);
	SetAF(res,1,dst);
	SetSZPF8(res);
	return (uint8)res;
}
inline uint16 I386::INC16(uint16 dst)
{
	uint32 res = (uint32)dst + 1;
	SetOF_Add16(res,1,dst);
	SetAF(res,1,dst);
	SetSZPF16(res);
	return (uint16)res;
}
inline uint32 I386::INC32(uint32 dst)
{
	uint64 res = (uint64)dst + 1;
	SetOF_Add32(res,1,dst);
	SetAF(res,1,dst);
	SetSZPF32(res);
	return (uint32)res;
}
inline uint8 I386::DEC8(uint8 dst)
{
	uint16 res = (uint16)dst - 1;
	SetOF_Sub8(res,1,dst);
	SetAF(res,1,dst);
	SetSZPF8(res);
	return (uint8)res;
}
inline uint16 I386::DEC16(uint16 dst)
{
	uint32 res = (uint32)dst - 1;
	SetOF_Sub16(res,1,dst);
	SetAF(res,1,dst);
	SetSZPF16(res);
	return (uint16)res;
}
inline uint32 I386::DEC32(uint32 dst)
{
	uint64 res = (uint64)dst - 1;
	SetOF_Sub32(res,1,dst);
	SetAF(res,1,dst);
	SetSZPF32(res);
	return (uint32)res;
}
inline void I386::PUSH8(uint8 val)
{
	if(operand_size)
		PUSH32((int32)(int8)val);
	else
		PUSH16((int16)(int8)val);
}
inline void I386::PUSH16(uint16 val)
{
	uint32 ea;
	if(STACK_32BIT) {
		REG32(ESP) -= 2;
		ea = translate(SS, REG32(ESP));
		WM16(ea, val);
	}
	else {
		REG16(SP) -= 2;
		ea = translate(SS, REG16(SP));
		WM16(ea, val);
	}
}
inline void I386::PUSH32(uint32 val)
{
	uint32 ea;
	if(STACK_32BIT) {
		REG32(ESP) -= 4;
		ea = translate(SS, REG32(ESP));
		WM32(ea, val);
	}
	else {
		REG16(SP) -= 4;
		ea = translate(SS, REG16(SP));
		WM32(ea, val);
	}
}
inline uint8 I386::POP8()
{
	uint8 val;
	uint32 ea;
	if(STACK_32BIT) {
		ea = translate(SS, REG32(ESP));
		val = RM8(ea);
		REG32(ESP) += 1;
	}
	else {
		ea = translate(SS, REG16(SP));
		val = RM8(ea);
		REG16(SP) += 1;
	}
	return val;
}
inline uint16 I386::POP16()
{
	uint16 val;
	uint32 ea;
	if(STACK_32BIT) {
		ea = translate(SS, REG32(ESP));
		val = RM16(ea);
		REG32(ESP) += 2;
	}
	else {
		ea = translate(SS, REG16(SP));
		val = RM16(ea);
		REG16(SP) += 2;
	}
	return val;
}
inline uint32 I386::POP32()
{
	uint32 val;
	uint32 ea;
	if(STACK_32BIT) {
		ea = translate(SS, REG32(ESP));
		val = RM32(ea);
		REG32(ESP) += 4;
	}
	else {
		ea = translate(SS, REG16(SP));
		val = RM32(ea);
		REG16(SP) += 4;
	}
	return val;
}

inline void I386::BUMP_SI(int adjustment)
{
	if(address_size)
		REG32(ESI) += ((DF) ? -adjustment : +adjustment);
	else
		REG16(SI) += ((DF) ? -adjustment : +adjustment);
}

inline void I386::BUMP_DI(int adjustment)
{
	if(address_size)
		REG32(EDI) += ((DF) ? -adjustment : +adjustment);
	else
		REG16(DI) += ((DF) ? -adjustment : +adjustment);
}

uint8 I386::shift_rotate8(uint8 modrm, uint32 val, uint8 shift)
{
	uint8 src = val;
	uint8 dst = val;

	if(shift == 0) {
		CYCLES_RM(modrm, 3, 7);
	}
	else if(shift == 1) {
		switch((modrm >> 3) & 7) {
			case 0:			/* ROL rm8, 1 */
				CF = (src & 0x80) ? 1 : 0;
				dst = (src << 1) + CF;
				OF = ((src ^ dst) & 0x80) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 1:			/* ROR rm8, 1 */
				CF = (src & 1) ? 1 : 0;
				dst = (CF << 7) | (src >> 1);
				OF = ((src ^ dst) & 0x80) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 2:			/* RCL rm8, 1 */
				dst = (src << 1) + CF;
				CF = (src & 0x80) ? 1 : 0;
				OF = ((src ^ dst) & 0x80) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 3:			/* RCR rm8, 1 */
				dst = (CF << 7) | (src >> 1);
				CF = src & 1;
				OF = ((src ^ dst) & 0x80) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 4:			/* SHL/SAL rm8, 1 */
			case 6:
				dst = src << 1;
				CF = (src & 0x80) ? 1 : 0;
				OF = (((CF << 7) ^ dst) & 0x80) ? 1 : 0;
				SetSZPF8(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 5:			/* SHR rm8, 1 */
				dst = src >> 1;
				CF = src & 1;
				OF = (dst & 0x80) ? 1 : 0;
				SetSZPF8(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 7:			/* SAR rm8, 1 */
				dst = (int8)(src) >> 1;
				CF = src & 1;
				OF = 0;
				SetSZPF8(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
		}

	}
	else {
		switch((modrm >> 3) & 7) {
			case 0:			/* ROL rm8, i8 */
				dst = ((src & ((uint8)0xff >> shift)) << shift) |
					  ((src & ((uint8)0xff << (8-shift))) >> (8-shift));
				CF = (src >> (8-shift)) & 1;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 1:			/* ROR rm8, i8 */
				dst = ((src & ((uint8)0xff << shift)) >> shift) |
					  ((src & ((uint8)0xff >> (8-shift))) << (8-shift));
				CF = (src >> (shift-1)) & 1;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 2:			/* RCL rm8, i8 */
				dst = ((src & ((uint8)0xff >> shift)) << shift) |
					  ((src & ((uint8)0xff << (9-shift))) >> (9-shift)) |
					  (CF << (shift-1));
				CF = (src >> (8-shift)) & 1;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 3:			/* RCR rm8, i8 */
				dst = ((src & ((uint8)0xff << shift)) >> shift) |
					  ((src & ((uint8)0xff >> (8-shift))) << (9-shift)) |
					  (CF << (8-shift));
				CF = (src >> (shift-1)) & 1;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 4:			/* SHL/SAL rm8, i8 */
			case 6:
				dst = src << shift;
				CF = (src & (1 << (8-shift))) ? 1 : 0;
				SetSZPF8(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 5:			/* SHR rm8, i8 */
				dst = src >> shift;
				CF = (src & (1 << (shift-1))) ? 1 : 0;
				SetSZPF8(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 7:			/* SAR rm8, i8 */
				dst = (int8)src >> shift;
				CF = (src & (1 << (shift-1))) ? 1 : 0;
				SetSZPF8(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
		}
	}

	return dst;
}

void I386::adc_rm8_r8()	// Opcode 0x10
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		src = ADD8(src, CF);
		dst = ADD8(dst, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = RM8(ea);
		src = ADD8(src, CF);
		dst = ADD8(dst, src);
		WM8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::adc_r8_rm8()	// Opcode 0x12
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		src = ADD8(src, CF);
		dst = ADD8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM8(ea);
		dst = LOAD_REG8(modrm);
		src = ADD8(src, CF);
		dst = ADD8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::adc_al_i8()	// Opcode 0x14
{
	uint8 src, dst;
	src = FETCH8();
	dst = REG8(AL);
	src = ADD8(src, CF);
	dst = ADD8(dst, src);
	REG8(AL) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::add_rm8_r8()	// Opcode 0x00
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = ADD8(dst, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = RM8(ea);
		dst = ADD8(dst, src);
		WM8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::add_r8_rm8()	// Opcode 0x02
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		dst = ADD8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM8(ea);
		dst = LOAD_REG8(modrm);
		dst = ADD8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::add_al_i8()	// Opcode 0x04
{
	uint8 src, dst;
	src = FETCH8();
	dst = REG8(AL);
	dst = ADD8(dst, src);
	REG8(AL) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::and_rm8_r8()	// Opcode 0x20
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = AND8(dst, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = RM8(ea);
		dst = AND8(dst, src);
		WM8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::and_r8_rm8()	// Opcode 0x22
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		dst = AND8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM8(ea);
		dst = LOAD_REG8(modrm);
		dst = AND8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::and_al_i8()	// Opcode 0x24
{
	uint8 src, dst;
	src = FETCH8();
	dst = REG8(AL);
	dst = AND8(dst, src);
	REG8(AL) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::clc()	// Opcode 0xf8
{
	CF = 0;
	CYCLES(CYCLES_CLC);
}

void I386::cld()	// Opcode 0xfc
{
	DF = 0;
	CYCLES(CYCLES_CLD);
}

void I386::cli()	// Opcode 0xfa
{
	IF = 0;
	CYCLES(CYCLES_CLI);
}

void I386::cmc()	// Opcode 0xf5
{
	CF ^= 1;
	CYCLES(CYCLES_CMC);
}

void I386::cmp_rm8_r8()	// Opcode 0x38
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		SUB8(dst, src);
		CYCLES(CYCLES_CMP_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = RM8(ea);
		SUB8(dst, src);
		CYCLES(CYCLES_CMP_REG_MEM);
	}
}

void I386::cmp_r8_rm8()	// Opcode 0x3a
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		SUB8(dst, src);
		CYCLES(CYCLES_CMP_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM8(ea);
		dst = LOAD_REG8(modrm);
		SUB8(dst, src);
		CYCLES(CYCLES_CMP_MEM_REG);
	}
}

void I386::cmp_al_i8()	// Opcode 0x3c
{
	uint8 src, dst;
	src = FETCH8();
	dst = REG8(AL);
	SUB8(dst, src);
	CYCLES(CYCLES_CMP_IMM_ACC);
}

void I386::cmpsb()	// Opcode 0xa6
{
	uint32 eas, ead;
	uint8 src, dst;
	if(segment_prefix)
		eas = translate(segment_override, address_size ? REG32(ESI) : REG16(SI));
	else
		eas = translate(DS, address_size ? REG32(ESI) : REG16(SI));
	ead = translate(ES, address_size ? REG32(EDI) : REG16(DI));
	src = RM8(eas);
	dst = RM8(ead);
	SUB8(dst, src);
	BUMP_SI(1);
	BUMP_DI(1);
	CYCLES(CYCLES_CMPS);
}

void I386::in_al_i8()	// Opcode 0xe4
{
	uint16 port = FETCH8();
	uint8 data = IN8(port);
	REG8(AL) = data;
	CYCLES(CYCLES_IN_VAR);
}

void I386::in_al_dx()	// Opcode 0xec
{
	uint16 port = REG16(DX);
	uint8 data = IN8(port);
	REG8(AL) = data;
	CYCLES(CYCLES_IN);
}

void I386::ja_rel8()	// Opcode 0x77
{
	int8 disp = FETCH8();
	if(CF == 0 && ZF == 0) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jbe_rel8()	// Opcode 0x76
{
	int8 disp = FETCH8();
	if(CF != 0 || ZF != 0) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jc_rel8()	// Opcode 0x72
{
	int8 disp = FETCH8();
	if(CF != 0) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jg_rel8()	// Opcode 0x7f
{
	int8 disp = FETCH8();
	if(ZF == 0 && (SF == OF)) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jge_rel8()	// Opcode 0x7d
{
	int8 disp = FETCH8();
	if((SF == OF)) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jl_rel8()	// Opcode 0x7c
{
	int8 disp = FETCH8();
	if((SF != OF)) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jle_rel8()	// Opcode 0x7e
{
	int8 disp = FETCH8();
	if(ZF != 0 || (SF != OF)) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jnc_rel8()	// Opcode 0x73
{
	int8 disp = FETCH8();
	if(CF == 0) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jno_rel8()	// Opcode 0x71
{
	int8 disp = FETCH8();
	if(OF == 0) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jnp_rel8()	// Opcode 0x7b
{
	int8 disp = FETCH8();
	if(PF == 0) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jns_rel8()	// Opcode 0x79
{
	int8 disp = FETCH8();
	if(SF == 0) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jnz_rel8()	// Opcode 0x75
{
	int8 disp = FETCH8();
	if(ZF == 0) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jo_rel8()	// Opcode 0x70
{
	int8 disp = FETCH8();
	if(OF != 0) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jp_rel8()	// Opcode 0x7a
{
	int8 disp = FETCH8();
	if(PF != 0) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::js_rel8()	// Opcode 0x78
{
	int8 disp = FETCH8();
	if(SF != 0) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jz_rel8()	// Opcode 0x74
{
	int8 disp = FETCH8();
	if(ZF != 0) {
		NEAR_BRANCH(disp);
		CYCLES(CYCLES_JCC_DISP8);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_DISP8_NOBRANCH);
}

void I386::jmp_rel8()	// Opcode 0xeb
{
	int8 disp = FETCH8();
	NEAR_BRANCH(disp);
	CYCLES(CYCLES_JMP_SHORT);		/* TODO: Timing = 7 + m */
}

void I386::lahf()	// Opcode 0x9f
{
	REG8(AH) = get_flags() & 0xd7;
	CYCLES(CYCLES_LAHF);
}

void I386::lodsb()	// Opcode 0xac
{
	uint32 eas;
	if(segment_prefix)
		eas = translate(segment_override, address_size ? REG32(ESI) : REG16(SI));
	else
		eas = translate(DS, address_size ? REG32(ESI) : REG16(SI));
	REG8(AL) = RM8(eas);
	BUMP_SI(1);
	CYCLES(CYCLES_LODS);
}

void I386::mov_rm8_r8()	// Opcode 0x88
{
	uint8 src;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG8(modrm);
		STORE_RM8(modrm, src);
		CYCLES(CYCLES_MOV_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		WM8(ea, src);
		CYCLES(CYCLES_MOV_REG_MEM);
	}
}

void I386::mov_r8_rm8()	// Opcode 0x8a
{
	uint8 src;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM8(modrm);
		STORE_REG8(modrm, src);
		CYCLES(CYCLES_MOV_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM8(ea);
		STORE_REG8(modrm, src);
		CYCLES(CYCLES_MOV_MEM_REG);
	}
}

void I386::mov_rm8_i8()	// Opcode 0xc6
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint8 val = FETCH8();
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_MOV_IMM_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint8 val = FETCH8();
		WM8(ea, val);
		CYCLES(CYCLES_MOV_IMM_MEM);
	}
}

void I386::mov_r32_cr()	// Opcode 0x0f 20
{
	uint8 modrm = FETCH8();
	uint8 c = (modrm >> 3) & 7;

	STORE_RM32(modrm, cr[c]);
	CYCLES(CYCLES_MOV_CR_REG);
}

void I386::mov_r32_dr()	// Opcode 0x0f 21
{
	uint8 modrm = FETCH8();
	uint8 d = (modrm >> 3) & 7;

	STORE_RM32(modrm, dr[d]);
	switch(d)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			CYCLES(CYCLES_MOV_REG_DR0_3);
			break;
		case 6:
		case 7:
			CYCLES(CYCLES_MOV_REG_DR6_7);
			break;
	}
}

void I386::mov_cr_r32()	// Opcode 0x0f 22
{
	uint8 modrm = FETCH8();
	uint8 c = (modrm >> 3) & 7;

	cr[c] = LOAD_RM32(modrm);
	switch(c)
	{
		case 0: CYCLES(CYCLES_MOV_REG_CR0); break;
		case 2: CYCLES(CYCLES_MOV_REG_CR2); break;
		case 3: CYCLES(CYCLES_MOV_REG_CR3); break;
		default:
//			fatalerror(_T("i386: mov_cr_r32 CR%d !\n"), cr);
			break;
	}
}

void I386::mov_dr_r32()	// Opcode 0x0f 23
{
	uint8 modrm = FETCH8();
	uint8 d = (modrm >> 3) & 7;

	dr[d] = LOAD_RM32(modrm);
	switch(d)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			CYCLES(CYCLES_MOV_DR0_3_REG);
			break;
		case 6:
		case 7:
			CYCLES(CYCLES_MOV_DR6_7_REG);
			break;
		default:
//			fatalerror(_T("i386: mov_dr_r32 DR%d !"), dr);
			break;
	}
}

void I386::mov_al_m8()	// Opcode 0xa0
{
	uint32 offset, ea;
	if(address_size)
		offset = FETCH32();
	else
		offset = FETCH16();
	/* TODO: Not sure if this is correct... */
	if(segment_prefix)
		ea = translate(segment_override, offset);
	else
		ea = translate(DS, offset);
	REG8(AL) = RM8(ea);
	CYCLES(CYCLES_MOV_IMM_MEM);
}

void I386::mov_m8_al()	// Opcode 0xa2
{
	uint32 offset, ea;
	if(address_size)
		offset = FETCH32();
	else
		offset = FETCH16();
	/* TODO: Not sure if this is correct... */
	if(segment_prefix)
		ea = translate(segment_override, offset);
	else
		ea = translate(DS, offset);
	WM8(ea, REG8(AL));
	CYCLES(CYCLES_MOV_MEM_ACC);
}

void I386::mov_rm16_sreg()	// Opcode 0x8c
{
	uint8 modrm = FETCH8();
	int s = (modrm >> 3) & 7;

	if(modrm >= 0xc0) {
		STORE_RM16(modrm, sreg[s].selector);
		CYCLES(CYCLES_MOV_SREG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM16(ea, sreg[s].selector);
		CYCLES(CYCLES_MOV_SREG_MEM);
	}
}

void I386::mov_sreg_rm16()	// Opcode 0x8e
{
	uint16 selector;
	uint8 modrm = FETCH8();
	int s = (modrm >> 3) & 7;

	if(modrm >= 0xc0) {
		selector = LOAD_RM16(modrm);
		CYCLES(CYCLES_MOV_REG_SREG);
	}
	else {
		uint32 ea = GetEA(modrm);
		selector = RM16(ea);
		CYCLES(CYCLES_MOV_MEM_SREG);
	}
	sreg[s].selector = selector;
	load_segment_descriptor(s);
}

void I386::mov_al_i8()	// Opcode 0xb0
{
	REG8(AL) = FETCH8();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_cl_i8()	// Opcode 0xb1
{
	REG8(CL) = FETCH8();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_dl_i8()	// Opcode 0xb2
{
	REG8(DL) = FETCH8();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_bl_i8()	// Opcode 0xb3
{
	REG8(BL) = FETCH8();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_ah_i8()	// Opcode 0xb4
{
	REG8(AH) = FETCH8();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_ch_i8()	// Opcode 0xb5
{
	REG8(CH) = FETCH8();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_dh_i8()	// Opcode 0xb6
{
	REG8(DH) = FETCH8();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_bh_i8()	// Opcode 0xb7
{
	REG8(BH) = FETCH8();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::movsb()	// Opcode 0xa4
{
	uint32 eas, ead;
	if(segment_prefix)
		eas = translate(segment_override, address_size ? REG32(ESI) : REG16(SI));
	else
		eas = translate(DS, address_size ? REG32(ESI) : REG16(SI));
	ead = translate(ES, address_size ? REG32(EDI) : REG16(DI));
	uint8 v = RM8(eas);
	WM8(ead, v);
	BUMP_SI(1);
	BUMP_DI(1);
	CYCLES(CYCLES_MOVS);
}

void I386::or_rm8_r8()	// Opcode 0x08
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = OR8(dst, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = RM8(ea);
		dst = OR8(dst, src);
		WM8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::or_r8_rm8()	// Opcode 0x0a
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		dst = OR8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM8(ea);
		dst = LOAD_REG8(modrm);
		dst = OR8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::or_al_i8()	// Opcode 0x0c
{
	uint8 src, dst;
	src = FETCH8();
	dst = REG8(AL);
	dst = OR8(dst, src);
	REG8(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::out_al_i8()	// Opcode 0xe6
{
	uint16 port = FETCH8();
	uint8 data = REG8(AL);
	OUT8(port, data);
	CYCLES(CYCLES_OUT_VAR);
}

void I386::out_al_dx()	// Opcode 0xee
{
	uint16 port = REG16(DX);
	uint8 data = REG8(AL);
	OUT8(port, data);
	CYCLES(CYCLES_OUT);
}

void I386::push_i8()	// Opcode 0x6a
{
	uint8 val = FETCH8();
	PUSH8(val);
	CYCLES(CYCLES_PUSH_IMM);
}

void I386::ins_generic(int size)
{
	uint32 ead;
	uint8 vb;
	uint16 vw;
	uint32 vd;

	ead = translate(ES, address_size ? REG32(EDI) : REG16(DI));

	switch(size) {
	case 1:
		vb = IN8(REG16(DX));
		WM8(ead, vb);
		break;
	case 2:
		vw = IN16(REG16(DX));
		WM16(ead, vw);
		break;
	case 4:
		vd = IN32(REG16(DX));
		WM32(ead, vd);
		break;
	}

	REG32(EDI) += ((DF) ? -1 : 1) * size;
	CYCLES(CYCLES_INS);	// TODO: Confirm this value
}

void I386::insb()	// Opcode 0x6c
{
	ins_generic(1);
}

void I386::insw()	// Opcode 0x6d
{
	ins_generic(2);
}

void I386::insd()	// Opcode 0x6d
{
	ins_generic(4);
}

void I386::outs_generic(int size)
{
	uint32 eas;
	uint8 vb;
	uint16 vw;
	uint32 vd;

	if(segment_prefix)
		eas = translate(segment_override, REG32(ESI));
	else
		eas = translate(DS, REG32(ESI));
	switch(size)
	{
	case 1:
		vb = RM8(eas);
		OUT8(REG16(DX), vb);
		break;
	case 2:
		vw = RM16(eas);
		OUT16(REG16(DX), vw);
		break;
	case 4:
		vd = RM32(eas);
		OUT32(REG16(DX), vd);
		break;
	}

	REG32(ESI) += ((DF) ? -1 : 1) * size;
	CYCLES(CYCLES_OUTS);	// TODO: Confirm this value
}

void I386::outsb()	// Opcode 0x6e
{
	outs_generic(1);
}

void I386::outsw()	// Opcode 0x6f
{
	outs_generic(2);
}

void I386::outsd()	// Opcode 0x6f
{
	outs_generic(4);
}

void I386::repeat(int invert_flag)
{
	uint32 repeated_eip = eip;
	uint32 repeated_pc = pc;
	uint8 opcode = FETCH8();
	uint32 eas, ead;
	uint32 count;
	int32 cycle_base = 0, cycle_adjustment = 0;
	int32 *flag = NULL;
	
	if(segment_prefix)
		eas = translate(segment_override, sreg[CS].d ? REG32(ESI) : REG16(SI));
	else
		eas = translate(DS, sreg[CS].d ? REG32(ESI) : REG16(SI));
	ead = translate(ES, sreg[CS].d ? REG32(EDI) : REG16(DI));
	if(opcode == 0x66) {
		operand_size ^= 1;
		repeated_eip = eip;
		repeated_pc = pc;
		opcode = FETCH8();
	}
	if(opcode == 0x67) {
		address_size ^= 1;
		repeated_eip = eip;
		repeated_pc = pc;
		opcode = FETCH8();
	}
	switch(opcode)
	{
	case 0x6c:
	case 0x6d:
		/* INSB, INSW, INSD */
		// TODO: cycle count
		cycle_base = 8;
		cycle_adjustment = -4;
		flag = NULL;
		break;
	case 0x6e:
	case 0x6f:
		/* OUTSB, OUTSW, OUTSD */
		// TODO: cycle count
		cycle_base = 8;
		cycle_adjustment = -4;
		flag = NULL;
		break;
	case 0xa4:
	case 0xa5:
		/* MOVSB, MOVSW, MOVSD */
		cycle_base = 8;
		cycle_adjustment = -4;
		flag = NULL;
		break;
	case 0xa6:
	case 0xa7:
		/* CMPSB, CMPSW, CMPSD */
		cycle_base = 5;
		cycle_adjustment = -1;
		flag = &ZF;
		break;
	case 0xac:
	case 0xad:
		/* LODSB, LODSW, LODSD */
		cycle_base = 5;
		cycle_adjustment = 1;
		flag = NULL;
		break;
	case 0xaa:
	case 0xab:
		/* STOSB, STOSW, STOSD */
		cycle_base = 5;
		cycle_adjustment = 0;
		flag = NULL;
		break;
	case 0xae:
	case 0xaf:
		/* SCASB, SCASW, SCASD */
		cycle_base = 5;
		cycle_adjustment = 0;
		flag = &ZF;
		break;
	default:
		emu->out_debug(_T("i386: Invalid REP/opcode %02X combination at %08X\n"), opcode, prev_pc);
		trap(ILLEGAL_INSTRUCTION, 0);
		return;
	}
	if(sreg[CS].d) {
		if(REG32(ECX) == 0)
			return;
	}
	else {
		if(REG16(CX) == 0)
			return;
	}
	/* now actually perform the repeat */
	CYCLES_NUM(cycle_base);
	do {
		eip = repeated_eip;
		pc = repeated_pc;
		decode_opcode();
		CYCLES_NUM(cycle_adjustment);
		if(sreg[CS].d)
			count = --REG32(ECX);
		else
			count = --REG16(CX);
	}
	while(count && (!flag || (invert_flag ? !*flag : *flag)));
}

void I386::rep()	// Opcode 0xf3
{
	repeat(0);
}

void I386::repne()	// Opcode 0xf2
{
	repeat(1);
}

void I386::sahf()	// Opcode 0x9e
{
	set_flags((get_flags() & 0xffffff00) | (REG8(AH) & 0xd7));
	CYCLES(CYCLES_SAHF);
}

void I386::sbb_rm8_r8()	// Opcode 0x18
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG8(modrm) + CF;
		dst = LOAD_RM8(modrm);
		dst = SUB8(dst, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm) + CF;
		dst = RM8(ea);
		dst = SUB8(dst, src);
		WM8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::sbb_r8_rm8()	// Opcode 0x1a
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM8(modrm) + CF;
		dst = LOAD_REG8(modrm);
		dst = SUB8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM8(ea) + CF;
		dst = LOAD_REG8(modrm);
		dst = SUB8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::sbb_al_i8()	// Opcode 0x1c
{
	uint8 src, dst;
	src = FETCH8() + CF;
	dst = REG8(AL);
	dst = SUB8(dst, src);
	REG8(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::scasb()	// Opcode 0xae
{
	uint32 eas;
	uint8 src, dst;
	eas = translate(ES, address_size ? REG32(EDI) : REG16(DI));
	src = RM8(eas);
	dst = REG8(AL);
	SUB8(dst, src);
	BUMP_DI(1);
	CYCLES(CYCLES_SCAS);
}

void I386::setalc()	// Opcode 0xd6 (undocumented)
{
	REG8(AL) = CF ? 0xff : 0;
	CYCLES_NUM(3);
}

void I386::seta_rm8()	// Opcode 0x0f 97
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(CF == 0 && ZF == 0)
		val = 1;
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::setbe_rm8()	// Opcode 0x0f 96
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(CF != 0 || ZF != 0) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::setc_rm8()	// Opcode 0x0f 92
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(CF != 0) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::setg_rm8()	// Opcode 0x0f 9f
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(ZF == 0 && (SF == OF)) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::setge_rm8()	// Opcode 0x0f 9d
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if((SF == OF)) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::setl_rm8()	// Opcode 0x0f 9c
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(SF != OF) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::setle_rm8()	// Opcode 0x0f 9e
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(ZF != 0 || (SF != OF)) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::setnc_rm8()	// Opcode 0x0f 93
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(CF == 0) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::setno_rm8()	// Opcode 0x0f 91
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(OF == 0) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::setnp_rm8()	// Opcode 0x0f 9b
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(PF == 0) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::setns_rm8()	// Opcode 0x0f 99
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(SF == 0) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::setnz_rm8()	// Opcode 0x0f 95
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(ZF == 0) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::seto_rm8()	// Opcode 0x0f 90
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(OF != 0) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::setp_rm8()	// Opcode 0x0f 9a
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(PF != 0) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::sets_rm8()	// Opcode 0x0f 98
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(SF != 0) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::setz_rm8()	// Opcode 0x0f 94
{
	uint8 modrm = FETCH8();
	uint8 val = 0;
	if(ZF != 0) {
		val = 1;
	}
	if(modrm >= 0xc0) {
		STORE_RM8(modrm, val);
		CYCLES(CYCLES_SETCC_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM8(ea, val);
		CYCLES(CYCLES_SETCC_MEM);
	}
}

void I386::stc()	// Opcode 0xf9
{
	CF = 1;
	CYCLES(CYCLES_STC);
}

void I386::std()	// Opcode 0xfd
{
	DF = 1;
	CYCLES(CYCLES_STD);
}

void I386::sti()	// Opcode 0xfb
{
	IF = 1;
	CYCLES(CYCLES_STI);
	decode_opcode();
}

void I386::stosb()	// Opcode 0xaa
{
	uint32 ead;
	ead = translate(ES, address_size ? REG32(EDI) : REG16(DI));
	WM8(ead, REG8(AL));
	BUMP_DI(1);
	CYCLES(CYCLES_STOS);
}

void I386::sub_rm8_r8()	// Opcode 0x28
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = SUB8(dst, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = RM8(ea);
		dst = SUB8(dst, src);
		WM8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::sub_r8_rm8()	// Opcode 0x2a
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		dst = SUB8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM8(ea);
		dst = LOAD_REG8(modrm);
		dst = SUB8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::sub_al_i8()	// Opcode 0x2c
{
	uint8 src, dst;
	src = FETCH8();
	dst = REG8(EAX);
	dst = SUB8(dst, src);
	REG8(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::test_al_i8()	// Opcode 0xa8
{
	uint8 src = FETCH8();
	uint8 dst = REG8(AL);
	dst = src & dst;
	SetSZPF8(dst);
	CF = 0;
	OF = 0;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::test_rm8_r8()	// Opcode 0x84
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = src & dst;
		SetSZPF8(dst);
		CF = 0;
		OF = 0;
		CYCLES(CYCLES_TEST_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = RM8(ea);
		dst = src & dst;
		SetSZPF8(dst);
		CF = 0;
		OF = 0;
		CYCLES(CYCLES_TEST_REG_MEM);
	}
}

void I386::xchg_r8_rm8()	// Opcode 0x86
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint8 src = LOAD_RM8(modrm);
		uint8 dst = LOAD_REG8(modrm);
		STORE_REG8(modrm, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_XCHG_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint8 src = RM8(ea);
		uint8 dst = LOAD_REG8(modrm);
		STORE_REG8(modrm, src);
		WM8(ea, dst);
		CYCLES(CYCLES_XCHG_REG_MEM);
	}
}

void I386::xor_rm8_r8()	// Opcode 0x30
{
	uint8 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG8(modrm);
		dst = LOAD_RM8(modrm);
		dst = XOR8(dst, src);
		STORE_RM8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG8(modrm);
		dst = RM8(ea);
		dst = XOR8(dst, src);
		WM8(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::xor_r8_rm8()	// Opcode 0x32
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM8(modrm);
		dst = LOAD_REG8(modrm);
		dst = XOR8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM8(ea);
		dst = LOAD_REG8(modrm);
		dst = XOR8(dst, src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::xor_al_i8()	// Opcode 0x34
{
	uint8 src, dst;
	src = FETCH8();
	dst = REG8(AL);
	dst = XOR8(dst, src);
	REG8(AL) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}



void I386::group80_8()	// Opcode 0x80
{
	uint32 ea;
	uint8 src, dst;
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 0:		// ADD Rm8, i8
			if(modrm >= 0xc0) {
				dst = LOAD_RM8(modrm);
				src = FETCH8();
				dst = ADD8(dst, src);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM8(ea);
				src = FETCH8();
				dst = ADD8(dst, src);
				WM8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 1:		// OR Rm8, i8
			if(modrm >= 0xc0) {
				dst = LOAD_RM8(modrm);
				src = FETCH8();
				dst = OR8(dst, src);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM8(ea);
				src = FETCH8();
				dst = OR8(dst, src);
				WM8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 2:		// ADC Rm8, i8
			if(modrm >= 0xc0) {
				dst = LOAD_RM8(modrm);
				src = FETCH8();
				src = ADD8(src, CF);
				dst = ADD8(dst, src);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM8(ea);
				src = FETCH8();
				src = ADD8(src, CF);
				dst = ADD8(dst, src);
				WM8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 3:		// SBB Rm8, i8
			if(modrm >= 0xc0) {
				dst = LOAD_RM8(modrm);
				src = FETCH8() + CF;
				dst = SUB8(dst, src);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM8(ea);
				src = FETCH8() + CF;
				dst = SUB8(dst, src);
				WM8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 4:		// AND Rm8, i8
			if(modrm >= 0xc0) {
				dst = LOAD_RM8(modrm);
				src = FETCH8();
				dst = AND8(dst, src);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM8(ea);
				src = FETCH8();
				dst = AND8(dst, src);
				WM8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 5:		// SUB Rm8, i8
			if(modrm >= 0xc0) {
				dst = LOAD_RM8(modrm);
				src = FETCH8();
				dst = SUB8(dst, src);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM8(ea);
				src = FETCH8();
				dst = SUB8(dst, src);
				WM8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 6:		// XOR Rm8, i8
			if(modrm >= 0xc0) {
				dst = LOAD_RM8(modrm);
				src = FETCH8();
				dst = XOR8(dst, src);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM8(ea);
				src = FETCH8();
				dst = XOR8(dst, src);
				WM8(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 7:		// CMP Rm8, i8
			if(modrm >= 0xc0) {
				dst = LOAD_RM8(modrm);
				src = FETCH8();
				SUB8(dst, src);
				CYCLES(CYCLES_CMP_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM8(ea);
				src = FETCH8();
				SUB8(dst, src);
				CYCLES(CYCLES_CMP_REG_MEM);
			}
			break;
	}
}

void I386::groupC0_8()	// Opcode 0xc0
{
	uint8 dst;
	uint8 modrm = FETCH8();
	uint8 shift;

	if(modrm >= 0xc0) {
		dst = LOAD_RM8(modrm);
		shift = FETCH8() & 0x1f;
		dst = shift_rotate8(modrm, dst, shift);
		STORE_RM8(modrm, dst);
	}
	else {
		uint32 ea = GetEA(modrm);
		dst = RM8(ea);
		shift = FETCH8() & 0x1f;
		dst = shift_rotate8(modrm, dst, shift);
		WM8(ea, dst);
	}
}

void I386::groupD0_8()	// Opcode 0xd0
{
	uint8 dst;
	uint8 modrm = FETCH8();

	if(modrm >= 0xc0) {
		dst = LOAD_RM8(modrm);
		dst = shift_rotate8(modrm, dst, 1);
		STORE_RM8(modrm, dst);
	}
	else {
		uint32 ea = GetEA(modrm);
		dst = RM8(ea);
		dst = shift_rotate8(modrm, dst, 1);
		WM8(ea, dst);
	}
}

void I386::groupD2_8()	// Opcode 0xd2
{
	uint8 dst;
	uint8 modrm = FETCH8();

	if(modrm >= 0xc0) {
		dst = LOAD_RM8(modrm);
		dst = shift_rotate8(modrm, dst, REG8(CL));
		STORE_RM8(modrm, dst);
	}
	else {
		uint32 ea = GetEA(modrm);
		dst = RM8(ea);
		dst = shift_rotate8(modrm, dst, REG8(CL));
		WM8(ea, dst);
	}
}

void I386::groupF6_8()	// Opcode 0xf6
{
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 0:			/* TEST Rm8, i8 */
			if(modrm >= 0xc0) {
				uint8 dst = LOAD_RM8(modrm);
				uint8 src = FETCH8();
				dst &= src;
				CF = OF = AF = 0;
				SetSZPF8(dst);
				CYCLES(CYCLES_TEST_IMM_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint8 dst = RM8(ea);
				uint8 src = FETCH8();
				dst &= src;
				CF = OF = AF = 0;
				SetSZPF8(dst);
				CYCLES(CYCLES_TEST_IMM_MEM);
			}
			break;
		case 2:			/* NOT Rm8 */
			if(modrm >= 0xc0) {
				uint8 dst = LOAD_RM8(modrm);
				dst = ~dst;
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_NOT_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint8 dst = RM8(ea);
				dst = ~dst;
				WM8(ea, dst);
				CYCLES(CYCLES_NOT_MEM);
			}
			break;
		case 3:			/* NEG Rm8 */
			if(modrm >= 0xc0) {
				uint8 dst = LOAD_RM8(modrm);
				dst = SUB8(0, dst);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_NEG_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint8 dst = RM8(ea);
				dst = SUB8(0, dst);
				WM8(ea, dst);
				CYCLES(CYCLES_NEG_MEM);
			}
			break;
		case 4:			/* MUL AL, Rm8 */
			{
				uint16 result;
				uint8 src, dst;
				if(modrm >= 0xc0) {
					src = LOAD_RM8(modrm);
					CYCLES(CYCLES_MUL8_ACC_REG);		/* TODO: Correct multiply timing */
				}
				else {
					uint32 ea = GetEA(modrm);
					src = RM8(ea);
					CYCLES(CYCLES_MUL8_ACC_MEM);		/* TODO: Correct multiply timing */
				}

				dst = REG8(AL);
				result = (uint16)src * (uint16)dst;
				REG16(AX) = (uint16)result;

				CF = OF = (REG16(AX) > 0xff);
			}
			break;
		case 5:			/* IMUL AL, Rm8 */
			{
				int16 result;
				int16 src, dst;
				if(modrm >= 0xc0) {
					src = (int16)(int8)LOAD_RM8(modrm);
					CYCLES(CYCLES_IMUL8_ACC_REG);		/* TODO: Correct multiply timing */
				}
				else {
					uint32 ea = GetEA(modrm);
					src = (int16)(int8)RM8(ea);
					CYCLES(CYCLES_IMUL8_ACC_MEM);		/* TODO: Correct multiply timing */
				}

				dst = (int16)(int8)REG8(AL);
				result = src * dst;

				REG16(AX) = (uint16)result;

				CF = OF = !(result == (int16)(int8)result);
			}
			break;
		case 6:			/* DIV AL, Rm8 */
			{
				uint8 src;
				if(modrm >= 0xc0) {
					src = LOAD_RM8(modrm);
					CYCLES(CYCLES_DIV8_ACC_REG);
				}
				else {
					uint32 ea = GetEA(modrm);
					src = RM8(ea);
					CYCLES(CYCLES_DIV8_ACC_MEM);
				}
				uint16 quotient = (uint16)REG16(AX);
				if(!src)
					trap(DIVIDE_FAULT, 1);
				else {
					uint16 remainder = quotient % (uint16)src;
					uint16 result = quotient / (uint16)src;
					if(result > 0xff)
						trap(DIVIDE_FAULT, 1);
					else {
						REG8(AH) = (uint8)remainder & 0xff;
						REG8(AL) = (uint8)result & 0xff;
					}
				}
			}
			break;
		case 7:			/* IDIV AL, Rm8 */
			{
				uint8 src;
				if(modrm >= 0xc0) {
					src = LOAD_RM8(modrm);
					CYCLES(CYCLES_IDIV8_ACC_REG);
				}
				else {
					uint32 ea = GetEA(modrm);
					src = RM8(ea);
					CYCLES(CYCLES_IDIV8_ACC_MEM);
				}
				int16 quotient = (int16)REG16(AX);
				if(!src)
					trap(DIVIDE_FAULT, 1);
				else {
					int16 remainder = quotient % (int16)(int8)src;
					int16 result = quotient / (int16)(int8)src;
					if(result > 0xff)
						trap(DIVIDE_FAULT, 1);
					else {
						REG8(AH) = (uint8)remainder & 0xff;
						REG8(AL) = (uint8)result & 0xff;
					}
				}
			}
			break;
	}
}

void I386::groupFE_8()	// Opcode 0xfe
{
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 0:			/* INC Rm8 */
			if(modrm >= 0xc0) {
				uint8 dst = LOAD_RM8(modrm);
				dst = INC8(dst);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_INC_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint8 dst = RM8(ea);
				dst = INC8(dst);
				WM8(ea, dst);
				CYCLES(CYCLES_INC_MEM);
			}
			break;
		case 1:			/* DEC Rm8 */
			if(modrm >= 0xc0) {
				uint8 dst = LOAD_RM8(modrm);
				dst = DEC8(dst);
				STORE_RM8(modrm, dst);
				CYCLES(CYCLES_DEC_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint8 dst = RM8(ea);
				dst = DEC8(dst);
				WM8(ea, dst);
				CYCLES(CYCLES_DEC_MEM);
			}
			break;
		case 6:			/* PUSH Rm8 */
			{
				uint8 val;
				if(modrm >= 0xc0) {
					val = LOAD_RM8(modrm);
				}
				else {
					uint32 ea = GetEA(modrm);
					val = RM8(ea);
				}
				if(operand_size)
					PUSH32(val);
				else
					PUSH16(val);
				CYCLES(CYCLES_PUSH_RM);
			}
			break;
		default:
//			fatalerror(_T("i386: groupFE_8 /%d unimplemented\n"), (modrm >> 3) & 7);
			break;
	}
}



void I386::segment_CS()	// Opcode 0x2e
{
	segment_prefix = 1;
	segment_override = CS;
	decode_opcode();
}

void I386::segment_DS()	// Opcode 0x3e
{
	segment_prefix = 1;
	segment_override = DS;
	CYCLES_NUM(1);	// TODO: Specify cycle count
	decode_opcode();
}

void I386::segment_ES()	// Opcode 0x26
{
	segment_prefix = 1;
	segment_override = ES;
	CYCLES_NUM(1);	// TODO: Specify cycle count
	decode_opcode();
}

void I386::segment_FS()	// Opcode 0x64
{
	segment_prefix = 1;
	segment_override = FS;
	CYCLES_NUM(1);	// TODO: Specify cycle count
	decode_opcode();
}

void I386::segment_GS()	// Opcode 0x65
{
	segment_prefix = 1;
	segment_override = GS;
	CYCLES_NUM(1);	// TODO: Specify cycle count
	decode_opcode();
}

void I386::segment_SS()	// Opcode 0x36
{
	segment_prefix = 1;
	segment_override = SS;
	CYCLES_NUM(1);	// TODO: Specify cycle count
	decode_opcode();
}

void I386::opsiz()	// Opcode 0x66
{
	operand_size ^= 1;
	decode_opcode();
}

void I386::adrsiz()	// Opcode 0x67
{
	address_size ^= 1;
	decode_opcode();
}

void I386::nop()	// Opcode 0x90
{
	CYCLES(CYCLES_NOP);
}

void I386::int3()	// Opcode 0xcc
{
	CYCLES(CYCLES_INT3);
	trap(3, 1);
}

void I386::intr()	// Opcode 0xcd
{
	int interrupt = FETCH8();
	CYCLES(CYCLES_INT);
#ifdef I386_BIOS_CALL
	BIOS_INT(interrupt);
#endif
	trap(interrupt, 1);
}

void I386::into()	// Opcode 0xce
{
	if(OF) {
		trap(OVERFLOW_TRAP, 1);
		CYCLES(CYCLES_INTO_OF1);
	}
	else
		CYCLES(CYCLES_INTO_OF0);
}

void I386::escape()	// Opcodes 0xd8 - 0xdf
{
	uint8 modrm = FETCH8();
	CYCLES_NUM(3);	// TODO: confirm this
	(void) LOAD_RM8(modrm);
}

void I386::hlt()	// Opcode 0xf4
{
	// TODO: We need to raise an exception in protected mode and when
	// the current privilege level is not zero
	halted = 1;
	CYCLES(CYCLES_HLT);
	if(cycles > 0)
		cycles = 0;
}

void I386::decimal_adjust(int direction)
{
	uint8 tmpAL = REG8(AL);

	if(AF || ((REG8(AL) & 0xf) > 9)) {
		REG8(AL) = REG8(AL) + (direction * 0x06);
		AF = 1;
		if(REG8(AL) & 0x100)
			CF = 1;
		if(direction > 0)
			tmpAL = REG8(AL);
	}

	if(CF || (tmpAL > 0x9f)) {
		REG8(AL) += (direction * 0x60);
		CF = 1;
	}

	SetSZPF8(REG8(AL));
}

void I386::daa()	// Opcode 0x27
{
	decimal_adjust(+1);
	CYCLES(CYCLES_DAA);
}

void I386::das()	// Opcode 0x2f
{
	decimal_adjust(-1);
	CYCLES(CYCLES_DAS);
}

void I386::aaa()	// Opcode 0x37
{
	if((REG8(AL) & 0x0f) || AF != 0) {
		REG16(AX) = REG16(AX) + 6;
		REG8(AH) = REG8(AH) + 1;
		AF = 1;
		CF = 1;
	}
	else {
		AF = 0;
		CF = 0;
	}
	REG8(AL) = REG8(AL) & 0x0f;
	CYCLES(CYCLES_AAA);
}

void I386::aas()	// Opcode 0x3f
{
	if(AF || ((REG8(AL) & 0xf) > 9)) {
		REG16(AX) -= 6;
		REG8(AH) -= 1;
		AF = 1;
		CF = 1;
	}
	else {
		AF = 0;
		CF = 0;
	}
	REG8(AL) &= 0x0f;
	CYCLES(CYCLES_AAS);
}

void I386::aad()	// Opcode 0xd5
{
	uint8 tempAL = REG8(AL);
	uint8 tempAH = REG8(AH);
	uint8 i = FETCH8();

	REG8(AL) = (tempAL + (tempAH * i)) & 0xff;
	REG8(AH) = 0;
	SetSZPF8(REG8(AL));
	CYCLES(CYCLES_AAD);
}

void I386::aam()	// Opcode 0xd4
{
	uint8 tempAL = REG8(AL);
	uint8 i = FETCH8();

	REG8(AH) = tempAL / i;
	REG8(AL) = tempAL % i;
	SetSZPF8(REG8(AL));
	CYCLES(CYCLES_AAM);
}

void I386::clts()	// Opcode 0x0f 0x06
{
	// TODO: #GP(0) is executed
	cr[0] &= ~8;	/* clear TS bit */
	CYCLES(CYCLES_CLTS);
}

void I386::wait()	// Opcode 0x9B
{
	// TODO
}

void I386::lock()	// Opcode 0xf0
{
	CYCLES(CYCLES_LOCK);		// TODO: Determine correct cycle count
	decode_opcode();
}

void I386::mov_r32_tr()	// Opcode 0x0f 24
{
	FETCH8();
	CYCLES_NUM(1);		// TODO: correct cycle count
}

void I386::mov_tr_r32()	// Opcode 0x0f 26
{
	FETCH8();
	CYCLES_NUM(1);		// TODO: correct cycle count
}

void I386::unimplemented()
{
	emu->out_debug(_T("i386: Unimplemented opcode %02X at %08X\n"), opcode, prev_pc);
}

void I386::invalid()
{
	emu->out_debug(_T("i386: Invalid opcode %02X at %08X\n"), opcode, prev_pc);
	trap(ILLEGAL_INSTRUCTION, 0);
}

uint16 I386::shift_rotate16(uint8 modrm, uint32 val, uint8 shift)
{
	uint16 src = val;
	uint16 dst = val;

	if(shift == 0) {
		CYCLES_RM(modrm, 3, 7);
	}
	else if(shift == 1) {
		switch((modrm >> 3) & 7)
		{
			case 0:			/* ROL rm16, 1 */
				CF = (src & 0x8000) ? 1 : 0;
				dst = (src << 1) + CF;
				OF = ((src ^ dst) & 0x8000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 1:			/* ROR rm16, 1 */
				CF = (src & 1) ? 1 : 0;
				dst = (CF << 15) | (src >> 1);
				OF = ((src ^ dst) & 0x8000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 2:			/* RCL rm16, 1 */
				dst = (src << 1) + CF;
				CF = (src & 0x8000) ? 1 : 0;
				OF = ((src ^ dst) & 0x8000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 3:			/* RCR rm16, 1 */
				dst = (CF << 15) | (src >> 1);
				CF = src & 1;
				OF = ((src ^ dst) & 0x8000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 4:			/* SHL/SAL rm16, 1 */
			case 6:
				dst = src << 1;
				CF = (src & 0x8000) ? 1 : 0;
				OF = (((CF << 15) ^ dst) & 0x8000) ? 1 : 0;
				SetSZPF16(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 5:			/* SHR rm16, 1 */
				dst = src >> 1;
				CF = src & 1;
				OF = (dst & 0x8000) ? 1 : 0;
				SetSZPF16(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 7:			/* SAR rm16, 1 */
				dst = (int16)(src) >> 1;
				CF = src & 1;
				OF = 0;
				SetSZPF16(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
		}
	}
	else {
		switch((modrm >> 3) & 7) {
			case 0:			/* ROL rm16, i8 */
				dst = ((src & ((uint16)0xffff >> shift)) << shift) |
					  ((src & ((uint16)0xffff << (16-shift))) >> (16-shift));
				CF = (src >> (16-shift)) & 1;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 1:			/* ROR rm16, i8 */
				dst = ((src & ((uint16)0xffff << shift)) >> shift) |
					  ((src & ((uint16)0xffff >> (16-shift))) << (16-shift));
				CF = (src >> (shift-1)) & 1;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 2:			/* RCL rm16, i8 */
				dst = ((src & ((uint16)0xffff >> shift)) << shift) |
					  ((src & ((uint16)0xffff << (17-shift))) >> (17-shift)) |
					  (CF << (shift-1));
				CF = (src >> (16-shift)) & 1;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 3:			/* RCR rm16, i8 */
				dst = ((src & ((uint16)0xffff << shift)) >> shift) |
					  ((src & ((uint16)0xffff >> (16-shift))) << (17-shift)) |
					  (CF << (16-shift));
				CF = (src >> (shift-1)) & 1;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 4:			/* SHL/SAL rm16, i8 */
			case 6:
				dst = src << shift;
				CF = (src & (1 << (16-shift))) ? 1 : 0;
				SetSZPF16(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 5:			/* SHR rm16, i8 */
				dst = src >> shift;
				CF = (src & (1 << (shift-1))) ? 1 : 0;
				SetSZPF16(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 7:			/* SAR rm16, i8 */
				dst = (int16)src >> shift;
				CF = (src & (1 << (shift-1))) ? 1 : 0;
				SetSZPF16(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
		}

	}
	return dst;
}



void I386::adc_rm16_r16()	// Opcode 0x11
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		src = ADD16(src, CF);
		dst = ADD16(dst, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = RM16(ea);
		src = ADD16(src, CF);
		dst = ADD16(dst, src);
		WM16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::adc_r16_rm16()	// Opcode 0x13
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		src = ADD16(src, CF);
		dst = ADD16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM16(ea);
		dst = LOAD_REG16(modrm);
		src = ADD16(src, CF);
		dst = ADD16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::adc_ax_i16()	// Opcode 0x15
{
	uint16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	src = ADD16(src, CF);
	dst = ADD16(dst, src);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::add_rm16_r16()	// Opcode 0x01
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = ADD16(dst, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = RM16(ea);
		dst = ADD16(dst, src);
		WM16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::add_r16_rm16()	// Opcode 0x03
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		dst = ADD16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM16(ea);
		dst = LOAD_REG16(modrm);
		dst = ADD16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::add_ax_i16()	// Opcode 0x05
{
	uint16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	dst = ADD16(dst, src);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::and_rm16_r16()	// Opcode 0x21
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = AND16(dst, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = RM16(ea);
		dst = AND16(dst, src);
		WM16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::and_r16_rm16()	// Opcode 0x23
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		dst = AND16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM16(ea);
		dst = LOAD_REG16(modrm);
		dst = AND16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::and_ax_i16()	// Opcode 0x25
{
	uint16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	dst = AND16(dst, src);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::bsf_r16_rm16()	// Opcode 0x0f bc
{
	uint16 src, dst, temp;
	uint8 modrm = FETCH8();

	if(modrm >= 0xc0) {
		src = LOAD_RM16(modrm);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM16(ea);
	}

	dst = 0;

	if(src == 0)
		ZF = 1;
	else {
		ZF = 0;
		temp = 0;
		while((src & (1 << temp)) == 0) {
			temp++;
			dst = temp;
			CYCLES(CYCLES_BSF);
		}
	}
	CYCLES(CYCLES_BSF_BASE);
	STORE_REG16(modrm, dst);
}

void I386::bsr_r16_rm16()	// Opcode 0x0f bd
{
	uint16 src, dst, temp;
	uint8 modrm = FETCH8();

	if(modrm >= 0xc0)
		src = LOAD_RM16(modrm);
	else {
		uint32 ea = GetEA(modrm);
		src = RM16(ea);
	}

	dst = 0;

	if(src == 0)
		ZF = 1;
	else {
		ZF = 0;
		dst = temp = 15;
		while((src & (1 << temp)) == 0) {
			temp--;
			dst = temp;
			CYCLES(CYCLES_BSR);
		}
	}
	CYCLES(CYCLES_BSR_BASE);
	STORE_REG16(modrm, dst);
}


void I386::bt_rm16_r16()	// Opcode 0x0f a3
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint16 dst = LOAD_RM16(modrm);
		uint16 bit = LOAD_REG16(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;

		CYCLES(CYCLES_BT_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint16 dst = RM16(ea);
		uint16 bit = LOAD_REG16(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;
		CYCLES(CYCLES_BT_REG_MEM);
	}
}

void I386::btc_rm16_r16()	// Opcode 0x0f bb
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint16 dst = LOAD_RM16(modrm);
		uint16 bit = LOAD_REG16(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;
		dst ^= (1 << bit);

		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_BTC_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint16 dst = RM16(ea);
		uint16 bit = LOAD_REG16(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;
		dst ^= (1 << bit);

		WM16(ea, dst);
		CYCLES(CYCLES_BTC_REG_MEM);
	}
}

void I386::btr_rm16_r16()	// Opcode 0x0f b3
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint16 dst = LOAD_RM16(modrm);
		uint16 bit = LOAD_REG16(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;
		dst &= ~(1 << bit);

		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_BTR_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint16 dst = RM16(ea);
		uint16 bit = LOAD_REG16(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;
		dst &= ~(1 << bit);

		WM16(ea, dst);
		CYCLES(CYCLES_BTR_REG_MEM);
	}
}

void I386::bts_rm16_r16()	// Opcode 0x0f ab
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint16 dst = LOAD_RM16(modrm);
		uint16 bit = LOAD_REG16(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;
		dst |= (1 << bit);

		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_BTS_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint16 dst = RM16(ea);
		uint16 bit = LOAD_REG16(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;
		dst |= (1 << bit);

		WM16(ea, dst);
		CYCLES(CYCLES_BTS_REG_MEM);
	}
}

void I386::call_abs16()	// Opcode 0x9a
{
	uint16 offset = FETCH16();
	uint16 ptr = FETCH16();
#ifdef I386_BIOS_CALL
	int operand_size = sreg[CS].d;
#endif
	if(PROTECTED_MODE) {
		/* TODO */
//		fatalerror(_T("i386: call_abs16 in protected mode unimplemented");
	}
	else {
		if(sreg[CS].d) {
			PUSH32(sreg[CS].selector);
			PUSH32(eip);
		}
		else {
			PUSH16(sreg[CS].selector);
			PUSH16(eip);
		}
		sreg[CS].selector = ptr;
		eip = offset;
		load_segment_descriptor(CS);
	}
	CYCLES(CYCLES_CALL_INTERSEG);		/* TODO: Timing = 17 + m */
	CHANGE_PC(eip);
#ifdef I386_BIOS_CALL
	if(operand_size) {
		BIOS_CALL_FAR32();
	}
	else {
		BIOS_CALL_FAR16();
	}
#endif
}

void I386::call_rel16()	// Opcode 0xe8
{
	int16 disp = FETCH16();

	PUSH16(eip);
	if(sreg[CS].d)
		eip += disp;
	else
		eip = (eip + disp) & 0xffff;
	CHANGE_PC(eip);
	CYCLES(CYCLES_CALL);		/* TODO: Timing = 7 + m */
#ifdef I386_BIOS_CALL
	BIOS_CALL_NEAR16();
#endif
}

void I386::cbw()	// Opcode 0x98
{
	REG16(AX) = (int16)((int8)REG8(AL));
	CYCLES(CYCLES_CBW);
}

void I386::cmp_rm16_r16()	// Opcode 0x39
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		SUB16(dst, src);
		CYCLES(CYCLES_CMP_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = RM16(ea);
		SUB16(dst, src);
		CYCLES(CYCLES_CMP_REG_MEM);
	}
}

void I386::cmp_r16_rm16()	// Opcode 0x3b
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		SUB16(dst, src);
		CYCLES(CYCLES_CMP_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM16(ea);
		dst = LOAD_REG16(modrm);
		SUB16(dst, src);
		CYCLES(CYCLES_CMP_MEM_REG);
	}
}

void I386::cmp_ax_i16()	// Opcode 0x3d
{
	uint16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	SUB16(dst, src);
	CYCLES(CYCLES_CMP_IMM_ACC);
}

void I386::cmpsw()	// Opcode 0xa7
{
	uint32 eas, ead;
	uint16 src, dst;
	if(segment_prefix)
		eas = translate(segment_override, address_size ? REG32(ESI) : REG16(SI));
	else
		eas = translate(DS, address_size ? REG32(ESI) : REG16(SI));
	ead = translate(ES, address_size ? REG32(EDI) : REG16(DI));
	src = RM16(eas);
	dst = RM16(ead);
	SUB16(dst, src);
	BUMP_SI(2);
	BUMP_DI(2);
	CYCLES(CYCLES_CMPS);
}

void I386::cwd()	// Opcode 0x99
{
	if(REG16(AX) & 0x8000)
		REG16(DX) = 0xffff;
	else
		REG16(DX) = 0x0000;
	CYCLES(CYCLES_CWD);
}

void I386::dec_ax()	// Opcode 0x48
{
	REG16(AX) = DEC16(REG16(AX));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_cx()	// Opcode 0x49
{
	REG16(CX) = DEC16(REG16(CX));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_dx()	// Opcode 0x4a
{
	REG16(DX) = DEC16(REG16(DX));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_bx()	// Opcode 0x4b
{
	REG16(BX) = DEC16(REG16(BX));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_sp()	// Opcode 0x4c
{
	REG16(SP) = DEC16(REG16(SP));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_bp()	// Opcode 0x4d
{
	REG16(BP) = DEC16(REG16(BP));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_si()	// Opcode 0x4e
{
	REG16(SI) = DEC16(REG16(SI));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_di()	// Opcode 0x4f
{
	REG16(DI) = DEC16(REG16(DI));
	CYCLES(CYCLES_DEC_REG);
}

void I386::imul_r16_rm16()	// Opcode 0x0f af
{
	uint8 modrm = FETCH8();
	int32 result;
	int32 src, dst;
	if(modrm >= 0xc0) {
		src = (int32)(int16)LOAD_RM16(modrm);
		CYCLES(CYCLES_IMUL16_REG_REG);		/* TODO: Correct multiply timing */
	}
	else {
		uint32 ea = GetEA(modrm);
		src = (int32)(int16)RM16(ea);
		CYCLES(CYCLES_IMUL16_REG_MEM);		/* TODO: Correct multiply timing */
	}

	dst = (int32)(int16)LOAD_REG16(modrm);
	result = src * dst;

	STORE_REG16(modrm, (uint16)result);

	CF = OF = !(result == (int32)(int16)result);
}

void I386::imul_r16_rm16_i16()	// Opcode 0x69
{
	uint8 modrm = FETCH8();
	int32 result;
	int32 src, dst;
	if(modrm >= 0xc0) {
		dst = (int32)(int16)LOAD_RM16(modrm);
		CYCLES(CYCLES_IMUL16_REG_IMM_REG);		/* TODO: Correct multiply timing */
	}
	else {
		uint32 ea = GetEA(modrm);
		dst = (int32)(int16)RM16(ea);
		CYCLES(CYCLES_IMUL16_MEM_IMM_REG);		/* TODO: Correct multiply timing */
	}

	src = (int32)(int16)FETCH16();
	result = src * dst;

	STORE_REG16(modrm, (uint16)result);

	CF = OF = !(result == (int32)(int16)result);
}

void I386::imul_r16_rm16_i8()	// Opcode 0x6b
{
	uint8 modrm = FETCH8();
	int32 result;
	int32 src, dst;
	if(modrm >= 0xc0) {
		dst = (int32)(int16)LOAD_RM16(modrm);
		CYCLES(CYCLES_IMUL16_REG_IMM_REG);		/* TODO: Correct multiply timing */
	}
	else {
		uint32 ea = GetEA(modrm);
		dst = (int32)(int16)RM16(ea);
		CYCLES(CYCLES_IMUL16_MEM_IMM_REG);		/* TODO: Correct multiply timing */
	}

	src = (int32)(int8)FETCH8();
	result = src * dst;

	STORE_REG16(modrm, (uint16)result);

	CF = OF = !(result == (int32)(int16)result);
}

void I386::in_ax_i8()	// Opcode 0xe5
{
	uint16 port = FETCH8();
	uint16 data = IN16(port);
	REG16(AX) = data;
	CYCLES(CYCLES_IN_VAR);
}

void I386::in_ax_dx()	// Opcode 0xed
{
	uint16 port = REG16(DX);
	uint16 data = IN16(port);
	REG16(AX) = data;
	CYCLES(CYCLES_IN);
}

void I386::inc_ax()	// Opcode 0x40
{
	REG16(AX) = INC16(REG16(AX));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_cx()	// Opcode 0x41
{
	REG16(CX) = INC16(REG16(CX));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_dx()	// Opcode 0x42
{
	REG16(DX) = INC16(REG16(DX));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_bx()	// Opcode 0x43
{
	REG16(BX) = INC16(REG16(BX));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_sp()	// Opcode 0x44
{
	REG16(SP) = INC16(REG16(SP));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_bp()	// Opcode 0x45
{
	REG16(BP) = INC16(REG16(BP));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_si()	// Opcode 0x46
{
	REG16(SI) = INC16(REG16(SI));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_di()	// Opcode 0x47
{
	REG16(DI) = INC16(REG16(DI));
	CYCLES(CYCLES_INC_REG);
}

void I386::iret16()	// Opcode 0xcf
{
	if(PROTECTED_MODE) {
		/* TODO: Virtual 8086-mode */
		/* TODO: Nested task */
		/* TODO: #SS(0) exception */
		/* TODO: All the protection-related stuff... */
		eip = POP16();
		sreg[CS].selector = POP16();
		set_flags(POP16());
		load_segment_descriptor(CS);
		CHANGE_PC(eip);
	}
	else {
		/* TODO: #SS(0) exception */
		/* TODO: #GP(0) exception */
		eip = POP16();
		sreg[CS].selector = POP16();
		set_flags(POP16());
		load_segment_descriptor(CS);
		CHANGE_PC(eip);
	}
	CYCLES(CYCLES_IRET);
}

void I386::ja_rel16()	// Opcode 0x0f 87
{
	int16 disp = FETCH16();
	if(CF == 0 && ZF == 0) {
		if(sreg[CS].d)
			eip += disp;
		else
			eip = (eip + disp) & 0xffff;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
}

void I386::jbe_rel16()	// Opcode 0x0f 86
{
	int16 disp = FETCH16();
	if(CF != 0 || ZF != 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jc_rel16()	// Opcode 0x0f 82
{
	int16 disp = FETCH16();
	if(CF != 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jg_rel16()	// Opcode 0x0f 8f
{
	int16 disp = FETCH16();
	if(ZF == 0 && (SF == OF)) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jge_rel16()	// Opcode 0x0f 8d
{
	int16 disp = FETCH16();
	if((SF == OF)) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jl_rel16()	// Opcode 0x0f 8c
{
	int16 disp = FETCH16();
	if((SF != OF)) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jle_rel16()	// Opcode 0x0f 8e
{
	int16 disp = FETCH16();
	if(ZF != 0 || (SF != OF)) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jnc_rel16()	// Opcode 0x0f 83
{
	int16 disp = FETCH16();
	if(CF == 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jno_rel16()	// Opcode 0x0f 81
{
	int16 disp = FETCH16();
	if(OF == 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jnp_rel16()	// Opcode 0x0f 8b
{
	int16 disp = FETCH16();
	if(PF == 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jns_rel16()	// Opcode 0x0f 89
{
	int16 disp = FETCH16();
	if(SF == 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jnz_rel16()	// Opcode 0x0f 85
{
	int16 disp = FETCH16();
	if(ZF == 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jo_rel16()	// Opcode 0x0f 80
{
	int16 disp = FETCH16();
	if(OF != 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jp_rel16()	// Opcode 0x0f 8a
{
	int16 disp = FETCH16();
	if(PF != 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::js_rel16()	// Opcode 0x0f 88
{
	int16 disp = FETCH16();
	if(SF != 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jz_rel16()	// Opcode 0x0f 84
{
	int16 disp = FETCH16();
	if(ZF != 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jcxz16()	// Opcode 0xe3
{
	int8 disp = FETCH8();
	if(REG16(CX) == 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCXZ);		/* TODO: Timing = 9 + m */
	}
	else {
		CYCLES(CYCLES_JCXZ_NOBRANCH);
	}
}

void I386::jmp_rel16()	// Opcode 0xe9
{
	int16 disp = FETCH16();

	if(sreg[CS].d) {
		eip += disp;
	}
	else {
		eip = (eip + disp) & 0xffff;
	}
	CHANGE_PC(eip);
	CYCLES(CYCLES_JMP);		/* TODO: Timing = 7 + m */
}

void I386::jmp_abs16()	// Opcode 0xea
{
	uint16 address = FETCH16();
	uint16 segment = FETCH16();

	if(PROTECTED_MODE) {
		/* TODO: #GP */
		/* TODO: access rights, etc. */
		eip = address;
		sreg[CS].selector = segment;
		performed_intersegment_jump = 1;
		load_segment_descriptor(CS);
		CHANGE_PC(eip);
	}
	else {
		eip = address;
		sreg[CS].selector = segment;
		performed_intersegment_jump = 1;
		load_segment_descriptor(CS);
		CHANGE_PC(eip);
	}
	CYCLES(CYCLES_JMP_INTERSEG);
}

void I386::lea16()	// Opcode 0x8d
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0)
		trap(ILLEGAL_INSTRUCTION, 0);
	else {
		uint32 ea = GetNonTranslatedEA(modrm);
		STORE_REG16(modrm, ea);
		CYCLES(CYCLES_LEA);
	}
}

void I386::leave16()	// Opcode 0xc9
{
	REG16(SP) = REG16(BP);
	REG16(BP) = POP16();
	CYCLES(CYCLES_LEAVE);
}

void I386::lodsw()	// Opcode 0xad
{
	uint32 eas;
	if(segment_prefix) {
		eas = translate(segment_override, address_size ? REG32(ESI) : REG16(SI));
	}
	else {
		eas = translate(DS, address_size ? REG32(ESI) : REG16(SI));
	}
	REG16(AX) = RM16(eas);
	BUMP_SI(2);
	CYCLES(CYCLES_LODS);
}

void I386::loop16()	// Opcode 0xe2
{
	int8 disp = FETCH8();
	REG16(CX)--;
	if(REG16(CX) != 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
	}
	CYCLES(CYCLES_LOOP);		/* TODO: Timing = 11 + m */
}

void I386::loopne16()	// Opcode 0xe0
{
	int8 disp = FETCH8();
	REG16(CX)--;
	if(REG16(CX) != 0 && ZF == 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
	}
	CYCLES(CYCLES_LOOPNZ);		/* TODO: Timing = 11 + m */
}

void I386::loopz16()	// Opcode 0xe1
{
	int8 disp = FETCH8();
	REG16(CX)--;
	if(REG16(CX) != 0 && ZF != 0) {
		if(sreg[CS].d) {
			eip += disp;
		}
		else {
			eip = (eip + disp) & 0xffff;
		}
		CHANGE_PC(eip);
	}
	CYCLES(CYCLES_LOOPZ);		/* TODO: Timing = 11 + m */
}

void I386::mov_rm16_r16()	// Opcode 0x89
{
	uint16 src;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG16(modrm);
		STORE_RM16(modrm, src);
		CYCLES(CYCLES_MOV_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		WM16(ea, src);
		CYCLES(CYCLES_MOV_REG_MEM);
	}
}

void I386::mov_r16_rm16()	// Opcode 0x8b
{
	uint16 src;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM16(modrm);
		STORE_REG16(modrm, src);
		CYCLES(CYCLES_MOV_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM16(ea);
		STORE_REG16(modrm, src);
		CYCLES(CYCLES_MOV_MEM_REG);
	}
}

void I386::mov_rm16_i16()	// Opcode 0xc7
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint16 val = FETCH16();
		STORE_RM16(modrm, val);
		CYCLES(CYCLES_MOV_IMM_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint16 val = FETCH16();
		WM16(ea, val);
		CYCLES(CYCLES_MOV_IMM_MEM);
	}
}

void I386::mov_ax_m16()	// Opcode 0xa1
{
	uint32 offset, ea;
	if(address_size)
		offset = FETCH32();
	else
		offset = FETCH16();
	/* TODO: Not sure if this is correct... */
	if(segment_prefix)
		ea = translate(segment_override, offset);
	else
		ea = translate(DS, offset);
	REG16(AX) = RM16(ea);
	CYCLES(CYCLES_MOV_MEM_ACC);
}

void I386::mov_m16_ax()	// Opcode 0xa3
{
	uint32 offset, ea;
	if(address_size)
		offset = FETCH32();
	else
		offset = FETCH16();
	/* TODO: Not sure if this is correct... */
	if(segment_prefix)
		ea = translate(segment_override, offset);
	else
		ea = translate(DS, offset);
	WM16(ea, REG16(AX));
	CYCLES(CYCLES_MOV_ACC_MEM);
}

void I386::mov_ax_i16()	// Opcode 0xb8
{
	REG16(AX) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_cx_i16()	// Opcode 0xb9
{
	REG16(CX) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_dx_i16()	// Opcode 0xba
{
	REG16(DX) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_bx_i16()	// Opcode 0xbb
{
	REG16(BX) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_sp_i16()	// Opcode 0xbc
{
	REG16(SP) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_bp_i16()	// Opcode 0xbd
{
	REG16(BP) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_si_i16()	// Opcode 0xbe
{
	REG16(SI) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_di_i16()	// Opcode 0xbf
{
	REG16(DI) = FETCH16();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::movsw()	// Opcode 0xa5
{
	uint32 eas, ead;
	uint16 v;
	if(segment_prefix)
		eas = translate(segment_override, address_size ? REG32(ESI) : REG16(SI));
	else
		eas = translate(DS, address_size ? REG32(ESI) : REG16(SI));
	ead = translate(ES, address_size ? REG32(EDI) : REG16(DI));
	v = RM16(eas);
	WM16(ead, v);
	BUMP_SI(2);
	BUMP_DI(2);
	CYCLES(CYCLES_MOVS);
}

void I386::movsx_r16_rm8()	// Opcode 0x0f be
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		int16 src = (int8)LOAD_RM8(modrm);
		STORE_REG16(modrm, src);
		CYCLES(CYCLES_MOVSX_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		int16 src = (int8)RM8(ea);
		STORE_REG16(modrm, src);
		CYCLES(CYCLES_MOVSX_MEM_REG);
	}
}

void I386::movzx_r16_rm8()	// Opcode 0x0f b6
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint16 src = (uint8)LOAD_RM8(modrm);
		STORE_REG16(modrm, src);
		CYCLES(CYCLES_MOVZX_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint16 src = (uint8)RM8(ea);
		STORE_REG16(modrm, src);
		CYCLES(CYCLES_MOVZX_MEM_REG);
	}
}

void I386::or_rm16_r16()	// Opcode 0x09
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = OR16(dst, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = RM16(ea);
		dst = OR16(dst, src);
		WM16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::or_r16_rm16()	// Opcode 0x0b
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		dst = OR16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM16(ea);
		dst = LOAD_REG16(modrm);
		dst = OR16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::or_ax_i16()	// Opcode 0x0d
{
	uint16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	dst = OR16(dst, src);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::out_ax_i8()	// Opcode 0xe7
{
	uint16 port = FETCH8();
	uint16 data = REG16(AX);
	OUT16(port, data);
	CYCLES(CYCLES_OUT_VAR);
}

void I386::out_ax_dx()	// Opcode 0xef
{
	uint16 port = REG16(DX);
	uint16 data = REG16(AX);
	OUT16(port, data);
	CYCLES(CYCLES_OUT);
}

void I386::pop_ax()	// Opcode 0x58
{
	REG16(AX) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_cx()	// Opcode 0x59
{
	REG16(CX) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_dx()	// Opcode 0x5a
{
	REG16(DX) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_bx()	// Opcode 0x5b
{
	REG16(BX) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_sp()	// Opcode 0x5c
{
	REG16(SP) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_bp()	// Opcode 0x5d
{
	REG16(BP) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_si()	// Opcode 0x5e
{
	REG16(SI) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_di()	// Opcode 0x5f
{
	REG16(DI) = POP16();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_ds16()	// Opcode 0x1f
{
	sreg[DS].selector = POP16();
	if(PROTECTED_MODE)
		load_segment_descriptor(DS);
	else
		load_segment_descriptor(DS);
	CYCLES(CYCLES_POP_SREG);
}

void I386::pop_es16()	// Opcode 0x07
{
	sreg[ES].selector = POP16();
	if(PROTECTED_MODE)
		load_segment_descriptor(ES);
	else
		load_segment_descriptor(ES);
	CYCLES(CYCLES_POP_SREG);
}

void I386::pop_fs16()	// Opcode 0x0f a1
{
	sreg[FS].selector = POP16();
	if(PROTECTED_MODE)
		load_segment_descriptor(FS);
	else
		load_segment_descriptor(FS);
	CYCLES(CYCLES_POP_SREG);
}

void I386::pop_gs16()	// Opcode 0x0f a9
{
	sreg[GS].selector = POP16();
	if(PROTECTED_MODE)
		load_segment_descriptor(GS);
	else
		load_segment_descriptor(GS);
	CYCLES(CYCLES_POP_SREG);
}

void I386::pop_ss16()	// Opcode 0x17
{
	sreg[SS].selector = POP16();
	if(PROTECTED_MODE)
		load_segment_descriptor(SS);
	else
		load_segment_descriptor(SS);
	CYCLES(CYCLES_POP_SREG);
}

void I386::pop_rm16()	// Opcode 0x8f
{
	uint8 modrm = FETCH8();
	uint16 val = POP16();
	
	if(modrm >= 0xc0)
		STORE_RM16(modrm, val);
	else {
		uint32 ea = GetEA(modrm);
		WM16(ea, val);
	}
	CYCLES(CYCLES_POP_RM);
}

void I386::popa()	// Opcode 0x61
{
	REG16(DI) = POP16();
	REG16(SI) = POP16();
	REG16(BP) = POP16();
	REG16(SP) += 2;
	REG16(BX) = POP16();
	REG16(DX) = POP16();
	REG16(CX) = POP16();
	REG16(AX) = POP16();
	CYCLES(CYCLES_POPA);
}

void I386::popf()	// Opcode 0x9d
{
	uint16 val = POP16();
	set_flags(val);
	CYCLES(CYCLES_POPF);
}

void I386::push_ax()	// Opcode 0x50
{
	PUSH16(REG16(AX));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_cx()	// Opcode 0x51
{
	PUSH16(REG16(CX));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_dx()	// Opcode 0x52
{
	PUSH16(REG16(DX));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_bx()	// Opcode 0x53
{
	PUSH16(REG16(BX));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_sp()	// Opcode 0x54
{
	PUSH16(REG16(SP));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_bp()	// Opcode 0x55
{
	PUSH16(REG16(BP));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_si()	// Opcode 0x56
{
	PUSH16(REG16(SI));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_di()	// Opcode 0x57
{
	PUSH16(REG16(DI));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_cs16()	// Opcode 0x0e
{
	PUSH16(sreg[CS].selector);
	CYCLES(CYCLES_PUSH_SREG);
}

void I386::push_ds16()	// Opcode 0x1e
{
	PUSH16(sreg[DS].selector);
	CYCLES(CYCLES_PUSH_SREG);
}

void I386::push_es16()	// Opcode 0x06
{
	PUSH16(sreg[ES].selector);
	CYCLES(CYCLES_PUSH_SREG);
}

void I386::push_fs16()	// Opcode 0x0f a0
{
	PUSH16(sreg[FS].selector);
	CYCLES(CYCLES_PUSH_SREG);
}

void I386::push_gs16()	// Opcode 0x0f a8
{
	PUSH16(sreg[GS].selector);
	CYCLES(CYCLES_PUSH_SREG);
}

void I386::push_ss16()	// Opcode 0x16
{
	PUSH16(sreg[SS].selector);
	CYCLES(CYCLES_PUSH_SREG);
}

void I386::push_i16()	// Opcode 0x68
{
	uint16 val = FETCH16();
	PUSH16(val);
	CYCLES(CYCLES_PUSH_IMM);
}

void I386::pusha()	// Opcode 0x60
{
	uint16 temp = REG16(SP);
	PUSH16(REG16(AX));
	PUSH16(REG16(CX));
	PUSH16(REG16(DX));
	PUSH16(REG16(BX));
	PUSH16(temp);
	PUSH16(REG16(BP));
	PUSH16(REG16(SI));
	PUSH16(REG16(DI));
	CYCLES(CYCLES_PUSHA);
}

void I386::pushf()	// Opcode 0x9c
{
	PUSH16(get_flags() & 0xffff);
	CYCLES(CYCLES_PUSHF);
}

void I386::ret_near16_i16()	// Opcode 0xc2
{
	int16 disp = FETCH16();
	eip = POP16();
	REG16(SP) += disp;
	CHANGE_PC(eip);
	CYCLES(CYCLES_RET_IMM);		/* TODO: Timing = 10 + m */
}

void I386::ret_near16()	// Opcode 0xc3
{
	eip = POP16();
	CHANGE_PC(eip);
	CYCLES(CYCLES_RET);		/* TODO: Timing = 10 + m */
}

void I386::sbb_rm16_r16()	// Opcode 0x19
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG16(modrm) + CF;
		dst = LOAD_RM16(modrm);
		dst = SUB16(dst, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm) + CF;
		dst = RM16(ea);
		dst = SUB16(dst, src);
		WM16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::sbb_r16_rm16()	// Opcode 0x1b
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM16(modrm) + CF;
		dst = LOAD_REG16(modrm);
		dst = SUB16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM16(ea) + CF;
		dst = LOAD_REG16(modrm);
		dst = SUB16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::sbb_ax_i16()	// Opcode 0x1d
{
	uint16 src, dst;
	src = FETCH16() + CF;
	dst = REG16(AX);
	dst = SUB16(dst, src);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::scasw()	// Opcode 0xaf
{
	uint32 eas;
	uint16 src, dst;
	eas = translate(ES, address_size ? REG32(EDI) : REG16(DI));
	src = RM16(eas);
	dst = REG16(AX);
	SUB16(dst, src);
	BUMP_DI(2);
	CYCLES(CYCLES_SCAS);
}

void I386::shld16_i8()	// Opcode 0x0f a4
{
	/* TODO: Correct flags */
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint16 dst = LOAD_RM16(modrm);
		uint16 upper = LOAD_REG16(modrm);
		uint8 shift = FETCH8();
		if(shift > 31 || shift == 0) {

		}
		else if(shift > 15) {
			CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (upper << (shift-16)) | (upper >> (32-shift));
			SetSZPF16(dst);
		}
		else {
			CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (16-shift));
			SetSZPF16(dst);
		}
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_SHLD_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint16 dst = RM16(ea);
		uint16 upper = LOAD_REG16(modrm);
		uint8 shift = FETCH8();
		if(shift > 31 || shift == 0) {

		}
		else if(shift > 15) {
			CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (upper << (shift-16)) | (upper >> (32-shift));
			SetSZPF16(dst);
		}
		else {
			CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (16-shift));
			SetSZPF16(dst);
		}
		WM16(ea, dst);
		CYCLES(CYCLES_SHLD_MEM);
	}
}

void I386::shld16_cl()	// Opcode 0x0f a5
{
	/* TODO: Correct flags */
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint16 dst = LOAD_RM16(modrm);
		uint16 upper = LOAD_REG16(modrm);
		uint8 shift = REG8(CL);
		if(shift > 31 || shift == 0) {

		}
		else if(shift > 15) {
			CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (upper << (shift-16)) | (upper >> (32-shift));
			SetSZPF16(dst);
		}
		else {
			CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (16-shift));
			SetSZPF16(dst);
		}
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_SHLD_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint16 dst = RM16(ea);
		uint16 upper = LOAD_REG16(modrm);
		uint8 shift = REG8(CL);
		if(shift > 31 || shift == 0) {

		}
		else if(shift > 15) {
			CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (upper << (shift-16)) | (upper >> (32-shift));
			SetSZPF16(dst);
		}
		else {
			CF = (dst & (1 << (16-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (16-shift));
			SetSZPF16(dst);
		}
		WM16(ea, dst);
		CYCLES(CYCLES_SHLD_MEM);
	}
}

void I386::shrd16_i8()	// Opcode 0x0f ac
{
	/* TODO: Correct flags */
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint16 dst = LOAD_RM16(modrm);
		uint16 upper = LOAD_REG16(modrm);
		uint8 shift = FETCH8();
		if(shift > 15 || shift == 0) {

		}
		else {
			CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (16-shift));
			SetSZPF16(dst);
		}
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_SHRD_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint16 dst = RM16(ea);
		uint16 upper = LOAD_REG16(modrm);
		uint8 shift = FETCH8();
		if(shift > 15 || shift == 0) {

		}
		else {
			CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (16-shift));
			SetSZPF16(dst);
		}
		WM16(ea, dst);
		CYCLES(CYCLES_SHRD_MEM);
	}
}

void I386::shrd16_cl()	// Opcode 0x0f ad
{
	/* TODO: Correct flags */
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint16 dst = LOAD_RM16(modrm);
		uint16 upper = LOAD_REG16(modrm);
		uint8 shift = REG8(CL);
		if(shift > 15 || shift == 0) {

		}
		else {
			CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (16-shift));
			SetSZPF16(dst);
		}
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_SHRD_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint16 dst = RM16(ea);
		uint16 upper = LOAD_REG16(modrm);
		uint8 shift = REG8(CL);
		if(shift > 15 || shift == 0) {

		}
		else {
			CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (16-shift));
			SetSZPF16(dst);
		}
		WM16(ea, dst);
		CYCLES(CYCLES_SHRD_MEM);
	}
}

void I386::stosw()	// Opcode 0xab
{
	uint32 ead;
	ead = translate(ES, address_size ? REG32(EDI) : REG16(DI));
	WM16(ead, REG16(AX));
	BUMP_DI(2);
	CYCLES(CYCLES_STOS);
}

void I386::sub_rm16_r16()	// Opcode 0x29
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = SUB16(dst, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = RM16(ea);
		dst = SUB16(dst, src);
		WM16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::sub_r16_rm16()	// Opcode 0x2b
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		dst = SUB16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM16(ea);
		dst = LOAD_REG16(modrm);
		dst = SUB16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::sub_ax_i16()	// Opcode 0x2d
{
	uint16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	dst = SUB16(dst, src);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::test_ax_i16()	// Opcode 0xa9
{
	uint16 src = FETCH16();
	uint16 dst = REG16(AX);
	dst = src & dst;
	SetSZPF16(dst);
	CF = 0;
	OF = 0;
	CYCLES(CYCLES_TEST_IMM_ACC);
}

void I386::test_rm16_r16()	// Opcode 0x85
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = src & dst;
		SetSZPF16(dst);
		CF = 0;
		OF = 0;
		CYCLES(CYCLES_TEST_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = RM16(ea);
		dst = src & dst;
		SetSZPF16(dst);
		CF = 0;
		OF = 0;
		CYCLES(CYCLES_TEST_REG_MEM);
	}
}

void I386::xchg_ax_cx()	// Opcode 0x91
{
	uint16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(CX);
	REG16(CX) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_ax_dx()	// Opcode 0x92
{
	uint16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(DX);
	REG16(DX) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_ax_bx()	// Opcode 0x93
{
	uint16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(BX);
	REG16(BX) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_ax_sp()	// Opcode 0x94
{
	uint16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(SP);
	REG16(SP) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_ax_bp()	// Opcode 0x95
{
	uint16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(BP);
	REG16(BP) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_ax_si()	// Opcode 0x96
{
	uint16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(SI);
	REG16(SI) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_ax_di()	// Opcode 0x97
{
	uint16 temp;
	temp = REG16(AX);
	REG16(AX) = REG16(DI);
	REG16(DI) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_r16_rm16()	// Opcode 0x87
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint16 src = LOAD_RM16(modrm);
		uint16 dst = LOAD_REG16(modrm);
		STORE_REG16(modrm, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_XCHG_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint16 src = RM16(ea);
		uint16 dst = LOAD_REG16(modrm);
		STORE_REG16(modrm, src);
		WM16(ea, dst);
		CYCLES(CYCLES_XCHG_REG_MEM);
	}
}

void I386::xor_rm16_r16()	// Opcode 0x31
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG16(modrm);
		dst = LOAD_RM16(modrm);
		dst = XOR16(dst, src);
		STORE_RM16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG16(modrm);
		dst = RM16(ea);
		dst = XOR16(dst, src);
		WM16(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::xor_r16_rm16()	// Opcode 0x33
{
	uint16 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM16(modrm);
		dst = LOAD_REG16(modrm);
		dst = XOR16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM16(ea);
		dst = LOAD_REG16(modrm);
		dst = XOR16(dst, src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::xor_ax_i16()	// Opcode 0x35
{
	uint16 src, dst;
	src = FETCH16();
	dst = REG16(AX);
	dst = XOR16(dst, src);
	REG16(AX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}



void I386::group81_16()	// Opcode 0x81
{
	uint32 ea;
	uint16 src, dst;
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 0:		// ADD Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				dst = ADD16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = FETCH16();
				dst = ADD16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 1:		// OR Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				dst = OR16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = FETCH16();
				dst = OR16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 2:		// ADC Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				src = ADD16(src, CF);
				dst = ADD16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = FETCH16();
				src = ADD16(src, CF);
				dst = ADD16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 3:		// SBB Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = FETCH16() + CF;
				dst = SUB16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = FETCH16() + CF;
				dst = SUB16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 4:		// AND Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				dst = AND16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = FETCH16();
				dst = AND16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 5:		// SUB Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				dst = SUB16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = FETCH16();
				dst = SUB16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 6:		// XOR Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				dst = XOR16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = FETCH16();
				dst = XOR16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 7:		// CMP Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = FETCH16();
				SUB16(dst, src);
				CYCLES(CYCLES_CMP_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = FETCH16();
				SUB16(dst, src);
				CYCLES(CYCLES_CMP_REG_MEM);
			}
			break;
	}
}

void I386::group83_16()	// Opcode 0x83
{
	uint32 ea;
	uint16 src, dst;
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 0:		// ADD Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = (uint16)(int16)(int8)FETCH8();
				dst = ADD16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = (uint16)(int16)(int8)FETCH8();
				dst = ADD16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 1:		// OR Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = (uint16)(int16)(int8)FETCH8();
				dst = OR16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = (uint16)(int16)(int8)FETCH8();
				dst = OR16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 2:		// ADC Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = (uint16)(int16)(int8)FETCH8();
				src = ADD16(src, CF);
				dst = ADD16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = (uint16)(int16)(int8)FETCH8();
				src = ADD16(src, CF);
				dst = ADD16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 3:		// SBB Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = ((uint16)(int16)(int8)FETCH8()) + CF;
				dst = SUB16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = ((uint16)(int16)(int8)FETCH8()) + CF;
				dst = SUB16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 4:		// AND Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = (uint16)(int16)(int8)FETCH8();
				dst = AND16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = (uint16)(int16)(int8)FETCH8();
				dst = AND16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 5:		// SUB Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = (uint16)(int16)(int8)FETCH8();
				dst = SUB16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = (uint16)(int16)(int8)FETCH8();
				dst = SUB16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 6:		// XOR Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = (uint16)(int16)(int8)FETCH8();
				dst = XOR16(dst, src);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = (uint16)(int16)(int8)FETCH8();
				dst = XOR16(dst, src);
				WM16(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 7:		// CMP Rm16, i16
			if(modrm >= 0xc0) {
				dst = LOAD_RM16(modrm);
				src = (uint16)(int16)(int8)FETCH8();
				SUB16(dst, src);
				CYCLES(CYCLES_CMP_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM16(ea);
				src = (uint16)(int16)(int8)FETCH8();
				SUB16(dst, src);
				CYCLES(CYCLES_CMP_REG_MEM);
			}
			break;
	}
}

void I386::groupC1_16()	// Opcode 0xc1
{
	uint16 dst;
	uint8 modrm = FETCH8();
	uint8 shift;

	if(modrm >= 0xc0) {
		dst = LOAD_RM16(modrm);
		shift = FETCH8() & 0x1f;
		dst = shift_rotate16(modrm, dst, shift);
		STORE_RM16(modrm, dst);
	}
	else {
		uint32 ea = GetEA(modrm);
		dst = RM16(ea);
		shift = FETCH8() & 0x1f;
		dst = shift_rotate16(modrm, dst, shift);
		WM16(ea, dst);
	}
}

void I386::groupD1_16()	// Opcode 0xd1
{
	uint16 dst;
	uint8 modrm = FETCH8();

	if(modrm >= 0xc0) {
		dst = LOAD_RM16(modrm);
		dst = shift_rotate16(modrm, dst, 1);
		STORE_RM16(modrm, dst);
	}
	else {
		uint32 ea = GetEA(modrm);
		dst = RM16(ea);
		dst = shift_rotate16(modrm, dst, 1);
		WM16(ea, dst);
	}
}

void I386::groupD3_16()	// Opcode 0xd3
{
	uint16 dst;
	uint8 modrm = FETCH8();

	if(modrm >= 0xc0) {
		dst = LOAD_RM16(modrm);
		dst = shift_rotate16(modrm, dst, REG8(CL));
		STORE_RM16(modrm, dst);
	}
	else {
		uint32 ea = GetEA(modrm);
		dst = RM16(ea);
		dst = shift_rotate16(modrm, dst, REG8(CL));
		WM16(ea, dst);
	}
}

void I386::groupF7_16()	// Opcode 0xf7
{
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 0:			/* TEST Rm16, i16 */
			if(modrm >= 0xc0) {
				uint16 dst = LOAD_RM16(modrm);
				uint16 src = FETCH16();
				dst &= src;
				CF = OF = AF = 0;
				SetSZPF16(dst);
				CYCLES(CYCLES_TEST_IMM_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint16 dst = RM16(ea);
				uint16 src = FETCH16();
				dst &= src;
				CF = OF = AF = 0;
				SetSZPF16(dst);
				CYCLES(CYCLES_TEST_IMM_MEM);
			}
			break;
		case 2:			/* NOT Rm16 */
			if(modrm >= 0xc0) {
				uint16 dst = LOAD_RM16(modrm);
				dst = ~dst;
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_NOT_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint16 dst = RM16(ea);
				dst = ~dst;
				WM16(ea, dst);
				CYCLES(CYCLES_NOT_MEM);
			}
			break;
		case 3:			/* NEG Rm16 */
			if(modrm >= 0xc0) {
				uint16 dst = LOAD_RM16(modrm);
				dst = SUB16(0, dst);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_NEG_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint16 dst = RM16(ea);
				dst = SUB16(0, dst);
				WM16(ea, dst);
				CYCLES(CYCLES_NEG_MEM);
			}
			break;
		case 4:			/* MUL AX, Rm16 */
			{
				uint32 result;
				uint16 src, dst;
				if(modrm >= 0xc0) {
					src = LOAD_RM16(modrm);
					CYCLES(CYCLES_MUL16_ACC_REG);		/* TODO: Correct multiply timing */
				}
				else {
					uint32 ea = GetEA(modrm);
					src = RM16(ea);
					CYCLES(CYCLES_MUL16_ACC_MEM);		/* TODO: Correct multiply timing */
				}

				dst = REG16(AX);
				result = (uint32)src * (uint32)dst;
				REG16(DX) = (uint16)(result >> 16);
				REG16(AX) = (uint16)result;

				CF = OF = (REG16(DX) != 0);
			}
			break;
		case 5:			/* IMUL AX, Rm16 */
			{
				int32 result;
				int32 src, dst;
				if(modrm >= 0xc0) {
					src = (int32)(int16)LOAD_RM16(modrm);
					CYCLES(CYCLES_IMUL16_ACC_REG);		/* TODO: Correct multiply timing */
				}
				else {
					uint32 ea = GetEA(modrm);
					src = (int32)(int16)RM16(ea);
					CYCLES(CYCLES_IMUL16_ACC_MEM);		/* TODO: Correct multiply timing */
				}

				dst = (int32)(int16)REG16(AX);
				result = src * dst;

				REG16(DX) = (uint16)(result >> 16);
				REG16(AX) = (uint16)result;

				CF = OF = !(result == (int32)(int16)result);
			}
			break;
		case 6:			/* DIV AX, Rm16 */
			{
				uint16 src;
				if(modrm >= 0xc0) {
					src = LOAD_RM16(modrm);
					CYCLES(CYCLES_DIV16_ACC_REG);
				}
				else {
					uint32 ea = GetEA(modrm);
					src = RM16(ea);
					CYCLES(CYCLES_DIV16_ACC_MEM);
				}
				uint32 quotient = ((uint32)(REG16(DX)) << 16) | (uint32)(REG16(AX));
				if(!src)
					trap(DIVIDE_FAULT, 1);
				else {
					uint32 remainder = quotient % (uint32)src;
					uint32 result = quotient / (uint32)src;
					if(result > 0xffff)
						trap(DIVIDE_FAULT, 1);
					else {
						REG16(DX) = (uint16)remainder;
						REG16(AX) = (uint16)result;
					}
				}
			}
			break;
		case 7:			/* IDIV AX, Rm16 */
			{
				uint16 src;
				if(modrm >= 0xc0) {
					src = LOAD_RM16(modrm);
					CYCLES(CYCLES_IDIV16_ACC_REG);
				}
				else {
					uint32 ea = GetEA(modrm);
					src = RM16(ea);
					CYCLES(CYCLES_IDIV16_ACC_MEM);
				}
				int32 quotient = (((int32)REG16(DX)) << 16) | ((uint32)REG16(AX));
				if(!src)
					trap(DIVIDE_FAULT, 1);
				else {
					int32 remainder = quotient % (int32)(int16)src;
					int32 result = quotient / (int32)(int16)src;
					if(result > 0xffff)
						trap(DIVIDE_FAULT, 1);
					else {
						REG16(DX) = (uint16)remainder;
						REG16(AX) = (uint16)result;
					}
				}
			}
			break;
	}
}

void I386::groupFF_16()	// Opcode 0xff
{
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 0:			/* INC Rm16 */
			if(modrm >= 0xc0) {
				uint16 dst = LOAD_RM16(modrm);
				dst = INC16(dst);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_INC_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint16 dst = RM16(ea);
				dst = INC16(dst);
				WM16(ea, dst);
				CYCLES(CYCLES_INC_MEM);
			}
			break;
		case 1:			/* DEC Rm16 */
			if(modrm >= 0xc0) {
				uint16 dst = LOAD_RM16(modrm);
				dst = DEC16(dst);
				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_DEC_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint16 dst = RM16(ea);
				dst = DEC16(dst);
				WM16(ea, dst);
				CYCLES(CYCLES_DEC_MEM);
			}
			break;
		case 2:			/* CALL Rm16 */
			{
				uint16 address;
				if(modrm >= 0xc0) {
					address = LOAD_RM16(modrm);
					CYCLES(CYCLES_CALL_REG);		/* TODO: Timing = 7 + m */
				}
				else {
					uint32 ea = GetEA(modrm);
					address = RM16(ea);
					CYCLES(CYCLES_CALL_MEM);		/* TODO: Timing = 10 + m */
				}
				PUSH16(eip);
				eip = address;
				CHANGE_PC(eip);
#ifdef I386_BIOS_CALL
				BIOS_CALL_NEAR16();
#endif
			}
			break;
		case 3:			/* CALL FAR Rm16 */
			{
				uint16 address, selector;
				if(modrm >= 0xc0) {
//					fatalerror(_T("NYI");
				}
				else {
					uint32 ea = GetEA(modrm);
					address = RM16(ea + 0);
					selector = RM16(ea + 2);
					CYCLES(CYCLES_CALL_MEM_INTERSEG);		/* TODO: Timing = 10 + m */
				}
				PUSH16(sreg[CS].selector);
				PUSH16(eip);
				sreg[CS].selector = selector;
				performed_intersegment_jump = 1;
				load_segment_descriptor(CS);
				eip = address;
				CHANGE_PC(eip);
#ifdef I386_BIOS_CALL
				BIOS_CALL_FAR16();
#endif
			}
			break;
		case 4:			/* JMP Rm16 */
			{
				uint16 address;
				if(modrm >= 0xc0) {
					address = LOAD_RM16(modrm);
					CYCLES(CYCLES_JMP_REG);		/* TODO: Timing = 7 + m */
				}
				else {
					uint32 ea = GetEA(modrm);
					address = RM16(ea);
					CYCLES(CYCLES_JMP_MEM);		/* TODO: Timing = 10 + m */
				}
				eip = address;
				CHANGE_PC(eip);
			}
			break;
		case 5:			/* JMP FAR Rm16 */
			{
				uint16 address, selector;
				if(modrm >= 0xc0) {
//					fatalerror(_T("NYI\n"));
				}
				else {
					uint32 ea = GetEA(modrm);
					address = RM16(ea + 0);
					selector = RM16(ea + 2);
					CYCLES(CYCLES_JMP_MEM_INTERSEG);		/* TODO: Timing = 10 + m */
				}
				sreg[CS].selector = selector;
				performed_intersegment_jump = 1;
				load_segment_descriptor(CS);
				eip = address;
				CHANGE_PC(eip);
			}
			break;
		case 6:			/* PUSH Rm16 */
			{
				uint16 val;
				if(modrm >= 0xc0) {
					val = LOAD_RM16(modrm);
				}
				else {
					uint32 ea = GetEA(modrm);
					val = RM16(ea);
				}
				PUSH16(val);
				CYCLES(CYCLES_PUSH_RM);
			}
			break;
		case 7:
			invalid();
			break;
		default:
//			fatalerror(_T("i386: groupFF_16 /%d unimplemented\n"), (modrm >> 3) & 7);
			break;
	}
}

void I386::group0F00_16()	// Opcode 0x0f 00
{
	uint32 address, ea;
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 2:			/* LLDT */
			if(PROTECTED_MODE && !V8086_MODE) {
				if(modrm >= 0xc0) {
					address = LOAD_RM16(modrm);
					ea = translate(CS, address);
					CYCLES(CYCLES_LLDT_REG);
				}
				else {
					ea = GetEA(modrm);
					CYCLES(CYCLES_LLDT_MEM);
				}
				ldtr.segment = RM16(ea);
			}
			else {
				trap(ILLEGAL_INSTRUCTION, 0);
			}
			break;

		case 3:			/* LTR */
			if(PROTECTED_MODE && !V8086_MODE) {
				if(modrm >= 0xc0) {
					address = LOAD_RM16(modrm);
					ea = translate(CS, address);
					CYCLES(CYCLES_LTR_REG);
				}
				else {
					ea = GetEA(modrm);
					CYCLES(CYCLES_LTR_MEM);
				}
				task.segment = RM16(ea);
			}
			else {
				trap(ILLEGAL_INSTRUCTION, 0);
			}
			break;

		default:
//			fatalerror(_T("i386: group0F00_16 /%d unimplemented\n"), (modrm >> 3) & 7);
			break;
	}
}

void I386::group0F01_16()	// Opcode 0x0f 01
{
	uint8 modrm = FETCH8();
	uint16 address;
	uint32 ea;
#ifdef HAS_I386
	switch((modrm >> 3) & 7) {
		case 0:			/* SGDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM16(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				WM16(ea, gdtr.limit);
				WM32(ea + 2, gdtr.base & 0xffffff);
				CYCLES(CYCLES_SGDT);
				break;
			}
		case 1:			/* SIDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM16(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				WM16(ea, idtr.limit);
				WM32(ea + 2, idtr.base & 0xffffff);
				CYCLES(CYCLES_SIDT);
				break;
			}
		case 2:			/* LGDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM16(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				gdtr.limit = RM16(ea);
				gdtr.base = RM32(ea + 2) & 0xffffff;
				CYCLES(CYCLES_LGDT);
				break;
			}
		case 3:			/* LIDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM16(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				idtr.limit = RM16(ea);
				idtr.base = RM32(ea + 2) & 0xffffff;
				CYCLES(CYCLES_LIDT);
				break;
			}
		case 4:			/* SMSW */
			{
				if(modrm >= 0xc0) {
					STORE_RM16(modrm, cr[0]);
					CYCLES(CYCLES_SMSW_REG);
				}
				else {
					uint32 ea = GetEA(modrm);
					WM16(ea, cr[0]);
					CYCLES(CYCLES_SMSW_MEM);
				}
				break;
			}
		case 6:			/* LMSW */
			{
				// TODO: Check for protection fault
				uint8 b;
				if(modrm >= 0xc0) {
					b = LOAD_RM8(modrm);
					CYCLES(CYCLES_LMSW_REG);
				}
				else {
					ea = GetEA(modrm);
					CYCLES(CYCLES_LMSW_MEM);
					b = RM8(ea);
				}
				cr[0] &= ~0x03;
				cr[0] |= b & 0x03;
				break;
			}
		default:
//			fatalerror(_T("i386: unimplemented opcode 0x0f 01 /%d at %08X\n"), (modrm >> 3) & 7, eip - 2);
			break;
	}
#else
	switch((modrm >> 3) & 7) {
		case 0:			/* SGDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM16(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				WM16(ea, gdtr.limit);
				WM32(ea + 2, gdtr.base & 0xffffff);
				CYCLES(CYCLES_SGDT);
				break;
			}
		case 1:			/* SIDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM16(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				WM16(ea, idtr.limit);
				WM32(ea + 2, idtr.base & 0xffffff);
				CYCLES(CYCLES_SIDT);
				break;
			}
		case 2:			/* LGDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM16(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				gdtr.limit = RM16(ea);
				gdtr.base = RM32(ea + 2) & 0xffffff;
				CYCLES(CYCLES_LGDT);
				break;
			}
		case 3:			/* LIDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM16(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				idtr.limit = RM16(ea);
				idtr.base = RM32(ea + 2) & 0xffffff;
				CYCLES(CYCLES_LIDT);
				break;
			}
		case 4:			/* SMSW */
			{
				if(modrm >= 0xc0) {
					STORE_RM16(modrm, cr[0]);
					CYCLES(CYCLES_SMSW_REG);
				}
				else {
					uint32 ea = GetEA(modrm);
					WM16(ea, cr[0]);
					CYCLES(CYCLES_SMSW_MEM);
				}
				break;
			}
		case 6:			/* LMSW */
			{
				// TODO: Check for protection fault
				uint8 b;
				if(modrm >= 0xc0) {
					b = LOAD_RM8(modrm);
					CYCLES(CYCLES_LMSW_REG);
				}
				else {
					ea = GetEA(modrm);
					CYCLES(CYCLES_LMSW_MEM);
				b = RM8(ea);
				}
				cr[0] &= ~0x03;
				cr[0] |= b & 0x03;
				break;
			}
		case 7:			/* INVLPG */
			{
				// Nothing to do ?
				break;
			}
		default:
//			fatalerror(_T("i486: unimplemented opcode 0x0f 01 /%d at %08X\n"), (modrm >> 3) & 7, eip - 2);
			break;
	}
#endif
}

void I386::group0FBA_16()	// Opcode 0x0f ba
{
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 4:			/* BT Rm16, i8 */
			if(modrm >= 0xc0) {
				uint16 dst = LOAD_RM16(modrm);
				uint8 bit = FETCH8();
				
				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				CYCLES(CYCLES_BT_IMM_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint16 dst = RM16(ea);
				uint8 bit = FETCH8();
				
				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				CYCLES(CYCLES_BT_IMM_MEM);
			}
			break;
		case 5:			/* BTS Rm16, i8 */
			if(modrm >= 0xc0) {
				uint16 dst = LOAD_RM16(modrm);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				dst |= (1 << bit);

				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_BTS_IMM_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint16 dst = RM16(ea);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				dst |= (1 << bit);

				WM16(ea, dst);
				CYCLES(CYCLES_BTS_IMM_MEM);
			}
			break;
		case 6:			/* BTR Rm16, i8 */
			if(modrm >= 0xc0) {
				uint16 dst = LOAD_RM16(modrm);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				dst &= ~(1 << bit);

				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_BTR_IMM_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint16 dst = RM16(ea);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				dst &= ~(1 << bit);

				WM16(ea, dst);
				CYCLES(CYCLES_BTR_IMM_MEM);
			}
			break;
		case 7:			/* BTC Rm16, i8 */
			if(modrm >= 0xc0) {
				uint16 dst = LOAD_RM16(modrm);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				dst ^= (1 << bit);

				STORE_RM16(modrm, dst);
				CYCLES(CYCLES_BTC_IMM_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint16 dst = RM16(ea);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				dst ^= (1 << bit);

				WM16(ea, dst);
				CYCLES(CYCLES_BTC_IMM_MEM);
			}
			break;
		default:
//			fatalerror(_T("i386: group0FBA_16 /%d unknown\n"), (modrm >> 3) & 7);
			break;
	}
}

void I386::bound_r16_m16_m16()	// Opcode 0x62
{
	uint8 modrm;
	int16 val, low, high;

	modrm = FETCH8();

	if(modrm >= 0xc0) {
		low = high = LOAD_RM16(modrm);
	}
	else {
		uint32 ea = GetEA(modrm);
		low = RM16(ea + 0);
		high = RM16(ea + 2);
	}
	val = LOAD_REG16(modrm);

	if((val < low) || (val > high)) {
		CYCLES(CYCLES_BOUND_OUT_RANGE);
		trap(5, 0);
	}
	else {
		CYCLES(CYCLES_BOUND_IN_RANGE);
	}
}

void I386::retf16()	// Opcode 0xcb
{
	eip = POP16();
	sreg[CS].selector = POP16();
	load_segment_descriptor(CS);
	CHANGE_PC(eip);

	CYCLES(CYCLES_RET_INTERSEG);
}

void I386::retf_i16()	// Opcode 0xca
{
	uint16 count = FETCH16();

	eip = POP16();
	sreg[CS].selector = POP16();
	load_segment_descriptor(CS);
	CHANGE_PC(eip);

	REG16(SP) += count;
	CYCLES(CYCLES_RET_IMM_INTERSEG);
}

void I386::xlat16()	// Opcode 0xd7
{
	uint32 ea;
	if(segment_prefix) {
		ea = translate(segment_override, REG16(BX) + REG8(AL));
	}
	else {
		ea = translate(DS, REG16(BX) + REG8(AL));
	}
	REG8(AL) = RM8(ea);
	CYCLES(CYCLES_XLAT);
}

void I386::load_far_pointer16(int s)
{
	uint8 modrm = FETCH8();

	if(modrm >= 0xc0) {
//		fatalerror(_T("NYI\n"));
	}
	else {
		uint32 ea = GetEA(modrm);
		STORE_REG16(modrm, RM16(ea + 0));
		sreg[s].selector = RM16(ea + 2);
		load_segment_descriptor(s);
	}
}

void I386::lds16()	// Opcode 0xc5
{
	load_far_pointer16(DS);
	CYCLES(CYCLES_LDS);
}

void I386::lss16()	// Opcode 0x0f 0xb2
{
	load_far_pointer16(SS);
	CYCLES(CYCLES_LSS);
}

void I386::les16()	// Opcode 0xc4
{
	load_far_pointer16(ES);
	CYCLES(CYCLES_LES);
}

void I386::lfs16()	// Opcode 0x0f 0xb4
{
	load_far_pointer16(FS);
	CYCLES(CYCLES_LFS);
}

void I386::lgs16()	// Opcode 0x0f 0xb5
{
	load_far_pointer16(GS);
	CYCLES(CYCLES_LGS);
}

uint32 I386::shift_rotate32(uint8 modrm, uint32 val, uint8 shift)
{
	uint32 dst, src;
	dst = val;
	src = val;

	if(shift == 0) {
		CYCLES_RM(modrm, 3, 7);
	}
	else if(shift == 1) {
		switch((modrm >> 3) & 7) {
			case 0:			/* ROL rm32, 1 */
				CF = (src & 0x80000000) ? 1 : 0;
				dst = (src << 1) + CF;
				OF = ((src ^ dst) & 0x80000000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 1:			/* ROR rm32, 1 */
				CF = (src & 1) ? 1 : 0;
				dst = (CF << 31) | (src >> 1);
				OF = ((src ^ dst) & 0x80000000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 2:			/* RCL rm32, 1 */
				dst = (src << 1) + CF;
				CF = (src & 0x80000000) ? 1 : 0;
				OF = ((src ^ dst) & 0x80000000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 3:			/* RCR rm32, 1 */
				dst = (CF << 31) | (src >> 1);
				CF = src & 1;
				OF = ((src ^ dst) & 0x80000000) ? 1 : 0;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 4:			/* SHL/SAL rm32, 1 */
			case 6:
				dst = src << 1;
				CF = (src & 0x80000000) ? 1 : 0;
				OF = (((CF << 31) ^ dst) & 0x80000000) ? 1 : 0;
				SetSZPF32(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 5:			/* SHR rm32, 1 */
				dst = src >> 1;
				CF = src & 1;
				OF = (src & 0x80000000) ? 1 : 0;
				SetSZPF32(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 7:			/* SAR rm32, 1 */
				dst = (int32)(src) >> 1;
				CF = src & 1;
				OF = 0;
				SetSZPF32(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
		}
	}
	else {
		switch((modrm >> 3) & 7) {
			case 0:			/* ROL rm32, i8 */
				dst = ((src & ((uint32)0xffffffff >> shift)) << shift) |
					  ((src & ((uint32)0xffffffff << (32-shift))) >> (32-shift));
				CF = (src >> (32-shift)) & 1;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 1:			/* ROR rm32, i8 */
				dst = ((src & ((uint32)0xffffffff << shift)) >> shift) |
					  ((src & ((uint32)0xffffffff >> (32-shift))) << (32-shift));
				CF = (src >> (shift-1)) & 1;
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 2:			/* RCL rm32, i8 */
				dst = ((src & ((uint32)0xffffffff >> shift)) << shift) |
					  ((src & ((uint32)0xffffffff << (33-shift))) >> (33-shift)) |
					  (CF << (shift-1));
				CF = (src >> (32-shift)) & 1;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 3:			/* RCR rm32, i8 */
				dst = ((src & ((uint32)0xffffffff << shift)) >> shift) |
					  ((src & ((uint32)0xffffffff >> (32-shift))) << (33-shift)) |
					  (CF << (32-shift));
				CF = (src >> (shift-1)) & 1;
				CYCLES_RM(modrm, CYCLES_ROTATE_CARRY_REG, CYCLES_ROTATE_CARRY_MEM);
				break;
			case 4:			/* SHL/SAL rm32, i8 */
			case 6:
				dst = src << shift;
				CF = (src & (1 << (32-shift))) ? 1 : 0;
				SetSZPF32(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 5:			/* SHR rm32, i8 */
				dst = src >> shift;
				CF = (src & (1 << (shift-1))) ? 1 : 0;
				SetSZPF32(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
			case 7:			/* SAR rm32, i8 */
				dst = (int32)src >> shift;
				CF = (src & (1 << (shift-1))) ? 1 : 0;
				SetSZPF32(dst);
				CYCLES_RM(modrm, CYCLES_ROTATE_REG, CYCLES_ROTATE_MEM);
				break;
		}

	}
	return dst;
}



void I386::adc_rm32_r32()	// Opcode 0x11
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		src = ADD32(src, CF);
		dst = ADD32(dst, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = RM32(ea);
		src = ADD32(src, CF);
		dst = ADD32(dst, src);
		WM32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::adc_r32_rm32()	// Opcode 0x13
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		src = ADD32(src, CF);
		dst = ADD32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM32(ea);
		dst = LOAD_REG32(modrm);
		src = ADD32(src, CF);
		dst = ADD32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::adc_eax_i32()	// Opcode 0x15
{
	uint32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	src = ADD32(src, CF);
	dst = ADD32(dst, src);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::add_rm32_r32()	// Opcode 0x01
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = ADD32(dst, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = RM32(ea);
		dst = ADD32(dst, src);
		WM32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::add_r32_rm32()	// Opcode 0x03
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		dst = ADD32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM32(ea);
		dst = LOAD_REG32(modrm);
		dst = ADD32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::add_eax_i32()	// Opcode 0x05
{
	uint32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	dst = ADD32(dst, src);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::and_rm32_r32()	// Opcode 0x21
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = AND32(dst, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = RM32(ea);
		dst = AND32(dst, src);
		WM32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::and_r32_rm32()	// Opcode 0x23
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		dst = AND32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM32(ea);
		dst = LOAD_REG32(modrm);
		dst = AND32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::and_eax_i32()	// Opcode 0x25
{
	uint32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	dst = AND32(dst, src);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::bsf_r32_rm32()	// Opcode 0x0f bc
{
	uint32 src, dst, temp;
	uint8 modrm = FETCH8();

	if(modrm >= 0xc0) {
		src = LOAD_RM32(modrm);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM32(ea);
	}

	dst = 0;

	if(src == 0) {
		ZF = 1;
	}
	else {
		ZF = 0;
		temp = 0;
		while((src & (1 << temp)) == 0) {
			temp++;
			dst = temp;
			CYCLES(CYCLES_BSF);
		}
	}
	CYCLES(CYCLES_BSF_BASE);
	STORE_REG32(modrm, dst);
}

void I386::bsr_r32_rm32()	// Opcode 0x0f bd
{
	uint32 src, dst, temp;
	uint8 modrm = FETCH8();

	if(modrm >= 0xc0) {
		src = LOAD_RM32(modrm);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM32(ea);
	}

	dst = 0;

	if(src == 0) {
		ZF = 1;
	}
	else {
		ZF = 0;
		dst = temp = 31;
		while((src & (1 << temp)) == 0) {
			temp--;
			dst = temp;
			CYCLES(CYCLES_BSR);
		}
	}
	CYCLES(CYCLES_BSR_BASE);
	STORE_REG32(modrm, dst);
}

void I386::bt_rm32_r32()	// Opcode 0x0f a3
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 dst = LOAD_RM32(modrm);
		uint32 bit = LOAD_REG32(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;

		CYCLES(CYCLES_BT_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 dst = RM32(ea);
		uint32 bit = LOAD_REG32(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;

		CYCLES(CYCLES_BT_REG_MEM);
	}
}

void I386::btc_rm32_r32()	// Opcode 0x0f bb
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 dst = LOAD_RM32(modrm);
		uint32 bit = LOAD_REG32(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;
		dst ^= (1 << bit);

		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_BTC_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 dst = RM32(ea);
		uint32 bit = LOAD_REG32(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;
		dst ^= (1 << bit);

		WM32(ea, dst);
		CYCLES(CYCLES_BTC_REG_MEM);
	}
}

void I386::btr_rm32_r32()	// Opcode 0x0f b3
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 dst = LOAD_RM32(modrm);
		uint32 bit = LOAD_REG32(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;
		dst &= ~(1 << bit);

		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_BTR_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 dst = RM32(ea);
		uint32 bit = LOAD_REG32(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;
		dst &= ~(1 << bit);

		WM32(ea, dst);
		CYCLES(CYCLES_BTR_REG_MEM);
	}
}

void I386::bts_rm32_r32()	// Opcode 0x0f ab
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 dst = LOAD_RM32(modrm);
		uint32 bit = LOAD_REG32(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;
		dst |= (1 << bit);

		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_BTS_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 dst = RM32(ea);
		uint32 bit = LOAD_REG32(modrm);

		if(dst & (1 << bit))
			CF = 1;
		else
			CF = 0;
		dst |= (1 << bit);

		WM32(ea, dst);
		CYCLES(CYCLES_BTS_REG_MEM);
	}
}

void I386::call_abs32()	// Opcode 0x9a
{
	uint32 offset = FETCH32();
	uint16 ptr = FETCH16();

	if(PROTECTED_MODE) {
		/* TODO */
//		fatalerror(_T("i386: call_abs32 in protected mode unimplemented");
	}
	else {
		PUSH32(sreg[CS].selector);
		PUSH32(eip);
		sreg[CS].selector = ptr;
		eip = offset;
		load_segment_descriptor(CS);
	}
	CYCLES(CYCLES_CALL_INTERSEG);
	CHANGE_PC(eip);
#ifdef I386_BIOS_CALL
	BIOS_CALL_FAR32();
#endif
}

void I386::call_rel32()	// Opcode 0xe8
{
	int32 disp = FETCH32();

	PUSH32(eip);
	eip += disp;
	CHANGE_PC(eip);
	CYCLES(CYCLES_CALL);		/* TODO: Timing = 7 + m */
#ifdef I386_BIOS_CALL
	BIOS_CALL_NEAR32();
#endif
}

void I386::cdq()	// Opcode 0x99
{
	if(REG32(EAX) & 0x80000000) {
		REG32(EDX) = 0xffffffff;
	}
	else {
		REG32(EDX) = 0x00000000;
	}
	CYCLES(CYCLES_CWD);
}

void I386::cmp_rm32_r32()	// Opcode 0x39
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		SUB32(dst, src);
		CYCLES(CYCLES_CMP_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = RM32(ea);
		SUB32(dst, src);
		CYCLES(CYCLES_CMP_REG_MEM);
	}
}

void I386::cmp_r32_rm32()	// Opcode 0x3b
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		SUB32(dst, src);
		CYCLES(CYCLES_CMP_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM32(ea);
		dst = LOAD_REG32(modrm);
		SUB32(dst, src);
		CYCLES(CYCLES_CMP_MEM_REG);
	}
}

void I386::cmp_eax_i32()	// Opcode 0x3d
{
	uint32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	SUB32(dst, src);
	CYCLES(CYCLES_CMP_IMM_ACC);
}

void I386::cmpsd()	// Opcode 0xa7
{
	uint32 eas, ead, src, dst;
	if(segment_prefix) {
		eas = translate(segment_override, address_size ? REG32(ESI) : REG16(SI));
	}
	else {
		eas = translate(DS, address_size ? REG32(ESI) : REG16(SI));
	}
	ead = translate(ES, address_size ? REG32(EDI) : REG16(DI));
	src = RM32(eas);
	dst = RM32(ead);
	SUB32(dst, src);
	BUMP_SI(4);
	BUMP_DI(4);
	CYCLES(CYCLES_CMPS);
}

void I386::cwde()	// Opcode 0x98
{
	REG32(EAX) = (int32)((int16)REG16(AX));
	CYCLES(CYCLES_CBW);
}

void I386::dec_eax()	// Opcode 0x48
{
	REG32(EAX) = DEC32(REG32(EAX));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_ecx()	// Opcode 0x49
{
	REG32(ECX) = DEC32(REG32(ECX));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_edx()	// Opcode 0x4a
{
	REG32(EDX) = DEC32(REG32(EDX));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_ebx()	// Opcode 0x4b
{
	REG32(EBX) = DEC32(REG32(EBX));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_esp()	// Opcode 0x4c
{
	REG32(ESP) = DEC32(REG32(ESP));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_ebp()	// Opcode 0x4d
{
	REG32(EBP) = DEC32(REG32(EBP));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_esi()	// Opcode 0x4e
{
	REG32(ESI) = DEC32(REG32(ESI));
	CYCLES(CYCLES_DEC_REG);
}

void I386::dec_edi()	// Opcode 0x4f
{
	REG32(EDI) = DEC32(REG32(EDI));
	CYCLES(CYCLES_DEC_REG);
}

void I386::imul_r32_rm32()	// Opcode 0x0f af
{
	uint8 modrm = FETCH8();
	int64 result;
	int64 src, dst;
	if(modrm >= 0xc0) {
		src = (int64)(int32)LOAD_RM32(modrm);
		CYCLES(CYCLES_IMUL32_REG_REG);		/* TODO: Correct multiply timing */
	}
	else {
		uint32 ea = GetEA(modrm);
		src = (int64)(int32)RM32(ea);
		CYCLES(CYCLES_IMUL32_REG_REG);		/* TODO: Correct multiply timing */
	}

	dst = (int64)(int32)LOAD_REG32(modrm);
	result = src * dst;

	STORE_REG32(modrm, (uint32)result);

	CF = OF = !(result == (int64)(int32)result);
}

void I386::imul_r32_rm32_i32()	// Opcode 0x69
{
	uint8 modrm = FETCH8();
	int64 result;
	int64 src, dst;
	if(modrm >= 0xc0) {
		dst = (int64)(int32)LOAD_RM32(modrm);
		CYCLES(CYCLES_IMUL32_REG_IMM_REG);		/* TODO: Correct multiply timing */
	}
	else {
		uint32 ea = GetEA(modrm);
		dst = (int64)(int32)RM32(ea);
		CYCLES(CYCLES_IMUL32_MEM_IMM_REG);		/* TODO: Correct multiply timing */
	}

	src = (int64)(int32)FETCH32();
	result = src * dst;

	STORE_REG32(modrm, (uint32)result);

	CF = OF = !(result == (int64)(int32)result);
}

void I386::imul_r32_rm32_i8()	// Opcode 0x6b
{
	uint8 modrm = FETCH8();
	int64 result;
	int64 src, dst;
	if(modrm >= 0xc0) {
		dst = (int64)(int32)LOAD_RM32(modrm);
		CYCLES(CYCLES_IMUL32_REG_IMM_REG);		/* TODO: Correct multiply timing */
	}
	else {
		uint32 ea = GetEA(modrm);
		dst = (int64)(int32)RM32(ea);
		CYCLES(CYCLES_IMUL32_MEM_IMM_REG);		/* TODO: Correct multiply timing */
	}

	src = (int64)(int8)FETCH8();
	result = src * dst;

	STORE_REG32(modrm, (uint32)result);

	CF = OF = !(result == (int64)(int32)result);
}

void I386::in_eax_i8()	// Opcode 0xe5
{
	uint16 port = FETCH8();
	uint32 data = IN32(port);
	REG32(EAX) = data;
	CYCLES(CYCLES_IN_VAR);
}

void I386::in_eax_dx()	// Opcode 0xed
{
	uint16 port = REG16(DX);
	uint32 data = IN32(port);
	REG32(EAX) = data;
	CYCLES(CYCLES_IN);
}

void I386::inc_eax()	// Opcode 0x40
{
	REG32(EAX) = INC32(REG32(EAX));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_ecx()	// Opcode 0x41
{
	REG32(ECX) = INC32(REG32(ECX));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_edx()	// Opcode 0x42
{
	REG32(EDX) = INC32(REG32(EDX));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_ebx()	// Opcode 0x43
{
	REG32(EBX) = INC32(REG32(EBX));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_esp()	// Opcode 0x44
{
	REG32(ESP) = INC32(REG32(ESP));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_ebp()	// Opcode 0x45
{
	REG32(EBP) = INC32(REG32(EBP));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_esi()	// Opcode 0x46
{
	REG32(ESI) = INC32(REG32(ESI));
	CYCLES(CYCLES_INC_REG);
}

void I386::inc_edi()	// Opcode 0x47
{
	REG32(EDI) = INC32(REG32(EDI));
	CYCLES(CYCLES_INC_REG);
}

void I386::iret32()	// Opcode 0xcf
{
	if(PROTECTED_MODE) {
		/* TODO: Virtual 8086-mode */
		/* TODO: Nested task */
		/* TODO: #SS(0) exception */
		/* TODO: All the protection-related stuff... */
		eip = POP32();
		sreg[CS].selector = POP32() & 0xffff;
		set_flags(POP32());
		load_segment_descriptor(CS);
		CHANGE_PC(eip);
	}
	else {
		/* TODO: #SS(0) exception */
		/* TODO: #GP(0) exception */
		eip = POP32();
		sreg[CS].selector = POP32() & 0xffff;
		set_flags(POP32());
		load_segment_descriptor(CS);
		CHANGE_PC(eip);
	}
	CYCLES(CYCLES_IRET);
}

void I386::ja_rel32()	// Opcode 0x0f 87
{
	int32 disp = FETCH32();
	if(CF == 0 && ZF == 0) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jbe_rel32()	// Opcode 0x0f 86
{
	int32 disp = FETCH32();
	if(CF != 0 || ZF != 0) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jc_rel32()	// Opcode 0x0f 82
{
	int32 disp = FETCH32();
	if(CF != 0) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jg_rel32()	// Opcode 0x0f 8f
{
	int32 disp = FETCH32();
	if(ZF == 0 && (SF == OF)) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jge_rel32()	// Opcode 0x0f 8d
{
	int32 disp = FETCH32();
	if((SF == OF)) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jl_rel32()	// Opcode 0x0f 8c
{
	int32 disp = FETCH32();
	if((SF != OF)) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jle_rel32()	// Opcode 0x0f 8e
{
	int32 disp = FETCH32();
	if(ZF != 0 || (SF != OF)) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jnc_rel32()	// Opcode 0x0f 83
{
	int32 disp = FETCH32();
	if(CF == 0) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jno_rel32()	// Opcode 0x0f 81
{
	int32 disp = FETCH32();
	if(OF == 0) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jnp_rel32()	// Opcode 0x0f 8b
{
	int32 disp = FETCH32();
	if(PF == 0) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jns_rel32()	// Opcode 0x0f 89
{
	int32 disp = FETCH32();
	if(SF == 0) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jnz_rel32()	// Opcode 0x0f 85
{
	int32 disp = FETCH32();
	if(ZF == 0) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jo_rel32()	// Opcode 0x0f 80
{
	int32 disp = FETCH32();
	if(OF != 0) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jp_rel32()	// Opcode 0x0f 8a
{
	int32 disp = FETCH32();
	if(PF != 0) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::js_rel32()	// Opcode 0x0f 88
{
	int32 disp = FETCH32();
	if(SF != 0) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jz_rel32()	// Opcode 0x0f 84
{
	int32 disp = FETCH32();
	if(ZF != 0) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCC_FULL_DISP);		/* TODO: Timing = 7 + m */
	}
	else {
		CYCLES(CYCLES_JCC_FULL_DISP_NOBRANCH);
	}
}

void I386::jcxz32()	// Opcode 0xe3
{
	int8 disp = FETCH8();
	if(REG32(ECX) == 0) {
		eip += disp;
		CHANGE_PC(eip);
		CYCLES(CYCLES_JCXZ);		/* TODO: Timing = 9 + m */
	}
	else {
		CYCLES(CYCLES_JCXZ_NOBRANCH);
	}
}

void I386::jmp_rel32()	// Opcode 0xe9
{
	uint32 disp = FETCH32();
	/* TODO: Segment limit */
	eip += disp;
	CHANGE_PC(eip);
	CYCLES(CYCLES_JMP);		/* TODO: Timing = 7 + m */
}

void I386::jmp_abs32()	// Opcode 0xea
{
	uint32 address = FETCH32();
	uint16 segment = FETCH16();

	if(PROTECTED_MODE) {
		/* TODO: #GP */
		/* TODO: access rights, etc. */
		eip = address;
		sreg[CS].selector = segment;
		load_segment_descriptor(CS);
		CHANGE_PC(eip);
	}
	else {
		eip = address;
		sreg[CS].selector = segment;
		load_segment_descriptor(CS);
		CHANGE_PC(eip);
	}
	CYCLES(CYCLES_JMP_INTERSEG);
}

void I386::lea32()	// Opcode 0x8d
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0)
		trap(ILLEGAL_INSTRUCTION, 0);
	else {
		uint32 ea = GetNonTranslatedEA(modrm);
		if(!address_size)
			ea &= 0xffff;
		STORE_REG32(modrm, ea);
		CYCLES(CYCLES_LEA);
	}
}

void I386::leave32()	// Opcode 0xc9
{
	REG32(ESP) = REG32(EBP);
	REG32(EBP) = POP32();
	CYCLES(CYCLES_LEAVE);
}

void I386::lodsd()	// Opcode 0xad
{
	uint32 eas;
	if(segment_prefix)
		eas = translate(segment_override, address_size ? REG32(ESI) : REG16(SI));
	else
		eas = translate(DS, address_size ? REG32(ESI) : REG16(SI));
	REG32(EAX) = RM32(eas);
	BUMP_SI(4);
	CYCLES(CYCLES_LODS);
}

void I386::loop32()	// Opcode 0xe2
{
	int8 disp = FETCH8();
	REG32(ECX)--;
	if(REG32(ECX) != 0) {
		eip += disp;
		CHANGE_PC(eip);
	}
	CYCLES(CYCLES_LOOP);		/* TODO: Timing = 11 + m */
}

void I386::loopne32()	// Opcode 0xe0
{
	int8 disp = FETCH8();
	REG32(ECX)--;
	if(REG32(ECX) != 0 && ZF == 0) {
		eip += disp;
		CHANGE_PC(eip);
	}
	CYCLES(CYCLES_LOOPNZ);		/* TODO: Timing = 11 + m */
}

void I386::loopz32()	// Opcode 0xe1
{
	int8 disp = FETCH8();
	REG32(ECX)--;
	if(REG32(ECX) != 0 && ZF != 0) {
		eip += disp;
		CHANGE_PC(eip);
	}
	CYCLES(CYCLES_LOOPZ);		/* TODO: Timing = 11 + m */
}

void I386::mov_rm32_r32()	// Opcode 0x89
{
	uint32 src;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG32(modrm);
		STORE_RM32(modrm, src);
		CYCLES(CYCLES_MOV_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		WM32(ea, src);
		CYCLES(CYCLES_MOV_REG_MEM);
	}
}

void I386::mov_r32_rm32()	// Opcode 0x8b
{
	uint32 src;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM32(modrm);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOV_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM32(ea);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOV_MEM_REG);
	}
}

void I386::mov_rm32_i32()	// Opcode 0xc7
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 val = FETCH32();
		STORE_RM32(modrm, val);
		CYCLES(CYCLES_MOV_IMM_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 val = FETCH32();
		WM32(ea, val);
		CYCLES(CYCLES_MOV_IMM_MEM);
	}
}

void I386::mov_eax_m32()	// Opcode 0xa1
{
	uint32 offset, ea;
	if(address_size) {
		offset = FETCH32();
	}
	else {
		offset = FETCH16();
	}
	if(segment_prefix) {
		ea = translate(segment_override, offset);
	}
	else {
		ea = translate(DS, offset);
	}
	REG32(EAX) = RM32(ea);
	CYCLES(CYCLES_MOV_MEM_ACC);
}

void I386::mov_m32_eax()	// Opcode 0xa3
{
	uint32 offset, ea;
	if(address_size) {
		offset = FETCH32();
	}
	else {
		offset = FETCH16();
	}
	if(segment_prefix) {
		ea = translate(segment_override, offset);
	}
	else {
		ea = translate(DS, offset);
	}
	WM32(ea, REG32(EAX));
	CYCLES(CYCLES_MOV_ACC_MEM);
}

void I386::mov_eax_i32()	// Opcode 0xb8
{
	REG32(EAX) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_ecx_i32()	// Opcode 0xb9
{
	REG32(ECX) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_edx_i32()	// Opcode 0xba
{
	REG32(EDX) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_ebx_i32()	// Opcode 0xbb
{
	REG32(EBX) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_esp_i32()	// Opcode 0xbc
{
	REG32(ESP) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_ebp_i32()	// Opcode 0xbd
{
	REG32(EBP) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_esi_i32()	// Opcode 0xbe
{
	REG32(ESI) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::mov_edi_i32()	// Opcode 0xbf
{
	REG32(EDI) = FETCH32();
	CYCLES(CYCLES_MOV_IMM_REG);
}

void I386::movsd()	// Opcode 0xa5
{
	uint32 eas, ead, v;
	if(segment_prefix) {
		eas = translate(segment_override, address_size ? REG32(ESI) : REG16(SI));
	}
	else {
		eas = translate(DS, address_size ? REG32(ESI) : REG16(SI));
	}
	ead = translate(ES, address_size ? REG32(EDI) : REG16(DI));
	v = RM32(eas);
	WM32(ead, v);
	BUMP_SI(4);
	BUMP_DI(4);
	CYCLES(CYCLES_MOVS);
}

void I386::movsx_r32_rm8()	// Opcode 0x0f be
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		int32 src = (int8)LOAD_RM8(modrm);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVSX_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		int32 src = (int8)RM8(ea);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVSX_MEM_REG);
	}
}

void I386::movsx_r32_rm16()	// Opcode 0x0f bf
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		int32 src = (int16)LOAD_RM16(modrm);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVSX_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		int32 src = (int16)RM16(ea);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVSX_MEM_REG);
	}
}

void I386::movzx_r32_rm8()	// Opcode 0x0f b6
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 src = (uint8)LOAD_RM8(modrm);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVZX_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 src = (uint8)RM8(ea);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVZX_MEM_REG);
	}
}

void I386::movzx_r32_rm16()	// Opcode 0x0f b7
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 src = (uint16)LOAD_RM16(modrm);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVZX_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 src = (uint16)RM16(ea);
		STORE_REG32(modrm, src);
		CYCLES(CYCLES_MOVZX_MEM_REG);
	}
}

void I386::or_rm32_r32()	// Opcode 0x09
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = OR32(dst, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = RM32(ea);
		dst = OR32(dst, src);
		WM32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::or_r32_rm32()	// Opcode 0x0b
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		dst = OR32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM32(ea);
		dst = LOAD_REG32(modrm);
		dst = OR32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::or_eax_i32()	// Opcode 0x0d
{
	uint32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	dst = OR32(dst, src);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::out_eax_i8()	// Opcode 0xe7
{
	uint16 port = FETCH8();
	uint32 data = REG32(EAX);
	OUT32(port, data);
	CYCLES(CYCLES_OUT_VAR);
}

void I386::out_eax_dx()	// Opcode 0xef
{
	uint16 port = REG16(DX);
	uint32 data = REG32(EAX);
	OUT32(port, data);
	CYCLES(CYCLES_OUT);
}

void I386::pop_eax()	// Opcode 0x58
{
	REG32(EAX) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_ecx()	// Opcode 0x59
{
	REG32(ECX) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_edx()	// Opcode 0x5a
{
	REG32(EDX) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_ebx()	// Opcode 0x5b
{
	REG32(EBX) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_esp()	// Opcode 0x5c
{
	REG32(ESP) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_ebp()	// Opcode 0x5d
{
	REG32(EBP) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_esi()	// Opcode 0x5e
{
	REG32(ESI) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_edi()	// Opcode 0x5f
{
	REG32(EDI) = POP32();
	CYCLES(CYCLES_POP_REG_SHORT);
}

void I386::pop_ds32()	// Opcode 0x1f
{
	sreg[DS].selector = POP32();
	if(PROTECTED_MODE) {
		load_segment_descriptor(DS);
	}
	else {
		load_segment_descriptor(DS);
	}
	CYCLES(CYCLES_POP_SREG);
}

void I386::pop_es32()	// Opcode 0x07
{
	sreg[ES].selector = POP32();
	if(PROTECTED_MODE) {
		load_segment_descriptor(ES);
	}
	else {
		load_segment_descriptor(ES);
	}
	CYCLES(CYCLES_POP_SREG);
}

void I386::pop_fs32()	// Opcode 0x0f a1
{
	sreg[FS].selector = POP32();
	if(PROTECTED_MODE) {
		load_segment_descriptor(FS);
	}
	else {
		load_segment_descriptor(FS);
	}
	CYCLES(CYCLES_POP_SREG);
}

void I386::pop_gs32()	// Opcode 0x0f a9
{
	sreg[GS].selector = POP32();
	if(PROTECTED_MODE) {
		load_segment_descriptor(GS);
	}
	else {
		load_segment_descriptor(GS);
	}
	CYCLES(CYCLES_POP_SREG);
}

void I386::pop_ss32()	// Opcode 0x17
{
	sreg[SS].selector = POP32();
	if(PROTECTED_MODE) {
		load_segment_descriptor(SS);
	}
	else {
		load_segment_descriptor(SS);
	}
	CYCLES(CYCLES_POP_SREG);
}

void I386::pop_rm32()	// Opcode 0x8f
{
	uint8 modrm = FETCH8();
	uint32 val = POP32();

	if(modrm >= 0xc0) {
		STORE_RM32(modrm, val);
	}
	else {
		uint32 ea = GetEA(modrm);
		WM32(ea, val);
	}
	CYCLES(CYCLES_POP_RM);
}

void I386::popad()	// Opcode 0x61
{
	REG32(EDI) = POP32();
	REG32(ESI) = POP32();
	REG32(EBP) = POP32();
	REG32(ESP) += 4;
	REG32(EBX) = POP32();
	REG32(EDX) = POP32();
	REG32(ECX) = POP32();
	REG32(EAX) = POP32();
	CYCLES(CYCLES_POPA);
}

void I386::popfd()	// Opcode 0x9d
{
	uint32 val = POP32();
	set_flags(val);
	CYCLES(CYCLES_POPF);
}

void I386::push_eax()	// Opcode 0x50
{
	PUSH32(REG32(EAX));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_ecx()	// Opcode 0x51
{
	PUSH32(REG32(ECX));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_edx()	// Opcode 0x52
{
	PUSH32(REG32(EDX));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_ebx()	// Opcode 0x53
{
	PUSH32(REG32(EBX));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_esp()	// Opcode 0x54
{
	PUSH32(REG32(ESP));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_ebp()	// Opcode 0x55
{
	PUSH32(REG32(EBP));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_esi()	// Opcode 0x56
{
	PUSH32(REG32(ESI));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_edi()	// Opcode 0x57
{
	PUSH32(REG32(EDI));
	CYCLES(CYCLES_PUSH_REG_SHORT);
}

void I386::push_cs32()	// Opcode 0x0e
{
	PUSH32(sreg[CS].selector);
	CYCLES(CYCLES_PUSH_SREG);
}

void I386::push_ds32()	// Opcode 0x1e
{
	PUSH32(sreg[DS].selector);
	CYCLES(CYCLES_PUSH_SREG);
}

void I386::push_es32()	// Opcode 0x06
{
	PUSH32(sreg[ES].selector);
	CYCLES(CYCLES_PUSH_SREG);
}

void I386::push_fs32()	// Opcode 0x0f a0
{
	PUSH32(sreg[FS].selector);
	CYCLES(CYCLES_PUSH_SREG);
}

void I386::push_gs32()	// Opcode 0x0f a8
{
	PUSH32(sreg[GS].selector);
	CYCLES(CYCLES_PUSH_SREG);
}

void I386::push_ss32()	// Opcode 0x16
{
	PUSH32(sreg[SS].selector);
	CYCLES(CYCLES_PUSH_SREG);
}

void I386::push_i32()	// Opcode 0x68
{
	uint32 val = FETCH32();
	PUSH32(val);
	CYCLES(CYCLES_PUSH_IMM);
}

void I386::pushad()	// Opcode 0x60
{
	uint32 temp = REG32(ESP);
	PUSH32(REG32(EAX));
	PUSH32(REG32(ECX));
	PUSH32(REG32(EDX));
	PUSH32(REG32(EBX));
	PUSH32(temp);
	PUSH32(REG32(EBP));
	PUSH32(REG32(ESI));
	PUSH32(REG32(EDI));
	CYCLES(CYCLES_PUSHA);
}

void I386::pushfd()	// Opcode 0x9c
{
	PUSH32(get_flags() & 0x00fcffff);
	CYCLES(CYCLES_PUSHF);
}

void I386::ret_near32_i16()	// Opcode 0xc2
{
	int16 disp = FETCH16();
	eip = POP32();
	REG32(ESP) += disp;
	CHANGE_PC(eip);
	CYCLES(CYCLES_RET_IMM);		/* TODO: Timing = 10 + m */
}

void I386::ret_near32()	// Opcode 0xc3
{
	eip = POP32();
	CHANGE_PC(eip);
	CYCLES(CYCLES_RET);		/* TODO: Timing = 10 + m */
}

void I386::sbb_rm32_r32()	// Opcode 0x19
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG32(modrm) + CF;
		dst = LOAD_RM32(modrm);
		dst = SUB32(dst, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm) + CF;
		dst = RM32(ea);
		dst = SUB32(dst, src);
		WM32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::sbb_r32_rm32()	// Opcode 0x1b
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM32(modrm) + CF;
		dst = LOAD_REG32(modrm);
		dst = SUB32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM32(ea) + CF;
		dst = LOAD_REG32(modrm);
		dst = SUB32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::sbb_eax_i32()	// Opcode 0x1d
{
	uint32 src, dst;
	src = FETCH32() + CF;
	dst = REG32(EAX);
	dst = SUB32(dst, src);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::scasd()	// Opcode 0xaf
{
	uint32 eas, src, dst;
	eas = translate(ES, address_size ? REG32(EDI) : REG16(DI));
	src = RM32(eas);
	dst = REG32(EAX);
	SUB32(dst, src);
	BUMP_DI(4);
	CYCLES(CYCLES_SCAS);
}

void I386::shld32_i8()	// Opcode 0x0f a4
{
	/* TODO: Correct flags */
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 dst = LOAD_RM32(modrm);
		uint32 upper = LOAD_REG32(modrm);
		uint8 shift = FETCH8();
		if(shift > 31 || shift == 0) {

		}
		else {
			CF = (dst & (1 << (32-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (32-shift));
			SetSZPF32(dst);
		}
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_SHLD_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 dst = RM32(ea);
		uint32 upper = LOAD_REG32(modrm);
		uint8 shift = FETCH8();
		if(shift > 31 || shift == 0) {

		}
		else {
			CF = (dst & (1 << (32-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (32-shift));
			SetSZPF32(dst);
		}
		WM32(ea, dst);
		CYCLES(CYCLES_SHLD_MEM);
	}
}

void I386::shld32_cl()	// Opcode 0x0f a5
{
	/* TODO: Correct flags */
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 dst = LOAD_RM32(modrm);
		uint32 upper = LOAD_REG32(modrm);
		uint8 shift = REG8(CL);
		if(shift > 31 || shift == 0) {

		}
		else {
			CF = (dst & (1 << (32-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (32-shift));
			SetSZPF32(dst);
		}
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_SHLD_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 dst = RM32(ea);
		uint32 upper = LOAD_REG32(modrm);
		uint8 shift = REG8(CL);
		if(shift > 31 || shift == 0) {

		}
		else {
			CF = (dst & (1 << (32-shift))) ? 1 : 0;
			dst = (dst << shift) | (upper >> (32-shift));
			SetSZPF32(dst);
		}
		WM32(ea, dst);
		CYCLES(CYCLES_SHLD_MEM);
	}
}

void I386::shrd32_i8()	// Opcode 0x0f ac
{
	/* TODO: Correct flags */
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 dst = LOAD_RM32(modrm);
		uint32 upper = LOAD_REG32(modrm);
		uint8 shift = FETCH8();
		if(shift > 31 || shift == 0) {

		}
		else {
			CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (32-shift));
			SetSZPF32(dst);
		}
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_SHRD_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 dst = RM32(ea);
		uint32 upper = LOAD_REG32(modrm);
		uint8 shift = FETCH8();
		if(shift > 31 || shift == 0) {

		}
		else {
			CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (32-shift));
			SetSZPF32(dst);
		}
		WM32(ea, dst);
		CYCLES(CYCLES_SHRD_MEM);
	}
}

void I386::shrd32_cl()	// Opcode 0x0f ad
{
	/* TODO: Correct flags */
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 dst = LOAD_RM32(modrm);
		uint32 upper = LOAD_REG32(modrm);
		uint8 shift = REG8(CL);
		if(shift > 31 || shift == 0) {

		}
		else {
			CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (32-shift));
			SetSZPF32(dst);
		}
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_SHRD_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 dst = RM32(ea);
		uint32 upper = LOAD_REG32(modrm);
		uint8 shift = REG8(CL);
		if(shift > 31 || shift == 0) {

		}
		else {
			CF = (dst & (1 << (shift-1))) ? 1 : 0;
			dst = (dst >> shift) | (upper << (32-shift));
			SetSZPF32(dst);
		}
		WM32(ea, dst);
		CYCLES(CYCLES_SHRD_MEM);
	}
}

void I386::stosd()	// Opcode 0xab
{
	uint32 eas = translate(ES, address_size ? REG32(EDI) : REG16(DI));
	WM32(eas, REG32(EAX));
	BUMP_DI(4);
	CYCLES(CYCLES_STOS);
}

void I386::sub_rm32_r32()	// Opcode 0x29
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = SUB32(dst, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = RM32(ea);
		dst = SUB32(dst, src);
		WM32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::sub_r32_rm32()	// Opcode 0x2b
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		dst = SUB32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM32(ea);
		dst = LOAD_REG32(modrm);
		dst = SUB32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::sub_eax_i32()	// Opcode 0x2d
{
	uint32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	dst = SUB32(dst, src);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}

void I386::test_eax_i32()	// Opcode 0xa9
{
	uint32 src = FETCH32();
	uint32 dst = REG32(EAX);
	dst = src & dst;
	SetSZPF32(dst);
	CF = 0;
	OF = 0;
	CYCLES(CYCLES_TEST_IMM_ACC);
}

void I386::test_rm32_r32()	// Opcode 0x85
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = src & dst;
		SetSZPF32(dst);
		CF = 0;
		OF = 0;
		CYCLES(CYCLES_TEST_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = RM32(ea);
		dst = src & dst;
		SetSZPF32(dst);
		CF = 0;
		OF = 0;
		CYCLES(CYCLES_TEST_REG_MEM);
	}
}

void I386::xchg_eax_ecx()	// Opcode 0x91
{
	uint32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(ECX);
	REG32(ECX) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_eax_edx()	// Opcode 0x92
{
	uint32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(EDX);
	REG32(EDX) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_eax_ebx()	// Opcode 0x93
{
	uint32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(EBX);
	REG32(EBX) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_eax_esp()	// Opcode 0x94
{
	uint32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(ESP);
	REG32(ESP) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_eax_ebp()	// Opcode 0x95
{
	uint32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(EBP);
	REG32(EBP) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_eax_esi()	// Opcode 0x96
{
	uint32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(ESI);
	REG32(ESI) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_eax_edi()	// Opcode 0x97
{
	uint32 temp;
	temp = REG32(EAX);
	REG32(EAX) = REG32(EDI);
	REG32(EDI) = temp;
	CYCLES(CYCLES_XCHG_REG_REG);
}

void I386::xchg_r32_rm32()	// Opcode 0x87
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 src = LOAD_RM32(modrm);
		uint32 dst = LOAD_REG32(modrm);
		STORE_REG32(modrm, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_XCHG_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 src = RM32(ea);
		uint32 dst = LOAD_REG32(modrm);
		STORE_REG32(modrm, src);
		WM32(ea, dst);
		CYCLES(CYCLES_XCHG_REG_MEM);
	}
}

void I386::xor_rm32_r32()	// Opcode 0x31
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_REG32(modrm);
		dst = LOAD_RM32(modrm);
		dst = XOR32(dst, src);
		STORE_RM32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = LOAD_REG32(modrm);
		dst = RM32(ea);
		dst = XOR32(dst, src);
		WM32(ea, dst);
		CYCLES(CYCLES_ALU_REG_MEM);
	}
}

void I386::xor_r32_rm32()	// Opcode 0x33
{
	uint32 src, dst;
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		src = LOAD_RM32(modrm);
		dst = LOAD_REG32(modrm);
		dst = XOR32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		src = RM32(ea);
		dst = LOAD_REG32(modrm);
		dst = XOR32(dst, src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_ALU_MEM_REG);
	}
}

void I386::xor_eax_i32()	// Opcode 0x35
{
	uint32 src, dst;
	src = FETCH32();
	dst = REG32(EAX);
	dst = XOR32(dst, src);
	REG32(EAX) = dst;
	CYCLES(CYCLES_ALU_IMM_ACC);
}



void I386::group81_32()	// Opcode 0x81
{
	uint32 ea;
	uint32 src, dst;
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 0:		// ADD Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				dst = ADD32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = FETCH32();
				dst = ADD32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 1:		// OR Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				dst = OR32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = FETCH32();
				dst = OR32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 2:		// ADC Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				src = ADD32(src, CF);
				dst = ADD32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = FETCH32();
				src = ADD32(src, CF);
				dst = ADD32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 3:		// SBB Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = FETCH32() + CF;
				dst = SUB32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = FETCH32() + CF;
				dst = SUB32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 4:		// AND Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				dst = AND32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = FETCH32();
				dst = AND32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 5:		// SUB Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				dst = SUB32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = FETCH32();
				dst = SUB32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 6:		// XOR Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				dst = XOR32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = FETCH32();
				dst = XOR32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 7:		// CMP Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = FETCH32();
				SUB32(dst, src);
				CYCLES(CYCLES_CMP_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = FETCH32();
				SUB32(dst, src);
				CYCLES(CYCLES_CMP_REG_MEM);
			}
			break;
	}
}

void I386::group83_32()	// Opcode 0x83
{
	uint32 ea;
	uint32 src, dst;
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 0:		// ADD Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = (uint32)(int32)(int8)FETCH8();
				dst = ADD32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = (uint32)(int32)(int8)FETCH8();
				dst = ADD32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 1:		// OR Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = (uint32)(int32)(int8)FETCH8();
				dst = OR32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = (uint32)(int32)(int8)FETCH8();
				dst = OR32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 2:		// ADC Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = (uint32)(int32)(int8)FETCH8();
				src = ADD32(src, CF);
				dst = ADD32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = (uint32)(int32)(int8)FETCH8();
				src = ADD32(src, CF);
				dst = ADD32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 3:		// SBB Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = ((uint32)(int32)(int8)FETCH8()) + CF;
				dst = SUB32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = ((uint32)(int32)(int8)FETCH8()) + CF;
				dst = SUB32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 4:		// AND Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = (uint32)(int32)(int8)FETCH8();
				dst = AND32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = (uint32)(int32)(int8)FETCH8();
				dst = AND32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 5:		// SUB Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = (uint32)(int32)(int8)FETCH8();
				dst = SUB32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = (uint32)(int32)(int8)FETCH8();
				dst = SUB32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 6:		// XOR Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = (uint32)(int32)(int8)FETCH8();
				dst = XOR32(dst, src);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_ALU_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = (uint32)(int32)(int8)FETCH8();
				dst = XOR32(dst, src);
				WM32(ea, dst);
				CYCLES(CYCLES_ALU_REG_MEM);
			}
			break;
		case 7:		// CMP Rm32, i32
			if(modrm >= 0xc0) {
				dst = LOAD_RM32(modrm);
				src = (uint32)(int32)(int8)FETCH8();
				SUB32(dst, src);
				CYCLES(CYCLES_CMP_REG_REG);
			}
			else {
				ea = GetEA(modrm);
				dst = RM32(ea);
				src = (uint32)(int32)(int8)FETCH8();
				SUB32(dst, src);
				CYCLES(CYCLES_CMP_REG_MEM);
			}
			break;
	}
}

void I386::groupC1_32()	// Opcode 0xc1
{
	uint32 dst;
	uint8 modrm = FETCH8();
	uint8 shift;

	if(modrm >= 0xc0) {
		dst = LOAD_RM32(modrm);
		shift = FETCH8() & 0x1f;
		dst = shift_rotate32(modrm, dst, shift);
		STORE_RM32(modrm, dst);
	}
	else {
		uint32 ea = GetEA(modrm);
		dst = RM32(ea);
		shift = FETCH8() & 0x1f;
		dst = shift_rotate32(modrm, dst, shift);
		WM32(ea, dst);
	}
}

void I386::groupD1_32()	// Opcode 0xd1
{
	uint32 dst;
	uint8 modrm = FETCH8();

	if(modrm >= 0xc0) {
		dst = LOAD_RM32(modrm);
		dst = shift_rotate32(modrm, dst, 1);
		STORE_RM32(modrm, dst);
	}
	else {
		uint32 ea = GetEA(modrm);
		dst = RM32(ea);
		dst = shift_rotate32(modrm, dst, 1);
		WM32(ea, dst);
	}
}

void I386::groupD3_32()	// Opcode 0xd3
{
	uint32 dst;
	uint8 modrm = FETCH8();

	if(modrm >= 0xc0) {
		dst = LOAD_RM32(modrm);
		dst = shift_rotate32(modrm, dst, REG8(CL));
		STORE_RM32(modrm, dst);
	}
	else {
		uint32 ea = GetEA(modrm);
		dst = RM32(ea);
		dst = shift_rotate32(modrm, dst, REG8(CL));
		WM32(ea, dst);
	}
}

void I386::groupF7_32()	// Opcode 0xf7
{
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 0:			/* TEST Rm32, i32 */
			if(modrm >= 0xc0) {
				uint32 dst = LOAD_RM32(modrm);
				uint32 src = FETCH32();
				dst &= src;
				CF = OF = AF = 0;
				SetSZPF32(dst);
				CYCLES(CYCLES_TEST_IMM_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint32 dst = RM32(ea);
				uint32 src = FETCH32();
				dst &= src;
				CF = OF = AF = 0;
				SetSZPF32(dst);
				CYCLES(CYCLES_TEST_IMM_MEM);
			}
			break;
		case 2:			/* NOT Rm32 */
			if(modrm >= 0xc0) {
				uint32 dst = LOAD_RM32(modrm);
				dst = ~dst;
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_NOT_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint32 dst = RM32(ea);
				dst = ~dst;
				WM32(ea, dst);
				CYCLES(CYCLES_NOT_MEM);
			}
			break;
		case 3:			/* NEG Rm32 */
			if(modrm >= 0xc0) {
				uint32 dst = LOAD_RM32(modrm);
				dst = SUB32(0, dst);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_NEG_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint32 dst = RM32(ea);
				dst = SUB32(0, dst);
				WM32(ea, dst);
				CYCLES(CYCLES_NEG_MEM);
			}
			break;
		case 4:			/* MUL EAX, Rm32 */
			{
				uint64 result;
				uint32 src, dst;
				if(modrm >= 0xc0) {
					src = LOAD_RM32(modrm);
					CYCLES(CYCLES_MUL32_ACC_REG);		/* TODO: Correct multiply timing */
				}
				else {
					uint32 ea = GetEA(modrm);
					src = RM32(ea);
					CYCLES(CYCLES_MUL32_ACC_MEM);		/* TODO: Correct multiply timing */
				}

				dst = REG32(EAX);
				result = (uint64)src * (uint64)dst;
				REG32(EDX) = (uint32)(result >> 32);
				REG32(EAX) = (uint32)result;

				CF = OF = (REG32(EDX) != 0);
			}
			break;
		case 5:			/* IMUL EAX, Rm32 */
			{
				int64 result;
				int64 src, dst;
				if(modrm >= 0xc0) {
					src = (int64)(int32)LOAD_RM32(modrm);
					CYCLES(CYCLES_IMUL32_ACC_REG);		/* TODO: Correct multiply timing */
				}
				else {
					uint32 ea = GetEA(modrm);
					src = (int64)(int32)RM32(ea);
					CYCLES(CYCLES_IMUL32_ACC_MEM);		/* TODO: Correct multiply timing */
				}

				dst = (int64)(int32)REG32(EAX);
				result = src * dst;

				REG32(EDX) = (uint32)(result >> 32);
				REG32(EAX) = (uint32)result;

				CF = OF = !(result == (int64)(int32)result);
			}
			break;
		case 6:			/* DIV EAX, Rm32 */
			{
				uint32 src;
				if(modrm >= 0xc0) {
					src = LOAD_RM32(modrm);
					CYCLES(CYCLES_DIV32_ACC_REG);
				}
				else {
					uint32 ea = GetEA(modrm);
					src = RM32(ea);
					CYCLES(CYCLES_DIV32_ACC_MEM);
				}
				uint64 quotient = ((uint64)(REG32(EDX)) << 32) | (uint64)(REG32(EAX));
				if(!src)
					trap(DIVIDE_FAULT, 1);
				else {
					uint64 remainder = quotient % (uint64)src;
					uint64 result = quotient / (uint64)src;
					if(result > 0xffffffff)
						trap(DIVIDE_FAULT, 1);
					else {
						REG32(EDX) = (uint32)remainder;
						REG32(EAX) = (uint32)result;
					}
				}
			}
			break;
		case 7:			/* IDIV EAX, Rm32 */
			{
				uint32 src;
				if(modrm >= 0xc0) {
					src = LOAD_RM32(modrm);
					CYCLES(CYCLES_IDIV32_ACC_REG);
				}
				else {
					uint32 ea = GetEA(modrm);
					src = RM32(ea);
					CYCLES(CYCLES_IDIV32_ACC_MEM);
				}
				int64 quotient = (((int64)REG32(EDX)) << 32) | ((uint64)REG32(EAX));
				if(!src)
					trap(DIVIDE_FAULT, 1);
				else {
					int64 remainder = quotient % (int64)(int32)src;
					int64 result = quotient / (int64)(int32)src;
					if(result > 0xffffffff)
						trap(DIVIDE_FAULT, 1);
					else {
						REG32(EDX) = (uint32)remainder;
						REG32(EAX) = (uint32)result;
					}
				}
			}
			break;
	}
}

void I386::groupFF_32()	// Opcode 0xff
{
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 0:			/* INC Rm32 */
			if(modrm >= 0xc0) {
				uint32 dst = LOAD_RM32(modrm);
				dst = INC32(dst);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_INC_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint32 dst = RM32(ea);
				dst = INC32(dst);
				WM32(ea, dst);
				CYCLES(CYCLES_INC_MEM);
			}
			break;
		case 1:			/* DEC Rm32 */
			if(modrm >= 0xc0) {
				uint32 dst = LOAD_RM32(modrm);
				dst = DEC32(dst);
				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_DEC_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint32 dst = RM32(ea);
				dst = DEC32(dst);
				WM32(ea, dst);
				CYCLES(CYCLES_DEC_MEM);
			}
			break;
		case 2:			/* CALL Rm32 */
			{
				uint32 address;
				if(modrm >= 0xc0) {
					address = LOAD_RM32(modrm);
					CYCLES(CYCLES_CALL_REG);		/* TODO: Timing = 7 + m */
				}
				else {
					uint32 ea = GetEA(modrm);
					address = RM32(ea);
					CYCLES(CYCLES_CALL_MEM);		/* TODO: Timing = 10 + m */
				}
				PUSH32(eip);
				eip = address;
				CHANGE_PC(eip);
#ifdef I386_BIOS_CALL
				BIOS_CALL_NEAR32();
#endif
			}
			break;
		case 3:			/* CALL FAR Rm32 */
			{
				uint16 selector;
				uint32 address;
				if(modrm >= 0xc0) {
//					fatalerror(_T("NYI\n"));
				}
				else {
					uint32 ea = GetEA(modrm);
					address = RM32(ea + 0);
					selector = RM16(ea + 4);
					CYCLES(CYCLES_CALL_MEM_INTERSEG);		/* TODO: Timing = 10 + m */
				}
				PUSH32(sreg[CS].selector);
				PUSH32(eip);
				sreg[CS].selector = selector;
				performed_intersegment_jump = 1;
				load_segment_descriptor(CS);
				eip = address;
				CHANGE_PC(eip);
#ifdef I386_BIOS_CALL
				BIOS_CALL_FAR32();
#endif
			}
			break;
		case 4:			/* JMP Rm32 */
			{
				uint32 address;
				if(modrm >= 0xc0) {
					address = LOAD_RM32(modrm);
					CYCLES(CYCLES_JMP_REG);		/* TODO: Timing = 7 + m */
				}
				else {
					uint32 ea = GetEA(modrm);
					address = RM32(ea);
					CYCLES(CYCLES_JMP_MEM);		/* TODO: Timing = 10 + m */
				}
				eip = address;
				CHANGE_PC(eip);
			}
			break;
		case 5:			/* JMP FAR Rm32 */
			{
				uint16 selector;
				uint32 address;
				if(modrm >= 0xc0) {
//					fatalerror(_T("NYI\n"));
				}
				else {
					uint32 ea = GetEA(modrm);
					address = RM32(ea + 0);
					selector = RM16(ea + 4);
					CYCLES(CYCLES_JMP_MEM_INTERSEG);		/* TODO: Timing = 10 + m */
				}
				sreg[CS].selector = selector;
				performed_intersegment_jump = 1;
				load_segment_descriptor(CS);
				eip = address;
				CHANGE_PC(eip);
			}
			break;
		case 6:			/* PUSH Rm32 */
			{
				uint32 val;
				if(modrm >= 0xc0) {
					val = LOAD_RM32(modrm);
				}
				else {
					uint32 ea = GetEA(modrm);
					val = RM32(ea);
				}
				PUSH32(val);
				CYCLES(CYCLES_PUSH_RM);
			}
			break;
		default:
//			fatalerror(_T("i386: groupFF_32 /%d unimplemented at %08X\n"), (modrm >> 3) & 7, pc-2);
			break;
	}
}

void I386::group0F00_32()	// Opcode 0x0f 00
{
	uint32 address, ea;
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 2:			/* LLDT */
			if(PROTECTED_MODE && !V8086_MODE) {
				if(modrm >= 0xc0) {
					address = LOAD_RM32(modrm);
					ea = translate(CS, address);
					CYCLES(CYCLES_LLDT_REG);
				}
				else {
					ea = GetEA(modrm);
					CYCLES(CYCLES_LLDT_MEM);
				}
				ldtr.segment = RM32(ea);
			}
			else {
				trap(ILLEGAL_INSTRUCTION, 0);
			}
			break;

		case 3:			/* LTR */
			if(PROTECTED_MODE && !V8086_MODE) {
				if(modrm >= 0xc0) {
					address = LOAD_RM32(modrm);
					ea = translate(CS, address);
					CYCLES(CYCLES_LTR_REG);
				}
				else {
					ea = GetEA(modrm);
					CYCLES(CYCLES_LTR_MEM);
				}
				task.segment = RM32(ea);
			}
			else {
				trap(ILLEGAL_INSTRUCTION, 0);
			}
			break;

		default:
//			fatalerror(_T("i386: group0F00_32 /%d unimplemented\n"), (modrm >> 3) & 7);
			break;
	}
}

void I386::group0F01_32()	// Opcode 0x0f 01
{
	uint8 modrm = FETCH8();
	uint32 address, ea;
#ifdef HAS_I386
	switch((modrm >> 3) & 7) {
		case 0:			/* SGDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM32(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				WM16(ea, gdtr.limit);
				WM32(ea + 2, gdtr.base);
				CYCLES(CYCLES_SGDT);
				break;
			}
		case 1:			/* SIDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM32(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				WM16(ea, idtr.limit);
				WM32(ea + 2, idtr.base);
				CYCLES(CYCLES_SIDT);
				break;
			}
		case 2:			/* LGDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM32(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				gdtr.limit = RM16(ea);
				gdtr.base = RM32(ea + 2);
				CYCLES(CYCLES_LGDT);
				break;
			}
		case 3:			/* LIDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM32(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				idtr.limit = RM16(ea);
				idtr.base = RM32(ea + 2);
				CYCLES(CYCLES_LIDT);
				break;
			}
		default:
//			fatalerror(_T("i386: unimplemented opcode 0x0f 01 /%d at %08X\n"), (modrm >> 3) & 7, eip - 2);
			break;
	}
#else
	switch((modrm >> 3) & 7) {
		case 0:			/* SGDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM32(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				WM16(ea, gdtr.limit);
				WM32(ea + 2, gdtr.base);
				CYCLES(CYCLES_SGDT);
				break;
			}
		case 1:			/* SIDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM32(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				WM16(ea, idtr.limit);
				WM32(ea + 2, idtr.base);
				CYCLES(CYCLES_SIDT);
				break;
			}
		case 2:			/* LGDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM32(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				gdtr.limit = RM16(ea);
				gdtr.base = RM32(ea + 2);
				CYCLES(CYCLES_LGDT);
				break;
			}
		case 3:			/* LIDT */
			{
				if(modrm >= 0xc0) {
					address = LOAD_RM32(modrm);
					ea = translate(CS, address);
				}
				else {
					ea = GetEA(modrm);
				}
				idtr.limit = RM16(ea);
				idtr.base = RM32(ea + 2);
				CYCLES(CYCLES_LIDT);
				break;
			}
		case 7:			/* INVLPG */
			{
				// Nothing to do ?
				break;
			}
		default:
//			fatalerror(_T("i486: unimplemented opcode 0x0f 01 /%d at %08X\n"), (modrm >> 3) & 7, eip - 2);
			break;
	}
#endif
}

void I386::group0FBA_32()	// Opcode 0x0f ba
{
	uint8 modrm = FETCH8();

	switch((modrm >> 3) & 7) {
		case 4:			/* BT Rm32, i8 */
			if(modrm >= 0xc0) {
				uint32 dst = LOAD_RM32(modrm);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;

				CYCLES(CYCLES_BT_IMM_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint32 dst = RM32(ea);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;

				CYCLES(CYCLES_BT_IMM_MEM);
			}
			break;
		case 5:			/* BTS Rm32, i8 */
			if(modrm >= 0xc0) {
				uint32 dst = LOAD_RM32(modrm);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				dst |= (1 << bit);

				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_BTS_IMM_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint32 dst = RM32(ea);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				dst |= (1 << bit);

				WM32(ea, dst);
				CYCLES(CYCLES_BTS_IMM_MEM);
			}
			break;
		case 6:			/* BTR Rm32, i8 */
			if(modrm >= 0xc0) {
				uint32 dst = LOAD_RM32(modrm);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				dst &= ~(1 << bit);

				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_BTR_IMM_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint32 dst = RM32(ea);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				dst &= ~(1 << bit);

				WM32(ea, dst);
				CYCLES(CYCLES_BTR_IMM_MEM);
			}
			break;
		case 7:			/* BTC Rm32, i8 */
			if(modrm >= 0xc0) {
				uint32 dst = LOAD_RM32(modrm);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				dst ^= (1 << bit);

				STORE_RM32(modrm, dst);
				CYCLES(CYCLES_BTC_IMM_REG);
			}
			else {
				uint32 ea = GetEA(modrm);
				uint32 dst = RM32(ea);
				uint8 bit = FETCH8();

				if(dst & (1 << bit))
					CF = 1;
				else
					CF = 0;
				dst ^= (1 << bit);

				WM32(ea, dst);
				CYCLES(CYCLES_BTC_IMM_MEM);
			}
			break;
		default:
//			fatalerror(_T("i386: group0FBA_32 /%d unknown\n"), (modrm >> 3) & 7);
			break;
	}
}

void I386::bound_r32_m32_m32()	// Opcode 0x62
{
	uint8 modrm;
	int32 val, low, high;

	modrm = FETCH8();

	if(modrm >= 0xc0) {
		low = high = LOAD_RM32(modrm);
	}
	else {
		uint32 ea = GetEA(modrm);
		low = RM32(ea + 0);
		high = RM32(ea + 4);
	}
	val = LOAD_REG32(modrm);

	if((val < low) || (val > high)) {
		CYCLES(CYCLES_BOUND_OUT_RANGE);
		trap(5, 0);
	}
	else {
		CYCLES(CYCLES_BOUND_IN_RANGE);
	}
}

void I386::retf32()	// Opcode 0xcb
{
	eip = POP32();
	sreg[CS].selector = POP32();
	load_segment_descriptor(CS);
	CHANGE_PC(eip);

	CYCLES(CYCLES_RET_INTERSEG);
}

void I386::retf_i32()	// Opcode 0xca
{
	uint16 count = FETCH16();

	eip = POP32();
	sreg[CS].selector = POP32();
	load_segment_descriptor(CS);
	CHANGE_PC(eip);

	REG32(ESP) += count;
	CYCLES(CYCLES_RET_IMM_INTERSEG);
}

void I386::xlat32()	// Opcode 0xd7
{
	uint32 ea;
	if(segment_prefix) {
		ea = translate(segment_override, REG32(EBX) + REG8(AL));
	}
	else {
		ea = translate(DS, REG32(EBX) + REG8(AL));
	}
	REG8(AL) = RM8(ea);
	CYCLES(CYCLES_XLAT);
}

void I386::load_far_pointer32(int s)
{
	uint8 modrm = FETCH8();

	if(modrm >= 0xc0) {
//		fatalerror(_T("NYI\n"));
	}
	else {
		uint32 ea = GetEA(modrm);
		STORE_REG32(modrm, RM32(ea + 0));
		sreg[s].selector = RM16(ea + 4);
		load_segment_descriptor(s);
	}
}

void I386::lds32()	// Opcode 0xc5
{
	load_far_pointer32(DS);
	CYCLES(CYCLES_LDS);
}

void I386::lss32()	// Opcode 0x0f 0xb2
{
	load_far_pointer32(SS);
	CYCLES(CYCLES_LSS);
}

void I386::les32()	// Opcode 0xc4
{
	load_far_pointer32(ES);
	CYCLES(CYCLES_LES);
}

void I386::lfs32()	// Opcode 0x0f 0xb4
{
	load_far_pointer32(FS);
	CYCLES(CYCLES_LFS);
}

void I386::lgs32()	// Opcode 0x0f 0xb5
{
	load_far_pointer32(GS);
	CYCLES(CYCLES_LGS);
}
// Intel 486+ specific opcodes

void I386::cpuid()	// Opcode 0x0F A2
{
	switch(REG32(EAX))
	{
		case 0:
			REG32(EAX) = 1;
#if defined(HAS_PENTIUM)
			REG32(EBX) = 0x756e6547;	// Genu
			REG32(EDX) = 0x49656e69;	// ineI
			REG32(ECX) = 0x6c65746e;	// ntel
#elif defined(HAS_MEDIAGX)
			REG32(EBX) = 0x69727943;	// Cyri
			REG32(EDX) = 0x736e4978;	// xIns
			REG32(ECX) = 0x6d616574;	// tead
#endif
			CYCLES(CYCLES_CPUID);
			break;
		case 1:
#if defined(HAS_PENTIUM)
			REG32(EAX) = (5 << 8) | (2 << 4) | (1);
			REG32(EDX) = 0;
#elif defined(HAS_MEDIAGX)
			REG32(EAX) = (4 << 8) | (4 << 4) | (1)
			REG32(EDX) = 1;
#endif
			CYCLES(CYCLES_CPUID_EAX1);
			break;
	}
}

void I386::invd()	// Opcode 0x0f 08
{
	// Nothing to do ?
	CYCLES(CYCLES_INVD);
}

void I386::wbinvd()	// Opcode 0x0f 09
{
	// Nothing to do ?
}

void I386::cmpxchg_rm8_r8()	// Opcode 0x0f b0
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint8 dst = LOAD_RM8(modrm);
		uint8 src = LOAD_REG8(modrm);

		if(REG8(AL) == dst) {
			STORE_RM8(modrm, src);
			ZF = 1;
			CYCLES(CYCLES_CMPXCHG_REG_REG_T);
		}
		else {
			REG8(AL) = dst;
			ZF = 0;
			CYCLES(CYCLES_CMPXCHG_REG_REG_F);
		}
	}
	else {
		uint32 ea = GetEA(modrm);
		uint8 dst = RM8(ea);
		uint8 src = LOAD_REG8(modrm);

		if(REG8(AL) == dst) {
			WM8(modrm, src);
			ZF = 1;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_T);
		}
		else {
			REG8(AL) = dst;
			ZF = 0;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_F);
		}
	}
}

void I386::cmpxchg_rm16_r16()	// Opcode 0x0f b1
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint16 dst = LOAD_RM16(modrm);
		uint16 src = LOAD_REG16(modrm);

		if(REG16(AX) == dst) {
			STORE_RM16(modrm, src);
			ZF = 1;
			CYCLES(CYCLES_CMPXCHG_REG_REG_T);
		}
		else {
			REG16(AX) = dst;
			ZF = 0;
			CYCLES(CYCLES_CMPXCHG_REG_REG_F);
		}
	}
	else {
		uint32 ea = GetEA(modrm);
		uint16 dst = RM16(ea);
		uint16 src = LOAD_REG16(modrm);

		if(REG16(AX) == dst) {
			WM16(modrm, src);
			ZF = 1;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_T);
		}
		else {
			REG16(AX) = dst;
			ZF = 0;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_F);
		}
	}
}

void I386::cmpxchg_rm32_r32()	// Opcode 0x0f b1
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 dst = LOAD_RM32(modrm);
		uint32 src = LOAD_REG32(modrm);

		if(REG32(EAX) == dst) {
			STORE_RM32(modrm, src);
			ZF = 1;
			CYCLES(CYCLES_CMPXCHG_REG_REG_T);
		}
		else {
			REG32(EAX) = dst;
			ZF = 0;
			CYCLES(CYCLES_CMPXCHG_REG_REG_F);
		}
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 dst = RM32(ea);
		uint32 src = LOAD_REG32(modrm);

		if(REG32(EAX) == dst) {
			WM32(ea, src);
			ZF = 1;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_T);
		}
		else {
			REG32(EAX) = dst;
			ZF = 0;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_F);
		}
	}
}

void I386::xadd_rm8_r8()	// Opcode 0x0f c0
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint8 dst = LOAD_RM8(modrm);
		uint8 src = LOAD_REG8(modrm);
		STORE_RM16(modrm, dst + src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_XADD);//_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint8 dst = RM8(ea);
		uint8 src = LOAD_REG8(modrm);
		WM8(ea, dst + src);
		STORE_REG8(modrm, dst);
		CYCLES(CYCLES_XADD);//_REG_MEM);
	}
}

void I386::xadd_rm16_r16()	// Opcode 0x0f c1
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint16 dst = LOAD_RM16(modrm);
		uint16 src = LOAD_REG16(modrm);
		STORE_RM16(modrm, dst + src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_XADD);//_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint16 dst = RM16(ea);
		uint16 src = LOAD_REG16(modrm);
		WM16(ea, dst + src);
		STORE_REG16(modrm, dst);
		CYCLES(CYCLES_XADD);//_REG_MEM);
	}
}

void I386::xadd_rm32_r32()	// Opcode 0x0f c1
{
	uint8 modrm = FETCH8();
	if(modrm >= 0xc0) {
		uint32 dst = LOAD_RM32(modrm);
		uint32 src = LOAD_REG32(modrm);
		STORE_RM32(modrm, dst + src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_XADD);//_REG_REG);
	}
	else {
		uint32 ea = GetEA(modrm);
		uint32 dst = RM32(ea);
		uint32 src = LOAD_REG32(modrm);
		WM32(ea, dst + src);
		STORE_REG32(modrm, dst);
		CYCLES(CYCLES_XADD);//_REG_MEM);
	}
}

// Pentium+ specific opcodes

void I386::rdmsr()	// Opcode 0x0f 32
{
	// TODO
	CYCLES(CYCLES_RDMSR);
}

void I386::wrmsr()	// Opcode 0x0f 30
{
	// TODO
	CYCLES_NUM(1);		// TODO: correct cycle count
}

void I386::rdtsc()	// Opcode 0x0f 31
{
	uint64 ts = tsc + (base_cycles - cycles);
	REG32(EAX) = (uint32)(ts);
	REG32(EDX) = (uint32)(ts >> 32);

	CYCLES(CYCLES_RDTSC);
}

void I386::cyrix_unknown()	// Opcode 0x0f 74
{
	CYCLES_NUM(1);
}

void I386::cmpxchg8b_m64()	// Opcode 0x0f c7
{
	uint8 modm = FETCH8();
	if(modm >= 0xc0) {
//		fatalerror(_T("invalid modm\n"));
	}
	else {
		uint32 ea = GetEA(modm);
		uint64 val = RM64(ea);
		uint64 edx_eax = (((uint64) REG32(EDX)) << 32) | REG32(EAX);
		uint64 ecx_ebx = (((uint64) REG32(ECX)) << 32) | REG32(EBX);

		if(val == edx_eax) {
			WM64(ea, ecx_ebx);
			ZF = 1;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_T);
		}
		else {
			REG32(EDX) = (uint32)(val >> 32);
			REG32(EAX) = (uint32)(val >>  0);
			ZF = 0;
			CYCLES(CYCLES_CMPXCHG_REG_MEM_F);
		}
	}
}

inline void I386::FPU_PUSH(x87reg_c val)
{
	fpu.top--;
	if(fpu.top < 0)
		fpu.top = 7;
	fpu.reg[fpu.top] = val;
}

inline void I386::FPU_POP()
{
	fpu.tag_word |= 3 << (fpu.top * 2);		// set FPU register tag to 3 (empty)
	fpu.top++;
	if(fpu.top > 7)
		fpu.top = 0;
}

void I386::fpu_group_d8()	// Opcode 0xd8
{
	uint8 modrm = FETCH8();
//	fatalerror(_T("I386: FPU Op D8 %02X at %08X\n"), modrm, pc-2);
}

void I386::fpu_group_d9()	// Opcode 0xd9
{
	uint8 modrm = FETCH8();

	if(modrm < 0xc0) {
		uint32 ea = GetEA(modrm);

		switch((modrm >> 3) & 7) {
			case 5:			// FLDCW
			{
				fpu.control_word = RM16(ea);
				CYCLES_NUM(1);		// TODO
				break;
			}

			case 7:			// FSTCW
			{
				WM16(ea, fpu.control_word);
				CYCLES_NUM(1);		// TODO
				break;
			}

//			default:
//				fatalerror(_T("I386: FPU Op D9 %02X at %08X\n"), modrm, pc-2);
		}
	}
	else {
		switch(modrm & 0x3f) {
			// FLD
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
			{
				x87reg_c t = ST(modrm & 7);
				FPU_PUSH(t);
				CYCLES_NUM(1);		// TODO
				break;
			}

			case 0x20:		// FCHS
			{
				ST(0).i ^= FPU_SIGN_BIT_DOUBLE;
				CYCLES_NUM(1);		// TODO
				break;
			}

			case 0x28:		// FLD1
			{
				x87reg_c t;
				t.f = 1.0;
				FPU_PUSH(t);
				CYCLES_NUM(1);		// TODO
				break;
			}

			case 0x2e:		// FLDZ
			{
				x87reg_c t;
				t.f = 0.0;
				FPU_PUSH(t);
				CYCLES_NUM(1);		// TODO
				break;
			}
//			default:
//				fatalerror(_T("I386: FPU Op D9 %02X at %08X\n"), modrm, pc-2);
		}
	}
}

void I386::fpu_group_da()	// Opcode 0xda
{
	uint8 modrm = FETCH8();
//	fatalerror(_T("I386: FPU Op DA %02X at %08X\n"), modrm, pc-2);
}

void I386::fpu_group_db()	// Opcode 0xdb
{
	uint8 modrm = FETCH8();

	if(modrm < 0xc0) {
//		fatalerror(_T("I386: FPU Op DB %02X at %08X\n"), modrm, pc-2);
	}
	else {
		switch(modrm & 0x3f) {
			case 0x23:		// FINIT
			{
				fpu.control_word = 0x37f;
				fpu.status_word = 0;
				fpu.tag_word = 0xffff;
				fpu.data_ptr = 0;
				fpu.inst_ptr = 0;
				fpu.opcode = 0;

				CYCLES_NUM(1);		// TODO
				break;
			}

			case 0x24:		// FSETPM (treated as nop on 387+)
			{
				CYCLES_NUM(1);
				break;
			}

//			default:
//				fatalerror(_T("I386: FPU Op DB %02X at %08X\n"), modrm, pc-2);
		}
	}
}

void I386::fpu_group_dc()	// Opcode 0xdc
{
	uint8 modrm = FETCH8();

	if(modrm < 0xc0) {
		//uint32 ea = GetEA(modrm);

//		switch((modrm >> 3) & 7) {
//			default:
//				fatalerror(_T("I386: FPU Op DC %02X at %08X\n"), modrm, pc-2);
//		}
	}
	else {
		switch(modrm & 0x3f) {
			case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
			{
				// FDIVR
				if((ST(modrm & 7).i & U64(0x7fffffffffffffff)) == 0) {
					// set result as infinity if zero divide is masked
					if(fpu.control_word & FPU_MASK_ZERO_DIVIDE) {
						ST(modrm & 7).i |= FPU_INFINITY_DOUBLE;
					}
				}
				else {
					ST(modrm & 7).f = ST(0).f / ST(modrm & 7).f;
				}
				CYCLES_NUM(1);		// TODO
				break;
			}

//			default:
//				fatalerror(_T("I386: FPU Op DC %02X at %08X\n"), modrm, pc-2);
		}
	}
}

void I386::fpu_group_dd()	// Opcode 0xdd
{
	uint8 modrm = FETCH8();

	if(modrm < 0xc0) {
		uint32 ea = GetEA(modrm);

		switch((modrm >> 3) & 7) {
			case 7:			// FSTSW
			{
				WM16(ea, (fpu.status_word & ~FPU_STACK_TOP_MASK) | (fpu.top << 10));
				CYCLES_NUM(1);		// TODO
				break;
			}

//			default:
//				fatalerror(_T("I386: FPU Op DD %02X at %08X\n"), modrm, pc-2);
		}
	}
	else {
//		switch(modrm & 0x3f) {
//			default:
//				fatalerror(_T("I386: FPU Op DD %02X at %08X\n"), modrm, pc-2);
//		}
	}
}

void I386::fpu_group_de()	// Opcode 0xde
{
	uint8 modrm = FETCH8();

	if(modrm < 0xc0) {
//		uint32 ea = GetEA(modrm);
//		switch((modrm >> 3) & 7) {
//			default:
//				fatalerror(_T("I386: FPU Op DE %02X at %08X\n"), modrm, pc-2);
//		}
	}
	else {
		switch(modrm & 0x3f) {
			case 0x19:			// FCOMPP
			{
				fpu.status_word &= ~(FPU_C3 | FPU_C2 | FPU_C0);
				if(ST(0).f > ST(1).f) {
					// C3 = 0, C2 = 0, C0 = 0
				}
				else if(ST(0).f < ST(1).f) {
					fpu.status_word |= FPU_C0;
				}
				else if(ST(0).f == ST(1).f) {
					fpu.status_word |= FPU_C3;
				}
				else {
					// unordered
					fpu.status_word |= (FPU_C3 | FPU_C2 | FPU_C0);
				}
				FPU_POP();
				FPU_POP();
				CYCLES_NUM(1);		// TODO
				break;
			}

			// FDIVP
			case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
			{
				if((ST(0).i & U64(0x7fffffffffffffff)) == 0) {
					// set result as infinity if zero divide is masked
					if(fpu.control_word & FPU_MASK_ZERO_DIVIDE) {
						ST(modrm & 7).i |= FPU_INFINITY_DOUBLE;
					}
				}
				else {
					ST(modrm & 7).f = ST(modrm & 7).f / ST(0).f;
				}
				FPU_POP();
				CYCLES_NUM(1);		// TODO
				break;
			}
//			default:
//				fatalerror(_T("I386: FPU Op DE %02X at %08X\n"), modrm, pc-2);
		}
	}
}

void I386::fpu_group_df()	// Opcode 0xdf
{
	uint8 modrm = FETCH8();

	if(modrm < 0xc0) {
//		switch((modrm >> 3) & 7)
//		{
//		default:
//			fatalerror(_T("I386: FPU Op DF %02X at %08X\n"), modrm, pc-2);
//		}
	}
	else {
		switch(modrm & 0x3f)
		{
		case 0x20:			// FSTSW AX
			REG16(AX) = (fpu.status_word & ~FPU_STACK_TOP_MASK) | (fpu.top << 10);
			CYCLES_NUM(1);		// TODO
			break;
//		default:
//			fatalerror(_T("I386: FPU Op DF %02X at %08X\n"), modrm, pc-2);
		}
	}
}
