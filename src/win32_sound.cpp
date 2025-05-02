/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 sound ]
*/

#include "emu.h"
#include "vm/vm.h"
#include "fileio.h"

#define DSOUND_BUFFER_SIZE (DWORD)(sound_samples * 8)
#define DSOUND_BUFFER_HALF (DWORD)(sound_samples * 4)

void EMU::initialize_sound()
{
	sound_ok = sound_started = now_mute = now_rec_snd = false;
	
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
	wfex.nChannels = 2;
	wfex.wBitsPerSample = 16;
	wfex.nSamplesPerSec = sound_rate;
	wfex.nBlockAlign = wfex.nChannels * wfex.wBitsPerSample / 8;
	wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;
	if(FAILED(lpdsp->SetFormat(&wfex))) {
		return;
	}
	
	// secondary buffer
	ZeroMemory(&pcmwf, sizeof(pcmwf));
	pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	pcmwf.wf.nChannels = 2;
	pcmwf.wBitsPerSample = 16;
	pcmwf.wf.nSamplesPerSec = sound_rate;
	pcmwf.wf.nBlockAlign = pcmwf.wf.nChannels * pcmwf.wBitsPerSample / 8;
	pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
	ZeroMemory(&dsbd, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2;
	dsbd.dwBufferBytes = DSOUND_BUFFER_SIZE;
	dsbd.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;
	if(FAILED(lpds->CreateSoundBuffer(&dsbd, &lpdsb, NULL))) {
		return;
	}
	
	sound_ok = first_half = true;
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

void EMU::update_sound(int* extra_frames)
{
	*extra_frames = 0;
	now_mute = false;
	
	if(sound_ok) {
		DWORD play_c, write_c, offset, size1, size2;
		WORD *ptr1, *ptr2;
		
		// start play
		if(!sound_started) {
			lpdsb->Play(0, 0, DSBPLAY_LOOPING);
			sound_started = true;
			return;
		}
		
		// check current position
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
		uint16* sound_buffer = vm->create_sound(extra_frames);
		if(now_rec_snd) {
			// record sound
			int length = sound_samples * sizeof(uint16) * 2; // stereo
			rec->Fwrite(sound_buffer, length, 1);
			rec_bytes += length;
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
	now_mute = true;
}

void EMU::start_rec_sound()
{
	if(!now_rec_snd) {
		rec = new FILEIO();
		if(rec->Fopen(bios_path(_T("sound.wav")), FILEIO_WRITE_BINARY)) {
			// write dummy wave header
			struct wavheader_t header;
			memset(&header, 0, sizeof(wavheader_t));
			rec->Fwrite(&header, sizeof(wavheader_t), 1);
			rec_bytes = 0;
			now_rec_snd = true;
		}
		else {
			// failed to open the wave file
			delete rec;
		}
	}
}

void EMU::stop_rec_sound()
{
	if(now_rec_snd) {
		// update wave header
		struct wavheader_t header;
		header.dwRIFF = 0x46464952;
		header.dwFileSize = rec_bytes + sizeof(wavheader_t) - 8;
		header.dwWAVE = 0x45564157;
		header.dwfmt_ = 0x20746d66;
		header.dwFormatSize = 16;
		header.wFormatTag = 1;
		header.wChannels = 2;
		header.wBitsPerSample = 16;
		header.dwSamplesPerSec = sound_rate;
		header.wBlockAlign = header.wChannels * header.wBitsPerSample / 8;
		header.dwAvgBytesPerSec = header.dwSamplesPerSec * header.wBlockAlign;
		header.dwdata = 0x61746164;
		header.dwDataLength = rec_bytes;
		
		rec->Fseek(0, FILEIO_SEEK_SET);
		rec->Fwrite(&header, sizeof(wavheader_t), 1);
		rec->Fclose();
		
		delete rec;
		now_rec_snd = false;
	}
}

void EMU::restart_rec_sound()
{
	if(now_rec_snd) {
		rec->Fclose();
		delete rec;
		now_rec_snd = false;
		
		start_rec_sound();
	}
}

