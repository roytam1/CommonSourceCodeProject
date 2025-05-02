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

#define FILE_VERSION	0x33

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
#if defined(USE_FD8) || defined(USE_FD7)
	_TCHAR recent_disk_path[8][8][_MAX_PATH];
#elif defined(USE_FD6) || defined(USE_FD5)
	_TCHAR recent_disk_path[6][8][_MAX_PATH];
#else
	_TCHAR recent_disk_path[4][8][_MAX_PATH];
#endif
#endif
#ifdef USE_QUICKDISK
	_TCHAR initial_quickdisk_path[_MAX_PATH];
	_TCHAR recent_quickdisk_path[8][_MAX_PATH];
#endif
#ifdef USE_DATAREC
	_TCHAR initial_datarec_path[_MAX_PATH];
	_TCHAR recent_datarec_path[8][_MAX_PATH];
#endif
#ifdef USE_BINARY_FILE1
	_TCHAR initial_binary_path[_MAX_PATH];
	_TCHAR recent_binary_path[2][8][_MAX_PATH];
#endif
	
	// screen
	int window_mode;
	bool use_d3d9;
	bool wait_vsync;
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
#ifdef USE_BOOT_MODE
	int boot_mode;		// FP-200, MZ-800, PASOPIA, PC-8801MA, PC-98DO
#endif
#ifdef USE_CPU_CLOCK_LOW
	bool cpu_clock_low;	// PC-8801MA, PC-9801E, PC-9801VM, PC-98DO
#endif
#if defined(_HC80) || defined(_PASOPIA) || defined(_PC8001SR) || defined(_PC8801MA)
	int device_type;
#endif
#if defined(USE_MONITOR_TYPE) || defined(USE_SCREEN_ROTATE)
	int monitor_type;
#endif
#ifdef USE_SCANLINE
	bool scan_line;
#endif
#ifdef USE_SOUND_DEVICE_TYPE
	int sound_device_type;
#endif
} config_t;

extern config_t config;

#endif

