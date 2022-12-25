/*
 * xvtarga.c - load routine for 'targa' format pictures
 *
 * written and submitted by:
 *     Derek Dongray    (dongray@genrad.com)
 *
 * The format read/written is actually Targa type 2 uncompressed as
 * produced by POVray 1.0
 *
 * LoadTarga(fname, pinfo)
 * WriteTarga(fp, pic, ptype, w,h, rmap,gmap,bmap,numcols, cstyle)
 */


/*
 * Targa Format (near as I can tell)
 *   0:
 *   1: colormap type
 *   2: image type  (1=colmap RGB, 2=uncomp RGB, 3=uncomp gray)
 *   3: 
 *   4: 
 *   5: colormap_length, low byte
 *   6: colormap_length, high byte
 *   7: bits per cmap entry     (8, 24, 32)
 *
 *  12: width, low byte
 *  13: width, high byte
 *  14: height, low byte
 *  15: height, high byte
 *  16: bits per pixel (8, 24)
 *  17: flags  
 */

#include "xv.h"

/***************************************************/
FILE *xv_fopen(
     char *fname, char *mode)
{
  FILE *fp;

#ifndef VMS
  fp = fopen(fname, mode);
#else
  fp = fopen(fname, mode, "ctx=stm");
#endif

  return fp;
}

#include <string.h>
/*
 * Just about everyone needs the strings routines.  We provide both forms here,
 * index/rindex and strchr/strrchr, so any systems that don't provide them all
 * need to have #defines here.
 */

#ifndef X_NOT_STDC_ENV

#include <string.h>
#ifndef index
#define index strchr
#endif
#ifndef rindex
#define rindex strrchr
#endif

#else

#ifdef SYSV
#include <string.h>
#define index strchr
#define rindex strrchr
#else
#include <strings.h>
#define strchr index
#define strrchr rindex
#endif

#endif /* X_NOT_STDC_ENV */

/***************************************************/
char *BaseName(char *fname)
{
  char *basname;

  /* given a complete path name ('/foo/bar/weenie.gif'), returns just the
     'simple' name ('weenie.gif').  Note that it does not make a copy of
     the name, so don't be modifying it... */

  basname = (char *) rindex(fname, '/');
  if (!basname) basname = fname;
  else basname++;

  return basname;
}

static long filesize;
static char *bname;

/*******************************************/
int LoadTarga(
     char    *fname,  // file name
     PICINFO *pinfo)  // holds image data & info
