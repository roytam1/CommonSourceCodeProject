/** iP6: PC-6000/6600 series emualtor ************************/
/**                                                         **/
/**                         Refresh.c                       **/
/**                                                         **/
/** by ISHIOKA Hiroshi 1998,1999                            **/
/** This code is based on fMSX written by Marat Fayzullin   **/
/** and Adaptions for any X-terminal by Arnold Metselaar    **/
/*************************************************************/

/*
	NEC PC-6001 Emulator 'yaPC-6001'
	NEC PC-6601 Emulator 'yaPC-6601'

	Author : tanam
	Date   : 2013.07.15-

	[ memory ]
*/

#include "memory.h"
#include "../../fileio.h"
#include "../z80.h"

#ifndef _PC6001
#define SETSCRVARM1(y1)	dest = &screen[y1][0];W=0;
#define SETSCRVARM5(y1)	dest = &screen[y1][0];W=0;
#define M1HEIGHT 192
#define M5HEIGHT 200
#define SeqPix21(c) dest[X*8+W]=c;W++;
#define SeqPix41(c) dest[X*8+W]=c;W++;dest[X*8+W]=c;W++;

// RefreshScr10: N60-BASIC select function
void MEMORY::RefreshScr10()
{
	if ((*VRAM&0x80) == 0x00)
		RefreshScr11();
	else
		switch (*(VRAM)&0x1C) {
		case 0x00: case 0x10: //  64x 64 color / 128x 64
			RefreshScr13a(); break;
		case 0x08: // 128x 64 color
			RefreshScr13b(); break;
		case 0x18: // 128x 96
			RefreshScr13c(); break;
		case 0x04: // 128x 96 color
			RefreshScr13d(); break;
		case 0x14: // 128x192
			RefreshScr13e(); break;
		default: // 128x192 color / 256x192
			RefreshScr13(); break;
		}
}

/** RefreshScr11: N60-BASIC screen 1,2 ***********************/
void MEMORY::RefreshScr11()
{
  register byte X,Y,K;
  register int FC,BC;
  register byte *S,*T1,*T2;
  byte *G;
  
  G = CGROM;		/* CGROM */ 
  T1 = VRAM;		/* attribute data */
  T2 = VRAM+0x0200;	/* ascii/semi-graphic data */
  for(Y=0; Y<M1HEIGHT; Y++) {
    SETSCRVARM1(Y);	/* Drawing area */
    for(X=0; X<32; X++, T1++, T2++) {
      /* get CGROM address and color */
      if (*T1&0x40) {	/* if semi-graphic */
	if (*T1&0x20) {		/* semi-graphic 6 */
	  S = G+((*T2&0x3f)<<4)+0x1000;
	  FC = BPal12[(*T1&0x02)<<1|(*T2)>>6]; BC = BPal[8];
	} else {		/* semi-graphic 4 */
	  S = G+((*T2&0x0f)<<4)+0x2000;
	  FC = BPal12[(*T2&0x70)>>4]; BC = BPal[8];
	}
      } else {		/* if normal character */
	S = G+((*T2)<<4); 
	FC = BPal11[(*T1&0x03)]; BC = BPal11[(*T1&0x03)^0x01];
      }
      K=*(S+Y%12);
W=0;
      SeqPix21(K&0x80? FC:BC); SeqPix21(K&0x40? FC:BC);
      SeqPix21(K&0x20? FC:BC); SeqPix21(K&0x10? FC:BC);
      SeqPix21(K&0x08? FC:BC); SeqPix21(K&0x04? FC:BC);
      SeqPix21(K&0x02? FC:BC); SeqPix21(K&0x01? FC:BC);
    }
///    if ((scale==2) && !IntLac) NOINTLACM1(Y);
    if (Y%12!=11) { T1-=32; T2-=32; }
  }
///  if(EndOfFrame) PutImage();
}

/** RefreshScr13: N60-BASIC screen 3,4 ***********************/
void MEMORY::RefreshScr13()
{
  register byte X,Y;
  register byte *T1,*T2;
  byte attr;

  T1 = VRAM;		/* attribute data */
  T2 = VRAM+0x0200;	/* graphic data */
  for (Y=0; Y<M1HEIGHT; Y++) {
    SETSCRVARM1(Y);	/* Drawing area */
    for (X=0; X<32; X++,T1++,T2++) {
      if (*T1&0x10) { /* 256x192 (SCREEN 4) */
///	if (scr4col) {
///	  attr = (*T1&0x02)<<1;
///	  SeqPix41(BPal15[attr|(*T2&0xC0)>>6]);
///	  SeqPix41(BPal15[attr|(*T2&0x30)>>4]);
///	  SeqPix41(BPal15[attr|(*T2&0x0C)>>2]);
///	  SeqPix41(BPal15[attr|(*T2&0x03)   ]);
///	} else {
	  attr = *T1&0x02;
W=0;
	  SeqPix21(BPal14[attr|(*T2&0x80)>>7]);
	  SeqPix21(BPal14[attr|(*T2&0x40)>>6]);
	  SeqPix21(BPal14[attr|(*T2&0x20)>>5]);
	  SeqPix21(BPal14[attr|(*T2&0x10)>>4]);
	  SeqPix21(BPal14[attr|(*T2&0x08)>>3]);
	  SeqPix21(BPal14[attr|(*T2&0x04)>>2]);
	  SeqPix21(BPal14[attr|(*T2&0x02)>>1]);
	  SeqPix21(BPal14[attr|(*T2&0x01)   ]);
///	}
      } else { /* 128x192 color (SCREEN 3) */
	attr = (*T1&0x02)<<1;
W=0;
	SeqPix41(BPal13[attr|(*T2&0xC0)>>6]);
	SeqPix41(BPal13[attr|(*T2&0x30)>>4]);
	SeqPix41(BPal13[attr|(*T2&0x0C)>>2]);
	SeqPix41(BPal13[attr|(*T2&0x03)   ]);
      }
    }
    if (T1 == VRAM+0x200) T1=VRAM;
///    if ((scale==2) && !IntLac) NOINTLACM1(Y);
  }
///  if(EndOfFrame) PutImage();
}

