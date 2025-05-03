/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ config ]
*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <tchar.h>
#include "vm/vm.h"

#define FILE_VERSION	0x21

void init_config();
void load_config();
void save_config();

typedef struct {
	int version1;	// config file version
	int version2;
	
	// recent files
#ifdef USE_CART
	_TCHAR recent_cart[8][_MAX_PATH];
#endif
#ifdef USE_FD1
	_TCHAR recent_disk[4][8][_MAX_PATH];
#endif
#ifdef USE_DATAREC
	_TCHAR recent_datarec[8][_MAX_PATH];
#endif
#ifdef USE_MEDIA
	_TCHAR recent_media[8][_MAX_PATH];
#endif
	// control
	int cpu_power;		// cpu clock <<= cpu_power
#ifdef USE_FD1
	bool ignore_crc;
#endif
	// screen
#ifdef USE_SCANLINE
	bool scan_line;
#endif
#ifdef USE_MONITOR_TYPE
	int monitor_type;
#endif;
	int window_mode;
	// sound
	int sound_frequency;	// 0=2000Hz, 1=4000Hz, 2=8000Hz, 3=11025Hz, 4=22050Hz, 5=44100Hz, 6=48000Hz
	int sound_latency;	// 0=100msec, 1=200msec, 2=300msec, 3=400msec
#ifdef USE_DIPSWITCH
	uint8 dipswitch;
#endif
	int d3d9_interval;	// 0=Don't wait vsync, 1=Wait vsync
	int d3d9_device;	// 0=Default, 1=Hardware TnL, 2=Hardware, 3=Software
	int d3d9_filter;	// 0=Default, 1=Point, 1=Linear
} config_t;

#endif

