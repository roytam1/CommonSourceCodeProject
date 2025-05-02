/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ config ]
*/

#include <stdlib.h>
#include "config.h"
#include "fileio.h"

config_t config;

void init_config()
{
	// initial settings
	config.version1 = FILE_VERSION;
	config.version2 = CONFIG_VERSION;
	
	for(int i = 0; i < 8; i++) {
#ifdef USE_CART
		_tcscpy(config.recent_cart[i], _T(""));
#endif
#ifdef USE_FD1
		for(int j = 0; j < 4; j++) {
			_tcscpy(config.recent_disk[j][i], _T(""));
		}
#endif
#ifdef USE_DATAREC
		_tcscpy(config.recent_datarec[i], _T(""));
#endif
#ifdef USE_MEDIA
		_tcscpy(config.recent_media[i], _T(""));
#endif
#ifdef USE_RAM
		_tcscpy(config.recent_ram[i], _T(""));
#endif
#ifdef USE_MZT
		_tcscpy(config.recent_mzt[i], _T(""));
#endif
	}
	config.cpu_power = 0;
#ifdef USE_FD1
	config.ignore_crc = false;
#endif
#ifdef USE_SCANLINE
	config.scan_line = false;
#endif
#if defined(USE_MONITOR_TYPE) || defined(USE_SCREEN_ROTATE)
	config.monitor_type = 0;
#endif
#ifdef USE_DIPSWITCH
	config.dipswitch = DIPSWITCH_DEFAULT;
#endif
#ifdef _HC80
	config.ramdisk_type = 2;	// default = Nonintelligent ram disk
#endif
#ifdef USE_SCREEN_X2
	config.window_mode = 1;
#else
	config.window_mode = 0;
#endif
	config.sound_frequency = 5;
	config.sound_latency = 0;
}

void load_config()
{
	// initial settings
	init_config();
	
	// get config path
	_TCHAR app_path[_MAX_PATH], config_path[_MAX_PATH];
	GetModuleFileName(NULL, app_path, _MAX_PATH);
	int pt = _tcslen(app_path);
	while(app_path[pt] != '\\') {
		pt--;
	}
	app_path[pt + 1] = '\0';
	_stprintf(config_path, _T("%s%s.cfg"), app_path, _T(CONFIG_NAME));
	
	// load config
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(config_path, FILEIO_READ_BINARY)) {
		fio->Fread((void *)&config, sizeof(config), 1);
		fio->Fclose();
	}
	delete fio;
	
	// check config version
	if(!(config.version1 == FILE_VERSION && config.version2 == CONFIG_VERSION)) {
		init_config();
	}
}

void save_config()
{
	// get config path
	_TCHAR app_path[_MAX_PATH], config_path[_MAX_PATH];
	GetModuleFileName(NULL, app_path, _MAX_PATH);
	int pt = _tcslen(app_path);
	while(app_path[pt] != '\\') {
		pt--;
	}
	app_path[pt + 1] = '\0';
	_stprintf(config_path, _T("%s%s.cfg"), app_path, _T(CONFIG_NAME));
	
	// save config
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(config_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite((void *)&config, sizeof(config), 1);
		fio->Fclose();
	}
	delete fio;
}

