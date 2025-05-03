/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Origin : M88 fdc.cpp / fdu.cpp

	Author : Takeda.Toshiya
	Date   : 2005.06.10-

	[ uPD765A ]
*/

#include <stdio.h>
#include "upd765a.h"
#include "disk.h"
#include "i8237.h"
#include "i8259.h"

#define REGIST_EVENT(phs, usec) { \
	if(phase_id != -1) \
		vm->cancel_callback(phase_id); \
	event_phase = phs; \
	vm->regist_callback(this, EVENT_FDC_PHASE, 100, false, &phase_id); \
}

#define CANCEL_EVENT() { \
	if(phase_id != -1) \
		vm->cancel_callback(phase_id); \
	if(seek_id != -1) \
		vm->cancel_callback(seek_id); \
	if(lost_id != -1) \
		vm->cancel_callback(lost_id); \
	phase_id = seek_id = lost_id = -1; \
}

#define CANCEL_LOST() { \
	if(lost_id != -1) \
		vm->cancel_callback(lost_id); \
	lost_id = -1; \
}

void UPD765A::initialize()
{
	for(int i = 0; i < MAX_DRIVES; i++) {
		fdc[i].track = 0;
		fdc[i].result = 0;
		disk[i] = new DISK();
	}
	_memset(buffer, 0, sizeof(buffer));
	
	phase = prevphase = PHASE_IDLE;
	status = S_RQM;
	seekstat = 0;
	accepttc = int_request = motor_on = ready = false;
	bufptr = buffer; // 暫定
	phase_id = tc_id = seek_id = lost_id = -1;
}

void UPD765A::release()
{
	for(int i = 0; i < MAX_DRIVES; i++)
		if(disk[i])
			delete disk[i];
}

void UPD765A::reset()
{
	shift_to_idle();
	int_request = false;
	CANCEL_EVENT();
	
	intr(false);
	drdy(false);
}

void UPD765A::event_callback(int event_id, int err)
{
	if(event_id == EVENT_FDC_PHASE) {
		phase = event_phase;
		process_cmd(command & 0x1f);
		phase_id = -1;
	}
	else if(event_id == EVENT_FDC_TC) {
		if(accepttc) {
			prevphase = phase;
			phase = PHASE_TC;
			accepttc = false;
			process_cmd(command & 0x1f);
		}
	}
	else if(event_id == EVENT_FDC_SEEK) {
		seek_event(event_drv);
		seek_id = -1;
	}
	else if(event_id == EVENT_FDC_LOST) {
		result = ST1_OR;
		shift_to_result7();
		drdy(false);
		lost_id = -1;
	}
}

void UPD765A::tc_on()
{
	// tc on
#if 0
	CANCEL_EVENT();
	vm->regist_callback(this, EVENT_FDC_TC, 10, false, &tc_id);
#else
	if(accepttc) {
		prevphase = phase;
		phase = PHASE_TC;
		accepttc = false;
		process_cmd(command & 0x1f);
	}
#endif
}

void UPD765A::intr(bool signal)
{
	vm->pic->request_int(6+0, signal);
}

void UPD765A::intr_ndma(bool signal)
{
	// interrupt request in non dma mode
#ifndef DMA_MODE
	vm->pic->request_int(6+0, signal);
#endif
}

void UPD765A::drdy(bool flag)
{
	if(flag) {
		CANCEL_LOST();
		vm->regist_callback(this, EVENT_FDC_LOST, 30000, false, &lost_id);
	}
#ifdef DMA_MODE
	vm->dmac->request_dma(0, flag);
#endif
	ready = flag;
}

// ----------------------------------------------------------------------------
// I/O
// ----------------------------------------------------------------------------

void UPD765A::write_data8(uint16 addr, uint8 data)
{
	write_io8(0x35, data);
}

uint8 UPD765A::read_data8(uint16 addr)
{
	return read_io8(0x35);
}

