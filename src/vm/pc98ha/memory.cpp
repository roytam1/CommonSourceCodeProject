/*
	NEC PC-98HA Emulator 'eHandy98'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.06.10 -

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 14, eb = (e) >> 14; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) \
			wbank[i] = wdmy; \
		else \
			wbank[i] = (w) + 0x4000 * (i - sb); \
		if((r) == rdmy) \
			rbank[i] = rdmy; \
		else \
			rbank[i] = (r) + 0x4000 * (i - sb); \
	} \
}

void MEMORY::initialize()
{
	// init memory
	_memset(ram, 0, sizeof(ram));
	_memset(vram, 0, sizeof(vram));
	_memset(learn, 0, sizeof(vram));
	_memset(ipl, 0xff, sizeof(ipl));
	_memset(kanji, 0xff, sizeof(kanji));
	_memset(dic, 0xff, sizeof(dic));
	_memset(ramdrv, 0, sizeof(ramdrv));
	_memset(romdrv, 0xff, sizeof(romdrv));
	_memset(ems, 0xff, sizeof(ems));
	_memset(memcard, 0, sizeof(memcard));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	// load rom/ram image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sIPL.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ipl, sizeof(ipl), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sKANJI.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(kanji, sizeof(kanji), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sDICT.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(dic, sizeof(dic), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sRAMDRV.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ramdrv, sizeof(ramdrv), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sROMDRV.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(romdrv, sizeof(romdrv), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sMEMCARD.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(memcard, sizeof(memcard), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sLEARN.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(learn, sizeof(learn), 1);
		fio->Fclose();
	}
	delete fio;
	
	// set memory bank
	learn_bank = dic_bank = kanji_bank = 0;
	ramdrv_bank = romdrv_bank = memcard_bank = memcard_sel = 0;
	ems_bank[0] = 0; ems_bank[1] = 1; ems_bank[2] = 2; ems_bank[3] = 3;
	update_bank();
hoge=false;
}

void MEMORY::release()
{
	// save ram image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sRAMDRV.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(ramdrv, sizeof(ramdrv), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sMEMCARD.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(memcard, sizeof(memcard), 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sLEARN.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(learn, sizeof(learn), 1);
		fio->Fclose();
	}


	_stprintf(file_path, _T("%sRAMDUMP.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(ram, 0xa0000, 1);
		fio->Fclose();
	}


	delete fio;
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xfffff;
//if(hoge&&(addr&0xf0000)<0xa0000)emu->out_debug("WR %6x %2x\n", addr,data);
//if((addr&0xf0000)==0xc0000)emu->out_debug("WR %6x %2x\n", addr,data);
//if((addr&0xf0000)==0xd0000)emu->out_debug("WR %6x %2x\n", addr,data);
//if((addr&0xf0000)==0xe0000)emu->out_debug("WR %6x %2x\n", addr,data);
	wbank[addr >> 14][addr & 0x3fff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xfffff;
//	return rbank[addr >> 14][addr & 0x3fff];
	uint8 val = rbank[addr >> 14][addr & 0x3fff];
//if((addr&0xf0000)==0xc0000)emu->out_debug("RD %6x = %2x\n", addr,val);
//if((addr&0xf0000)==0xd0000)emu->out_debug("RD %6x = %2x\n", addr,val);
//if((addr&0xf0000)==0xe0000)emu->out_debug("RD %6x = %2x\n", addr,val);
	return val;
}

void MEMORY::write_dma8(uint32 addr, uint32 data)
{
	write_data8(addr, data);
}

uint32 MEMORY::read_dma8(uint32 addr)
{
	return read_data8(addr);
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xffff)
	{
	case 0x0c10:
		learn_bank = data & 0xf;
		update_bank();
		break;
	case 0x0e8e:
		ramdrv_bank = data & 0x7f;
//		memcard_bank = data;
		update_bank();
		break;
	case 0x1e8e:
		memcard_sel = data;
		update_bank();
		break;
	case 0x4c10:
		dic_bank = data & 0x3f;
		update_bank();
		break;
	case 0x6e8e:
		// modem control 1
		break;
	case 0x7e8e:
		// modem control 2
		break;
	case 0x8810:
		// power status
		// bit0 = 1: power off
		// bit2 = 1: stanby
		// bit5 = 1: unknown
		break;
	case 0x8c10:
		kanji_bank = data & 0xf;
		update_bank();
		break;
	case 0xcc10:
hoge=true;
		romdrv_bank = data & 0xf;
		update_bank();
		break;
	}
}

uint32 MEMORY::read_io8(uint32 addr)
{
	switch(addr & 0xffff)
	{
	case 0x0c10:
//		return 0x43;
		return learn_bank | 0x40;
	case 0x0e8e:
		return ramdrv_bank;
//		return memcard_bank;
	case 0x0f8e:
		// memcard id
		return 4;	// memcardなし
//		return 0xe;	// memcardあり
	case 0x1e8e:
		return memcard_sel;
	case 0x1f8e:
		// memcard id
		return 0;
	case 0x4c10:
		return dic_bank;// | 0x40;
	case 0x8810:
		// bit7 = 1: docking station
		// bit6 = 1: ac power supply
		// bit3 = 1: unknown
		// bit2 = 1: li.battery low
		// bit1 = 1: battery low
		// bit0 = 1: power off
		return 0x40;
	case 0x8c10:
		return kanji_bank;// | 0x40;
	case 0xc810:
		return 0x10;
	case 0xcc10:
		return romdrv_bank;// | 0x40;
	}
	return 0xff;
}

/*
			I/O	PAGE
	00000-9FFFF			RAM 640KB
	A8000-AFFFF			VRAM 32KB
	C0000-C3FFF			EMS0
	C4000-C7FFF			EMS1
	C8000-CBFFF			EMS2
	CC000-CFFFF			EMS3
	D0000-D3FFF	0C10	16	学習RAM 256KB
	D4000-D7FFF	4C10	48	辞書ROM 768KB
	D8000-DBFFF	8C10	16	漢字ROM 256KB
	DC000-DFFFF	0E8E	88	RAMディスク 1408KB
			1E8E
	E0000-EFFFF	CC10	16	ROMディスク 1MB
	F0000-FFFFF			IPL
*/

