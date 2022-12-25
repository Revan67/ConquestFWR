//--------------------------------------------------------------------------//
//                                                                          //
//                             Dust.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/Dust.cpp 5     10/15/00 9:05p Jasony $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>
#include "Sector.h"
#include "Camera.h"
#include "FogOfWar.h"
#include "TerrainMap.h"
#include "UserDefaults.h"

#include "IDust.h"
#include "Startup.h"
#include <IConnection.h>
#include <TSmartPointer.h>
#include <DPlatform.h>
#include <renderer.h>

//--------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE Dust : public IDust
{
	BEGIN_DACOM_MAP_INBOUND(Dust)
	DACOM_INTERFACE_ENTRY(IDust)
	END_DACOM_MAP()

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
	
	U32 timer;

	Dust();

	~Dust();

	/* IDust */

	virtual void Render();

	void init();
};
//--------------------------------------------------------------------------//
//
Dust::Dust()
{
};
//--------------------------------------------------------------------------//
//

Dust::~Dust()
{

};

//---------------------------------------------------------------------------
//
void Dust::Render()
{
	timer = timer+10;
	//find four courners
	Vector vec[4];
	vec[0] = Vector(0,0,0);
	CAMERA->ScreenToPoint(vec[0].x,vec[0].y,0);
	vec[1] = Vector(IDEAL2REALX(639),0,0);
	CAMERA->ScreenToPoint(vec[1].x,vec[1].y,0);
	vec[2] = Vector(IDEAL2REALX(639),IDEAL2REALY(479),0);
	CAMERA->ScreenToPoint(vec[2].x,vec[2].y,0);
	vec[3] = Vector(0,IDEAL2REALY(479),0);
	CAMERA->ScreenToPoint(vec[3].x,vec[3].y,0);

	//this is no optimal but find the grid aligned square that fits.
	SINGLE maxX,maxY,minX,minY;
	if(vec[0].x > vec[1].x)
	{
		maxX = vec[0].x;
		minX = vec[1].x;
	}
	else
	{
		maxX = vec[1].x;
		minX = vec[0].x;
	}
	if(maxX < vec[2].x)
		maxX = vec[2].x;
	else if(minX > vec[2].x)
		minX = vec[2].x;
	if(maxX < vec[3].x)
		maxX = vec[3].x;
	else if(minX > vec[3].x)
		minX = vec[3].x;

	if(vec[0].y > vec[1].y)
	{
		maxY = vec[0].y;
		minY = vec[1].y;
	}
	else
	{
		maxY = vec[1].y;
		minY = vec[0].y;
	}
	if(maxY < vec[2].y)
		maxY = vec[2].y;
	else if(minY > vec[2].y)
		minY = vec[2].y;
	if(maxY < vec[3].y)
		maxY = vec[3].y;
	else if(minY > vec[3].y)
		minY = vec[3].y;

    bool bVisRule = DEFAULTS->GetDefaults()->bVisibilityRulesOff || DEFAULTS->GetDefaults()->bEditorMode;

	S32 maxGridX = maxX/GRIDSIZE;
	S32 minGridX = minX/GRIDSIZE;
	S32 maxGridY = maxY/GRIDSIZE;
	S32 minGridY = minY/GRIDSIZE;
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(SECTOR->GetCurrentSystem(),map);
	if(!map)
		return;

	BATCH->set_state(RPR_BATCH,false);
	DisableTextures();
	CAMERA->SetModelView();
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	PB.Begin(PB_POINTS);
	for(S32 gridX = minGridX; gridX <= maxGridX; ++gridX)
	{
		for(S32 gridY = minGridY; gridY <= maxGridY; ++gridY)
		{
			Vector pos = Vector(gridX*GRIDSIZE+(GRIDSIZE/2),gridY*GRIDSIZE+(GRIDSIZE/2),0);
			GRIDVECTOR gridVect;
			gridVect = pos;
			BOOL32 bOutOfSys = !(map->IsGridInSystem(gridVect));
			if(bVisRule || bOutOfSys || FOGOFWAR->CheckHardFogVisibility(MGlobals::GetThisPlayer(),SECTOR->GetCurrentSystem(),pos))
			{
				U8 twinkle;
				BOOL32 bFow = bVisRule||FOGOFWAR->CheckVisiblePosition(pos);
				if(bOutOfSys)
					PB.Color3ub(255,80,80);
				else if(bFow)
				{
					twinkle = 100+((gridX*234+gridY*2344+4534+timer)%150);
					PB.Color3ub(twinkle,twinkle,255);
				}
				else
					PB.Color3ub(80,80,140);
				PB.Vertex3f(gridX*GRIDSIZE+((gridX*12200+gridY*23220)%GRIDSIZE),gridY*GRIDSIZE+((gridX*12342+gridY*23454)%GRIDSIZE),((gridX*23253+gridY*48739)%500)-400 );
				if(bOutOfSys)
					PB.Color3ub(255,80,80);
				else if(bFow)
				{
					twinkle = 100+((gridX*234+gridY*4534+432+timer)%150);
					PB.Color3ub(twinkle,twinkle >> 1,255);
				}
				else
					PB.Color3ub(80,80,140);
				PB.Vertex3f(gridX*GRIDSIZE+((gridX*42000+gridY*22420)%GRIDSIZE),gridY*GRIDSIZE+((gridX*14654+gridY*25443)%GRIDSIZE),((gridX*23432+gridY*32479)%500)-400 );
				if(bOutOfSys)
					PB.Color3ub(255,80,80);
				else if(bFow)
				{
					twinkle = 100+((gridX*454+gridY*3453+342+timer)%150);
					PB.Color3ub(twinkle>>1,twinkle,255);
				}
				else
					PB.Color3ub(80,80,140);
				PB.Vertex3f(gridX*GRIDSIZE+((gridX*54500+gridY*22443)%GRIDSIZE),gridY*GRIDSIZE+((gridX*14564+gridY*12343)%GRIDSIZE),((gridX*54131+gridY*23487)%500)-400 );

				if(bFow && !bOutOfSys)
				{
					twinkle = 100+((gridX*3543+gridY*3523+8934+timer)%150);
					PB.Color3ub(twinkle>>1,twinkle>>1,255);
					PB.Vertex3f(gridX*GRIDSIZE+((gridX*14040+gridY*22340)%GRIDSIZE),gridY*GRIDSIZE+((gridX*45660+gridY*35444)%GRIDSIZE),((gridX*54378+gridY*13497)%500)-400 );
					twinkle = 100+((gridX*5454+gridY*2324+3234+timer)%150);
					PB.Color3ub(twinkle,twinkle,255);
					PB.Vertex3f(gridX*GRIDSIZE+((gridX*10340+gridY*22334)%GRIDSIZE),gridY*GRIDSIZE+((gridX*34554+gridY*35456)%GRIDSIZE),((gridX*45490+gridY*43478)%500)-400 );
					twinkle = 100+((gridX*2134+gridY*2432+234+timer)%150);
					PB.Color3ub(0,twinkle,255);
					PB.Vertex3f(gridX*GRIDSIZE+((gridX*23243+gridY*33355)%GRIDSIZE),gridY*GRIDSIZE+((gridX*34543+gridY*54332)%GRIDSIZE),((gridX*45323+gridY*54323)%500)-400 );
					twinkle = 100+((gridX*245+gridY*25345+5245+timer)%150);
					PB.Color3ub(twinkle,0,255);
					PB.Vertex3f(gridX*GRIDSIZE+((gridX*25452+gridY*53234)%GRIDSIZE),gridY*GRIDSIZE+((gridX*23454+gridY*34524)%GRIDSIZE),((gridX*46634+gridY*24543)%500)-400 );
					twinkle = 100+((gridX*2454+gridY*3543+2454+timer)%150);
					PB.Color3ub(twinkle,twinkle,255);
					PB.Vertex3f(gridX*GRIDSIZE+((gridX*54324+gridY*34534)%GRIDSIZE),gridY*GRIDSIZE+((gridX*23454+gridY*56324)%GRIDSIZE),((gridX*64245+gridY*24433)%500)-400 );
				}
			}
		}
	}
	PB.End();
}
//--------------------------------------------------------------------------//
//
void Dust::init()
{
	timer = 0;
}
//--------------------------------------------------------------------------//
//
struct _dustMang : GlobalComponent
{
	Dust * dust;

	virtual void Startup (void)
	{
		DUSTMANAGER = dust = new DAComponent<Dust>;
		AddToGlobalCleanupList(&dust);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		dust->init();
	}
};

static _dustMang __dustMang;


//---------------------------------------------------------------------------
//------------------------End Dust.cpp----------------------------------------
//---------------------------------------------------------------------------


