/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ file i/o ]
*/

#include "fileio.h"

FILEIO::FILEIO()
{
	fp = NULL;
}

FILEIO::~FILEIO(void)
{
	Fclose();
}

bool FILEIO::IsProtected(_TCHAR *filename)
{
	return (GetFileAttributes(filename) & FILE_ATTRIBUTE_READONLY) ? true : false;
}

bool FILEIO::Fopen(_TCHAR *filename, int mode)
{
	Fclose();
	
	switch(mode) {
	case FILEIO_READ_BINARY:
		return ((fp = _tfopen(filename, _T("rb"))) == NULL) ? false : true;
	case FILEIO_WRITE_BINARY:
		return ((fp = _tfopen(filename, _T("wb"))) == NULL) ? false : true;
	case FILEIO_READ_WRITE_BINARY:
		return ((fp = _tfopen(filename, _T("r+b"))) == NULL) ? false : true;
	case FILEIO_READ_ASCII:
		return ((fp = _tfopen(filename, _T("r"))) == NULL) ? false : true;
	case FILEIO_WRITE_ASCII:
		return ((fp = _tfopen(filename, _T("w"))) == NULL) ? false : true;
	case FILEIO_READ_WRITE_ASCII:
		return ((fp = _tfopen(filename, _T("r+w"))) == NULL) ? false : true;
	}
	return false;
}

void FILEIO::Fclose()
{
	if(fp) {
		fclose(fp);
	}
	fp = NULL;
}

int FILEIO::Fgetc()
{
	return fgetc(fp);
}

int FILEIO::Fputc(int c)
{
	return fputc(c, fp);
}

uint32 FILEIO::Fread(void* buffer, uint32 size, uint32 count)
{
	return fread(buffer, size, count, fp);
}

uint32 FILEIO::Fwrite(void* buffer, uint32 size, uint32 count)
{
	return fwrite(buffer, size, count, fp);
}

uint32 FILEIO::Fseek(long offset, int origin)
{
	switch(origin) {
	case FILEIO_SEEK_CUR:
		return fseek(fp, offset, SEEK_CUR);
	case FILEIO_SEEK_END:
		return fseek(fp, offset, SEEK_END);
	case FILEIO_SEEK_SET:
		return fseek(fp, offset, SEEK_SET);
	}
	return 0xFFFFFFFF;
}

uint32 FILEIO::Ftell()
{
	return ftell(fp);
}

void FILEIO::Remove(_TCHAR *filename)
{
	DeleteFile(filename);
//	_tremove(filename);	// not supported on wince
}
