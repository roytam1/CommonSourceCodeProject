/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 input ]
*/

#include "emu.h"
#include "vm/vm.h"
#include "fifo.h"

#define KEY_KEEP_FRAMES 3

void EMU::initialize_input()
{
	// initialize status
	_memset(key_status, 0, sizeof(key_status));
	_memset(joy_status, 0, sizeof(joy_status));
	_memset(mouse_status, 0, sizeof(mouse_status));
	
	// initialize joysticks
	joy_num = joyGetNumDevs();
	if(joy_num > 0) {
		joyGetDevCaps(JOYSTICKID1, &joycaps[0], sizeof(JOYCAPS));
		uint32 x = (joycaps[0].wXmin + joycaps[0].wXmax) / 2;
		joy_xmin[0] = (joycaps[0].wXmin + x) / 2;
		joy_xmax[0] = (joycaps[0].wXmax + x) / 2;
		uint32 y = (joycaps[0].wYmin + joycaps[0].wYmax) / 2;
		joy_ymin[0] = (joycaps[0].wYmin + x) / 2;
		joy_ymax[0] = (joycaps[0].wYmax + x) / 2;
	}
	if(joy_num > 1) {
		joyGetDevCaps(JOYSTICKID2, &joycaps[1], sizeof(JOYCAPS));
		uint32 x = (joycaps[1].wXmin + joycaps[1].wXmax) / 2;
		joy_xmin[1] = (joycaps[1].wXmin + x) / 2;
		joy_xmax[1] = (joycaps[1].wXmax + x) / 2;
		uint32 y = (joycaps[1].wYmin + joycaps[1].wYmax) / 2;
		joy_ymin[1] = (joycaps[1].wYmin + x) / 2;
		joy_ymax[1] = (joycaps[1].wYmax + x) / 2;
	}
	
	// mouse emulation is disenabled
	mouse_enable = false;
	
#ifdef USE_AUTO_KEY
	// initialize autokey
	autokey_buffer = new FIFO(65536);
	autokey_buffer->clear();
	autokey_phase = autokey_shift = 0;
#endif
}

void EMU::release_input()
{
	// release mouse
	if(mouse_enable) {
		disenable_mouse();
	}
	
#ifdef USE_AUTO_KEY
	// release autokey buffer
	if(autokey_buffer) {
		delete autokey_buffer;
	}
#endif
}

