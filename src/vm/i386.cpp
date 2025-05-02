/*
	Skelton for retropc emulator

	Origin : MAME i386 core
	Author : Takeda.Toshiya
	Date  : 2009.06.08-

	[ i386/i486/Pentium/MediaGX ]
*/

#include "i386.h"

/* ----------------------------------------------------------------------------
	MAME i386
---------------------------------------------------------------------------- */

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#pragma warning( disable : 4018 )
#pragma warning( disable : 4065 )
#pragma warning( disable : 4146 )
#pragma warning( disable : 4244 )
#endif

#if defined(HAS_I386)
	#define CPU_MODEL i386
#elif defined(HAS_I486)
	#define CPU_MODEL i486
#elif defined(HAS_PENTIUM)
	#define CPU_MODEL pentium
#elif defined(HAS_MEDIAGX)
	#define CPU_MODEL mediagx
#elif defined(HAS_PENTIUM_PRO)
	#define CPU_MODEL pentium_pro
#elif defined(HAS_PENTIUM_MMX)
	#define CPU_MODEL pentium_mmx
#elif defined(HAS_PENTIUM2)
	#define CPU_MODEL pentium2
#elif defined(HAS_PENTIUM3)
	#define CPU_MODEL pentium3
#elif defined(HAS_PENTIUM4)
	#define CPU_MODEL pentium4
#endif

#ifndef INLINE
#define INLINE inline
#endif

#ifndef _BIG_ENDIAN
#define LSB_FIRST
#endif

#define U64(v) UINT64(v)
#define offs_t UINT32

// constants for expression endianness
enum endianness_t
{
	ENDIANNESS_LITTLE,
	ENDIANNESS_BIG
};

// declare native endianness to be one or the other
#ifdef LSB_FIRST
const endianness_t ENDIANNESS_NATIVE = ENDIANNESS_LITTLE;
#else
const endianness_t ENDIANNESS_NATIVE = ENDIANNESS_BIG;
#endif
// endian-based value: first value is if 'endian' is little-endian, second is if 'endian' is big-endian
#define ENDIAN_VALUE_LE_BE(endian,leval,beval)	(((endian) == ENDIANNESS_LITTLE) ? (leval) : (beval))
// endian-based value: first value is if native endianness is little-endian, second is if native is big-endian
#define NATIVE_ENDIAN_VALUE_LE_BE(leval,beval)	ENDIAN_VALUE_LE_BE(ENDIANNESS_NATIVE, leval, beval)
// endian-based value: first value is if 'endian' matches native, second is if 'endian' doesn't match native
#define ENDIAN_VALUE_NE_NNE(endian,leval,beval)	(((endian) == ENDIANNESS_NATIVE) ? (neval) : (nneval))

enum line_state
{
	CLEAR_LINE = 0,				// clear (a fired or held) line
	ASSERT_LINE,				// assert an interrupt immediately
	HOLD_LINE,				// hold interrupt line until acknowledged
	PULSE_LINE				// pulse interrupt line instantaneously (only for NMI, RESET)
};

enum
{
	INPUT_LINE_IRQ = 0,
	INPUT_LINE_NMI
};

#define CPU_INIT_NAME(name)			cpu_init_##name
#define CPU_INIT(name)				void* CPU_INIT_NAME(name)()
#define CPU_INIT_CALL(name)			CPU_INIT_NAME(name)()

#define CPU_RESET_NAME(name)			cpu_reset_##name
#define CPU_RESET(name)				void CPU_RESET_NAME(name)(i386_state *cpustate)
#define CPU_RESET_CALL(name)			CPU_RESET_NAME(name)(cpustate)

#define CPU_EXECUTE_NAME(name)			cpu_execute_##name
#define CPU_EXECUTE(name)			int CPU_EXECUTE_NAME(name)(i386_state *cpustate, int cycles)
#define CPU_EXECUTE_CALL(name)			CPU_EXECUTE_NAME(name)(cpustate, cycles)

#define fatalerror(...) exit(1)
#define logerror(...)
#define popmessage(...)

