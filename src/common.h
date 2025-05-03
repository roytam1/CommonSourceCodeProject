/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ common header ]
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include <tchar.h>

// variable scope of 'for' loop for microsoft visual c++ 6.0 and embedded visual c++ 4.0
#if (defined(_MSC_VER) && (_MSC_VER == 1200)) || defined(_WIN32_WCE)
#define for if(0);else for
#endif
// disable warnings C4189, C4995 and C4996 for microsoft visual c++ 2005
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#pragma warning( disable : 4819 )
#pragma warning( disable : 4995 )
#pragma warning( disable : 4996 )
#endif

// type definition
#ifndef uint8
typedef unsigned char uint8;
#endif
#ifndef uint16
typedef unsigned short uint16;
#endif
#ifndef uint32
typedef unsigned int uint32;
#endif
#ifndef _WIN32_WCE
#ifndef uint64
typedef unsigned long long uint64;
#endif
#endif

#ifndef int8
typedef signed char int8;
#endif
#ifndef int16
typedef signed short int16;
#endif
#ifndef int32
typedef signed int int32;
#endif
#ifndef _WIN32_WCE
#ifndef int64
typedef signed long long int64;
#endif
#endif

typedef union {
	uint32 l;
	uint16 w;
	struct {
#ifdef _BIG_ENDIAN
		uint8 h, l;
#else
		uint8 l, h;
#endif
	} b;
} pair;

// memory functions
//#ifdef _WIN32_WCE
//#define _memcpy(dest, src, length) CopyMemory((dest), (src), (length))
//#define _memmove(dest, src, length) MoveMemory((dest), (src), (length))
//#define _memset(dest, value, length) FillMemory((dest), (length), (value))
//#else
#define _memcpy(dest, src, length) memcpy((dest), (src), (length))
#define _memmove(dest, src, length) memmove((dest), (src), (length))
#define _memset(dest, value, length) memset((dest), (value), (length))
//#endif

// rgb color
#ifdef _WIN32_WCE
#define _RGB565
#else
//#define _RGB555
#define _RGB888
#endif

#if defined(_RGB555)
#define RGB_COLOR(r, g, b) ((uint16)(((uint16)(r) & 0xf8) << 7) | (uint16)(((uint16)(g) & 0xf8) << 2) | (uint16)(((uint16)(b) & 0xf8) >> 3))
typedef uint16 scrntype;
#elif defined(_RGB565)
#define RGB_COLOR(r, g, b) ((uint16)(((uint16)(r) & 0xf8) << 8) | (uint16)(((uint16)(g) & 0xfc) << 3) | (uint16)(((uint16)(b) & 0xf8) >> 3))
typedef uint16 scrntype;
#elif defined(_RGB888)
#define RGB_COLOR(r, g, b) (((uint32)(r) << 16) | ((uint32)(g) << 8) | ((uint32)(b) << 0))
typedef uint32 scrntype;
#endif

#endif
