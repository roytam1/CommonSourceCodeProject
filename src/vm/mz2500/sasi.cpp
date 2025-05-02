/*
	SHARP MZ-2500 Emulator 'EmuZ-2500'
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2004.09.10 -

	[ sasi hdd ]
*/

#include "sasi.h"
#include "../../fileio.h"

void SASI::initialize()
{
	// hdd image file path
	_TCHAR app_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	// check file existance
	file_exist[0] = file_exist[1] = false;
	_stprintf(file_path[0], _T("%sHDD1.DAT"), app_path);
	if(fio->Fopen(file_path[0], FILEIO_READ_BINARY)) {
		file_exist[0] = true;
		fio->Fclose();
	}
	_stprintf(file_path[1], _T("%sHDD2.DAT"), app_path);
	if(fio->Fopen(file_path[1], FILEIO_READ_BINARY)) {
		file_exist[1] = true;
		fio->Fclose();
	}
	delete fio;
	
	// initialize sasi interface
	_memset(buffer, 0, sizeof(buffer));
	_memset(cmd, 0, sizeof(cmd));
	_memset(state_buf, 0, sizeof(state_buf));
	
	phase = 0;
	sector = 0;
	blocks = 0;
	cmd_ptr = 0;
	device = 0;
	unit = 0;
	buffer_ptr = 0;
	rw_mode = 0;
	state = 0;
	error = 0;
	state_ptr = 0;
}

void SASI::write_io8(uint32 addr, uint32 data)
{
	switch(addr & 0xff)
	{
	case 0xa4:
		// data
		if(phase == 2) {
			cmd[cmd_ptr++] = data;
			if(cmd_ptr == 6)
				check_cmd();
		}
		else if(phase == 3 && !rw_mode) {
			buffer[buffer_ptr++] = data;
			if(buffer_ptr == 256) {
				flush(unit);
				if(--blocks) {
					sector++;
					buffer_ptr = 0;
					if(!seek(unit)) {
						error = 0x0f;
						phase++;
					}
				}
				else
					phase++;
			}
		}
		else if(phase == 10) {
			if(++state_ptr == 10)
				phase = 4;
		}
		datareg = data;
		break;
	case 0xa5:
		// cmd
		if(data == 0x00) {
			if(phase == 1)
				phase = (device == 0) ? 2 : 0;
		}
		else if(data == 0x20) {
			device = (datareg & 1) ? 0 : 0x7f;
			if(device == 0) {
				phase = 1;
				cmd_ptr = 0;
			}
			else
				phase = 0;
		}
		break;
	}
}

uint32 SASI::read_io8(uint32 addr)
{
	uint32 val = 0;
	
	switch(addr & 0xff)
	{
	case 0xa4:
		// data
		if(phase == 3 && rw_mode) {
			val = buffer[buffer_ptr++];
			if(buffer_ptr == 256) {
				if(--blocks) {
					sector++;
					buffer_ptr = 0;
					if(!seek(unit)) {
						error = 0x0f;
						phase++;
					}
				}
				else
					phase++;
			}
		}
		else if(phase == 4) {
			val = error ? 0x02 : state;
			phase++;
		}
		else if(phase == 5)
			phase = 0;
		else if(phase == 9) {
			val = state_buf[state_ptr++];
			if(state_ptr == 4) {
				error = 0;
				phase = 4;
			}
		}
		return val;
	case 0xa5:
		// status
		if(phase)
			val |= 0x20;	// busy
		if(phase > 1)
			val |= 0x80;	// req
		if(phase == 2)
			val |= 0x08;	// c/d
		if(phase == 3 && rw_mode)
			val |= 0x04;	// i/o
		if(phase == 9)
			val |= 0x04;	// i/o
		if(phase == 4 || phase == 5)
			val |= 0x0c;	// i/o & c/d
		if(phase == 5)
			val |= 0x10;	// msg
		return val;
	}
	return 0xff;
}

int SASI::seek(int drv)
{
	_memset(buffer, 0, sizeof(buffer));
	
	if(!file_exist[drv & 1])
		return -1;
	FILEIO* fio = new FILEIO();
	if(!fio->Fopen(file_path[drv & 1], FILEIO_READ_BINARY)) {
		delete fio;
		return -1;
	}
	if(fio->Fseek(sector * 256, FILEIO_SEEK_SET) != 0) {
		fio->Fclose();
		delete fio;
		return 0;
	}
	if(fio->Fread(buffer, 256, 1) != 1) {
		fio->Fclose();
		delete fio;
		return 0;
	}
	fio->Fclose();
	delete fio;
	return 1;
}

