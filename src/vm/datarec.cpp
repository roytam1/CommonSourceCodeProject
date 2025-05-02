/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ data recorder ]
*/

#include "datarec.h"
#include "../fileio.h"

#define EVENT_SIGNAL	0
#define EVENT_SOUND	1

#ifndef DATAREC_FF_REW_SPEED
#define DATAREC_FF_REW_SPEED	10
#endif
#define TMP_SAMPLES 0x10000

#pragma pack(1)
typedef struct {
	char RIFF[4];
	uint32 file_len;
	char WAVE[4];
	char fmt[4];
	uint32 fmt_size;
	uint16 format_id;
	uint16 channels;
	uint32 sample_rate;
	uint32 data_speed;
	uint16 block_size;
	uint16 sample_bits;
} wav_header_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	char id[4];
	uint32 size;
} wav_chunk_t;
#pragma pack()

static uint8 wavheader[44] = {
	'R' , 'I' , 'F' , 'F' , 0x00, 0x00, 0x00, 0x00, 'W' , 'A' , 'V' , 'E' , 'f' , 'm' , 't' , ' ' ,
	0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x80, 0xbb, 0x00, 0x00, 0x80, 0xbb, 0x00, 0x00,
	0x01, 0x00, 0x08, 0x00, 'd' , 'a' , 't' , 'a' , 0x00, 0x00, 0x00, 0x00
};

void DATAREC::initialize()
{
	fio = new FILEIO();
	
	play = rec = remote = trigger = false;
	ff_rew = 0;
	in_signal = out_signal = changed = false;
	register_id = -1;
	
	buffer = buffer_bak = NULL;
#ifdef DATAREC_SOUND
	wav_buffer = NULL;
#endif
	buffer_ptr = buffer_length = 0;
	is_wav = false;
	
#ifdef DATAREC_SOUND
	mix_buffer = NULL;
	mix_buffer_ptr = mix_buffer_length = 0;
#endif
}

void DATAREC::reset()
{
	close_datarec();
}

void DATAREC::release()
{
#ifdef DATAREC_SOUND
	if(mix_buffer != NULL) {
		free(mix_buffer);
	}
#endif
	close_file();
	delete fio;
}

void DATAREC::write_signal(int id, uint32 data, uint32 mask)
{
	bool signal = ((data & mask) != 0);
	
	if(id == SIG_DATAREC_OUT) {
		if(out_signal != signal) {
			if(rec && remote) {
				changed = true;
			}
			out_signal = signal;
		}
	} else if(id == SIG_DATAREC_REMOTE) {
		set_remote(signal);
	} else if(id == SIG_DATAREC_TRIG) {
		// L->H: remote signal is switched
		if(signal && !trigger) {
			set_remote(!remote);
		}
		trigger = signal;
	}
}

void DATAREC::event_callback(int event_id, int err)
{
	if(event_id == EVENT_SIGNAL) {
		if(play) {
			bool signal = in_signal;
			if(is_wav) {
				if(buffer_ptr >= 0 && buffer_ptr < buffer_length) {
					signal = ((buffer[buffer_ptr] & 0x80) != 0);
#ifdef DATAREC_SOUND
					if(wav_buffer != NULL && ff_rew == 0) {
						wav_sample = wav_buffer[buffer_ptr];
					} else {
						wav_sample = 0;
					}
#endif
				}
				if(ff_rew < 0) {
					if((buffer_ptr = max(buffer_ptr - 1, 0)) == 0) {
						set_remote(false);	// top of tape
					}
				} else {
					if((buffer_ptr = min(buffer_ptr + 1, buffer_length)) == buffer_length) {
						set_remote(false);	// end of tape
					}
				}
				update_event();
			} else {
				if(ff_rew < 0) {
					if(buffer_bak != NULL) {
						memcpy(buffer, buffer_bak, buffer_length);
					}
					buffer_ptr = 0;
					set_remote(false);	// top of tape
				} else {
					while(buffer_ptr < buffer_length) {
						if((buffer[buffer_ptr] & 0x7f) == 0) {
							if(++buffer_ptr == buffer_length) {
								set_remote(false);	// end of tape
								break;
							}
						} else {
							signal = ((buffer[buffer_ptr] & 0x80) != 0);
							uint8 tmp = buffer[buffer_ptr];
							buffer[buffer_ptr] = (tmp & 0x80) | ((tmp & 0x7f) - 1);
							break;
						}
					}
				}
			}
			// notify the signal is changed
			if(signal != in_signal) {
				in_signal = signal;
				changed = true;
				write_signals(&outputs_out, in_signal ? 0xffffffff : 0);
			}
		} else if(rec) {
			if(is_wav) {
				buffer[buffer_ptr] = out_signal ? 0xff : 0;
				if(++buffer_ptr >= buffer_length) {
					fio->Fwrite(buffer, buffer_length, 1);
					buffer_ptr = 0;
				}
			} else {
				bool prev_signal = ((buffer[buffer_ptr] & 0x80) != 0);
				if(prev_signal != out_signal || (buffer[buffer_ptr] & 0x7f) == 0x7f) {
					if(++buffer_ptr >= buffer_length) {
						fio->Fwrite(buffer, buffer_length, 1);
						buffer_ptr = 0;
					}
					buffer[buffer_ptr] = out_signal ? 0x80 : 0;
				}
				buffer[buffer_ptr]++;
			}
		}
	}
#ifdef DATAREC_SOUND
	else if(event_id == EVENT_SOUND) {
		if(mix_buffer_ptr < mix_buffer_length) {
			mix_buffer[mix_buffer_ptr++] = wav_sample;
		}
	}
#endif
}

