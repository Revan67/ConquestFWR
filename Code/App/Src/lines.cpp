//--------------------------------------------------------------------------//
//                                                                          //
//                                Lines.cpp                                 //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Gboswood $

   $Header: /Conquest/App/Src/lines.cpp 1     9/20/98 1:01p Gboswood $

*/
//--------------------------------------------------------------------------//
#include "pch.h"
#include <globals.h>

#include <math.h>

//#define windowX SCREEN_WIDTH

static U16 *surf;
static VFX_RGB *val,*out;
static U32 windowX;

void IntensifyPixel(int x,int y,double distance)
{	
	COLORREF colorref;
	VFX_RGB *in_rgb = (VFX_RGB *) &colorref;
	
	double intensity = 1-0.45*(distance*distance);

	if (intensity < 0) 
		intensity = 0;

	//Table lookup done on an integer index; thickness 1
	colorref = PixelToColorRef(surf[x+y*windowX]);
	out->r = (U8)(val->r*intensity + in_rgb->r*(1-intensity));
	out->g = (U8)(val->g*intensity + in_rgb->g*(1-intensity));
	out->b = (U8)(val->b*intensity + in_rgb->b*(1-intensity));
	surf[x+y*windowX] = (U16)ColorRefToPixel(*((COLORREF *)out));
}