void EMU::update_input()
{
	// update key status
	for(int i = 0; i < 256; i++) {
		if(key_status[i] & 0x7f) {
			key_status[i] = (key_status[i] & 0x80) | ((key_status[i] & 0x7f) - 1);
#ifdef NOTIFY_KEY_DOWN
			if(!key_status[i]) {
				vm->key_up(i);
			}
#endif
		}
	}
	
	// update joystick status
	_memset(joy_status, 0, sizeof(joy_status));
	if(joy_num > 0) {
		// joystick #1
		JOYINFO joyinfo;
		if(joyGetPos(JOYSTICKID1, &joyinfo) == JOYERR_NOERROR) {
			if(joyinfo.wYpos < joy_ymin[0]) joy_status[0] |= 0x01;
			if(joyinfo.wYpos > joy_ymax[0]) joy_status[0] |= 0x02;
			if(joyinfo.wXpos < joy_xmin[0]) joy_status[0] |= 0x04;
			if(joyinfo.wXpos > joy_xmax[0]) joy_status[0] |= 0x08;
			if(joyinfo.wButtons & JOY_BUTTON1) joy_status[0] |= 0x10;
			if(joyinfo.wButtons & JOY_BUTTON2) joy_status[0] |= 0x20;
			if(joyinfo.wButtons & JOY_BUTTON3) joy_status[0] |= 0x40;
			if(joyinfo.wButtons & JOY_BUTTON4) joy_status[0] |= 0x80;
		}
	}
	if(joy_num > 1) {
		// joystick #2
		JOYINFO joyinfo;
		if(joyGetPos(JOYSTICKID2, &joyinfo) == JOYERR_NOERROR) {
			if(joyinfo.wYpos < joy_ymin[1]) joy_status[1] |= 0x01;
			if(joyinfo.wYpos > joy_ymax[1]) joy_status[1] |= 0x02;
			if(joyinfo.wXpos < joy_xmin[1]) joy_status[1] |= 0x04;
			if(joyinfo.wXpos > joy_xmax[1]) joy_status[1] |= 0x08;
			if(joyinfo.wButtons & JOY_BUTTON1) joy_status[1] |= 0x10;
			if(joyinfo.wButtons & JOY_BUTTON2) joy_status[1] |= 0x20;
			if(joyinfo.wButtons & JOY_BUTTON3) joy_status[1] |= 0x40;
			if(joyinfo.wButtons & JOY_BUTTON4) joy_status[1] |= 0x80;
		}
	}
#ifdef USE_KEY_TO_JOY
	// emulate joystick #1 with keyboard
	if(key_status[0x26]) joy_status[0] |= 0x01;	// up
	if(key_status[0x28]) joy_status[0] |= 0x02;	// down
	if(key_status[0x25]) joy_status[0] |= 0x04;	// left
	if(key_status[0x27]) joy_status[0] |= 0x08;	// right
#endif
	
	// update mouse status
	_memset(mouse_status, 0, sizeof(mouse_status));
	if(mouse_enable) {
		// get current status
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(main_window_handle, &pt);
		mouse_status[0] = pt.x - window_width / 2;
		mouse_status[1] = pt.y - window_height / 2;
		mouse_status[2] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000 ? 1 : 0) | (GetAsyncKeyState(VK_RBUTTON) & 0x8000 ? 2 : 0);
		// move mouse cursor to the center of window
		if(!(mouse_status[0] == 0 && mouse_status[1] == 0)) {
			pt.x = window_width / 2;
			pt.y = window_height / 2;
			ClientToScreen(main_window_handle, &pt);
			SetCursorPos(pt.x, pt.y);
		}
	}
	
#ifdef USE_AUTO_KEY
	// auto key
	switch(autokey_phase) {
	case 1:
		if(autokey_buffer && !autokey_buffer->empty()) {
			// update shift key status
			int shift = autokey_buffer->read_not_remove(0) & 0x100;
			if(shift && !autokey_shift) {
				key_down(VK_SHIFT);
			}
			else if(!shift && autokey_shift) {
				key_up(VK_SHIFT);
			}
			autokey_shift = shift;
			autokey_phase++;
			break;
		}
	case 3:
		if(autokey_buffer && !autokey_buffer->empty()) {
			key_down(autokey_buffer->read_not_remove(0) & 0xff);
		}
		autokey_phase++;
		break;
	case USE_AUTO_KEY:
		if(autokey_buffer && !autokey_buffer->empty()) {
			key_up(autokey_buffer->read_not_remove(0) & 0xff);
		}
		autokey_phase++;
		break;
	case USE_AUTO_KEY_RELEASE:
		if(autokey_buffer && !autokey_buffer->empty()) {
			// wait enough while vm analyzes one line
			if(autokey_buffer->read() == 0xd) {
				autokey_phase++;
				break;
			}
		}
	case 30:
		if(autokey_buffer && !autokey_buffer->empty()) {
			autokey_phase = 1;
		}
		else {
			stop_auto_key();
		}
		break;
	default:
		if(autokey_phase) {
			autokey_phase++;
		}
	}
#endif
}

