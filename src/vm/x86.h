/*
	Skelton for retropc emulator

	Origin : MAME
	Author : Takeda.Toshiya
	Date   : 2007.08.11 -

	[ 80x86 ]
*/

#ifndef _X86_H_ 
#define _X86_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_X86_TEST	0x10
#define SIG_X86_A20	0x11

#define INT_REQ_BIT	1
#define NMI_REQ_BIT	2

// regs
#define AX	0
#define CX	1
#define DX	2
#define BX	3
#define SP	4
#define BP	5
#define SI	6
#define DI	7

#define AL	0
#define AH	1
#define CL	2
#define CH	3
#define DL	4
#define DH	5
#define BL	6
#define BH	7
#define SPL	8
#define SPH	9
#define BPL	10
#define BPH	11
#define SIL	12
#define SIH	13
#define DIL	14
#define DIH	15

// sregs
#define ES	0
#define CS	1
#define SS	2
#define DS	3

// address mask
#ifndef I286
#define AMASK	0xfffff
#endif

// segment base
#define SegBase(seg) (sregs[seg] << 4)
#define DefaultBase(seg) ((seg_prefix && (seg == DS || seg == SS)) ? prefix_base : base[seg])

static const uint8 parity_table[256] = {
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};
static const uint8 mod_reg8[256] = {
	AL, AL, AL, AL, AL, AL, AL, AL, CL, CL, CL, CL, CL, CL, CL, CL,
	DL, DL, DL, DL, DL, DL, DL, DL, BL, BL, BL, BL, BL, BL, BL, BL,
	AH, AH, AH, AH, AH, AH, AH, AH, CH, CH, CH, CH, CH, CH, CH, CH,
	DH, DH, DH, DH, DH, DH, DH, DH, BH, BH, BH, BH, BH, BH, BH, BH,
	AL, AL, AL, AL, AL, AL, AL, AL, CL, CL, CL, CL, CL, CL, CL, CL,
	DL, DL, DL, DL, DL, DL, DL, DL, BL, BL, BL, BL, BL, BL, BL, BL,
	AH, AH, AH, AH, AH, AH, AH, AH, CH, CH, CH, CH, CH, CH, CH, CH,
	DH, DH, DH, DH, DH, DH, DH, DH, BH, BH, BH, BH, BH, BH, BH, BH,
	AL, AL, AL, AL, AL, AL, AL, AL, CL, CL, CL, CL, CL, CL, CL, CL,
	DL, DL, DL, DL, DL, DL, DL, DL, BL, BL, BL, BL, BL, BL, BL, BL,
	AH, AH, AH, AH, AH, AH, AH, AH, CH, CH, CH, CH, CH, CH, CH, CH,
	DH, DH, DH, DH, DH, DH, DH, DH, BH, BH, BH, BH, BH, BH, BH, BH,
	AL, AL, AL, AL, AL, AL, AL, AL, CL, CL, CL, CL, CL, CL, CL, CL,
	DL, DL, DL, DL, DL, DL, DL, DL, BL, BL, BL, BL, BL, BL, BL, BL,
	AH, AH, AH, AH, AH, AH, AH, AH, CH, CH, CH, CH, CH, CH, CH, CH,
	DH, DH, DH, DH, DH, DH, DH, DH, BH, BH, BH, BH, BH, BH, BH, BH,
};
static const uint8 mod_reg16[256] = {
	AX, AX, AX, AX, AX, AX, AX, AX, CX, CX, CX, CX, CX, CX, CX, CX,
	DX, DX, DX, DX, DX, DX, DX, DX, BX, BX, BX, BX, BX, BX, BX, BX,
	SP, SP, SP, SP, SP, SP, SP, SP, BP, BP, BP, BP, BP, BP, BP, BP,
	SI, SI, SI, SI, SI, SI, SI, SI, DI, DI, DI, DI, DI, DI, DI, DI,
	AX, AX, AX, AX, AX, AX, AX, AX, CX, CX, CX, CX, CX, CX, CX, CX,
	DX, DX, DX, DX, DX, DX, DX, DX, BX, BX, BX, BX, BX, BX, BX, BX,
	SP, SP, SP, SP, SP, SP, SP, SP, BP, BP, BP, BP, BP, BP, BP, BP,
	SI, SI, SI, SI, SI, SI, SI, SI, DI, DI, DI, DI, DI, DI, DI, DI,
	AX, AX, AX, AX, AX, AX, AX, AX, CX, CX, CX, CX, CX, CX, CX, CX,
	DX, DX, DX, DX, DX, DX, DX, DX, BX, BX, BX, BX, BX, BX, BX, BX,
	SP, SP, SP, SP, SP, SP, SP, SP, BP, BP, BP, BP, BP, BP, BP, BP,
	SI, SI, SI, SI, SI, SI, SI, SI, DI, DI, DI, DI, DI, DI, DI, DI,
	AX, AX, AX, AX, AX, AX, AX, AX, CX, CX, CX, CX, CX, CX, CX, CX,
	DX, DX, DX, DX, DX, DX, DX, DX, BX, BX, BX, BX, BX, BX, BX, BX,
	SP, SP, SP, SP, SP, SP, SP, SP, BP, BP, BP, BP, BP, BP, BP, BP,
	SI, SI, SI, SI, SI, SI, SI, SI, DI, DI, DI, DI, DI, DI, DI, DI
};
static const uint8 mod_rm8[256] = {
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	AL, CL, DL, BL, AH, CH, DH, BH, AL, CL, DL, BL, AH, CH, DH, BH,
	AL, CL, DL, BL, AH, CH, DH, BH, AL, CL, DL, BL, AH, CH, DH, BH,
	AL, CL, DL, BL, AH, CH, DH, BH, AL, CL, DL, BL, AH, CH, DH, BH,
	AL, CL, DL, BL, AH, CH, DH, BH, AL, CL, DL, BL, AH, CH, DH, BH
};
static const uint8 mod_rm16[256] = {
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	AX, CX, DX, BX, SP, BP, SI, DI, AX, CX, DX, BX, SP, BP, SI, DI,
	AX, CX, DX, BX, SP, BP, SI, DI, AX, CX, DX, BX, SP, BP, SI, DI,
	AX, CX, DX, BX, SP, BP, SI, DI, AX, CX, DX, BX, SP, BP, SI, DI,
	AX, CX, DX, BX, SP, BP, SI, DI, AX, CX, DX, BX, SP, BP, SI, DI
};

