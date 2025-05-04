/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ common header ]
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef _MSC_VER
	#if _MSC_VER == 1200
		// variable scope of 'for' loop for Microsoft Visual C++ 6.0
		#define for if(0);else for
	#endif
	#if _MSC_VER >= 1200
		#define SUPPORT_TCHAR_TYPE
	#endif
	#if _MSC_VER >= 1400
//		#define SUPPORT_SECURE_FUNCTIONS
		// disable warnings for Microsoft Visual C++ 2005 or later
		#pragma warning( disable : 4819 )
		//#pragma warning( disable : 4995 )
		#pragma warning( disable : 4996 )
	#endif
#endif

#ifdef _MSC_VER
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <process.h>
#endif
#ifdef SUPPORT_TCHAR_TYPE
#include <tchar.h>
#endif
#include <stdio.h>
#include <errno.h>

// endian
#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
	#if defined(__BYTE_ORDER) && (defined(__LITTLE_ENDIAN) || defined(__BIG_ENDIAN))
		#if __BYTE_ORDER == __LITTLE_ENDIAN
			#define __LITTLE_ENDIAN__
		#elif __BYTE_ORDER == __BIG_ENDIAN
			#define __BIG_ENDIAN__
		#endif
	#elif defined(SDL_BYTEORDER) && (defined(SDL_LIL_ENDIAN) || defined(SDL_BIG_ENDIAN))
		#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			#define __LITTLE_ENDIAN__
		#elif SDL_BYTEORDER == SDL_BIG_ENDIAN
			#define __BIG_ENDIAN__
		#endif
	#elif defined(WORDS_LITTLEENDIAN)
		#define __LITTLE_ENDIAN__
	#elif defined(WORDS_BIGENDIAN)
		#define __BIG_ENDIAN__
	#endif
#endif
#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
	// Microsoft Visual C++
	#define __LITTLE_ENDIAN__
#endif

// type definition
#ifndef uchar
	typedef unsigned char uchar;
#endif
#ifndef ushort
	typedef unsigned short ushort;
#endif
#ifndef uint
	typedef unsigned int uint;
#endif
#ifndef ulong
	typedef unsigned long ulong;
#endif

#ifndef int8
	#if defined(_MSC_VER)
		typedef signed __int8 int8;
	#elif defined(int8_t)
		typedef int8_t int8;
	#else
		typedef signed char int8;
	#endif
#endif
#ifndef int16
	#if defined(_MSC_VER)
		typedef signed __int16 int16;
	#elif defined(int16_t)
		typedef int16_t int16;
	#else
		typedef signed short int16;
	#endif
#endif
#ifndef int32
	#if defined(_MSC_VER)
		typedef signed __int32 int32;
	#elif defined(int32_t)
		typedef int32_t int32;
	#else
		typedef signed int int32;
	#endif
#endif
#ifndef int64
	#if defined(_MSC_VER)
		typedef signed __int64 int64;
	#elif defined(int64_t)
		typedef int64_t int64;
	#else
		typedef signed long long int64;
	#endif
#endif

#ifndef sint8
	#if defined(_MSC_VER)
		typedef signed __int8 sint8;
	#elif defined(int8_t)
		typedef int8_t sint8;
	#else
		typedef signed char sint8;
	#endif
#endif
#ifndef sint16
	#if defined(_MSC_VER)
		typedef signed __int16 sint16;
	#elif defined(int16_t)
		typedef int16_t sint16;
	#else
		typedef signed short sint16;
	#endif
#endif
#ifndef sint32
	#if defined(_MSC_VER)
		typedef signed __int32 sint32;
	#elif defined(int32_t)
		typedef int32_t sint32;
	#else
		typedef signed int sint32;
	#endif
#endif
#ifndef sint64
	#if defined(_MSC_VER)
		typedef signed __int64 sint64;
	#elif defined(int64_t)
		typedef int64_t sint64;
	#else
		typedef signed long long sint64;
	#endif
#endif