/** RefreshScr13a: N60-BASIC screen 3,4 **********************/
void MEMORY::RefreshScr13a() /*  64x 64 color / 128x 64 */
{
  register byte X,Y;
  register byte *T1,*T2;
  byte attr;
  int L;

  T1 = VRAM;		/* attribute data */
  T2 = VRAM+0x0200;	/* graphic data */
  for (Y=0; Y<M1HEIGHT; Y++) {
    SETSCRVARM1(Y);	/* Drawing area */
    for (X=0; X<16; X++,T1++,T2++) {
      if (*T1&0x10) { /* 128x 64 */
///	if (scr4col) {
///	  attr = (*T1&0x02)<<1;
///	  SeqPix41(L=BPal15[attr|(*T2&0xC0)>>6]);
///	  SeqPix41(L);
///	  SeqPix41(L=BPal15[attr|(*T2&0x30)>>4]);
///	  SeqPix41(L);
///	  SeqPix41(L=BPal15[attr|(*T2&0x0C)>>2]);
///	  SeqPix41(L);
///	  SeqPix41(L=BPal15[attr|(*T2&0x03)   ]);
///	  SeqPix41(L);
///	} else { /*  64x 64 color */
	  attr = *T1&0x02;
W=0;
	  SeqPix41(BPal14[attr|(*T2&0x80)>>7]);
	  SeqPix41(BPal14[attr|(*T2&0x40)>>6]);
	  SeqPix41(BPal14[attr|(*T2&0x20)>>5]);
	  SeqPix41(BPal14[attr|(*T2&0x10)>>4]);
	  SeqPix41(BPal14[attr|(*T2&0x08)>>3]);
	  SeqPix41(BPal14[attr|(*T2&0x04)>>2]);
	  SeqPix41(BPal14[attr|(*T2&0x02)>>1]);
	  SeqPix41(BPal14[attr|(*T2&0x01)   ]);
///	}
      } else { /*  64x 64 color */
	attr = (*T1&0x02)<<1;
W=0;
	SeqPix41(L=BPal13[attr|(*T2&0xC0)>>6]);
	SeqPix41(L);
	SeqPix41(L=BPal13[attr|(*T2&0x30)>>4]);
	SeqPix41(L);
	SeqPix41(L=BPal13[attr|(*T2&0x0C)>>2]);
	SeqPix41(L);
	SeqPix41(L=BPal13[attr|(*T2&0x03)   ]);
	SeqPix41(L);
      }
    }
    if (Y%3 != 2) { T1-=16; T2-=16; }
    else if (T1 == VRAM+0x200) T1=VRAM;
///    if ((scale==2) && !IntLac) NOINTLACM1(Y);
  }
///  if(EndOfFrame) PutImage();
}

/** RefreshScr13b: N60-BASIC screen 3,4 **********************/
void MEMORY::RefreshScr13b() /* 128x 64 color */
{
  register byte X,Y;
  register byte *T1,*T2;
  byte attr;

  T1 = VRAM;		/* attribute data */
  T2 = VRAM+0x0200;	/* graphic data */
  for (Y=0; Y<M1HEIGHT; Y++) {
    SETSCRVARM1(Y);	/* Drawing area */
    for (X=0; X<32; X++,T1++,T2++) {
W=0;
      attr = (*T1&0x02)<<1;
      SeqPix41(BPal13[attr|(*T2&0xC0)>>6]);
      SeqPix41(BPal13[attr|(*T2&0x30)>>4]);
      SeqPix41(BPal13[attr|(*T2&0x0C)>>2]);
      SeqPix41(BPal13[attr|(*T2&0x03)   ]);
    }
    if (Y%3 != 2) { T1-=32; T2-=32; }
    else if (T1 == VRAM+0x200) T1=VRAM;
///    if ((scale==2) && !IntLac) NOINTLACM1(Y);
  }
///  if(EndOfFrame) PutImage();
}

