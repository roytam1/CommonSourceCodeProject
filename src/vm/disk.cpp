/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.16-

	[ d88 handler ]
*/

#include "disk.h"
#include "../fileio.h"

DISK::DISK()
{
	insert = protect = change = false;
	file_size = 0;
	track_size = 0x1800;
	sector_size = sector_num = 0;
	sector = NULL;
}

DISK::~DISK()
{
	if(insert) {
		close();
	}
}

void DISK::open(_TCHAR path[])
{
	// check current disk image
	if(insert) {
		if(_tcsicmp(file_path, path) == 0) {
			return;
		}
		close();
	}
	
	// open disk image
	fi = new FILEIO();
	if(fi->Fopen(path, FILEIO_READ_BINARY)) {
		_tcscpy(file_path, path);
		_stprintf(tmp_path, _T("%s.$$$"), path);
		temporary = false;
		
		fi->Fseek(0, FILEIO_SEEK_END);
		file_size = fi->Ftell();
		fi->Fseek(0, FILEIO_SEEK_SET);
		protect = fi->IsProtected(path);
		
		// check file size
		if(0 < file_size && file_size <= DISK_BUFFER_SIZE) {
			fi->Fread(buffer, file_size, 1);
			insert = change = true;
			
			// check file format
			if((buffer[0] == 'T' && buffer[1] == 'D') || (buffer[0] == 't' && buffer[1] == 'd')) {
				// this is teledisk image and must be converted to d88
				insert = td2d();
				_stprintf(file_path, _T("%s.D88"), path);
			}
			else if(buffer[0] == 'I' && buffer[1] == 'M' && buffer[2] == 'D') {
				// this is imagedisk image and must be converted to d88
				insert = imd2d();
				_stprintf(file_path, _T("%s.D88"), path);
			}
		}
		fi->Fclose();
		if(temporary) {
			fi->Remove(tmp_path);
		}
		media_type = buffer[0x1b];
	}
	delete fi;
}

