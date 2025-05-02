/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ file i/o ]
*/

#include "fileio.h"

FILEIO::FILEIO()
{
	// èâä˙âª
	fp = NULL;
}

FILEIO::~FILEIO(void)
{
	// å„énññ
	if(fp != NULL)
		Fclose();
}

bool FILEIO::IsProtected(_TCHAR *filename)
{
	return (GetFileAttributes(filename) & FILE_ATTRIBUTE_READONLY) ? true : false;
}

#ifdef _WIN32_WCE

// ----------------------------------------
// for windows ce
// ----------------------------------------

bool FILEIO::Fopen(_TCHAR *filename, int mode)
{
	// close if already opened
	Fclose();
	
	switch(mode)
	{
		case FILEIO_READ_BINARY:
		case FILEIO_READ_ASCII:
			fp = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
			break;
		case FILEIO_WRITE_BINARY:
		case FILEIO_WRITE_ASCII:
			fp = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
			break;
		case FILEIO_READ_WRITE_BINARY:
		case FILEIO_READ_WRITE_ASCII:
			fp = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
			break;
	}
	if(fp == INVALID_HANDLE_VALUE)
		fp = NULL;
	return (fp == NULL) ? false : true;
}

void FILEIO::Fclose()
{
	if(fp)
		CloseHandle(fp);
	fp = NULL;
}

uint32 FILEIO::Fread(void* buffer, uint32 size, uint32 count)
{
	DWORD dwSize;
	ReadFile(fp, buffer, size * count, &dwSize, NULL);
	return (uint32)(dwSize / size);
}

uint32 FILEIO::Fwrite(void* buffer, uint32 size, uint32 count)
{
	DWORD dwSize;
	WriteFile(fp, buffer, size * count, &dwSize, NULL);
	return (uint32)(dwSize / size);
}

uint32 FILEIO::Fseek(long offset, int origin)
{
	switch(origin)
	{
		case FILEIO_SEEK_CUR:
			return SetFilePointer(fp, offset, NULL, FILE_CURRENT) != 0xFFFFFFFF ? 0 : 0xFFFFFFFF;
		case FILEIO_SEEK_END:
			return SetFilePointer(fp, offset, NULL, FILE_END) != 0xFFFFFFFF ? 0 : 0xFFFFFFFF;
		case FILEIO_SEEK_SET:
			return SetFilePointer(fp, offset, NULL, FILE_BEGIN) != 0xFFFFFFFF ? 0 : 0xFFFFFFFF;
	}
	return 0xFFFFFFFF;
}

uint32 FILEIO::Ftell()
{
	return SetFilePointer(fp, 0, NULL, FILE_CURRENT);
}

#else

// ----------------------------------------
// for ansi c
// ----------------------------------------

bool FILEIO::Fopen(_TCHAR *filename, int mode)
{
	// close if already opened
	Fclose();
	
	switch(mode)
	{
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
	if(fp)
		fclose(fp);
	fp = NULL;
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
	switch(origin)
	{
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

#endif

