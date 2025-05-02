/*
	SHARP X1twin Emulator 'eX1twin'
	Skelton for retropc emulator

	Origin : Ootake (joypad)
	       : xpce (psg)
	       : MESS (vdc)
	Author : Takeda.Toshiya
	Date   : 2009.03.11-

	[ PC-Eninge ]
*/

#include <math.h>
#include "pce.h"
#include "../huc6280.h"

#define STATE_VSW		0
#define STATE_VDS		1
#define STATE_VDW		2
#define STATE_VCR		3

/* Bits in the VDC status register */

#define VDC_BSY         0x40    /* Set when the VDC accesses VRAM */
#define VDC_VD          0x20    /* Set when in the vertical blanking period */
#define VDC_DV          0x10    /* Set when a VRAM > VRAM DMA transfer is done */
#define VDC_DS          0x08    /* Set when a VRAM > SATB DMA transfer is done */
#define VDC_RR          0x04    /* Set when the current scanline equals the RCR register */
#define VDC_OR          0x02    /* Set when there are more than 16 sprites on a line */
#define VDC_CR          0x01    /* Set when sprite #0 overlaps with another sprite */

/* Bits in the CR register */

#define CR_BB           0x80    /* Background blanking */
#define CR_SB           0x40    /* Object blanking */
#define CR_VR           0x08    /* Interrupt on vertical blank enable */
#define CR_RC           0x04    /* Interrupt on line compare enable */
#define CR_OV           0x02    /* Interrupt on sprite overflow enable */
#define CR_CC           0x01    /* Interrupt on sprite #0 collision enable */

/* Bits in the DCR regsiter */

#define DCR_DSR         0x10    /* VRAM > SATB auto-transfer enable */
#define DCR_DID         0x08    /* Destination diretion */
#define DCR_SID         0x04    /* Source direction */
#define DCR_DVC         0x02    /* VRAM > VRAM EOT interrupt enable */
#define DCR_DSC         0x01    /* VRAM > SATB EOT interrupt enable */

/* just to keep things simple... */
enum vdc_regs {MAWR = 0, MARR, VxR, reg3, reg4, CR, RCR, BXR, BYR, MWR, HSR, HDR, VPR, VDW, VCR, DCR, SOUR, DESR, LENR, DVSSR };

enum line_state
{
	CLEAR_LINE = 0,				// clear (a fired or held) line
	ASSERT_LINE,				// assert an interrupt immediately
	HOLD_LINE,				// hold interrupt line until acknowledged
	PULSE_LINE				// pulse interrupt line instantaneously (only for NMI, RESET)
};

enum
{
	INPUT_LINE_IRQ1 = 0,
	INPUT_LINE_IRQ2 = 1,
	INPUT_LINE_TIRQ = 2,
	INPUT_LINE_NMI
};

void PCE::initialize()
{
	// get context
	joy_stat = emu->joy_buffer();
	key_stat = emu->key_buffer();
	
	// register event
	register_vline_event(this);
}

void PCE::reset()
{
	// reset memory bus
	memset(ram, 0, sizeof(ram));
	bank = 0x80000;
	buffer = 0xff;	// ???
	
	// reset devices
	vdc_reset();
	psg_reset();
	joy_reset();
}

void PCE::event_vline(int v, int clock)
{
	pce_interrupt();
}

void PCE::write_data8(uint32 addr, uint32 data)
{
	uint8 mpr = (addr >> 13) & 0xff;
	uint16 ofs = addr & 0x1fff;
	
	switch(mpr) {
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
		// populous
		cart[addr & 0xfffff] = data;
		return;
	case 0xf7:
//		if(ofs < 0x800) {
//			backup[ofs] = data;
//		}
		return;
	case 0xf8:
		ram[ofs] = data;
		return;
	case 0xf9:
	case 0xfa:
	case 0xfb:
//		ram[addr & 0x7fff] = data;
		return;
	case 0xff:
		write_io8(addr, data);
		return;
	}
	// bank switch for sf2d
	if((addr & 0x1ffc) == 0x1ff0) {
		bank = 0x80000 * ((addr & 3) + 1);
	}
}

uint32 PCE::read_data8(uint32 addr)
{
	uint8 mpr = (addr >> 13) & 0xff;
	uint16 ofs = addr & 0x1fff;
	
	if(mpr <= 0x3f) {
		return cart[addr & 0x7ffff];
	}
	if(mpr <= 0x7f) {
		return cart[bank | (addr & 0x7ffff)];
	}
	switch(mpr) {
//	case 0xf7:
//		if(ofs < 0x800) {
//			return backup[ofs];
//		}
//		return 0xff;
	case 0xf8:
		return ram[ofs];
//	case 0xf9:
//	case 0xfa:
//	case 0xfb:
//		return ram[addr & 0x7fff];
	case 0xff:
		return read_io8(addr);
	}
	return 0xff;
}

void PCE::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0x1c00) {
	case 0x0000:	// vdc
		vdc_w(addr, data);
		break;
	case 0x0400:	// vce
		vce_w(addr, data);
		break;
	case 0x0800:	// psg
		buffer = data;
		psg_write(addr, data);
		break;
	case 0x0c00:	// timer
		buffer = data;
		d_cpu->timer_w(addr, data);
		break;
	case 0x1000:	// joypad
		buffer = data;
		joy_write(addr, data);
		break;
	case 0x1400:	// interrupt control
		buffer = data;
		d_cpu->irq_status_w(addr, data);
		break;
	}
}

