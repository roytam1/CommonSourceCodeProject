/*
	Skelton for retropc emulator

	Origin : Ootake
	Author : Takeda.Toshiya
	Date   : 2009.03.08-

	[ HuC6260 ]
*/

#include "huc6260.h"

#define INT_IRQ2	0x01
#define INT_IRQ1	0x02
#define INT_TIRQ	0x04
#define INT_NMI		0x08

#define CF	0x01
#define ZF	0x02
#define IF	0x04
#define DF	0x08
#define BF	0x10
#define TF	0x20
#define VF	0x40
#define NF	0x80

static const int cycles_table[256] =
{
	 8, 7, 3, 4, 6, 4, 6, 7, 3, 2, 2, 2, 7, 5, 7, 6,
	 2, 7, 7, 4, 6, 4, 6, 7, 2, 5, 2, 2, 7, 5, 7, 6,
	 7, 7, 3, 4, 4, 4, 6, 7, 4, 2, 2, 2, 5, 5, 7, 6,
	 2, 7, 7, 2, 4, 4, 6, 7, 2, 5, 2, 2, 5, 5, 7, 6,
	 7, 7, 3, 4, 7, 4, 6, 7, 3, 2, 2, 2, 4, 5, 7, 6,
	 2, 7, 7, 5, 2, 4, 6, 7, 2, 5, 3, 2, 2, 5, 7, 6,
	 7, 7, 2, 2, 4, 4, 6, 7, 4, 2, 2, 2, 7, 5, 7, 6,
	 2, 7, 7,17, 4, 4, 6, 7, 2, 5, 4, 2, 7, 5, 7, 6,
	 4, 7, 2, 7, 4, 4, 4, 7, 2, 2, 2, 2, 5, 5, 5, 6,
	 2, 7, 7, 8, 4, 4, 4, 7, 2, 5, 2, 2, 5, 5, 5, 6,
	 2, 7, 2, 7, 4, 4, 4, 7, 2, 2, 2, 2, 5, 5, 5, 6,
	 2, 7, 7, 8, 4, 4, 4, 7, 2, 5, 2, 2, 5, 5, 5, 6,
	 2, 7, 2,17, 4, 4, 6, 7, 2, 2, 2, 2, 5, 5, 7, 6,
	 2, 7, 7,17, 2, 4, 6, 7, 2, 5, 3, 2, 2, 5, 7, 6,
	 2, 7, 2,17, 4, 4, 6, 7, 2, 2, 2, 2, 5, 5, 7, 6,
	 2, 7, 7,17, 2, 4, 6, 7, 2, 5, 4, 2, 2, 5, 7, 6
};

// interuupt

inline void HUC6260::RefreshPrvIF()
{
	prvIF = _IF;
	prvIntStat = IntStat;
	prvIntMask = IntMask;
}

// flags

inline void HUC6260::UpdateFlagZN(uint8 val)
{
	_NF = _ZF = val;
}

inline void HUC6260::ExpandFlags(uint8 val)
{
	_CF = val & CF;
	_ZF = ~(val >> 1) & 1;
	_IF = val & IF;
	_DF = val & DF;
	_BF = val & BF;
	_TF = val & TF;
	_VF = val;
	_NF = val;
}

inline uint8 HUC6260::CompressFlags()
{
	return _CF | ((_ZF ? 0 : 1) << 1) | _IF | _DF | _BF | _TF | (_VF & VF) | (_NF & NF);
}

// opecode

inline void HUC6260::BIT(uint8 val)
{
	_ZF = _A & val;
	_VF = _NF = val;
}

inline void HUC6260::ADC(uint8 val)
{
	if(_TF) {
		uint8 M = RZP8(_X);
		if(_DF) {
			uint32 l = (M & 0x0F) + (val & 0x0F) + _CF;
			uint32 h = (M & 0xF0) + (val & 0xF0);
			if(l > 0x09) {
				h += 0x10;
				l += 0x06;
			}
			_VF = (uint8)((~(M ^ val) & (M ^ h) & NF) >> 1);
			if(h > 0x90) {
				h += 0x60;
			}
			_CF = (uint8)((h & 0x100) >> 8);
			UpdateFlagZN(M = (uint8)((l & 0x0F) + (h & 0xF0)));
			count -= 4;
		}
		else {
			uint32 sum = M + val + _CF;
			_VF = (uint8)((~(M ^ val) & (M ^ sum) & NF) >> 1);
			_CF = (uint8)((sum & 0x100) >> 8);
			UpdateFlagZN(M = (uint8)sum);
			count -= 3;
		}
		WZP8(_X, M);
	}
	else {
		if(_DF) {
			uint32 l = (_A & 0x0F) + (val & 0x0F) + _CF;
			uint32 h = (_A & 0xF0) + (val & 0xF0);
			if(l > 0x09) {
				h += 0x10;
				l += 0x06;
			}
			_VF = (uint8)((~(_A ^ val) & (_A ^ h) & NF) >> 1);
			if(h > 0x90) {
				h += 0x60;
			}
			_CF = (uint8)((h & 0x100) >> 8);
			UpdateFlagZN(_A = (uint8)((l & 0x0F) + (h & 0xF0)));
			count--;
		}
		else {
			uint32 sum = _A + val + _CF;
			_VF = (uint8)((~(_A ^ val) & (_A ^ sum) & NF) >> 1);
			_CF = (uint8)((sum & 0x100) >> 8);
			UpdateFlagZN(_A = (uint8)sum);
		}
	}
}

inline void HUC6260::SBC(uint8 val)
{
	uint32 cf = (~_CF) & CF;
	uint32 tmp = _A - val - cf;
	
	if(_DF) {
		uint32 l = (_A & 0x0F) - (val & 0x0F) - cf;
		uint32 h = (_A >> 4) - (val >> 4) - ((l & 0x10) == 0x10);
		if(l & 0x10) {
			l -= 6;
		}
		if(h & 0x10) {
			h -= 6;
		}
		_CF = (uint8)((~h & 0x10) >> 4);
		_VF = (uint8)(((_A ^ tmp) & (_A ^ val) & NF) >> 1);
		UpdateFlagZN(_A = (uint8)((l & 0x0F) | (h << 4)));
		count--;
	}
	else {
		_CF = (uint8)((~tmp & 0x100) >> 8);
		_VF = (uint8)(((_A ^ tmp) & (_A ^ val) & NF) >> 1);
		UpdateFlagZN(_A = (uint8)tmp);
	}
}

