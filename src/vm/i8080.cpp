/*
	Skelton for retropc emulator

	Origin : MAME
	Author : Takeda.Toshiya
	Date   : 2008.11.04 -

	[ i8080 / i8085 ]
*/

#include "i8080.h"

#define AF	regs[0].w
#define BC	regs[1].w
#define DE 	regs[2].w
#define HL	regs[3].w

#define _F	regs[0].b[0]
#define _A	regs[0].b[1]
#define _C	regs[1].b[0]
#define _B	regs[1].b[1]
#define _E	regs[2].b[0]
#define _D	regs[2].b[1]
#define _L	regs[3].b[0]
#define _H	regs[3].b[1]

#define CF	0x01
#define NF	0x02
#define VF	0x04
#define XF	0x08
#define HF	0x10
#define YF	0x20
#define ZF	0x40
#define SF	0x80

#define IM_M5	0x01
#define IM_M6	0x02
#define IM_M7	0x04
#define IM_IEN	0x08
#define IM_I5	0x10
#define IM_I6	0x20
#define IM_I7	0x40
#define IM_SID	0x80
// special
#define IM_INT	0x100
#define IM_NMI	0x200
//#define IM_REQ	(IM_I5 | IM_I6 | IM_I7 | IM_INT | IM_NMI)
#define IM_REQ	0x370

#ifndef CPU_START_ADDR
#define CPU_START_ADDR	0
#endif

// opecode definitions

#define INR(v) { \
	uint8 hc = ((v & 0x0f) == 0x0f) ? HF : 0; \
	++v; \
	_F = (_F & CF) | ZSP[v] | hc; \
}
#define DCR(v) { \
	uint8 hc = ((v & 0x0f) == 0x00) ? HF : 0; \
	--v; \
	_F = (_F & CF) | ZSP[v] | hc | NF; \
}
#define MVI(v) { \
	v = FETCH8(); \
}
#ifdef HAS_I8085
#define ANA(v) { \
	_A &= v; \
	_F = ZSP[_A]; \
	_F |= HF; \
}
#else
#define ANA(v) { \
	int i = (((_A | v) >> 3) & 1) * HF; \
	_A &= v; \
	_F = ZSP[_A]; \
	_F |= i; \
}
#endif
#define ORA(v) { \
	_A |= v; \
	_F = ZSP[_A]; \
}
#define XRA(v) { \
	_A ^= v; \
	_F = ZSP[_A]; \
}
#define RLC() { \
	_A = (_A << 1) | (_A >> 7); \
	_F = (_F & 0xfe) | (_A & CF); \
}
#define RRC() { \
	_F = (_F & 0xfe) | (_A & CF); \
	_A = (_A >> 1) | (_A << 7); \
}
#define RAL() { \
	int c = _F & CF; \
	_F = (_F & 0xfe) | (_A >> 7); \
	_A = (_A << 1) | c; \
}
#define RAR() { \
	int c = (_F&CF) << 7; \
	_F = (_F & 0xfe) | (_A & CF); \
	_A = (_A >> 1) | c; \
}
#define ADD(v) { \
	int q = _A + v; \
	_F = ZSP[q & 255] | ((q >> 8) & CF) | ((_A ^ q ^ v) & HF) | (((v ^ _A ^ SF) & (v ^ q) & SF) >> 5); \
	_A = q; \
}
#define ADC(v) {\
	int q = _A + v + (_F & CF); \
	_F = ZSP[q & 255] | ((q >> 8) & CF) | ((_A ^ q ^ v) & HF) | (((v ^ _A ^ SF) & (v ^ q) & SF) >> 5); \
	_A = q; \
}
#define SUB(v) { \
	int q = _A - v; \
	_F = ZSP[q & 255] | ((q >> 8) & CF) | NF | ((_A ^ q ^ v) & HF) | (((v ^ _A) & (_A ^ q) & SF) >> 5); \
	_A = q; \
}
#define SBB(v) { \
	int q = _A - v - (_F & CF); \
	_F = ZSP[q & 255] | ((q >> 8) & CF) | NF | ((_A ^ q ^ v) & HF) | (((v ^ _A) & (_A ^ q) & SF) >> 5); \
	_A = q; \
}
#define CMP(v) { \
	int q = _A - v; \
	_F = ZSP[q & 255] | ((q >> 8) & CF) | NF | ((_A ^ q ^ v) & HF) | (((v ^ _A) & (_A ^ q) & SF) >> 5); \
}
#define DAD(v) { \
	int q = HL + v; \
	_F = (_F & ~(HF + CF)) | (((HL ^ q ^ v) >> 8) & HF) | ((q >> 16) & CF); \
	HL = q; \
}
#define RET(c) { \
	if(c) { \
		count -= 6; \
		PC = POP16(); \
	}	\
}
#ifdef HAS_I8085
#define JMP(c) { \
	if(c) { \
		PC = FETCH16(); \
	} \
	else { \
		PC += 2; \
		count += 3; \
	} \
}
#define CALL(c) { \
	if(c) { \
		uint16 a = FETCH16(); \
		count -= 7; \
		PUSH16(PC); \
		PC = a; \
	} \
	else { \
		PC += 2; \
		count += 2; \
	} \
}
#else
#define JMP(c) { \
	if(c) { \
		PC = FETCH16(); \
	} \
	else { \
		PC += 2; \
	} \
}
#define CALL(c) { \
	if(c) { \
		uint16 a = FETCH16(); \
		count -= 6; \
		PUSH16(PC); \
		PC = a; \
	} \
	else { \
		PC += 2; \
	} \
}
#endif
#define RST(n) { \
	PUSH16(PC); \
	PC = 8 * n; \
}
#define DSUB() {\
	int q = _L - _C; \
	_F = ZS[q & 255] | ((q >> 8) & CF) | NF | ((_L ^ q ^ _C) & HF) | (((_C ^ _L) & (_L ^ q) & SF) >> 5); \
	_L = q; \
	q = _H - _B - (_F & CF); \
	_F = ZS[q & 255] | ((q >> 8) & CF) | NF | ((_H ^ q ^ _B) & HF) | (((_B ^ _H) & (_H ^ q) & SF) >> 5); \
	if(_L != 0) \
		_F &= ~ZF; \
}
#define INT(v) { \
	if(HALT) { \
		PC++; HALT = 0; \
	} \
	PUSH16(PC); PC = (v); \
}

