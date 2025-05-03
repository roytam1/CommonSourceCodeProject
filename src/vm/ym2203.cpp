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
	usec = (int)(1000000. / FRAMES_PER_SEC / LINES_PER_FRAME + 0.5);
	vm->regist_vsync_event(this);
	mute = false;
}

void YM2203::release()
{
	delete opn;
	free(sound_tmp);
}

void YM2203::reset()
{
	opn->Reset();
	opn->SetReg(0x27, 0);
	port[0].first = port[1].first = true;
}

void YM2203::write_io8(uint32 addr, uint32 data)
{
	if(addr & 1) {
		if(ch == 7)
			mode = data;
		else if(ch == 14 || ch == 15) {
			int p = ch - 14;
			if(port[p].wreg != data || port[p].first) {
				for(int i = 0; i < dcount[p]; i++) {
					int shift = dshift[p][i];
					uint32 val = (shift < 0) ? (data >> (-shift)) : (data << shift);
					uint32 mask = (shift < 0) ? (dmask[p][i] >> (-shift)) : (dmask[p][i] << shift);
					dev[p][i]->write_signal(did[p][i], val, mask);
				}
				port[p].wreg = data;
				port[p].first = false;
			}
		}
		opn->SetReg(ch, data);
	}
	else {
		ch = data;
		// prescaler
		if(0x2d <= ch && ch <= 0x2f)
			opn->SetReg(ch, 0);
	}
}

uint32 YM2203::read_io8(uint32 addr)
{
	if(addr & 1) {
		if(ch == 14)
			return (mode & 0x40) ? port[0].wreg : port[0].rreg;
		else if(ch == 15)
			return (mode & 0x80) ? port[1].wreg : port[1].rreg;
		else
			return opn->GetReg(ch);
	}
	else
		return opn->ReadStatus();
}

void YM2203::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_YM2203_PORT_A)
		port[0].rreg = (port[0].rreg & ~mask) | (data & mask);
	else if(id == SIG_YM2203_PORT_B)
		port[1].rreg = (port[1].rreg & ~mask) | (data & mask);
	else if(id == SIG_YM2203_MUTE)
		mute = (data & mask) ? true : false;
}

void YM2203::event_vsync(int v, int clock)
{
	opn->Count(usec);
}

void YM2203::mix(int32* buffer, int cnt)
{
	if(mute)
		return;
	_memset(sound_tmp, 0, cnt * 2 * sizeof(int32));
	opn->Mix(sound_tmp, cnt);
	for(int i = 0, j = 0; i < cnt; i++, j += 2)
		buffer[i] = sound_tmp[j];
}

void YM2203::init(int rate, int clock, int samples, int volf, int volp)
{
	opn->Init(clock, rate, false, NULL);
	opn->SetVolumeFM(volf);
	opn->SetVolumePSG(volp);
	sound_tmp = (int32*)malloc(samples * 2 * sizeof(int32));
}