uint32 PCE::read_io8(uint32 addr)
{
	switch (addr & 0x1c00) {
	case 0x0000: // vdc
		return vdc_r(addr);
	case 0x0400: // vce
		return vce_r(addr);
	case 0x0800: // psg
//		return psg_read(addr);
		return buffer;
	case 0x0c00: // timer
		buffer = (buffer & 0x80) | (d_cpu->timer_r(addr) & 0x7f);
		return buffer;
	case 0x1000: // joypad
		if(joy_nibble) {
			joy_count = (joy_count + 1) & 15;
		}
		buffer = (buffer & 0xb0) | (joy_read(addr) & 0x0f);
		return buffer;
	case 0x1400: // interrupt control
		if(addr & 2) {
			buffer = (buffer & 0xf8) | (d_cpu->irq_status_r(addr) & 0x07);
		}
		return buffer;
	}
	return 0xff;
}

void PCE::draw_screen()
{
	int dx = (SCREEN_WIDTH - vdc.physical_width) / 2;
	int dy = (SCREEN_HEIGHT - VDC_LPF) / 2;
	
	for(int y = 0; y < VDC_LPF; y++, dy++) {
		scrntype* src = &vce.bmp[y][86];
		scrntype* dst = emu->screen_buffer(dy) + dx;
		for(int x = 0; x < vdc.physical_width; x++) {
			dst[x] = src[x];
		}
	}
}

void PCE::open_cart(_TCHAR* file_path)
{
	FILEIO* fio = new FILEIO();
	
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		memset(cart, 0xff, sizeof(cart));
		fio->Fseek(0, FILEIO_SEEK_END);
		int size = fio->Ftell();
		int head = size % 1024;
		size -= head;
		fio->Fseek(head, FILEIO_SEEK_SET);
		fio->Fread(cart, size, 1);
		fio->Fclose();
		
		if(size == 384 * 1024) {
			memcpy(cart + 0x060000, cart + 0x040000, 0x020000);	/* Set up 060000 - 07FFFF mirror */
			memcpy(cart + 0x080000, cart + 0x040000, 0x040000);	/* Set up 080000 - 0BFFFF region */
			memcpy(cart + 0x0C0000, cart + 0x040000, 0x040000);	/* Set up 0C0000 - 0FFFFF region */
			memcpy(cart + 0x040000, cart, 0x040000);		/* Set up 040000 - 07FFFF region */
		}
		else {
			/* mirror 256KB rom data */
			if (size <= 0x040000)
				memcpy(cart + 0x040000, cart, 0x040000);
			/* mirror 512KB rom data */
			if (size <= 0x080000)
				memcpy(cart + 0x080000, cart, 0x080000);
		}
		vm->pce_running = true;
	}
	delete fio;
}

void PCE::close_cart()
{
	memset(cart, 0xff, sizeof(cart));
	vm->pce_running = false;
}

// vdc

void PCE::pce_interrupt()
{
	/* Draw the last scanline */
	if ( vce.current_bitmap_line >= 14 && vce.current_bitmap_line < 14 + 242 )
	{
		/* We are in the active display area */
		/* First fill the line with the overscan color */
		draw_overscan_line(vce.current_bitmap_line );

		/* Check if we need to draw more just the overscan color */
		if ( vdc.current_segment == STATE_VDW )
		{
			/* 0 - no sprite and background pixels drawn
			   1 - background pixel drawn
			   otherwise is 2 + sprite# */
			uint8 drawn[VDC_WPF];
			/* our line buffer */
			scrntype *line_buffer = &vce.bmp[vce.current_bitmap_line][86];

			/* clear our priority/sprite collision detection buffer. */
			memset(drawn, 0, VDC_WPF);

			vdc.y_scroll = ( vdc.current_segment_line == 0 ) ? vdc.vdc_data[BYR].w.l : ( vdc.y_scroll + 1 );

			/* Draw VDC #0 background layer */
			pce_refresh_line(vdc.current_segment_line, drawn, line_buffer);

			/* Draw VDC #0 sprite layer */
			if(vdc.vdc_data[CR].w.l & CR_SB)
			{
				pce_refresh_sprites(vdc.current_segment_line, drawn, line_buffer);
			}
		}
	}
	else
	{
		/* We are in one of the blanking areas */
		draw_black_line(vce.current_bitmap_line );
	}

	/* bump current scanline */
	vce.current_bitmap_line = ( vce.current_bitmap_line + 1 ) % VDC_LPF;
	vdc_advance_line();
}