void EMU::key_down(int code)
{
#ifdef _PV2000
	// CASIO PV-2000 patch
	if((0x30 <= code && code <= 0x5a) || (0xba <= code && code <= 0xe2)) {
		_memset(key_status + 0x30, 0, 0x5a - 0x30 + 1);
		_memset(key_status + 0xba, 0, 0xe2 - 0xba + 1);
	}
	else if(code == 0x8) {
		code = 0x25;
	}
#endif
	if(code == VK_SHIFT) {
		if(GetAsyncKeyState(VK_LSHIFT) & 0x8000) key_status[VK_LSHIFT] = 0x80;
		if(GetAsyncKeyState(VK_RSHIFT) & 0x8000) key_status[VK_RSHIFT] = 0x80;
		if(!(key_status[VK_LSHIFT] || key_status[VK_RSHIFT])) key_status[VK_LSHIFT] = 0x80;
	}
	else if(code == VK_CONTROL) {
		if(GetAsyncKeyState(VK_LCONTROL) & 0x8000) key_status[VK_LCONTROL] = 0x80;
		if(GetAsyncKeyState(VK_RCONTROL) & 0x8000) key_status[VK_RCONTROL] = 0x80;
		if(!(key_status[VK_LCONTROL] || key_status[VK_RCONTROL])) key_status[VK_LCONTROL] = 0x80;
	}
	else if(code == VK_MENU) {
		if(GetAsyncKeyState(VK_LMENU) & 0x8000) key_status[VK_LMENU] = 0x80;
		if(GetAsyncKeyState(VK_RMENU) & 0x8000) key_status[VK_RMENU] = 0x80;
		if(!(key_status[VK_LMENU] || key_status[VK_RMENU])) key_status[VK_LMENU] = 0x80;
	}
	if(code == 0xf0) {
		key_status[code = VK_CAPITAL] = KEY_KEEP_FRAMES;
	}
	else if(code == 0xf2) {
		key_status[code = VK_KANA] = KEY_KEEP_FRAMES;
	}
	else if(code == 0xf3 || code == 0xf4) {
		key_status[code = VK_KANJI] = KEY_KEEP_FRAMES;
	}
	else {
		key_status[code] = 0x80;
	}
#ifdef NOTIFY_KEY_DOWN
	vm->key_down(code);
#endif
}

void EMU::key_up(int code)
{
#ifdef _PV2000
	if(code == 0x8) {
		code = 0x25;
	}
#endif
	if(code == VK_SHIFT) {
		if(!(GetAsyncKeyState(VK_LSHIFT) & 0x8000)) key_status[VK_LSHIFT] &= 0x7f;
		if(!(GetAsyncKeyState(VK_RSHIFT) & 0x8000)) key_status[VK_RSHIFT] &= 0x7f;
	}
	else if(code == VK_CONTROL) {
		if(!(GetAsyncKeyState(VK_LCONTROL) & 0x8000)) key_status[VK_LCONTROL] &= 0x7f;
		if(!(GetAsyncKeyState(VK_RCONTROL) & 0x8000)) key_status[VK_RCONTROL] &= 0x7f;
	}
	else if(code == VK_MENU) {
		if(!(GetAsyncKeyState(VK_LMENU) & 0x8000)) key_status[VK_LMENU] &= 0x7f;
		if(!(GetAsyncKeyState(VK_RMENU) & 0x8000)) key_status[VK_RMENU] &= 0x7f;
	}
	if(key_status[code]) {
		key_status[code] &= 0x7f;
#ifdef NOTIFY_KEY_DOWN
		if(!key_status[code]) {
			vm->key_up(code);
		}
#endif
	}
}

#ifdef USE_BUTTON
void EMU::press_button(int num)
{
	int code = buttons[num].code;
	
	if(code) {
		key_down(code);
		key_status[code] = KEY_KEEP_FRAMES;
	}
	else {
		// code=0: reset virtual machine
		vm->reset();
	}
}
#endif

void EMU::enable_mouse()
{
	// enable mouse emulation
	if(!mouse_enable) {
		// hide mouse cursor
		ShowCursor(FALSE);
		// move mouse cursor to the center of window
		POINT pt;
		pt.x = window_width / 2;
		pt.y = window_height / 2;
		ClientToScreen(main_window_handle, &pt);
		SetCursorPos(pt.x, pt.y);
	}
	mouse_enable = true;
}

void EMU::disenable_mouse()
{
	// disenable mouse emulation
	if(mouse_enable) {
		ShowCursor(TRUE);
	}
	mouse_enable = false;
}

