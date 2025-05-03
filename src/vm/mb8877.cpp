/*
	Skelton for retropc emulator

	Origin : XM7
	Author : Takeda.Toshiya
	Date   : 2006.12.06 -

	[ MB8877 / MB8876 ]
*/

#include "mb8877.h"
#include "disk.h"
#include "../config.h"

#define FDC_ST_BUSY		0x01	// busy
#define FDC_ST_INDEX		0x02	// index hole
#define FDC_ST_DRQ		0x02	// data request
#define FDC_ST_TRACK00		0x04	// track0
#define FDC_ST_LOSTDATA		0x04	// data lost
#define FDC_ST_CRCERR		0x08	// crc error
#define FDC_ST_SEEKERR		0x10	// seek error
#define FDC_ST_RECNFND		0x10	// sector not found
#define FDC_ST_HEADENG		0x20	// head engage
#define FDC_ST_RECTYPE		0x20	// record type
#define FDC_ST_WRITEFAULT	0x20	// write fault
#define FDC_ST_WRITEP		0x40	// write protect
#define FDC_ST_NOTREADY		0x80	// media not inserted

#define FDC_CMD_TYPE1		1
#define FDC_CMD_RD_SEC		2
#define FDC_CMD_RD_MSEC		3
#define FDC_CMD_WR_SEC		4
#define FDC_CMD_WR_MSEC		5
#define FDC_CMD_RD_ADDR		6
#define FDC_CMD_RD_TRK		7
#define FDC_CMD_WR_TRK		8
#define FDC_CMD_TYPE4		0x80

#define EVENT_SEEK		0
#define EVENT_SEEKEND		1
#define EVENT_SEARCH		2
#define EVENT_TYPE4		3
#define EVENT_MULTI1		4
#define EVENT_MULTI2		5
#define EVENT_LOST		6

#define CANCEL_EVENT(event) { \
	if(regist_id[event] != -1) { \
		vm->cancel_event(regist_id[event]); \
		regist_id[event] = -1; \
	} \
	if(event == EVENT_SEEK) \
		now_seek = false; \
	if(event == EVENT_SEARCH) \
		now_search = false; \
}
#define REGIST_EVENT(event, wait) { \
	if(regist_id[event] != -1) { \
		vm->cancel_event(regist_id[event]); \
		regist_id[event] = -1; \
	} \
	vm->regist_event(this, (event << 8) | cmdtype, wait, false, &regist_id[event]); \
	if(event == EVENT_SEEK) \
		now_seek = true; \
	if(event == EVENT_SEARCH) \
		now_search = true; \
}

void MB8877::initialize()
{
	// config
	ignore_crc = config.ignore_crc;
	
	// initialize d88 handler
	for(int i = 0; i < MAX_DRIVE; i++)
		disk[i] = new DISK();
	
	// initialize fdc
	seektrk = 0;
	seekvct = true;
	indexcnt = sectorcnt = 0;
	status = cmdreg = trkreg = secreg = datareg = sidereg = cmdtype = 0;
	drvreg = 0;
	motor = false;	// motor off
	
	for(int i = 0; i < MAX_DRIVE; i++) {
		fdc[i].track = 0;
		fdc[i].index = 0;
		fdc[i].access = false;
	}
}

void MB8877::release()
{
	// release d88 handler
	for(int i = 0; i < MAX_DRIVE; i++)
		delete disk[i];
}

void MB8877::reset()
{
	for(int i = 0; i < 7; i++)
		regist_id[i] = -1;
	now_seek = now_search = false;
}

void MB8877::update_config()
{
	ignore_crc = config.ignore_crc;
}

void MB8877::write_dma8(uint32 addr, uint32 data)
{
	write_io8(3, data);
}

uint32 MB8877::read_dma8(uint32 addr)
{
	return read_io8(3);
}