void PCE::vdc_advance_line()
{
	int ret = 0;

	vdc.curline += 1;
	vdc.current_segment_line += 1;
	vdc.raster_count += 1;

	if ( vdc.satb_countdown )
	{
		vdc.satb_countdown -= 1;
		if ( vdc.satb_countdown == 0 )
		{
			if ( vdc.vdc_data[DCR].w.l & DCR_DSC )
			{
				vdc.status |= VDC_DS;	/* set satb done flag */
				ret = 1;
			}
		}
	}

	if ( vce.current_bitmap_line == 0 )
	{
		vdc.current_segment = STATE_VSW;
		vdc.current_segment_line = 0;
		vdc.vblank_triggered = 0;
		vdc.curline = 0;
	}

	if ( STATE_VSW == vdc.current_segment && vdc.current_segment_line >= ( vdc.vdc_data[VPR].b.l & 0x1F ) )
	{
		vdc.current_segment = STATE_VDS;
		vdc.current_segment_line = 0;
	}

	if ( STATE_VDS == vdc.current_segment && vdc.current_segment_line >= vdc.vdc_data[VPR].b.h )
	{
		vdc.current_segment = STATE_VDW;
		vdc.current_segment_line = 0;
		vdc.raster_count = 0x40;
	}

	if ( STATE_VDW == vdc.current_segment && vdc.current_segment_line > ( vdc.vdc_data[VDW].w.l & 0x01FF ) )
	{
		vdc.current_segment = STATE_VCR;
		vdc.current_segment_line = 0;

		/* Generate VBlank interrupt, sprite DMA */
		vdc.vblank_triggered = 1;
		if ( vdc.vdc_data[CR].w.l & CR_VR )
		{
			vdc.status |= VDC_VD;
			ret = 1;
		}

		/* do VRAM > SATB DMA if the enable bit is set or the DVSSR reg. was written to */
		if( ( vdc.vdc_data[DCR].w.l & DCR_DSR ) || vdc.dvssr_write )
		{
			int i;

			vdc.dvssr_write = 0;

			for( i = 0; i < 256; i++ )
			{
				vdc.sprite_ram[i] = ( vdc.vram[ ( vdc.vdc_data[DVSSR].w.l << 1 ) + i * 2 + 1 ] << 8 ) | vdc.vram[ ( vdc.vdc_data[DVSSR].w.l << 1 ) + i * 2 ];
			}

			/* generate interrupt if needed */
			if ( vdc.vdc_data[DCR].w.l & DCR_DSC )
			{
				vdc.satb_countdown = 4;
			}
		}
	}

	if ( STATE_VCR == vdc.current_segment )
	{
		if ( vdc.current_segment_line >= 3 && vdc.current_segment_line >= vdc.vdc_data[VCR].b.l )
		{
			vdc.current_segment = STATE_VSW;
			vdc.current_segment_line = 0;
			vdc.curline = 0;
		}
	}

	/* generate interrupt on line compare if necessary */
	if ( vdc.raster_count == vdc.vdc_data[RCR].w.l && vdc.vdc_data[CR].w.l & CR_RC )
	{
		vdc.status |= VDC_RR;
		ret = 1;
	}

	/* handle frame events */
	if(vdc.curline == 261 && ! vdc.vblank_triggered )
	{

		vdc.vblank_triggered = 1;
		if(vdc.vdc_data[CR].w.l & CR_VR)
		{	/* generate IRQ1 if enabled */
			vdc.status |= VDC_VD;	/* set vblank flag */
			ret = 1;
		}

		/* do VRAM > SATB DMA if the enable bit is set or the DVSSR reg. was written to */
		if ( ( vdc.vdc_data[DCR].w.l & DCR_DSR ) || vdc.dvssr_write )
		{
			int i;

			vdc.dvssr_write = 0;
			for( i = 0; i < 256; i++ )
			{
				vdc.sprite_ram[i] = ( vdc.vram[ ( vdc.vdc_data[DVSSR].w.l << 1 ) + i * 2 + 1 ] << 8 ) | vdc.vram[ ( vdc.vdc_data[DVSSR].w.l << 1 ) + i * 2 ];
			}

			/* generate interrupt if needed */
			if(vdc.vdc_data[DCR].w.l & DCR_DSC)
			{
				vdc.satb_countdown = 4;
			}
		}
	}

	if (ret)
		d_cpu->write_signal(INPUT_LINE_IRQ1, HOLD_LINE, 0);
}

void PCE::vdc_reset()
{
	/* clear context */
	memset(&vdc, 0, sizeof(vdc));
	memset(&vce, 0, sizeof(vce));

	vdc.inc = 1;

	/* initialize palette */
	int i;

	for( i = 0; i < 512; i++ )
	{
		int r = (( i >> 3) & 7) << 5;
		int g = (( i >> 6) & 7) << 5;
		int b = (( i     ) & 7) << 5;
		int y = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
		vce.palette[i] = RGB_COLOR(r, g, b);
		vce.palette[512+i] = RGB_COLOR(y, y, y);
	}
}

void PCE::draw_black_line(int line)
{
	int i;

	/* our line buffer */
	scrntype *line_buffer = vce.bmp[line];

	for( i=0; i< VDC_WPF; i++ )
		line_buffer[i] = 0;
}

void PCE::draw_overscan_line(int line)
{
	int i;

	/* Are we in greyscale mode or in color mode? */
	scrntype *color_base = vce.palette + (vce.vce_control & 0x80 ? 512 : 0);

	/* our line buffer */
	scrntype *line_buffer = vce.bmp[line];

	for ( i = 0; i < VDC_WPF; i++ )
		line_buffer[i] = color_base[vce.vce_data[0x100].w.l];
}

void PCE::vram_write(uint32 offset, uint8 data)
{
	if(offset & 0x10000)
	{
		return;
	}
	else
	{
		vdc.vram[offset] = data;
	}
}

uint8 PCE::vram_read(uint32 offset)
{
	uint8 temp;

	if(offset & 0x10000)
	{
		temp = vdc.vram[offset & 0xFFFF];
	}
	else
	{
		temp = vdc.vram[offset];
	}

	return temp;
}

