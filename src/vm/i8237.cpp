/*
	Skelton for retropc emulator

	Origin : MESS
	Author : Takeda.Toshiya
	Date   : 2006.12.06 -

	[ i8237 ]
*/

#include "i8237.h"

void I8237::reset()
{
	low_high = false;
	cmd = req = tc = 0;
	mask = 0xff;
}

void I8237::write_io8(uint32_t addr, uint32_t data)
{
	int ch = (addr >> 1) & 3;
	uint8_t bit = 1 << (data & 3);
	
	switch(addr & 0x0f) {
	case 0x00: case 0x02: case 0x04: case 0x06:
		if(low_high) {
			dma[ch].bareg = (dma[ch].bareg & 0xff) | (data << 8);
		} else {
			dma[ch].bareg = (dma[ch].bareg & 0xff00) | data;
		}
		dma[ch].areg = dma[ch].bareg;
		low_high = !low_high;
		break;
	case 0x01: case 0x03: case 0x05: case 0x07:
		if(low_high) {
			dma[ch].bcreg = (dma[ch].bcreg & 0xff) | (data << 8);
		} else {
			dma[ch].bcreg = (dma[ch].bcreg & 0xff00) | data;
		}
		dma[ch].creg = dma[ch].bcreg;
		low_high = !low_high;
		break;
	case 0x08:
		// command register
		cmd = data;
		break;
	case 0x09:
		// request register
		if(data & 4) {
			if(!(req & bit)) {
				req |= bit;
#ifndef SINGLE_MODE_DMA
				do_dma();
#endif
			}
		} else {
			req &= ~bit;
		}
		break;
	case 0x0a:
		// single mask register
		if(data & 4) {
			mask |= bit;
		} else {
			mask &= ~bit;
		}
		break;
	case 0x0b:
		// mode register
		dma[data & 3].mode = data;
		break;
	case 0x0c:
		low_high = false;
		break;
	case 0x0d:
		// clear master
		reset();
		break;
	case 0x0e:
		// clear mask register
		mask = 0;
		break;
	case 0x0f:
		// all mask register
		mask = data & 0x0f;
		break;
	}
}

uint32_t I8237::read_io8(uint32_t addr)
{
	int ch = (addr >> 1) & 3;
	uint32_t val = 0xff;
	
	switch(addr & 0x0f) {
	case 0x00: case 0x02: case 0x04: case 0x06:
		if(low_high) {
			val = dma[ch].areg >> 8;
		} else {
			val = dma[ch].areg & 0xff;
		}
		low_high = !low_high;
		return val;
	case 0x01: case 0x03: case 0x05: case 0x07:
		if(low_high) {
			val = dma[ch].creg >> 8;
		} else {
			val = dma[ch].creg & 0xff;
		}
		low_high = !low_high;
		return val;
	case 0x08:
		// status register
		val = (req << 4) | tc;
		tc = 0;
		return val;
	case 0x0d:
		// temporary register
		return tmp & 0xff;
	}
	return 0xff;
}

void I8237::write_signal(int id, uint32_t data, uint32_t mask)
{
	if(SIG_I8237_CH0 <= id && id <= SIG_I8237_CH3) {
		int ch = id - SIG_I8237_CH0;
		uint8_t bit = 1 << ch;
		if(data & mask) {
			if(!(req & bit)) {
				write_signals(&dma[ch].outputs_tc, 0);
				req |= bit;
#ifndef SINGLE_MODE_DMA
				do_dma();
#endif
			}
		} else {
			req &= ~bit;
		}
	} else if(SIG_I8237_BANK0 <= id && id <= SIG_I8237_BANK3) {
		// external bank registers
		int ch = id - SIG_I8237_BANK0;
		dma[ch].bankreg &= ~mask;
		dma[ch].bankreg |= data & mask;
	} else if(SIG_I8237_MASK0 <= id && id <= SIG_I8237_MASK3) {
		// external bank registers
		int ch = id - SIG_I8237_MASK0;
		dma[ch].incmask &= ~mask;
		dma[ch].incmask |= data & mask;
	}
}

uint32_t I8237::read_signal(int id)
{
	if(SIG_I8237_BANK0 <= id && id <= SIG_I8237_BANK3) {
		// external bank registers
		int ch = id - SIG_I8237_BANK0;
		return dma[ch].bankreg;
	} else if(SIG_I8237_MASK0 <= id && id <= SIG_I8237_MASK3) {
		// external bank registers
		int ch = id - SIG_I8237_MASK0;
		return dma[ch].incmask;
	}
	return 0;
}

