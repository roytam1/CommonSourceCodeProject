/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 emulation i/f ]
*/

#include "emu.h"
#include "vm/vm.h"
#include "config.h"

#ifndef FD_BASE_NUMBER
#define FD_BASE_NUMBER 1
#endif

// ----------------------------------------------------------------------------
// initialize
// ----------------------------------------------------------------------------

EMU::EMU(HWND hwnd, HINSTANCE hinst)
{
#ifdef _DEBUG_LOG
	// open debug logfile
	open_debug();
#endif
	message_count = 0;
	
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
#ifdef SUPPORT_SOUND_FREQ_55467HZ
	// PC-8801/9801 series
	static int freq_table[8] = {2000, 4000, 8000, 11025, 22050, 44100, 55467, 96000};
#else
	static int freq_table[8] = {2000, 4000, 8000, 11025, 22050, 44100, 48000, 96000};
#endif
	static double late_table[5] = {0.05, 0.1, 0.2, 0.3, 0.4};
	
	if(!(0 <= config.sound_frequency && config.sound_frequency < 8)) {
		config.sound_frequency = 6;	// default: 48KHz
	}
	if(!(0 <= config.sound_latency && config.sound_latency < 5)) {
		config.sound_latency = 1;	// default: 100msec
	}
	sound_rate = freq_table[config.sound_frequency];
	sound_samples = (int)(sound_rate * late_table[config.sound_latency] + 0.5);
	
#ifdef USE_CPU_TYPE
	cpu_type = config.cpu_type;
#endif
	
	// initialize
	vm = new VM(this);
	initialize_input();
	initialize_screen();
	initialize_sound();
	initialize_media();
#ifdef USE_SOCKET
	initialize_socket();
#endif
	vm->initialize_sound(sound_rate, sound_samples);
	vm->reset();
}

EMU::~EMU()
{
	release_input();
	release_screen();
	release_sound();
#ifdef USE_SOCKET
	release_socket();
#endif
	delete vm;
#ifdef _DEBUG_LOG
	close_debug();
#endif
}

// ----------------------------------------------------------------------------
// drive machine
// ----------------------------------------------------------------------------

int EMU::frame_interval()
{
#ifdef SUPPORT_VARIABLE_TIMING
	return (int)(1024. * 1000. / vm->frame_rate() + 0.5);
#else
	return (int)(1024. * 1000. / FRAMES_PER_SEC + 0.5);
#endif
}

int EMU::run()
{
	update_input();
	update_media();
#ifdef USE_SOCKET
	update_socket();
#endif
	
	// virtual machine may be driven to fill sound buffer
	int extra_frames = 0;
	update_sound(&extra_frames);
	
	// drive virtual machine
	if(extra_frames == 0) {
		vm->run();
		extra_frames = 1;
	}
	return extra_frames;
}

void EMU::reset()
{
#ifdef USE_CPU_TYPE
	if(cpu_type != config.cpu_type) {
		// stop sound
		if(sound_ok && sound_started) {
			lpdsb->Stop();
			sound_started = false;
		}
		// reinitialize virtual machine
		delete vm;
		vm = new VM(this);
		vm->initialize_sound(sound_rate, sound_samples);
		vm->reset();
		// restore inserted medias
		restore_media();
		cpu_type = config.cpu_type;
	} else {
#endif
		// reset virtual machine
		vm->reset();
#ifdef USE_CPU_TYPE
	}
#endif
	
	// restart recording
	restart_rec_video();
	restart_rec_sound();
}

#ifdef USE_SPECIAL_RESET
void EMU::special_reset()
{
	// reset virtual machine
	vm->special_reset();
	
	// restart recording
	restart_rec_video();
	restart_rec_sound();
}
#endif

#ifdef USE_POWER_OFF
void EMU::notify_power_off()
{
	vm->notify_power_off();
}
#endif

_TCHAR* EMU::bios_path(_TCHAR* file_name)
{
	static _TCHAR file_path[_MAX_PATH];
	_stprintf(file_path, _T("%s%s"), app_path, file_name);
	return file_path;
}

// ----------------------------------------------------------------------------
// timer
// ----------------------------------------------------------------------------

void EMU::get_host_time(cur_time_t* time)
{
	SYSTEMTIME sTime;
	GetLocalTime(&sTime);
	
	time->year = sTime.wYear;
	time->month = sTime.wMonth;
	time->day = sTime.wDay;
	time->day_of_week = sTime.wDayOfWeek;
	time->hour = sTime.wHour;
	time->minute = sTime.wMinute;
	time->second = sTime.wSecond;
}