// main

void I8080::reset()
{
	// reset
	PC = CPU_START_ADDR;
	IM = IM_M5 | IM_M6 | IM_M7;
	HALT = BUSREQ = false;
}

void I8080::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_CPU_NMI) {
		if(data & mask)
			IM |= IM_NMI;
		else
			IM &= ~IM_NMI;
	}
	else if(id == SIG_CPU_BUSREQ) {
		BUSREQ = ((data & mask) != 0);
		if(BUSREQ)
			count = first = 0;
		// busack
		for(int i = 0; i < dcount_busack; i++)
			d_busack[i]->write_signal(did_busack[i], BUSREQ ? 0xffffffff : 0, dmask_busack[i]);
	}
	else if(id == SIG_I8080_INTR) {
		if(data & mask)
			IM |= IM_INT;
		else
			IM &= ~IM_INT;
	}
#ifdef HAS_I8085
	else if(id == SIG_I8085_RST5) {
		if(data & mask)
			IM |= IM_I5;
		else
			IM &= ~IM_I5;
	}
	else if(id == SIG_I8085_RST6) {
		if(data & mask)
			IM |= IM_I6;
		else
			IM &= ~IM_I6;
	}
	else if(id == SIG_I8085_RST7) {
		if(data & mask)
			IM |= IM_I7;
		else
			IM &= ~IM_I7;
	}
	else if(id == SIG_I8085_SID)
		SID = ((data & mask) != 0);
#endif
}

void I8080::set_intr_line(bool line, bool pending, uint32 bit)
{
	if(line)
		IM |= IM_INT;
	else
		IM &= ~IM_INT;
}

