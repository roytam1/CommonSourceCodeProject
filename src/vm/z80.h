/*
	Skelton for retropc emulator

	Origin : MAME
	Author : Takeda.Toshiya
	Date   : 2006.08.18-

	[ Z80 ]
*/

#ifndef _Z80_H_ 
#define _Z80_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#ifdef HAS_NSC800
#define SIG_NSC800_INT	0
#define SIG_NSC800_RSTA	1
#define SIG_NSC800_RSTB	2
#define SIG_NSC800_RSTC	3
#endif

class Z80 : public DEVICE
{
private:
	/* ---------------------------------------------------------------------------
	contexts
	--------------------------------------------------------------------------- */
	
	DEVICE *d_mem, *d_io, *d_pic;
#ifdef SINGLE_MODE_DMA
	DEVICE *d_dma;
#endif
	outputs_t outputs_busack;
	
	/* ---------------------------------------------------------------------------
	registers
	--------------------------------------------------------------------------- */
	
	int icount;
	uint16 prevpc;
	pair af, bc, de, hl, ix, iy;
	pair af2, bc2, de2, hl2;
	uint8 I, R, R2;
	uint16 PC, SP, EA;
	
	bool busreq, halt;
	uint8 im, iff1, iff2, icr;
	bool after_ei, after_ldair;
	uint32 intr_req_bit, intr_pend_bit;
	
	/* ---------------------------------------------------------------------------
	virtual machine interfaces
	--------------------------------------------------------------------------- */
	
	// memory
	inline uint8 RM8(uint16 addr);
	inline void WM8(uint16 addr, uint8 val);
	inline uint16 RM16(uint16 addr);
	inline void WM16(uint16 addr, uint16 val);
	inline uint8 FETCHOP();
	inline uint8 FETCH8();
	inline uint16 FETCH16();
	inline uint16 POP16();
	inline void PUSH16(uint16 val);
	
	// i/o
	inline uint8 IN8(uint8 laddr, uint8 haddr);
	inline void OUT8(uint8 laddr, uint8 haddr, uint8 val);
	
	/* ---------------------------------------------------------------------------
	opecodes
	--------------------------------------------------------------------------- */
	
	inline uint8 INC(uint8 value);
	inline uint8 DEC(uint8 value);
	
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
	
	// opecode
	void OP_CB(uint8 code);
	void OP_XY(uint8 code);
	void OP_DD(uint8 code);
	void OP_FD(uint8 code);
	void OP_ED(uint8 code);
	void OP(uint8 code);
	void run_one_opecode();
	
	/* ---------------------------------------------------------------------------
	debug
	--------------------------------------------------------------------------- */
	
#ifdef _CPU_DEBUG_LOG
	int debug_count, debug_ptr;
	bool dasm_done;
	uint8 debug_ops[4];
	_TCHAR debug_dasm[32];
	
	inline uint8 DEBUG_FETCHOP() {
		return debug_ops[debug_ptr++];
	}
	inline uint8 DEBUG_FETCH8() {
		return debug_ops[debug_ptr++];
	}
	inline uint16 DEBUG_FETCH16() {
		uint16 val = debug_ops[debug_ptr] | (debug_ops[debug_ptr + 1] << 8);
		debug_ptr += 2;
		return val;
	}
	inline int8 DEBUG_FETCH8_REL() {
		return (int8)debug_ops[debug_ptr++];
	}
	inline uint16 DEBUG_FETCH8_RELPC() {
		int8 res = (int8)debug_ops[debug_ptr++];
		return prevpc + debug_ptr + res;
	}
	void DASM();
	void DASM_CB();
	void DASM_DD();
	void DASM_ED();
	void DASM_FD();
	void DASM_DDCB();
	void DASM_FDCB();
#endif
	
public:
	Z80(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		busreq = false;
#ifdef SINGLE_MODE_DMA
		d_dma = NULL;
#endif
		init_output_signals(&outputs_busack);
	}
	~Z80() {}
	
	// common functions
	void reset();
	int run(int clock);
	void write_signal(int id, uint32 data, uint32 mask);
	void set_intr_line(bool line, bool pending, uint32 bit) {
		uint32 mask = 1 << bit;
		intr_req_bit = line ? (intr_req_bit | mask) : (intr_req_bit & ~mask);
		intr_pend_bit = pending ? (intr_pend_bit | mask) : (intr_pend_bit & ~mask);
	}
	uint32 get_pc() {
		return prevpc;
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
#ifdef SINGLE_MODE_DMA
	void set_context_dma(DEVICE* device) {
		d_dma = device;
	}
#endif
	void set_context_busack(DEVICE* device, int id, uint32 mask) {
		register_output_signal(&outputs_busack, device, id, mask);
	}
	void set_pc(uint16 value) {
		PC = value;
	}
	void set_sp(uint16 value) {
		SP = value;
	}
};

#endif

