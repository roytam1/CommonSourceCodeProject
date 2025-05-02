/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.11-

	[ RP-5C01 ]
*/

#include "rp5c01.h"
#include "../fileio.h"

#define EVENT_1HZ	0
#define EVENT_16HZ	1

#define YEAR		time[0]
#define MONTH		time[1]
#define DAY		time[2]
#define DAY_OF_WEEK	time[3]
#define HOUR		time[4]
#define MINUTE		time[5]
#define SECOND		time[6]

void RP5C01::initialize()
{
	// load ram image
	_memset(ram, 0, sizeof(ram));
	
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sRP5C01.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
	
	// initialize
	_memset(regs, 0, sizeof(regs));
	regs[0x0a] = 1;
	regs[0x0d] = 8;
	regs[0x0f] = 0xc;
	alarm = pulse_1hz = pulse_16hz = false;
	
	// register event
	vm->register_event(this, EVENT_1HZ, 500000, true, NULL);
	vm->register_event(this, EVENT_16HZ, 31250, true, NULL);
	vm->register_frame_event(this);
}

void RP5C01::release()
{
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sRP5C01.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(ram, sizeof(ram), 1);
		fio->Fclose();
	}
	delete fio;
}

void RP5C01::write_io8(uint32 addr, uint32 data)
{
	int ch = addr & 0x0f;
	int mode = regs[0x0d] & 3;
	
	switch(ch) {
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0c:
		if(mode == 1) {
			regs[ch] = data;
		}
		else if(mode == 2) {
			ram[ch] = data;
		}
		else if(mode == 3) {
			ram[13 + ch] = data;
		}
		break;
	case 0x0d:
	case 0x0e:
	case 0x0f:
		regs[ch] = data;
		break;
	}
}

uint32 RP5C01::read_io8(uint32 addr)
{
	int ch = addr & 0x0f;
	int mode = regs[0x0d] & 3;
	
	switch(ch) {
	case 0x00:
		return (mode == 1) ? regs[ 0] : (mode == 2) ? ram[ 0] : (mode == 3) ? ram[13] : SECOND % 10;
	case 0x01:
		return (mode == 1) ? regs[ 1] : (mode == 2) ? ram[ 1] : (mode == 3) ? ram[14] : (uint8)(SECOND / 10);
	case 0x02:
		return (mode == 1) ? regs[ 2] : (mode == 2) ? ram[ 2] : (mode == 3) ? ram[15] : MINUTE % 10;
	case 0x03:
		return (mode == 1) ? regs[ 3] : (mode == 2) ? ram[ 3] : (mode == 3) ? ram[16] : (uint8)(MINUTE / 10);
	case 0x04:
		return (mode == 1) ? regs[ 4] : (mode == 2) ? ram[ 4] : (mode == 3) ? ram[17] : ((regs[0x0a] & 1) ? HOUR : HOUR % 12) % 10;
	case 0x05:
		return (mode == 1) ? regs[ 5] : (mode == 2) ? ram[ 5] : (mode == 3) ? ram[18] : (uint8)((regs[0x0a] & 1) ? HOUR / 10 : ((HOUR % 12) / 10) | (HOUR < 12 ? 0 : 2));
	case 0x06:
		return (mode == 1) ? regs[ 6] : (mode == 2) ? ram[ 6] : (mode == 3) ? ram[19] : DAY_OF_WEEK;
	case 0x07:
		return (mode == 1) ? regs[ 7] : (mode == 2) ? ram[ 7] : (mode == 3) ? ram[20] : DAY % 10;
	case 0x08:
		return (mode == 1) ? regs[ 8] : (mode == 2) ? ram[ 8] : (mode == 3) ? ram[21] : (uint8)(DAY / 10);
	case 0x09:
		return (mode == 1) ? regs[ 9] : (mode == 2) ? ram[ 9] : (mode == 3) ? ram[22] : MONTH % 10;
	case 0x0a:
		return (mode == 1) ? regs[10] : (mode == 2) ? ram[10] : (mode == 3) ? ram[23] : (uint8)(MONTH / 10);
	case 0x0b:
		if(mode == 1) {
			if(((YEAR - 0) % 4) == 0 && (((YEAR - 0) % 100) != 0 || ((YEAR - 0) % 400) == 0)) {
				return 0;
			}
			else if(((YEAR - 1) % 4) == 0 && (((YEAR - 1) % 100) != 0 || ((YEAR - 1) % 400) == 0)) {
				return 1;
			}
			else if(((YEAR - 2) % 4) == 0 && (((YEAR - 2) % 100) != 0 || ((YEAR - 2) % 400) == 0)) {
				return 2;
			}
			else {
				return 3;
			}
		}
		return                          (mode == 2) ? ram[11] : (mode == 3) ? ram[24] : YEAR % 10;
	case 0x0c:
		return (mode == 1) ? regs[12] : (mode == 2) ? ram[12] : (mode == 3) ? ram[25] : (uint8)((YEAR % 100) / 10);
	case 0x0d:
	case 0x0e:
	case 0x0f:
		return regs[ch];
	}
	return 0xff;
}

void RP5C01::event_callback(int event_id, int err)
{
	if(event_id == EVENT_1HZ) {
		pulse_1hz = !pulse_1hz;
	}
	else {
		pulse_16hz = !pulse_16hz;
	}
	bool pulse = alarm;
	pulse |= (regs[0x0f] & 8) ? false : pulse_1hz;
	pulse |= (regs[0x0f] & 4) ? false : pulse_16hz;
	
	write_signals(&outputs_pulse, pulse ? 0 : 0xffffffff);
}

void RP5C01::event_frame()
{
	// update clock
	emu->get_timer(time);
	
	int minute, hour, day;
	minute = (regs[2] & 0x0f) + (regs[3] & 7) * 10;
	if(regs[0x0a] & 1) {
		hour = (regs[4] & 0x0f) + (regs[5] & 3) * 10;
	}
	else {
		hour = (regs[4] & 0x0f) + (regs[5] & 1) * 10 + (regs[5] & 2 ? 12 : 0);
	}
	day = (regs[7] & 0x0f) + (regs[8] & 3) * 10;
	bool newval = ((regs[0x0d] & 4) && minute == MINUTE && hour == HOUR && day == DAY) ? true : false;
	
	if(alarm != newval) {
		write_signals(&outputs_alarm, newval ? 0 : 0xffffffff);
		alarm = newval;
	}
}

