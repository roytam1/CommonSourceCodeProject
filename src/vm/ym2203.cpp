/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.15-

	[ AY-3-8910 / YM2203 / YM2608 ]
*/

#include "ym2203.h"

void YM2203::initialize()
{
#ifdef HAS_YM2608
	chip = new FM::OPNA;
#else
	chip = new FM::OPN;
#endif
#ifdef SUPPORT_MAME_FM_DLL
	fmdll = new CFMDLL(_T("mamefm.dll"));
	dllchip = NULL;
#endif
	register_vline_event(this);
	mute = false;
	clock_prev = clock_accum = clock_busy = 0;
}

void YM2203::release()
{
	delete chip;
#ifdef SUPPORT_MAME_FM_DLL
	if(dllchip) {
		fmdll->Release(dllchip);
	}
	delete fmdll;
#endif
}

void YM2203::reset()
{
	chip->Reset();
#ifdef SUPPORT_MAME_FM_DLL
	if(dllchip) {
		fmdll->Reset(dllchip);
	}
#endif
	this->SetReg(0x27, 0); // stop timer
	
	port[0].first = port[1].first = true;
	irq_prev = busy = false;
}

#ifdef HAS_YM2608
#define amask 3
#else
#define amask 1
#endif

void YM2203::write_io8(uint32 addr, uint32 data)
{
	switch(addr & amask) {
	case 0:
#ifdef HAS_YM_SERIES
		ch = data;
		// write dummy data for prescaler
		if(0x2d <= ch && ch <= 0x2f) {
			update_count();
			this->SetReg(ch, 0);
			update_interrupt();
			clock_busy = current_clock();
			busy = true;
		}
#else
		ch = data & 0x0f;
#endif
		break;
	case 1:
		if(ch == 7) {
			mode = data;
		}
#ifdef SUPPORT_YM2203_PORT_A
		else if(ch == 14) {
			if(port[0].wreg != data || port[0].first) {
				write_signals(&port[0].outputs, data);
				port[0].wreg = data;
				port[0].first = false;
			}
		}
#endif
#ifdef SUPPORT_YM2203_PORT_B
		else if(ch == 15) {
			if(port[1].wreg != data || port[1].first) {
				write_signals(&port[1].outputs, data);
				port[1].wreg = data;
				port[1].first = false;
			}
		}
#endif
		// don't write again for prescaler
		if(!(0x2d <= ch && ch <= 0x2f)) {
			update_count();
			this->SetReg(ch, data);
#ifdef HAS_YM_SERIES
			update_interrupt();
			clock_busy = current_clock();
			busy = true;
#endif
		}
		break;
#ifdef HAS_YM2608
	case 2:
		ch1 = data1 = data;
		break;
	case 3:
		update_count();
		this->SetReg(0x100 | ch1, data);
		data1 = data;
		update_interrupt();
		break;
#endif
	}
}

uint32 YM2203::read_io8(uint32 addr)
{
	switch(addr & amask) {
#ifdef HAS_YM_SERIES
	case 0:
		{
			/* BUSY : x : x : x : x : x : FLAGB : FLAGA */
			update_count();
			update_interrupt();
			uint32 status = chip->ReadStatus() & ~0x80;
			if(busy) {
				// FIXME: we need to investigate the correct busy period
				if(passed_usec(clock_busy) < 8) {
					status |= 0x80;
				}
				busy = false;
			}
			return status;
		}
#endif
	case 1:
#ifdef SUPPORT_YM2203_PORT_A
		if(ch == 14) {
			return (mode & 0x40) ? port[0].wreg : port[0].rreg;
		}
#endif
#ifdef SUPPORT_YM2203_PORT_B
		else if(ch == 15) {
			return (mode & 0x80) ? port[1].wreg : port[1].rreg;
		}
#endif
		return chip->GetReg(ch);
#ifdef HAS_YM2608
	case 2:
		{
			/* BUSY : x : PCMBUSY : ZERO : BRDY : EOS : FLAGB : FLAGA */
			update_count();
			update_interrupt();
			uint32 status = chip->ReadStatusEx() & ~0x80;
			if(busy) {
				// FIXME: we need to investigate the correct busy period
				if(passed_usec(clock_busy) < 8) {
					status |= 0x80;
				}
				busy = false;
			}
			return status;
		}
	case 3:
		if(ch1 == 8) {
			return chip->GetReg(0x100 | ch1);
		}
//		else if(ch1 == 0x0f) {
//			return 0x80; // from mame fm.c
//		}
		return data1;
#endif
	}
	return 0xff;
}

