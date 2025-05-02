/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ data recorder ]
*/

#include "datarec.h"
#include "../fileio.h"

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
	char data[4];
	uint32 data_len;
} wav_data_t;
#pragma pack()

static uint8 wavheader[44] = {
	'R' , 'I' , 'F' , 'F' , 0x00, 0x00, 0x00, 0x00, 'W' , 'A' , 'V' , 'E' , 'f' , 'm' , 't' , ' ' ,
	0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x80, 0xbb, 0x00, 0x00, 0x80, 0xbb, 0x00, 0x00,
	0x01, 0x00, 0x08, 0x00, 'd' , 'a' , 't' , 'a' , 0x00, 0x00, 0x00, 0x00
};

void DATAREC::initialize()
{
	// data recorder
	fio = new FILEIO();
	memset(buffer, 0, sizeof(buffer));
	bufcnt = samples = 0;
	
	register_id = -1;
	play = rec = false;
	in = out = change = remote = trig = false;
	is_wav = is_tap = is_mzt = false;
}

void DATAREC::reset()
{
	close_datarec();
}

void DATAREC::release()
{
	close_datarec();
	delete fio;
}

void DATAREC::write_signal(int id, uint32 data, uint32 mask)
{
	bool signal = ((data & mask) != 0);
	
	if(id == SIG_DATAREC_OUT) {
		if(rec && remote && signal != out) {
			change = true;
		}
		out = signal;
	}
	else if(id == SIG_DATAREC_REMOTE) {
		remote = signal;
		write_signals(&outputs_remote, remote ? 0xffffffff : 0);
		update_event();
	}
	else if(id == SIG_DATAREC_REMOTE_NEG) {
		remote = !signal;
		write_signals(&outputs_remote, remote ? 0xffffffff : 0);
		update_event();
	}
	else if(id == SIG_DATAREC_TRIG) {
		// L->H: remote signal is switched
		if(signal && !trig) {
			remote = !remote;
			write_signals(&outputs_remote, remote ? 0xffffffff : 0);
			update_event();
		}
		trig = signal;
	}
	else if(id == SIG_DATAREC_REWIND) {
		// support rewind for play mode only !!!
		if(signal && play) {
			load_image();
		}
	}
}

void DATAREC::event_callback(int event_id, int err)
{
	if(play && remote) {
		// get the next signal
		bool signal = ((buffer[bufcnt] & 0x80) != 0);
		if(is_wav) {
			// inc pointer
			if(remain) {
				remain--;
			}
			if(++bufcnt >= DATAREC_BUFFER_SIZE) {
				memset(buffer, 0, sizeof(buffer));
				int samples = remain;
				for(int i = 0; i < DATAREC_BUFFER_SIZE; i++) {
					if(samples-- <= 0) {
						break;
					}
					buffer[i] = get_wav_sample();
				}
				bufcnt = 0;
			}
			update_event();
		}
		else {
			// inc pointer
			while(!(buffer[bufcnt] & 0x7f)) {
				if(remain) {
					remain--;
				}
				if(++bufcnt >= DATAREC_BUFFER_SIZE) {
					// NOTE: consider tap/mzt case !!!
					memset(buffer, 0x7f, sizeof(buffer));
					fio->Fread(buffer, sizeof(buffer), 1);
					bufcnt = 0;
				}
				signal = ((buffer[bufcnt] & 0x80) != 0);
				update_event();
			}
			// dec pulse count
			uint8 tmp = buffer[bufcnt];
			buffer[bufcnt] = (tmp & 0x80) | ((tmp & 0x7f) - 1);
		}
		// notify the signal is changed
		if(signal != in) {
			write_signals(&outputs_out, signal ? 0xffffffff : 0);
			change = true;
			in = signal;
		}
	}
	else if(rec && remote) {
		if(is_wav) {
			buffer[bufcnt] = out ? 0xf0 : 0x10;
			samples++;
			// inc pointer
			if(++bufcnt >= DATAREC_BUFFER_SIZE) {
				fio->Fwrite(buffer, sizeof(buffer), 1);
				bufcnt = 0;
			}
		}
		else {
			// inc pointer
			bool prv = ((buffer[bufcnt] & 0x80) != 0);
			if(prv != out || (buffer[bufcnt] & 0x7f) == 0x7f) {
				if(++bufcnt >= DATAREC_BUFFER_SIZE) {
					fio->Fwrite(buffer, sizeof(buffer), 1);
					bufcnt = 0;
				}
				buffer[bufcnt] = out ? 0x80 : 0;
			}
			// inc pulse count
			buffer[bufcnt]++;
		}
	}
}

