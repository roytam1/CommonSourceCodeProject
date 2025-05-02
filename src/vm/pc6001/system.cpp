//
// PC-6001/6601 disk I/O
// This file is based on a disk I/O program in C++
// by Mr. Yumitaro and translated into C for Cocoa iP6
// by Koichi NISHIDA 2006
//

/*
	NEC PC-6001 Emulator 'yaPC-6001'
	NEC PC-6001mk2 Emulator 'yaPC-6201'
	NEC PC-6601 Emulator 'yaPC-6601'
	PC-6801 Emulator 'PC-6801'

	Author : tanam
	Date   : 2013.12.04-

	[ system ]
*/

#include "d88.h"
#include "system.h"

// mini floppy disk information
typedef struct {
	int ATN;				// attention
	int DAC;				// data accepted
	int RFD;				// ready for data
	int DAV;				// data valid
	int command;			// received command
	int step;				// status for waiting parameter
	int blk;				// block number
	int drv;				// drive number - 1
	int trk;				// track number
	int sct;				// sector number
	int size;				// byte number to process
	unsigned char retdat;	// return from port D0H
} DISK60;

// command
enum FddCommand
{
	INIT				= 0x00,
	WRITE_DATA			= 0x01,
	READ_DATA			= 0x02,
	SEND_DATA			= 0x03,
	COPY				= 0x04,
	FORMAT				= 0x05,
	SEND_RESULT_STATUS	= 0x06,
	SEND_DRIVE_STATUS	= 0x07,
	TRANSMIT			= 0x11,
	RECEIVE				= 0x12,
	LOAD				= 0x14,
	SAVE				= 0x15,
	WAIT				= 0xff,	// waiting state
	EndofFdcCmd
};

static char FilePath[2][FILENAME_MAX];	// file path
static int Sys[2];						// system disk
static int isMount[2];				// is mounted?
static DISK60 mdisk;				// mini floppy disk info
static unsigned char io_D1H;
static unsigned char io_D2H;
static unsigned char io_D3H;
static int DrvNum = 1;

// mount disk
int DiskMount(int drvno, char *filename)
{
	if( drvno >= DrvNum ) return 0;
	if (isMount[drvno]) {
		Unmount88(drvno); 
		isMount[drvno] = 0;
	}
	if(!Mount88(drvno, filename)) return 0;
	isMount[drvno] = 1;
	strcpy(FilePath[drvno], filename);
	// system disk check
	Seek88(drvno, 0, 1);
	Sys[drvno] = (Getc88(drvno) == 'S' && Getc88(drvno) == 'Y' && Getc88(drvno) == 'S');
	Seek88(drvno, 0, 1);	// for safty
	return 1;
}

// unmount disk
void DiskUnmount(int drvno)
{
	if( drvno >= DrvNum ) return;
	Unmount88(drvno);
	isMount[drvno] = 0;
	*FilePath[drvno] = '\0';
	Sys[drvno] = 0;
}

// is mounted ?
int DiskIsMount(int drvno)
{
	if( drvno >= DrvNum ) return 0;
	return isMount[drvno];
}

// is system disk ?
int DiskIsSystem(int drvno)
{
	if( drvno >= DrvNum ) return 0;
	return Sys[drvno];
}

// is protected ?
int DiskIsProtect(int drvno)
{
	if(!DiskIsMount(drvno)) return 0;
	return IsProtect88(drvno);
}

// get file path
char *DiskGetFile(int drvno)
{
	return FilePath[drvno];
}

// get disk name
char *DiskGetName(int drvno)
{
	if(!DiskIsMount(drvno)) return "";
	
	return GetDiskImgName88(drvno);
}

// initialise disk
int DiskInit60()
{
    memset(&mdisk, 0, sizeof(DISK60));
	mdisk.command = WAIT;		// received command
	mdisk.retdat  = 0xff;		// data from port D0H
	io_D1H = 0;
	io_D2H = 0xf0 | 0x08 /* (mdisk.ATN<<3) */ | (mdisk.DAC<<2) | (mdisk.RFD<<1) | mdisk.DAV;
	io_D3H = 0;
	return 1;
}

// data input (port D0H)
static unsigned char FddIn60()
{
	unsigned char ret;

	if (mdisk.DAV) {		// if data is valid
		if (mdisk.step == 6) {
			mdisk.retdat = Getc88(mdisk.drv);
			if(--mdisk.size == 0) mdisk.step = 0;
		}
		mdisk.DAC = 1;
		ret = mdisk.retdat;
	} else {			// if data is not valid
		ret = 0xff;
	}
	return ret;
}

