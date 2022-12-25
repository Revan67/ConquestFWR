#ifndef TOBJSELECT
#define TOBJSELECT
//--------------------------------------------------------------------------//
//                                                                          //
//                               TObjSelect.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjSelect.h 91    10/26/00 6:34p Jasony $
*/			    
//---------------------------------------------------------------------------

#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef _3DMATH_H
#include "3DMath.h"
#endif

#ifndef CAMERA_H
#include "Camera.h"
#endif

#ifndef SUPERTRANS_H
#include "SuperTrans.h"
#endif

#ifndef ENGINE_H
#include <engine.h>
#endif

#ifndef RENDERER_H
#include <renderer.h>
#endif

#ifndef ARCHHOLDER_H
#include "ArchHolder.h"
#endif

#ifndef DRAWAGENT_H
#include "DrawAgent.h"
#endif

#ifndef USERDEFAULTS_H
#include "UserDefaults.h"
#endif

#ifndef MGLOBALS_H
#include "MGlobals.h"
#endif

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

#ifndef _INC_STDIO
#include <stdio.h>
#endif

#ifndef HKEVENT_H
#include <HKEvent.h>
#endif

#ifndef DBHOTKEYS_H
#include "dbhotkeys.h"
#endif

#ifndef TSMARTPOINTER_H
#include <TSmartPointer.h>
#endif

#ifndef GENDATA_H
#include "GenData.h"
#endif

#ifndef VIEW2D_H
#include <View2D.h>
#endif

#ifndef ICAMERA_H
#include <ICamera.h>
#endif

#ifndef WINDOWMANAGER_H
#include <WindowManager.h>
#endif

#ifndef IUNBORNMESHLIST_H
#include "IUnbornMeshList.h"
#endif

#ifndef ILAUNCHER_H
#include "ILauncher.h"
#endif

#ifndef MESHRENDER_H
#include "MeshRender.h"
#endif

#define ObjectSelection _Cos

template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectSelection : public Base
{
	typename typedef Base::INITINFO SELECTINITINFO;

	struct InitNode initNode;
	//
	// corner data
	//
	OBJBOX box;	// maxx,minx,maxy,miny,maxz,minz  in object coordinates
	SINGLE boxRadius;		// largest of box coordinates

	//this in only for mimic	
	U32 aliasArchetypeID;
	U8 aliasPlayerID;

	ObjectSelection (void);

	~ObjectSelection (void);

	/* IBaseObject methods */

	virtual SINGLE TestHighlight (const RECT & rect);

	virtual void DrawSelected (void);

	virtual void DrawHighlighted (void);

	virtual bool GetObjectBox (OBJBOX & _box) const
	{
		memcpy(_box, box, sizeof(box));
		return true;
	}

	void initSelection (const SELECTINITINFO & data)
	{
		ENGINE->update_instance(instanceIndex,0,0);
		ComputeCorners(box, instanceIndex);
		boxRadius = __max(box[0], -box[1]);
		boxRadius = __max(boxRadius, box[2]);
		boxRadius = __max(boxRadius, -box[3]);
	}

	static void getPadPoints (VFX_POINT points[4], const TRANSFORM & transform, const OBJBOX & _box);

#include "corners.h"

	bool get_pbs(					float & cx,
														float & cy,
														float & radius,
														float & depth);
private:
	bool getStats (SINGLE & hull, SINGLE & supplies, U32 & hullMax, U32 & suppliesMax) const;


};

