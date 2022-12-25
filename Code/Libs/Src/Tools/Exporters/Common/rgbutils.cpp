//$Header: /Tools/Exporters/Common/rgbutils.cpp 7     8/07/99 9:15p Mstembera $
//Copyright (c) 1997 Digital Anvil, Inc.

#include <memory.h>
#include <malloc.h>
#include <stddef.h>
#include <limits.h>
#include <assert.h>

#ifndef SGI
#pragma warning( 3 : 4100 ) // unreferenced formal parameter
#pragma warning( 3 : 4189 ) // local variable is initialized but not referenced
#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( error : 4700 )
#pragma warning( 3 : 4706 ) // assignment within conditional expression
#endif

#include "rgbutils.h"

//****************************************************************************
//*                                                                          *
//* VFX RGB color manipulation library                                       *
//*                                                                          *
//* 32-bit protected-mode source compatible with MSVC 10.2                   *
//*                                                                          *
//* Version 1.00 of 28-Jan-97: Initial, derived from IMAGEMAN RGBUTILS.H     *
//*                                                                          *
//* Author: John Miles                                                       *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  Contains C++ implementation of Wu's color quantizer (v. 2)              *
//*  (See Graphics Gems vol. II, pp. 126-133)                                *
//*                                                                          *
//*  Author:     Xiaolin Wu                                                  *
//*              Dept. of Computer Science                                   *
//*              Univ. of Western Ontario                                    *
//*              London, Ontario N6A 5B7                                     *
//*              wu@csd.uwo.ca                                               *
//*                                                                          *
//*  Algorithm:  Greedy orthogonal bipartition of RGB space for variance     *
//*              minimization aided by inclusion-exclusion tricks.           *
//*                                                                          *
//*  The author thanks Tom Lane at Tom_Lane@G.GP.CS.CMU.EDU for much         *
//*  additional documentation and a cure to a previous bug.                  *
//*  Free to distribute, comments and suggestions are appreciated.           *
//*                                                                          *
//*  Modifications:                                                          *
//*                                                                          *
//*  25-Sep-93: (John Miles) Modified to use 386FX 32-bit types and          *
//*                          class-style API; added CMAP_class (nearest-     *
//*                          neighbor mapping)                               *
//*                                                                          *
//*  28-Jan-97: (John Miles) Modified to use TYPEDEFS.H, ported to C++       *
//*                                                                          *
//****************************************************************************

#define RGBU_RED   2
#define RGBU_GREEN 1   
#define RGBU_BLUE  0

//****************************************************************************
//
// At conclusion of the histogram step, we can interpret
//   wt[r] [g] [b] = sum over voxel of P(c)
//   mr[r] [g] [b] = sum over voxel of r*P(c), similarly for mg, mb
//   m2[r] [g] [b] = sum over voxel of c^2*P(c)
//
// Actually each of these should be divided by 'size' to give the usual
// interpretation of P() as ranging from 0 to 1, but we needn't do that here.
//
// We convert histogram into cumulative moments so that we can 
// rapidly calculate the sums of the above quantities over any desired box.
//
//****************************************************************************

void CQ::M3d(long *vwt, long *vmr, long *vmg, long *vmb, float *m2) const
{
   unsigned long    ind1, ind2;
   unsigned char     i, r, g, b;
   long    line, line_r, line_g, line_b,
          area[33], area_r[33], area_g[33], area_b[33];
   float line2, area2[33];

   for (r=1; r<=32; ++r)
     {
     for (i=0; i<=32; ++i)
        {
        area2[i] = 0.0F;
        area[i] = area_r[i] = area_g[i] = area_b[i] = 0;
        }

     for (g=1; g<=32; ++g)
        {
        line2 = 0.0F;
        line = line_r = line_g = line_b = 0;

        for (b=1; b<=32; ++b)
           {
           //
           // [r] [g] [b]
           //

           ind1 = (r<<10) + (r<<6) + r + (g<<5) + g + b;

           line   += vwt[ind1];
           line_r += vmr[ind1]; 
           line_g += vmg[ind1]; 
           line_b += vmb[ind1];

           line2  += m2[ind1];

           area[b]   += line;
           area_r[b] += line_r;
           area_g[b] += line_g;
           area_b[b] += line_b;

           area2[b] += line2;

           //
           // [r-1] [g] [b]
           //

           ind2 = ind1 - (33*33);

           vwt[ind1] = vwt[ind2] + area  [b];
           vmr[ind1] = vmr[ind2] + area_r[b];
           vmg[ind1] = vmg[ind2] + area_g[b];
           vmb[ind1] = vmb[ind2] + area_b[b];

           m2[ind1] = m2[ind2] + area2[b];
           }
        }
     }
}

