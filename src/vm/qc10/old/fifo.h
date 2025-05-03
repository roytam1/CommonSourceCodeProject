/*
	EPSON QC-10/QX-10 Emulator 'eQC-10'
	(Skelton for Z-80 PC Emulator)

	Author : Takeda.Toshiya
	Date   : 2005.06.08-

	[ fifo buffer ]
*/

#ifndef _FIFO_H_
#define _FIFO_H_

#include "../common.h"

#define FIFO_SIZE	0x10000

class FIFO
{
private:
	int buf[FIFO_SIZE];
	int cnt, rpt, wpt, size;
public:
	FIFO(int s) {
		cnt = rpt = wpt = 0;
		size = s;
	}
	~FIFO() {}
	
	void init() {
		cnt = rpt = wpt = 0;
	}
	int read() {
		int val = 0;
		if(cnt) {
			val = buf[rpt];
			rpt = (rpt + 1) & (size - 1);
			cnt--;
		}
		return val;
	}
	void write(int val) {
		if(cnt < FIFO_SIZE) {
			buf[wpt] = val;
			wpt = (wpt + 1) & (size - 1);
			cnt++;
		}
	}
	int count() {
		return cnt;
	}
	bool full() {
		return (cnt < FIFO_SIZE) ? false : true;
	}
	bool empty() {
		return cnt ? false : true;
	}
};

#endif

