/*
	NEC PC-6001 Emulator 'yaPC-6001'

	Author : tanam
	Date   : 2013.07.15-

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

void MEMORY::reset()
{
	int J;
	// load rom image
	FILEIO* fio = new FILEIO();
#ifdef _TANAM
	if(fio->Fopen(emu->bios_path(_T("ROM/BASICROM.60")), FILEIO_READ_BINARY)) {
#else
	if(fio->Fopen(emu->bios_path(_T("BASICROM.60")), FILEIO_READ_BINARY)) {
#endif
		fio->Fread(BASICROM, 0x4000, 1);
		fio->Fclose();
	}
#ifdef _TANAM
	if(fio->Fopen(emu->bios_path(_T("ROM/EXTROM.60")), FILEIO_READ_BINARY)) {
#else
	if(fio->Fopen(emu->bios_path(_T("EXTROM.60")), FILEIO_READ_BINARY)) {
#endif
		fio->Fread(EXTROM, 0x4000, 1);
		fio->Fclose();
		EXTROM1 = EXTROM; EXTROM2 = EXTROM + 0x2000;
	} else {
		EXTROM1 = EXTROM2 = EmptyRAM;
	}
	delete fio;
	memset(RAM ,0,0x10000);	// clear RAM
	memset(EmptyRAM, 0, sizeof(EmptyRAM));
	for(J=0;J<4;J++) {RdMem[J]=BASICROM+0x2000*J;WrMem[J]=RAM+0x2000*J;};
	RdMem[2] = EXTROM1; RdMem[3] = EXTROM2;
	for(J=4;J<8;J++) {RdMem[J]=RAM+0x2000*J;WrMem[J]=RAM+0x2000*J;};
	EnWrite[0]=EnWrite[1]=0; EnWrite[2]=EnWrite[3]=1;
	inserted = false;
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	/* normal memory write ..*/
   	if(EnWrite[addr >> 14]) 
   		WrMem[addr >> 13][addr & 0x1FFF] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	/* normal memory read ..*/
   	return(RdMem[addr >> 13][addr & 0x1FFF]);
}

void MEMORY::open_cart(_TCHAR* file_path)
{
	FILEIO* fio = new FILEIO();
	
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(EXTROM, 0x4000, 1);
		fio->Fclose();
		inserted = true;
	} else {
		EXTROM1 = EXTROM2 = EmptyRAM;
	}
	delete fio;
}

void MEMORY::close_cart()
{
	EXTROM1 = EXTROM2 = EmptyRAM;
	inserted = false;
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	if (addr ==0xddf0) {
		RdMem[0]=RAM;
		RdMem[1]=RAM+0x2000;
		RdMem[2]=RAM+0x4000;
		RdMem[3]=RAM+0x6000;
		EnWrite[0]=EnWrite[1]=1;
	}
	if (addr ==0x11f0) {
		RdMem[0]=BASICROM;
		RdMem[1]=BASICROM+0x2000;
		RdMem[2]=EXTROM1;
		RdMem[3]=EXTROM2;
		EnWrite[0]=EnWrite[1]=0;
	}
}
