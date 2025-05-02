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
	_memset(&config, 0, sizeof(config_t));
	
	config.version1 = FILE_VERSION;
	config.version2 = CONFIG_VERSION;
	
	config.window_mode = 0;
	config.stretch_screen = false;
	
	config.sound_frequency = 6;	// 48KHz
	config.sound_latency = 1;	// 100msec
	
	config.cpu_power = 0;
	
#ifdef USE_DIPSWITCH
	config.dipswitch = DIPSWITCH_DEFAULT;
#endif
#ifdef _HC80
	config.ramdisk_type = 2;	// Nonintelligent ram disk
#endif
}

void load_config()
{
	// initial settings
	init_config();
	
	// get config path
	_TCHAR app_path[_MAX_PATH], config_path[_MAX_PATH];
	GetModuleFileName(NULL, app_path, _MAX_PATH);
	int pt = _tcslen(app_path);
	while(pt >= 0 && app_path[pt] != _T('\\')) {
		pt--;
	}
	app_path[pt + 1] = _T('\0');
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
	while(pt >= 0 && app_path[pt] != _T('\\')) {
		pt--;
	}
	app_path[pt + 1] = _T('\0');
	_stprintf(config_path, _T("%s%s.cfg"), app_path, _T(CONFIG_NAME));
	
	// save config
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(config_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite((void *)&config, sizeof(config), 1);
		fio->Fclose();
	}
	delete fio;
}

