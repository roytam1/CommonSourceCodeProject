/*
	Skelton for retropc emulator

	Origin : MAME i386 core
	Author : Takeda.Toshiya
	Date  : 2009.06.08-

	[ i386/i486/Pentium/MediaGX ]
*/

#ifndef _I386_H_ 
#define _I386_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_I386_A20	1

class I386 : public DEVICE
{
private:
	/* ---------------------------------------------------------------------------
	contexts
	--------------------------------------------------------------------------- */
	
	DEVICE *d_mem, *d_io, *d_pic, *d_bios;
#ifdef SINGLE_MODE_DMA
	DEVICE *d_dma;
#endif
	
	/* ---------------------------------------------------------------------------
	structs
	--------------------------------------------------------------------------- */
	
	typedef struct {
		uint16 selector;
		uint16 flags;
		uint32 base;
		uint32 limit;
		int d;	// operand size
	} sreg_c;
	
	typedef struct {
		uint32 base;
		uint16 limit;
	} sys_table_c;
	
	typedef struct {
		uint16 segment;
		uint16 flags;
		uint32 base;
		uint32 limit;
	} seg_desc_c;
	
	typedef union {
		uint32 d[8];
		uint16 w[16];
		uint8 b[32];
	} gpr_c;
	
	typedef union {
		uint64 i;
		double f;
	} x87reg_c;
	
	typedef struct {
		x87reg_c reg[8];
		uint16 control_word;
		uint16 status_word;
		uint16 tag_word;
		uint64 data_ptr;
		uint64 inst_ptr;
		uint16 opcode;
		int top;
	} fpu_c;
	
	typedef struct {
		struct {
			int b;
			int w;
			int d;
		} reg;
		struct {
			int b;
			int w;
			int d;
		} rm;
	} modrm_c;
	
	/* ---------------------------------------------------------------------------
	registers
	--------------------------------------------------------------------------- */
	
	gpr_c reg;
	sreg_c sreg[6];
	fpu_c fpu;
	uint32 eip, pc;
	uint32 prev_eip, prev_pc;
	uint16 prev_cs;
	uint8 prev_cpl;
	uint32 eflags;
//	uint8 CF, DF, SF, OF, ZF, PF, AF, IF, TF;
	int32 CF, DF, SF, OF, ZF, PF, AF, IF, TF;
	uint8 performed_intersegment_jump;
	
	uint32 cr[4];		// control registers
	uint32 dr[8];		// debug registers
	uint32 tr[8];		// test registers
	sys_table_c gdtr;	// global descriptor table register
	sys_table_c idtr;	// interrupt descriptor table register
	seg_desc_c task;	// task register
	seg_desc_c ldtr;	// local descriptor table register
	
	int operand_size, address_size;
	int segment_prefix, segment_override;
	int cycles, base_cycles;
	uint8 opcode;
	int halted, busreq, irq_state;
	uint32 a20_mask;
	uint64 tsc;
	
	int parity_table[256];
	modrm_c modrm_table[256];
	
	/* ---------------------------------------------------------------------------
	opecode
	--------------------------------------------------------------------------- */
	
	// sub
	void load_protected_mode_segment(sreg_c *seg);
	void load_segment_descriptor(int segment);
	uint32 get_flags();
	void set_flags(uint32 f);
	void sib_byte(uint8 mod, uint32* out_ea, uint8* out_segment);
	void modrm_to_EA(uint8 mod_rm, uint32* out_ea, uint8* out_segment);
	uint32 GetNonTranslatedEA(uint8 modrm);
	uint32 GetEA(uint8 modrm);
	uint32 translate(int segment, uint32 ip);
	void CHANGE_PC(uint32 pc);
	void NEAR_BRANCH(int32 offs);
	void trap(int irq, int irq_gate);
	void CYCLES(int x);
	void CYCLES_RM(int modrm, int r, int m);
	
