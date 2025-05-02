/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2014.02.12-

	[ Pioneer LD-700 ]
*/

#define EVENT_ACK		0

#define PHASE_IDLE		0
#define PHASE_HEADER_PULSE	1
#define PHASE_HEADER_SPACE	2
#define PHASE_BITS_PULSE	3
#define PHASE_BITS_SPACE	4

#define STATUS_EJECT		0
#define STATUS_STOP		1
#define STATUS_PLAY		2
#define STATUS_PAUSE		3

#define SEEK_CHAPTER		0x40
#define SEEK_FRAME		0x41
#define SEEK_WAIT		0x5f

#include "ld700.h"
#include "../fileio.h"

void LD700::initialize()
{
	prev_signal = false;
	prev_time = 0;
	command = num_bits = 0;
	
	status = STATUS_EJECT;
	phase = PHASE_IDLE;
	seek_mode = seek_num = 0;
	accepted = false;
	cur_frame_raw = cur_track = 0;
	
	register_frame_event(this);
}

void LD700::write_signal(int id, uint32 data, uint32 mask)
{
	if(id == SIG_LD700_LREMO) {
		bool signal = ((data & mask) != 0);
		if(prev_signal != signal) {
			int usec = (int)passed_usec(prev_time);
			prev_time = current_clock();
			prev_signal = signal;
			
			// from openmsx-0.10.0/src/laserdisc/
			switch(phase) {
			case PHASE_IDLE:
				if(signal) {
					command = num_bits = 0;
					phase = PHASE_HEADER_PULSE;
				}
				break;
			case PHASE_HEADER_PULSE:
				if(5800 <= usec && usec < 11200) {
					phase = PHASE_HEADER_SPACE;
				} else {
					phase = PHASE_IDLE;
				}
				break;
			case PHASE_HEADER_SPACE:
				if(3400 <= usec && usec < 6200) {
					phase = PHASE_BITS_PULSE;
				} else {
					phase = PHASE_IDLE;
				}
				break;
			case PHASE_BITS_PULSE:
				if(usec >= 380 && usec < 1070) {
					phase = PHASE_BITS_SPACE;
				} else {
					phase = PHASE_IDLE;
				}
				break;
			case PHASE_BITS_SPACE:
				if(1260 <= usec && usec < 4720) {
					// bit 1
					command |= 1 << num_bits;
				} else if(usec < 300 || usec >= 1065) {
					// error
					phase = PHASE_IDLE;
					break;
				}
				if(++num_bits == 32) {
					uint8 custom      = ( command >>  0) & 0xff;
					uint8 custom_comp = (~command >>  8) & 0xff;
					uint8 code        = ( command >> 16) & 0xff;
					uint8 code_comp   = (~command >> 24) & 0xff;
					if(custom == custom_comp && custom == 0xa8 && code == code_comp) {
						// command accepted
						accepted = true;
					}
					phase = PHASE_IDLE;
				} else {
					phase = PHASE_BITS_PULSE;
				}
				break;
			}
		}
	}
}

void LD700::event_frame()
{
	int prev_frame_raw = cur_frame_raw;
	int prev_track = cur_track;
	bool seek_done = false;
	
	cur_frame_raw = get_cur_frame_raw();
	cur_track = get_cur_track();
	
	if(accepted) {
		command = (command >> 16) & 0xff;
		switch(command) {
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
			if(status != STATUS_EJECT /*&& status != STATUS_STOP*/) {
				seek_num = (seek_num * 10 + command) % 100000;
			}
			break;
		case 0x16:
			if(status != STATUS_EJECT) {
				if(status == STATUS_STOP) {
					//emu->close_laser_disc();
				} else {
					emu->stop_movie();
					emu->set_cur_movie_frame(0, false);
					set_status(STATUS_STOP);
				}
			}
			break;
		case 0x17:
			if(status != STATUS_EJECT) {
				emu->play_movie();
				set_status(STATUS_PLAY);
			}
			break;
		case 0x18:
			if(status != STATUS_EJECT /*&& status != STATUS_STOP*/) {
				emu->pause_movie();
				set_status(STATUS_PAUSE);
			}
			break;
		case 0x40:
		case 0x41:
		case 0x5f:
			if(status != STATUS_EJECT /*&& status != STATUS_STOP*/) {
				seek_mode = command;
				seek_num = 0;
			}
			break;
		case 0x42:
			if(status != STATUS_EJECT /*&& status != STATUS_STOP*/) {
				if(seek_mode == SEEK_CHAPTER) {
					set_cur_track(seek_num);
					cur_track = seek_num;
				} else if(seek_mode == SEEK_FRAME) {
					set_cur_frame(seek_num, false);
				}
				if(status == STATUS_PAUSE) {
					emu->play_movie();
					set_status(STATUS_PLAY);
				}
				seek_mode = 0;
				seek_done = true;
			}
			break;
		case 0x45:
			if(status != STATUS_EJECT /*&& status != STATUS_STOP*/) {
				seek_num = 0;
			}
			break;
		default:
			emu->out_debug(_T("LaserDisc: Unknown Command %02X\n"), command);
		}
		accepted = false;
		set_ack(true);
	}
	
	if(!seek_done && status == STATUS_PLAY) {
		for(int i = 0; i < num_pauses; i++) {
			if(prev_frame_raw < pause_frame_raw[i] && cur_frame_raw >= pause_frame_raw[i]) {
				emu->pause_movie();
				set_status(STATUS_PAUSE);
				break;
			}
		}
	}
#ifdef USE_TAPE
	if(prev_track != cur_track) {
		_TCHAR wav_path[MAX_PATH];
		_stprintf(wav_path, _T("%s_TRK%d.WAV"), get_file_path_without_extensiton(disc_path), cur_track);
		vm->play_tape(wav_path);
	}
#endif
}

