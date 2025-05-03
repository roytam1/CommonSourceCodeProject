/*
	Skelton for retropc emulator

	Origin : M88
	Author : Takeda.Toshiya
	Date   : 2006.09.17-

	[ uPD765A ]
*/

#ifndef _UPD765A_H_
#define _UPD765A_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define SIG_UPD765A_TC		0
#define SIG_UPD765A_MOTOR	1
#define SIG_UPD765A_SELECT	2

#define DRIVE_MASK	(MAX_DRIVE - 1)

class DISK;

class UPD765A : public DEVICE
{
private:
	DEVICE *d_intr[MAX_OUTPUT], *d_drq[MAX_OUTPUT], *d_hdu[MAX_OUTPUT], *d_acctc[MAX_OUTPUT];
	int did_intr[MAX_OUTPUT], did_drq[MAX_OUTPUT], did_hdu[MAX_OUTPUT], did_acctc[MAX_OUTPUT];
	uint32 dmask_intr[MAX_OUTPUT], dmask_drq[MAX_OUTPUT], dmask_hdu[MAX_OUTPUT], dmask_acctc[MAX_OUTPUT];
	int dcount_intr, dcount_drq, dcount_hdu, dcount_acctc;
	
	// fdc
	typedef struct {
		uint8 track;
		uint8 result;
		bool access;
	} fdc_t;
	fdc_t fdc[MAX_DRIVE];
	DISK* disk[MAX_DRIVE];
	
	uint8 hdu, hdue, id[4], eot, gpl, dtl;
	
	int phase, prevphase;
	uint8 status, seekstat, command;
	uint32 result;
	bool acctc, intr, drq, motor, index;
	int sel;	// external drive select
	
	uint8* bufptr;
	uint8 buffer[0x4000];
	int count;
	int event_phase, event_drv;
	int phase_id, seek_id, drq_id, lost_id;
#ifdef UPD765A_DMA_MODE
	bool dma_done;
#endif
	
	// update status
	void set_intr(bool val);
	void req_intr(bool val);
	void req_intr_ndma(bool val);
	void set_drq(bool val);
	void set_hdu(uint8 val);
	void set_acctc(bool val);
	
	// phase shift
	void shift_to_idle();
	void shift_to_cmd(int length);
	void shift_to_exec();
	void shift_to_read(int length);
	void shift_to_write(int length);
	void shift_to_scan(int length);
	void shift_to_result(int length);
	void shift_to_result7();
	
	// command
	void process_cmd(int cmd);
	void cmd_sence_devstat();
	void cmd_sence_intstat();
	uint8 get_devstat(int drv);
	void cmd_seek();
	void cmd_recalib();
	void seek(int drv, int trk);
	void seek_event(int drv);
	void cmd_read_data();
	void cmd_write_data();
	void cmd_scan();
	void cmd_read_diagnostic();
	void read_data(bool deleted, bool scan);
	void write_data(bool deleted);
	void read_diagnostic();
	uint32 read_sector();
	uint32 write_sector(bool deleted);
	uint32 find_id();
	uint32 check_cond(bool write);
	void get_sector_params();
	bool id_incr();
	void cmd_read_id();
	void cmd_write_id();
	uint32 read_id();
	uint32 write_id();
	void cmd_specify();
	void cmd_invalid();
	
public:
	UPD765A(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dcount_intr = dcount_drq = dcount_hdu = dcount_acctc = 0;
	}
	~UPD765A() {}
	
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
	
	// unique function
	void set_context_intr(DEVICE* device, int id, uint32 mask) {
		int c = dcount_intr++;
		d_intr[c] = device; did_intr[c] = id; dmask_intr[c] = mask;
	}
	void set_context_drq(DEVICE* device, int id, uint32 mask) {
		int c = dcount_drq++;
		d_drq[c] = device; did_drq[c] = id; dmask_drq[c] = mask;
	}
	void set_context_hdu(DEVICE* device, int id, uint32 mask) {
		int c = dcount_hdu++;
		d_hdu[c] = device; did_hdu[c] = id; dmask_hdu[c] = mask;
	}
	void set_context_acctc(DEVICE* device, int id, uint32 mask) {
		int c = dcount_acctc++;
		d_acctc[c] = device; did_acctc[c] = id; dmask_acctc[c] = mask;
	}
	void open_disk(_TCHAR path[], int drv);
	void close_disk(int drv);
	bool disk_inserted(int drv);
	uint8 fdc_status();
};

#endif

