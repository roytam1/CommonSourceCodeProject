/*
	CANON X-07 Emulator 'eX-07'
	Skelton for retropc emulator

	Origin : J.Brigaud
	Author : Takeda.Toshiya
	Date   : 2007.12.26 -

	[ i/o ]
*/

#include <math.h>
#include "io.h"
#include "../../fifo.h"

//memo: how to request the display size changing
//emu->change_screen_size(TV_SCREEN_WIDTH, TV_SCREEN_HEIGHT, 0, TV_WINDOW_WIDTH1, TV_WINDOW_HEIGHT1, TV_WINDOW_WIDTH2, TV_WINDOW_HEIGHT2);
//emu->change_screen_size(SCREEN_WIDTH, SCREEN_HEIGHT, 0, WINDOW_WIDTH1, WINDOW_HEIGHT1, WINDOW_WIDTH2, WINDOW_HEIGHT2);

void IO::initialize()
{
	// load font
	_TCHAR app_path[_MAX_PATH], file_path[_MAX_PATH];
	emu->application_path(app_path);
	FILEIO* fio = new FILEIO();
	
	_stprintf(file_path, _T("%sFONT.ROM"), app_path);
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(font, sizeof(font), 1);
		fio->Fclose();
	}
	delete fio;
	
	// init fifo
	key_buf = new FIFO(20);
	cmd_buf = new FIFO(256);
	rsp_buf = new FIFO(256);
	
	// wram
	_memset(wram, 0, 0x200);
	for(int i = 0; i < 12; i++)
		strcpy((char*)wram + udk_ofs[i], udk_ini[i]);
	for(int i = 0; i < 0x200; i++) {
		if(wram[i] == '^')
			wram[i] = 13;
	}
	_memcpy(udc, font, sizeof(udc));
	
	// cmt
	cmt_fio = new FILEIO();
	cmt_play = cmt_rec = false;
	
	// video
	vm->regist_frame_event(this);
	vm->regist_vline_event(this);
}

void IO::release()
{
	close_datarec();
	delete cmt_fio;
}

void IO::reset()
{
	// registers
	_memset(rregs, 0, sizeof(rregs));
	_memset(wregs, 0, sizeof(wregs));
	
	// t6834
	cmd_buf->clear();
	rsp_buf->clear();
	sub_int = 0;
	
	// keyboard
	key_buf->clear();
	ctrl = shift = kana = graph = brk = false;
	stick = 0x30;
	strig = strig1 = 0xff;
	
	// data recorder
	close_datarec();
	cmt_mode = false;
	
	// video
	_memset(lcd, 0, sizeof(lcd));
	locate_on = cursor_on = udk_on = false;
	locate_x = locate_y = cursor_x = cursor_y = cursor_blink = 0;
	scroll_min = 0;
	scroll_max = 4;
	
	// beep
	regist_id = -1;
}

void IO::event_callback(int event_id, int err)
{
	if(event_id == EVENT_BEEP) {
		regist_id = -1;
		d_beep->write_signal(did_beep_on, 0, 0);
		rregs[4] = (wregs[4] &= ~2);
	}
	else if(event_id == EVENT_CMT) {
		sub_int |= 2;
		update_intr();
	}
}

void IO::event_frame()
{
	cursor_blink++;
}

void IO::event_vline(int v, int clock)
{
	vblank = !(v < 192);
}

void IO::write_io8(uint32 addr, uint32 data)
{
//	emu->out_debug("OUT\t%4x, %2x\n", addr, data);
	switch(addr & 0xff)
	{
	case 0x80:
		font_code = data;
		break;
	case 0xf0:
//		d_mem->write_signal(0, data, 0xff);
	case 0xf1:
	case 0xf2:
	case 0xf3:
	case 0xf6:
	case 0xf7:
		wregs[addr & 7] = data;
		break;
	case 0xf4:
		rregs[4] = wregs[4] = data;
		// cmt
		cmt_mode = ((data & 0xc) == 8) ? true : false;
//		if(cmt_mode && (wregs[5] & 4))
//			recv_from_cmt();
		// beep
		if((data & 0xe) == 0xe) {
			int freq = 192000 / (wregs[2] | (wregs[3] << 8));
			d_beep->write_signal(did_beep_freq, freq, 0xffffffff);
			d_beep->write_signal(did_beep_on, 1, 1);
			// temporary patch: regist the event to stop
			int intv = ram[0x450] * 50000;
			if(regist_id != -1)
				vm->cancel_event(regist_id);
			vm->regist_event(this, EVENT_BEEP, intv, false, &regist_id);
		}
		else
			d_beep->write_signal(did_beep_on, 0, 1);
		break;
	case 0xf5:
		wregs[5] = data;
		if(data & 1)
			ack_from_sub();
		if(data & 2)
			send_to_sub();
		if(data & 4)
			recv_from_cmt();
		if(data & 8)
			send_to_cmt();
//		if(data & 0x20)
//			print(prt_data);
		break;
	}
}

