/*
	Skelton for retropc emulator

	Origin : MAME
	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ Z80 ]
*/

#include "z80.h"

#define AF	regs[0].w
#define BC	regs[1].w
#define DE 	regs[2].w
#define HL	regs[3].w
#define IX	regs[4].w
#define IY	regs[5].w

#define _F	regs[0].b.l
#define _A	regs[0].b.h
#define _C	regs[1].b.l
#define _B	regs[1].b.h
#define _E	regs[2].b.l
#define _D	regs[2].b.h
#define _L	regs[3].b.l
#define _H	regs[3].b.h
#define _XL	regs[4].b.l
#define _XH	regs[4].b.h
#define _YL	regs[5].b.l
#define _YH	regs[5].b.h

#define CF	0x01
#define NF	0x02
#define PF	0x04
#define VF	0x04
#define XF	0x08
#define HF	0x10
#define YF	0x20
#define ZF	0x40
#define SF	0x80

// opecode definitions

#define EAX() { \
	int res = FETCH8(); \
	EA = IX + ((res < 128) ? res : res - 256); \
}

#define EAY() { \
	int res = FETCH8(); \
	EA = IY + ((res < 128) ? res : res - 256); \
}

#define JP() { \
	PC = RM16(PC); \
}

#define JP_COND(cond) { \
	if(cond) \
		PC = RM16(PC); \
	else \
		PC += 2; \
}

#define JR() { \
	int res = FETCH8(); \
	PC += (res < 128) ? res : res - 256; \
}

#define JR_COND(cond, opcode) { \
	if(cond) { \
		int res = FETCH8(); \
		PC += (res < 128) ? res : res - 256; \
		count -= cc_ex[opcode]; \
	} \
	else \
		PC++; \
}

#define CALL() { \
	EA = FETCH16(); \
	PUSH16(PC); \
	PC = EA; \
}

#define CALL_COND(cond, opcode) { \
	if(cond) { \
		EA = FETCH16(); \
		PUSH16(PC); \
		PC = EA; \
		count -= cc_ex[opcode]; \
	} \
	else \
		PC += 2; \
}

#define RET() { \
	PC = POP16(); \
}

#define RET_COND(cond, opcode) { \
	if(cond) { \
		PC = POP16(); \
		count -= cc_ex[opcode]; \
	} \
}

#define DI() { \
	IFF1 = IFF2 = 0; \
}

#define EI() { \
	IFF1 = IFF2 = 1; \
	OP(FETCHOP()); \
}

#define RST(addr) { \
	PUSH16(PC); \
	PC = addr; \
}

#define RETN() { \
	PC = POP16(); \
	IFF1 = IFF2; \
}

#define RETI() { \
	PC = POP16(); \
	IFF1 = IFF2; \
	NOTIFY_RETI(); \
}

#define EX_AF() { \
	uint16 tmp; \
	tmp = AF; AF = exAF; exAF = tmp; \
}

#define EX_DE_HL() { \
	uint16 tmp; \
	tmp = DE; DE = HL; HL = tmp; \
}

#define EXX() { \
	uint16 tmp; \
	tmp = BC; BC = exBC; exBC = tmp; \
	tmp = DE; DE = exDE; exDE = tmp; \
	tmp = HL; HL = exHL; exHL = tmp; \
}

inline uint16 Z80::EXSP(uint16 reg) {
	uint16 res = RM16(SP);
	WM16(SP, reg);
	return res;
}

inline uint8 Z80::INC(uint8 value) {
	uint8 res = value + 1;
	_F = (_F & CF) | SZHV_inc[res];
	return res;
}

inline uint8 Z80::DEC(uint8 value) {
	uint8 res = value - 1;
	_F = (_F & CF) | SZHV_dec[res];
	return res;
}

#define ADD(value) { \
	uint16 val = value; \
	uint16 res = _A + val; \
	_F = SZ[res & 0xff] | ((res >> 8) & CF) | ((_A ^ res ^ val) & HF) | (((val ^ _A ^ 0x80) & (val ^ res) & 0x80) >> 5); \
	_A = (uint8)res; \
}

#define ADC(value) { \
	uint16 val = value; \
	uint16 res = _A + val + (_F & CF); \
	_F = SZ[res & 0xff] | ((res >> 8) & CF) | ((_A ^ res ^ val) & HF) | (((val ^ _A ^ 0x80) & (val ^ res) & 0x80) >> 5); \
	_A = (uint8)res; \
}

#define SUB(value) { \
	uint16 val = value; \
	uint16 res = _A - val; \
	_F = SZ[res & 0xff] | ((res >> 8) & CF) | NF | ((_A ^ res ^ val) & HF) | (((val ^ _A) & (_A ^ res) & 0x80) >> 5); \
	_A = (uint8)res; \
}

#define SBC(value) { \
	uint16 val = value; \
	uint16 res = _A - val - (_F & CF); \
	_F = SZ[res & 0xff] | ((res >> 8) & CF) | NF | ((_A ^ res ^ val) & HF) | (((val ^ _A) & (_A ^ res) & 0x80) >> 5); \
	_A = (uint8)res; \
}

inline uint16 Z80::ADD16(uint16 dreg, uint16 sreg) {
	uint32 res = dreg + sreg;
	_F = (uint8)((_F & (SF | ZF | VF)) | (((dreg ^ res ^ sreg) >> 8) & HF) | ((res >> 16) & CF) | ((res >> 8) & (YF | XF)));
	return (uint16)res;
}

#define ADC16(reg) { \
	uint32 res = HL + reg + (_F & CF); \
	_F = (uint8)((((HL ^ res ^ reg) >> 8) & HF) | ((res >> 16) & CF) | ((res >> 8) & (SF | YF | XF)) | ((res & 0xffff) ? 0 : ZF) | (((reg ^ HL ^ 0x8000) & (reg ^ res) & 0x8000) >> 13)); \
	HL = (uint16)res; \
}

#define SBC16(reg) { \
	uint32 res = HL - reg - (_F & CF); \
	_F = (uint8)((((HL ^ res ^ reg) >> 8) & HF) | NF | ((res >> 16) & CF) | ((res >> 8) & (SF | YF | XF)) | ((res & 0xffff) ? 0 : ZF) | (((reg ^ HL) & (HL ^ res) &0x8000) >> 13)); \
	HL = (uint16)res; \
}

#define NEG() { \
	uint8 value = _A; \
	_A = 0; \
	SUB(value); \
}

#define DAA() { \
	uint16 idx = _A; \
	if(_F & CF) idx |= 0x100; \
	if(_F & HF) idx |= 0x200; \
	if(_F & NF) idx |= 0x400; \
	AF = DAATable[idx]; \
}

#define AND(value) { \
	_A &= value; \
	_F = SZP[_A] | HF; \
}

#define OR(value) { \
	_A |= value; \
	_F = SZP[_A]; \
}

#define XOR(value) { \
	_A ^= value; \
	_F = SZP[_A]; \
}

#define CP(value) { \
	uint16 val = value; \
	uint16 res = _A - val; \
	_F = (SZ[res & 0xff] & (SF | ZF)) | (val & (YF | XF)) | ((res >> 8) & CF) | NF | ((_A ^ res ^ val) & HF) | ((((val ^ _A) & (_A ^ res)) >> 5) & VF); \
}

#define RLCA() { \
	_A = (_A << 1) | (_A >> 7); \
	_F = (_F & (SF | ZF | PF)) | (_A & (YF | XF | CF)); \
}

#define RRCA() { \
	_F = (_F & (SF | ZF | PF)) | (_A & CF); \
	_A = (_A >> 1) | (_A << 7); \
	_F |= (_A & (YF | XF)); \
}

#define RLA() { \
	uint8 res = (_A << 1) | (_F & CF); \
	uint8 c = (_A & 0x80) ? CF : 0; \
	_F = (_F & (SF | ZF | PF)) | c | (res & (YF | XF)); \
	_A = res; \
}

#define RRA() { \
	uint8 res = (_A >> 1) | (_F << 7); \
	uint8 c = (_A & 0x01) ? CF : 0; \
	_F = (_F & (SF | ZF | PF)) | c | (res & (YF | XF)); \
	_A = res; \
}

#define RRD() { \
	uint8 n = RM8(HL); \
	WM8(HL, (n >> 4) | (_A << 4)); \
	_A = (_A & 0xf0) | (n & 0x0f); \
	_F = (_F & CF) | SZP[_A]; \
}

#define RLD() { \
	uint8 n = RM8(HL); \
	WM8(HL, (n << 4) | (_A & 0x0f)); \
	_A = (_A & 0xf0) | (n >> 4); \
	_F = (_F & CF) | SZP[_A]; \
}

inline uint8 Z80::RLC(uint8 value) {
	uint8 res = value;
	uint8 c = (res & 0x80) ? CF : 0;
	res = ((res << 1) | (res >> 7)) & 0xff;
	_F = SZP[res] | c;
	return res;
}

inline uint8 Z80::RRC(uint8 value) {
	uint8 res = value;
	uint8 c = (res & 0x01) ? CF : 0;
	res = ((res >> 1) | (res << 7)) & 0xff;
	_F = SZP[res] | c;
	return res;
}

inline uint8 Z80::RL(uint8 value) {
	uint8 res = value;
	uint8 c = (res & 0x80) ? CF : 0;
	res = ((res << 1) | (_F & CF)) & 0xff;
	_F = SZP[res] | c;
	return res;
}

inline uint8 Z80::RR(uint8 value) {
	uint8 res = value;
	uint8 c = (res & 0x01) ? CF : 0;
	res = ((res >> 1) | (_F << 7)) & 0xff;
	_F = SZP[res] | c;
	return res;
}

inline uint8 Z80::SLA(uint8 value) {
	uint8 res = value;
	uint8 c = (res & 0x80) ? CF : 0;
	res = (res << 1) & 0xff;
	_F = SZP[res] | c;
	return res;
}

inline uint8 Z80::SRA(uint8 value) {
	uint8 res = value;
	uint8 c = (res & 0x01) ? CF : 0;
	res = ((res >> 1) | (res & 0x80)) & 0xff;
	_F = SZP[res] | c;
	return res;
}

inline uint8 Z80::SLL(uint8 value) {
	uint8 res = value;
	uint8 c = (res & 0x80) ? CF : 0;
	res = ((res << 1) | 0x01) & 0xff;
	_F = SZP[res] | c;
	return res;
}

inline uint8 Z80::SRL(uint8 value) {
	uint8 res = value;
	uint8 c = (res & 0x01) ? CF : 0;
	res = (res >> 1) & 0xff;
	_F = SZP[res] | c;
	return res;
}

#define BIT(bit, reg) { \
	_F = (_F & CF) | HF | SZ_BIT[reg & (1 << bit)]; \
}

#define BIT_XY(bit, reg) { \
	_F = (_F & CF) | HF | (SZ_BIT[reg & (1 << bit)] & ~(YF | XF)) | ((EA >> 8) & (YF | XF)); \
}

inline uint8 Z80::RES(uint8 bit, uint8 value) {
	return value & ~(1 << bit);
}

inline uint8 Z80::SET(uint8 bit, uint8 value) {
	return value | (1 << bit);
}

#define LDI() { \
	uint8 io = RM8(HL); \
	WM8(DE, io); \
	_F &= SF | ZF | CF; \
	if((_A + io) & 0x02) _F |= YF; \
	if((_A + io) & 0x08) _F |= XF; \
	HL++; DE++; BC--; \
	if(BC) _F |= VF; \
}

#define CPI() { \
	uint8 val = RM8(HL); \
	uint8 res = _A - val; \
	HL++; BC--; \
	_F = (_F & CF) | (SZ[res] & ~(YF | XF)) | ((_A ^ val ^ res) & HF) | NF; \
	if(_F & HF) res -= 1; \
	if(res & 0x02) _F |= YF; \
	if(res & 0x08) _F |= XF; \
	if(BC) _F |= VF; \
}

#define INI() { \
	uint8 io = IN8(_C, _B); \
	_B--; \
	WM8(HL, io); \
	HL++; \
	_F = SZ[_B]; \
	if(io & SF) _F |= NF; \
	if((((_C + 1) & 0xff) + io) & 0x100) _F |= HF | CF; \
	if((irep_tmp[_C & 3][io & 3] ^ breg_tmp[_B] ^ (_C >> 2) ^ (io >> 2)) & 1) _F |= PF; \
}

#define OUTI() { \
	uint8 io = RM8(HL); \
	_B--; \
	OUT8(_C, _B, io); \
	HL++; \
	_F = SZ[_B]; \
	if(io & SF) _F |= NF; \
	if((((_C + 1) & 0xff) + io) & 0x100) _F |= HF | CF; \
	if((irep_tmp[_C & 3][io & 3] ^ breg_tmp[_B] ^ (_C >> 2) ^ (io >> 2)) & 1) _F |= PF; \
}

#define LDD() { \
	uint8 io = RM8(HL); \
	WM8(DE, io); \
	_F &= SF | ZF | CF; \
	if((_A + io) & 0x02) _F |= YF; \
	if((_A + io) & 0x08) _F |= XF; \
	HL--; DE--; BC--; \
	if(BC) _F |= VF; \
}

#define CPD() { \
	uint8 val = RM8(HL); \
	uint8 res = _A - val; \
	HL--; BC--; \
	_F = (_F & CF) | (SZ[res] & ~(YF | XF)) | ((_A ^ val ^ res) & HF) | NF; \
	if(_F & HF) res -= 1; \
	if(res & 0x02) _F |= YF; \
	if(res & 0x08) _F |= XF; \
	if(BC) _F |= VF; \
}

