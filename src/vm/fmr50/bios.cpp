/*
	FUJITSU FMR-50 Emulator 'eFMR-50'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.10.06 -

	[ bios ]
*/

#include "bios.h"
#include "../disk.h"

// regs
#define AX	regs[0]
#define CX	regs[1]
#define DX	regs[2]
#define BX	regs[3]
#define SP	regs[4]
#define BP	regs[5]
#define SI	regs[6]
#define DI	regs[7]

#define AL	regs8[0]
#define AH	regs8[1]
#define CL	regs8[2]
#define CH	regs8[3]
#define DL	regs8[4]
#define DH	regs8[5]
#define BL	regs8[6]
#define BH	regs8[7]
#define SPL	regs8[8]
#define SPH	regs8[9]
#define BPL	regs8[10]
#define BPH	regs8[11]
#define SIL	regs8[12]
#define SIH	regs8[13]
#define DIL	regs8[14]
#define DIH	regs8[15]

// sregs
#define ES	sregs[0]
#define CS	sregs[1]
#define SS	sregs[2]
#define DS	sregs[3]

// error
#define BIOS_ERR__NOTREADY	1
#define BIOS_ERR__PROTECTED	2
#define BIOS_ERR__DELETED	4
#define BIOS_ERR__NOTFOUND	8
#define BIOS_ERR__CRCERROR	0x10

#define BIOS_ERR__NOTCONNECTED	4

// fdc status
#define FDC_ST_BUSY		0x01	// busy
#define FDC_ST_INDEX		0x02	// index hole
#define FDC_ST_DRQ		0x02	// data request
#define FDC_ST_TRACK00		0x04	// track0
#define FDC_ST_LOSTDATA		0x04	// data lost
#define FDC_ST_CRCERR		0x08	// crc error
#define FDC_ST_SEEKERR		0x10	// seek error
#define FDC_ST_RECNFND		0x10	// sector not found
#define FDC_ST_HEADENG		0x20	// head engage
#define FDC_ST_RECTYPE		0x20	// record type
#define FDC_ST_WRITEFAULT	0x20	// write fault
#define FDC_ST_WRITEP		0x40	// write protect
#define FDC_ST_NOTREADY		0x80	// media not inserted

void BIOS::initialize()
{
	for(int i = 0; i < MAX_DRIVE; i++)
		access[i] = false;
	secnum = 1;
}

bool BIOS::bios_call(uint32 PC, uint16 regs[], uint16 sregs[], int32* ZeroFlag, int32* CarryFlag)
{
	uint8 *regs8 = (uint8 *)regs;
	int drv = AL & 0xf;
	
	if(PC == 0xfffc4) {
//		emu->out_debug("BIOSCALL: AH=%2x,AL=%2x,CX=%4x,DH=%2x,DL=%2x,BX=%4x,DS=%2x,DI=%2x\n", AH,AL,CX,DH,DL,BX,DS,DI);
		if(AH == 5) {
			// read sectors
			if((AL & 0xf0) == 0x20) {
				// floppy
				if(!(drv < MAX_DRIVE && disk[drv]->insert)) {
					AH = 0x80;
					CX = BIOS_ERR__NOTREADY;
					*CarryFlag = 1;
					return true;
				}
				// get initial c/h/r
				int ofs = DS * 16 + DI;
				int trk = CX;
				int hed = DH & 1;
				int sct = DL;
				while(BX > 0) {
					// search sector
					disk[drv]->get_track(trk, hed);
					access[drv] = true;
					secnum = sct;
					if(!disk[drv]->get_sector(trk, hed, sct - 1)) {
						AH = 0x80;
						CX = BIOS_ERR__NOTFOUND;
						*CarryFlag = 1;
						return true;
					}
					// check deleted mark
					if(disk[drv]->deleted) {
						AH = 0x80;
						CX = BIOS_ERR__DELETED;
						*CarryFlag = 1;
						return true;
					}
					// data transfer
					for(int i = 0; i < disk[drv]->sector_size; i++)
						d_mem->write_data8(ofs++, disk[drv]->sector[i]);
					BX--;
					// check crc error
					if(disk[drv]->status) {
						AH = 0x80;
						CX = BIOS_ERR__CRCERROR;
						*CarryFlag = 1;
						return true;
					}
					// update c/h/r
					if(++sct > disk[drv]->sector_num) {
						sct = 1;
						if(++hed > 1) {
							hed = 0;
							++trk;
						}
					}
				}
				AH = 0;
				CX = 0;
				*CarryFlag = 0;
				return true;
			}
			if((AL & 0xf0) == 0xb0) {
				// scsi
				if(!(drv < MAX_SCSI)) {
					AH = 0x80;
					CX = BIOS_ERR__NOTCONNECTED;
					*CarryFlag = 1;
					return true;
				}
				AH = 0;
				CX = 0;
				*CarryFlag = 0;
				return true;
			}
			AH = 2;
			*CarryFlag = 1;
			return true;
		}
	}
	return false;
}

