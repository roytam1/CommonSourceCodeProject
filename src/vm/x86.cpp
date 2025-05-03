/*
	Skelton for retropc emulator

	Origin : MAME
	Author : Takeda.Toshiya
	Date  : 2007.08.11 -

	[ 80x86 ]
*/

#include "x86.h"

// interrupt vector
#define NMI_INT_VECTOR			2
#define ILLEGAL_INSTRUCTION		6
#define GENERAL_PROTECTION_FAULT	0xd

// flags
#define CF	(CarryVal != 0)
#define SF	(SignVal < 0)
#define ZF	(ZeroVal == 0)
#define PF	parity_table[ParityVal]
#define AF	(AuxVal != 0)
#define OF	(OverVal != 0)
#define DF	(DirVal < 0)
#define MD	(MF != 0)
#define PM	(msw & 1)
#define CPL	(sregs[CS] & 3)
#define IOPL	((flags & 0x3000) >> 12)

#define SetTF(x) (TF = (x))
#define SetIF(x) (IF = (x))
#define SetDF(x) (DirVal = (x) ? -1 : 1)
#define SetMD(x) (MF = (x))
#define SetOFW_Add(x, y, z) (OverVal = ((x) ^ (y)) & ((x) ^ (z)) & 0x8000)
#define SetOFB_Add(x, y, z) (OverVal = ((x) ^ (y)) & ((x) ^ (z)) & 0x80)
#define SetOFW_Sub(x, y, z) (OverVal = ((z) ^ (y)) & ((z) ^ (x)) & 0x8000)
#define SetOFB_Sub(x, y, z) (OverVal = ((z) ^ (y)) & ((z) ^ (x)) & 0x80)
#define SetCFB(x) (CarryVal = (x) & 0x100)
#define SetCFW(x) (CarryVal = (x) & 0x10000)
#define SetAF(x, y, z) (AuxVal = ((x) ^ ((y) ^ (z))) & 0x10)
#define SetSF(x) (SignVal = (x))
#define SetZF(x) (ZeroVal = (x))
#define SetPF(x) (ParityVal = (x))
#define SetSZPF_Byte(x) (ParityVal = SignVal = ZeroVal = (int8)(x))
#define SetSZPF_Word(x) (ParityVal = SignVal = ZeroVal = (int16)(x))

#define CompressFlags() (uint16)(CF | (PF << 2) | (AF << 4) | (ZF << 6) | (SF << 7) | (TF << 8) | (IF << 9) | (DF << 10) | (OF << 11) | (MD << 15))
#define ExpandFlags(f) { \
	CarryVal = (f) & 1; \
	ParityVal = !((f) & 4); \
	AuxVal = (f) & 0x10; \
	ZeroVal = !((f) & 0x40); \
	SignVal = ((f) & 0x80) ? -1 : 0; \
	TF = ((f) & 0x100) >> 8; \
	IF = ((f) & 0x200) >> 9; \
	MF = ((f) & 0x8000) >> 15; \
	DirVal = ((f) & 0x400) ? -1 : 1; \
	OverVal = (f) & 0x800; \
}

// opecodes
#define ADDB(dst, src) { unsigned res = (dst) + (src); SetCFB(res); SetOFB_Add(res, src, dst); SetAF(res, src, dst); SetSZPF_Byte(res); dst = (uint8)res; }
#define ADDW(dst, src) { unsigned res = (dst) + (src); SetCFW(res); SetOFW_Add(res, src, dst); SetAF(res, src, dst); SetSZPF_Word(res); dst = (uint16)res; }
#define SUBB(dst, src) { unsigned res = (dst) - (src); SetCFB(res); SetOFB_Sub(res, src, dst); SetAF(res, src, dst); SetSZPF_Byte(res); dst = (uint8)res; }
#define SUBW(dst, src) { unsigned res = (dst) - (src); SetCFW(res); SetOFW_Sub(res, src, dst); SetAF(res, src, dst); SetSZPF_Word(res); dst = (uint16)res; }
#define ORB(dst, src) dst |= (src); CarryVal = OverVal = AuxVal = 0; SetSZPF_Byte(dst)
#define ORW(dst, src) dst |= (src); CarryVal = OverVal = AuxVal = 0; SetSZPF_Word(dst)
#define ANDB(dst, src) dst &= (src); CarryVal = OverVal = AuxVal = 0; SetSZPF_Byte(dst)
#define ANDW(dst, src) dst &= (src); CarryVal = OverVal = AuxVal = 0; SetSZPF_Word(dst)
#define XORB(dst, src) dst ^= (src); CarryVal = OverVal = AuxVal = 0; SetSZPF_Byte(dst)
#define XORW(dst, src) dst ^= (src); CarryVal = OverVal = AuxVal = 0; SetSZPF_Word(dst)

// memory
#define RegWord(ModRM) regs.w[mod_reg16[ModRM]]
#define RegByte(ModRM) regs.b[mod_reg8[ModRM]]
#define GetRMWord(ModRM) ((ModRM) >= 0xc0 ? regs.w[mod_rm16[ModRM]] : (GetEA(ModRM), RM16(EA)))
#define PutbackRMWord(ModRM, val) { \
	if(ModRM >= 0xc0) \
		regs.w[mod_rm16[ModRM]] = val; \
	else \
		WM16(EA, val); \
}
#define GetNextRMWord() RM16(EA + 2)
#define GetRMWordOfs(ofs) RM16(EA - EO + (uint16)(EO + (ofs)))
#define GetRMByteOfs(ofs) RM8(EA - EO + (uint16)(EO + (ofs)))
#define PutRMWord(ModRM, val) { \
	if (ModRM >= 0xc0) \
		regs.w[mod_rm16[ModRM]] = val; \
	else { \
		GetEA(ModRM); \
		WM16(EA, val); \
	} \
}
#define PutRMWordOfs(ofs, val) WM16(EA - EO + (uint16)(EO + (ofs)), val)
#define PutRMByteOfs(offs, val) WM8(EA - EO + (uint16)(EO + (offs)), val)
#define PutImmRMWord(ModRM) { \
	if (ModRM >= 0xc0) \
		regs.w[mod_rm16[ModRM]] = FETCH16(); \
	else { \
		GetEA(ModRM); \
		uint16 val = FETCH16(); \
		WM16(EA , val); \
	} \
}
#define GetRMByte(ModRM) ((ModRM) >= 0xc0 ? regs.b[mod_rm8[ModRM]] : RM8(GetEA(ModRM)))
#define PutRMByte(ModRM, val) { \
	if(ModRM >= 0xc0) \
		regs.b[mod_rm8[ModRM]] = val; \
	else \
		WM8(GetEA(ModRM), val); \
}
#define PutImmRMByte(ModRM) { \
	if (ModRM >= 0xc0) \
		regs.b[mod_rm8[ModRM]] = FETCH8(); \
	else { \
		GetEA(ModRM); \
		WM8(EA , FETCH8()); \
	} \
}
#define PutbackRMByte(ModRM, val) { \
	if (ModRM >= 0xc0) \
		regs.b[mod_rm8[ModRM]] = val; \
	else \
		WM8(EA, val); \
}
#define DEF_br8(dst, src) \
	unsigned ModRM = FETCHOP(); \
	unsigned src = RegByte(ModRM); \
	unsigned dst = GetRMByte(ModRM)
#define DEF_wr16(dst, src) \
	unsigned ModRM = FETCHOP(); \
	unsigned src = RegWord(ModRM); \
	unsigned dst = GetRMWord(ModRM)
#define DEF_r8b(dst, src) \
	unsigned ModRM = FETCHOP(); \
	unsigned dst = RegByte(ModRM); \
	unsigned src = GetRMByte(ModRM)
#define DEF_r16w(dst, src) \
	unsigned ModRM = FETCHOP(); \
	unsigned dst = RegWord(ModRM); \
	unsigned src = GetRMWord(ModRM)
#define DEF_ald8(dst, src) \
	unsigned src = FETCHOP(); \
	unsigned dst = regs.b[AL]
#define DEF_axd16(dst, src) \
	unsigned src = FETCHOP(); \
	unsigned dst = regs.w[AX]; \
	src += (FETCH8() << 8)

void X86::initialize()
{
#ifdef I286
	AMASK = 0xfffff;
#endif
	prefix_base = 0;	// ???
	seg_prefix = false;
}

void X86::reset()
{
	for(int i = 0; i < 8; i++)
		regs.w[i] = 0;
	_memset(sregs, 0, sizeof(sregs));
	_memset(limit, 0, sizeof(limit));
	_memset(base, 0, sizeof(base));
	EA = 0;
	EO = 0;
	gdtr_base = idtr_base = ldtr_base = tr_base = 0;
	gdtr_limit = idtr_limit = ldtr_limit = tr_limit = 0;
	ldtr_sel = tr_sel = 0;
	AuxVal = OverVal = SignVal = ZeroVal = CarryVal = DirVal = 0;
	ParityVal = TF = IF = MF = 0;
	halt = false;
	intstat = busy = 0;
	
	sregs[CS] = 0xf000;
	limit[CS] = limit[SS] = limit[DS] = limit[ES] = 0xffff;
	base[CS] = SegBase(CS);
	idtr_limit = 0x3ff;
#ifdef I286
	PC = 0xffff0;
	msw = 0xfff0;
	flags = 2;
#else
	PC = 0xffff0 & AMASK;
	msw = flags = 0;
#endif
	ExpandFlags(flags);
#ifdef V30
	SetMD(1);
#endif
}

void X86::run(int clock)
{
	// return now if BUSREQ
	if(busreq) {
		count = extra_count = first = 0;
		return;
	}
	
	// run cpu while given clocks
	count += clock;
	first = count;
	
	// adjust for any interrupts
	count -= extra_count;
	extra_count = 0;
	
	while(count > 0) {
		seg_prefix = false;
		op(FETCHOP());
		if(intstat) {
			if(halt) {
				PC++;
				halt = false;
			}
			unsigned intnum;
			if(intstat & NMI_REQ_BIT)
				intnum = NMI_INT_VECTOR;
			else
				intnum = ACK_INTR() & 0xff;
			intstat = 0;
			interrupt(intnum);
		}
	}
	count -= extra_count;
	extra_count = 0;
	first = count;
}

void X86::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_CPU_NMI) {
		if(data & mask)
			intstat |= NMI_REQ_BIT;
		else
			intstat &= ~NMI_REQ_BIT;
	}
	else if(id == SIG_CPU_BUSREQ) {
		busreq = (data & mask) ? true : false;
		if(busreq)
			count = extra_count = first = 0;
	}
	else if(id == SIG_X86_TEST)
		busy = (data & mask) ? 1 : 0;
#ifdef I286
	else if(id == SIG_X86_A20)
		AMASK = (data & mask) ? 0xffffff : 0xfffff;
#endif
}

void X86::interrupt(unsigned num)
{
#ifdef I286
	if(PM) {
		if((num << 3) >= idtr_limit) // go into shutdown mode
			return;
		_pushf();
		PUSH16(sregs[CS]);
		PUSH16(PC - base[CS]);
		uint16 word1 = RM16(idtr_base + (num << 3));
		uint16 word2 = RM16(idtr_base + (num << 3) + 2);
		uint16 word3 = RM16(idtr_base + (num << 3) + 4);
		switch(word3 & 0xf00)
		{
		case 0x500: // task gate
			i286_data_descriptor(CS, word2);
			PC = base[CS] + word1;
			break;
		case 0x600: // interrupt gate
			TF = IF = 0;
			i286_data_descriptor(CS, word2);
			PC = base[CS] + word1;
			break;
		case 0x700: // trap gate
			i286_data_descriptor(CS, word2);
			PC = base[CS] + word1;
			break;
		}
	}
	else {
#endif
		uint16 ip = PC - base[CS];
		unsigned ofs = RM16(num * 4);
		unsigned seg = RM16(num * 4 + 2);
		_pushf();
		TF = IF = 0;
		PUSH16(sregs[CS]);
		PUSH16(ip);
		sregs[CS] = (uint16)seg;
		base[CS] = SegBase(CS);
		PC = (base[CS] + ofs) & AMASK;
#ifdef I286
	}
#endif
	extra_count += cycles.exception;
}

unsigned X86::GetEA(unsigned ModRM)
{
	switch(ModRM)
	{
	case 0x00: case 0x08: case 0x10: case 0x18: case 0x20: case 0x28: case 0x30: case 0x38:
		count -= 7; EO = (uint16)(regs.w[BX] + regs.w[SI]); EA = DefaultBase(DS) + EO; return EA;
	case 0x01: case 0x09: case 0x11: case 0x19: case 0x21: case 0x29: case 0x31: case 0x39:
		count -= 8; EO = (uint16)(regs.w[BX] + regs.w[DI]); EA = DefaultBase(DS) + EO; return EA;
	case 0x02: case 0x0a: case 0x12: case 0x1a: case 0x22: case 0x2a: case 0x32: case 0x3a:
		count -= 8; EO = (uint16)(regs.w[BP] + regs.w[SI]); EA = DefaultBase(SS) + EO; return EA;
	case 0x03: case 0x0b: case 0x13: case 0x1b: case 0x23: case 0x2b: case 0x33: case 0x3b:
		count -= 7; EO = (uint16)(regs.w[BP] + regs.w[DI]); EA = DefaultBase(SS) + EO; return EA;
	case 0x04: case 0x0c: case 0x14: case 0x1c: case 0x24: case 0x2c: case 0x34: case 0x3c:
		count -= 5; EO = regs.w[SI]; EA = DefaultBase(DS) + EO; return EA;
	case 0x05: case 0x0d: case 0x15: case 0x1d: case 0x25: case 0x2d: case 0x35: case 0x3d:
		count -= 5; EO = regs.w[DI]; EA = DefaultBase(DS) + EO; return EA;
	case 0x06: case 0x0e: case 0x16: case 0x1e: case 0x26: case 0x2e: case 0x36: case 0x3e:
		count -= 6; EO = FETCHOP(); EO += FETCHOP() << 8; EA = DefaultBase(DS) + EO; return EA;
	case 0x07: case 0x0f: case 0x17: case 0x1f: case 0x27: case 0x2f: case 0x37: case 0x3f:
		count -= 5; EO = regs.w[BX]; EA = DefaultBase(DS) + EO; return EA;
	case 0x40: case 0x48: case 0x50: case 0x58: case 0x60: case 0x68: case 0x70: case 0x78:
		count -= 11; EO = (uint16)(regs.w[BX] + regs.w[SI] + (int8)FETCHOP()); EA = DefaultBase(DS) + EO; return EA;
	case 0x41: case 0x49: case 0x51: case 0x59: case 0x61: case 0x69: case 0x71: case 0x79:
		count -= 12; EO = (uint16)(regs.w[BX] + regs.w[DI] + (int8)FETCHOP()); EA = DefaultBase(DS) + EO; return EA;
	case 0x42: case 0x4a: case 0x52: case 0x5a: case 0x62: case 0x6a: case 0x72: case 0x7a:
		count -= 12; EO = (uint16)(regs.w[BP] + regs.w[SI] + (int8)FETCHOP()); EA = DefaultBase(SS) + EO; return EA;
	case 0x43: case 0x4b: case 0x53: case 0x5b: case 0x63: case 0x6b: case 0x73: case 0x7b:
		count -= 11; EO = (uint16)(regs.w[BP] + regs.w[DI] + (int8)FETCHOP()); EA = DefaultBase(SS) + EO; return EA;
	case 0x44: case 0x4c: case 0x54: case 0x5c: case 0x64: case 0x6c: case 0x74: case 0x7c:
		count -= 9; EO = (uint16)(regs.w[SI] + (int8)FETCHOP()); EA = DefaultBase(DS) + EO; return EA;
	case 0x45: case 0x4d: case 0x55: case 0x5d: case 0x65: case 0x6d: case 0x75: case 0x7d:
		count -= 9; EO = (uint16)(regs.w[DI] + (int8)FETCHOP()); EA = DefaultBase(DS) + EO; return EA;
	case 0x46: case 0x4e: case 0x56: case 0x5e: case 0x66: case 0x6e: case 0x76: case 0x7e:
		count -= 9; EO = (uint16)(regs.w[BP] + (int8)FETCHOP()); EA = DefaultBase(SS) + EO; return EA;
	case 0x47: case 0x4f: case 0x57: case 0x5f: case 0x67: case 0x6f: case 0x77: case 0x7f:
		count -= 9; EO = (uint16)(regs.w[BX] + (int8)FETCHOP()); EA = DefaultBase(DS) + EO; return EA;
	case 0x80: case 0x88: case 0x90: case 0x98: case 0xa0: case 0xa8: case 0xb0: case 0xb8:
		count -= 11; EO = FETCHOP(); EO += FETCHOP() << 8; EO += regs.w[BX] + regs.w[SI]; EA = DefaultBase(DS) + (uint16)EO; return EA;
	case 0x81: case 0x89: case 0x91: case 0x99: case 0xa1: case 0xa9: case 0xb1: case 0xb9:
		count -= 12; EO = FETCHOP(); EO += FETCHOP() << 8; EO += regs.w[BX] + regs.w[DI]; EA = DefaultBase(DS) + (uint16)EO; return EA;
	case 0x82: case 0x8a: case 0x92: case 0x9a: case 0xa2: case 0xaa: case 0xb2: case 0xba:
		count -= 12; EO = FETCHOP(); EO += FETCHOP() << 8; EO += regs.w[BP] + regs.w[SI]; EA = DefaultBase(SS) + (uint16)EO; return EA;
	case 0x83: case 0x8b: case 0x93: case 0x9b: case 0xa3: case 0xab: case 0xb3: case 0xbb:
		count -= 11; EO = FETCHOP(); EO += FETCHOP() << 8; EO += regs.w[BP] + regs.w[DI]; EA = DefaultBase(SS) + (uint16)EO; return EA;
	case 0x84: case 0x8c: case 0x94: case 0x9c: case 0xa4: case 0xac: case 0xb4: case 0xbc:
		count -= 9; EO = FETCHOP(); EO += FETCHOP() << 8; EO += regs.w[SI]; EA = DefaultBase(DS) + (uint16)EO; return EA;
	case 0x85: case 0x8d: case 0x95: case 0x9d: case 0xa5: case 0xad: case 0xb5: case 0xbd:
		count -= 9; EO = FETCHOP(); EO += FETCHOP() << 8; EO += regs.w[DI]; EA = DefaultBase(DS) + (uint16)EO; return EA;
	case 0x86: case 0x8e: case 0x96: case 0x9e: case 0xa6: case 0xae: case 0xb6: case 0xbe:
		count -= 9; EO = FETCHOP(); EO += FETCHOP() << 8; EO += regs.w[BP]; EA = DefaultBase(SS) + (uint16)EO; return EA;
	case 0x87: case 0x8f: case 0x97: case 0x9f: case 0xa7: case 0xaf: case 0xb7: case 0xbf:
		count -= 9; EO = FETCHOP(); EO += FETCHOP() << 8; EO += regs.w[BX]; EA = DefaultBase(DS) + (uint16)EO; return EA;
	}
	return 0;
}