// data/command output (port D1H)
static void FddOut60(unsigned char dat)
{
	if (mdisk.command == WAIT) {	// when command
		mdisk.command = dat;
		switch (mdisk.command) {
		case INIT:					// 00h init
			break;
		case WRITE_DATA:			// 01h write data
			mdisk.step = 1;
			break;
		case READ_DATA:				// 02h read data
			mdisk.step = 1;
			break;
		case SEND_DATA:				// 03h send data
			mdisk.step = 6;
			break;
		case COPY:					// 04h copy
			break;
		case FORMAT:				// 05h format
			break;
		case SEND_RESULT_STATUS:	// 06h send result status
			mdisk.retdat = 0x40;
			break;
		case SEND_DRIVE_STATUS:		// 07h send drive status
			mdisk.retdat |= 0x0a;
			break;
		case TRANSMIT:				// 11h transnmit
			break;
		case RECEIVE:				// 12h receive
			break;
		case LOAD:					// 14h load
			break;
		case SAVE:					// 15h save
			break;
		}
	}else{					// when data
		switch (mdisk.command) {
		case WRITE_DATA:			// 01h write data
			switch (mdisk.step) {
			case 1:	// 01h:block number
				mdisk.blk = dat;
				mdisk.size = mdisk.blk*256;
				mdisk.step++;
				break;
			case 2:	// 02h:drive number - 1
				mdisk.drv = dat;
				mdisk.step++;
				break;
			case 3:	// 03h:track number
				mdisk.trk = dat;
				mdisk.step++;
				break;
			case 4:	// 04h:sector number
				mdisk.sct = dat;
				// double drack number(1D->2D)
				Seek88(mdisk.drv, mdisk.trk*2, mdisk.sct);
				mdisk.step++;
				break;
			case 5:	// 05h:write data
				Putc88(mdisk.drv, dat);
				if( --mdisk.size == 0 ){
					mdisk.step = 0;
				}
				break;
			}
			break;
		case READ_DATA:				// 02h read data
			switch (mdisk.step) {
			case 1:	// 01h:block number
				mdisk.blk = dat;
				mdisk.size = mdisk.blk*256;
				mdisk.step++;
				break;
			case 2:	// 02h:drive number-1
				mdisk.drv = dat;
				mdisk.step++;
				break;
			case 3:	// 03h:track number
				mdisk.trk = dat;
				mdisk.step++;
				break;
			case 4:	// 04h:sector number
				mdisk.sct = dat;
				// double track number(1D->2D)
				Seek88(mdisk.drv, mdisk.trk*2, mdisk.sct);
				mdisk.step = 0;
				break;
			}
		}
	}
}

// control input from disk unit (port D2H)
static unsigned char FddCntIn60(void)
{
	static int old;

	if ((old & 0x01) ^ mdisk.DAV || mdisk.RFD && mdisk.DAV) {
		mdisk.DAC = mdisk.DAV;
	} else
	if (mdisk.ATN) {
		mdisk.RFD = 1;
		mdisk.command = WAIT;
	}
	else if (mdisk.DAC) {
		mdisk.DAV = 0;
	}
	else if (mdisk.RFD) {
		mdisk.DAV = 1;
	}	
	old = io_D2H;
	io_D2H = 0xf0 | 0x08 /* (mdisk.ATN<<3) */ | (mdisk.DAC<<2) | (mdisk.RFD<<1) | mdisk.DAV;
	return (io_D2H);
}

// control output to disk unit (port D3H)
static void FddCntOut60(unsigned char dat)
{
	// 8255 basic behavior
	if (!(dat&0x80)) {		// check msb
							// ignore when 1
		switch ((dat>>1)&0x07) {
		case 7:	// bit7 ATN
			mdisk.ATN = dat&1;
			break;
		case 6:	// bit6 DAC
			mdisk.DAC = dat&1;
			break;
		case 5:	// bit5 RFD
			mdisk.RFD = dat&1;
			break;
		case 4:	// bit4 DAV
			mdisk.DAV = dat&1;
			break;
		}
		io_D2H = 0xf0 | 0x08 /* (mdisk.ATN<<3) */ | (mdisk.DAC<<2) | (mdisk.RFD<<1) | mdisk.DAV;
	}
}