// v30
#ifdef V30
static const uint16 bytes[] = {
	   1,    2,    4,    8,
	  16,   32,   64,  128,
	 256,  512, 1024, 2048,
	4096, 8192,16384,32768
};
#endif

// clocks
struct x86_cycles {
	uint8 exception, iret;					// exception, IRET
	uint8 int3, int_imm, into_nt, into_t;			// INTs
	uint8 override;						// segment overrides
	uint8 flag_ops, lahf, sahf;				// flag operations
	uint8 aaa, aas, aam, aad;				// arithmetic adjusts
	uint8 daa, das;						// decimal adjusts
	uint8 cbw, cwd;						// sign extension
	uint8 hlt, load_ptr, lea, nop, wait, xlat;		// misc
	uint8 jmp_short, jmp_near, jmp_far;			// direct JMPs
	uint8 jmp_r16, jmp_m16, jmp_m32;			// indirect JMPs
	uint8 call_near, call_far;				// direct CALLs
	uint8 call_r16, call_m16, call_m32;			// indirect CALLs
	uint8 ret_near, ret_far, ret_near_imm, ret_far_imm;	// returns
	uint8 jcc_nt, jcc_t, jcxz_nt, jcxz_t;			// conditional JMPs
	uint8 loop_nt, loop_t, loope_nt, loope_t;		// loops
	uint8 in_imm8, in_imm16, in_dx8, in_dx16;		// port reads
	uint8 out_imm8, out_imm16, out_dx8, out_dx16;		// port writes
	uint8 mov_rr8, mov_rm8, mov_mr8;			// move, 8-bit
	uint8 mov_ri8, mov_mi8;					// move, 8-bit immediate
	uint8 mov_rr16, mov_rm16, mov_mr16;			// move, 16-bit
	uint8 mov_ri16, mov_mi16;				// move, 16-bit immediate
	uint8 mov_am8, mov_am16, mov_ma8, mov_ma16;		// move, AL/AX memory
	uint8 mov_sr, mov_sm, mov_rs, mov_ms;			// move, segment registers
	uint8 xchg_rr8, xchg_rm8;				// exchange, 8-bit
	uint8 xchg_rr16, xchg_rm16, xchg_ar16;			// exchange, 16-bit
	uint8 push_r16, push_m16, push_seg, pushf;		// pushes
	uint8 pop_r16, pop_m16, pop_seg, popf;			// pops
	uint8 alu_rr8, alu_rm8, alu_mr8;			// ALU ops, 8-bit
	uint8 alu_ri8, alu_mi8, alu_mi8_ro;			// ALU ops, 8-bit immediate
	uint8 alu_rr16, alu_rm16, alu_mr16;			// ALU ops, 16-bit
	uint8 alu_ri16, alu_mi16, alu_mi16_ro;			// ALU ops, 16-bit immediate
	uint8 alu_r16i8, alu_m16i8, alu_m16i8_ro;		// ALU ops, 16-bit w/8-bit immediate
	uint8 mul_r8, mul_r16, mul_m8, mul_m16;			// MUL
	uint8 imul_r8, imul_r16, imul_m8, imul_m16;		// IMUL
	uint8 div_r8, div_r16, div_m8, div_m16;			// DIV
	uint8 idiv_r8, idiv_r16, idiv_m8, idiv_m16;		// IDIV
	uint8 incdec_r8, incdec_r16, incdec_m8, incdec_m16;	// INC/DEC
	uint8 negnot_r8, negnot_r16, negnot_m8, negnot_m16;	// NEG/NOT
	uint8 rot_reg_1, rot_reg_base, rot_reg_bit;		// reg shift/rotate
	uint8 rot_m8_1, rot_m8_base, rot_m8_bit;		// m8 shift/rotate
	uint8 rot_m16_1, rot_m16_base, rot_m16_bit;		// m16 shift/rotate
	uint8 cmps8, rep_cmps8_base, rep_cmps8_count;		// CMPS 8-bit
	uint8 cmps16, rep_cmps16_base, rep_cmps16_count;	// CMPS 16-bit
	uint8 scas8, rep_scas8_base, rep_scas8_count;		// SCAS 8-bit
	uint8 scas16, rep_scas16_base, rep_scas16_count;	// SCAS 16-bit
	uint8 lods8, rep_lods8_base, rep_lods8_count;		// LODS 8-bit
	uint8 lods16, rep_lods16_base, rep_lods16_count;	// LODS 16-bit
	uint8 stos8, rep_stos8_base, rep_stos8_count;		// STOS 8-bit
	uint8 stos16, rep_stos16_base, rep_stos16_count;	// STOS 16-bit
	uint8 movs8, rep_movs8_base, rep_movs8_count;		// MOVS 8-bit
	uint8 movs16, rep_movs16_base, rep_movs16_count;	// MOVS 16-bit
	uint8 ins8, rep_ins8_base, rep_ins8_count;		// (80186) INS 8-bit
	uint8 ins16, rep_ins16_base, rep_ins16_count;		// (80186) INS 16-bit
	uint8 outs8, rep_outs8_base, rep_outs8_count;		// (80186) OUTS 8-bit
	uint8 outs16, rep_outs16_base, rep_outs16_count;	// (80186) OUTS 16-bit
	uint8 push_imm, pusha, popa;				// (80186) PUSH immediate, PUSHA/POPA
	uint8 imul_rri8, imul_rmi8;				// (80186) IMUL immediate 8-bit
	uint8 imul_rri16, imul_rmi16;				// (80186) IMUL immediate 16-bit
	uint8 enter0, enter1, enter_base, enter_count, leave;	// (80186) ENTER/LEAVE
	uint8 bound;						// (80186) BOUND
};

