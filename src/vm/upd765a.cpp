/*
	Skelton for retropc emulator

	Origin : M88
	Author : Takeda.Toshiya
	Date   : 2006.09.17-

	[ uPD765A ]
*/

#include "upd765a.h"
#include "disk.h"

#define EVENT_PHASE	0
#define EVENT_SEEK	1
#define EVENT_DRQ	2
#define EVENT_LOST	3

#define PHASE_IDLE	0
#define PHASE_CMD	1
#define PHASE_EXEC	2
#define PHASE_READ	3
#define PHASE_WRITE	4
#define PHASE_SCAN	5
#define PHASE_TC	6
#define PHASE_TIMER	7
#define PHASE_RESULT	8

#define S_D0B	0x01
#define S_D1B	0x02
#define S_D2B	0x04
#define S_D3B	0x08
#define S_CB	0x10
#define S_NDM	0x20
#define S_DIO	0x40
#define S_RQM	0x80

#define ST0_NR	0x000008
#define ST0_EC	0x000010
#define ST0_SE	0x000020
#define ST0_AT	0x000040
#define ST0_IC	0x000080
#define ST0_AI	0x0000c0

#define ST1_MA	0x000100
#define ST1_NW	0x000200
#define ST1_ND	0x000400
#define ST1_OR	0x001000
#define ST1_DE	0x002000
#define ST1_EN	0x008000

#define ST2_MD	0x010000
#define ST2_BC	0x020000
#define ST2_SN	0x040000
#define ST2_SH	0x080000
#define ST2_NC	0x100000
#define ST2_DD	0x200000
#define ST2_CM	0x400000

#define ST3_HD	0x04
#define ST3_TS	0x08
#define ST3_T0	0x10
#define ST3_RY	0x20
#define ST3_WP	0x40
#define ST3_FT	0x80

#define REGIST_EVENT(phs, usec) { \
	if(phase_id != -1) \
		vm->cancel_event(phase_id); \
	event_phase = phs; \
	vm->regist_event(this, EVENT_PHASE, 100, false, &phase_id); \
}

#define CANCEL_EVENT() { \
	if(phase_id != -1) \
		vm->cancel_event(phase_id); \
	if(seek_id != -1) \
		vm->cancel_event(seek_id); \
	if(drq_id != -1) \
		vm->cancel_event(drq_id); \
	if(lost_id != -1) \
		vm->cancel_event(lost_id); \
	phase_id = seek_id = drq_id = lost_id = -1; \
}

#define CANCEL_DRQ() { \
	if(drq_id != -1) \
		vm->cancel_event(drq_id); \
	drq_id = -1; \
}

#define CANCEL_LOST() { \
	if(lost_id != -1) \
		vm->cancel_event(lost_id); \
	lost_id = -1; \
}

void UPD765A::initialize()
{
	// initialize d88 handler
	for(int i = 0; i < MAX_DRIVE; i++)
		disk[i] = new DISK();
	
	// initialize fdc
	for(int i = 0; i < MAX_DRIVE; i++) {
		fdc[i].track = 0;
		fdc[i].result = 0;
		fdc[i].access = false;
	}
	_memset(buffer, 0, sizeof(buffer));
	
	phase = prevphase = PHASE_IDLE;
	status = S_RQM;
	seekstat = 0;
	bufptr = buffer; // temporary
	phase_id = seek_id = drq_id = lost_id = -1;
	motor = false;	// motor off
	
	set_intr(false);
	set_drq(false);
	set_hdu(0);
	set_acctc(false);
}

void UPD765A::release()
{
	for(int i = 0; i < MAX_DRIVE; i++) {
		if(disk[i])
			delete disk[i];
	}
}

void UPD765A::reset()
{
	shift_to_idle();
	CANCEL_EVENT();
	
	set_intr(false);
	set_drq(false);
}

void UPD765A::write_dma8(uint32 addr, uint32 data)
{
#ifdef UPD765A_DMA_MODE
	dma_data_lost = false;
#endif
	write_io8(1, data);
}

