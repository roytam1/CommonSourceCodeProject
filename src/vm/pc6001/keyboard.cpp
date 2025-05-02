/*
	NEC PC-6001 Emulator 'yaPC-6001'

	Author : tanam
	Date   : 2013.07.15-

	[ keyboard ]
*/

#include "memory.h"
#include "keyboard.h"
#include "../event.h"
#include "../../fileio.h"
#include "../i8255.h"

#define EVENT_SIGNAL			0
#define DATAREC_FF_REW_SPEED	10

#define CAS_NONE				0
#define CAS_SAVEBYTE			1
#define CAS_LOADING				2
#define CAS_LOADBYTE			3

/* normal (small alphabet) */
byte Keys1[256][2] =
{
/* 0       1         2        3        4        5        6        7 */
/* 00 */
  /* */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /* BS      TAB                                ENTER                  */
  {0,0x7f},{0,0x09},{0,0x00},{0,0x00},{0,0x00},{0,0x0d},{0,0x00},{0,0x00},

/* 10 */
  /* SHIFT   CTRL      ALT     PAUSE    CAPS     KANA                  */
  {0,0x00},{0,0x00},{0,0x00},{1,0xfa},{1,0xfb},{0,0x00},{0,0x00},{0,0x00},
  /*                           ESC                                     */
  {0,0x00},{0,0x00},{0,0x00},{0,0x1b},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 20 */
  /* SPC    PAGE UP PAGE DOWN    END    HOME      LEFT    UP      RIGHT */
  {0,0x20},{0,0xfe},{0,0xfe},{0,0xfe},{0,0x0c},{0,0x1d},{0,0x1e},{0,0x1c},
  /* DOWN                              PRN SCR   INSERT   DELETE  HELP  */
  {0,0x1f},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x12},{0,0x08},{0,0x00},

/* 30 */
  /* 0        1         2        3        4       5          6       7  */
  {0,0x30},{0,0x31},{0,0x32},{0,0x33},{0,0x34},{0,0x35},{0,0x36},{0,0x37},
  /* 8        9                                                         */
  {0,0x38},{0,0x39},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 40 */
  /*          A         B        C        D       E         F       G   */
  {0,0x00},{0,0x61},{0,0x62},{0,0x63},{0,0x64},{0,0x65},{0,0x66},{0,0x67},
  /*   H      I         J        K        L       M         N       O   */
  {0,0x68},{0,0x69},{0,0x6a},{0,0x6b},{0,0x6c},{0,0x6d},{0,0x6e},{0,0x6f},

/* 50 */
  /*  P       Q         R        S        T       U         V       W   */
  {0,0x70},{0,0x71},{0,0x72},{0,0x73},{0,0x74},{0,0x75},{0,0x76},{0,0x77},
  /*  X       Y         Z        */
  {0,0x78},{0,0x79},{0,0x7a},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 60   10key*/
  /*  0       1         2        3        4       5         6       7   */
  {0,0x30},{0,0x31},{0,0x32},{0,0x33},{0,0x34},{0,0x35},{0,0x36},{0,0x37},
  /*  8       9         *        +                -         .       /   */
  {0,0x38},{0,0x39},{0,0x2a},{0,0x2b},{0,0x00},{0,0x2d},{0,0x2e},{0,0x2f},

/* 70 */
  /* F1       F2       F3        F4      F5       F6       F7       F8  */
  {1,0xf0},{1,0xf1},{1,0xf2},{1,0xf3},{1,0xf4},{1,0x00},{0,0x00},{1,0xfd},
  /* F9       F10      F11       F12     F13      F14      F15      F16 */
  {0,0x03},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 80 */
  /* F17      F18      F19       F20     F21      F22      F23      F24 */
  {0,0x20},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 90 */
  /* NUM     SCROLL                                                     */
  {0,0x00},{1,0xfe},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* A0 */
  /*                       SCROLL LOCK                                  */
  {0,0x00},{0,0x00},{0,0x00},{1,0xfe},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*                    *       +        ,         -        .        /  */
  {0,0x00},{0,0x00},{0,0x2a},{0,0x2b},{0,0x2c},{0,0x2d},{0,0x2e},{0,0x2f},

/* B0 */
  /*                                                                    */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*                    :*       ;+       ,<       -        .>      /?  */
  {0,0x00},{0,0x00},{0,0x3A},{0,0x3b},{0,0x2c},{0,0x2d},{0,0x2e},{0,0x2f},

