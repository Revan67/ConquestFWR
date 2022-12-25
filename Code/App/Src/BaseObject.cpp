//--------------------------------------------------------------------------//
//                                                                          //
//                               BaseObject.cpp                             //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /Conquest/App/Src/BaseObject.cpp 25    6/30/00 4:48p Tmauer $
*/			    
//---------------------------------------------------------------------------
#include "pch.h"
#include <globals.h>

#include "IObject.h"
#include "SuperTrans.h"
#include "UserDefaults.h"
#include "GridVector.h"
#include "Camera.h"

#include <View2D.h>
#include <stdlib.h>

extern Vector circleLines[NUM_CIRCLE_SEGS];

#define MAX_HEIGHT (480)
#define BRACKET_LENGTH 10
//--------------------------------------------------------------------------//
//
BOOL32 __stdcall RectIntersects (const RECT & rect1, const RECT & rect2)
{
	// see if top line of rect1 intersects left of right line of rect2
	if ((rect2.left  >= rect1.left && rect2.left  <= rect1.right) ||
		(rect2.right >= rect1.left && rect2.right <= rect1.right))
	{
		if (rect2.top <= rect1.top && rect2.bottom >= rect1.top)
			goto Done;
		if (rect2.top <= rect1.bottom && rect2.bottom >= rect1.bottom)
			goto Done;
	}

	// see if top line of rect2 intersects left of right line of rect1
	if ((rect1.left  >= rect2.left && rect1.left  <= rect2.right) ||
		(rect1.right >= rect2.left && rect1.right <= rect2.right))
	{
		if (rect1.top <= rect2.top && rect1.bottom >= rect2.top)
			goto Done;
		if (rect1.top <= rect2.bottom && rect1.bottom >= rect2.bottom)
			goto Done;
	}

	// see if rect1 encompasses rect2
	if (rect1.left <= rect2.left && rect1.right  >= rect2.right &&
		rect1.top  <= rect2.top  && rect1.bottom >= rect2.bottom)
	{
		goto Done;
	}
	
	// see if rect2 encompasses rect1
	if (rect2.left <= rect1.left && rect2.right  >= rect1.right &&
		rect2.top  <= rect1.top  && rect2.bottom >= rect1.bottom)
	{
		goto Done;
	}
	return 0;
Done:	
	return 1;
}
//--------------------------------------------------------------------------//
//
#pragma warning (disable : 4701)	// variable may be used before initialized
static void findFourPoints (const ViewPoint * const points, S32 numPoints, S32 & _tleft, S32 & _tright, S32 & _bleft, S32 & _bright)
{
	S32 tleft, tright, bleft, bright;
	S32 minY, maxY;
	const long max_int = 0x7FFFFFFF;
	const long min_int = 0x80000000;
	int i;

	minY = max_int;
	maxY = min_int;

	for (i = 0; i < numPoints; i++)
	{
		if (points[i].y < minY)
		{
			minY = points[i].y;
			tleft = tright = i;
		}
		else
		if (points[i].y == minY)
		{
			if (points[i].x < points[tleft].x)
				tleft = i;
			else
			if (points[i].x > points[tright].x)
				tright = i;
		}

		if (points[i].y > maxY)
		{
			maxY = points[i].y;
			bleft = bright = i;
		}
		else
		if (points[i].y == maxY)
		{
			if (points[i].x < points[bleft].x)
				bleft = i;
			else
			if (points[i].x > points[bright].x)
				bright = i;
		}
	}

	_tleft  = tleft;
	_tright = tright;
	_bleft  = bleft;
	_bright = bright;
}
//-------------------------------------------------------------------
//
static void do_segment (int _x0, int _y0, int _x1, int _y1, S32 points[MAX_HEIGHT])
{
	int dx, dy, incrE, incrNE, d, x0, x1, y0, y1;
//	byte *Ptr;
	
	x0 = _x0;	
	y0 = _y0;	
	x1 = _x1;	
	y1 = _y1;	

	dx = x1 - x0;
	dy = y1 - y0;

	if (abs(dx) > abs(dy))
	{
//		int y_inc = main_window.x_max+1;
		int y_inc = 1;

		if (dx < 0)
		{
		 	x1 = x0;
			x0 = _x1;
			y1 = y0;
			y0 = _y1;
			dx = -dx;
			dy = -dy;
		}
		
		if (dy < 0)
		{
			dy = -dy;
			y_inc = -y_inc;
		}
		
//		Ptr = main_window.buffer + x0 + (y0 * (main_window.x_max+1));
//		*Ptr = color;
		if ((U32)y0 < MAX_HEIGHT)
			points[y0] = x0;
		
		d = (dy * 2) - dx;
		incrE = dy * 2;
		incrNE = (dy - dx) * 2;
		
		while (x0 < x1)
		{
			if (d <= 0)
			{
				d += incrE;
//				Ptr++;
			}
			else
			{
				d += incrNE;
//				Ptr += 1 + y_inc;
				y0 += y_inc;
			}
			x0++;
//			*Ptr = color;
			if ((U32)y0 < MAX_HEIGHT)
				points[y0] = x0;
		}

	}
	else
	{
		int x_inc = 1;
//		const int y_inc = main_window.x_max+1;

		if (dy < 0)
		{
		 	y1 = y0;
			y0 = _y1;
			x1 = x0;
			x0 = _x1;
			dy = -dy;
			dx = -dx;
		}
		
		if (dx < 0)
		{
			dx = -dx;
			x_inc = -x_inc;
		}
		
//		Ptr = main_window.buffer + x0 + (y0 * (main_window.x_max+1));
//		*Ptr = color;
		if ((U32)y0 < MAX_HEIGHT)
			points[y0] = x0;

		d = (dx * 2) - dy;
		incrE = dx * 2;
		incrNE = (dx - dy) * 2;
		
		while (y0 < y1)
		{
			if (d <= 0)
			{
				d += incrE;
//				Ptr += y_inc;
			}
			else
			{
				d += incrNE;
//				Ptr += y_inc + x_inc;
				x0 += x_inc;
			}
			y0++;
//			*Ptr = color;
			if ((U32)y0 < MAX_HEIGHT)
				points[y0] = x0;
		}
	}
}
//--------------------------------------------------------------------------//
// check if rect intersects with polygon
//
BOOL32 __stdcall RectIntersects (const RECT & _rect, struct ViewPoint * points, S32 numPoints)
{
	S32 tleft, tright, bleft, bright;
	BOOL32 result = 0;
	S32 leftSide[MAX_HEIGHT];
	S32 rightSide[MAX_HEIGHT];
	RECT rect;
	int i,j;

	//
	// convert to ideal coordinates
	//
	if (SCREENRESX != SCREEN_WIDTH || SCREENRESY != SCREEN_HEIGHT)
	{
		rect.left   = REAL2IDEALX(_rect.left);
		rect.right  = REAL2IDEALX(_rect.right);
		rect.top    = REAL2IDEALY(_rect.top);
		rect.bottom = REAL2IDEALY(_rect.bottom);

		for (i=0; i < numPoints; i++)
		{
			points[i].x = REAL2IDEALX(points[i].x);
			points[i].y = REAL2IDEALY(points[i].y);
		}
	}
	else
		rect = _rect;
	

	findFourPoints(points, numPoints, tleft, tright, bleft, bright);

	//
	// do quick check to see if intersection is possible
	//
	if (rect.top > points[bleft].y || rect.bottom < points[tleft].y)
		goto Done;	// not possible

//	memset(leftSide, -1, sizeof(leftSide));
//	memset(rightSide, -1, sizeof(rightSide));
	//
	// draw each segment on the right side
	//
	i = tright;
	do
	{
		if ((j = i+1) >= numPoints)		// circular list
			j = 0;
		do_segment (points[i].x, points[i].y, points[j].x, points[j].y, rightSide);
		i = j;
	} while (i != bright);

	//
	// draw each segment on the left side
	//
	i = bleft;
	do
	{
		if ((j = i+1) >= numPoints)		// circular list
			j = 0;
		do_segment (points[i].x, points[i].y, points[j].x, points[j].y, leftSide);
		i = j;
	} while (i != tleft);

	//
	// now compare the strips
	//

	i = __max(rect.top, points[tleft].y);
	j = __min(rect.bottom, points[bleft].y);

	for ( ; i <= j; i++)
	{
		if (rightSide[i] >= rect.left && leftSide[i] <= rect.right)
		{
			result = 1;
			break;
		}
	}

Done:
	return result;
}
#pragma warning (default : 4701)	// variable may be used before initialized
//--------------------------------------------------------------------------//
//-------------------------------OBJECT Methods-----------------------------//
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//
const TRANSFORM & IBaseObject::GetTransform (void) const
{
	static TRANSFORM transform;

	return transform;
}
//--------------------------------------------------------------------------//
//
Vector IBaseObject::GetVelocity (void)
{
	return Vector(0,0,0);
}
//--------------------------------------------------------------------------//
//
GRIDVECTOR IBaseObject::GetGridPosition (void) const
{
	GRIDVECTOR tmp;
	tmp = GetTransform().translation;
	return tmp;
}
//--------------------------------------------------------------------------//
//
U32 IBaseObject::GetSystemID (void) const
{
	return 0;
}
//--------------------------------------------------------------------------//
//
SINGLE IBaseObject::TestHighlight (const RECT & rect)
{
	bHighlight = 0;
	return 0.0f;
}
//--------------------------------------------------------------------------//
//
void IBaseObject::DrawSelected (void)
{
}
//--------------------------------------------------------------------------//
//
void IBaseObject::DrawHighlighted (void)
{
}
//--------------------------------------------------------------------------//
//
void IBaseObject::DrawFleetMoniker (bool bAllShips)
{
}
//--------------------------------------------------------------------------//
//
void IBaseObject::SetReady(bool _bReady)
{
}
//--------------------------------------------------------------------------//
// set bVisible if possible for any part of object to appear
//
void IBaseObject::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)	
{
	bVisible = (GetSystemID() == currentSystem &&
			   (IsVisibleToPlayer(currentPlayer) ||
			     defaults.bVisibilityRulesOff ||
			     defaults.bEditorMode) );
}
//-------------------------------------------------------------------
// call the FogOfWar manager, if appropriate
//
void IBaseObject::RevealFog (const U32 currentSystem)
{
}
//-------------------------------------------------------------------
//
void IBaseObject::SetTerrainFootprint (struct ITerrainMap * terrainMap)
{
}
//-------------------------------------------------------------------
//
void IBaseObject::CastVisibleArea (void)
{
}
//-------------------------------------------------------------------
//
BOOL32 IBaseObject::Update (void)
{
	return 1;
}
//-------------------------------------------------------------------
//
void IBaseObject::PhysicalUpdate (SINGLE dt)
{
}
//-------------------------------------------------------------------
//
void IBaseObject::Render (void)
{
}
//-------------------------------------------------------------------
//
void IBaseObject::MapRender (bool bPing)
{
}
//-------------------------------------------------------------------
//
void IBaseObject::MapTerrainRender ()
{
}
//-------------------------------------------------------------------
//
void IBaseObject::View (void)
{
}
//-------------------------------------------------------------------
//
void IBaseObject::DEBUG_print (void) const
{
}
//-------------------------------------------------------------------
// return false if not supported
//
bool IBaseObject::GetObjectBox (OBJBOX & box) const
{
	memset(box, 0, sizeof(box));
	return false;
}
//-------------------------------------------------------------------
//
U32 IBaseObject::GetPartID (void) const
{
	return 0;
}
//-------------------------------------------------------------------
//
bool IBaseObject::GetMissionData (MDATA & mdata) const
{
	return false;	// not handled
}
//-------------------------------------------------------------------
//
bool IBaseObject::MatchesSomeFilter(DWORD filter)
{
	return false;
}
//-------------------------------------------------------------------
//
U32 IBaseObject::GetPlayerID (void) const
{
	return 0;
}
//-------------------------------------------------------------------
//
S32 IBaseObject::GetObjectIndex (void) const
{
	return -1;
};
//-------------------------------------------------------------------
//
void IBaseObject::drawRangeCircle(SINGLE range,COLORREF color)
{
	BATCH->set_state(RPR_BATCH,false);
	DisableTextures();
	CAMERA->SetModelView();
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	PB.Begin(PB_LINES);
	PB.Color3ub(GetRValue(color),GetGValue(color),GetBValue(color));
	Vector oldVect = (circleLines[0]*range*GRIDSIZE)+GetTransform().translation;
	for(U32 i = 1 ; i <= NUM_CIRCLE_SEGS; ++i)
	{
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		oldVect = (circleLines[i%NUM_CIRCLE_SEGS]*range*GRIDSIZE)+GetTransform().translation;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
	}
	PB.End();
}
//-------------------------------------------------------------------
//
void IBaseObject::drawPartialRangeCircle(SINGLE range,SINGLE spin, COLORREF color)
{
	BATCH->set_state(RPR_BATCH,false);
	DisableTextures();
	CAMERA->SetModelView();
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	PB.Begin(PB_LINES);
	PB.Color3ub(GetRValue(color),GetGValue(color),GetBValue(color));
	U32 startDraw = (U32)(spin*NUM_CIRCLE_SEGS);
	if(startDraw > 0xFFFFFFFF-NUM_CIRCLE_SEGS)
		startDraw = startDraw-(0xFFFFFFFF-NUM_CIRCLE_SEGS);
	Vector oldVect = (circleLines[startDraw%NUM_CIRCLE_SEGS]*range*GRIDSIZE)+GetTransform().translation;
	for(U32 i = 1 ; i <= NUM_CIRCLE_SEGS/4; ++i)
	{
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		oldVect = (circleLines[(startDraw+i)%NUM_CIRCLE_SEGS]*range*GRIDSIZE)+GetTransform().translation;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
	}
	PB.End();
}
//-------------------------------------------------------------------
//
void IBaseObject::drawCircle(Vector center,SINGLE range,COLORREF color)
{
	BATCH->set_state(RPR_BATCH,false);
	DisableTextures();
	CAMERA->SetModelView();
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	PB.Begin(PB_LINES);
	PB.Color3ub(GetRValue(color),GetGValue(color),GetBValue(color));
	Vector oldVect = (circleLines[0]*range)+center;
	for(U32 i = 1 ; i <= NUM_CIRCLE_SEGS; ++i)
	{
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
		oldVect = (circleLines[i%NUM_CIRCLE_SEGS]*range)+center;
		PB.Vertex3f(oldVect.x,oldVect.y,oldVect.z);
	}
	PB.End();
}
//-------------------------------------------------------------------
//
void IBaseObject::drawLine(Vector end1, Vector end2,COLORREF color)
{
	BATCH->set_state(RPR_BATCH,false);
	DisableTextures();
	CAMERA->SetModelView();
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	PB.Begin(PB_LINES);
	PB.Color3ub(GetRValue(color),GetGValue(color),GetBValue(color));
	PB.Vertex3f(end1.x,end1.y,end1.z);
	PB.Vertex3f(end2.x,end2.y,end2.z);
	PB.End();
}
//-------------------------------------------------------------------
//-------------------------END ObjList.cpp---------------------------
//-------------------------------------------------------------------
