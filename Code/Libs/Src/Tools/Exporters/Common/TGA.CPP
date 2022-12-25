
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

#include "mmsystem.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <io.h>
#include <fcntl.h>
#include <float.h>

//#include "3db.h"
#include "mtl_txt.h"
#include "names.h"

struct VFX_RGBA
{
   U8 r;
   U8 g;
   U8 b;
   U8 alpha;
};

//****************************************************************************
//*                                                                          *
//*  Read all or part of a file into memory, returning memory location       *
//*  or NULL on error                                                        *
//*                                                                          *
//*  Memory will be allocated if dest==NULL                                  *
//*                                                                          *
//****************************************************************************

void * FILE_read (C8     *filename, //)     
                  void   *dest         = NULL,
                  S32     len          = -1,
                  S32     start_offset = 0)

{
   HANDLE handle;
   U32    n_bytes;
   U32    nbytes_read;
   S32    result;
   void  *buf;

   //
   // Open file
   //

   handle = CreateFile(filename,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                       NULL);

   if (handle == INVALID_HANDLE_VALUE)
      {
      return NULL;
      }

   //
   // Set pointer to beginning of range
   //

   if (SetFilePointer(handle,
                      start_offset,
                      NULL,
                      FILE_BEGIN) == 0xffffffff)
      {
      CloseHandle(handle);
      return NULL;
      }

   //
   // Allocate memory for file range
   //

   n_bytes = len;

   if (n_bytes == 0xffffffff)
      {
      n_bytes = GetFileSize(handle, NULL) - start_offset;
      }

   buf = (dest == NULL) ? malloc(n_bytes) : dest;

   if (buf == NULL)
      {
      CloseHandle(handle);
      return NULL;
      }

   //
   // Read range
   //

   result = ReadFile(handle,
                     buf,
                     n_bytes,
                    &nbytes_read,
                     NULL);

   CloseHandle(handle);

   if ((!result) || (nbytes_read != n_bytes))
      {
      if (dest != buf)
         {
         free(buf);
         }

      return NULL;
      }   

   return buf;
}

//------------------------------------------------------------------
//
// 8-bpp / 24-bpp / 32-bpp .TGA parser
//
//------------------------------------------------------------------

#pragma pack(1)  // Do NOT allow compiler to reorder structs!

struct TGA_HDR
{
   U8    len_image_ID;
   U8    color_map_present;
   U8    image_type;
   S16   color_map_origin;
   S16   color_map_len;
   U8    color_map_entry_size;
   S16   X_origin;
   S16   Y_origin;
   S16   pixel_width;
   S16   pixel_height;
   U8    bits_per_pixel;
   U8    image_descriptor_flags;
};

#define TGA_INTERLEAVE_4 0x80
#define TGA_INTERLEAVE_2 0x40
#define TGA_ORIGIN_TOP   0x20
#define TGA_ORIGIN_RIGHT 0x10
#define TGA_ATTRIB_MASK  0x0F

#define TGA_FORMAT_PAL_UNCOMPRESSED 1
#define TGA_FORMAT_PAL_RLE          9
#define TGA_FORMAT_RGB_UNCOMPRESSED 2
#define TGA_FORMAT_RGB_RLE          10

#pragma pack()

//****************************************************************************
//
// Parse memory-resident copy of .TGA file, returning dimensions and pointer
// to decoded pixel block
//
//****************************************************************************

