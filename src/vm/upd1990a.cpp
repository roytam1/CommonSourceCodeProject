/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2008.04.19-

	[ uPD1990A / uPD4990A ]
*/

#include "upd1990a.h"

#define EVENT_1SEC	0
#define EVENT_TP	1

void UPD1990A::initialize()
{
	// initialize rtc
	shift_data = 0;
#ifdef HAS_UPD4990A
	shift_cmd = 0;
#endif
	emu->get_host_time(&cur_time);
	
	// register events
	if(outputs_tp.count != 0) {
		register_event(this, EVENT_TP, 1000000.0 / 512.0, true, &register_id);
	} else {
		register_id = -1;
	}
	register_event(this, EVENT_1SEC, 1000000.0, true, NULL);
}

void UPD1990A::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_UPD1990A_CLK) {
		bool next = ((data & mask) != 0);
		if(!clk && next) {
			if(mode == 1) {
#ifdef HAS_UPD4990A
				shift_data = (shift_data >> 1) | (din ? (uint64)1 << (52 - 1) : 0);
#else
				shift_data = (shift_data >> 1) | (din ? (uint64)1 << (40 - 1) : 0);
#endif
				// output LSB
				write_signals(&outputs_dout, (shift_data & 1) ? 0xffffffff : 0);
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
			
			if(mode == 2) {
				uint64 tmp = shift_data;
				cur_time.second = FROM_BCD(tmp);
				tmp >>= 8;
				cur_time.minute = FROM_BCD(tmp);
				tmp >>= 8;
				cur_time.hour = FROM_BCD(tmp);
				tmp >>= 8;
				cur_time.day = FROM_BCD(tmp);
				tmp >>= 8;
				cur_time.day_of_week = tmp & 0x0f;
				tmp >>= 4;
				cur_time.month = tmp & 0x0f;
#ifdef HAS_UPD4990A
				tmp >>= 4;
				cur_time.year = FROM_BCD(tmp);
				cur_time.update_year();
				cur_time.update_day_of_week();
#endif
			}
			else if(mode == 3) {
				shift_data = 0;
#ifdef HAS_UPD4990A
				shift_data |= TO_BCD(cur_time.year);
#endif
				shift_data <<= 4;
				shift_data |= cur_time.month;
				shift_data <<= 4;
				shift_data |= cur_time.day_of_week;
				shift_data <<= 8;
				shift_data |= TO_BCD(cur_time.day);
				shift_data <<= 8;
				shift_data |= TO_BCD(cur_time.hour);
				shift_data <<= 8;
				shift_data |= TO_BCD(cur_time.minute);
				shift_data <<= 8;
				shift_data |= TO_BCD(cur_time.second);
				// output LSB
				write_signals(&outputs_dout, (shift_data & 1) ? 0xffffffff : 0);
			}
			else if(mode >= 4 && mode <= 6) {
				if(tpmode != mode && outputs_tp.count != 0) {
					if(register_id != -1) {
						cancel_event(register_id);
						register_id = -1;
					}
					switch(cmd) {
					case 4:	// 64Hz
						register_event(this, 0, 1000000.0 / 128.0, true, &register_id);
						break;
					case 5:	// 256Hz
						register_event(this, 0, 1000000.0 / 512.0, true, &register_id);
						break;
					case 6:	// 2048Hz
						register_event(this, 0, 1000000.0 / 4096.0, true, &register_id);
						break;
					}
				}
				tpmode = mode;
			}
		}
		stb = next;
	}
	else if(id == SIG_UPD1990A_CMD) {
		cmd = (cmd & ~mask) | (data & mask);
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
	if(event_id == EVENT_1SEC) {
		if(cur_time.initialized) {
			cur_time.increment();
		} else {
			emu->get_host_time(&cur_time);	// resync
			cur_time.initialized = true;
		}
	} else if(event_id == EVENT_TP) {
		write_signals(&outputs_tp, tp ? 0xffffffff : 0);
		tp = !tp;
	}
}