void MB8877::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 3)
	{
	case 0:
		// command reg
#ifdef MB8876
		cmdreg = (~data) & 0xff;
#else
		cmdreg = data;
#endif
		process_cmd();
		break;
	case 1:
		// track reg
#ifdef MB8876
		trkreg = (~data) & 0xff;
#else
		trkreg = data;
#endif
		if((status & FDC_ST_BUSY) && (fdc[drvreg].index == 0)) {
			// track reg is written after command starts
			if(cmdtype == FDC_CMD_RD_SEC || cmdtype == FDC_CMD_RD_MSEC || cmdtype == FDC_CMD_WR_SEC || cmdtype == FDC_CMD_WR_MSEC)
				process_cmd();
		}
		break;
	case 2:
		// sector reg
#ifdef MB8876
		secreg = (~data) & 0xff;
#else
		secreg = data;
#endif
		if((status & FDC_ST_BUSY) && (fdc[drvreg].index == 0)) {
			// sector reg is written after command starts
			if(cmdtype == FDC_CMD_RD_SEC || cmdtype == FDC_CMD_RD_MSEC || cmdtype == FDC_CMD_WR_SEC || cmdtype == FDC_CMD_WR_MSEC)
				process_cmd();
		}
		break;
	case 3:
		// data reg
#ifdef MB8876
		datareg = (~data) & 0xff;
#else
		datareg = data;
#endif
		if(motor && (status & FDC_ST_DRQ) && !now_search) {
			if(cmdtype == FDC_CMD_WR_SEC || cmdtype == FDC_CMD_WR_MSEC) {
				// write or multisector write
				if(fdc[drvreg].index < disk[drvreg]->sector_size) {
					if(!disk[drvreg]->protect) {
						disk[drvreg]->sector[fdc[drvreg].index] = datareg;
						// dm, ddm
						disk[drvreg]->deleted = (cmdreg & 1) ? 0x10 : 0;
					}
					else {
						status |= FDC_ST_WRITEFAULT;
						status &= ~FDC_ST_BUSY;
						status &= ~FDC_ST_DRQ;
						cmdtype = 0;
						set_irq(true);
					}
					fdc[drvreg].index++;
				}
				if(fdc[drvreg].index >= disk[drvreg]->sector_size) {
					if(cmdtype == FDC_CMD_WR_SEC) {
						// single sector
						status &= ~FDC_ST_BUSY;
						cmdtype = 0;
						set_irq(true);
					}
					else {
						// multisector
						REGIST_EVENT(EVENT_MULTI1, 30);
						REGIST_EVENT(EVENT_MULTI2, 60);
					}
					status &= ~FDC_ST_DRQ;
				}
				fdc[drvreg].access = true;
			}
			else if(cmdtype == FDC_CMD_WR_TRK) {
				// read track
				if(fdc[drvreg].index < disk[drvreg]->track_size) {
					if(!disk[drvreg]->protect)
						disk[drvreg]->track[fdc[drvreg].index] = datareg;
					else {
						status |= FDC_ST_WRITEFAULT;
						status &= ~FDC_ST_BUSY;
						status &= ~FDC_ST_DRQ;
						cmdtype = 0;
						set_irq(true);
					}
					fdc[drvreg].index++;
				}
				if(fdc[drvreg].index >= disk[drvreg]->track_size) {
					status &= ~FDC_ST_BUSY;
					status &= ~FDC_ST_DRQ;
					cmdtype = 0;
					set_irq(true);
				}
				fdc[drvreg].access = true;
			}
			if(!(status & FDC_ST_DRQ))
				set_drq(false);
		}
		break;
	}
}