void PCE::vdc_w(uint16 offset, uint8 data)
{
	switch(offset&3)
	{
		case 0x00:	/* VDC register select */
			vdc.vdc_register = (data & 0x1F);
			break;

		case 0x02:	/* VDC data (LSB) */
			vdc.vdc_data[vdc.vdc_register].b.l = data;
			switch(vdc.vdc_register)
			{
				case VxR:	/* LSB of data to write to VRAM */
					vdc.vdc_latch = data;
					break;

				case BYR:
					vdc.y_scroll=vdc.vdc_data[BYR].w.l;
					break;

				case HDR:
					vdc.physical_width = ((data & 0x003F) + 1) << 3;
					break;

				case VDW:
					vdc.physical_height &= 0xFF00;
					vdc.physical_height |= (data & 0xFF);
					vdc.physical_height &= 0x01FF;
					break;

				case LENR:
					break;
				case SOUR:
					break;
				case DESR:
					break;
			}
			break;

		case 0x03:	/* VDC data (MSB) */
			vdc.vdc_data[vdc.vdc_register].b.h = data;
			switch(vdc.vdc_register)
			{
				case VxR:	/* MSB of data to write to VRAM */
					vram_write(vdc.vdc_data[MAWR].w.l*2+0, vdc.vdc_latch);
					vram_write(vdc.vdc_data[MAWR].w.l*2+1, data);
					vdc.vdc_data[MAWR].w.l += vdc.inc;
					break;

				case CR:
					{
						static const unsigned char inctab[] = {1, 32, 64, 128};
						vdc.inc = inctab[(data >> 3) & 3];
					}
					break;

				case VDW:
					vdc.physical_height &= 0x00FF;
					vdc.physical_height |= (data << 8);
					vdc.physical_height &= 0x01FF;
					break;

				case DVSSR:
					/* Force VRAM <> SATB DMA for this frame */
					vdc.dvssr_write = 1;
					break;

				case BYR:
					vdc.y_scroll=vdc.vdc_data[BYR].w.l;
					break;

				case LENR:
					vdc_do_dma();
					break;
				case SOUR:
					break;
				case DESR:
					break;
			}
			break;
	}
}

uint8 PCE::vdc_r(uint16 offset)
{
	int temp = 0;
	switch(offset & 3)
	{
		case 0x00:
			temp = vdc.status;
			vdc.status &= ~(VDC_VD | VDC_DV | VDC_DS | VDC_RR | VDC_OR | VDC_CR);
			d_cpu->write_signal(INPUT_LINE_IRQ1, CLEAR_LINE, 0);
			break;

		case 0x02:
			temp = vram_read(vdc.vdc_data[MARR].w.l * 2 + 0);
			break;

		case 0x03:
			temp = vram_read(vdc.vdc_data[MARR].w.l * 2 + 1);
			if ( vdc.vdc_register == VxR )
			{
				vdc.vdc_data[MARR].w.l += vdc.inc;
			}
			break;
	}
	return (temp);
}

void PCE::vce_w(uint16 offset, uint8 data)
{
	switch(offset & 7)
	{
		case 0x00:	/* control reg. */
			vce.vce_control = data;
			break;

		case 0x02:	/* color table address (LSB) */
			vce.vce_address.b.l = data;
			vce.vce_address.w.l &= 0x1FF;
			break;

		case 0x03:	/* color table address (MSB) */
			vce.vce_address.b.h = data;
			vce.vce_address.w.l &= 0x1FF;
			break;

		case 0x04:	/* color table data (LSB) */
			vce.vce_data[vce.vce_address.w.l].b.l = data;
			break;

		case 0x05:	/* color table data (MSB) */
			vce.vce_data[vce.vce_address.w.l].b.h = data & 0x01;

			/* bump internal address */
			vce.vce_address.w.l = (vce.vce_address.w.l + 1) & 0x01FF;
			break;
	}
}

uint8 PCE::vce_r(uint16 offset)
{
	int temp = 0xFF;
	switch(offset & 7)
	{
		case 0x04:	/* color table data (LSB) */
			temp = vce.vce_data[vce.vce_address.w.l].b.l;
			break;

		case 0x05:	/* color table data (MSB) */
			temp = vce.vce_data[vce.vce_address.w.l].b.h;
			temp |= 0xFE;
			vce.vce_address.w.l = (vce.vce_address.w.l + 1) & 0x01FF;
			break;
	}
	return (temp);
}


