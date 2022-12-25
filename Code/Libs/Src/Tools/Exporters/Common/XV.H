#ifndef XV_H
#define XV_H
#include <stdio.h>
#include <stdlib.h>

/* values 'picType' can take */
/* indicies into conv24MB */
#define CONV24_8BIT  0
#define CONV24_24BIT 1
#define CONV32_32BIT 2
#define PIC8  CONV24_8BIT
#define PIC24 CONV24_24BIT
#define PIC32 CONV32_32BIT

#define F_TARGA     9
#define F_FULLCOLOR 0

#define F_BMP       ( 6 )
#define ISTR_WARNING  1
#define F_GREYSCALE 1
#define F_BWDITHER  2
#define ISTR_WARNING  1

#define MONO(rd,gn,bl) ( ((int)(rd)*11 + (int)(gn)*16 + (int)(bl)*5) >> 5)

#ifndef PARM
#define PARM(a) a
#endif
#if 0
#undef PARM
#ifdef __STDC__
#  define PARM(a) a
#else
#  define PARM(a) ()
#  define const
#endif
#endif

#ifndef byte
typedef unsigned char byte;
#endif

#ifndef u_int
typedef unsigned int u_int;
#endif

typedef struct { byte *pic;                  /* image data */
                 int   w, h;                 /* pic size */
                 int   type;                 /* PIC8 or PIC24 */

                 byte  r[256],g[256],b[256];
                                             /* colormap, if PIC8 */

                 int   normw, normh;         /* 'normal size' of image file
                                                (normally eq. w,h, except when
                                                doing 'quick' load for icons */

                 int   frmType;              /* def. Format type to save in */
                 int   colType;              /* def. Color type to save in */
                 char  fullInfo[128];        /* Format: field in info box */
                 char  shrtInfo[128];        /* short format info */
                 char *comment;              /* comment text */

                 int   numpages;             /* # of page files, if >1 */
                 char  pagebname[64];        /* basename of page files */
               } PICINFO;
#endif
