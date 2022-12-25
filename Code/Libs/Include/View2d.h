//---------------------------------------------------------------------------
//
// View2D.H - General type definitions for 2D view geometry
// 
//---------------------------------------------------------------------------

#ifndef VIEW2D_H
#define VIEW2D_H

//

#include "typedefs.h"

//

// NOTE: These used to be VFX_RECT and VFX_POINT, respectively. 

//
// ViewRect
//
// x0,y0 - upper left corner of the rectangle.
// x1,y1 - lower right corner of the rectangle.
//
// NOTE: (x1,y1) is the last point included in the rectangle.
// i.e. width(ViewRect) == (x1-x0)+1 (and similar for height).  
// This is different than the standard Win32 RECT structure,
// which is not inclusive.
//
struct ViewRect
{
	S32 x0;
	S32 y0;
	S32 x1;
	S32 y1;
}; 

// 
// ViewPoint
//
// x,y - the point.
//
struct ViewPoint
{
   S32 x;
   S32 y;
};

//

#endif // EOF