void I8080::run(int clock)
{
	// return now if BUSREQ
	if(BUSREQ) {
		count = first = 0;
		return;
	}
	
	// run cpu while given clocks
	count += clock;
	first = count;
	while(count > 0) {
		OP(FETCHOP());
		if(IM & IM_REQ) {
			if(IM & IM_NMI) {
				INT(0x24);
				count -= 5;	// unknown
				RIM_IEN = IM & IM_IEN;
				IM &= ~(IM_IEN | IM_NMI);
			}
			else if(IM & IM_IEN) {
#ifdef HAS_I8085
				if(!(IM & IM_M7) && (IM & IM_I7)) {
					INT(0x3c);
					count -= 7;	// unknown
					RIM_IEN = 0;
					IM &= ~(IM_IEN | IM_I7);
				}
				else if(!(IM & IM_M6) && (IM & IM_I6)) {
					INT(0x34);
					count -= 7;	// unknown
					RIM_IEN = 0;
					IM &= ~(IM_IEN | IM_I6);
				}
				else if(!(IM & IM_M5) && (IM & IM_I5)) {
					INT(0x2c);
					count -= 7;	// unknown
					RIM_IEN = 0;
					IM &= ~(IM_IEN | IM_I5);
				}
				else
#endif
				if(IM & IM_INT) {
					uint32 vector = ACK_INTR();
					uint8 v0 = vector;
					uint16 v12 = vector >> 8;
					// support JMP/CALL/RST only
					count -= cc_op[v0];
					switch(v0)
					{
					case 0xc3:	// JMP
						PC = v12;
						break;
					case 0xcd:	// CALL
						PUSH16(PC);
						PC = v12;
#ifdef HAS_I8085
						count -= 7;
#else
						count -= 6;
#endif
						break;
					case 0xc7:	// RST 0
						RST(0);
						break;
					case 0xcf:	// RST 1
						RST(1);
						break;
					case 0xd7:	// RST 2
						RST(2);
						break;
					case 0xdf:	// RST 3
						RST(3);
						break;
					case 0xe7:	// RST 4
						RST(4);
						break;
					case 0xef:	// RST 5
						RST(5);
						break;
					case 0xf7:	// RST 6
						RST(6);
						break;
					case 0xff:	// RST 7
						RST(7);
						break;
					}
					RIM_IEN = 0;
					IM &= ~(IM_IEN | IM_INT);
				}
			}
		}
	}
	first = count;
}

