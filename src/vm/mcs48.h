/*
	Skelton for retropc emulator

	Origin : MAME 0.148
	Author : Takeda.Toshiya
	Date   : 2013.05.01-

	[ MCS48 ]
*/

#ifndef _MCS84_H_ 
#define _MCS48_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define MCS48_PORT_P0	0x100	/* Not used */
#define MCS48_PORT_P1	0x101	/* P10-P17 */
#define MCS48_PORT_P2	0x102	/* P20-P28 */
#define MCS48_PORT_T0	0x110
#define MCS48_PORT_T1	0x111
#define MCS48_PORT_BUS	0x120	/* DB0-DB7 */
#define MCS48_PORT_PROG	0x121	/* PROG line to 8243 expander */

class MCS48 : public DEVICE
{
private:
	/* ---------------------------------------------------------------------------
	contexts
	--------------------------------------------------------------------------- */
	
	DEVICE *d_io, *d_intr;
	void *opaque;
	
public:
	MCS48(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		d_io = d_intr = NULL;
	}
	~MCS48() {}
	
	// common functions
	void initialize();
	void release();
	void reset();
	int run(int icount);
	void write_signal(int id, uint32 data, uint32 mask);
	uint32 get_pc();
	
	// unique functions
	void set_context_io(DEVICE* device) {
		d_io = device;
	}
	void set_context_intr(DEVICE* device) {
		d_intr = device;
	}
	void load_rom_image(_TCHAR *file_path);
	uint8 *get_rom_ptr();
	uint8 *get_ram_ptr();
};

#endif

