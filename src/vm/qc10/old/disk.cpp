/*
	Skelton for Z-80 PC Emulator

	Author : Takeda.Toshiya
	Date   : 2004.08.30 -

	[ d88 handler ]
*/

#include "disk.h"
#include "../fileio.h"

DISK::DISK()
{
	insert = protect = false;
	file_size = 0;
	track_size = 0x1800;
	sector_size = sector_num = 0;
	sector = NULL;
}

DISK::~DISK()
{
	if(insert)
		close();
}

void DISK::open(_TCHAR path[])
{
	// eject disk if inserted
	if(insert)
		close();
	
	// open disk image
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(path, FILEIO_READ_BINARY)) {
		// file size, file name
		fio->Fseek(0, FILEIO_SEEK_END);
		file_size = fio->Ftell();
		fio->Fseek(0, FILEIO_SEEK_SET);
		_tcscpy(file_path, path);
		
		// check file size
		if(0 < file_size && file_size <= DISK_BUFFER_SIZE) {
			fio->Fread(buf, file_size, 1);
			insert = true;
		}
		fio->Fclose();
		protect = fio->IsProtected(path);
	}
	delete fio;
}

void DISK::close()
{
	// write disk image
	if(insert && !protect && file_size) {
		// write image
		FILEIO* fio = new FILEIO();
		if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
			fio->Fwrite(buf, file_size, 1);
			fio->Fclose();
		}
		delete fio;
	}
	insert = protect = false;
	file_size = 0;
	sector_size = sector_num = 0;
	sector = NULL;
}

bool DISK::get_track(int trk, int side)
{
	sector_size = sector_num = 0;
	
	// disk not inserted
	if(!insert)
		return false;
	
	// search track
	if(trk < 0 || trk > 83)
		return false;
	
	int trkside = trk * 2 + (side & 1);
	uint32 offset = buf[0x20 + trkside * 4 + 0];
	offset |= buf[0x20 + trkside * 4 + 1] << 8;
	offset |= buf[0x20 + trkside * 4 + 2] << 16;
	offset |= buf[0x20 + trkside * 4 + 3] << 24;
	
	if(!offset)
		return false;
	
	// track found
	uint8* t = buf + offset;
	sector_num = t[4] | (t[5] << 8);
	
	for(int i = 0; i < sector_num; i++) {
		verify[i] = t[0];
		t += (t[0xe] | (t[0xf] << 8)) + 0x10;
	}
	return true;
}

