#ifndef RGBUTILS_H_
#define RGBUTILS_H_

//****************************************************************************
//*                                                                          *
//* RGBUTILS.H: RGB color manipulation library                               *
//*                                                                          *
//* 32-bit protected-mode source compatible with MSVC 10.2                   *
//*                                                                          *
//* Version 1.00 of 28-Jan-97: Initial, derived from IMAGEMAN RGBUTILS.H     *
//*                                                                          *
//* Author: John Miles                                                       *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//* Copyright (C) 1997 Miles Design, Inc.                                    *
//*                                                                          *
//****************************************************************************

#ifdef SGI
#ifndef bool
typedef unsigned char bool;
#endif
#endif

#ifndef true
#define true ((bool)1)
#endif

#ifndef false
#define false ((bool)0)
#endif

struct VFX_RGB
{
	unsigned char r;     // 8 bits per component
	unsigned char g;
	unsigned char b;
};

struct RGB_BOX
{
    long r0;    // min value, exclusive
    long r1;    // max value, inclusive

    long g0;  
    long g1;

    long b0;  
    long b1;

    long vol;
};

class CMAP
{
   short       *scoreboard;
   const VFX_RGB* palette;
   const unsigned long colors;

public:

	//
	// Color remapping functions
	//

	CMAP (const VFX_RGB* palette, unsigned long colors);
	~CMAP (void);

	unsigned char nearest_neighbor (const VFX_RGB& triplet);

	// just to avoid compiler warning
	CMAP & operator = (const CMAP & cm) { *this = cm; return *this;}
};

struct CQT
{
	long t[33][33][33];
};

class CQ
{
	CQT wt;
	CQT mr;
	CQT mg;
	CQT mb;

	float* m2;

	float Var (const RGB_BOX& cube) const;
	void M3d (long* vwt, long* vmr, long* vmg, long* vmb, float* m2) const;
	long Vol (const RGB_BOX& cube, const CQT& mmt) const;
	long Bottom (const RGB_BOX& cube, unsigned char dir, const CQT& mmt) const;
	long Top (const RGB_BOX& cube, unsigned char dir, long pos, const CQT& mmt) const;

	float Maximize (const RGB_BOX& cube,
					unsigned char dir, 
					long first, 
					long last, 
					long *cut,
					long whole_r, 
					long whole_g, 
					long whole_b, 
					long whole_w) const;

	long Cut (RGB_BOX *set1, RGB_BOX *set2) const;

	public:

	CQ (void);
	~CQ (void);

	//
	// Color quantization functions
	//

	void  reset     (void);
	void  add_color (const VFX_RGB& triplet);
	unsigned long   quantize  (VFX_RGB *out, unsigned long colors);
};

struct VRGB : public VFX_RGB
{
      bool operator == (const VRGB * ptr) const
      {
      return ((r == ptr->r) && (g == ptr->g) && (b == ptr->b));
      }

      bool operator == (const VRGB& ptr) const
      {
      return ((r == ptr.r) && (g == ptr.g) && (b == ptr.b));
      }
};

void ResizeVRGB(VRGB **rgb, const int old_width, const int old_height, int & new_width, int & new_height,
				bool calc_new_size);
void VRGBtoIntensity(const VRGB *rgb, unsigned char *intensity, const int count);

#endif
