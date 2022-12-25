//--------------------------------------------------------------------------//
//                                                                          //
//                               Nugget.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Nugget.cpp 125   10/23/00 11:27a Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TComponent.h"
#include "TObjFrame.h"
#include "TObject.h"
#include "TObjTrans.h"
#include "TObjMission.h"
#include "ObjList.h"
#include "Camera.h"
#include "ICamera.h"
#include "SuperTrans.h"
#include "UserDefaults.h"
#include "Mission.h"
#include "IMissionActor.h"
#include "MGlobals.h"
#include "Startup.h"
#include "DrawAgent.h"
#include "TerrainMap.h"
#include "Anim2d.h"
#include "INugget.h"
#include "GenData.h"
#include <DNugget.h>
#include "MPart.h"
#include "OpAgent.h"
#include "NetVector.h"
#include "CQBatch.h"
#include "DQuickSave.h"
#include "ObjMap.h"
#include "TObjPhys.h"
#include "SysMap.h"
#include <ITextureLibrary.h>

#include <WindowManager.h>
#include <Engine.h>
#include <TSmartPointer.h>
#include <Physics.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>
#include <EventSys.h>
#include <stdio.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
struct NUGGET_INIT
{
	const BT_NUGGET_DATA * pData;
	PARCHETYPE pArchetype;
	AnimArchetype *animArch;

	Vector rigidBodyArm;
	S32 archIndex;
};

inline MISSION_DATA::M_CAPS & MISSION_DATA::M_CAPS::operator |= (const MISSION_DATA::M_CAPS & other)
{
	CQASSERT(sizeof(*this) == 4);
	U32 * ptr = (U32 *) this;
	*ptr |= *((U32 *) &other);

	return *this;
}

#define TOLERANCE 0.00001f

struct _NO_VTABLE AnimNugget : IBaseObject, INugget, IPhysicalObject, BASE_ANIMNUGGET_SAVELOAD
{
	BEGIN_MAP_INBOUND(AnimNugget)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(INugget)
	_INTERFACE_ENTRY(IPhysicalObject)
	END_MAP()

	AnimNugget * nextNugget;

	SINGLE t;
	SINGLE pulse;
	S32 myMaxAnimSize;
	SINGLE sinA,cosA;
	bool pulseUp;

	U16 trueNetSupplies;

	int map_square;
	U32 mapSysID;

	SINGLE invisibleTimer;

	AnimNugget (void);

	virtual ~AnimNugget (void);	// See ObjList.cpp