void DATAREC::set_remote(bool value)
{
	if(remote != value) {
		remote = value;
		update_event();
	}
}

void DATAREC::set_ff_rew(int value)
{
	if(ff_rew != value) {
		if(register_id != -1) {
			cancel_event(register_id);
			register_id = -1;
		}
		ff_rew = value;
		update_event();
	}
}

void DATAREC::update_event()
{
	if(remote && (play || rec)) {
		if(register_id == -1) {
			if(ff_rew != 0) {
				register_event(this, EVENT_SIGNAL, 1000000. / sample_rate / DATAREC_FF_REW_SPEED, true, &register_id);
			} else {
				register_event(this, EVENT_SIGNAL, 1000000. / sample_rate, true, &register_id);
			}
		}
	} else {
		if(register_id != -1) {
			cancel_event(register_id);
			register_id = -1;
		}
	}
	
	// update signals
#ifdef DATAREC_SOUND
	if(!(play && remote)) {
		wav_sample = 0;
	}
#endif
	write_signals(&outputs_remote, remote ? 0xffffffff : 0);
	write_signals(&outputs_rotate, (register_id != -1) ? 0xffffffff : 0);
	write_signals(&outputs_end, (buffer_ptr == buffer_length) ? 0xffffffff : 0);
	write_signals(&outputs_top, (buffer_ptr == 0) ? 0xffffffff : 0);
}

bool DATAREC::play_datarec(_TCHAR* file_path)
{
	close_datarec();
	
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		if(check_file_extension(file_path, _T(".wav"))) {
			// standard PCM wave file
			if((buffer_length = load_wav_image(0)) == 0) {
				return false;
			}
			is_wav = true;
		} else if(check_file_extension(file_path, _T(".tap"))) {
			// SHARP X1 series tape image
			if((buffer_length = load_tap_image()) == 0) {
				return false;
			}
			buffer = (uint8 *)malloc(buffer_length);
			load_tap_image();
		} else if(check_file_extension(file_path, _T(".mzt")) || check_file_extension(file_path, _T(".m12"))) {
			// SHARP MZ series tape image
			if((buffer_length = load_mzt_image()) == 0) {
				return false;
			}
			buffer = (uint8 *)malloc(buffer_length);
			load_mzt_image();
		} else if(check_file_extension(file_path, _T(".mtw"))) {
			// skip mzt image
			uint8 header[128];
			fio->Fread(header, sizeof(header), 1);
			uint16 size = header[0x12] | (header[0x13] << 8);
			// load standard PCM wave file
			if((buffer_length = load_wav_image(sizeof(header) + size)) == 0) {
				return false;
			}
			is_wav = true;
		} else {
			// standard cas image for my emulator
			if((buffer_length = load_cas_image()) == 0) {
				return false;
			}
		}
		if(!is_wav && buffer_length != 0) {
			buffer_bak = (uint8 *)malloc(buffer_length);
			memcpy(buffer_bak, buffer, buffer_length);
		}
		
		// get the first signal
		bool signal = ((buffer[0] & 0x80) != 0);
		if(signal != in_signal) {
			write_signals(&outputs_out, signal ? 0xffffffff : 0);
			in_signal = signal;
		}
		play = true;
		update_event();
	}
	return play;
}