	// opecodes
	void run_one_opecode();
	void decode_opcode();
	uint8 OR8(uint8 dst, uint8 src);
	uint16 OR16(uint16 dst, uint16 src);
	uint32 OR32(uint32 dst, uint32 src);
	uint8 AND8(uint8 dst, uint8 src);
	uint16 AND16(uint16 dst, uint16 src);
	uint32 AND32(uint32 dst, uint32 src);
	uint8 XOR8(uint8 dst, uint8 src);
	uint16 XOR16(uint16 dst, uint16 src);
	uint32 XOR32(uint32 dst, uint32 src);
	uint8 SUB8(uint8 dst, uint8 src);
	uint16 SUB16(uint16 dst, uint16 src);
	uint32 SUB32(uint32 dst, uint32 src);
	uint8 ADD8(uint8 dst, uint8 src);
	uint16 ADD16(uint16 dst, uint16 src);
	uint32 ADD32(uint32 dst, uint32 src);
	uint8 INC8(uint8 dst);
	uint16 INC16(uint16 dst);
	uint32 INC32(uint32 dst);
	uint8 DEC8(uint8 dst);
	uint16 DEC16(uint16 dst);
	uint32 DEC32(uint32 dst);
	void PUSH8(uint8 val);
	void PUSH16(uint16 val);
	void PUSH32(uint32 val);
	uint8 POP8();
	uint16 POP16();
	uint32 POP32();
	void BUMP_SI(int adjustment);
	void BUMP_DI(int adjustment);
	uint8 shift_rotate8(uint8 modrm, uint32 val, uint8 shift);
	void adc_rm8_r8();
	void adc_r8_rm8();
	void adc_al_i8();
	void add_rm8_r8();
	void add_r8_rm8();
	void add_al_i8();
	void and_rm8_r8();
	void and_r8_rm8();
	void and_al_i8();
	void clc();
	void cld();
	void cli();
	void cmc();
	void cmp_rm8_r8();
	void cmp_r8_rm8();
	void cmp_al_i8();
	void cmpsb();
	void in_al_i8();
	void in_al_dx();
	void ja_rel8();
	void jbe_rel8();
	void jc_rel8();
	void jg_rel8();
	void jge_rel8();
	void jl_rel8();
	void jle_rel8();
	void jnc_rel8();
	void jno_rel8();
	void jnp_rel8();
	void jns_rel8();
	void jnz_rel8();
	void jo_rel8();
	void jp_rel8();
	void js_rel8();
	void jz_rel8();
	void jmp_rel8();
	void lahf();
	void lodsb();
	void mov_rm8_r8();
	void mov_r8_rm8();
	void mov_rm8_i8();
	void mov_r32_cr();
	void mov_r32_dr();
	void mov_cr_r32();
	void mov_dr_r32();
	void mov_al_m8();
	void mov_m8_al();
	void mov_rm16_sreg();
	void mov_sreg_rm16();
	void mov_al_i8();
	void mov_cl_i8();
	void mov_dl_i8();
	void mov_bl_i8();
	void mov_ah_i8();
	void mov_ch_i8();
	void mov_dh_i8();
	void mov_bh_i8();
	void movsb();
	void or_rm8_r8();
	void or_r8_rm8();
	void or_al_i8();
	void out_al_i8();
	void out_al_dx();
	void push_i8();
	void ins_generic(int size);
	void insb();
	void insw();
	void insd();
	void outs_generic(int size);
	void outsb();
	void outsw();
	void outsd();
	void repeat(int invert_flag);
	void rep();
	void repne();
	void sahf();
	void sbb_rm8_r8();
	void sbb_r8_rm8();
	void sbb_al_i8();
	void scasb();
	void setalc();
	void seta_rm8();
	void setbe_rm8();
	void setc_rm8();
	void setg_rm8();
	void setge_rm8();
	void setl_rm8();
	void setle_rm8();
	void setnc_rm8();
	void setno_rm8();
	void setnp_rm8();
	void setns_rm8();
	void setnz_rm8();
	void seto_rm8();
	void setp_rm8();
	void sets_rm8();
	void setz_rm8();
	void stc();
	void std();
	void sti();
	void stosb();
	void sub_rm8_r8();
	void sub_r8_rm8();
	void sub_al_i8();
	void test_al_i8();
	void test_rm8_r8();
	void xchg_r8_rm8();
	void xor_rm8_r8();
	void xor_r8_rm8();
	void xor_al_i8();
	void group80_8();
	void groupC0_8();
	void groupD0_8();
	void groupD2_8();
	void groupF6_8();
	void groupFE_8();
	void segment_CS();
	void segment_DS();
	void segment_ES();
	void segment_FS();
	void segment_GS();
	void segment_SS();
	void opsiz();
	void adrsiz();
	void nop();
	void int3();
	void intr();
	void into();
	void escape();
	void hlt();
	void decimal_adjust(int direction);
	void daa();
	void das();
	void aaa();
	void aas();
	void aad();
	void aam();
	void clts();
	void wait();
	void lock();
	void mov_r32_tr();
	void mov_tr_r32();
	void unimplemented();
	void invalid();
	uint16 shift_rotate16(uint8 modrm, uint32 val, uint8 shift);
	void adc_rm16_r16();
	void adc_r16_rm16();
	void adc_ax_i16();
	void add_rm16_r16();
	void add_r16_rm16();
	void add_ax_i16();
	void and_rm16_r16();
	void and_r16_rm16();
	void and_ax_i16();
	void bsf_r16_rm16();
	void bsr_r16_rm16();
	void bt_rm16_r16();
	void btc_rm16_r16();
	void btr_rm16_r16();
	void bts_rm16_r16();
	void call_abs16();
	void call_rel16();
	void cbw();
	void cmp_rm16_r16();
	void cmp_r16_rm16();
	void cmp_ax_i16();
	void cmpsw();
	void cwd();
	void dec_ax();
	void dec_cx();
	void dec_dx();
	void dec_bx();
	void dec_sp();
	void dec_bp();
	void dec_si();
	void dec_di();
	void imul_r16_rm16();
	void imul_r16_rm16_i16();
	void imul_r16_rm16_i8();
	void in_ax_i8();
	void in_ax_dx();
	void inc_ax();
	void inc_cx();
	void inc_dx();
	void inc_bx();
	void inc_sp();
	void inc_bp();
	void inc_si();
	void inc_di();
	void iret16();
	void ja_rel16();
	void jbe_rel16();
	void jc_rel16();
	void jg_rel16();
	void jge_rel16();
	void jl_rel16();
	void jle_rel16();
	void jnc_rel16();
	void jno_rel16();
	void jnp_rel16();
	void jns_rel16();
	void jnz_rel16();
	void jo_rel16();
	void jp_rel16();
	void js_rel16();
	void jz_rel16();
	void jcxz16();
	void jmp_rel16();
	void jmp_abs16();
	void lea16();
	void leave16();
	void lodsw();
	void loop16();
	void loopne16();
	void loopz16();
	void mov_rm16_r16();
	void mov_r16_rm16();
	void mov_rm16_i16();
	void mov_ax_m16();
	void mov_m16_ax();
	void mov_ax_i16();
	void mov_cx_i16();
	void mov_dx_i16();
	void mov_bx_i16();
	void mov_sp_i16();
	void mov_bp_i16();
	void mov_si_i16();
	void mov_di_i16();
	void movsw();
	void movsx_r16_rm8();
	void movzx_r16_rm8();
	void or_rm16_r16();
	void or_r16_rm16();
	void or_ax_i16();
	void out_ax_i8();
	void out_ax_dx();
	void pop_ax();
	void pop_cx();
	void pop_dx();
	void pop_bx();
	void pop_sp();
	void pop_bp();
	void pop_si();
	void pop_di();
	void pop_ds16();
	void pop_es16();
	void pop_fs16();
	void pop_gs16();
	void pop_ss16();
	void pop_rm16();
	void popa();
	void popf();
	void push_ax();
	void push_cx();
	void push_dx();
	void push_bx();
	void push_sp();
	void push_bp();
	void push_si();
	void push_di();
	void push_cs16();
	void push_ds16();
	void push_es16();
	void push_fs16();
	void push_gs16();
	void push_ss16();
	void push_i16();
	void pusha();
	void pushf();
	void ret_near16_i16();
	void ret_near16();
	void sbb_rm16_r16();
	void sbb_r16_rm16();
	void sbb_ax_i16();
	void scasw();
	void shld16_i8();
	void shld16_cl();
	void shrd16_i8();
	void shrd16_cl();
	void stosw();
	void sub_rm16_r16();
	void sub_r16_rm16();
	void sub_ax_i16();
	void test_ax_i16();
	void test_rm16_r16();
	void xchg_ax_cx();
	void xchg_ax_dx();
	void xchg_ax_bx();
	void xchg_ax_sp();
	void xchg_ax_bp();
	void xchg_ax_si();
	void xchg_ax_di();
	void xchg_r16_rm16();
	void xor_rm16_r16();
	void xor_r16_rm16();
	void xor_ax_i16();
	void group81_16();
	void group83_16();
	void groupC1_16();
	void groupD1_16();
	void groupD3_16();
	void groupF7_16();
	void groupFF_16();
	void group0F00_16();
	void group0F01_16();
	void group0FBA_16();
	void bound_r16_m16_m16();
	void retf16();
	void retf_i16();
	void xlat16();
	void load_far_pointer16(int s);
	void lds16();
	void lss16();
	void les16();
	void lfs16();
	void lgs16();
	uint32 shift_rotate32(uint8 modrm, uint32 val, uint8 shift);
	void adc_rm32_r32();
	void adc_r32_rm32();
	void adc_eax_i32();
	void add_rm32_r32();
	void add_r32_rm32();
	void add_eax_i32();
	void and_rm32_r32();
	void and_r32_rm32();
	void and_eax_i32();
	void bsf_r32_rm32();
	void bsr_r32_rm32();
	void bt_rm32_r32();
	void btc_rm32_r32();
	void btr_rm32_r32();
	void bts_rm32_r32();
	void call_abs32();
	void call_rel32();
	void cdq();
	void cmp_rm32_r32();
	void cmp_r32_rm32();
	void cmp_eax_i32();
	void cmpsd();
	void cwde();
	void dec_eax();
	void dec_ecx();
	void dec_edx();
	void dec_ebx();
	void dec_esp();
	void dec_ebp();
	void dec_esi();
	void dec_edi();
	void imul_r32_rm32();
	void imul_r32_rm32_i32();
	void imul_r32_rm32_i8();
	void in_eax_i8();
	void in_eax_dx();
	void inc_eax();
	void inc_ecx();
	void inc_edx();
	void inc_ebx();
	void inc_esp();
	void inc_ebp();
	void inc_esi();
	void inc_edi();
	void iret32();
	void ja_rel32();
	void jbe_rel32();
	void jc_rel32();
	void jg_rel32();
	void jge_rel32();
	void jl_rel32();
	void jle_rel32();
	void jnc_rel32();
	void jno_rel32();
	void jnp_rel32();
	void jns_rel32();
	void jnz_rel32();
	void jo_rel32();
	void jp_rel32();
	void js_rel32();
	void jz_rel32();
	void jcxz32();
	void jmp_rel32();
	void jmp_abs32();
	void lea32();
	void leave32();
	void lodsd();
	void loop32();
	void loopne32();
	void loopz32();
	void mov_rm32_r32();
	void mov_r32_rm32();
	void mov_rm32_i32();
	void mov_eax_m32();
	void mov_m32_eax();
	void mov_eax_i32();
	void mov_ecx_i32();
	void mov_edx_i32();
	void mov_ebx_i32();
	void mov_esp_i32();
	void mov_ebp_i32();
	void mov_esi_i32();
	void mov_edi_i32();
	void movsd();
	void movsx_r32_rm8();
	void movsx_r32_rm16();
	void movzx_r32_rm8();
	void movzx_r32_rm16();
	void or_rm32_r32();
	void or_r32_rm32();
	void or_eax_i32();
	void out_eax_i8();
	void out_eax_dx();
	void pop_eax();
	void pop_ecx();
	void pop_edx();
	void pop_ebx();
	void pop_esp();
	void pop_ebp();
	void pop_esi();
	void pop_edi();
	void pop_ds32();
	void pop_es32();
	void pop_fs32();
	void pop_gs32();
	void pop_ss32();
	void pop_rm32();
	void popad();
	void popfd();
	void push_eax();
	void push_ecx();
	void push_edx();
	void push_ebx();
	void push_esp();
	void push_ebp();
	void push_esi();
	void push_edi();
	void push_cs32();
	void push_ds32();
	void push_es32();
	void push_fs32();
	void push_gs32();
	void push_ss32();
	void push_i32();
	void pushad();
	void pushfd();
	void ret_near32_i16();
	void ret_near32();
	void sbb_rm32_r32();
	void sbb_r32_rm32();
	void sbb_eax_i32();
	void scasd();
	void shld32_i8();
	void shld32_cl();
	void shrd32_i8();
	void shrd32_cl();
	void stosd();
	void sub_rm32_r32();
	void sub_r32_rm32();
	void sub_eax_i32();
	void test_eax_i32();
	void test_rm32_r32();
	void xchg_eax_ecx();
	void xchg_eax_edx();
	void xchg_eax_ebx();
	void xchg_eax_esp();
	void xchg_eax_ebp();
	void xchg_eax_esi();
	void xchg_eax_edi();
	void xchg_r32_rm32();
	void xor_rm32_r32();
	void xor_r32_rm32();
	void xor_eax_i32();
	void group81_32();
	void group83_32();
	void groupC1_32();
	void groupD1_32();
	void groupD3_32();
	void groupF7_32();
	void groupFF_32();
	void group0F00_32();
	void group0F01_32();
	void group0FBA_32();
	void bound_r32_m32_m32();
	void retf32();
	void retf_i32();
	void xlat32();
	void load_far_pointer32(int s);
	void lds32();
	void lss32();
	void les32();
	void lfs32();
	void lgs32();
	// Intel 486+ specific opcodes
	void cpuid();
	void invd();
	void wbinvd();
	void cmpxchg_rm8_r8();
	void cmpxchg_rm16_r16();
	void cmpxchg_rm32_r32();
	void xadd_rm8_r8();
	void xadd_rm16_r16();
	void xadd_rm32_r32();
	// Pentium+ specific opcodes
	void rdmsr();
	void wrmsr();
	void rdtsc();
	void cyrix_unknown();
	void cmpxchg8b_m64();
	void FPU_PUSH(x87reg_c val);
	void FPU_POP();
	void fpu_group_d8();
	void fpu_group_d9();
	void fpu_group_da();
	void fpu_group_db();
	void fpu_group_dc();
	void fpu_group_dd();
	void fpu_group_de();
	void fpu_group_df();
	