#define IND() { \
	uint8 io = IN8(_C, _B); \
	_B--; \
	WM8(HL, io); \
	HL--; \
	_F = SZ[_B]; \
	if(io & SF) _F |= NF; \
	if((((_C - 1) & 0xff) + io) & 0x100) _F |= HF | CF; \
	if((drep_tmp[_C & 3][io & 3] ^ breg_tmp[_B] ^ (_C >> 2) ^ (io >> 2)) & 1) _F |= PF; \
}

#define OUTD() { \
	uint8 io = RM8(HL); \
	_B--; \
	OUT8(_C, _B, io); \
	HL--; \
	_F = SZ[_B]; \
	if(io & SF) _F |= NF; \
	if((((_C - 1) & 0xff) + io) & 0x100) _F |= HF | CF; \
	if((drep_tmp[_C & 3][io & 3] ^ breg_tmp[_B] ^ (_C >> 2) ^ (io >> 2)) & 1) _F |= PF; \
}

#define LDIR() { \
	LDI(); \
	if(BC) { \
		PC -= 2; \
		count -= cc_ex[0xb0]; \
	} \
}

#define CPIR() { \
	for(;;) { \
		CPI(); \
		if(BC && !(_F & ZF)) \
			count -= cc_ex[0xb1]; \
		else \
			break; \
	} \
}

#define INIR() { \
	for(;;) { \
		INI(); \
		if(_B) \
			count -= cc_ex[0xb2]; \
		else \
			break; \
	} \
}

#define OTIR() { \
	for(;;) { \
		OUTI(); \
		if(_B) \
			count -= cc_ex[0xb3]; \
		else \
			break; \
	} \
}

#define LDDR() { \
	for(;;) { \
		LDD(); \
		if(BC) \
			count -= cc_ex[0xb8]; \
		else \
			break; \
	} \
}

#define CPDR() { \
	for(;;) { \
		CPD(); \
		if(BC && !(_F & ZF)) \
			count -= cc_ex[0xb9]; \
		else \
			break; \
	} \
}

#define INDR() { \
	for(;;) { \
		IND(); \
		if(_B) \
			count -= cc_ex[0xba]; \
		else \
			break; \
	} \
}

#define OTDR() { \
	for(;;) { \
		OUTD(); \
		if(_B) \
			count -= cc_ex[0xbb]; \
		else \
			break; \
	} \
}

#ifdef NSC800
#define NSC800_INT(v) { \
	if(halt) { \
		PC++; halt = 0; \
	} \
	PUSH16(PC); PC = (v); count -= 7; IFF1 = IFF2 = 0; \
}
#endif

// main

void Z80::reset()
{
	// reset
	PC = CPU_START_ADDR;
//	AF = SP = 0xffff;
	_I = _R = 0;
	IM = IFF1 = IFF2 = ICR = 0;
	halt = false;
	intr_req_bit = intr_pend_bit = 0;
}

void Z80::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_CPU_NMI)
		intr_req_bit = (data & mask) ? (intr_req_bit | NMI_REQ_BIT) : (intr_req_bit & ~NMI_REQ_BIT);
	else if(id == SIG_CPU_BUSREQ) {
		busreq = ((data & mask) != 0);
		if(busreq)
			count = first = 0;
		// busack
		for(int i = 0; i < dcount; i++)
			d_busack[i]->write_signal(did[i], busreq ? 0xffffffff : 0, dmask[i]);
	}
#ifdef NSC800
	else if(id == SIG_NSC800_INT)
		intr_req_bit = (data & mask) ? (intr_req_bit | 1) : (intr_req_bit & ~1);
	else if(id == SIG_NSC800_RSTA)
		intr_req_bit = (data & mask) ? (intr_req_bit | 8) : (intr_req_bit & ~8);
	else if(id == SIG_NSC800_RSTB)
		intr_req_bit = (data & mask) ? (intr_req_bit | 4) : (intr_req_bit & ~4);
	else if(id == SIG_NSC800_RSTC)
		intr_req_bit = (data & mask) ? (intr_req_bit | 2) : (intr_req_bit & ~2);
#endif
}

void Z80::run(int clock)
{
	// return now if BUSREQ
	if(busreq) {
		count = first = 0;
		return;
	}
	
	// run cpu while given clocks
	count += clock;
	first = count;
	while(count > 0) {
		OP(FETCHOP());
		if(intr_req_bit) {
			if(intr_req_bit & NMI_REQ_BIT) {
				// nmi
				if(halt) {
					PC++;
					halt = false;
				}
				PUSH16(PC);
				PC = 0x0066;
				count -= 5;
				IFF1 = 0;
				intr_req_bit &= ~NMI_REQ_BIT;
			}
#ifdef NSC800
			else if((intr_req_bit & 1) && (ICR & 1)) {
				// INTR
				uint8 vector = ACK_INTR();
				NSC800_INT(vector);
				intr_req_bit &= ~1;
			}
			else if((intr_req_bit & 8) && (ICR & 8)) {
				// RSTA
				NSC800_INT(0x3c);
				intr_req_bit &= ~8;
			}
			else if((intr_req_bit & 4) && (ICR & 4)) {
				// RSTB
				NSC800_INT(0x34);
				intr_req_bit &= ~4;
			}
			else if((intr_req_bit & 2) && (ICR & 2)) {
				// RSTC
				NSC800_INT(0x2c);
				intr_req_bit &= ~2;
			}
#else
			else if(IFF1) {
				// interrupt
				if(halt) {
					PC++;
					halt = false;
				}
				uint32 vector = ACK_INTR();
				uint8 v0 = vector;
				uint16 v12 = vector >> 8;
				if(IM == 0) {
					// mode 0 (support CALL/RST only)
					PUSH16(PC);
					switch(v0)
					{
					case 0xcd:		// CALL
						PC = v12;
						break;
					case 0xc7:		// RST 00H
						PC = 0x0000;
						break;
					case 0xcf:		// RST 08H
						PC = 0x0008;
						break;
					case 0xd7:		// RST 10H
						PC = 0x0010;
						break;
					case 0xdf:		// RST 18H
						PC = 0x0018;
						break;
					case 0xe7:		// RST 20H
						PC = 0x0020;
						break;
					case 0xef:		// RST 28H
						PC = 0x0028;
						break;
					case 0xf7:		// RST 30H
						PC = 0x0030;
						break;
					case 0xff:		// RST 38H
						PC = 0x0038;
						break;
					}
					count -= 7;
				}
				else if(IM == 1) {
					// mode 1
					PUSH16(PC);
					PC = 0x0038;
					count -= 7;
				}
				else {
					// mode 2
					PUSH16(PC);
					PC = RM16((_I << 8) | v0);
					count -= 7;
				}
				IFF1 = IFF2 = 0;
				intr_req_bit = 0;
			}
			else
				intr_req_bit &= intr_pend_bit;
#endif
		}
	}
	first = count;
}