bool DATAREC::rec_datarec(_TCHAR* file_path)
{
	close_datarec();
	
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		sample_rate = 48000;
		buffer_length = 1024 * 1024;
		buffer = (uint8 *)malloc(buffer_length);
		
		if(check_file_extension(file_path, _T(".wav"))) {
			// write wave header
			fio->Fwrite(wavheader, sizeof(wavheader), 1);
			is_wav = true;
		} else {
			// initialize buffer
			buffer[0] = out_signal ? 0x80 : 0;
		}
		rec = true;
		update_event();
	}
	return rec;
}

void DATAREC::close_datarec()
{
	close_file();
	
	play = rec = is_wav = false;
	buffer_ptr = buffer_length = 0;
	update_event();
	
	// no sounds
	write_signals(&outputs_out, 0);
	in_signal = false;
}

void DATAREC::close_file()
{
	if(rec) {
		if(is_wav) {
			save_wav_image();
		} else {
			fio->Fwrite(buffer, buffer_ptr + 1, 1);
		}
	}
	if(play || rec) {
		fio->Fclose();
	}
	if(buffer != NULL) {
		free(buffer);
		buffer = NULL;
	}
	if(buffer_bak != NULL) {
		free(buffer_bak);
		buffer_bak = NULL;
	}
#ifdef DATAREC_SOUND
	if(wav_buffer != NULL) {
		free(wav_buffer);
		wav_buffer = NULL;
	}
#endif
}

// standard cas image for my emulator

int DATAREC::load_cas_image()
{
	sample_rate = 48000;
	
	// get file size
	fio->Fseek(0, FILEIO_SEEK_END);
	int file_size = fio->Ftell();
	
	// load samples
	if(file_size > 0) {
		buffer = (uint8 *)malloc(file_size);
		fio->Fseek(0, FILEIO_SEEK_SET);
		fio->Fread(buffer, file_size, 1);
	}
	return file_size;
}

// standard PCM wave file