/* C0 */
  /*  @                                                                 */
  {0,0x40},{1,0x00},{1,0x00},{0,0x00},{0,0x00},{0,0x00},{1,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* D0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*                            [{        \|        ]}      ^           */
  {0,0x00},{0,0x00},{0,0x00},{0,0x5b},{0,0x5c},{0,0x5d},{0,0x5e},{0,0x00},

/* E0 */
  /*                     _                                              */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{1,0xfe},{1,0xfb},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
/* F0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0xba},{0,0xbb},
  {0,0xbc},{0,0xbd},{0,0xbe},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
};

/* normal (small alphabet) + shift */
byte Keys2[256][2] =
{
/* 00 */
  {0,0x35},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /* BS      TAB                                ENTER                  */
  {0,0x7f},{0,0x09},{0,0x00},{0,0x00},{0,0x00},{0,0x0d},{0,0x00},{0,0x00},

/* 10 */
  /* SHIFT   CTRL      ALT     PAUSE    CAPS     KANA                  */
  {0,0x00},{0,0x00},{0,0x00},{1,0xfa},{1,0xfb},{0,0x00},{0,0x00},{0,0x00},
  /*                           ESC                                     */
  {0,0x00},{0,0x00},{0,0x00},{0,0x1b},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 20 */
  /* SPC    PAGE UP PAGE DOWN    END    HOME      LEFT    UP      RIGHT */
  {0,0x20},{1,0xfc},{1,0xfc},{0,0xfc},{0,0x0b},{0,0x1d},{0,0x1e},{0,0x1c},
  /* DOWN                              PRN SCR   INSERT   DELETE  HELP  */
  {0,0x1f},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x12},{0,0x08},{0,0x00},

/* 30 */
  /* 0        1         2        3        4       5          6       7  */
  {0,0x00},{0,0x21},{0,0x22},{0,0x23},{0,0x24},{0,0x25},{0,0x26},{0,0x27},
  /* 8        9                                                         */
  {0,0x28},{0,0x29},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 40 */
  /*          A         B        C        D       E         F       G   */
  {0,0x40},{0,0x41},{0,0x42},{0,0x43},{0,0x44},{0,0x45},{0,0x46},{0,0x47},
  /*   H      I         J        K        L       M         N       O   */
  {0,0x48},{0,0x49},{0,0x4a},{0,0x4b},{0,0x4c},{0,0x4d},{0,0x4e},{0,0x4f},

/* 50 */
  /*  P       Q         R        S        T       U         V       W   */
  {0,0x50},{0,0x51},{0,0x52},{0,0x53},{0,0x54},{0,0x55},{0,0x56},{0,0x57},
  /*  X       Y         Z        */
  {0,0x58},{0,0x59},{0,0x5a},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 60  10 key */
  /*  0       1         2        3        4       5         6       7   */
  {0,0x30},{0,0x31},{0,0x32},{0,0x33},{0,0x34},{0,0x35},{0,0x36},{0,0x37},
  /*  8       9         *        +                -         .       /   */
  {0,0x38},{0,0x39},{0,0x2a},{0,0x2b},{0,0x00},{0,0x2d},{0,0x2e},{0,0x2f},

/* 70 */
  /* F1       F2       F3        F4      F5       F6       F7       F8  */
  {1,0xf5},{1,0xf6},{1,0xf7},{1,0xf8},{1,0xf9},{1,0x00},{0,0x00},{1,0xfd},
  {0,0x03},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 80 */
  /* F17      F18      F19       F20     F21      F22      F23      F24 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x09},{0,0x00},{0,0x00},{0,0x00},{0,0x0d},{0,0x00},{0,0x00},

/* 90 */
  /* NUM     SCROLL                                                     */
  {0,0x00},{1,0xfe},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* A0 */
  /*                       SCROLL LOCK                                  */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x2a},{0,0x2b},{0,0x2c},{0,0x2d},{0,0x2e},{0,0x2f},

/* B0 */
  {0,0x30},{0,0x31},{0,0x32},{0,0x33},{0,0x34},{0,0x35},{0,0x36},{0,0x37},
  /*                    :*       ;+       ,<       -        .>      /?  */
  {0,0x38},{0,0x39},{0,0x2A},{0,0x2b},{0,0x3c},{0,0x3d},{0,0x3e},{0,0x3f},

/* C0 */
  {0,0x00},{1,0x00},{1,0x00},{0,0x00},{0,0x00},{0,0x00},{1,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* D0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*                            [{        \|        ]}      ^           */
  {0,0x00},{0,0x00},{0,0x00},{0,0x7B},{0,0x7C},{0,0x7D},{0,0x00},{0,0x00},

/* E0 */
  /*                     _                                              */
  {0,0x00},{0,0x00},{0,0x5f},{0,0x00},{1,0xfe},{1,0xfb},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
/* F0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0xba},{0,0xbb},
  {0,0xbc},{0,0xbd},{0,0xbe},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
};

/* hiragana */
byte Keys3[256][2] =
{
/* 00 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /* BS      TAB                                ENTER                  */
  {0,0x7f},{0,0x09},{0,0x00},{0,0x00},{0,0x00},{0,0x0d},{0,0x00},{0,0x00},

/* 10 */
  /* SHIFT   CTRL      ALT     PAUSE    CAPS     KANA                  */
  {0,0x00},{0,0x00},{0,0x00},{1,0xfa},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*                           ESC                                     */
  {0,0x00},{0,0x00},{0,0x00},{0,0x1b},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 20 */
  /* SPC    PAGE UP PAGE DOWN    END    HOME      LEFT    UP      RIGHT */
  {0,0x20},{0,0xfe},{0,0xfe},{0,0x00},{0,0x0c},{0,0x1d},{0,0x1e},{0,0x1c},
  /* DOWN                              PRN SCR   INSERT   DELETE  HELP  */
  {0,0x1f},{0,0x00},{0,0x00},{0,0x00},{0,0xe8},{0,0x12},{0,0x08},{0,0xf2},

/* 30 */
  /* 0        1         2        3        4       5          6       7  */
  {0,0xfc},{0,0xe7},{0,0xec},{0,0x91},{0,0x93},{0,0x94},{0,0x95},{0,0xf4},
  /* 8        9                                                         */
  {0,0xf5},{0,0xf6},{0,0x99},{0,0xfa},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 40 */
  /*          A         B        C        D       E         F       G   */
  {0,0x00},{0,0xe1},{0,0x9a},{0,0x9f},{0,0x9c},{0,0x92},{0,0xea},{0,0x97},
  /*   H      I         J        K        L       M         N       O   */
  {0,0x98},{0,0xe6},{0,0xef},{0,0xe9},{0,0xf8},{0,0xf3},{0,0xf0},{0,0xf7},

/* 50 */
  /*  P       Q         R        S        T       U         V       W   */
  {0,0x9e},{0,0xe0},{0,0x9d},{0,0xe4},{0,0x96},{0,0xe5},{0,0xeb},{0,0xe3},
  /*  X       Y         Z        */
  {0,0x9b},{0,0xfd},{0,0xe2},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 60  10 key */
  /*  0       1         2        3        4       5         6       7   */
  {0,0x30},{0,0x31},{0,0x32},{0,0x33},{0,0x34},{0,0x35},{0,0x36},{0,0x37},
  /*  8       9         *        +                -         .       /   */
  {0,0x38},{0,0x39},{0,0x2a},{0,0x2b},{0,0x00},{0,0x2d},{0,0x2e},{0,0x2f},

/* 70 */
  /* F1       F2       F3        F4      F5       F6       F7       F8  */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x03},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 80 */
  {0,0x20},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x09},{0,0x00},{0,0x00},{0,0x00},{0,0x0d},{0,0x00},{0,0x00},