#ifdef I386_BIOS_CALL
#define BIOS_INT(num) if(cpustate->bios != NULL) { \
	uint16 regs[8], sregs[4]; \
	regs[0] = REG16(AX); regs[1] = REG16(CX); regs[2] = REG16(DX); regs[3] = REG16(BX); \
	regs[4] = REG16(SP); regs[5] = REG16(BP); regs[6] = REG16(SI); regs[7] = REG16(DI); \
	sregs[0] = cpustate->sreg[ES].selector; sregs[1] = cpustate->sreg[CS].selector; \
	sregs[2] = cpustate->sreg[SS].selector; sregs[3] = cpustate->sreg[DS].selector; \
	int32 ZeroFlag = cpustate->ZF, CarryFlag = cpustate->CF; \
	if(cpustate->bios->bios_int(num, regs, sregs, &ZeroFlag, &CarryFlag)) { \
		REG16(AX) = regs[0]; REG16(CX) = regs[1]; REG16(DX) = regs[2]; REG16(BX) = regs[3]; \
		REG16(SP) = regs[4]; REG16(BP) = regs[5]; REG16(SI) = regs[6]; REG16(DI) = regs[7]; \
		cpustate->ZF = (UINT8)ZeroFlag; cpustate->CF = (UINT8)CarryFlag; \
		return; \
	} \
}
#define BIOS_CALL(address) if(cpustate->bios != NULL) { \
	uint16 regs[8], sregs[4]; \
	regs[0] = REG16(AX); regs[1] = REG16(CX); regs[2] = REG16(DX); regs[3] = REG16(BX); \
	regs[4] = REG16(SP); regs[5] = REG16(BP); regs[6] = REG16(SI); regs[7] = REG16(DI); \
	sregs[0] = cpustate->sreg[ES].selector; sregs[1] = cpustate->sreg[CS].selector; \
	sregs[2] = cpustate->sreg[SS].selector; sregs[3] = cpustate->sreg[DS].selector; \
	int32 ZeroFlag = cpustate->ZF, CarryFlag = cpustate->CF; \
	if(cpustate->bios->bios_call(address, regs, sregs, &ZeroFlag, &CarryFlag)) { \
		REG16(AX) = regs[0]; REG16(CX) = regs[1]; REG16(DX) = regs[2]; REG16(BX) = regs[3]; \
		REG16(SP) = regs[4]; REG16(BP) = regs[5]; REG16(SI) = regs[6]; REG16(DI) = regs[7]; \
		cpustate->ZF = (UINT8)ZeroFlag; cpustate->CF = (UINT8)CarryFlag; \
		return; \
	} \
}
#endif

#include "mame/softfloat/softfloat.c"
#include "mame/i386/i386.c"

void I386::initialize()
{
	opaque = CPU_INIT_CALL(CPU_MODEL);
	
	i386_state *cpustate = (i386_state *)opaque;
	cpustate->pic = d_pic;
	cpustate->program = d_mem;
	cpustate->io = d_io;
#ifdef I386_BIOS_CALL
	cpustate->bios = d_bios;
#endif
#ifdef SINGLE_MODE_DMA
	cpustate->dma = d_dma;
#endif
}

void I386::release()
{
	free(opaque);
}

void I386::reset()
{
	i386_state *cpustate = (i386_state *)opaque;
	CPU_RESET_CALL(CPU_MODEL);
}

int I386::run(int cycles)
{
	i386_state *cpustate = (i386_state *)opaque;
	return CPU_EXECUTE_CALL(i386);
}

void I386::write_signal(int id, uint32 data, uint32 mask)
{
	i386_state *cpustate = (i386_state *)opaque;
	
	if(id == SIG_CPU_NMI) {
		i386_set_irq_line(cpustate, INPUT_LINE_NMI, (data & mask) ? HOLD_LINE : CLEAR_LINE);
	}
	else if(id == SIG_CPU_IRQ) {
		i386_set_irq_line(cpustate, INPUT_LINE_IRQ, (data & mask) ? HOLD_LINE : CLEAR_LINE);
	}
	else if(id == SIG_CPU_BUSREQ) {
		cpustate->busreq = (data & mask) ? 1 : 0;
	}
	else if(id == SIG_I386_A20) {
		i386_set_a20_line(cpustate, data & mask);
	}
}

void I386::set_intr_line(bool line, bool pending, uint32 bit)
{
	i386_state *cpustate = (i386_state *)opaque;
	i386_set_irq_line(cpustate, INPUT_LINE_IRQ, line ? HOLD_LINE : CLEAR_LINE);
}

uint32 I386::get_pc()
{
	i386_state *cpustate = (i386_state *)opaque;
	return cpustate->prev_pc;
}