uint32 UPD765A::read_dma8(uint32 addr)
{
#ifdef UPD765A_DMA_MODE
	dma_data_lost = false;
#endif
	return read_io8(1);
}

void UPD765A::write_io8(uint32 addr, uint32 data)
{
	// fdc data
	if((status & (S_RQM | S_DIO)) == S_RQM) {
		status &= ~S_RQM;
//		req_intr_ndma(false);
		
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
//				req_intr_ndma(true);
#ifdef UPD765A_DRQ_DELAY
				set_drq(false);
				CANCEL_DRQ();
				vm->regist_event(this, EVENT_DRQ, 50, false, &drq_id);
#else
				status |= S_RQM;
#endif
				CANCEL_LOST();
			}
			else {
#ifdef UPD765A_DRQ_DELAY
					CANCEL_DRQ();
#endif
				status &= ~S_NDM;
				set_drq(false);
				process_cmd(command & 0x1f);
			}
			fdc[hdu & DRIVE_MASK].access = true;
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
//				req_intr_ndma(true);
#ifdef UPD765A_DRQ_DELAY
				set_drq(false);
				CANCEL_DRQ();
				vm->regist_event(this, EVENT_DRQ, 50, false, &drq_id);
#else
				status |= S_RQM;
#endif
				CANCEL_LOST();
			}
			else {
#ifdef UPD765A_DRQ_DELAY
					CANCEL_DRQ();
#endif
				status &= ~S_NDM;
				set_drq(false);
				cmd_scan();
			}
			fdc[hdu & DRIVE_MASK].access = true;
			break;
		}
	}
}

uint32 UPD765A::read_io8(uint32 addr)
{
	if(addr & 1) {
		// fdc data
		if((status & (S_RQM | S_DIO)) == (S_RQM | S_DIO)) {
			uint8 data;
			status &= ~S_RQM;
//			req_intr_ndma(false);
			
			switch(phase)
			{
			case PHASE_RESULT:
				data = *bufptr++;
				if(--count)
					status |= S_RQM;
				else {
					CANCEL_LOST();
					shift_to_idle();
				}
				return data;
				
			case PHASE_READ:
				data = *bufptr++;
				if(--count) {
//					req_intr_ndma(true);
#ifdef UPD765A_DRQ_DELAY
					set_drq(false);
					CANCEL_DRQ();
					vm->regist_event(this, EVENT_DRQ, 50, false, &drq_id);
#else
					status |= S_RQM;
#endif
					CANCEL_LOST();
				}
				else {
#ifdef UPD765A_DRQ_DELAY
					CANCEL_DRQ();
#endif
					status &= ~S_NDM;
					set_drq(false);
					process_cmd(command & 0x1f);
				}
				fdc[hdu & DRIVE_MASK].access = true;
				return data;
			}
		}
		return 0xff;
	}
	else
		// fdc status
		return seekstat | status;
}

void UPD765A::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_UPD765A_TC) {
		if((data & mask) && acctc) {
			// tc on
			prevphase = phase;
			phase = PHASE_TC;
			set_acctc(false);
			process_cmd(command & 0x1f);
		}
		else if(!(data & mask)) {
			// tc off
		}
	}
	else if(id == SIG_UPD765A_MOTOR)
		motor = ((data & mask) != 0);
	else if(id == SIG_UPD765A_SELECT)
		sel = data & DRIVE_MASK;
}

uint32 UPD765A::read_signal(int ch)
{
	// get access status
	uint32 stat = 0;
	for(int i = 0; i < MAX_DRIVE; i++) {
		if(fdc[i].access)
			stat |= 1 << i;
		fdc[i].access = false;
	}
	return stat;
}

void UPD765A::event_callback(int event_id, int err)
{
	if(event_id == EVENT_PHASE) {
		phase = event_phase;
		process_cmd(command & 0x1f);
		phase_id = -1;
	}
	else if(event_id == EVENT_SEEK) {
		seek_event(event_drv);
		seek_id = -1;
	}
	else if(event_id == EVENT_DRQ) {
		status |= S_RQM;
		set_drq(true);
		drq_id = -1;
	}
	else if(event_id == EVENT_LOST) {
		result = ST1_OR;
		shift_to_result7();
		set_drq(false);
		lost_id = -1;
	}
}