#ifdef I286
// for 80286
static const struct x86_cycles cycles = {
	23,17,			// exception, IRET
	 0, 2, 3, 1,		// INTs
	 2,			// segment overrides
	 2, 2, 2,		// flag operations
	 3, 3,16,14,		// arithmetic adjusts
	 3, 3,			// decimal adjusts
	 2, 2,			// sign extension
	 2, 7, 3, 3, 3, 5,	// misc
	 7, 7,11,		// direct JMPs
	 7,11,26,		// indirect JMPs
	 7,13,			// direct CALLs
	 7,11,29,		// indirect CALLs
	11,15,11,15,		// returns
	 3, 7, 4, 8,		// conditional JMPs
	 4, 8, 4, 8,		// loops
	 5, 5, 5, 5,		// port reads
	 3, 3, 3, 3,		// port writes
	 2, 3, 3,		// move, 8-bit
	 2, 3,			// move, 8-bit immediate
	 2, 3, 3,		// move, 16-bit
	 2, 3,			// move, 16-bit immediate
	 5, 5, 3, 3,		// move, AL/AX memory
	 2, 5, 2, 3,		// move, segment registers
	 3, 5,			// exchange, 8-bit
	 3, 5, 3,		// exchange, 16-bit
	 5, 5, 3, 3,		// pushes
	 5, 5, 5, 5,		// pops
	 2, 7, 7,		// ALU ops, 8-bit
	 3, 7, 7,		// ALU ops, 8-bit immediate
	 2, 7, 7,		// ALU ops, 16-bit
	 3, 7, 7,		// ALU ops, 16-bit immediate
	 3, 7, 7,		// ALU ops, 16-bit w/8-bit immediate
	13,21,16,24,		// MUL
	13,21,16,24,		// IMUL
	14,22,17,25,		// DIV
	17,25,20,28,		// IDIV
	 2, 2, 7, 7,		// INC/DEC
	 2, 2, 7, 7,		// NEG/NOT
	 2, 5, 0,		// reg shift/rotate
	 7, 8, 1,		// m8 shift/rotate
	 7, 8, 1,		// m16 shift/rotate
	13, 5,12,		// CMPS 8-bit
	13, 5,12,		// CMPS 16-bit
	 9, 5, 8,		// SCAS 8-bit
	 9, 5, 8,		// SCAS 16-bit
	 5, 5, 4,		// LODS 8-bit
	 5, 5, 4,		// LODS 16-bit
	 4, 4, 3,		// STOS 8-bit
	 4, 4, 3,		// STOS 16-bit
	 5, 5, 4,		// MOVS 8-bit
	 5, 5, 4,		// MOVS 16-bit
	 5, 5, 4,		// (80186) INS 8-bit
	 5, 5, 4,		// (80186) INS 16-bit
	 5, 5, 4,		// (80186) OUTS 8-bit
	 5, 5, 4,		// (80186) OUTS 16-bit
	 3,17,19,		// (80186) PUSH immediate, PUSHA/POPA
	21,24,			// (80186) IMUL immediate 8-bit
	21,24,			// (80186) IMUL immediate 16-bit
	11,15,12, 4, 5,		// (80186) ENTER/LEAVE
	13,			// (80186) BOUND
};
#else
// for 8086, v30
static const struct x86_cycles cycles = {
	51,32,			// exception, IRET
	 2, 0, 4, 2,		// INTs
	 2,			// segment overrides
	 2, 4, 4,		// flag operations
	 4, 4,83,60,		// arithmetic adjusts
	 4, 4,			// decimal adjusts
	 2, 5,			// sign extension
	 2,24, 2, 2, 3,11,	// misc
	15,15,15,		// direct JMPs
	11,18,24,		// indirect JMPs
	19,28,			// direct CALLs
	16,21,37,		// indirect CALLs
	20,32,24,31,		// returns
	 4,16, 6,18,		// conditional JMPs
	 5,17, 6,18,		// loops
	10,14, 8,12,		// port reads
	10,14, 8,12,		// port writes
	 2, 8, 9,		// move, 8-bit
	 4,10,			// move, 8-bit immediate
	 2, 8, 9,		// move, 16-bit
	 4,10,			// move, 16-bit immediate
	10,10,10,10,		// move, AL/AX memory
	 2, 8, 2, 9,		// move, segment registers
	 4,17,			// exchange, 8-bit
	 4,17, 3,		// exchange, 16-bit
	15,24,14,14,		// pushes
	12,25,12,12,		// pops
	 3, 9,16,		// ALU ops, 8-bit
	 4,17,10,		// ALU ops, 8-bit immediate
	 3, 9,16,		// ALU ops, 16-bit
	 4,17,10,		// ALU ops, 16-bit immediate
	 4,17,10,		// ALU ops, 16-bit w/8-bit immediate
	70,118,76,128,		// MUL
	80,128,86,138,		// IMUL
	80,144,86,154,		// DIV
	101,165,107,175,	// IDIV
	 3, 2,15,15,		// INC/DEC
	 3, 3,16,16,		// NEG/NOT
	 2, 8, 4,		// reg shift/rotate
	15,20, 4,		// m8 shift/rotate
	15,20, 4,		// m16 shift/rotate
	22, 9,21,		// CMPS 8-bit
	22, 9,21,		// CMPS 16-bit
	15, 9,14,		// SCAS 8-bit
	15, 9,14,		// SCAS 16-bit
	12, 9,11,		// LODS 8-bit
	12, 9,11,		// LODS 16-bit
	11, 9,10,		// STOS 8-bit
	11, 9,10,		// STOS 16-bit
	18, 9,17,		// MOVS 8-bit
	18, 9,17,		// MOVS 16-bit
};
#endif

