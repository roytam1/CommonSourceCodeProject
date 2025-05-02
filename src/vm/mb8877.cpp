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

extern config_t config;

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
	status = cmdreg = trkreg = secreg = datareg = drvreg = sidereg = cmdtype = 0;
	
	for(int i = 0; i < MAX_DRIVE; i++) {
		fdc[i].track = 0;
		fdc[i].index = 0;
		fdc[i].access = false;
	}
	for(int i = 0; i < 7; i++)
		regist_id[i] = -1;
	now_seek = now_search = false;
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

void MB8877::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 3)
	{
	case 0x0:
		// command reg
#ifdef MB8876
		cmdreg = (~data) & 0xff;
#else
		cmdreg = data;
#endif
		process_cmd();
		break;
	case 0x1:
		// track reg
#ifdef MB8876
		trkreg = (~data) & 0xff;
#else
		trkreg = data;
#endif
		if((status & FDC_ST_BUSY) && (fdc[drvreg & DRIVE_MASK].index == 0)) {
			// track reg is written after command starts
			if(cmdtype == FDC_CMD_RD_SEC || cmdtype == FDC_CMD_RD_MSEC || cmdtype == FDC_CMD_WR_SEC || cmdtype == FDC_CMD_WR_MSEC)
				process_cmd();
		}
		break;
	case 0x2:
		// sector reg
#ifdef MB8876
		secreg = (~data) & 0xff;
#else
		secreg = data;
#endif
		if((status & FDC_ST_BUSY) && (fdc[drvreg & DRIVE_MASK].index == 0)) {
			// sector reg is written after command starts
			if(cmdtype == FDC_CMD_RD_SEC || cmdtype == FDC_CMD_RD_MSEC || cmdtype == FDC_CMD_WR_SEC || cmdtype == FDC_CMD_WR_MSEC)
				process_cmd();
		}
		break;
	case 0x3:
		// data reg
#ifdef MB8876
		datareg = (~data) & 0xff;
#else
		datareg = data;
#endif
		if((drvreg & 0x80) && (status & FDC_ST_DRQ) && !now_search) {
			if(cmdtype == FDC_CMD_WR_SEC || cmdtype == FDC_CMD_WR_MSEC) {
				// write or multisector write
				if(fdc[drvreg & DRIVE_MASK].index < disk[drvreg & DRIVE_MASK]->sector_size) {
					if(!disk[drvreg & DRIVE_MASK]->protect) {
						disk[drvreg & DRIVE_MASK]->sector[fdc[drvreg & DRIVE_MASK].index] = datareg;
						// dm, ddm
						disk[drvreg & DRIVE_MASK]->deleted = (cmdreg & 1) ? 0x10 : 0x00;
					}
					else {
						status |= FDC_ST_WRITEFAULT;
						status &= ~FDC_ST_BUSY;
						status &= ~FDC_ST_DRQ;
						cmdtype = 0;
					}
					fdc[drvreg & DRIVE_MASK].index++;
				}
				if(fdc[drvreg & DRIVE_MASK].index >= disk[drvreg & DRIVE_MASK]->sector_size) {
					if(cmdtype == FDC_CMD_WR_SEC) {
						// single sector
						status &= ~FDC_ST_BUSY;
						status &= ~FDC_ST_DRQ;
						cmdtype = 0;
					}
					else {
						// multisector
						status &= ~FDC_ST_DRQ;
						REGIST_EVENT(EVENT_MULTI1, 30);
						REGIST_EVENT(EVENT_MULTI2, 60);
					}
				}
				fdc[drvreg & DRIVE_MASK].access = true;
			}
			else if(cmdtype == FDC_CMD_WR_TRK) {
				// read track
				if(fdc[drvreg & DRIVE_MASK].index < disk[drvreg & DRIVE_MASK]->track_size) {
					if(!disk[drvreg & DRIVE_MASK]->protect)
						disk[drvreg & DRIVE_MASK]->track[fdc[drvreg & DRIVE_MASK].index] = datareg;
					else {
						status |= FDC_ST_WRITEFAULT;
						status &= ~FDC_ST_BUSY;
						status &= ~FDC_ST_DRQ;
						cmdtype = 0;
					}
					fdc[drvreg & DRIVE_MASK].index++;
				}
				if(fdc[drvreg & DRIVE_MASK].index >= disk[drvreg & DRIVE_MASK]->track_size) {
					status &= ~FDC_ST_BUSY;
					status &= ~FDC_ST_DRQ;
					cmdtype = 0;
				}
				fdc[drvreg & DRIVE_MASK].access = true;
			}
		}
		break;
	}
}