// ----------------------------------------------------------------------------
// debug log
// ----------------------------------------------------------------------------

void EMU::open_debug()
{
#ifdef _DEBUG_CONSOLE
	AllocConsole();
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTitle("Debug Log");
#endif
#ifdef _DEBUG_FILE
	debug = fopen("d:\\debug.log", "w");
#endif
}

void EMU::close_debug()
{
#ifdef _DEBUG_CONSOLE
	FreeConsole();
#endif
#ifdef _DEBUG_FILE
	if(debug) {
		fclose(debug);
	}
#endif
}

void EMU::out_debug(const _TCHAR* format, ...)
{
#ifdef _DEBUG_LOG
	va_list ap;
	_TCHAR buffer[1024];
	
	va_start(ap, format);
	_vstprintf(buffer, format, ap);
#ifdef _DEBUG_CONSOLE
	DWORD dwWritten;
	WriteConsole(hConsole, buffer, _tcslen(buffer), &dwWritten, NULL);
#endif
#ifdef _DEBUG_FILE
	if(debug) {
		_ftprintf(debug, _T("%s"), buffer);
	}
#endif
	va_end(ap);
#endif
}

void EMU::out_message(const _TCHAR* format, ...)
{
	va_list ap;
	va_start(ap, format);
	_vstprintf(message, format, ap);
	va_end(ap);
	message_count = 4; // 4sec
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void EMU::initialize_media()
{
#ifdef USE_CART
	memset(&cart_status, 0, sizeof(cart_status));
#endif
#ifdef USE_FD1
	memset(disk_status, 0, sizeof(disk_status));
#endif
#ifdef USE_QUICKDISK
	memset(&quickdisk_status, 0, sizeof(quickdisk_status));
#endif
#ifdef USE_TAPE
	memset(&tape_status, 0, sizeof(tape_status));
#endif
}

void EMU::update_media()
{
#ifdef USE_FD1
	for(int drv = 0; drv < 8; drv++) {
		if(disk_status[drv].wait_count != 0 && --disk_status[drv].wait_count == 0) {
			vm->open_disk(drv, disk_status[drv].path, disk_status[drv].offset);
			out_message(_T("FD%d: %s"), drv + FD_BASE_NUMBER, disk_status[drv].path);
		}
	}
#endif
#ifdef USE_QUICKDISK
	if(quickdisk_status.wait_count != 0 && --quickdisk_status.wait_count == 0) {
		vm->open_quickdisk(quickdisk_status.path);
		out_message(_T("QD: %s"), quickdisk_status.path);
	}
#endif
#ifdef USE_TAPE
	if(tape_status.wait_count != 0 && --tape_status.wait_count == 0) {
		if(tape_status.play) {
			vm->play_tape(tape_status.path);
		} else {
			vm->rec_tape(tape_status.path);
		}
		out_message(_T("CMT: %s"), tape_status.path);
	}
#endif
}

void EMU::restore_media()
{
#ifdef USE_CART
	if(cart_status.path[0] != _T('\0')) {
		vm->open_cart(cart_status.path);
	}
#endif
#ifdef USE_FD1
	for(int drv = 0; drv < 8; drv++) {
		if(disk_status[drv].path[0] != _T('\0')) {
			vm->open_disk(drv, disk_status[drv].path, disk_status[drv].offset);
		}
	}
#endif
#ifdef USE_QUICKDISK
	if(quickdisk_status.path[0] != _T('\0')) {
		vm->open_quickdisk(quickdisk_status.path);
	}
#endif
#ifdef USE_TAPE
	if(tape_status.path[0] != _T('\0')) {
		if(tape_status.play) {
			vm->play_tape(tape_status.path);
		} else {
			tape_status.path[0] = _T('\0');
		}
	}
#endif
}

#ifdef USE_CART
void EMU::open_cart(_TCHAR* file_path)
{
	vm->open_cart(file_path);
	_tcscpy(cart_status.path, file_path);
	out_message(_T("Cart: %s"), file_path);
	
	// restart recording
	restart_rec_video();
	restart_rec_sound();
}

void EMU::close_cart()
{
	vm->close_cart();
	clear_media_status(&cart_status);
	out_message(_T("Cart: Ejected"));
	
	// stop recording
	stop_rec_video();
	stop_rec_sound();
}

bool EMU::cart_inserted()
{
	return vm->cart_inserted();
}
#endif

#ifdef USE_FD1
void EMU::open_disk(int drv, _TCHAR* file_path, int offset)
{
	if(vm->disk_inserted(drv)) {
		vm->close_disk(drv);
		// wait 0.5sec
#ifdef SUPPORT_VARIABLE_TIMING
		disk_status[drv].wait_count = (int)(vm->frame_rate() / 2);
#else
		disk_status[drv].wait_count = (int)(FRAMES_PER_SEC / 2);
#endif
		out_message(_T("FD%d: Ejected"), drv + FD_BASE_NUMBER);
	} else if(disk_status[drv].wait_count == 0) {
		vm->open_disk(drv, file_path, offset);
		out_message(_T("FD%d: %s"), drv + FD_BASE_NUMBER, file_path);
	}
	_tcscpy(disk_status[drv].path, file_path);
	disk_status[drv].offset = offset;
}

void EMU::close_disk(int drv)
{
	vm->close_disk(drv);
	clear_media_status(&disk_status[drv]);
	out_message(_T("FD%d: Ejected"), drv + FD_BASE_NUMBER);
}

bool EMU::disk_inserted(int drv)
{
	return vm->disk_inserted(drv);
}
#endif

#ifdef USE_QUICKDISK
void EMU::open_quickdisk(_TCHAR* file_path)
{
	if(vm->quickdisk_inserted()) {
		vm->close_quickdisk();
		// wait 0.5sec
#ifdef SUPPORT_VARIABLE_TIMING
		quickdisk_status.wait_count = (int)(vm->frame_rate() / 2);
#else
		quickdisk_status.wait_count = (int)(FRAMES_PER_SEC / 2);
#endif
		out_message(_T("QD: Ejected"));
	} else if(quickdisk_status.wait_count == 0) {
		vm->open_quickdisk(file_path);
		out_message(_T("QD: %s"), file_path);
	}
	_tcscpy(quickdisk_status.path, file_path);
}

void EMU::close_quickdisk()
{
	vm->close_quickdisk();
	clear_media_status(&quickdisk_status);
	out_message(_T("QD: Ejected"));
}

bool EMU::quickdisk_inserted()
{
	return vm->quickdisk_inserted();
}
#endif

#ifdef USE_TAPE
void EMU::play_tape(_TCHAR* file_path)
{
	if(vm->tape_inserted()) {
		vm->close_tape();
		// wait 0.5sec
#ifdef SUPPORT_VARIABLE_TIMING
		tape_status.wait_count = (int)(vm->frame_rate() / 2);
#else
		tape_status.wait_count = (int)(FRAMES_PER_SEC / 2);
#endif
		out_message(_T("CMT: Ejected"));
	} else if(tape_status.wait_count == 0) {
		vm->play_tape(file_path);
		out_message(_T("CMT: %s"), file_path);
	}
	_tcscpy(tape_status.path, file_path);
	tape_status.play = true;
}

void EMU::rec_tape(_TCHAR* file_path)
{
	if(vm->tape_inserted()) {
		vm->close_tape();
		// wait 0.5sec
#ifdef SUPPORT_VARIABLE_TIMING
		tape_status.wait_count = (int)(vm->frame_rate() / 2);
#else
		tape_status.wait_count = (int)(FRAMES_PER_SEC / 2);
#endif
		out_message(_T("CMT: Ejected"));
	} else if(tape_status.wait_count == 0) {
		vm->rec_tape(file_path);
		out_message(_T("CMT: %s"), file_path);
	}
	_tcscpy(tape_status.path, file_path);
	tape_status.play = false;
}

void EMU::close_tape()
{
	vm->close_tape();
	clear_media_status(&tape_status);
	out_message(_T("CMT: Ejected"));
}

bool EMU::tape_inserted()
{
	return vm->tape_inserted();
}
#endif

#ifdef USE_TAPE_BUTTON
void EMU::push_play()
{
	vm->push_play();
}

void EMU::push_stop()
{
	vm->push_stop();
}
#endif

#ifdef USE_BINARY_FILE1
void EMU::load_binary(int drv, _TCHAR* file_path)
{
	vm->load_binary(drv, file_path);
	out_message(_T("Load: %s"), file_path);
}

void EMU::save_binary(int drv, _TCHAR* file_path)
{
	vm->save_binary(drv, file_path);
	out_message(_T("Save: %s"), file_path);
}
#endif

bool EMU::now_skip()
{
	return vm->now_skip();
}

void EMU::update_config()
{
	vm->update_config();
}