void PCE::pce_refresh_line(int line, uint8 *drawn, scrntype *line_buffer)
{
	static const int width_table[4] = {5, 6, 7, 7};

	int scroll_y = ( vdc.y_scroll & 0x01FF);
	int scroll_x = (vdc.vdc_data[BXR].w.l & 0x03FF);
	int nt_index;

	/* is virtual map 32 or 64 characters tall ? (256 or 512 pixels) */
	int v_line = (scroll_y) & (vdc.vdc_data[MWR].w.l & 0x0040 ? 0x1FF : 0x0FF);

	/* row within character */
	int v_row = (v_line & 7);

	/* row of characters in BAT */
	int nt_row = (v_line >> 3);

	/* virtual X size (# bits to shift) */
	int v_width =		width_table[(vdc.vdc_data[MWR].w.l >> 4) & 3];

	/* pointer to the name table (Background Attribute Table) in VRAM */
	uint8 *bat = &(vdc.vram[nt_row << (v_width+1)]);

	/* Are we in greyscale mode or in color mode? */
	scrntype *color_base = vce.palette + (vce.vce_control & 0x80 ? 512 : 0);

	int b0, b1, b2, b3;
	int i0, i1, i2, i3;
	int cell_pattern_index;
	int cell_palette;
	int x, c, i;

	/* character blanking bit */
	if(!(vdc.vdc_data[CR].w.l & CR_BB))
	{
		return;
	}
	else
	{
		int	pixel = 0;
		int phys_x = - ( scroll_x & 0x07 );

		for(i=0;i<(vdc.physical_width >> 3) + 1;i++)
		{
			nt_index = (i + (scroll_x >> 3)) & ((2 << (v_width-1))-1);
			nt_index *= 2;

			/* get name table data: */

			/* palette # = index from 0-15 */
			cell_palette = ( bat[nt_index + 1] >> 4 ) & 0x0F;

			/* This is the 'character number', from 0-0x0FFF         */
			/* then it is shifted left 4 bits to form a VRAM address */
			/* and one more bit to convert VRAM word offset to a     */
			/* byte-offset within the VRAM space                     */
			cell_pattern_index = ( ( ( bat[nt_index + 1] << 8 ) | bat[nt_index] ) & 0x0FFF) << 5;

			b0 = vram_read((cell_pattern_index) + (v_row << 1) + 0x00);
			b1 = vram_read((cell_pattern_index) + (v_row << 1) + 0x01);
			b2 = vram_read((cell_pattern_index) + (v_row << 1) + 0x10);
			b3 = vram_read((cell_pattern_index) + (v_row << 1) + 0x11);

			for(x=0;x<8;x++)
			{
				i0 = (b0 >> (7-x)) & 1;
				i1 = (b1 >> (7-x)) & 1;
				i2 = (b2 >> (7-x)) & 1;
				i3 = (b3 >> (7-x)) & 1;
				c = (cell_palette << 4 | i3 << 3 | i2 << 2 | i1 << 1 | i0);

				/* colour #0 always comes from palette #0 */
				if ( ! ( c & 0x0F ) )
					c &= 0x0F;

				if ( phys_x >= 0 && phys_x < vdc.physical_width )
				{
					drawn[ pixel ] = c ? 1 : 0;
					line_buffer[ pixel ] = color_base[vce.vce_data[c].w.l];
					pixel++;
//					if ( vdc.physical_width != 512 )
//					{
//						while ( pixel < ( ( ( phys_x + 1 ) * 512 ) / vdc.physical_width ) )
//						{
//							drawn[ pixel ] = c ? 1 : 0;
//							line_buffer[ pixel ] = color_base[vce.vce_data[c].w.l];
//							pixel++;
//						}
//					}
				}
				phys_x += 1;
			}
		}
	}
}

void PCE::conv_obj(int i, int l, int hf, int vf, char *buf)
{
	int b0, b1, b2, b3, i0, i1, i2, i3, x;
	int xi;
	int tmp;

	l &= 0x0F;
	if(vf) l = (15 - l);

	tmp = l + ( i << 5);

	b0 = vram_read((tmp + 0x00)<<1);
	b0 |= vram_read(((tmp + 0x00)<<1)+1)<<8;
	b1 = vram_read((tmp + 0x10)<<1);
	b1 |= vram_read(((tmp + 0x10)<<1)+1)<<8;
	b2 = vram_read((tmp + 0x20)<<1);
	b2 |= vram_read(((tmp + 0x20)<<1)+1)<<8;
	b3 = vram_read((tmp + 0x30)<<1);
	b3 |= vram_read(((tmp + 0x30)<<1)+1)<<8;

	for(x=0;x<16;x++)
	{
		if(hf) xi = x; else xi = (15 - x);
		i0 = (b0 >> xi) & 1;
		i1 = (b1 >> xi) & 1;
		i2 = (b2 >> xi) & 1;
		i3 = (b3 >> xi) & 1;
		buf[x] = (i3 << 3 | i2 << 2 | i1 << 1 | i0);
	}
}