void UPD765A::set_intr(bool val)
{
	req_intr(val);
	intr = val;
}

void UPD765A::req_intr(bool val)
{
	for(int i = 0; i < dcount_intr; i++)
		d_intr[i]->write_signal(did_intr[i], val ? 0xffffffff : 0, dmask_intr[i]);
}

void UPD765A::req_intr_ndma(bool val)
{
#ifndef UPD765A_DMA_MODE
	for(int i = 0; i < dcount_intr; i++)
		d_intr[i]->write_signal(did_intr[i], val ? 0xffffffff : 0, dmask_intr[i]);
#endif
}

void UPD765A::set_drq(bool val)
{
#ifdef UPD765A_DMA_MODE
	if(val) {
		drq = dma_data_lost = true;
		for(int i = 0; i < dcount_drq; i++)
			d_drq[i]->write_signal(did_drq[i], 0xffffffff, dmask_drq[i]);
		if(!dma_data_lost)
			return;
		// data lost if dma request is not accepted
		result = ST1_OR;
		shift_to_result7();
	}
	drq = false;
	for(int i = 0; i < dcount_drq; i++)
		d_drq[i]->write_signal(did_drq[i], 0, 0);
#else
	CANCEL_LOST();
	if(val)
		vm->regist_event(this, EVENT_LOST, 30000, false, &lost_id);
	drq = val;
	for(int i = 0; i < dcount_drq; i++)
		d_drq[i]->write_signal(did_drq[i], val ? 0xffffffff : 0, dmask_drq[i]);
#endif
}

void UPD765A::set_hdu(uint8 val)
{
	for(int i = 0; i < dcount_hdu; i++)
		d_hdu[i]->write_signal(did_hdu[i], val, dmask_hdu[i]);
	hdu = val;
}