// I/O access functions
void OutD1H_60(unsigned char data) { io_D1H = data; FddOut60(io_D1H); }
void OutD2H_60(unsigned char data) {
	mdisk.ATN = (data & 0x80) >> 7;
	mdisk.DAC = (data & 0x40) >> 6;
	mdisk.RFD = (data & 0x20) >> 5;
	mdisk.DAV = (data & 0x10) >> 4;
	io_D2H = 0xf0 | 0x08 /* (mdisk.ATN<<3) */ | (mdisk.DAC<<2) | (mdisk.RFD<<1) | mdisk.DAV;
}
void OutD3H_60(unsigned char data) { io_D3H = data; FddCntOut60(io_D3H); }

unsigned char InD0H_60() { return FddIn60(); }
unsigned char InD1H_60() { return io_D1H; }
unsigned char InD2H_60() { io_D2H = FddCntIn60(); return io_D2H; }
unsigned char InD3H_60() { return io_D3H; }


// data buffer (256BytesX4)
static unsigned char Data[4][256];
static int Index[4] = {0, 0, 0, 0};

typedef struct {
	unsigned char Data[10];
	int Index;
} CmdBuffer;
	
static CmdBuffer CmdIn;					// command buffer
static CmdBuffer CmdOut;				// status buffer
static unsigned char SeekST0;			// ST0 when SEEK
static unsigned char LastCylinder;		// last read cylinder
static int SeekEnd;						// complete seek flag
static unsigned char SendSectors;		// amount(100H unit)
static int DIO;							// data direction TRUE: Buffer->CPU FALSE: CPU->Buffer
static unsigned char Status;			// FDC status register

// push data to data buffer
static void Push(int part, unsigned char data)
{
	if (part > 3) return;
	
	if(Index[part] < 256) Data[part][Index[part]++] = data;
}

// pop data from data buffer
static unsigned char Pop(int part)
{
	if(part > 3) return 0xff;
	
	if(Index[part] > 0) return Data[part][--Index[part]];
	else                return 0xff;
}

// clear data
static void Clear(int i)
{
	Index[i] = 0;
}

// FDC Status
#define FDC_BUSY			(0x10)
#define FDC_READY			(0x00)
#define FDC_NON_DMA			(0x20)
#define FDC_FD2PC			(0x40)
#define FDC_PC2FD			(0x00)
#define FDC_DATA_READY		(0x80)

// Result Status 0
#define ST0_NOT_READY		(0x08)
#define ST0_EQUIP_CHK		(0x10)
#define ST0_SEEK_END		(0x20)
#define ST0_IC_NT			(0x00)
#define ST0_IC_AT			(0x40)
#define ST0_IC_IC			(0x80)
#define ST0_IC_AI			(0xc0)

// Result Status 1
#define ST1_NOT_WRITABLE	(0x02)

// Result Status 2

// Result Status 3
#define ST3_TRACK0			(0x10)
#define ST3_READY			(0x20)
#define ST3_WRITE_PROTECT	(0x40)
#define ST3_FAULT			(0x80)

// initialise
int DiskInit66(void)
{
	memset( &CmdIn,  0, sizeof( CmdBuffer ) );
	memset( &CmdOut, 0, sizeof( CmdBuffer ) );
	SeekST0 = 0;
	LastCylinder = 0;
	SeekEnd = 0;
	SendSectors  = 0;
	Status = FDC_DATA_READY | FDC_READY | FDC_PC2FD;
	isMount[0] = isMount[1] = 0;
	return 1;
}

// push data to status buffer
static void PushStatus(int data)
{
	CmdOut.Data[CmdOut.Index++] = data;
}

// pop data from status buffer
static unsigned char PopStatus()
{
	return CmdOut.Data[--CmdOut.Index];
}

static void Exec();

// write to FDC
static void OutFDC(unsigned char data)
{
	const int CmdLength[] = { 0,0,0,3,2,9,9,2,1,0,0,0,0,6,0,3 };
	
	CmdIn.Data[CmdIn.Index++] = data;
	if (CmdLength[CmdIn.Data[0]&0xf] == CmdIn.Index) Exec();
}

// read from FDC
static unsigned char InFDC()
{
	if (CmdOut.Index == 1) Status = FDC_DATA_READY | FDC_PC2FD;
	return PopStatus();
}