#define WRITEABLE(a) (((a) & 0xa) == 2)
#define READABLE(a) ((((a) & 0xa) == 0xa) || (((a) & 8) == 0))

void X86::rotate_shift_byte(unsigned ModRM, unsigned cnt)
{
	unsigned src = GetRMByte(ModRM);
	unsigned dst = src;
	
	if(cnt == 0)
		count -= (ModRM >= 0xc0) ? cycles.rot_reg_base : cycles.rot_m8_base;
	else if(cnt == 1) {
		count -= (ModRM >= 0xc0) ? cycles.rot_reg_1 : cycles.rot_m8_1;
		
		switch(ModRM & 0x38)
		{
		case 0x00:	// ROL eb, 1
			CarryVal = src & 0x80;
			dst = (src << 1) + CF;
			PutbackRMByte(ModRM, dst);
			OverVal = (src ^ dst) & 0x80;
			break;
		case 0x08:	// ROR eb, 1
			CarryVal = src & 1;
			dst = ((CF << 8) + src) >> 1;
			PutbackRMByte(ModRM, dst);
			OverVal = (src ^ dst) & 0x80;
			break;
		case 0x10:	// RCL eb, 1
			dst = (src << 1) + CF;
			PutbackRMByte(ModRM, dst);
			SetCFB(dst);
			OverVal = (src ^ dst) & 0x80;
			break;
		case 0x18:	// RCR eb, 1
			dst = ((CF << 8) + src) >> 1;
			PutbackRMByte(ModRM, dst);
			CarryVal = src & 1;
			OverVal = (src ^ dst) & 0x80;
			break;
		case 0x20:	// SHL eb, 1
		case 0x30:
			dst = src << 1;
			PutbackRMByte(ModRM, dst);
			SetCFB(dst);
			OverVal = (src ^ dst) & 0x80;
			AuxVal = 1;
			SetSZPF_Byte(dst);
			break;
		case 0x28:	// SHR eb, 1
			dst = src >> 1;
			PutbackRMByte(ModRM, dst);
			CarryVal = src & 1;
			OverVal = src & 0x80;
			AuxVal = 1;
			SetSZPF_Byte(dst);
			break;
		case 0x38:	// SAR eb, 1
			dst = ((int8)src) >> 1;
			PutbackRMByte(ModRM, dst);
			CarryVal = src & 1;
			OverVal = 0;
			AuxVal = 1;
			SetSZPF_Byte(dst);
			break;
		}
	}
	else {
		count -= (ModRM >= 0xc0) ? cycles.rot_reg_base + cycles.rot_reg_bit : cycles.rot_m8_base + cycles.rot_m8_bit;
		
		switch(ModRM & 0x38)
		{
		case 0x00:	// ROL eb, cnt
			for(; cnt > 0; cnt--) {
				CarryVal = dst & 0x80;
				dst = (dst << 1) + CF;
			}
			PutbackRMByte(ModRM, (uint8)dst);
			break;
		case 0x08:	// ROR eb, cnt
			for(; cnt > 0; cnt--) {
				CarryVal = dst & 1;
				dst = (dst >> 1) + (CF << 7);
			}
			PutbackRMByte(ModRM, (uint8)dst);
			break;
		case 0x10:	// RCL eb, cnt
			for(; cnt > 0; cnt--) {
				dst = (dst << 1) + CF;
				SetCFB(dst);
			}
			PutbackRMByte(ModRM, (uint8)dst);
			break;
		case 0x18:	// RCR eb, cnt
			for(; cnt > 0; cnt--) {
				dst = (CF << 8) + dst;
				CarryVal = dst & 1;
				dst >>= 1;
			}
			PutbackRMByte(ModRM, (uint8)dst);
			break;
		case 0x20:
		case 0x30:	// SHL eb, cnt
			dst <<= cnt;
			SetCFB(dst);
			AuxVal = 1;
			SetSZPF_Byte(dst);
			PutbackRMByte(ModRM, (uint8)dst);
			break;
		case 0x28:	// SHR eb, cnt
			dst >>= cnt - 1;
			CarryVal = dst & 1;
			dst >>= 1;
			SetSZPF_Byte(dst);
			AuxVal = 1;
			PutbackRMByte(ModRM, (uint8)dst);
			break;
		case 0x38:	// SAR eb, cnt
			dst = ((int8)dst) >> (cnt - 1);
			CarryVal = dst & 1;
			dst = ((int8)((uint8)dst)) >> 1;
			SetSZPF_Byte(dst);
			AuxVal = 1;
			PutbackRMByte(ModRM, (uint8)dst);
			break;
		}
	}
}

void X86::rotate_shift_word(unsigned ModRM, unsigned cnt)
{
	unsigned src = GetRMWord(ModRM);
	unsigned dst = src;
	
	if(cnt == 0) {
		count -= (ModRM >= 0xc0) ? cycles.rot_reg_base : cycles.rot_m16_base;
	}
	else if(cnt == 1) {
		count -= (ModRM >= 0xc0) ? cycles.rot_reg_1 : cycles.rot_m16_1;
		
		switch(ModRM & 0x38)
		{
		case 0x00:	// ROL ew, 1
			CarryVal = src & 0x8000;
			dst = (src << 1) + CF;
			PutbackRMWord(ModRM, dst);
			OverVal = (src ^ dst) & 0x8000;
			break;
		case 0x08:	// ROR ew, 1
			CarryVal = src & 1;
			dst = ((CF << 16) + src) >> 1;
			PutbackRMWord(ModRM, dst);
			OverVal = (src ^ dst) & 0x8000;
			break;
		case 0x10:	// RCL ew, 1
			dst = (src << 1) + CF;
			PutbackRMWord(ModRM, dst);
			SetCFW(dst);
			OverVal = (src ^ dst) & 0x8000;
			break;
		case 0x18:	// RCR ew, 1
			dst = ((CF << 16) + src) >> 1;
			PutbackRMWord(ModRM, dst);
			CarryVal = src & 1;
			OverVal = (src ^ dst) & 0x8000;
			break;
		case 0x20:	// SHL ew, 1
		case 0x30:
			dst = src << 1;
			PutbackRMWord(ModRM, dst);
			SetCFW(dst);
			OverVal = (src ^ dst) & 0x8000;
			AuxVal = 1;
			SetSZPF_Word(dst);
			break;
		case 0x28:	// SHR ew, 1
			dst = src >> 1;
			PutbackRMWord(ModRM, dst);
			CarryVal = src & 1;
			OverVal = src & 0x8000;
			AuxVal = 1;
			SetSZPF_Word(dst);
			break;
		case 0x38:	// SAR ew, 1
			dst = ((int16)src) >> 1;
			PutbackRMWord(ModRM, dst);
			CarryVal = src & 1;
			OverVal = 0;
			AuxVal = 1;
			SetSZPF_Word(dst);
			break;
		}
	}
	else {
		count -= (ModRM >= 0xc0) ? cycles.rot_reg_base + cycles.rot_reg_bit : cycles.rot_m8_base + cycles.rot_m16_bit;
		
		switch(ModRM & 0x38)
		{
		case 0x00:	// ROL ew, cnt
			for(; cnt > 0; cnt--) {
				CarryVal = dst & 0x8000;
				dst = (dst << 1) + CF;
			}
			PutbackRMWord(ModRM, dst);
			break;
		case 0x08:	// ROR ew, cnt
			for(; cnt > 0; cnt--) {
				CarryVal = dst & 1;
				dst = (dst >> 1) + (CF << 15);
			}
			PutbackRMWord(ModRM, dst);
			break;
		case 0x10:	// RCL ew, cnt
			for(; cnt > 0; cnt--) {
				dst = (dst << 1) + CF;
				SetCFW(dst);
			}
			PutbackRMWord(ModRM, dst);
			break;
		case 0x18:	// RCR ew, cnt
			for(; cnt > 0; cnt--) {
				dst = dst + (CF << 16);
				CarryVal = dst & 1;
				dst >>= 1;
			}
			PutbackRMWord(ModRM, dst);
			break;
		case 0x20:
		case 0x30:	// SHL ew, cnt
			dst <<= cnt;
			SetCFW(dst);
			AuxVal = 1;
			SetSZPF_Word(dst);
			PutbackRMWord(ModRM, dst);
			break;
		case 0x28:	// SHR ew, cnt
			dst >>= cnt - 1;
			CarryVal = dst & 1;
			dst >>= 1;
			SetSZPF_Word(dst);
			AuxVal = 1;
			PutbackRMWord(ModRM, dst);
			break;
		case 0x38:	// SAR ew, cnt
			dst = ((int16)dst) >> (cnt - 1);
			CarryVal = dst & 1;
			dst = ((int16)((uint16)dst)) >> 1;
			SetSZPF_Word(dst);
			AuxVal = 1;
			PutbackRMWord(ModRM, dst);
			break;
		}
	}
}

#ifdef I286
int X86::i286_selector_okay(uint16 selector)
{
	if(selector & 4)
		return (selector & ~7) < ldtr_limit;
	else
		return (selector & ~7) < gdtr_limit;
}

void X86::i286_data_descriptor(int reg, uint16 selector)
{
	if(PM) {
		if(selector & 4) {
			// local descriptor table
			if(selector > ldtr_limit)
				interrupt(GENERAL_PROTECTION_FAULT);
			sregs[reg] = selector;
			limit[reg] = RM16(ldtr_base + (selector & ~7));
			base[reg] = RM16(ldtr_base + (selector & ~7) + 2) | (RM16(ldtr_base + (selector & ~7) + 4) << 16);
			base[reg] &= 0xffffff;
		}
		else {
			// global descriptor table
			if(!(selector & ~7) || (selector > gdtr_limit))
				interrupt(GENERAL_PROTECTION_FAULT);
			sregs[reg] = selector;
			limit[reg] = RM16(gdtr_base + (selector & ~7));
			base[reg] = RM16(gdtr_base + (selector & ~7) + 2);
			uint16 tmp = RM16(gdtr_base + (selector & ~7) + 4);
			base[reg] |= (tmp & 0xff) << 16;
		}
	}
	else {
		sregs[reg] = selector;
		base[reg] = selector << 4;
	}
}

void X86::i286_code_descriptor(uint16 selector, uint16 offset)
{
	if(PM) {
		uint16 word1, word2, word3;
		if(selector & 4) {
			// local descriptor table
			if(selector > ldtr_limit)
				interrupt(GENERAL_PROTECTION_FAULT);
			word1 = RM16(ldtr_base + (selector & ~7));
			word2 = RM16(ldtr_base + (selector & ~7) + 2);
			word3 = RM16(ldtr_base + (selector & ~7) + 4);
		}
		else {
			// global descriptor table
			if(!(selector & ~7) || (selector > gdtr_limit))
				interrupt(GENERAL_PROTECTION_FAULT);
			word1 = RM16(gdtr_base + (selector & ~7));
			word2 = RM16(gdtr_base + (selector & ~7) + 2);
			word3 = RM16(gdtr_base + (selector & ~7) + 4);
		}
		if(word3 & 0x1000) {
			sregs[CS] = selector;
			limit[CS] = word1;
			base[CS] = word2 | ((word3 & 0xff) << 16);
			PC = base[CS] + offset;
		}
		else {
			// systemdescriptor
			switch(word3 & 0xf00)
			{
			case 0x400: // call gate
				i286_data_descriptor(CS, word2);
				PC = base[CS] + word1;
				break;
			case 0x500: // task gate
				i286_data_descriptor(CS, word2);
				PC = base[CS] + word1;
				break;
			case 0x600: // interrupt gate
				TF = IF = 0;
				i286_data_descriptor(CS, word2);
				PC = base[CS] + word1;
				break;
			case 0x700: // trap gate
				i286_data_descriptor(CS, word2);
				PC = base[CS] + word1;
				break;
			}
		}
	}
	else {
		sregs[CS] = selector;
		base[CS] = selector << 4;
		PC = base[CS] + offset;
	}
}
#endif