void MEMORY::update_bank()
{
	SET_BANK(0x00000, 0xfffff, wdmy, rdmy);
	
	SET_BANK(0x00000, 0x9ffff, ram, ram);
	SET_BANK(0xa8000, 0xaffff, vram, vram);
	SET_BANK(0xc0000, 0xc3fff, ems + 0x4000 * ems_bank[0], ems + 0x4000 * ems_bank[0]);
	SET_BANK(0xc4000, 0xc7fff, ems + 0x4000 * ems_bank[1], ems + 0x4000 * ems_bank[1]);
	SET_BANK(0xc8000, 0xcbfff, ems + 0x4000 * ems_bank[2], ems + 0x4000 * ems_bank[2]);
	SET_BANK(0xcc000, 0xcffff, ems + 0x4000 * ems_bank[3], ems + 0x4000 * ems_bank[3]);
	if(learn_bank < 16) {
		SET_BANK(0xd0000, 0xd3fff, learn + 0x4000 * learn_bank, learn + 0x4000 * learn_bank);
	}
	if(dic_bank < 48) {
		SET_BANK(0xd4000, 0xd7fff, wdmy, dic + 0x4000 * dic_bank);
	}
	if(kanji_bank < 16) {
		SET_BANK(0xd8000, 0xdbfff, wdmy, kanji + 0x4000 * kanji_bank);
	}
	if(memcard_sel == 0x82) {
		// memory card
		SET_BANK(0xdc000, 0xdffff, memcard + 0x4000 * memcard_bank, memcard + 0x4000 * memcard_bank);
	}
	else if(ramdrv_bank < 88) {
		// ram disk drive
		SET_BANK(0xdc000, 0xdffff, ramdrv + 0x4000 * ramdrv_bank, ramdrv + 0x4000 * ramdrv_bank);
	}
	if(romdrv_bank < 16) {
		SET_BANK(0xe0000, 0xeffff, wdmy, romdrv + 0x10000 * romdrv_bank);
	}
	SET_BANK(0xf0000, 0xfffff, wdmy, ipl);
}

