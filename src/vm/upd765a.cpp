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
#define EVENT_RESULT7	4
#define EVENT_INDEX	5

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

#define DRIVE_MASK	3

#define REGIST_EVENT(phs, usec) { \
	if(phase_id != -1) { \
		vm->cancel_event(phase_id); \
	} \
	event_phase = phs; \
	vm->regist_event(this, EVENT_PHASE, 100, false, &phase_id); \
}

#define CANCEL_EVENT() { \
	if(phase_id != -1) { \
		vm->cancel_event(phase_id); \
	} \
	if(seek_id != -1) { \
		vm->cancel_event(seek_id); \
	} \
	if(drq_id != -1) { \
		vm->cancel_event(drq_id); \
	} \
	if(lost_id != -1) { \
		vm->cancel_event(lost_id); \
	} \
	if(result7_id != -1) { \
		vm->cancel_event(result7_id); \
	} \
	phase_id = seek_id = drq_id = lost_id = result7_id = -1; \
}

#define CANCEL_DRQ() { \
	if(drq_id != -1) { \
		vm->cancel_event(drq_id); \
	} \
	drq_id = -1; \
}

#define CANCEL_LOST() { \
	if(lost_id != -1) { \
		vm->cancel_event(lost_id); \
	} \
	lost_id = -1; \
}

void UPD765A::initialize()
{
	// initialize d88 handler
	for(int i = 0; i < 4; i++) {
		disk[i] = new DISK();
	}
	
	// initialize fdc
	for(int i = 0; i < 4; i++) {
		fdc[i].track = 0;
		fdc[i].result = 0;
		fdc[i].access = false;
	}
	_memset(buffer, 0, sizeof(buffer));
	
	phase = prevphase = PHASE_IDLE;
	status = S_RQM;
	seekstat = 0;
	bufptr = buffer; // temporary
	phase_id = seek_id = drq_id = lost_id = result7_id = -1;
	motor = false;	// motor off
	force_ready = false;
	reset_signal = true;
	
	set_irq(false);
	set_drq(false);
#ifdef UPD765A_EXT_DRVSEL
	hdu = 0;
#else
	set_hdu(0);
#endif
	set_acctc(false);
	
	// index hole event
	if(outputs_index.count) {
		index_count = 0;
		int id;
		vm->regist_event(this, EVENT_INDEX, 1000000 / 360 / 16, true, &id);
	}
}

void UPD765A::release()
{
	for(int i = 0; i < 4; i++) {
		if(disk[i]) {
			delete disk[i];
		}
	}
}