void AALine (const PANE *pane,int x0,int y0, int x1, int y1, COLORREF colorref)
{
	int X0 = x0+pane->x0;
	int X1 = x1+pane->x0;
	int Y0 = y0+pane->y0;
	int Y1 = y1+pane->y0;
	
	//if (value == 0xFFFFFFFF)
	//	value = 0x4000FFFF;

	surf = (U16 *)(pane->window->buffer);
	windowX = pane->window->x_max+1;

	VFX_RGB tmpRgb1, tmpRgb2;
	VFX_RGB *rgb = &tmpRgb1;
//  COLORREF colorref;
	VFX_RGB *in_rgb = (VFX_RGB *)&colorref;
	VFX_RGB *out_rgb;

	//colorref = PixelToColorRef(value);
	rgb->r = in_rgb->r;
	rgb->g = in_rgb->g;
	rgb->b = in_rgb->b;

	val = rgb;

	out_rgb = &tmpRgb2;
	out = out_rgb;

	int dx = X1-X0;
	int dy = Y1-Y0;
	int d; // = 2*dy - dx;										//initial value dstart
	int incrE;// = 2*dy;										//increment used for move to E
	int incrNE;// = 2*(dy-dx);									//increment used for move to NE
	int two_v_dx;											//Numerator; v=0 for start pixel
	double invDenom = 1.0 / (2.0*sqrt((double)(dx*dx + dy*dy)));		//Precomputed inverse denominator
	double two_dx_invDenom;// = 2.0*dx*invDenom;				//Precomputed constant

	int x = X0;
	int y = Y0;
	
	int incX = (dx > 0) ? 1 : -1;
	int incY = (dy > 0) ? 1 : -1;
	
	if (dx*incX > dy*incY)
	{
		d = 2*dy*incY - dx*incY;
		incrE = 2*dy*incY;
		incrNE = 2*(dy*incY-dx*incX);
		two_dx_invDenom = 2.0*dx*invDenom*incX;
		if ((x >= pane->x0 && x <= pane->x1) && (y >= pane->y0 && y <= pane->y1))
		{
			IntensifyPixel(x,y,0);								/* Start pixel */
			IntensifyPixel(x,y+incY,two_dx_invDenom);				/* Neighbor */
			IntensifyPixel(x,y-incY,two_dx_invDenom);				/* Neighbor */
		}
		while (x != X1) {
			if (d<0) 
			{										/* Choose E */
				two_v_dx = d+dx*incX;
				d += incrE;
				x += incX;
			}
			else
			{										//Choose NE
				two_v_dx = d-dx*incX;
				d += incrNE;
				x += incX;
				y += incY;
			}	
			if ((x >= pane->x0 && x <= pane->x1) && (y >= pane->y0 && y <= pane->y1))
			{
			/* Now set chosen pixel and it's neighbors */
			IntensifyPixel(x,y,two_v_dx*invDenom);
			IntensifyPixel(x,y+incY,two_dx_invDenom - two_v_dx*invDenom);
			IntensifyPixel(x,y-incY,two_dx_invDenom + two_v_dx*invDenom);
			}
		}
	}
	else
	{
		d = 2*dx*incX - dy*incY;
		incrE = 2*dx*incX;
		incrNE = 2*(dx*incX-dy*incY);
		two_dx_invDenom = 2.0*dy*invDenom*incY;
		if ((x >= pane->x0 && x <= pane->x1) && (y >= pane->y0 && y <= pane->y1))
		{
			IntensifyPixel(x,y,0);								/* Start pixel */
			IntensifyPixel(x+incX,y,two_dx_invDenom);				/* Neighbor */
			IntensifyPixel(x-incX,y,two_dx_invDenom);				/* Neighbor */
		}
		while (y != Y1)
		{
			if (d<0) 
			{										/* Choose E */
				two_v_dx = d+dy*incY;
				d += incrE;
				y += incY;
			}
			else
			{										//Choose NE
				two_v_dx = d-dy*incY;
				d += incrNE;
				x += incX;
				y += incY;
			}	
			if ((x >= pane->x0 && x <= pane->x1) && (y >= pane->y0 && y <= pane->y1))
			{
				/* Now set chosen pixel and it's neighbors */
				IntensifyPixel(x,y,two_v_dx*invDenom);
				IntensifyPixel(x+incX,y,two_dx_invDenom - two_v_dx*invDenom);
				IntensifyPixel(x-incX,y,two_dx_invDenom + two_v_dx*invDenom);
			}
		}
	}
}
/*
void AALine(U16 *surface,int x0,int y0, int x1, int y1, U32 value)
{
	int dy, dx, incrE, incrNE, d,x,y;
	int incX,incY;
	VFX_RGB *rgb = new VFX_RGB;
	VFX_RGB *in_rgb,*out_rgb;

	in_rgb = VFX_color_to_RGB(value);
	rgb->r = in_rgb->r;
	rgb->g = in_rgb->g;
	rgb->b = in_rgb->b;

	out_rgb = new VFX_RGB;

	dx = x1 - x0;
	dy = y1 - y0;
	incX = (dx > 0) ? 1 : -1;
	incY = (dy > 0) ? 1 : -1;
	x = x0;
	y = y0;
    
	surface[x+y*windowX] = value;

	if (dx*incX > dy*incY)
	{	
		//slope > 1
		d = (2 * dy*incY - dx*incX);
		incrE = 2*dy*incY;
		incrNE = 2*(dy*incY - dx*incX);

		while(x != x1)
		{
			if (d <= 0)
			{
				d += incrE;
				x += incX;
			}
			else
			{
				d += incrNE;
				x += incX;
				y += incY;
			}
			surface[x+y*windowX] = 0xFFFF & VFX_pixel_value(rgb);
		}
	}
	else
	{
		//slope < 1
		d = (2 * dx*incX - dy*incY);
		incrE = 2*dx*incX;
		incrNE = 2*(dx*incX - dy*incY);

		while(y != y1)
		{
			if (d <= 0)
			{
				d += incrE;
				y += incY;
			}
			else
		    {
				d += incrNE;
				y += incY;
				x += incX;
			}
			in_rgb = VFX_color_to_RGB(0x80000000 | surface[x+y*windowX-1]);
			if (incX < 0)
			{
				out_rgb->r = rgb->r*(1-(SINGLE)d*incY/incrNE);// + in_rgb->r*(1-fabs((SINGLE)d/incrNE));
				out_rgb->g = rgb->g*(1-(SINGLE)d*incY/incrNE);// + in_rgb->g*(1-fabs((SINGLE)d/incrNE));
				out_rgb->b = rgb->b*(1-(SINGLE)d*incY/incrNE);// + in_rgb->b*(1-fabs((SINGLE)d/incrNE));
			}
			else
			{
				out_rgb->r = rgb->r*((SINGLE)d*incY/incrNE);// + in_rgb->r*(fabs((SINGLE)d/incrNE));
				out_rgb->g = rgb->g*((SINGLE)d*incY/incrNE);// + in_rgb->g*(fabs((SINGLE)d/incrNE));
				out_rgb->b = rgb->b*((SINGLE)d*incY/incrNE);// + in_rgb->b*(fabs((SINGLE)d/incrNE));
			}
			surface[x+y*windowX-1] = 0xFFFF & VFX_pixel_value(out_rgb);
			surface[x+y*windowX] = 0xFFFF & VFX_pixel_value(rgb);
	//		surface[x+y*windowX+1] = 0xFFFF & VFX_pixel_value(rgb);
		//	surface[x+y*windowX+2] = surface[x+y*windowX+2]*((SINGLE)d/incrNE) + value*(1-(SINGLE)d/incrNE);
			in_rgb = VFX_color_to_RGB(0x80000000 | surface[x+y*windowX+2]);
			if (incX < 0)
			{
				out_rgb->r = rgb->r*((SINGLE)d*incY/incrNE);// + in_rgb->r*(fabs((SINGLE)d/incrNE));
				out_rgb->g = rgb->g*((SINGLE)d*incY/incrNE);// + in_rgb->g*(fabs((SINGLE)d/incrNE));
				out_rgb->b = rgb->b*((SINGLE)d*incY/incrNE);// + in_rgb->b*(fabs((SINGLE)d/incrNE));
			}
			else
			{
				out_rgb->r = rgb->r*(1-(SINGLE)d*incY/incrNE);// + in_rgb->r*(1-fabs((SINGLE)d/incrNE));
				out_rgb->g = rgb->g*(1-(SINGLE)d*incY/incrNE);// + in_rgb->g*(1-fabs((SINGLE)d/incrNE));
				out_rgb->b = rgb->b*(1-(SINGLE)d*incY/incrNE);// + in_rgb->b*(1-fabs((SINGLE)d/incrNE));
			}
			surface[x+y*windowX+1] = 0xFFFF & VFX_pixel_value(out_rgb);
		}

	}
	delete out_rgb;
	delete rgb;
}*/
//--------------------------------------------------------------------------//
//-------------------------------End Lines.cpp------------------------------//
//--------------------------------------------------------------------------//