inline void HUC6260::AND(uint8 val)
{
	if(_TF) {
		uint8 M = RZP8(_X) & val;
		UpdateFlagZN(M);
		WZP8(_X, M);
		count -= 3;
	}
	else {
		_A &= val;
		UpdateFlagZN(_A);
	}
}

inline uint8 HUC6260::ASL(uint8 val)
{
	_CF = val >> 7;
	UpdateFlagZN(val = val << 1);
	return val;
}

inline uint8 HUC6260::LSR(uint8 val)
{
	_CF = val & CF;
	UpdateFlagZN(val = val >> 1);
	return val;
}

inline uint8 HUC6260::ROL(uint8 val)
{
	uint8 old = _CF;
	_CF = val >> 7;
	UpdateFlagZN(val = (val << 1) | old);
	return val;
}

inline uint8 HUC6260::ROR(uint8 val)
{
	uint8 old = _CF << 7;
	_CF = val & CF;
	UpdateFlagZN(val = (val >> 1) | old);
	return val;
}

inline void HUC6260::CMP(uint8 val)
{
	uint32 tmp = _A - val;
	UpdateFlagZN((uint8)tmp);
	_CF = (uint8)((~tmp & 0x100) >> 8);
}

inline void HUC6260::CPX(uint8 val)
{
	uint32 tmp = _X - val;
	UpdateFlagZN((uint8)tmp);
	_CF = (uint8)((~tmp & 0x100) >> 8);
}

inline void HUC6260::CPY(uint8 val)
{
	uint32 tmp = _Y - val;
	UpdateFlagZN((uint8)tmp);
	_CF = (uint8)((~tmp & 0x100) >> 8);
}

inline void HUC6260::EOR(uint8 val)
{
	if(_TF) {
		uint8 M = RZP8(_X) ^ val;
		UpdateFlagZN(M);
		WZP8(_X, M);
		count -= 3;
	}
	else {
		_A ^= val;
		UpdateFlagZN(_A);
	}
}

inline void HUC6260::ORA(uint8 val)
{
	if(_TF) {
		uint8 M = RZP8(_X) | val;
		UpdateFlagZN(M);
		WZP8(_X, M);
		count -= 3;
	}
	else {
		_A |= val;
		UpdateFlagZN(_A);
	}
}

inline void HUC6260::LDA(uint8 val)
{
	_A = _NF = _ZF = val;
}

inline void HUC6260::LDX(uint8 val)
{
	_X = _NF = _ZF = val;
}

inline void HUC6260::LDY(uint8 val)
{
	_Y = _NF = _ZF = val;
}

inline void HUC6260::TAX()
{
	_X = _A;
	UpdateFlagZN(_X);
}

inline void HUC6260::TAY()
{
	_Y = _A;
	UpdateFlagZN(_Y);
}

inline void HUC6260::TXA()
{
	_A = _X;
	UpdateFlagZN(_A);
}

inline void HUC6260::TYA()
{
	_A = _Y;
	UpdateFlagZN(_A);
}

inline void HUC6260::TSX()
{
	_X = _S;
	UpdateFlagZN(_X);
}

inline void HUC6260::BBRi(uint8 bit)
{
	uint8 addr8 = FETCH8();
	int8 rel8 = (int8)FETCH8();
	if((RZP8(addr8) & bit) == 0) {
		count--;
		count--;
		PC += (int16)rel8;
	}
}

inline void HUC6260::BBSi(uint8 bit)
{
	uint8 addr8 = FETCH8();
	int8 rel8 = (int8)FETCH8();
	if(RZP8(addr8) & bit) {
		count--;
		count--;
		PC += (int16)rel8;
	}
}

inline uint8 HUC6260::TRB(uint8 val)
{
	uint8 M = ~_A & val;
	_VF = _NF = val;
	_ZF = M;
	return M;
}

inline uint8 HUC6260::TSB(uint8 val)
{
	uint8 M = _A | val;

	_VF = _NF = val;
	_ZF = M;
	return M;
}

inline void HUC6260::TST(uint8 imm, uint8 M)
{
	_VF = _NF = M;
	_ZF = M & imm;
}

inline void HUC6260::RMBi(uint8 zp, uint8 bit)
{
	WZP8(zp, RZP8(zp) & (~bit));
}

inline void HUC6260::SMBi(uint8 zp, uint8 bit)
{
	WZP8(zp, RZP8(zp) | bit);
}

// main

void HUC6260::initialize()
{
	for(int i = 0; i < 256; i++) {
		cycles_high[i] = cycles_table[i];
		cycles_slow[i] = cycles_table[i] << 2;
	}
}

void HUC6260::reset()
{
	count = 0;
	speed_low = 1;
	cycles = cycles_slow;
	
	TransOpe = 0;
	IntStat = IntMask = 0;
	_CF = _ZF = 0;
	_IF = IF;
	RefreshPrvIF();
	_DF = _BF = _TF = _VF = _NF = 0;
	_A = _X = _Y = _S = _P = 0;
	_memset(MPR, 0, sizeof(MPR));
	
	PC = RM16(0xFFFE);
}

void HUC6260::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_CPU_NMI) {
		if(data & mask) {
			IntStat |= INT_NMI;
		}
		else {
			IntStat &= ~INT_NMI;
		}
	}
	else if(id == SIG_HUC6260_IRQ2) {
		if(data & mask) {
			IntStat |= INT_IRQ2;
		}
		else {
			IntStat &= ~INT_IRQ2;
		}
	}
	else if(id == SIG_HUC6260_IRQ1) {
		if(data & mask) {
			IntStat |= INT_IRQ1;
		}
		else {
			IntStat &= ~INT_IRQ1;
		}
	}
	else if(id == SIG_HUC6260_TIRQ) {
		if(data & mask) {
			IntStat |= INT_TIRQ;
		}
		else {
			IntStat &= ~INT_TIRQ;
		}
	}
	else if(id == SIG_HUC6260_INTMASK) {
		IntMask = (IntMask & ~mask) | (data & mask);
	}
}