uint32 IO::read_io8(uint32 addr)
{
	uint32 val = 0xff;
	
	switch(addr & 0xff)
	{
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x87:
	case 0x88:
	case 0x89:
	case 0x8a:
	case 0x8b:
	case 0x8c:
		val = ((addr & 0xf) < 8) ? udc[(font_code << 3) | (addr & 7)] : 0;
		break;
	case 0x90:
		val =  vblank ? 0x80 : 0;
		break;
	case 0xf0:
	case 0xf1:
	case 0xf3:
	case 0xf4:
	case 0xf5:
	case 0xf7:
		val = rregs[addr & 7];
		break;
	case 0xf2:
		if(wregs[5] & 4)
			rregs[2] |= 2;
		else
			rregs[2] &= 0xfd;
		val = rregs[2] | 2;
		break;
	case 0xf6:
		if(cmt_mode)
			rregs[6] |= 5;
		val = rregs[6];
		break;
	}
//	emu->out_debug("IN\t%4x = %2x\n", addr, val);
	return val;
}

void IO::update_intr()
{
	if(!key_buf->empty()) {
		rregs[0] = 0;
		rregs[1] = key_buf->read();
		rregs[2] |= 1;
		d_cpu->write_signal(did_rsta, 1, 1);
	}
	else if(brk) {
		rregs[0] = 0x80;
		rregs[1] = 5;
		rregs[2] |= 1;
		brk = false;
		d_cpu->write_signal(did_rsta, 1, 1);
	}
	else if(sub_int & 1) {
		recv_from_sub();
		sub_int &= ~1;
		d_cpu->write_signal(did_rsta, 1, 1);
	}
	else if(sub_int & 2) {
		sub_int &= ~2;
		d_cpu->write_signal(did_rstb, 1, 1);
	}
}

// ----------------------------------------------------------------------------
// video
// ----------------------------------------------------------------------------

void IO::draw_screen()
{
	scrntype cd = RGB_COLOR(48, 56, 16);
	scrntype cb = RGB_COLOR(160, 168, 160);
	
	for(int y = 0; y < 4; y++) {
		int py = y * 8;
		for(int x = 0; x < 20; x++) {
			int px = x * 6;
			if(cursor_on && (cursor_blink & 0x20) && cursor_x == x && cursor_y == y) {
				for(int l = 0; l < 8; l++) {
					scrntype* dest = emu->screen_buffer(py + l);
					dest += px;
					dest[0] = dest[1] = dest[2] = dest[3] = dest[4] = dest[5] = (l < 7) ? cb : cd;
				}
			}
			else {
				for(int l = 0; l < 8; l++) {
					uint8* src = &lcd[py + l][px];
					scrntype* dest = emu->screen_buffer(py + l);
					dest += px;
					dest[0] = src[0] ? cd : cb;
					dest[1] = src[1] ? cd : cb;
					dest[2] = src[2] ? cd : cb;
					dest[3] = src[3] ? cd : cb;
					dest[4] = src[4] ? cd : cb;
					dest[5] = src[5] ? cd : cb;
				}
			}
		}
	}
}

