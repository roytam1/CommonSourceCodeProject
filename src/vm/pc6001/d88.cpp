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

#include <stdio.h>
#include <string.h>
#include <limits.h>

// disable warnings C4996 for microsoft visual c++ 2005
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#pragma warning( disable : 4996 )
#endif

#include "D88.h"

// D88 sector information
typedef struct {
	unsigned char c;			// C of ID (cylinder No, when one side, track No)
	unsigned char h;			// H of ID (header address when one side, 0)
	unsigned char r;			// R of ID (sector No in a track)
	unsigned char n;			// N of ID (sector size 0:256 1:256 2:512 3:1024)
	unsigned short sec_nr;		// sector number of this track
	unsigned char density;		// density     0x00:double  0x40:single
	unsigned char deleted;		// deleted mark 0x00:normal 0x10:deleted
	unsigned char status;		// status
	unsigned char reserve[5];	// reserve
	unsigned short size;		// data size of this sector part
	unsigned long data;			// offset to data
	unsigned short offset;		// offset to the next data from first sector
} D88SECTOR;
	
// D88 information
typedef struct {
	unsigned char name[17];		// disk name(ASCII + '\0')
	unsigned char reserve[9];	// reserve
	unsigned char protect;		// write protect  0x00:yes 0x10:no
	unsigned char type;			// disk kind      0x00:2D   0x10:2DD  0x20:2HD
	unsigned long size;			// disk sise      (header part + all tracks)
	unsigned long table[164];	// offset table of track part (Track 0-163)
	D88SECTOR secinfo;			// sector info
	FILE *fp;					// FILE pointer
} D88INFO;

static D88INFO d88[2];						// D88 info
static char FileName88[2][FILENAME_MAX];   // file name buffer
static int Protected88[2];					// protect

// read D88 header
static int ReadHeader88(int drvno)
{
	if (d88[drvno].fp) {
		int i;        
		// disk name
		fread(d88[drvno].name, sizeof(unsigned char), 17, d88[drvno].fp);
		d88[drvno].name[16] = '\0';
		// read reserve
		if (fread(d88[drvno].reserve, 1, 9, d88[drvno].fp)<9) return 0;
		// write protect
		if (feof(d88[drvno].fp)) return 0;
		d88[drvno].protect = fgetc(d88[drvno].fp);
		if(d88[drvno].protect) Protected88[drvno] = 1;
		else if(Protected88[drvno]) d88[drvno].protect = 0x10;
		// disk kind
		if (feof(d88[drvno].fp)) return 0;
		d88[drvno].type = fgetc(d88[drvno].fp);
		// disk size
		if (feof(d88[drvno].fp)) return 0;
		d88[drvno].size = fgetc(d88[drvno].fp);
		d88[drvno].size += (fgetc(d88[drvno].fp)<<8);
		d88[drvno].size += (fgetc(d88[drvno].fp)<<16);
		d88[drvno].size += (fgetc(d88[drvno].fp)<<24);
		// offset table of the track part
		for (i=0; i<164; i++) {
			if (feof(d88[drvno].fp)) return 0;		
			d88[drvno].table[i] = fgetc(d88[drvno].fp);
			d88[drvno].table[i] += (fgetc(d88[drvno].fp)<<8);
			d88[drvno].table[i] += (fgetc(d88[drvno].fp)<<16);
			d88[drvno].table[i] += (fgetc(d88[drvno].fp)<<24);
		}
		return 1;
	} else return 0;
}

// initialise
void Init88()
{
	memset(d88, 0, sizeof(D88INFO) * 2);
}

// mount
int Mount88(int drvno, char *fname)
{
	strcpy(FileName88[drvno], fname);
	// open file
	// open with read & write
	if (!(d88[drvno].fp=fopen( FileName88[drvno], "r+b"))) {
		// if No, open with read only
		if (!(d88[drvno].fp=fopen(FileName88[drvno], "rb"))) {
			*FileName88[drvno] = 0;
			return 0;
		} else
			Protected88[drvno] = 1;	// protect is yes
	}
	return ReadHeader88(drvno);	// read D88 header
}

// unmount
void Unmount88(int drvno)
{
	if (d88[drvno].fp) fclose(d88[drvno].fp);
	d88[drvno].fp = NULL;
}

