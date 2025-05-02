/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.09.17-

	[ fifo buffer ]
*/

#ifndef _FIFO_H_
#define _FIFO_H_

#include "common.h"

class FIFO
{
private:
	int* buf;
	int cnt, rpt, wpt, size;
public:
	FIFO(int s) {
		cnt = rpt = wpt = 0;
		size = s;
		buf = (int*)malloc(s * sizeof(int));
	}
	void release() {
		free(buf);
	}
	void clear() {
		cnt = rpt = wpt = 0;
	}
	void write(int val) {
		if(cnt < size) {
			buf[wpt++] = val;
			if(wpt >= size) {
				wpt = 0;
			}
			cnt++;
		}
	}
	int read() {
		int val = 0;
		if(cnt) {
			val = buf[rpt++];
			if(rpt >= size) {
				rpt = 0;
			}
			cnt--;
		}
		return val;
	}
	int read_not_remove(int pt) {
		if(pt < cnt) {
			pt += rpt;
			if(pt >= size) {
				pt -= size;
			}
			return buf[pt];
		}
		return 0;
	}
	int count() {
		return cnt;
	}
	bool full() {
		return (cnt < size) ? false : true;
	}
	bool empty() {
		return cnt ? false : true;
	}
};

#endif