/** RefreshScr13c: N60-BASIC screen 3,4 **********************/
void MEMORY::RefreshScr13c() /* 128x 96 */
{
  register byte X,Y;
  register byte *T1,*T2;
  byte attr;
///  int L;

  T1 = VRAM;		/* attribute data */
  T2 = VRAM+0x0200;	/* graphic data */
  for (Y=0; Y<M1HEIGHT; Y++) {
    SETSCRVARM1(Y);	/* Drawing area */
    for (X=0; X<16; X++,T1++,T2++) {
///      if (scr4col) {
///	attr = (*T1&0x02)<<1;
///	SeqPix41(L=BPal15[attr|(*T2&0xC0)>>6]);
///	SeqPix41(L);
///	SeqPix41(L=BPal15[attr|(*T2&0x30)>>4]);
///	SeqPix41(L);
///	SeqPix41(L=BPal15[attr|(*T2&0x0C)>>2]);
///	SeqPix41(L);
///	SeqPix41(L=BPal15[attr|(*T2&0x03)   ]);
///	SeqPix41(L);
///      } else {
	attr = *T1&0x02;
W=0;
	SeqPix41(BPal14[attr|(*T2&0x80)>>7]);
	SeqPix41(BPal14[attr|(*T2&0x40)>>6]);
	SeqPix41(BPal14[attr|(*T2&0x20)>>5]);
	SeqPix41(BPal14[attr|(*T2&0x10)>>4]);
	SeqPix41(BPal14[attr|(*T2&0x08)>>3]);
	SeqPix41(BPal14[attr|(*T2&0x04)>>2]);
	SeqPix41(BPal14[attr|(*T2&0x02)>>1]);
	SeqPix41(BPal14[attr|(*T2&0x01)   ]);
///      }
    }
    if (!(Y&1)) { T1-=16; T2-=16; }
    else if (T1 == VRAM+0x200) T1=VRAM;
///    if ((scale==2) && !IntLac) NOINTLACM1(Y);
  }
///  if(EndOfFrame) PutImage();
}

/** RefreshScr13d: N60-BASIC screen 3,4 **********************/
void MEMORY::RefreshScr13d() /* 128x 96 color */
{
  register byte X,Y;
  register byte *T1,*T2;
  byte attr;

  T1 = VRAM;		/* attribute data */
  T2 = VRAM+0x0200;	/* graphic data */
  for (Y=0; Y<M1HEIGHT; Y++) {
    SETSCRVARM1(Y);	/* Drawing area */
    for (X=0; X<32; X++,T1++,T2++) {
      attr = (*T1&0x02)<<1;
W=0;
      SeqPix41(BPal13[attr|(*T2&0xC0)>>6]);
      SeqPix41(BPal13[attr|(*T2&0x30)>>4]);
      SeqPix41(BPal13[attr|(*T2&0x0C)>>2]);
      SeqPix41(BPal13[attr|(*T2&0x03)   ]);
    }
    if (!(Y&1)) { T1-=32; T2-=32; }
    else if (T1 == VRAM+0x200) T1=VRAM;
///    if ((scale==2) && !IntLac) NOINTLACM1(Y);
  }
///  if(EndOfFrame) PutImage();
}

/** RefreshScr13e: N60-BASIC screen 3,4 **********************/
void MEMORY::RefreshScr13e() /* 128x192 */
{
  register byte X,Y;
  register byte *T1,*T2;
  byte attr;
///  int L;

  T1 = VRAM;		/* attribute data */
  T2 = VRAM+0x0200;	/* graphic data */
  for (Y=0; Y<M1HEIGHT; Y++) {
    SETSCRVARM1(Y);	/* Drawing area */
    for (X=0; X<16; X++,T1++,T2++) {
///      if (scr4col) {
///	attr = (*T1&0x02)<<1;
///	SeqPix41(L=BPal15[attr|(*T2&0xC0)>>6]);
///	SeqPix41(L);
///	SeqPix41(L=BPal15[attr|(*T2&0x30)>>4]);
///	SeqPix41(L);
///	SeqPix41(L=BPal15[attr|(*T2&0x0C)>>2]);
///	SeqPix41(L);
///	SeqPix41(L=BPal15[attr|(*T2&0x03)   ]);
///	SeqPix41(L);
///      } else {
	attr = *T1&0x02;
W=0;
	SeqPix41(BPal14[attr|(*T2&0x80)>>7]);
	SeqPix41(BPal14[attr|(*T2&0x40)>>6]);
	SeqPix41(BPal14[attr|(*T2&0x20)>>5]);
	SeqPix41(BPal14[attr|(*T2&0x10)>>4]);
	SeqPix41(BPal14[attr|(*T2&0x08)>>3]);
	SeqPix41(BPal14[attr|(*T2&0x04)>>2]);
	SeqPix41(BPal14[attr|(*T2&0x02)>>1]);
	SeqPix41(BPal14[attr|(*T2&0x01)   ]);
///      }
    }
    if (T1 == VRAM+0x200) T1=VRAM;
///    if ((scale==2) && !IntLac) NOINTLACM1(Y);
  }
///  if(EndOfFrame) PutImage();
}