void PCE::pce_refresh_sprites(int line, uint8 *drawn, scrntype *line_buffer)
{
	int i;
	uint8 sprites_drawn = 0;

	/* Are we in greyscale mode or in color mode? */
	scrntype *color_base = vce.palette + (vce.vce_control & 0x80 ? 512 : 0);

	/* count up: Highest priority is Sprite 0 */
	for(i = 0; i < 64; i++)
	{
		static const int cgy_table[] = {16, 32, 64, 64};

		int obj_y = (vdc.sprite_ram[(i << 2) + 0] & 0x03FF) - 64;
		int obj_x = (vdc.sprite_ram[(i << 2) + 1] & 0x03FF) - 32;
		int obj_i = (vdc.sprite_ram[(i << 2) + 2] & 0x07FE);
		int obj_a = (vdc.sprite_ram[(i << 2) + 3]);
		int cgx   = (obj_a >> 8) & 1;   /* sprite width */
		int cgy   = (obj_a >> 12) & 3;  /* sprite height */
		int hf    = (obj_a >> 11) & 1;  /* horizontal flip */
		int vf    = (obj_a >> 15) & 1;  /* vertical flip */
		int palette = (obj_a & 0x000F);
		int priority = (obj_a >> 7) & 1;
		int obj_h = cgy_table[cgy];
		int obj_l = (line - obj_y);
		int cgypos;
		char buf[16];

		if ((obj_y == -64) || (obj_y > line)) continue;
		if ((obj_x == -32) || (obj_x >= vdc.physical_width)) continue;

		/* no need to draw an object that's ABOVE where we are. */
		if((obj_y + obj_h) < line) continue;

		/* If CGX is set, bit 0 of sprite pattern index is forced to 0 */
		if ( cgx )
			obj_i &= ~2;

		/* If CGY is set to 1, bit 1 of the sprite pattern index is forced to 0. */
		if ( cgy & 1 )
			obj_i &= ~4;

		/* If CGY is set to 2 or 3, bit 1 and 2 of the sprite pattern index are forced to 0. */
		if ( cgy & 2 )
			obj_i &= ~12;

		if (obj_l < obj_h)
		{

			sprites_drawn++;
			if(sprites_drawn > 16)
			{
				if(vdc.vdc_data[CR].w.l & CR_OV)
				{
					/* note: flag is set only if irq is taken, Mizubaku Daibouken relies on this behaviour */
					vdc.status |= VDC_OR;
					d_cpu->write_signal(INPUT_LINE_IRQ1, ASSERT_LINE, 0);
				}
				continue;  /* Should cause an interrupt */
			}

			cgypos = (obj_l >> 4);
			if(vf) cgypos = ((obj_h - 1) >> 4) - cgypos;

			if(cgx == 0)
			{
				int x;
				int pixel_x = obj_x;//( ( obj_x * 512 ) / vdc.physical_width );

				conv_obj(obj_i + (cgypos << 2), obj_l, hf, vf, buf);

				for(x = 0; x < 16; x++)
				{
					if(((obj_x + x) < (vdc.physical_width)) && ((obj_x + x) >= 0))
					{
						if ( buf[x] )
						{
							if( drawn[pixel_x] < 2 )
							{
								if( priority || drawn[pixel_x] == 0 )
								{
									line_buffer[pixel_x] = color_base[vce.vce_data[0x100 + (palette << 4) + buf[x]].w.l];
//									if ( vdc.physical_width != 512 )
//									{
//										int dp = 1;
//										while ( pixel_x + dp < ( ( ( obj_x + x + 1 ) * 512 ) / vdc.physical_width ) )
//										{
//											drawn[pixel_x + dp] = i + 2;
//											line_buffer[pixel_x + dp] = color_base[vce.vce_data[0x100 + (palette << 4) + buf[x]].w.l];
//											dp++;
//										}
//									}
								}
								drawn[pixel_x] = i + 2;
							}
							/* Check for sprite #0 collision */
							else if (drawn[pixel_x] == 2)
							{
								if(vdc.vdc_data[CR].w.l & CR_CC)
									d_cpu->write_signal(INPUT_LINE_IRQ1, ASSERT_LINE, 0);
								vdc.status |= VDC_CR;
							}
						}
					}
//					if ( vdc.physical_width != 512 )
//					{
//						pixel_x = ( ( obj_x + x + 1 ) * 512 ) / vdc.physical_width;
//					}
//					else
//					{
						pixel_x += 1;
//					}
				}
			}
			else
			{
				int x;
				int pixel_x = obj_x;//( ( obj_x * 512 ) / vdc.physical_width );

				conv_obj(obj_i + (cgypos << 2) + (hf ? 2 : 0), obj_l, hf, vf, buf);

				for(x = 0; x < 16; x++)
				{
					if(((obj_x + x) < (vdc.physical_width)) && ((obj_x + x) >= 0))
					{
						if ( buf[x] )
						{
							if( drawn[pixel_x] < 2 )
							{
								if ( priority || drawn[pixel_x] == 0 )
								{
									line_buffer[pixel_x] = color_base[vce.vce_data[0x100 + (palette << 4) + buf[x]].w.l];
//									if ( vdc.physical_width != 512 )
//									{
//										int dp = 1;
//										while ( pixel_x + dp < ( ( ( obj_x + x + 1 ) * 512 ) / vdc.physical_width ) )
//										{
//											drawn[pixel_x + dp] = i + 2;
//											line_buffer[pixel_x + dp] = color_base[vce.vce_data[0x100 + (palette << 4) + buf[x]].w.l];
//											dp++;
//										}
//									}
								}
								drawn[pixel_x] = i + 2;
							}
							/* Check for sprite #0 collision */
							else if ( drawn[pixel_x] == 2 )
							{
								if(vdc.vdc_data[CR].w.l & CR_CC)
									d_cpu->write_signal(INPUT_LINE_IRQ1, ASSERT_LINE, 0);
								vdc.status |= VDC_CR;
							}
						}
					}
//					if ( vdc.physical_width != 512 )
//					{
//						pixel_x = ( ( obj_x + x + 1 ) * 512 ) / vdc.physical_width;
//					}
//					else
//					{
						pixel_x += 1;
//					}
				}

				/* 32 pixel wide sprites are counted as 2 sprites and the right half
				   is only drawn if there are 2 open slots.
				*/
				sprites_drawn++;
				if( sprites_drawn > 16 )
				{
					if(vdc.vdc_data[CR].w.l&CR_OV)
					{
						/* note: flag is set only if irq is taken, Mizubaku Daibouken relies on this behaviour */
						vdc.status |= VDC_OR;
						d_cpu->write_signal(INPUT_LINE_IRQ1, ASSERT_LINE, 0);
					}
				}
				else
				{
					conv_obj(obj_i + (cgypos << 2) + (hf ? 0 : 2), obj_l, hf, vf, buf);
					for(x = 0; x < 16; x++)
					{
						if(((obj_x + 0x10 + x) < (vdc.physical_width)) && ((obj_x + 0x10 + x) >= 0))
						{
							if ( buf[x] )
							{
								if( drawn[pixel_x] < 2 )
								{
									if( priority || drawn[pixel_x] == 0 )
									{
										line_buffer[pixel_x] = color_base[vce.vce_data[0x100 + (palette << 4) + buf[x]].w.l];
//										if ( vdc.physical_width != 512 )
//										{
//											int dp = 1;
//											while ( pixel_x + dp < ( ( ( obj_x + x + 17 ) * 512 ) / vdc.physical_width ) )
//											{
//												drawn[pixel_x + dp] = i + 2;
//												line_buffer[pixel_x + dp] = color_base[vce.vce_data[0x100 + (palette << 4) + buf[x]].w.l];
//												dp++;
//											}
//										}
									}
									drawn[pixel_x] = i + 2;
								}
								/* Check for sprite #0 collision */
								else if ( drawn[pixel_x] == 2 )
								{
									if(vdc.vdc_data[CR].w.l & CR_CC)
										d_cpu->write_signal(INPUT_LINE_IRQ1, ASSERT_LINE, 0);
									vdc.status |= VDC_CR;
								}
							}
						}
//						if ( vdc.physical_width != 512 )
//						{
//							pixel_x = ( ( obj_x + x + 17 ) * 512 ) / vdc.physical_width;
//						}
//						else
//						{
							pixel_x += 1;
//						}
					}
				}
			}
		}
	}
}