void IO::draw_font(int x, int y, uint8 code)
{
	if(x < 20 && y < 4) {
		int px = x * 6;
		int py = y * 8;
		int ofs = code << 3;
		for(int l = 0; l < 8; l++) {
			uint8 pat = udc[ofs + l];
			lcd[py + l][px + 0] = (pat & 0x80) ? 0xff : 0;
			lcd[py + l][px + 1] = (pat & 0x40) ? 0xff : 0;
			lcd[py + l][px + 2] = (pat & 0x20) ? 0xff : 0;
			lcd[py + l][px + 3] = (pat & 0x10) ? 0xff : 0;
			lcd[py + l][px + 4] = (pat & 0x08) ? 0xff : 0;
			lcd[py + l][px + 5] = (pat & 0x04) ? 0xff : 0;
		}
	}
}

void IO::draw_udk()
{
	for(int i = 0, x = 0; i < 5; i++) {
		int ofs = udk_ofs[i + (shift ? 6 : 0)];
		draw_font(x++, 3, 0x83);
		for(int j = 0; j < 3; j++)
			draw_font(x++, 3, wram[ofs++]);
	}
}

#define draw_point(x, y, c) { if((x) < 120 && (y) < 32) lcd[y][x] = c; }

void IO::draw_line(int sx, int sy, int ex, int ey)
{
	int next_x = sx, next_y = sy;
	int delta_x = abs(ex - sx) * 2;
	int delta_y = abs(ey - sy) * 2;
	int step_x = (ex < sx) ? -1 : 1;
	int step_y = (ey < sy) ? -1 : 1;
	
	if(delta_x > delta_y) {
		int frac = delta_y - delta_x / 2;
		while(next_x != ex) {
			if(frac >= 0) {
				next_y += step_y;
				frac -= delta_x;
			}
			next_x += step_x;
			frac += delta_y;
			draw_point(next_x, next_y, 0xff);
		}
	}
	else {
		int frac = delta_x - delta_y / 2;
		while(next_y != ey) {
			if(frac >= 0) {
				next_x += step_x;
				frac -= delta_y;
			}
			next_y += step_y;
			frac += delta_x;
			draw_point(next_x, next_y, 0xff);
		}
	}
	draw_point(sx, sy, 0xff);
	draw_point(ex, ey, 0xff);
}

void IO::draw_circle(int x, int y, int r)
{
#if 0
	// high accuracy
	double xlim = sqrt((double)(r * r) / 2);
	
	for(int cx = 0, cy = r; cx <= xlim ; cx++) {
		double d1 = (cx * cx + cy * cy) - r * r;
		double d2 = (cx * cx + (cy - 1) * (cy - 1)) - r * r;
		if(abs(d1) > abs(d2))
			cy--;
		draw_point(cx + x, cy + y, 0xff);
		draw_point(cx + x, -cy + y, 0xff);
		draw_point(-cx + x, cy + y, 0xff);
		draw_point(-cx + x, -cy + y, 0xff);
		draw_point(cy + x, cx + y, 0xff);
		draw_point(cy + x, -cx + y, 0xff);
		draw_point(-cy + x, cx + y, 0xff);
		draw_point(-cy + x, -cx + y, 0xff);
	}
#else
	// high speed
	int cx = 0, cy = r;
	int d = 2 - 2 * r;
	
	draw_point(cx + x, cy + y, 0xff);
	draw_point(cx + x, -cy + y, 0xff);
	draw_point(cy + x, cx + y, 0xff);
	draw_point(-cy + x, cx + y, 0xff);
	while(1) {
		if(d > -cy) {
			cy--;
			d += 1 - 2 * cy;
		}
		if(d <= cx) {
			cx++;
			d += 1 + 2 * cx;
		}
		if(!cy)
			return;
		draw_point(cx + x, cy + y, 0xff);
		draw_point(-cx + x, cy + y, 0xff);
		draw_point(-cx + x, -cy + y, 0xff);
		draw_point(cx + x, -cy + y, 0xff);
	}
#endif
}

void IO::line_clear(int y)
{
	if(y < 4) {
		for(int l = y * 8; l < (y + 1) * 8; l++)
			_memset(lcd[l], 0, 120);
	}
}

void IO::scroll()
{
	if(scroll_min <= scroll_max && scroll_max <= 4) {
		for(int l = scroll_min * 8; l < (scroll_max - 1) * 8; l++)
			_memcpy(lcd[l], lcd[l + 8], 120);
		for(int l = (scroll_max - 1) * 8; l < scroll_max * 8; l++)
			_memset(lcd[l], 0, 120);
	}
}