void EMU::toggle_mouse()
{
	// toggle mouse enable / disenable
	if(mouse_enable) {
		disenable_mouse();
	}
	else {
		enable_mouse();
	}
}

#ifdef USE_AUTO_KEY
static int autokey_table[256] = {
	// 0x100: shift
	// 0x200: kana
	// 0x400: alphabet
	// 0x800: ALPHABET
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x00d,0x000,0x000,0x00d,0x000,0x000,
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
	0x020,0x131,0x132,0x133,0x134,0x135,0x136,0x137,0x138,0x139,0x1ba,0x1bb,0x0bc,0x0bd,0x0be,0x0bf,
	0x030,0x031,0x032,0x033,0x034,0x035,0x036,0x037,0x038,0x039,0x0ba,0x0bb,0x1bc,0x1bd,0x1be,0x1bf,
	0x0c0,0x441,0x442,0x443,0x444,0x445,0x446,0x447,0x448,0x449,0x44a,0x44b,0x44c,0x44d,0x44e,0x44f,
	0x450,0x451,0x452,0x453,0x454,0x455,0x456,0x457,0x458,0x459,0x45a,0x0db,0x0dc,0x0dd,0x0de,0x1e2,
	0x1c0,0x841,0x842,0x843,0x844,0x845,0x846,0x847,0x848,0x849,0x84a,0x84b,0x84c,0x84d,0x84e,0x84f,
	0x850,0x851,0x852,0x853,0x854,0x855,0x856,0x857,0x858,0x859,0x85a,0x1db,0x1dc,0x1dd,0x1de,0x000,
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
	// kana -->
	0x000,0x3be,0x3db,0x3dd,0x3bc,0x3bf,0x330,0x333,0x345,0x334,0x335,0x336,0x337,0x338,0x339,0x35a,
	0x2dc,0x233,0x245,0x234,0x235,0x236,0x254,0x247,0x248,0x2ba,0x242,0x258,0x244,0x252,0x250,0x243,
	0x251,0x241,0x25a,0x257,0x253,0x255,0x249,0x231,0x2bc,0x24b,0x246,0x256,0x232,0x2de,0x2bd,0x24a,
	0x24e,0x2dd,0x2bf,0x24d,0x237,0x238,0x239,0x24f,0x24c,0x2be,0x2bb,0x2e2,0x230,0x259,0x2c0,0x2db,
	// <--- kana
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
	0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000
};

void EMU::start_auto_key()
{
	stop_auto_key();
	
	if(OpenClipboard(NULL)) {
		HANDLE hClip = GetClipboardData(CF_TEXT);
		if(hClip) {
			autokey_buffer->clear();
			char* buf = (char*)GlobalLock(hClip);
			int size = strlen(buf), prev_kana = 0;
			for(int i = 0; i < size; i++) {
				int code = buf[i] & 0xff;
				if((0x81 <= code && code <= 0x9f) || 0xe0 <= code) {
					i++;	// kanji ?
					continue;
				}
				else if(code == 0xa) {
					continue;	// cr-lf
				}
				if((code = autokey_table[code]) != 0) {
					int kana = code & 0x200;
					if(prev_kana != kana) {
						autokey_buffer->write(0xf2);
					}
					prev_kana = kana;
#if defined(USE_AUTO_KEY_NO_CAPS)
					if((code & 0x100) && !(code & (0x400 | 0x800))) {
#elif defined(USE_AUTO_KEY_CAPS)
					if(code & (0x100 | 0x800)) {
#else
					if(code & (0x100 | 0x400)) {
#endif
						autokey_buffer->write((code & 0xff) | 0x100);
					}
					else {
						autokey_buffer->write(code & 0xff);
					}
				}
			}
			if(prev_kana) {
				autokey_buffer->write(0xf2);
			}
			GlobalUnlock(hClip);
			
			autokey_phase = 1;
			autokey_shift = 0;
		}
		CloseClipboard();
	}
}

void EMU::stop_auto_key()
{
	if(autokey_shift) {
		key_up(VK_SHIFT);
	}
	autokey_phase = autokey_shift = 0;
}
#endif