void PCE::vdc_do_dma()
{
	int src = vdc.vdc_data[SOUR].w.l;
	int dst = vdc.vdc_data[DESR].w.l;
	int len = vdc.vdc_data[LENR].w.l;

	int did = (vdc.vdc_data[DCR].w.l >> 3) & 1;
	int sid = (vdc.vdc_data[DCR].w.l >> 2) & 1;
	int dvc = (vdc.vdc_data[DCR].w.l >> 1) & 1;

	do {
		uint8 l, h;

		l = vram_read(src<<1);
		h = vram_read((src<<1) + 1);

		vram_write(dst<<1,l);
		vram_write(1+(dst<<1),h);

		if(sid) src = (src - 1) & 0xFFFF;
		else	src = (src + 1) & 0xFFFF;

		if(did) dst = (dst - 1) & 0xFFFF;
		else	dst = (dst + 1) & 0xFFFF;

		len = (len - 1) & 0xFFFF;

	} while (len != 0xFFFF);

	vdc.status |= VDC_DV;
	vdc.vdc_data[SOUR].w.l = src;
	vdc.vdc_data[DESR].w.l = dst;
	vdc.vdc_data[LENR].w.l = len;
	if(dvc)
	{
		d_cpu->write_signal(INPUT_LINE_IRQ1, ASSERT_LINE, 0);
	}

}

// psg

void PCE::psg_reset()
{
	memset(psg, 0, sizeof(psg));
	for (int i = 0; i < 6; i++) {
		psg[i].regs[4] = 0x80;
	}
	psg[4].randval = psg[5].randval = 0x51f631e4;
	
	psg_ch = 0;
	psg_vol = psg_lfo_freq = psg_lfo_ctrl = 0;
}

void PCE::psg_write(uint16 addr, uint8 data)
{
	switch(addr & 0x1f) {
	case 0:
		psg_ch = data & 7;
		break;
	case 1:
		psg_vol = data;
		break;
	case 2:
		psg[psg_ch].regs[2] = data;
		break;
	case 3:
//		psg[psg_ch].regs[3] = data & 0x1f;
		psg[psg_ch].regs[3] = data & 0xf;
		break;
	case 4:
		psg[psg_ch].regs[4] = data;
		break;
	case 5:
		psg[psg_ch].regs[5] = data;
		break;
	case 6:
		if(psg[psg_ch].regs[4] & 0x40) {
			psg[psg_ch].wav[0] =data & 0x1f;
		}
		else {
			psg[psg_ch].wav[psg[psg_ch].wavptr] = data & 0x1f;
			psg[psg_ch].wavptr = (psg[psg_ch].wavptr + 1) & 0x1f;
		}
		break;
	case 7:
		psg[psg_ch].regs[7] = data;
		break;
	case 8:
		psg_lfo_freq = data;
		break;
	case 9:
		psg_lfo_ctrl = data;
		break;
	}
}

uint8 PCE::psg_read(uint16 addr)
{
	int ptr;
	
	switch(addr & 0x1f) {
	case 0:
		return psg_ch;
	case 1:
		return psg_vol;
	case 2:
		return psg[psg_ch].regs[2];
	case 3:
		return psg[psg_ch].regs[3];
	case 4:
		return psg[psg_ch].regs[4];
	case 5:
		return psg[psg_ch].regs[5];
	case 6:
		ptr = psg[psg_ch].wavptr;
		psg[psg_ch].wavptr = (psg[psg_ch].wavptr + 1) & 0x1f;
		return psg[psg_ch].wav[ptr];
	case 7:
		return psg[psg_ch].regs[7];
	case 8:
		return psg_lfo_freq;
	case 9:
		return psg_lfo_ctrl;
	}
	return 0xff;
}

