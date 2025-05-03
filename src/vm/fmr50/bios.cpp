/*
	FUJITSU FMR-50 Emulator 'eFMR-50'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.10.06 -

	[ bios ]
*/

#include "bios.h"
#include "../../fileio.h"
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
#define FDD_ERR_NOTREADY	1
#define FDD_ERR_PROTECTED	2
#define FDD_ERR_DELETED		4
#define FDD_ERR_NOTFOUND	8
#define FDD_ERR_CRCERROR	0x10

#define SCSI_ERR_NOTREADY	1
#define SCSI_ERR_PARAMERROR	2
#define SCSI_ERR_NOTCONNECTED	4

void BIOS::initialize()
{
	// check ipl
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sIPL.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(buffer, sizeof(buffer), 1);
		fio->Fclose();
		
		disk_pc1 = disk_pc2 = cmos_pc = wait_pc = -1;
		uint32 addr = 0xfffc4;
		if(buffer[addr & 0x3fff] == 0xea) {
			int ofs = buffer[++addr & 0x3fff];
			ofs |= buffer[++addr & 0x3fff] << 8;
			int seg = buffer[++addr & 0x3fff];
			seg |= buffer[++addr & 0x3fff] << 8;
			disk_pc1 = addr = ofs + (seg << 4);
		}
		if(buffer[addr & 0x3fff] == 0xea) {
			int ofs = buffer[++addr & 0x3fff];
			ofs |= buffer[++addr & 0x3fff] << 8;
			int seg = buffer[++addr & 0x3fff];
			seg |= buffer[++addr & 0x3fff] << 8;
			disk_pc2 = ofs + (seg << 4);
		}
//		cmos_pc = 0xfffc9;
//		wait_pc = 0xfffd3;
	}
	else {
		// use pseudo ipl
		disk_pc1 = disk_pc2 = -1;
		cmos_pc = 0xfffc9;
		wait_pc = 0xfffd3;
	}
	// init scsi
	_memset(scsi_blocks, 0, sizeof(scsi_blocks));
	for(int i = 0; i < MAX_SCSI; i++) {
		_stprintf(scsi_path[i], _T("%sSCSI%d.DAT"), app_path, i);
		if(fio->Fopen(scsi_path[i], FILEIO_READ_BINARY)) {
			fio->Fseek(0, FILEIO_SEEK_END);
			scsi_blocks[i] = fio->Ftell() / BLOCK_SIZE;
			fio->Fclose();
		}
	}
	delete fio;
	
	// regist event
	vm->regist_frame_event(this);
}

void BIOS::reset()
{
	for(int i = 0; i < MAX_DRIVE; i++)
		access_fdd[i] = false;
	access_scsi = false;
	secnum = 1;
	timeout = 0;
}

void BIOS::event_frame()
{
	timeout++;
}

