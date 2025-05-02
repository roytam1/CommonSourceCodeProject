/*
	Skelton for retropc emulator

	Origin : Ootake
	Author : Takeda.Toshiya
	Date   : 2009.03.08-

	[ HuC6260 ]
*/

#ifndef _HUC6260_H_ 
#define _HUC6260_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_HUC6260_IRQ2	0
#define SIG_HUC6260_IRQ1	1
#define SIG_HUC6260_TIRQ	2
#define SIG_HUC6260_INTMASK	3
#define SIG_HUC6260_INTSTAT	4

class HUC6260 : public DEVICE
{
private:
	/* ---------------------------------------------------------------------------
	contexts
	--------------------------------------------------------------------------- */
	
	DEVICE *d_mem;
	
	/* ---------------------------------------------------------------------------
	registers
	--------------------------------------------------------------------------- */
	
	int cycles_high[256], cycles_slow[256], *cycles;
	int count, first, speed_low;
	
	uint8 _A, _X, _Y, _S, _P;
	uint8 _CF, _ZF, _IF, _DF, _BF, _TF, _VF, _NF, prvIF;
	uint16 PC, prvPC;
	uint8 IntStat, prvIntStat;
	uint8 IntMask, prvIntMask;
	uint32 MPR[8];
	int TransOpe, TransDir;
	uint16 TransSrc, TransDst, TransLen;
	
	/* ---------------------------------------------------------------------------
	virtual machine interfaces
	--------------------------------------------------------------------------- */
	
	// memory
	inline uint8 RM8(uint16 addr) {
		return d_mem->read_data8(MPR[addr >> 13] | (addr & 0x1fff));
	}
	inline void WM8(uint16 addr, uint8 val) {
		d_mem->write_data8(MPR[addr >> 13] | (addr & 0x1fff), val);
	}
	inline void WM8(uint16 mpr, uint16 addr, uint8 val) {
		d_mem->write_data8((mpr << 13) | (addr & 0x1fff), val);
	}
	inline uint16 RM16(uint16 addr) {
		return d_mem->read_data16(MPR[addr >> 13] | (addr & 0x1fff));
	}
	inline void WM16(uint16 addr, uint16 val) {
		d_mem->write_data16(MPR[addr >> 13] | (addr & 0x1fff), val);
	}
	inline uint8 RZP8(uint8 addr) {
		// zero page : $2000-$20FF
		return d_mem->read_data8(MPR[1] | addr);
	}
	inline void WZP8(uint8 addr, uint8 val) {
		// zero page : $2000-$20FF
		d_mem->write_data8(MPR[1] | addr, val);
	}
	inline uint16 RZP16(uint8 addr) {
		// zero page : $2000-$20FF
		return d_mem->read_data16(MPR[1] | addr);
	}
	inline void WZP16(uint8 addr, uint16 val) {
		// zero page : $2000-$20FF
		d_mem->write_data16(MPR[1] | addr, val);
	}
	inline void PUSH8(uint8 val) {
		// stack : $2100-$21FF
		d_mem->write_data8(MPR[1] | 0x100 | (_S--), val);
	}
	inline uint8 POP8() {
		// stack : $2100-$21FF
		return d_mem->read_data8(MPR[1] | 0x100 | (++_S));
	}
	inline uint8 FETCH8() {
		uint8 val = d_mem->read_data8(MPR[PC >> 13] | (PC & 0x1fff));
		PC++;
		return val;
	}
	inline uint16 FETCH16() {
		uint16 val = d_mem->read_data16(MPR[PC >> 13] | (PC & 0x1fff));
		PC += 2;
		return val;
	}
	
	/* ---------------------------------------------------------------------------
	opecodes
	--------------------------------------------------------------------------- */
	
	// interuupt
	void RefreshPrvIF();
	
	// flags
	void UpdateFlagZN(uint8 val);
	void ExpandFlags(uint8 val);
	uint8 CompressFlags();
	
	// opecodes
	void BIT(uint8 val);
	void ADC(uint8 val);
	void SBC(uint8 val);
	void AND(uint8 val);
	uint8 ASL(uint8 val);
	uint8 LSR(uint8 val);
	uint8 ROL(uint8 val);
	uint8 ROR(uint8 val);
	void CMP(uint8 val);
	void CPX(uint8 val);
	void CPY(uint8 val);
	void EOR(uint8 val);
	void ORA(uint8 val);
	void LDA(uint8 val);
	void LDX(uint8 val);
	void LDY(uint8 val);
	void TAX();
	void TAY();
	void TXA();
	void TYA();
	void TSX();
	void BBRi(uint8 bit);
	void BBSi(uint8 bit);
	uint8 TRB(uint8 val);
	uint8 TSB(uint8 val);
	void TST(uint8 imm, uint8 M);
	void RMBi(uint8 zp, uint8 bit);
	void SMBi(uint8 zp, uint8 bit);
	void OP(uint8 code);
	
public:
	HUC6260(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		count = first = 0;	// passed_clock must be zero at initialize
	}
	~HUC6260() {}
	
	// common functions
	void initialize();
	void reset();
	void run(int clock);
	void write_signal(int id, uint32 data, uint32 mask);
	uint32 read_signal(int id);
	int passed_clock() {
		return first - count;
	}
	uint32 get_prv_pc() {
		return prvPC;
	}
	
	// unique function
	void set_context_mem(DEVICE* device) {
		d_mem = device;
	}
};

#endif