//-------------------------------------------------------------------
//
template <class Base> 
ObjectSelection< Base >::ObjectSelection (void) : initNode(this, InitProc(&ObjectSelection::initSelection))
{
	aliasArchetypeID = -1;
}
//-------------------------------------------------------------------
//
template <class Base> 
ObjectSelection< Base >::~ObjectSelection (void) 
{
}
//-------------------------------------------------------------------
// set bHighlight if possible for any part of object to appear within rect
//
template <class Base> 
SINGLE ObjectSelection< Base >::TestHighlight (const RECT & rect)
{
	SINGLE closeness = 999999.0f;

	bHighlight = 0;
	if (bVisible)
	{
		if ((instanceIndex != INVALID_INSTANCE_INDEX) != 0)
		{
			float depth=0, center_x=0, center_y=0, radius=0;

			if ((bVisible = get_pbs(center_x, center_y, radius, depth)) != 0)
			{
				RECT _rect;

				_rect.left  = center_x - radius;
				_rect.right	= center_x + radius;
				_rect.top = center_y - radius;
				_rect.bottom = center_y + radius;

				RECT screenRect = { 0, 0, SCREENRESX, SCREENRESY };

				if ((bVisible = RectIntersects(_rect, screenRect)) != 0)
				{
					if(BUILDARCHEID == 0)		// no highlighting in buildmode
					{
						if (RectIntersects(rect, _rect))
						{
							ViewPoint points[64];
							int numVerts = sizeof(points) / sizeof(ViewPoint);
							INSTANCE_INDEX id = instanceIndex;
							if (aliasArchetypeID != INVALID_INSTANCE_INDEX && bSpecialRender)
							{
								MeshChain *mc;
								mc = UNBORNMANAGER->GetMeshChain(transform,aliasArchetypeID);
								id = mc->mi[0]->instanceIndex;
							}
							if (REND->get_instance_projected_bounding_polygon(id, MAINCAM, LODPERCENT, numVerts, points, numVerts, depth))
							{
								bHighlight = RectIntersects(rect, points, numVerts);

								if (rect.left == rect.right && rect.top == rect.bottom)
									closeness = fabs(rect.left - center_x) * fabs(rect.top - center_y);
								else
									closeness = 0.0f;
							}
						}
					}
				}
			}
		}
	}
	
	if(bVisible)
	{
		if(objClass == OC_SPACESHIP || objClass == OC_PLATFORM)//right now I only want ships and platforms to count toward the metric
			OBJLIST->IncrementShipsToRender();
	}
	return closeness;
}
//-------------------------------------------------------------------
//
#define L1 box[3]	//-1000
#define L2 box[2]	// 1000
#define W1 box[1]	//-500
#define W2 box[0]	// 500
#define H1 box[5]
#define H2 box[4]
template <class Base> 
void ObjectSelection< Base >::DrawSelected (void)
{
	if (bVisible==0 || instanceIndex == INVALID_INSTANCE_INDEX)
		return;

	BATCH->set_state(RPR_BATCH, TRUE);
	CAMERA->SetModelView(&transform);

	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	DisableTextures();		
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	const COLORREF color = COLORTABLE[MGlobals::GetColorID(playerID)];
	PB.Color4ub(GetRValue(color)/2,GetGValue(color)/2,GetBValue(color)/2, 180);		// player color
	
	const SINGLE TWIDTH  = __min((0.1*(L2-L1)) , 200.0);

	PB.Begin(PB_TRIANGLES);

#define _J 1
#if _J
	PB.Vertex3f(W1, L1, 0 );
	PB.Vertex3f(W1+TWIDTH, L1, 0);
	PB.Vertex3f(W1, L1+TWIDTH, 0);
#endif
	PB.Vertex3f(W1,L1 , 0);
	PB.Vertex3f(W1, L1,TWIDTH/2 );
	PB.Vertex3f(W1, L1+TWIDTH,0 );

	PB.Vertex3f(W1, L1,0 );
	PB.Vertex3f(W1+TWIDTH, L1,0 );
	PB.Vertex3f(W1, L1,TWIDTH/2 );
	//-------------------------

#if _J
	PB.Vertex3f(W2-TWIDTH, L1,0 );
	PB.Vertex3f(W2, L1,0 );
	PB.Vertex3f(W2, L1+TWIDTH,0 );
#endif

	PB.Vertex3f(W2-TWIDTH, L1,0 );
	PB.Vertex3f(W2, L1, 0);
	PB.Vertex3f(W2, L1, TWIDTH/2);
	
	PB.Vertex3f(W2, L1, 0);
	PB.Vertex3f(W2, L1, TWIDTH/2);
	PB.Vertex3f(W2, L1+TWIDTH, 0 );

	//-------------------------

#if _J
	PB.Vertex3f(W1, L2-TWIDTH,0 );
	PB.Vertex3f(W1+TWIDTH, L2,0 );
	PB.Vertex3f(W1, L2, 0);
#endif

	PB.Vertex3f(W1, L2-TWIDTH, 0);
	PB.Vertex3f(W1, L2, TWIDTH/2);
	PB.Vertex3f(W1, L2, 0);

	PB.Vertex3f(W1, L2, TWIDTH/2);
	PB.Vertex3f(W1+TWIDTH, L2,0 );
	PB.Vertex3f(W1, L2, 0);

	//-------------------------

#if _J
	PB.Vertex3f(W2, L2-TWIDTH, 0);
	PB.Vertex3f(W2, L2, 0);
	PB.Vertex3f(W2-TWIDTH, L2, 0);
#endif

	PB.Vertex3f(W2, L2-TWIDTH, 0);
	PB.Vertex3f(W2, L2, 0);
	PB.Vertex3f(W2, L2, TWIDTH/2);

	PB.Vertex3f(W2, L2,TWIDTH/2 );
	PB.Vertex3f(W2, L2,0 );
	PB.Vertex3f(W2-TWIDTH,L2, 0 );

	PB.End(); 	// end of GL_QUADS
}
//-------------------------------------------------------------------
//
template <class Base> 
void ObjectSelection< Base >::DrawHighlighted (void)
{
	if (bVisible==0)
		return;
	const USER_DEFAULTS * const pDefaults = DEFAULTS->GetDefaults();

	SINGLE hull, supplies;
	U32 hullMax, suppliesMax;

	if (getStats(hull, supplies, hullMax, suppliesMax))
	{
		Vector point;
		S32 x, y;

		int TBARLENGTH = 100;
		if (hullMax < 1000)
		{
			if (hullMax < 100)
			{
				if (hullMax > 0)
					TBARLENGTH = 20;
				else
				{
					// no hull points, length should be decided by supplies
					// use same length as max supplies
				}
			}
			else // hullMax >= 100
			{
				TBARLENGTH = 20 + (((hullMax - 100)*80) / (1000-100));
			}
		}
		TBARLENGTH = IDEAL2REALX(TBARLENGTH);

		// want the bar length to match up with a little rectangle square
		if (TBARLENGTH%5)
		{
			TBARLENGTH -= TBARLENGTH%5;
		}


		point.x = 0;
		point.y = H2+250.0;
		point.z = 0;

		CAMERA->PointToScreen(point, &x, &y, &transform);
		PANE * pane = CAMERA->GetPane();

		if (hull >= 0.0f)
		{
			COLORREF color;

			// draw the green (health) bar
			// colors  (0,130,0) (227,227,34) (224, 51, 37)

			// choose the color

			if (hull > 0.667F)
				color = RGB(0,130,0);
			else
			if (hull > 0.5F)
			{
				SINGLE diff = (0.667F - hull) / (0.667F - 0.5F);
				U8 r, g, b;
				r = (U8) ((227 - 0) * diff) + 0;
				g = (U8) ((227 - 130) * diff) + 130;
				b = (U8) ((34 - 0) * diff) + 0;
				color = RGB(r,g,b);
			}
			else
			if (hull > 0.25F)
			{
				SINGLE diff = (0.5F - hull) / (0.5F - 0.25F);
				U8 r, g, b;
				r = (U8) ((224 - 227) * diff) + 227;
				g = (U8) ((51 - 227) * diff) + 227;
				b = (U8) ((37 - 34) * diff) + 34;
				color = RGB(r,g,b);
			}
			else
			{
				color = RGB(224,51,37);
			}

			// done choosing the color
			
			DA::RectangleHash(pane, x-(TBARLENGTH/2), y, x+(TBARLENGTH/2), y+2, RGB(128,128,128));
//			DA::RectangleFill(pane, x-(TBARLENGTH/2), y, x-(TBARLENGTH/2)+S32(TBARLENGTH*hull), y+2, color);

			int xpos = x-(TBARLENGTH/2);
			int max = S32(TBARLENGTH*hull);
			int xrc;

			// make sure at least one bar gets displayed for one health point
			if (max == 0 && hull > 0.0f)
			{
				max = 1;
			}
			
			for (int i = 0; i < max; i+=5)
			{
				xrc = xpos + i;
				DA::RectangleFill(pane, xrc, y, xrc+3, y+2, color);
			}
		}
		if (supplies >= 0.0f)
		{
			//
			// draw the blue (supplies) bar RGB(0,128,225) or blue grey if locked
			//
			if ((pDefaults->bCheatsEnabled && DBHOTKEY->GetHotkeyState(IDH_HIGHLIGHT_ALL)) || pDefaults->bEditorMode || playerID == 0 || MGlobals::AreAllies(playerID, MGlobals::GetThisPlayer()))
			{
				DA::RectangleHash(pane, x-(TBARLENGTH/2), y+5, x+(TBARLENGTH/2), y+5+2, RGB(128,128,128));
				if (supplies > 0.0f)
				{
//					DA::RectangleFill(pane, x-(TBARLENGTH/2), y+5, x-(TBARLENGTH/2)+S32(TBARLENGTH*supplies), y+5+2, RGB(0,128,255));
					COLORREF color;
					if(fieldFlags.suppliesLocked())
						color = RGB(180,180,240);
					else
						color = RGB(0,128,240);

					int xpos = x-(TBARLENGTH/2);
					int max = S32(TBARLENGTH*supplies);
					int xrc;

					// make sure at least one bar gets displayed for one supply point
					if (max == 0 && supplies > 0.0f)
					{
						max = 1;
					}
					
					for (int i = 0; i < max; i+=5)
					{
						xrc = xpos + i;
						DA::RectangleFill(pane, xrc, y+5, xrc+3, y+7, color);
					}
				}
			}
			else	// don't know supply info
			{
			}
		}

		if (nextHighlighted==0 && OBJLIST->GetHighlightedList()==this)
		{
			COMPTR<IFontDrawAgent> pFont;
			if (OBJLIST->GetUnitFont(pFont) == GR_OK)
			{
				if (bShowPartName)
					pFont->SetFontColor(RGB(140,140,180) | 0xFF000000, 0);
				else
					pFont->SetFontColor(RGB(180,180,180) | 0xFF000000, 0);
				wchar_t temp[M_MAX_STRING];
				WM->GetCursorPos(x, y);
				y += IDEAL2REALY(24);
#ifdef _DEBUG
				_localAnsiToWide(partName, temp, sizeof(temp));
				pFont->StringDraw(pane, x, y, temp);
#else
				if (bShowPartName)
				{
					_localAnsiToWide(partName, temp, sizeof(temp));
				}
				else
				{
					wchar_t * ptr;
					wcsncpy(temp, _localLoadStringW(pInitData->displayName), sizeof(temp)/sizeof(wchar_t));

					if (bMimic)
					{
						VOLPTR(ILaunchOwner) launchOwner=static_cast<IBaseObject *>(this);
						CQASSERT(launchOwner);
						OBJPTR<ILauncher> launcher;
						launchOwner->GetLauncher(2,launcher);
						if (launcher)
						{
							VOLPTR(IMimic) mimic=launcher.ptr;
							// if we are enemies
							U32 allyMask=MGlobals::GetAllyMask(MGlobals::GetThisPlayer());
							if (mimic->IsDiscoveredTo(allyMask)==0)
							{
								wcsncpy(temp, _localLoadStringW(IDS_MIMICKED_SHIP), sizeof(temp)/sizeof(wchar_t));
							}
						}
					}

					if ((ptr = wcschr(temp, '#')) != 0)
					{
						*ptr = 0;
					}
					if ((ptr = wcschr(temp, '(')) != 0)
					{
						*ptr = 0;
					}
				}
				pFont->StringDraw(pane, x, y, temp);
#endif
			}
		}
	}
}
//-------------------------------------------------------------------
//
// hull,supplies in range 0 to 1.0
//
template <class Base> 
bool ObjectSelection< Base >::getStats (SINGLE & hull, SINGLE & supplies, U32 & hullMax, U32 & suppliesMax) const
{
	hull = -1.0f;
	supplies = -1.0F;

	if ((hullMax = hullPointsMax) != 0)
		hull = SINGLE(GetDisplayHullPoints()) / SINGLE(hullPointsMax);

	if ((suppliesMax = supplyPointsMax) != 0)
		supplies = SINGLE(GetDisplaySupplies()) / SINGLE(supplyPointsMax);

	return (hull >= 0 || supplies >= 0);
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectSelection< Base >::getPadPoints (VFX_POINT points[4], const TRANSFORM & transform, const OBJBOX & _box)
{
	//
	// calculate points (in clock-wise order) for terrain foot-pad system
	//
	Vector pt, wpt;

	pt.y = 0;
	pt.x = _box[0];	// maxx
	pt.z = _box[5];	// minz

	wpt = transform.rotate_translate(pt);
	points[0].x = wpt.x;
	points[0].y = wpt.y;

	pt.z = _box[4];	// maxz

	wpt = transform.rotate_translate(pt);
	points[1].x = wpt.x;
	points[1].y = wpt.y;

	pt.x = _box[1];	// minx

	wpt = transform.rotate_translate(pt);
	points[2].x = wpt.x;
	points[2].y = wpt.y;

	pt.z = _box[5];	// minz

	wpt = transform.rotate_translate(pt);
	points[3].x = wpt.x;
	points[3].y = wpt.y;
}
//-------------------------------------------------------------------
//
template <class Base> 
bool ObjectSelection< Base >::get_pbs(float & cx,
														float & cy,
														float & radius,
														float & depth)
{
	bool result = false;
	CQASSERT(instanceIndex != INVALID_INSTANCE_INDEX);
	float obj_rad;
	Vector wcenter(0,0,0);
	const Transform *cam2world = CAMERA->GetTransform();
	if (aliasArchetypeID != INVALID_INSTANCE_INDEX)
		UNBORNMANAGER->GetBoundingSphere(transform,aliasArchetypeID,obj_rad,wcenter);
	else
		ENGINE->get_instance_bounding_sphere(instanceIndex,0,&obj_rad,&wcenter);

	if (obj_rad > 20000)
	{
		CQTRACE11("%s moved without update",(char *)partName);
		obj_rad = 3000;
	}

	Vector vcenter = cam2world->inverse_rotate_translate(transform*wcenter);
				
	// Make sure object is in front of near plane.
	if (vcenter.z < -MAINCAM->get_znear())
	{
		const struct ViewRect * pane = MAINCAM->get_pane();
		
		float x_screen_center = float(pane->x1 - pane->x0) * 0.5f;
		float y_screen_center = float(pane->y1 - pane->y0) * 0.5f;
		float screen_center_x = pane->x0 + x_screen_center;
		float screen_center_y = pane->y0 + y_screen_center;
		
		float w = -1.0 / vcenter.z;
		float sphere_center_x = vcenter.x * w;
		float sphere_center_y = vcenter.y * w;
		
		cx = screen_center_x + sphere_center_x * MAINCAM->get_hpc()*MAINCAM->get_znear();
		cy = screen_center_y + sphere_center_y * MAINCAM->get_vpc()*MAINCAM->get_znear();
		
		float center_distance = vcenter.magnitude();
		
		if(center_distance >= obj_rad)
		{
			float dx = fabs(cx - screen_center_x);
			float dy = fabs(cy - screen_center_y);
			
			//changes 1/26 - rmarr
			//function should now not return TRUE with obscene radii
			float outer_angle = asin(obj_rad / center_distance);
			sphere_center_x = fabs(sphere_center_x);
			float inner_angle = atan(sphere_center_x);
			
			//	float near_plane_radius = tan(inner_angle + outer_angle);
			//	near_plane_radius -= sphere_center_x;
			//	radius = near_plane_radius * camera->get_hpc();
			
			float near_plane_radius = tan(inner_angle - outer_angle);
			near_plane_radius = sphere_center_x-near_plane_radius;
			radius = near_plane_radius * MAINCAM->get_hpc()*MAINCAM->get_znear();
			
			int view_w = (pane->x1 - pane->x0 + 1) >> 1;
			int view_h = (pane->y1 - pane->y0 + 1) >> 1;
			
			if ((dx < (view_w + radius)) && (dy < (view_h + radius)))
			{
				depth = -vcenter.z;
				result = true;
			}
		}
	}
				
	return result;
}
//-------------------------------------------------------------------
//------------------------END TObjSelect.h---------------------------
//-------------------------------------------------------------------
#endif