// ----------------------------------------------------------------------------
// keyboard
// ----------------------------------------------------------------------------

void IO::key_down(int code)
{
	int fctn, ptr;
	
	switch(code)
	{
	case 0x10:
		shift = true;
		if(udk_on)
			draw_udk();
		break;
	case 0x11:
		ctrl = true;
		break;
	case 0x12:
		if(graph)
			graph = kana = false;
		else
			graph = true;
		break;
	case 0x13:
		brk = true;
		update_intr();
		break;
	case 0x15:
		if(kana)
			graph = kana = false;
		else
			kana = true;
		break;
	case 0x20:
		strig1 = 0;
		goto strig_key;
	case 0x25:	// left
		stick = 0x37;
		code = 0x1d;
		goto strig_key;
	case 0x26:	// up
		stick = 0x31;
		code = 0x1e;
		goto strig_key;
	case 0x27:	// right
		stick = 0x32;
		code = 0x1c;
		goto strig_key;
	case 0x28:	// down
		stick = 0x36;
		code = 0x1f;
strig_key:
		if(!key_buf->full()) {
			key_buf->write(code);
			update_intr();
		}
		break;
	case 0x70:	// F1
		fctn = shift ? 6 : 0;
		goto fkey;
	case 0x71:	// F2
		fctn = shift ? 7 : 1;
		goto fkey;
	case 0x72:	// F3
		fctn = shift ? 8 : 2;
		goto fkey;
	case 0x73:	// F4
		fctn = shift ? 9 : 3;
		goto fkey;
	case 0x74:	// F5
		fctn = shift ? 10 : 4;
		goto fkey;
	case 0x75:	// F6
		strig = 0;
		fctn = shift ? 11 : 5;
		goto fkey;
	case 0x76:	// F7
		fctn = 6;
		goto fkey;
	case 0x77:	// F8
		fctn = 7;
		goto fkey;
	case 0x78:	// F9
		fctn = 8;
		goto fkey;
	case 0x79:	// F10
		fctn = 9;
		goto fkey;
	case 0x7a:	// F11
		fctn = 10;
		goto fkey;
	case 0x7b:	// F12
		fctn = 11;
fkey:
		ptr = udk_ofs[fctn] + 3;
		for(;;) {
			uint8 val = wram[ptr++];
			if(!val)
				break;
			if(!key_buf->full())
				key_buf->write(val);
		}
		if(!key_buf->empty())
			update_intr();
		break;
	default:
		if(!key_buf->full()) {
			uint8 val = 0;
			if(ctrl)
				val = key_tbl_c[code];
			else if(kana) {
				if(shift)
					val = key_tbl_ks[code];
				else
					val = key_tbl_k[code];
			}
			else if(graph)
				val = key_tbl_g[code];
			else {
				if(shift)
					val = key_tbl_s[code];
				else
					val = key_tbl[code];
			}
			if(val) {
				key_buf->write(val);
				update_intr();
			}
		}
	}
}

void IO::key_up(int code)
{
	switch(code)
	{
	case 0x08:	// bs->left
		stick = 0x30;
		break;
	case 0x10:
		shift = false;
		if(udk_on)
			draw_udk();
		break;
	case 0x11:
		ctrl = false;
		break;
	case 0x20:
		strig1 = 0xff;
		break;
	case 0x25:	// left
	case 0x26:	// up
	case 0x27:	// right
	case 0x28:	// down
		stick = 0x30;
		break;
	case 0x75:	// F6
		strig = 0xff;
		break;
	}
}

// ----------------------------------------------------------------------------
// cmt
// ----------------------------------------------------------------------------

void IO::play_datarec(_TCHAR* filename)
{
	close_datarec();
	if(cmt_fio->Fopen(filename, FILEIO_READ_BINARY)) {
		_memset(cmt_buf, 0, sizeof(cmt_buf));
		cmt_fio->Fread(cmt_buf, sizeof(cmt_buf), 1);
		cmt_ptr = 0;
		cmt_play = true;
		// recieve first byte
		if(cmt_mode && (wregs[5] & 4))
			recv_from_cmt();
	}
}

