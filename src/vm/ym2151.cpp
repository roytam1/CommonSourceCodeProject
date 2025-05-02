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
	irq = mute = false;
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
	opm->Count(usec);
	update_interrupt();
}

void YM2151::update_interrupt()
{
	bool next = opm->ReadIRQ();
	if(irq != next) {
		write_signals(&outputs_irq, next ? 0xffffffff : 0);
		irq = next;
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
}

void YM2151::update_timing(int clocks, double frames_per_sec, double lines_per_frame)
{
	usec = (int)(1000000. / frames_per_sec / lines_per_frame + 0.5);
}