// read D88 sector information
static void ReadSector88(int drvno)
{
	if (d88[drvno].fp) {
		d88[drvno].secinfo.c = fgetc(d88[drvno].fp);				// C of ID (cylinder No, when one side, track No)
		d88[drvno].secinfo.h = fgetc(d88[drvno].fp);				// H of ID (header address, when one side, 0)
		d88[drvno].secinfo.r = fgetc(d88[drvno].fp);				// R of ID (sector No in a track)
		d88[drvno].secinfo.n = fgetc(d88[drvno].fp);				// N of ID (sector size 0:256 1:256 2:512 3:1024)
		d88[drvno].secinfo.sec_nr = fgetc(d88[drvno].fp);		// sector number in this track
		d88[drvno].secinfo.sec_nr += (fgetc(d88[drvno].fp)<<8);			// data size of this sector part
		d88[drvno].secinfo.density = fgetc(d88[drvno].fp);		// density     0x00:double  0x40:single
		d88[drvno].secinfo.deleted = fgetc(d88[drvno].fp);		// DELETED MARK 0x00:normal 0x10:deleted
		d88[drvno].secinfo.status = fgetc(d88[drvno].fp);			// status
		fread(d88[drvno].secinfo.reserve, sizeof(unsigned char), 5, d88[drvno].fp);	// read reserve
		d88[drvno].secinfo.size = fgetc(d88[drvno].fp);			// data size of this sector part
		d88[drvno].secinfo.size += (fgetc(d88[drvno].fp)<<8);			// data size of this sector part
		d88[drvno].secinfo.data = ftell( d88[drvno].fp );			// offset to data
		d88[drvno].secinfo.offset = 0;						// offset to the next data from first sector
	}
}

// read 1 byte
unsigned char Getc88(int drvno)
{
	if (d88[drvno].fp) {
		unsigned char dat = fgetc(d88[drvno].fp);
		d88[drvno].secinfo.offset++;
		if(d88[drvno].secinfo.offset >= d88[drvno].secinfo.size) ReadSector88(drvno);
		return dat;
	}
	return 0xff;
}

// write 1 byte
int Putc88(int drvno, unsigned char dat)
{
	if (d88[drvno].fp && !d88[drvno].protect) {
		fseek(d88[drvno].fp, 0, SEEK_CUR);
		fputc(dat, d88[drvno].fp);
		fseek(d88[drvno].fp, 0, SEEK_CUR);
		d88[drvno].secinfo.offset++;
		if( d88[drvno].secinfo.offset >= d88[drvno].secinfo.size ) ReadSector88(drvno);
		return 1;
	}
	return 0;
}

// seek
int Seek88(int drvno, int trackno, int sectno)
{
	if (d88[drvno].fp) {
		int TryCnt;
		// if the track is invalid, error
		if (!d88[drvno].table[trackno]) return 0;
		// seek the top of the sector
		fseek(d88[drvno].fp, d88[drvno].table[trackno], SEEK_SET);
		// read sector information
		ReadSector88(drvno);
		if (d88[drvno].secinfo.sec_nr < sectno) return 0;	// sector number is valid ?
		TryCnt = 2;	// try number
		while (d88[drvno].secinfo.r != sectno) {
			if (!TryCnt) return 0;
			fseek(d88[drvno].fp, (long)d88[drvno].secinfo.size, SEEK_CUR);
			if (d88[drvno].secinfo.sec_nr > 255) return 0;
			ReadSector88(drvno);	// read next sector information
			if (d88[drvno].secinfo.r == d88[drvno].secinfo.sec_nr)
				TryCnt--;
		}
		return 1;
	}
	return 0;
}

// get current CHRN
unsigned long GetCHRN88(int drvno)
{
	return d88[drvno].secinfo.c<<24 || d88[drvno].secinfo.h<<16 || d88[drvno].secinfo.r<<8 || d88[drvno].secinfo.n;
}

// get file name
char *GetFileName88(int drvno)
{
	return FileName88[drvno];
}

// get disk image name
char *GetDiskImgName88(int drvno)
{
	return (char *)d88[drvno].name;
}

// get protect state
int IsProtect88(int drvno)
{
	return Protected88[drvno];
}
