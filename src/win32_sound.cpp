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
	sound_ok = now_mute = now_recs = FALSE;
	
	// initialize direct sound
	PCMWAVEFORMAT pcmwf;
	DSBUFFERDESC dsbd;
	WAVEFORMATEX wfex;
	
	if(FAILED(DirectSoundCreate(NULL, &lpds, NULL))) {
		return;
	}
	if(FAILED(lpds->SetCooperativeLevel(main_window_handle, DSSCL_PRIORITY))) {
		return;
	}
	
	// primary buffer
	ZeroMemory(&dsbd, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	if(FAILED(lpds->CreateSoundBuffer(&dsbd, &lpdsp, NULL))) {
		return;
	}
	ZeroMemory(&wfex, sizeof(wfex));
	wfex.wFormatTag = WAVE_FORMAT_PCM;
	wfex.nChannels = 1;
	wfex.nSamplesPerSec = sound_rate;
	wfex.nBlockAlign = 2;
	wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;
	wfex.wBitsPerSample = 16;
	if(FAILED(lpdsp->SetFormat(&wfex))) {
		return;
	}
	
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
	if(FAILED(lpds->CreateSoundBuffer(&dsbd, &lpdsb, NULL))) {
		return;
	}
	
	// start play
	lpdsb->Play(0, 0, DSBPLAY_LOOPING);
	sound_ok = first_half = TRUE;
}

void EMU::release_sound()
{
	// release direct sound
	if(lpdsp) {
		lpdsp->Release();
	}
	if(lpdsb) {
		lpdsb->Release();
	}
	if(lpds) {
		lpds->Release();
	}
	lpdsp = NULL;
	lpdsb = NULL;
	lpds = NULL;
	
	// stop recording
	stop_rec_sound();
}

void EMU::update_sound()
{
	now_mute = FALSE;
	
	if(sound_ok) {
		// check current position
		DWORD play_c, write_c, offset, size1, size2;
		WORD *ptr1, *ptr2;
		
		if(FAILED(lpdsb->GetCurrentPosition(&play_c, &write_c))) {
			return;
		}
		if(first_half) {
			if(play_c < DSOUND_BUFFER_HALF) {
				return;
			}
			offset = 0;
		}
		else {
			if(play_c > DSOUND_BUFFER_HALF) {
				return;
			}
			offset = DSOUND_BUFFER_HALF;
		}
		
		// sound buffer must be updated
		uint16* sound_buffer = vm->create_sound(0, true);
		if(now_recs) {
			// record sound
			rec->Fwrite(sound_buffer, sound_samples * 2, 1);
			rec_bufs++;
		}
		if(lpdsb->Lock(offset, DSOUND_BUFFER_HALF, (void **)&ptr1, &size1, (void**)&ptr2, &size2, 0) == DSERR_BUFFERLOST) {
			lpdsb->Restore();
		}
		if(sound_buffer) {
			if(ptr1) {
				CopyMemory(ptr1, sound_buffer, size1);
			}
			if(ptr2) {
				CopyMemory(ptr2, sound_buffer + size1, size2);
			}
		}
		lpdsb->Unlock(ptr1, size1, ptr2, size2);
		first_half = !first_half;
	}
}

void EMU::mute_sound()
{
	if(!now_mute && sound_ok) {
		// check current position
		DWORD size1, size2;
		WORD *ptr1, *ptr2;
		
		if(lpdsb->Lock(0, DSOUND_BUFFER_SIZE, (void **)&ptr1, &size1, (void**)&ptr2, &size2, 0) == DSERR_BUFFERLOST) {
			lpdsb->Restore();
		}
		if(ptr1) {
			ZeroMemory(ptr1, size1);
		}
		if(ptr2) {
			ZeroMemory(ptr2, size2);
		}
		lpdsb->Unlock(ptr1, size1, ptr2, size2);
	}
	now_mute = TRUE;
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
			now_recs = TRUE;
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
		now_recs = FALSE;
		
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
			for(int j = 0; j < sound_samples; j++) {
				buf_o[j * 2] = buf_o[j * 2 + 1] = buf_t[j];
			}
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
		now_recs = FALSE;
		
		start_rec_sound();
	}
}