bool DATAREC::play_datarec(_TCHAR* filename)
{
	close_datarec();
	
	if(fio->Fopen(filename, FILEIO_READ_BINARY)) {
		// check file extension
		is_wav = check_extension(filename, _T(".wav"));
		is_tap = check_extension(filename, _T(".tap"));
		is_mzt = check_extension(filename, _T(".mzt")) || check_extension(filename, _T(".m12"));
		
		// load image file
		load_image();
	}
	return play;
}

bool DATAREC::rec_datarec(_TCHAR* filename)
{
	close_datarec();
	
	if(fio->Fopen(filename, FILEIO_WRITE_BINARY)) {
		// default sample info
		ch = 1;
		sample_rate = 48000;
		sample_bits = 8;
		
		// check file extension
		is_wav = check_extension(filename, _T(".wav"));
		is_tap = is_mzt = false; // not supported for record
		
		// open for rec
		if(is_wav) {
			// write wave header
			fio->Fwrite(wavheader, sizeof(wavheader), 1);
		}
		else {
			// initialize buffer
			buffer[0] = out ? 0x80 : 0;
		}
		bufcnt = samples = 0;
		rec = true;
		update_event();
	}
	return rec;
}

void DATAREC::close_datarec()
{
	// close file
	if(rec) {
		if(is_wav) {
			save_wav_image();
		}
		else {
			fio->Fwrite(buffer, bufcnt + 1, 1);
		}
	}
	if(play || rec) {
		fio->Fclose();
	}
	play = rec = false;
	update_event();
	
	// no sounds
	write_signals(&outputs_out, 0);
	in = false;
}

void DATAREC::load_image()
{
	// get file size
	fio->Fseek(0, FILEIO_SEEK_END);
	remain = fio->Ftell();
	fio->Fseek(0, FILEIO_SEEK_SET);
	
	// default sample info
	ch = 1;
	sample_rate = 48000;
	sample_bits = 8;
	
	// load image file for play
	if(is_wav) {
		// standard PCM wave file
		remain = load_wav_image();
	}
	else if(is_tap) {
		// SHARP X1 series tape image
		remain = load_tap_image();
	}
	else if(is_mzt) {
		// SHARP MZ series tape image
		remain = load_mzt_image();
	}
	else {
		memset(buffer, 0x7f, sizeof(buffer));
		fio->Fread(buffer, sizeof(buffer), 1);
	}
	bufcnt = samples = 0;
	
	if(remain > 0) {
		// get the first signal
		bool signal = ((buffer[0] & 0x80) != 0);
		
		// notify the signal is changed
		if(signal != in) {
			write_signals(&outputs_out, signal ? 0xffffffff : 0);
			in = signal;
		}
		play = true;
		update_event();
	}
}

bool DATAREC::skip()
{
	bool val = change;
	change = false;
	return val;
}

void DATAREC::update_event()
{
	if(remote && ((play && remain > 0) || rec)) {
		if(register_id == -1) {
			register_event(this, 0, 1000000. / (double)sample_rate, true, &register_id);
		}
	}
	else {
		if(register_id != -1) {
			cancel_event(register_id);
		}
		register_id = -1;
	}
	
	// end of tape ?
	bool signal = (play && remain == 0);
	write_signals(&outputs_end, signal ? 0xffffffff : 0);
}

bool DATAREC::check_extension(_TCHAR* filename, _TCHAR* ext)
{
	int nam_len = _tcslen(filename);
	int ext_len = _tcslen(ext);
	
	if(nam_len >= ext_len) {
		if(_tcsncicmp(&filename[nam_len - ext_len], ext, ext_len) == 0) {
			return true;
		}
	}
	return false;
}

// standard PCM wave file

int DATAREC::load_wav_image()
{
	// check wave header
	wav_header_t header;
	wav_data_t data;
	
	fio->Fread(&header, sizeof(header), 1);
	if(header.format_id != 1) {
		// this is not pcm format !!!
		fio->Fclose();
		return 0;
	}
	fio->Fseek(header.fmt_size - 0x10, FILEIO_SEEK_CUR);
	fio->Fread(&data, sizeof(data), 1);
	ch = header.channels;
	sample_rate = header.sample_rate;
	sample_bits = header.sample_bits;
	
	remain = data.data_len / ch;
	if(sample_bits == 16) {
		remain /= 2;
	}
	
	// import samples
	memset(buffer, 0, sizeof(buffer));
	int samples = remain;
	for(int i = 0; i < DATAREC_BUFFER_SIZE; i++) {
		if(samples-- <= 0) {
			break;
		}
		buffer[i] = get_wav_sample();
	}
	return remain;
}

