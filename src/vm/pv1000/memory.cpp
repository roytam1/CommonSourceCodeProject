/*
	CASIO PV-1000 Emulator 'ePV-1000'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.11.16 -

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"

#define SET_BANK(s, e, w, r) { \
	int sb = (s) >> 11, eb = (e) >> 11; \
	for(int i = sb; i <= eb; i++) { \
		wbank[i] = (w) + 0x800 * (i - sb); \
		rbank[i] = (r) + 0x800 * (i - sb); \
	} \
}

void MEMORY::initialize()
{
	// load bios
	_memset(mem         , 0xff, 0x8000);
	_memset(mem + 0x8000, 0x00, 0x8000);
	_memset(rdmy, 0xff, sizeof(rdmy));
	
	mem[0] = 0xc3;	// for debug
	mem[1] = 0x00;
	mem[2] = 0x00;
	
	// $b800-$baff	vram
	// $bb00-$bbff	work
	// $bc00-$bfff	pattern ? 8x8 * 128patterns
	SET_BANK(0x0000, 0x7fff, wdmy        , mem         );
	SET_BANK(0x8000, 0xb7ff, wdmy        , rdmy        );
	SET_BANK(0xb800, 0xbfff, mem + 0xb800, mem + 0xb800);
	SET_BANK(0xc000, 0xffff, wdmy        , rdmy        );
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	addr &= 0xffff;
	wbank[addr >> 11][addr & 0x7ff] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	addr &= 0xffff;
	return rbank[addr >> 11][addr & 0x7ff];
}

void MEMORY::open_cart(_TCHAR* filename)
{
	FILEIO* fio = new FILEIO();
	
	if(fio->Fopen(filename, FILEIO_READ_BINARY)) {
		_memset(mem, 0xff, 0x8000);
		// load 8kb
		fio->Fread(mem, 0x2000, 1);
		_memcpy(mem + 0x2000, mem, 0x2000);
		// load 8kb
		fio->Fread(mem + 0x2000, 0x2000, 1);
		_memcpy(mem + 0x4000, mem, 0x4000);
		// load 16kb
		fio->Fread(mem + 0x4000, 0x4000, 1);
		fio->Fclose();
	}
	delete fio;
}

void MEMORY::close_cart()
{
	_memset(mem, 0xff, 0x8000);
}
