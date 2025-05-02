/*
	EPOCH Super Cassette Vision Emulator 'eSCV'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.21 -

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define MEM_WAIT_VDP_B 0
#define MEM_WAIT_VDP_W 0

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 7, eb = (e) >> 7; \
	for(int i = sb; i <= eb; i++) { \
		if((w) == wdmy) { \
			wbank[i] = wdmy; \
		} \
		else { \
			wbank[i] = (w) + 0x80 * (i - sb); \
		} \
		if((r) == rdmy) { \
			rbank[i] = rdmy; \
		} \
		else { \
			rbank[i] = (r) + 0x80 * (i - sb); \
		} \
	} \
}

void MEMORY::initialize()
{
	// load bios
	_memset(bios, 0xff, sizeof(bios));
	_memset(cart, 0xff, sizeof(cart));
	_memset(sram, 0xff, sizeof(sram));
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sBIOS.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(bios, 0x1000, 1);
		fio->Fclose();
	}
	delete fio;
	set_bank(0);
	
	// cart is not opened
	_memset(&header, 0, sizeof(header_t));
}

void MEMORY::release()
{
	close_cart();
}

void MEMORY::reset()
{
	// clear memory
	_memset(vram, 0, sizeof(vram));
	for(int i = 0x1000; i < 0x1200; i += 32) {
		static uint8 tmp[32] = {
			0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
			0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
		};
		_memcpy(vram + i, tmp, 32);
	}
	_memset(wreg, 0, sizeof(wreg));
	
	// initialize memory bank
	set_bank(0);
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	if(addr == 0x3600) {
		d_sound->write_data8(addr, data);
	}
	if((addr & 0xfe00) == 0x3400) {
		wbank[0x68][addr & 3] = data;
	}
	else {
		wbank[addr >> 7][addr & 0x7f] = data;
	}
}

uint32 MEMORY::read_data8(uint32 addr)
{
	return rbank[addr >> 7][addr & 0x7f];
}

void MEMORY::write_data16(uint32 addr, uint32 data)
{
	write_data8(addr, data & 0xff);
	write_data8(addr + 1, data >> 8);
}

uint32 MEMORY::read_data16(uint32 addr)
{
	return read_data8(addr) | (read_data8(addr + 1) << 8);
}

void MEMORY::write_data8w(uint32 addr, uint32 data, int* wait)
{
	*wait = (0x2000 <= addr && addr < 0x3600) ? MEM_WAIT_VDP_B : 0;
	write_data8(addr, data);
}

uint32 MEMORY::read_data8w(uint32 addr, int* wait)
{
	*wait = (0x2000 <= addr && addr < 0x3600) ? MEM_WAIT_VDP_B : 0;
	return read_data8(addr);
}

void MEMORY::write_data16w(uint32 addr, uint32 data, int* wait)
{
	*wait = (0x2000 <= addr && addr < 0x3600) ? MEM_WAIT_VDP_W : 0;
	write_data8(addr, data & 0xff);
	write_data8(addr + 1, data >> 8);
}

uint32 MEMORY::read_data16w(uint32 addr, int* wait)
{
	*wait = (0x2000 <= addr && addr < 0x3600) ? MEM_WAIT_VDP_W : 0;
	return read_data8(addr) | (read_data8(addr + 1) << 8);
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	set_bank((data >> 5) & 3);
}

void MEMORY::set_bank(uint8 bank)
{
	// $0000-$0fff : cpu internal rom
	// $2000-$3fff : vram
	// $8000-$ff7f : cartridge rom
	// ($e000-$ff7f : 8kb sram)
	// $ff80-$ffff : cpu internam ram
	
	SET_BANK(0x0000, 0x0fff, wdmy, bios);
	SET_BANK(0x1000, 0x1fff, wdmy, rdmy);
	SET_BANK(0x2000, 0x3fff, vram, vram);
	SET_BANK(0x4000, 0x7fff, wdmy, rdmy);
	SET_BANK(0x8000, 0xff7f, wdmy, cart + 0x8000 * bank);
	if(header.ctype == 1 && (bank & 1)) {
		SET_BANK(0xe000, 0xff7f, sram, sram);
	}
	SET_BANK(0xff80, 0xffff, wreg, wreg);
}

void MEMORY::open_cart(_TCHAR* filename)
{
	// close cart and initialize memory
	close_cart();
	
	// get save file path
	_tcscpy(save_path, filename);
	int len = _tcslen(save_path);
	if(save_path[len - 4] == _T('.')) {
		save_path[len - 3] = _T('S');
		save_path[len - 2] = _T('A');
		save_path[len - 1] = _T('V');
	}
	else
		_stprintf(save_path, _T("%s.SAV"), filename);
	
	// open cart and backuped sram
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(filename, FILEIO_READ_BINARY)) {
		// load header
		fio->Fread(&header, sizeof(header_t), 1);
		if(!(header.id[0] == 'S' && header.id[1] == 'C' && header.id[2] == 'V' && header.id[3] == 0x1a)) {
			// failed to load header
			_memset(&header, 0, sizeof(header_t));
			fio->Fseek(0, FILEIO_SEEK_SET);
		}
		
		// load rom image, PC5=0, PC6=0
		fio->Fread(cart, 0x8000, 1);
		_memcpy(cart + 0x8000, cart, 0x8000);
		
		// load rom image, PC5=1, PC6=0
		if(header.ctype == 0) {
			fio->Fread(cart + 0xe000, 0x2000, 1);
		}
		else if(header.ctype == 2) {
			fio->Fread(cart + 0x8000, 0x8000, 1);
		}
		_memcpy(cart + 0x10000, cart, 0x10000);
		
		// load rom image, PC5=0/1, PC6=1
		if(header.ctype == 2) {
			fio->Fread(cart + 0x10000, 0x10000, 1);
		}
		else if(header.ctype == 3) {
			fio->Fread(cart + 0x10000, 0x8000, 1);
			_memcpy(cart + 0x18000, cart + 0x10000, 0x8000);
		}
		fio->Fclose();
		
		// load backuped sram
		// note: shogi nyumon has sram but it is not battery-backuped
		if(header.ctype == 1 && cart[1] != 0x48 && fio->Fopen(save_path, FILEIO_READ_BINARY)) {
			fio->Fread(sram, 0x2000, 1);
			fio->Fclose();
		}
	}
	delete fio;
}

void MEMORY::close_cart()
{
	// save backuped sram
	// note: shogi nyumon has sram but it is not battery-backuped
	if(header.ctype == 1 && cart[1] != 0x48) {
		FILEIO* fio = new FILEIO();
		if(fio->Fopen(save_path, FILEIO_WRITE_BINARY)) {
			fio->Fwrite(sram, 0x2000, 1);
			fio->Fclose();
		}
		delete fio;
	}
	
	// initialize memory
	_memset(&header, 0, sizeof(header_t));
	_memset(cart, 0xff, sizeof(cart));
	_memset(sram, 0xff, sizeof(sram));
}