//****************************************************************************
//
// Compute sum over a box of any given statistic
//
//****************************************************************************

long CQ::Vol(const RGB_BOX& cube, const CQT& mmt) const
{
   return (mmt.t [cube.r1] [cube.g1] [cube.b1] 
          -mmt.t [cube.r1] [cube.g1] [cube.b0]
          -mmt.t [cube.r1] [cube.g0] [cube.b1]
          +mmt.t [cube.r1] [cube.g0] [cube.b0]
          -mmt.t [cube.r0] [cube.g1] [cube.b1]
          +mmt.t [cube.r0] [cube.g1] [cube.b0]
          +mmt.t [cube.r0] [cube.g0] [cube.b1]
          -mmt.t [cube.r0] [cube.g0] [cube.b0]);
}

//****************************************************************************
//
// The next two routines allow a slightly more efficient calculation
// of Vol() for a proposed subbox of a given box.  The sum of Top()
// and Bottom() is the Vol() of a subbox split in the given direction
// and with the specified new upper bound.
//
// Compute part of Vol(cube, mmt) that doesn't depend on r1, g1, or b1
// (depending on dir)
//
//****************************************************************************

long CQ::Bottom(const RGB_BOX& cube, unsigned char dir, const CQT& mmt) const
{
   switch (dir)
      {
      case RGBU_RED   : return (-mmt.t [cube.r0] [cube.g1] [cube.b1]
                           +mmt.t [cube.r0] [cube.g1] [cube.b0]
                           +mmt.t [cube.r0] [cube.g0] [cube.b1]
                           -mmt.t [cube.r0] [cube.g0] [cube.b0]);
                                
                                
      case RGBU_GREEN : return (-mmt.t [cube.r1] [cube.g0] [cube.b1]
                           +mmt.t [cube.r1] [cube.g0] [cube.b0]
                           +mmt.t [cube.r0] [cube.g0] [cube.b1]
                           -mmt.t [cube.r0] [cube.g0] [cube.b0]);
                                
      case RGBU_BLUE  : return (-mmt.t [cube.r1] [cube.g1] [cube.b0]
                           +mmt.t [cube.r1] [cube.g0] [cube.b0]
                           +mmt.t [cube.r0] [cube.g1] [cube.b0]
                           -mmt.t [cube.r0] [cube.g0] [cube.b0]);
      }

   return 0;
}

//****************************************************************************
//
// Compute remainder of Vol(cube, mmt), substituting pos for
// r1, g1, or b1 (depending on dir)
//
//****************************************************************************

long CQ::Top(const RGB_BOX& cube, unsigned char dir, long pos, const CQT& mmt) const
{
   switch (dir)
      {
      case RGBU_RED   : return (mmt.t [pos] [cube.g1] [cube.b1]  
                          -mmt.t [pos] [cube.g1] [cube.b0]
                          -mmt.t [pos] [cube.g0] [cube.b1]
                          +mmt.t [pos] [cube.g0] [cube.b0]);
                               
      case RGBU_GREEN : return (mmt.t [cube.r1] [pos] [cube.b1] 
                          -mmt.t [cube.r1] [pos] [cube.b0]
                          -mmt.t [cube.r0] [pos] [cube.b1]
                          +mmt.t [cube.r0] [pos] [cube.b0]);
                               
      case RGBU_BLUE  : return (mmt.t [cube.r1] [cube.g1] [pos]
                          -mmt.t [cube.r1] [cube.g0] [pos]
                          -mmt.t [cube.r0] [cube.g1] [pos]
                          +mmt.t [cube.r0] [cube.g0] [pos]);
      }

   return 0;
}

//****************************************************************************
//
// Compute the weighted variance of a box
// NB: as with the raw statistics, this is really the variance * size
//
//****************************************************************************

