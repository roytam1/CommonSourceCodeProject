/*
	Skelton for retropc emulator

	Origin : XM7
	Author : Takeda.Toshiya
	Date   : 2006.12.06 -

	[ MB8877 / MB8876 ]
*/

#ifndef _MB8877_H_ 
#define _MB8877_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_MB8877_DRIVEREG	0
#define SIG_MB8877_SIDEREG	1

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

#define DRIVE_MASK	(MAX_DRIVE - 1)

// 6msec, 12msec, 20msec, 30msec
static const int seek_wait[4] = {6000, 12000, 20000, 30000};

class DISK;

class MB8877 : public DEVICE
{
private:
	// config
	bool ignore_crc;
	
	// disk info
	DISK* disk[MAX_DRIVE];
	
	// drive info
	typedef struct {
		int track;
		int index;
		bool access;
	} fdc_t;
	fdc_t fdc[MAX_DRIVE];
	
	// registor
	uint8 status;
	uint8 cmdreg;
	uint8 trkreg;
	uint8 secreg;
	uint8 datareg;
	uint8 drvreg;
	uint8 sidereg;
	uint8 cmdtype;
	
	// event
	int regist_id[7];
	
	// status
	bool now_search;
	bool now_seek;
	int seektrk;
	bool seekvct;
	int indexcnt;
	int sectorcnt;
	
	// image handler
	uint8 search_track();
	uint8 search_sector(int trk, int side, int sct, bool compare);
	uint8 search_addr();
	bool make_track();
	
	// command
	void process_cmd();
	void cmd_restore();
	void cmd_seek();
	void cmd_step();
	void cmd_stepin();
	void cmd_stepout();
	void cmd_readdata();
	void cmd_writedata();
	void cmd_readaddr();
	void cmd_readtrack();
	void cmd_writetrack();
	void cmd_forceint();
	
public:
	MB8877(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {}
	~MB8877() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	uint32 read_signal(int ch);
	void event_callback(int event_id, int err);
	void update_config();
	
	// unique function
	void open_disk(_TCHAR path[], int drv);
	void close_disk(int drv);
	bool disk_inserted(int drv);
};

#endif