bool BIOS::bios_call(uint32 PC, uint16 regs[], uint16 sregs[], int32* ZeroFlag, int32* CarryFlag)
{
	uint8 *regs8 = (uint8 *)regs;
	int drv = AL & 0xf;
	
	if(PC == 0xfffc4 || PC == disk_pc1 || PC == disk_pc2) {
		// disk bios
#ifdef _DEBUG_LOG
		emu->out_debug("%6x\tDISK BIOS: AH=%2x,AL=%2x,CX=%4x,DX=%4x,BX=%4x,DS=%2x,DI=%2x\n", vm->get_prv_pc(), AH,AL,CX,DX,BX,DS,DI);
#endif
		if(AH == 2) {
			// drive status
			if((AL & 0xf0) == 0x20) {
				// floppy
				if(!(drv < MAX_DRIVE && disk[drv]->insert)) {
					AH = 0x80;
					CX = FDD_ERR_NOTREADY;
					*CarryFlag = 1;
					return true;
				}
				AH = 0;
				DL = 4;
				if(disk[drv]->protect)
					DL |= 2;
				CX = 0;
				*CarryFlag = 0;
				return true;
			}
			if((AL & 0xf0) == 0xb0) {
				// scsi
				if(!(drv < MAX_SCSI && scsi_blocks[drv])) {
					AH = 0x80;
					CX = SCSI_ERR_NOTCONNECTED;
					*CarryFlag = 1;
					return true;
				}
				AH = 0;
				AL = (BLOCK_SIZE == 128) ? 0 : (BLOCK_SIZE == 256) ? 1 : (BLOCK_SIZE == 512) ? 2 : 3;
				BX = scsi_blocks[drv] >> 16;
				DX = scsi_blocks[drv] & 0xffff;
				CX = 0;
				*CarryFlag = 0;
				return true;
			}
			AH = 2;
			*CarryFlag = 1;
			return true;
		}
		else if(AH == 3 || AH == 4) {
			// resture/seek
			if((AL & 0xf0) == 0x20) {
				// floppy
				if(!(drv < MAX_DRIVE && disk[drv]->insert)) {
					AH = 0x80;
					CX = FDD_ERR_NOTREADY;
					*CarryFlag = 1;
					return true;
				}
				AH = 0;
				CX = 0;
				*CarryFlag = 0;
				return true;
			}
			if((AL & 0xf0) == 0xb0) {
				// scsi
				if(!(drv < MAX_SCSI && scsi_blocks[drv])) {
					AH = 0x80;
					CX = SCSI_ERR_NOTCONNECTED;
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
		else if(AH == 5) {
			// read sectors
			if((AL & 0xf0) == 0x20) {
				// floppy
				if(!(drv < MAX_DRIVE && disk[drv]->insert)) {
					AH = 0x80;
					CX = FDD_ERR_NOTREADY;
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
					access_fdd[drv] = true;
					secnum = sct;
					if(!disk[drv]->get_sector(trk, hed, sct - 1)) {
						AH = 0x80;
						CX = FDD_ERR_NOTFOUND;
						*CarryFlag = 1;
						return true;
					}
					// check deleted mark
					if(disk[drv]->deleted) {
						AH = 0x80;
						CX = FDD_ERR_DELETED;
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
						CX = FDD_ERR_CRCERROR;
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
				if(!(drv < MAX_SCSI && scsi_blocks[drv])) {
					AH = 0x80;
					CX = SCSI_ERR_NOTCONNECTED;
					*CarryFlag = 1;
					return true;
				}
				FILEIO* fio = new FILEIO();
				if(!fio->Fopen(scsi_path[drv], FILEIO_READ_BINARY)) {
					AH = 0x80;
					CX = SCSI_ERR_NOTREADY;
					*CarryFlag = 1;
					delete fio;
					return true;
				}
				// get params
				int ofs = DS * 16 + DI;
				int block = (CL << 16) | DX;
				fio->Fseek(block * BLOCK_SIZE, FILEIO_SEEK_SET);
				while(BX > 0) {
					// check block
					access_scsi = true;
					if(!(block++ < scsi_blocks[drv])) {
						AH = 0x80;
						CX = SCSI_ERR_PARAMERROR;
						*CarryFlag = 1;
						fio->Fclose();
						delete fio;
						return true;
					}
					// data transfer
					fio->Fread(buffer, BLOCK_SIZE, 1);
					for(int i = 0; i < BLOCK_SIZE; i++)
						d_mem->write_data8(ofs++, buffer[i]);
					BX--;
				}
				AH = 0;
				CX = 0;
				*CarryFlag = 0;
				fio->Fclose();
				delete fio;
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
					CX = FDD_ERR_NOTREADY;
					*CarryFlag = 1;
					return true;
				}
				if(disk[drv]->protect) {
					AH = 0x80;
					CX = FDD_ERR_PROTECTED;
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
					access_fdd[drv] = true;
					secnum = sct;
					if(!disk[drv]->get_sector(trk, hed, sct - 1)) {
						AH = 0x80;
						CX = FDD_ERR_NOTFOUND;
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
				if(!(drv < MAX_SCSI && scsi_blocks[drv])) {
					AH = 0x80;
					CX = SCSI_ERR_NOTCONNECTED;
					*CarryFlag = 1;
					return true;
				}
				FILEIO* fio = new FILEIO();
				if(!fio->Fopen(scsi_path[drv], FILEIO_READ_WRITE_BINARY)) {
					AH = 0x80;
					CX = SCSI_ERR_NOTREADY;
					*CarryFlag = 1;
					delete fio;
					return true;
				}
				// get params
				int ofs = DS * 16 + DI;
				int block = (CL << 16) | DX;
				fio->Fseek(block * BLOCK_SIZE, FILEIO_SEEK_SET);
				while(BX > 0) {
					// check block
					access_scsi = true;
					if(!(block++ < scsi_blocks[drv])) {
						AH = 0x80;
						CX = SCSI_ERR_PARAMERROR;
						*CarryFlag = 1;
						fio->Fclose();
						delete fio;
						return true;
					}
					// data transfer
					for(int i = 0; i < BLOCK_SIZE; i++)
						buffer[i] = d_mem->read_data8(ofs++);
					fio->Fwrite(buffer, BLOCK_SIZE, 1);
					BX--;
				}
				AH = 0;
				CX = 0;
				*CarryFlag = 0;
				fio->Fclose();
				delete fio;
				return true;
			}
			AH = 2;
			*CarryFlag = 1;
			return true;
		}
		else if(AH == 7) {
			// verify sectors
			if((AL & 0xf0) == 0x20) {
				// floppy
				if(!(drv < MAX_DRIVE && disk[drv]->insert)) {
					AH = 0x80;
					CX = FDD_ERR_NOTREADY;
					*CarryFlag = 1;
					return true;
				}
				// get initial c/h/r
				int trk = CX;
				int hed = DH & 1;
				int sct = DL;
				while(BX > 0) {
					// search sector
					disk[drv]->get_track(trk, hed);
					access_fdd[drv] = true;
					secnum = sct;
					if(!disk[drv]->get_sector(trk, hed, sct - 1)) {
						AH = 0x80;
						CX = FDD_ERR_NOTFOUND;
						*CarryFlag = 1;
						return true;
					}
					BX--;
					// check crc error
					if(disk[drv]->status) {
						AH = 0x80;
						CX = FDD_ERR_CRCERROR;
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
				if(!(drv < MAX_SCSI && scsi_blocks[drv])) {
					AH = 0x80;
					CX = SCSI_ERR_NOTCONNECTED;
					*CarryFlag = 1;
					return true;
				}
				// get params
				int block = (CL << 16) | DX;
				while(BX > 0) {
					// check block
					access_scsi = true;
					if(!(block++ < scsi_blocks[drv])) {
						AH = 0x80;
						CX = SCSI_ERR_PARAMERROR;
						*CarryFlag = 1;
						return true;
					}
					BX--;
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
		else if(AH == 8) {
			// reset hard drive controller
			AH = 0;
			CX = 0;
			*CarryFlag = 0;
			return true;
		}
		else if(AH == 9) {
			// read id
			if((AL & 0xf0) == 0x20) {
				// floppy
				if(!(drv < MAX_DRIVE && disk[drv]->insert)) {
					AH = 0x80;
					CX = FDD_ERR_NOTREADY;
					*CarryFlag = 1;
					return true;
				}
				// get initial c/h
				int ofs = DS * 16 + DI;
				int trk = CX;
				int hed = DH & 1;
				// search sector
				disk[drv]->get_track(trk, hed);
				access_fdd[drv] = true;
				if(++secnum > disk[drv]->sector_num)
					secnum = 1;
				if(!disk[drv]->get_sector(trk, hed, secnum - 1)) {
					AH = 0x80;
					CX = FDD_ERR_NOTFOUND;
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
		else if(AH == 0xa) {
			// format track
			if((AL & 0xf0) == 0x20) {
				// floppy
				if(!(drv < MAX_DRIVE && disk[drv]->insert)) {
					AH = 0x80;
					CX = FDD_ERR_NOTREADY;
					*CarryFlag = 1;
					return true;
				}
				// get initial c/h
				int trk = CX;
				int hed = DH & 1;
				// search sector
				disk[drv]->get_track(trk, hed);
				access_fdd[drv] = true;
				for(int i = 0; i < disk[drv]->sector_num; i++) {
					disk[drv]->get_sector(trk, hed, i);
					_memset(disk[drv]->sector, 0xe5, disk[drv]->sector_size);
					disk[drv]->deleted = 0;
					disk[drv]->status = 0;
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
		else if(AH == 0xd) {
			// read error
			AH = 0;
			CX = 0;
			*CarryFlag = 0;
			return true;
		}
		else if(AH == 0xe) {
			// drive check
			if((AL & 0xf0) == 0xb0) {
				// scsi
				int ofs = DS * 16 + DI;
				if(!(drv < MAX_SCSI && scsi_blocks[drv])) {
					AH = 0x80;
					CX = SCSI_ERR_NOTCONNECTED;
					*CarryFlag = 1;
					return true;
				}
				AH = 0;
				CX = 0;
				DL = 0;
				*CarryFlag = 0;
				return true;
			}
		}
		else if(AH == 0xfa) {
			// unknown
//			if((AL & 0xf0) == 0xb0) {
				// scsi
#if 0
				AH = 0x80;
				*CarryFlag = 1;
#else
				AH = 0;
				BL = 0;//x80;
				CX = 0;
				*CarryFlag = 0;
#endif
				return true;
//			}
		}
		// for pseudo bios
		else if(AH == 0x80) {
			// pseudo bios: init i/o
			for(int i = 0; i < 66; i++)
				d_io->write_io8(iotable[i][0], iotable[i][1]);
			// init cmos
			_memset(cmos, 0, 0x800);
			_memcpy(cmos + 0x000, cmos_t, sizeof(cmos_t));
			_memcpy(cmos + 0x7d1, cmos_b, sizeof(cmos_b));
			// init int vector
			for(int i = 0, ofs = 0; i < 256; i++) {
				d_mem->write_data8(ofs++, 0x08);
				d_mem->write_data8(ofs++, 0x00);
				d_mem->write_data8(ofs++, 0xff);
				d_mem->write_data8(ofs++, 0xff);
			}
			// init screen
			_memset(vram, 0, 0x40000);
			_memset(cvram, 0, 0x1000);
			_memset(kvram, 0, 0x1000);
			_memcpy(cvram + 0xf00, msg_c, sizeof(msg_c));
			_memcpy(kvram + 0xf00, msg_k, sizeof(msg_k));
			*CarryFlag = 0;
			return true;
		}
		else if(AH == 0x81) {
			// pseudo bios: boot from fdd #0
			*ZeroFlag = (timeout > (int)(FRAMES_PER_SEC * 4));
			if(!disk[0]->insert) {
				*CarryFlag = 1;
				return true;
			}
			// load ipl
			disk[0]->get_track(0, 0);
			access_fdd[0] = true;
			if(!disk[0]->get_sector(0, 0, 0)) {
				*CarryFlag = 1;
				return true;
			}
			for(int i = 0; i < disk[0]->sector_size; i++)
				buffer[i] = disk[0]->sector[i];
			// check ipl
			if(!(buffer[0] == 'I' && buffer[1] == 'P' && buffer[2] == 'L' && buffer[3] == '1')) {
				*CarryFlag = 1;
				return true;
			}
			// data transfer
			for(int i = 0; i < disk[0]->sector_size; i++)
				d_mem->write_data8(0xb0000 + i, buffer[i]);
			// clear screen
			_memset(cvram, 0, 0x1000);
			_memset(kvram, 0, 0x1000);
			// set cmos
//			_memcpy(cmos + 0xa8, cmos_f, sizeof(cmos_f));
			// set result
			AX = 0xff;
			CX = 0;
			BX = 2;
			*ZeroFlag = 1;
			*CarryFlag = 0;
			return true;
		}
		else if(AH == 0x82) {
			// pseudo bios: boot from scsi-hdd #0
			timeout = 0;
			if(!scsi_blocks[0]) {
				*CarryFlag = 1;
				return true;
			}
			FILEIO* fio = new FILEIO();
			if(!fio->Fopen(scsi_path[drv], FILEIO_READ_BINARY)) {
				*CarryFlag = 1;
				delete fio;
				return true;
			}
			// load ipl
			access_scsi = true;
			fio->Fread(buffer, BLOCK_SIZE * 4, 1);
			fio->Fclose();
			delete fio;
			// check ipl
			if(!(buffer[0] == 'I' && buffer[1] == 'P' && buffer[2] == 'L' && buffer[3] == '1')) {
				*CarryFlag = 1;
				return true;
			}
			// data transfer
			for(int i = 0; i < BLOCK_SIZE * 4; i++)
				d_mem->write_data8(0xb0000 + i, buffer[i]);
			// clear screen
			_memset(cvram, 0, 0x1000);
			_memset(kvram, 0, 0x1000);
			// set cmos
//			_memcpy(cmos + 0xa8, cmos_s, sizeof(cmos_s));
			// set result
			AX = 0xffff;
			CX = 0;
			BX = 1;
			*ZeroFlag = 1;
			*CarryFlag = 0;
			return true;
		}
	}
	else if(PC == cmos_pc) {
		// cmos
#ifdef _DEBUG_LOG
		emu->out_debug("%6x\tCMOS BIOS: AH=%2x,AL=%2x,CX=%4x,DX=%4x,BX=%4x,DS=%2x,DI=%2x\n", vm->get_prv_pc(), AH,AL,CX,DX,BX,DS,DI);
#endif
		if(AH == 0) {
			// init cmos
			_memcpy(cmos + 0x000, cmos_t, sizeof(cmos_t));
			_memcpy(cmos + 0x7d1, cmos_b, sizeof(cmos_b));
		}
		else if(AH == 5) {
			// get $a2
			BX = cmos[0xa2] | (cmos[0xa3] << 8);
		}
		else if(AH == 10) {
			// memory to cmos
			int block = AL * 10;
			int len = cmos[block + 6] | (cmos[block + 7] << 8);
			int dst = cmos[block + 8] | (cmos[block + 9] << 8);
			int src = DS * 16 + DI;
			for(int i = 0; i < len; i++)
				cmos[dst++] = d_mem->read_data8(src++);
		}
		else if(AH == 11) {
			// cmos to memory
			int block = AL * 10;
			int len = cmos[block + 6] | (cmos[block + 7] << 8);
			int src = cmos[block + 8] | (cmos[block + 9] << 8);
			int dst = DS * 16 + DI;
			for(int i = 0; i < len; i++)
				d_mem->write_data8(dst++, cmos[src++]);
		}
		else if(AH == 20) {
			// check block header
			BX = 0;
		}
		AH = 0;
		*CarryFlag = 0;
		return true;
	}
	else if(PC == wait_pc) {
		// wait
#ifdef _DEBUG_LOG
		emu->out_debug("%6x\tWAIT BIOS: AH=%2x,AL=%2x,CX=%4x,DX=%4x,BX=%4x,DS=%2x,DI=%2x\n", vm->get_prv_pc(), AH,AL,CX,DX,BX,DS,DI);
#endif
		*CarryFlag = 0;
		return true;
	}
	return false;
}

bool BIOS::bios_int(int intnum, uint16 regs[], uint16 sregs[], int32* ZeroFlag, int32* CarryFlag)
{
	if(intnum == 0x93)
		return bios_call(0xfffc4, regs, sregs, ZeroFlag, CarryFlag);
	return false;
}

uint32 BIOS::read_signal(int ch)
{
	// get access status
	uint32 stat = 0;
	for(int i = 0; i < MAX_DRIVE; i++) {
		if(access_fdd[i])
			stat |= 1 << i;
		access_fdd[i] = false;
	}
	if(access_scsi)
		stat |= 0x10;
	access_scsi = false;
	return stat;
}