float CQ::Var(const RGB_BOX& cube) const
{
   float dr, dg, db, xx;

   dr = (float) Vol(cube, mr); 
   dg = (float) Vol(cube, mg); 
   db = (float) Vol(cube, mb);

   xx = m2 [cube.r1*33*33 + cube.g1*33 + cube.b1] 
       -m2 [cube.r1*33*33 + cube.g1*33 + cube.b0]
       -m2 [cube.r1*33*33 + cube.g0*33 + cube.b1]
       +m2 [cube.r1*33*33 + cube.g0*33 + cube.b0]
       -m2 [cube.r0*33*33 + cube.g1*33 + cube.b1]
       +m2 [cube.r0*33*33 + cube.g1*33 + cube.b0]
       +m2 [cube.r0*33*33 + cube.g0*33 + cube.b1]
       -m2 [cube.r0*33*33 + cube.g0*33 + cube.b0];

   return (xx - (dr*dr + dg*dg + db*db) / (float) Vol(cube, wt));    
}

//****************************************************************************
//
// We want to minimize the sum of the variances of two subboxes.
// The sum(c^2) terms can be ignored since their sum over both subboxes
// is the same (the sum for the whole box) no matter where we split.
// The remaining terms have a minus sign in the variance formula,
// so we drop the minus sign and MAXIMIZE the sum of the two terms.
//
//****************************************************************************

float CQ::Maximize(const RGB_BOX& cube, //)
                   unsigned char    dir, 
                   long     first, 
                   long     last, 
                   long    *cut,
                   long     whole_r, 
                   long     whole_g, 
                   long     whole_b, 
                   long     whole_w) const
{
   long    half_r, half_g, half_b, half_w;
   long    base_r, base_g, base_b, base_w;
   long    i;
   float temp, max;

   base_r = Bottom(cube, dir, mr);
   base_g = Bottom(cube, dir, mg);
   base_b = Bottom(cube, dir, mb);
   base_w = Bottom(cube, dir, wt);

   max = 0.0F;
   *cut = -1;

   for (i=first; i<last; ++i)
      {
      half_r = base_r + Top(cube, dir, i, mr);
      half_g = base_g + Top(cube, dir, i, mg);
      half_b = base_b + Top(cube, dir, i, mb);
      half_w = base_w + Top(cube, dir, i, wt);

      //
      // Now half_x is sum over lower half of box, if split at i 
      //
      // Subbox could be empty of pixels; never split into an empty box
      // 

      if (half_w == 0)
         continue;

      temp = ((float) half_r*half_r +
              (float) half_g*half_g +
              (float) half_b*half_b) / half_w;

      half_r = whole_r - half_r;
      half_g = whole_g - half_g;
      half_b = whole_b - half_b;
      half_w = whole_w - half_w;

      //
      // Subbox could be empty of pixels; never split into an empty box
      // 

      if (half_w == 0)
         continue;

      temp += ((float) half_r*half_r +
               (float) half_g*half_g +
               (float) half_b*half_b) / half_w;

      if (temp > max)
         {
         max=temp;
         *cut=i;
         }
      }

   return max;
}

long CQ::Cut(RGB_BOX *set1, RGB_BOX *set2) const
{
   unsigned char     dir;
   long    cutr, cutg, cutb;
   float maxr, maxg, maxb;
   long    whole_r, whole_g, whole_b, whole_w;

   whole_r = Vol(*set1, mr);
   whole_g = Vol(*set1, mg);
   whole_b = Vol(*set1, mb);
   whole_w = Vol(*set1, wt);

   maxr = Maximize(*set1, RGBU_RED,   set1->r0+1, set1->r1, &cutr,
                   whole_r, whole_g, whole_b, whole_w);

   maxg = Maximize(*set1, RGBU_GREEN, set1->g0+1, set1->g1, &cutg,
                   whole_r, whole_g, whole_b, whole_w);

   maxb = Maximize(*set1, RGBU_BLUE,  set1->b0+1, set1->b1, &cutb,
                   whole_r, whole_g, whole_b, whole_w);

   if ((maxr >= maxg) && (maxr >= maxb))
      {
      dir = RGBU_RED;
                            
      if (cutr < 0)
         return 0;
      }
   else
      if ((maxg >= maxr) && (maxg >= maxb))
         {
         dir = RGBU_GREEN;

         if (cutg < 0)
            return 0;
         }
      else
         {
         dir = RGBU_BLUE;

         if (cutb < 0)
            return 0;
         }

    set2->r1 = set1->r1;
    set2->g1 = set1->g1;
    set2->b1 = set1->b1;

    switch (dir)
      {
      case RGBU_RED:

          set2->r0 = set1->r1 = cutr;
          set2->g0 = set1->g0;
          set2->b0 = set1->b0;
          break;

      case RGBU_GREEN:

          set2->g0 = set1->g1 = cutg;
          set2->r0 = set1->r0;
          set2->b0 = set1->b0;
          break;

      case RGBU_BLUE:

          set2->b0 = set1->b1 = cutb;
          set2->r0 = set1->r0;
          set2->g0 = set1->g0;
          break;
      }

    set1->vol = (set1->r1 - set1->r0) *
                (set1->g1 - set1->g0) *
                (set1->b1 - set1->b0);

    set2->vol = (set2->r1 - set2->r0) *
                (set2->g1 - set2->g0) *
                (set2->b1 - set2->b0);

    return 1;
}