void UPD765A::write_io8(uint16 addr, uint8 data)
{
	switch(addr & 0xff)
	{
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
			// fdc control
			motor_on = true;
			break;
		case 0x35:
			// fdc data
			if((status & (S_RQM | S_DIO)) == S_RQM) {
				status &= ~S_RQM;
				intr_ndma(false);
				
				switch(phase)
				{
					case PHASE_IDLE:
						command = data;
						process_cmd(command & 0x1f);
						break;
						
					case PHASE_CMD:
						*bufptr++ = data;
						if(--count)
							status |= S_RQM;
						else
							process_cmd(command & 0x1f);
						break;
						
					case PHASE_WRITE:
						*bufptr++ = data;
						if(--count) {
							status |= S_RQM;
							intr_ndma(true);
							CANCEL_LOST();
						}
						else {
							drdy(false);
							status &= ~S_NDM;
							process_cmd(command & 0x1f);
						}
						break;
						
					case PHASE_SCAN:
						if(data != 0xff) {
							if(((command & 0x1f) == 0x11 && *bufptr != data) ||
							   ((command & 0x1f) == 0x19 && *bufptr >  data) ||
							   ((command & 0x1f) == 0x1d && *bufptr <  data))
								result &= ~ST2_SH;
						}
						bufptr++;
						if(--count) {
							status |= S_RQM;
							intr_ndma(true);
							CANCEL_LOST();
						}
						else {
							drdy(false);
							status &= ~S_NDM;
							cmd_scan();
						}
						break;
				}
			}
			break;
	}
}

uint8 UPD765A::read_io8(uint16 addr)
{
	switch(addr & 0xff)
	{
		case 0x34:
			// fdc status
			return seekstat | status;
			
		case 0x35:
			// fdc data
			if((status & (S_RQM | S_DIO)) == (S_RQM | S_DIO)) {
				status &= ~S_RQM;
				intr_ndma(false);
				
				switch(phase)
				{
					case PHASE_RESULT:
					{
						uint8 data = *bufptr++;
						if(--count)
							status |= S_RQM;
						else
							shift_to_idle();
						return data;
					}
					case PHASE_READ:
					{
						uint8 data = *bufptr++;
						if(--count) {
							status |= S_RQM;
							intr_ndma(true);
							CANCEL_LOST();
						}
						else {
							drdy(false);
							status &= ~S_NDM;
							process_cmd(command & 0x1f);
						}
						return data;
					}
				}
			}
			return 0xff;
	}
	return 0xff;
}

// ----------------------------------------------------------------------------
// phase shift
// ----------------------------------------------------------------------------

void UPD765A::shift_to_idle()
{
	phase = PHASE_IDLE;
	status = S_RQM;
	accepttc = false;
}

void UPD765A::shift_to_cmd(int length)
{
	phase = PHASE_CMD;
	status = S_RQM | S_CB;
	accepttc = false;
	bufptr = buffer;
	count = length;
}

void UPD765A::shift_to_exec()
{
	phase = PHASE_EXEC;
	process_cmd(command & 0x1f);
}

void UPD765A::shift_to_read(int length)
{
	phase = PHASE_READ;
	status = S_RQM | S_DIO | S_NDM | S_CB;
	accepttc = true;
	bufptr = buffer;
	count = length;
	
	intr_ndma(true);
	drdy(true);
}

void UPD765A::shift_to_write(int length)
{
	phase = PHASE_WRITE;
	status = S_RQM | S_NDM | S_CB;
	accepttc = true;
	bufptr = buffer;
	count = length;
	
	intr_ndma(true);
	drdy(true);
}

void UPD765A::shift_to_scan(int length)
{
	phase = PHASE_SCAN;
	status = S_RQM | S_NDM | S_CB;
	accepttc = true;
	result = ST2_SH;
	bufptr = buffer;
	count = length;
	
	intr_ndma(true);
	drdy(true);
}

void UPD765A::shift_to_result(int length)
{
	phase = PHASE_RESULT;
	status = S_RQM | S_CB | S_DIO;
	accepttc = false;
	bufptr = buffer;
	count = length;
}

void UPD765A::shift_to_result7()
{
	buffer[0] = (result & 0xf8) | (hdue & 7);
	buffer[1] = uint8(result >>  8);
	buffer[2] = uint8(result >> 16);
	buffer[3] = id[0];
	buffer[4] = id[1];
	buffer[5] = id[2];
	buffer[6] = id[3];
	intr(true);
	shift_to_result(7);
}

// ----------------------------------------------------------------------------
// command
// ----------------------------------------------------------------------------

void UPD765A::process_cmd(int cmd)
{
	switch(cmd & 0x1f)
	{
		case 0x2:
			cmd_read_diagnostic();
			break;
		case 0x3:
			cmd_specify();
			break;
		case 0x4:
			cmd_sence_devstat();
			break;
		case 0x5:
		case 0x9:
			cmd_write_data();
			break;
		case 0x6:
		case 0xc:
			cmd_read_data();
			break;
		case 0x7:
			cmd_recalib();
			break;
		case 0x8:
			cmd_sence_intstat();
			break;
		case 0xa:
			cmd_read_id();
			break;
		case 0xd:
			cmd_write_id();
			break;
		case 0xf:
			cmd_seek();
			break;
		case 0x11:
		case 0x19:
		case 0x1d:
			cmd_scan();
			break;
		default:
			cmd_invalid();
			break;
	}
}

