/*
	Skelton for retropc emulator

	Origin : Neko Project 2
	Author : Takeda.Toshiya
	Date   : 2006.12.06 -

	[ uPD7220 ]
*/

#ifndef _UPD7220_H_
#define _UPD7220_H_

#include "vm.h"
#include "../emu.h"
#include "device.h"

#define ADDR_MASK	(vram_size - 1)

#define CMD_RESET	0x00
#define CMD_SYNC	0x0e
#define CMD_SLAVE	0x6e
#define CMD_MASTER	0x6f
#define CMD_START	0x6b
#define CMD_BCTRL	0x0c
#define CMD_ZOOM	0x46
#define CMD_SCROLL	0x70
#define CMD_CSRFORM	0x4b
#define CMD_PITCH	0x47
#define CMD_LPEN	0xc0
#define CMD_VECTW	0x4c
#define CMD_VECTE	0x6c
#define CMD_TEXTW	0x78
#define CMD_TEXTE	0x68
#define CMD_CSRW	0x49
#define CMD_CSRR	0xe0
#define CMD_MASK	0x4a
#define CMD_WRITE	0x20
#define CMD_READ	0xa0
#define CMD_DMAR	0xa4
#define CMD_DMAW	0x24

#define STAT_LPEN	0x80
#define STAT_HBLANK	0x40
#define STAT_VSYNC	0x20
#define STAT_DMA	0x10
#define STAT_DRAW	0x08
#define STAT_EMPTY	0x04
#define STAT_FULL	0x02
#define STAT_DRDY	0x01

#define MODE_MIX	((sync[0] & 0x22) == 0x00)
#define MODE_GFX	((sync[0] & 0x22) == 0x02)
#define MODE_CHR	((sync[0] & 0x22) == 0x20)

#define RT_MULBIT	15
#define RT_TABLEBIT	12
#define RT_TABLEMAX	(1 << RT_TABLEBIT)

static const int vectdir[16][4] = {
	{ 0, 1, 1, 0}, { 1, 1, 1,-1}, { 1, 0, 0,-1}, { 1,-1,-1,-1}, { 0,-1,-1, 0}, {-1,-1,-1, 1}, {-1, 0, 0, 1}, {-1, 1, 1, 1},
	{ 0, 1, 1, 1}, { 1, 1, 1, 0}, { 1, 0, 1,-1}, { 1,-1, 0,-1}, { 0,-1,-1,-1}, {-1,-1,-1, 0}, {-1, 0,-1, 1}, {-1, 1, 0, 1}	// SL
};

class FIFO;

class UPD7220 : public DEVICE
{
private:
	DEVICE* dev;
	int did;
	
	// vram
	uint8* vram;
	int vram_size;
	
	// fifo and regs
	FIFO *fi, *fo, *ft;
	int cmdreg;
	uint8 statreg;
	
	// params
	uint8 sync[16];		// sync
	uint8 zr, zw, zoom;	// zoom
	uint8 ra[16];		// scroll, textw
	uint8 cs[3];		// cursor
	uint8 pitch;		// pitch
	uint32 lad;		// lpen
	uint8 vect[11];		// vectw
	int ead, dad;	// csrw, csrr
	uint8 maskl, maskh;	// mask
	
	bool start, hsync, vsync;
	bool low_high;		// dma word access
	
	// draw
	void draw_vectl();
	void draw_vectt();
	void draw_vectc();
	void draw_vectr();
	void draw_text();
	void pset(int x, int y);
	
	int rt[RT_TABLEMAX + 1];
	int dx, dy;	// from ead, dad
	int dir, sl, dc, d, d2, d1, dm;
	uint16 pattern;
	
	// command
	void check_cmd();
	void process_cmd();
	
	void cmd_reset();
	void cmd_sync();
	void cmd_master();
	void cmd_slave();
	void cmd_start();
	void cmd_bctrl();
	void cmd_zoom();
	void cmd_scroll();
	void cmd_csrform();
	void cmd_pitch();
	void cmd_lpen();
	void cmd_vectw();
	void cmd_vecte();
	void cmd_texte();
	void cmd_csrw();
	void cmd_csrr();
	void cmd_mask();
	void cmd_write();
	void cmd_read();
	void cmd_dmaw();
	void cmd_dmar();
	
	void cmd_write_sub(uint32 addr, uint8 data);
	uint8 cmd_read_sub(uint32 addr) { return vram[addr & ADDR_MASK]; }
	void vectreset();
	
public:
	UPD7220(VM* parent_vm, EMU* parent_emu) : DEVICE(parent_vm, parent_emu) {
		dev = NULL;
	}
	~UPD7220() {}
	
	// common functions
	void initialize();
	void release();
	void write_data8(uint32 addr, uint32 data);	// for dma
	uint32 read_data8(uint32 addr);
	void write_io8(uint32 addr, uint32 data);
	uint32 read_io8(uint32 addr);
	void event_vsync(int v, int clock);
	void event_hsync(int v, int h, int clock);
	
	// unique functions
	void set_context(DEVICE* device, int id) {
		dev = device; did = id;
	}
	void set_vram_ptr(uint8* ptr, int size) {
		vram = ptr; vram_size = size;
	}
};

#endif