int DATAREC::load_wav_image(int offset)
{
	// check wave header
	wav_header_t header;
	wav_chunk_t chunk;
	
	fio->Fseek(offset, FILEIO_SEEK_SET);
	fio->Fread(&header, sizeof(header), 1);
	if(header.format_id != 1 || !(header.sample_bits == 8 || header.sample_bits == 16)) {
		// this is not pcm format !!!
		fio->Fclose();
		return 0;
	}
	fio->Fseek(header.fmt_size - 0x10, FILEIO_SEEK_CUR);
	while(1) {
		fio->Fread(&chunk, sizeof(chunk), 1);
		if(strncmp(chunk.id, "data", 4) == 0) {
			break;
		}
		fio->Fseek(chunk.size, FILEIO_SEEK_CUR);
	}
	
	int samples = chunk.size / header.channels;
	int tmp_length = header.channels * TMP_SAMPLES;
	if(header.sample_bits == 16) {
		samples /= 2;
		tmp_length *= 2;
	}
	sample_rate = header.sample_rate;
	
	// load samples
	if(samples > 0) {
		bool prev_signal = false, signal;
		buffer = (uint8 *)malloc(samples);
#ifdef DATAREC_SOUND
		if(header.channels > 1) {
			wav_buffer = (int16 *)malloc(samples * sizeof(int16));
		}
#endif
		uint8 *tmp_buffer = (uint8 *)malloc(tmp_length);
		fio->Fread(tmp_buffer, tmp_length, 1);
		
		for(int i = 0, tmp_ptr = 0; i < samples; i++) {
			typedef union {
				int16 s16;
				struct {
					uint8 l, h;
				} b;
			} sample_pair;
			sample_pair data;
			
			data.b.l = tmp_buffer[tmp_ptr++];
			if(header.sample_bits == 16) {
				data.b.h = tmp_buffer[tmp_ptr++];
				signal = (data.s16 > (prev_signal ? -4096 : 4096));
			} else {
				signal = (data.b.l > 128 + (prev_signal ? -16 : 16));
			}
			buffer[i] = signal ? 0xff : 0;
			prev_signal = signal;
#ifdef DATAREC_SOUND
			if(header.channels > 1) {
				data.b.l = tmp_buffer[tmp_ptr++];
				if(header.sample_bits == 16) {
					data.b.h = tmp_buffer[tmp_ptr++];
					wav_buffer[i] = data.s16;
				} else {
					wav_buffer[i] = ((int16)data.b.l - 128) * 256;
				}
			}
			for(int i = 3; i <= header.channels; i++) {
#else
			for(int i = 2; i <= header.channels; i++) {
#endif
				tmp_buffer[tmp_ptr++];
				if(header.sample_bits == 16) {
					tmp_buffer[tmp_ptr++];
				}
			}
			if(tmp_ptr == tmp_length) {
				fio->Fread(tmp_buffer, tmp_length, 1);
				tmp_ptr = 0;
			}
		}
		free(tmp_buffer);
	}
	return samples;
}

void DATAREC::save_wav_image()
{
	// write samples remained in buffer
	if(buffer_ptr > 0) {
		fio->Fwrite(buffer, buffer_ptr, 1);
	}
	int samples = fio->Ftell() - sizeof(wavheader);
	int total = samples + 0x24;
	
	// write header
	uint8 wav[44];
	memcpy(wav, wavheader, sizeof(wavheader));
	wav[ 4] = (uint8)((total   >>  0) & 0xff);
	wav[ 5] = (uint8)((total   >>  8) & 0xff);
	wav[ 6] = (uint8)((total   >> 16) & 0xff);
	wav[ 7] = (uint8)((total   >> 24) & 0xff);
	wav[40] = (uint8)((samples >>  0) & 0xff);
	wav[41] = (uint8)((samples >>  8) & 0xff);
	wav[42] = (uint8)((samples >> 16) & 0xff);
	wav[43] = (uint8)((samples >> 24) & 0xff);
	
	fio->Fseek(0, FILEIO_SEEK_SET);
	fio->Fwrite(wav, sizeof(wav), 1);
}

// SHARP X1 series tape image

/*
	new tape file format for t-tune (from tape_fmt.txt)

	offset:size :
	00H   :  4  : 識別インデックス "TAPE"
	04H   : 17  : テープの名前(asciiz)
	15H   :  5  : リザーブ
	1AH   :  1  : ライトプロテクトノッチ(00H=書き込み可、10H=書き込み禁止）
	1BH   :  1  : 記録フォーマットの種類(01H=定速サンプリング方法）
	1CH   :  4  : サンプリング周波数(Ｈｚ単位）
	20H   :  4  : テープデータのサイズ（ビット単位）
	24H   :  4  : テープの位置（ビット単位）
	28H   :  ?  : テープのデータ
*/

int DATAREC::load_tap_image()
{
	// get file size
	fio->Fseek(0, FILEIO_SEEK_END);
	int file_size = fio->Ftell();
	fio->Fseek(0, FILEIO_SEEK_SET);
	
	// check header
	uint8 header[4];
	fio->Fread(header, 4, 1);
	
	if(header[0] == 'T' && header[1] == 'A' && header[2] == 'P' && header[3] == 'E') {
		// skip name, reserved, write protect notch
		fio->Fseek(17 + 5 + 1, FILEIO_SEEK_CUR);
		// format
		if(fio->Fgetc() != 0x01) {
			// unknown data format
			return 0;
		}
		// sample rate
		fio->Fread(header, 4, 1);
		sample_rate = header[0] | (header[1] << 8) | (header[2] << 16) | (header[3] << 24);
		// data length
		fio->Fread(header, 4, 1);
		// play position
		fio->Fread(header, 4, 1);
	} else {
		// sample rate
		sample_rate = header[0] | (header[1] << 8) | (header[2] << 16) | (header[3] << 24);
	}
	
	// load samples
	int ptr = 0, data;
	uint8 prev_data = 0;
	while((data = fio->Fgetc()) != EOF) {
		for(int i = 0, bit = 0x80; i < 8; i++, bit >>= 1) {
			// inc pointer
			bool prev_signal = ((prev_data & 0x80) != 0);
			bool cur_signal = ((data & bit) != 0);
			if(prev_signal != cur_signal || (prev_data & 0x7f) == 0x7f) {
				if((prev_data & 0x7f) == 0) {
					// don't inc pointer
				} else {
					ptr++;
				}
				prev_data = cur_signal ? 0x80 : 0;
			}
			// inc pulse count
			prev_data++;
			if(buffer != NULL) {
				buffer[ptr] = prev_data;
			}
		}
	}
	return (ptr + 1);
}

// SHARP MZ series tape image

#define MZT_PUT_BIT(bit, len) { \
	for(int l = 0; l < (len); l++) { \
		if(buffer != NULL) { \
			buffer[ptr++] = (bit) ? 0x98 : 0x8b; \
			buffer[ptr++] = (bit) ? 0x1d : 0x0f; \
		} else { \
			ptr += 2; \
		} \
	} \
}