// sence status

void UPD765A::cmd_sence_devstat()
{
	switch(phase)
	{
		case PHASE_IDLE:
			shift_to_cmd(1);
			return;
			
		case PHASE_CMD:
			buffer[0] = get_devstat(buffer[0] & 3);
			shift_to_result(1);
			return;
	}
}

void UPD765A::cmd_sence_intstat()
{
	if(int_request) {
//	if(1) {
		int_request = false;
		buffer[0] = buffer[1] = 0;
		int i;
		for(i = 0; i < 4; i++) {
			if(fdc[i].result) {
				buffer[0] = (uint8)fdc[i].result;
				buffer[1] = (uint8)fdc[i].track;
				fdc[i].result = 0;
				break;
			}
		}
		for (; i < 4; i++) {
			if(fdc[i].result)
				int_request = true;
		}
		if(!int_request)
			intr(false);
		shift_to_result(2);
	}
	else {
		buffer[0] = (uint8)ST0_IC;
		shift_to_result(1);
//		buffer[1] = 0;
//		shift_to_result(2);
	}
	status &= ~S_CB;
}

uint8 UPD765A::get_devstat(int drv)
{
	if(drv >= MAX_DRIVES)
		return 0x80 | drv;
	if(!disk[drv]->insert)
		return drv;
	return 0x28 | drv | (fdc[drv].track ? 0 : 0x10) | ((fdc[drv].track & 1) ? 0x4 : 0) | (disk[drv]->protect ? 0x40 : 0);
}

// seek

void UPD765A::cmd_seek()
{
	switch(phase)
	{
		case PHASE_IDLE:
			shift_to_cmd(2);
			break;
			
		case PHASE_CMD:
			seek(buffer[0] & 3, buffer[1]);
			shift_to_idle();
			break;
	}
}

void UPD765A::cmd_recalib()
{
	switch(phase)
	{
		case PHASE_IDLE:
			shift_to_cmd(1);
			break;
			
		case PHASE_CMD:
			seek(buffer[0] & 3, 0);
			shift_to_idle();
			break;
	}
}

void UPD765A::seek(int drv, int trk)
{
	// 移動量を求める
	int seektime = 100;//(trk == fdc[drv].track) ? 100 : 40 * abs(trk - fdc[drv].track) + 500; //usec
	
	if(get_devstat(drv) & 0x80) {
		// ドライブ番号が不正
		fdc[drv].result = (drv & 3) | ST0_SE | ST0_NR | ST0_AT;
		int_request = true;
		intr(true);
	}
	else {
		fdc[drv].track = trk;
		event_drv = drv;
#if 1
		if(seek_id != -1)
			vm->cancel_callback(seek_id);
		vm->regist_callback(this, EVENT_FDC_SEEK, seektime, false, &seek_id);
		seekstat |= 1 << drv;
#else
		seek_event(drv);
#endif
	}
}

void UPD765A::seek_event(int drv)
{
	int trk = fdc[drv].track;
	
	if(drv >= MAX_DRIVES)
		fdc[drv].result = (drv & 3) | ST0_SE;
	else if(disk[drv]->get_track(trk, 0) || disk[drv]->get_track(trk, 1))
		fdc[drv].result = (drv & 3) | ST0_SE;
	else
		fdc[drv].result = (drv & 3) | ST0_SE | ST0_NR | ST0_AT;
	
	int_request = true;
	intr(true);
	seekstat &= ~(1 << drv);
}

// read/write/scan data, read diagnostics

void UPD765A::cmd_read_data()
{
	switch(phase)
	{
		case PHASE_IDLE:
			shift_to_cmd(8);
			return;
			
		case PHASE_CMD:
			get_sector_params();
			REGIST_EVENT(PHASE_EXEC, 25000 << __min(7, id[3]));
			return;
			
		case PHASE_EXEC:
			read_data((command & 0x1f) == 12 ? true : false, false);
			return;
			
		case PHASE_READ:
			if(result) {
				shift_to_result7();
				return;
			}
			if(!id_incr()) {
				REGIST_EVENT(PHASE_TIMER, 2000);
				return;
			}
			REGIST_EVENT(PHASE_EXEC, 25000 << __min(7, id[3]));
			return;
			
		case PHASE_TC:
			CANCEL_EVENT();
			shift_to_result7();
			return;
			
		case PHASE_TIMER:
//			result = ST0_AT | ST1_EN;
			result = ST1_EN;
			shift_to_result7();
			return;
	}
}

