
retro pc emulator common source code
								Dec 6, 2006

--- What's this ?

This archive includes the all source codes of:

	Emu5		SORD m5 emulator
	EmuGaki		CASIO PV-2000 emulator
	EmuLTI8		MITSUBISHI Elec. MULTI8 emulator
	EmuPIA7		TOSHIBA PASOPIA7 emulator
	EmuZ-2500	SHARP MZ-2500 emulator
	ePV-1000	CASIO PV-1000 emulator
	eRX-78		BANDAI RX-78 emulator
	eSCV		EPOCH Super Cassette Vision emulator

You can compile them with:

	Win32		Mircosoft Visual C++ 6.0 with Service Pack 6.
			Microsoft Visual C++ 2500


--- License

The copyright belongs to the author, but you can use the source codes
under the GNU GENERAL PUBLIC LICENSE.


--- Thanks

- vm/device.h
	XM6
- vm/fmgen/*
	M88
- vm/i8259.*
	MESS 8259 core
- vm/mb8877.*
	XM7
- vm/sn76489an.*
	MAME SN76496 core
- vm/tms9918a.*
	MAME TMS9928 core
- vm/upd7220.*
	Neko Project 2
- vm/upd765a.*
	M88 fdc/fdu core
- vm/upd7801.*
	MAME uPD7810 core.
- vm/w3100a.*
	Mr.Oh! Ishi
- vm/z80.*
	MAME Z80 core
- vm/m5/*
	MESS sord driver
- vm/mz2500/sasi.*
	X millenium
- vm/pv1000/*
	This machine is reverse engineered by Mr.Enri
	http://www2.odn.ne.jp/~haf09260/Pv1000/EnrPV.htm
- vm/pv2000/*
	This machine is reverse engineered by Mr.Enri
	http://www2.odn.ne.jp/~haf09260/Pv2000/EnrPV.htm
- vm/scv/*
	This machine is reverse engineered by Mr.Enri and Mr.333
	http://www2.odn.ne.jp/~haf09260/Scv/EnrScv.htm
- win32_sound.cpp
	XM7 (DirectSound), M88 (wavOut API)
- res/*.ico
	See also res/icon.txt

The design of emulation core is based on nester and XM6.


----------------------------------------
Takeda.Toshiya
t-takeda@m1.interq.or.jp
http://www1.interq.or.jp/~t-takeda/top.html