#ifndef uint8
	#if defined(_MSC_VER)
		typedef unsigned __int8 uint8;
	#elif defined(uint8_t)
		typedef uint8_t uint8;
	#else
		typedef unsigned char uint8;
	#endif
#endif
#ifndef uint16
	#if defined(_MSC_VER)
		typedef unsigned __int16 uint16;
	#elif defined(uint16_t)
		typedef uint16_t uint16;
	#else
		typedef unsigned short uint16;
	#endif
#endif
#ifndef uint32
	#if defined(_MSC_VER)
		typedef unsigned __int32 uint32;
	#elif defined(uint32_t)
		typedef uint32_t uint32;
	#else
		typedef unsigned int uint32;
	#endif
#endif
#ifndef uint64
	#if defined(_MSC_VER)
		typedef unsigned __int64 uint64;
	#elif defined(uint64_t)
		typedef uint64_t uint64;
	#else
		typedef unsigned long long uint64;
	#endif
#endif

#ifndef SUPPORT_TCHAR_TYPE
	#ifndef _TCHAR
		typedef char _TCHAR;
	#endif
#endif

#ifndef _MSC_VER
	#ifndef LPTSTR
		typedef _TCHAR* LPTSTR;
	#endif
	#ifndef LPCTSTR
		typedef const _TCHAR* LPCTSTR;
	#endif
	#ifndef BOOL
		typedef int BOOL;
	#endif
	#ifndef BYTE
		#if defined(uint8_t)
			typedef uint8_t BYTE;
		#else
			typedef unsigned char BYTE;
		#endif
	#endif
	#ifndef WORD
		#if defined(uint16_t)
			typedef uint16_t WORD;
		#else
			typedef unsigned short WORD;
		#endif
	#endif
	#ifndef DWORD
		#if defined(uint32_t)
			typedef uint32_t DWORD;
		#else
			typedef unsigned int DWORD;
		#endif
	#endif
	#ifndef QWORD
		#if defined(uint64_t)
			typedef uint64_t QWORD;
		#else
			typedef unsigned long long QWORD;
		#endif
	#endif
	#ifndef INT8
		#if defined(int8_t)
			typedef int8_t INT8;
		#else
			typedef signed char INT8;
		#endif
	#endif
	#ifndef INT16
		#if defined(int16_t)
			typedef int16_t INT16;
		#else
			typedef signed short INT16;
		#endif
	#endif
	#ifndef INT32
		#if defined(int32_t)
			typedef int32_t INT32;
		#else
			typedef signed int INT32;
		#endif
	#endif
	#ifndef INT64
		#if defined(int64_t)
			typedef int64_t INT64;
		#else
			typedef signed long long INT64;
		#endif
	#endif
	#ifndef UINT8
		#if defined(uint8_t)
			typedef uint8_t UINT8;
		#else
			typedef unsigned char UINT8;
		#endif
	#endif
	#ifndef UINT16
		#if defined(uint16_t)
			typedef uint16_t UINT16;
		#else
			typedef unsigned short UINT16;
		#endif
	#endif
	#ifndef UINT32
		#if defined(uint32_t)
			typedef uint32_t UINT32;
		#else
			typedef unsigned int UINT32;
		#endif
	#endif
	#ifndef UINT64
		#if defined(uint64_t)
			typedef uint64_t UINT64;
		#else
			typedef unsigned long long UINT64;
		#endif
	#endif
#endif

