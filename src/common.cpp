/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2013.01.17-

	[ common ]
*/

#include "common.h"

bool check_file_extension(_TCHAR* filename, _TCHAR* ext)
{
	int nam_len = _tcslen(filename);
	int ext_len = _tcslen(ext);
	
	return (nam_len >= ext_len && _tcsncicmp(&filename[nam_len - ext_len], ext, ext_len) == 0);
}

uint32 getcrc32(uint8 data[], int size)
{
	static bool initialized = false;
	static uint32 table[256];
	
	if(!initialized) {
		for(int i = 0; i < 256; i++) {
			uint32 c = i;
			for(int j = 0; j < 8; j++) {
				if(c & 1) {
					c = (c >> 1) ^ 0xedb88320;
				} else {
					c >>= 1;
				}
			}
			table[i] = c;
		}
		initialized = true;
	}
	
	uint32 c = ~0;
	for(int i = 0; i < size; i++) {
		c = table[(c ^ data[i]) & 0xff] ^ (c >> 8);
	}
	return ~c;
}