void UPD765A::set_acctc(bool val)
{
	for(int i = 0; i < dcount_acctc; i++)
		d_acctc[i]->write_signal(did_acctc[i], val ? 0xffffffff : 0, dmask_acctc[i]);
	acctc = val;
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

void UPD765A::cmd_sence_devstat()
{
	switch(phase)
	{
	case PHASE_IDLE:
		shift_to_cmd(1);
		break;
	case PHASE_CMD:
		buffer[0] = get_devstat(buffer[0] & DRIVE_MASK);
		shift_to_result(1);
		break;
	}
}

void UPD765A::cmd_sence_intstat()
{
	if(intr) {
		intr = false;
		buffer[0] = buffer[1] = 0;
		int i;
		for(i = 0; i < MAX_DRIVE; i++) {
			if(fdc[i].result) {
				buffer[0] = (uint8)fdc[i].result;
				buffer[1] = (uint8)fdc[i].track;
				fdc[i].result = 0;
				break;
			}
		}
		for(; i < MAX_DRIVE; i++) {
			if(fdc[i].result)
				intr = true;
		}
		if(!intr)
			set_intr(intr);
		shift_to_result(2);
	}
	else {
		buffer[0] = (uint8)ST0_IC;
		shift_to_result(1);
//		buffer[1] = 0;
//		shift_to_result(2);
	}
//	status &= ~S_CB;
}

uint8 UPD765A::get_devstat(int drv)
{
	if(drv >= MAX_DRIVE)
		return 0x80 | drv;
	if(!disk[drv]->insert)
		return drv;
	return 0x28 | drv | (fdc[drv].track ? 0 : 0x10) | ((fdc[drv].track & 1) ? 0x4 : 0) | (disk[drv]->protect ? 0x40 : 0);
}

void UPD765A::cmd_seek()
{
	switch(phase)
	{
	case PHASE_IDLE:
		shift_to_cmd(2);
		break;
	case PHASE_CMD:
		seek(buffer[0] & DRIVE_MASK, buffer[1]);
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
		seek(buffer[0] & DRIVE_MASK, 0);
		shift_to_idle();
		break;
	}
}

void UPD765A::seek(int drv, int trk)
{
	// get distance
	int seektime = 100;//(trk == fdc[drv].track) ? 100 : 40 * abs(trk - fdc[drv].track) + 500; //usec
	
	if(get_devstat(drv) & 0x80) {
		// invalid drive number
		fdc[drv].result = (drv & DRIVE_MASK) | ST0_SE | ST0_NR | ST0_AT;
		set_intr(true);
	}
	else {
		fdc[drv].track = trk;
		event_drv = drv;
#ifdef UPD765A_WAIT_SEEK
		if(seek_id != -1)
			vm->cancel_event(seek_id);
		vm->regist_event(this, EVENT_SEEK, seektime, false, &seek_id);
		seekstat |= 1 << drv;
#else
		seek_event(drv);
#endif
	}
}

void UPD765A::seek_event(int drv)
{
	int trk = fdc[drv].track;
	
	if(drv >= MAX_DRIVE)
		fdc[drv].result = (drv & DRIVE_MASK) | ST0_SE;
	else if(!disk[drv]->insert)
		fdc[drv].result = (drv & DRIVE_MASK) | ST0_SE | ST0_NR;
	else if(disk[drv]->get_track(trk, 0) || disk[drv]->get_track(trk, 1))
		fdc[drv].result = (drv & DRIVE_MASK) | ST0_SE;
	else
		fdc[drv].result = (drv & DRIVE_MASK) | ST0_SE | ST0_NR | ST0_AT;
	set_intr(true);
	seekstat &= ~(1 << drv);
}

void UPD765A::cmd_read_data()
{
	switch(phase)
	{
	case PHASE_IDLE:
		shift_to_cmd(8);
		break;
	case PHASE_CMD:
		get_sector_params();
		REGIST_EVENT(PHASE_EXEC, 25000 << __min(7, id[3]));
		break;
	case PHASE_EXEC:
		read_data((command & 0x1f) == 12 ? true : false, false);
		break;
	case PHASE_READ:
		if(result) {
			shift_to_result7();
			break;
		}
		if(!id_incr()) {
			REGIST_EVENT(PHASE_TIMER, 2000);
			break;
		}
		REGIST_EVENT(PHASE_EXEC, 25000 << __min(7, id[3]));
		break;
	case PHASE_TC:
		CANCEL_EVENT();
		shift_to_result7();
		break;
	case PHASE_TIMER:
//		result = ST0_AT | ST1_EN;
		result = ST1_EN;
		shift_to_result7();
		break;
	}
}

void UPD765A::cmd_write_data()
{
	switch(phase)
	{
	case PHASE_IDLE:
		shift_to_cmd(8);
		break;
	case PHASE_CMD:
		get_sector_params();
		REGIST_EVENT(PHASE_EXEC, 20000);
		break;
	case PHASE_EXEC:
		result = check_cond(true);
		if(result & ST1_MA) {
			REGIST_EVENT(PHASE_EXEC, 1000000);	// retry
			break;
		}
		if(!result)
			result = find_id();
		if(result)
			shift_to_result7();
		else {
			int length = 0x80 << __min(8, id[3]);
			if(!id[3]) {
				length = __min(dtl, 0x80);
				_memset(buffer + length, 0, 0x80 - length);
			}
			shift_to_write(length);
		}
		break;
	case PHASE_WRITE:
		write_data((command & 0x1f) == 9 ? true : false);
		if(result) {
			shift_to_result7();
			break;
		}
		phase = PHASE_EXEC;
		if(!id_incr()) {
			REGIST_EVENT(PHASE_TIMER, 2000);
			break;
		}
		REGIST_EVENT(PHASE_EXEC, 10000);
		break;
	case PHASE_TIMER:
//		result = ST0_AT | ST1_EN;
		result = ST1_EN;
		shift_to_result7();
		break;
	case PHASE_TC:
		CANCEL_EVENT();
		if(prevphase == PHASE_WRITE) {
			// terminate while transfer ?
			_memset(bufptr, 0, count);
			write_data((command & 0x1f) == 9 ? true : false);
		}
		shift_to_result7();
		break;
	}
}

void UPD765A::cmd_scan()
{
	switch(phase)
	{
	case PHASE_IDLE:
		shift_to_cmd(9);
		break;
	case PHASE_CMD:
		get_sector_params();
		dtl = dtl | 0x100;
		REGIST_EVENT(PHASE_EXEC, 20000);
		break;
	case PHASE_EXEC:
		read_data(false, true);
		break;
	case PHASE_SCAN:
		if(result) {
			shift_to_result7();
			break;
		}
		phase = PHASE_EXEC;
		if(!id_incr()) {
			REGIST_EVENT(PHASE_TIMER, 2000);
			break;
		}
		REGIST_EVENT(PHASE_EXEC, 10000);
		break;
	case PHASE_TC:
		CANCEL_EVENT();
		shift_to_result7();
		break;
	case PHASE_TIMER:
//		result = ST0_AT | ST1_EN;
		result = ST1_EN;
		shift_to_result7();
		break;
	}
}

void UPD765A::cmd_read_diagnostic()
{
	switch(phase)
	{
	case PHASE_IDLE:
		shift_to_cmd(8);
		break;
	case PHASE_CMD:
		get_sector_params();
		REGIST_EVENT(PHASE_EXEC, 25000 << __min(7, id[3]));
		break;
	case PHASE_EXEC:
		read_diagnostic();
		break;
	case PHASE_READ:
		if(result) {
			shift_to_result7();
			break;
		}
		if(!id_incr()) {
			REGIST_EVENT(PHASE_TIMER, 2000);
			break;
		}
		REGIST_EVENT(PHASE_EXEC, 10000);
		break;
	case PHASE_TC:
		CANCEL_EVENT();
		shift_to_result7();
		break;
	case PHASE_TIMER:
//		result = ST0_AT | ST1_EN;
		result = ST1_EN;
		shift_to_result7();
		break;
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
	int drv = hdu & DRIVE_MASK;
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
	int drv = hdu & DRIVE_MASK;
	int trk = fdc[drv].track;
	int side = (hdu >> 2) & 1;
	
	// get sector counts in the current track
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
#ifdef UPD765A_STRICT_ID
		if(disk[drv]->id[0] != id[0] || disk[drv]->id[1] != id[1] || disk[drv]->id[3] != id[3])
			continue;
#endif
		if(disk[drv]->id[2] != id[2])
			continue;
		// sector number is matched
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
	int drv = hdu & DRIVE_MASK;
	int trk = fdc[drv].track;
	int side = (hdu >> 2) & 1;
	
	if(!disk[drv]->insert)
		return ST0_AT | ST1_MA;
	if(disk[drv]->protect)
		return ST0_AT | ST1_NW;
	
	// get sector counts in the current track
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
#ifdef UPD765A_STRICT_ID
		if(disk[drv]->id[0] != id[0] || disk[drv]->id[1] != id[1] || disk[drv]->id[3] != id[3])
			continue;
#endif
		if(disk[drv]->id[2] != id[2])
			continue;
		// sector number is matched
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
	int drv = hdu & DRIVE_MASK;
	int trk = fdc[drv].track;
	int side = (hdu >> 2) & 1;
	
	// get sector counts in the current track
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
#ifdef UPD765A_STRICT_ID
		if(disk[drv]->id[0] != id[0] || disk[drv]->id[1] != id[1] || disk[drv]->id[3] != id[3])
			continue;
#endif
		if(disk[drv]->id[2] != id[2])
			continue;
		// sector number is matched
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
	int drv = hdu & DRIVE_MASK;
	hdue = hdu;
	if(drv >= MAX_DRIVE)
		return ST0_AT | ST0_NR;
	if(!disk[drv]->insert)
		return ST0_AT | ST1_MA;
	return 0;
}

void UPD765A::get_sector_params()
{
	set_hdu(buffer[0]);
	hdue = buffer[0];
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
		set_hdu(hdu ^ 4);
		id[1] ^= 1;
		if (id[1] & 1)
			return true;
	}
	id[0]++;
	return false;
}

void UPD765A::cmd_read_id()
{
	switch(phase)
	{
	case PHASE_IDLE:
		shift_to_cmd(1);
		break;
	case PHASE_CMD:
		set_hdu(buffer[0]);
//		break;
	case PHASE_EXEC:
		if(check_cond(false) & ST1_MA) {
			REGIST_EVENT(PHASE_EXEC, 1000000);
			break;
		}
		REGIST_EVENT(PHASE_TIMER, 5000);
		break;
	case PHASE_TIMER:
		result = read_id();
		shift_to_result7();
		break;
	}
}

void UPD765A::cmd_write_id()
{
	switch(phase)
	{
	case PHASE_IDLE:
		shift_to_cmd(5);
		break;
	case PHASE_CMD:
		set_hdu(buffer[0]);
		id[3] = buffer[1];
		eot = buffer[2];
		if(!eot) {
			REGIST_EVENT(PHASE_TIMER, 1000000);
			break;
		}
		shift_to_write(4 * eot);
		break;
	case PHASE_TC:
	case PHASE_WRITE:
		set_acctc(false);
		REGIST_EVENT(PHASE_TIMER, 4000000);
		break;
	case PHASE_TIMER:
		result =  write_id();
		shift_to_result7();
		break;
	}
}

uint32 UPD765A::read_id()
{
	int drv = hdu & DRIVE_MASK;
	int trk = fdc[drv].track;
	int side = (hdu >> 2) & 1;
	
	// get sector counts in the current track
	if(!disk[drv]->get_track(trk, side))
		return ST0_AT | ST1_MA;
	int secnum = disk[drv]->sector_num;
	if(!secnum)
		return ST0_AT | ST1_MA;
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

void UPD765A::shift_to_idle()
{
	phase = PHASE_IDLE;
	status = S_RQM;
	set_acctc(false);
}

void UPD765A::shift_to_cmd(int length)
{
	phase = PHASE_CMD;
	status = S_RQM | S_CB;
	set_acctc(false);
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
	set_acctc(true);
	bufptr = buffer;
	count = length;
//	req_intr_ndma(true);
	set_drq(true);
}

void UPD765A::shift_to_write(int length)
{
	phase = PHASE_WRITE;
	status = S_RQM | S_NDM | S_CB;
	set_acctc(true);
	bufptr = buffer;
	count = length;
//	req_intr_ndma(true);
	set_drq(true);
}

void UPD765A::shift_to_scan(int length)
{
	phase = PHASE_SCAN;
	status = S_RQM | S_NDM | S_CB;
	set_acctc(true);
	result = ST2_SH;
	bufptr = buffer;
	count = length;
//	req_intr_ndma(true);
	set_drq(true);
}

void UPD765A::shift_to_result(int length)
{
	phase = PHASE_RESULT;
	status = S_RQM | S_CB | S_DIO;
	set_acctc(false);
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
	req_intr(true);
	shift_to_result(7);
}


// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void UPD765A::open_disk(_TCHAR path[], int drv)
{
	if(drv < MAX_DRIVE)
		disk[drv]->open(path);
}

void UPD765A::close_disk(int drv)
{
	if(drv < MAX_DRIVE)
		disk[drv]->close();
}

bool UPD765A::disk_inserted(int drv)
{
	if(drv < MAX_DRIVE)
		return disk[drv]->insert;
	return false;
}

uint8 UPD765A::fdc_status()
{
	// for each virtual machines
#ifdef _QC10
	int drv = hdu & DRIVE_MASK;
	return (disk[drv]->insert ? 8 : 0) | (motor ? 0 : 2) | (intr ? 1 : 0);
#elif defined(_MZ3500)
	index = (disk[sel]->insert) ? !index : true;
	return (motor ? 4 : 0) | (index ? 2 : 0) | (drq ? 1 : 0);
#else
	return 0;
#endif
}