void DATAREC::save_wav_image()
{
	// write samples remained in buffer
	if(bufcnt) {
		fio->Fwrite(buffer, bufcnt, 1);
	}
	
	// write header
	uint8 wav[44];
	memcpy(wav, wavheader, sizeof(wavheader));
	int total = samples + 0x24;
	wav[ 4] = (uint8)((total >>  0) & 0xff);
	wav[ 5] = (uint8)((total >>  8) & 0xff);
	wav[ 6] = (uint8)((total >> 16) & 0xff);
	wav[ 7] = (uint8)((total >> 24) & 0xff);
	wav[40] = (uint8)((samples >>  0) & 0xff);
	wav[41] = (uint8)((samples >>  8) & 0xff);
	wav[42] = (uint8)((samples >> 16) & 0xff);
	wav[43] = (uint8)((samples >> 24) & 0xff);
	
	fio->Fseek(0, FILEIO_SEEK_SET);
	fio->Fwrite(wav, sizeof(wav), 1);
}

uint8 DATAREC::get_wav_sample()
{
	typedef union {
		int16 s16;
		struct {
			uint8 l, h;
		} b;
	} sample_pair;
	
	sample_pair data;
	data.b.l = fio->Fgetc();
	if(sample_bits == 16) {
		data.b.h = fio->Fgetc();
	}
	for(int i = 2; i <= ch; i++) {
		fio->Fgetc();
		if(sample_bits == 16) {
			fio->Fgetc();
		}
	}
	if(sample_bits == 16) {
		return (data.s16 > 0) ? 0xf0 : 0x10;
	}
	return data.b.l;
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
	int format = 0x01, skip = 0, length;
	int ptr = 0, data;
	uint8 tmp[4];
	
	// fill buffer with no signals
	memset(buffer, 0x7f, sizeof(buffer));
	
	// check header
	fio->Fread(tmp, 4, 1);
	
	if(tmp[0] == 'T' && tmp[1] == 'A' && tmp[2] == 'P' && tmp[3] == 'E') {
		// name, reserved, write protect notch
		fio->Fseek(17 + 5 + 1, FILEIO_SEEK_CUR);
		// format
		format = fio->Fgetc();
		// sample rate
		fio->Fread(tmp, 4, 1);
		sample_rate = tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);
		// data length
		fio->Fread(tmp, 4, 1);
		length = tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);
		// play position
		fio->Fread(tmp, 4, 1);
		skip = tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);
	}
	else {
		// sample rate
		sample_rate = tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);
		length = (remain - 4) * 8;
	}
	if(format != 0x01) {
		// unknow data format
		return 0;
	}
	
	// load samples
	buffer[0] = 0;
	
	while((data = fio->Fgetc()) != EOF /*&& length > 0*/) {
		for(int i = 0, bit = 0x80; i < 8; i++, bit >>= 1) {
			// skip any samples
			length--;
			if(skip > 0) {
				skip--;
				//continue;
			}
			
			// inc pointer
			bool prv = ((buffer[ptr] & 0x80) != 0);
			bool cur = ((data & bit) != 0);
			if(prv != cur || (buffer[ptr] & 0x7f) == 0x7f) {
				if((buffer[ptr] & 0x7f) == 0) {
					// don't inc pointer
				}
				else if(++ptr >= DATAREC_BUFFER_SIZE) {
					return DATAREC_BUFFER_SIZE; // buffer overflow !!!
				}
				buffer[ptr] = cur ? 0x80 : 0;
			}
			// inc pulse count
			buffer[ptr]++;
		}
	}
	return (ptr + 1);
}

// SHARP MZ series tape image

#define MZT_PUT_BIT(bit, len) { \
	for(int l = 0; l < (len); l++) { \
		if(ptr < DATAREC_BUFFER_SIZE) { \
			buffer[ptr++] = (bit) ? 0x98 : 0x8b; \
			buffer[ptr++] = (bit) ? 0x1d : 0x0f; \
		} \
	} \
}

#define MZT_PUT_BYTE(byte) { \
	MZT_PUT_BIT(1, 1); \
	for(int j = 0; j < 8; j++) { \
		if((byte) & (0x80 >> j)) { \
			MZT_PUT_BIT(1, 1); \
			count++; \
		} \
		else { \
			MZT_PUT_BIT(0, 1); \
		} \
	} \
}

#define MZT_PUT_BLOCK(buf, len) { \
	count = 0; \
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
	int ptr = 0, count;
	
	// fill buffer with no signals
	memset(buffer, 0x7f, sizeof(buffer));
	
	while(remain > 128) {
		// load header
		uint8 header[128], ram[0x20000];
		fio->Fread(header, sizeof(header), 1);
		remain -= sizeof(header);
		
		uint16 size = header[0x12] | (header[0x13] << 8);
		uint16 offs = header[0x14] | (header[0x15] << 8);
		memset(ram, 0, sizeof(ram));
		fio->Fread(ram + offs, size, 1);
		remain -= size;
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
		// output
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