void X86::op(uint8 code)
{
	prvPC = PC - 1;
//emu->out_debug("%5x\n",prvPC);
	
	switch(code)
	{
	case 0x00: _add_br8(); break;
	case 0x01: _add_wr16(); break;
	case 0x02: _add_r8b(); break;
	case 0x03: _add_r16w(); break;
	case 0x04: _add_ald8(); break;
	case 0x05: _add_axd16(); break;
	case 0x06: _push_es(); break;
	case 0x07: _pop_es(); break;
	case 0x08: _or_br8(); break;
	case 0x09: _or_wr16(); break;
	case 0x0a: _or_r8b(); break;
	case 0x0b: _or_r16w(); break;
	case 0x0c: _or_ald8(); break;
	case 0x0d: _or_axd16(); break;
	case 0x0e: _push_cs(); break;
	case 0x0f: _op0f(); break;
	case 0x10: _adc_br8(); break;
	case 0x11: _adc_wr16(); break;
	case 0x12: _adc_r8b(); break;
	case 0x13: _adc_r16w(); break;
	case 0x14: _adc_ald8(); break;
	case 0x15: _adc_axd16(); break;
	case 0x16: _push_ss(); break;
	case 0x17: _pop_ss(); break;
	case 0x18: _sbb_br8(); break;
	case 0x19: _sbb_wr16(); break;
	case 0x1a: _sbb_r8b(); break;
	case 0x1b: _sbb_r16w(); break;
	case 0x1c: _sbb_ald8(); break;
	case 0x1d: _sbb_axd16(); break;
	case 0x1e: _push_ds(); break;
	case 0x1f: _pop_ds(); break;
	case 0x20: _and_br8(); break;
	case 0x21: _and_wr16(); break;
	case 0x22: _and_r8b(); break;
	case 0x23: _and_r16w(); break;
	case 0x24: _and_ald8(); break;
	case 0x25: _and_axd16(); break;
	case 0x26: _es(); break;
	case 0x27: _daa(); break;
	case 0x28: _sub_br8(); break;
	case 0x29: _sub_wr16(); break;
	case 0x2a: _sub_r8b(); break;
	case 0x2b: _sub_r16w(); break;
	case 0x2c: _sub_ald8(); break;
	case 0x2d: _sub_axd16(); break;
	case 0x2e: _cs(); break;
	case 0x2f: _das(); break;
	case 0x30: _xor_br8(); break;
	case 0x31: _xor_wr16(); break;
	case 0x32: _xor_r8b(); break;
	case 0x33: _xor_r16w(); break;
	case 0x34: _xor_ald8(); break;
	case 0x35: _xor_axd16(); break;
	case 0x36: _ss(); break;
	case 0x37: _aaa(); break;
	case 0x38: _cmp_br8(); break;
	case 0x39: _cmp_wr16(); break;
	case 0x3a: _cmp_r8b(); break;
	case 0x3b: _cmp_r16w(); break;
	case 0x3c: _cmp_ald8(); break;
	case 0x3d: _cmp_axd16(); break;
	case 0x3e: _ds(); break;
	case 0x3f: _aas(); break;
	case 0x40: _inc_ax(); break;
	case 0x41: _inc_cx(); break;
	case 0x42: _inc_dx(); break;
	case 0x43: _inc_bx(); break;
	case 0x44: _inc_sp(); break;
	case 0x45: _inc_bp(); break;
	case 0x46: _inc_si(); break;
	case 0x47: _inc_di(); break;
	case 0x48: _dec_ax(); break;
	case 0x49: _dec_cx(); break;
	case 0x4a: _dec_dx(); break;
	case 0x4b: _dec_bx(); break;
	case 0x4c: _dec_sp(); break;
	case 0x4d: _dec_bp(); break;
	case 0x4e: _dec_si(); break;
	case 0x4f: _dec_di(); break;
	case 0x50: _push_ax(); break;
	case 0x51: _push_cx(); break;
	case 0x52: _push_dx(); break;
	case 0x53: _push_bx(); break;
	case 0x54: _push_sp(); break;
	case 0x55: _push_bp(); break;
	case 0x56: _push_si(); break;
	case 0x57: _push_di(); break;
	case 0x58: _pop_ax(); break;
	case 0x59: _pop_cx(); break;
	case 0x5a: _pop_dx(); break;
	case 0x5b: _pop_bx(); break;
	case 0x5c: _pop_sp(); break;
	case 0x5d: _pop_bp(); break;
	case 0x5e: _pop_si(); break;
	case 0x5f: _pop_di(); break;
	case 0x60: _pusha(); break;
	case 0x61: _popa(); break;
	case 0x62: _bound(); break;
	case 0x63: _arpl(); break;
	case 0x64: _repc(0); break;
	case 0x65: _repc(1); break;
	case 0x66: _invalid(); break;
	case 0x67: _invalid(); break;
	case 0x68: _push_d16(); break;
	case 0x69: _imul_d16(); break;
	case 0x6a: _push_d8(); break;
	case 0x6b: _imul_d8(); break;
	case 0x6c: _insb(); break;
	case 0x6d: _insw(); break;
	case 0x6e: _outsb(); break;
	case 0x6f: _outsw(); break;
	case 0x70: _jo(); break;
	case 0x71: _jno(); break;
	case 0x72: _jb(); break;
	case 0x73: _jnb(); break;
	case 0x74: _jz(); break;
	case 0x75: _jnz(); break;
	case 0x76: _jbe(); break;
	case 0x77: _jnbe(); break;
	case 0x78: _js(); break;
	case 0x79: _jns(); break;
	case 0x7a: _jp(); break;
	case 0x7b: _jnp(); break;
	case 0x7c: _jl(); break;
	case 0x7d: _jnl(); break;
	case 0x7e: _jle(); break;
	case 0x7f: _jnle(); break;
	case 0x80: _op80(); break;
	case 0x81: _op81(); break;
	case 0x82: _op82(); break;
	case 0x83: _op83(); break;
	case 0x84: _test_br8(); break;
	case 0x85: _test_wr16(); break;
	case 0x86: _xchg_br8(); break;
	case 0x87: _xchg_wr16(); break;
	case 0x88: _mov_br8(); break;
	case 0x89: _mov_wr16(); break;
	case 0x8a: _mov_r8b(); break;
	case 0x8b: _mov_r16w(); break;
	case 0x8c: _mov_wsreg(); break;
	case 0x8d: _lea(); break;
	case 0x8e: _mov_sregw(); break;
	case 0x8f: _popw(); break;
	case 0x90: _nop(); break;
	case 0x91: _xchg_axcx(); break;
	case 0x92: _xchg_axdx(); break;
	case 0x93: _xchg_axbx(); break;
	case 0x94: _xchg_axsp(); break;
	case 0x95: _xchg_axbp(); break;
	case 0x96: _xchg_axsi(); break;
	case 0x97: _xchg_axdi(); break;
	case 0x98: _cbw(); break;
	case 0x99: _cwd(); break;
	case 0x9a: _call_far(); break;
	case 0x9b: _wait(); break;
	case 0x9c: _pushf(); break;
	case 0x9d: _popf(); break;
	case 0x9e: _sahf(); break;
	case 0x9f: _lahf(); break;
	case 0xa0: _mov_aldisp(); break;
	case 0xa1: _mov_axdisp(); break;
	case 0xa2: _mov_dispal(); break;
	case 0xa3: _mov_dispax(); break;
	case 0xa4: _movsb(); break;
	case 0xa5: _movsw(); break;
	case 0xa6: _cmpsb(); break;
	case 0xa7: _cmpsw(); break;
	case 0xa8: _test_ald8(); break;
	case 0xa9: _test_axd16(); break;
	case 0xaa: _stosb(); break;
	case 0xab: _stosw(); break;
	case 0xac: _lodsb(); break;
	case 0xad: _lodsw(); break;
	case 0xae: _scasb(); break;
	case 0xaf: _scasw(); break;
	case 0xb0: _mov_ald8(); break;
	case 0xb1: _mov_cld8(); break;
	case 0xb2: _mov_dld8(); break;
	case 0xb3: _mov_bld8(); break;
	case 0xb4: _mov_ahd8(); break;
	case 0xb5: _mov_chd8(); break;
	case 0xb6: _mov_dhd8(); break;
	case 0xb7: _mov_bhd8(); break;
	case 0xb8: _mov_axd16(); break;
	case 0xb9: _mov_cxd16(); break;
	case 0xba: _mov_dxd16(); break;
	case 0xbb: _mov_bxd16(); break;
	case 0xbc: _mov_spd16(); break;
	case 0xbd: _mov_bpd16(); break;
	case 0xbe: _mov_sid16(); break;
	case 0xbf: _mov_did16(); break;
	case 0xc0: _rotshft_bd8(); break;
	case 0xc1: _rotshft_wd8(); break;
	case 0xc2: _ret_d16(); break;
	case 0xc3: _ret(); break;
	case 0xc4: _les_dw(); break;
	case 0xc5: _lds_dw(); break;
	case 0xc6: _mov_bd8(); break;
	case 0xc7: _mov_wd16(); break;
	case 0xc8: _enter(); break;
	case 0xc9: _leav(); break;	// _leave()
	case 0xca: _retf_d16(); break;
	case 0xcb: _retf(); break;
	case 0xcc: _int3(); break;
	case 0xcd: _int(); break;
	case 0xce: _into(); break;
	case 0xcf: _iret(); break;
	case 0xd0: _rotshft_b(); break;
	case 0xd1: _rotshft_w(); break;
	case 0xd2: _rotshft_bcl(); break;
	case 0xd3: _rotshft_wcl(); break;
	case 0xd4: _aam(); break;
	case 0xd5: _aad(); break;
	case 0xd6: _setalc(); break;
	case 0xd7: _xlat(); break;
	case 0xd8: _escape(); break;
	case 0xd9: _escape(); break;
	case 0xda: _escape(); break;
	case 0xdb: _escape(); break;
	case 0xdc: _escape(); break;
	case 0xdd: _escape(); break;
	case 0xde: _escape(); break;
	case 0xdf: _escape(); break;
	case 0xe0: _loopne(); break;
	case 0xe1: _loope(); break;
	case 0xe2: _loop(); break;
	case 0xe3: _jcxz(); break;
	case 0xe4: _inal(); break;
	case 0xe5: _inax(); break;
	case 0xe6: _outal(); break;
	case 0xe7: _outax(); break;
	case 0xe8: _call_d16(); break;
	case 0xe9: _jmp_d16(); break;
	case 0xea: _jmp_far(); break;
	case 0xeb: _jmp_d8(); break;
	case 0xec: _inaldx(); break;
	case 0xed: _inaxdx(); break;
	case 0xee: _outdxal(); break;
	case 0xef: _outdxax(); break;
	case 0xf0: _lock(); break;
	case 0xf1: _invalid(); break;
	case 0xf2: _rep(0); break;	// repne
	case 0xf3: _rep(1); break;	// repe
	case 0xf4: _hlt(); break;
	case 0xf5: _cmc(); break;
	case 0xf6: _opf6(); break;
	case 0xf7: _opf7(); break;
	case 0xf8: _clc(); break;
	case 0xf9: _stc(); break;
	case 0xfa: _cli(); break;
	case 0xfb: _sti(); break;
	case 0xfc: _cld(); break;
	case 0xfd: _std(); break;
	case 0xfe: _opfe(); break;
	case 0xff: _opff(); break;
	}
};