/** RefreshScr51: N60m/66-BASIC screen 1,2 *******************/
void MEMORY::RefreshScr51()
{
  register byte X,Y,K;
  register int FC,BC;
  register byte *S,*T1,*T2;
  byte *G;

  G = CGROM;		/* CGROM */ 
  T1 = VRAM;		/* attribute data */
  T2 = VRAM+0x0400;	/* ascii/semi-graphic data */
  for(Y=0; Y<M5HEIGHT; Y++) {
    SETSCRVARM5(Y);	/* Drawing area */
    for(X=0; X<40; X++, T1++, T2++) {
      /* get CGROM address and color */
      S = G+(*T2<<4)+(*T1&0x80?0x1000:0);
      FC = BPal[(*T1)&0x0F]; BC = BPal[(((*T1)&0x70)>>4)|CSS2];
      K=*(S+Y%10);
W=0;
      SeqPix21(K&0x80? FC:BC); SeqPix21(K&0x40? FC:BC);
      SeqPix21(K&0x20? FC:BC); SeqPix21(K&0x10? FC:BC);
      SeqPix21(K&0x08? FC:BC); SeqPix21(K&0x04? FC:BC);
      SeqPix21(K&0x02? FC:BC); SeqPix21(K&0x01? FC:BC);
    }
///    if ((scale==2) && !IntLac) NOINTLACM5(Y);
    if (Y%10!=9) { T1-=40; T2-=40; }
  }
///  if(EndOfFrame) PutImage();
}

/** RefreshScr53: N60m/66-BASIC screen 3 *********************/
void MEMORY::RefreshScr53()
{
  register byte X,Y;
  register byte *T1,*T2;
  
  T1 = VRAM;		/* attribute data */
  T2 = VRAM+0x2000;	/* graphic data */
  for(Y=0; Y<M5HEIGHT; Y++) {
    SETSCRVARM5(Y);	/* Drawing area */
    for(X=0; X<40; X++) {
W=0;
      SeqPix41(BPal53[CSS3|((*T1)&0xC0)>>6|((*T2)&0xC0)>>4]);
      SeqPix41(BPal53[CSS3|((*T1)&0x30)>>4|((*T2)&0x30)>>2]);
      SeqPix41(BPal53[CSS3|((*T1)&0x0C)>>2|((*T2)&0x0C)   ]);
      SeqPix41(BPal53[CSS3|((*T1)&0x03)   |((*T2)&0x03)<<2]);
      T1++; T2++;
    }
///    if ((scale==2) && !IntLac) NOINTLACM5(Y);
  }
///  if(EndOfFrame) PutImage();
}

/** RefreshScr54: N60m/66-BASIC screen 4 *********************/
void MEMORY::RefreshScr54()
{
  register byte X,Y;
  register byte *T1,*T2;
  byte cssor;

  T1 = VRAM;		/* attribute data */
  T2 = VRAM+0x2000;	/* graphic data */
  /* CSS OR */
  cssor = CSS3|CSS2|CSS1;
  for(Y=0; Y<M5HEIGHT; Y++) {
    SETSCRVARM5(Y);	/* Drawing area */
    for(X=0; X<40; X++) {
W=0;
      SeqPix21(BPal53[cssor|((*T1)&0x80)>>7|((*T2)&0x80)>>6]);
      SeqPix21(BPal53[cssor|((*T1)&0x40)>>6|((*T2)&0x40)>>5]);
      SeqPix21(BPal53[cssor|((*T1)&0x20)>>5|((*T2)&0x20)>>4]);
      SeqPix21(BPal53[cssor|((*T1)&0x10)>>4|((*T2)&0x10)>>3]);
      SeqPix21(BPal53[cssor|((*T1)&0x08)>>3|((*T2)&0x08)>>2]);
      SeqPix21(BPal53[cssor|((*T1)&0x04)>>2|((*T2)&0x04)>>1]);
      SeqPix21(BPal53[cssor|((*T1)&0x02)>>1|((*T2)&0x02)   ]);
      SeqPix21(BPal53[cssor|((*T1)&0x01)   |((*T2)&0x01)<<1]);
      T1++;T2++;
    }
///    if ((scale==2) && !IntLac) NOINTLACM5(Y);
  }
///  if(EndOfFrame) PutImage();
}

void MEMORY::draw_screen()
{
	if (CRTMode1) {
		if (CRTMode2)
			if (CRTMode3) RefreshScr54();
			else RefreshScr53();
		else RefreshScr51();
		// copy to screen
		for(int y = 0; y < 200; y++) {
			scrntype* dest = emu->screen_buffer(y);
			for(int x = 0; x < 320; x++) {
				dest[x] = palette[screen[y][x]];
			}
		}
	} else {
		RefreshScr10();
		// copy to screen
		for(int y = 0; y < 200; y++) {
			scrntype* dest = emu->screen_buffer(y);
			for(int x = 0; x < 320; x++) {
				if (x >= 32 && x < 288 && y >=4 && y < 196) dest[x] = palette[screen[y-4][x-32]];
				else dest[x] = palette[8];
			}
		}
	}
}

