/*
	SHARP MZ-1500 Emulator 'EmuZ-1500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2011.02.17-

	[ quick disk ]
*/

#ifndef _QUICKDISK_H_
#define _QUICKDISK_H_

#include "../vm.h"
#include "../../emu.h"
#include "../device.h"

#define QUICKDISK_SIO_RTSA	0
#define QUICKDISK_SIO_DTRB	1
#define QUICKDISK_SIO_SYNC	2
#define QUICKDISK_SIO_RXDONE	3
#define QUICKDISK_SIO_DATA	4
#define QUICKDISK_SIO_BREAK	5
#define QUICKDISK_SIO_TXDONE	6

#define QUICKDISK_BUFFER_SIZE	65536

class QUICKDISK : public DEVICE
{
private:
	DEVICE *d_sio;
	
	_TCHAR file_path[_MAX_PATH];
	bool insert, protect, home;
	bool modified;
	bool accessed;
	
	uint16 buffer[QUICKDISK_BUFFER_SIZE];
	int buffer_ptr;
	int break_step;
	bool first_data;
	
	bool wrga, mton, sync;
	bool motor_on;
	int restore_id, step_id, end_id;
	
	void restore();
	void end_of_disk();
	void set_insert(bool val);
	void set_protect(bool val);
	void set_home(bool val);
	
public:
	QUICKDISK(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~QUICKDISK() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_signal(int id, uint32 data, uint32 mask);
	uint32 read_signal(int ch);
	void event_callback(int event_id, int err);
	
	// unique functions
	void set_context_sio(DEVICE* device) {
		d_sio = device;
	}
	void open_disk(_TCHAR path[]);
	void close_disk();
};

#endif