uint32 MB8877::read_io8(uint32 addr)
{
	uint32 val;
	
	switch(addr & 3)
	{
	case 0:
		// status reg
		
		// now force interrupt
		if(cmdtype == FDC_CMD_TYPE4) {
			// MZ-2500 RELICS invites STATUS = 0
			if(!disk[drvreg]->insert || !motor)
				status = FDC_ST_NOTREADY;
			else
				status = 0;
#ifdef MB8876
			return (~status) & 0xff;
#else
			return status;
#endif
		}
		// now sector search
		if(now_search)
#ifdef MB8876
			return (~FDC_ST_BUSY) & 0xff;
#else
			return FDC_ST_BUSY;
#endif
		// disk not inserted, motor stop
		if(!disk[drvreg]->insert || !motor)
			status |= FDC_ST_NOTREADY;
		else
			status &= ~FDC_ST_NOTREADY;
		// write protect
		if(cmdtype == FDC_CMD_TYPE1 || cmdtype == FDC_CMD_WR_SEC || cmdtype == FDC_CMD_WR_MSEC || cmdtype == FDC_CMD_WR_TRK) {
			if(disk[drvreg]->insert && disk[drvreg]->protect)
				status |= FDC_ST_WRITEP;
			else
				status &= ~FDC_ST_WRITEP;
		}
		else
			status &= ~FDC_ST_WRITEP;
		
		// track0, index hole
		if(cmdtype == FDC_CMD_TYPE1) {
			if(fdc[drvreg].track == 0)
				status |= FDC_ST_TRACK00;
			else
				status &= ~FDC_ST_TRACK00;
			if(!(status & FDC_ST_NOTREADY)) {
				if(indexcnt == 0)
					status |= FDC_ST_INDEX;
				else
					status &= ~FDC_ST_INDEX;
				if(++indexcnt >= ((disk[drvreg]->sector_num == 0) ? 16 : disk[drvreg]->sector_num))
					indexcnt = 0;
			}
		}
		
		// show busy a moment
		val = status;
		if(cmdtype == FDC_CMD_TYPE1 && !now_seek)
			status &= ~FDC_ST_BUSY;
#ifdef MB8876
		return (~val) & 0xff;
#else
		return val;
#endif
	case 1:
		// track reg
#ifdef MB8876
		return (~trkreg) & 0xff;
#else
		return trkreg;
#endif
	case 2:
		// sector reg
#ifdef MB8876
		return (~secreg) & 0xff;
#else
		return secreg;
#endif
	case 3:
		// data reg
		if(motor && (status & FDC_ST_DRQ) && !now_search) {
			if(cmdtype == FDC_CMD_RD_SEC || cmdtype == FDC_CMD_RD_MSEC) {
				// read or multisector read
				if(fdc[drvreg].index < disk[drvreg]->sector_size) {
					datareg = disk[drvreg]->sector[fdc[drvreg].index];
					fdc[drvreg].index++;
				}
				if(fdc[drvreg].index >= disk[drvreg]->sector_size) {
					if(cmdtype == FDC_CMD_RD_SEC) {
						// single sector
						status &= ~FDC_ST_BUSY;
						cmdtype = 0;
						set_irq(true);
					}
					else {
						// multisector
						REGIST_EVENT(EVENT_MULTI1, 30);
						REGIST_EVENT(EVENT_MULTI2, 60);
					}
					status &= ~FDC_ST_DRQ;
				}
				fdc[drvreg].access = true;
			}
			else if(cmdtype == FDC_CMD_RD_ADDR) {
				// read address
				if(fdc[drvreg].index < 6) {
					datareg = disk[drvreg]->id[fdc[drvreg].index];
					fdc[drvreg].index++;
				}
				if(fdc[drvreg].index >= 6) {
					status &= ~FDC_ST_BUSY;
					status &= ~FDC_ST_DRQ;
					cmdtype = 0;
					set_irq(true);
				}
				fdc[drvreg].access = true;
			}
			else if(cmdtype == FDC_CMD_RD_TRK) {
				// read track
				if(fdc[drvreg].index < disk[drvreg]->track_size) {
					datareg = disk[drvreg]->track[fdc[drvreg].index];
					fdc[drvreg].index++;
				}
				if(fdc[drvreg].index >= disk[drvreg]->track_size) {
					status &= ~FDC_ST_BUSY;
					status &= ~FDC_ST_DRQ;
					status |= FDC_ST_LOSTDATA;
					cmdtype = 0;
					set_irq(true);
				}
				fdc[drvreg].access = true;
			}
			if(!(status & FDC_ST_DRQ))
				set_drq(false);
		}
#ifdef MB8876
		return (~datareg) & 0xff;
#else
		return datareg;
#endif
	}
	return 0xff;
}

void MB8877::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_MB8877_DRIVEREG)
		drvreg = data & DRIVE_MASK;
	else if(id == SIG_MB8877_SIDEREG)
		sidereg = (data & mask) ? 1 : 0;
	else if(id == SIG_MB8877_MOTOR)
		motor = ((data & mask) != 0);
}

uint32 MB8877::read_signal(int ch)
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

