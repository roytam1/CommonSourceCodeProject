//
// PC-6001/6601 disk I/O
// This file is based on a disk I/O program in C++
// by Mr. Yumitaro and translated into C for Cocoa iP6
// by Koichi NISHIDA 2006
//

/*
	NEC PC-6601 Emulator 'yaPC-6601'

	Author : tanam
	Date   : 2013.12.04-

	[ d88 ]
*/

#ifndef D88_H_INCLUDED
#define D88_H_INCLUDED

void Init88();
int Mount88(int, char *);			// mount
void Unmount88(int);				// unmount
unsigned char Getc88(int);			// read 1byte
int Putc88(int, unsigned char);		// write 1byte
int Seek88(int, int, int);			// seek
unsigned long GetCHRN88(int);		// get CHRN
char *GetFileName88(int);			// get file name
char *GetDiskImgName88(int);		// get disk image name
int IsProtect88(int);				// get protect status

#endif	// D88_H_INCLUDED