/* 90 */
  /* NUM     SCROLL                                                     */
  {0,0x00},{1,0xfe},{0,0x00},{0,0x00},{0,0x00},{0,0x37},{0,0x34},{0,0x38},
  {0,0x36},{0,0x32},{0,0x39},{0,0x33},{0,0x31},{0,0x35},{0,0x30},{0,0x2e},

/* A0 */
  /*                       SCROLL LOCK                                  */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x99},{0,0xfa},{0,0xe8},{0,0xee},{0,0xf9},{0,0xf2},

/* B0 */
  {0,0x30},{0,0x31},{0,0x32},{0,0x33},{0,0x34},{0,0x35},{0,0x36},{0,0x37},
  /*                    :*       ;+       ,<       -        .>      /?  */
  {0,0x38},{0,0x39},{0,0x99},{0,0xfa},{0,0xe8},{0,0xee},{0,0xf9},{0,0xf2},

/* C0 */
  {0,0xde},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{1,0xfd},{0,0x00},
  {0,0x00},{0,0x00},{0,0x12},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* D0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*                            [{        \|        ]}      ^           */
  {0,0x00},{0,0x00},{0,0x00},{0,0xdf},{0,0xb0},{0,0xf1},{0,0xed},{0,0x00},

/* E0 */
  /*                    _              scroll?    caps? */
  {0,0x00},{0,0x00},{0,0xfb},{0,0x00},{1,0xfe},{1,0xfb},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
/* F0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x7f},
};

/* hiragana + shift */
byte Keys4[256][2] =
{
/* 00 */
  {0,0x35},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /* BS      TAB                                ENTER                  */
  {0,0x7f},{0,0x09},{0,0x00},{0,0x00},{0,0x00},{0,0x0d},{0,0x00},{0,0x00},

/* 10 */
  /* SHIFT   CTRL      ALT     PAUSE    CAPS     KANA                  */
  {0,0x00},{0,0x00},{0,0x00},{1,0xfa},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*                           ESC                                     */
  {0,0x00},{0,0x00},{0,0x00},{0,0x1b},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 20 */
  /* SPC    PAGE UP PAGE DOWN    END    HOME      LEFT    UP      RIGHT */
  {0,0x20},{1,0xfc},{1,0xfc},{0,0x00},{0,0x0b},{0,0x1d},{0,0x1e},{0,0x1c},
  /* DOWN                              PRN SCR   INSERT   DELETE  HELP  */
  {0,0x1f},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0xa1},{0,0xa5},

/* 30 */
  /* 0        1         2        3        4       5          6       7  */
  {0,0x86},{0,0x00},{0,0x00},{0,0x87},{0,0x89},{0,0x8a},{0,0x8b},{0,0x8c},
  /* 8        9                                                         */
  {0,0x8d},{0,0x8e},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 40 */
  /*          A         B        C        D       E         F       G   */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x88},{0,0x00},{0,0x00},
  /*   H      I         J        K        L       M         N       O   */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 50 */
  /*  P       Q         R        S        T       U         V       W   */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*  X       Y         Z        */
  {0,0x00},{0,0x00},{0,0x8f},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 60  10 key*/
  /*  0       1         2        3        4       5         6       7   */
  {0,0x30},{0,0x31},{0,0x32},{0,0x33},{0,0x34},{0,0x35},{0,0x36},{0,0x37},
  /*  8       9         *        +                -         .       /   */
  {0,0x38},{0,0x39},{0,0x2a},{0,0x2b},{0,0x00},{0,0x2d},{0,0x2e},{0,0x2f},

/* 70 */
  /* F1       F2       F3        F4      F5       F6       F7       F8  */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x03},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 80 */
  {0,0x20},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x09},{0,0x00},{0,0x00},{0,0x00},{0,0x0d},{0,0x00},{0,0x00},

/* 90 */
  /* NUM     SCROLL                                                     */
  {0,0x00},{1,0xfe},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* A0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x2a},{0,0x2b},{0,0x2c},{0,0x2d},{0,0x2e},{0,0x2f},

/* B0 */
  {0,0x30},{0,0x31},{0,0x32},{0,0x33},{0,0x34},{0,0x35},{0,0x36},{0,0x37},
  /*                    :*       ;+       ,<       -        .>      /?  */
  {0,0x38},{0,0x39},{0,0x00},{0,0x00},{0,0xa4},{0,0x00},{0,0xa1},{0,0x00},

/* C0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{1,0xfd},{0,0x00},
  {0,0x00},{0,0x00},{0,0x12},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
/* D0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*                            [{        \|        ]}      ^           */
  {0,0x00},{0,0x00},{0,0x00},{0,0xa2},{0,0x00},{0,0xa3},{0,0x00},{0,0x00},
/* E0 */
  /*                     _                                              */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{1,0xfe},{1,0xfb},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