// read
static void Read()
{
	int Drv, C, H, R, N;
	int i, j;
	
	Drv = CmdIn.Data[1]&3;		// drive number No.(0-3)
	C   = CmdIn.Data[2];		// cylinder
	H   = CmdIn.Data[3];		// head address
	R   = CmdIn.Data[4];		// sector No.
	N   = CmdIn.Data[5] ? CmdIn.Data[5]*256 : 256;	// sector size
	
	if (DiskIsMount(Drv)) {
		// seek
		// double track number(1D->2D)
		Seek88(Drv, C*2, R);
		for (i = 0; i < SendSectors; i++) {
			Clear(i);
			for(j=0; j<N; j++)
				Push(i, Getc88(Drv));
		}
	}
	PushStatus(N);	// N
	PushStatus(R);	// R
	PushStatus(H);	// H
	PushStatus(C);	// C
	PushStatus(0);	// st2
	PushStatus(0);	// st1
	PushStatus(DiskIsMount(Drv) ? 0 : ST0_NOT_READY);	// st0  bit3 : media not ready
	Status = FDC_DATA_READY | FDC_FD2PC;
}

// Write
static void Write(void)
{
	int Drv, C, H, R, N;
	int i, j;
	
	Drv = CmdIn.Data[1]&3;		// drive No.(0-3)
	C   = CmdIn.Data[2];		// cylinder
	H   = CmdIn.Data[3];		// head address
	R   = CmdIn.Data[4];		// sector No.
	N   = CmdIn.Data[5] ? CmdIn.Data[5]*256 : 256;	// sector size
	
	if (DiskIsMount(Drv)) {
		// seek
		// double track number(1D->2D)
		Seek88(Drv, C*2, R);
		for (i=0; i<SendSectors; i++) {
			for(j=0; j<0x100; j++)
				Putc88(Drv, Pop(i));	// write data
		}
	}
	
	PushStatus(N);	// N
	PushStatus(R);	// R
	PushStatus(H);	// H
	PushStatus(C);	// C
	PushStatus(0);	// st2
	PushStatus(0);	// st1
	
	PushStatus(DiskIsMount(Drv) ? 0 : ST0_NOT_READY);	// st0  bit3 : media not ready
	
	Status = FDC_DATA_READY | FDC_FD2PC;
}

// seek
void Seek(void)
{
	int Drv,C;
	
	Drv = CmdIn.Data[1]&3;		// drive No.(0-3)
	C   = CmdIn.Data[2];		// cylinder
	
	if (!DiskIsMount(Drv)) {	// disk unmounted ?
		SeekST0      = ST0_IC_AT | ST0_SEEK_END | ST0_NOT_READY | Drv;
		SeekEnd      = 0;
		LastCylinder = 0;
	} else { // seek
		// double number(1D->2D)
		Seek88(Drv, C*2, 1);
		SeekST0      = ST0_IC_NT | ST0_SEEK_END | Drv;
		SeekEnd      = 1;
		LastCylinder = C;
	}
}

// sense interrupt status
static void SenseInterruptStatus(void)
{
	if (SeekEnd) {
		SeekEnd = 0;
		PushStatus(LastCylinder);
		PushStatus(SeekST0);
	} else {
		PushStatus(0);
		PushStatus(ST0_IC_IC);
	}
	
	Status = FDC_DATA_READY | FDC_FD2PC;
}

// execute FDC command
void Exec()
{
	CmdOut.Index = 0;
	switch (CmdIn.Data[0] & 0xf) {
	case 0x03:	// Specify
		break;
	case 0x05:	// Write Data
		Write();
		break;
	case 0x06:	// Read Data
		Read();
		break;
	case 0x08:	// Sense Interrupt Status
		SenseInterruptStatus();
		break;
	case 0x0d:	// Write ID
		// Format is Not Implimented
		break;
	case 0x07:	// Recalibrate
		CmdIn.Data[2] = 0;	// Seek to TRACK0
	case 0x0f:	// Seek
		Seek();
		break;
	default: ;	// Invalid
	}
	CmdIn.Index = 0;
}

