/*
	Skelton for retropc emulator

	Origin : MESS UPD7810 Core
	Author : Takeda.Toshiya
	Date   : 2006.08.21 -

	[ uPD7801 ]
*/

#ifndef _UPD7801_H_
#define _UPD7801_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_UPD7801_INTF0	0
#define SIG_UPD7801_INTF1	1
#define SIG_UPD7801_INTF2	2
#define SIG_UPD7801_WAIT	3

// virtual i/o port address
#define P_A	0
#define P_B	1
#define P_C	2
#define P_SI	3
#define P_SO	4

class UPD7801 : public DEVICE
{
private:
	/* ---------------------------------------------------------------------------
	contexts
	--------------------------------------------------------------------------- */
	
	DEVICE *d_mem, *d_io;
	
	/* ---------------------------------------------------------------------------
	registers
	--------------------------------------------------------------------------- */
	
	int count, period, scount, tcount;
	bool wait;
	
	pair regs[4];
	uint16 SP, PC, prevPC, altVA, altBC, altDE, altHL;
	uint8 PSW, IRR, IFF, SIRQ, HALT, MK, MB, MC, TM0, TM1, SR;
	// for port c
	uint8 SAK, TO, PORTC;
	
	/* ---------------------------------------------------------------------------
	virtual machine interface
	--------------------------------------------------------------------------- */
	
	// memory
	inline uint8 RM8(uint16 addr);
	inline void WM8(uint16 addr, uint8 val);
	inline uint16 RM16(uint16 addr);
	inline void WM16(uint16 addr, uint16 val);
	inline uint8 FETCH8();
	inline uint16 FETCH16();
	inline uint16 FETCHWA();
	inline uint8 POP8();
	inline void PUSH8(uint8 val);
	inline uint16 POP16();
	inline void PUSH16(uint16 val);
	
	// i/o
	inline uint8 IN8(int port);
	inline void OUT8(int port, uint8 val);
	inline void UPDATE_PORTC(uint8 IOM);
	
	/* ---------------------------------------------------------------------------
	opecode
	--------------------------------------------------------------------------- */
	
	void run_one_opecode();
	void OP();
	void OP48();
	void OP4C();
	void OP4D();
	void OP60();
	void OP64();
	void OP70();
	void OP74();
	
public:
	UPD7801(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~UPD7801() {}
	
	// common function
	void reset();
	int run(int clock);
	void write_signal(int id, uint32 data, uint32 mask);
	uint32 get_pc() {
		return prevPC;
	}
	
	// unique function
	void set_context_mem(DEVICE* device) {
		d_mem = device;
	}
	void set_context_io(DEVICE* device) {
		d_io = device;
	}
};

#endif