/* F0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x7f},
};

/* katakana */
byte Keys5[256][2] =
{
/* 00 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x7f},{0,0x09},{0,0x00},{0,0x00},{0,0x00},{0,0x0d},{0,0x00},{0,0x00},

/* 10 */
  /* SHIFT   CTRL      ALT     PAUSE    CAPS     KANA                  */
  {0,0x00},{0,0x00},{0,0x00},{1,0xfa},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*                           ESC                                     */
  {0,0x00},{0,0x00},{0,0x00},{0,0x1b},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 20 */
  /* SPC    PAGE UP PAGE DOWN    END    HOME      LEFT    UP      RIGHT */
  {0,0x20},{0,0xfe},{0,0xfe},{0,0x00},{0,0x0c},{0,0x1d},{0,0x1e},{0,0x1c},
  /* DOWN                              PRN SCR   INSERT   DELETE  HELP  */
  {0,0x1f},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x12},{0,0x08},{0,0x00},

/* 30 */
  /* 0        1         2        3        4       5          6       7  */
  {0,0xdc},{0,0xc7},{0,0xcc},{0,0xb1},{0,0xb3},{0,0xb4},{0,0xb5},{0,0xd4},
  /* 8        9                                                         */
  {0,0xd5},{0,0xd6},{0,0xb9},{0,0xda},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 40 */
  /*          A         B        C        D       E         F       G   */
  {0,0x00},{0,0xc1},{0,0xba},{0,0xbf},{0,0xbc},{0,0xb2},{0,0xca},{0,0xb7},
  /*   H      I         J        K        L       M         N       O   */
  {0,0xb8},{0,0xc6},{0,0xcf},{0,0xc9},{0,0xd8},{0,0xd3},{0,0xd0},{0,0xd7},

/* 50 */
  /*  P       Q         R        S        T       U         V       W   */
  {0,0xbe},{0,0xc0},{0,0xbd},{0,0xc4},{0,0xb6},{0,0xc5},{0,0xcb},{0,0xc3},
  /*  X       Y         Z        */
  {0,0xbb},{0,0xdd},{0,0xc2},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 60  10 key*/
  /*  0       1         2        3        4       5         6       7   */
  {0,0x30},{0,0x31},{0,0x32},{0,0x33},{0,0x34},{0,0x35},{0,0x36},{0,0x37},
  /*  8       9         *        +                -         .       /   */
  {0,0x38},{0,0x39},{0,0x2a},{0,0x2b},{0,0x00},{0,0x2d},{0,0x2e},{0,0x2f},

/* 70 */
  /* F1       F2       F3        F4      F5       F6       F7       F8  */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x03},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 80 */
  {0,0x20},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x09},{0,0x00},{0,0x00},{0,0x00},{0,0x0d},{0,0x00},{0,0x00},

/* 90 */
  /* NUM     SCROLL                                                     */
  {0,0x00},{1,0xfe},{0,0x00},{0,0x00},{0,0x00},{0,0x37},{0,0x34},{0,0x38},
  {0,0x36},{0,0x32},{0,0x39},{0,0x33},{0,0x31},{0,0x35},{0,0x30},{0,0x2e},

/* A0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0xb9},{0,0xda},{0,0xc8},{0,0xce},{0,0xd9},{0,0xd2},

/* B0 */
  {0,0x30},{0,0x31},{0,0x32},{0,0x33},{0,0x34},{0,0x35},{0,0x36},{0,0x37},
  /*                    :*       ;+       ,<       -        .>      /?  */
  {0,0x38},{0,0x39},{0,0xb9},{0,0xda},{0,0xc8},{0,0xce},{0,0xd9},{0,0xd2},

/* C0 */
  {0,0xde},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x12},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* D0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*                            [{        \|        ]}      ^           */
  {0,0x00},{0,0x00},{0,0x00},{0,0xdf},{0,0xb0},{0,0xd1},{0,0xcd},{0,0x00},

/* E0 */
  /*                     _                                              */
  {0,0x00},{0,0x00},{0,0xdb},{0,0x00},{1,0xfe},{1,0xfb},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
/* F0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x7f},
};

/* katakana + shift */
byte Keys6[256][2] =
{
/* 00 */
  {0,0x35},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x7f},{0,0x09},{0,0x00},{0,0x00},{0,0x00},{0,0x0d},{0,0x00},{0,0x00},
/* 10 */
  {0,0x00},{0,0x00},{0,0x00},{1,0xfa},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x1b},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
/* 20 */
  /* SPC    PAGE UP PAGE DOWN    END    HOME      LEFT    UP      RIGHT */
  {0,0x20},{1,0xfc},{1,0xfc},{0,0x00},{0,0x0b},{0,0x1d},{0,0x1e},{0,0x1c},
  /* DOWN                              PRN SCR   INSERT   DELETE  HELP  */
  {0,0x1f},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0xa1},{0,0xa5},

/* 30 */
  /* 0        1         2        3        4       5          6       7  */
  {0,0xa6},{0,0x00},{0,0x00},{0,0xa7},{0,0xa9},{0,0xaa},{0,0xab},{0,0xac},
  /* 8        9                                                         */
  {0,0xad},{0,0xae},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 40 */
  /*          A         B        C        D       E         F       G   */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0xa8},{0,0x00},{0,0x00},
  /*   H      I         J        K        L       M         N       O   */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 50 */
  /*  P       Q         R        S        T       U         V       W   */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*  X       Y         Z        */
  {0,0x00},{0,0x00},{0,0xaf},{0,0xa2},{0,0xb0},{0,0xa3},{0,0x00},{0,0x00},

/* 60 */
  /*  0       1         2        3        4       5         6       7   */
  {0,0x30},{0,0x31},{0,0x32},{0,0x33},{0,0x34},{0,0x35},{0,0x36},{0,0x37},
  /*  8       9         *        +                -         .       /   */
  {0,0x38},{0,0x39},{0,0x2a},{0,0x2b},{0,0x00},{0,0x2d},{0,0x2e},{0,0x2f},

/* 70 */
  /* F1       F2       F3        F4      F5       F6       F7       F8  */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x03},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 80 */
  {0,0x20},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x09},{0,0x00},{0,0x00},{0,0x00},{0,0x0d},{0,0x00},{0,0x00},