void IO::rec_datarec(_TCHAR* filename)
{
	close_datarec();
	if(cmt_fio->Fopen(filename, FILEIO_WRITE_BINARY)) {
		cmt_ptr = 0;
		cmt_rec = true;
	}
}

void IO::close_datarec()
{
	if(cmt_rec)
		cmt_fio->Fwrite(cmt_buf, cmt_ptr, 1);
	if(cmt_rec || cmt_play)
		cmt_fio->Fclose();
	cmt_play = cmt_rec = false;
}

void IO::send_to_cmt()
{
	if(cmt_rec && cmt_mode) {
		cmt_buf[cmt_ptr++] = wregs[7];
		if(!(cmt_ptr &= (CMT_BUF_SIZE - 1)))
			cmt_fio->Fwrite(cmt_buf, sizeof(cmt_buf), 1);
	}
}

void IO::recv_from_cmt()
{
	if(cmt_play && cmt_mode) {
		rregs[6] |= 2;
		rregs[7] = cmt_buf[cmt_ptr++];
		// regist event for rstb
		int id;
		vm->regist_event(this, EVENT_CMT, 2000, false, &id);
		// update buffer
		cmt_ptr &= CMT_BUF_SIZE - 1;
		if(!cmt_ptr) {
			_memset(cmt_buf, 0, sizeof(cmt_buf));
			cmt_fio->Fread(cmt_buf, sizeof(cmt_buf), 1);
		}
	}
}

// ----------------------------------------------------------------------------
// sub cpu
// ----------------------------------------------------------------------------

void IO::send_to_sub()
{
	// send command
	if(cmd_buf->empty()) {
		if(locate_on && (wregs[1] & 0x7f) != 0x24 && 0x20 <= wregs[1] && wregs[1] < 0x80) {
			cursor_x++;
			draw_font(cursor_x, cursor_x, wregs[1]);
		}
		else {
			if((wregs[1] & 0x7f) < 0x47)
				cmd_buf->write(wregs[1] & 0x7f);
			locate_on = 0;
		}
	}
	else {
		cmd_buf->write(wregs[1]);
		if(cmd_buf->count() == 2) {
			uint8 cmd_type = cmd_buf->read_not_remove(0);
			if(cmd_type == 7 && wregs[1] > 4) {
				cmd_buf->clear();
				cmd_buf->write(wregs[1] & 0x7f);
			}
			else if(cmd_type == 0xc && wregs[1] == 0xb0) {
				_memset(lcd, 0, sizeof(lcd));
				cmd_buf->clear();
				cmd_buf->write(wregs[1] & 0x7f);
			}
		}
	}
	// check cmd length
	if(!cmd_buf->empty()) {
		uint8 cmd_type = cmd_buf->read_not_remove(0);
		uint8 cmd_len = sub_cmd_len[cmd_type];
		if(cmd_len & 0x80) {
			if((cmd_len & 0x7f) < cmd_buf->count() && !wregs[1])
				cmd_len = cmd_buf->count();
		}
		if(cmd_buf->count() == cmd_len) {
			// process command
			rsp_buf->clear();
			process_sub();
			cmd_buf->clear();
			if(rsp_buf->count()) {
				sub_int |= 1;
				update_intr();
			}
//			emu->out_debug("CMD TYPE = %2x, LEN = %d RSP=%d\n", cmd_type, cmd_len, rsp_buf->count());
		}
	}
}

void IO::recv_from_sub()
{
	rregs[0] = 0x40;
	rregs[1] = rsp_buf->read_not_remove(0);
	rregs[2] |= 1;
}

void IO::ack_from_sub()
{
	rsp_buf->read();
	rregs[2] &= 0xfe;
	if(!rsp_buf->empty()) {
		sub_int |= 1;
		update_intr();
	}
}

#define YEAR		time[0]
#define MONTH		time[1]
#define DAY		time[2]
#define DAY_OF_WEEK	time[3]
#define HOUR		time[4]
#define MINUTE		time[5]
#define SECOND		time[6]

