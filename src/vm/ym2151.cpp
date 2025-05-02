/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2009.03.08-

	[ YM2151 ]
*/

#include "ym2151.h"

void YM2151::initialize()
{
	opm = new FM::OPM;
	register_vline_event(this);
	mute = false;
	clock_prev = clock_accum = 0;
}

void YM2151::release()
{
	delete opm;
}

void YM2151::reset()
{
	opm->Reset();
}

void YM2151::write_io8(uint32 addr, uint32 data)
{
	if(addr & 1) {
		update_count();
		opm->SetReg(ch, data);
		update_interrupt();
	}
	else {
		ch = data;
	}
}

uint32 YM2151::read_io8(uint32 addr)
{
	if(addr & 1) {
		update_count();
		update_interrupt();
		return opm->ReadStatus();
	}
	return 0xff;
}

void YM2151::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_YM2151_MUTE) {
		mute = ((data & mask) != 0);
	}
}

void YM2151::event_vline(int v, int clock)
{
	update_count();
	update_interrupt();
}

void YM2151::update_count()
{
	clock_accum += clock_const * passed_clock(clock_prev);
	uint32 count = clock_accum >> 20;
	if(count) {
		opm->Count(count);
		clock_accum -= count << 20;
	}
	clock_prev = current_clock();
}

void YM2151::update_interrupt()
{
	if(opm->ReadIRQ()) {
		write_signals(&outputs_irq, 0);
		write_signals(&outputs_irq, 0xffffffff);
	}
}

void YM2151::mix(int32* buffer, int cnt)
{
	if(cnt > 0 && !mute) {
		opm->Mix(buffer, cnt);
	}
}

void YM2151::init(int rate, int clock, int samples, int vol)
{
	opm->Init(clock, rate, false);
	opm->SetVolume(vol);
	
	chip_clock = clock;
}

void YM2151::SetReg(uint addr, uint data)
{
	opm->SetReg(addr, data);
}

void YM2151::update_timing(int new_clocks, double new_frames_per_sec, int new_lines_per_frame)
{
	clock_const = (uint32)((double)chip_clock * 1024.0 * 1024.0 / (double)new_clocks + 0.5);
}