void MEMORY::initialize()
{
	// for mkII/66
	int Pal11[ 4] = { 15, 8,10, 8 };
	int Pal12[ 8] = { 10,11,12, 9,15,14,13, 1 };
	int Pal13[ 8] = { 10,11,12, 9,15,14,13, 1 };
	int Pal14[ 4] = {  8,10, 8,15 };
	int Pal15[ 8] = {  8,9,11,14, 8,9,14,15 };
	int Pal53[32] = {  0, 4, 1, 5, 2, 6, 3, 7, 8,12, 9,13,10,14,11,15,
		10,11,12, 9,15,14,13, 1,10,11,12, 9,15,14,13, 1 };
	
	for(int i=0;i<32;i++) {
		BPal53[i]=Pal53[i];
		if (i>15) continue;
		BPal[i]=i;
		if (i>7) continue;
		BPal12[i]=Pal12[i];
		BPal13[i]=Pal13[i];
		BPal15[i]=Pal15[i];
		if (i>3) continue;
		BPal11[i]=Pal11[i];
		BPal14[i]=Pal14[i];
	}
	
	// mk2` palette
	palette[ 0] = RGB_COLOR(0x14,0x14,0x14); // COL065			= 141414			;mk2` “§–¾(•)
	palette[ 1] = RGB_COLOR(0xFF,0xAC,0x00); // COL066			= FFAC00			;mk2` žò
	palette[ 2] = RGB_COLOR(0x00,0xFF,0xAC); // COL067			= 00FFAC			;mk2` Â—Î
	palette[ 3] = RGB_COLOR(0xAC,0xFF,0x00); // COL068			= ACFF00			;mk2` ‰©—Î
	palette[ 4] = RGB_COLOR(0xAC,0x00,0xFF); // COL069			= AC00FF			;mk2` ÂŽ‡
	palette[ 5] = RGB_COLOR(0xFF,0x00,0xAC); // COL070			= FF00AC			;mk2` ÔŽ‡
	palette[ 6] = RGB_COLOR(0x00,0xAC,0xFF); // COL071			= 00ACFF			;mk2` ‹óF
	palette[ 7] = RGB_COLOR(0xAC,0xAC,0xAC); // COL072			= ACACAC			;mk2` ŠDF
	palette[ 8] = RGB_COLOR(0x14,0x14,0x14); // COL073			= 141414			;mk2` •
	palette[ 9] = RGB_COLOR(0xFF,0x00,0x00); // COL074			= FF0000			;mk2` Ô
	palette[10] = RGB_COLOR(0x00,0xFF,0x00); // COL075			= 00FF00			;mk2` —Î
	palette[11] = RGB_COLOR(0xFF,0xFF,0x00); // COL076			= FFFF00			;mk2` ‰©
	palette[12] = RGB_COLOR(0x00,0x00,0xFF); // COL077			= 0000FF			;mk2` Â
	palette[13] = RGB_COLOR(0xFF,0x00,0xFF); // COL078			= FF00FF			;mk2` ƒ}ƒ[ƒ“ƒ^
	palette[14] = RGB_COLOR(0x00,0xFF,0xFF); // COL079			= 00FFFF			;mk2` ƒVƒAƒ“
	palette[15] = RGB_COLOR(0xFF,0xFF,0xFF); // COL080			= FFFFFF			;mk2` ”’
}

