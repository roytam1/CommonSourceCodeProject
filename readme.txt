retro pc emulator common source code
								12/31/2009

--- What's this ?

This archive includes the all source codes of:

	Emu5		SORD m5 emulator
	EmuGaki		CASIO PV-2000 emulator
	EmuLTI8		MITSUBISHI Elec. MULTI8 emulator
	EmuPIA		TOSHIBA PASOPIA emulator
	EmuPIA7		TOSHIBA PASOPIA7 emulator
	EmuZ-700	SHARP MZ-700 emulator
	EmuZ-2500	SHARP MZ-2500 emulator
	EmuZ-2800	SHARP MZ-2800 emulator
	EmuZ-3500	SHARP MZ-3500 emulator
	EmuZ-5500	SHARP MZ-5500 emulator
	EmuZ-6500	SHARP MZ-6500 emulator
	eBabbage-2nd	Gijutsu Hyoron Sha Babbage-2nd
	eFMR-30		FUJITSU FMR-30 emulator
	eFMR-50		FUJITSU FMR-50 emulator
	eFMR-60		FUJITSU FMR-60 emulator
	eHANDY98	NEC PC-98HA emulator
	eHC-40		EPSON HC-40/PX-4 emulator
	eHC-80		EPSON HC-80/PX-8/Geneva emulator
	eMYCOMZ-80A	Japan Electronics College MYCOMZ-80A emulator
	eN5200		NEC N5200 emulator
	ePC-8201	NEC PC-8201/PC-8201A emulator
	ePC-98LT	NEC PC-98LT emulator
	ePC-100		NEC PC-100 emulator
	ePV-1000	CASIO PV-1000 emulator
	ePyuTa		TOMY PyuTa and PyuTa Jr. emulator
	eQC-10		EPSON QC-10/QX-10 emulator
	eRX-78		BANDAI RX-78 emulator
	eSCV		EPOCH Super Cassette Vision emulator
	eTK-80BS	NEC TK-80BS (COMPO BS/80) emulator
	eX-07		CANON X-07 emulator
	eX1twin		SHARP X1twin emulator
	eYS-6464A	Shinko Sangyo YS-6464A

You can compile them with:

	Windows PC	Microsoft Visual C++ 2008 width SP1
	CE.NET 4.x	Microsoft eMbedded Visual C++ 4.0 width SP4


--- License

The copyright belongs to the author, but you can use the source codes
under the GNU GENERAL PUBLIC LICENSE.


--- Thanks

- vm/device.h
	XM6
- vm/fmgen/*
	M88/fmgen
- vm/hd63484.*
	MAME HD63484 core
- vm/huc6260.*
	Ootake CPU core
- vm/i86.*
	MAME i86 core
- vm/i386.*
	MAME i386 core
- vm/i8259.*
	Neko Project 2 and MESS 8259 core
- vm/mb8877.*
	XM7
- vm/sn76489an.*
	MAME SN76496 core
- vm/tf20.*
	vfloppy 1.4 by Mr.Justin Mitchell and Mr.Fred Jan Kraan
- vm/tms9918a.*
	MAME TMS9928 core
- vm/tms9995.*
	MAME TMS99xx core
- vm/upd71071.*
	88VA Eternal Grafx
- vm/upd7220.*
	Neko Project 2
- vm/upd765a.*
	M88 fdc/fdu core
- vm/upd7801.*
	MAME uPD7810 core
- vm/w3100a.*
	Mr.Oh!Ishi for the chip specification info
- vm/z80.*
	MAME Z80 core
- vm/fmr50/bios.*
	FM-TOWNS emulator on bochs
	UNZ pseudo BIOS
- vm/hc40/*
	Mr.Fred Han Kraan for EPSON HC-40/PX-4 hardware design info
- vm/hc80/*
	Mr.Fred Han Kraan for EPSON HC-80/PX-8/Geneva hardware design info
- vm/hc80/io.*
	Mr.Dennis Heynlein for intelligent ram disk unit
- vm/m5/*
	MESS sord driver
	Mr.Moriya for Sord M5 hardware design info
- vm/mycomz80a/mon.c
	Based on MON80 by Mr.Tesuya Suzuki
- vm/mz2500/sasi.*
	X millenium
- vm/pv1000/*
	Mr.Enri for CASIO PV-1000 hardware design info
- vm/pv2000/*
	Mr.Enri for CASIO PV-2000 hardware design info
- vm/pyuta/*
	MESS tutor driver
	Mr.Enri for TOMY PyuTa Jr. hardware design info
- vm/qc10/*
	Mr.Fred Han Kraan for EPSON QC-10/QX-10 hardware design info
- vm/scv/*
	Mr.Enri and Mr.333 for Epoch Super Cassette Vision hardware info
- vm/x07/io.*
	x07_emul by Mr.Jacques Brigaud
- vm/x1twin/pce.*
	Ootake (Joypad and Timer)
	xpce (PSG and VDC)
- vm/x1twin/sub.*
	X millenium T-tune
- win32_sound.cpp
	XM7 for DirectSound implement
	M88 for wavOut API implement
- res/*.ico
	Mr.Temmaru and Mr.Marukun
	See also res/icon.txt

- emulation core design
	nester and XM6

----------------------------------------
TAKEDA, toshiya
t-takeda@m1.interq.or.jp
http://homepage3.nifty.com/takeda-toshiya/
