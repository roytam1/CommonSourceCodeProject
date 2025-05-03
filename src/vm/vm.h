/*
	Skelton for retropc emulator

	Author : Takeda.Toshiya
	Date   : 2006.08.18 -

	[ common header ]
*/

#ifndef _VM_H_
#define _VM_H_

// SORD m5
#ifdef _M5
#include "m5/m5.h"
#endif

// MITSUBISHI Elec. MULTI8
#ifdef _MULTI8
#include "multi8/multi8.h"
#endif

// SHARP MZ-2500
#ifdef _MZ2500
#include "mz2500/mz2500.h"
#endif

// TOSHIBA PASOPIA
#ifdef _PASOPIA
#include "pasopia/pasopia.h"
#endif

// TOSHIBA PASOPIA 7
#ifdef _PASOPIA7
#include "pasopia7/pasopia7.h"
#endif

// CASIO PV-1000
#ifdef _PV1000
#include "pv1000/pv1000.h"
#endif

// CASIO PV-2000
#ifdef _PV2000
#include "pv2000/pv2000.h"
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

#endif