//****************************************************************************
//
// Construct an instance of class CQ
//
// Note: Each instance requires approx. 800K for histographic data,
// etc.
//
//****************************************************************************

CQ::CQ(void)
{
   //
   // Warning: virtual functions unsupported!
   //
   
   memset(this, 0, sizeof(*this));

   m2 = (float *) calloc(33*33*33,sizeof(float));

   reset();
}

//****************************************************************************
//
// Free an instance of CQ
//
//****************************************************************************

CQ::~CQ(void)
{
   free(m2);
}

//****************************************************************************
//
// Initialize color histogram
//
// Histogram is in elements 1..HISTSIZE along each axis,
// element 0 is for base or marginal value
//
//****************************************************************************

void CQ::reset(void)
{
   unsigned long i, j, k;
   
   for (i=0; i<33; ++i)
      for (j=0; j<33; ++j)
         for (k=0; k<33; ++k)
            {
            wt.t [i] [j] [k] = 0;
            mr.t [i] [j] [k] = 0;
            mg.t [i] [j] [k] = 0;
            mb.t [i] [j] [k] = 0;

            m2[i*33*33 + j*33 + k] = 0.0F;
            }
}

//****************************************************************************
//
// Build 3D color histogram of color counts
//
//****************************************************************************

void CQ::add_color(const VFX_RGB& triplet)
{
   static long table[256];
   static long table_valid = 0;
   long        r, g, b;
   long        inr, ing, inb;
   long        i;

   //
   // Build table of squares, if not already valid
   // 

   if (!table_valid)
      {
      for (i=0; i<256; ++i)
         {
         table[i] = i*i;
         }

      table_valid = 1;
      }
      
   r = triplet.r; 
   g = triplet.g; 
   b = triplet.b;

   inr = (r >> 3) + 1; 
   ing = (g >> 3) + 1; 
   inb = (b >> 3) + 1;

   wt.t [inr] [ing] [inb]++;
   mr.t [inr] [ing] [inb] += r;
   mg.t [inr] [ing] [inb] += g;
   mb.t [inr] [ing] [inb] += b;

   m2 [inr*33*33 + ing*33 + inb] += (float)(table[r] + table[g] + table[b]);
}

//****************************************************************************
//
// Generate optimal color palette based on input
//
//****************************************************************************

unsigned long CQ::quantize(VFX_RGB *out, unsigned long colors)

{
   RGB_BOX *cube;
   unsigned long      next;
   unsigned long      k,i;
   long      weight;
   float  *vv, temp;

   if (colors == 0)
      {
      return 0;
      }

   if (colors > 256)
      {
      colors = 256;
      }

   if ((vv = (float *) calloc(colors,sizeof(float))) == NULL)
      return 0;

   if ((cube = (RGB_BOX *) calloc(colors,sizeof(RGB_BOX))) == NULL)
      {
      free(vv);
      return 0;
      }

   M3d((long *)    &wt,
       (long *)    &mr,
       (long *)    &mg,
       (long *)    &mb,
       (float *) m2);

   cube[0].r0 = cube[0].g0 = cube[0].b0 = 0;
   cube[0].r1 = cube[0].g1 = cube[0].b1 = 32;

   next = 0;

   for (i=1; i < colors; ++i)
      {
      if (Cut(&cube[next], &cube[i]))
         {
         //
         // Volume test ensures we won't try to cut one-cell box
         //

         vv[next] = (cube[next].vol > 1) ?
                    Var(cube[next]) : 0.0F;

         vv[i]    = (cube[i].vol > 1)    ?
                    Var(cube[i])    : 0.0F;
         }
      else
         {
         //
         // Don't try to split this box again
         // 

         vv[next] = 0.0F;
         i--;
         }

      next = 0; temp = vv[0];

      for (k=1; k <= i; ++k)
         {
         if (vv[k] > temp)
            {
            temp = vv[k];
            next = k;
            }
         }

      if (temp <= 0.0F)
         {
         colors = i+1;
         break;
         }
      }

   for (k=0; k < colors; ++k)
      {
      weight = Vol(cube[k], wt);

      if (weight)
         {
         out[k].r = (unsigned char) (Vol(cube[k], mr) / weight);
         out[k].g = (unsigned char) (Vol(cube[k], mg) / weight);
         out[k].b = (unsigned char) (Vol(cube[k], mb) / weight);
         }
      else
         {
         out[k].r = out[k].g = out[k].b = 0;
         }
      }

   free(cube);
   free(vv);

   return colors;
}