void Z80::OP(uint8 code)
{
	prvPC = PC - 1;
	count -= cc_op[code];
	
	switch(code)
	{
	case 0x00: // NOP
		break;
	case 0x01: // LD BC, w
		BC = FETCH16();
		break;
	case 0x02: // LD (BC), A
		WM8(BC, _A);
		break;
	case 0x03: // INC BC
		BC++;
		break;
	case 0x04: // INC B
		_B = INC(_B);
		break;
	case 0x05: // DEC B
		_B = DEC(_B);
		break;
	case 0x06: // LD B, n
		_B = FETCH8();
		break;
	case 0x07: // RLCA
		RLCA();
		break;
	case 0x08: // EX AF, AF'
		EX_AF();
		break;
	case 0x09: // ADD HL, BC
		HL = ADD16(HL, BC);
		break;
	case 0x0a: // LD A, (BC)
		_A = RM8(BC);
		break;
	case 0x0b: // DEC BC
		BC--;
		break;
	case 0x0c: // INC C
		_C = INC(_C);
		break;
	case 0x0d: // DEC C
		_C = DEC(_C);
		break;
	case 0x0e: // LD C, n
		_C = FETCH8();
		break;
	case 0x0f: // RRCA
		RRCA();
		break;
	case 0x10: // DJNZ o
		_B--;
		JR_COND(_B, 0x10);
		break;
	case 0x11: // LD DE, w
		DE = FETCH16();
		break;
	case 0x12: // LD (DE), A
		WM8(DE, _A);
		break;
	case 0x13: // INC DE
		DE++;
		break;
	case 0x14: // INC D
		_D = INC(_D);
		break;
	case 0x15: // DEC D
		_D = DEC(_D);
		break;
	case 0x16: // LD D, n
		_D = FETCH8();
		break;
	case 0x17: // RLA
		RLA();
		break;
	case 0x18: // JR o
		JR();
		break;
	case 0x19: // ADD HL, DE
		HL = ADD16(HL, DE);
		break;
	case 0x1a: // LD A, (DE)
		_A = RM8(DE);
		break;
	case 0x1b: // DEC DE
		DE--;
		break;
	case 0x1c: // INC E
		_E = INC(_E);
		break;
	case 0x1d: // DEC E
		_E = DEC(_E);
		break;
	case 0x1e: // LD E, n
		_E = FETCH8();
		break;
	case 0x1f: // RRA
		RRA();
		break;
	case 0x20: // JR NZ, o
		JR_COND(!(_F & ZF), 0x20);
		break;
	case 0x21: // LD HL, w
		HL = FETCH16();
		break;
	case 0x22: // LD (w), HL
		EA = FETCH16();
		WM16(EA, HL);
		break;
	case 0x23: // INC HL
		HL++;
		break;
	case 0x24: // INC H
		_H = INC(_H);
		break;
	case 0x25: // DEC H
		_H = DEC(_H);
		break;
	case 0x26: // LD H, n
		_H = FETCH8();
		break;
	case 0x27: // DAA
		DAA();
		break;
	case 0x28: // JR Z, o
		JR_COND(_F & ZF, 0x28);
		break;
	case 0x29: // ADD HL, HL
		HL = ADD16(HL, HL);
		break;
	case 0x2a: // LD HL, (w)
		EA = FETCH16();
		HL = RM16(EA);
		break;
	case 0x2b: // DEC HL
		HL--;
		break;
	case 0x2c: // INC L
		_L = INC(_L);
		break;
	case 0x2d: // DEC L
		_L = DEC(_L);
		break;
	case 0x2e: // LD L, n
		_L = FETCH8();
		break;
	case 0x2f: // CPL
		_A ^= 0xff;
		_F = (_F & (SF | ZF | PF | CF)) | HF | NF | (_A & (YF | XF));
		break;
	case 0x30: // JR NC, o
		JR_COND(!(_F & CF), 0x30);
		break;
	case 0x31: // LD SP, w
		SP = FETCH16();
		break;
	case 0x32: // LD (w), A
		EA = FETCH16();
		WM8(EA, _A);
		break;
	case 0x33: // INC SP
		SP++;
		break;
	case 0x34: // INC (HL)
		WM8(HL, INC(RM8(HL)));
		break;
	case 0x35: // DEC (HL)
		WM8(HL, DEC(RM8(HL)));
		break;
	case 0x36: // LD (HL), n
		WM8(HL, FETCH8());
		break;
	case 0x37: // SCF
		_F = (_F & (SF | ZF | PF)) | CF | (_A & (YF | XF));
		break;
	case 0x38: // JR C, o
		JR_COND(_F & CF, 0x38);
		break;
	case 0x39: // ADD HL, SP
		HL = ADD16(HL, SP);
		break;
	case 0x3a: // LD A, (w)
		EA = FETCH16();
		_A = RM8(EA);
		break;
	case 0x3b: // DEC SP
		SP--;
		break;
	case 0x3c: // INC A
		_A = INC(_A);
		break;
	case 0x3d: // DEC A
		_A = DEC(_A);
		break;
	case 0x3e: // LD A, n
		_A = FETCH8();
		break;
	case 0x3f: // CCF
		_F = ((_F & (SF | ZF | PF | CF)) | ((_F & CF) << 4) | (_A & (YF | XF))) ^ CF;
		break;
	case 0x40: // LD B, B
		break;
	case 0x41: // LD B, C
		_B = _C;
		break;
	case 0x42: // LD B, D
		_B = _D;
		break;
	case 0x43: // LD B, E
		_B = _E;
		break;
	case 0x44: // LD B, H
		_B = _H;
		break;
	case 0x45: // LD B, L
		_B = _L;
		break;
	case 0x46: // LD B, (HL)
		_B = RM8(HL);
		break;
	case 0x47: // LD B, A
		_B = _A;
		break;
	case 0x48: // LD C, B
		_C = _B;
		break;
	case 0x49: // LD C, C
		break;
	case 0x4a: // LD C, D
		_C = _D;
		break;
	case 0x4b: // LD C, E
		_C = _E;
		break;
	case 0x4c: // LD C, H
		_C = _H;
		break;
	case 0x4d: // LD C, L
		_C = _L;
		break;
	case 0x4e: // LD C, (HL)
		_C = RM8(HL);
		break;
	case 0x4f: // LD C, A
		_C = _A;
		break;
	case 0x50: // LD D, B
		_D = _B;
		break;
	case 0x51: // LD D, C
		_D = _C;
		break;
	case 0x52: // LD D, D
		break;
	case 0x53: // LD D, E
		_D = _E;
		break;
	case 0x54: // LD D, H
		_D = _H;
		break;
	case 0x55: // LD D, L
		_D = _L;
		break;
	case 0x56: // LD D, (HL)
		_D = RM8(HL);
		break;
	case 0x57: // LD D, A
		_D = _A;
		break;
	case 0x58: // LD E, B
		_E = _B;
		break;
	case 0x59: // LD E, C
		_E = _C;
		break;
	case 0x5a: // LD E, D
		_E = _D;
		break;
	case 0x5b: // LD E, E
		break;
	case 0x5c: // LD E, H
		_E = _H;
		break;
	case 0x5d: // LD E, L
		_E = _L;
		break;
	case 0x5e: // LD E, (HL)
		_E = RM8(HL);
		break;
	case 0x5f: // LD E, A
		_E = _A;
		break;
	case 0x60: // LD H, B
		_H = _B;
		break;
	case 0x61: // LD H, C
		_H = _C;
		break;
	case 0x62: // LD H, D
		_H = _D;
		break;
	case 0x63: // LD H, E
		_H = _E;
		break;
	case 0x64: // LD H, H
		break;
	case 0x65: // LD H, L
		_H = _L;
		break;
	case 0x66: // LD H, (HL)
		_H = RM8(HL);
		break;
	case 0x67: // LD H, A
		_H = _A;
		break;
	case 0x68: // LD L, B
		_L = _B;
		break;
	case 0x69: // LD L, C
		_L = _C;
		break;
	case 0x6a: // LD L, D
		_L = _D;
		break;
	case 0x6b: // LD L, E
		_L = _E;
		break;
	case 0x6c: // LD L, H
		_L = _H;
		break;
	case 0x6d: // LD L, L
		break;
	case 0x6e: // LD L, (HL)
		_L = RM8(HL);
		break;
	case 0x6f: // LD L, A
		_L = _A;
		break;
	case 0x70: // LD (HL), B
		WM8(HL, _B);
		break;
	case 0x71: // LD (HL), C
		WM8(HL, _C);
		break;
	case 0x72: // LD (HL), D
		WM8(HL, _D);
		break;
	case 0x73: // LD (HL), E
		WM8(HL, _E);
		break;
	case 0x74: // LD (HL), H
		WM8(HL, _H);
		break;
	case 0x75: // LD (HL), L
		WM8(HL, _L);
		break;
	case 0x76: // HALT
		PC--;
		halt = true;
		break;
	case 0x77: // LD (HL), A
		WM8(HL, _A);
		break;
	case 0x78: // LD A, B
		_A = _B;
		break;
	case 0x79: // LD A, C
		_A = _C;
		break;
	case 0x7a: // LD A, D
		_A = _D;
		break;
	case 0x7b: // LD A, E
		_A = _E;
		break;
	case 0x7c: // LD A, H
		_A = _H;
		break;
	case 0x7d: // LD A, L
		_A = _L;
		break;
	case 0x7e: // LD A, (HL)
		_A = RM8(HL);
		break;
	case 0x7f: // LD A, A
		break;
	case 0x80: // ADD A, B
		ADD(_B);
		break;
	case 0x81: // ADD A, C
		ADD(_C);
		break;
	case 0x82: // ADD A, D
		ADD(_D);
		break;
	case 0x83: // ADD A, E
		ADD(_E);
		break;
	case 0x84: // ADD A, H
		ADD(_H);
		break;
	case 0x85: // ADD A, L
		ADD(_L);
		break;
	case 0x86: // ADD A, (HL)
		ADD(RM8(HL));
		break;
	case 0x87: // ADD A, A
		ADD(_A);
		break;
	case 0x88: // ADC A, B
		ADC(_B);
		break;
	case 0x89: // ADC A, C
		ADC(_C);
		break;
	case 0x8a: // ADC A, D
		ADC(_D);
		break;
	case 0x8b: // ADC A, E
		ADC(_E);
		break;
	case 0x8c: // ADC A, H
		ADC(_H);
		break;
	case 0x8d: // ADC A, L
		ADC(_L);
		break;
	case 0x8e: // ADC A, (HL)
		ADC(RM8(HL));
		break;
	case 0x8f: // ADC A, A
		ADC(_A);
		break;
	case 0x90: // SUB B
		SUB(_B);
		break;
	case 0x91: // SUB C
		SUB(_C);
		break;
	case 0x92: // SUB D
		SUB(_D);
		break;
	case 0x93: // SUB E
		SUB(_E);
		break;
	case 0x94: // SUB H
		SUB(_H);
		break;
	case 0x95: // SUB L
		SUB(_L);
		break;
	case 0x96: // SUB (HL)
		SUB(RM8(HL));
		break;
	case 0x97: // SUB A
		SUB(_A);
		break;
	case 0x98: // SBC A, B
		SBC(_B);
		break;
	case 0x99: // SBC A, C
		SBC(_C);
		break;
	case 0x9a: // SBC A, D
		SBC(_D);
		break;
	case 0x9b: // SBC A, E
		SBC(_E);
		break;
	case 0x9c: // SBC A, H
		SBC(_H);
		break;
	case 0x9d: // SBC A, L
		SBC(_L);
		break;
	case 0x9e: // SBC A, (HL)
		SBC(RM8(HL));
		break;
	case 0x9f: // SBC A, A
		SBC(_A);
		break;
	case 0xa0: // AND B
		AND(_B);
		break;
	case 0xa1: // AND C
		AND(_C);
		break;
	case 0xa2: // AND D
		AND(_D);
		break;
	case 0xa3: // AND E
		AND(_E);
		break;
	case 0xa4: // AND H
		AND(_H);
		break;
	case 0xa5: // AND L
		AND(_L);
		break;
	case 0xa6: // AND (HL)
		AND(RM8(HL));
		break;
	case 0xa7: // AND A
		AND(_A);
		break;
	case 0xa8: // XOR B
		XOR(_B);
		break;
	case 0xa9: // XOR C
		XOR(_C);
		break;
	case 0xaa: // XOR D
		XOR(_D);
		break;
	case 0xab: // XOR E
		XOR(_E);
		break;
	case 0xac: // XOR H
		XOR(_H);
		break;
	case 0xad: // XOR L
		XOR(_L);
		break;
	case 0xae: // XOR (HL)
		XOR(RM8(HL));
		break;
	case 0xaf: // XOR A
		XOR(_A);
		break;
	case 0xb0: // OR B
		OR(_B);
		break;
	case 0xb1: // OR C
		OR(_C);
		break;
	case 0xb2: // OR D
		OR(_D);
		break;
	case 0xb3: // OR E
		OR(_E);
		break;
	case 0xb4: // OR H
		OR(_H);
		break;
	case 0xb5: // OR L
		OR(_L);
		break;
	case 0xb6: // OR (HL)
		OR(RM8(HL));
		break;
	case 0xb7: // OR A
		OR(_A);
		break;
	case 0xb8: // CP B
		CP(_B);
		break;
	case 0xb9: // CP C
		CP(_C);
		break;
	case 0xba: // CP D
		CP(_D);
		break;
	case 0xbb: // CP E
		CP(_E);
		break;
	case 0xbc: // CP H
		CP(_H);
		break;
	case 0xbd: // CP L
		CP(_L);
		break;
	case 0xbe: // CP (HL)
		CP(RM8(HL));
		break;
	case 0xbf: // CP A
		CP(_A);
		break;
	case 0xc0: // RET NZ
		RET_COND(!(_F & ZF), 0xc0);
		break;
	case 0xc1: // POP BC
		BC = POP16();
		break;
	case 0xc2: // JP NZ, a
		JP_COND(!(_F & ZF));
		break;
	case 0xc3: // JP a
		JP();
		break;
	case 0xc4: // CALL NZ, a
		CALL_COND(!(_F & ZF), 0xc4);
		break;
	case 0xc5: // PUSH BC
		PUSH16(BC);
		break;
	case 0xc6: // ADD A, n
		ADD(FETCH8());
		break;
	case 0xc7: // RST 0
		RST(0x00);
		break;
	case 0xc8: // RET Z
		RET_COND(_F & ZF, 0xc8);
		break;
	case 0xc9: // RET
		RET();
		break;
	case 0xca: // JP Z, a
		JP_COND(_F & ZF);
		break;
	case 0xcb: // **** CB xx
		OP_CB();
		break;
	case 0xcc: // CALL Z, a
		CALL_COND(_F & ZF, 0xcc);
		break;
	case 0xcd: // CALL a
		CALL();
		break;
	case 0xce: // ADC A, n
		ADC(FETCH8());
		break;
	case 0xcf: // RST 1
		RST(0x08);
		break;
	case 0xd0: // RET NC
		RET_COND(!(_F & CF), 0xd0);
		break;
	case 0xd1: // POP DE
		DE = POP16();
		break;
	case 0xd2: // JP NC, a
		JP_COND(!(_F & CF));
		break;
	case 0xd3: // OUT (n), A
		OUT8(FETCH8(), _A, _A);
		break;
	case 0xd4: // CALL NC, a
		CALL_COND(!(_F & CF), 0xd4);
		break;
	case 0xd5: // PUSH DE
		PUSH16(DE);
		break;
	case 0xd6: // SUB n
		SUB(FETCH8());
		break;
	case 0xd7: // RST 2
		RST(0x10);
		break;
	case 0xd8: // RET C
		RET_COND(_F & CF, 0xd8);
		break;
	case 0xd9: // EXX
		EXX();
		break;
	case 0xda: // JP C, a
		JP_COND(_F & CF);
		break;
	case 0xdb: // IN A, (n)
		_A = IN8(FETCH8(), _A);
		break;
	case 0xdc: // CALL C, a
		CALL_COND(_F & CF, 0xdc);
		break;
	case 0xdd: // **** DD xx
		OP_DD();
		break;
	case 0xde: // SBC A, n
		SBC(FETCH8());
		break;
	case 0xdf: // RST 3
		RST(0x18);
		break;
	case 0xe0: // RET PO
		RET_COND(!(_F & PF), 0xe0);
		break;
	case 0xe1: // POP HL
		HL = POP16();
		break;
	case 0xe2: // JP PO, a
		JP_COND(!(_F & PF));
		break;
	case 0xe3: // EX HL, (SP)
		HL = EXSP(HL);
		break;
	case 0xe4: // CALL PO, a
		CALL_COND(!(_F & PF), 0xe4);
		break;
	case 0xe5: // PUSH HL
		PUSH16(HL);
		break;
	case 0xe6: // AND n
		AND(FETCH8());
		break;
	case 0xe7: // RST 4
		RST(0x20);
		break;
	case 0xe8: // RET PE
		RET_COND(_F & PF, 0xe8);
		break;
	case 0xe9: // JP (HL)
		PC = HL;
		break;
	case 0xea: // JP PE, a
		JP_COND(_F & PF);
		break;
	case 0xeb: // EX DE, HL
		EX_DE_HL();
		break;
	case 0xec: // CALL PE, a
		CALL_COND(_F & PF, 0xec);
		break;
	case 0xed: // **** ED xx
		OP_ED();
		break;
	case 0xee: // XOR n
		XOR(FETCH8());
		break;
	case 0xef: // RST 5
		RST(0x28);
		break;
	case 0xf0: // RET P
		RET_COND(!(_F & SF), 0xf0);
		break;
	case 0xf1: // POP AF
		AF = POP16();
		break;
	case 0xf2: // JP P, a
		JP_COND(!(_F & SF));
		break;
	case 0xf3: // DI
		DI();
		break;
	case 0xf4: // CALL P, a
		CALL_COND(!(_F & SF), 0xf4);
		break;
	case 0xf5: // PUSH AF
		PUSH16(AF);
		break;
	case 0xf6: // OR n
		OR(FETCH8());
		break;
	case 0xf7: // RST 6
		RST(0x30);
		break;
	case 0xf8: // RET M
		RET_COND(_F & SF, 0xf8);
		break;
	case 0xf9: // LD SP, HL
		SP = HL;
		break;
	case 0xfa: // JP M, a
		JP_COND(_F & SF);
		break;
	case 0xfb: // EI
		EI();
		break;
	case 0xfc: // CALL M, a
		CALL_COND(_F & SF, 0xfc);
		break;
	case 0xfd: // **** FD xx
		OP_FD();
		break;
	case 0xfe: // CP n
		CP(FETCH8());
		break;
	case 0xff: // RST 7
		RST(0x38);
		break;
	}
}