// note: if SINGLE_MODE_DMA is defined, do_dma() is called in every machine cycle

void I8237::do_dma()
{
	for(int ch = 0; ch < 4; ch++) {
		uint8_t bit = 1 << ch;
		if((req & bit) && !(mask & bit)) {
			// execute dma
			while(req & bit) {
				switch(dma[ch].mode & 0x0c) {
				case 0x00:
					// verify
					tmp = read_io(ch);
					break;
				case 0x04:
					// io -> memory
					tmp = read_io(ch);
					write_mem(dma[ch].areg | (dma[ch].bankreg << 16), tmp);
					break;
				case 0x08:
					// memory -> io
					tmp = read_mem(dma[ch].areg | (dma[ch].bankreg << 16));
					write_io(ch, tmp);
					break;
				}
				if(dma[ch].mode & 0x20) {
					dma[ch].areg--;
					if(dma[ch].areg == 0xffff) {
						dma[ch].bankreg = (dma[ch].bankreg & ~dma[ch].incmask) | ((dma[ch].bankreg - 1) & dma[ch].incmask);
					}
				} else {
					dma[ch].areg++;
					if(dma[ch].areg == 0) {
						dma[ch].bankreg = (dma[ch].bankreg & ~dma[ch].incmask) | ((dma[ch].bankreg + 1) & dma[ch].incmask);
					}
				}
				
				// check dma condition
				if(dma[ch].creg-- == 0) {
					// TC
					if(dma[ch].mode & 0x10) {
						// self initialize
						dma[ch].areg = dma[ch].bareg;
						dma[ch].creg = dma[ch].bcreg;
					} else {
						mask |= bit;
					}
					req &= ~bit;
					tc |= bit;
					write_signals(&dma[ch].outputs_tc, 0xffffffff);
#ifdef SINGLE_MODE_DMA
				} else if((dma[ch].mode & 0xc0) == 0x40) {
					// single mode
					break;
#endif
				}
			}
		}
	}
#ifdef SINGLE_MODE_DMA
	if(d_dma) {
		d_dma->do_dma();
	}
#endif
}

void I8237::write_mem(uint32_t addr, uint32_t data)
{
	if(mode_word) {
		d_mem->write_dma_data16((addr << 1) & addr_mask, data);
	} else {
		d_mem->write_dma_data8(addr & addr_mask, data);
	}
}

uint32_t I8237::read_mem(uint32_t addr)
{
	if(mode_word) {
		return d_mem->read_dma_data16((addr << 1) & addr_mask);
	} else {
		return d_mem->read_dma_data8(addr & addr_mask);
	}
}

void I8237::write_io(int ch, uint32_t data)
{
	if(mode_word) {
		dma[ch].dev->write_dma_io16(0, data);
	} else {
		dma[ch].dev->write_dma_io8(0, data);
	}
}

uint32_t I8237::read_io(int ch)
{
	if(mode_word) {
		return dma[ch].dev->read_dma_io16(0);
	} else {
		return dma[ch].dev->read_dma_io8(0);
	}
}

#define STATE_VERSION	2

bool I8237::process_state(FILEIO* state_fio, bool loading)
{
	if(!state_fio->StateCheckUint32(STATE_VERSION)) {
		return false;
	}
	if(!state_fio->StateCheckInt32(this_device_id)) {
		return false;
	}
	for(int i = 0; i < 4; i++) {
		state_fio->StateValue(dma[i].areg);
		state_fio->StateValue(dma[i].creg);
		state_fio->StateValue(dma[i].bareg);
		state_fio->StateValue(dma[i].bcreg);
		state_fio->StateValue(dma[i].mode);
		state_fio->StateValue(dma[i].bankreg);
		state_fio->StateValue(dma[i].incmask);
	}
	state_fio->StateValue(low_high);
	state_fio->StateValue(cmd);
	state_fio->StateValue(req);
	state_fio->StateValue(mask);
	state_fio->StateValue(tc);
	state_fio->StateValue(tmp);
	state_fio->StateValue(mode_word);
	state_fio->StateValue(addr_mask);
	return true;
}

