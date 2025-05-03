/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.14-

	[ kanji rom ]
*/

#include "kanji.h"
#include "../../fileio.h"

void KANJI::initialize()
{
	// init image
	_memset(kanji, 0, sizeof(kanji));
	
	// load rom image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sANK16.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(kanji, 0x1000, 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sKANJI.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(kanji + 0x1000, 0x4ac00, 1);
		fio->Fclose();
	}
	// xmillenium rom
	_stprintf(file_path, _T("%sFNT0816.X1"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(kanji, 0x1000, 1);
		fio->Fclose();
	}
	_stprintf(file_path, _T("%sFNT1616.X1"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(kanji + 0x1000, 0x4ac00, 1);
		fio->Fclose();
	}
	delete fio;
}

void KANJI::reset()
{
	kaddr = ofs = flag = 0;
	pattern = &kanji[0];
}

void KANJI::write_io8(uint32 addr, uint32 data)
{
	switch(addr)
	{
	case 0xe80:
		kaddr = (kaddr & 0xff00) | data;
		break;
	case 0xe81:
		kaddr = (kaddr & 0xff) | (data << 8);
		break;
	case 0xe82:
		get_kanji_pattern(kaddr & 0xfff0);
		break;
	}
}

uint32 KANJI::read_io8(uint32 addr)
{
	switch(addr)
	{
	case 0xe80:
		if(kaddr & 0xff00) {
			uint32 val = pattern[ofs * 2 + 0];
			flag |= 1;
			if(flag == 3) {
				ofs = (ofs + 1) & 15;
				flag = 0;
			}
			return val;
		}
		return jis2addr(kaddr << 8) >> 8;
	case 0xe81:
		if(kaddr & 0xff00) {
			uint32 val = pattern[ofs * 2 + 1];
			flag |= 2;
			if(flag == 3) {
				ofs = (ofs + 1) & 15;
				flag = 0;
			}
			return val;
		}
		return 0;
	}
	return 0xff;
}

uint16 KANJI::jis2addr(uint16 code)
{
	uint16 val;
	uint8 l = code & 0xff;
	uint8 h = code >> 8;
	if(h > 0x28)
		val = 0x4000 + (h - 0x30) * 0x600;
	else
		val = 0x0100 + (h - 0x21) * 0x600;
	if(l >= 0x20)
		val += (l - 0x20) * 0x10;
	return val;
}

void KANJI::get_kanji_pattern(uint16 addr)
{
	uint16 l, h;
	
	// addr -> jis
	if(addr < 0x4000)
		h = 0x21 + (addr - 0x100) / 0x600;
	else
		h = 0x30 + (addr - 0x4000) / 0x600;
	if(h > 0x28)
		addr -= 0x4000 + (h - 0x30) * 0x600;
	else
		addr -= 0x0100 + (h - 0x21) * 0x600;
	if(addr)
		l = 0x20 + addr / 0x10;
	else
		l = 0x20;
	if(!(l || h)) {
		pattern = &kanji[0];
		return;
	}
	
	// jis -> sjis
	if(h & 1) {
		l += 0x1f;
		if(l >= 0x7f)
			l++;
	}
	else
		l += 0x7e;
	h = (h - 0x21) / 2 + 0x81;
	if(h >= 0xa0)
		h += 0x40;
	uint32 sjis = ((h & 0xff) << 8) | (l & 0xff);
	
	// sjis -> font
	if(sjis < 0x100)
		pattern = &kanji[sjis * 16];
	else if(0x8140 <= sjis && sjis < 0x84c0)
		pattern = &kanji[0x01000 + (sjis - 0x8140) * 32];
	else if(0x8890 <= sjis && sjis < 0xa000)
		pattern = &kanji[0x08000 + (sjis - 0x8890) * 32];
	else if(0xe040 <= sjis && sjis < 0xeab0)
		pattern = &kanji[0x36e00 + (sjis - 0xe040) * 32];
	else
		pattern = &kanji[0];
}