class X86 : public DEVICE
{
private:
	/* ---------------------------------------------------------------------------
	contexts
	--------------------------------------------------------------------------- */
	
	DEVICE *d_mem, *d_io, *d_pic, *d_bios;
	
	/* ---------------------------------------------------------------------------
	registers
	--------------------------------------------------------------------------- */
	
	// clocks
	int count, extra_count, first;
	bool busreq, halt;
	
	// regs
	union REGTYPE {
		uint8 b[16];
		uint16 w[8];
	} regs;
	uint16 sregs[4], limit[4];
	uint32 base[4];
	unsigned EA;
	uint16 EO;
	uint32 gdtr_base, idtr_base, ldtr_base, tr_base;
	uint16 gdtr_limit, idtr_limit, ldtr_limit, tr_limit;
	uint16 ldtr_sel, tr_sel;
	
	// flags
	uint16 flags, msw;
	int32 AuxVal, OverVal, SignVal, ZeroVal, CarryVal, DirVal;
	uint8 ParityVal, TF, IF, MF;
	int intstat, busy;
	
	// addr
	uint32 PC, prvPC;
#ifdef I286
	uint32 AMASK;
#endif
	
	// prefix
	unsigned prefix_base;
	bool seg_prefix;
	
	/* ---------------------------------------------------------------------------
	virtual machine interfaces
	--------------------------------------------------------------------------- */
	