typedef union {
	struct {
#ifdef __BIG_ENDIAN__
		uint8 h3, h2, h, l;
#else
		uint8 l, h, h2, h3;
#endif
	} b;
	struct {
#ifdef __BIG_ENDIAN__
		int8 h3, h2, h, l;
#else
		int8 l, h, h2, h3;
#endif
	} sb;
	struct {
#ifdef __BIG_ENDIAN__
		uint16 h, l;
#else
		uint16 l, h;
#endif
	} w;
	struct {
#ifdef __BIG_ENDIAN__
		int16 h, l;
#else
		int16 l, h;
#endif
	} sw;
	uint32 d;
	int32 sd;
	inline void read_2bytes_le_from(uint8 *t)
	{
		b.l = t[0]; b.h = t[1]; b.h2 = b.h3 = 0;
	}
	inline void write_2bytes_le_to(uint8 *t)
	{
		t[0] = b.l; t[1] = b.h;
	}
	inline void read_2bytes_be_from(uint8 *t)
	{
		b.h3 = b.h2 = 0; b.h = t[0]; b.l = t[1];
	}
	inline void write_2bytes_be_to(uint8 *t)
	{
		t[0] = b.h; t[1] = b.l;
	}
	inline void read_4bytes_le_from(uint8 *t)
	{
		b.l = t[0]; b.h = t[1]; b.h2 = t[2]; b.h3 = t[3];
	}
	inline void write_4bytes_le_to(uint8 *t)
	{
		t[0] = b.l; t[1] = b.h; t[2] = b.h2; t[3] = b.h3;
	}
	inline void read_4bytes_be_from(uint8 *t)
	{
		b.h3 = t[0]; b.h2 = t[1]; b.h = t[2]; b.l = t[3];
	}
	inline void write_4bytes_be_to(uint8 *t)
	{
		t[0] = b.h3; t[1] = b.h2; t[2] = b.h; t[3] = b.l;
	}

} pair;

// max/min from WinDef.h
#ifndef max
	#define MAX_MACRO_NOT_DEFINED
	inline int max(int a, int b);
	inline unsigned int max(unsigned int a, unsigned int b);
#endif
#ifndef min
	#define MIN_MACRO_NOT_DEFINED
	inline int min(int a, int b);
	inline unsigned int min(unsigned int a, unsigned int b);
#endif

// _TCHAR
#ifndef SUPPORT_TCHAR_TYPE
	#ifndef _tfopen
		#define _tfopen fopen
	#endif
	#ifndef _tcscmp
		#define _tcscmp strcmp
	#endif
	#ifndef _tcscpy
		#define _tcscpy strcpy
	#endif
	#ifndef _tcsicmp
		#define _tcsicmp stricmp
	#endif
	#ifndef _tcslen
		#define _tcslen strlen
	#endif
	#ifndef _tcsncat
		#define _tcsncat strncat
	#endif
	#ifndef _tcsncpy
		#define _tcsncpy strncpy
	#endif
	#ifndef _tcsncicmp
		#define _tcsncicmp strnicmp
	#endif
	#ifndef _tcsstr
		#define _tcsstr strstr
	#endif
	#ifndef _tcstok
		#define _tcstok strtok
	#endif
	#ifndef _tcstol
		#define _tcstol strtol
	#endif
	#ifndef _tcstoul
		#define _tcstoul strtoul
	#endif
	#ifndef _stprintf
		#define _stprintf sprintf
	#endif
	#ifndef _vstprintf
		#define _vstprintf vsprintf
	#endif
	#ifndef _taccess
		#define _taccess access
	#endif
	#ifndef _tremove
		#define _tremove remove
	#endif
	#ifndef _trename
		#define _trename rename
	#endif
	#define __T(x) x
	#define _T(x) __T(x)
	#define _TEXT(x) __T(x)
#endif

// secture functions
#ifndef SUPPORT_SECURE_FUNCTIONS
	#ifndef errno_t
		typedef int errno_t;
	#endif
//	errno_t my_tfopen_s(FILE** pFile, const _TCHAR *filename, const _TCHAR *mode);
	errno_t my_strcpy_s(char *strDestination, size_t numberOfElements, const char *strSource);
	errno_t my_tcscpy_s(_TCHAR *strDestination, size_t numberOfElements, const _TCHAR *strSource);
	char *my_strtok_s(char *strToken, const char *strDelimit, char **context);
	_TCHAR *my_tcstok_s(_TCHAR *strToken, const char *strDelimit, _TCHAR **context);
	#define my_fprintf_s fprintf
	int my_sprintf_s(char *buffer, size_t sizeOfBuffer, const char *format, ...);
	int my_stprintf_s(_TCHAR *buffer, size_t sizeOfBuffer, const _TCHAR *format, ...);
	int my_vsprintf_s(char *buffer, size_t numberOfElements, const char *format, va_list argptr);
	int my_vstprintf_s(_TCHAR *buffer, size_t numberOfElements, const _TCHAR *format, va_list argptr);