void Z80::OP_CB()
{
	uint8 code = FETCHOP();
	count -= cc_cb[code];
	
	switch(code)
	{
	case 0x00: // RLC B
		_B = RLC(_B);
		break;
	case 0x01: // RLC C
		_C = RLC(_C);
		break;
	case 0x02: // RLC D
		_D = RLC(_D);
		break;
	case 0x03: // RLC E
		_E = RLC(_E);
		break;
	case 0x04: // RLC H
		_H = RLC(_H);
		break;
	case 0x05: // RLC L
		_L = RLC(_L);
		break;
	case 0x06: // RLC (HL)
		WM8(HL, RLC(RM8(HL)));
		break;
	case 0x07: // RLC A
		_A = RLC(_A);
		break;
	case 0x08: // RRC B
		_B = RRC(_B);
		break;
	case 0x09: // RRC C
		_C = RRC(_C);
		break;
	case 0x0a: // RRC D
		_D = RRC(_D);
		break;
	case 0x0b: // RRC E
		_E = RRC(_E);
		break;
	case 0x0c: // RRC H
		_H = RRC(_H);
		break;
	case 0x0d: // RRC L
		_L = RRC(_L);
		break;
	case 0x0e: // RRC (HL)
		WM8(HL, RRC(RM8(HL)));
		break;
	case 0x0f: // RRC A
		_A = RRC(_A);
		break;
	case 0x10: // RL B
		_B = RL(_B);
		break;
	case 0x11: // RL C
		_C = RL(_C);
		break;
	case 0x12: // RL D
		_D = RL(_D);
		break;
	case 0x13: // RL E
		_E = RL(_E);
		break;
	case 0x14: // RL H
		_H = RL(_H);
		break;
	case 0x15: // RL L
		_L = RL(_L);
		break;
	case 0x16: // RL (HL)
		WM8(HL, RL(RM8(HL)));
		break;
	case 0x17: // RL A
		_A = RL(_A);
		break;
	case 0x18: // RR B
		_B = RR(_B);
		break;
	case 0x19: // RR C
		_C = RR(_C);
		break;
	case 0x1a: // RR D
		_D = RR(_D);
		break;
	case 0x1b: // RR E
		_E = RR(_E);
		break;
	case 0x1c: // RR H
		_H = RR(_H);
		break;
	case 0x1d: // RR L
		_L = RR(_L);
		break;
	case 0x1e: // RR (HL)
		WM8(HL, RR(RM8(HL)));
		break;
	case 0x1f: // RR A
		_A = RR(_A);
		break;
	case 0x20: // SLA B
		_B = SLA(_B);
		break;
	case 0x21: // SLA C
		_C = SLA(_C);
		break;
	case 0x22: // SLA D
		_D = SLA(_D);
		break;
	case 0x23: // SLA E
		_E = SLA(_E);
		break;
	case 0x24: // SLA H
		_H = SLA(_H);
		break;
	case 0x25: // SLA L
		_L = SLA(_L);
		break;
	case 0x26: // SLA (HL)
		WM8(HL, SLA(RM8(HL)));
		break;
	case 0x27: // SLA A
		_A = SLA(_A);
		break;
	case 0x28: // SRA B
		_B = SRA(_B);
		break;
	case 0x29: // SRA C
		_C = SRA(_C);
		break;
	case 0x2a: // SRA D
		_D = SRA(_D);
		break;
	case 0x2b: // SRA E
		_E = SRA(_E);
		break;
	case 0x2c: // SRA H
		_H = SRA(_H);
		break;
	case 0x2d: // SRA L
		_L = SRA(_L);
		break;
	case 0x2e: // SRA (HL)
		WM8(HL, SRA(RM8(HL)));
		break;
	case 0x2f: // SRA A
		_A = SRA(_A);
		break;
	case 0x30: // SLL B
		_B = SLL(_B);
		break;
	case 0x31: // SLL C
		_C = SLL(_C);
		break;
	case 0x32: // SLL D
		_D = SLL(_D);
		break;
	case 0x33: // SLL E
		_E = SLL(_E);
		break;
	case 0x34: // SLL H
		_H = SLL(_H);
		break;
	case 0x35: // SLL L
		_L = SLL(_L);
		break;
	case 0x36: // SLL (HL)
		WM8(HL, SLL(RM8(HL)));
		break;
	case 0x37: // SLL A
		_A = SLL(_A);
		break;
	case 0x38: // SRL B
		_B = SRL(_B);
		break;
	case 0x39: // SRL C
		_C = SRL(_C);
		break;
	case 0x3a: // SRL D
		_D = SRL(_D);
		break;
	case 0x3b: // SRL E
		_E = SRL(_E);
		break;
	case 0x3c: // SRL H
		_H = SRL(_H);
		break;
	case 0x3d: // SRL L
		_L = SRL(_L);
		break;
	case 0x3e: // SRL (HL)
		WM8(HL, SRL(RM8(HL)));
		break;
	case 0x3f: // SRL A
		_A = SRL(_A);
		break;
	case 0x40: // BIT 0, B
		BIT(0, _B);
		break;
	case 0x41: // BIT 0, C
		BIT(0, _C);
		break;
	case 0x42: // BIT 0, D
		BIT(0, _D);
		break;
	case 0x43: // BIT 0, E
		BIT(0, _E);
		break;
	case 0x44: // BIT 0, H
		BIT(0, _H);
		break;
	case 0x45: // BIT 0, L
		BIT(0, _L);
		break;
	case 0x46: // BIT 0, (HL)
		BIT(0, RM8(HL));
		break;
	case 0x47: // BIT 0, A
		BIT(0, _A);
		break;
	case 0x48: // BIT 1, B
		BIT(1, _B);
		break;
	case 0x49: // BIT 1, C
		BIT(1, _C);
		break;
	case 0x4a: // BIT 1, D
		BIT(1, _D);
		break;
	case 0x4b: // BIT 1, E
		BIT(1, _E);
		break;
	case 0x4c: // BIT 1, H
		BIT(1, _H);
		break;
	case 0x4d: // BIT 1, L
		BIT(1, _L);
		break;
	case 0x4e: // BIT 1, (HL)
		BIT(1, RM8(HL));
		break;
	case 0x4f: // BIT 1, A
		BIT(1, _A);
		break;
	case 0x50: // BIT 2, B
		BIT(2, _B);
		break;
	case 0x51: // BIT 2, C
		BIT(2, _C);
		break;
	case 0x52: // BIT 2, D
		BIT(2, _D);
		break;
	case 0x53: // BIT 2, E
		BIT(2, _E);
		break;
	case 0x54: // BIT 2, H
		BIT(2, _H);
		break;
	case 0x55: // BIT 2, L
		BIT(2, _L);
		break;
	case 0x56: // BIT 2, (HL)
		BIT(2, RM8(HL));
		break;
	case 0x57: // BIT 2, A
		BIT(2, _A);
		break;
	case 0x58: // BIT 3, B
		BIT(3, _B);
		break;
	case 0x59: // BIT 3, C
		BIT(3, _C);
		break;
	case 0x5a: // BIT 3, D
		BIT(3, _D);
		break;
	case 0x5b: // BIT 3, E
		BIT(3, _E);
		break;
	case 0x5c: // BIT 3, H
		BIT(3, _H);
		break;
	case 0x5d: // BIT 3, L
		BIT(3, _L);
		break;
	case 0x5e: // BIT 3, (HL)
		BIT(3, RM8(HL));
		break;
	case 0x5f: // BIT 3, A
		BIT(3, _A);
		break;
	case 0x60: // BIT 4, B
		BIT(4, _B);
		break;
	case 0x61: // BIT 4, C
		BIT(4, _C);
		break;
	case 0x62: // BIT 4, D
		BIT(4, _D);
		break;
	case 0x63: // BIT 4, E
		BIT(4, _E);
		break;
	case 0x64: // BIT 4, H
		BIT(4, _H);
		break;
	case 0x65: // BIT 4, L
		BIT(4, _L);
		break;
	case 0x66: // BIT 4, (HL)
		BIT(4, RM8(HL));
		break;
	case 0x67: // BIT 4, A
		BIT(4, _A);
		break;
	case 0x68: // BIT 5, B
		BIT(5, _B);
		break;
	case 0x69: // BIT 5, C
		BIT(5, _C);
		break;
	case 0x6a: // BIT 5, D
		BIT(5, _D);
		break;
	case 0x6b: // BIT 5, E
		BIT(5, _E);
		break;
	case 0x6c: // BIT 5, H
		BIT(5, _H);
		break;
	case 0x6d: // BIT 5, L
		BIT(5, _L);
		break;
	case 0x6e: // BIT 5, (HL)
		BIT(5, RM8(HL));
		break;
	case 0x6f: // BIT 5, A
		BIT(5, _A);
		break;
	case 0x70: // BIT 6, B
		BIT(6, _B);
		break;
	case 0x71: // BIT 6, C
		BIT(6, _C);
		break;
	case 0x72: // BIT 6, D
		BIT(6, _D);
		break;
	case 0x73: // BIT 6, E
		BIT(6, _E);
		break;
	case 0x74: // BIT 6, H
		BIT(6, _H);
		break;
	case 0x75: // BIT 6, L
		BIT(6, _L);
		break;
	case 0x76: // BIT 6, (HL)
		BIT(6, RM8(HL));
		break;
	case 0x77: // BIT 6, A
		BIT(6, _A);
		break;
	case 0x78: // BIT 7, B
		BIT(7, _B);
		break;
	case 0x79: // BIT 7, C
		BIT(7, _C);
		break;
	case 0x7a: // BIT 7, D
		BIT(7, _D);
		break;
	case 0x7b: // BIT 7, E
		BIT(7, _E);
		break;
	case 0x7c: // BIT 7, H
		BIT(7, _H);
		break;
	case 0x7d: // BIT 7, L
		BIT(7, _L);
		break;
	case 0x7e: // BIT 7, (HL)
		BIT(7, RM8(HL));
		break;
	case 0x7f: // BIT 7, A
		BIT(7, _A);
		break;
	case 0x80: // RES 0, B
		_B = RES(0, _B);
		break;
	case 0x81: // RES 0, C
		_C = RES(0, _C);
		break;
	case 0x82: // RES 0, D
		_D = RES(0, _D);
		break;
	case 0x83: // RES 0, E
		_E = RES(0, _E);
		break;
	case 0x84: // RES 0, H
		_H = RES(0, _H);
		break;
	case 0x85: // RES 0, L
		_L = RES(0, _L);
		break;
	case 0x86: // RES 0, (HL)
		WM8(HL, RES(0, RM8(HL)));
		break;
	case 0x87: // RES 0, A
		_A = RES(0, _A);
		break;
	case 0x88: // RES 1, B
		_B = RES(1, _B);
		break;
	case 0x89: // RES 1, C
		_C = RES(1, _C);
		break;
	case 0x8a: // RES 1, D
		_D = RES(1, _D);
		break;
	case 0x8b: // RES 1, E
		_E = RES(1, _E);
		break;
	case 0x8c: // RES 1, H
		_H = RES(1, _H);
		break;
	case 0x8d: // RES 1, L
		_L = RES(1, _L);
		break;
	case 0x8e: // RES 1, (HL)
		WM8(HL, RES(1, RM8(HL)));
		break;
	case 0x8f: // RES 1, A
		_A = RES(1, _A);
		break;
	case 0x90: // RES 2, B
		_B = RES(2, _B);
		break;
	case 0x91: // RES 2, C
		_C = RES(2, _C);
		break;
	case 0x92: // RES 2, D
		_D = RES(2, _D);
		break;
	case 0x93: // RES 2, E
		_E = RES(2, _E);
		break;
	case 0x94: // RES 2, H
		_H = RES(2, _H);
		break;
	case 0x95: // RES 2, L
		_L = RES(2, _L);
		break;
	case 0x96: // RES 2, (HL)
		WM8(HL, RES(2, RM8(HL)));
		break;
	case 0x97: // RES 2, A
		_A = RES(2, _A);
		break;
	case 0x98: // RES 3, B
		_B = RES(3, _B);
		break;
	case 0x99: // RES 3, C
		_C = RES(3, _C);
		break;
	case 0x9a: // RES 3, D
		_D = RES(3, _D);
		break;
	case 0x9b: // RES 3, E
		_E = RES(3, _E);
		break;
	case 0x9c: // RES 3, H
		_H = RES(3, _H);
		break;
	case 0x9d: // RES 3, L
		_L = RES(3, _L);
		break;
	case 0x9e: // RES 3, (HL)
		WM8(HL, RES(3, RM8(HL)));
		break;
	case 0x9f: // RES 3, A
		_A = RES(3, _A);
		break;
	case 0xa0: // RES 4, B
		_B = RES(4, _B);
		break;
	case 0xa1: // RES 4, C
		_C = RES(4, _C);
		break;
	case 0xa2: // RES 4, D
		_D = RES(4, _D);
		break;
	case 0xa3: // RES 4, E
		_E = RES(4, _E);
		break;
	case 0xa4: // RES 4, H
		_H = RES(4, _H);
		break;
	case 0xa5: // RES 4, L
		_L = RES(4, _L);
		break;
	case 0xa6: // RES 4, (HL)
		WM8(HL, RES(4, RM8(HL)));
		break;
	case 0xa7: // RES 4, A
		_A = RES(4, _A);
		break;
	case 0xa8: // RES 5, B
		_B = RES(5, _B);
		break;
	case 0xa9: // RES 5, C
		_C = RES(5, _C);
		break;
	case 0xaa: // RES 5, D
		_D = RES(5, _D);
		break;
	case 0xab: // RES 5, E
		_E = RES(5, _E);
		break;
	case 0xac: // RES 5, H
		_H = RES(5, _H);
		break;
	case 0xad: // RES 5, L
		_L = RES(5, _L);
		break;
	case 0xae: // RES 5, (HL)
		WM8(HL, RES(5, RM8(HL)));
		break;
	case 0xaf: // RES 5, A
		_A = RES(5, _A);
		break;
	case 0xb0: // RES 6, B
		_B = RES(6, _B);
		break;
	case 0xb1: // RES 6, C
		_C = RES(6, _C);
		break;
	case 0xb2: // RES 6, D
		_D = RES(6, _D);
		break;
	case 0xb3: // RES 6, E
		_E = RES(6, _E);
		break;
	case 0xb4: // RES 6, H
		_H = RES(6, _H);
		break;
	case 0xb5: // RES 6, L
		_L = RES(6, _L);
		break;
	case 0xb6: // RES 6, (HL)
		WM8(HL, RES(6, RM8(HL)));
		break;
	case 0xb7: // RES 6, A
		_A = RES(6, _A);
		break;
	case 0xb8: // RES 7, B
		_B = RES(7, _B);
		break;
	case 0xb9: // RES 7, C
		_C = RES(7, _C);
		break;
	case 0xba: // RES 7, D
		_D = RES(7, _D);
		break;
	case 0xbb: // RES 7, E
		_E = RES(7, _E);
		break;
	case 0xbc: // RES 7, H
		_H = RES(7, _H);
		break;
	case 0xbd: // RES 7, L
		_L = RES(7, _L);
		break;
	case 0xbe: // RES 7, (HL)
		WM8(HL, RES(7, RM8(HL)));
		break;
	case 0xbf: // RES 7, A
		_A = RES(7, _A);
		break;
	case 0xc0: // SET 0, B
		_B = SET(0, _B);
		break;
	case 0xc1: // SET 0, C
		_C = SET(0, _C);
		break;
	case 0xc2: // SET 0, D
		_D = SET(0, _D);
		break;
	case 0xc3: // SET 0, E
		_E = SET(0, _E);
		break;
	case 0xc4: // SET 0, H
		_H = SET(0, _H);
		break;
	case 0xc5: // SET 0, L
		_L = SET(0, _L);
		break;
	case 0xc6: // SET 0, (HL)
		WM8(HL, SET(0, RM8(HL)));
		break;
	case 0xc7: // SET 0, A
		_A = SET(0, _A);
		break;
	case 0xc8: // SET 1, B
		_B = SET(1, _B);
		break;
	case 0xc9: // SET 1, C
		_C = SET(1, _C);
		break;
	case 0xca: // SET 1, D
		_D = SET(1, _D);
		break;
	case 0xcb: // SET 1, E
		_E = SET(1, _E);
		break;
	case 0xcc: // SET 1, H
		_H = SET(1, _H);
		break;
	case 0xcd: // SET 1, L
		_L = SET(1, _L);
		break;
	case 0xce: // SET 1, (HL)
		WM8(HL, SET(1, RM8(HL)));
		break;
	case 0xcf: // SET 1, A
		_A = SET(1, _A);
		break;
	case 0xd0: // SET 2, B
		_B = SET(2, _B);
		break;
	case 0xd1: // SET 2, C
		_C = SET(2, _C);
		break;
	case 0xd2: // SET 2, D
		_D = SET(2, _D);
		break;
	case 0xd3: // SET 2, E
		_E = SET(2, _E);
		break;
	case 0xd4: // SET 2, H
		_H = SET(2, _H);
		break;
	case 0xd5: // SET 2, L
		_L = SET(2, _L);
		break;
	case 0xd6: // SET 2, (HL)
		WM8(HL, SET(2, RM8(HL)));
		break;
	case 0xd7: // SET 2, A
		_A = SET(2, _A);
		break;
	case 0xd8: // SET 3, B
		_B = SET(3, _B);
		break;
	case 0xd9: // SET 3, C
		_C = SET(3, _C);
		break;
	case 0xda: // SET 3, D
		_D = SET(3, _D);
		break;
	case 0xdb: // SET 3, E
		_E = SET(3, _E);
		break;
	case 0xdc: // SET 3, H
		_H = SET(3, _H);
		break;
	case 0xdd: // SET 3, L
		_L = SET(3, _L);
		break;
	case 0xde: // SET 3, (HL)
		WM8(HL, SET(3, RM8(HL)));
		break;
	case 0xdf: // SET 3, A
		_A = SET(3, _A);
		break;
	case 0xe0: // SET 4, B
		_B = SET(4, _B);
		break;
	case 0xe1: // SET 4, C
		_C = SET(4, _C);
		break;
	case 0xe2: // SET 4, D
		_D = SET(4, _D);
		break;
	case 0xe3: // SET 4, E
		_E = SET(4, _E);
		break;
	case 0xe4: // SET 4, H
		_H = SET(4, _H);
		break;
	case 0xe5: // SET 4, L
		_L = SET(4, _L);
		break;
	case 0xe6: // SET 4, (HL)
		WM8(HL, SET(4, RM8(HL)));
		break;
	case 0xe7: // SET 4, A
		_A = SET(4, _A);
		break;
	case 0xe8: // SET 5, B
		_B = SET(5, _B);
		break;
	case 0xe9: // SET 5, C
		_C = SET(5, _C);
		break;
	case 0xea: // SET 5, D
		_D = SET(5, _D);
		break;
	case 0xeb: // SET 5, E
		_E = SET(5, _E);
		break;
	case 0xec: // SET 5, H
		_H = SET(5, _H);
		break;
	case 0xed: // SET 5, L
		_L = SET(5, _L);
		break;
	case 0xee: // SET 5, (HL)
		WM8(HL, SET(5, RM8(HL)));
		break;
	case 0xef: // SET 5, A
		_A = SET(5, _A);
		break;
	case 0xf0: // SET 6, B
		_B = SET(6, _B);
		break;
	case 0xf1: // SET 6, C
		_C = SET(6, _C);
		break;
	case 0xf2: // SET 6, D
		_D = SET(6, _D);
		break;
	case 0xf3: // SET 6, E
		_E = SET(6, _E);
		break;
	case 0xf4: // SET 6, H
		_H = SET(6, _H);
		break;
	case 0xf5: // SET 6, L
		_L = SET(6, _L);
		break;
	case 0xf6: // SET 6, (HL)
		WM8(HL, SET(6, RM8(HL)));
		break;
	case 0xf7: // SET 6, A
		_A = SET(6, _A);
		break;
	case 0xf8: // SET 7, B
		_B = SET(7, _B);
		break;
	case 0xf9: // SET 7, C
		_C = SET(7, _C);
		break;
	case 0xfa: // SET 7, D
		_D = SET(7, _D);
		break;
	case 0xfb: // SET 7, E
		_E = SET(7, _E);
		break;
	case 0xfc: // SET 7, H
		_H = SET(7, _H);
		break;
	case 0xfd: // SET 7, L
		_L = SET(7, _L);
		break;
	case 0xfe: // SET 7, (HL)
		WM8(HL, SET(7, RM8(HL)));
		break;
	case 0xff: // SET 7, A
		_A = SET(7, _A);
		break;
	}
}

