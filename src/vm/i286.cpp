/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date  : 2012.10.18-

	[ i286 ]
*/

#include "i286.h"

/* ----------------------------------------------------------------------------
	MAME i286
---------------------------------------------------------------------------- */

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#pragma warning( disable : 4018 )
#pragma warning( disable : 4146 )
#pragma warning( disable : 4244 )
#endif

//#if defined(HAS_I86)
//	#define CPU_MODEL i8086
//#elif defined(HAS_I88)
//	#define CPU_MODEL i8088
//#elif defined(HAS_I186)
//	#define CPU_MODEL i80186
//#elif defined(HAS_I286)
	#define CPU_MODEL i80286
//#endif

#ifndef INLINE
#define INLINE inline
#endif

#ifndef _BIG_ENDIAN
#define LSB_FIRST
#endif

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

// Disassembler constants
const UINT32 DASMFLAG_SUPPORTED     = 0x80000000;   // are disassembly flags supported?
const UINT32 DASMFLAG_STEP_OUT      = 0x40000000;   // this instruction should be the end of a step out sequence
const UINT32 DASMFLAG_STEP_OVER     = 0x20000000;   // this instruction should be stepped over by setting a breakpoint afterwards
const UINT32 DASMFLAG_OVERINSTMASK  = 0x18000000;   // number of extra instructions to skip when stepping over
const UINT32 DASMFLAG_OVERINSTSHIFT = 27;           // bits to shift after masking to get the value
const UINT32 DASMFLAG_LENGTHMASK    = 0x0000ffff;   // the low 16-bits contain the actual length

/* Highly useful macro for compile-time knowledge of an array size */
#define ARRAY_LENGTH(x)     (sizeof(x) / sizeof(x[0]))

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
#define CPU_RESET(name)				void CPU_RESET_NAME(name)(cpu_state *cpustate)
#define CPU_RESET_CALL(name)			CPU_RESET_NAME(name)(cpustate)

#define CPU_EXECUTE_NAME(name)			cpu_execute_##name
#define CPU_EXECUTE(name)			int CPU_EXECUTE_NAME(name)(cpu_state *cpustate, int icount)
#define CPU_EXECUTE_CALL(name)			CPU_EXECUTE_NAME(name)(cpustate, icount)

#define CPU_DISASSEMBLE_NAME(name)		cpu_disassemble_##name
#define CPU_DISASSEMBLE(name)			int CPU_DISASSEMBLE_NAME(name)(char *buffer, offs_t eip, const UINT8 *oprom)
#define CPU_DISASSEMBLE_CALL(name)		CPU_DISASSEMBLE_NAME(name)(buffer, eip, oprom)

#define logerror(...)

//#if defined(HAS_I86) || defined(HAS_I88) || defined(HAS_I186)
//	#define cpu_state i8086_state
//	#include "mame/i86/i86.c"
//#elif defined(HAS_I286)
	#define cpu_state i80286_state
	#include "mame/i86/i286.c"
//#endif
#ifdef _CPU_DEBUG_LOG
#include "mame/i386/i386dasm.c"
#endif

void I286::initialize()
{
	opaque = CPU_INIT_CALL(CPU_MODEL);
	
	cpu_state *cpustate = (cpu_state *)opaque;
	cpustate->pic = d_pic;
	cpustate->program = d_mem;
	cpustate->io = d_io;
#ifdef I86_BIOS_CALL
	cpustate->bios = d_bios;
#endif
#ifdef SINGLE_MODE_DMA
	cpustate->dma = d_dma;
#endif
}

void I286::release()
{
	free(opaque);
}

void I286::reset()
{
	cpu_state *cpustate = (cpu_state *)opaque;
	int busreq = cpustate->busreq;
	
	CPU_RESET_CALL(CPU_MODEL);
	
	cpustate->pic = d_pic;
	cpustate->program = d_mem;
	cpustate->io = d_io;
#ifdef I86_BIOS_CALL
	cpustate->bios = d_bios;
#endif
#ifdef SINGLE_MODE_DMA
	cpustate->dma = d_dma;
#endif
	cpustate->busreq = busreq;
#ifdef _CPU_DEBUG_LOG
	debug_count = 0;
#endif
}

int I286::run(int icount)
{
	cpu_state *cpustate = (cpu_state *)opaque;
#ifdef _CPU_DEBUG_LOG
	if(debug_count) {
		char buffer[256];
		UINT64 eip = cpustate->pc - cpustate->sregs[CS];
		UINT8 ops[16];
		for(int i = 0; i < 16; i++) {
			ops[i] = d_mem->read_data8(cpustate->pc + i);
		}
		UINT8 *oprom = ops;
		
		CPU_DISASSEMBLE_CALL(x86_16);
		emu->out_debug(_T("%4x\t%s\n"), cpustate->pc, buffer);
		
		if(--debug_count == 0) {
			emu->out_debug(_T("<--------------------------------------------------------------- I286 DASM ----\n"));
		}
	}
#endif
	return CPU_EXECUTE_CALL(CPU_MODEL);
}

void I286::write_signal(int id, uint32 data, uint32 mask)
{
	cpu_state *cpustate = (cpu_state *)opaque;
	
	if(id == SIG_CPU_NMI) {
		set_irq_line(cpustate, INPUT_LINE_NMI, (data & mask) ? HOLD_LINE : CLEAR_LINE);
	}
	else if(id == SIG_CPU_IRQ) {
		set_irq_line(cpustate, INPUT_LINE_IRQ, (data & mask) ? HOLD_LINE : CLEAR_LINE);
	}
	else if(id == SIG_CPU_BUSREQ) {
		cpustate->busreq = (data & mask) ? 1 : 0;
	}
	else if(id == SIG_I86_TEST) {
		cpustate->test_state = (data & mask) ? 1 : 0;
	}
#ifdef HAS_I286
	else if(id == SIG_I86_A20) {
		i80286_set_a20_line(cpustate, data & mask);
	}
#endif
#ifdef _CPU_DEBUG_LOG
	else if(id == SIG_CPU_DEBUG) {
		if(debug_count == 0) {
			emu->out_debug(_T("---- I286 DASM --------------------------------------------------------------->\n"));
		}
		debug_count = 16;
	}
#endif
}

void I286::set_intr_line(bool line, bool pending, uint32 bit)
{
	cpu_state *cpustate = (cpu_state *)opaque;
	set_irq_line(cpustate, INPUT_LINE_IRQ, line ? HOLD_LINE : CLEAR_LINE);
}

void I286::set_extra_clock(int icount)
{
	cpu_state *cpustate = (cpu_state *)opaque;
	cpustate->extra_cycles += icount;
}

int I286::get_extra_clock()
{
	cpu_state *cpustate = (cpu_state *)opaque;
	return cpustate->extra_cycles;
}

uint32 I286::get_pc()
{
	cpu_state *cpustate = (cpu_state *)opaque;
	return cpustate->prevpc;
}

void I286::set_address_mask(uint32 mask)
{
	cpu_state *cpustate = (cpu_state *)opaque;
	cpustate->amask = mask;
}

uint32 I286::get_address_mask()
{
	cpu_state *cpustate = (cpu_state *)opaque;
	return cpustate->amask;
}

void I286::set_shutdown_flag(int shutdown)
{
	cpu_state *cpustate = (cpu_state *)opaque;
	cpustate->shutdown = shutdown;
}

int I286::get_shutdown_flag()
{
	cpu_state *cpustate = (cpu_state *)opaque;
	return cpustate->shutdown;
}