uint32 HUC6260::read_signal(int id)
{
	if(id == SIG_HUC6260_INTMASK) {
		return IntMask;
	}
	if(id == SIG_HUC6260_INTSTAT) {
		return IntStat;
	}
	return 0;
}

void HUC6260::run(int clock)
{
	uint8 code, ureg8;
	
	// run cpu while given clocks
	count += clock;
	first = clock;
	
	while(count > 0) {
		switch(TransOpe) {
		case 0:
			if(!prvIF && (prvIntStat & ~prvIntMask)) {
				if((prvIntStat & INT_IRQ2) && !(prvIntMask & INT_IRQ2)) {
					IntStat &= ~INT_IRQ2;
					PUSH8(PC >> 8);
					PUSH8(PC & 0xFF);
					_BF = 0;
					PUSH8(CompressFlags());
					_IF = IF;
					RefreshPrvIF();
					_TF = _DF = 0;
					PC = RM16(0xFFF6);
					count -= 8;
				}
				else if((prvIntStat & INT_IRQ1) && !(prvIntMask & INT_IRQ1)) {
					IntStat &= ~INT_IRQ1;
					PUSH8(PC >> 8);
					PUSH8(PC & 0xFF);
					_BF = 0;
					PUSH8(CompressFlags());
					_IF = IF;
					RefreshPrvIF();
					_TF = _DF = 0;
					PC = RM16(0xFFF8);
					count -= 8;
				}
				else if((prvIntStat & INT_TIRQ) && !(prvIntMask & INT_TIRQ)) {
					IntStat &= ~INT_TIRQ;
					PUSH8(PC >> 8);
					PUSH8(PC & 0xFF);
					_BF = 0;
					PUSH8(CompressFlags());
					_IF = IF;
					RefreshPrvIF();
					_TF = _DF = 0;
					PC = RM16(0xFFFA);
					count -= 8;
				}
			}
			RefreshPrvIF();
			prvPC = PC;
			code = FETCH8();
			count -= cycles[code];
			OP(code);
			break;
		case 1:	// TII
			count -= 5;
			ureg8 = RM8(TransSrc++);
			WM8(TransDst++, ureg8);
			if(--TransLen <= 0) {
				TransOpe = 0;
				_TF = 0;
				RefreshPrvIF();
			}
			break;
		case 2:	// TDD
			count -= 5;
			ureg8 = RM8(TransSrc--);
			WM8(TransDst--, ureg8);
			if(--TransLen <= 0) {
				TransOpe = 0;
				_TF = 0;
				RefreshPrvIF();
			}
			break;
		case 3:	// TIN
			count -= 5;
			ureg8 = RM8(TransSrc++);
			WM8(TransDst, ureg8);
			if(--TransLen <= 0) {
				TransOpe = 0;
				_TF = 0;
				RefreshPrvIF();
			}
			break;
		case 4:	// TIA
			ureg8 = RM8(TransSrc++);
			if(TransDir) {
				count -= 5;
				WM8(TransDst++, ureg8);
				TransDir = 0;
			}
			else {
				count -= 6;
				WM8(TransDst--, ureg8);
				TransDir = 1;
			}
			if(--TransLen <= 0) {
				TransOpe = 0;
				_TF = 0;
				RefreshPrvIF();
			}
			break;
		case 5:	// TAI
			if(TransDir) {
				count -= 5;
				TransDir = 0;
				ureg8 = RM8(TransSrc++);
			}
			else {
				count -= 6;
				TransDir = 1;
				ureg8 = RM8(TransSrc--);
			}
			WM8(TransDst++, ureg8);
			if(--TransLen <= 0) {
				TransOpe = 0;
				_TF = 0;
				RefreshPrvIF();
			}
			break;
		}
	}
	first = count;
}