void MEMORY::reset()
{
	int I, J;
	byte *addr=RAM;

	IBF5 = INT3 = WIE6 = 1;
	TimerSW=0;
	portF0 = 0x11;
	portF1 = 0xdd;
	CRTMode1 = CRTMode2 = CRTMode3 = 0;
	CSS3=CSS2=CSS1=0;
	// load rom image
	FILEIO* fio = new FILEIO();
#ifdef _PC6601
	if (fio->Fopen(emu->bios_path(_T("CGROM66.66")), FILEIO_READ_BINARY)) {
		fio->Fread(CGROM5, 0x2000, 1);
		fio->Fclose();
	}
	if (fio->Fopen(emu->bios_path(_T("BASICROM.66")), FILEIO_READ_BINARY)) {
		fio->Fread(BASICROM, 0x8000, 1);
		fio->Fclose();
	}
	if (fio->Fopen(emu->bios_path(_T("CGROM60.66")), FILEIO_READ_BINARY)) {
		fio->Fread(CGROM1, 0x2400, 1);
		fio->Fclose();
	}
	if (fio->Fopen(emu->bios_path(_T("KANJIROM.66")), FILEIO_READ_BINARY)) {
		fio->Fread(KANJIROM, 0x8000, 1);
		fio->Fclose();
	}
	if (fio->Fopen(emu->bios_path(_T("VOICEROM.66")), FILEIO_READ_BINARY)) {
		fio->Fread(VOICEROM, 0x4000, 1);
		fio->Fclose();
	}
#else
	if (fio->Fopen(emu->bios_path(_T("CGROM62.62")), FILEIO_READ_BINARY)) {
		fio->Fread(CGROM5, 0x2000, 1);
		fio->Fclose();
	}
	if (fio->Fopen(emu->bios_path(_T("BASICROM.62")), FILEIO_READ_BINARY)) {
		fio->Fread(BASICROM, 0x8000, 1);
		fio->Fclose();
	}
	if (fio->Fopen(emu->bios_path(_T("CGROM60.62")), FILEIO_READ_BINARY)) {
		fio->Fread(CGROM1, 0x2000, 1);
		fio->Fclose();
	}
	if (fio->Fopen(emu->bios_path(_T("KANJIROM.62")), FILEIO_READ_BINARY)) {
		fio->Fread(KANJIROM, 0x8000, 1);
		fio->Fclose();
	}
	if (fio->Fopen(emu->bios_path(_T("VOICEROM.62")), FILEIO_READ_BINARY)) {
		fio->Fread(VOICEROM, 0x4000, 1);
		fio->Fclose();
	}
#endif
	memset(RAM ,0,0x10000);	// clear RAM
	memset(EmptyRAM, 0, 0x2000);	// clear EmptyRAM
	CGROM = CGROM1;
	CGSW93 = 0;
	for(I=0; I<256; I++ ){
		for( J=0; J<64; J++ ){
			*addr++ = 0x00;
			*addr++ = 0xff;
		}
		for( J=0; J<64; J++ ){
			*addr++ = 0xff;
			*addr++ = 0x00;
		}
	}
	VRAM = RAM+0xE000;
	for (I=0; I<0x200; I++ ) *(VRAM+I)=0xde;
	if (!inserted) {
		EXTROM1 = EXTROM2 = EmptyRAM;
	}
	for(J=0;J<4;J++) {RdMem[J]=BASICROM+0x2000*J;WrMem[J]=RAM+0x2000*J;};
	for(J=4;J<8;J++) {RdMem[J]=RAM+0x2000*J;WrMem[J]=RAM+0x2000*J;};
	EnWrite[0]=EnWrite[1]=0; EnWrite[2]=EnWrite[3]=1;
	CurKANJIROM = NULL;
#else
void MEMORY::reset()
{
	int J;
	// load rom image
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(emu->bios_path(_T("BASICROM.60")), FILEIO_READ_BINARY)) {
		fio->Fread(BASICROM, 0x4000, 1);
		fio->Fclose();
	}
	if(fio->Fopen(emu->bios_path(_T("CGROM60.60")), FILEIO_READ_BINARY)) {
		fio->Fread(CGROM1, 0x1000, 1);
		fio->Fclose();
	}
	memset(RAM ,0,0x10000);	// clear RAM
	memset(EmptyRAM, 0, sizeof(EmptyRAM));
	CGROM = CGROM1;
	CGSW93 = 0;
	VRAM = RAM;
	if (!inserted) {
		EXTROM1 = EXTROM2 = EmptyRAM;
	}
	for(J=0;J<4;J++) {RdMem[J]=BASICROM+0x2000*J;WrMem[J]=RAM+0x2000*J;};
	RdMem[2] = EXTROM1; RdMem[3] = EXTROM2;
	for(J=4;J<8;J++) {RdMem[J]=RAM+0x2000*J;WrMem[J]=RAM+0x2000*J;};
	EnWrite[0]=EnWrite[1]=0; EnWrite[2]=EnWrite[3]=1;
#endif
	delete fio;
}

void MEMORY::write_data8(uint32 addr, uint32 data)
{
	/* normal memory write ..*/
	if(EnWrite[addr >> 14]) 
		WrMem[addr >> 13][addr & 0x1FFF] = data;
}

uint32 MEMORY::read_data8(uint32 addr)
{
	return(RdMem[addr >> 13][addr & 0x1FFF]);
}

void MEMORY::open_cart(_TCHAR* file_path)
{
	FILEIO* fio = new FILEIO();
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		fio->Fread(EXTROM, 0x4000, 1);
		fio->Fclose();
		EXTROM1 = EXTROM;
		EXTROM2 = EXTROM + 0x2000;
		inserted = true;
	} else {
		EXTROM1 = EXTROM2 = EmptyRAM;
	}
	delete fio;
}

void MEMORY::close_cart()
{
	EXTROM1 = EXTROM2 = EmptyRAM;
	inserted = false;
}

void MEMORY::write_io8(uint32 addr, uint32 data)
{
	unsigned int VRAMHead[2][4] = {
		{ 0xc000, 0xe000, 0x8000, 0xa000 },
		{ 0x8000, 0xc000, 0x0000, 0x4000 }
	};
	uint16 port=(addr & 0x00ff);
	byte Value=data;
	switch(port)
	{
#ifdef _PC6001
	case 0x92:
		CGSW93 = (Value&4)?0:1;
		break;
	case 0x93:
		if ((Value & 0x80) == 0) {
			switch(Value) {
			case 0x04: CGSW93=1; RdMem[3]=CGROM1; break;
			case 0x05: CGSW93=0;RdMem[3]=EXTROM2;break;
			}
		}
		break;
	/// CP/M ///
	case 0xf0:
		if (Value ==0xdd) {
			RdMem[0]=RAM;
			RdMem[1]=RAM+0x2000;
			RdMem[2]=RAM+0x4000;
			RdMem[3]=RAM+0x6000;
			EnWrite[0]=EnWrite[1]=1;
		} else {
			RdMem[0]=BASICROM;
			RdMem[1]=BASICROM+0x2000;
			RdMem[2]=EXTROM1;
			RdMem[3]=EXTROM2;
			EnWrite[0]=EnWrite[1]=0;
		}
		break;
	}
	return;
}
#else
	case 0x92:
		CGSW93 = (Value&4)?0:1;
		WIE6 = (Value&0x40)?1:0;
		break;
	case 0x93:
		switch(Value) {
			case 0x0c: WIE6 = 0; break;
			case 0x0d: WIE6 = 1; break;
			case 0x04: CGSW93=1; RdMem[3]=CGROM1; break;
			case 0x05: CGSW93=0;write_io8(0xf0, portF0);break;
		}
		break;
	case 0xB0:
		VRAM=(RAM+VRAMHead[CRTMode1][(data&0x06)>>1]);
		if (CRTMode1 && Value == 6) TimerSW=0; /// Colony Oddysey
		else TimerSW = (Value&0x01)?0:1;
		break;
	case 0xC0: // CSS
		CSS3=(Value&0x04)<<2;CSS2=(Value&0x02)<<2;CSS1=(Value&0x01)<<2;
