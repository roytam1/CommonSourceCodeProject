/*
	Skelton for retropc emulator

	Origin : MAME
	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ Z80 ]
*/

#ifndef _Z80_H_ 
#define _Z80_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#ifdef NSC800
#define SIG_NSC800_INT	0
#define SIG_NSC800_RSTA	1
#define SIG_NSC800_RSTB	2
#define SIG_NSC800_RSTC	3
#endif
#define NMI_REQ_BIT	0x80000000

class Z80 : public DEVICE
{
private:
	/* ---------------------------------------------------------------------------
	contexts
	--------------------------------------------------------------------------- */
	
	DEVICE *d_mem, *d_io, *d_pic;
	DEVICE *d_busack[MAX_OUTPUT];
	int did[MAX_OUTPUT], dcount;
	uint32 dmask[MAX_OUTPUT];
	
	/* ---------------------------------------------------------------------------
	registers
	--------------------------------------------------------------------------- */
	
	int count, first;
	bool busreq, halt;
	pair regs[7], wz;
	uint8 _I, _R, IM, IFF1, IFF2, ICR;
	uint16 SP, PC, prvPC, exAF, exBC, exDE, exHL, EA;
	uint32 intr_req_bit, intr_pend_bit;
	
	/* ---------------------------------------------------------------------------
	flag tables
	--------------------------------------------------------------------------- */
	
	uint8 SZ[256];
	uint8 SZ_BIT[256];
	uint8 SZP[256];
	uint8 SZHV_inc[256];
	uint8 SZHV_dec[256];
	uint8 SZHVC_add[2 * 256 * 256];
	uint8 SZHVC_sub[2 * 256 * 256];
	
	/* ---------------------------------------------------------------------------
	virtual machine interfaces
	--------------------------------------------------------------------------- */
	
	inline uint8 RM8(uint16 addr);
	inline void WM8(uint16 addr, uint8 val);
	inline uint16 RM16(uint16 addr);
	inline void WM16(uint16 addr, uint16 val);
	inline uint8 FETCHOP();
	inline uint8 FETCH8();
	inline uint16 FETCH16();
	inline uint16 POP16();
	inline uint8 IN8(uint16 addr);
	inline void OUT8(uint16 addr, uint8 val);
	
	/* ---------------------------------------------------------------------------
	opecodes
	--------------------------------------------------------------------------- */
	
	// CB,DD,ED,FD
	void OP(uint8 code);
	void OP_CB(uint8 code);
	void OP_DD(uint8 code);
	void OP_ED(uint8 code);
	void OP_FD(uint8 code);
	void OP_XYCB(uint8 code);
	
	// opecode
	inline uint8 INC(uint8 value);
	inline uint8 DEC(uint8 value);
	inline uint16 EXSP(uint16 reg);
	inline uint16 ADD16(uint16 dreg, uint16 sreg);
	inline uint8 RLC(uint8 value);
	inline uint8 RRC(uint8 value);
	inline uint8 RL(uint8 value);
	inline uint8 RR(uint8 value);
	inline uint8 SLA(uint8 value);
	inline uint8 SRA(uint8 value);
	inline uint8 SLL(uint8 value);
	inline uint8 SRL(uint8 value);
	inline uint8 RES(uint8 bit, uint8 value);
	inline uint8 SET(uint8 bit, uint8 value);
	
public:
	Z80(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		count = first = 0;	// passed_clock must be zero at initialize
		busreq = false;
		dcount = 0;
	}
	~Z80() {}
	
	// common functions
	void initialize();
	void reset();
	void run(int clock);
	void write_signal(int id, uint32 data, uint32 mask);
	void set_intr_line(bool line, bool pending, uint32 bit) {
		uint32 mask = 1 << bit;
		intr_req_bit = line ? (intr_req_bit | mask) : (intr_req_bit & ~mask);
		intr_pend_bit = pending ? (intr_pend_bit | mask) : (intr_pend_bit & ~mask);
	}
	int passed_clock() {
		return first - count;
	}
	uint32 get_prv_pc() {
		return prvPC;
	}
	void set_pc(uint32 pc) {
		PC = pc;
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
	void set_context_busack(DEVICE* device, int id, uint32 mask) {
		int c = dcount++;
		d_busack[c] = device; did[c] = id; dmask[c] = mask;
	}
};

#endif