void IO::process_sub()
{
	static uint8 dow[8] = {128, 192, 224, 240, 248, 252, 254, 255};
	int time[8];
	uint8 val;
	uint16 addr;
	int sx, sy, ex, ey, cr, i;
	
	uint8 cmd_type = cmd_buf->read();
	switch(cmd_type & 0x7f)
	{
	case 0x00:	// unknown
		break;
	case 0x01:	// TimeCall
		emu->get_timer(time);
		rsp_buf->write((YEAR >> 8) & 0xff);
		rsp_buf->write(YEAR & 0xff);
		rsp_buf->write(MONTH);
		rsp_buf->write(DAY);
		rsp_buf->write(dow[DAY_OF_WEEK]);
		rsp_buf->write(HOUR);
		rsp_buf->write(MINUTE);
		rsp_buf->write(SECOND);
		break;
	case 0x02:	// Stick
		rsp_buf->write(stick);
		break;
	case 0x03:	// Strig
		rsp_buf->write(strig);
		break;
	case 0x04:	// Strig1
		rsp_buf->write(strig1);
		break;
	case 0x05:	// RamRead
		addr = cmd_buf->read();
		addr |= cmd_buf->read() << 8;
		if(addr == 0xc00e)
			val = 0xa;
		else if(addr == 0xd000)
			val = 0x30;
		else if(WRAM_OFS_UDC0 <= addr && addr < WRAM_OFS_UDC1)
			val = udc[FONT_OFS_UDC0 + (addr - WRAM_OFS_UDC0)];
		else if(WRAM_OFS_UDC1 <= addr && addr < WRAM_OFS_KBUF)
			val = udc[FONT_OFS_UDC1 + (addr - WRAM_OFS_UDC1)];
		else
			val = wram[addr & 0x7ff];
		rsp_buf->write(val);
		break;
	case 0x06:	// RamWrite
		val = cmd_buf->read();
		addr = cmd_buf->read();
		addr |= cmd_buf->read() << 8;
		if(WRAM_OFS_UDC0 <= addr && addr < WRAM_OFS_UDC1)
			udc[FONT_OFS_UDC0 + (addr - WRAM_OFS_UDC0)] = val;
		else if(WRAM_OFS_UDC1 <= addr && addr < WRAM_OFS_KBUF)
			udc[FONT_OFS_UDC1 + (addr - WRAM_OFS_UDC1)] = val;
		else
			wram[addr & 0x7ff] = val;
		break;
	case 0x07:	// ScrollSet
		scroll_min = cmd_buf->read();
		scroll_max = cmd_buf->read() + 1;
		break;
	case 0x08:	// ScrollExec
		scroll();
		break;
	case 0x09:	// LineClear
		val = cmd_buf->read();
		line_clear(val);
		break;
	case 0x0a:	// TimeSet
		cmd_buf->read();
		cmd_buf->read();
		cmd_buf->read();
		cmd_buf->read();
		cmd_buf->read();
		cmd_buf->read();
		cmd_buf->read();
		cmd_buf->read();
		break;
	case 0x0b:	// CalcDay
		break;
	case 0x0c:	// AlarmSet
		for(i = 0; i < 8; i++)
			alarm[i] = cmd_buf->read();
		break;
	case 0x0d:	// BuzzerOff
//		d_beep->write_signal(did_beep_on, 0, 0);
		break;
	case 0x0e:	// BuzzerOn
//		d_beep->write_signal(did_beep_on, 1, 1);
		break;
	case 0x0f:	// TrfLine
		sy = cmd_buf->read();
		for(i = 0; i < 120; i++) {
			if(sy < 32)
				rsp_buf->write(lcd[sy][i]);
			else
				rsp_buf->write(0);
		}
		break;
	case 0x10:	// TestPoint
		sx = cmd_buf->read();
		sy = cmd_buf->read();
		if(sx < 120 && sy < 32)
			rsp_buf->write(lcd[sy][sx]);
		else
			rsp_buf->write(0);
		break;
	case 0x11:	// Pset
		sx = cmd_buf->read();
		sy = cmd_buf->read();
		draw_point(sx, sy, 0xff);
		break;
	case 0x12:	// Preset
		sx = cmd_buf->read();
		sy = cmd_buf->read();
		draw_point(sx, sy, 0);
		break;
	case 0x13:	// Peor
		sx = cmd_buf->read();
		sy = cmd_buf->read();
		if(sx < 120 && sy < 32)
			lcd[sy][sx] = ~lcd[sy][sx];
		break;
	case 0x14:	// Line
		sx = cmd_buf->read();
		sy = cmd_buf->read();
		ex = cmd_buf->read();
		ey = cmd_buf->read();
		draw_line(sx, sy, ex, ey);
		break;
	case 0x15:	// Circle
		sx = cmd_buf->read();
		sy = cmd_buf->read();
		cr = cmd_buf->read();
		draw_circle(sx, sy, cr);
		break;
	case 0x16:	// UDKWrite
		val = cmd_buf->read();
		for(i = 0; i < udk_size[val]; i++)
			wram[udk_ofs[val] + i] = cmd_buf->read();
		break;
	case 0x17:	// UDKRead
		val = cmd_buf->read();
		for(i = 0; i < udk_size[val]; i++) {
			uint8 code = wram[udk_ofs[val] + i];
			rsp_buf->write(code);
			if(!code)
				break;
		}
		break;
	case 0x18:	// UDKOn
	case 0x19:	// UDKOff
		break;
	case 0x1a:	// UDCWrite
		addr = cmd_buf->read() << 3;
		for(i = 0; i < 8; i++)
			udc[addr + i] = cmd_buf->read();
		break;
	case 0x1b:	// UDCRead
		addr = cmd_buf->read() << 3;
		for(i = 0; i < 8; i++)
			rsp_buf->write(udc[addr + i]);
		break;
	case 0x1c:	// UDCInit
		_memcpy(udc, font, sizeof(udc));
		break;
	case 0x1d:	// StartPgmWrite
		break;
	case 0x1e:	// SPWriteCont
		break;
	case 0x1f:	// SPOn
		break;
	case 0x20:	// SPOff
		break;
	case 0x21:	// StartPgmRead
		for(i = 0; i < 128; i++)
			rsp_buf->write(0);
		break;
	case 0x22:	// OnStat
		rsp_buf->write(4);	// 0x41 ?
		break;
	case 0x23:	// OFFReq
		break;
	case 0x24:	// Locate
		sx = cmd_buf->read();
		sy = cmd_buf->read();
		val = cmd_buf->read();
		locate_on = (locate_x != sx || locate_y != sy);
		locate_x = cursor_x = sx;
		locate_y = cursor_y = sy;
		if(val)
			draw_font(sx, sy, val);
		break;
	case 0x25:	// CursOn
		cursor_on = true;
		break;
	case 0x26:	// CursOff
		cursor_on = false;
		break;
	case 0x27:	// TestKey
	case 0x28:	// TestChr
		rsp_buf->write(0);
		break;
	case 0x29:	// InitSec
	case 0x2a:	// InitDate
	case 0x2b:	// ScrOff
	case 0x2c:	// ScrOn
		break;
	case 0x2d:	// KeyBufferClear
		key_buf->clear();
		break;
	case 0x2e:	// ClsScr
		_memset(lcd, 0, sizeof(lcd));
		break;
	case 0x2f:	// Home
		cursor_x = cursor_y = 0;
		break;
	case 0x30:	// UDKOn
		udk_on = true;
		draw_udk();
		break;
	case 0x31:	// UDKOff
		udk_on = false;
		line_clear(3);
		break;
	case 0x36:	// AlarmRead
		for(i = 0; i < 8; i++)
			rsp_buf->write(alarm[i]);
		break;
	case 0x37:	// BuzzZero
		rsp_buf->write(0);
		break;
	case 0x40:
		_memset(wram, 0, 0x200);
		for(i = 0; i < 12; i++)
			strcpy((char*)wram + udk_ofs[i], udk_ini[i]);
		for(i = 0; i < 0x200; i++) {
			// CR
			if(wram[i] == '^')
				wram[i] = 13;
		}
		break;
	case 0x42:	// ReadCar
		for(i = 0; i < 8; i++)
			rsp_buf->write(0);
		break;
	case 0x43:	// ScanR
	case 0x44:	// ScanL
		rsp_buf->write(0);
		rsp_buf->write(0);
		break;
	case 0x45:	// TimeChk
	case 0x46:	// AlmChk
		rsp_buf->write(0);
		break;
	}
}