#define MZT_PUT_BYTE(byte) { \
	MZT_PUT_BIT(1, 1); \
	for(int j = 0; j < 8; j++) { \
		if((byte) & (0x80 >> j)) { \
			MZT_PUT_BIT(1, 1); \
			count++; \
		} else { \
			MZT_PUT_BIT(0, 1); \
		} \
	} \
}

#define MZT_PUT_BLOCK(buf, len) { \
	int count = 0; \
	for(int i = 0; i < (len); i++) { \
		MZT_PUT_BYTE((buf)[i]); \
	} \
	uint8 hi = (count >> 8) & 0xff; \
	uint8 lo = (count >> 0) & 0xff; \
	MZT_PUT_BYTE(hi); \
	MZT_PUT_BYTE(lo); \
}

int DATAREC::load_mzt_image()
{
	sample_rate = 48000;
	
	// get file size
	fio->Fseek(0, FILEIO_SEEK_END);
	int file_size = fio->Ftell();
	fio->Fseek(0, FILEIO_SEEK_SET);
	
	// load mzt file
	int ptr = 0;
	while(file_size > 128) {
		// load header
		uint8 header[128], ram[0x20000];
		fio->Fread(header, sizeof(header), 1);
		file_size -= sizeof(header);
		
		uint16 size = header[0x12] | (header[0x13] << 8);
		uint16 offs = header[0x14] | (header[0x15] << 8);
		memset(ram, 0, sizeof(ram));
		fio->Fread(ram + offs, size, 1);
		file_size -= size;
#if 0
		// apply mz700win patch
		if(header[0x40] == 'P' && header[0x41] == 'A' && header[0x42] == 'T' && header[0x43] == ':') {
			int patch_ofs = 0x44;
			for(; patch_ofs < 0x80; ) {
				uint16 patch_addr = header[patch_ofs] | (header[patch_ofs + 1] << 8);
				patch_ofs += 2;
				if(patch_addr == 0xffff) {
					break;
				}
				int patch_len = header[patch_ofs++];
				for(int i = 0; i < patch_len; i++) {
					ram[patch_addr + i] = header[patch_ofs++];
				}
			}
			for(int i = 0x40; i < patch_ofs; i++) {
				header[i] = 0;
			}
		}
#endif
		// output to buffer
		MZT_PUT_BIT(0, 10000);
		MZT_PUT_BIT(1, 40);
		MZT_PUT_BIT(0, 40);
		MZT_PUT_BIT(1, 1);
		MZT_PUT_BLOCK(header, 128);
		MZT_PUT_BIT(1, 1);
		MZT_PUT_BIT(0, 256);
		MZT_PUT_BLOCK(header, 128);
		MZT_PUT_BIT(1, 1);
		MZT_PUT_BIT(0, 10000);
		MZT_PUT_BIT(1, 20);
		MZT_PUT_BIT(0, 20);
		MZT_PUT_BIT(1, 1);
		MZT_PUT_BLOCK(ram + offs, size);
		MZT_PUT_BIT(1, 1);
	}
	return ptr;
}

#ifdef DATAREC_SOUND
void DATAREC::initialize_sound(int rate, int samples)
{
	mix_buffer = (int16 *)malloc(samples * sizeof(int16));
	mix_buffer_length = samples;
	register_event(this, EVENT_SOUND, 1000000. / (double)rate, true, NULL);
}

void DATAREC::mix(int32* buffer, int cnt)
{
	int16 sample = 0;
	for(int i = 0; i < cnt; i++) {
		if(i < mix_buffer_ptr) {
			sample = mix_buffer[i];
		}
		*buffer += sample;
		*buffer += sample;
	}
	if(cnt < mix_buffer_ptr) {
		memmove(mix_buffer, mix_buffer + cnt, (mix_buffer_ptr - cnt) * sizeof(int16));
		mix_buffer_ptr -= cnt;
	} else {
		mix_buffer_ptr = 0;
	}
}
#endif