inline void X86::_add_br8()	// Opcode 0x00
{
	DEF_br8(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_mr8;
	ADDB(dst, src);
	PutbackRMByte(ModRM, dst);
}

inline void X86::_add_wr16()	// Opcode 0x01
{
	DEF_wr16(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_mr16;
	ADDW(dst, src);
	PutbackRMWord(ModRM, dst);
}

inline void X86::_add_r8b()	// Opcode 0x02
{
	DEF_r8b(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_rm8;
	ADDB(dst, src);
	RegByte(ModRM) = dst;
}

inline void X86::_add_r16w()	// Opcode 0x03
{
	DEF_r16w(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_rm16;
	ADDW(dst, src);
	RegWord(ModRM) = dst;
}

inline void X86::_add_ald8()	// Opcode 0x04
{
	DEF_ald8(dst, src);
	count -= cycles.alu_ri8;
	ADDB(dst, src);
	regs.b[AL] = dst;
}

inline void X86::_add_axd16()	// Opcode 0x05
{
	DEF_axd16(dst, src);
	count -= cycles.alu_ri16;
	ADDW(dst, src);
	regs.w[AX] = dst;
}

inline void X86::_push_es()	// Opcode 0x06
{
	count -= cycles.push_seg;
	PUSH16(sregs[ES]);
}

inline void X86::_pop_es()	// Opcode 0x07
{
	sregs[ES] = POP16();
	base[ES] = SegBase(ES);
	count -= cycles.pop_seg;
}

inline void X86::_or_br8()	// Opcode 0x08
{
	DEF_br8(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_mr8;
	ORB(dst, src);
	PutbackRMByte(ModRM, dst);
}

inline void X86::_or_wr16()	// Opcode 0x09
{
	DEF_wr16(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_mr16;
	ORW(dst, src);
	PutbackRMWord(ModRM, dst);
}

inline void X86::_or_r8b()	// Opcode 0x0a
{
	DEF_r8b(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_rm8;
	ORB(dst, src);
	RegByte(ModRM) = dst;
}

inline void X86::_or_r16w()	// Opcode 0x0b
{
	DEF_r16w(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_rm16;
	ORW(dst, src);
	RegWord(ModRM) = dst;
}

inline void X86::_or_ald8()	// Opcode 0x0c
{
	DEF_ald8(dst, src);
	count -= cycles.alu_ri8;
	ORB(dst, src);
	regs.b[AL] = dst;
}

inline void X86::_or_axd16()	// Opcode 0x0d
{
	DEF_axd16(dst, src);
	count -= cycles.alu_ri16;
	ORW(dst, src);
	regs.w[AX] = dst;
}

inline void X86::_push_cs()	// Opcode 0x0e
{
	count -= cycles.push_seg;
	PUSH16(sregs[CS]);
}

inline void X86::_op0f()
{
#ifdef I286
	unsigned next = FETCHOP();
	uint16 ModRM, tmp;
	uint32 addr;
	
	switch(next)
	{
	case 0:
		ModRM = FETCHOP();
		switch(ModRM & 0x38)
		{
		case 0:	// sldt
			if(!PM)
				interrupt(ILLEGAL_INSTRUCTION);
			PutRMWord(ModRM, ldtr_sel);
			count -= 2;
			break;
		case 8:	// str
			if(!PM)
				interrupt(ILLEGAL_INSTRUCTION);
			PutRMWord(ModRM, tr_sel);
			count -= 2;
			break;
		case 0x10:	// lldt
			if(!PM)
				interrupt(ILLEGAL_INSTRUCTION);
			if(PM && (CPL != 0))
				interrupt(GENERAL_PROTECTION_FAULT);
			ldtr_sel = GetRMWord(ModRM);
			if((ldtr_sel & ~7) >= gdtr_limit)
				interrupt(GENERAL_PROTECTION_FAULT);
			ldtr_limit = RM16(gdtr_base + (ldtr_sel & ~7));
			ldtr_base = RM16(gdtr_base + (ldtr_sel & ~7) + 2) | (RM16(gdtr_base + (ldtr_sel & ~7) + 4) << 16);
			ldtr_base &= 0xffffff;
			count -= 24;
			break;
		case 0x18:	// ltr
			if(!PM)
				interrupt(ILLEGAL_INSTRUCTION);
			if(CPL!= 0)
				interrupt(GENERAL_PROTECTION_FAULT);
			tr_sel = GetRMWord(ModRM);
			if((tr_sel & ~7) >= gdtr_limit)
				interrupt(GENERAL_PROTECTION_FAULT);
			tr_limit = RM16(gdtr_base + (tr_sel & ~7));
			tr_base = RM16(gdtr_base + (tr_sel & ~7) + 2) | (RM16(gdtr_base + (tr_sel & ~7) + 4) << 16);
			tr_base &= 0xffffff;
			count -= 27;
			break;
		case 0x20:	// verr
			if(!PM)
				interrupt(ILLEGAL_INSTRUCTION);
			tmp = GetRMWord(ModRM);
			if(tmp & 4)
				ZeroVal = (((tmp & ~7) < ldtr_limit) && READABLE(RM8(ldtr_base + (tmp & ~7) + 5)));
			else
				ZeroVal = (((tmp & ~7) < gdtr_limit) && READABLE(RM8(gdtr_base + (tmp & ~7) + 5)));
			count -= 11;
			break;
		case 0x28: // verw
			if(!PM)
				interrupt(ILLEGAL_INSTRUCTION);
			tmp = GetRMWord(ModRM);
			if(tmp & 4)
				ZeroVal = (((tmp & ~7) < ldtr_limit) && WRITEABLE(RM8(ldtr_base + (tmp & ~7) + 5)));
			else
				ZeroVal = (((tmp & ~7) < gdtr_limit) && WRITEABLE(RM8(gdtr_base + (tmp & ~7) + 5)));
			count -= 16;
			break;
		default:
			interrupt(ILLEGAL_INSTRUCTION);
			break;
		}
		break;
	case 1:
		ModRM = FETCHOP();
		switch(ModRM & 0x38)
		{
		case 0:	// sgdt
			PutRMWord(ModRM, gdtr_limit);
			PutRMWordOfs(2, gdtr_base & 0xffff);
			PutRMByteOfs(4, gdtr_base >> 16);
			count -= 9;
			break;
		case 8:	// sidt
			PutRMWord(ModRM, idtr_limit);
			PutRMWordOfs(2, idtr_base & 0xffff);
			PutRMByteOfs(4, idtr_base >> 16);
			count -= 9;
			break;
		case 0x10:	// lgdt
			if(PM && (CPL!= 0))
				interrupt(GENERAL_PROTECTION_FAULT);
			gdtr_limit = GetRMWord(ModRM);
			gdtr_base = GetRMWordOfs(2) | (GetRMByteOfs(4) << 16);
			break;
		case 0x18:	// lidt
			if(PM && (CPL!= 0))
				interrupt(GENERAL_PROTECTION_FAULT);
			idtr_limit = GetRMWord(ModRM);
			idtr_base = GetRMWordOfs(2) | (GetRMByteOfs(4) << 16);
			count -= 11;
			break;
		case 0x20:	// smsw
			PutRMWord(ModRM, msw);
			count -= 16;
			break;
		case 0x30:	// lmsw
			if(PM && (CPL!= 0))
				interrupt(GENERAL_PROTECTION_FAULT);
			msw = (msw & 1) | GetRMWord(ModRM);
			count -= 13;
			break;
		default:
			interrupt(ILLEGAL_INSTRUCTION);
			break;
		}
		break;
	case 2:	// LAR
		ModRM = FETCHOP();
		tmp = GetRMWord(ModRM);
		ZeroVal = i286_selector_okay(tmp);
		if(ZeroVal)
			RegWord(ModRM) = tmp;
		count -= 16;
		break;
	case 3:	// LSL
		if(!PM)
			interrupt(ILLEGAL_INSTRUCTION);
		ModRM = FETCHOP();
		tmp = GetRMWord(ModRM);
		ZeroVal = i286_selector_okay(tmp);
		if(ZeroVal) {
			if(tmp & 4)
				addr = ldtr_base + (tmp & ~7);
			else
				addr = gdtr_base + (tmp & ~7);
			RegWord(ModRM) = RM16(addr);
		}
		count -= 26;
		break;
	case 6:	// clts
		if(PM && (CPL!= 0))
			interrupt(GENERAL_PROTECTION_FAULT);
		msw = ~8;
		count -= 5;
		break;
	default:
		interrupt(ILLEGAL_INSTRUCTION);
		break;
	}
#elif defined(V30)
	unsigned code = FETCH8();
	unsigned ModRM, tmp1, tmp2;
	
	switch(code)
	{
	case 0x10:	// 0F 10 47 30 - TEST1 [bx+30h], cl
		ModRM = FETCH8();
		if(ModRM >= 0xc0) {
			tmp1 = regs.b[mod_rm8[ModRM]];
			count -= 3;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM8(EA);
			count = old - 12;
		}
		tmp2 = regs.b[CL] & 7;
		ZeroVal = tmp1 & bytes[tmp2] ? 1 : 0;
		// SetZF(tmp1 & (1 << tmp2));
		break;
	case 0x11:	// 0F 11 47 30 - TEST1 [bx+30h], cl
		ModRM = FETCH8();
		// tmp1 = GetRMWord(ModRM);
		if(ModRM >= 0xc0) {
			tmp1 = regs.w[mod_rm16[ModRM]];
			count -= 3;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM16(EA);
			count = old - 12;
		}
		tmp2 = regs.b[CL] & 0xf;
		ZeroVal = tmp1 & bytes[tmp2] ? 1 : 0;
		// SetZF(tmp1 & (1 << tmp2));
		break;
	case 0x12:	// 0F 12 [mod:000:r/m] - CLR1 reg/m8, cl
		ModRM = FETCH8();
		if(ModRM >= 0xc0) {
			tmp1 = regs.b[mod_rm8[ModRM]];
			count -= 5;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM8(EA);
			count = old - 14;
		}
		tmp2 = regs.b[CL] & 7;
		tmp1 &= ~(bytes[tmp2]);
		PutbackRMByte(ModRM, tmp1);
		break;
	case 0x13:	// 0F 13 [mod:000:r/m] - CLR1 reg/m16, cl
		ModRM = FETCH8();
		// tmp1 = GetRMWord(ModRM);
		if(ModRM >= 0xc0) {
			tmp1 = regs.w[mod_rm16[ModRM]];
			count -= 5;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM16(EA);
			count = old - 14;
		}
		tmp2 = regs.b[CL] & 0xf;
		tmp1 &= ~(bytes[tmp2]);
		PutbackRMWord(ModRM, tmp1);
		break;
	case 0x14:	// 0F 14 47 30 - SET1 [bx+30h], cl
		ModRM = FETCH8();
		if(ModRM >= 0xc0) {
			tmp1 = regs.b[mod_rm8[ModRM]];
			count -= 4;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM8(EA);
			count = old - 13;
		}
		tmp2 = regs.b[CL] & 7;
		tmp1 |= (bytes[tmp2]);
		PutbackRMByte(ModRM, tmp1);
		break;
	case 0x15:	// 0F 15 C6 - SET1 si, cl
		ModRM = FETCH8();
		// tmp1 = GetRMWord(ModRM);
		if(ModRM >= 0xc0) {
			tmp1 = regs.w[mod_rm16[ModRM]];
			count -= 4;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM16(EA);
			count = old - 13;
		}
		tmp2 = regs.b[CL] & 0xf;
		tmp1 |= (bytes[tmp2]);
		PutbackRMWord(ModRM, tmp1);
		break;
	case 0x16:	// 0F 16 C6 - NOT1 si, cl
		ModRM = FETCH8();
		if(ModRM >= 0xc0) {
			tmp1 = regs.b[mod_rm8[ModRM]];
			count -= 4;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM8(EA);
			count = old - 18;
		}
		tmp2 = regs.b[CL] & 7;
		if(tmp1 & bytes[tmp2])
			tmp1 &= ~(bytes[tmp2]);
		else
			tmp1 |= (bytes[tmp2]);
		PutbackRMByte(ModRM, tmp1);
		break;
	case 0x17:	// 0F 17 C6 - NOT1 si, cl
		ModRM = FETCH8();
		// tmp1 = GetRMWord(ModRM);
		if(ModRM >= 0xc0) {
			tmp1 = regs.w[mod_rm16[ModRM]];
			count -= 4;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM16(EA);
			count = old - 18;
		}
		tmp2 = regs.b[CL] & 0xf;
		if(tmp1 & bytes[tmp2])
			tmp1 &= ~(bytes[tmp2]);
		else
			tmp1 |= (bytes[tmp2]);
		PutbackRMWord(ModRM, tmp1);
		break;
	case 0x18:	// 0F 18 XX - TEST1 [bx+30h], 07
		ModRM = FETCH8();
		// tmp1 = GetRMByte(ModRM);
		if(ModRM >= 0xc0) {
			tmp1 = regs.b[mod_rm8[ModRM]];
			count -= 4;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM8(EA);
			count = old - 13;
		}
		tmp2 = FETCH8();
		tmp2 &= 0xf;
		ZeroVal = tmp1 & (bytes[tmp2]) ? 1 : 0;
		// SetZF(tmp1 & (1 << tmp2));
		break;
	case 0x19:	// 0F 19 XX - TEST1 [bx+30h], 07
		ModRM = FETCH8();
		// tmp1 = GetRMWord(ModRM);
		if(ModRM >= 0xc0) {
			tmp1 = regs.w[mod_rm16[ModRM]];
			count -= 4;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM16(EA);
			count = old - 13;
		}
		tmp2 = FETCH8();
		tmp2 &= 0xf;
		ZeroVal = tmp1 & (bytes[tmp2]) ? 1 : 0;
		// SetZF(tmp1 & (1 << tmp2));
		break;
	case 0x1a:	// 0F 1A 06 - CLR1 si, cl
		ModRM = FETCH8();
		// tmp1 = GetRMByte(ModRM);
		if(ModRM >= 0xc0) {
			tmp1 = regs.b[mod_rm8[ModRM]];
			count -= 6;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM8(EA);
			count = old - 15;
		}
		tmp2 = FETCH8();
		tmp2 &= 7;
		tmp1 &= ~(bytes[tmp2]);
		PutbackRMByte(ModRM, tmp1);
		break;
	case 0x1B:	// 0F 1B 06 - CLR1 si, cl
		ModRM = FETCH8();
		// tmp1 = GetRMWord(ModRM);
		if(ModRM >= 0xc0) {
			tmp1 = regs.w[mod_rm16[ModRM]];
			count -= 6;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM16(EA);
			count = old - 15;
		}
		tmp2 = FETCH8();
		tmp2 &= 0xf;
		tmp1 &= ~(bytes[tmp2]);
		PutbackRMWord(ModRM, tmp1);
		break;
	case 0x1C:	// 0F 1C 47 30 - SET1 [bx+30h], cl
		ModRM = FETCH8();
		// tmp1 = GetRMByte(ModRM);
		if(ModRM >= 0xc0) {
			tmp1 = regs.b[mod_rm8[ModRM]];
			count -= 5;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM8(EA);
			count = old - 14;
		}
		tmp2 = FETCH8();
		tmp2 &= 7;
		tmp1 |= (bytes[tmp2]);
		PutbackRMByte(ModRM, tmp1);
		break;
	case 0x1D:	// 0F 1D C6 - SET1 si, cl
		ModRM = FETCH8();
		if(ModRM >= 0xc0) {
			tmp1 = regs.w[mod_rm16[ModRM]];
			count -= 5;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM16(EA);
			count = old - 14;
		}
		tmp2 = FETCH8();
		tmp2 &= 0xf;
		tmp1 |= (bytes[tmp2]);
		PutbackRMWord(ModRM, tmp1);
		break;
	case 0x1e:	// 0F 1e C6 - NOT1 si, 07
		ModRM = FETCH8();
		// tmp1 = GetRMByte(ModRM);
		if(ModRM >= 0xc0) {
			tmp1 = regs.b[mod_rm8[ModRM]];
			count -= 5;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM8(EA);
			count = old - 19;
		}
		tmp2 = FETCH8();
		tmp2 &= 7;
		if(tmp1 & bytes[tmp2])
			tmp1 &= ~(bytes[tmp2]);
		else
			tmp1 |= (bytes[tmp2]);
		PutbackRMByte(ModRM, tmp1);
		break;
	case 0x1f:	// 0F 1f C6 - NOT1 si, 07
		ModRM = FETCH8();
		//tmp1 = GetRMWord(ModRM);
		if(ModRM >= 0xc0) {
			tmp1 = regs.w[mod_rm16[ModRM]];
			count -= 5;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM16(EA);
			count = old - 19;
		}
		tmp2 = FETCH8();
		tmp2 &= 0xf;
		if(tmp1 & bytes[tmp2])
			tmp1 &= ~(bytes[tmp2]);
		else
			tmp1 |= (bytes[tmp2]);
		PutbackRMWord(ModRM, tmp1);
		break;
	case 0x20:	// 0F 20 59 - add4s
		{
			// length in words !
			int cnt = (regs.b[CL] + 1) / 2;
			unsigned di = regs.w[DI];
			unsigned si = regs.w[SI];
			
			ZeroVal = 1;
			CarryVal = 0;	// NOT ADC
			for(int i = 0; i < cnt; i++) {
				tmp1 = RM8(DS, si);
				tmp2 = RM8(ES, di);
				int v1 = (tmp1 >> 4) * 10 + (tmp1 & 0xf);
				int v2 = (tmp2 >> 4) * 10 + (tmp2 & 0xf);
				int result = v1 + v2 + CarryVal;
				CarryVal = result > 99 ? 1 : 0;
				result = result % 100;
				v1 = ((result / 10) << 4) | (result % 10);
				WM8(ES, di, v1);
				if(v1)
					ZeroVal = 0;
				si++;
				di++;
			}
			OverVal = CarryVal;
			count -= 7 + 19 * cnt;
		}
		break;
	case 0x22:	// 0F 22 59 - sub4s
		{
			int cnt = (regs.b[CL] + 1) / 2;
			unsigned di = regs.w[DI];
			unsigned si = regs.w[SI];
			
			ZeroVal = 1;
			CarryVal = 0;	// NOT ADC
			for(int i = 0; i < cnt; i++) {
				tmp1 = RM8(ES, di);
				tmp2 = RM8(DS, si);
				int v1 = (tmp1 >> 4) * 10 + (tmp1 & 0xf);
				int v2 = (tmp2 >> 4) * 10 + (tmp2 & 0xf), result;
				if(v1 < (v2 + CarryVal)) {
					v1 += 100;
					result = v1 - (v2 + CarryVal);
					CarryVal = 1;
				}
				else {
					result = v1 - (v2 + CarryVal);
					CarryVal = 0;
				}
				v1 = ((result / 10) << 4) | (result % 10);
				WM8(ES, di, v1);
				if(v1)
					ZeroVal = 0;
				si++;
				di++;
			}
			OverVal = CarryVal;
			count -= 7 + 19 * cnt;
		}
		break;
	case 0x25:
		count -= 16;
		break;
	case 0x26:	// 0F 22 59 - cmp4s
		{
			int cnt = (regs.b[CL] + 1) / 2;
			unsigned di = regs.w[DI];
			unsigned si = regs.w[SI];
			
			ZeroVal = 1;
			CarryVal = 0;	// NOT ADC
			for(int i = 0; i < cnt; i++) {
				tmp1 = RM8(ES, di);
				tmp2 = RM8(DS, si);
				int v1 = (tmp1 >> 4) * 10 + (tmp1 & 0xf);
				int v2 = (tmp2 >> 4) * 10 + (tmp2 & 0xf), result;
				if(v1 < (v2 + CarryVal)) {
					v1 += 100;
					result = v1 - (v2 + CarryVal);
					CarryVal = 1;
				}
				else {
					result = v1 - (v2 + CarryVal);
					CarryVal = 0;
				}
				v1 = ((result / 10) << 4) | (result % 10);
				// WM8(ES, di, v1);	// no store, only compare
				if(v1)
					ZeroVal = 0;
				si++;
				di++;
			}
			OverVal = CarryVal;
			count -= 7 + 19 * (regs.b[CL] + 1);
		}
		break;
	case 0x28:	// 0F 28 C7 - ROL4 bh
		ModRM = FETCH8();
		// tmp1 = GetRMByte(ModRM);
		if(ModRM >= 0xc0) {
			tmp1 = regs.b[mod_rm8[ModRM]];
			count -= 25;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM8(EA);
			count = old - 28;
		}
		tmp1 <<= 4;
		tmp1 |= regs.b[AL] & 0xf;
		regs.b[AL] = (regs.b[AL] & 0xf0) | ((tmp1 >> 8) & 0xf);
		tmp1 &= 0xff;
		PutbackRMByte(ModRM, tmp1);
		break;
	case 0x29:	// 0F 29 C7 - ROL4 bx
		ModRM = FETCH8();
		break;
	case 0x2A:	// 0F 2a c2 - ROR4 bh
		ModRM = FETCH8();
		if(ModRM >= 0xc0) {
			tmp1 = regs.b[mod_rm8[ModRM]];
			count -= 29;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM8(EA);
			count = old - 33;
		}
		tmp2 = (regs.b[AL] & 0xf) << 4;
		regs.b[AL] = (regs.b[AL] & 0xf0) | (tmp1 & 0xf);
		tmp1 = tmp2 | (tmp1 >> 4);
		PutbackRMByte(ModRM, tmp1);
		break;
	case 0x2B:	// 0F 2b c2 - ROR4 bx
		ModRM = FETCH8();
		break;
	case 0x2D:	// 0Fh 2Dh < 1111 1RRR>
		ModRM = FETCH8();
		count -= 15;
		break;
	case 0x31:	// 0F 31 [mod:reg:r/m] - INS reg8, reg8 or INS reg8, imm4
		ModRM = FETCH8();
		if(ModRM >= 0xc0) {
			tmp1 = regs.b[mod_rm8[ModRM]];
			count -= 29;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM8(EA);
			count = old - 33;
		}
		break;
	case 0x33:	// 0F 33 [mod:reg:r/m] - EXT reg8, reg8 or EXT reg8, imm4
		ModRM = FETCH8();
		if(ModRM >= 0xc0) {
			tmp1 = regs.b[mod_rm8[ModRM]];
			count -= 29;
		}
		else {
			int old = count;
			GetEA(ModRM);
			tmp1 = RM8(EA);
			count = old - 33;
		}
		break;
	case 0x91:
		count -= 12;
		break;
	case 0x94:
		ModRM = FETCH8();
		count -= 11;
		break;
	case 0x95:
		ModRM = FETCH8();
		count -= 11;
		break;
	case 0xbe:
		count -= 2;
		break;
	case 0xe0:
		ModRM = FETCH8();
		count -= 12;
		break;
	case 0xf0:
		ModRM = FETCH8();
		count -= 12;
		break;
	case 0xff:	// 0F ff imm8 - BRKEM
		ModRM = FETCH8();
		count -= 38;
		interrupt(ModRM);
		break;
	}
#else
	_invalid();
#endif
}

inline void X86::_adc_br8()	// Opcode 0x10
{
	DEF_br8(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_mr8;
	src += CF;
	ADDB(dst, src);
	PutbackRMByte(ModRM, dst);
}

inline void X86::_adc_wr16()	// Opcode 0x11
{
	DEF_wr16(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_mr16;
	src += CF;
	ADDW(dst, src);
	PutbackRMWord(ModRM, dst);
}

inline void X86::_adc_r8b()	// Opcode 0x12
{
	DEF_r8b(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_rm8;
	src += CF;
	ADDB(dst, src);
	RegByte(ModRM) = dst;
}

inline void X86::_adc_r16w()	// Opcode 0x13
{
	DEF_r16w(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_rm16;
	src += CF;
	ADDW(dst, src);
	RegWord(ModRM) = dst;
}

inline void X86::_adc_ald8()	// Opcode 0x14
{
	DEF_ald8(dst, src);
	count -= cycles.alu_ri8;
	src += CF;
	ADDB(dst, src);
	regs.b[AL] = dst;
}

inline void X86::_adc_axd16()	// Opcode 0x15
{
	DEF_axd16(dst, src);
	count -= cycles.alu_ri16;
	src += CF;
	ADDW(dst, src);
	regs.w[AX] = dst;
}

inline void X86::_push_ss()	// Opcode 0x16
{
	PUSH16(sregs[SS]);
	count -= cycles.push_seg;
}

inline void X86::_pop_ss()	// Opcode 0x17
{
#ifdef I286
	uint16 tmp = POP16();
	i286_data_descriptor(SS, tmp);
#else
	sregs[SS] = POP16();
	base[SS] = SegBase(SS);
#endif
	count -= cycles.pop_seg;
	op(FETCHOP());
}

inline void X86::_sbb_br8()	// Opcode 0x18
{
	DEF_br8(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_mr8;
	src += CF;
	SUBB(dst, src);
	PutbackRMByte(ModRM, dst);
}

inline void X86::_sbb_wr16()	// Opcode 0x19
{
	DEF_wr16(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_mr16;
	src += CF;
	SUBW(dst, src);
	PutbackRMWord(ModRM, dst);
}

inline void X86::_sbb_r8b()	// Opcode 0x1a
{
	DEF_r8b(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_rm8;
	src += CF;
	SUBB(dst, src);
	RegByte(ModRM) = dst;
}

inline void X86::_sbb_r16w()	// Opcode 0x1b
{
	DEF_r16w(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_rm16;
	src += CF;
	SUBW(dst, src);
	RegWord(ModRM) = dst;
}

inline void X86::_sbb_ald8()	// Opcode 0x1c
{
	DEF_ald8(dst, src);
	count -= cycles.alu_ri8;
	src += CF;
	SUBB(dst, src);
	regs.b[AL] = dst;
}

inline void X86::_sbb_axd16()	// Opcode 0x1d
{
	DEF_axd16(dst, src);
	count -= cycles.alu_ri16;
	src += CF;
	SUBW(dst, src);
	regs.w[AX] = dst;
}

inline void X86::_push_ds()	// Opcode 0x1e
{
	PUSH16(sregs[DS]);
	count -= cycles.push_seg;
}

inline void X86::_pop_ds()	// Opcode 0x1f
{
#ifdef I286
	uint16 tmp = POP16();
	i286_data_descriptor(DS, tmp);
#else
	sregs[DS] = POP16();
	base[DS] = SegBase(DS);
#endif
	count -= cycles.push_seg;
}

inline void X86::_and_br8()	// Opcode 0x20
{
	DEF_br8(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_mr8;
	ANDB(dst, src);
	PutbackRMByte(ModRM, dst);
}

inline void X86::_and_wr16()	// Opcode 0x21
{
	DEF_wr16(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_mr16;
	ANDW(dst, src);
	PutbackRMWord(ModRM, dst);
}

inline void X86::_and_r8b()	// Opcode 0x22
{
	DEF_r8b(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_rm8;
	ANDB(dst, src);
	RegByte(ModRM) = dst;
}

inline void X86::_and_r16w()	// Opcode 0x23
{
	DEF_r16w(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_rm16;
	ANDW(dst, src);
	RegWord(ModRM) = dst;
}

inline void X86::_and_ald8()	// Opcode 0x24
{
	DEF_ald8(dst, src);
	count -= cycles.alu_ri8;
	ANDB(dst, src);
	regs.b[AL] = dst;
}

inline void X86::_and_axd16()	// Opcode 0x25
{
	DEF_axd16(dst, src);
	count -= cycles.alu_ri16;
	ANDW(dst, src);
	regs.w[AX] = dst;
}

inline void X86::_es()	// Opcode 0x26
{
	seg_prefix = true;
	prefix_base = base[ES];
	count -= cycles.override;
	op(FETCHOP());
}

inline void X86::_daa()	// Opcode 0x27
{
	if(AF || ((regs.b[AL] & 0xf) > 9)) {
		int tmp = regs.b[AL] + 6;
		regs.b[AL] = tmp;
		AuxVal = 1;
		CarryVal |= tmp & 0x100;
	}
	if(CF || (regs.b[AL] > 0x9f)) {
		regs.b[AL] += 0x60;
		CarryVal = 1;
	}
	SetSZPF_Byte(regs.b[AL]);
	count -= cycles.daa;
}

inline void X86::_sub_br8()	// Opcode 0x28
{
	DEF_br8(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_mr8;
	SUBB(dst, src);
	PutbackRMByte(ModRM, dst);
}

inline void X86::_sub_wr16()	// Opcode 0x29
{
	DEF_wr16(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_mr16;
	SUBW(dst, src);
	PutbackRMWord(ModRM, dst);
}

inline void X86::_sub_r8b()	// Opcode 0x2a
{
	DEF_r8b(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_rm8;
	SUBB(dst, src);
	RegByte(ModRM) = dst;
}

inline void X86::_sub_r16w()	// Opcode 0x2b
{
	DEF_r16w(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_rm16;
	SUBW(dst, src);
	RegWord(ModRM) = dst;
}

inline void X86::_sub_ald8()	// Opcode 0x2c
{
	DEF_ald8(dst, src);
	count -= cycles.alu_ri8;
	SUBB(dst, src);
	regs.b[AL] = dst;
}

inline void X86::_sub_axd16()	// Opcode 0x2d
{
	DEF_axd16(dst, src);
	count -= cycles.alu_ri16;
	SUBW(dst, src);
	regs.w[AX] = dst;
}

inline void X86::_cs()	// Opcode 0x2e
{
	seg_prefix = true;
	prefix_base = base[CS];
	count -= cycles.override;
	op(FETCHOP());
}

inline void X86::_das()	// Opcode 0x2f
{
	uint8 tmpAL = regs.b[AL];
	if(AF || ((regs.b[AL] & 0xf) > 9)) {
		int tmp = regs.b[AL] - 6;
		regs.b[AL] = tmp;
		AuxVal = 1;
		CarryVal |= tmp & 0x100;
	}
	if(CF || (tmpAL > 0x9f)) {
		regs.b[AL] -= 0x60;
		CarryVal = 1;
	}
	SetSZPF_Byte(regs.b[AL]);
	count -= cycles.das;
}

inline void X86::_xor_br8()	// Opcode 0x30
{
	DEF_br8(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_mr8;
	XORB(dst, src);
	PutbackRMByte(ModRM, dst);
}

inline void X86::_xor_wr16()	// Opcode 0x31
{
	DEF_wr16(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_mr16;
	XORW(dst, src);
	PutbackRMWord(ModRM, dst);
}

inline void X86::_xor_r8b()	// Opcode 0x32
{
	DEF_r8b(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_rm8;
	XORB(dst, src);
	RegByte(ModRM) = dst;
}

inline void X86::_xor_r16w()	// Opcode 0x33
{
	DEF_r16w(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_rm16;
	XORW(dst, src);
	RegWord(ModRM) = dst;
}

inline void X86::_xor_ald8()	// Opcode 0x34
{
	DEF_ald8(dst, src);
	count -= cycles.alu_ri8;
	XORB(dst, src);
	regs.b[AL] = dst;
}

inline void X86::_xor_axd16()	// Opcode 0x35
{
	DEF_axd16(dst, src);
	count -= cycles.alu_ri16;
	XORW(dst, src);
	regs.w[AX] = dst;
}

inline void X86::_ss()	// Opcode 0x36
{
	seg_prefix = true;
	prefix_base = base[SS];
	count -= cycles.override;
	op(FETCHOP());
}

inline void X86::_aaa()	// Opcode 0x37
{
	uint8 ALcarry = 1;
	if(regs.b[AL]>0xf9)
		ALcarry = 2;
	if(AF || ((regs.b[AL] & 0xf) > 9)) {
		regs.b[AL] += 6;
		regs.b[AH] += ALcarry;
		AuxVal = 1;
		CarryVal = 1;
	}
	else {
		AuxVal = 0;
		CarryVal = 0;
	}
	regs.b[AL] &= 0x0F;
	count -= cycles.aaa;
}

inline void X86::_cmp_br8()	// Opcode 0x38
{
	DEF_br8(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_rm8;
	SUBB(dst, src);
}

inline void X86::_cmp_wr16()	// Opcode 0x39
{
	DEF_wr16(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_rm16;
	SUBW(dst, src);
}

inline void X86::_cmp_r8b()	// Opcode 0x3a
{
	DEF_r8b(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_rm8;
	SUBB(dst, src);
}

inline void X86::_cmp_r16w()	// Opcode 0x3b
{
	DEF_r16w(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_rm16;
	SUBW(dst, src);
}

inline void X86::_cmp_ald8()	// Opcode 0x3c
{
	DEF_ald8(dst, src);
	count -= cycles.alu_ri8;
	SUBB(dst, src);
}

inline void X86::_cmp_axd16()	// Opcode 0x3d
{
	DEF_axd16(dst, src);
	count -= cycles.alu_ri16;
	SUBW(dst, src);
}

inline void X86::_ds()	// Opcode 0x3e
{
	seg_prefix = true;
	prefix_base = base[DS];
	count -= cycles.override;
	op(FETCHOP());
}

inline void X86::_aas()	// Opcode 0x3f
{
	uint8 ALcarry = 1;
	if(regs.b[AL]>0xf9)
		ALcarry = 2;
	if(AF || ((regs.b[AL] & 0xf) > 9)) {
		regs.b[AL] -= 6;
		regs.b[AH] -= 1;
		AuxVal = 1;
		CarryVal = 1;
	}
	else {
		AuxVal = 0;
		CarryVal = 0;
	}
	regs.b[AL] &= 0x0F;
	count -= cycles.aas;
}

#define IncWordReg(reg) { \
	unsigned tmp = (unsigned)regs.w[reg]; \
	unsigned tmp1 = tmp + 1; \
	SetOFW_Add(tmp1, tmp, 1); \
	SetAF(tmp1, tmp, 1); \
	SetSZPF_Word(tmp1); \
	regs.w[reg] = tmp1; \
	count -= cycles.incdec_r16; \
}

inline void X86::_inc_ax()	// Opcode 0x40
{
	IncWordReg(AX);
}

inline void X86::_inc_cx()	// Opcode 0x41
{
	IncWordReg(CX);
}

inline void X86::_inc_dx()	// Opcode 0x42
{
	IncWordReg(DX);
}

inline void X86::_inc_bx()	// Opcode 0x43
{
	IncWordReg(BX);
}

inline void X86::_inc_sp()	// Opcode 0x44
{
	IncWordReg(SP);
}

inline void X86::_inc_bp()	// Opcode 0x45
{
	IncWordReg(BP);
}

inline void X86::_inc_si()	// Opcode 0x46
{
	IncWordReg(SI);
}

inline void X86::_inc_di()	// Opcode 0x47
{
	IncWordReg(DI);
}

#define DecWordReg(reg) { \
	unsigned tmp = (unsigned)regs.w[reg]; \
	unsigned tmp1 = tmp - 1; \
	SetOFW_Sub(tmp1, 1, tmp); \
	SetAF(tmp1, tmp, 1); \
	SetSZPF_Word(tmp1); \
	regs.w[reg] = tmp1; \
	count -= cycles.incdec_r16; \
}

inline void X86::_dec_ax()	// Opcode 0x48
{
	DecWordReg(AX);
}

inline void X86::_dec_cx()	// Opcode 0x49
{
	DecWordReg(CX);
}

inline void X86::_dec_dx()	// Opcode 0x4a
{
	DecWordReg(DX);
}

inline void X86::_dec_bx()	// Opcode 0x4b
{
	DecWordReg(BX);
}

inline void X86::_dec_sp()	// Opcode 0x4c
{
	DecWordReg(SP);
}

inline void X86::_dec_bp()	// Opcode 0x4d
{
	DecWordReg(BP);
}

inline void X86::_dec_si()	// Opcode 0x4e
{
	DecWordReg(SI);
}

inline void X86::_dec_di()	// Opcode 0x4f
{
	DecWordReg(DI);
}

inline void X86::_push_ax()	// Opcode 0x50
{
	count -= cycles.push_r16;
	PUSH16(regs.w[AX]);
}

inline void X86::_push_cx()	// Opcode 0x51
{
	count -= cycles.push_r16;
	PUSH16(regs.w[CX]);
}

inline void X86::_push_dx()	// Opcode 0x52
{
	count -= cycles.push_r16;
	PUSH16(regs.w[DX]);
}

inline void X86::_push_bx()	// Opcode 0x53
{
	count -= cycles.push_r16;
	PUSH16(regs.w[BX]);
}

inline void X86::_push_sp()	// Opcode 0x54
{
	count -= cycles.push_r16;
	PUSH16(regs.w[SP]);
}

inline void X86::_push_bp()	// Opcode 0x55
{
	count -= cycles.push_r16;
	PUSH16(regs.w[BP]);
}

inline void X86::_push_si()	// Opcode 0x56
{
	count -= cycles.push_r16;
	PUSH16(regs.w[SI]);
}

inline void X86::_push_di()	// Opcode 0x57
{
	count -= cycles.push_r16;
	PUSH16(regs.w[DI]);
}

inline void X86::_pop_ax()	// Opcode 0x58
{
	count -= cycles.pop_r16;
	regs.w[AX] = POP16();
}

inline void X86::_pop_cx()	// Opcode 0x59
{
	count -= cycles.pop_r16;
	regs.w[CX] = POP16();
}

inline void X86::_pop_dx()	// Opcode 0x5a
{
	count -= cycles.pop_r16;
	regs.w[DX] = POP16();
}

inline void X86::_pop_bx()	// Opcode 0x5b
{
	count -= cycles.pop_r16;
	regs.w[BX] = POP16();
}

inline void X86::_pop_sp()	// Opcode 0x5c
{
	count -= cycles.pop_r16;
	regs.w[SP] = POP16();
}

inline void X86::_pop_bp()	// Opcode 0x5d
{
	count -= cycles.pop_r16;
	regs.w[BP] = POP16();
}

inline void X86::_pop_si()	// Opcode 0x5e
{
	count -= cycles.pop_r16;
	regs.w[SI] = POP16();
}

inline void X86::_pop_di()	// Opcode 0x5f
{
	count -= cycles.pop_r16;
	regs.w[DI] = POP16();
}

inline void X86::_pusha()	// Opcode 0x60
{
#if defined(I286) || defined(V30)
	unsigned tmp = regs.w[SP];
	count -= cycles.pusha;
	PUSH16(regs.w[AX]);
	PUSH16(regs.w[CX]);
	PUSH16(regs.w[DX]);
	PUSH16(regs.w[BX]);
	PUSH16(tmp);
	PUSH16(regs.w[BP]);
	PUSH16(regs.w[SI]);
	PUSH16(regs.w[DI]);
#else
	_invalid();
#endif
}

inline void X86::_popa()	// Opcode 0x61
{
#if defined(I286) || defined(V30)
	count -= cycles.popa;
	regs.w[DI] = POP16();
	regs.w[SI] = POP16();
	regs.w[BP] = POP16();
	unsigned tmp = POP16();
	regs.w[BX] = POP16();
	regs.w[DX] = POP16();
	regs.w[CX] = POP16();
	regs.w[AX] = POP16();
#else
	_invalid();
#endif
}

inline void X86::_bound()	// Opcode 0x62
{
#if defined(I286) || defined(V30)
	unsigned ModRM = FETCHOP();
	int low = (int16)GetRMWord(ModRM);
	int high = (int16)GetNextRMWord();
	int tmp = (int16)RegWord(ModRM);
	if(tmp < low || tmp>high) {
		PC-= 2;
		interrupt(5);
	}
	count -= cycles.bound;
#else
	_invalid();
#endif
}

inline void X86::_arpl()	// Opcode 0x63
{
#ifdef I286
	if(PM) {
		uint16 ModRM = FETCHOP();
		uint16 tmp = GetRMWord(ModRM);
		ZeroVal = i286_selector_okay(RegWord(ModRM)) && i286_selector_okay(RegWord(ModRM)) && ((tmp & 3) < (RegWord(ModRM) & 3));
		if(ZeroVal)
			PutbackRMWord(ModRM, (tmp & ~3) | (RegWord(ModRM) & 3));
		count -= 21;
	}
	else
		interrupt(ILLEGAL_INSTRUCTION);
#else
	_invalid();
#endif
}

#if 0
inline void X86::_brkn()	// Opcode 0x63 BRKN - Break to Native Mode
{
	unsigned vector = FETCH8();
}
#endif

inline void X86::_repc(int flagval)
{
#ifdef V30
	unsigned next = FETCHOP();
	unsigned cnt = regs.w[CX];
	
	switch(next)
	{
	case 0x26:	// ES:
		seg_prefix = true;
		prefix_base = base[ES];
		count -= 2;
		_repc(flagval);
		break;
	case 0x2e:	// CS:
		seg_prefix = true;
		prefix_base = base[CS];
		count -= 2;
		_repc(flagval);
		break;
	case 0x36:	// SS:
		seg_prefix = true;
		prefix_base = base[SS];
		count -= 2;
		_repc(flagval);
		break;
	case 0x3e:	// DS:
		seg_prefix = true;
		prefix_base = base[DS];
		count -= 2;
		_repc(flagval);
		break;
	case 0x6c:	// REP INSB
		count -= 9 - cnt;
		for(; (CF == flagval) && (cnt > 0); cnt--)
			_insb();
		regs.w[CX] = cnt;
		break;
	case 0x6d:	// REP INSW
		count -= 9 - cnt;
		for(; (CF == flagval) && (cnt > 0); cnt--)
			_insw();
		regs.w[CX] = cnt;
		break;
	case 0x6e:	// REP OUTSB
		count -= 9 - cnt;
		for(; (CF == flagval) && (cnt > 0); cnt--)
			_outsb();
		regs.w[CX] = cnt;
		break;
	case 0x6f:	// REP OUTSW
		count -= 9 - cnt;
		for(; (CF == flagval) && (cnt > 0); cnt--)
			_outsw();
		regs.w[CX] = cnt;
		break;
	case 0xa4:	// REP MOVSB
		count -= 9 - cnt;
		for(; (CF == flagval) && (cnt > 0); cnt--)
			_movsb();
		regs.w[CX] = cnt;
		break;
	case 0xa5:	// REP MOVSW
		count -= 9 - cnt;
		for(; (CF == flagval) && (cnt > 0); cnt--)
			_movsw();
		regs.w[CX] = cnt;
		break;
	case 0xa6:	// REP(N)E CMPSB
		count -= 9;
		for(ZeroVal = !flagval; (ZF == flagval) && (CF == flagval) && (cnt > 0); cnt--)
			_cmpsb();
		regs.w[CX] = cnt;
		break;
	case 0xa7:	// REP(N)E CMPSW
		count -= 9;
		for(ZeroVal = !flagval; (ZF == flagval) && (CF == flagval) && (cnt > 0); cnt--)
			_cmpsw();
		regs.w[CX] = cnt;
		break;
	case 0xaa:	// REP STOSB
		count -= 9 - cnt;
		for(; (CF == flagval) && (cnt > 0); cnt--)
			_stosb();
		regs.w[CX] = cnt;
		break;
	case 0xab:	// REP STOSW
		count -= 9 - cnt;
		for(; (CF == flagval) && (cnt > 0); cnt--)
			_stosw();
		regs.w[CX] = cnt;
		break;
	case 0xac:	// REP LODSB
		count -= 9;
		for(; (CF == flagval) && (cnt > 0); cnt--)
			_lodsb();
		regs.w[CX] = cnt;
		break;
	case 0xad:	// REP LODSW
		count -= 9;
		for(; (CF == flagval) && (cnt > 0); cnt--)
			_lodsw();
		regs.w[CX] = cnt;
		break;
	case 0xae:	// REP(N)E SCASB
		count -= 9;
		for(ZeroVal = !flagval; (ZF == flagval) && (CF == flagval) && (cnt > 0); cnt--)
			_scasb();
		regs.w[CX] = cnt;
		break;
	case 0xaf:	// REP(N)E SCASW
		count -= 9;
		for(ZeroVal = !flagval; (ZF == flagval) && (CF == flagval) && (cnt > 0); cnt--)
			_scasw();
		regs.w[CX] = cnt;
		break;
	default:
		op(next);
	}
#else
	_invalid();
#endif
}

inline void X86::_push_d16()	// Opcode 0x68
{
#if defined(I286) || defined(V30)
	unsigned tmp = FETCH8();
	count -= cycles.push_imm;
	tmp += FETCH8() << 8;
	PUSH16(tmp);
#else
	_invalid();
#endif
}

inline void X86::_imul_d16()	// Opcode 0x69
{
#if defined(I286) || defined(V30)
	DEF_r16w(dst, src);
	unsigned src2 = FETCH8();
	src += (FETCH8() << 8);
	count -= (ModRM >= 0xc0) ? cycles.imul_rri16 : cycles.imul_rmi16;
	dst = (int32)((int16)src) * (int32)((int16)src2);
	CarryVal = OverVal = (((int32)dst) >> 15 != 0) && (((int32)dst) >> 15 != -1);
	RegWord(ModRM) = (uint16)dst;
#else
	_invalid();
#endif
}

inline void X86::_push_d8()	// Opcode 0x6a
{
#if defined(I286) || defined(V30)
	unsigned tmp = (uint16)((int16)((int8)FETCH8()));
	count -= cycles.push_imm;
	PUSH16(tmp);
#else
	_invalid();
#endif
}

inline void X86::_imul_d8()	// Opcode 0x6b
{
#if defined(I286) || defined(V30)
	DEF_r16w(dst, src);
	unsigned src2 = (uint16)((int16)((int8)FETCH8()));
	count -= (ModRM >= 0xc0) ? cycles.imul_rri8 : cycles.imul_rmi8;
	dst = (int32)((int16)src) * (int32)((int16)src2);
	CarryVal = OverVal = (((int32)dst) >> 15 != 0) && (((int32)dst) >> 15 != -1);
	RegWord(ModRM) = (uint16)dst;
#else
	_invalid();
#endif
}

inline void X86::_insb()	// Opcode 0x6c
{
#if defined(I286) || defined(V30)
	count -= cycles.ins8;
	WM8(ES, regs.w[DI], IN8(regs.w[DX]));
	regs.w[DI] += DirVal;
#else
	_invalid();
#endif
}

inline void X86::_insw()	// Opcode 0x6d
{
#if defined(I286) || defined(V30)
	count -= cycles.ins16;
	WM8(ES, regs.w[DI], IN8(regs.w[DX]));
	WM8(ES, regs.w[DI] + 1, IN8(regs.w[DX] + 1));
	regs.w[DI] += 2 * DirVal;
#else
	_invalid();
#endif
}

inline void X86::_outsb()	// Opcode 0x6e
{
#if defined(I286) || defined(V30)
	count -= cycles.outs8;
	OUT8(regs.w[DX], RM8(DS, regs.w[SI]));
	regs.w[SI] += DirVal; // GOL 11/27/01
#else
	_invalid();
#endif
}

inline void X86::_outsw()	// Opcode 0x6f
{
#if defined(I286) || defined(V30)
	count -= cycles.outs16;
	OUT8(regs.w[DX], RM8(DS, regs.w[SI]));
	OUT8(regs.w[DX] + 1, RM8(DS, regs.w[SI] + 1));
	regs.w[SI] += 2 * DirVal;
#else
	_invalid();
#endif
}

inline void X86::_jo()	// Opcode 0x70
{
	int tmp = (int)((int8)FETCH8());
	if(OF) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jno()	// Opcode 0x71
{
	int tmp = (int)((int8)FETCH8());
	if(!OF) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jb()	// Opcode 0x72
{
	int tmp = (int)((int8)FETCH8());
	if(CF) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jnb()	// Opcode 0x73
{
	int tmp = (int)((int8)FETCH8());
	if(!CF) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jz()	// Opcode 0x74
{
	int tmp = (int)((int8)FETCH8());
	if(ZF) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jnz()	// Opcode 0x75
{
	int tmp = (int)((int8)FETCH8());
	if(!ZF) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jbe()	// Opcode 0x76
{
	int tmp = (int)((int8)FETCH8());
	if(CF || ZF) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jnbe()	// Opcode 0x77
{
	int tmp = (int)((int8)FETCH8());
	if(!(CF || ZF)) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_js()	// Opcode 0x78
{
	int tmp = (int)((int8)FETCH8());
	if(SF) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jns()	// Opcode 0x79
{
	int tmp = (int)((int8)FETCH8());
	if(!SF) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jp()	// Opcode 0x7a
{
	int tmp = (int)((int8)FETCH8());
	if(PF) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jnp()	// Opcode 0x7b
{
	int tmp = (int)((int8)FETCH8());
	if(!PF) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jl()	// Opcode 0x7c
{
	int tmp = (int)((int8)FETCH8());
	if((SF!= OF) && !ZF) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jnl()	// Opcode 0x7d
{
	int tmp = (int)((int8)FETCH8());
	if(ZF || (SF == OF)) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jle()	// Opcode 0x7e
{
	int tmp = (int)((int8)FETCH8());
	if(ZF || (SF!= OF)) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_jnle()	// Opcode 0x7f
{
	int tmp = (int)((int8)FETCH8());
	if((SF == OF) && !ZF) {
		PC += tmp;
		count -= cycles.jcc_t;
	}
	else
		count -= cycles.jcc_nt;
}

inline void X86::_op80()	// Opcode 0x80
{
	unsigned ModRM = FETCHOP();
	unsigned dst = GetRMByte(ModRM);
	unsigned src = FETCH8();
	
	switch(ModRM & 0x38)
	{
	case 0x00:	// ADD eb, d8
		ADDB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x08:	// OR eb, d8
		ORB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x10:	// ADC eb, d8
		src += CF;
		ADDB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x18:	// SBB eb, b8
		src += CF;
		SUBB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x20:	// AND eb, d8
		ANDB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x28:	// SUB eb, d8
		SUBB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x30:	// XOR eb, d8
		XORB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x38:	// CMP eb, d8
		SUBB(dst, src);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8_ro;
		break;
	}
}

inline void X86::_op81()	// Opcode 0x81
{
	unsigned ModRM = FETCH8();
	unsigned dst = GetRMWord(ModRM);
	unsigned src = FETCH8();
	src += (FETCH8() << 8);
	
	switch(ModRM & 0x38)
	{
	case 0x00:	// ADD ew, d16
		ADDW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri16 : cycles.alu_mi16;
		break;
	case 0x08:	// OR ew, d16
		ORW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri16 : cycles.alu_mi16;
		break;
	case 0x10:	// ADC ew, d16
		src += CF;
		ADDW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri16 : cycles.alu_mi16;
		break;
	case 0x18:	// SBB ew, d16
		src += CF;
		SUBW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri16 : cycles.alu_mi16;
		break;
	case 0x20:	// AND ew, d16
		ANDW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri16 : cycles.alu_mi16;
		break;
	case 0x28:	// SUB ew, d16
		SUBW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri16 : cycles.alu_mi16;
		break;
	case 0x30:	// XOR ew, d16
		XORW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri16 : cycles.alu_mi16;
		break;
	case 0x38:	// CMP ew, d16
		SUBW(dst, src);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri16 : cycles.alu_mi16_ro;
		break;
	}
}

inline void X86::_op82()	// Opcode 0x82
{
	unsigned ModRM = FETCH8();
	unsigned dst = GetRMByte(ModRM);
	unsigned src = FETCH8();
	
	switch(ModRM & 0x38)
	{
	case 0x00:	// ADD eb, d8
		ADDB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x08:	// OR eb, d8
		ORB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x10:	// ADC eb, d8
		src += CF;
		ADDB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x18:	// SBB eb, d8
		src += CF;
		SUBB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x20:	// AND eb, d8
		ANDB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x28:	// SUB eb, d8
		SUBB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x30:	// XOR eb, d8
		XORB(dst, src);
		PutbackRMByte(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8;
		break;
	case 0x38:	// CMP eb, d8
		SUBB(dst, src);
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8_ro;
		break;
	}
}

inline void X86::_op83()	// Opcode 0x83
{
	unsigned ModRM = FETCH8();
	unsigned dst = GetRMWord(ModRM);
	unsigned src = (uint16)((int16)((int8)FETCH8()));
	
	switch(ModRM & 0x38)
	{
	case 0x00:	// ADD ew, d16
		ADDW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_r16i8 : cycles.alu_m16i8;
		break;
	case 0x08:	// OR ew, d16
		ORW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_r16i8 : cycles.alu_m16i8;
		break;
	case 0x10:	// ADC ew, d16
		src += CF;
		ADDW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_r16i8 : cycles.alu_m16i8;
		break;
	case 0x18:	// SBB ew, d16
		src += CF;
		SUBW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_r16i8 : cycles.alu_m16i8;
		break;
	case 0x20:	// AND ew, d16
		ANDW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_r16i8 : cycles.alu_m16i8;
		break;
	case 0x28:	// SUB ew, d16
		SUBW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_r16i8 : cycles.alu_m16i8;
		break;
	case 0x30:	// XOR ew, d16
		XORW(dst, src);
		PutbackRMWord(ModRM, dst);
		count -= (ModRM >= 0xc0) ? cycles.alu_r16i8 : cycles.alu_m16i8;
		break;
	case 0x38:	// CMP ew, d16
		SUBW(dst, src);
		count -= (ModRM >= 0xc0) ? cycles.alu_r16i8 : cycles.alu_m16i8_ro;
		break;
	}
}

inline void X86::_test_br8()	// Opcode 0x84
{
	DEF_br8(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr8 : cycles.alu_rm8;
	ANDB(dst, src);
}

inline void X86::_test_wr16()	// Opcode 0x85
{
	DEF_wr16(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.alu_rr16 : cycles.alu_rm16;
	ANDW(dst, src);
}

inline void X86::_xchg_br8()	// Opcode 0x86
{
	DEF_br8(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.xchg_rr8 : cycles.xchg_rm8;
	RegByte(ModRM) = dst;
	PutbackRMByte(ModRM, src);
}

inline void X86::_xchg_wr16()	// Opcode 0x87
{
	DEF_wr16(dst, src);
	count -= (ModRM >= 0xc0) ? cycles.xchg_rr16 : cycles.xchg_rm16;
	RegWord(ModRM) = dst;
	PutbackRMWord(ModRM, src);
}

inline void X86::_mov_br8()	// Opcode 0x88
{
	unsigned ModRM = FETCH8();
	uint8 src = RegByte(ModRM);
	count -= (ModRM >= 0xc0) ? cycles.mov_rr8 : cycles.mov_mr8;
	PutRMByte(ModRM, src);
}

inline void X86::_mov_wr16()	// Opcode 0x89
{
	unsigned ModRM = FETCH8();
	uint16 src = RegWord(ModRM);
	count -= (ModRM >= 0xc0) ? cycles.mov_rr16 : cycles.mov_mr16;
	PutRMWord(ModRM, src);
}

inline void X86::_mov_r8b()	// Opcode 0x8a
{
	unsigned ModRM = FETCH8();
	uint8 src = GetRMByte(ModRM);
	count -= (ModRM >= 0xc0) ? cycles.mov_rr8 : cycles.mov_rm8;
	RegByte(ModRM) = src;
}

inline void X86::_mov_r16w()	// Opcode 0x8b
{
	unsigned ModRM = FETCH8();
	uint16 src = GetRMWord(ModRM);
	count -= (ModRM >= 0xc0) ? cycles.mov_rr8 : cycles.mov_rm16;
	RegWord(ModRM) = src;
}

inline void X86::_mov_wsreg()	// Opcode 0x8c
{
	unsigned ModRM = FETCH8();
	count -= (ModRM >= 0xc0) ? cycles.mov_rs : cycles.mov_ms;
#ifdef I286
	if(ModRM & 0x20) {
		interrupt(ILLEGAL_INSTRUCTION);
		return;
	}
#else
	if(ModRM & 0x20)
		return;
#endif
	PutRMWord(ModRM, sregs[(ModRM & 0x38) >> 3]);
}

inline void X86::_lea()	// Opcode 0x8d
{
	unsigned ModRM = FETCH8();
	count -= cycles.lea;
	GetEA(ModRM);
	RegWord(ModRM) = EO;
}

inline void X86::_mov_sregw()	// Opcode 0x8e
{
	unsigned ModRM = FETCH8();
	uint16 src = GetRMWord(ModRM);
	
	count -= (ModRM >= 0xc0) ? cycles.mov_sr : cycles.mov_sm;
#ifdef I286
	switch(ModRM & 0x38)
	{
	case 0x00:	// mov es, ew
		i286_data_descriptor(ES, src);
		break;
	case 0x08:	// mov cs, ew
		break;
	case 0x10:	// mov ss, ew
		i286_data_descriptor(SS, src);
		op(FETCHOP());
		break;
	case 0x18:	// mov ds, ew
		i286_data_descriptor(DS, src);
		break;
	}
#else
	switch(ModRM & 0x38)
	{
	case 0x00:	// mov es, ew
		sregs[ES] = src;
		base[ES] = SegBase(ES);
		break;
	case 0x08:	// mov cs, ew
		break;
	case 0x10:	// mov ss, ew
		sregs[SS] = src;
		base[SS] = SegBase(SS);
		op(FETCHOP());
		break;
	case 0x18:	// mov ds, ew
		sregs[DS] = src;
		base[DS] = SegBase(DS);
		break;
	}
#endif
}

inline void X86::_popw()	// Opcode 0x8f
{
	unsigned ModRM = FETCH8();
	uint16 tmp = POP16();
	count -= (ModRM >= 0xc0) ? cycles.pop_r16 : cycles.pop_m16;
	PutRMWord(ModRM, tmp);
}

#define XchgAXReg(reg) { \
	uint16 tmp = regs.w[reg]; \
	regs.w[reg] = regs.w[AX]; \
	regs.w[AX] = tmp; \
	count -= cycles.xchg_ar16; \
}

inline void X86::_nop()	// Opcode 0x90
{
	count -= cycles.nop;
}

inline void X86::_xchg_axcx()	// Opcode 0x91
{
	XchgAXReg(CX);
}

inline void X86::_xchg_axdx()	// Opcode 0x92
{
	XchgAXReg(DX);
}

inline void X86::_xchg_axbx()	// Opcode 0x93
{
	XchgAXReg(BX);
}

inline void X86::_xchg_axsp()	// Opcode 0x94
{
	XchgAXReg(SP);
}

inline void X86::_xchg_axbp()	// Opcode 0x95
{
	XchgAXReg(BP);
}

inline void X86::_xchg_axsi()	// Opcode 0x96
{
	XchgAXReg(SI);
}

inline void X86::_xchg_axdi()	// Opcode 0x97
{
	XchgAXReg(DI);
}

inline void X86::_cbw()	// Opcode 0x98
{
	count -= cycles.cbw;
	regs.b[AH] = (regs.b[AL] & 0x80) ? 0xff : 0;
}

inline void X86::_cwd()	// Opcode 0x99
{
	count -= cycles.cwd;
	regs.w[DX] = (regs.b[AH] & 0x80) ? 0xffff : 0;
}

inline void X86::_call_far()
{
	unsigned tmp1 = FETCH8();
	tmp1 += FETCH8() << 8;
	unsigned tmp2 = FETCH8();
	tmp2 += FETCH8() << 8;
	uint16 ip = PC - base[CS];
	PUSH16(sregs[CS]);
	PUSH16(ip);
#ifdef I286
	i286_code_descriptor(tmp2, tmp1);
#else
	sregs[CS] = (uint16)tmp2;
	base[CS] = SegBase(CS);
	PC = (base[CS] + (uint16)tmp1) & AMASK;
#endif
	count -= cycles.call_far;
}

inline void X86::_wait()	// Opcode 0x9b
{
	if(busy)
		PC--;
	count -= cycles.wait;
}

inline void X86::_pushf()	// Opcode 0x9c
{
	unsigned tmp = CompressFlags();
	count -= cycles.pushf;
#ifdef I286
	PUSH16(tmp &= ~0xf000);
#else
	PUSH16(tmp | 0xf000);
#endif
}

inline void X86::_popf()	// Opcode 0x9d
{
	unsigned tmp = POP16();
	count -= cycles.popf;
	ExpandFlags(tmp);
	if(TF) {
		op(FETCHOP());
		interrupt(1);
	}
}

inline void X86::_sahf()	// Opcode 0x9e
{
	unsigned tmp = (CompressFlags() & 0xff00) | (regs.b[AH] & 0xd5);
	count -= cycles.sahf;
	ExpandFlags(tmp);
}

inline void X86::_lahf()	// Opcode 0x9f
{
	regs.b[AH] = CompressFlags() & 0xff;
	count -= cycles.lahf;
}

inline void X86::_mov_aldisp()	// Opcode 0xa0
{
	unsigned addr = FETCH8();
	addr += FETCH8() << 8;
	count -= cycles.mov_am8;
	regs.b[AL] = RM8(DS, addr);
}

inline void X86::_mov_axdisp()	// Opcode 0xa1
{
	unsigned addr = FETCH8();
	addr += FETCH8() << 8;
	count -= cycles.mov_am16;
	regs.b[AL] = RM8(DS, addr);
	regs.b[AH] = RM8(DS, addr + 1);
}

inline void X86::_mov_dispal()	// Opcode 0xa2
{
	unsigned addr = FETCH8();
	addr += FETCH8() << 8;
	count -= cycles.mov_ma8;
	WM8(DS, addr, regs.b[AL]);
}

inline void X86::_mov_dispax()	// Opcode 0xa3
{
	unsigned addr = FETCH8();
	addr += FETCH8() << 8;
	count -= cycles.mov_ma16;
	WM8(DS, addr, regs.b[AL]);
	WM8(DS, addr + 1, regs.b[AH]);
}

inline void X86::_movsb()	// Opcode 0xa4
{
	uint8 tmp = RM8(DS, regs.w[SI]);
	WM8(ES, regs.w[DI], tmp);
	regs.w[DI] += DirVal;
	regs.w[SI] += DirVal;
	count -= cycles.movs8;
}

inline void X86::_movsw()	// Opcode 0xa5
{
	uint16 tmp = RM16(DS, regs.w[SI]);
	WM16(ES, regs.w[DI], tmp);
	regs.w[DI] += 2 * DirVal;
	regs.w[SI] += 2 * DirVal;
	count -= cycles.movs16;
}

inline void X86::_cmpsb()	// Opcode 0xa6
{
	unsigned dst = RM8(ES, regs.w[DI]);
	unsigned src = RM8(DS, regs.w[SI]);
	SUBB(src, dst);
	regs.w[DI] += DirVal;
	regs.w[SI] += DirVal;
	count -= cycles.cmps8;
}

inline void X86::_cmpsw()	// Opcode 0xa7
{
	unsigned dst = RM16(ES, regs.w[DI]);
	unsigned src = RM16(DS, regs.w[SI]);
	SUBW(src, dst);
	regs.w[DI] += 2 * DirVal;
	regs.w[SI] += 2 * DirVal;
	count -= cycles.cmps16;
}

inline void X86::_test_ald8()	// Opcode 0xa8
{
	DEF_ald8(dst, src);
	count -= cycles.alu_ri8;
	ANDB(dst, src);
}

inline void X86::_test_axd16()	// Opcode 0xa9
{
	DEF_axd16(dst, src);
	count -= cycles.alu_ri16;
	ANDW(dst, src);
}

inline void X86::_stosb()	// Opcode 0xaa
{
	WM8(ES, regs.w[DI], regs.b[AL]);
	regs.w[DI] += DirVal;
	count -= cycles.stos8;
}

inline void X86::_stosw()	// Opcode 0xab
{
	WM8(ES, regs.w[DI], regs.b[AL]);
	WM8(ES, regs.w[DI] + 1, regs.b[AH]);
	regs.w[DI] += 2 * DirVal;
	count -= cycles.stos16;
}

inline void X86::_lodsb()	// Opcode 0xac
{
	regs.b[AL] = RM8(DS, regs.w[SI]);
	regs.w[SI] += DirVal;
	count -= cycles.lods8;
}

inline void X86::_lodsw()	// Opcode 0xad
{
	regs.w[AX] = RM16(DS, regs.w[SI]);
	regs.w[SI] += 2 * DirVal;
	count -= cycles.lods16;
}

inline void X86::_scasb()	// Opcode 0xae
{
	unsigned src = RM8(ES, regs.w[DI]);
	unsigned dst = regs.b[AL];
	SUBB(dst, src);
	regs.w[DI] += DirVal;
	count -= cycles.scas8;
}

inline void X86::_scasw()	// Opcode 0xaf
{
	unsigned src = RM16(ES, regs.w[DI]);
	unsigned dst = regs.w[AX];
	SUBW(dst, src);
	regs.w[DI] += 2 * DirVal;
	count -= cycles.scas16;
}

inline void X86::_mov_ald8()	// Opcode 0xb0
{
	regs.b[AL] = FETCH8();
	count -= cycles.mov_ri8;
}

inline void X86::_mov_cld8()	// Opcode 0xb1
{
	regs.b[CL] = FETCH8();
	count -= cycles.mov_ri8;
}

inline void X86::_mov_dld8()	// Opcode 0xb2
{
	regs.b[DL] = FETCH8();
	count -= cycles.mov_ri8;
}

inline void X86::_mov_bld8()	// Opcode 0xb3
{
	regs.b[BL] = FETCH8();
	count -= cycles.mov_ri8;
}

inline void X86::_mov_ahd8()	// Opcode 0xb4
{
	regs.b[AH] = FETCH8();
	count -= cycles.mov_ri8;
}

inline void X86::_mov_chd8()	// Opcode 0xb5
{
	regs.b[CH] = FETCH8();
	count -= cycles.mov_ri8;
}

inline void X86::_mov_dhd8()	// Opcode 0xb6
{
	regs.b[DH] = FETCH8();
	count -= cycles.mov_ri8;
}

inline void X86::_mov_bhd8()	// Opcode 0xb7
{
	regs.b[BH] = FETCH8();
	count -= cycles.mov_ri8;
}

inline void X86::_mov_axd16()	// Opcode 0xb8
{
	regs.b[AL] = FETCH8();
	regs.b[AH] = FETCH8();
	count -= cycles.mov_ri16;
}

inline void X86::_mov_cxd16()	// Opcode 0xb9
{
	regs.b[CL] = FETCH8();
	regs.b[CH] = FETCH8();
	count -= cycles.mov_ri16;
}

inline void X86::_mov_dxd16()	// Opcode 0xba
{
	regs.b[DL] = FETCH8();
	regs.b[DH] = FETCH8();
	count -= cycles.mov_ri16;
}

inline void X86::_mov_bxd16()	// Opcode 0xbb
{
	regs.b[BL] = FETCH8();
	regs.b[BH] = FETCH8();
	count -= cycles.mov_ri16;
}

inline void X86::_mov_spd16()	// Opcode 0xbc
{
	regs.b[SPL] = FETCH8();
	regs.b[SPH] = FETCH8();
	count -= cycles.mov_ri16;
}

inline void X86::_mov_bpd16()	// Opcode 0xbd
{
	regs.b[BPL] = FETCH8();
	regs.b[BPH] = FETCH8();
	count -= cycles.mov_ri16;
}

inline void X86::_mov_sid16()	// Opcode 0xbe
{
	regs.b[SIL] = FETCH8();
	regs.b[SIH] = FETCH8();
	count -= cycles.mov_ri16;
}

inline void X86::_mov_did16()	// Opcode 0xbf
{
	regs.b[DIL] = FETCH8();
	regs.b[DIH] = FETCH8();
	count -= cycles.mov_ri16;
}

inline void X86::_rotshft_bd8()	// Opcode 0xc0
{
#if defined(I286) || defined(V30)
	unsigned ModRM = FETCH8();
	unsigned cnt = FETCH8();
	rotate_shift_byte(ModRM, cnt);
#else
	_invalid();
#endif
}

inline void X86::_rotshft_wd8()	// Opcode 0xc1
{
#if defined(I286) || defined(V30)
	unsigned ModRM = FETCH8();
	unsigned cnt = FETCH8();
	rotate_shift_word(ModRM, cnt);
#else
	_invalid();
#endif
}

inline void X86::_ret_d16()	// Opcode 0xc2
{
	unsigned cnt = FETCH8();
	cnt += FETCH8() << 8;
	PC = POP16();
	PC = (PC + base[CS]) & AMASK;
	regs.w[SP] += cnt;
	count -= cycles.ret_near_imm;
}

inline void X86::_ret()	// Opcode 0xc3
{
	PC = POP16();
	PC = (PC + base[CS]) & AMASK;
	count -= cycles.ret_near;
}

inline void X86::_les_dw()	// Opcode 0xc4
{
	unsigned ModRM = FETCH8();
	uint16 tmp = GetRMWord(ModRM);
	RegWord(ModRM) = tmp;
#ifdef I286
	i286_data_descriptor(ES, GetNextRMWord());
#else
	sregs[ES] = GetNextRMWord();
	base[ES] = SegBase(ES);
#endif
	count -= cycles.load_ptr;
}

inline void X86::_lds_dw()	// Opcode 0xc5
{
	unsigned ModRM = FETCH8();
	uint16 tmp = GetRMWord(ModRM);
	RegWord(ModRM) = tmp;
#ifdef I286
	i286_data_descriptor(DS, GetNextRMWord());
#else
	sregs[DS] = GetNextRMWord();
	base[DS] = SegBase(DS);
#endif
	count -= cycles.load_ptr;
}

inline void X86::_mov_bd8()	// Opcode 0xc6
{
	unsigned ModRM = FETCH8();
	count -= (ModRM >= 0xc0) ? cycles.mov_ri8 : cycles.mov_mi8;
	PutImmRMByte(ModRM);
}

inline void X86::_mov_wd16()	// Opcode 0xc7
{
	unsigned ModRM = FETCH8();
	count -= (ModRM >= 0xc0) ? cycles.mov_ri16 : cycles.mov_mi16;
	PutImmRMWord(ModRM);
}

inline void X86::_enter()	// Opcode 0xc8
{
#if defined(I286) || defined(V30)
	unsigned nb = FETCH8();
	nb += FETCH8() << 8;
	unsigned level = FETCH8();
	count -= (level == 0) ? cycles.enter0 : (level == 1) ? cycles.enter1 : cycles.enter_base + level * cycles.enter_count;
	PUSH16(regs.w[BP]);
	regs.w[BP] = regs.w[SP];
	regs.w[SP] -= nb;
	for(unsigned i = 1; i < level; i++)
		PUSH16(RM16(SS, regs.w[BP]-i*2));
	if(level)
		PUSH16(regs.w[BP]);
#else
	_invalid();
#endif
}

inline void X86::_leav()	// Opcode 0xc9
{
#if defined(I286) || defined(V30)
	count -= cycles.leave;
	regs.w[SP] = regs.w[BP];
	regs.w[BP] = POP16();
#else
	_invalid();
#endif
}

inline void X86::_retf_d16()	// Opcode 0xca
{
	unsigned cnt = FETCH8();
	cnt += FETCH8() << 8;
#ifdef I286
	{
		int tmp1 = POP16();
		int tmp2 = POP16();
		i286_code_descriptor(tmp2, tmp1);
	}
#else
	PC = POP16();
	sregs[CS] = POP16();
	base[CS] = SegBase(CS);
	PC = (PC + base[CS]) & AMASK;
#endif
	regs.w[SP] += cnt;
	count -= cycles.ret_far_imm;
}

inline void X86::_retf()	// Opcode 0xcb
{
#ifdef I286
	{
		int tmp1 = POP16();
		int tmp2 = POP16();
		i286_code_descriptor(tmp2, tmp1);
	}
#else
	PC = POP16();
	sregs[CS] = POP16();
	base[CS] = SegBase(CS);
	PC = (PC + base[CS]) & AMASK;
#endif
	count -= cycles.ret_far;
}

inline void X86::_int3()	// Opcode 0xcc
{
	count -= cycles.int3;
	interrupt(3);
}

inline void X86::_int()	// Opcode 0xcd
{
	unsigned num = FETCH8();
	count -= cycles.int_imm;
	interrupt(num);
}

inline void X86::_into()	// Opcode 0xce
{
	if(OF) {
		count -= cycles.into_t;
		interrupt(4);
	}
	else
		count -= cycles.into_nt;
}

inline void X86::_iret()	// Opcode 0xcf
{
	count -= cycles.iret;
#ifdef I286
	{
		int tmp1 = POP16();
		int tmp2 = POP16();
		i286_code_descriptor(tmp2, tmp1);
	}
#else
	PC = POP16();
	sregs[CS] = POP16();
	base[CS] = SegBase(CS);
	PC = (PC + base[CS]) & AMASK;
#endif
	_popf();
}

inline void X86::_rotshft_b()	// Opcode 0xd0
{
	rotate_shift_byte(FETCHOP(), 1);
}

inline void X86::_rotshft_w()	// Opcode 0xd1
{
	rotate_shift_word(FETCHOP(), 1);
}

inline void X86::_rotshft_bcl()	// Opcode 0xd2
{
	rotate_shift_byte(FETCHOP(), regs.b[CL]);
}

inline void X86::_rotshft_wcl()	// Opcode 0xd3
{
	rotate_shift_word(FETCHOP(), regs.b[CL]);
}

inline void X86::_aam()	// Opcode 0xd4
{
	unsigned mult = FETCH8();
	count -= cycles.aam;
	if(mult == 0)
		interrupt(0);
	else {
		regs.b[AH] = regs.b[AL] / mult;
		regs.b[AL] %= mult;
		SetSZPF_Word(regs.w[AX]);
	}
}

inline void X86::_aad()	// Opcode 0xd5
{
	unsigned mult = FETCH8();
	count -= cycles.aad;
	regs.b[AL] = regs.b[AH] * mult + regs.b[AL];
	regs.b[AH] = 0;
	SetZF(regs.b[AL]);
	SetPF(regs.b[AL]);
	SignVal = 0;
}

inline void X86::_setalc()	// Opcode 0xd6
{
#ifdef V30
	regs.b[AL] = (CF) ? 0xff : 0x00;
	count -= 3;
#else
	_invalid();
#endif
}

inline void X86::_xlat()	// Opcode 0xd7
{
	unsigned dest = regs.w[BX] + regs.b[AL];
	count -= cycles.xlat;
	regs.b[AL] = RM8(DS, dest);
}

inline void X86::_escape()	// Opcodes 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde and 0xdf
{
	unsigned ModRM = FETCH8();
	count -= cycles.nop;
	GetRMByte(ModRM);
}

inline void X86::_loopne()	// Opcode 0xe0
{
	int disp = (int)((int8)FETCH8());
	unsigned tmp = regs.w[CX] - 1;
	regs.w[CX] = tmp;
	if(!ZF && tmp) {
		count -= cycles.loop_t;
		PC += disp;
	}
	else
		count -= cycles.loop_nt;
}

inline void X86::_loope()	// Opcode 0xe1
{
	int disp = (int)((int8)FETCH8());
	unsigned tmp = regs.w[CX] - 1;
	regs.w[CX] = tmp;
	if(ZF && tmp) {
		count -= cycles.loope_t;
		PC += disp;
	}
	else
		count -= cycles.loope_nt;
}

inline void X86::_loop()	// Opcode 0xe2
{
	int disp = (int)((int8)FETCH8());
	unsigned tmp = regs.w[CX] - 1;
	regs.w[CX] = tmp;
	if(tmp) {
		count -= cycles.loop_t;
		PC += disp;
	}
	else
		count -= cycles.loop_nt;
}

inline void X86::_jcxz()	// Opcode 0xe3
{
	int disp = (int)((int8)FETCH8());
	if(regs.w[CX] == 0) {
		count -= cycles.jcxz_t;
		PC += disp;
	}
	else
		count -= cycles.jcxz_nt;
}

inline void X86::_inal()	// Opcode 0xe4
{
	unsigned port = FETCH8();
	count -= cycles.in_imm8;
	regs.b[AL] = IN8(port);
}

inline void X86::_inax()	// Opcode 0xe5
{
	unsigned port = FETCH8();
	count -= cycles.in_imm16;
	regs.b[AL] = IN8(port);
	regs.b[AH] = IN8(port + 1);
}

inline void X86::_outal()	// Opcode 0xe6
{
	unsigned port = FETCH8();
	count -= cycles.out_imm8;
	OUT8(port, regs.b[AL]);
}

inline void X86::_outax()	// Opcode 0xe7
{
	unsigned port = FETCH8();
	count -= cycles.out_imm16;
	OUT8(port, regs.b[AL]);
	OUT8(port + 1, regs.b[AH]);
}

inline void X86::_call_d16()	// Opcode 0xe8
{
	uint16 tmp = FETCH16();
	uint16 ip = PC - base[CS];
	PUSH16(ip);
	ip += tmp;
	PC = (ip + base[CS]) & AMASK;
	count -= cycles.call_near;
}

inline void X86::_jmp_d16()	// Opcode 0xe9
{
	uint16 tmp = FETCH16();
	uint16 ip = PC - base[CS] + tmp;
	PC = (ip + base[CS]) & AMASK;
	count -= cycles.jmp_near;
}

inline void X86::_jmp_far()	// Opcode 0xea
{
	unsigned tmp1 = FETCH8();
	tmp1 += FETCH8() << 8;
	unsigned tmp2 = FETCH8();
	tmp2 += FETCH8() << 8;
#ifdef I286
	i286_code_descriptor(tmp2, tmp1);
#else
	sregs[CS] = (uint16)tmp2;
	base[CS] = SegBase(CS);
	PC = (base[CS] + tmp1) & AMASK;
#endif
	count -= cycles.jmp_far;
}

inline void X86::_jmp_d8()	// Opcode 0xeb
{
	int tmp = (int)((int8)FETCH8());
	PC += tmp;
	count -= cycles.jmp_short;
}

inline void X86::_inaldx()	// Opcode 0xec
{
	count -= cycles.in_dx8;
	regs.b[AL] = IN8(regs.w[DX]);
}

inline void X86::_inaxdx()	// Opcode 0xed
{
	unsigned port = regs.w[DX];
	count -= cycles.in_dx16;
	regs.b[AL] = IN8(port);
	regs.b[AH] = IN8(port + 1);
}

inline void X86::_outdxal()	// Opcode 0xee
{
	count -= cycles.out_dx8;
	OUT8(regs.w[DX], regs.b[AL]);
}

inline void X86::_outdxax()	// Opcode 0xef
{
	unsigned port = regs.w[DX];
	count -= cycles.out_dx16;
	OUT8(port, regs.b[AL]);
	OUT8(port + 1, regs.b[AH]);
}

inline void X86::_lock()	// Opcode 0xf0
{
	count -= cycles.nop;
	op(FETCHOP());
}

inline void X86::_rep(int flagval)
{
	unsigned next = FETCHOP();
	unsigned cnt = regs.w[CX];
	
	switch(next)
	{
	case 0x26:	// ES:
		seg_prefix = true;
		prefix_base = base[ES];
		count -= cycles.override;
		_rep(flagval);
		break;
	case 0x2e:	// CS:
		seg_prefix = true;
		prefix_base = base[CS];
		count -= cycles.override;
		_rep(flagval);
		break;
	case 0x36:	// SS:
		seg_prefix = true;
		prefix_base = base[SS];
		count -= cycles.override;
		_rep(flagval);
		break;
	case 0x3e:	// DS:
		seg_prefix = true;
		prefix_base = base[DS];
		count -= cycles.override;
		_rep(flagval);
		break;
#ifndef I86
	case 0x6c:	// REP INSB
		count -= cycles.rep_ins8_base;
		for(; cnt > 0; cnt--) {
			WM8(ES, regs.w[DI], IN8(regs.w[DX]));
			regs.w[DI] += DirVal;
			count -= cycles.rep_ins8_count;
		}
		regs.w[CX] = cnt;
		break;
	case 0x6d:	// REP INSW
		count -= cycles.rep_ins16_base;
		for(; cnt > 0; cnt--) {
			WM8(ES, regs.w[DI], IN8(regs.w[DX]));
			WM8(ES, regs.w[DI] + 1, IN8(regs.w[DX] + 1));
			regs.w[DI] += 2 * DirVal;
			count -= cycles.rep_ins16_count;
		}
		regs.w[CX] = cnt;
		break;
	case 0x6e:	// REP OUTSB
		count -= cycles.rep_outs8_base;
		for(; cnt > 0; cnt--) {
			OUT8(regs.w[DX], RM8(DS, regs.w[SI]));
			regs.w[SI] += DirVal;
			count -= cycles.rep_outs8_count;
		}
		regs.w[CX] = cnt;
		break;
	case 0x6f:	// REP OUTSW
		count -= cycles.rep_outs16_base;
		for(; cnt > 0; cnt--) {
			OUT8(regs.w[DX], RM8(DS, regs.w[SI]));
			OUT8(regs.w[DX] + 1, RM8(DS, regs.w[SI] + 1));
			regs.w[SI] += 2 * DirVal;
			count -= cycles.rep_outs16_count;
		}
		regs.w[CX] = cnt;
		break;
#endif
	case 0xa4:	// REP MOVSB
		count -= cycles.rep_movs8_base;
		for(; cnt > 0; cnt--) {
			uint8 tmp = RM8(DS, regs.w[SI]);
			WM8(ES, regs.w[DI], tmp);
			regs.w[DI] += DirVal;
			regs.w[SI] += DirVal;
			count -= cycles.rep_movs8_count;
		}
		regs.w[CX] = cnt;
		break;
	case 0xa5:	// REP MOVSW
		count -= cycles.rep_movs16_base;
		for(; cnt > 0; cnt--) {
			uint16 tmp = RM16(DS, regs.w[SI]);
			WM16(ES, regs.w[DI], tmp);
			regs.w[DI] += 2 * DirVal;
			regs.w[SI] += 2 * DirVal;
			count -= cycles.rep_movs16_count;
		}
		regs.w[CX] = cnt;
		break;
	case 0xa6:	// REP(N)E CMPSB
		count -= cycles.rep_cmps8_base;
		for(ZeroVal = !flagval; (ZF == flagval) && (cnt > 0); cnt--) {
			unsigned dst = RM8(ES, regs.w[DI]);
			unsigned src = RM8(DS, regs.w[SI]);
			SUBB(src, dst);
			regs.w[DI] += DirVal;
			regs.w[SI] += DirVal;
			count -= cycles.rep_cmps8_count;
		}
		regs.w[CX] = cnt;
		break;
	case 0xa7:	// REP(N)E CMPSW
		count -= cycles.rep_cmps16_base;
		for(ZeroVal = !flagval; (ZF == flagval) && (cnt > 0); cnt--) {
			unsigned dst = RM16(ES, regs.w[DI]);
			unsigned src = RM16(DS, regs.w[SI]);
			SUBW(src, dst);
			regs.w[DI] += 2 * DirVal;
			regs.w[SI] += 2 * DirVal;
			count -= cycles.rep_cmps16_count;
		}
		regs.w[CX] = cnt;
		break;
	case 0xaa:	// REP STOSB
		count -= cycles.rep_stos8_base;
		for(; cnt > 0; cnt--) {
			WM8(ES, regs.w[DI], regs.b[AL]);
			regs.w[DI] += DirVal;
			count -= cycles.rep_stos8_count;
		}
		regs.w[CX] = cnt;
		break;
	case 0xab:	// REP STOSW
		count -= cycles.rep_stos16_base;
		for(; cnt > 0; cnt--) {
			WM8(ES, regs.w[DI], regs.b[AL]);
			WM8(ES, regs.w[DI] + 1, regs.b[AH]);
			regs.w[DI] += 2 * DirVal;
			count -= cycles.rep_stos16_count;
		}
		regs.w[CX] = cnt;
		break;
	case 0xac:	// REP LODSB
		count -= cycles.rep_lods8_base;
		for(; cnt > 0; cnt--) {
			regs.b[AL] = RM8(DS, regs.w[SI]);
			regs.w[SI] += DirVal;
			count -= cycles.rep_lods8_count;
		}
		regs.w[CX] = cnt;
		break;
	case 0xad:	// REP LODSW
		count -= cycles.rep_lods16_base;
		for(; cnt > 0; cnt--) {
			regs.w[AX] = RM16(DS, regs.w[SI]);
			regs.w[SI] += 2 * DirVal;
			count -= cycles.rep_lods16_count;
		}
		regs.w[CX] = cnt;
		break;
	case 0xae:	// REP(N)E SCASB
		count -= cycles.rep_scas8_base;
		for(ZeroVal = !flagval; (ZF == flagval) && (cnt > 0); cnt--) {
			unsigned src = RM8(ES, regs.w[DI]);
			unsigned dst = regs.b[AL];
			SUBB(dst, src);
			regs.w[DI] += DirVal;
			count -= cycles.rep_scas8_count;
		}
		regs.w[CX] = cnt;
		break;
	case 0xaf:	// REP(N)E SCASW
		count -= cycles.rep_scas16_base;
		for(ZeroVal = !flagval; (ZF == flagval) && (cnt > 0); cnt--) {
			unsigned src = RM16(ES, regs.w[DI]);
			unsigned dst = regs.w[AX];
			SUBW(dst, src);
			regs.w[DI] += 2 * DirVal;
			count -= cycles.rep_scas16_count;
		}
		regs.w[CX] = cnt;
		break;
	default:
		op(next);
	}
}

inline void X86::_hlt()	// Opcode 0xf4
{
	PC--;
	halt = true;
	count -= 2;
}

inline void X86::_cmc()	// Opcode 0xf5
{
	count -= cycles.flag_ops;
	CarryVal = !CF;
}

inline void X86::_opf6()	// Opecode 0xf6
{
	unsigned ModRM = FETCH8();
	unsigned tmp1 = (unsigned)GetRMByte(ModRM), tmp2;
	
	switch(ModRM & 0x38)
	{
	case 0x00:	// TEST Eb, data8
	case 0x08:	// ???
		count -= (ModRM >= 0xc0) ? cycles.alu_ri8 : cycles.alu_mi8_ro;
		tmp1 &= FETCH8();
		CarryVal = OverVal = AuxVal = 0;
		SetSZPF_Byte(tmp1);
		break;
	case 0x10:	// NOT Eb
		count -= (ModRM >= 0xc0) ? cycles.negnot_r8 : cycles.negnot_m8;
		PutbackRMByte(ModRM, ~tmp1);
		break;
	case 0x18:	// NEG Eb
		count -= (ModRM >= 0xc0) ? cycles.negnot_r8 : cycles.negnot_m8;
		tmp2 = 0;
		SUBB(tmp2, tmp1);
		PutbackRMByte(ModRM, tmp2);
		break;
	case 0x20:	// MUL AL, Eb
		count -= (ModRM >= 0xc0) ? cycles.mul_r8 : cycles.mul_m8;
		{
			tmp2 = regs.b[AL];
			SetSF((int8)tmp2);
			SetPF(tmp2);
			uint16 result = (uint16)tmp2 * tmp1;
			regs.w[AX] = (uint16)result;
			SetZF(regs.w[AX]);
			CarryVal = OverVal = (regs.b[AH] != 0);
		}
		break;
	case 0x28:	// IMUL AL, Eb
		count -= (ModRM >= 0xc0) ? cycles.imul_r8 : cycles.imul_m8;
		{
			tmp2 = (unsigned)regs.b[AL];
			SetSF((int8)tmp2);
			SetPF(tmp2);
			int16 result = (int16)((int8)tmp2) * (int16)((int8)tmp1);
			regs.w[AX] = (uint16)result;
			SetZF(regs.w[AX]);
			CarryVal = OverVal = (result >> 7 != 0) && (result >> 7 != -1);
		}
		break;
	case 0x30:	// DIV AL, Ew
		count -= (ModRM >= 0xc0) ? cycles.div_r8 : cycles.div_m8;
		if(tmp1) {
			uint16 result = regs.w[AX];
			if((result / tmp1) > 0xff)
				interrupt(0);
			else {
				regs.b[AH] = result % tmp1;
				regs.b[AL] = result / tmp1;
			}
		}
		else
			interrupt(0);
		break;
	case 0x38:	// IDIV AL, Ew
		count -= (ModRM >= 0xc0) ? cycles.idiv_r8 : cycles.idiv_m8;
		if(tmp1) {
			int16 result = regs.w[AX];
			tmp2 = result % (int16)((int8)tmp1);
			if((result /= (int16)((int8)tmp1)) > 0xff)
				interrupt(0);
			else {
				regs.b[AL] = (uint8)result;
				regs.b[AH] = tmp2;
			}
		}
		else
			interrupt(0);
		break;
	}
}

inline void X86::_opf7()
{
	// Opcode 0xf7
	unsigned ModRM = FETCH8();
	unsigned tmp1 = GetRMWord(ModRM), tmp2;
	
	switch(ModRM & 0x38)
	{
	case 0x00:	// TEST Ew, data16
	case 0x08:	// ???
		count -= (ModRM >= 0xc0) ? cycles.alu_ri16 : cycles.alu_mi16_ro;
		tmp2 = FETCH8();
		tmp2 += FETCH8() << 8;
		tmp1 &= tmp2;
		CarryVal = OverVal = AuxVal = 0;
		SetSZPF_Word(tmp1);
		break;
	case 0x10:	// NOT Ew
		count -= (ModRM >= 0xc0) ? cycles.negnot_r16 : cycles.negnot_m16;
		tmp1 = ~tmp1;
		PutbackRMWord(ModRM, tmp1);
		break;
	case 0x18:	// NEG Ew
		count -= (ModRM >= 0xc0) ? cycles.negnot_r16 : cycles.negnot_m16;
		tmp2 = 0;
		SUBW(tmp2, tmp1);
		PutbackRMWord(ModRM, tmp2);
		break;
	case 0x20:	// MUL AX, Ew
		count -= (ModRM >= 0xc0) ? cycles.mul_r16 : cycles.mul_m16;
		{
			tmp2 = regs.w[AX];
			SetSF((int16)tmp2);
			SetPF(tmp2);
			uint32 result = (uint32)tmp2 * tmp1;
			regs.w[AX] = (uint16)result;
			result >>= 16;
			regs.w[DX] = result;
			SetZF(regs.w[AX] | regs.w[DX]);
			CarryVal = OverVal = (regs.w[DX] != 0);
		}
		break;
	case 0x28:	// IMUL AX, Ew
		count -= (ModRM >= 0xc0) ? cycles.imul_r16 : cycles.imul_m16;
		{
			tmp2 = regs.w[AX];
			SetSF((int16)tmp2);
			SetPF(tmp2);
			int32 result = (int32)((int16)tmp2) * (int32)((int16)tmp1);
			CarryVal = OverVal = (result >> 15 != 0) && (result >> 15 != -1);
			regs.w[AX] = (uint16)result;
			result = (uint16)(result >> 16);
			regs.w[DX] = result;
			SetZF(regs.w[AX] | regs.w[DX]);
		}
		break;
	case 0x30:	// DIV AX, Ew
		count -= (ModRM >= 0xc0) ? cycles.div_r16 : cycles.div_m16;
		if(tmp1) {
			uint32 result = (regs.w[DX] << 16) + regs.w[AX];
			tmp2 = result % tmp1;
			if((result / tmp1) > 0xffff)
				interrupt(0);
			else {
				regs.w[DX] = tmp2;
				result /= tmp1;
				regs.w[AX] = result;
			}
		}
		else
			interrupt(0);
		break;
	case 0x38:	// IDIV AX, Ew
		count -= (ModRM >= 0xc0) ? cycles.idiv_r16 : cycles.idiv_m16;
		if(tmp1) {
			int32 result = (regs.w[DX] << 16) + regs.w[AX];
			tmp2 = result % (int32)((int16)tmp1);
			if((result /= (int32)((int16)tmp1)) > 0xffff)
				interrupt(0);
			else {
				regs.w[AX] = result;
				regs.w[DX] = tmp2;
			}
		}
		else
			interrupt(0);
		break;
	}
}

inline void X86::_clc()	// Opcode 0xf8
{
	count -= cycles.flag_ops;
	CarryVal = 0;
}

inline void X86::_stc()	// Opcode 0xf9
{
	count -= cycles.flag_ops;
	CarryVal = 1;
}

inline void X86::_cli()	// Opcode 0xfa
{
	count -= cycles.flag_ops;
	SetIF(0);
}

inline void X86::_sti()	// Opcode 0xfb
{
	count -= cycles.flag_ops;
	SetIF(1);
	op(FETCHOP());
}

inline void X86::_cld()	// Opcode 0xfc
{
	count -= cycles.flag_ops;
	SetDF(0);
}

inline void X86::_std()	// Opcode 0xfd
{
	count -= cycles.flag_ops;
	SetDF(1);
}

inline void X86::_opfe()	// Opcode 0xfe
{
	unsigned ModRM = FETCH8();
	unsigned tmp1 = GetRMByte(ModRM), tmp2;
	count -= (ModRM >= 0xc0) ? cycles.incdec_r8 : cycles.incdec_m8;
	if((ModRM & 0x38) == 0) {
		// INC eb
		tmp2 = tmp1 + 1;
		SetOFB_Add(tmp2, tmp1, 1);
	}
	else {
		// DEC eb
		tmp2 = tmp1 - 1;
		SetOFB_Sub(tmp2, 1, tmp1);
	}
	SetAF(tmp2, tmp1, 1);
	SetSZPF_Byte(tmp2);
	PutbackRMByte(ModRM, (uint8)tmp2);
}

inline void X86::_opff()	// Opcode 0xff
{
	unsigned ModRM = FETCHOP(), tmp1, tmp2;
	uint16 ip;
	
	switch(ModRM & 0x38)
	{
	case 0x00:	// INC ew
		count -= (ModRM >= 0xc0) ? cycles.incdec_r16 : cycles.incdec_m16;
		tmp1 = GetRMWord(ModRM);
		tmp2 = tmp1 + 1;
		SetOFW_Add(tmp2, tmp1, 1);
		SetAF(tmp2, tmp1, 1);
		SetSZPF_Word(tmp2);
		PutbackRMWord(ModRM, (uint16)tmp2);
		break;
	case 0x08:	// DEC ew
		count -= (ModRM >= 0xc0) ? cycles.incdec_r16 : cycles.incdec_m16;
		tmp1 = GetRMWord(ModRM);
		tmp2 = tmp1 - 1;
		SetOFW_Sub(tmp2, 1, tmp1);
		SetAF(tmp2, tmp1, 1);
		SetSZPF_Word(tmp2);
		PutbackRMWord(ModRM, (uint16)tmp2);
		break;
	case 0x10:	// CALL ew
		count -= (ModRM >= 0xc0) ? cycles.call_r16 : cycles.call_m16;
		tmp1 = GetRMWord(ModRM);
		ip = PC - base[CS];
		PUSH16(ip);
		PC = (base[CS] + (uint16)tmp1) & AMASK;
		break;
	case 0x18:	// CALL FAR ea
		count -= cycles.call_m32;
		tmp1 = sregs[CS];
		tmp2 = GetRMWord(ModRM);
		ip = PC - base[CS];
		PUSH16(tmp1);
		PUSH16(ip);
#ifdef I286
		i286_code_descriptor(GetNextRMWord(), tmp2);
#else
		sregs[CS] = GetNextRMWord();
		base[CS] = SegBase(CS);
		PC = (base[CS] + tmp2) & AMASK;
#endif
		break;
	case 0x20:	// JMP ea
		count -= (ModRM >= 0xc0) ? cycles.jmp_r16 : cycles.jmp_m16;
		ip = GetRMWord(ModRM);
		PC = (base[CS] + ip) & AMASK;
		break;
	case 0x28:	// JMP FAR ea
		count -= cycles.jmp_m32;
#ifdef I286
		tmp1 = GetRMWord(ModRM);
		i286_code_descriptor(GetNextRMWord(), tmp1);
#else
		PC = GetRMWord(ModRM);
		sregs[CS] = GetNextRMWord();
		base[CS] = SegBase(CS);
		PC = (PC + base[CS]) & AMASK;
#endif
		break;
	case 0x30:	// PUSH ea
		count -= (ModRM >= 0xc0) ? cycles.push_r16 : cycles.push_m16;
		tmp1 = GetRMWord(ModRM);
		PUSH16(tmp1);
		break;
	case 0x38:	// invalid ???
		count -= 10;
		break;
	}
}

inline void X86::_invalid()
{
#ifdef I286
	interrupt(ILLEGAL_INSTRUCTION);
#else
	PC--;
	count -= 10;
#endif
}