bool DISK::make_track(int trk, int side)
{
	sector_size = sector_num = 0;
	
	// create dummy track
	for(int i = 0; i < 0x1800; i++)
		track[i] = rand();
	track_size = 0x1800;
	
	// disk not inserted
	if(!insert)
		return false;
	
	// search track
	if(trk < 0 || trk > 83)
		return false;
	
	int trkside = trk * 2 + (side & 1);
	uint32 offset = buf[0x20 + trkside * 4 + 0];
	offset |= buf[0x20 + trkside * 4 + 1] << 8;
	offset |= buf[0x20 + trkside * 4 + 2] << 16;
	offset |= buf[0x20 + trkside * 4 + 3] << 24;
	
	if(!offset)
		return false;
	
	// get verify info
	uint8* t = buf + offset;
	sector_num = t[4] | (t[5] << 8);
	
	// make track image
	int gap3 = (sector_num <= 5) ? 0x74 : (sector_num <= 10) ? 0x54 : (sector_num <= 16) ? 0x33 : 0x10;
	int p = 0;
	
	// gap0
	for(int i = 0; i < 80; i++)
		track[p++] = 0x4e;
	// sync
	for(int i = 0; i < 12; i++)
		track[p++] = 0;
	// index mark
	track[p++] = 0xc2;
	track[p++] = 0xc2;
	track[p++] = 0xc2;
	track[p++] = 0xfc;
	// gap1
	for(int i = 0; i < 50; i++)
		track[p++] = 0x4e;
	// sectors
	for(int i = 0; i < sector_num; i ++) {
		// sync
		for(int j = 0; j < 12; j++)
			track[p++] = 0;
		// id address mark
		track[p++] = 0xa1;
		track[p++] = 0xa1;
		track[p++] = 0xa1;
		track[p++] = 0xfe;
		track[p++] = t[0];
		track[p++] = t[1];
		track[p++] = t[2];
		track[p++] = t[3];
		uint16 crc = 0;
		crc = (uint16)((crc << 8) ^ crc_table[(uint8)(crc >> 8) ^ t[0]]);
		crc = (uint16)((crc << 8) ^ crc_table[(uint8)(crc >> 8) ^ t[1]]);
		crc = (uint16)((crc << 8) ^ crc_table[(uint8)(crc >> 8) ^ t[2]]);
		crc = (uint16)((crc << 8) ^ crc_table[(uint8)(crc >> 8) ^ t[3]]);
		track[p++] = crc >> 8;
		track[p++] = crc & 0xff;
		// gap2
		for(int j = 0; j < 22; j++)
			track[p++] = 0x4e;
		// sync
		for(int j = 0; j < 12; j++)
			track[p++] = 0;
		// data mark, deleted mark
		track[p++] = 0xa1;
		track[p++] = 0xa1;
		track[p++] = 0xa1;
		track[p++] = (t[7]) ? 0xf8 : 0xfb;
		// data
		int size = t[0xe] | (t[0xf] << 8);
		crc = 0;
		for(int j = 0; j < size; j++) {
			track[p++] = t[0x10 + j];
			crc = (uint16)((crc << 8) ^ crc_table[(uint8)(crc >> 8) ^ t[0x10 + j]]);
		}
		track[p++] = crc >> 8;
		track[p++] = crc & 0xff;
		t += size + 0x10;
		// gap3
		for(int j = 0; j < gap3; j++)
			track[p++] = 0x4e;
	}
	// gap4
	while(p < 0x1800)
		track[p++] = 0x4e;
	track_size = p;
	
	return true;
}

bool DISK::get_sector(int trk, int side, int index)
{
	sector_size = sector_num = 0;
	sector = NULL;
	
	// disk not inserted
	if(!insert)
		return false;
	
	// search track
	if(trk < 0 || trk > 83)
		return false;
	
	int trkside = trk * 2 + (side & 1);
	uint32 offset = buf[0x20 + trkside * 4 + 0];
	offset |= buf[0x20 + trkside * 4 + 1] << 8;
	offset |= buf[0x20 + trkside * 4 + 2] << 16;
	offset |= buf[0x20 + trkside * 4 + 3] << 24;
	
	if(!offset)
		return false;
	
	// track found
	uint8* t = buf + offset;
	sector_num = t[4] | (t[5] << 8);
	
	if(index >= sector_num)
		return false;
	
	// skip sector
	for(int i = 0; i < index; i++)
		t += (t[0xe] | (t[0xf] << 8)) + 0x10;
	
	// header info
	id[0] = t[0];
	id[1] = t[1];
	id[2] = t[2];
	id[3] = t[3];
	uint16 crc = 0;
	crc = (uint16)((crc << 8) ^ crc_table[(uint8)(crc >> 8) ^ t[0]]);
	crc = (uint16)((crc << 8) ^ crc_table[(uint8)(crc >> 8) ^ t[1]]);
	crc = (uint16)((crc << 8) ^ crc_table[(uint8)(crc >> 8) ^ t[2]]);
	crc = (uint16)((crc << 8) ^ crc_table[(uint8)(crc >> 8) ^ t[3]]);
	id[4] = crc >> 8;
	id[5] = crc & 0xff;
	density = t[6];
	deleted = t[7];
	status = t[8];
	sector = t + 0x10;
	sector_size = t[0xe] | (t[0xf] << 8);
	
	return true;
}