void I8080::OP(uint8 code)
{
	uint8 tmp8;
	uint16 tmp16;
	
	prvPC = PC - 1;
	count -= cc_op[code];
	
	switch(code)
	{
	case 0x00: // NOP
		break;
	case 0x01: // LXI B,nnnn
		BC = FETCH16();
		break;
	case 0x02: // STAX B
		WM8(BC, _A);
		break;
	case 0x03: // INX B
		BC++;
#ifdef HAS_I8085
		if(_C == 0x00) _F |= XF; else _F &= ~XF;
#endif
		break;
	case 0x04: // INR B
		INR(_B);
		break;
	case 0x05: // DCR B
		DCR(_B);
		break;
	case 0x06: // MVI B,nn
		MVI(_B);
		break;
	case 0x07: // RLC
		RLC();
		break;
	case 0x08: // DSUB (NOP)
#ifdef HAS_I8085
		DSUB();
#endif
		break;
	case 0x09: // DAD B
		DAD(BC);
		break;
	case 0x0a: // LDAX B
		_A = RM8(BC);
		break;
	case 0x0b: // DCX B
		BC--;
#ifdef HAS_I8085
		if(_C == 0xff) _F |= XF; else _F &= ~XF;
#endif
		break;
	case 0x0c: // INR C
		INR(_C);
		break;
	case 0x0d: // DCR C
		DCR(_C);
		break;
	case 0x0e: // MVI C,nn
		MVI(_C);
		break;
	case 0x0f: // RRC
		RRC();
		break;
	case 0x10: // ASRH (NOP)
#ifdef HAS_I8085
		_F = (_F & ~CF) | (_L & CF);
		HL = (HL >> 1);
#endif
		break;
	case 0x11: // LXI D,nnnn
		DE = FETCH16();
		break;
	case 0x12: // STAX D
		WM8(DE, _A);
		break;
	case 0x13: // INX D
		DE++;
#ifdef HAS_I8085
		if(_E == 0x00) _F |= XF; else _F &= ~XF;
#endif
		break;
	case 0x14: // INR D
		INR(_D);
		break;
	case 0x15: // DCR D
		DCR(_D);
		break;
	case 0x16: // MVI D,nn
		MVI(_D);
		break;
	case 0x17: // RAL
		RAL();
		break;
	case 0x18: // RLDE (NOP)
#ifdef HAS_I8085
		_F = (_F & ~(CF | VF)) | (_D >> 7);
		DE = (DE << 1) | (DE >> 15);
		if(0 != (((DE >> 15) ^ _F) & CF))
			_F |= VF;
#endif
		break;
	case 0x19: // DAD D
		DAD(DE);
		break;
	case 0x1a: // LDAX D
		_A = RM8(DE);
		break;
	case 0x1b: // DCX D
		DE--;
#ifdef HAS_I8085
		if(_E == 0xff) _F |= XF; else _F &= ~XF;
#endif
		break;
	case 0x1c: // INR E
		INR(_E);
		break;
	case 0x1d: // DCR E
		DCR(_E);
		break;
	case 0x1e: // MVI E,nn
		MVI(_E);
		break;
	case 0x1f: // RAR
		RAR();
		break;
	case 0x20: // RIM (NOP)
#ifdef HAS_I8085
		_A = (IM & 0x7f) | (SID ? 0x80 : 0) | RIM_IEN;
		RIM_IEN = 0;
#endif
		break;
	case 0x21: // LXI H,nnnn
		HL = FETCH16();
		break;
	case 0x22: // SHLD nnnn
		WM16(FETCH16(), HL);
		break;
	case 0x23: // INX H
		HL++;
#ifdef HAS_I8085
		if(_L == 0x00) _F |= XF; else _F &= ~XF;
#endif
		break;
	case 0x24: // INR H
		INR(_H);
		break;
	case 0x25: // DCR H
		DCR(_H);
		break;
	case 0x26: // MVI H,nn
		MVI(_H);
		break;
	case 0x27: // DAA
		tmp16 = _A;
		if(_F & CF) tmp16 |= 0x100;
		if(_F & HF) tmp16 |= 0x200;
		if(_F & NF) tmp16 |= 0x400;
		AF = DAA[tmp16];
#ifdef HAS_I8080
		_F &= 0xd5;
#endif
		break;
	case 0x28: // LDEH nn (NOP)
#ifdef HAS_I8085
		DE = (HL + FETCH8()) & 0xffff;
#endif
		break;
	case 0x29: // DAD H
		DAD(HL);
		break;
	case 0x2a: // LHLD nnnn
		HL = RM16(FETCH16());
		break;
	case 0x2b: // DCX H
		HL--;
#ifdef HAS_I8085
		if(_L == 0xff) _F |= XF; else _F &= ~XF;
#endif
		break;
	case 0x2c: // INR L
		INR(_L);
		break;
	case 0x2d: // DCR L
		DCR(_L);
		break;
	case 0x2e: // MVI L,nn
		MVI(_L);
		break;
	case 0x2f: // CMA
#ifdef HAS_I8085
		_A ^= 0xff;
		_F |= HF + NF;
#else
		_A ^= 0xff;
#endif
		break;
	case 0x30: // SIM (NOP)
#ifdef HAS_I8085
		if(_A & 0x40) {
			for(int i = 0; i < dcount_sod; i++)
				d_sod[i]->write_signal(did_sod[i], (_A & 0x80) ? 0 : 0xffffffff, dmask_sod[i]);
		}
		if(_A & 0x10)
			IM &= ~IM_I7;
		if(_A & 8)
			IM = (IM & ~(IM_M5 | IM_M6 | IM_M7)) | (_A & (IM_M5 | IM_M6 | IM_M7));
#endif
		break;
	case 0x31: // LXI SP,nnnn
		SP = FETCH16();
		break;
	case 0x32: // STAX nnnn
		WM8(FETCH16(), _A);
		break;
	case 0x33: // INX SP
		SP++;
#ifdef HAS_I8085
		if((SP & 0xff) == 0) _F |= XF; else _F &= ~XF;
#endif
		break;
	case 0x34: // INR M
		tmp8 = RM8(HL);
		INR(tmp8);
		WM8(HL, tmp8);
		break;
	case 0x35: // DCR M
		tmp8 = RM8(HL);
		DCR(tmp8);
		WM8(HL, tmp8);
		break;
	case 0x36: // MVI M,nn
		WM8(HL, FETCH8());
		break;
	case 0x37: // STC
		_F = (_F & 0xfe) | CF;
		break;
	case 0x38: // LDES nn (NOP)
#ifdef HAS_I8085
		DE = (SP + FETCH8()) & 0xffff;
#endif
		break;
	case 0x39: // DAD SP
		DAD(SP);
		break;
	case 0x3a: // LDAX nnnn
		_A = RM8(FETCH16());
		break;
	case 0x3b: // DCX SP
		SP--;
#ifdef HAS_I8085
		if((SP & 0xff) == 0xff) _F |= XF; else _F &= ~XF;
#endif
		break;
	case 0x3c: // INR A
		INR(_A);
		break;
	case 0x3d: // DCR A
		DCR(_A);
		break;
	case 0x3e: // MVI A,nn
		MVI(_A);
		break;
	case 0x3f: // CMC
		_F = (_F & 0xfe) | ((_F & CF)==1 ? 0 : 1);
		break;
	case 0x40: // MOV B,B
		break;
	case 0x41: // MOV B,C
		_B = _C;
		break;
	case 0x42: // MOV B,D
		_B = _D;
		break;
	case 0x43: // MOV B,E
		_B = _E;
		break;
	case 0x44: // MOV B,H
		_B = _H;
		break;
	case 0x45: // MOV B,L
		_B = _L;
		break;
	case 0x46: // MOV B,M
		_B = RM8(HL);
		break;
	case 0x47: // MOV B,A
		_B = _A;
		break;
	case 0x48: // MOV C,B
		_C = _B;
		break;
	case 0x49: // MOV C,C
		break;
	case 0x4a: // MOV C,D
		_C = _D;
		break;
	case 0x4b: // MOV C,E
		_C = _E;
		break;
	case 0x4c: // MOV C,H
		_C = _H;
		break;
	case 0x4d: // MOV C,L
		_C = _L;
		break;
	case 0x4e: // MOV C,M
		_C = RM8(HL);
		break;
	case 0x4f: // MOV C,A
		_C = _A;
		break;
	case 0x50: // MOV D,B
		_D = _B;
		break;
	case 0x51: // MOV D,C
		_D = _C;
		break;
	case 0x52: // MOV D,D
		break;
	case 0x53: // MOV D,E
		_D = _E;
		break;
	case 0x54: // MOV D,H
		_D = _H;
		break;
	case 0x55: // MOV D,L
		_D = _L;
		break;
	case 0x56: // MOV D,M
		_D = RM8(HL);
		break;
	case 0x57: // MOV D,A
		_D = _A;
		break;
	case 0x58: // MOV E,B
		_E = _B;
		break;
	case 0x59: // MOV E,C
		_E = _C;
		break;
	case 0x5a: // MOV E,D
		_E = _D;
		break;
	case 0x5b: // MOV E,E
		break;
	case 0x5c: // MOV E,H
		_E = _H;
		break;
	case 0x5d: // MOV E,L
		_E = _L;
		break;
	case 0x5e: // MOV E,M
		_E = RM8(HL);
		break;
	case 0x5f: // MOV E,A
		_E = _A;
		break;
	case 0x60: // MOV H,B
		_H = _B;
		break;
	case 0x61: // MOV H,C
		_H = _C;
		break;
	case 0x62: // MOV H,D
		_H = _D;
		break;
	case 0x63: // MOV H,E
		_H = _E;
		break;
	case 0x64: // MOV H,H
		break;
	case 0x65: // MOV H,L
		_H = _L;
		break;
	case 0x66: // MOV H,M
		_H = RM8(HL);
		break;
	case 0x67: // MOV H,A
		_H = _A;
		break;
	case 0x68: // MOV L,B
		_L = _B;
		break;
	case 0x69: // MOV L,C
		_L = _C;
		break;
	case 0x6a: // MOV L,D
		_L = _D;
		break;
	case 0x6b: // MOV L,E
		_L = _E;
		break;
	case 0x6c: // MOV L,H
		_L = _H;
		break;
	case 0x6d: // MOV L,L
		break;
	case 0x6e: // MOV L,M
		_L = RM8(HL);
		break;
	case 0x6f: // MOV L,A
		_L = _A;
		break;
	case 0x70: // MOV M,B
		WM8(HL, _B);
		break;
	case 0x71: // MOV M,C
		WM8(HL, _C);
		break;
	case 0x72: // MOV M,D
		WM8(HL, _D);
		break;
	case 0x73: // MOV M,E
		WM8(HL, _E);
		break;
	case 0x74: // MOV M,H
		WM8(HL, _H);
		break;
	case 0x75: // MOV M,L
		WM8(HL, _L);
		break;
	case 0x76: // HLT
		PC--;
		HALT = 1;
		break;
	case 0x77: // MOV M,A
		WM8(HL, _A);
		break;
	case 0x78: // MOV A,B
		_A = _B;
		break;
	case 0x79: // MOV A,C
		_A = _C;
		break;
	case 0x7a: // MOV A,D
		_A = _D;
		break;
	case 0x7b: // MOV A,E
		_A = _E;
		break;
	case 0x7c: // MOV A,H
		_A = _H;
		break;
	case 0x7d: // MOV A,L
		_A = _L;
		break;
	case 0x7e: // MOV A,M
		_A = RM8(HL);
		break;
	case 0x7f: // MOV A,A
		break;
	case 0x80: // ADD B
		ADD(_B);
		break;
	case 0x81: // ADD C
		ADD(_C);
		break;
	case 0x82: // ADD D
		ADD(_D);
		break;
	case 0x83: // ADD E
		ADD(_E);
		break;
	case 0x84: // ADD H
		ADD(_H);
		break;
	case 0x85: // ADD L
		ADD(_L);
		break;
	case 0x86: // ADD M
		ADD(RM8(HL));
		break;
	case 0x87: // ADD A
		ADD(_A);
		break;
	case 0x88: // ADC B
		ADC(_B);
		break;
	case 0x89: // ADC C
		ADC(_C);
		break;
	case 0x8a: // ADC D
		ADC(_D);
		break;
	case 0x8b: // ADC E
		ADC(_E);
		break;
	case 0x8c: // ADC H
		ADC(_H);
		break;
	case 0x8d: // ADC L
		ADC(_L);
		break;
	case 0x8e: // ADC M
		ADC(RM8(HL));
		break;
	case 0x8f: // ADC A
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
	case 0x96: // SUB M
		SUB(RM8(HL));
		break;
	case 0x97: // SUB A
		SUB(_A);
		break;
	case 0x98: // SBB B
		SBB(_B);
		break;
	case 0x99: // SBB C
		SBB(_C);
		break;
	case 0x9a: // SBB D
		SBB(_D);
		break;
	case 0x9b: // SBB E
		SBB(_E);
		break;
	case 0x9c: // SBB H
		SBB(_H);
		break;
	case 0x9d: // SBB L
		SBB(_L);
		break;
	case 0x9e: // SBB M
		SBB(RM8(HL));
		break;
	case 0x9f: // SBB A
		SBB(_A);
		break;
	case 0xa0: // ANA B
		ANA(_B);
		break;
	case 0xa1: // ANA C
		ANA(_C);
		break;
	case 0xa2: // ANA D
		ANA(_D);
		break;
	case 0xa3: // ANA E
		ANA(_E);
		break;
	case 0xa4: // ANA H
		ANA(_H);
		break;
	case 0xa5: // ANA L
		ANA(_L);
		break;
	case 0xa6: // ANA M
		ANA(RM8(HL));
		break;
	case 0xa7: // ANA A
		ANA(_A);
		break;
	case 0xa8: // XRA B
		XRA(_B);
		break;
	case 0xa9: // XRA C
		XRA(_C);
		break;
	case 0xaa: // XRA D
		XRA(_D);
		break;
	case 0xab: // XRA E
		XRA(_E);
		break;
	case 0xac: // XRA H
		XRA(_H);
		break;
	case 0xad: // XRA L
		XRA(_L);
		break;
	case 0xae: // XRA M
		XRA(RM8(HL));
		break;
	case 0xaf: // XRA A
		XRA(_A);
		break;
	case 0xb0: // ORA B
		ORA(_B);
		break;
	case 0xb1: // ORA C
		ORA(_C);
		break;
	case 0xb2: // ORA D
		ORA(_D);
		break;
	case 0xb3: // ORA E
		ORA(_E);
		break;
	case 0xb4: // ORA H
		ORA(_H);
		break;
	case 0xb5: // ORA L
		ORA(_L);
		break;
	case 0xb6: // ORA M
		ORA(RM8(HL));
		break;
	case 0xb7: // ORA A
		ORA(_A);
		break;
	case 0xb8: // CMP B
		CMP(_B);
		break;
	case 0xb9: // CMP C
		CMP(_C);
		break;
	case 0xba: // CMP D
		CMP(_D);
		break;
	case 0xbb: // CMP E
		CMP(_E);
		break;
	case 0xbc: // CMP H
		CMP(_H);
		break;
	case 0xbd: // CMP L
		CMP(_L);
		break;
	case 0xbe: // CMP M
		CMP(RM8(HL));
		break;
	case 0xbf: // CMP A
		CMP(_A);
		break;
	case 0xc0: // RNZ
		RET(!(_F & ZF));
		break;
	case 0xc1: // POP B
		BC = POP16();
		break;
	case 0xc2: // JNZ nnnn
		JMP(!(_F & ZF));
		break;
	case 0xc3: // JMP nnnn
		JMP(1);
		break;
	case 0xc4: // CNZ nnnn
		CALL(!(_F & ZF));
		break;
	case 0xc5: // PUSH B
		PUSH16(BC);
		break;
	case 0xc6: // ADI nn
		tmp8 = FETCH8();
		ADD(tmp8);
			break;
	case 0xc7: // RST 0
		RST(0);
		break;
	case 0xc8: // RZ
		RET(_F & ZF);
		break;
	case 0xc9: // RET
		RET(1);
		break;
	case 0xca: // JZ nnnn
		JMP(_F & ZF);
		break;
	case 0xcb: // RST 8 (JMP nnnn)
#ifdef HAS_I8085
		if(_F & VF) {
			count -= 12;
			RST(8);
		}
		else
			count -= 6;
#else
		JMP(1);
#endif
		break;
	case 0xcc: // CZ nnnn
		CALL(_F & ZF);
		break;
	case 0xcd: // CALL nnnn
		CALL(1);
		break;
	case 0xce: // ACI nn
		tmp8 = FETCH8();
		ADC(tmp8);
		break;
	case 0xcf: // RST 1
		RST(1);
		break;
	case 0xd0: // RNC
		RET(!(_F & CF));
		break;
	case 0xd1: // POP D
		DE = POP16();
		break;
	case 0xd2: // JNC nnnn
		JMP(!(_F & CF));
		break;
	case 0xd3: // OUT nn
		OUT8(FETCH8(), _A);
		break;
	case 0xd4: // CNC nnnn
		CALL(!(_F & CF));
		break;
	case 0xd5: // PUSH D
		PUSH16(DE);
		break;
	case 0xd6: // SUI nn
		tmp8 = FETCH8();
		SUB(tmp8);
		break;
	case 0xd7: // RST 2
		RST(2);
		break;
	case 0xd8: // RC
		RET(_F & CF);
		break;
	case 0xd9: // SHLX (RET)
#ifdef HAS_I8085
		WM16(DE, HL);
#else
		RET(1);
#endif
		break;
	case 0xda: // JC nnnn
		JMP(_F & CF);
		break;
	case 0xdb: // IN nn
		_A = IN8(FETCH8());
		break;
	case 0xdc: // CC nnnn
		CALL(_F & CF);
		break;
	case 0xdd: // JNX nnnn (CALL nnnn)
#ifdef HAS_I8085
		JMP(!(_F & XF));
#else
		CALL(1);
#endif
		break;
	case 0xde: // SBI nn
		tmp8 = FETCH8();
		SBB(tmp8);
		break;
	case 0xdf: // RST 3
		RST(3);
		break;
	case 0xe0: // RPO
		RET(!(_F & VF));
		break;
	case 0xe1: // POP H
		HL = POP16();
		break;
	case 0xe2: // JPO nnnn
		JMP(!(_F & VF));
		break;
	case 0xe3: // XTHL
		tmp16 = POP16();
		PUSH16(HL);
		HL = tmp16;
		break;
	case 0xe4: // CPO nnnn
		CALL(!(_F & VF));
		break;
	case 0xe5: // PUSH H
		PUSH16(HL);
		break;
	case 0xe6: // ANI nn
		tmp8 = FETCH8();
		ANA(tmp8);
		break;
	case 0xe7: // RST 4
		RST(4);
		break;
	case 0xe8: // RPE
		RET(_F & VF);
		break;
	case 0xe9: // PCHL
		PC = HL;
		break;
	case 0xea: // JPE nnnn
		JMP(_F & VF);
		break;
	case 0xeb: // XCHG
		tmp16 = DE;
		DE = HL;
		HL = tmp16;
		break;
	case 0xec: // CPE nnnn
		CALL(_F & VF);
		break;
	case 0xed: // LHLX (CALL nnnn)
#ifdef HAS_I8085
		HL = RM16(DE);
#else
		CALL(1);
#endif
		break;
	case 0xee: // XRI nn
		tmp8 = FETCH8();
		XRA(tmp8);
		break;
	case 0xef: // RST 5
		RST(5);
		break;
	case 0xf0: // RP
		RET(!(_F&SF));
		break;
	case 0xf1: // POP A
		AF = POP16();
		break;
	case 0xf2: // JP nnnn
		JMP(!(_F & SF));
		break;
	case 0xf3: // DI
		IM &= ~IM_IEN;
		break;
	case 0xf4: // CP nnnn
		CALL(!(_F & SF));
		break;
	case 0xf5: // PUSH A
		PUSH16(AF);
		break;
	case 0xf6: // ORI nn
		tmp8 = FETCH8();
		ORA(tmp8);
		break;
	case 0xf7: // RST 6
		RST(6);
		break;
	case 0xf8: // RM
		RET(_F & SF);
		break;
	case 0xf9: // SPHL
		SP = HL;
		break;
	case 0xfa: // JM nnnn
		JMP(_F & SF);
		break;
	case 0xfb: // EI
		IM |= IM_IEN;
		OP(FETCHOP());
		break;
	case 0xfc: // CM nnnn
		CALL(_F & SF);
		break;
	case 0xfd: // JX nnnn (CALL nnnn)
#ifdef HAS_I8085
		JMP(_F & XF);
#else
		CALL(1);
#endif
		break;
	case 0xfe: // CPI nn
		tmp8 = FETCH8();
		CMP(tmp8);
		break;
	case 0xff: // RST 7
		RST(7);
		break;
	}
}