void UPD765A::cmd_write_data()
{
	switch(phase)
	{
		case PHASE_IDLE:
			shift_to_cmd(8);
			return;
			
		case PHASE_CMD:
			get_sector_params();
			REGIST_EVENT(PHASE_EXEC, 20000);
			return;
			
		case PHASE_EXEC:
		{
			result = check_cond(true);
			if(result & ST1_MA) {
				REGIST_EVENT(PHASE_EXEC, 1000000);	// retry
				return;
			}
			if(!result)
				result = find_id();
			if(result) {
				shift_to_result7();
				return;
			}
			int length = 0x80 << __min(8, id[3]);
			if(!id[3]) {
				length = __min(dtl, 0x80);
				_memset(buffer + length, 0, 0x80 - length);
			}
			shift_to_write(length);
			return;
		}
		case PHASE_WRITE:
			write_data((command & 0x1f) == 9 ? true : false);
			if(result) {
				shift_to_result7();
				return;
			}
			phase = PHASE_EXEC;
			if(!id_incr()) {
				REGIST_EVENT(PHASE_TIMER, 2000);
				return;
			}
			REGIST_EVENT(PHASE_EXEC, 10000);
			return;
			
		case PHASE_TIMER:
//			result = ST0_AT | ST1_EN;
			result = ST1_EN;
			shift_to_result7();
			return;
			
		case PHASE_TC:
			CANCEL_EVENT();
			if(prevphase == PHASE_WRITE) {
				// 転送中に停止した？
				_memset(bufptr, 0, count);
				write_data((command & 0x1f) == 9 ? true : false);
			}
			shift_to_result7();
			return;
	}
}

void UPD765A::cmd_scan()
{
	switch(phase)
	{
		case PHASE_IDLE:
			shift_to_cmd(9);
			return;
			
		case PHASE_CMD:
			get_sector_params();
			dtl = dtl | 0x100;
			REGIST_EVENT(PHASE_EXEC, 20000);
			return;
			
		case PHASE_EXEC:
			read_data(false, true);
			return;
			
		case PHASE_SCAN:
			if(result) {
				shift_to_result7();
				return;
			}
			phase = PHASE_EXEC;
			if(!id_incr()) {
				REGIST_EVENT(PHASE_TIMER, 2000);
				return;
			}
			REGIST_EVENT(PHASE_EXEC, 10000);
			return;
			
		case PHASE_TC:
			CANCEL_EVENT();
			shift_to_result7();
			return;
			
		case PHASE_TIMER:
//			result = ST0_AT | ST1_EN;
			result = ST1_EN;
			shift_to_result7();
			return;
	}
}

void UPD765A::cmd_read_diagnostic()
{
	switch(phase)
	{
		case PHASE_IDLE:
			shift_to_cmd(8);
			return;
			
		case PHASE_CMD:
			get_sector_params();
			REGIST_EVENT(PHASE_EXEC, 25000 << __min(7, id[3]));
			return;
			
		case PHASE_EXEC:
			read_diagnostic();
			return;
			
		case PHASE_READ:
			if(result) {
				shift_to_result7();
				return;
			}
			if(!id_incr()) {
				REGIST_EVENT(PHASE_TIMER, 2000);
				return;
			}
			REGIST_EVENT(PHASE_EXEC, 10000);
			return;
			
		case PHASE_TC:
			CANCEL_EVENT();
			shift_to_result7();
			return;
			
		case PHASE_TIMER:
//			result = ST0_AT | ST1_EN;
			result = ST1_EN;
			shift_to_result7();
			return;
	}
}

void UPD765A::read_data(bool deleted, bool scan)
{
	result = check_cond(false);
	if(result & ST1_MA) {
		REGIST_EVENT(PHASE_EXEC, 10000);
		return;
	}
	if(result) {
		shift_to_result7();
		return;
	}
	
	result = read_sector();
	if(deleted)
		result ^= ST2_CM;
	if((result & ~ST2_CM) && !(result & ST2_DD)) {
		shift_to_result7();
		return;
	}
	if((result & ST2_CM) && (command & 0x20)) {
		REGIST_EVENT(PHASE_TIMER, 100000);
		return;
	}
	int length = id[3] ? (0x80 << __min(8, id[3])) : (__min(dtl, 0x80));
	
	if(!scan)
		shift_to_read(length);
	else
		shift_to_scan(length);
	return;
}

