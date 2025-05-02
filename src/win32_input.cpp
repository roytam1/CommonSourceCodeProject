/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ win32 input ]
*/

#include "emu.h"
#include "vm/vm.h"

#define KEY_KEEP_FRAMES 3

void EMU::initialize_input()
{
	// initialize status
	_memset(key_status, 0, sizeof(key_status));
	_memset(joy_status, 0, sizeof(joy_status));
	_memset(mouse_status, 0, sizeof(mouse_status));
	
#ifndef _WIN32_WCE
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
}

void EMU::release_input()
{
	// release mouse
	if(mouse_enable)
		disenable_mouse();
}

void EMU::update_input()
{
	// update key status
	for(int i = 0; i < 256; i++) {
		if(key_status[i] & 0x7f)
			key_status[i] = (key_status[i] & 0x80) | ((key_status[i] & 0x7f) - 1);
	}
	
	// update joystick status
#ifndef _WIN32_WCE
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
#endif
#ifdef USE_JOYKEY
	// emulate joystick #1 with keyboard
	if(key_status[0x26]) joy_status[0] |= 0x01;	// up
	if(key_status[0x28]) joy_status[0] |= 0x02;	// down
	if(key_status[0x25]) joy_status[0] |= 0x04;	// left
	if(key_status[0x27]) joy_status[0] |= 0x08;	// right
#endif
	// update mouse status
	_memset(mouse_status, 0, sizeof(mouse_status));
	
#ifndef _WIN32_WCE
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
	if(code == 0xf0) // CAPS
		key_status[VK_CAPITAL] = KEY_KEEP_FRAMES;
	else if(code == 0xf2) // KANA
		key_status[VK_KANA] = KEY_KEEP_FRAMES;
	else if(code == 0xf3 || code == 0xf4) // KANJI
		key_status[VK_KANJI] = KEY_KEEP_FRAMES;
	else
#ifdef _WIN32_WCE
		// keep pressed for some frames
		key_status[code] = 0x80 | KEY_KEEP_FRAMES;
#else
		key_status[code] = 0x80;
#endif
#ifdef _PV2000
	// CASIO PV-2000 patch
	if(!(code == 0x9 || code == 0x10 || code == 0x11))
		vm->key_down();
#endif
}

void EMU::key_up(int code)
{
#ifdef _PV2000
	if(code == 0x8)
		code = 0x25;
#endif
	key_status[code] &= 0x7f;
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