bool BIOS::bios_int(int intnum, uint16 regs[], uint16 sregs[], int32* ZeroFlag, int32* CarryFlag)
{
	uint8 *regs8 = (uint8 *)regs;
	int drv = AL & 0xf;
	
	if(intnum == 0x93) {
//		emu->out_debug("BIOSINT: AH=%2x,AL=%2x,CX=%4x,DH=%2x,DL=%2x,BX=%4x,DS=%2x,DI=%2x\n", AH,AL,CX,DH,DL,BX,DS,DI);
		// disk bios
		if(AH == 2) {
			// drive status
			if((AL & 0xf0) == 0x20) {
				// floppy
				if(drv < MAX_DRIVE && disk[drv]->insert) {
					AH = 0;
					DL = 4;
					if(disk[drv]->protect)
						DL |= 2;
					CX = 0;
					*CarryFlag = 0;
					return true;
				}
				AH = 0x80;
				CX = BIOS_ERR__NOTREADY;
				*CarryFlag = 1;
				return true;
			}
			if((AL & 0xf0) == 0xb0) {
				// scsi
				if(drv < MAX_SCSI) {
					AH = 0;
					AL = 2;		// LEN = 512B
					BX = 0xa0;	// 20MB/512B = $A000
					DX = 0;
					CX = 0;
					*CarryFlag = 0;
					return true;
				}
				AH = 0x80;
				CX = BIOS_ERR__NOTCONNECTED;
				*CarryFlag = 1;
				return true;
			}
			AH = 2;
			*CarryFlag = 1;
			return true;
		}
		else if(AH == 5) {
			// read sectors
			if((AL & 0xf0) == 0x20) {
				// floppy
				if(!(drv < MAX_DRIVE && disk[drv]->insert)) {
					AH = 0x80;
					CX = BIOS_ERR__NOTREADY;
					*CarryFlag = 1;
					return true;
				}
				// get initial c/h/r
				int ofs = DS * 16 + DI;
				int trk = CX;
				int hed = DH & 1;
				int sct = DL;
				while(BX > 0) {
					// search sector
					disk[drv]->get_track(trk, hed);
					access[drv] = true;
					secnum = sct;
					if(!disk[drv]->get_sector(trk, hed, sct - 1)) {
						AH = 0x80;
						CX = BIOS_ERR__NOTFOUND;
						*CarryFlag = 1;
						return true;
					}
					// check deleted mark
					if(disk[drv]->deleted) {
						AH = 0x80;
						CX = BIOS_ERR__DELETED;
						*CarryFlag = 1;
						return true;
					}
					// data transfer
					for(int i = 0; i < disk[drv]->sector_size; i++)
						d_mem->write_data8(ofs++, disk[drv]->sector[i]);
					BX--;
					// check crc error
					if(disk[drv]->status) {
						AH = 0x80;
						CX = BIOS_ERR__CRCERROR;
						*CarryFlag = 1;
						return true;
					}
					// update c/h/r
					if(++sct > disk[drv]->sector_num) {
						sct = 1;
						if(++hed > 1) {
							hed = 0;
							++trk;
						}
					}
				}
				AH = 0;
				CX = 0;
				*CarryFlag = 0;
				return true;
			}
			if((AL & 0xf0) == 0xb0) {
				// scsi
				if(!(drv < MAX_SCSI)) {
					AH = 0x80;
					CX = BIOS_ERR__NOTCONNECTED;
					*CarryFlag = 1;
					return true;
				}
				AH = 0;
				CX = 0;
				*CarryFlag = 0;
				return true;
			}
			AH = 2;
			*CarryFlag = 1;
			return true;
		}
		else if(AH == 6) {
			// write sectors
			if((AL & 0xf0) == 0x20) {
				// floppy
				if(!(drv < MAX_DRIVE && disk[drv]->insert)) {
					AH = 0x80;
					CX = BIOS_ERR__NOTREADY;
					*CarryFlag = 1;
					return true;
				}
				if(disk[drv]->protect) {
					AH = 0x80;
					CX = BIOS_ERR__PROTECTED;
					*CarryFlag = 1;
					return true;
				}
				// get initial c/h/r
				int ofs = DS * 16 + DI;
				int trk = CX;
				int hed = DH & 1;
				int sct = DL;
				while(BX > 0) {
					// search sector
					disk[drv]->get_track(trk, hed);
					access[drv] = true;
					secnum = sct;
					if(!disk[drv]->get_sector(trk, hed, sct - 1)) {
						AH = 0x80;
						CX = BIOS_ERR__NOTFOUND;
						*CarryFlag = 1;
						return true;
					}
					// data transfer
					for(int i = 0; i < disk[drv]->sector_size; i++)
						disk[drv]->sector[i] = d_mem->read_data8(ofs++);
					BX--;
					// clear deleted mark and crc error
					disk[drv]->deleted = 0;
					disk[drv]->status = 0;
					// update c/h/r
					if(++sct > disk[drv]->sector_num) {
						sct = 1;
						if(++hed > 1) {
							hed = 0;
							++trk;
						}
					}
				}
				AH = 0;
				CX = 0;
				*CarryFlag = 0;
				return true;
			}
			if((AL & 0xf0) == 0xb0) {
				// scsi
				if(!(drv < MAX_SCSI)) {
					AH = 0x80;
					CX = BIOS_ERR__NOTCONNECTED;
					*CarryFlag = 1;
					return true;
				}
				AH = 0;
				CX = 0;
				*CarryFlag = 0;
				return true;
			}
			AH = 2;
			*CarryFlag = 1;
			return true;
		}
		else if(AH == 9) {
			// read id
			if((AL & 0xf0) == 0x20) {
				// floppy
				if(!(drv < MAX_DRIVE && disk[drv]->insert)) {
					AH = 0x80;
					CX = BIOS_ERR__NOTREADY;
					*CarryFlag = 1;
					return true;
				}
				// get initial c/h/r
				int ofs = DS * 16 + DI;
				int trk = CX;
				int hed = DH & 1;
				// search sector
				disk[drv]->get_track(trk, hed);
				access[drv] = true;
				if(++secnum > disk[drv]->sector_num)
					secnum = 1;
				if(!disk[drv]->get_sector(trk, hed, secnum - 1)) {
					AH = 0x80;
					CX = BIOS_ERR__NOTFOUND;
					*CarryFlag = 1;
					return true;
				}
				for(int i = 0; i < 6; i++)
					d_mem->write_data8(ofs++, disk[drv]->id[i]);
				AH = 0;
				CX = 0;
				*CarryFlag = 0;
				return true;
			}
			AH = 2;
			*CarryFlag = 1;
			return true;
		}
	}
	return false;
}

uint32 BIOS::read_signal(int ch)
{
	// get access status
	uint32 stat = 0;
	for(int i = 0; i < MAX_DRIVE; i++) {
		if(access[i])
			stat |= 1 << i;
		access[i] = false;
	}
	return stat;
}

