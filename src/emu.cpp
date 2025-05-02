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
#ifdef _DEBUG_LOG
	// open debug logfile
	open_debug();
#endif
	
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
	
#ifdef USE_CPU_CLOCK_LOW
	cpu_clock_low = config.cpu_clock_low;
#endif
	
	// initialize
	vm = new VM(this);
	initialize_input();
	initialize_screen();
	initialize_sound();
#ifdef USE_FD1
	initialize_disk_insert();
#endif
#ifdef USE_MEDIA
	initialize_media();
#endif
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
#ifdef USE_MEDIA
	release_media();
#endif
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
	update_timer();
#ifdef USE_FD1
	update_disk_insert();
#endif
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
#ifdef USE_CPU_CLOCK_LOW
	if(cpu_clock_low != config.cpu_clock_low) {
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
		
		// restore inserted floppy disks
#ifdef USE_FD1
		for(int drv = 0; drv < 8; drv++) {
			if(disk_insert[drv].path[0] != _T('\0')) {
				vm->open_disk(drv, disk_insert[drv].path, disk_insert[drv].offset);
			}
		}
#endif
		cpu_clock_low = config.cpu_clock_low;
	}
	else {
#endif
		// reset virtual machine
		vm->reset();
#ifdef USE_CPU_CLOCK_LOW
	}
#endif
	
#ifdef USE_MEDIA
	stop_media();
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
#ifdef USE_MEDIA
	stop_media();
#endif
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

void EMU::application_path(_TCHAR* path)
{
	_tcscpy(path, app_path);
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

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

#ifdef USE_CART
void EMU::open_cart(_TCHAR* file_path)
{
	vm->open_cart(file_path);
	
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
void EMU::open_disk(int drv, _TCHAR* file_path, int offset)
{
	if(vm->disk_inserted(drv) || disk_insert[drv].wait_count != 0) {
		if(vm->disk_inserted(drv)) {
			vm->close_disk(drv);
			// wait 0.5sec
#ifdef SUPPORT_VARIABLE_TIMING
			disk_insert[drv].wait_count = (int)(vm->frame_rate() / 2);
#else
			disk_insert[drv].wait_count = (int)(FRAMES_PER_SEC / 2);
#endif
		}
	}
	else {
		vm->open_disk(drv, file_path, offset);
	}
	_tcscpy(disk_insert[drv].path, file_path);
	disk_insert[drv].offset = offset;
}
void EMU::close_disk(int drv)
{
	vm->close_disk(drv);
	disk_insert[drv].path[0] = _T('\0');
}
void EMU::initialize_disk_insert()
{
	memset(disk_insert, 0, sizeof(disk_insert));
}
void EMU::update_disk_insert()
{
	for(int drv = 0; drv < 8; drv++) {
		if(disk_insert[drv].wait_count != 0 && --disk_insert[drv].wait_count == 0) {
			vm->open_disk(drv, disk_insert[drv].path, disk_insert[drv].offset);
		}
	}
}
#endif

#ifdef USE_QUICKDISK
void EMU::open_quickdisk(_TCHAR* file_path)
{
	vm->open_quickdisk(file_path);
}
void EMU::close_quickdisk()
{
	vm->close_quickdisk();
}
#endif

#ifdef USE_DATAREC
void EMU::play_datarec(_TCHAR* file_path)
{
	vm->play_datarec(file_path);
}
void EMU::rec_datarec(_TCHAR* file_path)
{
	vm->rec_datarec(file_path);
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

#ifdef USE_BINARY_FILE1
void EMU::load_binary(int drv, _TCHAR* file_path)
{
	vm->load_binary(drv, file_path);
}
void EMU::save_binary(int drv, _TCHAR* file_path)
{
	vm->save_binary(drv, file_path);
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

// ----------------------------------------------------------------------------
// misc
// ----------------------------------------------------------------------------

uint32 EMU::getcrc32(uint8 data[], int size)
{
	uint32 c, table[256];
	for(int i = 0; i < 256; i++) {
		uint32 c = i;
		for(int j = 0; j < 8; j++) {
			if(c & 1) {
				c = (c >> 1) ^ 0xedb88320;
			}
			else {
				c >>= 1;
			}
		}
		table[i] = c;
	}
	c = ~0;
	for(int i = 0; i < size; i++) {
		c = table[(c ^ data[i]) & 0xff] ^ (c >> 8);
	}
	return ~c;
}

