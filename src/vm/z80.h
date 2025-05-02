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

#ifdef Z80_M1_CYCLE_WAIT
#define SIG_Z80_M1_CYCLE_WAIT	0
#endif
#ifdef HAS_NSC800
#define SIG_NSC800_INT	1
#define SIG_NSC800_RSTA	2
#define SIG_NSC800_RSTB	3
#define SIG_NSC800_RSTC	4
#endif
#define NMI_REQ_BIT	0x80000000

class Z80 : public DEVICE
{
private:
	/* ---------------------------------------------------------------------------
	contexts
	--------------------------------------------------------------------------- */
	
	DEVICE *d_mem, *d_io, *d_pic;
	outputs_t outputs_busack;
	
	/* ---------------------------------------------------------------------------
	registers
	--------------------------------------------------------------------------- */
	
	int count, first;
#ifdef Z80_M1_CYCLE_WAIT
	int m1_cycle_wait;
#endif
	bool busreq, halt;
	pair regs[6];
	uint8 _I, _R, IM, IFF1, IFF2, ICR;
	uint16 SP, PC, prvPC, exAF, exBC, exDE, exHL, EA;
	uint32 intr_req_bit, intr_pend_bit;
	
	/* ---------------------------------------------------------------------------
	virtual machine interfaces
	--------------------------------------------------------------------------- */
	
	// memory
	inline uint8 RM8(uint16 addr) {
#ifdef CPU_MEMORY_WAIT
		int wait;
		uint8 val = d_mem->read_data8w(addr, &wait);
		count -= wait;
		return val;
#else
		return d_mem->read_data8(addr);
#endif
	}
	inline void WM8(uint16 addr, uint8 val) {
#ifdef CPU_MEMORY_WAIT
		int wait;
		d_mem->write_data8w(addr, val, &wait);
		count -= wait;
#else
		d_mem->write_data8(addr, val);
#endif
	}
	
	inline uint16 RM16(uint16 addr) {
#ifdef CPU_MEMORY_WAIT
		int wait;
		uint16 val = d_mem->read_data16w(addr, &wait);
		count -= wait;
		return val;
#else
		return d_mem->read_data16(addr);
#endif
	}
	inline void WM16(uint16 addr, uint16 val) {
#ifdef CPU_MEMORY_WAIT
		int wait;
		d_mem->write_data16w(addr, val, &wait);
		count -= wait;
#else
		d_mem->write_data16(addr, val);
#endif
	}
	inline uint8 FETCHOP() {
#ifdef Z80_M1_CYCLE_WAIT
		count -= m1_cycle_wait;
#endif
		_R = (_R & 0x80) | ((_R + 1) & 0x7f);
		return d_mem->read_data8(PC++);
	}
	inline uint8 FETCH8() {
#ifdef CPU_MEMORY_WAIT
		int wait;
		uint8 val = d_mem->read_data8w(PC++, &wait);
		count -= wait;
		return val;
#else
		return d_mem->read_data8(PC++);
#endif
	}
	inline uint16 FETCH16() {
#ifdef CPU_MEMORY_WAIT
		int wait;
		uint16 val = d_mem->read_data16w(PC, &wait);
		count -= wait;
#else
		uint16 val = d_mem->read_data16(PC);
#endif
		PC += 2;
		return val;
	}
	inline uint16 POP16() {
#ifdef CPU_MEMORY_WAIT
		int wait;
		uint16 val = d_mem->read_data16w(SP, &wait);
		count -= wait;
#else
		uint16 val = d_mem->read_data16(SP);
#endif
		SP += 2;
		return val;
	}
	inline void PUSH16(uint16 val) {
		SP -= 2;
#ifdef CPU_MEMORY_WAIT
		int wait;
		d_mem->write_data16w(SP, val, &wait);
		count -= wait;
#else
		d_mem->write_data16(SP, val);
#endif
	}
	
	// i/o
	inline uint8 IN8(uint8 laddr, uint8 haddr) {
		uint32 addr = laddr | (haddr << 8);
#ifdef CPU_IO_WAIT
		int wait;
		uint8 val = d_io->read_io8w(addr, &wait);
		count -= wait;
		return val;
#else
		return d_io->read_io8(addr);
#endif
	}
	inline void OUT8(uint8 laddr, uint8 haddr, uint8 val) {
#ifdef HAS_NSC800
		if(laddr == 0xbb) {
			ICR = val;
			return;
		}
#endif
		uint32 addr = laddr | (haddr << 8);
#ifdef CPU_IO_WAIT
		int wait;
		d_io->write_io8w(addr, val, &wait);
		count -= wait;
#else
		d_io->write_io8(addr, val);
#endif
	}
	
	// interrupt
	inline void NOTIFY_RETI() {
		d_pic->intr_reti();
	}
	inline uint32 ACK_INTR() {
		return d_pic->intr_ack();
	}
	
	/* ---------------------------------------------------------------------------
	opecodes
	--------------------------------------------------------------------------- */
	
	// CB,DD,ED,FD
	void OP(uint8 code);
	void OP_CB();
	void OP_DD();
	void OP_ED();
	void OP_FD();
	void OP_XY();
	
	// opecode
	inline uint16 EXSP(uint16 reg);
	inline uint8 INC(uint8 value);
	inline uint8 DEC(uint8 value);
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
	
	/* ---------------------------------------------------------------------------
	debug
	--------------------------------------------------------------------------- */
	
#ifdef _CPU_DEBUG_LOG
	int debug_count, debug_ptr;
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
		int res = debug_ops[debug_ptr++];
		return (res < 128) ? res : (res - 256);
	}
	inline uint16 DEBUG_FETCH8_RELPC() {
		int res = debug_ops[debug_ptr++];
		return prvPC + debug_ptr + ((res < 128) ? res : (res - 256));
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
		count = first = 0;	// passed_clock must be zero at initialize
#ifdef Z80_M1_CYCLE_WAIT
		m1_cycle_wait = Z80_M1_CYCLE_WAIT;
#endif
		busreq = false;
		init_output_signals(&outputs_busack);
	}
	~Z80() {}
	
	// common functions
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
		regist_output_signal(&outputs_busack, device, id, mask);
	}
};

#endif