/*******************************************/
{
  /* returns '1' on success */

  FILE  *fp;
  int    i, row, c, c1, w, h, flags, intlace, topleft;
  byte *pic24, *pp, *pic32;

  bname = BaseName(fname);

  pinfo->pic     = (byte *) NULL;
  pinfo->comment = (char *) NULL;

  fp=xv_fopen(fname,"rb");
  if (!fp) {
     fprintf(stderr,"%s:  %s", bname, "can't open file");
     return 0;
   }

  /* compute file length */
  fseek(fp, 0L, 2);
  filesize = ftell(fp);
  fseek(fp, 0L, 0);

  if (filesize < 18) {
     fclose(fp);
     fprintf(stderr,"%s:  %s", bname, "file is too short");
     return 0;
   }

  /* Discard the first few bytes of the file.
     The format check has already been done or we wouldn't be here. */

  for (i=0; i<12; i++) {
    c=getc(fp);
  }


  /* read in header information */
  c=getc(fp); c1=getc(fp);
  w = c1*256 + c;

  c=getc(fp); c1=getc(fp);
  h = c1*256 + c;
  // printf("w=%d h=%d\n",w,h);

  if (w<1 || h<1) {
    fclose(fp);
    fprintf(stderr,"%s:  error in Targa header (bad image size)", bname);
    return 0;
  }

  c=getc(fp);
  if ((c!=24) && (c!=32))  {
    fclose(fp);
    fprintf(stderr,"%s:  unsupported type (not 24/32-bit)", bname);
    fprintf(stderr,"bits=%d",c);
    return 0;
  }

  flags   = getc(fp);
  topleft = (flags & 0x20) >> 5;
  intlace = (flags & 0xc0) >> 6;
  // printf("intlace=%d topleft=%d\n",intlace,topleft);

if(c==24){
  pic24 = (byte *) calloc((size_t) w*h*3, (size_t) 1);
  if (!pic24) printf("couldn't malloc 'pic24'");

  /* read the data */
  for (i=0; i<h; i++) {
    if (intlace == 2) {        /* four pass interlace */
      if      (i < (1*h) / 4) row = 4 * i;
      else if (i < (2*h) / 4) row = 4 * (i - ((1*h)/4)) + 1;
      else if (i < (3*h) / 4) row = 4 * (i - ((2*h)/4)) + 2;
      else                    row = 4 * (i - ((3*h)/4)) + 3;
    }

    else if (intlace == 1) {   /* two pass interlace */
      if      (i < h / 2) row = 2 * i;
      else                row = 2 * (i - h/2) + 1;
    }
    
    else row = i;              /* no interlace */



    if (!topleft) row = (h - row - 1);     /* bottom-left origin: invert y */


    c = fread(pic24 + (row*w*3), (size_t) 1, (size_t) w*3, fp);
  }

  /* swap R,B values (file is in BGR, pic24 should be in RGB) */ 
  for (i=0, pp=pic24; i<w*h; i++, pp+=3) {
    c = pp[0];  pp[0] = pp[2];  pp[2] = (unsigned char)c;
  }
  
  pinfo->pic     = pic24;
  pinfo->type    = PIC24;
  pinfo->w       = w;
  pinfo->h       = h;
  pinfo->normw = pinfo->w;   pinfo->normh = pinfo->h;
  pinfo->frmType = F_TARGA;
  sprintf(pinfo->fullInfo,"Targa, uncompressed RGB.  (%ld bytes)", filesize);
  sprintf(pinfo->shrtInfo,"%dx%d Targa.", w,h);
  pinfo->colType = F_FULLCOLOR;
}
else{ // pic32
  pic32 = (byte *) calloc((size_t) w*h*4, (size_t) 1);
  if (!pic32) printf("couldn't malloc 'pic32'");


  /* read the data */
  for (i=0; i<h; i++) {
    if (intlace == 2) {        /* four pass interlace */
      if      (i < (1*h) / 4) row = 4 * i;
      else if (i < (2*h) / 4) row = 4 * (i - ((1*h)/4)) + 1;
      else if (i < (3*h) / 4) row = 4 * (i - ((2*h)/4)) + 2;
      else                    row = 4 * (i - ((3*h)/4)) + 3;
    }

    else if (intlace == 1) {   /* two pass interlace */
      if      (i < h / 2) row = 2 * i;
      else                row = 2 * (i - h/2) + 1;
    }
    
    else row = i;              /* no interlace */



    if (!topleft) row = (h - row - 1);     /* bottom-left origin: invert y */


    c = fread(pic32 + (row*w*4), (size_t) 1, (size_t) w*4, fp);
  }

  /* swap R,B values (file is in BGR, pic32 should be in RGB) */
  for (i=0, pp=pic32; i<w*h; i++, pp+=4) {
    c = pp[0];  pp[0] = pp[2];  pp[2] = (unsigned char)c;
  }

  pinfo->pic     = pic32;
  pinfo->type    = PIC32;
  pinfo->w       = w;
  pinfo->h       = h;
  pinfo->normw = pinfo->w;   pinfo->normh = pinfo->h;
  pinfo->frmType = F_TARGA;
  sprintf(pinfo->fullInfo,"Targa, uncompressed RGBA.  (%ld bytes)", filesize);
  sprintf(pinfo->shrtInfo,"%dx%d Targa.", w,h);
  pinfo->colType = F_FULLCOLOR;
}

  fclose(fp);

  return 1;
}


/*******************************************/
int WriteTarga(
     FILE *fp,
     byte *pic,
     int ptype, int w, int h,
     byte *rmap, byte *gmap, byte *bmap,
     int /*numcols*/, int /*colorstyle*/)
/*******************************************/
{
  int i, j;
  byte *xpic;

  /* write the header */
  for (i=0; i<12; i++) putc( (i==2) ? 2 : 0, fp);
  
  putc(w&0xff,     fp);
  putc((w>>8)&0xff,fp);
  putc(h&0xff,     fp);
  putc((h>>8)&0xff,fp);

  putc(24,fp);
  putc(0x20,fp);

  xpic = pic;

  for (i=0; i<h; i++) {
    //if ((i&63)==0) putchar('.');

    for (j=0; j<w; j++) {
      if (ptype==PIC8) {
	putc(bmap[*xpic],fp);  putc(gmap[*xpic],fp);  putc(rmap[*xpic],fp);
	xpic++;
      }
      else {  /* PIC24 */
	putc(xpic[2], fp);  /* b */
	putc(xpic[1], fp);  /* g */
	putc(xpic[0], fp);  /* r */
	xpic+=3;
      }
    }
  }

  if (ferror(fp)) return -1;

  return 0;
}