// I/O access functions
void OutB1H_66(unsigned char data) { DIO = data&2 ? 1 : 0; }			// FD mode
void OutB2H_66(unsigned char data) {}									// FDC INT?
void OutB3H_66(unsigned char data) {}									// in out of PortB2h
void OutD0H_66(unsigned char data) { Push(0, data); }					// Buffer
void OutD1H_66(unsigned char data) { Push(1, data); }					// Buffer
void OutD2H_66(unsigned char data) { Push(2, data); }					// Buffer
void OutD3H_66(unsigned char data) { Push(3, data); }					// Buffer
void OutD6H_66(unsigned char data) {}									// select drive
void OutD8H_66(unsigned char data) {}									//
void OutDAH_66(unsigned char data) { SendSectors = ~(data - 0x10); }	// set transfer amount
void OutDDH_66(unsigned char data) { OutFDC(data); }					// FDC data register
void OutDEH_66(unsigned char data) {}									// ?
	
unsigned char InB2H_66() { return 3; }									// FDC INT
unsigned char InD0H_66() { return Pop(0); }								// Buffer
unsigned char InD1H_66() { return Pop(1); }								// Buffer
unsigned char InD2H_66() { return Pop(2); }								// Buffer
unsigned char InD3H_66() { return Pop(3); }								// Buffer
unsigned char InD4H_66() { return 0; }									// Mortor(on 0/off 1)
unsigned char InDCH_66() { return Status; }								// FDC status register
unsigned char InDDH_66() { return InFDC(); }							// FDC data register

void SYSTEM::initialize()
{
	isMount[0] = isMount[1] = 0;
	Init88();
#if defined(_PC6601) || defined(_PC6801)
	DiskInit66();
#else
	DiskInit60();
#endif
}

void SYSTEM::write_io8(uint32 addr, uint32 data)
{
	// disk I/O
	uint16 port=(addr & 0x00ff);
	byte Value=(data & 0xff);
	
	switch(port)
	{
#if defined(_PC6601) || defined(_PC6801)
	// disk I/O
	case 0xB1:
		OutB1H_66(Value); break;
		break;
	case 0xB2:
		OutB2H_66(Value); break;
		break;
	case 0xB3:
		OutB3H_66(Value); break;
		break;
	case 0xD0:
		OutD0H_66(Value);
		break;
	case 0xD1:
		OutD1H_66(Value);
		break;
	case 0xD2:
		OutD2H_66(Value);
		break;
	case 0xD3:
		OutD3H_66(Value);
		break;
	case 0xD6:
		OutD6H_66(Value);
		break;
	case 0xD8:
		OutD8H_66(Value);
		break;
	case 0xDA:
		OutDAH_66(Value);
		break;
	case 0xDD:
		OutDDH_66(Value);
		break;
	case 0xDE:
		OutDEH_66(Value);
		break;
#else
	case 0xD1:
		OutD1H_60(Value);
		break;
	case 0xD2:
		OutD2H_60(Value);
		break;
	case 0xD3:
		OutD3H_60(Value);
		break;
#endif
	}
	return;
}

uint32 SYSTEM::read_io8(uint32 addr)
{
	// disk I/O
	uint16 port=(addr & 0x00ff);
	byte Value=0xff;
	
	switch(port)
	{
#if defined(_PC6601) || defined(_PC6801)
	case 0xB2:
		Value=InB2H_66();
		break;
	case 0xD0:
		Value=InD0H_66();
		break;
	case 0xD1:
		Value=InD1H_66();
		break;
	case 0xD2:
		Value=InD2H_66();
		break;
	case 0xD3:
		Value=InD3H_66();
		break;
	case 0xD4:
		Value=InD4H_66();
		break;
	case 0xDC:
		Value=InDCH_66();
		break;
	case 0xDD:
		Value=InDDH_66();
		break;
#else
	case 0xB2:
		Value=1;
		break;
	case 0xD0:
		Value=InD0H_60(); break;
		break;
	case 0xD1:
		Value=InD1H_60(); break;
		break;
	case 0xD2:
		Value=InD2H_60(); break;
		break;
	case 0xD3:
		Value=InD3H_60(); break;
		break;
#endif
	}
	return(Value);
}

void SYSTEM::open_disk(int drv, _TCHAR* file_path, int offset)
{
	if (drv < DrvNum) {
		DiskMount( drv, file_path );
	}
}

void SYSTEM::close_disk(int drv)
{
	if (drv < DrvNum) {
		DiskUnmount(drv);
	}
}

bool SYSTEM::disk_inserted(int drv)
{
	if (drv < DrvNum && isMount[drv])
		return true;
	return false;
}