void YM2203::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_YM2203_MUTE) {
		mute = ((data & mask) != 0);
	}
#ifdef SUPPORT_YM2203_PORT_A
	else if(id == SIG_YM2203_PORT_A) {
		port[0].rreg = (port[0].rreg & ~mask) | (data & mask);
	}
#endif
#ifdef SUPPORT_YM2203_PORT_B
	else if(id == SIG_YM2203_PORT_B) {
		port[1].rreg = (port[1].rreg & ~mask) | (data & mask);
	}
#endif
}

void YM2203::event_vline(int v, int clock)
{
	update_count();
#ifdef HAS_YM_SERIES
	update_interrupt();
#endif
}

void YM2203::update_count()
{
	clock_accum += clock_const * passed_clock(clock_prev);
	uint32 count = clock_accum >> 20;
	if(count) {
		chip->Count(count);
		clock_accum -= count << 20;
	}
	clock_prev = current_clock();
}

#ifdef HAS_YM_SERIES
void YM2203::update_interrupt()
{
	bool irq = chip->ReadIRQ();
	if(!irq_prev && irq) {
		write_signals(&outputs_irq, 0xffffffff);
	}
	else if(irq_prev && !irq) {
		write_signals(&outputs_irq, 0);
	}
	irq_prev = irq;
}
#endif

void YM2203::mix(int32* buffer, int cnt)
{
	if(cnt > 0 && !mute) {
		chip->Mix(buffer, cnt);
#ifdef SUPPORT_MAME_FM_DLL
		if(dllchip) {
			fmdll->Mix(dllchip, buffer, cnt);
		}
#endif
	}
}

void YM2203::init(int rate, int clock, int samples, int volf, int volp)
{
#ifdef HAS_YM2608
	chip->Init(clock, rate, false, emu->application_path());
#else
	chip->Init(clock, rate, false, NULL);
#endif
	chip->SetVolumeFM(volf);
	chip->SetVolumePSG(volp);
	
#ifdef SUPPORT_MAME_FM_DLL
#ifdef HAS_YM2608
	fmdll->Create((LPVOID*)&dllchip, clock, rate);
#else
	fmdll->Create((LPVOID*)&dllchip, clock * 2, rate);
#endif
	if(dllchip) {
		fmdll->SetVolumeFM(dllchip, volf);
//		fmdll->SetVolumePSG(dllchip, volp);
		chip->SetChannelMask(0x3f);
		fmdll->SetChannelMask(dllchip, ~0x3f);
	}
#endif
	chip_clock = clock;
}

void YM2203::SetReg(uint addr, uint data)
{
	chip->SetReg(addr, data);
#ifdef SUPPORT_MAME_FM_DLL
	if(dllchip) {
		fmdll->SetReg(dllchip, addr, data);
	}
#endif
}

void YM2203::update_timing(int new_clocks, double new_frames_per_sec, int new_lines_per_frame)
{
#ifdef HAS_YM2608
	clock_const = (uint32)((double)chip_clock * 1024.0 * 1024.0 / (double)new_clocks / 2.0 + 0.5);
#else
	clock_const = (uint32)((double)chip_clock * 1024.0 * 1024.0 / (double)new_clocks + 0.5);
#endif
}

