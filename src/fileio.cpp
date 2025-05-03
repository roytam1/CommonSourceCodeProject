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

bool FILEIO::IsFileExists(_TCHAR *filename)
{
	DWORD attr = GetFileAttributes(filename);
	if(attr == -1) {
		return false;
	}
	return ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

bool FILEIO::IsProtected(_TCHAR *filename)
{
	return ((GetFileAttributes(filename) & FILE_ATTRIBUTE_READONLY) != 0);
}

bool FILEIO::Fopen(_TCHAR *filename, int mode)
{
	Fclose();
	
	switch(mode) {
	case FILEIO_READ_BINARY:
		return ((fp = _tfopen(filename, _T("rb"))) != NULL);
	case FILEIO_WRITE_BINARY:
		return ((fp = _tfopen(filename, _T("wb"))) != NULL);
	case FILEIO_READ_WRITE_BINARY:
		return ((fp = _tfopen(filename, _T("r+b"))) != NULL);
	case FILEIO_READ_ASCII:
		return ((fp = _tfopen(filename, _T("r"))) != NULL);
	case FILEIO_WRITE_ASCII:
		return ((fp = _tfopen(filename, _T("w"))) != NULL);
	case FILEIO_READ_WRITE_ASCII:
		return ((fp = _tfopen(filename, _T("r+w"))) != NULL);
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

bool FILEIO::FgetBool()
{
	BYTE buffer[sizeof(bool)];
	this->Fread(buffer, sizeof(buffer), 1);
	return *(bool *)buffer;
}

void FILEIO::FputBool(bool val)
{
	this->Fwrite(&val, sizeof(bool), 1);
}

uint8 FILEIO::FgetUint8()
{
	BYTE buffer[sizeof(uint8)];
	this->Fread(buffer, sizeof(buffer), 1);
	return *(uint8 *)buffer;
}

void FILEIO::FputUint8(uint8 val)
{
	this->Fwrite(&val, sizeof(uint8), 1);
}

uint16 FILEIO::FgetUint16()
{
	BYTE buffer[sizeof(uint16)];
	this->Fread(buffer, sizeof(buffer), 1);
	return *(uint16 *)buffer;
}

void FILEIO::FputUint16(uint16 val)
{
	this->Fwrite(&val, sizeof(uint16), 1);
}

uint32 FILEIO::FgetUint32()
{
	BYTE buffer[sizeof(uint32)];
	this->Fread(buffer, sizeof(buffer), 1);
	return *(uint32 *)buffer;
}

void FILEIO::FputUint32(uint32 val)
{
	this->Fwrite(&val, sizeof(uint32), 1);
}

uint64 FILEIO::FgetUint64()
{
	BYTE buffer[sizeof(uint64)];
	this->Fread(buffer, sizeof(buffer), 1);
	return *(uint64 *)buffer;
}

void FILEIO::FputUint64(uint64 val)
{
	this->Fwrite(&val, sizeof(uint64), 1);
}

int8 FILEIO::FgetInt8()
{
	BYTE buffer[sizeof(int8)];
	this->Fread(buffer, sizeof(buffer), 1);
	return *(int8 *)buffer;
}

void FILEIO::FputInt8(int8 val)
{
	this->Fwrite(&val, sizeof(int8), 1);
}

int16 FILEIO::FgetInt16()
{
	BYTE buffer[sizeof(int16)];
	this->Fread(buffer, sizeof(buffer), 1);
	return *(int16 *)buffer;
}

void FILEIO::FputInt16(int16 val)
{
	this->Fwrite(&val, sizeof(int16), 1);
}

int32 FILEIO::FgetInt32()
{
	BYTE buffer[sizeof(int32)];
	this->Fread(buffer, sizeof(buffer), 1);
	return *(int32 *)buffer;
}

void FILEIO::FputInt32(int32 val)
{
	this->Fwrite(&val, sizeof(int32), 1);
}

int64 FILEIO::FgetInt64()
{
	BYTE buffer[sizeof(int64)];
	this->Fread(buffer, sizeof(buffer), 1);
	return *(int64 *)buffer;
}

void FILEIO::FputInt64(int64 val)
{
	this->Fwrite(&val, sizeof(int64), 1);
}

float FILEIO::FgetFloat()
{
	BYTE buffer[sizeof(float)];
	this->Fread(buffer, sizeof(buffer), 1);
	return *(float *)buffer;
}

void FILEIO::FputFloat(float val)
{
	this->Fwrite(&val, sizeof(float), 1);
}

double FILEIO::FgetDouble()
{
	BYTE buffer[sizeof(double)];
	this->Fread(buffer, sizeof(buffer), 1);
	return *(double *)buffer;
}

void FILEIO::FputDouble(double val)
{
	this->Fwrite(&val, sizeof(double), 1);
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