void DISK::close()
{
	// write disk image
	if(insert && !protect && file_size) {
		// write image
		FILEIO* fio = new FILEIO();
		if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
			fio->Fwrite(buffer, file_size, 1);
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
	if(!insert) {
		return false;
	}
	
	// search track
	int trkside = trk * 2 + (side & 1);
	if(!(0 <= trkside && trkside < 164)) {
		return false;
	}
	uint32 offset = buffer[0x20 + trkside * 4 + 0];
	offset |= buffer[0x20 + trkside * 4 + 1] << 8;
	offset |= buffer[0x20 + trkside * 4 + 2] << 16;
	offset |= buffer[0x20 + trkside * 4 + 3] << 24;
	
	if(!offset) {
		return false;
	}

	// track found
	uint8* t = buffer + offset;
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
	for(int i = 0; i < 0x1800; i++) {
		track[i] = rand();
	}
	track_size = 0x1800;
	
	// disk not inserted
	if(!insert) {
		return false;
	}
	
	// search track
	int trkside = trk * 2 + (side & 1);
	if(!(0 <= trkside && trkside < 164)) {
		return false;
	}
	uint32 offset = buffer[0x20 + trkside * 4 + 0];
	offset |= buffer[0x20 + trkside * 4 + 1] << 8;
	offset |= buffer[0x20 + trkside * 4 + 2] << 16;
	offset |= buffer[0x20 + trkside * 4 + 3] << 24;
	
	if(!offset) {
		return false;
	}
	
	// get verify info
	uint8* t = buffer + offset;
	sector_num = t[4] | (t[5] << 8);
	
	// make track image
	int gap3 = (sector_num <= 5) ? 0x74 : (sector_num <= 10) ? 0x54 : (sector_num <= 16) ? 0x33 : 0x10;
	int p = 0;
	
	// gap0
	for(int i = 0; i < 80; i++) {
		track[p++] = 0x4e;
	}
	// sync
	for(int i = 0; i < 12; i++) {
		track[p++] = 0;
	}
	// index mark
	track[p++] = 0xc2;
	track[p++] = 0xc2;
	track[p++] = 0xc2;
	track[p++] = 0xfc;
	// gap1
	for(int i = 0; i < 50; i++) {
		track[p++] = 0x4e;
	}
	// sectors
	for(int i = 0; i < sector_num; i ++) {
		// sync
		for(int j = 0; j < 12; j++) {
			track[p++] = 0;
		}
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
		for(int j = 0; j < 22; j++) {
			track[p++] = 0x4e;
		}
		// sync
		for(int j = 0; j < 12; j++) {
			track[p++] = 0;
		}
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
		for(int j = 0; j < gap3; j++) {
			track[p++] = 0x4e;
		}
	}
	// gap4
	while(p < 0x1800) {
		track[p++] = 0x4e;
	}
	track_size = p;
	
	return true;
}

bool DISK::get_sector(int trk, int side, int index)
{
	sector_size = sector_num = 0;
	sector = NULL;
	
	// disk not inserted
	if(!insert) {
		return false;
	}
	
	// search track
	int trkside = trk * 2 + (side & 1);
	if(!(0 <= trkside && trkside < 164)) {
		return false;
	}
	uint32 offset = buffer[0x20 + trkside * 4 + 0];
	offset |= buffer[0x20 + trkside * 4 + 1] << 8;
	offset |= buffer[0x20 + trkside * 4 + 2] << 16;
	offset |= buffer[0x20 + trkside * 4 + 3] << 24;
	
	if(!offset) {
		return false;
	}
	
	// track found
	uint8* t = buffer + offset;
	sector_num = t[4] | (t[5] << 8);
	
	if(index >= sector_num) {
		return false;
	}
	
	// skip sector
	for(int i = 0; i < index; i++) {
		t += (t[0xe] | (t[0xf] << 8)) + 0x10;
	}
	
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

// teledisk image decoder

/*
	this teledisk image decoder is based on:
	
		LZHUF.C English version 1.0 based on Japanese version 29-NOV-1988
		LZSS coded by Haruhiko OKUMURA
		Adaptive Huffman Coding coded by Haruyasu YOSHIZAKI
		Edited and translated to English by Kenji RIKITAKE
		TDLZHUF.C by WTK
*/

#define COPYBUFFER(src, size) { \
	if(file_size + (size) > DISK_BUFFER_SIZE) { \
		return false; \
	} \
	_memcpy(buffer + file_size, (src), (size)); \
	file_size += (size); \
}

bool DISK::td2d()
{
	struct td_hdr_t hdr;
	struct td_cmt_t cmt;
	struct td_trk_t trk;
	struct td_sct_t sct;
	struct d88_hdr_t d88_hdr;
	struct d88_sct_t d88_sct;
	uint8 obuf[512];
	
	// check teledisk header
	fi->Fseek(0, FILEIO_SEEK_SET);
	fi->Fread(&hdr, sizeof(td_hdr_t), 1);
	if(hdr.sig[0] == 't' && hdr.sig[1] == 'd') {
		// decompress to the temporary file
		FILEIO* fo = new FILEIO();
		if(!fo->Fopen(tmp_path, FILEIO_WRITE_BINARY)) {
			delete fo;
			return false;
		}
		int rd = 1;
		init_decode();
		do {
			if((rd = decode(obuf, 512)) > 0) {
				fo->Fwrite(obuf, rd, 1);
			}
		}
		while(rd > 0);
		fo->Fclose();
		delete fo;
		temporary = true;
		
		// reopen the temporary file
		fi->Fclose();
		if(!fi->Fopen(tmp_path, FILEIO_READ_BINARY)) {
			return false;
		}
	}
	if(hdr.flag & 0x80) {
		// skip comment
		fi->Fread(&cmt, sizeof(td_cmt_t), 1);
		fi->Fseek(cmt.len, FILEIO_SEEK_CUR);
	}
	
	// create d88 image
	file_size = 0;
	
	// create d88 header
	_memset(&d88_hdr, 0, sizeof(d88_hdr_t));
	strcpy(d88_hdr.title, "TELEDISK");
	d88_hdr.protect = 0; // non-protected
	d88_hdr.type = 0; // 2d
	COPYBUFFER(&d88_hdr, sizeof(d88_hdr_t));
	
	// create tracks
	int trkcnt = 0, trkptr = sizeof(d88_hdr_t);
	fi->Fread(&trk, sizeof(td_trk_t), 1);
	while(trk.nsec != 0xff) {
		d88_hdr.trkptr[trkcnt++] = trkptr;
		
		// read sectors in this track
		for(int i = 0; i < trk.nsec; i++) {
			uint8 buf[2048], dst[2048];
			_memset(buf, 0, sizeof(buf));
			_memset(dst, 0, sizeof(dst));
			
			// read sector header
			fi->Fread(&sct, sizeof(td_sct_t), 1);
			
			// create d88 sector header
			_memset(&d88_sct, 0, sizeof(d88_sct_t));
			d88_sct.c = sct.c;
			d88_sct.h = sct.h;
			d88_sct.r = sct.r;
			d88_sct.n = sct.n;
			d88_sct.nsec = trk.nsec;
			d88_sct.dens = 0; // ”{–§“x
			d88_sct.del = (sct.ctrl & 4) ? 0x10 : 0;
			d88_sct.stat = (sct.ctrl & 2) ? 0x10 : 0; // crc?
			d88_sct.size = secsize[sct.n & 3];
			
			// create sector image
			if(sct.ctrl != 0x10) {
				// read sector source
				int len = fi->Fgetc();
				len += fi->Fgetc() * 256 - 1;
				int flag = fi->Fgetc(), d = 0;
				fi->Fread(buf, len, 1);
				
				// convert
				if(flag == 0) {
					_memcpy(dst, buf, len);
				}
				else if(flag == 1) {
					int len2 = buf[0] | (buf[1] << 8);
					while(len2--) {
						dst[d++] = buf[2];
						dst[d++] = buf[3];
					}
				}
				else if(flag == 2) {
					for(int s = 0; s < len;) {
						int type = buf[s++];
						int len2 = buf[s++];
						if(type == 0) {
							while(len2--) {
								dst[d++] = buf[s++];
							}
						}
						else if(type < 5) {
							uint8 pat[256];
							int n = 2;
							while(type-- > 1) {
								n *= 2;
							}
							for(int j = 0; j < n; j++) {
								pat[j] = buf[s++];
							}
							while(len2--) {
								for(int j = 0; j < n; j++) {
									dst[d++] = pat[j];
								}
							}
						}
						else {
							break; // unknown type
						}
					}
				}
				else {
					break; // unknown flag
				}
			}
			else {
				d88_sct.size = 0;
			}
			
			// copy to d88
			COPYBUFFER(&d88_sct, sizeof(d88_sct_t));
			COPYBUFFER(dst, d88_sct.size);
			trkptr += sizeof(d88_sct_t) + d88_sct.size;
		}
		// read next track
		fi->Fread(&trk, sizeof(td_trk_t), 1);
	}
	d88_hdr.size = trkptr;
	_memcpy(buffer, &d88_hdr, sizeof(d88_hdr_t));
	return true;
}

int DISK::next_word()
{
	if(ibufndx >= ibufcnt) {
		ibufndx = ibufcnt = 0;
		_memset(inbuf, 0, 512);
		for(int i = 0; i < 512; i++) {
			int d = fi->Fgetc();
			if(d == EOF) {
				if(i) {
					break;
				}
				return(-1);
			}
			inbuf[i] = d;
			ibufcnt = i + 1;
		}
	}
	while(getlen <= 8) {
		getbuf |= inbuf[ibufndx++] << (8 - getlen);
		getlen += 8;
	}
	return 0;
}

int DISK::get_bit()
{
	if(next_word() < 0) {
		return -1;
	}
	short i = getbuf;
	getbuf <<= 1;
	getlen--;
	return (i < 0) ? 1 : 0;
}

int DISK::get_byte()
{
	if(next_word() != 0) {
		return -1;
	}
	uint16 i = getbuf;
	getbuf <<= 8;
	getlen -= 8;
	i >>= 8;
	return (int)i;
}

void DISK::start_huff()
{
	int i, j;
	for(i = 0; i < N_CHAR; i++) {
		freq[i] = 1;
		son[i] = i + TABLE_SIZE;
		prnt[i + TABLE_SIZE] = i;
	}
	i = 0; j = N_CHAR;
	while(j <= ROOT_POSITION) {
		freq[j] = freq[i] + freq[i + 1];
		son[j] = i;
		prnt[i] = prnt[i + 1] = j;
		i += 2; j++;
	}
	freq[TABLE_SIZE] = 0xffff;
	prnt[ROOT_POSITION] = 0;
}

void DISK::reconst()
{
	short i, j = 0, k;
	uint16 f, l;
	for(i = 0; i < TABLE_SIZE; i++) {
		if(son[i] >= TABLE_SIZE) {
			freq[j] = (freq[i] + 1) / 2;
			son[j] = son[i];
			j++;
		}
	}
	for(i = 0, j = N_CHAR; j < TABLE_SIZE; i += 2, j++) {
		k = i + 1;
		f = freq[j] = freq[i] + freq[k];
		for(k = j - 1; f < freq[k]; k--);
		k++;
		l = (j - k) * 2;
		_memmove(&freq[k + 1], &freq[k], l);
		freq[k] = f;
		_memmove(&son[k + 1], &son[k], l);
		son[k] = i;
	}
	for(i = 0; i < TABLE_SIZE; i++) {
		if((k = son[i]) >= TABLE_SIZE) {
			prnt[k] = i;
		}
		else {
			prnt[k] = prnt[k + 1] = i;
		}
	}
}

void DISK::update(int c)
{
	int i, j, k, l;
	if(freq[ROOT_POSITION] == MAX_FREQ) {
		reconst();
	}
	c = prnt[c + TABLE_SIZE];
	do {
		k = ++freq[c];
		if(k > freq[l = c + 1]) {
			while(k > freq[++l]);
			l--;
			freq[c] = freq[l];
			freq[l] = k;
			i = son[c];
			prnt[i] = l;
			if(i < TABLE_SIZE) {
				prnt[i + 1] = l;
			}
			j = son[l];
			son[l] = i;
			prnt[j] = c;
			if(j < TABLE_SIZE) {
				prnt[j + 1] = c;
			}
			son[c] = j;
			c = l;
		}
	}
	while((c = prnt[c]) != 0);
}

short DISK::decode_char()
{
	int ret;
	uint16 c = son[ROOT_POSITION];
	while(c < TABLE_SIZE) {
		if((ret = get_bit()) < 0) {
			return -1;
		}
		c += (unsigned)ret;
		c = son[c];
	}
	c -= TABLE_SIZE;
	update(c);
	return c;
}

short DISK::decode_position()
{
	short bit;
	uint16 i, j, c;
	if((bit = get_byte()) < 0) {
		return -1;
	}
	i = (uint16)bit;
	c = (uint16)d_code[i] << 6;
	j = d_len[i] - 2;
	while(j--) {
		if((bit = get_bit()) < 0) {
			 return -1;
		}
		i = (i << 1) + bit;
	}
	return (c | i & 0x3f);
}

void DISK::init_decode()
{
	ibufcnt= ibufndx = bufcnt = getbuf = 0;
	getlen = 0;
	start_huff();
	for(int i = 0; i < STRING_BUFFER_SIZE - LOOKAHEAD_BUFFER_SIZE; i++) {
		text_buf[i] = ' ';
	}
	ptr = STRING_BUFFER_SIZE - LOOKAHEAD_BUFFER_SIZE;
}

int DISK::decode(uint8 *buf, int len)
{
	short c, pos;
	int  count;
	for(count = 0; count < len;) {
		if(bufcnt == 0) {
			if((c = decode_char()) < 0) {
				return count;
			}
			if(c < 256) {
				*(buf++) = (uint8)c;
				text_buf[ptr++] = (uint8)c;
				ptr &= (STRING_BUFFER_SIZE - 1);
				count++;
			} 
			else {
				if((pos = decode_position()) < 0) {
					return count;
				}
				bufpos = (ptr - pos - 1) & (STRING_BUFFER_SIZE - 1);
				bufcnt = c - 255 + THRESHOLD;
				bufndx = 0;
			}
		}
		else {
			while(bufndx < bufcnt && count < len) {
				c = text_buf[(bufpos + bufndx) & (STRING_BUFFER_SIZE - 1)];
				*(buf++) = (uint8)c;
				bufndx++;
				text_buf[ptr++] = (uint8)c;
				ptr &= (STRING_BUFFER_SIZE - 1);
				count++;
			}
			if(bufndx >= bufcnt) {
				bufndx = bufcnt = 0;
			}
		}
	}
	return count;
}

// imd image decoder

bool DISK::imd2d()
{
	struct imd_trk_t trk;
	struct d88_hdr_t d88_hdr;
	struct d88_sct_t d88_sct;
	
	// skip comment
	fi->Fseek(0, FILEIO_SEEK_SET);
	int tmp;
	while((tmp = fi->Fgetc()) != 0x1a) {
		if(tmp == EOF) {
			return false;
		}
	}
	
	// create d88 image
	file_size = 0;
	
	// create d88 header
	_memset(&d88_hdr, 0, sizeof(d88_hdr_t));
	strcpy(d88_hdr.title, "IMAGEDISK");
	d88_hdr.protect = 0; // non-protected
	d88_hdr.type = 0x20; // 2hd
	COPYBUFFER(&d88_hdr, sizeof(d88_hdr_t));
	
	// create tracks
	int trkptr = sizeof(d88_hdr_t);
	for(int t = 0; t < 164; t++) {
		// check end of file
		if(fi->Fread(&trk, sizeof(imd_trk_t), 1) != 1) {
			break;
		}
		// check track header
		if(trk.mode > 5 || trk.size > 6) {
			return false;
		}
		if(!trk.nsec) {
			continue;
		}
		d88_hdr.trkptr[t] = trkptr;
		
		// setup sector id
		uint8 c[64], h[64], r[64];
		fi->Fread(r, trk.nsec, 1);
		if(trk.head & 0x80) {
			fi->Fread(c, trk.nsec, 1);
		}
		else {
			_memset(c, trk.cyl, sizeof(c));
		}
		if(trk.head & 0x40) {
			fi->Fread(h, trk.nsec, 1);
		}
		else {
			_memset(h, trk.head & 1, sizeof(h));
		}
		
		// read sectors in this track
		for(int i = 0; i < trk.nsec; i++) {
			// create d88 sector header
			static uint8 del[] = {0, 0, 0, 0x10, 0x10, 0, 0, 0x10, 0x10};
			static uint8 err[] = {0, 0, 0, 0, 0, 0x10, 0x10, 0x10, 0x10};
			int sectype = fi->Fgetc();
			if(sectype > 8) {
				return false;
			}
			_memset(&d88_sct, 0, sizeof(d88_sct_t));
			d88_sct.c = c[i];
			d88_sct.h = h[i];
			d88_sct.r = r[i];
			d88_sct.n = trk.size;
			d88_sct.nsec = trk.nsec;
			d88_sct.dens = (trk.mode < 3) ? 0x40 : 0;
			d88_sct.del = del[sectype];
			d88_sct.stat = err[sectype];
			d88_sct.size = secsize[trk.size];
			
			// create sector image
			uint8 dst[8192];
			if(sectype == 1 || sectype == 3 || sectype == 5 || sectype == 7) {
				// uncompressed
				fi->Fread(dst, d88_sct.size, 1);
			}
			else if(sectype == 2 || sectype == 4 || sectype == 6 || sectype == 8) {
				// compressed
				int tmp = fi->Fgetc();
				_memset(dst, tmp, d88_sct.size);
			}
			else {
				d88_sct.size = 0;
			}
			
			// copy to d88
			COPYBUFFER(&d88_sct, sizeof(d88_sct_t));
			COPYBUFFER(dst, d88_sct.size);
			trkptr += sizeof(d88_sct_t) + d88_sct.size;
		}
	}
	d88_hdr.size = trkptr;
	_memcpy(buffer, &d88_hdr, sizeof(d88_hdr_t));
	return true;
}