#ifdef TGA_2_TXM
#include "mtl_txt.h"
void Tga2txm(char *file_name, txt_lib *tl, int keep_ext)
{
  PICINFO pinfo;
  VRGB *rgb; // Source data image
  unsigned char *alpha;
  txt *t;
  int x,y;
  char txt_name[256] = {0};

  t=(txt*)Malloc(sizeof(txt));

  pinfo.numpages = 1;
  pinfo.pagebname[0] = '\0';
  if(LoadTarga(file_name, &pinfo) != 1){
    fprintf(stderr,"\nError: LoadTarga failed.\n");
    exit(1);
  }
  // printf("w=%d h=%d\n",pinfo.normw, pinfo.normh);

  InitTexture(t);
  strcpy(txt_name, file_name);
  StripPath(txt_name);
  if(!keep_ext)
	StripExtension(txt_name);

  t->name=(char*)Malloc((strlen(txt_name)+1)*sizeof(char));
  strcpy(t->name, txt_name);
  t->mip_count=1;
  t->mip_map=(mip**)Malloc(sizeof(mip*));
  t->mip_map[0]=(mip*)Malloc(sizeof(mip));
  InitMip(t->mip_map[0]);
  t->mip_map[0]->level=(char*)Malloc((strlen("MIP level 0")+1)*sizeof(char));
  strcpy(t->mip_map[0]->level, "MIP level 0");
  t->mip_map[0]->x_size=pinfo.normw;
  t->mip_map[0]->y_size=pinfo.normh;
#ifdef N_PALETTE_COLORS
  t->mip_map[0]->color_count=N_PALETTE_COLORS;
#else
  t->mip_map[0]->color_count=256;
#endif
  t->mip_map[0]->depth = (color_depth)txt_depth;

  if(pinfo.type==PIC24){
    // 1/3 for mip data
    rgb=(VRGB*)malloc((4*pinfo.normw*pinfo.normh*sizeof(VRGB))/3); 
    for(y=0; y<pinfo.normh; y++){
      for(x=0; x<pinfo.normw; x++){
        rgb[y*pinfo.normw+x].r=pinfo.pic[3*y*pinfo.normw+3*x];
        rgb[y*pinfo.normw+x].g=pinfo.pic[3*y*pinfo.normw+3*x+1];
        rgb[y*pinfo.normw+x].b=pinfo.pic[3*y*pinfo.normw+3*x+2];
      }
    }
    LoadTxtRGB(rgb, t);
    free(rgb);
  }else
  if(pinfo.type==PIC32){
    // 1/3 for mip data
    rgb=(VRGB*)Malloc((4*pinfo.normw*pinfo.normh*sizeof(VRGB))/3); 
    alpha=(unsigned char*)Malloc((4*pinfo.normw*pinfo.normh*
                                  sizeof(unsigned char))/3);
    for(y=0; y<pinfo.normh; y++){
      for(x=0; x<pinfo.normw; x++){
        rgb[y*pinfo.normw+x].r=pinfo.pic[4*y*pinfo.normw+4*x];
        rgb[y*pinfo.normw+x].g=pinfo.pic[4*y*pinfo.normw+4*x+1];
        rgb[y*pinfo.normw+x].b=pinfo.pic[4*y*pinfo.normw+4*x+2];
        alpha[y*pinfo.normw+x]=pinfo.pic[4*y*pinfo.normw+4*x+3];
      }
    }
    LoadTxtRGB(rgb, t);
    free(rgb);

    LoadTxtAlpha(alpha, t);
    Free(alpha);
  }else
  if(pinfo.type==PIC8){
    fprintf(stderr,"PIC8 not presently supported.\n");
  }
  else{
    fprintf(stderr,"Unknown TGA format %d.\n",pinfo.type);
  }

  InsertTexture(tl, t);
}
#endif