/***************************************************************************/
//
// CMAP_class notes:
//
// Like CQ above, the CMAP functions require 8-bit RGB values
// for proper operation, and operate at 5-bit resolution internally.
//
/***************************************************************************/

/***************************************************************************/
//
// Construct an instance of CMAP_class
//
/***************************************************************************/

CMAP::CMAP(const VFX_RGB *_palette, unsigned long _colors) : palette (_palette), colors (_colors)
{
	unsigned long i;

	scoreboard = (short *) calloc(32768, sizeof(short));

	//
	// Initialize all RGB scoreboard values to -1 (unmapped)
	//

	for (i=0; i < 32768; i++)
	{
		scoreboard[i] = -1;
	}
}

/***************************************************************************/
//
// Free an instance of CMAP
//
/***************************************************************************/

CMAP::~CMAP()
{
   if (scoreboard != NULL)
      {
      free(scoreboard);
      }
}

/***************************************************************************/
//
// Find palette color whose RGB value is closest to *triplet
//
// Minimize sum of square axis displacements; true Euclidean 
// distance is not needed for comparisons
//
/***************************************************************************/

unsigned char CMAP::nearest_neighbor(const VFX_RGB& triplet)
{
   unsigned long        r,g,b,key,c,min,dist;
   long        i,dr,dg,db;
   static unsigned long square[511];
   static unsigned long square_valid = 0;

   //
   // Convert 8-bit RGB to 5-bit RGB
   //

   r = triplet.r;
   g = triplet.g;
   b = triplet.b;

   //
   // See if this triplet has already been remapped; if so, return
   // proper value immediately
   //

   key = ((r>>3) << 10) | ((g>>3) << 5) | (b>>3);

  if (scoreboard[key] != -1)
  {
	return (unsigned char) scoreboard[key];
  }

   //
   // Build square[] table if not already valid
   //

  if (!square_valid)
  {
	for (i=-255; i<=255; i++)
	{
		square[i+255] = i*i;
	}

	square_valid = 1;
  }

   //
   // Find best-fit palette entry
   //

   i   = colors;
   min = ULONG_MAX;
   c   = 0;

  while (i > 0)
  {
	  i--;

	  dr = (long) palette[i].r - (long) r + 255;
	  dg = (long) palette[i].g - (long) g + 255;
	  db = (long) palette[i].b - (long) b + 255;

	  dist = square[dr] + square[dg] + square[db];

	 if (dist <= min)
	 {
		 c = i;

		if (dist > 0)
		{
			min = dist;
		}
		 else
		{
			break;
		}
	 }
  }

   //
   // Log match in scoreboard to avoid redundant searches later
   //

   scoreboard[key] = (short) c;

   return (unsigned char) c;
}

#define __max(a, b)  (((a) > (b)) ? (a) : (b))

void CalcNewSize(const int old_width, const int old_height, int & new_width, int & new_height)
{
	if(new_width == -1 && new_height == -1)
	{
#if 1 // non square but still power of 2

		new_width = old_width;
		// make power of 2
		if(new_width & (new_width-1))
		{
			int tmp_size = 1;
			while(tmp_size < new_width)
			{
				tmp_size *= 2;
			}
			new_width = tmp_size;
		}

		new_height = old_height;
		// make power of 2
		if(new_height & (new_height-1))
		{
			int tmp_size = 1;
			while(tmp_size < new_height)
			{
				tmp_size *= 2;
			}
			new_height = tmp_size;
		}

#else // square
		// figure out new texture size
		new_width = __max(old_width, old_height);
		// make power of 2
		if(new_width & (new_width-1))
		{
			int tmp_size = 1;
			while(tmp_size < new_width)
			{
				tmp_size *= 2;
			}
			new_width = tmp_size;
		}
		new_height = new_width;
#endif
	}
	else
	{
		assert(new_width >= old_width);
		assert(new_height >= old_height);
	}
}