VFX_RGBA * __cdecl TGA_parse(void *TGA_image, S32 *x_res, S32 *y_res, S32 *orig_BPP)
{
   //
   // Acquire header pointer and validate file type
   //

   TGA_HDR *file = (TGA_HDR *) TGA_image;

   switch (file->image_type)
      {
      case 0:
         printf("No image data in file");
         return NULL;

      case TGA_FORMAT_RGB_UNCOMPRESSED:
      case TGA_FORMAT_RGB_RLE:
      case TGA_FORMAT_PAL_UNCOMPRESSED:
      case TGA_FORMAT_PAL_RLE:
         break;

      default:
         printf("Unrecognized image format %d -- 24-bpp or 32-bpp RGB file required",
            file->image_type);
         return NULL;
      }

   //
   // Require 8-bit, 24-bit, or 32-bit files
   //

   if ((file->bits_per_pixel != 32) && 
       (file->bits_per_pixel != 24) &&
       (file->bits_per_pixel != 8))
      {
      printf("File has %d bits per pixel -- 8-bpp, 24-bpp, or 32-bpp file required",
         file->bits_per_pixel);
      return NULL;
      }

   S32 BPP = file->bits_per_pixel / 8;

   //
   // We don't support interleaved files or files with weird origins...
   //

   if (file->image_descriptor_flags & (TGA_INTERLEAVE_2 | 
                                       TGA_INTERLEAVE_4 |
                                       TGA_ORIGIN_RIGHT))
      {
      printf("Unrecognized image descriptor %X",
         file->image_descriptor_flags);
      return NULL;
      }

   //
   // Input pointer follows header
   //

   U8 *in = ((U8 *) file) + sizeof(TGA_HDR);

   //
   // Skip image ID field, if present
   //

   in += file->len_image_ID;

   //
   // Skip color map, if present
   //

   VFX_RGB *color_map = NULL;

   if (file->color_map_present)
      {
      color_map = (VFX_RGB *) in;

      in += (((file->color_map_entry_size) / 8) * file->color_map_len);
      }

   //
   // Allocate memory for output RGBA data
   //

   VFX_RGBA *out = (VFX_RGBA *) malloc(sizeof(VFX_RGBA) * 
                                       file->pixel_width *
                                       file->pixel_height);

   if (out == NULL)
      {
      printf("Could not allocate output data block");
      return NULL;
      }

   //
   // Set up Y origin and increment
   //

   S32 y,dy,ylim;

   if (file->image_descriptor_flags & TGA_ORIGIN_TOP)
      {
      y    =  0;
      dy   =  1;
      ylim =  file->pixel_height;
      }
   else
      {
      y    =  file->pixel_height-1;
      dy   = -1;
      ylim = -1;
      }       

   //
   // Unpack file to output RGB buffer
   // 

   S32 w = file->pixel_width;

   switch (file->image_type)
      {
      //
      // Uncompressed palettized data
      // 

      case TGA_FORMAT_PAL_UNCOMPRESSED:
         {
         while (y != ylim)
            {
            for (S32 x=0; x < w; x++)
               {
               VFX_RGBA RGB;

               RGB.r = color_map[*in].b;
               RGB.g = color_map[*in].g;
               RGB.b = color_map[*in].r;
               RGB.alpha = 0;

               in += BPP;

               out[(y * w) + x] = RGB;
               }

            y += dy;
            }
         break;
         }

      //
      // RLE-encoded palettized data
      //

      case TGA_FORMAT_PAL_RLE:
         {
         enum {rep,raw};
         S32  state;
         S32  cnt;
         U8  *val;

         state = (*in & 0x80) ? rep : raw;
         cnt   = (*in & 0x7f);
         val   = &in[1];

         while (y != ylim)
            {
            for (S32 x=0; x < w; x++)
               {
               VFX_RGBA RGB;

               RGB.r = color_map[*val].b;
               RGB.g = color_map[*val].g;
               RGB.b = color_map[*val].r;
               RGB.alpha = 0;

               if (state == raw)
                  {
                  val += BPP;
                  }

               if (!cnt--)
                  {
                  in    = &val[(state == rep) * BPP];
                  state =  (*in & 0x80) ? rep : raw;
                  cnt   =  (*in & 0x7f);
                  val   = &in[1];
                  }

               out[(y * w) + x] = RGB;
               }

            y += dy;
            }

         break;
         }

      //
      // Uncompressed RGB data
      //

      case TGA_FORMAT_RGB_UNCOMPRESSED:
         {
         while (y != ylim)
            {
            for (S32 x=0; x < w; x++)
               {
               VFX_RGBA RGB;

               RGB.r = in[2];
               RGB.g = in[1];
               RGB.b = in[0];

               if (BPP == 4)
                  {
                  RGB.alpha = in[3];
                  }
               else
                  {
                  RGB.alpha = 0;
                  }

               in += BPP;

               out[(y * w) + x] = RGB;
               }

            y += dy;
            }
         break;
         }

      //
      // RLE-encoded RGB data
      //

      case TGA_FORMAT_RGB_RLE:
         {
         enum {rep,raw};
         S32  state;
         S32  cnt;
         U8  *val;

         state = (*in & 0x80) ? rep : raw;
         cnt   = (*in & 0x7f);
         val   = &in[1];

         while (y != ylim)
            {
            for (S32 x=0; x < w; x++)
               {
               VFX_RGBA RGB;

               RGB.r = val[2];
               RGB.g = val[1];
               RGB.b = val[0];

               if (BPP == 4)
                  {
                  RGB.alpha = val[3];
                  }
               else
                  {
                  RGB.alpha = 0;
                  }

               if (state == raw)
                  {
                  val += BPP;
                  }

               if (!cnt--)
                  {
                  in    = &val[(state == rep) * BPP];
                  state =  (*in & 0x80) ? rep : raw;
                  cnt   =  (*in & 0x7f);
                  val   = &in[1];
                  }

               out[(y * w) + x] = RGB;
               }

            y += dy;
            }

         break;
         }
      }
 
   //
   // Return file location and size to caller
   //

   if (x_res != NULL)
      {
      *x_res = file->pixel_width;
      }

   if (y_res != NULL)
      {
      *y_res = file->pixel_height;
      }

   if (orig_BPP != NULL)
      {
      *orig_BPP = BPP;
      }

   return out;
}

