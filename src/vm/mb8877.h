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

class DISK;

class MB8877 : public DEVICE
{
private:
	// config
	bool ignore_crc;
	
	// disk info
	DISK* disk[MAX_DRIVE];
	
	// output signals
	outputs_t outputs_irq;
	outputs_t outputs_drq;
	
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
		init_output_signals(&outputs_irq);
		init_output_signals(&outputs_drq);
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
		regist_output_signal(&outputs_irq, device, id, mask);
	}
	void set_context_drq(DEVICE* device, int id, uint32 mask) {
		regist_output_signal(&outputs_drq, device, id, mask);
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
