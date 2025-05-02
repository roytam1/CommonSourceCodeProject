/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.15-

	[ AY-3-8912 / YM2203 / YM2608 ]
*/

#include "ym2203.h"

void YM2203::initialize()
{
#ifdef HAS_YM2608
	chip = new FM::OPNA;
#else
	chip = new FM::OPN;
#endif
	register_vline_event(this);
	mute = false;
}

void YM2203::release()
{
	delete chip;
}

void YM2203::reset()
{
	chip->Reset();
	chip->SetReg(0x27, 0);
#if defined(_X1TWIN) || defined(_X1TURBO)
	chip->SetReg(0x2e, 0);
#endif
	port[0].first = port[1].first = true;
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
#ifdef HAS_AY_3_8912
		ch = data & 0x0f;
#else
		ch = data;
		// write dummy data for prescaler
		if(0x2d <= ch && ch <= 0x2f) {
			chip->SetReg(ch, 0);
		}
#endif
		break;
	case 1:
		if(ch == 7) {
			mode = data;
		}
		else if(ch == 14 || ch == 15) {
			int p = ch - 14;
			if(port[p].wreg != data || port[p].first) {
				write_signals(&port[p].outputs, data);
				port[p].wreg = data;
				port[p].first = false;
			}
		}
		// don't write again for prescaler
		if(!(0x2d <= ch && ch <= 0x2f)) {
			chip->SetReg(ch, data);
#ifndef HAS_AY_3_8912
			update_interrupt();
#endif
		}
		break;
#ifdef HAS_YM2608
	case 2:
		ch1 = data1 = data;
		break;
	case 3:
		chip->SetReg(0x100 | ch1, data);
		data1 = data;
		update_interrupt();
		break;
#endif
	}
}

uint32 YM2203::read_io8(uint32 addr)
{
	switch(addr & amask) {
#ifndef HAS_AY_3_8912
	case 0:
		return chip->ReadStatus();
#endif
	case 1:
		if(ch == 14) {
#if defined(_PC98DO) || defined(_PC8801MA)
			return port[0].rreg;
#else
			return (mode & 0x40) ? port[0].wreg : port[0].rreg;
#endif
		}
		else if(ch == 15) {
#if defined(_PC98DO) || defined(_PC8801MA)
			return port[1].rreg;
#else
			return (mode & 0x80) ? port[1].wreg : port[1].rreg;
#endif
		}
		return chip->GetReg(ch);
#ifdef HAS_YM2608
	case 2:
		return chip->ReadStatusEx();
	case 3:
		if(ch1 == 8) {
			return chip->GetReg(0x100 | ch1);
		}
		return data1;
#endif
	}
	return 0xff;
}

void YM2203::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_YM2203_PORT_A) {
		port[0].rreg = (port[0].rreg & ~mask) | (data & mask);
	}
	else if(id == SIG_YM2203_PORT_B) {
		port[1].rreg = (port[1].rreg & ~mask) | (data & mask);
	}
	else if(id == SIG_YM2203_MUTE) {
		mute = ((data & mask) != 0);
	}
}

void YM2203::event_vline(int v, int clock)
{
	chip->Count(usec_per_vline);
#ifndef HAS_AY_3_8912
	update_interrupt();
#endif
}

#ifndef HAS_AY_3_8912
void YM2203::update_interrupt()
{
	if(chip->ReadIRQ()) {
		write_signals(&outputs_irq, 0);
		write_signals(&outputs_irq, 0xffffffff);
	}
}
#endif

void YM2203::mix(int32* buffer, int cnt)
{
	if(cnt > 0 && !mute) {
		chip->Mix(buffer, cnt);
	}
}

void YM2203::init(int rate, int clock, int samples, int volf, int volp)
{
#ifdef HAS_YM2608
	_TCHAR app_path[_MAX_PATH];
	emu->application_path(app_path);
	chip->Init(clock, rate, false, app_path);
#else
	chip->Init(clock, rate, false, NULL);
#endif
	chip->SetVolumeFM(volf);
	chip->SetVolumePSG(volp);
}

void YM2203::update_timing(int new_clocks, double new_frames_per_sec, int new_lines_per_frame)
{
	usec_per_vline = (int)(1000000.0 / new_frames_per_sec / (double)new_lines_per_frame + 0.5);
}