void UPD765A::reset()
{
	shift_to_idle();
	CANCEL_EVENT();
	
	set_irq(false);
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
//		req_irq_ndma(false);
		
		switch(phase) {
		case PHASE_IDLE:
//			emu->out_debug("FDC: CMD=%2x\n", data);
			switch(data & 0x1f) {
			case 0x02:
				emu->out_debug("FDC: CMD=%2x READ DIAGNOSTIC\n", data);
				break;
			case 0x03:
				emu->out_debug("FDC: CMD=%2x SPECIFY\n", data);
				break;
			case 0x04:
				emu->out_debug("FDC: CMD=%2x SENCE DEVSTAT\n", data);
				break;
			case 0x05:
			case 0x09:
				emu->out_debug("FDC: CMD=%2x WRITE DATA\n", data);
				break;
			case 0x06:
			case 0x0c:
				emu->out_debug("FDC: CMD=%2x READ DATA\n", data);
				break;
			case 0x07:
				emu->out_debug("FDC: CMD=%2x RECALIB\n", data);
				break;
			case 0x08:
				emu->out_debug("FDC: CMD=%2x SENCE INTSTAT\n", data);
				break;
			case 0x0a:
				emu->out_debug("FDC: CMD=%2x READ ID\n", data);
				break;
			case 0x0d:
				emu->out_debug("FDC: CMD=%2x WRITE ID\n", data);
				break;
			case 0x0f:
				emu->out_debug("FDC: CMD=%2x SEEK\n", data);
				break;
			case 0x11:
			case 0x19:
			case 0x1d:
				emu->out_debug("FDC: CMD=%2x SCAN\n", data);
				break;
			default:
				emu->out_debug("FDC: CMD=%2x INVALID\n", data);
				break;
			}
			command = data;
			process_cmd(command & 0x1f);
			break;
			
		case PHASE_CMD:
			emu->out_debug("FDC: PARAM=%2x\n", data);
			*bufptr++ = data;
			if(--count) {
				status |= S_RQM;
			}
			else {
				process_cmd(command & 0x1f);
			}
			break;
			
		case PHASE_WRITE:
			emu->out_debug("FDC: WRITE=%2x\n", data);
			*bufptr++ = data;
			if(--count) {
//				req_irq_ndma(true);
#ifdef UPD765A_DRQ_DELAY
				set_drq(false);
				CANCEL_DRQ();
				vm->regist_event(this, EVENT_DRQ, 50, false, &drq_id);
#else
				status |= S_RQM;
				// update data lost event
				CANCEL_LOST();
				vm->regist_event(this, EVENT_LOST, 30000, false, &lost_id);
#endif
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
				   ((command & 0x1f) == 0x1d && *bufptr <  data)) {
					result &= ~ST2_SH;
				}
			}
			bufptr++;
			if(--count) {
//				req_irq_ndma(true);
#ifdef UPD765A_DRQ_DELAY
				set_drq(false);
				CANCEL_DRQ();
				vm->regist_event(this, EVENT_DRQ, 50, false, &drq_id);
#else
				status |= S_RQM;
				// update data lost event
				CANCEL_LOST();
				vm->regist_event(this, EVENT_LOST, 30000, false, &lost_id);
#endif
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
//			req_irq_ndma(false);
			
			switch(phase) {
			case PHASE_RESULT:
				data = *bufptr++;
				emu->out_debug("FDC: RESULT=%2x\n", data);
				if(--count) {
					status |= S_RQM;
				}
				else {
					CANCEL_LOST();
					shift_to_idle();
				}
				return data;
				
			case PHASE_READ:
				data = *bufptr++;
				emu->out_debug("FDC: READ=%2x\n", data);
				if(--count) {
//					req_irq_ndma(true);
#ifdef UPD765A_DRQ_DELAY
					set_drq(false);
					CANCEL_DRQ();
					vm->regist_event(this, EVENT_DRQ, 50, false, &drq_id);
#else
					status |= S_RQM;
					// update data lost event
					CANCEL_LOST();
					vm->regist_event(this, EVENT_LOST, 30000, false, &lost_id);
#endif
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
	else {
		// fdc status
		return seekstat | status;
	}
}

void UPD765A::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_UPD765A_RESET) {
		bool next = ((data & mask) != 0);
		if(!reset_signal && next) {
			reset();
		}
		reset_signal = next;
	}
	else if(id == SIG_UPD765A_TC) {
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
	else if(id == SIG_UPD765A_MOTOR) {
		motor = ((data & mask) != 0);
	}
#ifdef UPD765A_EXT_DRVSEL
	else if(id == SIG_UPD765A_DRVSEL) {
		hdu = data & DRIVE_MASK;
		write_signals(&outputs_hdu, hdu);
	}
#endif
	else if(id == SIG_UPD765A_FREADY) {
		// for NEC PC-98x1 series
		force_ready = ((data & mask) != 0);
	}
}

uint32 UPD765A::read_signal(int ch)
{
	// get access status
	uint32 stat = 0;
	for(int i = 0; i < 4; i++) {
		if(fdc[i].access) {
			stat |= 1 << i;
		}
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
emu->out_debug("FDC: DATA LOST\n");
		result = ST1_OR;
		shift_to_result7();
		set_drq(false);
		lost_id = -1;
	}
	else if(event_id == EVENT_RESULT7) {
		shift_to_result7_event();
		result7_id = -1;
	}
	else if(event_id == EVENT_INDEX) {
		int drv = hdu & DRIVE_MASK;
		if(disk[drv]->insert) {
			write_signals(&outputs_index, (index_count == 0) ? 0 : 0xffffffff);
			index_count = (index_count + 1) & 15;
		}
		else {
			write_signals(&outputs_index, 0xffffffff);
		}
	}
}

void UPD765A::set_irq(bool val)
{
	req_irq(val);
	irq = val;
}

void UPD765A::req_irq(bool val)
{
	write_signals(&outputs_irq, val ? 0xffffffff : 0);
}

void UPD765A::req_irq_ndma(bool val)
{
#ifndef UPD765A_DMA_MODE
	write_signals(&outputs_irq, val ? 0xffffffff : 0);
#endif
}

void UPD765A::set_drq(bool val)
{
emu->out_debug("FDC: DRQ=%d\n",val?1:0);
#ifdef UPD765A_DMA_MODE
	if(val) {
		drq = dma_data_lost = true;
		write_signals(&outputs_drq, 0xffffffff);
		if(!dma_data_lost) {
			return;
		}
		// data lost if dma request is not accepted
		result = ST1_OR;
		shift_to_result7();
	}
	drq = false;
	write_signals(&outputs_drq, 0);
#else
	CANCEL_LOST();
	if(val) {
		vm->regist_event(this, EVENT_LOST, 30000, false, &lost_id);
	}
	drq = val;
	write_signals(&outputs_drq, val ? 0xffffffff : 0);
#endif
}

void UPD765A::set_hdu(uint8 val)
{
#ifndef UPD765A_EXT_DRVSEL
	hdu = val;
	write_signals(&outputs_hdu, hdu);
#endif
}

void UPD765A::set_acctc(bool val)
{
	acctc = val;
	write_signals(&outputs_acctc, acctc ? 0xffffffff : 0);
}

// ----------------------------------------------------------------------------
// command
// ----------------------------------------------------------------------------

void UPD765A::process_cmd(int cmd)
{
	switch(cmd & 0x1f) {
	case 0x02:
		cmd_read_diagnostic();
		break;
	case 0x03:
		cmd_specify();
		break;
	case 0x04:
		cmd_sence_devstat();
		break;
	case 0x05:
	case 0x09:
		cmd_write_data();
		break;
	case 0x06:
	case 0x0c:
		cmd_read_data();
		break;
	case 0x07:
		cmd_recalib();
		break;
	case 0x08:
		cmd_sence_intstat();
		break;
	case 0x0a:
		cmd_read_id();
		break;
	case 0x0d:
		cmd_write_id();
		break;
	case 0x0f:
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
	switch(phase) {
	case PHASE_IDLE:
		shift_to_cmd(1);
		break;
	case PHASE_CMD:
		set_hdu(buffer[0]);
		buffer[0] = get_devstat(buffer[0] & DRIVE_MASK);
		shift_to_result(1);
		break;
	}
}

void UPD765A::cmd_sence_intstat()
{
	if(irq) {
		irq = false;
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
		for(; i < 4; i++) {
			if(fdc[i].result) {
				irq = true;
			}
		}
		if(!irq) {
			set_irq(irq);
		}
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
	if(drv >= MAX_DRIVE) {
		return 0x80 | drv;
	}
	if(!disk[drv]->insert) {
		return drv;
	}
	return 0x28 | drv | (fdc[drv].track ? 0 : 0x10) | ((fdc[drv].track & 1) ? 0x04 : 0) | (disk[drv]->protect ? 0x40 : 0);
}

void UPD765A::cmd_seek()
{
	switch(phase) {
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
	switch(phase) {
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
	
	if(drv >= MAX_DRIVE) {
		// invalid drive number
		fdc[drv].result = (drv & DRIVE_MASK) | ST0_SE | ST0_NR | ST0_AT;
		set_irq(true);
	}
	else {
		fdc[drv].track = trk;
		event_drv = drv;
#ifdef UPD765A_WAIT_SEEK
		if(seek_id != -1) {
			vm->cancel_event(seek_id);
		}
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
	
	if(drv >= MAX_DRIVE) {
		fdc[drv].result = (drv & DRIVE_MASK) | ST0_SE | ST0_NR | ST0_AT;
	}
	else if(force_ready || disk[drv]->get_track(trk, 0) || disk[drv]->get_track(trk, 1)) {
		fdc[drv].result = (drv & DRIVE_MASK) | ST0_SE;
	}
	else {
#ifdef UPD765A_NO_ST0_AT_FOR_SEEK
		// for NEC PC-100
		fdc[drv].result = (drv & DRIVE_MASK) | ST0_SE | ST0_NR;
#else
		fdc[drv].result = (drv & DRIVE_MASK) | ST0_SE | ST0_NR | ST0_AT;
#endif
	}
	set_irq(true);
	seekstat &= ~(1 << drv);
}

void UPD765A::cmd_read_data()
{
	switch(phase) {
	case PHASE_IDLE:
		shift_to_cmd(8);
		break;
	case PHASE_CMD:
		get_sector_params();
		REGIST_EVENT(PHASE_EXEC, 25000 << __min(7, id[3]));
		break;
	case PHASE_EXEC:
		read_data((command & 0x1f) == 12, false);
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
	switch(phase) {
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
		if(!result) {
			result = find_id();
		}
		if(result) {
			shift_to_result7();
		}
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
		write_data((command & 0x1f) == 9);
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
			write_data((command & 0x1f) == 9);
		}
		shift_to_result7();
		break;
	}
}

void UPD765A::cmd_scan()
{
	switch(phase) {
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
	switch(phase) {
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
	if(deleted) {
		result ^= ST2_CM;
	}
	if((result & ~ST2_CM) && !(result & ST2_DD)) {
		shift_to_result7();
		return;
	}
	if((result & ST2_CM) && (command & 0x20)) {
		REGIST_EVENT(PHASE_TIMER, 100000);
		return;
	}
	int length = id[3] ? (0x80 << __min(8, id[3])) : (__min(dtl, 0x80));
	if(!scan) {
		shift_to_read(length);
	}
	else {
		shift_to_scan(length);
	}
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
	if(!disk[drv]->get_track(trk, side)) {
		emu->out_debug("FDC: TRACK NOT FOUND (TRK=%d SIDE=%d)\n", trk, side);
		return ST0_AT | ST1_MA;
	}
	int secnum = disk[drv]->sector_num;
	if(!secnum) {
		emu->out_debug("FDC: NO SECTORS IN TRACK (TRK=%d SIDE=%d)\n", trk, side);
		return ST0_AT | ST1_MA;
	}
	int cy = -1;
	for(int i = 0; i < secnum; i++) {
		if(!disk[drv]->get_sector(trk, side, i)) {
			continue;
		}
		cy = disk[drv]->id[0];
#ifdef UPD765A_STRICT_ID
		if(disk[drv]->id[0] != id[0] || disk[drv]->id[1] != id[1] || disk[drv]->id[3] != id[3]) {
			continue;
		}
#endif
		if(disk[drv]->id[2] != id[2]) {
			continue;
		}
		// sector number is matched
		_memcpy(buffer, disk[drv]->sector, __min(0x2000, disk[drv]->sector_size));
		if(disk[drv]->status) {
			return ST0_AT | ST1_DE | ST2_DD;
		}
		if(disk[drv]->deleted) {
			return ST2_CM;
		}
		return 0;
	}
	emu->out_debug("FDC: SECTOR NOT FOUND (TRK=%d SIDE=%d ID=%2x,%2x,%2x,%2x)\n", trk, side, id[0], id[1], id[2], id[3]);
	if(cy != id[0] && cy != -1) {
		if(cy == 0xff) {
			return ST0_AT | ST1_ND | ST2_BC;
		}
		else {
			return ST0_AT | ST1_ND | ST2_NC;
		}
	}
	return ST0_AT | ST1_ND;
}

uint32 UPD765A::write_sector(bool deleted)
{
	int drv = hdu & DRIVE_MASK;
	int trk = fdc[drv].track;
	int side = (hdu >> 2) & 1;
	
	if(!disk[drv]->insert) {
		return ST0_AT | ST1_MA;
	}
	if(disk[drv]->protect) {
		return ST0_AT | ST1_NW;
	}
	// get sector counts in the current track
	if(!disk[drv]->get_track(trk, side)) {
		return ST0_AT | ST1_MA;
	}
	int secnum = disk[drv]->sector_num;
	if(!secnum) {
		return ST0_AT | ST1_MA;
	}
	int cy = -1;
	for(int i = 0; i < secnum; i++) {
		if(!disk[drv]->get_sector(trk, side, i)) {
			continue;
		}
		cy = disk[drv]->id[0];
#ifdef UPD765A_STRICT_ID
		if(disk[drv]->id[0] != id[0] || disk[drv]->id[1] != id[1] || disk[drv]->id[3] != id[3]) {
			continue;
		}
#endif
		if(disk[drv]->id[2] != id[2]) {
			continue;
		}
		// sector number is matched
		int size = 0x80 << __min(8, id[3]);
		_memcpy(disk[drv]->sector, buffer, __min(size, disk[drv]->sector_size));
//		if(deleted) {
//			disk[drv]->deleted = ??;//
//		}
		return 0;
	}
	if(cy != id[0] && cy != -1) {
		if(cy == 0xff) {
			return ST0_AT | ST1_ND | ST2_BC;
		}
		else {
			return ST0_AT | ST1_ND | ST2_NC;
		}
	}
	return ST0_AT | ST1_ND;
}

uint32 UPD765A::find_id()
{
	int drv = hdu & DRIVE_MASK;
	int trk = fdc[drv].track;
	int side = (hdu >> 2) & 1;
	
	// get sector counts in the current track
	if(!disk[drv]->get_track(trk, side)) {
		return ST0_AT | ST1_MA;
	}
	int secnum = disk[drv]->sector_num;
	if(!secnum) {
		return ST0_AT | ST1_MA;
	}
	int cy = -1;
	for(int i = 0; i < secnum; i++) {
		if(!disk[drv]->get_sector(trk, side, i)) {
			continue;
		}
		cy = disk[drv]->id[0];
#ifdef UPD765A_STRICT_ID
		if(disk[drv]->id[0] != id[0] || disk[drv]->id[1] != id[1] || disk[drv]->id[3] != id[3]) {
			continue;
		}
#endif
		if(disk[drv]->id[2] != id[2]) {
			continue;
		}
		// sector number is matched
		return 0;
	}
	if(cy != id[0] && cy != -1) {
		if(cy == 0xff) {
			return ST0_AT | ST1_ND | ST2_BC;
		}
		else {
			return ST0_AT | ST1_ND | ST2_NC;
		}
	}
	return ST0_AT | ST1_ND;
}

uint32 UPD765A::check_cond(bool write)
{
	int drv = hdu & DRIVE_MASK;
	hdue = hdu;
	if(drv >= MAX_DRIVE) {
		return ST0_AT | ST0_NR;
	}
	if(!disk[drv]->insert) {
		return ST0_AT | ST1_MA;
	}
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
		if((dtl & 0xff) == 0x02) {
			id[2]++;
		}
	}
	if(id[2]++ != eot) {
		return true;
	}
	id[2] = 1;
	if(command & 0x80) {
		set_hdu(hdu ^ 4);
		id[1] ^= 1;
		if (id[1] & 1) {
			return true;
		}
	}
	id[0]++;
	return false;
}

void UPD765A::cmd_read_id()
{
	switch(phase) {
	case PHASE_IDLE:
		shift_to_cmd(1);
		break;
	case PHASE_CMD:
		set_hdu(buffer[0]);
//		break;
	case PHASE_EXEC:
		if(check_cond(false) & ST1_MA) {
//			REGIST_EVENT(PHASE_EXEC, 1000000);
//			break;
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
	switch(phase) {
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
	if(!disk[drv]->get_track(trk, side)) {
		return ST0_AT | ST1_MA;
	}
	int secnum = disk[drv]->sector_num;
	if(!secnum) {
		return ST0_AT | ST1_MA;
	}
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
	if(!(result = check_cond(true))) {
		result = ST0_AT | ST1_NW;
	}
	return result;
}

void UPD765A::cmd_specify()
{
	switch(phase) {
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
//	req_irq_ndma(true);
	set_drq(true);
}

void UPD765A::shift_to_write(int length)
{
	phase = PHASE_WRITE;
	status = S_RQM | S_NDM | S_CB;
	set_acctc(true);
	bufptr = buffer;
	count = length;
//	req_irq_ndma(true);
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
//	req_irq_ndma(true);
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
#ifdef UPD765A_WAIT_RESULT7
	if(result7_id != -1) {
		vm->cancel_event(result7_id);
	}
	vm->regist_event(this, EVENT_RESULT7, 100, false, &result7_id);
#else
	shift_to_result7_event();
#endif
}

void UPD765A::shift_to_result7_event()
{
#ifdef UPD765A_NO_ST1_EN_OR_FOR_RESULT7
	// for NEC PC-9801 (XANADU)
	result &= ~(ST1_EN | ST1_OR);
#endif
	buffer[0] = (result & 0xf8) | (hdue & 7);
	buffer[1] = uint8(result >>  8);
	buffer[2] = uint8(result >> 16);
	buffer[3] = id[0];
	buffer[4] = id[1];
	buffer[5] = id[2];
	buffer[6] = id[3];
	
#ifdef UPD765A_NO_IRQ_FOR_RESULT7
	// for EPSON QC-10
	req_irq(true);
#else
	// for sence interrupt status
	int drv = hdu & DRIVE_MASK;
	fdc[drv].result = buffer[0];
	
	set_irq(true);
#endif
	shift_to_result(7);
}


// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void UPD765A::open_disk(_TCHAR path[], int drv)
{
	if(drv < MAX_DRIVE) {
		disk[drv]->open(path);
#ifdef UPD765A_MEDIA_CHANGE
		// media is changed
		if(disk[drv]->change) {
			fdc[drv].result = (drv & DRIVE_MASK) | ST0_AI;
			set_irq(true);
		}
#endif
	}
}

void UPD765A::close_disk(int drv)
{
	if(drv < MAX_DRIVE && disk[drv]->insert) {
		disk[drv]->close();
#ifdef UPD765A_MEDIA_CHANGE
		// media is ejected
		fdc[drv].result = (drv & DRIVE_MASK) | ST0_AI;
		set_irq(true);
#endif
	}
}

bool UPD765A::disk_inserted(int drv)
{
	if(drv < MAX_DRIVE) {
		return disk[drv]->insert;
	}
	return false;
}

bool UPD765A::disk_inserted()
{
	int drv = hdu & DRIVE_MASK;
	return disk_inserted(drv);
}

uint8 UPD765A::media_type(int drv)
{
	if(drv < MAX_DRIVE && disk[drv]->insert) {
		return disk[drv]->media_type;
	}
	return 0;
}