/* 90 */
  {0,0x00},{1,0xfe},{0,0x00},{0,0x00},{0,0x00},{0,0x37},{0,0x34},{0,0x38},
  {0,0x36},{0,0x32},{0,0x39},{0,0x33},{0,0x31},{0,0x35},{0,0x30},{0,0x2e},

/* A0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x2a},{0,0x2b},{0,0x2c},{0,0x2d},{0,0x2e},{0,0x2f},

/* B0 */
  {0,0x30},{0,0x31},{0,0x32},{0,0x33},{0,0x34},{0,0x35},{0,0x36},{0,0x37},
  /*                    :*       ;+       ,<       -        .>      /?  */
  {0,0x38},{0,0x39},{0,0x00},{0,0x00},{0,0xa4},{0,0x00},{0,0xa1},{1,0x00},

/* C0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{1,0xfd},{0,0x00},
  {0,0x00},{0,0x00},{0,0x12},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* D0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*                            [{        \|        ]}      ^           */
  {0,0x00},{0,0x00},{0,0x00},{0,0xa2},{0,0x00},{0,0xa3},{0,0x00},{0,0x00},

/* E0 */
  /*                     _                                              */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{1,0xfe},{1,0xfb},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* F0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x7f},
};

/* with graph key */
byte Keys7[256][2] =
{
/* 00 */
  {0,0x35},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x7f},{0,0x09},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 10 */
  {0,0x00},{0,0x00},{0,0x00},{1,0xfa},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 20 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{1,0x1f},{1,0x17},{1,0x1d},{1,0x80},

/* 30 */
  {1,0x0f},{1,0x07},{1,0x01},{1,0x02},{1,0x03},{1,0x04},{1,0x05},{1,0x06},
  {1,0x0d},{1,0x0e},{1,0x81},{1,0x82},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 40 */
  /*          A         B        C        D       E         F       G   */
  {1,0x83},{0,0x00},{1,0x1b},{1,0x1a},{1,0x14},{1,0x18},{1,0x15},{1,0x13},
  /*   H      I         J        K        L       M         N       O   */
  {1,0x0a},{1,0x16},{0,0x00},{0,0x00},{1,0x1e},{1,0x0b},{0,0x00},{0,0x00},

/* 50 */
  {1,0x10},{0,0x00},{1,0x12},{1,0x0c},{1,0x19},{0,0x00},{1,0x11},{0,0x00},
  {1,0x1c},{1,0x08},{0,0x00},{1,0x84},{1,0x09},{1,0x85},{0,0x00},{0,0x00},

/* 60 */
  {0,0x00},{0,0x00},{1,0x1b},{1,0x1a},{1,0x14},{1,0x18},{1,0x15},{1,0x13},
  {1,0x0a},{1,0x16},{0,0x00},{0,0x00},{1,0x1e},{1,0x0b},{0,0x00},{0,0x00},

/* 70 */
  /* F1       F2       F3        F4      F5       F6       F7       F8  */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x03},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* 80 */
  {0,0x20},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x09},{0,0x00},{0,0x00},{0,0x00},{0,0x0d},{0,0x00},{0,0x00},

/* 90 */
  /* NUM     SCROLL                                                     */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x37},{0,0x34},{0,0x38},
  {0,0x36},{0,0x32},{0,0x39},{0,0x33},{0,0x31},{0,0x35},{0,0x30},{0,0x2e},

/* A0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x2a},{0,0x2b},{0,0x2c},{1,0x17},{0,0x2e},{0,0x2f},

/* B0 */
  {0,0x30},{0,0x31},{0,0x32},{0,0x33},{0,0x34},{0,0x35},{0,0x36},{0,0x37},
  /*                    :*       ;+       ,<       -        .>      /?  */
  {0,0x38},{0,0x39},{0,0x81},{0,0x82},{1,0x1f},{1,0x17},{1,0x1d},{0,0x80},

/* C0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x12},{0,0x00},{0,0x00},{1,0x17},{0,0x00},{0,0x00},

/* D0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  /*                            [{        \|        ]}      ^           */
  {0,0x00},{0,0x00},{0,0x00},{0,0x84},{1,0x09},{0,0x85},{0,0x00},{0,0x00},

/* E0 */
  /*                     _                                              */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{1,0xfe},{1,0xfb},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},

/* F0 */
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},
  {0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x00},{0,0x7f},
};

void KEYBOARD::update_keyboard()
{
	for (int code=0; code < 256; code++) {
		if (key_stat[code] & 0x80) {
			if (code == VK_SHIFT || code == VK_CONTROL) continue;
			key_stat[code]=0;
			if (code == 0x75) {kanaMode = -1 * (kanaMode-1);continue;} // VK_F6
			if (code == 0x76) {katakana = -1 * (katakana-1);continue;} // VK_F7
			if (code == 0x77) {kbFlagGraph = -1 * (kbFlagGraph-1);continue;} // VK_F8
			p6key=code;
			byte *Keys;
			byte ascii=0;
			if (kbFlagGraph) {
				Keys = Keys7[code];
			} else if (kanaMode) {
				if (katakana) {
					if (stick0 & STICK0_SHIFT) Keys = Keys6[code];
					else Keys = Keys5[code];
				} else if (stick0 & STICK0_SHIFT) { 
					Keys = Keys4[code];
				} else {
					Keys = Keys3[code];
				}
			} else if (stick0 & STICK0_SHIFT) { 
				Keys = Keys2[code];
			} else { 
				Keys = Keys1[code];
			}
			ascii = Keys[1];
			/* control key + alphabet key */
			if ((kbFlagCtrl == 1) && (code >= 0x41) && (code <= 0x5a)) ascii = code - 0x41 + 1;
			/* function key */
			if (!kanaMode && (ascii>0xef && ascii<0xfa)) kbFlagFunc=1;
			d_pio->write_signal(SIG_I8255_PORT_A, ascii, 0xff);
		}
	}
}