void MB8877::event_callback(int event_id, int err)
{
	int event = event_id >> 8;
	int cmd = event_id & 0xff;
	regist_id[event] = -1;
	
	// cancel event if the command is finished or other command is executed
	if(cmd != cmdtype) {
		if(event == EVENT_SEEK)
			now_seek = false;
		else if(event == EVENT_SEARCH)
			now_search = false;
		return;
	}
	
	switch(event)
	{
	case EVENT_SEEK:
		if(seektrk > fdc[drvreg].track)
			fdc[drvreg].track++;
		else if(seektrk < fdc[drvreg].track)
			fdc[drvreg].track--;
		if(cmdreg & 0x10)
			trkreg = fdc[drvreg].track;
		else if((cmdreg & 0xf0) == 0)
			trkreg--;
		if(seektrk == fdc[drvreg].track) {
			// auto update
			if((cmdreg & 0x10) || ((cmdreg & 0xf0) == 0))
				trkreg = fdc[drvreg].track;
			if((cmdreg & 0xf0) == 0)
				datareg = 0;
			status |= search_track();
			now_seek = false;
			set_irq(true);
		}
		else {
			REGIST_EVENT(EVENT_SEEK, seek_wait[cmdreg & 3]);
		}
		break;
	case EVENT_SEEKEND:
		if(seektrk == fdc[drvreg].track) {
			// auto update
			if((cmdreg & 0x10) || ((cmdreg & 0xf0) == 0))
				trkreg = fdc[drvreg].track;
			if((cmdreg & 0xf0) == 0)
				datareg = 0;
			status |= search_track();
			now_seek = false;
			CANCEL_EVENT(EVENT_SEEK);
			set_irq(true);
		}
		break;
	case EVENT_SEARCH:
		now_search = false;
		// start dma
		if(status & FDC_ST_DRQ)
			set_drq(true);
		break;
	case EVENT_TYPE4:
		cmdtype = FDC_CMD_TYPE4;
		break;
	case EVENT_MULTI1:
		secreg++;
		break;
	case EVENT_MULTI2:
		if(cmdtype == FDC_CMD_RD_MSEC)
			cmd_readdata();
		else if(cmdtype == FDC_CMD_WR_MSEC)
			cmd_writedata();
		break;
	case EVENT_LOST:
		if(status & FDC_ST_BUSY) {
			status |= FDC_ST_LOSTDATA;
			status &= ~FDC_ST_BUSY;
			//status &= ~FDC_ST_DRQ;
			set_irq(true);
			//set_drq(false);
		}
		break;
	}
}

// ----------------------------------------------------------------------------
// command
// ----------------------------------------------------------------------------

void MB8877::process_cmd()
{
	CANCEL_EVENT(EVENT_TYPE4);
	
	switch(cmdreg & 0xf0)
	{
	// type-1
	case 0x00:
		cmd_restore();
		break;
	case 0x10:
		cmd_seek();
		break;
	case 0x20:
	case 0x30:
		cmd_step();
		break;
	case 0x40:
	case 0x50:
		cmd_stepin();
		break;
	case 0x60:
	case 0x70:
		cmd_stepout();
		break;
	// type-2
	case 0x80:
	case 0x90:
		cmd_readdata();
		break;
	case 0xa0:
	case 0xb0:
		cmd_writedata();
		break;
	// type-3
	case 0xc0:
		cmd_readaddr();
		break;
	case 0xe0:
		cmd_readtrack();
		break;
	case 0xf0:
		cmd_writetrack();
		break;
	// type-4
	case 0xd0:
		cmd_forceint();
		break;
	default:
		break;
	}
}

void MB8877::cmd_restore()
{
	// type-1 restore
	cmdtype = FDC_CMD_TYPE1;
	status = FDC_ST_HEADENG | FDC_ST_BUSY;
	trkreg = 0xff;
	
	seektrk = 0;
	seekvct = true;
	
	REGIST_EVENT(EVENT_SEEK, seek_wait[cmdreg & 3]);
	REGIST_EVENT(EVENT_SEEKEND, 300);
}

void MB8877::cmd_seek()
{
	// type-1 seek
	cmdtype = FDC_CMD_TYPE1;
	status = FDC_ST_HEADENG | FDC_ST_BUSY;
	
#if 0
	seektrk = fdc[drvreg].track + datareg - trkreg;
#else
	seektrk = datareg;
#endif
	seektrk = (seektrk > 83) ? 83 : (seektrk < 0) ? 0 : seektrk;
	seekvct = (datareg > trkreg) ? false : true;
	
	REGIST_EVENT(EVENT_SEEK, seek_wait[cmdreg & 3]);
	REGIST_EVENT(EVENT_SEEKEND, 300);
}

void MB8877::cmd_step()
{
	// type-1 step
	if(seekvct)
		cmd_stepout();
	else
		cmd_stepin();
}

void MB8877::cmd_stepin()
{
	// type-1 step in
	cmdtype = FDC_CMD_TYPE1;
	status = FDC_ST_HEADENG | FDC_ST_BUSY;
	
	seektrk = (fdc[drvreg].track < 83) ? fdc[drvreg].track + 1 : 83;
	seekvct = false;
	
	REGIST_EVENT(EVENT_SEEK, seek_wait[cmdreg & 3]);
	REGIST_EVENT(EVENT_SEEKEND, 300);
}