void LD700::event_callback(int event_id, int err)
{
	if(event_id == EVENT_ACK) {
		set_ack(false);
	}
}

void LD700::set_status(int value)
{
	if(status != value) {
		write_signals(&outputs_drec_remote, (value == STATUS_PLAY) ? 0xffffffff : 0);
		write_signals(&outputs_exv, !(value == STATUS_EJECT || value == STATUS_STOP) ? 0xffffffff : 0);
		status = value;
	}
}

void LD700::set_ack(bool value)
{
	if(value) {
		register_event(this, EVENT_ACK, 46000, false, NULL);
	}
	write_signals(&outputs_ack, value ? 0xffffffff : 0);
}

void LD700::set_cur_frame(int frame, bool relative)
{
	if(relative) {
		if(frame == 0) {
			return;
		}
	} else {
		if(frame < 0) {
			return;
		}
	}
	
	bool sign = (frame >= 0);
	frame = (int)((double)abs(frame) / 59.94 * emu->get_movie_fps() + 0.5);
	if(!sign) {
		frame = -frame;
	}
	if(relative && frame == 0) {
		frame = sign ? 1 : -1;
	}
	emu->set_cur_movie_frame(frame, relative);
}

int LD700::get_cur_frame()
{
	return (int)(emu->get_cur_movie_frame() * 59.94 / emu->get_movie_fps() + 0.5);
}

int LD700::get_cur_frame_raw()
{
	return emu->get_cur_movie_frame();
}

void LD700::set_cur_track(int track)
{
	if(track >= 1 && track <= num_tracks) {
		emu->set_cur_movie_frame(track_frame_raw[track - 1], false);
	}
}

int LD700::get_cur_track()
{
	if(num_tracks != 0) {
		int frame = emu->get_cur_movie_frame();
		for(int t = num_tracks; t > 0; t--) {
			if(track_frame_raw[t - 1] <= frame) {
				return t;
			}
		}
	}
	return 0;
}

void LD700::open_disc(_TCHAR* file_path)
{
	if(emu->open_movie_file(file_path)) {
		_tcscpy(disc_path, file_path);
		
		// read cue sheet
		_TCHAR cue_path[MAX_PATH], tmp[1024], p1[1024], p2[1024], p3[1024], *str1, *str2;
		_stprintf(cue_path, _T("%s.CUE"), get_file_path_without_extensiton(file_path));
		
		num_tracks = 1;
		memset(track_frame_raw, 0, sizeof(track_frame_raw));
		
		FILEIO* fio = new FILEIO();
		if(fio->Fopen(cue_path, FILEIO_READ_ASCII)) {
			int track = 0;
			while(fio->Fgets(tmp, 1024) != NULL) {
				p1[0] = p2[0] = p3[0] = _T('\0');
				_stscanf(tmp, _T("%s %s %s"), p1, p2, p3);
				if(_tcsicmp(p1, _T("TRACK")) == 0) {
					track = _tstoi(p2);
					if(track > num_tracks && track <= MAX_TRACKS) {
						num_tracks = track;
					}
				} else if(_tcsicmp(p1, _T("INDEX")) == 0) {
					int number = _tstoi(p2);
					int frame_raw = -1;
					if((str1 = _tcsstr(p3, _T(":"))) != NULL) {
						*str1 = _T('\0');
						if((str2 = _tcsstr(str1 + 1, _T(":"))) != NULL) {
							*str2 = _T('\0');
							double m = (double)_tstoi(p3);
							double s = (double)_tstoi(str1 + 1);
							double f = (double)_tstoi(str2 + 1);
							
							frame_raw = (int)((m * 60 + s) * emu->get_movie_fps() + f + 0.5);
						}
					}
					if(frame_raw != -1 && number > 0) {
						if(number == 1 && track >= 1 && track <= MAX_TRACKS) {
							track_frame_raw[track - 1] = frame_raw;
						}
						if(num_pauses < MAX_PAUSES) {
							pause_frame_raw[num_pauses++] = frame_raw;
						}
					}
				}
			}
			for(int i = 1; i < num_tracks; i++) {
				if(track_frame_raw[i] == 0) {
					track_frame_raw[i] = track_frame_raw[i - 1];
				}
			}
			fio->Fclose();
		}
		delete fio;
		
		set_status(STATUS_STOP);
	} else {
		close_disc();
	}
}

void LD700::close_disc()
{
	emu->close_movie_file();
	num_tracks = 0;
	set_status(STATUS_EJECT);
}

bool LD700::disc_inserted()
{
	return (status != STATUS_EJECT);
}
