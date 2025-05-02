/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 emulation i/f ]
*/

#include "emu.h"
#include "vm/vm.h"

#include "config.h"

// ----------------------------------------------------------------------------
// initialize
// ----------------------------------------------------------------------------

EMU::EMU(HWND hwnd, HINSTANCE hinst)
{
	// open debug logfile
	open_debug();
	
	// store main window handle
	main_window_handle = hwnd;
	instance_handle = hinst;
	
	// get module path
	GetModuleFileName(NULL, app_path, _MAX_PATH);
	int pt = _tcslen(app_path);
	while(pt >= 0 && app_path[pt] != _T('\\')) {
		pt--;
	}
	app_path[pt + 1] = _T('\0');
	
	// load sound config
	static int freq_table[8] = {2000, 4000, 8000, 11025, 22050, 44100, 48000, 96000};
	static double late_table[4] = {0.1, 0.2, 0.3, 0.4};
	
	if(!(0 <= config.sound_frequency && config.sound_frequency <= 7)) {
		config.sound_frequency = 5;
	}
	if(!(0 <= config.sound_latency && config.sound_latency <= 3)) {
		config.sound_latency = 0;
	}
	int frequency = freq_table[config.sound_frequency];
	int latency = (int)(frequency * late_table[config.sound_latency]);
	
	// initialize
	vm = new VM(this);
	initialize_input();
	initialize_screen();
	initialize_sound(frequency, latency);
#ifdef USE_MEDIA
	initialize_media();
#endif
#ifdef USE_SOCKET
	initialize_socket();
#endif
}

EMU::~EMU()
{
	release_input();
	release_screen();
	release_sound();
#ifdef USE_MEDIA
	release_media();
#endif
#ifdef USE_SOCKET
	release_socket();
#endif
	if(vm) {
		delete vm;
	}
	close_debug();
}

// ----------------------------------------------------------------------------
// drive machine
// ----------------------------------------------------------------------------

void EMU::run()
{
	// run real machine
	update_input();
	update_sound();
	update_timer();
#ifdef USE_SOCKET
	update_socket();
#endif
	// run virtual machine
	vm->run();
}

void EMU::reset()
{
	// reset virtual machine
	vm->reset();
#ifdef USE_MEDIA
	stop_media();
#endif
	// restart recording
	restart_rec_video();
	restart_rec_sound();
}

#ifdef USE_IPL_RESET
void EMU::ipl_reset()
{
	// reset virtual machine
	vm->ipl_reset();
#ifdef USE_MEDIA
	stop_media();
#endif
	// restart recording
	restart_rec_video();
	restart_rec_sound();
}
#endif

void EMU::application_path(_TCHAR* path)
{
	_tcscpy(path, app_path);
}

// ----------------------------------------------------------------------------
// debug log
// ----------------------------------------------------------------------------

void EMU::open_debug()
{
#ifdef _DEBUG_LOG
	debug = fopen("d:\\debug.log", "w");
#endif
}

void EMU::close_debug()
{
#ifdef _DEBUG_LOG
	fclose(debug);
#endif
}

void EMU::out_debug(const _TCHAR* format, ...)
{
#ifdef _DEBUG_LOG
	va_list ap;
	_TCHAR buffer[1024];
	
	va_start(ap, format);
	_vstprintf(buffer, format, ap);
	if(debug) {
		_ftprintf(debug, _T("%s"), buffer);
	}
	va_end(ap);
#endif
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

#ifdef USE_CART
void EMU::open_cart(_TCHAR* filename)
{
	vm->open_cart(filename);
	
	// restart recording
	restart_rec_video();
	restart_rec_sound();
}
void EMU::close_cart()
{
	vm->close_cart();
	
	// stop recording
	stop_rec_video();
	stop_rec_sound();
}
#endif

#ifdef USE_FD1
void EMU::open_disk(_TCHAR* filename, int drv)
{
	vm->open_disk(filename, drv);
}
void EMU::close_disk(int drv)
{
	vm->close_disk(drv);
}
#endif

#ifdef USE_DATAREC
void EMU::play_datarec(_TCHAR* filename)
{
	vm->play_datarec(filename);
}
void EMU::rec_datarec(_TCHAR* filename)
{
	vm->rec_datarec(filename);
}
void EMU::close_datarec()
{
	vm->close_datarec();
}
#endif

#ifdef USE_DATAREC_BUTTON
void EMU::push_play()
{
	vm->push_play();
}
void EMU::push_stop()
{
	vm->push_stop();
}
#endif

BOOL EMU::now_skip()
{
	return (BOOL)vm->now_skip();
}

void EMU::update_config()
{
	vm->update_config();
}

#ifdef USE_RAM
void EMU::load_ram(_TCHAR* filename)
{
	vm->load_ram(filename);
}
void EMU::save_ram(_TCHAR* filename)
{
	vm->save_ram(filename);
}
#endif

#ifdef USE_MZT
void EMU::open_mzt(_TCHAR* filename)
{
	vm->open_mzt(filename);
}
#endif