#else
//	#define my_tfopen_s _tfopen_s
	#define my_strcpy_s strcpy_s
	#define my_tcscpy_s _tcscpy_s
	#define my_strtok_s strtok_s
	#define my_tcstok_s _tcstok_s
	#define my_fprintf_s fprintf_s
	#define my_sprintf_s sprintf_s
	#define my_stprintf_s _stprintf_s
	#define my_vsprintf_s vsprintf_s
	#define my_vstprintf_s _vstprintf_s
#endif

#ifndef _MSC_VER
	BOOL MyWritePrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString, LPCTSTR lpFileName);
	DWORD MyGetPrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize, LPCTSTR lpFileName);
	UINT MyGetPrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault, LPCTSTR lpFileName);
#else
	#define MyWritePrivateProfileString WritePrivateProfileString
	#define MyGetPrivateProfileString GetPrivateProfileString
	#define MyGetPrivateProfileInt GetPrivateProfileInt
#endif

// rgb color
#define _RGB888

#if defined(_RGB555) || defined(_RGB565)
	typedef uint16 scrntype;
	scrntype RGB_COLOR(uint r, uint g, uint b);
	scrntype RGBA_COLOR(uint r, uint g, uint b, uint a);
	uint8 R_OF_COLOR(scrntype c);
	uint8 G_OF_COLOR(scrntype c);
	uint8 B_OF_COLOR(scrntype c);
	uint8 A_OF_COLOR(scrntype c);
#elif defined(_RGB888)
	typedef uint32 scrntype;
	#define RGB_COLOR(r, g, b)	(((uint32)(r) << 16) | ((uint32)(g) << 8) | ((uint32)(b) << 0))
	#define RGBA_COLOR(r, g, b, a)	(((uint32)(r) << 16) | ((uint32)(g) << 8) | ((uint32)(b) << 0) | ((uint32)(a) << 24))	
	#define R_OF_COLOR(c)		(((c) >> 16) & 0xff)
	#define G_OF_COLOR(c)		(((c) >>  8) & 0xff)
	#define B_OF_COLOR(c)		(((c)      ) & 0xff)
	#define A_OF_COLOR(c)		(((c) >> 24) & 0xff)
#endif

// wav file header
#pragma pack(1)
typedef struct {
	char id[4];
	uint32 size;
} wav_chunk_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	wav_chunk_t riff_chunk;
	char wave[4];
	wav_chunk_t fmt_chunk;
	uint16 format_id;
	uint16 channels;
	uint32 sample_rate;
	uint32 data_speed;
	uint16 block_size;
	uint16 sample_bits;
} wav_header_t;
#pragma pack()

// misc
bool check_file_extension(const _TCHAR* file_path, const _TCHAR* ext);
_TCHAR *get_file_path_without_extensiton(const _TCHAR* file_path);
uint32 getcrc32(uint8 data[], int size);

#define array_length(array) (sizeof(array) / sizeof(array[0]))

#define FROM_BCD(v)	(((v) & 0x0f) + (((v) >> 4) & 0x0f) * 10)
#define TO_BCD(v)	((int)(((v) % 100) / 10) << 4) | ((v) % 10)
#define TO_BCD_LO(v)	((v) % 10)
#define TO_BCD_HI(v)	(int)(((v) % 100) / 10)

#define LEAP_YEAR(y)	(((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

typedef struct cur_time_s {
	int year, month, day, day_of_week, hour, minute, second;
	bool initialized;
	cur_time_s()
	{
		initialized = false;
	}
	void increment();
	void update_year();
	void update_day_of_week();
	void save_state(void *f);
	bool load_state(void *f);
} cur_time_t;

#endif
