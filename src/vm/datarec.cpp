/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ data recorder ]
*/

#include "datarec.h"
#include "../fileio.h"

void DATAREC::initialize()
{
	// data recorder
	fio = new FILEIO();
	_memset(buffer, 0, sizeof(buffer));
	bufcnt = samples = 0;
	
	regist_id = -1;
	play = rec = false;
	in = out = change = remote = false;
	is_wave = false;
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
	if(id == SIG_DATAREC_OUT) {
		bool signal = ((data & mask) != 0);
		if(rec && remote && signal != out)
			change = true;
		out = signal;
	}
	else if(id == SIG_DATAREC_REMOTE) {
		remote = ((data & mask) != 0);
		update_event();
	}
}

void DATAREC::event_callback(int event_id, int err)
{
	if(play && remote) {
		// get the next signal
		bool signal = ((buffer[bufcnt] & 0x80) != 0);
		if(is_wave) {
			// inc pointer
			if(++bufcnt >= BUFFER_SIZE) {
				_memset(buffer, 0, sizeof(buffer));
				fio->Fread(buffer, sizeof(buffer), 1);
				bufcnt = 0;
			}
			if(remain)
				remain--;
			update_event();
		}
		else {
			// inc pointer
			while(!(buffer[bufcnt] & 0x7f)) {
				if(++bufcnt >= BUFFER_SIZE) {
					_memset(buffer, 0x7f, sizeof(buffer));
					fio->Fread(buffer, sizeof(buffer), 1);
					bufcnt = 0;
				}
				signal = ((buffer[bufcnt] & 0x80) != 0);
				if(remain)
					remain--;
				update_event();
			}
			// dec pulse count
			uint8 tmp = buffer[bufcnt];
			buffer[bufcnt] = (tmp & 0x80) | ((tmp & 0x7f) - 1);
		}
		// notify the signal is changed
		if(signal != in) {
			for(int i = 0; i < dcount; i++)
				dev[i]->write_signal(did[i], signal ? 0xffffffff : 0, dmask[i]);
			change = true;
			in = signal;
		}
	}
	else if(rec && remote) {
		if(is_wave) {
			buffer[bufcnt] = out ? 0xf0 : 0x10;
			samples++;
			// inc pointer
			if(++bufcnt >= BUFFER_SIZE) {
				fio->Fwrite(buffer, sizeof(buffer), 1);
				bufcnt = 0;
			}
		}
		else {
			// inc pointer
			bool prv = ((buffer[bufcnt] & 0x80) != 0);
			if(prv != out || (buffer[bufcnt] & 0x7f) == 0x7f) {
				if(++bufcnt >= BUFFER_SIZE) {
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

void DATAREC::play_datarec(_TCHAR* filename)
{
	close_datarec();
	
	if(fio->Fopen(filename, FILEIO_READ_BINARY)) {
		// get file size
		fio->Fseek(0, FILEIO_SEEK_END);
		remain = fio->Ftell();
		fio->Fseek(0, FILEIO_SEEK_SET);
		
		// open for play
		is_wave = check_extension(filename);
		if(is_wave) {
			fio->Fseek(sizeof(header), FILEIO_SEEK_SET);
			remain -= sizeof(header);
			
			_memset(buffer, 0, sizeof(buffer));
			fio->Fread(buffer, sizeof(buffer), 1);
		}
		else {
			_memset(buffer, 0x7f, sizeof(buffer));
			fio->Fread(buffer, sizeof(buffer), 1);
		}
		bufcnt = samples = 0;
		// get the first signal
		bool signal = ((buffer[0] & 0x80) != 0);
		// notify the signal is changed
		if(signal != in) {
			for(int i = 0; i < dcount; i++)
				dev[i]->write_signal(did[i], signal ? 0xffffffff : 0, dmask[i]);
			in = signal;
		}
		play = true;
		update_event();
	}
}

void DATAREC::rec_datarec(_TCHAR* filename)
{
	close_datarec();
	
	if(fio->Fopen(filename, FILEIO_WRITE_BINARY)) {
		// check file extension
		is_wave = check_extension(filename);
		
		// open for rec
		if(is_wave) {
			// write wave header
			fio->Fwrite(header, sizeof(header), 1);
		}
		else {
			// initialize buffer
			buffer[0] = out ? 0x80 : 0;
		}
		bufcnt = samples = 0;
		rec = true;
		update_event();
	}
}

void DATAREC::close_datarec()
{
	// close file
	if(rec) {
		if(is_wave) {
			if(bufcnt)
				fio->Fwrite(buffer, bufcnt, 1);
			// write header
			uint8 wav[44];
			_memcpy(wav, header, sizeof(header));
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
		else
			fio->Fwrite(buffer, bufcnt + 1, 1);
	}
	if(play || rec)
		fio->Fclose();
	play = rec = false;
	update_event();
	
	// no sounds
	for(int i = 0; i < dcount; i++)
		dev[i]->write_signal(did[i], 0, dmask[i]);
	in = false;
}

void DATAREC::update_event()
{
	if(remote && ((play && remain > 0) || rec)) {
		if(regist_id == -1)
			vm->regist_event(this, 0, 21, true, &regist_id);
	}
	else {
		if(regist_id != -1)
			vm->cancel_event(regist_id);
		regist_id = -1;
	}
}

bool DATAREC::check_extension(_TCHAR* filename)
{
	int pt = _tcslen(filename);
	if(pt >= 4) {
		bool tmp = (filename[pt - 4] == _T('.'));
		tmp &= (filename[pt - 3] == _T('W') || filename[pt - 3] == _T('w'));
		tmp &= (filename[pt - 2] == _T('A') || filename[pt - 2] == _T('a'));
		tmp &= (filename[pt - 1] == _T('V') || filename[pt - 1] == _T('v'));
		return tmp;
	}
	return false;
}

bool DATAREC::skip()
{
	bool val = change;
	change = false;
	return val;
}