	// memory
	inline uint8 RM8(uint32 seg, uint32 ofs) {
		return d_mem->read_data8((DefaultBase(seg) + ofs) & AMASK);
	}
	inline uint16 RM16(uint32 seg, uint32 ofs) {
		return d_mem->read_data16((DefaultBase(seg) + ofs) & AMASK);
	}
	inline void WM8(uint32 seg, uint32 ofs, uint8 val) {
		d_mem->write_data8((DefaultBase(seg) + ofs) & AMASK, val);
	}
	inline void WM16(uint32 seg, uint32 ofs, uint16 val) {
		d_mem->write_data16((DefaultBase(seg) + ofs) & AMASK, val);
	}
	inline uint8 RM8(uint32 addr) {
		return d_mem->read_data8(addr & AMASK);
	}
	inline uint16 RM16(uint32 addr) {
		return d_mem->read_data16(addr & AMASK);
	}
	inline void WM8(uint32 addr, uint8 val) {
		d_mem->write_data8(addr & AMASK, val);
	}
	inline void WM16(uint32 addr, uint16 val) {
		d_mem->write_data16(addr & AMASK, val);
	}
	inline uint8 FETCHOP() {
		return d_mem->read_data8(PC++);
	}
	inline uint8 FETCH8() {
		return d_mem->read_data8(PC++);
	}
	inline uint16 FETCH16() {
		uint16 val = d_mem->read_data16(PC);
		PC += 2;
		return val;
	}
	inline void PUSH16(uint16 val) {
		regs.w[SP] -= 2;
		WM16((base[SS] + regs.w[SP]) & AMASK, val);
	}
	inline uint16 POP16() {
		uint16 var = RM16((base[SS] + regs.w[SP]) & AMASK);
		regs.w[SP] += 2;
		return var;
	}
	
	// i/o
	inline uint8 IN8(uint32 addr) {
		return d_io->read_io8(addr);
	}
	inline void OUT8(uint32 addr, uint8 val) {
		d_io->write_io8(addr, val);
	}
	
