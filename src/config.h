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

#define CONFIG_VERSION	0x13

void init_config();
void load_config();
void save_config();

typedef struct {
	int version;	// config file version
	
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
	// sound
	int sound_frequency;	// 0=11025Hz, 1=22050Hz, 2=44100Hz, 3=48000Hz
	int sound_latency;	// 0=100msec, 1=200msec, 2=300msec, 3=400msec
} config_t;

#endif

