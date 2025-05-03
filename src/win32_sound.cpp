/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 sound ]
*/

#include "emu.h"
#include "vm/vm.h"
#include "fileio.h"

#define WAVEOUT_BUFFER_SIZE (DWORD)(sound_samples * 2)
#define DSOUND_BUFFER_SIZE (DWORD)(sound_samples * 4)
#define DSOUND_BUFFER_HALF (DWORD)(sound_samples * 2)

void EMU::initialize_sound(int rate, int samples)
{
	sound_rate = rate;
	sound_samples = samples;
	vm->initialize_sound(sound_rate, sound_samples);
	sound_ok = now_mute = now_recs = false;
	
#ifdef _USE_WAVEOUT
	// initialize waveOut
	ZeroMemory(wavehdr, sizeof(wavehdr));
	for(int i = 0; i < 4; i++) {
		wavehdr[i].lpData = (LPSTR)malloc(WAVEOUT_BUFFER_SIZE);
		ZeroMemory(wavehdr[i].lpData, WAVEOUT_BUFFER_SIZE);
		wavehdr[i].dwBufferLength = WAVEOUT_BUFFER_SIZE;
	}
	
	// format setting
	WAVEFORMATEX wf;
	ZeroMemory(&wf, sizeof(WAVEFORMATEX));
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = 1;
	wf.nSamplesPerSec = sound_rate;
	wf.wBitsPerSample = 16;
	wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
	
	// start play 1st and 2nd buffer
	if(waveOutOpen(&hwo, WAVE_MAPPER, &wf, (DWORD)main_window_handle, 0, CALLBACK_WINDOW) != MMSYSERR_NOERROR)
		return;
	for(int i = 0; i < 2; i++) {
		wavehdr[i].dwUser = 0;
		wavehdr[i].dwFlags = 0;
		wavehdr[i].dwLoops = 0;
		wavehdr[i].lpNext = NULL;
		wavehdr[i].reserved = 0;
		if(!waveOutPrepareHeader(hwo, &wavehdr[i], sizeof(WAVEHDR))) {
			if(!waveOutWrite(hwo, &wavehdr[i], sizeof(WAVEHDR)))
				continue;
			// failed
			waveOutUnprepareHeader(hwo, &wavehdr[i], sizeof(WAVEHDR));
		}
		wavehdr[i].dwFlags = 0;
	}
	play_block = 0;
#else
	// initialize direct sound
	PCMWAVEFORMAT pcmwf;
	DSBUFFERDESC dsbd;
	WAVEFORMATEX wfex;
	
	if(FAILED(DirectSoundCreate(NULL, &lpds, NULL)))
		return;
#ifdef _WIN32_WCE
	if(FAILED(lpds->SetCooperativeLevel(main_window_handle, DSSCL_NORMAL)))
#else
	if(FAILED(lpds->SetCooperativeLevel(main_window_handle, DSSCL_PRIORITY)))
#endif
		return;
	
	// primary buffer
	ZeroMemory(&dsbd, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	if(FAILED(lpds->CreateSoundBuffer(&dsbd, &lpdsp, NULL)))
		return;
	ZeroMemory(&wfex, sizeof(wfex));
	wfex.wFormatTag = WAVE_FORMAT_PCM;
	wfex.nChannels = 1;
	wfex.nSamplesPerSec = sound_rate;
	wfex.nBlockAlign = 2;
	wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;
	wfex.wBitsPerSample = 16;
#ifndef _WIN32_WCE
	if(FAILED(lpdsp->SetFormat(&wfex)))
		return;
#endif
	
	// secondary buffer
	ZeroMemory(&pcmwf, sizeof(pcmwf));
	pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	pcmwf.wf.nChannels = 1;
	pcmwf.wf.nSamplesPerSec = sound_rate;
	pcmwf.wf.nBlockAlign = 2;
	pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
	pcmwf.wBitsPerSample = 16;
	ZeroMemory(&dsbd, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2;
	dsbd.dwBufferBytes = DSOUND_BUFFER_SIZE;
	dsbd.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;
	if(FAILED(lpds->CreateSoundBuffer(&dsbd, &lpdsb, NULL)))
		return;
	
	// start play
	lpdsb->Play(0, 0, DSBPLAY_LOOPING);
	first_half = true;
#endif
	sound_ok = true;
}

void EMU::release_sound()
{
#ifdef _USE_WAVEOUT
	// release waveOut
	if(hwo) {
		while(waveOutReset(hwo) == MMSYSERR_HANDLEBUSY)
			Sleep(10);
		for(int i = 0; i < 4; i++) {
			if(wavehdr[i].dwFlags & WHDR_PREPARED)
				waveOutUnprepareHeader(hwo, &wavehdr[i], sizeof(WAVEHDR));
		}
		while(waveOutClose(hwo) == MMSYSERR_HANDLEBUSY)
			Sleep(10);
		hwo = NULL;
	}
	for(int i = 0; i < 4; i++)
		if(wavehdr[i].lpData)
			free(wavehdr[i].lpData);
#else
	// release direct sound
	if(lpdsp)
		lpdsp->Release();
	if(lpdsb)
		lpdsb->Release();
	if(lpds)
		lpds->Release();
	lpdsp = NULL;
	lpdsb = NULL;
	lpds = NULL;
#endif
	// stop recording
	stop_rec_sound();
}

void EMU::update_sound()
{
	now_mute = false;
	
#ifndef _USE_WAVEOUT
	if(sound_ok) {
		// check current position
		DWORD play_c, write_c, offset, size1, size2;
		WORD *ptr1, *ptr2;
		
		if(FAILED(lpdsb->GetCurrentPosition(&play_c, &write_c)))
			return;
		if(first_half) {
			if(play_c < DSOUND_BUFFER_HALF)
				return;
			offset = 0;
		}
		else {
			if(play_c > DSOUND_BUFFER_HALF)
				return;
			offset = DSOUND_BUFFER_HALF;
		}
		
		// sound buffer must be updated
		uint16* sound_buffer = vm->create_sound(0, true);
		if(now_recs) {
			// record sound
			rec->Fwrite(sound_buffer, sound_samples * 2, 1);
			rec_bufs++;
		}
		if(lpdsb->Lock(offset, DSOUND_BUFFER_HALF, (void **)&ptr1, &size1, (void**)&ptr2, &size2, 0) == DSERR_BUFFERLOST)
			lpdsb->Restore();
		if(sound_buffer) {
			if(ptr1)
				CopyMemory(ptr1, sound_buffer, size1);
			if(ptr2)
				CopyMemory(ptr2, sound_buffer + size1, size2);
		}
		lpdsb->Unlock(ptr1, size1, ptr2, size2);
		first_half = !first_half;
	}
#endif
}

#ifdef _USE_WAVEOUT
void EMU::notify_sound()
{
	if(sound_ok) {
		int prev = play_block;
		int next = (play_block + 2) & 3;
		play_block = (play_block + 1) & 3;
		
		// prepare next block
		if(now_mute)
			ZeroMemory(wavehdr[next].lpData, WAVEOUT_BUFFER_SIZE);
		else {
			// sound buffer must be updated
			uint16* sound_buffer = vm->create_sound(0, true);
			if(now_recs) {
				// record sound
				rec->Fwrite(sound_buffer, sound_samples * 2, 1);
				rec_bufs++;
			}
			CopyMemory(wavehdr[next].lpData, sound_buffer, WAVEOUT_BUFFER_SIZE);
		}
		for(int i = 0; i < 2; i++) {
			wavehdr[next].dwUser = 0;
			wavehdr[next].dwFlags = 0;
			wavehdr[next].dwLoops = 0;
			wavehdr[next].lpNext = NULL;
			wavehdr[next].reserved = 0;
			if(!waveOutPrepareHeader(hwo, &wavehdr[next], sizeof(WAVEHDR))) {
				if(!waveOutWrite(hwo, &wavehdr[next], sizeof(WAVEHDR)))
					break;
				waveOutUnprepareHeader(hwo, &wavehdr[next], sizeof(WAVEHDR));
			}
			wavehdr[next].dwFlags = 0;
		}
		
		// unprepare block
		if(wavehdr[prev].dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(hwo, &wavehdr[prev], sizeof(WAVEHDR));
		wavehdr[prev].dwFlags = 0;
	}
}
#endif

void EMU::mute_sound()
{
#ifndef _USE_WAVEOUT
	if(!now_mute && sound_ok) {
		// check current position
		DWORD size1, size2;
		WORD *ptr1, *ptr2;
		
		if(lpdsb->Lock(0, DSOUND_BUFFER_SIZE, (void **)&ptr1, &size1, (void**)&ptr2, &size2, 0) == DSERR_BUFFERLOST)
			lpdsb->Restore();
		if(ptr1)
			ZeroMemory(ptr1, size1);
		if(ptr2)
			ZeroMemory(ptr2, size2);
		lpdsb->Unlock(ptr1, size1, ptr2, size2);
	}
#endif
	now_mute = true;
}

void EMU::start_rec_sound()
{
	if(!now_recs) {
		_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
		application_path(app_path);
		_stprintf(file_path, _T("%ssound.tmp"), app_path);
		
		rec = new FILEIO();
		if(rec->Fopen(file_path, FILEIO_WRITE_BINARY)) {
			// write wave header
			rec_bufs = 0;
			now_recs = true;
		}
		else {
			// failed to open the wave file
			delete rec;
		}
	}
}

void EMU::stop_rec_sound()
{
	if(now_recs) {
		rec->Fclose();
		delete rec;
		now_recs = false;
		
		_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH], tmp_path[_MAX_PATH];
		application_path(app_path);
		_stprintf(file_path, _T("%ssound.wav"), app_path);
		_stprintf(tmp_path, _T("%ssound.tmp"), app_path);
		
		FILEIO* out = new FILEIO();
		if(!rec->Fopen(file_path, FILEIO_WRITE_BINARY)) {
			delete out;
			return;
		}
		FILEIO* tmp = new FILEIO();
		tmp->Fopen(tmp_path, FILEIO_READ_BINARY);
		
		// write header
		uint32 length = rec_bufs * sound_samples * 4;
		struct wavheader_t header;
		header.dwRIFF = 0x46464952;
		header.dwFileSize = length + sizeof(wavheader_t) - 8;
		header.dwWAVE = 0x45564157;
		header.dwfmt_ = 0x20746d66;
		header.dwFormatSize = 16;
		header.wFormatTag = 1;
		header.wChannels = 2;
		header.dwSamplesPerSec = sound_rate;
		header.dwAvgBytesPerSec = sound_rate * 4;
		header.wBlockAlign = 4;
		header.wBitsPerSample = 16;
		header.dwdata = 0x61746164;
		header.dwDataLength = length;
		out->Fwrite(&header, sizeof(wavheader_t), 1);
		
		// convert mono to stereo
		uint16* buf_t = (uint16*)malloc(sound_samples * 2);
		uint16* buf_o = (uint16*)malloc(sound_samples * 4);
		for(int i = 0; i < rec_bufs; i++) {
			tmp->Fread(buf_t, sound_samples * 2, 1);
			for(int j = 0; j < sound_samples; j++)
				buf_o[j * 2] = buf_o[j * 2 + 1] = buf_t[j];
			out->Fwrite(buf_o, sound_samples * 4, 1);
		}
		free(buf_t);
		free(buf_o);
		
		out->Fclose();
		delete out;
		tmp->Fclose();
		tmp->Remove(tmp_path);
		delete tmp;
	}
}

void EMU::restart_rec_sound()
{
	if(now_recs) {
		rec->Fclose();
		delete rec;
		now_recs = false;
		
		start_rec_sound();
	}
}

