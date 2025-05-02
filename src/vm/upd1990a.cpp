/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.19-

	[ uPD1990A / uPD4990A ]
*/

#include "upd1990a.h"

#define YEAR		time[0]
#define MONTH		time[1]
#define DAY		time[2]
#define DAY_OF_WEEK	time[3]
#define HOUR		time[4]
#define MINUTE		time[5]
#define SECOND		time[6]

void UPD1990A::initialize()
{
	shift_out = 0;
#ifdef HAS_UPD4990A
	shift_cmd = 0;
#endif
	
	// register event
	if(outputs_tp.count != 0) {
		register_event(this, 0, 1000000.0 / 512.0, true, &event_id);
	}
}

void UPD1990A::write_io8(uint32 addr, uint32 data)
{
	// XXX: NEC PC-98x1 $20
	cmd = data & 7;
	din = ((data & 0x20) != 0);
	write_signal(SIG_UPD1990A_STB, data, 8);
	write_signal(SIG_UPD1990A_CLK, data, 0x10);
}

void UPD1990A::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_UPD1990A_CLK) {
		bool next = ((data & mask) != 0);
		if(!clk && next) {
			if(mode == 1) {
				shift_out >>= 1;
				// output LSB
				write_signals(&outputs_dout, (shift_out & 1) ? 0xffffffff : 0);
			}
#ifdef HAS_UPD4990A
			shift_cmd = (shift_cmd >> 1) | (din ? 8 : 0);
#endif
		}
		clk = next;
	}
	else if(id == SIG_UPD1990A_STB) {
		bool next = ((data & mask) != 0);
		if(!stb && next && !clk) {
#ifdef HAS_UPD4990A
			if(cmd == 7) {
				mode = shift_cmd & 0x0f;
			}
			else
#endif
			mode = cmd & 7;
			
			if(mode == 3) {
				int time[8];
				emu->get_timer(time);
				shift_out  = to_bcd(SECOND);
				shift_out |= to_bcd(MINUTE) << 8;
				shift_out |= to_bcd(HOUR) << 16;
				shift_out |= to_bcd(DAY) << 24;
				shift_out |= (uint64)(DAY_OF_WEEK) << 32;
				shift_out |= (uint64)(MONTH) << 36;
#ifdef HAS_UPD4990A
				shift_out |= to_bcd(YEAR % 100) << 40;
#endif
				// output LSB
				write_signals(&outputs_dout, (shift_out & 1) ? 0xffffffff : 0);
			}
			else if(mode >= 4 && mode <= 6) {
				if(tpmode != mode && outputs_tp.count != 0) {
					if(event_id != -1) {
						cancel_event(event_id);
						event_id = -1;
					}
					switch(cmd) {
					case 4:	// 64Hz
						register_event(this, 0, 1000000.0 / 128.0, true, &event_id);
						break;
					case 5:	// 256Hz
						register_event(this, 0, 1000000.0 / 512.0, true, &event_id);
						break;
					case 6:	// 2048Hz
						register_event(this, 0, 1000000.0 / 4096.0, true, &event_id);
						break;
					}
				}
				tpmode = mode;
			}
		}
		stb = next;
	}
	else if(id == SIG_UPD1990A_C0) {
		cmd = (cmd & ~1) | (data & mask ? 1 : 0);
	}
	else if(id == SIG_UPD1990A_C1) {
		cmd = (cmd & ~2) | (data & mask ? 2 : 0);
	}
	else if(id == SIG_UPD1990A_C2) {
		cmd = (cmd & ~4) | (data & mask ? 4 : 0);
	}
	else if(id == SIG_UPD1990A_DIN) {
		din = ((data & mask) != 0);
	}
}

void UPD1990A::event_callback(int event_id, int err)
{
	write_signals(&outputs_tp, tp ? 0xffffffff : 0);
	tp = !tp;
}

uint64 UPD1990A::to_bcd(int data)
{
	return (uint64)(((int)(data / 10) << 4) | (data % 10));
}