uint32 MB8877::read_io8(uint32 addr)
{
	uint32 val;
	
	switch(addr & 3)
	{
	case 0x0:
		// status reg
		
		// now force interrupt
		if(cmdtype == FDC_CMD_TYPE4)
#ifdef MB8876
			return 0xff;
#else
			return 0;
#endif
		// now sector search
		if(now_search)
#ifdef MB8876
			return (~FDC_ST_BUSY) & 0xff;
#else
			return FDC_ST_BUSY;
#endif
		// disk not inserted, motor stop
		if(!disk[drvreg & DRIVE_MASK]->insert || !(drvreg & 0x80))
			status |= FDC_ST_NOTREADY;
		else
			status &= ~FDC_ST_NOTREADY;
		// write protect
		if(cmdtype == FDC_CMD_TYPE1 || cmdtype == FDC_CMD_WR_SEC || cmdtype == FDC_CMD_WR_MSEC || cmdtype == FDC_CMD_WR_TRK) {
			if(disk[drvreg & DRIVE_MASK]->protect)
				status |= FDC_ST_WRITEP;
			else
				status &= ~FDC_ST_WRITEP;
		}
		else
			status &= ~FDC_ST_WRITEP;
		
		// track0, index hole
		if(cmdtype == FDC_CMD_TYPE1) {
			if(fdc[drvreg & DRIVE_MASK].track == 0)
				status |= FDC_ST_TRACK00;
			else
				status &= ~FDC_ST_TRACK00;
			if(!(status & FDC_ST_NOTREADY)) {
				if(indexcnt == 0)
					status |= FDC_ST_INDEX;
				else
					status &= ~FDC_ST_INDEX;
				if(++indexcnt >= ((disk[drvreg & DRIVE_MASK]->sector_num == 0) ? 16 : disk[drvreg & DRIVE_MASK]->sector_num))
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
	case 0x1:
		// track reg
#ifdef MB8876
		return (~trkreg) & 0xff;
#else
		return trkreg;
#endif
	case 0x2:
		// sector reg
#ifdef MB8876
		return (~secreg) & 0xff;
#else
		return secreg;
#endif
	case 0x3:
		// data reg
		if((drvreg & 0x80) && (status & FDC_ST_DRQ) && !now_search) {
			if(cmdtype == FDC_CMD_RD_SEC || cmdtype == FDC_CMD_RD_MSEC) {
				// read or multisector read
				if(fdc[drvreg & DRIVE_MASK].index < disk[drvreg & DRIVE_MASK]->sector_size) {
					datareg = disk[drvreg & DRIVE_MASK]->sector[fdc[drvreg & DRIVE_MASK].index];
					fdc[drvreg & DRIVE_MASK].index++;
				}
				if(fdc[drvreg & DRIVE_MASK].index >= disk[drvreg & DRIVE_MASK]->sector_size) {
					if(cmdtype == FDC_CMD_RD_SEC) {
						// single sector
						status &= ~FDC_ST_BUSY;
						status &= ~FDC_ST_DRQ;
						cmdtype = 0;
					}
					else {
						// multisector
						status &= ~FDC_ST_DRQ;
						REGIST_EVENT(EVENT_MULTI1, 30);
						REGIST_EVENT(EVENT_MULTI2, 60);
					}
				}
				fdc[drvreg & DRIVE_MASK].access = true;
			}
			else if(cmdtype == FDC_CMD_RD_ADDR) {
				// read address
				if(fdc[drvreg & DRIVE_MASK].index < 6) {
					datareg = disk[drvreg & DRIVE_MASK]->id[fdc[drvreg & DRIVE_MASK].index];
					fdc[drvreg & DRIVE_MASK].index++;
				}
				if(fdc[drvreg & DRIVE_MASK].index >= 6) {
					status &= ~FDC_ST_BUSY;
					status &= ~FDC_ST_DRQ;
					cmdtype = 0;
				}
				fdc[drvreg & DRIVE_MASK].access = true;
			}
			else if(cmdtype == FDC_CMD_RD_TRK) {
				// read track
				if(fdc[drvreg & DRIVE_MASK].index < disk[drvreg & DRIVE_MASK]->track_size) {
					datareg = disk[drvreg & DRIVE_MASK]->track[fdc[drvreg & DRIVE_MASK].index];
					fdc[drvreg & DRIVE_MASK].index++;
				}
				if(fdc[drvreg & DRIVE_MASK].index >= disk[drvreg & DRIVE_MASK]->track_size) {
					status &= ~FDC_ST_BUSY;
					status &= ~FDC_ST_DRQ;
					status |= FDC_ST_LOSTDATA;
					cmdtype = 0;
				}
				fdc[drvreg & DRIVE_MASK].access = true;
			}
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
		drvreg = data & mask;
	else if(id == SIG_MB8877_SIDEREG)
		sidereg = data & mask;
}

void MB8877::event_callback(int event_id)
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
		if(seektrk > fdc[drvreg & DRIVE_MASK].track)
			fdc[drvreg & DRIVE_MASK].track++;
		else if(seektrk < fdc[drvreg & DRIVE_MASK].track)
			fdc[drvreg & DRIVE_MASK].track--;
		if(cmdreg & 0x10)
			trkreg = fdc[drvreg & DRIVE_MASK].track;
		else if((cmdreg & 0xf0) == 0x00)
			trkreg--;
		if(seektrk == fdc[drvreg & DRIVE_MASK].track) {
			// auto update
			if((cmdreg & 0x10) || ((cmdreg & 0xf0) == 0x00))
				trkreg = fdc[drvreg & DRIVE_MASK].track;
			if((cmdreg & 0xf0) == 0x00)
				datareg = 0;
			status |= search_track();
			now_seek = false;
		}
		else {
			REGIST_EVENT(EVENT_SEEK, seek_wait[cmdreg & 3]);
		}
		break;
	case EVENT_SEEKEND:
		if(seektrk == fdc[drvreg & DRIVE_MASK].track) {
			// auto update
			if((cmdreg & 0x10) || ((cmdreg & 0xf0) == 0x00))
				trkreg = fdc[drvreg & DRIVE_MASK].track;
			if((cmdreg & 0xf0) == 0x00)
				datareg = 0;
			status |= search_track();
			now_seek = false;
			CANCEL_EVENT(EVENT_SEEK);
		}
		break;
	case EVENT_SEARCH:
		now_search = false;
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
	seektrk = fdc[drvreg & DRIVE_MASK].track + datareg - trkreg;
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
	
	seektrk = (fdc[drvreg & DRIVE_MASK].track < 83) ? fdc[drvreg & DRIVE_MASK].track + 1 : 83;
	seekvct = false;
	
	REGIST_EVENT(EVENT_SEEK, seek_wait[cmdreg & 3]);
	REGIST_EVENT(EVENT_SEEKEND, 300);
}

void MB8877::cmd_stepout()
{
	// type-1 step out
	cmdtype = FDC_CMD_TYPE1;
	status = FDC_ST_HEADENG | FDC_ST_BUSY;
	
	seektrk = (fdc[drvreg & DRIVE_MASK].track > 0) ? fdc[drvreg & DRIVE_MASK].track - 1 : 0;
	seekvct = true;
	
	REGIST_EVENT(EVENT_SEEK, seek_wait[cmdreg & 3]);
	REGIST_EVENT(EVENT_SEEKEND, 300);
}

void MB8877::cmd_readdata()
{
	// type-2 read data
	cmdtype = (cmdreg & 0x10) ? FDC_CMD_RD_MSEC : FDC_CMD_RD_SEC;
	if(cmdreg & 0x02)
		status = search_sector(fdc[drvreg & DRIVE_MASK].track, ((cmdreg & 8) ? 1 : 0), secreg, true);
	else
		status = search_sector(fdc[drvreg & DRIVE_MASK].track, sidereg & 1, secreg, false);
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
	if(cmdreg & 0x02)
		status = search_sector(fdc[drvreg & DRIVE_MASK].track, ((cmdreg & 8) ? 1 : 0), secreg, true);
	else
		status = search_sector(fdc[drvreg & DRIVE_MASK].track, sidereg & 1, secreg, false);
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
	
	disk[drvreg & DRIVE_MASK]->track_size = 0x1800;
	fdc[drvreg & DRIVE_MASK].index = 0;
	
	REGIST_EVENT(EVENT_SEARCH, 200);
	REGIST_EVENT(EVENT_LOST, 150000);
}

void MB8877::cmd_forceint()
{
	// type-4 force interrupt
	if(cmdtype == 0) {
		status = 0;
		cmdtype = FDC_CMD_TYPE1;
	}
	status &= ~FDC_ST_BUSY;
	
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
	int drv = drvreg & DRIVE_MASK;
	int trk = fdc[drv].track;
	int side = sidereg & 1;
	
	if(!disk[drv]->get_track(trk, side))
		return FDC_ST_SEEKERR;
	
	// verify track number
	if(!(cmdreg & 0x4))
		return 0;
	for(int i = 0; i < disk[drv]->sector_num; i++) {
		if(disk[drv]->verify[i] == trkreg)
			return 0;
	}
	return FDC_ST_SEEKERR;
}

uint8 MB8877::search_sector(int trk, int side, int sct, bool compare)
{
	int drv = drvreg & DRIVE_MASK;
	
	// get track
	if(!disk[drv]->get_track(trk, side))
		return FDC_ST_RECNFND;
	
	// first scanned sector
	int sector_num = disk[drv]->sector_num;
	if(sectorcnt >= sector_num)
		sectorcnt = 0;
	
	// scan sectors
	for(int i = 0; i < sector_num; i++) {
		// get sector
		int index = sectorcnt + i;
		if(index >= sector_num)
			index -= sector_num;
		disk[drv]->get_sector(trk, side, index);
		
		// check id
		if(disk[drv]->id[2] != sct)
			continue;
		// check density
		if(disk[drv]->density)
			continue;
		
		// sector found
		sectorcnt = index + 1;
		if(sectorcnt >= sector_num)
			sectorcnt -= sector_num;
		
		fdc[drv].index = 0;
		return (disk[drv]->deleted ? FDC_ST_RECTYPE : 0) | ((disk[drv]->status && !ignore_crc) ? FDC_ST_CRCERR : 0);
	}
	
	// sector not found
	disk[drv]->sector_size = 0;
	return FDC_ST_RECNFND;
}

uint8 MB8877::search_addr()
{
	int drv = drvreg & DRIVE_MASK;
	int trk = fdc[drv].track;
	int side = sidereg & 1;
	
	// get track
	if(!disk[drv]->get_track(trk, side))
		return FDC_ST_RECNFND;
	
	// get sector
	if(sectorcnt >= disk[drv]->sector_num)
		sectorcnt = 0;
	if(disk[drv]->get_sector(trk, side, sectorcnt)) {
		sectorcnt++;
		
		fdc[drv].index = 0;
		secreg = disk[drv]->id[0];
		return (disk[drv]->status && !ignore_crc) ? FDC_ST_CRCERR : 0;
	}
	
	// sector not found
	disk[drv]->sector_size = 0;
	return FDC_ST_RECNFND;
}

bool MB8877::make_track()
{
	int drv = drvreg & DRIVE_MASK;
	int trk = fdc[drv].track;
	int side = sidereg & 1;
	
	return disk[drv]->make_track(trk, side);
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

