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
#define SIG_MB8877_MOTOR	2

#define DRIVE_MASK	(MAX_DRIVE - 1)

// 6msec, 12msec, 20msec, 30msec
static const int seek_wait[4] = {6000, 12000, 20000, 30000};

class DISK;

class MB8877 : public DEVICE
{
private:
	DEVICE *d_irq[MAX_OUTPUT], *d_drq[MAX_OUTPUT];
	int did_irq[MAX_OUTPUT], did_drq[MAX_OUTPUT];
	uint32 dmask_irq[MAX_OUTPUT], dmask_drq[MAX_OUTPUT];
	int dcount_irq, dcount_drq;
	
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
	bool motor;
	
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
	
	// irq/dma
	void set_irq(bool val);
	void set_drq(bool val);
	
public:
	MB8877(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_irq = dcount_drq = 0;
	}
	~MB8877() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	void write_dma8(uint32 addr, uint32 data);
	uint32 read_dma8(uint32 addr);
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void write_signal(int id, uint32 data, uint32 mask);
	uint32 read_signal(int ch);
	void event_callback(int event_id, int err);
	void update_config();
	
	// unique function
	void set_context_irq(DEVICE* device, int id, uint32 mask) {
		int c = dcount_irq++;
		d_irq[c] = device; did_irq[c] = id; dmask_irq[c] = mask;
	}
	void set_context_drq(DEVICE* device, int id, uint32 mask) {
		int c = dcount_drq++;
		d_drq[c] = device; did_drq[c] = id; dmask_drq[c] = mask;
	}
	DISK* get_disk_handler(int drv) {
		return disk[drv];
	}
	void open_disk(_TCHAR path[], int drv);
	void close_disk(int drv);
	bool disk_inserted(int drv);
	uint8 fdc_status();
};

#endif
