/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ common header ]
*/

#ifndef _VM_H_
#define _VM_H_

// FUJITSU FMR-50
#ifdef _FMR50
#include "fmr50/fmr50.h"
#endif

// EPSON HC-40
#ifdef _HC40
#include "hc40/hc40.h"
#endif

// EPSON HC-80
#ifdef _HC80
#include "hc80/hc80.h"
#endif

// SORD m5
#ifdef _M5
#include "m5/m5.h"
#endif

// MITSUBISHI Elec. MULTI8
#ifdef _MULTI8
#include "multi8/multi8.h"
#endif

// SHARP MZ-700
#ifdef _MZ700
#include "mz700/mz700.h"
#endif

// SHARP MZ-2500
#ifdef _MZ2500
#include "mz2500/mz2500.h"
#endif

// SHARP MZ-2800
#ifdef _MZ2800
#include "mz2800/mz2800.h"
#endif

// SHARP MZ-3500
#ifdef _MZ3500
#include "mz3500/mz3500.h"
#endif

// SHARP MZ-5500
#ifdef _MZ5500
#include "mz5500/mz5500.h"
#endif

// TOSHIBA PASOPIA
#ifdef _PASOPIA
#include "pasopia/pasopia.h"
#endif

// TOSHIBA PASOPIA 7
#ifdef _PASOPIA7
#include "pasopia7/pasopia7.h"
#endif

// NEC PC-98HA
#ifdef _PC98HA
#include "pc98ha/pc98ha.h"
#endif

// NEC PC-100
#ifdef _PC100
#include "pc100/pc100.h"
#endif

// SHARP PC-3200
#ifdef _PC3200
#include "pc3200/pc3200.h"
#endif

// CASIO PV-1000
#ifdef _PV1000
#include "pv1000/pv1000.h"
#endif

// CASIO PV-2000
#ifdef _PV2000
#include "pv2000/pv2000.h"
#endif

// TOMY PYUTA
#ifdef _PYUTA
#include "pyuta/pyuta.h"
#endif

// EPSON QC-10
#ifdef _QC10
#include "qc10/qc10.h"
#endif

// BANDAI RX-78
#ifdef _RX78
#include "rx78/rx78.h"
#endif

// EPOCH Super Cassette Vision
#ifdef _SCV
#include "scv/scv.h"
#endif

// CANON X-07
#ifdef _X07
#include "x07/x07.h"
#endif

#endif
