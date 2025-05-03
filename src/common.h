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

#ifndef int8
typedef signed char int8;
#endif
#ifndef int16
typedef signed short int16;
#endif
#ifndef int32
typedef signed int int32;
#endif

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
// RGB565
#define RGB_COLOR(r, g, b) (uint16)(((uint16)(r) << 11) | ((uint16)(g) << 6) | (uint16)(b))
#else
// RGB555
#define RGB_COLOR(r, g, b) (uint16)(((uint16)(r) << 10) | ((uint16)(g) << 5) | (uint16)(b))
#endif

#endif
