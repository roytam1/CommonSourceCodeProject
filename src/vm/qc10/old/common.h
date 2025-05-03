/*
	Skelton for Z-80 PC Emulator

	Author : Takeda.Toshiya
	Date   : 2004.04.29 -

	[ Common Header ]
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include <tchar.h>

// device settings

#define FRAMES_PER_10SECS	458
#define FRAMES_PER_SEC		45.8
#define LINES_PER_FRAME 	421
#define CHARS_PER_LINE		108

#define CPU_CLOCKS		3993600
#define CLOCKS_PER_LINE		207

#define SCREEN_WIDTH		640
#define SCREEN_HEIGHT		400

// variable scope of 'for' loop (for microsoft visual c++ 6.0)
#define for if(0);else for

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