void KEYBOARD::write_signal(int id, uint32 data, uint32 mask)
{
	bool signal = ((data & mask) != 0);
	
	if(id == SIG_DATAREC_OUT) {
		if(out_signal != signal) {
			if(rec && remote) {
				changed++;
			}
			if(prev_clock != 0) {
				if(out_signal) {
					positive_clocks += passed_clock(prev_clock);
				} else {
					negative_clocks += passed_clock(prev_clock);
				}
				prev_clock = current_clock();
			}
			out_signal = signal;
		}
	} else if(id == SIG_DATAREC_REMOTE) {
		set_remote(signal);
	} else if(id == SIG_DATAREC_TRIG) {
		// L->H: remote signal is switched
		if(signal && !trigger) {
			set_remote(!remote);
		}
		trigger = signal;
	}
}

void KEYBOARD::event_callback(int event_id, int err)
{
	if(event_id == EVENT_SIGNAL) {
		if(play) {
			if(buffer_ptr < buffer_length && ff_rew == 0) {
				emu->out_message(_T("CMT: Play (%d %%)"), 100 * buffer_ptr / buffer_length);
			}
			bool signal = in_signal;
			if(is_wav) {
				if(buffer_ptr >= 0 && buffer_ptr < buffer_length) {
					signal = ((buffer[buffer_ptr] & 0x80) != 0);
					CasData[buffer_ptr]=buffer[buffer_ptr];
				}
				if(ff_rew < 0) {
					if((buffer_ptr = max(buffer_ptr - 1, 0)) == 0) {
						set_remote(false);	// top of tape
					}
				} else {
					if((buffer_ptr = min(buffer_ptr + 1, buffer_length)) == buffer_length) {
						set_remote(false);	// end of tape
					}
				}
				update_event();
			} else {
				if(ff_rew < 0) {
					if(buffer_bak != NULL) {
						memcpy(buffer, buffer_bak, buffer_length);
					}
					buffer_ptr = 0;
					set_remote(false);	// top of tape
				} else {
					while(buffer_ptr < buffer_length) {
						if((buffer[buffer_ptr] & 0x7f) == 0) {
							if(++buffer_ptr == buffer_length) {
								set_remote(false);	// end of tape
								break;
							}
						} else {
							signal = ((buffer[buffer_ptr] & 0x80) != 0);
							uint8 tmp = buffer[buffer_ptr];
							buffer[buffer_ptr] = (tmp & 0x80) | ((tmp & 0x7f) - 1);
							break;
						}
					}
				}
			}
			// notify the signal is changed
			if(signal != in_signal) {
				in_signal = signal;
				changed++;
				write_signals(&outputs_out, in_signal ? 0xffffffff : 0);
			}
		} else if(rec) {
			if(out_signal) {
				positive_clocks += passed_clock(prev_clock);
			} else {
				negative_clocks += passed_clock(prev_clock);
			}
			if(is_wav) {
				if(positive_clocks != 0 || negative_clocks != 0) {
					buffer[buffer_ptr] = (255 * positive_clocks) / (positive_clocks + negative_clocks);
				} else {
					buffer[buffer_ptr] = 0;
				}
				if(++buffer_ptr >= buffer_length) {
					buffer_ptr = 0;
				}
			} else {
				bool prev_signal = ((buffer[buffer_ptr] & 0x80) != 0);
				bool cur_signal = (positive_clocks > negative_clocks);
				if(prev_signal != cur_signal || (buffer[buffer_ptr] & 0x7f) == 0x7f) {
					if(++buffer_ptr >= buffer_length) {
						buffer_ptr = 0;
					}
					buffer[buffer_ptr] = cur_signal ? 0x80 : 0;
				}
				buffer[buffer_ptr]++;
			}
			prev_clock = current_clock();
			positive_clocks = negative_clocks = 0;
		}
	}
}

void KEYBOARD::set_remote(bool value)
{
	if(remote != value) {
		remote = value;
		update_event();
	}
}

void KEYBOARD::set_ff_rew(int value)
{
	if(ff_rew != value) {
		if(register_id != -1) {
			cancel_event(register_id);
			register_id = -1;
		}
		ff_rew = value;
		apss_signals = false;
		update_event();
	}
}

bool KEYBOARD::do_apss(int value)
{
	bool result = false;
	
	if(play) {
		set_ff_rew(0);
		set_remote(true);
		set_ff_rew(value > 0 ? 1 : -1);
		apss_remain = value;
		
		while(apss_remain != 0 && remote) {
			event_callback(EVENT_SIGNAL, 0);
		}
		result = (apss_remain == 0);
	}
	
	// stop cmt
	set_remote(false);
	set_ff_rew(0);
	
	if(value > 0) {
		emu->out_message(_T("CMT: APSS (+%d)"), value);
	} else {
		emu->out_message(_T("CMT: APSS (%d)"), value);
	}
	return result;
}

