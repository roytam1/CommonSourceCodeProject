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

#define FILE_VERSION	0x30

void init_config();
void load_config();
void save_config();

typedef struct {
	int version1;	// config file version
	int version2;
	
	// recent files
#ifdef USE_CART
	_TCHAR initial_cart_path[_MAX_PATH];
	_TCHAR recent_cart_path[8][_MAX_PATH];
#endif
#ifdef USE_FD1
	_TCHAR initial_disk_path[_MAX_PATH];
	_TCHAR recent_disk_path[4][8][_MAX_PATH];
#endif
#ifdef USE_QUICKDISK
	_TCHAR initial_quickdisk_path[_MAX_PATH];
	_TCHAR recent_quickdisk_path[8][_MAX_PATH];
#endif
#ifdef USE_DATAREC
	_TCHAR initial_datarec_path[_MAX_PATH];
	_TCHAR recent_datarec_path[8][_MAX_PATH];
#endif
#ifdef USE_MEDIA
	_TCHAR initial_media_path[_MAX_PATH];
	_TCHAR recent_media_path[8][_MAX_PATH];
#endif
#ifdef USE_RAM
	_TCHAR initial_ram_path[_MAX_PATH];
	_TCHAR recent_ram_path[8][_MAX_PATH];
#endif
	
	// screen
	int window_mode;
	bool stretch_screen;
	
	// sound
	int sound_frequency;
	int sound_latency;
	
	// virtual machine
	int cpu_power;
#ifdef USE_FD1
	bool ignore_crc;
#endif
#ifdef USE_DIPSWITCH
	uint8 dipswitch;
#endif
#ifdef _HC80
	int ramdisk_type;
#endif
#if defined(USE_MONITOR_TYPE) || defined(USE_SCREEN_ROTATE)
	int monitor_type;
#endif;
#ifdef USE_SCANLINE
	bool scan_line;
#endif
} config_t;

extern config_t config;

#endif