	/* ---------------------------------------------------------------------------
	virtual machine interfaces
	--------------------------------------------------------------------------- */
	
	// memory
	void translate_address(uint32 *address) {
		uint32 a = *address;
		uint32 pdbr = cr[3] & 0xfffff000;
		uint32 directory = (a >> 22) & 0x3ff;
		uint32 table = (a >> 12) & 0x3ff;
		uint32 offset = a & 0xfff;
		
		// TODO: 4MB pages
		uint32 page_dir = d_mem->read_data32(pdbr + directory * 4);
		uint32 page_entry = d_mem->read_data32((page_dir & 0xfffff000) + (table * 4));
		*address = (page_entry & 0xfffff000) | offset;
	}
	uint8 RM8(uint32 ea) {
		uint32 addr = ea;
		if(cr[0] & 0x80000000)
			translate_address(&addr);
		return d_mem->read_data8(addr & a20_mask);
	}
	uint16 RM16(uint32 ea) {
		uint32 addr = ea;
		if(cr[0] & 0x80000000)
			translate_address(&addr);
		return d_mem->read_data16(addr & a20_mask);
	}
	uint32 RM32(uint32 ea) {
		uint32 addr = ea;
		if(cr[0] & 0x80000000)
			translate_address(&addr);
		return d_mem->read_data32(addr & a20_mask);
	}
	uint64 RM64(uint32 ea) {
		uint32 addr = ea;
		if(cr[0] & 0x80000000)
			translate_address(&addr);
		return d_mem->read_data32(addr & a20_mask) | (((uint64)d_mem->read_data32((addr + 4) & a20_mask)) << 32);
	}
	void WM8(uint32 ea, uint8 val) {
		uint32 addr = ea;
		if(cr[0] & 0x80000000)
			translate_address(&addr);
		d_mem->write_data8(addr & a20_mask, val);
	}
	void WM16(uint32 ea, uint16 val) {
		uint32 addr = ea;
		if(cr[0] & 0x80000000)
			translate_address(&addr);
		d_mem->write_data16(addr & a20_mask, val);
	}
	void WM32(uint32 ea, uint32 val) {
		uint32 addr = ea;
		if(cr[0] & 0x80000000)
			translate_address(&addr);
		d_mem->write_data32(addr & a20_mask, val);
	}
	void WM64(uint32 ea, uint64 val) {
		uint32 addr = ea;
		if(cr[0] & 0x80000000)
			translate_address(&addr);
		d_mem->write_data32(addr & a20_mask, val & 0xffffffff);
		d_mem->write_data32((addr + 4) & a20_mask, (val >> 32) & 0xffffffff);
	}
	uint8 FETCHOP() {
		uint32 addr = pc;
		if(cr[0] & 0x80000000)
			translate_address(&addr);
		uint8 val = d_mem->read_data8(prev_pc = addr & a20_mask);
		eip++;
		pc++;
		return val;
	}
	uint8 FETCH8() {
		uint32 addr = pc;
		if(cr[0] & 0x80000000)
			translate_address(&addr);
		uint8 val = d_mem->read_data8(addr & a20_mask);
		eip++;
		pc++;
		return val;
	}
	uint16 FETCH16() {
		uint32 addr = pc;
		if(cr[0] & 0x80000000)
			translate_address(&addr);
		uint16 val = d_mem->read_data16(addr & a20_mask);
		eip += 2;
		pc += 2;
		return val;
	}
	uint32 FETCH32() {
		uint32 addr = pc;
		if(cr[0] & 0x80000000)
			translate_address(&addr);
		uint32 val = d_mem->read_data32(addr & a20_mask);
		eip += 4;
		pc += 4;
		return val;
	}
	