int SASI::flush(int drv)
{
	if(!file_exist[drv & 1])
		return -1;
	FILEIO* fio = new FILEIO();
	if(!fio->Fopen(file_path[drv & 1], FILEIO_READ_WRITE_BINARY)) {
		delete fio;
		return -1;
	}
	if(fio->Fseek(sector * 256, FILEIO_SEEK_SET) != 0) {
		fio->Fclose();
		delete fio;
		return 0;
	}
	if(fio->Fwrite(buffer, 256, 1) != 1) {
		fio->Fclose();
		delete fio;
		return 0;
	}
	fio->Fclose();
	delete fio;
	return 1;
}

int SASI::format(int drv)
{
	if(!file_exist[drv & 1])
		return -1;
	FILEIO* fio = new FILEIO();
	if(!fio->Fopen(file_path[drv & 1], FILEIO_READ_WRITE_BINARY)) {
		delete fio;
		return -1;
	}
	if(fio->Fseek(sector * 256, FILEIO_SEEK_SET) != 0) {
		fio->Fclose();
		delete fio;
		return 0;
	}
	// format 33 blocks
	_memset(buffer, 0, sizeof(buffer));
	for(int i = 0; i < 33; i++) {
		if(fio->Fwrite(buffer, 256, 1) != 1) {
			fio->Fclose();
			delete fio;
			return 0;
		}
	}
	fio->Fclose();
	delete fio;
	return 1;
}

void SASI::check_cmd()
{
	int result;
	unit = (cmd[1] >> 5) & 1;
	
	switch(cmd[0])
	{
	case 0x00:
		if(device == 0) {
			state = 0;
			error = 0;
		}
		else {
			state = 0x02;
			error = 0x7f;
		}
		phase = 4;
		break;
	case 0x01:
		if(device == 0) {
			sector = 0;
			state = 0;
		}
		else {
			state = 0x02;
			error = 0x7f;
		}
		phase = 4;
		break;
	case 0x03:
		state_buf[0] = error;
		state_buf[1] = (uint8)((unit << 5) | ((sector >> 16) & 0x1f));
		state_buf[2] = (uint8)(sector >> 8);
		state_buf[3] = (uint8)sector;
		error = 0;
		phase = 9;
		state = 0;
		state_ptr = 0;
		break;
	case 0x04:
		phase = 4;
		state = 0;
		break;
	case 0x06:
		error = 0;
		state = 0;
		phase = 4;
		sector = cmd[1] & 0x1f;
		sector = (sector << 8) | cmd[2];
		sector = (sector << 8) | cmd[3];
		result = format(unit);
		if((result == 0) || (result == -1)) {
			state = 0x02;
			error = 0x7f;
		}
		break;
	case 0x08:
		sector = cmd[1] & 0x1f;
		sector = (sector << 8) | cmd[2];
		sector = (sector << 8) | cmd[3];
		blocks = cmd[4];
		phase = 3;
		rw_mode = 1;
		buffer_ptr = 0;
		state = 0;
		result = seek(unit);
		if((result == 0) || (result == -1))
			error = 0x0f;
		break;
	case 0x0a:
		sector = cmd[1] & 0x1f;
		sector = (sector << 8) | cmd[2];
		sector = (sector << 8) | cmd[3];
		blocks = cmd[4];
		phase = 3;
		rw_mode = 0;
		buffer_ptr = 0;
		state = 0;
		_memset(buffer, 0, sizeof(buffer));
		result = seek(unit);
		if((result == 0) || (result == -1))
			error = 0x0f;
		break;
	case 0x0b:
		if(device == 0) {
			state = 0;
			error = 0;
		}
		else {
			state = 0x02;
			error = 0x7f;
		}
		phase = 4;
		break;
	case 0xc2:
		phase = 10;
		state_ptr = 0;
		if(device == 0) {
			state = 0;
			error = 0;
		}
		else {
			state = 0x02;
			error = 0x7f;
		}
		break;
	default:
		phase = 4;
	}
}