void MB8877::cmd_stepout()
{
	// type-1 step out
	cmdtype = FDC_CMD_TYPE1;
	status = FDC_ST_HEADENG | FDC_ST_BUSY;
	
	seektrk = (fdc[drvreg].track > 0) ? fdc[drvreg].track - 1 : 0;
	seekvct = true;
	
	REGIST_EVENT(EVENT_SEEK, seek_wait[cmdreg & 3]);
	REGIST_EVENT(EVENT_SEEKEND, 300);
}

void MB8877::cmd_readdata()
{
	// type-2 read data
	cmdtype = (cmdreg & 0x10) ? FDC_CMD_RD_MSEC : FDC_CMD_RD_SEC;
	if(cmdreg & 2)
		status = search_sector(fdc[drvreg].track, ((cmdreg & 8) ? 1 : 0), secreg, true);
	else
		status = search_sector(fdc[drvreg].track, sidereg, secreg, false);
	if(!(status & FDC_ST_RECNFND))
		status |= FDC_ST_DRQ | FDC_ST_BUSY;
	REGIST_EVENT(EVENT_SEARCH, 200);
	CANCEL_EVENT(EVENT_LOST);
	if(!(status & FDC_ST_RECNFND)) {
		REGIST_EVENT(EVENT_LOST, 30000);
	}
}

void MB8877::cmd_writedata()
{
	// type-2 write data
	cmdtype = (cmdreg & 0x10) ? FDC_CMD_WR_MSEC : FDC_CMD_WR_SEC;
	if(cmdreg & 2)
		status = search_sector(fdc[drvreg].track, ((cmdreg & 8) ? 1 : 0), secreg, true);
	else
		status = search_sector(fdc[drvreg].track, sidereg, secreg, false);
	status &= ~FDC_ST_RECTYPE;
	if(!(status & FDC_ST_RECNFND))
		status |= FDC_ST_DRQ | FDC_ST_BUSY;
	
	REGIST_EVENT(EVENT_SEARCH, 200);
	CANCEL_EVENT(EVENT_LOST);
	if(!(status & FDC_ST_RECNFND)) {
		REGIST_EVENT(EVENT_LOST, 30000);
	}
}

void MB8877::cmd_readaddr()
{
	// type-3 read address
	cmdtype = FDC_CMD_RD_ADDR;
	status = search_addr();
	if(!(status & FDC_ST_RECNFND))
		status |= FDC_ST_DRQ | FDC_ST_BUSY;
	
	REGIST_EVENT(EVENT_SEARCH, 200);
	CANCEL_EVENT(EVENT_LOST);
	if(!(status & FDC_ST_RECNFND)) {
		REGIST_EVENT(EVENT_LOST, 10000);
	}
}

void MB8877::cmd_readtrack()
{
	// type-3 read track
	cmdtype = FDC_CMD_RD_TRK;
	status = FDC_ST_BUSY | FDC_ST_DRQ;
	
	make_track();
	
	REGIST_EVENT(EVENT_SEARCH, 200);
	REGIST_EVENT(EVENT_LOST, 150000);
}

void MB8877::cmd_writetrack()
{
	// type-3 write track
	cmdtype = FDC_CMD_WR_TRK;
	status = FDC_ST_BUSY | FDC_ST_DRQ;
	
	disk[drvreg]->track_size = 0x1800;
	fdc[drvreg].index = 0;
	
	REGIST_EVENT(EVENT_SEARCH, 200);
	REGIST_EVENT(EVENT_LOST, 150000);
}

void MB8877::cmd_forceint()
{
	// type-4 force interrupt
#if 0
	if(!disk[drvreg]->insert || !motor)
		status = FDC_ST_NOTREADY | FDC_ST_HEADENG;
	else
		status = FDC_ST_HEADENG;
	cmdtype = FDC_CMD_TYPE4;
#else
	if(cmdtype == 0 || cmdtype == 4) {
		status = 0;
		cmdtype = FDC_CMD_TYPE1;
	}
	status &= ~FDC_ST_BUSY;
#endif
	
	// force interrupt if bit0-bit3 is high
	if(cmdreg & 0xf)
		set_irq(true);
	else
		set_irq(false);
	
	CANCEL_EVENT(EVENT_SEEK);
	CANCEL_EVENT(EVENT_SEEKEND);
	CANCEL_EVENT(EVENT_SEARCH);
	CANCEL_EVENT(EVENT_TYPE4);
	CANCEL_EVENT(EVENT_MULTI1);
	CANCEL_EVENT(EVENT_MULTI2);
	CANCEL_EVENT(EVENT_LOST);
	REGIST_EVENT(EVENT_TYPE4, 100);
}