void HUC6260::OP(uint8 code)
{
	uint8 ureg8, addr8;
	int8 rel8;
	uint16 addr16;
	
	switch(code) {
	case 0x00:	// BRK
		PC++;
		PUSH8(PC >> 8);
		PUSH8(PC & 0xFF);
		_TF = 0;
		_BF = BF;
		PUSH8(CompressFlags());
		_IF = IF;
		RefreshPrvIF();
		_DF = 0;
		PC = RM16(0xFFF6);
		break;
	case 0x01:	// ORA(IND,X)
		addr16 = RZP16(FETCH8() + _X);
		ORA(RM8(addr16));
		break;
	case 0x02:	// SXY
		ureg8 = _X;
		_X = _Y;
		_Y = ureg8;
		break;
	case 0x03:	// ST0 #$nn
		ureg8 = FETCH8();
		WM8(0xFF, 0x10, ureg8);
		break;
	case 0x04:	// TSB $ZZ
		addr8 = FETCH8();
		WZP8(addr8, TSB(RZP8(addr8)));
		break;
	case 0x05:	// ORA $ZZ
		ureg8 = FETCH8();
		ORA(RZP8(ureg8));
		break;
	case 0x06:	// ASL $ZZ
		addr8 = FETCH8();
		WZP8(addr8, ASL(RZP8(addr8)));
		break;
	case 0x07:	// RMB0 $ZZ
		ureg8 = FETCH8();
		RMBi(ureg8, 0x01);
		break;
	case 0x08:	// PHP
		_TF = 0;
		PUSH8(CompressFlags());
		break;
	case 0x09:	// ORA #$nn
		ureg8 = FETCH8();
		ORA(ureg8);
		break;
	case 0x0A:	// ASLA
		_A = ASL(_A);
		break;
	case 0x0C:	// TSB $hhll
		addr16 = FETCH16();
		WM8(addr16, TSB(RM8(addr16)));
		break;
	case 0x0D:	// ORA $hhll
		addr16 = FETCH16();
		ORA(RM8(addr16));
		break;
	case 0x0E:	// ASL $hhll
		addr16 = FETCH16();
		WM8(addr16, ASL(RM8(addr16)));
		break;
	case 0x0F:	// BBR0 $ZZ,$rr
		BBRi(0x01);
		break;
	case 0x10:	// BPLREL
		rel8 = (int8)FETCH8();
		if((_NF & NF) == 0) {
			count--;
			count--;
			PC += (int16)rel8;
		}
		break;
	case 0x11:	// ORA(IND),Y
		addr16 = RZP16(FETCH8()) + _Y;
		ORA(RM8(addr16));
		break;
	case 0x12:	// ORA(IND)
		addr16 = RZP16(FETCH8());
		ORA(RM8(addr16));
		break;
	case 0x13:	// ST1 #$nn
		ureg8 = FETCH8();
		WM8(0xFF, 0x12, ureg8);
		break;
	case 0x14:	// TRB $ZZ
		addr8 = FETCH8();
		WZP8(addr8, TRB(RZP8(addr8)));
		break;
	case 0x15:	// ORA $ZZ,X
		ureg8 = FETCH8() + _X;
		ORA(RZP8(ureg8));
		break;
	case 0x16:	// ASL $ZZ,X
		addr8 = FETCH8() + _X;
		WZP8(addr8, ASL(RZP8(addr8)));
		break;
	case 0x17:	// RMB1 $ZZ
		ureg8 = FETCH8();
		RMBi(ureg8, 0x02);
		break;
	case 0x18:	// CLC
		_CF = 0;
		break;
	case 0x19:	// ORA $hhll,Y
		addr16 = FETCH16() + _Y;
		ORA(RM8(addr16));
		break;
	case 0x1A:	// INCA
		UpdateFlagZN(++_A);
		break;
	case 0x1C:	// TRB $hhll
		addr16 = FETCH16();
		WM8(addr16, TRB(RM8(addr16)));
		break;
	case 0x1D:	// ORA $hhll,X
		addr16 = FETCH16() + _X;
		ORA(RM8(addr16));
		break;
	case 0x1E:	// ASL $hhll,X
		addr16 = FETCH16() + _X;
		WM8(addr16, ASL(RM8(addr16)));
		break;
	case 0x1F:	// BBR1 $ZZ,$rr
		BBRi(0x02);
		break;
	case 0x20:	// JSR $hhll
		++PC;
		PUSH8(PC >> 8);
		PUSH8(PC & 0xFF);
		--PC;
		PC = FETCH16();
		break;
	case 0x21:	// AND(IND,X)
		addr16 = RZP16(FETCH8() + _X);
		AND(RM8(addr16));
		break;
	case 0x22:	// SAX
		ureg8 = _A;
		_A = _X;
		_X = ureg8;
		break;
	case 0x23:	// ST2 #$nn
		ureg8 = FETCH8();
		WM8(0xFF, 0x13, ureg8);
		break;
	case 0x24:	// BIT $ZZ
		addr8 = FETCH8();
		BIT(RZP8(addr8));
		break;
	case 0x25:	// AND $ZZ
		ureg8 = FETCH8();
		AND(RZP8(ureg8));
		break;
	case 0x26:	// ROL $ZZ
		addr8 = FETCH8();
		WZP8(addr8, ROL(RZP8(addr8)));
		break;
	case 0x27:	// RMB2 $ZZ
		ureg8 = FETCH8();
		RMBi(ureg8, 0x04);
		break;
	case 0x28:	// PLP
		ExpandFlags(POP8());
		_BF = BF;
		RefreshPrvIF();
		return;
	case 0x29:	// AND #$nn
		ureg8 = FETCH8();
		AND(ureg8);
		break;
	case 0x2A:	// ROLA
		_A = ROL(_A);
		break;
	case 0x2C:	// BIT $hhll
		addr16 = FETCH16();
		BIT(RM8(addr16));
		break;
	case 0x2D:	// AND $hhll
		addr16 = FETCH16();
		AND(RM8(addr16));
		break;
	case 0x2E:	// ROL $hhll
		addr16 = FETCH16();
		WM8(addr16, ROL(RM8(addr16)));
		break;
	case 0x2F:	// BBR2 $ZZ,$rr
		BBRi(0x04);
		break;
	case 0x30:	// BMI $rr
		rel8 = (int8)FETCH8();
		if(_NF & NF) {
			count--;
			count--;
			PC += (int16)rel8;
		}
		break;
	case 0x31:	// AND(IND),Y
		addr16 = RZP16(FETCH8()) + _Y;
		AND(RM8(addr16));
		break;
	case 0x32:	// AND(IND)
		addr16 = RZP16(FETCH8());
		AND(RM8(addr16));
		break;
	case 0x34:	// BIT $ZZ,X
		addr8 = FETCH8() + _X;
		BIT(RZP8(addr8));
		break;
	case 0x35:	// AND $ZZ,X
		ureg8 = FETCH8() + _X;
		AND(RZP8(ureg8));
		break;
	case 0x36:	// ROL $ZZ,X
		addr8 = FETCH8() + _X;
		WZP8(addr8, ROL(RZP8(addr8)));
		break;
	case 0x37:	// RMB3 $ZZ
		ureg8 = FETCH8();
		RMBi(ureg8, 0x08);
		break;
	case 0x38:	// SEC
		_CF = CF;
		break;
	case 0x39:	// AND $hhll,Y
		addr16 = FETCH16() + _Y;
		AND(RM8(addr16));
		break;
	case 0x3A:	// DECA
		UpdateFlagZN(--_A);
		break;
	case 0x3C:	// BIT $hhll,X
		addr16 = FETCH16() + _X;
		BIT(RM8(addr16));
		break;
	case 0x3D:	// AND $hhll,X
		addr16 = FETCH16() + _X;
		AND(RM8(addr16));
		break;
	case 0x3E:	// ROL $hhll,X
		addr16 = FETCH16() + _X;
		WM8(addr16, ROL(RM8(addr16)));
		break;
	case 0x3F:	// BBR3 $ZZ,$rr
		BBRi(0x08);
		break;
	case 0x40:	// RTI
		ExpandFlags(POP8());
		_BF = BF;
		RefreshPrvIF();
		PC = POP8();
		PC |= POP8() << 8;
		return;
	case 0x41:	// EOR(IND,X)
		addr16 = RZP16(FETCH8() + _X);
		EOR(RM8(addr16));
		break;
	case 0x42:	// SAY
		ureg8 = _A;
		_A = _Y;
		_Y = ureg8;
		break;
	case 0x43:	// TMAi
		ureg8 = FETCH8();
		if(ureg8 & 0x80) { _A = (uint8)(MPR[7] >> 13); break; }
		if(ureg8 & 0x40) { _A = (uint8)(MPR[6] >> 13); break; }
		if(ureg8 & 0x20) { _A = (uint8)(MPR[5] >> 13); break; }
		if(ureg8 & 0x10) { _A = (uint8)(MPR[4] >> 13); break; }
		if(ureg8 & 0x08) { _A = (uint8)(MPR[3] >> 13); break; }
		if(ureg8 & 0x04) { _A = (uint8)(MPR[2] >> 13); break; }
		if(ureg8 & 0x02) { _A = (uint8)(MPR[1] >> 13); break; }
		if(ureg8 & 0x01) { _A = (uint8)(MPR[0] >> 13); break; }
		break;
	case 0x44:	// BSR $rr
		PUSH8(PC >> 8);
		PUSH8(PC & 0xFF);
		rel8 = (int8)FETCH8();
		PC += (int16)rel8;
		break;
	case 0x45:	// EOR $ZZ
		ureg8 = FETCH8();
		EOR(RZP8(ureg8));
		break;
	case 0x46:	// LSR $ZZ
		addr8 = FETCH8();
		WZP8(addr8, LSR(RZP8(addr8)));
		break;
	case 0x47:	// RMB4 $ZZ
		ureg8 = FETCH8();
		RMBi(ureg8, 0x10);
		break;
	case 0x48:	// PHA
		PUSH8(_A);
		break;
	case 0x49:	// EOR #$nn
		ureg8 = FETCH8();
		EOR(ureg8);
		break;
	case 0x4A:	// LSRA
		_A = LSR(_A);
		break;
	case 0x4C:	// JMP $hhll
		PC = FETCH16();
		break;
	case 0x4D:	// EOR $hhll
		addr16 = FETCH16();
		EOR(RM8(addr16));
		break;
	case 0x4E:	// LSR $hhll
		addr16 = FETCH16();
		WM8(addr16, LSR(RM8(addr16)));
		break;
	case 0x4F:	// BBR4 $ZZ,$rr
		BBRi(0x10);
		break;
	case 0x50:	// BVC $rr
		rel8 = (int8)FETCH8();
		if((_VF & VF) == 0) {
			count--;
			count--;
			PC += (int16)rel8;
		}
		break;
	case 0x51:	// EOR(IND),Y
		addr16 = RZP16(FETCH8()) + _Y;
		EOR(RM8(addr16));
		break;
	case 0x52:	// EOR(IND)
		addr16 = RZP16(FETCH8());
		EOR(RM8(addr16));
		break;
	case 0x53:	// TAMi
		ureg8 = FETCH8();
		if(ureg8 & 0x01) MPR[0] = _A << 13;
		if(ureg8 & 0x02) MPR[1] = _A << 13;
		if(ureg8 & 0x04) MPR[2] = _A << 13;
		if(ureg8 & 0x08) MPR[3] = _A << 13;
		if(ureg8 & 0x10) MPR[4] = _A << 13;
		if(ureg8 & 0x20) MPR[5] = _A << 13;
		if(ureg8 & 0x40) MPR[6] = _A << 13;
		if(ureg8 & 0x80) MPR[7] = _A << 13;
		break;
	case 0x54:	// CSL
		if(!speed_low) {
			count -= 9;
			speed_low = 1;
			cycles = cycles_slow;
		}
		break;
	case 0x55:	// EOR $ZZ,X
		ureg8 = FETCH8() + _X;
		EOR(RZP8(ureg8));
		break;
	case 0x56:	// LSR $ZZ,X
		addr8 = FETCH8() + _X;
		WZP8(addr8, LSR(RZP8(addr8)));
		break;
	case 0x57:	// RMB5 $ZZ
		ureg8 = FETCH8();
		RMBi(ureg8, 0x20);
		break;
	case 0x58:	// CLI
		_IF = 0;
		break;
	case 0x59:	// EOR $hhll,Y
		addr16 = FETCH16() + _Y;
		EOR(RM8(addr16));
		break;
	case 0x5A:	// PHY
		PUSH8(_Y);
		break;
	case 0x5D:	// EOR $hhll,X
		addr16 = FETCH16() + _X;
		EOR(RM8(addr16));
		break;
	case 0x5E:	// LSR $hhll,X
		addr16 = FETCH16() + _X;
		WM8(addr16, LSR(RM8(addr16)));
		break;
	case 0x5F:	// BBR5 $ZZ,$rr
		BBRi(0x20);
		break;
	case 0x60:	// RTS
		PC = POP8();
		PC |= POP8() << 8;
		++PC;
		break;
	case 0x61:	// ADC($ZZ,X)
		addr16 = RZP16(FETCH8() + _X);
		ADC(RM8(addr16));
		break;
	case 0x62:	// CLA
		_A = 0;
		break;
	case 0x64:	// STZ $ZZ
		addr8 = FETCH8();
		WZP8(addr8, 0);
		break;
	case 0x65:	// ADC $ZZ
		ureg8 = FETCH8();
		ADC(RZP8(ureg8));
		break;
	case 0x66:	// ROR $ZZ
		addr8 = FETCH8();
		WZP8(addr8, ROR(RZP8(addr8)));
		break;
	case 0x67:	// RMB6 $ZZ
		ureg8 = FETCH8();
		RMBi(ureg8, 0x40);
		break;
	case 0x68:	// PLA
		_A = POP8();
		UpdateFlagZN(_A);
		break;
	case 0x69:	// ADC #$nn
		ureg8 = FETCH8();
		ADC(ureg8);
		break;
	case 0x6A:	// RORA
		_A = ROR(_A);
		break;
	case 0x6C:	// JMP($hhll)
		addr16 = FETCH16();
		PC = RM16(addr16);
		break;
	case 0x6D:	// ADC $hhll
		addr16 = FETCH16();
		ADC(RM8(addr16));
		break;
	case 0x6E:	// ROR $hhll
		addr16 = FETCH16();
		WM8(addr16, ROR(RM8(addr16)));
		break;
	case 0x6F:	// BBR6 $ZZ,$rr
		BBRi(0x40);
		break;
	case 0x70:	// BVS $rr
		rel8 = (int8)FETCH8();
		if(_VF & VF) {
			count--;
			count--;
			PC += (int16)rel8;
		}
		break;
	case 0x71:	// ADC($ZZ),Y
		addr16 = RZP16(FETCH8()) + _Y;
		ADC(RM8(addr16));
		break;
	case 0x72:	// ADC($ZZ)
		addr16 = RZP16(FETCH8());
		ADC(RM8(addr16));
		break;
	case 0x73:	// TII $SHSL,$DHDL,$LHLL
		TransSrc = FETCH16();
		TransDst = FETCH16();
		TransLen	= FETCH16();
		TransOpe = 1;
		return;
	case 0x74:	// STZ $ZZ,X
		addr8 = FETCH8() + _X;
		WZP8(addr8, 0);
		break;
	case 0x75:	// ADC $ZZ,X
		ureg8 = FETCH8() + _X;
		ADC(RZP8(ureg8));
		break;
	case 0x76:	// ROR $ZZ,X
		addr8 = FETCH8() + _X;
		WZP8(addr8, ROR(RZP8(addr8)));
		break;
	case 0x77:	// RMB7 $ZZ
		ureg8 = FETCH8();
		RMBi(ureg8, 0x80);
		break;
	case 0x78:	// SEI
		_IF = IF;
		break;
	case 0x79:	// ADC $hhll,Y
		addr16 = FETCH16() + _Y;
		ADC(RM8(addr16));
		break;
	case 0x7A:	// PLY
		_Y = POP8();
		UpdateFlagZN(_Y);
		break;
	case 0x7C:	// JMP $hhll,X
		addr16 = FETCH16() + _X;
		PC = RM16(addr16);
		break;
	case 0x7D:	// ADC $hhll,X
		addr16 = FETCH16() + _X;
		ADC(RM8(addr16));
		break;
	case 0x7E:	// ROR $hhll,X
		addr16 = FETCH16() + _X;
		WM8(addr16, ROR(RM8(addr16)));
		break;
	case 0x7F:	// BBR7 $ZZ,$rr
		BBRi(0x80);
		break;
	case 0x80:	// BRA $rr
		rel8 = (int8)FETCH8();
		PC += (int16)rel8;
		break;
	case 0x81:	// STA(IND,X)
		addr16 = RZP16(FETCH8() + _X);
		WM8(addr16, _A);
		break;
	case 0x82:	// CLX
		_X = 0;
		break;
	case 0x83:	// TST #$nn,$ZZ
		ureg8 = FETCH8();
		addr8 = FETCH8();
		TST(ureg8, RZP8(addr8));
		break;
	case 0x84:	// STY $ZZ
		addr8 = FETCH8();
		WZP8(addr8, _Y);
		break;
	case 0x85:	// STA $ZZ
		addr8 = FETCH8();
		WZP8(addr8, _A);
		break;
	case 0x86:	// STX $ZZ
		addr8 = FETCH8();
		WZP8(addr8, _X);
		break;
	case 0x87:	// SMB0 $ZZ
		ureg8 = FETCH8();
		SMBi(ureg8, 0x01);
		break;
	case 0x88:	// DEY
		UpdateFlagZN(--_Y);
		break;
	case 0x89:	// BIT #$nn
		ureg8 = FETCH8();
		BIT(ureg8);
		break;
	case 0x8A:	// TXA
		TXA();
		break;
	case 0x8C:	// STY $hhll
		addr16 = FETCH16();
		WM8(addr16, _Y);
		break;
	case 0x8D:	// STA $hhll
		addr16 = FETCH16();
		WM8(addr16, _A);
		break;
	case 0x8E:	// STX $hhll
		addr16 = FETCH16();
		WM8(addr16, _X);
		break;
	case 0x8F:	// BBS0 $ZZ,$rr
		BBSi(0x01);
		break;
	case 0x90:	// BCC $rr
		rel8 = (int8)FETCH8();
		if(_CF == 0) {
			count--;
			count--;
			PC += (int16)rel8;
		}
		break;
	case 0x91:	// STA(IND),Y
		addr16 = RZP16(FETCH8()) + _Y;
		WM8(addr16, _A);
		break;
	case 0x92:	// STA(IND)
		addr16 = RZP16(FETCH8());
		WM8(addr16, _A);
		break;
	case 0x93:	// TST #$nn,$hhll
		ureg8 = FETCH8();
		addr16 = FETCH16();
		TST(ureg8, RM8(addr16));
		break;
	case 0x94:	// STY $ZZ,X
		addr8 = FETCH8() + _X;
		WZP8(addr8, _Y);
		break;
	case 0x95:	// STA $ZZ,X
		addr8 = FETCH8() + _X;
		WZP8(addr8, _A);
		break;
	case 0x96:	// STX $ZZ,Y
		addr8 = FETCH8() + _Y;
		WZP8(addr8, _X);
		break;
	case 0x97:	// SMB1 $ZZ
		ureg8 = FETCH8();
		SMBi(ureg8, 0x02);
		break;
	case 0x98:	// TYA
		TYA();
		break;
	case 0x99:	// STA $hhll,Y
		addr16 = FETCH16() + _Y;
		WM8(addr16, _A);
		break;
	case 0x9A:	// TXS
		_S = _X;
		break;
	case 0x9C:	// STZ $hhll
		addr16 = FETCH16();
		WM8(addr16, 0);
		break;
	case 0x9D:	// STA $hhll,X
		addr16 = FETCH16() + _X;
		WM8(addr16, _A);
		break;
	case 0x9E:	// STZ $hhll,X
		addr16 = FETCH16() + _X;
		WM8(addr16, 0);
		break;
	case 0x9F:	// BBS1 $ZZ,$rr
		BBSi(0x02);
		break;
	case 0xA0:	// LDY #$nn
		ureg8 = FETCH8();
		LDY(ureg8);
		break;
	case 0xA1:	// LDA(IND,X)
		addr16 = RZP16(FETCH8() + _X);
		LDA(RM8(addr16));
		break;
	case 0xA2:	// LDX #$nn
		ureg8 = FETCH8();
		LDX(ureg8);
		break;
	case 0xA3:	// TST #$nn,$ZZ,X
		ureg8 = FETCH8();
		addr8 = FETCH8() + _X;
		TST(ureg8, RZP8(addr8));
		break;
	case 0xA4:	// LDY $ZZ
		addr8 = FETCH8();
		LDY(RZP8(addr8));
		break;
	case 0xA5:	// LDA $ZZ
		addr8 = FETCH8();
		LDA(RZP8(addr8));
		break;
	case 0xA6:	// LDX $ZZ
		addr8 = FETCH8();
		LDX(RZP8(addr8));
		break;
	case 0xA7:	// SMB2 $ZZ
		ureg8 = FETCH8();
		SMBi(ureg8, 0x04);
		break;
	case 0xA8:	// TAY
		TAY();
		break;
	case 0xA9:	// LDA #$nn
		ureg8 = FETCH8();
		LDA(ureg8);
		break;
	case 0xAA:	// TAX
		TAX();
		break;
	case 0xAC:	// LDY $hhll
		addr16 = FETCH16();
		LDY(RM8(addr16));
		break;
	case 0xAD:	// LDA $hhll
		addr16 = FETCH16();
		LDA(RM8(addr16));
		break;
	case 0xAE:	// LDX $hhll
		addr16 = FETCH16();
		LDX(RM8(addr16));
		break;
	case 0xAF:	// BBS2 $ZZ,$rr
		BBSi(0x04);
		break;
	case 0xB0:	// BCS $rr
		rel8 = (int8)FETCH8();
		if(_CF) {
			count--;
			count--;
			PC += (int16)rel8;
		}
		break;
	case 0xB1:	// LDA(IND),Y
		addr16 = RZP16(FETCH8()) + _Y;
		LDA(RM8(addr16));
		break;
	case 0xB2:	// LDA (IND)
		addr16 = RZP16(FETCH8());
		LDA(RM8(addr16));
		break;
	case 0xB3:	// TST #$nn,$hhll,X
		ureg8 = FETCH8();
		addr16 = FETCH16() + _X;
		TST(ureg8, RM8(addr16));
		break;
	case 0xB4:	// LDY $ZZ,X
		addr8 = FETCH8() + _X;
		LDY(RZP8(addr8));
		break;
	case 0xB5:	// LDA $ZZ,X
		addr8 = FETCH8() + _X;
		LDA(RZP8(addr8));
		break;
	case 0xB6:	// LDX $ZZ,Y
		addr8 = FETCH8() + _Y;
		LDX(RZP8(addr8));
		break;
	case 0xB7:	// SMB3 $ZZ
		ureg8 = FETCH8();
		SMBi(ureg8, 0x08);
		break;
	case 0xB8:	// CLV
		_VF = 0;
		break;
	case 0xB9:	// LDA $hhll,Y
		addr16 = FETCH16() + _Y;
		LDA(RM8(addr16));
		break;
	case 0xBA:	// TSX
		TSX();
		break;
	case 0xBC:	// LDY $hhll,X
		addr16 = FETCH16() + _X;
		LDY(RM8(addr16));
		break;
	case 0xBD:	// LDA $hhll,X
		addr16 = FETCH16() + _X;
		LDA(RM8(addr16));
		break;
	case 0xBE:	// LDX $hhll,Y
		addr16 = FETCH16() + _Y;
		LDX(RM8(addr16));
		break;
	case 0xBF:	// BBS3 $ZZ,$rr
		BBSi(0x08);
		break;
	case 0xC0:	// CPY #$nn
		ureg8 = FETCH8();
		CPY(ureg8);
		break;
	case 0xC1:	// CMP(IND,X)
		addr16 = RZP16(FETCH8() + _X);
		CMP(RM8(addr16));
		break;
	case 0xC2:	// CLY
		_Y = 0;
		break;
	case 0xC3:	// TDD $SHSL,$DHDL,$LHLL
		TransSrc = FETCH16();
		TransDst = FETCH16();
		TransLen	= FETCH16();
		TransOpe = 2;
		return;
	case 0xC4:	// CPY $ZZ
		ureg8 = FETCH8();
		CPY(RZP8(ureg8));
		break;
	case 0xC5:	// CMP $ZZ
		ureg8 = FETCH8();
		CMP(RZP8(ureg8));
		break;
	case 0xC6:	// DEC $ZZ
		addr8 = FETCH8();
		ureg8 = RZP8(addr8);
		UpdateFlagZN(--ureg8);
		WZP8(addr8, ureg8);
		break;
	case 0xC7:	// SMB4 $ZZ
		ureg8 = FETCH8();
		SMBi(ureg8, 0x10);
		break;
	case 0xC8:	// INY
		UpdateFlagZN(++_Y);
		break;
	case 0xC9:	// CMP #$nn
		ureg8 = FETCH8();
		CMP(ureg8);
		break;
	case 0xCA:	// DEX
		UpdateFlagZN(--_X);
		break;
			
	case 0xCC:	// CPY $hhll
		addr16 = FETCH16();
		CPY(RM8(addr16));
		break;
	case 0xCD:	// CMP $hhll
		addr16 = FETCH16();
		CMP(RM8(addr16));
		break;
	case 0xCE:	// DEC $hhll
		addr16 = FETCH16();
		ureg8 = RM8(addr16);
		UpdateFlagZN(--ureg8);
		WM8(addr16, ureg8);
		break;
	case 0xCF:	// BBS4 $ZZ,$rr
		BBSi(0x10);
		break;
	case 0xD0:	// BNE $rr
		rel8 = (int8)FETCH8();
		if(_ZF) {
			count--;
			count--;
			PC += (int16)rel8;
		}
		break;
	case 0xD1:	// CMP(IND),Y
		addr16 = RZP16(FETCH8()) + _Y;
		CMP(RM8(addr16));
		break;
	case 0xD2:	// CMP(IND)
		addr16 = RZP16(FETCH8());
		CMP(RM8(addr16));
		break;
	case 0xD3:	// TIN $SHSL,$DHDL,$LHLL
		TransSrc = FETCH16();
		TransDst = FETCH16();
		TransLen	= FETCH16();
		TransOpe = 3;
		return;
	case 0xD4:	// CSH
		if(speed_low) {
			count -= 6;
			speed_low = 0;
			cycles = cycles_high;
		}
		break;
	case 0xD5:	// CMP $ZZ,X
		ureg8 = FETCH8() + _X;
		CMP(RZP8(ureg8));
		break;
	case 0xD6:	// DEC $ZZ,X
		addr8 = FETCH8() + _X;
		ureg8 = RZP8(addr8);
		UpdateFlagZN(--ureg8);
		WZP8(addr8, ureg8);
		break;
	case 0xD7:	// SMB5 $ZZ
		ureg8 = FETCH8();
		SMBi(ureg8, 0x20);
		break;
	case 0xD8:	// CLD
		_DF = 0;
		break;
	case 0xD9:	// CMP $hhll,Y
		addr16 = FETCH16() + _Y;
		CMP(RM8(addr16));
		break;
	case 0xDA:	// PHX
		PUSH8(_X);
		break;
	case 0xDD:	// CMP $hhll,X
		addr16 = FETCH16() + _X;
		CMP(RM8(addr16));
		break;
	case 0xDE:	// DEC $hhll,X
		addr16 = FETCH16() + _X;
		ureg8 = RM8(addr16);
		UpdateFlagZN(--ureg8);
		WM8(addr16, ureg8);
		break;
	case 0xDF:	// BBS5 $ZZ,$rr
		BBSi(0x20);
		break;
	case 0xE0:	// CPX #$nn
		ureg8 = FETCH8();
		CPX(ureg8);
		break;
	case 0xE1:	// SBC(IND,X)
		addr16 = RZP16(FETCH8() + _X);
		SBC(RM8(addr16));
		break;
	case 0xE3:	// TIA $SHSL,$DHDL,$LHLL
		TransSrc = FETCH16();
		TransDst = FETCH16();
		TransLen	= FETCH16();
		TransDir = 1;
		TransOpe = 4;
		return;
	case 0xE4:	// CPX $ZZ
		ureg8 = FETCH8();
		CPX(RZP8(ureg8));
		break;
	case 0xE5:	// SBC $ZZ
		ureg8 = FETCH8();
		SBC(RZP8(ureg8));
		break;
	case 0xE6:	// INC $ZZ
		addr8 = FETCH8();
		ureg8 = RZP8(addr8);
		UpdateFlagZN(++ureg8);
		WZP8(addr8, ureg8);
		break;
	case 0xE7:	// SMB6 $ZZ
		ureg8 = FETCH8();
		SMBi(ureg8, 0x40);
		break;
	case 0xE8:	// INX
		UpdateFlagZN(++_X);
		break;
	case 0xE9:	// SBC #$nn
		ureg8 = FETCH8();
		SBC(ureg8);
		break;
	case 0xEA:	// NOP
		break;
	case 0xEC:	// CPX $hhll
		addr16 = FETCH16();
		CPX(RM8(addr16));
		break;
	case 0xED:	// SBC $hhll
		addr16 = FETCH16();
		SBC(RM8(addr16));
		break;
	case 0xEE:	// INC $hhll
		addr16 = FETCH16();
		ureg8 = RM8(addr16);
		UpdateFlagZN(++ureg8);
		WM8(addr16, ureg8);
		break;
	case 0xEF:	// BBS6 $ZZ,$rr
		BBSi(0x40);
		break;
	case 0xF0:	// BEQ $rr
		rel8 = (int8)FETCH8();
		if(_ZF == 0) {
			count--;
			count--;
			PC += (int16)rel8;
		}
		break;
	case 0xF1:	// SBC(IND),Y
		addr16 = RZP16(FETCH8()) + _Y;
		SBC(RM8(addr16));
		break;
	case 0xF2:	// SBC(IND)
		addr16 = RZP16(FETCH8());
		SBC(RM8(addr16));
		break;
	case 0xF3:	// TAI $SHSL,$DHDL,$LHLL
		TransSrc = FETCH16();
		TransDst = FETCH16();
		TransLen = FETCH16();
		TransDir = 1;
		TransOpe = 5;
		return;
	case 0xF4:	// SET
		_TF = TF;
		return;
	case 0xF5:	// SBC $ZZ,X
		ureg8 = FETCH8() + _X;
		SBC(RZP8(ureg8));
		break;
	case 0xF6:	// INC $ZZ,X
		addr8 = FETCH8() + _X;
		ureg8 = RZP8(addr8);
		UpdateFlagZN(++ureg8);
		WZP8(addr8, ureg8);
		break;
	case 0xF7:	// SMB7 $ZZ
		ureg8 = FETCH8();
		SMBi(ureg8, 0x80);
		break;
	case 0xF8:	// SED
		_DF = DF;
		break;
	case 0xF9:	// SBC $hhll,Y
		addr16 = FETCH16() + _Y;
		SBC(RM8(addr16));
		break;
	case 0xFA:	// PLX
		_X = POP8();
		UpdateFlagZN(_X);
		break;
	case 0xFD:	// SBC $hhll,X
		addr16 = FETCH16() + _X;
		SBC(RM8(addr16));
		break;
	case 0xFE:	// INC $hhll,X
		addr16 = FETCH16() + _X;
		ureg8 = RM8(addr16);
		UpdateFlagZN(++ureg8);
		WM8(addr16, ureg8);
		break;
	case 0xFF:	// BBS7 $ZZ,$rr
		BBSi(0x80);
		break;
	default:
		break;
	}
	_TF = 0;
}