void PCE::mix(int32* buffer, int cnt)
{
	int vol_tbl[32] = {
		 100, 451, 508, 573, 646, 728, 821, 925,1043,1175,1325, 1493, 1683, 1898, 2139, 2411,
		2718,3064,3454,3893,4388,4947,5576,6285,7085,7986,9002,10148,11439,12894,14535,16384
	};
	
	for(int ch = 0; ch < 6; ch++) {
		if(!(psg[ch].regs[4] & 0x80)) {
			// mute
			psg[ch].genptr = psg[ch].remain = 0;
		}
		else if(psg[ch].regs[4] & 0x40) {
			// dda
			int32 wav = ((int32)psg[ch].wav[0] - 16) * 702;
			int32 vol = max((psg_vol >> 3) & 0x1e, (psg_vol << 1) & 0x1e) + (psg[ch].regs[4] & 0x1f) + max((psg[ch].regs[5] >> 3) & 0x1e, (psg[ch].regs[5] << 1) & 0x1e) - 60;
			vol = (vol < 0) ? 0 : (vol > 31) ? 31 : vol;
			vol = wav * vol_tbl[vol] / 16384;
			for(int i = 0, j = 0; i < cnt; i++, j += 2) {
				buffer[j    ] += vol; // L
				buffer[j + 1] += vol; // R
			}
		}
		else if(ch >= 4 && (psg[ch].regs[7] & 0x80)) {
			// noise
			uint16 freq = (psg[ch].regs[7] & 0x1f);
			int32 vol = max((psg_vol >> 3) & 0x1e, (psg_vol << 1) & 0x1e) + (psg[ch].regs[4] & 0x1f) + max((psg[ch].regs[5] >> 3) & 0x1e, (psg[ch].regs[5] << 1) & 0x1e) - 60;
			vol = (vol < 0) ? 0 : (vol > 31) ? 31 : vol;
			vol = vol_tbl[vol];
			for(int i = 0, j = 0; i < cnt; i++, j += 2) {
				psg[ch].remain += 3000 + freq * 512;
				uint32 t = psg[ch].remain / sample_rate;
				if(t >= 1) {
					if(psg[ch].randval & 0x80000) {
						psg[ch].randval = ((psg[ch].randval ^ 4) << 1) + 1;
						psg[ch].noise = true;
					}
					else {
						psg[ch].randval <<= 1;
						psg[ch].noise = false;
					}
					psg[ch].remain -= sample_rate * t;
				}
				int32 outvol = (int32)((psg[ch].noise ? 10 * 702 : -10 * 702) * vol / 16384);
				buffer[j    ] += outvol; // L
				buffer[j + 1] += outvol; // R
			}
		}
		else {
			int32 wav[32];
			for(int i = 0; i < 32; i++) {
				wav[i] = ((int32)psg[ch].wav[i] - 16) * 702;
			}
			uint32 freq = psg[ch].regs[2] + ((uint32)psg[ch].regs[3] << 8);
			if(freq) {
				int32 vol = max((psg_vol >> 3) & 0x1e, (psg_vol << 1) & 0x1e) + (psg[ch].regs[4] & 0x1f) + max((psg[ch].regs[5] >> 3) & 0x1e, (psg[ch].regs[5] << 1) & 0x1e) - 60;
				vol = (vol < 0) ? 0 : (vol > 31) ? 31 : vol;
				vol = vol_tbl[vol];
				for(int i = 0, j = 0; i < cnt; i++, j += 2) {
					int32 outvol = wav[psg[ch].genptr] * vol / 16384;
					buffer[j    ] += outvol; // L
					buffer[j + 1] += outvol; // R
					psg[ch].remain += 32 * 1118608 / freq;
					uint32 t = psg[ch].remain / (10 * sample_rate);
					psg[ch].genptr = (psg[ch].genptr + t) & 0x1f;
					psg[ch].remain -= 10 * sample_rate * t;
				}
			}
		}
	}
}

// joypad (non multipad)

void PCE::joy_reset()
{
	joy_count = joy_nibble = joy_second = 0;
}

void PCE::joy_write(uint16 addr, uint8 data)
{
	joy_nibble = data & 1;
	if(data & 2) {
		// clear flag
		joy_count = 0;
		joy_nibble = 0;
		joy_second ^= 1;
	}
}

uint8 PCE::joy_read(uint16 addr)
{
	uint8 val = 0xf;
	uint16 stat = 0;
	
	if(!joy_count) {
		return 0;
	}
	if(joy_count > 5) {
		return 0xf;
	}
	if(joy_count == 1) {
		stat = joy_stat[0];
		if(key_stat[0x26]) stat |= 0x001;	// up
		if(key_stat[0x28]) stat |= 0x002;	// down
		if(key_stat[0x25]) stat |= 0x004;	// left
		if(key_stat[0x27]) stat |= 0x008;	// right
		if(key_stat[0x44]) stat |= 0x010;	// d (1)
		if(key_stat[0x53]) stat |= 0x020;	// s (2)
		if(key_stat[0x20]) stat |= 0x040;	// space (select)
		if(key_stat[0x0d]) stat |= 0x080;	// enter (run)
		if(key_stat[0x41]) stat |= 0x100;	// a (3)
		if(key_stat[0x51]) stat |= 0x200;	// q (4)
		if(key_stat[0x57]) stat |= 0x400;	// w (5)
		if(key_stat[0x45]) stat |= 0x800;	// e (6)
	}
	else if(joy_count == 2) {
		stat = joy_stat[1];
	}
	if(joy_second) {
		if(joy_nibble) {
			if(stat & 0x001) val &= ~1;	// up
			if(stat & 0x008) val &= ~2;	// right
			if(stat & 0x002) val &= ~4;	// down
			if(stat & 0x004) val &= ~8;	// left
		}
		else {
			if(stat & 0x010) val &= ~1;	// b1
			if(stat & 0x020) val &= ~2;	// b2
			if(stat & 0x040) val &= ~4;	// sel
			if(stat & 0x080) val &= ~8;	// run
		}
	}
	else {
		if(joy_nibble) {
			val = 0;
		}
		else {
			if(stat & 0x100) val &= ~1;	// b3
			if(stat & 0x200) val &= ~2;	// b4
			if(stat & 0x400) val &= ~4;	// b5
			if(stat & 0x800) val &= ~8;	// b6
		}
	}
	if(joy_count == 5) {
		joy_second = 0;
	}
	return val;
}