	// interrupt
	inline uint32 ACK_INTR() {
		return d_pic->intr_ack();
	}
	
	/* ---------------------------------------------------------------------------
	opecode
	--------------------------------------------------------------------------- */
	
	// sub
	void interrupt(unsigned num);
	unsigned GetEA(unsigned ModRM);
	void rotate_shift_byte(unsigned ModRM, unsigned cnt);
	void rotate_shift_word(unsigned ModRM, unsigned cnt);
#ifdef I286
	int i286_selector_okay(uint16 selector);
	void i286_data_descriptor(int reg, uint16 selector);
	void i286_code_descriptor(uint16 selector, uint16 offset);
#endif
	
	// opecode
	void op(uint8 code);
	inline void _add_br8();
	inline void _add_wr16();
	inline void _add_r8b();
	inline void _add_r16w();
	inline void _add_ald8();
	inline void _add_axd16();
	inline void _push_es();
	inline void _pop_es();
	inline void _or_br8();
	inline void _or_wr16();
	inline void _or_r8b();
	inline void _or_r16w();
	inline void _or_ald8();
	inline void _or_axd16();
	inline void _push_cs();
	inline void _op0f();
	inline void _adc_br8();
	inline void _adc_wr16();
	inline void _adc_r8b();
	inline void _adc_r16w();
	inline void _adc_ald8();
	inline void _adc_axd16();
	inline void _push_ss();
	inline void _pop_ss();
	inline void _sbb_br8();
	inline void _sbb_wr16();
	inline void _sbb_r8b();
	inline void _sbb_r16w();
	inline void _sbb_ald8();
	inline void _sbb_axd16();
	inline void _push_ds();
	inline void _pop_ds();
	inline void _and_br8();
	inline void _and_wr16();
	inline void _and_r8b();
	inline void _and_r16w();
	inline void _and_ald8();
	inline void _and_axd16();
	inline void _es();
	inline void _daa();
	inline void _sub_br8();
	inline void _sub_wr16();
	inline void _sub_r8b();
	inline void _sub_r16w();
	inline void _sub_ald8();
	inline void _sub_axd16();
	inline void _cs();
	inline void _das();
	inline void _xor_br8();
	inline void _xor_wr16();
	inline void _xor_r8b();
	inline void _xor_r16w();
	inline void _xor_ald8();
	inline void _xor_axd16();
	inline void _ss();
	inline void _aaa();
	inline void _cmp_br8();
	inline void _cmp_wr16();
	inline void _cmp_r8b();
	inline void _cmp_r16w();
	inline void _cmp_ald8();
	inline void _cmp_axd16();
	inline void _ds();
	inline void _aas();
	inline void _inc_ax();
	inline void _inc_cx();
	inline void _inc_dx();
	inline void _inc_bx();
	inline void _inc_sp();
	inline void _inc_bp();
	inline void _inc_si();
	inline void _inc_di();
	inline void _dec_ax();
	inline void _dec_cx();
	inline void _dec_dx();
	inline void _dec_bx();
	inline void _dec_sp();
	inline void _dec_bp();
	inline void _dec_si();
	inline void _dec_di();
	inline void _push_ax();
	inline void _push_cx();
	inline void _push_dx();
	inline void _push_bx();
	inline void _push_sp();
	inline void _push_bp();
	inline void _push_si();
	inline void _push_di();
	inline void _pop_ax();
	inline void _pop_cx();
	inline void _pop_dx();
	inline void _pop_bx();
	inline void _pop_sp();
	inline void _pop_bp();
	inline void _pop_si();
	inline void _pop_di();
	inline void _pusha();
	inline void _popa();
	inline void _bound();
	inline void _arpl();
	inline void _repc(int flagval);
	inline void _push_d16();
	inline void _imul_d16();
	inline void _push_d8();
	inline void _imul_d8();
	inline void _insb();
	inline void _insw();
	inline void _outsb();
	inline void _outsw();
	inline void _jo();
	inline void _jno();
	inline void _jb();
	inline void _jnb();
	inline void _jz();
	inline void _jnz();
	inline void _jbe();
	inline void _jnbe();
	inline void _js();
	inline void _jns();
	inline void _jp();
	inline void _jnp();
	inline void _jl();
	inline void _jnl();
	inline void _jle();
	inline void _jnle();
	inline void _op80();
	inline void _op81();
	inline void _op82();
	inline void _op83();
	inline void _test_br8();
	inline void _test_wr16();
	inline void _xchg_br8();
	inline void _xchg_wr16();
	inline void _mov_br8();
	inline void _mov_wr16();
	inline void _mov_r8b();
	inline void _mov_r16w();
	inline void _mov_wsreg();
	inline void _lea();
	inline void _mov_sregw();
	inline void _popw();
	inline void _nop();
	inline void _xchg_axcx();
	inline void _xchg_axdx();
	inline void _xchg_axbx();
	inline void _xchg_axsp();
	inline void _xchg_axbp();
	inline void _xchg_axsi();
	inline void _xchg_axdi();
	inline void _cbw();
	inline void _cwd();
	inline void _call_far();
	inline void _wait();
	inline void _pushf();
	inline void _popf();
	inline void _sahf();
	inline void _lahf();
	inline void _mov_aldisp();
	inline void _mov_axdisp();
	inline void _mov_dispal();
	inline void _mov_dispax();
	inline void _movsb();
	inline void _movsw();
	inline void _cmpsb();
	inline void _cmpsw();
	inline void _test_ald8();
	inline void _test_axd16();
	inline void _stosb();
	inline void _stosw();
	inline void _lodsb();
	inline void _lodsw();
	inline void _scasb();
	inline void _scasw();
	inline void _mov_ald8();
	inline void _mov_cld8();
	inline void _mov_dld8();
	inline void _mov_bld8();
	inline void _mov_ahd8();
	inline void _mov_chd8();
	inline void _mov_dhd8();
	inline void _mov_bhd8();
	inline void _mov_axd16();
	inline void _mov_cxd16();
	inline void _mov_dxd16();
	inline void _mov_bxd16();
	inline void _mov_spd16();
	inline void _mov_bpd16();
	inline void _mov_sid16();
	inline void _mov_did16();
	inline void _rotshft_bd8();
	inline void _rotshft_wd8();
	inline void _ret_d16();
	inline void _ret();
	inline void _les_dw();
	inline void _lds_dw();
	inline void _mov_bd8();
	inline void _mov_wd16();
	inline void _enter();
	inline void _leav();	// _leave()
	inline void _retf_d16();
	inline void _retf();
	inline void _int3();
	inline void _int();
	inline void _into();
	inline void _iret();
	inline void _rotshft_b();
	inline void _rotshft_w();
	inline void _rotshft_bcl();
	inline void _rotshft_wcl();
	inline void _aam();
	inline void _aad();
	inline void _setalc();
	inline void _xlat();
	inline void _escape();
	inline void _loopne();
	inline void _loope();
	inline void _loop();
	inline void _jcxz();
	inline void _inal();
	inline void _inax();
	inline void _outal();
	inline void _outax();
	inline void _call_d16();
	inline void _jmp_d16();
	inline void _jmp_far();
	inline void _jmp_d8();
	inline void _inaldx();
	inline void _inaxdx();
	inline void _outdxal();
	inline void _outdxax();
	inline void _lock();
	inline void _rep(int flagval);
	inline void _hlt();
	inline void _cmc();
	inline void _opf6();
	inline void _opf7();
	inline void _clc();
	inline void _stc();
	inline void _cli();
	inline void _sti();
	inline void _cld();
	inline void _std();
	inline void _opfe();
	inline void _opff();
	inline void _invalid();
	
public:
	X86(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		d_bios = NULL;
		count = extra_count = first = 0;	// passed_clock must be zero at initialize
		busreq = false;
	}
	~X86() {}
	
	// common functions
	void initialize();
	void reset();
	void run(int clock);
	void write_signal(int id, uint32 data, uint32 mask);
	void set_intr_line(bool line, bool pending, uint32 bit) {
		if(line)
			intstat |= INT_REQ_BIT;
		else
			intstat &= ~INT_REQ_BIT;
	}
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
	void set_context_io(DEVICE* device) {
		d_io = device;
	}
	void set_context_intr(DEVICE* device) {
		d_pic = device;
	}
	void set_context_bios(DEVICE* device) {
		d_bios = device;
	}
};

#endif
