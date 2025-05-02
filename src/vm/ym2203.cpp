/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.15-

	[ YM2203 ]
*/

#include "ym2203.h"

void YM2203::initialize()
{
	opn = new FM::OPN;
	register_vline_event(this);
	mute = false;
#ifndef HAS_AY_3_8912
	irq = false;
#endif
}

void YM2203::release()
{
	delete opn;
}

void YM2203::reset()
{
	opn->Reset();
	opn->SetReg(0x27, 0);
#if defined(_X1TWIN) || defined(_X1TURBO)
	opn->SetReg(0x2e, 0);
#endif
	port[0].first = port[1].first = true;
}

void YM2203::write_io8(uint32 addr, uint32 data)
{
	if(addr & 1) {
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
			opn->SetReg(ch, data);
#ifndef HAS_AY_3_8912
			update_interrupt();
#endif
		}
	}
	else {
#ifdef HAS_AY_3_8912
		ch = data & 0x0f;
#else
		ch = data;
		// write dummy data for prescaler
		if(0x2d <= ch && ch <= 0x2f) {
			opn->SetReg(ch, 0);
		}
#endif
	}
}

uint32 YM2203::read_io8(uint32 addr)
{
	if(addr & 1) {
		if(ch == 14) {
			return (mode & 0x40) ? port[0].wreg : port[0].rreg;
		}
		else if(ch == 15) {
			return (mode & 0x80) ? port[1].wreg : port[1].rreg;
		}
		else {
			return opn->GetReg(ch);
		}
	}
	else {
#ifndef HAS_AY_3_8912
		return opn->ReadStatus();
#else
		return 0xff;
#endif
	}
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
	opn->Count(usec);
#ifndef HAS_AY_3_8912
	update_interrupt();
#endif
}

#ifndef HAS_AY_3_8912
void YM2203::update_interrupt()
{
	bool next = opn->ReadIRQ();
	if(irq != next) {
		write_signals(&outputs_irq, next ? 0xffffffff : 0);
		irq = next;
	}
}
#endif

void YM2203::mix(int32* buffer, int cnt)
{
	if(cnt > 0 && !mute) {
		opn->Mix(buffer, cnt);
	}
}

void YM2203::init(int rate, int clock, int samples, int volf, int volp)
{
	opn->Init(clock, rate, false, NULL);
	opn->SetVolumeFM(volf);
	opn->SetVolumePSG(volp);
}

void YM2203::update_timing(double frames_per_sec, double lines_per_frame)
{
	usec = (int)(1000000. / frames_per_sec / lines_per_frame + 0.5);
}