void KEYBOARD::update_event()
{
	if(remote && (play || rec)) {
		if(register_id == -1) {
			if(ff_rew != 0) {
				register_event(this, EVENT_SIGNAL, 1000000. / sample_rate / DATAREC_FF_REW_SPEED, true, &register_id);
				if(ff_rew > 0) {
					emu->out_message(_T("CMT: Fast Forward"));
				} else {
					emu->out_message(_T("CMT: Fast Rewind"));
				}
			} else {
				register_event(this, EVENT_SIGNAL, 1000000. / sample_rate, true, &register_id);
				if(play) {
					if(buffer_ptr < buffer_length) {
						emu->out_message(_T("CMT: Play (%d %%)"), 100 * buffer_ptr / buffer_length);
					} else {
						emu->out_message(_T("CMT: Play"));
					}
				} else {
					emu->out_message(_T("CMT: Record"));
				}
			}
			prev_clock = current_clock();
			positive_clocks = negative_clocks = 0;
		}
	} else {
		if(register_id != -1) {
			cancel_event(register_id);
			register_id = -1;
			if(buffer_ptr == buffer_length) {
				emu->out_message(_T("CMT: Stop (End-of-Tape)"));
			} else if(buffer_ptr == 0) {
				emu->out_message(_T("CMT: Stop (Beginning-of-Tape)"));
			} else {
				emu->out_message(_T("CMT: Stop"));
			}
		}
		prev_clock = 0;
	}
	
	// update signals
	write_signals(&outputs_remote, remote ? 0xffffffff : 0);
	write_signals(&outputs_rotate, (register_id != -1) ? 0xffffffff : 0);
	write_signals(&outputs_end, (buffer_ptr == buffer_length) ? 0xffffffff : 0);
	write_signals(&outputs_top, (buffer_ptr == 0) ? 0xffffffff : 0);
}

bool KEYBOARD::play_tape(_TCHAR* file_path)
{
	close_tape();
	
	if(fio->Fopen(file_path, FILEIO_READ_BINARY)) {
		if(check_file_extension(file_path, _T(".cas"))) {
			buffer = (uint8 *)malloc(0x10000);
			buffer_length=load_cas_image();
			CasIndex=0;
			is_wav = true;
		} else if(check_file_extension(file_path, _T(".p6"))) {
			buffer = (uint8 *)malloc(0x10000);
			buffer_length=load_cas_image();
			CasIndex=0;
			is_wav = true;
		} else {
			// unknown image
			return false;
		}
		if(!is_wav && buffer_length != 0) {
			buffer_bak = (uint8 *)malloc(buffer_length);
			memcpy(buffer_bak, buffer, buffer_length);
		}
		
		// get the first signal
		bool signal = ((buffer[0] & 0x80) != 0);
		if(signal != in_signal) {
			write_signals(&outputs_out, signal ? 0xffffffff : 0);
			in_signal = signal;
		}
		
		play = true;
		update_event();
	}
	return play;
}

bool KEYBOARD::rec_tape(_TCHAR* file_path)
{
	close_tape();
	
	if(fio->Fopen(file_path, FILEIO_WRITE_BINARY)) {
		sample_rate = 48000;
		buffer_length = 1024 * 1024;
		buffer = (uint8 *)malloc(buffer_length);		
		// initialize buffer
		CasIndex=0;
		rec = true;
		update_event();
	}
	return rec;
}

void KEYBOARD::close_tape()
{
	close_file();
	
	play = rec = is_wav = false;
	buffer_ptr = buffer_length = 0;
	update_event();
	
	// no sounds
	write_signals(&outputs_out, 0);
	in_signal = false;
}

void KEYBOARD::close_file()
{
	if(rec) {
		fio->Fwrite(CasData, CasIndex + 1, 1);
	}
	if(play || rec) {
		fio->Fclose();
	}
	if(buffer != NULL) {
		free(buffer);
		buffer = NULL;
	}
	if(buffer_bak != NULL) {
		free(buffer_bak);
		buffer_bak = NULL;
	}
	if(apss_buffer != NULL) {
		free(apss_buffer);
		apss_buffer = NULL;
	}
}

int KEYBOARD::load_cas_image()
{	
	sample_rate = 48000;

	fio->Fseek(0, FILEIO_SEEK_SET);
	int ptr = 0, data;
	while((data = fio->Fgetc()) != EOF) {
		buffer[ptr] = data;
		ptr++;
	}
	return ptr;
}


void KEYBOARD::initialize()
{
	fio = new FILEIO();	
	play = rec = remote = trigger = false;
	ff_rew = 0;
	in_signal = out_signal = false;
	register_id = -1;
	
	buffer = buffer_bak = NULL;
	apss_buffer = NULL;
	buffer_ptr = buffer_length = 0;
	is_wav = false;
	changed = 0;

	key_stat = emu->key_buffer();
	kbFlagCtrl=0;
	kbFlagGraph=0;
	kbFlagFunc=0;
	kanaMode=0;
	katakana=0;
	stick0=0;
	p6key=0;
	counter=0;
	vcounter=0;
	tape=0;
	// register event to update the key status
	register_frame_event(this);
#ifndef _PC6001
	// register event
	register_vline_event(this);
#endif
}

void KEYBOARD::reset()
{
	p6key=0;
	CasMode=CAS_NONE;
	CasIndex=0;
	memset(CasData, 0, 0x10000);
	close_tape();
	portF3 = 0;
	portF7 = 0x06;
	portFA = 0;
	portFB = 0;
	TimerIntFlag=1;
	TimerSW_F3=1;
	IntSW_F3=1;
#ifdef _PC6801	
///	CmtIntFlag=0;
	StrigIntFlag=0;
///	KeyIntFlag=0;
///	keyGFlag=0;
/// TimerSWFlag=0;
/// WaitFlag=0;
	TvrIntFlag=0;
	DateIntFlag=0;
/// VrtcIntFlag=0;
	portBC=0x22;
	portF6 = 0x7f;
#else
	portF6 = 3;
#endif
}

void KEYBOARD::release()
{
	close_file();
	delete fio;
}