// ----------------------------------------------------------------------------
// media handler
// ----------------------------------------------------------------------------

uint8 MB8877::search_track()
{
	int trk = fdc[drvreg].track;
	
	if(!disk[drvreg]->get_track(trk, sidereg))
		return FDC_ST_SEEKERR;
	
	// verify track number
	if(!(cmdreg & 4))
		return 0;
	for(int i = 0; i < disk[drvreg]->sector_num; i++) {
		if(disk[drvreg]->verify[i] == trkreg)
			return 0;
	}
	return FDC_ST_SEEKERR;
}

uint8 MB8877::search_sector(int trk, int side, int sct, bool compare)
{
	// get track
	if(!disk[drvreg]->get_track(trk, side)) {
		set_irq(true);
		return FDC_ST_RECNFND;
	}
	
	// first scanned sector
	int sector_num = disk[drvreg]->sector_num;
	if(sectorcnt >= sector_num)
		sectorcnt = 0;
	
	// scan sectors
	for(int i = 0; i < sector_num; i++) {
		// get sector
		int index = sectorcnt + i;
		if(index >= sector_num)
			index -= sector_num;
		disk[drvreg]->get_sector(trk, side, index);
		
		// check id
		if(disk[drvreg]->id[2] != sct)
			continue;
		// check density
		if(disk[drvreg]->density)
			continue;
		
		// sector found
		sectorcnt = index + 1;
		if(sectorcnt >= sector_num)
			sectorcnt -= sector_num;
		
		fdc[drvreg].index = 0;
		return (disk[drvreg]->deleted ? FDC_ST_RECTYPE : 0) | ((disk[drvreg]->status && !ignore_crc) ? FDC_ST_CRCERR : 0);
	}
	
	// sector not found
	disk[drvreg]->sector_size = 0;
	set_irq(true);
	return FDC_ST_RECNFND;
}

uint8 MB8877::search_addr()
{
	int trk = fdc[drvreg].track;
	
	// get track
	if(!disk[drvreg]->get_track(trk, sidereg)) {
		set_irq(true);
		return FDC_ST_RECNFND;
	}
	
	// get sector
	if(sectorcnt >= disk[drvreg]->sector_num)
		sectorcnt = 0;
	if(disk[drvreg]->get_sector(trk, sidereg, sectorcnt)) {
		sectorcnt++;
		
		fdc[drvreg].index = 0;
		secreg = disk[drvreg]->id[0];
		return (disk[drvreg]->status && !ignore_crc) ? FDC_ST_CRCERR : 0;
	}
	
	// sector not found
	disk[drvreg]->sector_size = 0;
	set_irq(true);
	return FDC_ST_RECNFND;
}

bool MB8877::make_track()
{
	int trk = fdc[drvreg].track;
	
	return disk[drvreg]->make_track(trk, sidereg);
}

// ----------------------------------------------------------------------------
// irq / drq
// ----------------------------------------------------------------------------

void MB8877::set_irq(bool val)
{
	for(int i = 0; i < dcount_irq; i++)
		d_irq[i]->write_signal(did_irq[i], val ? 0xffffffff : 0, dmask_irq[i]);
}

void MB8877::set_drq(bool val)
{
	for(int i = 0; i < dcount_drq; i++)
		d_drq[i]->write_signal(did_drq[i], val ? 0xffffffff : 0, dmask_drq[i]);
}

// ----------------------------------------------------------------------------
// user interface
// ----------------------------------------------------------------------------

void MB8877::open_disk(_TCHAR path[], int drv)
{
	if(drv < MAX_DRIVE)
		disk[drv]->open(path);
}

void MB8877::close_disk(int drv)
{
	if(drv < MAX_DRIVE) {
		disk[drv]->close();
		cmdtype = 0;
	}
}

bool MB8877::disk_inserted(int drv)
{
	if(drv < MAX_DRIVE)
		return disk[drv]->insert;
	return false;
}

uint8 MB8877::fdc_status()
{
	// for each virtual machines
#ifdef _FMR50
	return disk[drvreg]->insert ? 2 : 0;
#else
	return 0;
#endif
}
