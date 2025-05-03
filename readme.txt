retro pc emulator common source code
								4/12/2008

--- What's this ?

This archive includes the all source codes of:

	Emu5		SORD m5 emulator
	EmuGaki		CASIO PV-2000 emulator
	EmuLTI8		MITSUBISHI Elec. MULTI8 emulator
	EmuPIA		TOSHIBA PASOPIA emulator
	EmuPIA7		TOSHIBA PASOPIA7 emulator
	EmuZ-2500	SHARP MZ-2500 emulator
	EmuZ-2800	SHARP MZ-2800 emulator
	EmuZ-5500	SHARP MZ-5500 emulator
	eHC-40		EPSON HC-40/PX-4 emulator
	eHC-80		EPSON HC-80/PX-8/Geneva emulator
	ePV-1000	CASIO PV-1000 emulator
	ePyuTa		TOMY PyuTa and PyuTa Jr. emulator
	eQC-10		EPSON QC-10/QX-10 emulator
	eRX-78		BANDAI RX-78 emulator
	eSCV		EPOCH Super Cassette Vision emulator
	eX-07		CANON X-07 emulator

You can compile them with:

	Windows PC	Microsoft Visual C++ 2005 width SP1
	CE.NET 4.x	Microsoft eMbedded Visual C++ 4.0 width SP4


--- License

The copyright belongs to the author, but you can use the source codes
under the GNU GENERAL PUBLIC LICENSE.


--- Thanks

- vm/device.h
	XM6
- vm/fmgen/*
	M88/fmgen
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
- vm/x86.*
	MAME x86 ore
- vm/w3100a.*
	Mr.Oh!Ishi for the chip specification info
- vm/z80.*
	MAME Z80 core
- vm/hc40/*
	Mr.Fred Han Kraan for EPSON HC-40/PX-4 hardware design info
- vm/hc80/*
	Mr.Fred Han Kraan for EPSON HC-80/PX-8/Geneva hardware design info
- vm/m5/*
	MESS sord driver
	Mr.Moriya for Sord M5 hardware design info
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
http://www1.interq.or.jp/~t-takeda/top.html