void UPD765A::write_data(bool deleted)
{
	if(result = check_cond(true)) {
		shift_to_result7();
		return;
	}
	result = write_sector(deleted);
	return;
}

void UPD765A::read_diagnostic()
{
	int drv = hdu & 3;
	int trk = fdc[drv].track;
	int side = (hdu >> 2) & 1;
	
	result = check_cond(false);
	if(result & ST1_MA) {
		REGIST_EVENT(PHASE_EXEC, 10000);
		return;
	}
	if(result) {
		shift_to_result7();
		return;
	}
	
	if(!disk[drv]->make_track(trk, side)) {
		result = ST1_ND;
		shift_to_result7();
		return;
	}
	
	int length = __min(0x2000, disk[drv]->track_size);
	_memcpy(buffer, disk[drv]->track, length);
	shift_to_read(length);
	return;
}

uint32 UPD765A::read_sector()
{
	int drv = hdu & 3;
	int trk = fdc[drv].track;
	int side = (hdu >> 2) & 1;
	
	// 現在のトラック内のセクタ数取得
	if(!disk[drv]->get_track(trk, side))
		return ST0_AT | ST1_MA;
	int secnum = disk[drv]->sector_num;
	if(!secnum)
		return ST0_AT | ST1_MA;
	
	int cy = -1;
	for(int i = 0; i < secnum; i++) {
		if(!disk[drv]->get_sector(trk, side, i))
			continue;
		cy = disk[drv]->id[0];
		if(disk[drv]->id[2] != id[2])
			continue;
		
		// セクタ番号が一致した
		_memcpy(buffer, disk[drv]->sector, __min(0x2000, disk[drv]->sector_size));
		
		if(disk[drv]->status)
			return ST0_AT | ST1_DE | ST2_DD;
		if(disk[drv]->deleted)
			return ST2_CM;
		return 0;
	}
	if(cy != id[0] && cy != -1) {
		if(cy == 0xff)
			return ST0_AT | ST1_ND | ST2_BC;
		else
			return ST0_AT | ST1_ND | ST2_NC;
	}
	return ST0_AT | ST1_ND;
}

uint32 UPD765A::write_sector(bool deleted)
{
	int drv = hdu & 3;
	int trk = fdc[drv].track;
	int side = (hdu >> 2) & 1;
	
	if(!disk[drv]->insert)
		return ST0_AT | ST1_MA;
	if(disk[drv]->protect)
		return ST0_AT | ST1_NW;
	
	// 現在のトラック内のセクタ数取得
	if(!disk[drv]->get_track(trk, side))
		return ST0_AT | ST1_MA;
	int secnum = disk[drv]->sector_num;
	if(!secnum)
		return ST0_AT | ST1_MA;
	
	int cy = -1;
	for(int i = 0; i < secnum; i++) {
		if(!disk[drv]->get_sector(trk, side, i))
			continue;
		cy = disk[drv]->id[0];
		if(disk[drv]->id[2] != id[2])
			continue;
		
		// セクタ番号が一致した
		int size = 0x80 << __min(8, id[3]);
		_memcpy(disk[drv]->sector, buffer, __min(size, disk[drv]->sector_size));
//		if(deleted)
//			disk[drv]->deleted = ??;//
		return 0;
	}
	if(cy != id[0] && cy != -1) {
		if(cy == 0xff)
			return ST0_AT | ST1_ND | ST2_BC;
		else
			return ST0_AT | ST1_ND | ST2_NC;
	}
	return ST0_AT | ST1_ND;
}

uint32 UPD765A::find_id()
{
	int drv = hdu & 3;
	int trk = fdc[drv].track;
	int side = (hdu >> 2) & 1;
	
	// 現在のトラック内のセクタ数取得
	if(!disk[drv]->get_track(trk, side))
		return ST0_AT | ST1_MA;
	int secnum = disk[drv]->sector_num;
	if(!secnum)
		return ST0_AT | ST1_MA;
	
	int cy = -1;
	for(int i = 0; i < secnum; i++) {
		if(!disk[drv]->get_sector(trk, side, i))
			continue;
		cy = disk[drv]->id[0];
		if(disk[drv]->id[2] != id[2])
			continue;
		
		// セクタ番号が一致した
		return 0;
	}
	if(cy != id[0] && cy != -1) {
		if(cy == 0xff)
			return ST0_AT | ST1_ND | ST2_BC;
		else
			return ST0_AT | ST1_ND | ST2_NC;
	}
	return ST0_AT | ST1_ND;
}