void Z80::OP_DD()
{
	uint8 code = FETCHOP();
	count -= cc_xy[code];
	
	switch(code)
	{
	case 0x09: // ADD IX, BC
		IX = ADD16(IX, BC);
		break;
	case 0x19: // ADD IX, DE
		IX = ADD16(IX, DE);
		break;
	case 0x21: // LD IX, w
		IX = FETCH16();
		break;
	case 0x22: // LD (w), IX
		EA = FETCH16();
		WM16(EA, IX);
		break;
	case 0x23: // INC IX
		IX++;
		break;
	case 0x24: // INC HX
		_XH = INC(_XH);
		break;
	case 0x25: // DEC HX
		_XH = DEC(_XH);
		break;
	case 0x26: // LD HX, n
		_XH = FETCH8();
		break;
	case 0x29: // ADD IX, IX
		IX = ADD16(IX, IX);
		break;
	case 0x2a: // LD IX, (w)
		EA = FETCH16();
		IX = RM16(EA);
		break;
	case 0x2b: // DEC IX
		IX--;
		break;
	case 0x2c: // INC LX
		_XL = INC(_XL);
		break;
	case 0x2d: // DEC LX
		_XL = DEC(_XL);
		break;
	case 0x2e: // LD LX, n
		_XL = FETCH8();
		break;
	case 0x34: // INC (IX+o)
		EAX();
		WM8(EA, INC(RM8(EA)));
		break;
	case 0x35: // DEC (IX+o)
		EAX();
		WM8(EA, DEC(RM8(EA)));
		break;
	case 0x36: // LD (IX+o), n
		EAX();
		WM8(EA, FETCH8());
		break;
	case 0x39: // ADD IX, SP
		IX = ADD16(IX, SP);
		break;
	case 0x44: // LD B, HX
		_B = _XH;
		break;
	case 0x45: // LD B, LX
		_B = _XL;
		break;
	case 0x46: // LD B, (IX+o)
		EAX();
		_B = RM8(EA);
		break;
	case 0x4c: // LD C, HX
		_C = _XH;
		break;
	case 0x4d: // LD C, LX
		_C = _XL;
		break;
	case 0x4e: // LD C, (IX+o)
		EAX();
		_C = RM8(EA);
		break;
	case 0x54: // LD D, HX
		_D = _XH;
		break;
	case 0x55: // LD D, LX
		_D = _XL;
		break;
	case 0x56: // LD D, (IX+o)
		EAX();
		_D = RM8(EA);
		break;
	case 0x5c: // LD E, HX
		_E = _XH;
		break;
	case 0x5d: // LD E, LX
		_E = _XL;
		break;
	case 0x5e: // LD E, (IX+o)
		EAX();
		_E = RM8(EA);
		break;
	case 0x60: // LD HX, B
		_XH = _B;
		break;
	case 0x61: // LD HX, C
		_XH = _C;
		break;
	case 0x62: // LD HX, D
		_XH = _D;
		break;
	case 0x63: // LD HX, E
		_XH = _E;
		break;
	case 0x64: // LD HX, HX
		break;
	case 0x65: // LD HX, LX
		_XH = _XL;
		break;
	case 0x66: // LD H, (IX+o)
		EAX();
		_H = RM8(EA);
		break;
	case 0x67: // LD HX, A
		_XH = _A;
		break;
	case 0x68: // LD LX, B
		_XL = _B;
		break;
	case 0x69: // LD LX, C
		_XL = _C;
		break;
	case 0x6a: // LD LX, D
		_XL = _D;
		break;
	case 0x6b: // LD LX, E
		_XL = _E;
		break;
	case 0x6c: // LD LX, HX
		_XL = _XH;
		break;
	case 0x6d: // LD LX, LX
		break;
	case 0x6e: // LD L, (IX+o)
		EAX();
		_L = RM8(EA);
		break;
	case 0x6f: // LD LX, A
		_XL = _A;
		break;
	case 0x70: // LD (IX+o), B
		EAX();
		WM8(EA, _B);
		break;
	case 0x71: // LD (IX+o), C
		EAX();
		WM8(EA, _C);
		break;
	case 0x72: // LD (IX+o), D
		EAX();
		WM8(EA, _D);
		break;
	case 0x73: // LD (IX+o), E
		EAX();
		WM8(EA, _E);
		break;
	case 0x74: // LD (IX+o), H
		EAX();
		WM8(EA, _H);
		break;
	case 0x75: // LD (IX+o), L
		EAX();
		WM8(EA, _L);
		break;
	case 0x77: // LD (IX+o), A
		EAX();
		WM8(EA, _A);
		break;
	case 0x7c: // LD A, HX
		_A = _XH;
		break;
	case 0x7d: // LD A, LX
		_A = _XL;
		break;
	case 0x7e: // LD A, (IX+o)
		EAX();
		_A = RM8(EA);
		break;
	case 0x84: // ADD A, HX
		ADD(_XH);
		break;
	case 0x85: // ADD A, LX
		ADD(_XL);
		break;
	case 0x86: // ADD A, (IX+o)
		EAX();
		ADD(RM8(EA));
		break;
	case 0x8c: // ADC A, HX
		ADC(_XH);
		break;
	case 0x8d: // ADC A, LX
		ADC(_XL);
		break;
	case 0x8e: // ADC A, (IX+o)
		EAX();
		ADC(RM8(EA));
		break;
	case 0x94: // SUB HX
		SUB(_XH);
		break;
	case 0x95: // SUB LX
		SUB(_XL);
		break;
	case 0x96: // SUB (IX+o)
		EAX();
		SUB(RM8(EA));
		break;
	case 0x9c: // SBC A, HX
		SBC(_XH);
		break;
	case 0x9d: // SBC A, LX
		SBC(_XL);
		break;
	case 0x9e: // SBC A, (IX+o)
		EAX();
		SBC(RM8(EA));
		break;
	case 0xa4: // AND HX
		AND(_XH);
		break;
	case 0xa5: // AND LX
		AND(_XL);
		break;
	case 0xa6: // AND (IX+o)
		EAX();
		AND(RM8(EA));
		break;
	case 0xac: // XOR HX
		XOR(_XH);
		break;
	case 0xad: // XOR LX
		XOR(_XL);
		break;
	case 0xae: // XOR (IX+o)
		EAX();
		XOR(RM8(EA));
		break;
	case 0xb4: // OR HX
		OR(_XH);
		break;
	case 0xb5: // OR LX
		OR(_XL);
		break;
	case 0xb6: // OR (IX+o)
		EAX();
		OR(RM8(EA));
		break;
	case 0xbc: // CP HX
		CP(_XH);
		break;
	case 0xbd: // CP LX
		CP(_XL);
		break;
	case 0xbe: // CP (IX+o)
		EAX();
		CP(RM8(EA));
		break;
	case 0xcb: // ** DD CB xx
		EAX();
		OP_XY();
		break;
	case 0xe1: // POP IX
		IX = POP16();
		break;
	case 0xe3: // EX (SP), IX
		IX = EXSP(IX);
		break;
	case 0xe5: // PUSH IX
		PUSH16(IX);
		break;
	case 0xe9: // JP (IX)
		PC = IX;
		break;
	case 0xf9: // LD SP, IX
		SP = IX;
		break;
	}
}

