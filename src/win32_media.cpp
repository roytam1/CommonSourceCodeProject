/*
	Skelton for Z-80 PC Emulator

	Author : Takeda.Toshiya
	Date   : 2004.09.05 -

	[ win32 media ]
*/

#include <stdio.h>
#include "emu.h"

void EMU::initialize_media()
{
	media_cnt = 0;
	media_playing = false;
}

void EMU::release_media()
{
	close_media();
}

void EMU::open_media(_TCHAR* filename)
{
	close_media();
	
	// get media root path
	_TCHAR root[_MAX_PATH], path[_MAX_PATH], tmp[_MAX_PATH];
	_tcscpy(root, filename);
	int pt = _tcslen(root);
	while(root[pt] != '\\') {
		pt--;
	}
	root[pt + 1] = '\0';
	
	// get file path
	FILE* fp = fopen(filename, "r");
	if(fp != NULL) {
		while(_fgetts(tmp, _MAX_PATH, fp) != NULL) {
			int l = _tcslen(tmp) - 1;
			if(l > 0) {
				tmp[l] = (tmp[l] == '\n') ? '\0' : tmp[l];
				_stprintf(path, "%s%s", root, tmp);
				_tcscpy(media_path[media_cnt++], path);
			}
			if(media_cnt >= MEDIA_MAX) {
				break;
			}
		}
		fclose(fp);
	}
}

void EMU::close_media()
{
	stop_media();
	media_cnt = 0;
}

int EMU::media_count()
{
	return media_cnt;
}

void EMU::play_media(int trk)
{
	stop_media();
	
	if(1 <= trk && trk <= media_cnt) {
		_TCHAR cmd[_MAX_PATH];
		_stprintf(cmd, _T("open \"%s\" alias tape"), media_path[trk - 1]);
		mciSendString(cmd, NULL, 0, NULL);
		mciSendString(_T("play tape notify"), NULL, 0, main_window_handle);
		media_playing = true;
	}
}

void EMU::stop_media()
{
	if(media_playing) {
		mciSendString(_T("stop tape"), NULL, 0, NULL);
		mciSendString(_T("close tape"), NULL, 0, NULL);
		media_playing = false;
	}
}