//------------------------------------------------------------------
void Tga2txm(char *file_name, txt_lib *tl, int keep_ext)
{
  VRGB *rgb; // Source data image
  unsigned char *alpha;
  txt *t;
  int x,y;
  char txt_name[256] = {0};


  void *ptr = FILE_read(file_name);

  if (ptr == NULL)
   {
   fprintf(stderr,"\nFile not found\n");
   exit(1);
   }

  S32 width,height,BPP;

  VFX_RGBA *image = TGA_parse(ptr, &width, &height, &BPP);

  if (image == NULL)
   {
   fprintf(stderr,"\nCould not parse .TGA file\n");
   exit(1);
   }

  t = (txt *) Malloc(sizeof(txt));

  // printf("w=%d h=%d\n",width, height);

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
  t->mip_map[0]->level=(char*)Malloc((strlen(MIP_level_0)+1)*sizeof(char));
  strcpy(t->mip_map[0]->level, MIP_level_0);
  t->mip_map[0]->x_size=width;
  t->mip_map[0]->y_size=height;
  t->mip_map[0]->color_count=N_PALETTE_COLORS; // 255
  t->mip_map[0]->depth = (color_depth)txt_depth;

  if((BPP == 3) || (BPP == 1)){
    // 1/3 for mip data
    rgb=(VRGB*)malloc((4*width*height*sizeof(VRGB))/3); 
    for(y=0; y<height; y++){
      for(x=0; x<width; x++){
        rgb[y*width+x].r=image[y*width+x].r;
        rgb[y*width+x].g=image[y*width+x].g;
        rgb[y*width+x].b=image[y*width+x].b;
      }
    }
    LoadTxtRGB(rgb, t);
    free(rgb);
  }else
  if(BPP == 4){
    // 1/3 for mip data
    rgb=(VRGB*)Malloc((4*width*height*sizeof(VRGB))/3); 
    alpha=(unsigned char*)Malloc((4*width*height*
                                  sizeof(unsigned char))/3);
    for(y=0; y<height; y++){
      for(x=0; x<width; x++){
        rgb[y*width+x].r=image[y*width+x].r;
        rgb[y*width+x].g=image[y*width+x].g;
        rgb[y*width+x].b=image[y*width+x].b;
        alpha[y*width+x]=image[y*width+x].alpha;
      }
    }
    LoadTxtRGB(rgb, t);
    free(rgb);

    LoadTxtAlpha(alpha, t);
    Free(alpha);
  }else{
    fprintf(stderr,"Unknown TGA format %d.\n",BPP);
  }

  InsertTexture(tl, t);
}