	virtual SINGLE TestHighlight (const RECT &rect);

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	};

	const TRANSFORM & GetTransform (void) const;

	virtual void GameplayTestVisible(const U32 currentPlayer);

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual void CastVisibleArea (void);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual void MapRender (bool bPing);

	virtual void DrawHighlighted (void);

	virtual void DrawSelected (void);

	virtual U32 GetPartID (void) const
	{
		return dwMissionID;
	};

	virtual void UpdateVisibilityFlags (void);

	virtual void UpdateVisibilityFlags2 (void);

	virtual bool IsTargetableByPlayer (U32 _playerID);

	/* INugget methods */

	virtual void InitNugget(U32 partID, U32 _systemID, const Vector &destPos,S32 scrapValue, SINGLE lifeTime, bool bRealized);

	virtual void SetDepleted (bool depSetting);

	virtual void IncHarvestCount ();

	virtual void DecHarvestCount ();

	virtual U8 GetHarvestCount ();

	virtual void SetProcessID (U32 newProcID);

	virtual U32 GetProcessID ();

	virtual M_RESOURCE_TYPE GetResourceType();

	virtual enum M_NUGGET_TYPE GetNuggetType();

	virtual U32 GetSupplies();

	virtual U32 GetMaxSupplies();

	virtual void SetSupplies(U32 newSupplies);

	// IPhysicalObject

	virtual void SetSystemID (U32 newSystemID);

	virtual void SetPosition (const Vector & position, U32 newSystemID);

	virtual void SetTransform (const TRANSFORM & transform, U32 newSystemID);

	virtual void SetVelocity (const Vector & velocity)
	{
	}

	virtual void SetAngVelocity (const Vector & angVelocity)
	{
	}

};
//---------------------------------------------------------------------------
//
AnimNugget::AnimNugget (void)
{
}
//---------------------------------------------------------------------------
//
AnimNugget::~AnimNugget (void)
{
	if(bHighlight)
		OBJLIST->FlushHighlightedList();
	OBJLIST->RemovePartID(this, dwMissionID);
	OBJMAP->RemoveObjectFromMap(this,mapSysID,map_square);
//	if (CQFLAGS.bTraceMission)
//		CQTRACE11("Nugget Deleted : 0x%X",dwMissionID);
}
//---------------------------------------------------------------------------
//
const TRANSFORM & AnimNugget::GetTransform (void) const
{
	static TRANSFORM animTransform;
	animTransform.translation = position;

	return animTransform;
}
//---------------------------------------------------------------------------
//
SINGLE AnimNugget::TestHighlight (const RECT &rect)
{
	bHighlight = 0;
	if (bVisible)
	{
//		MISSION_DATA::M_CAPS caps = getCaps(OBJLIST->GetSelectedList());
		const BOOL32 bEditorMode = DEFAULTS->GetDefaults()->bEditorMode;
		
//		if(caps.harvestOk || bEditorMode)
//		{
			// only highlight when mouse is over us
			if (rect.left == rect.right && rect.top == rect.bottom || bEditorMode)
			{
				S32 screenX,screenY;
				CAMERA->PointToScreen(position,&screenX,&screenY);
				if (screenX-15 < rect.right && screenX+15 > rect.left && screenY-15 < rect.bottom && screenY+15 > rect.top)
				{
					bHighlight = TRUE;
				}
			}
//		}
	}

	return 0.0f;
}
//---------------------------------------------------------------------------
//
void AnimNugget::GameplayTestVisible(const U32 currentPlayer)
{
	if(bDepleted && (!  (((0x01 << (currentPlayer-1)) & shadowVisibilityFlags) && shadowSupplies[currentPlayer-1]) ))
	{
		bVisible = false;
		return;
	}

	if (bRealized==false || (invisibleTimer > 0))
	{
		bVisible = false;
		return;
	}
}
//---------------------------------------------------------------------------
//
void AnimNugget::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	if(bDepleted && (!  (((0x01 << (currentPlayer-1)) & shadowVisibilityFlags) && shadowSupplies[currentPlayer-1]) ))
	{
		bVisible = false;
		return;
	}

	if (bRealized==false || (invisibleTimer > 0))
	{
		bVisible = false;
		return;
	}

	IBaseObject::TestVisible(defaults,currentSystem,currentPlayer);
	
	if (bVisible)
	{
		S32 screenX,screenY;
		CAMERA->PointToScreen(position,&screenX,&screenY);
		
		RECT _rect;
		
		_rect.left  = screenX - 15;
		_rect.right	= screenX + 15;
		_rect.top = screenY - 15;
		_rect.bottom = screenY + 15;
		
		RECT screenRect = { 0, 0, SCREENRESX, SCREENRESY };
		
		bVisible = RectIntersects(_rect, screenRect);
	}
}
//---------------------------------------------------------------------------
//
void AnimNugget::CastVisibleArea (void)
{
}
//---------------------------------------------------------------------------
//
BOOL32 AnimNugget::Update (void)
{
	if(invisibleTimer > 0)
	{
		invisibleTimer -= ELAPSED_TIME;
		if(invisibleTimer <= 0)
			NUGGETMANAGER->CameraMove(systemID);
	}

	if (lifeTimeRemaining>0)
	{
		lifeTimeRemaining -= ELAPSED_TIME;
		if (lifeTimeRemaining <= 0)
			supplies = 0;
	}
	if(supplies == 0 && THEMATRIX->IsMaster() && (!harvestCount) && !bUnregistered)
	{
		UnregisterWatchersForObjectForPlayerMask(this,((~shadowVisibilityFlags) << 1)|(0x01 << SYSVOLATILEPTR));
		bUnregistered = true;
	}

	if(THEMATRIX->IsMaster() && (!shadowVisibilityFlags) && (supplies == 0) && (supplies == trueNetSupplies) && (!harvestCount) && (!deleteOK) &&
		((!deathOp) ||(deathOp == processID)))
	{
		NUGGETMANAGER->SendDeleteNugget(GetPartID(),processID);
		deleteOK = true;
	}
	if((supplies == 0) && (supplies == trueNetSupplies) && (!harvestCount) && deleteOK && ((!deathOp) ||(deathOp == processID)))
	{
		if (CQFLAGS.bTraceMission)
			CQTRACE12("Nugget DeferredDestruction : %x deathOp:%d\n",dwMissionID,deathOp);
		OBJLIST->DeferredDestruction(GetPartID());
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void AnimNugget::PhysicalUpdate (SINGLE dt)
{
}
//---------------------------------------------------------------------------
//
void AnimNugget::Render (void)
{
		SINGLE dt = OBJLIST->GetRealRenderTime();
		NUGGET_INIT * nugInit = (NUGGET_INIT *)ARCHLIST->GetArchetypeHandle(pArchetype);

		SINGLE useSupply;
		U32 playerIndex = MGlobals::GetThisPlayer()-1;
		if((0x01 << (playerIndex)) & shadowVisibilityFlags)
			useSupply = shadowSupplies[playerIndex];
		else
			useSupply = supplies;

		CQASSERT(useSupply <= supplyPointsMax);
		if(useSupply == 0)
			return;
		
		
		if(pulseUp)
		{
			pulse += dt*((((SINGLE)useSupply)/(supplyPointsMax*2.0))+0.5);
			if(pulse > 1.0)
			{
				pulse = 1.0;
				pulseUp = false;
			}
		}
		else
		{
			pulse -= dt*((((SINGLE)useSupply)/(supplyPointsMax*2.0))+0.5);
			if(pulse < 0.0)
			{
				pulse = 0.0;
				pulseUp = true;
			}
		}
		t += dt;

		SINGLE totalTime = nugInit->animArch->frame_cnt/nugInit->animArch->capture_rate;
		while(t > totalTime)
			t -= totalTime;
		U32 frame = (t*nugInit->animArch->capture_rate);
		frame = frame%nugInit->animArch->frame_cnt;//just in case
		
		const AnimFrame* anim = &(nugInit->animArch->frames[frame]);

		SINGLE scale = (SINGLE(useSupply))/(SINGLE(supplyPointsMax));

		U8 red = (nugInit->pData->color.redHi-nugInit->pData->color.redLo)*pulse + nugInit->pData->color.redLo;
		U8 green = (nugInit->pData->color.greenHi-nugInit->pData->color.greenLo)*pulse + nugInit->pData->color.greenLo;
		U8 blue = (nugInit->pData->color.blueHi-nugInit->pData->color.blueLo)*pulse + nugInit->pData->color.blueLo;
		U8 alpha = (nugInit->pData->color.alphaIn-nugInit->pData->color.alphaOut)*scale + nugInit->pData->color.alphaOut;

		SINGLE animWidth = (myMaxAnimSize-nugInit->pData->animSizeMin)*scale + nugInit->pData->animSizeMin;

		CAMERA->SetPerspective();

		CAMERA->SetModelView();

		BATCH->set_state(RPR_BATCH,false);
		BATCH->set_state(RPR_STATE_ID,0);
		//BATCH->set_state(RPR_BATCH,true);
		BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
		SetupDiffuseBlend(nugInit->animArch->frames[frame].texture,TRUE);
		//BATCH->set_state(RPR_STATE_ID,nugInit->animArch->frames[frame].texture);
	
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		if(nugInit->pData->zRender)
		{
			//for now ZENABLE and ZWRITEENABLE are not batched
			//can we get away with alpha testing?
			BATCH->set_render_state(D3DRS_ALPHATESTENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ALPHAREF,8);
			BATCH->set_render_state(D3DRS_ALPHAFUNC,D3DCMP_GREATER);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
		}
		else
		{
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		}

		Vector v0,v1,v2,v3;
		Vector epos (position);

		SINGLE x = animWidth;
		SINGLE y = (nugInit->animArch->frames[0].y1 - nugInit->animArch->frames[0].y0)* x / (nugInit->animArch->frames[0].x1 - nugInit->animArch->frames[0].x0);
		SINGLE pivx = (x)*0.5f;
		SINGLE pivy = (y)*0.5f;

		Transform trans;
		
		Vector cpos (MAINCAM->get_position());
					
		Vector look (epos - cpos);
					
		Vector i (look.y, -look.x, 0);
					
		if (fabs (i.x) < TOLERANCE && fabs (i.y) < TOLERANCE)
		{
			i.x = 1.0f;
		}
					
		i.normalize ();
					
		Vector k (-look);
		k.normalize ();
		Vector j (cross_product (k, i));

		trans.set_i(i);
		trans.set_j(j);
		trans.set_k(k);
		trans.translation = epos;


		v0.set(((0-pivx)*cosA-(0-pivy)*sinA),((0-pivx)*sinA+(0-pivy)*cosA),0);
		v1.set(((x-pivx)*cosA-(0-pivy)*sinA),((x-pivx)*sinA+(0-pivy)*cosA),0);
		v2.set(((x-pivx)*cosA-(y-pivy)*sinA),((x-pivx)*sinA+(y-pivy)*cosA),0);
		v3.set(((0-pivx)*cosA-(y-pivy)*sinA),((0-pivx)*sinA+(y-pivy)*cosA),0);

		v0 = trans*v0;
		v1 = trans*v1;
		v2 = trans*v2;
		v3 = trans*v3;

	if (bVisible)
	{
		PB.Begin (PB_QUADS);
		PB.Color4ub (red, green, blue, alpha);
		
		PB.TexCoord2f (anim->x0, anim->y0);
		PB.Vertex3f (v0.x, v0.y, v0.z);
		PB.TexCoord2f (anim->x1, anim->y0);
		PB.Vertex3f (v1.x, v1.y, v1.z);
		PB.TexCoord2f (anim->x1, anim->y1);
		PB.Vertex3f (v2.x, v2.y, v2.z);
		PB.TexCoord2f (anim->x0, anim->y1);
		PB.Vertex3f (v3.x, v3.y, v3.z);
		PB.End ();
		
		BATCH->set_render_state(D3DRS_ALPHATESTENABLE,FALSE);
		BATCH->set_state(RPR_STATE_ID,0);
	}
}
//---------------------------------------------------------------------------
//
void AnimNugget::MapRender (bool bPing)
{
}
//---------------------------------------------------------------------------
//
void AnimNugget::DrawHighlighted (void)
{
	SINGLE useSupply;
	U32 playerIndex = MGlobals::GetThisPlayer()-1;
	if((0x01 << (playerIndex)) & shadowVisibilityFlags)
		useSupply = shadowSupplies[playerIndex];
	else
		useSupply = supplies;

	
	BT_NUGGET_DATA * nugData = (BT_NUGGET_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
	SINGLE scale = ((SINGLE)(useSupply))/((SINGLE)(supplyPointsMax));

	Vector point;

	point.x = 0;
	point.y = -((myMaxAnimSize-nugData->animSizeMin)*scale+nugData->animSizeMin);
	point.z = 0;

	TRANSFORM trans;
	trans.translation = position;

	S32 x,y;
	CAMERA->PointToScreen(point, &x, &y, &trans);

	PANE * pane = CAMERA->GetPane();
	
	if (useSupply > 0)
	{
		int TBARLENGTH = 80;
		useSupply /= supplyPointsMax;
		if (supplyPointsMax < 1500)
		{
			if (supplyPointsMax < 500)
				TBARLENGTH = 30;
			else
			{
				TBARLENGTH = 30 + (((supplyPointsMax - 500)*50) / 1000);
			}
		}
		TBARLENGTH = IDEAL2REALX(TBARLENGTH);
		//
		// draw the blue (supplies) bar RGB(50,50,200)
		//
		
		if (OBJLIST->GetHighlightedList()==this && nextHighlighted==0)
		{
			DA::RectangleHash(pane, x-(TBARLENGTH/2), y+5, x+(TBARLENGTH/2), y+5+2, RGB(128,128,128));
			if (useSupply > 0.0f)
			{
				COLORREF color ;
				if(nugData->resType == M_GAS)
					color = RGB(128,0,255);
				else
					color = RGB(255,255,255);

				int xpos = x-(TBARLENGTH/2);
				int max = S32(TBARLENGTH*useSupply);
				int xrc;

				// make sure at least one bar gets displayed for one supply point
				if (max == 0 && useSupply > 0.0f)
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
	}

#ifdef _DEBUG
	if ((nextHighlighted==0) && (OBJLIST->GetHighlightedList()==this))
	{
		COMPTR<IFontDrawAgent> pFont;
		OBJLIST->GetUnitFont(pFont);

		pFont->SetFontColor(RGB(180,180,180) | 0xFF000000, 0);
		wchar_t temp[M_MAX_STRING];
		swprintf(temp,L"#%x#",dwMissionID);
		pFont->StringDraw(pane, x-20, y+10, temp);
	}
#endif

}
//---------------------------------------------------------------------------
//
void AnimNugget::DrawSelected (void)
{
	if (bVisible==0)
		return;
	
	TRANSFORM trans;
	trans.rotate_about_i(PI/2);
	trans.translation = position;
	
	CAMERA->SetModelView(&trans);
	
	PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	//no textures
	PIPE->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	PIPE->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );	
	//
	PIPE->set_render_state(D3DRS_ZENABLE,TRUE);
	PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
	PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	PB.Color4ub(216,201,20, 180);		// RGB_GOLD
	
	const SINGLE TWIDTH  = 100;
#define W1 -200
#define L1 -200
#define W2 200
#define L2 200

	PB.Begin(PB_TRIANGLES);

	PB.Vertex3f(W1, 0, L1);
	PB.Vertex3f(W1+TWIDTH, 0, L1);
	PB.Vertex3f(W1, 0, L1+TWIDTH);

	PB.Vertex3f(W2-TWIDTH, 0, L1);
	PB.Vertex3f(W2, 0, L1);
	PB.Vertex3f(W2, 0, L1+TWIDTH);

	PB.Vertex3f(W1, 0, L2-TWIDTH);
	PB.Vertex3f(W1+TWIDTH, 0, L2);
	PB.Vertex3f(W1, 0, L2);

	PB.Vertex3f(W2, 0, L2-TWIDTH);
	PB.Vertex3f(W2, 0, L2);
	PB.Vertex3f(W2-TWIDTH, 0, L2);

	PB.End(); 	// end of GL_QUADS
}
//--------------------------------------------------------------------------
//
void AnimNugget::UpdateVisibilityFlags (void)
{
	U8 newShadowVisibilityFlags;
	if(supplies == 0)
	{
		newShadowVisibilityFlags = shadowVisibilityFlags & (~GetPendingVisibilityFlags());
	}
	else
		newShadowVisibilityFlags = GetVisibilityFlags() & (~GetPendingVisibilityFlags());
	if(newShadowVisibilityFlags != shadowVisibilityFlags)
	{
		U32 newFlags = newShadowVisibilityFlags & (~shadowVisibilityFlags);
		if(newFlags)
		{
			//record state
			for(U32 i = 0; i < MAX_PLAYERS; ++i)
			{
				if((newFlags >> i) & 0x01)
				{
					shadowSupplies[i] = supplies;
				}
			}
		}
		U32 oldFlags = (~newShadowVisibilityFlags) & shadowVisibilityFlags;
		if(oldFlags)
		{
			if(supplies == 0)
				if(THEMATRIX->IsMaster())
					UnregisterWatchersForObjectForPlayerMask(this,oldFlags << 1);
		}
		shadowVisibilityFlags = newShadowVisibilityFlags;
	}
	U32 visFlags = GetVisibilityFlags();
	IBaseObject::UpdateVisibilityFlags2();//calling 2 on purpose;
	if(visFlags != GetVisibilityFlags())
		NUGGETMANAGER->CameraMove(systemID);
}
//---------------------------------------------------------------------------
//
void AnimNugget::UpdateVisibilityFlags2 (void)
{
	U8 newShadowVisibilityFlags;
	if(supplies == 0)
	{
		newShadowVisibilityFlags = shadowVisibilityFlags & (~GetPendingVisibilityFlags());
	}
	else
		newShadowVisibilityFlags = GetVisibilityFlags() & (~GetPendingVisibilityFlags());
	if(newShadowVisibilityFlags != shadowVisibilityFlags)
	{
		U32 newFlags = newShadowVisibilityFlags & (~shadowVisibilityFlags);
		if(newFlags)
		{
			//record state
			for(U32 i = 0; i < MAX_PLAYERS; ++i)
			{
				if((newFlags >> i) & 0x01)
				{
					shadowSupplies[i] = supplies;
				}
			}
		}
		shadowVisibilityFlags = newShadowVisibilityFlags;
	}
	U32 visFlags = GetVisibilityFlags();
	IBaseObject::UpdateVisibilityFlags2();//calling 2 on purpose;
	if(visFlags != GetVisibilityFlags())
		NUGGETMANAGER->CameraMove(systemID);
}
//---------------------------------------------------------------------------
//
bool AnimNugget::IsTargetableByPlayer (U32 _playerID)
{
	if((supplies == 0) && (_playerID >0) && (_playerID <= MAX_PLAYERS))
	{
		return ((0x01 << (_playerID-1)) & shadowVisibilityFlags) != 0;
	}
	return true;
}
//---------------------------------------------------------------------------
//
void AnimNugget::InitNugget(U32 partID, U32 _systemID, const Vector &destPos,S32 scrapValue, SINGLE lifeTime, bool _bRealized)
{
	deathOp = 0;
	bUnregistered = false;
	dwMissionID = partID;
	systemID = _systemID;
	position = destPos;
	supplyPointsMax = scrapValue;
	bRealized = _bRealized;
	lifeTimeRemaining = lifeTime;

	trueNetSupplies = supplies = scrapValue;

	CQASSERT(systemID <= MAX_SYSTEMS && "No nuggets in hyperspace please");
	map_square = OBJMAP->GetMapSquare(systemID,position);
	mapSysID = systemID;
	SetReady(true);
	OBJMAP->AddObjectToMap(this,mapSysID,map_square);
	OBJLIST->AddPartID(this, dwMissionID);
}
//---------------------------------------------------------------------------
//
void AnimNugget::SetDepleted (bool depSetting)
{
	bDepleted = depSetting;
}
//---------------------------------------------------------------------------
//
void AnimNugget::IncHarvestCount ()
{
	CQASSERT(harvestCount != 255); //overflow
	++harvestCount;
}
//---------------------------------------------------------------------------
//
void AnimNugget::DecHarvestCount ()
{
	CQASSERT(harvestCount);
	--harvestCount;
}
//---------------------------------------------------------------------------
//
U8 AnimNugget::GetHarvestCount ()
{
	return harvestCount;
}
//---------------------------------------------------------------------------
//
void AnimNugget::SetProcessID (U32 newProcID)
{
	processID = newProcID;
}
//---------------------------------------------------------------------------
//
U32 AnimNugget::GetProcessID ()
{
	return processID;
}
//---------------------------------------------------------------------------
//
M_RESOURCE_TYPE AnimNugget::GetResourceType()
{
	BT_NUGGET_DATA * nugData = (BT_NUGGET_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
	return nugData->resType;
}
//---------------------------------------------------------------------------
//
enum M_NUGGET_TYPE AnimNugget::GetNuggetType()
{
	BT_NUGGET_DATA * nugData = (BT_NUGGET_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
	return nugData->nuggetType;
}

//---------------------------------------------------------------------------
//
U32 AnimNugget::GetSupplies()
{
	return supplies;
}
//---------------------------------------------------------------------------
//
U32 AnimNugget::GetMaxSupplies()
{
	return supplyPointsMax;
}
//---------------------------------------------------------------------------
//
void AnimNugget::SetSupplies(U32 newSupplies)
{
	supplies = newSupplies;
}
//---------------------------------------------------------------------------
//
void AnimNugget::SetSystemID (U32 newSystemID)
{
	systemID = newSystemID;
}
//---------------------------------------------------------------------------
//
void AnimNugget::SetPosition (const Vector & _position, U32 newSystemID)
{
	position = _position;
	systemID = newSystemID;

	int new_map_square = OBJMAP->GetMapSquare(systemID,position);
	if (new_map_square != map_square || systemID != mapSysID)
	{
		if(mapSysID)
			OBJMAP->RemoveObjectFromMap(this,mapSysID,map_square);
		map_square = new_map_square;
		mapSysID = systemID;
		OBJMAP->AddObjectToMap(this,mapSysID,map_square);
	}
}
//---------------------------------------------------------------------------
//
void AnimNugget::SetTransform (const TRANSFORM & transform, U32 newSystemID)
{
	position = transform.translation;
	systemID = newSystemID;

	int new_map_square = OBJMAP->GetMapSquare(systemID,position);
	if (new_map_square != map_square || systemID != mapSysID)
	{
		if(mapSysID)
			OBJMAP->RemoveObjectFromMap(this,mapSysID,map_square);
		map_square = new_map_square;
		mapSysID = systemID;
		OBJMAP->AddObjectToMap(this,systemID,map_square);
	}
}
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
//

inline struct IBaseObject * createAnimNugget (const NUGGET_INIT & data)
{
	AnimNugget * obj = new ObjectImpl<AnimNugget>;

	obj->pArchetype = data.pArchetype;
	obj->objClass = OC_NUGGET;
	obj->myMaxAnimSize = (data.pData->animSizeMax-data.pData->animSizeSmallMax)*(((SINGLE)(rand()%1000))/1000.0)+data.pData->animSizeSmallMax;
	SINGLE totalTime = data.animArch->frame_cnt/data.animArch->capture_rate;
	obj->t = ((SINGLE)(rand()%1000) / 1000.0) * totalTime;
	if(!(data.pData->bOriented))
	{
		obj->cosA = 1;
		obj->sinA = 0;
	}
	else
	{
		SINGLE angle = 2*PI*rand()/RAND_MAX;
		obj->cosA = (SINGLE)cos(angle);
		obj->sinA = (SINGLE)sin(angle);
	}
	obj->pulseUp = ((rand()%2)== 0);
	obj->pulse = ((SINGLE)(rand()%1000))/1000.0;

	obj->trueNetSupplies = obj->supplies = obj->supplyPointsMax = data.pData->maxSupplies;
	obj->shadowVisibilityFlags = 0;
	obj->bDepleted = false;
	obj->deleteOK = false;
	return obj;
}

//------------------------------------------------------------------------------------------
//---------------------------NuggetManager----------------------------------------------
//------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------//
//
MISSION_DATA::M_CAPS getCaps (IBaseObject * selected)
{
	MISSION_DATA::M_CAPS caps;
	MPart hSource;

	memset(&caps, 0, sizeof(caps));

	while (selected)
	{
		hSource = selected;
		if (hSource.isValid())
			caps |= hSource->caps;

		selected = selected->nextSelected;
	}

	return caps;
}
//---------------------------------------------------------------------------
//
/*
struct DelayNuggetData
{
	DelayNuggetData * next;
	Vector position;
	U32 dwMissionID;
	U32 deathOp;
	U16 supplies;
	bool bDeleteOk:1;
};
*/

#pragma pack (push , 1)
struct SupplySync
{
	U16 suppies;
	U32 dwMissionID;
};

struct CreateSync
{
	U32 dwMissionID;
	U32 archeID;
	NETGRIDVECTOR position;
	U16 supplyValue;
};
#pragma pack ( pop )
//---------------------------------------------------------------------------
//

#define MAX_NUGGET_DELETE 20

struct DACOM_NO_VTABLE NuggetManager : public INuggetManager, IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(NuggetManager)
	DACOM_INTERFACE_ENTRY(INuggetManager)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	void * operator new (size_t size)
	{
		return calloc(size,1);
	}
	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	U32 eventHandle;
	U32 currentSync;
	U32 currentUpdate;

	bool bCameraMoved;
	U32 lastSystem;
	AnimNugget * nuggetLists[MAX_SYSTEMS];

	NuggetManager (void) 
	{
	}

	virtual ~NuggetManager (void);

	/* IEventCallback methods */
	GENRESULT __stdcall Notify (U32 message, void *param);

	/* INuggetManager methods */

	virtual IBaseObject * FindNugget (U32 dwMissionID);

	virtual void FindNugget (U32 dwMissionID, U32 fromPlayerID, OBJPTR<IBaseObject> & obj, enum OBJID objid);

	virtual void CreateNugget (PARCHETYPE pArchetype, U32 systemID, const Vector & position, U32 supplyValue, U32 lifeTime,
		U32 dwMissionID,bool network);

	virtual void RealizeNugget (U32 nuggetID, const Vector & position, U32 systemID, SINGLE animDelay);

	virtual U32 GetSyncData(void * buffer);

	virtual void PutSyncData(void * buffer,U32 bufferSize);

	virtual void ReceiveNuggetData(void * buffer,U32 bufferSize);

	virtual void ReceiveNuggetDeath (U32 nuggetID, void * buffer, U32 bufferSize);

	virtual void Save(IFileSystem * outFile);

	virtual void Load(IFileSystem * inFile);

	virtual void Close();

	virtual void Update();

	virtual void Render();

	virtual void UpdateVisibilityFlags();

	virtual void TestVisible(const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual IBaseObject * TestVisibleHighlight(SINGLE & closest, const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer, const RECT & selectionRect);

	virtual void MapRender(bool bPing);

	virtual void DeleteNugget(U32 dwMissionID);

	virtual void SendDeleteNugget(U32 dwMissionID,U32 deathOp);

	virtual IBaseObject * GetFirstNugget(U32 systemID);

	virtual IBaseObject * GetNextNugget(IBaseObject *nugget);

	virtual void RemoveNuggetsFromSystem(U32 systemID);

	virtual void MoveNuggetsToSystem(U32 oldSystemID, U32 newSystemID);

	virtual void CameraMove(U32 systemID);

	void createNugget (PARCHETYPE pArchetype, U32 systemID, const Vector & position, U32 supplyValue, U32 lifeTime, U32 dwMissionID, bool bRealized);

	void init (void);

	IDAComponent * getBase (void)
	{
		return static_cast<INuggetManager *>(this);
	}
};
//--------------------------------------------------------------------------//
//
NuggetManager::~NuggetManager()
{
	if (GS)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}
}
//--------------------------------------------------------------------------//
//
GENRESULT NuggetManager::Notify (U32 message, void *param)
{
	switch (message)
	{
	case CQE_SYSTEM_CHANGED:
	case CQE_CAMERA_MOVED:
		bCameraMoved = 1;
		break;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void NuggetManager::Close()
{
	for(U32 i = 0; i < MAX_SYSTEMS; ++i)
	{
		while(nuggetLists[i])
		{
			AnimNugget * nug = nuggetLists[i];
			nuggetLists[i] = nug->nextNugget;
			delete nug;
		}
	}
}
//--------------------------------------------------------------------------//
// called 30 times a second.
// Update two lists so that nuggets get updated 4 times a second (30 / 16 * 2)
//
void NuggetManager::Update()
{
	++currentUpdate;
	currentUpdate = currentUpdate%MAX_SYSTEMS;
	AnimNugget * nugget = nuggetLists[currentUpdate];
	while(nugget)
	{
		nugget->Update();
		nugget = nugget->nextNugget;
	}

	++currentUpdate;
	currentUpdate = currentUpdate%MAX_SYSTEMS;
	nugget = nuggetLists[currentUpdate];
	while(nugget)
	{
		nugget->Update();
		nugget = nugget->nextNugget;
	}
}
//--------------------------------------------------------------------------//
//
void NuggetManager::Render()
{
	U32 systemIndex = SECTOR->GetCurrentSystem()-1;
	if(systemIndex < MAX_SYSTEMS)
	{
		AnimNugget * nugget = nuggetLists[systemIndex];
		
		bool bDoubleRenderKludge = 1;
		while(nugget)
		{
			if (nugget->bVisible)
			{
				if (bDoubleRenderKludge)
				{
					nugget->bVisible = false;
					nugget->Render();
					nugget->bVisible = true;
					bDoubleRenderKludge = 0;
				}
				nugget->Render();
			}
			nugget = nugget->nextNugget;
		}
	}

	bCameraMoved = false;
}
//--------------------------------------------------------------------------//
//
void NuggetManager::UpdateVisibilityFlags()
{
	for(U32 i = 0; i < MAX_SYSTEMS; ++i)
	{
		AnimNugget * nugget = nuggetLists[i];
		while(nugget)
		{
			nugget->UpdateVisibilityFlags();
			nugget = nugget->nextNugget;
		}
	}
}
//--------------------------------------------------------------------------//
//
void NuggetManager::TestVisible(const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	if((currentSystem != lastSystem) && lastSystem)
	{
		AnimNugget * nugget = nuggetLists[lastSystem-1];
		while(nugget)
		{
			nugget->bVisible = false;
			nugget = nugget->nextNugget;
		}
		CQASSERT(bCameraMoved);		// if system has changed, we should have noticed camera moved
	}
	lastSystem = currentSystem;
	AnimNugget * nugget = nuggetLists[currentSystem-1];
	if (bCameraMoved)		// optimization
	{
		while(nugget)
		{
			nugget->TestVisible(defaults,currentSystem,currentPlayer);
			nugget = nugget->nextNugget;
		}
	}
	else
	{
		while(nugget)
		{
			nugget->GameplayTestVisible(currentPlayer);
			nugget = nugget->nextNugget;
		}
	}
}
//--------------------------------------------------------------------------//
//
IBaseObject * NuggetManager::TestVisibleHighlight(SINGLE & closest, const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer, const RECT & selectionRect)
{
	MISSION_DATA::M_CAPS caps = getCaps(OBJLIST->GetSelectedList());
	const BOOL32 bEditorMode = DEFAULTS->GetDefaults()->bEditorMode;

	if((currentSystem != lastSystem) && lastSystem)
	{
		AnimNugget * nugget = nuggetLists[lastSystem-1];
		while(nugget)
		{
			nugget->bVisible = false;
			nugget = nugget->nextNugget;
		}
		CQASSERT(bCameraMoved);		// if system has changed, we should have noticed camera moved
	}
	lastSystem = currentSystem;
	
	IBaseObject * retVal = NULL;
	AnimNugget * nugget = nuggetLists[currentSystem-1];
	while (nugget)
	{
		if (bCameraMoved)		// optimization
			nugget->TestVisible(defaults, currentSystem, currentPlayer);
		else
			nugget->GameplayTestVisible(currentPlayer);
		if(nugget->bVisible && (caps.harvestOk || bEditorMode))
		{
			SINGLE closeness = nugget->TestHighlight(selectionRect);
			if (nugget->bHighlight && closeness < closest)
			{
				closest = closeness;
				retVal = nugget;
			}
		}
		nugget = nugget->nextNugget;
	}
	return retVal;
}

#define GAS_VALUE 1
#define METAL_VALUE 2
#define GET_NUGMAP_VALUE(X,Y) ((nugFields[((X)/16)+ ((Y)*4)] >> ((((X)%16)*2))) & 0x03)
#define SET_NUGMAP_VALUE(X,Y,VALUE) {                                \
	nugFields[((X)/16)+ ((Y)*4)] &= (~((0x03) << (((X)%16)*2)));      \
	nugFields[((X)/16)+ ((Y)*4)] |= (((VALUE)&0x03) << (((X)%16)*2)); }

#define NUG_MAP_SIZE 4*64
//--------------------------------------------------------------------------//
//
void NuggetManager::MapRender(bool bPing)
{
	U32 nugFields[NUG_MAP_SIZE];
	memset(nugFields,0,sizeof(nugFields));
	U32 systemIndex = SECTOR->GetCurrentSystem()-1;
	if(systemIndex < MAX_SYSTEMS)
	{
		AnimNugget * nugget = nuggetLists[systemIndex];
		while(nugget)
		{
			U32 gridX = nugget->GetPosition().x/GRIDSIZE;
			U32 gridY = nugget->GetPosition().y/GRIDSIZE;
			U32 curVal = GET_NUGMAP_VALUE(gridX,gridY);
			if(nugget->GetResourceType() == M_GAS)
			{
				if(curVal == 0)
					SET_NUGMAP_VALUE(gridX,gridY,GAS_VALUE);
			}
			else//metal
			{
				if(curVal != METAL_VALUE)
					SET_NUGMAP_VALUE(gridX,gridY,METAL_VALUE);

			}
			nugget = nugget->nextNugget;
		}
	}
	for(U32 count = 0; count < NUG_MAP_SIZE; ++count)
	{
		U32 val = nugFields[count];
		if(val)
		{
			for(U32 shift = 0; shift < 32; shift+= 2)
			{
				U32 type = ((val >> shift) & 0x3);
				if(type == GAS_VALUE)
					SYSMAP->DrawSquare(Vector((((count%4)*16)+(shift/2))*GRIDSIZE+(GRIDSIZE/2),(count/4)*GRIDSIZE+(GRIDSIZE/2),0),GRIDSIZE,RGB(128,0,128));
				else if(type == METAL_VALUE)
					SYSMAP->DrawSquare(Vector((((count%4)*16)+(shift/2))*GRIDSIZE+(GRIDSIZE/2),(count/4)*GRIDSIZE+(GRIDSIZE/2),0),GRIDSIZE,RGB(128,128,128));
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void NuggetManager::DeleteNugget(U32 dwMissionID)
{
	for(U32 i = 0; i < MAX_SYSTEMS; ++i)
	{
		AnimNugget * nugget = nuggetLists[i];
		AnimNugget * prev = NULL;
		while(nugget)
		{
			if(nugget->dwMissionID == dwMissionID)
			{
				if(prev)
					prev->nextNugget = nugget->nextNugget;
				else
					nuggetLists[i] = nugget->nextNugget;
				delete nugget;
				return;
			}
			prev = nugget;
			nugget = nugget->nextNugget;
		}
	}
}
//--------------------------------------------------------------------------//
//
void NuggetManager::SendDeleteNugget(U32 dwMissionID,U32 deathOp)
{
	THEMATRIX->SendNuggetDeath(dwMissionID, &deathOp, sizeof(U32));
}
//--------------------------------------------------------------------------//
//
IBaseObject * NuggetManager::GetFirstNugget(U32 systemID)
{
	if(systemID == 0) return NULL;
	return nuggetLists[systemID - 1];
}
//--------------------------------------------------------------------------//
//
IBaseObject * NuggetManager::GetNextNugget(IBaseObject *nugget)
{
	if(nugget) 
	{
		CQASSERT(nugget->objClass == OC_NUGGET);
		AnimNugget * animnugget = (AnimNugget *)nugget;
		return animnugget->nextNugget;
	}

	return NULL;
}
//--------------------------------------------------------------------------//
//
void NuggetManager::RemoveNuggetsFromSystem(U32 systemID)
{
	AnimNugget * nugget = nuggetLists[systemID-1];
	while(nugget)
	{
		AnimNugget * delNug = nugget;
		nugget = nugget->nextNugget;
		delete delNug;
	}
	nuggetLists[systemID-1] = NULL;
}
//--------------------------------------------------------------------------//
//
void NuggetManager::MoveNuggetsToSystem(U32 oldSystemID, U32 newSystemID)
{
	nuggetLists[newSystemID-1] = nuggetLists[oldSystemID-1];
	AnimNugget * nugget = nuggetLists[newSystemID-1];
	while(nugget)
	{
		nugget->systemID = newSystemID;
		nugget = nugget->nextNugget;
	}
	nuggetLists[oldSystemID-1] = NULL;
}
//--------------------------------------------------------------------------//
//
void NuggetManager::CameraMove(U32 systemID)
{
	if(systemID == lastSystem)
		bCameraMoved = true;
}
//--------------------------------------------------------------------------//
//
IBaseObject * NuggetManager::FindNugget (U32 dwMissionID)
{
	for(U32 i = 0; i < MAX_SYSTEMS; ++i)
	{
		AnimNugget * nugget = nuggetLists[i];
		while(nugget)
		{
			if(nugget->dwMissionID == dwMissionID)
				return nugget;
			nugget = nugget->nextNugget;
		}
	}
	return NULL;
}
//--------------------------------------------------------------------------//
//
void NuggetManager::FindNugget (U32 dwMissionID, U32 fromPlayerID, OBJPTR<IBaseObject> & obj, enum OBJID objid)
{
	IBaseObject * result = FindNugget(dwMissionID);
	if (result)
	{
		if(THEMATRIX->IsMaster())
		{
			if(result->IsTargetableByPlayer(fromPlayerID))
			{
				result->QueryInterface(objid, obj, fromPlayerID);
			}
			else
			{
				result = 0;
				obj = NULL;
			}
		}
		else
			result->QueryInterface(objid, obj, fromPlayerID);
	}
	else
	{
		result = 0;
		obj = NULL;
	}
}

//--------------------------------------------------------------------------//
//
void NuggetManager::createNugget (PARCHETYPE pArchetype, U32 systemID, const Vector & position, U32 supplyValue, U32 lifeTime, U32 dwMissionID, bool bRealized)
{
	AnimNugget *obj= (AnimNugget *) (MGlobals::CreateInstance(pArchetype, dwMissionID));
	obj->InitNugget(dwMissionID,systemID, position,supplyValue,lifeTime, bRealized);

	obj->nextNugget = nuggetLists[systemID-1];
	nuggetLists[systemID-1] = obj;

	bCameraMoved = true;
}
//--------------------------------------------------------------------------//
//
void NuggetManager::CreateNugget (PARCHETYPE pArchetype, U32 systemID, const Vector & position, U32 supplyValue, U32 lifeTime,
		U32 dwMissionID,bool network)
{
	const bool bMaster = THEMATRIX->IsMaster();
	CQASSERT(bMaster || network==false);

	if(network && bMaster)
	{
		CreateSync buffer;
	
		if (FindNugget(dwMissionID) != 0)
			return;		// this can happen right after a host migration!
			
		buffer.dwMissionID = dwMissionID;
		buffer.archeID = ARCHLIST->GetArchetypeDataID(pArchetype);
		buffer.position.init(position, systemID);
		buffer.supplyValue = supplyValue;
		THEMATRIX->SendNuggetData(&buffer,sizeof(buffer));
	}

	createNugget(pArchetype, systemID, position, supplyValue, lifeTime, dwMissionID, network==false);
}
//--------------------------------------------------------------------------//
//
void NuggetManager::RealizeNugget (U32 nuggetID, const Vector & position, U32 systemID, SINGLE animDelay)
{
	AnimNugget * nugget = (AnimNugget *)FindNugget(nuggetID);

	if (nugget)
	{
		nugget->bRealized = bCameraMoved = true;
		nugget->invisibleTimer = animDelay;
	}
}
//--------------------------------------------------------------------------//
//
U32 NuggetManager::GetSyncData(void * buffer)
{
	SupplySync * syncBuffer = (SupplySync *)((U8 *)buffer);
	++currentSync;
	currentSync = currentSync%MAX_SYSTEMS;
	AnimNugget * nugget = nuggetLists[currentSync];
	U32 count = 0;
	while(nugget && (count < 20))
	{
		if(nugget->supplies != nugget->trueNetSupplies && (!nugget->deleteOK))
		{
			syncBuffer[count].dwMissionID = nugget->dwMissionID;
			nugget->trueNetSupplies = syncBuffer[count].suppies = nugget->supplies;
			++count;
		}
		nugget = nugget->nextNugget;
	}

	if(!count)
		return 0;

	return count*sizeof(SupplySync);
}
//--------------------------------------------------------------------------//
//
void NuggetManager::PutSyncData(void * buffer,U32 bufferSize)
{
	if(!bufferSize)
		return;
	SupplySync * syncBuffer = (SupplySync *)((U8 *)buffer);
	U32 count = bufferSize/sizeof(SupplySync);
	for(U32 i = 0; i < count; ++ i)
	{
		AnimNugget * nugget = (AnimNugget *)FindNugget(syncBuffer[i].dwMissionID);
		if(nugget)
		{
			CQASSERT(syncBuffer[i].suppies <= nugget->supplyPointsMax);
			nugget->trueNetSupplies = nugget->supplies = syncBuffer[i].suppies;
		}
		else
		{
			CQBOMB1("Received sync data for dead nugget 0x%X.", syncBuffer->dwMissionID);
		}
	}
}
//--------------------------------------------------------------------------//
//
void NuggetManager::ReceiveNuggetDeath (U32 nuggetID, void * buffer, U32 bufferSize)
{
	AnimNugget * nugget = (AnimNugget *)FindNugget(nuggetID);
	if(nugget)
	{
		nugget->deleteOK = true;
		nugget->deathOp = *((U32 *)buffer);
	}
	else
	{
		CQBOMB1("Received NUGGETDEATH for dead nugget 0x%X.", nuggetID);
	}
}
//--------------------------------------------------------------------------//
//
void NuggetManager::ReceiveNuggetData (void * buffer, U32 bufferSize)
{
	CreateSync * createSync = (CreateSync *) buffer;
	CQASSERT(bufferSize == sizeof(CreateSync));

	AnimNugget * nugget = (AnimNugget *)FindNugget(createSync->dwMissionID);
	if(nugget)
	{
		CQBOMB1("Received duplicate create packet for nugget 0x%X", createSync->dwMissionID);
	}
	else
	{
		Vector pos = createSync->position;
		pos.x += rand()%1000-500;
		pos.y += rand()%1000-500;
		pos.z += rand()%1000-500;
		Vector stop_pos = pos;
		stop_pos.x += rand()%400-200;
		stop_pos.y += rand()%400-200;
		stop_pos.z = 700+rand()%500;

		createNugget(ARCHLIST->LoadArchetype(createSync->archeID), createSync->position.systemID, stop_pos, createSync->supplyValue, 60*5, createSync->dwMissionID, false);
	}
}
//--------------------------------------------------------------------------//
//
void NuggetManager::Save(IFileSystem * outFile)
{
	outFile->SetCurrentDirectory("\\");
	U32 dwWritten;

	COMPTR<IFileSystem> file;

	DAFILEDESC fdesc;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	
	fdesc.lpFileName = "NuggetManager";

	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		return;

	AnimNugget * nugget;
	U32 num = 0;

	U32 count;
	for(count = 0; count < MAX_SYSTEMS;++count)
	{
		nugget = nuggetLists[count];
		while(nugget)
		{
			++num;
			nugget = nugget->nextNugget;
		}
	}
	file->WriteFile(0,&num,sizeof(num),&dwWritten);

	for(count = 0; count < MAX_SYSTEMS;++count)
	{
		nugget = nuggetLists[count];
		while(nugget)
		{
			U32 archID = ARCHLIST->GetArchetypeDataID(nugget->pArchetype);
			file->WriteFile(0,&archID,sizeof(archID),&dwWritten);

			BASE_ANIMNUGGET_SAVELOAD * saveload = nugget;
			file->WriteFile(0,saveload,sizeof(BASE_ANIMNUGGET_SAVELOAD),&dwWritten);
			num = nugget->GetVisibilityFlags();
			file->WriteFile(0,&num,sizeof(U32),&dwWritten);
			nugget = nugget->nextNugget;
		}
	}
}
//--------------------------------------------------------------------------//
//
void NuggetManager::Load(IFileSystem * inFile)
{
	inFile->SetCurrentDirectory("\\");
	DAFILEDESC fdesc = "NuggetManager";
	COMPTR<IFileSystem> file;
	U32 dwRead;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		return;

	U32 num, i;
	file->ReadFile(0,&num,sizeof(num),&dwRead);
	for(i = 0; i < num; ++i)
	{
		U32 archID;
		file->ReadFile(0,&(archID),sizeof(archID),&dwRead);
		BASE_ANIMNUGGET_SAVELOAD saveload;
		file->ReadFile(0,&(saveload),sizeof(saveload),&dwRead);

		AnimNugget *obj= (AnimNugget *) (MGlobals::CreateInstance(ARCHLIST->LoadArchetype(archID), saveload.dwMissionID));
		obj->InitNugget(saveload.dwMissionID,saveload.systemID, saveload.position,saveload.supplyPointsMax,saveload.lifeTimeRemaining, saveload.bRealized);

		obj->nextNugget = nuggetLists[saveload.systemID-1];
		nuggetLists[saveload.systemID-1] = obj;

		BASE_ANIMNUGGET_SAVELOAD * otherObj = obj;
		(*otherObj) = saveload;
		obj->bRealized = true;

		U32 visFlags;
		file->ReadFile(0,&visFlags,sizeof(visFlags),&dwRead);

		obj->SetJustVisibilityFlags(visFlags);
	}
}
//--------------------------------------------------------------------------//
//
void NuggetManager::init()
{
	COMPTR<IDAConnectionPoint> connection;
	
	if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
	{
		CQASSERT(eventHandle==0);
		connection->Advise(getBase(), &eventHandle);
	}
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _nuggetManager : GlobalComponent
{
	NuggetManager * nManager;

	virtual void Startup (void)
	{
		NUGGETMANAGER = nManager = new DAComponent<NuggetManager>;
		AddToGlobalCleanupList((IDAComponent **) &nManager);
	}

	virtual void Initialize (void)
	{
		nManager->init();
	}
};

static _nuggetManager nuggetManager;
//------------------------------------------------------------------------------------------
//---------------------------Nugget Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE NuggetFactory : public IObjectFactory
{
	struct OBJTYPE : NUGGET_INIT
	{
		
		void * operator new (size_t size)
		{
			return calloc(size,1);
		}
		void   operator delete (void *ptr)
		{
			::free(ptr);
		}

		OBJTYPE (void)
		{
			archIndex = -1;
			rigidBodyArm = Vector(0,0,0);
		}

		~OBJTYPE (void)
		{
			if (archIndex != -1)
				ENGINE->release_archetype(archIndex);
			if (animArch)
				delete animArch;
		}
	};

	U32 factoryHandle;		// handles to callback
	U32 eventHandle;
	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(NuggetFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	NuggetFactory (void) { }

	~NuggetFactory (void);

	void init (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IObjectFactory methods */

	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);

	/* NuggetFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
NuggetFactory::~NuggetFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);

}
//--------------------------------------------------------------------------//
//
void NuggetFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);

}
//-----------------------------------------------------------------------------
//
HANDLE NuggetFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_NUGGET)
	{
		BT_NUGGET_DATA * data = (BT_NUGGET_DATA *) _data;
		result = new OBJTYPE;
		result->pData = data;
		result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		
		{
			COMPTR<IFileSystem> file;
			DAFILEDESC fdesc = data->nugget_anim2D;
			
			fdesc.lpImplementation = "UTF";
			
			if (OBJECTDIR->CreateInstance(&fdesc, file) != GR_OK)
			{
				CQFILENOTFOUND(fdesc.lpFileName);
				goto Error;
			}

			if(data->nugget_mesh[0])
			{
				result->animArch = 0;
				DAFILEDESC fdesc = data->nugget_mesh;
				COMPTR<IFileSystem> objFile;

				if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
					TEXLIB->load_library(objFile, 0);
				else
				{
					CQFILENOTFOUND(data->nugget_mesh);
					goto Done;
				}

				if ((result->archIndex = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX)
					goto Error;
			}
			else
			{
				result->archIndex = INVALID_ARCHETYPE_INDEX;
				if ((result->animArch = ANIM2D->create_archetype(file)) == 0)
					goto Error;
			}
		}
		
		goto Done;
	}

Error:
	delete result;
	result = 0;
Done:
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 NuggetFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * NuggetFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createAnimNugget(*objtype);
}
//-------------------------------------------------------------------
//
void NuggetFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}

//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _nugget : GlobalComponent
{
	NuggetFactory * nugget;

	virtual void Startup (void)
	{
		nugget = new DAComponent<NuggetFactory>;
		AddToGlobalCleanupList((IDAComponent **) &nugget);
	}

	virtual void Initialize (void)
	{
		nugget->init();
	}
};

static _nugget nugget;
//---------------------------------------------------------------------------
//--------------------------End Nugget.cpp--------------------------------
//---------------------------------------------------------------------------
