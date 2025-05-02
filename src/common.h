/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ common header ]
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include <tchar.h>

// variable scope of 'for' loop (for microsoft visual c++ 6.0)
#if defined(_MSC_VER) && (_MSC_VER == 1200)
#define for if(0);else for
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

// for windows ce
#ifdef _WIN32_WCE
#define _memcpy(dest, src, length) CopyMemory((dest), (src), (length))
#define _memset(dest, value, length) FillMemory((dest), (length), (value))
#else
#define _memcpy(dest, src, length) memcpy((dest), (src), (length))
#define _memset(dest, value, length) memset((dest), (value), (length))
#endif

#endif