void KEYBOARD::event_frame()
{
	if (key_stat[VK_CONTROL] & 0x80) kbFlagCtrl=1;
	else kbFlagCtrl=0;
	stick0 = 0;
	if (key_stat[VK_SPACE]) stick0 |= STICK0_SPACE;
	if (key_stat[VK_LEFT]) stick0 |= STICK0_LEFT;
	if (key_stat[VK_RIGHT]) stick0 |= STICK0_RIGHT;
	if (key_stat[VK_DOWN]) stick0 |= STICK0_DOWN;
	if (key_stat[VK_UP]) stick0 |= STICK0_UP;
	if (key_stat[VK_F9]) stick0 |= STICK0_STOP;
	if (key_stat[VK_SHIFT]) stick0 |= STICK0_SHIFT;
///#ifdef _PC6001
///	d_mem->write_data8(0xfeca, stick0);
///#endif
	update_keyboard();
	if (p6key) d_cpu->write_signal(SIG_CPU_IRQ, 0x16, 0xff);
}
#ifndef _PC6001
void KEYBOARD::event_vline(int v, int clock)
{
	if (!sr_mode) {
		if(vcounter++ >= portF6 * 10) {
			vcounter = 0;
			if (TimerSW && TimerSW_F3) {
				d_cpu->write_signal(SIG_CPU_IRQ, 0x06, 0xff);
			}
		}
		return;
	}
	if (vcounter++ >= portF6 / 3) {
		vcounter=0;
		TimerIntFlag=1;
		if (IntSW_F3 && TimerSW_F3)
			d_cpu->write_signal(SIG_CPU_IRQ, 0x06, 0xff);
	}
	if (counter++ >= 200) {
		counter=0;
		TimerIntFlag=0;
		if (VrtcIntFlag)
			d_cpu->write_signal(SIG_CPU_IRQ, 0x06, 0xff);
	}
}
#endif
uint32 KEYBOARD::intr_ack() {
	/* interrupt priority (PC-6601SR) */
	/* 1.Timer     , 2.subCPU    , 3.Voice     , 4.VRTC      */
	/* 5.RS-232C   , 6.Joy Stick , 7.EXT INT   , 8.Printer   */
	if (p6key && IntSW_F3) { /* if any key pressed */
		p6key = 0;
		if (kbFlagGraph || kbFlagFunc) {
			kbFlagFunc=0;
			return(INTADDR_KEY2); /* special key (graphic key, etc.) */
		} else {
			return(INTADDR_KEY1); /* normal key */
		}
	} else if (StrigIntFlag && IntSW_F3) { /* if command 6 */
		StrigIntFlag=-1;
		return(INTADDR_STRIG);
	} else if (TimerIntFlag && IntSW_F3 && TimerSW_F3) {
		TimerIntFlag=0;
		return(INTADDR_TIMER); /* timer interrupt */
	} else if (CmtIntFlag && (p6key == 0xFA) && kbFlagGraph) {
		return(INTADDR_CMTSTOP); /* Press STOP while CMT Load or Save */
	} else if (tape++ > 20) {
		tape = 0;
		if (CmtIntFlag) return(INTADDR_CMTREAD);
#ifdef _PC6801
	} else if (sr_mode && DateIntFlag) { /// == INTFLAG_REQ && DateMode ==DATE_READ) //date interrupt
		return( INTADDR_DATE);
	} else if (sr_mode && TvrIntFlag) { /// == INTFLAG_REQ && TvrMode ==TVR_READ) 
		return( INTADDR_TVR);
	} else if(sr_mode && VrtcIntFlag ) {	// VRTC interrupt 2002/4/21
		return( INTADDR_VRTC);
#endif
	}
	return(INTADDR_TIMER);
}

void KEYBOARD::write_io8(uint32 addr, uint32 data)
{
	uint16 port=(addr & 0x00ff);
	byte Value=data;
	switch(port)
	{
	case 0xBC:
		portBC= Value;
	case 0xF3:
		portF3=Value;
		TimerSW_F3=(Value&0x04)?0:1;
		IntSW_F3=(Value&0x01)?0:1;
		break;
	case 0xF4: break;
	case 0xF5: break;
	case 0xF6:
		portF6 = Value;
///		SetTimerIntClock(Value);
		break;
	case 0xF7:
		portF7 = Value;
		break;
	case 0xFA:
		portFA = Value;
		break;
	case 0xFB:
		portFB = Value;
		break;
	}
	if (addr==0x0690) {	// STRIG Interrupt REQUEST
		StrigIntFlag = 1;
	} else if (port == 0x90) {
		if (CasMode == CAS_SAVEBYTE) {  /* CMT SAVE */
			if (CasIndex<0x10000) CasData[CasIndex++]=data;
			CasMode=CAS_NONE; 
		}
		if (data==0x3e || data==0x3d) { //	１－１）0x3E 受信(1200baud）　または　0x3D 受信(600baud）
			CasMode=CAS_NONE;
		}
		if (data==0x39) { ///
			CasMode=CAS_NONE;
		}
		if (data==0x38) { /* CMT SAVE DATA */
			CasMode=CAS_SAVEBYTE; 
		}
		if (data==0x1e || data==0x1d) { //	１－１）0x1E 受信(1200baud）　または　0x1D 受信(600baud）
			CasMode=CAS_NONE;
		}
		if (data==0x1a && CasMode!=CAS_NONE) { /* CMT LOAD STOP */
			CasMode=CAS_NONE;
		}
		/* CMT LOAD OPEN(0x1E,0x19(1200baud)/0x1D,0x19(600baud)) */
		if (data==0x19) {
			CasMode=CAS_LOADING;
		}
	}
	d_pio->write_io8(addr, data);
}

uint32 KEYBOARD::read_io8(uint32 addr)
{
	uint16 port=(addr & 0x00ff);
	byte Value=0xff;
	switch(port)
	{
	case 0xF3: Value=portF3;break;
	case 0xF6: Value=portF6;break;
	case 0xF7: Value=portF7;break;
	case 0xFA: Value=portFA;break;
	case 0xFB: Value=portFB;break;
	}
	if (port == 0x90) {
		if (CasMode == CAS_LOADING && CasIndex < 0x10000) {
			Value=CasData[CasIndex++];
		} else if (StrigIntFlag==-1) {
			Value=stick0;
			StrigIntFlag=0;
		} else {
			Value = (d_pio->read_io8(addr));
		}
	}
	return Value;
}
