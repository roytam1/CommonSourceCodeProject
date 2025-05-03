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
	
#ifdef SUPPORT_JOYSTICK
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
#endif
	// mouse emulation is disenabled
	mouse_enable = false;
	
#ifdef SUPPORT_AUTO_KEY
	// initialize autokey
	autokey_buffer = new FIFO(65536);
	autokey_buffer->clear();
	autokey_phase = autokey_shift = 0;
#endif
}

void EMU::release_input()
{
	// release mouse
	if(mouse_enable)
		disenable_mouse();
	
#ifdef SUPPORT_AUTO_KEY
	// release autokey buffer
	if(autokey_buffer)
		delete autokey_buffer;
#endif
}

void EMU::update_input()
{
	// update key status
	for(int i = 0; i < 256; i++) {
		if(key_status[i] & 0x7f) {
			key_status[i] = (key_status[i] & 0x80) | ((key_status[i] & 0x7f) - 1);
#ifdef NOTIFY_KEY_DOWN
			if(!key_status[i])
				vm->key_up(i);
#endif
		}
	}
	
	// update joystick status
	_memset(joy_status, 0, sizeof(joy_status));
#ifdef SUPPORT_JOYSTICK
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
#endif
#ifdef USE_KEY_TO_JOY
	// emulate joystick #1 with keyboard
	if(key_status[0x26]) joy_status[0] |= 0x01;	// up
	if(key_status[0x28]) joy_status[0] |= 0x02;	// down
	if(key_status[0x25]) joy_status[0] |= 0x04;	// left
	if(key_status[0x27]) joy_status[0] |= 0x08;	// right
#endif
	// update mouse status
	_memset(mouse_status, 0, sizeof(mouse_status));
#ifdef SUPPORT_MOUSE
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
#endif
#ifdef SUPPORT_AUTO_KEY
	// auto key
	switch(autokey_phase)
	{
	case 1:
		if(autokey_buffer && !autokey_buffer->empty()) {
			// update shift key status
			int shift = autokey_buffer->read_not_remove(0) & 0x100;
			if(shift && !autokey_shift)
				key_down(VK_SHIFT);
			else if(!shift && autokey_shift)
				key_up(VK_SHIFT);
			autokey_shift = shift;
			autokey_phase++;
			break;
		}
	case 3:
		if(autokey_buffer && !autokey_buffer->empty())
			key_down(autokey_buffer->read_not_remove(0) & 0xff);
		autokey_phase++;
		break;
	case USE_AUTO_KEY:
		if(autokey_buffer && !autokey_buffer->empty())
			key_up(autokey_buffer->read_not_remove(0) & 0xff);
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
		if(autokey_buffer && !autokey_buffer->empty())
			autokey_phase = 1;
		else
			stop_auto_key();
		break;
	default:
		if(autokey_phase)
			autokey_phase++;
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
	else if(code == 0x8)
		code = 0x25;
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
	if(code == 0xf0) // CAPS
		key_status[code = VK_CAPITAL] = KEY_KEEP_FRAMES;
	else if(code == 0xf2) // KANA
		key_status[code = VK_KANA] = KEY_KEEP_FRAMES;
	else if(code == 0xf3 || code == 0xf4) // KANJI
		key_status[code = VK_KANJI] = KEY_KEEP_FRAMES;
	else
#ifdef _WIN32_WCE
		// keep pressed for some frames
		key_status[code] = 0x80 | KEY_KEEP_FRAMES;
#else
		key_status[code] = 0x80;
#endif
#ifdef NOTIFY_KEY_DOWN
	vm->key_down(code);
#endif
}

void EMU::key_up(int code)
{
#ifdef _PV2000
	if(code == 0x8)
		code = 0x25;
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
		if(!key_status[code])
			vm->key_up(code);
#endif
	}
}

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
	if(mouse_enable)
		ShowCursor(TRUE);
	mouse_enable = false;
}

void EMU::toggle_mouse()
{
	// toggle mouse enable / disenable
	if(mouse_enable)
		disenable_mouse();
	else
		enable_mouse();
}

#ifdef SUPPORT_AUTO_KEY
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
				else if(code == 0xa)
					continue;	// cr-lf
				if((code = autokey_table[code]) != 0) {
					int kana = code & 0x200;
					if(prev_kana != kana)
						autokey_buffer->write(0xf2);
					prev_kana = kana;
#if defined(USE_AUTO_KEY_NO_CAPS)
					if((code & 0x100) && !(code & (0x400 | 0x800)))
#elif defined(USE_AUTO_KEY_CAPS)
					if(code & (0x100 | 0x800))
#else
					if(code & (0x100 | 0x400))
#endif
						autokey_buffer->write((code & 0xff) | 0x100);
					else
						autokey_buffer->write(code & 0xff);
				}
			}
			if(prev_kana)
				autokey_buffer->write(0xf2);
			GlobalUnlock(hClip);
			
			autokey_phase = 1;
			autokey_shift = 0;
		}
		CloseClipboard();
	}
}

void EMU::stop_auto_key()
{
	if(autokey_shift)
		key_up(VK_SHIFT);
	autokey_phase = autokey_shift = 0;
}
#endif