void Z80::OP_ED()
{
	uint8 code = FETCHOP();
	count -= cc_ed[code];
	
	switch(code)
	{
	case 0x40: // IN B, (C)
		_B = IN8(_C, _B);
		_F = (_F & CF) | SZP[_B];
		break;
	case 0x41: // OUT (C), B
		OUT8(_C, _B, _B);
		break;
	case 0x42: // SBC HL, BC
		SBC16(BC);
		break;
	case 0x43: // LD (w), BC
		EA = FETCH16();
		WM16(EA, BC);
		break;
	case 0x44: // NEG
		NEG();
		break;
	case 0x45: // RETN;
		RETN();
		break;
	case 0x46: // IM 0
		IM = 0;
		break;
	case 0x47: // LD I, A
		_I = _A;
		break;
	case 0x48: // IN C, (C)
		_C = IN8(_C, _B);
		_F = (_F & CF) | SZP[_C];
		break;
	case 0x49: // OUT (C), C
		OUT8(_C, _B, _C);
		break;
	case 0x4a: // ADC HL, BC
		ADC16(BC);
		break;
	case 0x4b: // LD BC, (w)
		EA = FETCH16();
		BC = RM16(EA);
		break;
	case 0x4c: // NEG
		NEG();
		break;
	case 0x4d: // RETI
		RETI();
		break;
	case 0x4e: // IM 0
		IM = 0;
		break;
	case 0x4f: // LD R, A
		_R = _A;
		break;
	case 0x50: // IN D, (C)
		_D = IN8(_C, _B);
		_F = (_F & CF) | SZP[_D];
		break;
	case 0x51: // OUT (C), D
		OUT8(_C, _B, _D);
		break;
	case 0x52: // SBC HL, DE
		SBC16(DE);
		break;
	case 0x53: // LD (w), DE
		EA = FETCH16();
		WM16(EA, DE);
		break;
	case 0x54: // NEG
		NEG();
		break;
	case 0x55: // RETN;
		RETN();
		break;
	case 0x56: // IM 1
		IM = 1;
		break;
	case 0x57: // LD A, I
		_A = _I;
		_F = (_F & CF) | SZ[_A] | (IFF2 << 2);
		break;
	case 0x58: // IN E, (C)
		_E = IN8(_C, _B);
		_F = (_F & CF) | SZP[_E];
		break;
	case 0x59: // OUT (C), E
		OUT8(_C, _B, _E);
		break;
	case 0x5a: // ADC HL, DE
		ADC16(DE);
		break;
	case 0x5b: // LD DE, (w)
		EA = FETCH16();
		DE = RM16(EA);
		break;
	case 0x5c: // NEG
		NEG();
		break;
	case 0x5d: // RETI
		RETI();
		break;
	case 0x5e: // IM 2
		IM = 2;
		break;
	case 0x5f: // LD A, R
		_A = _R;
		_F = (_F & CF) | SZ[_A] | (IFF2 << 2);
		break;
	case 0x60: // IN H, (C)
		_H = IN8(_C, _B);
		_F = (_F & CF) | SZP[_H];
		break;
	case 0x61: // OUT (C), H
		OUT8(_C, _B, _H);
		break;
	case 0x62: // SBC HL, HL
		SBC16(HL);
		break;
	case 0x63: // LD (w), HL
		EA = FETCH16();
		WM16(EA, HL);
		break;
	case 0x64: // NEG
		NEG();
		break;
	case 0x65: // RETN;
		RETN();
		break;
	case 0x66: // IM 0
		IM = 0;
		break;
	case 0x67: // RRD (HL)
		RRD();
		break;
	case 0x68: // IN L, (C)
		_L = IN8(_C, _B);
		_F = (_F & CF) | SZP[_L];
		break;
	case 0x69: // OUT (C), L
		OUT8(_C, _B, _L);
		break;
	case 0x6a: // ADC HL, HL
		ADC16(HL);
		break;
	case 0x6b: // LD HL, (w)
		EA = FETCH16();
		HL = RM16(EA);
		break;
	case 0x6c: // NEG
		NEG();
		break;
	case 0x6d: // RETI
		RETI();
		break;
	case 0x6e: // IM 0
		IM = 0;
		break;
	case 0x6f: // RLD (HL)
		RLD();
		break;
	case 0x70: // IN 0, (C)
		_F = (_F & CF) | SZP[IN8(_C, _B)];
		break;
	case 0x71: // OUT (C), 0
		OUT8(_C, _B, 0);
		break;
	case 0x72: // SBC HL, SP
		SBC16(SP);
		break;
	case 0x73: // LD (w), SP
		EA = FETCH16();
		WM16(EA, SP);
		break;
	case 0x74: // NEG
		NEG();
		break;
	case 0x75: // RETN;
		RETN();
		break;
	case 0x76: // IM 1
		IM = 1;
		break;
	case 0x78: // IN A, (C)
		_A = IN8(_C, _B);
		_F = (_F & CF) | SZP[_A];
		break;
	case 0x79: // OUT (C), E
		OUT8(_C, _B, _A);
		break;
	case 0x7a: // ADC HL, SP
		ADC16(SP);
		break;
	case 0x7b: // LD SP, (w)
		EA = RM16(PC);
		PC += 2;
		SP = RM16(EA);
		break;
	case 0x7c: // NEG
		NEG();
		break;
	case 0x7d: // RETI
		RETI();
		break;
	case 0x7e: // IM 2
		IM = 2;
		break;
	case 0xa0: // LDI
		LDI();
		break;
	case 0xa1: // CPI
		CPI();
		break;
	case 0xa2: // INI
		INI();
		break;
	case 0xa3: // OUTI
		OUTI();
		break;
	case 0xa8: // LDD
		LDD();
		break;
	case 0xa9: // CPD
		CPD();
		break;
	case 0xaa: // IND
		IND();
		break;
	case 0xab: // OUTD
		OUTD();
		break;
	case 0xb0: // LDIR
		LDIR();
		break;
	case 0xb1: // CPIR
		CPIR();
		break;
	case 0xb2: // INIR
		INIR();
		break;
	case 0xb3: // OTIR
		OTIR();
		break;
	case 0xb8: // LDDR
		LDDR();
		break;
	case 0xb9: // CPDR
		CPDR();
		break;
	case 0xba: // INDR
		INDR();
		break;
	case 0xbb: // OTDR
		OTDR();
		break;
	}
}

void Z80::OP_FD()
{
	uint8 code = FETCHOP();
	count -= cc_xy[code];
	
	switch(code)
	{
	case 0x09: // ADD IY, BC
		IY = ADD16(IY, BC);
		break;
	case 0x19: // ADD IY, DE
		IY = ADD16(IY, DE);
		break;
	case 0x21: // LD IY, w
		IY = RM16(PC);
		PC += 2;
		break;
	case 0x22: // LD (w), IY
		EA = RM16(PC);
		PC += 2;
		WM16(EA, IY);
		break;
	case 0x23: // INC IY
		IY++;
		break;
	case 0x24: // INC HY
		_YH = INC(_YH);
		break;
	case 0x25: // DEC HY
		_YH = DEC(_YH);
		break;
	case 0x26: // LD HY, n
		_YH = FETCH8();
		break;
	case 0x29: // ADD IY, IY
		IY = ADD16(IY, IY);
		break;
	case 0x2a: // LD IY, (w)
		EA = RM16(PC);
		PC += 2;
		IY = RM16(EA);
		break;
	case 0x2b: // DEC IY
		IY--;
		break;
	case 0x2c: // INC LY
		_YL = INC(_YL);
		break;
	case 0x2d: // DEC LY
		_YL = DEC(_YL);
		break;
	case 0x2e: // LD LY, n
		_YL = FETCH8();
		break;
	case 0x34: // INC (IY+o)
		EAY();
		WM8(EA, INC(RM8(EA)));
		break;
	case 0x35: // DEC (IY+o)
		EAY();
		WM8(EA, DEC(RM8(EA)));
		break;
	case 0x36: // LD (IY+o), n
		EAY();
		WM8(EA, FETCH8());
		break;
	case 0x39: // ADD IY, SP
		IY = ADD16(IY, SP);
		break;
	case 0x44: // LD B, HY
		_B = _YH;
		break;
	case 0x45: // LD B, LY
		_B = _YL;
		break;
	case 0x46: // LD B, (IY+o)
		EAY();
		_B = RM8(EA);
		break;
	case 0x4c: // LD C, HY
		_C = _YH;
		break;
	case 0x4d: // LD C, LY
		_C = _YL;
		break;
	case 0x4e: // LD C, (IY+o)
		EAY();
		_C = RM8(EA);
		break;
	case 0x54: // LD D, HY
		_D = _YH;
		break;
	case 0x55: // LD D, LY
		_D = _YL;
		break;
	case 0x56: // LD D, (IY+o)
		EAY();
		_D = RM8(EA);
		break;
	case 0x5c: // LD E, HY
		_E = _YH;
		break;
	case 0x5d: // LD E, LY
		_E = _YL;
		break;
	case 0x5e: // LD E, (IY+o)
		EAY();
		_E = RM8(EA);
		break;
	case 0x60: // LD HY, B
		_YH = _B;
		break;
	case 0x61: // LD HY, C
		_YH = _C;
		break;
	case 0x62: // LD HY, D
		_YH = _D;
		break;
	case 0x63: // LD HY, E
		_YH = _E;
		break;
	case 0x64: // LD HY, HY
		break;
	case 0x65: // LD HY, LY
		_YH = _YL;
		break;
	case 0x66: // LD H, (IY+o)
		EAY();
		_H = RM8(EA);
		break;
	case 0x67: // LD HY, A
		_YH = _A;
		break;
	case 0x68: // LD LY, B
		_YL = _B;
		break;
	case 0x69: // LD LY, C
		_YL = _C;
		break;
	case 0x6a: // LD LY, D
		_YL = _D;
		break;
	case 0x6b: // LD LY, E
		_YL = _E;
		break;
	case 0x6c: // LD LY, HY
		_YL = _YH;
		break;
	case 0x6d: // LD LY, LY
		break;
	case 0x6e: // LD L, (IY+o)
		EAY();
		_L = RM8(EA);
		break;
	case 0x6f: // LD LY, A
		_YL = _A;
		break;
	case 0x70: // LD (IY+o), B
		EAY();
		WM8(EA, _B);
		break;
	case 0x71: // LD (IY+o), C
		EAY();
		WM8(EA, _C);
		break;
	case 0x72: // LD (IY+o), D
		EAY();
		WM8(EA, _D);
		break;
	case 0x73: // LD (IY+o), E
		EAY();
		WM8(EA, _E);
		break;
	case 0x74: // LD (IY+o), H
		EAY();
		WM8(EA, _H);
		break;
	case 0x75: // LD (IY+o), L
		EAY();
		WM8(EA, _L);
		break;
	case 0x77: // LD (IY+o), A
		EAY();
		WM8(EA, _A);
		break;
	case 0x7c: // LD A, HY
		_A = _YH;
		break;
	case 0x7d: // LD A, LY
		_A = _YL;
		break;
	case 0x7e: // LD A, (IY+o)
		EAY();
		_A = RM8(EA);
		break;
	case 0x84: // ADD A, HY
		ADD(_YH);
		break;
	case 0x85: // ADD A, LY
		ADD(_YL);
		break;
	case 0x86: // ADD A, (IY+o)
		EAY();
		ADD(RM8(EA));
		break;
	case 0x8c: // ADC A, HY
		ADC(_YH);
		break;
	case 0x8d: // ADC A, LY
		ADC(_YL);
		break;
	case 0x8e: // ADC A, (IY+o)
		EAY();
		ADC(RM8(EA));
		break;
	case 0x94: // SUB HY
		SUB(_YH);
		break;
	case 0x95: // SUB LY
		SUB(_YL);
		break;
	case 0x96: // SUB (IY+o)
		EAY();
		SUB(RM8(EA));
		break;
	case 0x9c: // SBC A, HY
		SBC(_YH);
		break;
	case 0x9d: // SBC A, LY
		SBC(_YL);
		break;
	case 0x9e: // SBC A, (IY+o)
		EAY();
		SBC(RM8(EA));
		break;
	case 0xa4: // AND HY
		AND(_YH);
		break;
	case 0xa5: // AND LY
		AND(_YL);
		break;
	case 0xa6: // AND (IY+o)
		EAY();
		AND(RM8(EA));
		break;
	case 0xac: // XOR HY
		XOR(_YH);
		break;
	case 0xad: // XOR LY
		XOR(_YL);
		break;
	case 0xae: // XOR (IY+o)
		EAY();
		XOR(RM8(EA));
		break;
	case 0xb4: // OR HY
		OR(_YH);
		break;
	case 0xb5: // OR LY
		OR(_YL);
		break;
	case 0xb6: // OR (IY+o)
		EAY();
		OR(RM8(EA));
		break;
	case 0xbc: // CP HY
		CP(_YH);
		break;
	case 0xbd: // CP LY
		CP(_YL);
		break;
	case 0xbe: // CP (IY+o)
		EAY();
		CP(RM8(EA));
		break;
	case 0xcb: // ** FD CB xx
		EAY();
		OP_XY();
		break;
	case 0xe1: // POP IY
		IY = POP16();
		break;
	case 0xe3: // EX (SP), IY
		IY = EXSP(IY);
		break;
	case 0xe5: // PUSH IY
		PUSH16(IY);
		break;
	case 0xe9: // JP (IY)
		PC = IY;
		break;
	case 0xf9: // LD SP, IY
		SP = IY;
		break;
	}
}