void ResizeVRGB(VRGB **rgb, const int old_width, const int old_height, int & new_width, int & new_height,
				bool calc_new_size)
{
	// sanity check
	/*
	if((old_width == old_height) && !(old_width & (old_width-1)))
	{
		new_width = new_height = old_width;
		return;
	}
	*/
	if( !(old_width & (old_width-1)) )
	{
		new_width = old_width;
		
	}

	if( !(old_height & (old_height-1)) )
	{
		new_height = old_height;
		
	}

	if(new_width == old_width && new_height == old_height)
	{
		return;
	}


	if(calc_new_size)
	{
		CalcNewSize(old_width, old_height, new_width, new_height);
	}
	else
	{
		assert(new_width >= old_width);
		assert(new_height >= old_height);
	}

	VRGB *new_rgb = (VRGB*)malloc((4 * new_width * new_height * sizeof(VRGB)) / 3);

	for(int row = 0; row < new_height; row++)
	{
		int old_row = (row * old_height) / new_height; 
		assert(old_row >= 0);
		float row_fraction;
		if(old_row == old_height - 1)
		{
			old_row--;
			row_fraction = 1.0f;
		}
		else
		{
			row_fraction = (float)(row * old_height) / (float)new_height - old_row;
		}	 
		assert(row_fraction >= 0.0f && row_fraction <= 1.0f);

		for(int column = 0; column < new_width; column++)
		{
			int old_column = (column * old_width) / new_width; 
			assert(old_column >= 0);
			float column_fraction;
			if(old_column == old_width - 1)
			{
				old_column--;
				column_fraction = 1.0f;
			}
			else
			{
				column_fraction = (float)(column * old_width) / (float)new_width - old_column;
			}
			assert(column_fraction >= 0.0f && column_fraction <= 1.0f);


			float pixel1 = (1.0f - column_fraction) * (*rgb)[old_row * old_width + old_column].r +
						   (column_fraction) * (*rgb)[old_row * old_width + old_column + 1].r;

			float pixel2 = (1.0f - column_fraction) * (*rgb)[(old_row + 1) * old_width + old_column].r +
						   (column_fraction) * (*rgb)[(old_row + 1)* old_width + old_column + 1].r;
							
			new_rgb[row * new_width + column].r =
				(unsigned char)( (1.0f - row_fraction) * pixel1 +
								 (row_fraction) * pixel2 +
								 .4999f);

				 pixel1 = (1.0f - column_fraction) * (*rgb)[old_row * old_width + old_column].g +
						   (column_fraction) * (*rgb)[old_row * old_width + old_column + 1].g;

				 pixel2 = (1.0f - column_fraction) * (*rgb)[(old_row + 1) * old_width + old_column].g +
						   (column_fraction) * (*rgb)[(old_row + 1)* old_width + old_column + 1].g;
							
			new_rgb[row * new_width + column].g =
				(unsigned char)( (1.0f - row_fraction) * pixel1 +
								 (row_fraction) * pixel2 +
								 .4999f);

				 pixel1 = (1.0f - column_fraction) * (*rgb)[old_row * old_width + old_column].b +
						   (column_fraction) * (*rgb)[old_row * old_width + old_column + 1].b;

				 pixel2 = (1.0f - column_fraction) * (*rgb)[(old_row + 1) * old_width + old_column].b +
						   (column_fraction) * (*rgb)[(old_row + 1)* old_width + old_column + 1].b;
							
			new_rgb[row * new_width + column].b =
				(unsigned char)( (1.0f - row_fraction) * pixel1 +
								 (row_fraction) * pixel2 +
								 .4999f);
		}
	}

	free(*rgb);
	*rgb = new_rgb;
}

void VRGBtoIntensity(const VRGB *rgb, unsigned char *intensity, const int count)
{
	assert(rgb && intensity && count > 0);
	for(int i = 0; i < count; i++)
	{
		intensity[i] = (unsigned char)
				(.299f * (float)rgb[i].r +
				 .587f * (float)rgb[i].g +
				 .114f * (float)rgb[i].b +
				 .4999f);
	}
}