	// i/o
	uint8 IN8(uint32 addr) {
		return d_io->read_io8(addr);
	}
	uint16 IN16(uint32 addr) {
		return d_io->read_io16(addr);
	}
	uint32 IN32(uint32 addr) {
		return d_io->read_io32(addr);
	}
	void OUT8(uint32 addr, uint8 val) {
		d_io->write_io8(addr, val);
	}
	void OUT16(uint32 addr, uint16 val) {
		d_io->write_io16(addr, val);
	}
	void OUT32(uint32 addr, uint32 val) {
		d_io->write_io32(addr, val);
	}
	
	// interrupt
	uint32 ACK_INTR() {
		return d_pic->intr_ack();
	}
	
public:
	I386(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		d_bios = NULL;
#ifdef SINGLE_MODE_DMA
		d_dma = NULL;
#endif
		busreq = 0;
	}
	~I386() {}
	
	// common functions
	void initialize();
	void reset();
	int run(int clock);
	void write_signal(int id, uint32 data, uint32 mask);
	void set_intr_line(bool line, bool pending, uint32 bit);
	uint32 get_pc() {
		return prev_pc;
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
#ifdef SINGLE_MODE_DMA
	void set_context_dma(DEVICE* device) {
		d_dma = device;
	}
#endif
};

#endif