void Z80::OP_XY()
{
	uint8 code = FETCH8();
	count -= cc_xycb[code];
	
	switch(code)
	{
	case 0x00: // RLC B=(XY+o)
		_B = RLC(RM8(EA));
		WM8(EA, _B);
		break;
	case 0x01: // RLC C=(XY+o)
		_C = RLC(RM8(EA));
		WM8(EA, _C);
		break;
	case 0x02: // RLC D=(XY+o)
		_D = RLC(RM8(EA));
		WM8(EA, _D);
		break;
	case 0x03: // RLC E=(XY+o)
		_E = RLC(RM8(EA));
		WM8(EA, _E);
		break;
	case 0x04: // RLC H=(XY+o)
		_H = RLC(RM8(EA));
		WM8(EA, _H);
		break;
	case 0x05: // RLC L=(XY+o)
		_L = RLC(RM8(EA));
		WM8(EA, _L);
		break;
	case 0x06: // RLC (XY+o)
		WM8(EA, RLC(RM8(EA)));
		break;
	case 0x07: // RLC A=(XY+o)
		_A = RLC(RM8(EA));
		WM8(EA, _A);
		break;
	case 0x08: // RRC B=(XY+o)
		_B = RRC(RM8(EA));
		WM8(EA, _B);
		break;
	case 0x09: // RRC C=(XY+o)
		_C = RRC(RM8(EA));
		WM8(EA, _C);
		break;
	case 0x0a: // RRC D=(XY+o)
		_D = RRC(RM8(EA));
		WM8(EA, _D);
		break;
	case 0x0b: // RRC E=(XY+o)
		_E = RRC(RM8(EA));
		WM8(EA, _E);
		break;
	case 0x0c: // RRC H=(XY+o)
		_H = RRC(RM8(EA));
		WM8(EA, _H);
		break;
	case 0x0d: // RRC L=(XY+o)
		_L = RRC(RM8(EA));
		WM8(EA, _L);
		break;
	case 0x0e: // RRC (XY+o)
		WM8(EA, RRC(RM8(EA)));
		break;
	case 0x0f: // RRC A=(XY+o)
		_A = RRC(RM8(EA));
		WM8(EA, _A);
		break;
	case 0x10: // RL B=(XY+o)
		_B = RL(RM8(EA));
		WM8(EA, _B);
		break;
	case 0x11: // RL C=(XY+o)
		_C = RL(RM8(EA));
		WM8(EA, _C);
		break;
	case 0x12: // RL D=(XY+o)
		_D = RL(RM8(EA));
		WM8(EA, _D);
		break;
	case 0x13: // RL E=(XY+o)
		_E = RL(RM8(EA));
		WM8(EA, _E);
		break;
	case 0x14: // RL H=(XY+o)
		_H = RL(RM8(EA));
		WM8(EA, _H);
		break;
	case 0x15: // RL L=(XY+o)
		_L = RL(RM8(EA));
		WM8(EA, _L);
		break;
	case 0x16: // RL (XY+o)
		WM8(EA, RL(RM8(EA)));
		break;
	case 0x17: // RL A=(XY+o)
		_A = RL(RM8(EA));
		WM8(EA, _A);
		break;
	case 0x18: // RR B=(XY+o)
		_B = RR(RM8(EA));
		WM8(EA, _B);
		break;
	case 0x19: // RR C=(XY+o)
		_C = RR(RM8(EA));
		WM8(EA, _C);
		break;
	case 0x1a: // RR D=(XY+o)
		_D = RR(RM8(EA));
		WM8(EA, _D);
		break;
	case 0x1b: // RR E=(XY+o)
		_E = RR(RM8(EA));
		WM8(EA, _E);
		break;
	case 0x1c: // RR H=(XY+o)
		_H = RR(RM8(EA));
		WM8(EA, _H);
		break;
	case 0x1d: // RR L=(XY+o)
		_L = RR(RM8(EA));
		WM8(EA, _L);
		break;
	case 0x1e: // RR (XY+o)
		WM8(EA, RR(RM8(EA)));
		break;
	case 0x1f: // RR A=(XY+o)
		_A = RR(RM8(EA));
		WM8(EA, _A);
		break;
	case 0x20: // SLA B=(XY+o)
		_B = SLA(RM8(EA));
		WM8(EA, _B);
		break;
	case 0x21: // SLA C=(XY+o)
		_C = SLA(RM8(EA));
		WM8(EA, _C);
		break;
	case 0x22: // SLA D=(XY+o)
		_D = SLA(RM8(EA));
		WM8(EA, _D);
		break;
	case 0x23: // SLA E=(XY+o)
		_E = SLA(RM8(EA));
		WM8(EA, _E);
		break;
	case 0x24: // SLA H=(XY+o)
		_H = SLA(RM8(EA));
		WM8(EA, _H);
		break;
	case 0x25: // SLA L=(XY+o)
		_L = SLA(RM8(EA));
		WM8(EA, _L);
		break;
	case 0x26: // SLA (XY+o)
		WM8(EA, SLA(RM8(EA)));
		break;
	case 0x27: // SLA A=(XY+o)
		_A = SLA(RM8(EA));
		WM8(EA, _A);
		break;
	case 0x28: // SRA B=(XY+o)
		_B = SRA(RM8(EA));
		WM8(EA, _B);
		break;
	case 0x29: // SRA C=(XY+o)
		_C = SRA(RM8(EA));
		WM8(EA, _C);
		break;
	case 0x2a: // SRA D=(XY+o)
		_D = SRA(RM8(EA));
		WM8(EA, _D);
		break;
	case 0x2b: // SRA E=(XY+o)
		_E = SRA(RM8(EA));
		WM8(EA, _E);
		break;
	case 0x2c: // SRA H=(XY+o)
		_H = SRA(RM8(EA));
		WM8(EA, _H);
		break;
	case 0x2d: // SRA L=(XY+o)
		_L = SRA(RM8(EA));
		WM8(EA, _L);
		break;
	case 0x2e: // SRA (XY+o)
		WM8(EA, SRA(RM8(EA)));
		break;
	case 0x2f: // SRA A=(XY+o)
		_A = SRA(RM8(EA));
		WM8(EA, _A);
		break;
	case 0x30: // SLL B=(XY+o)
		_B = SLL(RM8(EA));
		WM8(EA, _B);
		break;
	case 0x31: // SLL C=(XY+o)
		_C = SLL(RM8(EA));
		WM8(EA, _C);
		break;
	case 0x32: // SLL D=(XY+o)
		_D = SLL(RM8(EA));
		WM8(EA, _D);
		break;
	case 0x33: // SLL E=(XY+o)
		_E = SLL(RM8(EA));
		WM8(EA, _E);
		break;
	case 0x34: // SLL H=(XY+o)
		_H = SLL(RM8(EA));
		WM8(EA, _H);
		break;
	case 0x35: // SLL L=(XY+o)
		_L = SLL(RM8(EA));
		WM8(EA, _L);
		break;
	case 0x36: // SLL (XY+o)
		WM8(EA, SLL(RM8(EA)));
		break;
	case 0x37: // SLL A=(XY+o)
		_A = SLL(RM8(EA));
		WM8(EA, _A);
		break;
	case 0x38: // SRL B=(XY+o)
		_B = SRL(RM8(EA));
		WM8(EA, _B);
		break;
	case 0x39: // SRL C=(XY+o)
		_C = SRL(RM8(EA));
		WM8(EA, _C);
		break;
	case 0x3a: // SRL D=(XY+o)
		_D = SRL(RM8(EA));
		WM8(EA, _D);
		break;
	case 0x3b: // SRL E=(XY+o)
		_E = SRL(RM8(EA));
		WM8(EA, _E);
		break;
	case 0x3c: // SRL H=(XY+o)
		_H = SRL(RM8(EA));
		WM8(EA, _H);
		break;
	case 0x3d: // SRL L=(XY+o)
		_L = SRL(RM8(EA));
		WM8(EA, _L);
		break;
	case 0x3e: // SRL (XY+o)
		WM8(EA, SRL(RM8(EA)));
		break;
	case 0x3f: // SRL A=(XY+o)
		_A = SRL(RM8(EA));
		WM8(EA, _A);
		break;
	case 0x40: // BIT 0, B=(XY+o)
	case 0x41: // BIT 0, C=(XY+o)
	case 0x42: // BIT 0, D=(XY+o)
	case 0x43: // BIT 0, E=(XY+o)
	case 0x44: // BIT 0, H=(XY+o)
	case 0x45: // BIT 0, L=(XY+o)
	case 0x46: // BIT 0, (XY+o)
	case 0x47: // BIT 0, A=(XY+o)
		BIT_XY(0, RM8(EA));
		break;
	case 0x48: // BIT 1, B=(XY+o)
	case 0x49: // BIT 1, C=(XY+o)
	case 0x4a: // BIT 1, D=(XY+o)
	case 0x4b: // BIT 1, E=(XY+o)
	case 0x4c: // BIT 1, H=(XY+o)
	case 0x4d: // BIT 1, L=(XY+o)
	case 0x4e: // BIT 1, (XY+o)
	case 0x4f: // BIT 1, A=(XY+o)
		BIT_XY(1, RM8(EA));
		break;
	case 0x50: // BIT 2, B=(XY+o)
	case 0x51: // BIT 2, C=(XY+o)
	case 0x52: // BIT 2, D=(XY+o)
	case 0x53: // BIT 2, E=(XY+o)
	case 0x54: // BIT 2, H=(XY+o)
	case 0x55: // BIT 2, L=(XY+o)
	case 0x56: // BIT 2, (XY+o)
	case 0x57: // BIT 2, A=(XY+o)
		BIT_XY(2, RM8(EA));
		break;
	case 0x58: // BIT 3, B=(XY+o)
	case 0x59: // BIT 3, C=(XY+o)
	case 0x5a: // BIT 3, D=(XY+o)
	case 0x5b: // BIT 3, E=(XY+o)
	case 0x5c: // BIT 3, H=(XY+o)
	case 0x5d: // BIT 3, L=(XY+o)
	case 0x5e: // BIT 3, (XY+o)
	case 0x5f: // BIT 3, A=(XY+o)
		BIT_XY(3, RM8(EA));
		break;
	case 0x60: // BIT 4, B=(XY+o)
	case 0x61: // BIT 4, C=(XY+o)
	case 0x62: // BIT 4, D=(XY+o)
	case 0x63: // BIT 4, E=(XY+o)
	case 0x64: // BIT 4, H=(XY+o)
	case 0x65: // BIT 4, L=(XY+o)
	case 0x66: // BIT 4, (XY+o)
	case 0x67: // BIT 4, A=(XY+o)
		BIT_XY(4, RM8(EA));
		break;
	case 0x68: // BIT 5, B=(XY+o)
	case 0x69: // BIT 5, C=(XY+o)
	case 0x6a: // BIT 5, D=(XY+o)
	case 0x6b: // BIT 5, E=(XY+o)
	case 0x6c: // BIT 5, H=(XY+o)
	case 0x6d: // BIT 5, L=(XY+o)
	case 0x6e: // BIT 5, (XY+o)
	case 0x6f: // BIT 5, A=(XY+o)
		BIT_XY(5, RM8(EA));
		break;
	case 0x70: // BIT 6, B=(XY+o)
	case 0x71: // BIT 6, C=(XY+o)
	case 0x72: // BIT 6, D=(XY+o)
	case 0x73: // BIT 6, E=(XY+o)
	case 0x74: // BIT 6, H=(XY+o)
	case 0x75: // BIT 6, L=(XY+o)
	case 0x76: // BIT 6, (XY+o)
	case 0x77: // BIT 6, A=(XY+o)
		BIT_XY(6, RM8(EA));
		break;
	case 0x78: // BIT 7, B=(XY+o)
	case 0x79: // BIT 7, C=(XY+o)
	case 0x7a: // BIT 7, D=(XY+o)
	case 0x7b: // BIT 7, E=(XY+o)
	case 0x7c: // BIT 7, H=(XY+o)
	case 0x7d: // BIT 7, L=(XY+o)
	case 0x7e: // BIT 7, (XY+o)
	case 0x7f: // BIT 7, A=(XY+o)
		BIT_XY(7, RM8(EA));
		break;
	case 0x80: // RES 0, B=(XY+o)
		_B = RES(0, RM8(EA));
		WM8(EA, _B);
		break;
	case 0x81: // RES 0, C=(XY+o)
		_C = RES(0, RM8(EA));
		WM8(EA, _C);
		break;
	case 0x82: // RES 0, D=(XY+o)
		_D = RES(0, RM8(EA));
		WM8(EA, _D);
		break;
	case 0x83: // RES 0, E=(XY+o)
		_E = RES(0, RM8(EA));
		WM8(EA, _E);
		break;
	case 0x84: // RES 0, H=(XY+o)
		_H = RES(0, RM8(EA));
		WM8(EA, _H);
		break;
	case 0x85: // RES 0, L=(XY+o)
		_L = RES(0, RM8(EA));
		WM8(EA, _L);
		break;
	case 0x86: // RES 0, (XY+o)
		WM8(EA, RES(0, RM8(EA)));
		break;
	case 0x87: // RES 0, A=(XY+o)
		_A = RES(0, RM8(EA));
		WM8(EA, _A);
		break;
	case 0x88: // RES 1, B=(XY+o)
		_B = RES(1, RM8(EA));
		WM8(EA, _B);
		break;
	case 0x89: // RES 1, C=(XY+o)
		_C = RES(1, RM8(EA));
		WM8(EA, _C);
		break;
	case 0x8a: // RES 1, D=(XY+o)
		_D = RES(1, RM8(EA));
		WM8(EA, _D);
		break;
	case 0x8b: // RES 1, E=(XY+o)
		_E = RES(1, RM8(EA));
		WM8(EA, _E);
		break;
	case 0x8c: // RES 1, H=(XY+o)
		_H = RES(1, RM8(EA));
		WM8(EA, _H);
		break;
	case 0x8d: // RES 1, L=(XY+o)
		_L = RES(1, RM8(EA));
		WM8(EA, _L);
		break;
	case 0x8e: // RES 1, (XY+o)
		WM8(EA, RES(1, RM8(EA)));
		break;
	case 0x8f: // RES 1, A=(XY+o)
		_A = RES(1, RM8(EA));
		WM8(EA, _A);
		break;
	case 0x90: // RES 2, B=(XY+o)
		_B = RES(2, RM8(EA));
		WM8(EA, _B);
		break;
	case 0x91: // RES 2, C=(XY+o)
		_C = RES(2, RM8(EA));
		WM8(EA, _C);
		break;
	case 0x92: // RES 2, D=(XY+o)
		_D = RES(2, RM8(EA));
		WM8(EA, _D);
		break;
	case 0x93: // RES 2, E=(XY+o)
		_E = RES(2, RM8(EA));
		WM8(EA, _E);
		break;
	case 0x94: // RES 2, H=(XY+o)
		_H = RES(2, RM8(EA));
		WM8(EA, _H);
		break;
	case 0x95: // RES 2, L=(XY+o)
		_L = RES(2, RM8(EA));
		WM8(EA, _L);
		break;
	case 0x96: // RES 2, (XY+o)
		WM8(EA, RES(2, RM8(EA)));
		break;
	case 0x97: // RES 2, A=(XY+o)
		_A = RES(2, RM8(EA));
		WM8(EA, _A);
		break;
	case 0x98: // RES 3, B=(XY+o)
		_B = RES(3, RM8(EA));
		WM8(EA, _B);
		break;
	case 0x99: // RES 3, C=(XY+o)
		_C = RES(3, RM8(EA));
		WM8(EA, _C);
		break;
	case 0x9a: // RES 3, D=(XY+o)
		_D = RES(3, RM8(EA));
		WM8(EA, _D);
		break;
	case 0x9b: // RES 3, E=(XY+o)
		_E = RES(3, RM8(EA));
		WM8(EA, _E);
		break;
	case 0x9c: // RES 3, H=(XY+o)
		_H = RES(3, RM8(EA));
		WM8(EA, _H);
		break;
	case 0x9d: // RES 3, L=(XY+o)
		_L = RES(3, RM8(EA));
		WM8(EA, _L);
		break;
	case 0x9e: // RES 3, (XY+o)
		WM8(EA, RES(3, RM8(EA)));
		break;
	case 0x9f: // RES 3, A=(XY+o)
		_A = RES(3, RM8(EA));
		WM8(EA, _A);
		break;
	case 0xa0: // RES 4, B=(XY+o)
		_B = RES(4, RM8(EA));
		WM8(EA, _B);
		break;
	case 0xa1: // RES 4, C=(XY+o)
		_C = RES(4, RM8(EA));
		WM8(EA, _C);
		break;
	case 0xa2: // RES 4, D=(XY+o)
		_D = RES(4, RM8(EA));
		WM8(EA, _D);
		break;
	case 0xa3: // RES 4, E=(XY+o)
		_E = RES(4, RM8(EA));
		WM8(EA, _E);
		break;
	case 0xa4: // RES 4, H=(XY+o)
		_H = RES(4, RM8(EA));
		WM8(EA, _H);
		break;
	case 0xa5: // RES 4, L=(XY+o)
		_L = RES(4, RM8(EA));
		WM8(EA, _L);
		break;
	case 0xa6: // RES 4, (XY+o)
		WM8(EA, RES(4, RM8(EA)));
		break;
	case 0xa7: // RES 4, A=(XY+o)
		_A = RES(4, RM8(EA));
		WM8(EA, _A);
		break;
	case 0xa8: // RES 5, B=(XY+o)
		_B = RES(5, RM8(EA));
		WM8(EA, _B);
		break;
	case 0xa9: // RES 5, C=(XY+o)
		_C = RES(5, RM8(EA));
		WM8(EA, _C);
		break;
	case 0xaa: // RES 5, D=(XY+o)
		_D = RES(5, RM8(EA));
		WM8(EA, _D);
		break;
	case 0xab: // RES 5, E=(XY+o)
		_E = RES(5, RM8(EA));
		WM8(EA, _E);
		break;
	case 0xac: // RES 5, H=(XY+o)
		_H = RES(5, RM8(EA));
		WM8(EA, _H);
		break;
	case 0xad: // RES 5, L=(XY+o)
		_L = RES(5, RM8(EA));
		WM8(EA, _L);
		break;
	case 0xae: // RES 5, (XY+o)
		WM8(EA, RES(5, RM8(EA)));
		break;
	case 0xaf: // RES 5, A=(XY+o)
		_A = RES(5, RM8(EA));
		WM8(EA, _A);
		break;
	case 0xb0: // RES 6, B=(XY+o)
		_B = RES(6, RM8(EA));
		WM8(EA, _B);
		break;
	case 0xb1: // RES 6, C=(XY+o)
		_C = RES(6, RM8(EA));
		WM8(EA, _C);
		break;
	case 0xb2: // RES 6, D=(XY+o)
		_D = RES(6, RM8(EA));
		WM8(EA, _D);
		break;
	case 0xb3: // RES 6, E=(XY+o)
		_E = RES(6, RM8(EA));
		WM8(EA, _E);
		break;
	case 0xb4: // RES 6, H=(XY+o)
		_H = RES(6, RM8(EA));
		WM8(EA, _H);
		break;
	case 0xb5: // RES 6, L=(XY+o)
		_L = RES(6, RM8(EA));
		WM8(EA, _L);
		break;
	case 0xb6: // RES 6, (XY+o)
		WM8(EA, RES(6, RM8(EA)));
		break;
	case 0xb7: // RES 6, A=(XY+o)
		_A = RES(6, RM8(EA));
		WM8(EA, _A);
		break;
	case 0xb8: // RES 7, B=(XY+o)
		_B = RES(7, RM8(EA));
		WM8(EA, _B);
		break;
	case 0xb9: // RES 7, C=(XY+o)
		_C = RES(7, RM8(EA));
		WM8(EA, _C);
		break;
	case 0xba: // RES 7, D=(XY+o)
		_D = RES(7, RM8(EA));
		WM8(EA, _D);
		break;
	case 0xbb: // RES 7, E=(XY+o)
		_E = RES(7, RM8(EA));
		WM8(EA, _E);
		break;
	case 0xbc: // RES 7, H=(XY+o)
		_H = RES(7, RM8(EA));
		WM8(EA, _H);
		break;
	case 0xbd: // RES 7, L=(XY+o)
		_L = RES(7, RM8(EA));
		WM8(EA, _L);
		break;
	case 0xbe: // RES 7, (XY+o)
		WM8(EA, RES(7, RM8(EA)));
		break;
	case 0xbf: // RES 7, A=(XY+o)
		_A = RES(7, RM8(EA));
		WM8(EA, _A);
		break;
	case 0xc0: // SET 0, B=(XY+o)
		_B = SET(0, RM8(EA));
		WM8(EA, _B);
		break;
	case 0xc1: // SET 0, C=(XY+o)
		_C = SET(0, RM8(EA));
		WM8(EA, _C);
		break;
	case 0xc2: // SET 0, D=(XY+o)
		_D = SET(0, RM8(EA));
		WM8(EA, _D);
		break;
	case 0xc3: // SET 0, E=(XY+o)
		_E = SET(0, RM8(EA));
		WM8(EA, _E);
		break;
	case 0xc4: // SET 0, H=(XY+o)
		_H = SET(0, RM8(EA));
		WM8(EA, _H);
		break;
	case 0xc5: // SET 0, L=(XY+o)
		_L = SET(0, RM8(EA));
		WM8(EA, _L);
		break;
	case 0xc6: // SET 0, (XY+o)
		WM8(EA, SET(0, RM8(EA)));
		break;
	case 0xc7: // SET 0, A=(XY+o)
		_A = SET(0, RM8(EA));
		WM8(EA, _A);
		break;
	case 0xc8: // SET 1, B=(XY+o)
		_B = SET(1, RM8(EA));
		WM8(EA, _B);
		break;
	case 0xc9: // SET 1, C=(XY+o)
		_C = SET(1, RM8(EA));
		WM8(EA, _C);
		break;
	case 0xca: // SET 1, D=(XY+o)
		_D = SET(1, RM8(EA));
		WM8(EA, _D);
		break;
	case 0xcb: // SET 1, E=(XY+o)
		_E = SET(1, RM8(EA));
		WM8(EA, _E);
		break;
	case 0xcc: // SET 1, H=(XY+o)
		_H = SET(1, RM8(EA));
		WM8(EA, _H);
		break;
	case 0xcd: // SET 1, L=(XY+o)
		_L = SET(1, RM8(EA));
		WM8(EA, _L);
		break;
	case 0xce: // SET 1, (XY+o)
		WM8(EA, SET(1, RM8(EA)));
		break;
	case 0xcf: // SET 1, A=(XY+o)
		_A = SET(1, RM8(EA));
		WM8(EA, _A);
		break;
	case 0xd0: // SET 2, B=(XY+o)
		_B = SET(2, RM8(EA));
		WM8(EA, _B);
		break;
	case 0xd1: // SET 2, C=(XY+o)
		_C = SET(2, RM8(EA));
		WM8(EA, _C);
		break;
	case 0xd2: // SET 2, D=(XY+o)
		_D = SET(2, RM8(EA));
		WM8(EA, _D);
		break;
	case 0xd3: // SET 2, E=(XY+o)
		_E = SET(2, RM8(EA));
		WM8(EA, _E);
		break;
	case 0xd4: // SET 2, H=(XY+o)
		_H = SET(2, RM8(EA));
		WM8(EA, _H);
		break;
	case 0xd5: // SET 2, L=(XY+o)
		_L = SET(2, RM8(EA));
		WM8(EA, _L);
		break;
	case 0xd6: // SET 2, (XY+o)
		WM8(EA, SET(2, RM8(EA)));
		break;
	case 0xd7: // SET 2, A=(XY+o)
		_A = SET(2, RM8(EA));
		WM8(EA, _A);
		break;
	case 0xd8: // SET 3, B=(XY+o)
		_B = SET(3, RM8(EA));
		WM8(EA, _B);
		break;
	case 0xd9: // SET 3, C=(XY+o)
		_C = SET(3, RM8(EA));
		WM8(EA, _C);
		break;
	case 0xda: // SET 3, D=(XY+o)
		_D = SET(3, RM8(EA));
		WM8(EA, _D);
		break;
	case 0xdb: // SET 3, E=(XY+o)
		_E = SET(3, RM8(EA));
		WM8(EA, _E);
		break;
	case 0xdc: // SET 3, H=(XY+o)
		_H = SET(3, RM8(EA));
		WM8(EA, _H);
		break;
	case 0xdd: // SET 3, L=(XY+o)
		_L = SET(3, RM8(EA));
		WM8(EA, _L);
		break;
	case 0xde: // SET 3, (XY+o)
		WM8(EA, SET(3, RM8(EA)));
		break;
	case 0xdf: // SET 3, A=(XY+o)
		_A = SET(3, RM8(EA));
		WM8(EA, _A);
		break;
	case 0xe0: // SET 4, B=(XY+o)
		_B = SET(4, RM8(EA));
		WM8(EA, _B);
		break;
	case 0xe1: // SET 4, C=(XY+o)
		_C = SET(4, RM8(EA));
		WM8(EA, _C);
		break;
	case 0xe2: // SET 4, D=(XY+o)
		_D = SET(4, RM8(EA));
		WM8(EA, _D);
		break;
	case 0xe3: // SET 4, E=(XY+o)
		_E = SET(4, RM8(EA));
		WM8(EA, _E);
		break;
	case 0xe4: // SET 4, H=(XY+o)
		_H = SET(4, RM8(EA));
		WM8(EA, _H);
		break;
	case 0xe5: // SET 4, L=(XY+o)
		_L = SET(4, RM8(EA));
		WM8(EA, _L);
		break;
	case 0xe6: // SET 4, (XY+o)
		WM8(EA, SET(4, RM8(EA)));
		break;
	case 0xe7: // SET 4, A=(XY+o)
		_A = SET(4, RM8(EA));
		WM8(EA, _A);
		break;
	case 0xe8: // SET 5, B=(XY+o)
		_B = SET(5, RM8(EA));
		WM8(EA, _B);
		break;
	case 0xe9: // SET 5, C=(XY+o)
		_C = SET(5, RM8(EA));
		WM8(EA, _C);
		break;
	case 0xea: // SET 5, D=(XY+o)
		_D = SET(5, RM8(EA));
		WM8(EA, _D);
		break;
	case 0xeb: // SET 5, E=(XY+o)
		_E = SET(5, RM8(EA));
		WM8(EA, _E);
		break;
	case 0xec: // SET 5, H=(XY+o)
		_H = SET(5, RM8(EA));
		WM8(EA, _H);
		break;
	case 0xed: // SET 5, L=(XY+o)
		_L = SET(5, RM8(EA));
		WM8(EA, _L);
		break;
	case 0xee: // SET 5, (XY+o)
		WM8(EA, SET(5, RM8(EA)));
		break;
	case 0xef: // SET 5, A=(XY+o)
		_A = SET(5, RM8(EA));
		WM8(EA, _A);
		break;
	case 0xf0: // SET 6, B=(XY+o)
		_B = SET(6, RM8(EA));
		WM8(EA, _B);
		break;
	case 0xf1: // SET 6, C=(XY+o)
		_C = SET(6, RM8(EA));
		WM8(EA, _C);
		break;
	case 0xf2: // SET 6, D=(XY+o)
		_D = SET(6, RM8(EA));
		WM8(EA, _D);
		break;
	case 0xf3: // SET 6, E=(XY+o)
		_E = SET(6, RM8(EA));
		WM8(EA, _E);
		break;
	case 0xf4: // SET 6, H=(XY+o)
		_H = SET(6, RM8(EA));
		WM8(EA, _H);
		break;
	case 0xf5: // SET 6, L=(XY+o)
		_L = SET(6, RM8(EA));
		WM8(EA, _L);
		break;
	case 0xf6: // SET 6, (XY+o)
		WM8(EA, SET(6, RM8(EA)));
		break;
	case 0xf7: // SET 6, A=(XY+o)
		_A = SET(6, RM8(EA));
		WM8(EA, _A);
		break;
	case 0xf8: // SET 7, B=(XY+o)
		_B = SET(7, RM8(EA));
		WM8(EA, _B);
		break;
	case 0xf9: // SET 7, C=(XY+o)
		_C = SET(7, RM8(EA));
		WM8(EA, _C);
		break;
	case 0xfa: // SET 7, D=(XY+o)
		_D = SET(7, RM8(EA));
		WM8(EA, _D);
		break;
	case 0xfb: // SET 7, E=(XY+o)
		_E = SET(7, RM8(EA));
		WM8(EA, _E);
		break;
	case 0xfc: // SET 7, H=(XY+o)
		_H = SET(7, RM8(EA));
		WM8(EA, _H);
		break;
	case 0xfd: // SET 7, L=(XY+o)
		_L = SET(7, RM8(EA));
		WM8(EA, _L);
		break;
	case 0xfe: // SET 7, (XY+o)
		WM8(EA, SET(7, RM8(EA)));
		break;
	case 0xff: // SET 7, A=(XY+o)
		_A = SET(7, RM8(EA));
		WM8(EA, _A);
		break;
	}
}

