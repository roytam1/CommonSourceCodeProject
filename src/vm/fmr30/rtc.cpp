/*
	FUJITSU FMR-30 Emulator 'eFMR-30'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.12.30 -

	[ rtc ]
*/

#include "rtc.h"
#include "../i8259.h"
#include "../../fileio.h"

#define EVENT_1HZ	0
#define EVENT_32HZ	1
#define EVENT_DONE	2

#define POWON	8
#define TCNT	34
#define CKHM	35
#define CKL	36
#define POWOF	36
#define POFMI	37
#define POFH	38
#define POFD	39

#define YEAR		time[0]
#define MONTH		time[1]
#define DAY		time[2]
#define DAY_OF_WEEK	time[3]
#define HOUR		time[4]
#define MINUTE		time[5]
#define SECOND		time[6]

#define BCD(d) (((d) % 10) | ((uint8)((d) / 10) << 4))

void RTC::initialize()
{
	// load rtc regs image
	_memset(regs, 0, sizeof(regs));
	regs[POWON] = 0x10;	// cleared
	
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sRTC.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(regs + 8, 32, 1);
		fio->Fclose();
	}
	delete fio;
	
	// init registers
//	regs[POWON] &= 0x1f;	// local power on
//	regs[POWOF] = 0x80;	// program power off

	regs[POWON] = 0x10;	// cleared
	regs[POWOF] = 0x20;	// illegal power off


	regs[TCNT] = 0;
	update_checksum();
	
	rtcmr = rtdsr = 0;
	
	// update calendar
	update_calendar();
	
	// register event
	vm->register_event_by_clock(this, EVENT_1HZ, CPU_CLOCKS, true, NULL);
	vm->register_event_by_clock(this, EVENT_32HZ, CPU_CLOCKS >> 5, true, NULL);
}

void RTC::release()
{
	// set power off time
	regs[POFMI] = BCD(MINUTE);
	regs[POFH] = BCD(HOUR);
	regs[POFD] = BCD(DAY);
	
	// save rtc regs image
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sRTC.BIN"), app_path);
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		fio->Fwrite(regs + 8, 32, 1);
		fio->Fclose();
	}
	delete fio;
}

void RTC::write_io16(uint32 addr, uint32 data)
{
	switch(addr) {
	case 0:
		rtcmr = data;
		break;
	case 2:
		// echo reset
		rtdsr &= ~(data & 0xe);
		update_intr();
		break;
	case 4:
		if(!(rtdsr & 1)) {
			rtadr = data;
			rtdsr |= 1;
			// register event
			vm->register_event(this, EVENT_DONE, 100, false, NULL);
		}
		break;
	case 6:
		rtobr = data;
		break;
	}
}

uint32 RTC::read_io16(uint32 addr)
{
	switch(addr) {
	case 2:
		return rtdsr;
	case 6:
		return rtibr;
	}
	return 0xff;
}

void RTC::event_callback(int event_id, int err)
{
	if(event_id == EVENT_1HZ) {
		// update calendar
		update_calendar();
		
		// 1sec interrupt
		rtdsr |= 4;
		update_intr();
	}
	else if(event_id == EVENT_32HZ) {
		// update tcnt
		regs[TCNT]++;
	}
	else if(event_id == EVENT_DONE) {
		int ch = (rtadr >> 1) & 0x3f;
		if(rtadr & 1) {
			// invalid address
		}
		else if(rtadr & 0x80) {
			// write
			if(ch == POWON) {
				regs[ch] = (regs[ch] & 0xe0) | (rtobr & 0x1f);
				if((rtobr & 0xe0) == 0xc0) {
					// reipl
					regs[ch] = (regs[ch] & 0x1f) | 0xc0;
					vm->reset();
				}
				else if((rtobr & 0xe0) == 0xe0) {
					// power off
					emu->power_off();
				}
				update_checksum();
			}
			else if(7 <= ch && ch < 32) {
				regs[ch] = (uint8)rtobr;
				update_checksum();
			}
		}
		else {
			// read
			if(ch < 40) {
				rtibr = regs[ch];
			}
		}
		// update flags
		rtdsr &= ~1;
		rtdsr |= 2;
		update_intr();
	}
}

void RTC::update_calendar()
{
	emu->get_timer(time);
	regs[0] = BCD(SECOND);
	regs[1] = BCD(MINUTE);
	regs[2] = BCD(HOUR);
	regs[3] = DAY_OF_WEEK;
	regs[4] = BCD(DAY);
	regs[5] = BCD(MONTH);
	regs[6] = BCD(YEAR);
}

void RTC::update_checksum()
{
	int sum = 0;
	for(int i = 8; i < 32; i++) {
		sum += regs[i] & 0xf;
		sum += (regs[i] >> 4) & 0xf;
	}
	uint8 ckh = (sum >> 6) & 0xf;
	uint8 ckm = (sum >> 2) & 0xf;
	uint8 ckl = (sum >> 0) & 3;
	
	regs[CKHM] = ckh | (ckm << 4);
	regs[CKL] = (regs[CKL] & 0xf0) | ckl | 0xc;
}

void RTC::update_intr()
{
	d_pic->write_signal(SIG_I8259_CHIP0 | SIG_I8259_IR1, (rtcmr & rtdsr & 0xe) ? 1 : 0, 1);
}