///		refreshAll=1;		
		break;
	case 0xC1: // CRT controller mode
///		if((CRTMode1)&&(Value&0x02)) ClearScr();
		CRTMode1=(Value&0x02) ? 0 : 1;
		CRTMode2=(Value&0x04) ? 0 : 1;
		CRTMode3=(Value&0x08) ? 0 : 1;
		CGROM = ((CRTMode1 == 0) ? CGROM1 : CGROM5);
///		refreshAll=1;		
		break;
	case 0xC2: // ROM swtich
		if ((Value&0x02)==0x00) CurKANJIROM=KANJIROM;
		else CurKANJIROM=KANJIROM+0x4000;
		if ((Value&0x01)==0x00) {
///			if(RdMem[0]!=BASICROM) RdMem[0]=VOICEROM;
///			if(RdMem[1]!=BASICROM+0x2000) RdMem[1]=VOICEROM+0x2000;
			if(RdMem[2]!=BASICROM+0x4000) RdMem[2]=VOICEROM;
			if(RdMem[3]!=BASICROM+0x6000) RdMem[3]=VOICEROM+0x2000;
		}
		else {
			write_io8(0xF0,portF0); 	
		};
		break;
	case 0xC3: break; // C2H in/out switch
	case 0xF0: // read block set 
		portF0 = Value;
		switch(data & 0x0f)
		{
		case 0x00: RdMem[0]=RdMem[1]=EmptyRAM; break;
		case 0x01: RdMem[0]=BASICROM;RdMem[1]=BASICROM+0x2000; break;
		case 0x02: RdMem[0]=CurKANJIROM;RdMem[1]=CurKANJIROM+0x2000; break;
		case 0x03: RdMem[0]=RdMem[1]=EXTROM2; break;
		case 0x04: RdMem[0]=RdMem[1]=EXTROM1; break;
		case 0x05: RdMem[0]=CurKANJIROM;RdMem[1]=BASICROM+0x2000; break;
		case 0x06: RdMem[0]=BASICROM;RdMem[1]=CurKANJIROM+0x2000;break;
		case 0x07: RdMem[0]=EXTROM1;RdMem[1]=EXTROM2; break;
		case 0x08: RdMem[0]=EXTROM2;RdMem[1]=EXTROM1; break;
		case 0x09: RdMem[0]=EXTROM2;RdMem[1]=BASICROM+0x2000; break;
		case 0x0a: RdMem[0]=BASICROM;RdMem[1]=EXTROM2; break;
		case 0x0b: RdMem[0]=EXTROM1;RdMem[1]=CurKANJIROM+0x2000; break;
		case 0x0c: RdMem[0]=CurKANJIROM;RdMem[1]=EXTROM1; break;
		case 0x0d: RdMem[0]=RAM;RdMem[1]=RAM+0x2000; break;
		case 0x0e: RdMem[0]=RdMem[1]=EmptyRAM; break;
		case 0x0f: RdMem[0]=RdMem[1]=EmptyRAM; break;
		};
		switch(data & 0xf0)
		{
		case 0x00: RdMem[2]=RdMem[3]=EmptyRAM; break;
		case 0x10: RdMem[2]=BASICROM+0x4000;RdMem[3]=BASICROM+0x6000; break;
		case 0x20: RdMem[2]=VOICEROM;RdMem[3]=VOICEROM+0x2000; break;
		case 0x30: RdMem[2]=RdMem[3]=EXTROM2; break;
		case 0x40: RdMem[2]=RdMem[3]=EXTROM1; break;
		case 0x50: RdMem[2]=VOICEROM;RdMem[3]=BASICROM+0x6000; break;
		case 0x60: RdMem[2]=BASICROM+0x4000;RdMem[3]=VOICEROM+0x2000; break;
		case 0x70: RdMem[2]=EXTROM1;RdMem[3]=EXTROM2; break;
		case 0x80: RdMem[2]=EXTROM2;RdMem[3]=EXTROM1; break;
		case 0x90: RdMem[2]=EXTROM2;RdMem[3]=BASICROM+0x6000; break;
		case 0xa0: RdMem[2]=BASICROM+0x4000;RdMem[3]=EXTROM2; break;
		case 0xb0: RdMem[2]=EXTROM1;RdMem[3]=VOICEROM+0x2000; break;
		case 0xc0: RdMem[2]=VOICEROM;RdMem[3]=EXTROM1; break;
		case 0xd0: RdMem[2]=RAM+0x4000;RdMem[3]=RAM+0x6000; break;
		case 0xe0: RdMem[2]=RdMem[3]=EmptyRAM; break;
		case 0xf0: RdMem[2]=RdMem[3]=EmptyRAM; break;
		};
		if (CGSW93)	RdMem[3] = CGROM;
		break;
	case 0xF1: // read block set
		portF1 = Value;
		switch(data & 0x0f)
		{
		case 0x00: RdMem[4]=RdMem[5]=EmptyRAM; break;
		case 0x01: RdMem[4]=BASICROM;RdMem[5]=BASICROM+0x2000; break;
		case 0x02: RdMem[4]=CurKANJIROM;RdMem[5]=CurKANJIROM+0x2000; break;
		case 0x03: RdMem[4]=RdMem[5]=EXTROM2; break;
		case 0x04: RdMem[4]=RdMem[5]=EXTROM1; break;
		case 0x05: RdMem[4]=CurKANJIROM;RdMem[5]=BASICROM+0x2000; break;
		case 0x06: RdMem[4]=BASICROM;RdMem[5]=CurKANJIROM+0x2000; break;
		case 0x07: RdMem[4]=EXTROM1;RdMem[5]=EXTROM2; break;
		case 0x08: RdMem[4]=EXTROM2;RdMem[5]=EXTROM1; break;
		case 0x09: RdMem[4]=EXTROM2;RdMem[5]=BASICROM+0x2000; break;
		case 0x0a: RdMem[4]=BASICROM;RdMem[5]=EXTROM2; break;
		case 0x0b: RdMem[4]=EXTROM1;RdMem[5]=CurKANJIROM+0x2000; break;
		case 0x0c: RdMem[4]=CurKANJIROM;RdMem[5]=EXTROM1; break;
		case 0x0d: RdMem[4]=RAM+0x8000;RdMem[5]=RAM+0xa000; break;
		case 0x0e: RdMem[4]=RdMem[5]=EmptyRAM; break;
		case 0x0f: RdMem[4]=RdMem[5]=EmptyRAM; break;
		};
		switch(data & 0xf0)
		{
		case 0x00: RdMem[6]=RdMem[7]=EmptyRAM; break;
		case 0x10: RdMem[6]=BASICROM+0x4000;RdMem[7]=BASICROM+0x6000; break;
		case 0x20: RdMem[6]=CurKANJIROM;RdMem[7]=CurKANJIROM+0x2000; break;
		case 0x30: RdMem[6]=RdMem[7]=EXTROM2; break;
		case 0x40: RdMem[6]=RdMem[7]=EXTROM1; break;
		case 0x50: RdMem[6]=CurKANJIROM;RdMem[7]=BASICROM+0x6000; break;
		case 0x60: RdMem[6]=BASICROM+0x4000;RdMem[7]=CurKANJIROM+0x2000; break;
		case 0x70: RdMem[6]=EXTROM1;RdMem[7]=EXTROM2; break;
		case 0x80: RdMem[6]=EXTROM2;RdMem[7]=EXTROM1; break;
		case 0x90: RdMem[6]=EXTROM2;RdMem[7]=BASICROM+0x6000; break;
		case 0xa0: RdMem[6]=BASICROM+0x4000;RdMem[7]=EXTROM2; break;
		case 0xb0: RdMem[6]=EXTROM1;RdMem[7]=CurKANJIROM+0x2000; break;
		case 0xc0: RdMem[6]=CurKANJIROM;RdMem[7]=EXTROM1; break;
		case 0xd0: RdMem[6]=RAM+0xc000;RdMem[7]=RAM+0xe000; break;
		case 0xe0: RdMem[6]=RdMem[7]=EmptyRAM; break;
		case 0xf0: RdMem[6]=RdMem[7]=EmptyRAM; break;
		};
		break;
	case 0xF2: // write ram block set
		if (data & 0x40) {EnWrite[3]=1;WrMem[6]=RAM+0xc000;WrMem[7]=RAM+0xe000;}
		else EnWrite[3]=0;
		if (data & 0x010) {EnWrite[2]=1;WrMem[4]=RAM+0x8000;WrMem[5]=RAM+0xa000;}
		else EnWrite[2]=0;
		if (data & 0x04) {EnWrite[1]=1;WrMem[2]=RAM+0x4000;WrMem[3]=RAM+0x6000;}
		else EnWrite[1]=0;
		if (data & 0x01) {EnWrite[0]=1;WrMem[0]=RAM;WrMem[1]=RAM+0x2000;}
		else EnWrite[0]=0;
		break;
	}
	return;
}

uint32 MEMORY::read_io8(uint32 addr)
{
	// disk I/O
	uint16 port=(addr & 0x00ff);
	byte Value=0xff;

	switch(port)
	{
	case 0xF0: Value=portF0;break;
	case 0xF1: Value=portF1;break;
	}
	return(Value);
}

#endif