uint32 UPD765A::check_cond(bool write)
{
	int drv = hdu & 3;
	hdue = hdu;
	if(drv >= MAX_DRIVES)
		return ST0_AT | ST0_NR;
	if(!disk[drv]->insert)
		return ST0_AT | ST1_MA;
	return 0;
}

void UPD765A::get_sector_params()
{
	hdu = hdue = buffer[0];
	id[0] = buffer[1];
	id[1] = buffer[2];
	id[2] = buffer[3];
	id[3] = buffer[4];
	eot = buffer[5];
	gpl = buffer[6];
	dtl = buffer[7];
}

bool UPD765A::id_incr()
{
	if((command & 19) == 17) {
		// scan equal
		if((dtl & 0xff) == 0x02)
			id[2]++;
	}
	if(id[2]++ != eot)
		return true;
	
	id[2] = 1;
	if(command & 0x80) {
		hdu ^= 4;
		id[1] ^= 1;
		if (id[1] & 1)
			return true;
	}
	id[0]++;
	return false;
}

// read/write id

void UPD765A::cmd_read_id()
{
	switch(phase)
	{
		case PHASE_IDLE:
			shift_to_cmd(1);
			return;
			
		case PHASE_CMD:
			hdu = buffer[0];
		case PHASE_EXEC:
			if(check_cond(false) & ST1_MA) {
				REGIST_EVENT(PHASE_EXEC, 1000000);
				return;
			}
			REGIST_EVENT(PHASE_TIMER, 5000);
			return;
			
		case PHASE_TIMER:
			result = read_id();
			shift_to_result7();
			return;
	}
}

void UPD765A::cmd_write_id()
{
	switch(phase)
	{
		case PHASE_IDLE:
			shift_to_cmd(5);
			return;
			
		case PHASE_CMD:
			hdu = buffer[0];
			id[3] = buffer[1];
			eot = buffer[2];
			
			if(!eot) {
				REGIST_EVENT(PHASE_TIMER, 1000000);
				return;
			}
			shift_to_write(4 * eot);
			return;
			
		case PHASE_TC:
		case PHASE_WRITE:
			accepttc = false;
			REGIST_EVENT(PHASE_TIMER, 4000000);
			return;
			
		case PHASE_TIMER:
			result =  write_id();
			shift_to_result7();
			return;
	}
}

uint32 UPD765A::read_id()
{
	int drv = hdu & 3;
	int trk = fdc[drv].track;
	int side = (hdu >> 2) & 1;
	
	// 現在のトラック内のセクタ数取得
	if(!disk[drv]->get_track(trk, side))
		return ST0_AT | ST1_MA;
	int secnum = disk[drv]->sector_num;
	if(!secnum)
		return ST0_AT | ST1_MA;
	
	// 適当な実装
	if(disk[drv]->get_sector(trk, side, rand() % secnum)) {
		id[0] = disk[drv]->id[0];
		id[1] = disk[drv]->id[1];
		id[2] = disk[drv]->id[2];
		id[3] = disk[drv]->id[3];
		return 0;
	}
	return ST0_AT | ST1_ND;
}

uint32 UPD765A::write_id()
{
	if(!(result = check_cond(true)))
		result = ST0_AT | ST1_NW;
	return result;
}

// others

void UPD765A::cmd_specify()
{
	switch(phase)
	{
		case PHASE_IDLE:
			shift_to_cmd(2);
			break;
			
		case PHASE_CMD:
			shift_to_idle();
			status = 0x80;//0xff;
			break;
	}
}

void UPD765A::cmd_invalid()
{
	buffer[0] = (uint8)ST0_IC;
	shift_to_result(1);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void UPD765A::insert_disk(_TCHAR path[], int drv)
{
	disk[drv & 3]->open(path);
}

void UPD765A::eject_disk(int drv)
{
	disk[drv & 3]->close();
}

// ----------------------------------------------------------------------------
// unique function for $30
// ----------------------------------------------------------------------------

uint8 UPD765A::fdc_status()
{ 
	return (disk[hdu & 3]->insert ? 8 : 0) | (motor_on ? 0 : 2) | (int_request ? 1 : 0);
}

