//--------------------------------------------------------------------------//
//                                                                          //
//                               Cloud.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $

    $Header: /Conquest/App/Src/cloud.cpp 230   10/18/00 5:25p Rmarr $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include <stdio.h>

#include "TDocClient.h"
#include "Menu.h"
#include "Resource.h"
#include "DBHotkeys.h"
#include "SysMap.h"
#include "UserDefaults.h"
#include "Camera.h"
#include "ANIM2D.h"
#include "lightman.h"
#include "IObject.h"
#include "TObject.h"
#include "Objlist.h"
#include "sector.h"
#include "Startup.h"
#include "TResclient.h"
#include "DPlanet.h"
#include "DNebula.h"
#include "DMineField.h"
#include "DEffectOpts.h"
//replace when an actual mission object is available
//#include "MPart.h"
#include "TObjFrame.h"
#include "TObjTrans.h"
#include "TObjMission.h"
#include "Field.h"
#include "CQLight.h"
#include "Mission.h"
#include "MyVertex.h"
#include "BRadix.h"
#include "SFX.h"
#include "Minefield.h"
#include "DrawAgent.h"
#include "MGlobals.h"
#include "AntiMatter.h"
#include "ICamera.h"
#include "FogofWar.h"
#include "MPart.h"
#include "GridVector.h"
#include "INugget.h"
#include "DNugget.h"
#include "RandomGen.h"
#include <DQuickSave.h>
#include "IExplosion.h"
#include "IBlast.h"
#include "CQBatch.h"
#include "ObjMap.h"
#include "EffectPlayer.h"

#include <WindowManager.h>
#include <renderer.h>
#include <mesh.h>
#include <3DMath.h>
#include <TComponent.h>
#include <FileSys.h>
#include <TSmartPointer.h>
#include <Heapobj.h>
#include <EventSys.h>
#include <RendPipeline.h>
//#include <RPUL\PrimitiveBuilder.h>

#include <malloc.h>
//#include <stdlib.h>
//#include <dplay.h>

#define ROAM 5000

#define FTS FIELD_TILE_SIZE
#define HFTS (FTS/2)
#define SSPEED 1024
#define ORIGIN 10000
#define EXTENT 40000
#define MID ((ORIGIN+EXTENT)*0.5)
#define LOW 1300
#define HIGH 1450
#define INSET (FIELD_TILE_SIZE*0.25)
#define INSETRATIO (SINGLE(INSET)/SINGLE(HFTS-INSET))
#define SPARK_LIFE 0.6
#define SPARK_FPS (8.0/SPARK_LIFE)
//#define NUM_SPARKS 40

#define MAX_ALPHA 110
#define MID_ALPHA 65

#define ROID_FADE 1.0f

static U32 NEBTEXTUREUSED=0;
static U32 FIELDTEXTUREUSED=0;
static U32 ANTIMATTERTEXMEMUSED=0;
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//


IField::~IField (void)
{
	if(factory)
		factory->DeleteField(this);
	factory = 0;
}

////////////////////

struct Corner
{
	SINGLE u,v;
	SINGLE height;
	bool inset;
	U8 alpha;
};



struct Square 
{
	S32 x,y;
	Corner c[3][3];
	SINGLE shftX,shftY;
};

struct SparkZone : XYCoord
{
};

struct Spark
{
	AnimInstance inst;
	SINGLE timeToLive;
	SINGLE curAngle;
	BOOL32 alive:1;
	BOOL32 launched:1;
	U32 zone;
};

struct Cloudzone
{
	RECT r;
	SINGLE alpha[9];
};

template DefaultArchetype<BT_MINEFIELD_DATA>;

template <class Type>
DefaultArchetype<Type>::DefaultArchetype()
{
	anchorX = ANCHOR_OFF;
	
	const char *fname = "edit.tga";
	textureID = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAA4);
}

template <class Type>
DefaultArchetype<Type>::~DefaultArchetype()
{
	TMANAGER->ReleaseTextureRef(textureID);
	textureID = 0;
}

template <class Type>
void DefaultArchetype<Type>::Notify(U32 message, void *param)
{
	MSG *msg = (MSG *) param;
	
	switch (message)
	{
	case WM_LBUTTONDOWN:
		OnLButtonDown();
		break;
	case WM_LBUTTONUP:
		OnLButtonUp();
		break;
	case WM_MOUSEMOVE:
		mouseX = LOWORD(msg->lParam);
		mouseY = HIWORD(msg->lParam);
		break;
	}
}

template <class Type>
void DefaultArchetype<Type>::OnLButtonDown()
{
//	S32 intx,inty;
	Vector vec;
	anchorX = mouseX;
	anchorY = mouseY;
	if (!snapping)
	{
//		vec.x = anchorX;
//		vec.y = anchorY;
//		CAMERA->ScreenToPoint(vec.x, vec.y, 0);
	//	intx = (S32)vec.x;
	//	inty = (S32)vec.y;
	//	snapX = intx % HFTS;
	//	snapY = inty % HFTS;
		snapX = snapY = 0;
		snapping = TRUE;
	}
}

template <class Type>
void DefaultArchetype<Type>::OnLButtonUp()
{
	Vector vec,vec2;
	S32 intx,inty;
	
	if (!laidSquare)
	{
		laidSquare = TRUE;
		numSquares = 0;
	}
	
	if (numSquares != MAX_SQUARES)
	{
		vec.x = anchorX;
		vec.y = anchorY;
		CAMERA->ScreenToPoint(vec.x, vec.y, 0);
		
		S32 intx2,inty2;
		vec2.x = mouseX;
		vec2.y = mouseY;
		CAMERA->ScreenToPoint(vec2.x, vec2.y, 0);
		
		vec.x = floor(vec.x/FTS)*FTS+FTS*0.5;// + snapX;
		vec.y = floor(vec.y/FTS)*FTS+FTS*0.5;// + snapY;
			
		vec2.x = floor(vec2.x/FTS)*FTS+FTS*0.5;// + snapX;
		vec2.y = floor(vec2.y/FTS)*FTS+FTS*0.5;// + snapY;
		
		intx = (S32)vec.x;
		inty = (S32)vec.y;
		intx2 = (S32)vec2.x;
		inty2 = (S32)vec2.y;
		
		S32 incX=1,incY=1;
		
		if (intx2-intx != 0)
		{
			incX = (intx2-intx)/abs(intx2-intx);
		}
		if (inty2-inty != 0)
		{
			incY = (inty2-inty)/abs(inty2-inty);
		}
		
		for (int cx=intx;incX*cx<=incX*intx2;cx+=incX*FTS)
		{
			for (int cy=inty;incY*cy<=incY*inty2;cy+=incY*FTS)
			{
				if (numSquares != MAX_SQUARES)
				{
					bool bFound = 0;
					for (unsigned int n=0;n<numSquares;n++)
					{
						if (squares[n].x == cx && squares[n].y == cy)
							bFound = 1;
					}

					if (!bFound)
					{
						squares[numSquares].x = cx;
						squares[numSquares].y = cy;
					
						numSquares++;
					}
				}
			}
		}
	}
	anchorX = ANCHOR_OFF;
}

template <class Type>
void DefaultArchetype<Type>::EndEdit()
{	
	if (laidSquare == 0)
		return;
	
	laidSquare = FALSE;
	
	IField *obj;
	
	
	obj = (IField *)ARCHLIST->CreateInstance(pArchetype);
	if (obj==0)
		return;

	MPartNC(obj)->dwMissionID = MGlobals::CreateNewPartID(0);
	_ltoa(MPartNC(obj)->dwMissionID,MPartNC(obj)->partName,16);
	OBJLIST->AddPartID(obj,obj->GetPartID());

	obj->Setup();//squares,numSquares);
	
	OBJLIST->AddObject(obj);

	obj->PlaceBaseNuggets();
	SYSMAP->InvalidateMap(SECTOR->GetCurrentSystem());
}

template <class Type>
void DefaultArchetype<Type>::Edit()
{
	snapping = FALSE;
	numSquares = 0;
}

template <class Type>
void DefaultArchetype<Type>::RenderEdit()
{
	Vector vec,vec2;
	U32 i;
	S32 intx,inty,intx2,inty2;
	
	vec.x = mouseX;
	vec.y = mouseY;
	BATCH->set_state(RPR_BATCH,FALSE);
	DisableTextures();
	
	CAMERA->SetPerspective();
	CAMERA->SetModelView();
	BATCH->set_render_state(D3DRS_ZENABLE,0);
	
	
	if (CAMERA->ScreenToPoint(vec.x, vec.y, 0) != 0)
	{
		vec2 = vec;
		if (1)//snapping)
		{
			vec.x = floor(vec.x/FTS)*FTS+FTS*0.5;// + snapX;
			vec.y = floor(vec.y/FTS)*FTS+FTS*0.5;// + snapY;
			vec2 = vec;
			if (anchorX != ANCHOR_OFF)
			{
				//Always "snapping" by this point
				vec2.x = anchorX;
				vec2.y = anchorY;
				CAMERA->ScreenToPoint(vec2.x, vec2.y, 0);
				vec2.x = floor(vec2.x/FTS)*FTS+FTS*0.5;// + snapX;
				vec2.y = floor(vec2.y/FTS)*FTS+FTS*0.5;// + snapY;
			}
		}
		
		
		
		intx = (S32)vec.x;
		inty = (S32)vec.y;
		intx2 = (S32)vec2.x;
		inty2 = (S32)vec2.y;
		S32 incX=1,incY=1;
		
		if (intx2-intx != 0)
		{
			incX = (intx-intx2)/abs(intx2-intx);
		}
		if (inty2-inty != 0)
		{
			incY = (inty-inty2)/abs(inty2-inty);
		}
		
		PB.Color3ub(255,255,255);
		
		PB.Begin(PB_LINES);
		for (int cx=intx2;incX*cx<=incX*intx;cx+=incX*2*HFTS)
		{
			for (int cy=inty2;incY*cy<=incY*inty;cy+=incY*2*HFTS)
			{
				
				
				PB.Vertex3f(cx-HFTS,cy-HFTS,0);
				PB.Vertex3f(cx+HFTS,cy-HFTS,0);
				
				PB.Vertex3f(cx-HFTS,cy-HFTS,0);
				PB.Vertex3f(cx-HFTS,cy+HFTS,0);
				
				PB.Vertex3f(cx-HFTS,cy+HFTS,0);
				PB.Vertex3f(cx+HFTS,cy+HFTS,0);
				
				PB.Vertex3f(cx+HFTS,cy-HFTS,0);
				PB.Vertex3f(cx+HFTS,cy+HFTS,0);
				
				
			}
		}
		
		PB.End(); //PB_LINES
	}
	
	//Render happy faces
	//		BATCH->set_texture_stage_texture( 0,textureID);
	SetupDiffuseBlend(textureID,TRUE);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	PB.Begin(PB_QUADS);
	for (i=0;i<numSquares;i++)
	{
		XYCoord *sq = &squares[i];
		
		
		SINGLE RVAL,GVAL,BVAL;
		
		RVAL = 1.0;
		GVAL = 1.0;
		BVAL = 1.0;
		
		Vector v[3],n[3];
		
		PB.TexCoord2f(0,0);		PB.Vertex3f(sq->x-HFTS,sq->y+HFTS,0);
		PB.TexCoord2f(1,0);		PB.Vertex3f(sq->x+HFTS,sq->y+HFTS,0);
		PB.TexCoord2f(1,1);		PB.Vertex3f(sq->x+HFTS,sq->y-HFTS,0);
		PB.TexCoord2f(0,1);		PB.Vertex3f(sq->x-HFTS,sq->y-HFTS,0);
		
	}
	
	PB.End(); // PB_TRIANGLES
	
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
}

template <class Type>
void DefaultArchetype<Type>::SetUpPosition(S32 * arrX, S32 * arrY, U32 numPoints)
{
	CQASSERT(numPoints < MAX_SQUARES);
	numSquares = numPoints;
	for(U32 i = 0; i <numPoints; ++i)
	{
		squares[i].x = arrX[i];
		squares[i].y = arrY[i];
	}
}

struct AsteroidArchetype : DefaultArchetype<BT_ASTEROIDFIELD_DATA>
{
	//archetype stuff
	U32 texID,texID2,mapTex;//BG_tex;
	U32 softwareTexClearID,softwareTexFogID;
	AnimArchetype *animArch[MAX_ASTEROID_TYPES];

	AsteroidArchetype (void)
	{
	}

	~AsteroidArchetype (void)
	{
		for (int i=0;i<MAX_ASTEROID_TYPES;i++)
		{
			if (animArch[i])
				delete animArch[i];
		}
		
		TMANAGER->ReleaseTextureRef(texID);
		texID = 0;
			
		TMANAGER->ReleaseTextureRef(texID2);
		texID2 = 0;

		if(softwareTexClearID)
		{
			TMANAGER->ReleaseTextureRef(softwareTexClearID);
			softwareTexClearID = 0;
		}
		if(softwareTexFogID)
		{
			TMANAGER->ReleaseTextureRef(softwareTexFogID);
			softwareTexFogID = 0;
		}


	//	TMANAGER->ReleaseTextureRef(BG_tex);
	//	BG_tex = 0;
		
	}
};

struct NebulaArchetype : DefaultArchetype<BT_NEBULA_DATA>
{
	//archetype stuff
	unsigned int mapTex;

	IEffectHandle * cloudEffect;

	NebulaArchetype (void)
	{
		cloudEffect = NULL;
	}

	~NebulaArchetype (void)
	{
		if(cloudEffect)
		{
			EFFECTPLAYER->ReleaseEffect(cloudEffect);
			cloudEffect = NULL;
		}
	}
};

typedef AsteroidArchetype ASTEROIDFIELD_INIT;
typedef NebulaArchetype NEBULA_INIT;
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

struct Asteroid
{
	INSTANCE_INDEX idx;
	AnimInstance anim;
	Vector vel;
	SINGLE alphaVel;
	SINGLE alpha;
	SINGLE alphaTarget;
	U8 lastSquare;
};

struct AsteroidField : ObjectMission<
									ObjectTransform<
												   ObjectFrame<
															   IField, 
															   ASTEROIDFIELD_SAVELOAD,
															   ASTEROIDFIELD_INIT
															  >
												   >
									>,
									ISaveLoad, IQuickSaveLoad
{	
	BEGIN_MAP_INBOUND(AsteroidField)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IField)
	END_MAP()

	Square *squares;//[MAX_SQUARES];
	U32 numSquares;
	U32 numNuggets;
	Asteroid *roids;
	U32 *roid_sort;
	S32 numRoids;
	AsteroidArchetype *dArch;
	AnimArchetype *arch[MAX_ASTEROID_TYPES];//,*arch2;
	U32 texList[MAX_ASTEROID_TYPES];
	U8 numRoidTypes;
	RECT bounds;
	U32 texID,texID2;
//	U32 BG_tex;
	SINGLE angle;
	HSOUND hAmbientSound;
	U32 multiStages;
	SINGLE rotateSpeed;

	//culling vars
	S32 vx_min,vx_max,vy_min,vy_max;

	//map render stuff
	U32 mapTex;
	RPVertex *map_vert_list;
	U16 *map_id_list;
	int map_index_count,map_vert_count;
	SINGLE lastShift;

	//terrain map stuff
	GRIDVECTOR *gvec;//[MAX_SQUARES];

	int map_square;

	AsteroidField();
	~AsteroidField();

	// IBaseObject

	virtual void CastVisibleArea (void);

	virtual void DoUpdate (SINGLE dt);

	virtual void DrawBillboard (bool bTop);

	virtual void RenderBackground (void);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual void MapTerrainRender ();

	virtual void View (void);

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual SINGLE TestHighlight (const RECT & rect);	// set bHighlight if possible for any part of object to appear within rect,

	// ISaveLoad 

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file); 

	virtual void ResolveAssociations();

	/* IQuickSaveLoad methods */
	
	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void)
	{}

	//AsteroidField

	virtual BOOL32 Setup();//XYCoord *_squares,U32 _numSquares);
	
	virtual BOOL32 Init(HANDLE dArch);

	virtual void SetSystemID (U32 _systemID)
	{
		systemID = _systemID;
	}
	
	virtual BOOL32 ObjInField(U32 objSystemID,const Vector &pos);

	S32 GetSquareOfPos(Vector pos,U8 lastSquare=0);

 	virtual Vector GetCenterPos (void);

	virtual void SetFieldFlags(FIELDFLAGS &flags)
	{
		flags.bAsteroids = TRUE;
		flags.damagePerTwentySeconds = dArch->pData->attributes.damage*20.0f;
	}

	virtual void PlaceBaseNuggets();


	void setTerrainFootprint (struct ITerrainMap * terrainMap);
	void unsetTerrainFootprint (struct ITerrainMap * terrainMap);

	U32 hasSquareAt(S32 x, S32 y);

	void setupSoftwareQuads(U32 i,U8 fogBits);

	void drawCournerQuad(U8 bits,Vector * vec);
};

AsteroidField::AsteroidField (void)
{
	for (int i=0;i<MAX_ASTEROID_TYPES;i++)
	{
		texList[i] = -1;
	}

	multiStages = 0xffffffff;
}

AsteroidField::~AsteroidField (void)
{
	// object specific
/*	U32 i;
	for (i=0;i<numRoids;i++)
		ANIM2D->destroy_instance(roids[i]);*/
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(systemID, map);
	if(map)
		unsetTerrainFootprint(map);

	if (hAmbientSound)
		SFXMANAGER->CloseHandle(hAmbientSound);

	delete [] roids;
	delete [] roid_sort;

	if (map_vert_list)
		delete [] map_vert_list;
	if (map_id_list)
		delete [] map_id_list;

	if (OBJMAP)
		OBJMAP->RemoveObjectFromMap(this,systemID,map_square);

	delete [] squares;
	delete [] gvec;
}

void AsteroidField::DoUpdate (SINGLE dt)
{
	int i;
	
	for (i=0;i<numRoids;i++)
	{
		Vector newPos;
		newPos = roids[i].anim.GetPosition();
		bool bOnScreen = (newPos.x > vx_min && newPos.x < vx_max && newPos.y > vy_min && newPos.y < vy_max);
		if (roids[i].vel.x != 0 || roids[i].vel.y != 0)
		{
			if (roids[i].alphaVel != 0)
			{
				CQASSERT(roids[i].alphaVel >= -1.0);
				CQASSERT(roids[i].alphaVel < 6.0);
				roids[i].alpha += roids[i].alphaVel*dt;
				if (roids[i].alpha > roids[i].alphaTarget)
				{
					roids[i].alpha = roids[i].alphaTarget;
					roids[i].alphaVel = 0;
				}
				
				if (roids[i].alpha < 0)
				{
					roids[i].alpha = 0;
					roids[i].alphaVel = 0;
				}
				
				roids[i].anim.color.a = 255*roids[i].alpha;
			}
			
			if (bOnScreen || (rand()%8==0))
			{
				newPos = roids[i].anim.GetPosition()+roids[i].vel*dt;
				//	CQASSERT(roids[i].vel.z == 0 && roids[i].vel.x < 1000);
				
				S32 square;// = GetSquareOfPos(newPos);
				while ((square = GetSquareOfPos(newPos,roids[i].lastSquare)) < 0)
				{
					if (square == -1)
					{
						while ((square = GetSquareOfPos(newPos)) == -1)
						{
							newPos += roids[i].vel*2;  //go forward two full seconds at a time
						}
					}
					
					if (square == -2)
					{
						CQASSERT(roids[i].vel.x != 0 || roids[i].vel.y != 0);
						int side = rand()%2;
						
						switch (side)
						{
						case 0:
							newPos.x = bounds.left+10;
							newPos.y = bounds.top + rand()%(bounds.bottom-bounds.top);
							break;
						case 1:
							newPos.x = bounds.left + rand()%(bounds.right-bounds.left);
							newPos.y = bounds.top+10;
							break;
						}
					}
					roids[i].alphaVel = ROID_FADE*roids[i].alphaTarget;
				}
				
				roids[i].lastSquare = square;
				
				
				roids[i].anim.SetPosition(newPos);
				
			}

			if (roids[i].alphaVel >= 0)
			{
				Vector futurePos = roids[i].anim.GetPosition()+roids[i].vel/(ROID_FADE*roids[i].alphaTarget);
				if (GetSquareOfPos(futurePos) < 0)
				{
					roids[i].alphaVel = -ROID_FADE*roids[i].alphaTarget;
				}
			}
		}
		
		//if not on screen, don't bother to animate
		if (bOnScreen)
			roids[i].anim.update(dt);
	}

	angle += rotateSpeed*dt;
}

void AsteroidField::DrawBillboard(bool bTop)
{
	{
		if (CQRENDERFLAGS.bMultiTexture)
		{
			//	if (bTop)
			BATCH->set_texture_stage_texture( 0, texID2 );
			//	else
			//		BATCH->set_texture_stage_texture( 0, BG_tex);
			BATCH->set_texture_stage_texture( 1, texID );
			
			// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
			BATCH->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
			BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
			
			// filtering - bilinear with mips
			//	BATCH->set_sampler_state( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
			//	BATCH->set_sampler_state( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
			//	BATCH->set_sampler_state( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
			
			// addressing - clamped
			BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
			BATCH->set_sampler_state( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
			
			BATCH->set_texture_stage_state( 1, D3DTSS_TEXCOORDINDEX, 1);
			// blending - This is the same as D3DTMAPBLEND_MODULATEALPHA
			//if (bTop)
			//{
			BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			BATCH->set_texture_stage_state( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
			BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
			BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT );
			
			// filtering - bilinear with mips
			//		BATCH->set_sampler_state( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
			//		BATCH->set_sampler_state( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
			//		BATCH->set_sampler_state( 1, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
			
			// addressing - clamped
			BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
			BATCH->set_sampler_state( 1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
			//	}
			//	else
			//		BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
		}
		else
			multiStages = 0;
	}

	if (multiStages == 0xffffffff)
		multiStages = (BATCH->verify_state() == GR_OK);

	if (multiStages != 1)
	{
	//	if (bTop)
			SetupDiffuseBlend(texID,TRUE);
	//	else
	//		SetupDiffuseBlend(BG_tex,TRUE);
	}

//	BATCH->set_texture_stage_texture( 0,tex2);
	BATCH->set_render_state(D3DRS_DITHERENABLE,TRUE);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);

	SINGLE cosA = cos(2*angle*PI)*0.707;
	SINGLE sinA = sin(2*angle*PI)*0.707;

	PB.Color4ub(255,255,255,255);


	unsigned int c=0,drawn = 0;
	unsigned int limit = numSquares;
	if (bTop)
	{
		drawn = numSquares;
		int set = (drawn*3)/numSquares;
		c = set+(((drawn*3)%numSquares)/3)*3;
		limit = 2*numSquares;
	}
	
	BATCHDESC desc;
	desc.type = D3DPT_TRIANGLELIST;
	desc.vertex_format = D3DFVF_RPVERTEX2;
	desc.num_verts = (limit-drawn)*4;
	desc.num_indices = (limit-drawn)*6;
	CQBATCH->GetPrimBuffer(&desc);
	Vertex2 *v_list = (Vertex2 *)desc.verts;
	U16 *id_list = desc.indices;
	
	int v_cnt=0;
	int id_cnt=0;

	while (drawn < limit)
	{
		if (drawn == numSquares*0.5)
		{
			cosA = cos(-2*angle*PI)*0.707;
			sinA = sin(-2*angle*PI)*0.707;
		}
		
		Square *sq = &squares[c%numSquares];
		Vector epos;
		epos.set(sq->x+(drawn%6)*150-75,sq->y,(int)(-600+(drawn/numSquares)*2000+(drawn%8)*200));
		if (!bTop)
			epos.z -= 1000;
		else
			epos.z -= 500;
		drawn++;
		c += 3;
		if (c >= numSquares)
			c = c%3+1;
		
		Vector cpos (CAMERA->GetPosition());
		
		Vector look (epos - cpos);
		
		look.z = -50000;
		look.normalize();
		
		Vector i (look.y, -look.x, 0);
		
		if (fabs (i.x) < 1e-5 && fabs (i.y) < 1e-5)
		{
			i.x = 1.0f;
		}
		
		i.normalize ();
		Vector first_i = i;
		
		Vector k (-look);
		k.normalize ();
		Vector j (cross_product (k, i));
		
		SINGLE a;
		a=atan2(look.x,look.y);
		
		TRANSFORM trans;
		trans.set_i(i);
		trans.set_j(j);
		trans.set_k(k);
		trans.rotate_about_k(-a+PI*0.125*drawn);
		
		i = trans.get_i();
		j = trans.get_j();
		
		S32 size = 2800;
		if (!bTop)
			size = 1400;
		
		Vector v[4],v_flat[4];
		v[0] = epos - i*size - j*size;
		v[1] = epos + i*size - j*size;
		v[2] = epos + i*size + j*size;
		v[3] = epos - i*size + j*size;
		
		bool bOnScreen = FALSE;
		for (int cc=0;cc<4;cc++)
		{
			if (v[cc].x > vx_min && v[cc].x < vx_max && v[cc].y > vy_min && v[cc].y < vy_max)
				//	if (MAINCAM->object_visibility(v[cc],0))
			{
				bOnScreen = TRUE;
				//bust out of loop
				cc=4;
			}
		}
		
		if (bOnScreen)
		{
			for (int d=0;d<4;d++)
			{
				v_flat[d] = epos;
				v_flat[d].z = 0;
			}
			
			j.set(-first_i.y,first_i.x,0);
			trans.set_i(first_i);
			trans.set_j(j);
			trans.set_k(Vector(0,0,1));
			trans.rotate_about_k(-a+PI*0.125*drawn);
			
			i = trans.get_i();
			j = trans.get_j();
			
			size *= 0.707;
			v_flat[0] += -i*size-j*size;
			v_flat[1] += i*size-j*size;
			v_flat[2] += i*size+j*size;
			v_flat[3] += -i*size+j*size;
			
			SINGLE drift = 0.8*angle;

			v_list[v_cnt].pos = v[0];
			v_list[v_cnt].color = 0xffffffff;
			v_list[v_cnt].u = 0.0f+0.5f*(drawn%2);
			v_list[v_cnt].v = 0.0f+0.5f*((drawn%4)>1);
			v_list[v_cnt].u2 = drift+cosA+0.5;
			v_list[v_cnt].v2 = sinA+0.5;

			v_list[v_cnt+1].pos = v[1];
			v_list[v_cnt+1].color = 0xffffffff;
			v_list[v_cnt+1].u = 0.5f+0.5f*(drawn%2);
			v_list[v_cnt+1].v = 0.0f+0.5f*((drawn%4)>1);
			v_list[v_cnt+1].u2 = drift+sinA+0.5;
			v_list[v_cnt+1].v2 = -cosA+0.5;

			v_list[v_cnt+2].pos = v[2];
			v_list[v_cnt+2].color = 0xffffffff;
			v_list[v_cnt+2].u = 0.5f+0.5f*(drawn%2);
			v_list[v_cnt+2].v = 0.5f+0.5f*((drawn%4)>1);
			v_list[v_cnt+2].u2 = drift-cosA+0.5f;
			v_list[v_cnt+2].v2 = -sinA+0.5;

			v_list[v_cnt+3].pos = v[3];
			v_list[v_cnt+3].color = 0xffffffff;
			v_list[v_cnt+3].u = 0.0f+0.5f*(drawn%2);
			v_list[v_cnt+3].v = 0.5f+0.5f*((drawn%4)>1);
			v_list[v_cnt+3].u2 = drift-sinA+0.5;
			v_list[v_cnt+3].v2 = cosA+0.5;

			id_list[id_cnt] = v_cnt;
			id_list[id_cnt+1] = v_cnt+1;
			id_list[id_cnt+2] = v_cnt+2;
			id_list[id_cnt+3] = v_cnt;
			id_list[id_cnt+4] = v_cnt+2;
			id_list[id_cnt+5] = v_cnt+3;

			v_cnt +=4;
			id_cnt +=6;
		}
	}
	desc.num_verts = v_cnt;
	desc.num_indices = id_cnt;
	CQBATCH->ReleasePrimBuffer(&desc);
}

void AsteroidField::RenderBackground ()
{
	if (bVisible)
	{
		if(CQEFFECTS.bExpensiveTerrain==0)
		{
		}
		else
		{

	//		BT_ASTEROIDFIELD_DATA *data = dArch->pData;

			int i;
			Vector view;
		//	CAMERA->SetPerspective();

			BATCH->set_state(RPR_BATCH,FALSE);
			CAMERA->SetModelView();
			BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
			BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
			BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

		//	DrawBillboard(0);

			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ALPHATESTENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ALPHAREF,100);
			BATCH->set_render_state(D3DRS_ALPHAFUNC,D3DCMP_GREATER);

			/*BATCH->set_render_state(D3DRS_FOGENABLE, TRUE);
			BATCH->set_render_state(D3DRS_FOGCOLOR, D3DRGB(0.4,0.3,0.2));
			BATCH->set_render_state(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
			SINGLE start=10000;
			SINGLE end=18000;
			BATCH->set_render_state(D3DRS_FOGTABLESTART, *((DWORD*) &start));
			BATCH->set_render_state(D3DRS_FOGTABLEEND, *((DWORD*) &end));*/

			Vector cpos = CAMERA->GetPosition();
			int tx = 0;
			while (texList[tx] != -1)
			{
				ANIM2D->start_batch(texList[tx]);//roids[0]);
				for (i=0;i<numRoids;i++)
				{
					Vector rpos = roids[i].anim.GetPosition();
					roid_sort[i] = (rpos-cpos).magnitude_squared();
				}

				RadixSort::sort(roid_sort,numRoids);	

				for (i=0;i<numRoids;i++)
				{
				//	rpos.z = -i*20+1000;
				//	roids[i].anim.SetPosition(rpos);
					Asteroid *roid = &roids[RadixSort::index_list[i]];
				
					if (roid->anim.retrieve_current_frame()->texture == texList[tx])
					{
						Vector pos = roid->anim.GetPosition();
						if (pos.x > vx_min && pos.x < vx_max && pos.y > vy_min && pos.y < vy_max)
							ANIM2D->render_instance(&roid->anim);
					}
				}
				ANIM2D->end_batch();

				tx++;
			}

		//	CQASSERT(numPolyRoids==0 && "Poly roids are disabled pending a hardware revolution");
		}
	}
	BATCH->set_render_state(D3DRS_ALPHATESTENABLE,FALSE);
}

void AsteroidField::Render (void)
{
	if (bVisible && CQEFFECTS.bExpensiveTerrain)
	{
		SINGLE dt = OBJLIST->GetRealRenderTime();

		DoUpdate(dt);

		//BATCH->set_state(RPR_BATCH,TRUE);
		BATCH->set_state(RPR_BATCH,TRUE);
		BATCH->set_state(RPR_STATE_ID,0);
		CAMERA->SetModelView();
		BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		DrawBillboard(1);
	}
}

#define M_INT(x,y) /

//---------------------------------------------------------------------------------------
//
void AsteroidField::CastVisibleArea (void)
{
	SetVisibleToAllies(GetVisibilityFlags());  //sticky visibility flags - must saveload
}
//---------------------------------------------------------------------------
//
void AsteroidField::MapTerrainRender ()
{
	for(U32 i = 0; i < numSquares; ++i)
	{
		if(mapTex != -1)
			SYSMAP->DrawIcon(gvec[i],GRIDSIZE,mapTex);	
		else
			SYSMAP->DrawSquare(gvec[i],GRIDSIZE,RGB(dArch->pData->animColor.r,dArch->pData->animColor.g,dArch->pData->animColor.b));
	}
}

//---------------------------------------------------------------------------
//
void AsteroidField::View (void)
{
	ASTEROIDFIELD_DATA data;
	Vector vec;

	memset(&data, 0, sizeof(data));
	
	memcpy(data.name,name, sizeof(data.name));

	if (DEFAULTS->GetUserData("BT_ASTEROIDFIELD_DATA", " ", &data, sizeof(data)))
	{
		memcpy(name, data.name, sizeof(data.name));
	}
}
//---------------------------------------------------------------------------
//
void AsteroidField::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	bVisible = FALSE;
	if (GetSystemID() == currentSystem)
	{
		Vector tl,tr,br,bl;
		CAMERA->PaneToPoints(tl,tr,br,bl);
		const Transform *inverseWorldROT = CAMERA->GetWorldTransform();
		tl = inverseWorldROT->rotate_translate(tl);
		tr = inverseWorldROT->rotate_translate(tr);
		br = inverseWorldROT->rotate_translate(br);
		bl = inverseWorldROT->rotate_translate(bl);
		vx_min = min(tl.x,min(bl.x,min(tr.x,br.x)))-4000;
		vx_max = max(tl.x,max(bl.x,max(tr.x,br.x)))+4000;
		vy_min = min(tl.y,min(bl.y,min(tr.y,br.y)))-4000;
		vy_max = max(tl.y,max(bl.y,max(tr.y,br.y)))+4000;
		for (unsigned int i=0;i<numSquares;i++)
		{
			if (squares[i].x+HFTS > vx_min && squares[i].x-HFTS < vx_max)
			{
				if (squares[i].y+HFTS > vy_min && squares[i].y-HFTS < vy_max)
				{
					bVisible = TRUE;
				}
			}
		}
		//bVisible = TRUE;
	}
}
//---------------------------------------------------------------------------
//
SINGLE AsteroidField::TestHighlight (const RECT & rect)	// set bHighlight if possible for any part of object to appear within rect,
{
	bHighlight = 0;
	// don't highlight in lasso mode
	if (bVisible && rect.left==rect.right && rect.top==rect.bottom)
	{
		if (rect.left > 0)
		{
			Vector pos;
			
			pos.x = rect.left;
			pos.y = rect.top;
			pos.z = 0;
			
			CAMERA->ScreenToPoint(pos.x,pos.y,pos.z);
			
			if (FOGOFWAR->CheckHardFogVisibility(MGlobals::GetThisPlayer(), systemID, pos))
			{
				if (pos.x > bounds.left && pos.x < bounds.right)
				{
					if (pos.y > bounds.top && pos.y < bounds.bottom)
					{
						for (int i=0;i<(int)numSquares;i++)
						{
							SINGLE diffX = pos.x-squares[i].x;
							SINGLE diffY = pos.y-squares[i].y;
							if (fabs(diffX) < HFTS && fabs(diffY) < HFTS)
							{
								bHighlight = TRUE;
								factory->FieldHighlighted(this);
								
								/*	if (fabs(diffX) < HFTS-INSET && fabs(diffY) < HFTS-INSET)
								{
								return TRUE;
								}
								else
								{
								if (diffX < 0)
								{
								if (diffY < 0)
								TESTSQUARE(0,0,-1,-1)
								else
								TESTSQUARE(0,2,-1,1)
								}
								else
								{
								if (diffY < 0)
								TESTSQUARE(2,0,1,-1)
								else
								TESTSQUARE(2,2,1,1)
								}
							}*/
							}
						}
					}
				}
			}
		}
	}

	return 1e20;		// a large number
}
//---------------------------------------------------------------------------
//
/*BOOL32 Nebula::OnScreen()
{
	U32 i;
	BOOL32 onScreen = FALSE;
	Vector tl,tr,br,bl;
	CAMERA->PaneToPoints(tl,tr,br,bl);
	for (i=0;i<numSquares;i++)
	{
		if (squares[i].x+HFTS > min(bl.x,tr.x) && squares[i].x-HFTS < max(tl.x,br.x))
		{
			if (squares[i].y+HFTS > min(bl.y,tr.y) && squares[i].y-HFTS < max(tl.y,br.y))
			{
				onScreen = TRUE;
			}
		}
	}

	return onScreen;
}*/
//---------------------------------------------------------------------------
//
Vector AsteroidField::GetCenterPos (void)
{
	Vector vec(0,0,0);
	for (int i=0;i<(int)numSquares;i++)
	{
		vec.x+= squares[i].x;
		vec.y+= squares[i].y;
	}
	vec.x = vec.x/numSquares;
	vec.y = vec.y/numSquares;
	return vec;
}
//---------------------------------------------------------------------------
//
void AsteroidField::PlaceBaseNuggets()
{
	U32 validNuggets = 0;
	if(dArch->nuggetType[3])
		validNuggets = 4;
	else if(dArch->nuggetType[2])
		validNuggets = 3;
	else if(dArch->nuggetType[1])
		validNuggets = 2;
	else if(dArch->nuggetType[0])
		validNuggets = 1;
	if(validNuggets)
	{
		RandomGen randGen(((U16)dwMissionID >> 3) %255);
		numNuggets = dArch->pData->nuggetsPerSquare*numSquares;
		U32 nuggetID = 1;
		for(U32 i = 0 ; i < numNuggets; ++i)
		{
			CQASSERT((nuggetID != 0x00000080) && "Excessive Number of Nuggets in Nebula");
			U32 square = randGen.rand() % numSquares;
			Vector pos;
			pos.x = this->squares[square].x + GRIDSIZE*((randGen.rand()%1000)/1000.0) - (GRIDSIZE*0.5);
			pos.y = this->squares[square].y + GRIDSIZE*((randGen.rand()%1000)/1000.0) - (GRIDSIZE*0.5);
			pos.z = dArch->pData->nuggetZHeight;

			PARCHETYPE pArch = dArch->nuggetType[randGen.rand()%validNuggets];
			BT_NUGGET_DATA * nData = (BT_NUGGET_DATA *)(ARCHLIST->GetArchetypeData(pArch));
			NUGGETMANAGER->CreateNugget(pArch,systemID,pos,nData->maxSupplies,0,dwMissionID | (nuggetID << 24),false);
			++nuggetID;
		}
	}
}
//---------------------------------------------------------------------------
//
BOOL32 AsteroidField::Save(IFileSystem *inFile)
{
	DAFILEDESC fdesc;
	COMPTR<IFileSystem> file,file2;
	BOOL32 result = 0;

	DWORD dwWritten;

	ASTEROIDFIELD_SAVELOAD save;

	
	fdesc.lpFileName = "Data";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;
//	if (file->WriteFile(0, &numRoids, sizeof(numRoids), &dwWritten, 0) == 0)
//		goto Done;
	if (file->WriteFile(0, &numSquares, sizeof(numSquares), &dwWritten, 0) == 0)
		goto Done;
	if (file->WriteFile(0, &numNuggets, sizeof(numNuggets), &dwWritten, 0) == 0)
		goto Done;

	XYCoord *sq;
	sq = new XYCoord[numSquares];
	unsigned int i;
	for (i=0;i<numSquares;i++)
	{
		sq[i].x = squares[i].x;
		sq[i].y = squares[i].y;
	}

	if (file->WriteFile(0, sq, sizeof(XYCoord)*numSquares, &dwWritten, 0) == 0)
		goto Done;

	delete [] sq;

	fdesc.lpFileName = "ASTEROIDFIELD_SAVELOAD";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	//memcpy(&save, static_cast<ASTEROIDFIELD_SAVELOAD *>(this), sizeof(ASTEROIDFIELD_SAVELOAD));
	save.exploredFlags = GetVisibilityFlags();
	FRAME_save(save);

	if (file->WriteFile(0,&save,sizeof(save),&dwWritten,0) == 0)
		goto Done;


	file.free();
	
/*	fdesc.lpFileName = "Roids";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	int i;
	for (i=0;i<numRoids;i++)
	{
		Vector pos = roids[i].anim.GetPosition();
		if (file->WriteFile(0, &pos, sizeof(pos), &dwWritten, 0) == 0)
			goto Done;
	}*/

	
	result = 1;

Done:

	return result;

}

BOOL32 AsteroidField::Load(IFileSystem *inFile)
{
	ASTEROIDFIELD_SAVELOAD load;
	DAFILEDESC fdesc;
	COMPTR<IFileSystem> file,file2;
	BOOL32 result = 0;

	DWORD dwRead;
	Vector pos;

	fdesc.lpFileName = "Data";
	fdesc.lpImplementation = "DOS";
	
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;
	
	if (file->ReadFile(0, &numSquares, sizeof(numSquares), &dwRead, 0) == 0)
		goto Done;
	if (file->ReadFile(0, &numNuggets, sizeof(numNuggets), &dwRead, 0) == 0)
		goto Done;

	XYCoord *sq;
	sq = new XYCoord[numSquares];
	if (file->ReadFile(0, sq, sizeof(XYCoord)*numSquares, &dwRead, 0) == 0)
		goto Done;

	U8 buffer[1024];

	fdesc.lpFileName = "ASTEROIDFIELD_SAVELOAD";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
	{
		goto Done;
	}

	memset(buffer, 0, sizeof(buffer));
	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("ASTEROIDFIELD_SAVELOAD", buffer, &load);

	CQASSERT(sizeof(ASTEROIDFIELD_SAVELOAD) < 1024);

	FRAME_load(load);
	SetVisibleToAllies(load.exploredFlags);
	UpdateVisibilityFlags();


//SkipSystemID:

	dArch->numSquares = numSquares;
	memcpy(dArch->squares,sq,sizeof(XYCoord)*numSquares);
	Setup();//sq,numSquares);

	delete [] sq;

//	file.free();

/*	fdesc.lpFileName = "Roids";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;
	roids = new Asteroid[numRoids];
	memset(roids,0,sizeof(Asteroid)*numRoids);
	
	for (i=0;i<numRoids;i++)
	{
		roids[i].anim.Init(arch[rand()%numRoidTypes]);
		Vector pos;// = roids[i].GetPosition();
		if (file->ReadFile(0, &pos, sizeof(pos), &dwRead, 0) == 0)
			goto Done;
		roids[i].anim.SetPosition(pos);
		roids[i].anim.SetWidth(175+150*(SINGLE)rand()/RAND_MAX);
		roids[i].anim.Randomize();
	}*/

	SYSMAP->InvalidateMap(systemID);

	result = 1;

Done:

	return result;

}

//---------------------------------------------------------------------------
//
void AsteroidField::ResolveAssociations()
{
}

/* IQuickSaveLoad methods */

void AsteroidField::QuickSave (struct IFileSystem * file)
{
	DAFILEDESC fdesc = partName;
	HANDLE hFile;

	file->CreateDirectory("MT_QFIELDLOAD");
	if (file->SetCurrentDirectory("MT_QFIELDLOAD") == 0)
		CQERROR0("QuickSave failed on directory 'MT_QFIELDLOAD'");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR1("QuickSave failed on part '%s'", fdesc.lpFileName);
	}
	else
	{
		MT_QFIELDLOAD qload;
		DWORD dwWritten;

		qload.systemID = systemID;
		qload.numSquares = numSquares;
		for (unsigned int i=0;i<numSquares;i++)
		{
			qload.pos[i] = Vector(squares[i].x,squares[i].y,0);
		}

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//---------------------------------------------------------------------------
//
void AsteroidField::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_QFIELDLOAD qload;
	MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);

	SetSystemID(qload.systemID);
	numSquares = qload.numSquares;
	dwMissionID = MGlobals::CreateNewPartID(0);

	MGlobals::InitMissionData(this, dwMissionID);
	partName = szInstanceName;

	OBJLIST->AddPartID(this, dwMissionID);

	dArch->numSquares = numSquares;
	for (unsigned int i=0;i<numSquares;i++)
	{
		Vector pos = qload.pos[i];
		dArch->squares[i].x = pos.x;
		dArch->squares[i].y = pos.y;
	}
	Setup();
	PlaceBaseNuggets();
};

/* IField Methods */
BOOL32 AsteroidField::ObjInField(U32 objSystemID,const Vector &pos)
{
	if (objSystemID != systemID)
		return FALSE;

	if (pos.x > bounds.left && pos.x < bounds.right)
	{
		if (pos.y > bounds.top && pos.y < bounds.bottom)
		{
			for (int i=0;i<(int)numSquares;i++)
			{
				SINGLE diffX = pos.x-squares[i].x;
				SINGLE diffY = pos.y-squares[i].y;
				if (fabs(diffX) < HFTS && fabs(diffY) < HFTS)
				{
					return TRUE;
				/*	if (fabs(diffX) < HFTS-INSET && fabs(diffY) < HFTS-INSET)
					{
						return TRUE;
					}
					else
					{
						if (diffX < 0)
						{
							if (diffY < 0)
								TESTSQUARE(0,0,-1,-1)
							else
								TESTSQUARE(0,2,-1,1)
						}
						else
						{
							if (diffY < 0)
								TESTSQUARE(2,0,1,-1)
							else
								TESTSQUARE(2,2,1,1)
						}
					}*/
				}
			}
		}
	}

	return FALSE;
}

//----------------------
// -1 : not in field
// -2 : not in bounds
S32 AsteroidField::GetSquareOfPos(Vector pos,U8 lastSquare)
{
	//put little buffers on bounds for rounding error??
	if (pos.x >= bounds.left-2 && pos.x <= bounds.right+2)
	{
		if (pos.y >= bounds.top-2 && pos.y <= bounds.bottom+2)
		{
			SINGLE px = pos.x+HFTS;
			SINGLE py = pos.y+HFTS;
			int cnt=0;
			int i = lastSquare;
			while (cnt<(int)numSquares)
			{
				if (i>=(int)numSquares)
					i-= numSquares;

				SINGLE diffX = px-squares[i].x;
				SINGLE diffY = py-squares[i].y;
				if (diffX > 0 && diffX < FTS && diffY > 0 && diffY < FTS)
				{
					return i;
				}
				cnt++;
				i++;
			}

			return -1;
		}
	}

	return -2;
}

BOOL32 AsteroidField::Init(HANDLE hArchetype)
{
	dArch = (AsteroidArchetype *)hArchetype;
	BOOL32 result = 0;
	
	FRAME_init(*dArch);

	BT_ASTEROIDFIELD_DATA *data = dArch->pData;

	texID = dArch->texID;
	texID2 = dArch->texID2;
//	BG_tex = dArch->BG_tex;
	mapTex = dArch->mapTex;

	hAmbientSound = SFXMANAGER->Open(data->ambientSFX);
	infoHelpID = dArch->pData->infoHelpID;

	S32 texCnt=0;
	for (int i=0;i<MAX_ASTEROID_TYPES;i++)
	{
		if (dArch->animArch[i])
		{
			arch[numRoidTypes++] = dArch->animArch[i];
			
			bool newTex = 1;
			for (int t=0;t<texCnt;t++)
			{
				if (texList[t] == dArch->animArch[i]->frames[0].texture)
				{
					newTex = 0;
				}
			}
			
			if (newTex)
				texList[texCnt++] = dArch->animArch[i]->frames[0].texture;
		}
	}
	
	result = 1;
				
	//Done:

	return result;
}

BOOL32 AsteroidField::Setup()//struct XYCoord *_squares,U32 _numSquares)
{
	BOOL32 result = 0;
	COMPTR<ITerrainMap> map;
//	S32 depth = 0;
	int i,j;

	CQASSERT(systemID);
	SECTOR->GetTerrainMap(systemID, map);

	if (gvec)
		unsetTerrainFootprint(map);

	if (dArch->numSquares)//_numSquares)
	{
		numSquares = dArch->numSquares;//_numSquares;
	}
	
	squares = new Square[numSquares];
	gvec = new GRIDVECTOR[numSquares];
	for (i=0;i<(int)numSquares;i++)
	{
		squares[i].x = dArch->squares[i].x;//_squares[i].x;
		squares[i].y = dArch->squares[i].y;//_squares[i].y;
		gvec[i] = Vector(squares[i].x,squares[i].y,0);
	}

	setTerrainFootprint(map);
	
	BT_ASTEROIDFIELD_DATA *data = dArch->pData;
	
	numRoids = data->asteroidsPerSquare*numSquares;
	CQASSERT(data->polyroidsPerSquare == 0 && "Please talk to Rob if you feel you need to use these");
	
	//asteroids MUST be able to be returned to asteroid space
	if (data->minDriftSpeed < 100)
		data->minDriftSpeed = 100;
	
	Vector drift(1,1,0);
	if (numRoids && CQEFFECTS.bExpensiveTerrain)
	{
		roids = new Asteroid[numRoids];
		roid_sort = new U32[numRoids];
		memset(roids,0,sizeof(Asteroid)*numRoids);
		//Set up anim roids
		for (i=0;i<numRoids;i++)
		{
			roids[i].idx = INVALID_INSTANCE_INDEX;
			
			Vector dir = drift;
			dir.x += -0.9+(rand()%1800)*0.001;
			dir.y += -0.9+(rand()%1800)*0.001;
			dir.normalize();
			
			if (rand()%100 > data->stationaryPercentage*100)
			{
				roids[i].vel = dir*(data->minDriftSpeed+rand()%(data->maxDriftSpeed-data->minDriftSpeed));
			}
			else
				roids[i].vel.set(0,0,0);
			
			U32 cnt = rand()%numSquares;
			S32 x, y, z=0;
			
			x = squares[cnt].x + rand()%(2*HFTS)-HFTS;
			y = squares[cnt].y + rand()%(2*HFTS)-HFTS;
			if (data->range)
				z = (rand() % data->range) - (data->range / 2);
			
			
			Vector vec = Vector (x, y, data->depth+z);
			
		//	SINGLE alph = 0.5 + 0.5*(vec.z-data->depth+data->range*0.5)/data->range;
			
			roids[i].alpha = roids[i].alphaTarget = 1.0f;//alph;
			
			roids[i].anim.Init(arch[rand()%numRoidTypes]);
			roids[i].anim.SetPosition(vec);
		//	SINGLE num = 10*(SINGLE)rand()/RAND_MAX;
			S32 num = rand()%(data->animSizeMax-data->animSizeMin);
			roids[i].anim.SetWidth(dArch->pData->animSizeMin+num);
			roids[i].anim.Randomize();
			roids[i].anim.color.r = data->animColor.r;
			roids[i].anim.color.g = data->animColor.g;
			roids[i].anim.color.b = data->animColor.b;
			roids[i].anim.color.a = 255*roids[i].alpha;
			rotateSpeed = 0.005*data->modTexSpeedScale;
			//roids[i].alpha = 1.0;
			
			
			//roid->anim.SetColor(255,255,255,255*alph);
		}
	}
	
	U32 roidCnt,cnt;
	roidCnt = 0;
	
	for (cnt=0;cnt<numSquares;cnt++)
	{
		for (i=0;i<3;i++)
		{
			for (j=0;j<3;j++)
			{
				squares[cnt].c[i][j].inset = TRUE;
		//		squares[cnt].c[i][j].alpha = 1.0;//((SINGLE)(i%2+j%2)/2.0);
			}
		}
	}

/*	{
		for (i = 0; i < roidsPerSquare; i++)
		{
			S32 x, y, z=0;
			
			x = squares[cnt].x + rand()%(2*HFTS)-HFTS;
			y = squares[cnt].y + rand()%(2*HFTS)-HFTS;
			if (data->range)
				z = (rand() % data->range) - (data->range / 2);

			
			Vector vec = Vector (x, y, data->depth+z);

			SINGLE alph = 0.1 + 0.9*(vec.z-data->depth+data->range*0.5)/data->range;

			roids[roidCnt].idx = INVALID_INSTANCE_INDEX;
			roids[roidCnt].alpha = roids[roidCnt].alphaTarget = alph;
			
			roids[roidCnt].anim.Init(arch[rand()%numRoidTypes]);
			roids[roidCnt].anim.SetPosition(vec);
			SINGLE num = 20*(SINGLE)rand()/RAND_MAX;
			roids[roidCnt].anim.SetWidth(175+num*num);
			roids[roidCnt].anim.Randomize();
			roids[roidCnt].anim.color.a = 255*roids[roidCnt].alpha;
			roidCnt++;
		}

	
	}*/

	bounds.left = bounds.top = 9999999;
	bounds.right = bounds.bottom = -1;
	for (int k=0;k<(int)numSquares;k++)
	{
		if (squares[k].x-HFTS < bounds.left)
			bounds.left = squares[k].x-HFTS;
		if (squares[k].x+HFTS > bounds.right)
			bounds.right = squares[k].x+HFTS;
		if (squares[k].y-HFTS < bounds.top)
			bounds.top = squares[k].y-HFTS;
		if (squares[k].y+HFTS > bounds.bottom)
			bounds.bottom = squares[k].y+HFTS;

		squares[k].c[1][1].height = LOW;
		squares[k].c[1][1].alpha = MAX_ALPHA;
		for (i=0;i<3;i++)
		{
			for (j=0;j<3;j++)
			{
				if (j != 1 || i != 1)
				{
					squares[k].c[i][j].alpha = 0;
				}
				U8 score = 0;
				if (j == 1 || i == 1)
				{
					score = 2;
				}
				else
					score = 1;
				
				for (int l=0;l<(int)numSquares;l++)
				{
					if (k != l)
					{
						Square sq1 = squares[k];
						Square sq2 = squares[l];
						if (sq1.x-HFTS+HFTS*i <= sq2.x + HFTS && sq1.x-HFTS+HFTS*i >= sq2.x-HFTS)
						{
							
							if (sq1.y-HFTS+HFTS*j <= sq2.y + HFTS && sq1.y-HFTS+HFTS*j >= sq2.y-HFTS)
							{
								
								if (j == 1 || i == 1)
								{
									if (sq1.x == sq2.x || sq1.y == sq2.y)
									{
										score += 2;
									}
									else
										score++;
								}
								else
								{
									if (sq1.x == sq2.x || sq1.y == sq2.y)
									{
										score++;
									}
									else
									{
										if (abs(sq1.x - sq2.x) == HFTS || abs(sq1.y - sq2.y) == HFTS)
											score += 2;
										else
											score ++;
									}
									
								}

								if (score >= 4)
								{
									squares[k].c[i][j].alpha = MAX_ALPHA;
									squares[k].c[i][j].height = HIGH;
								}
								
								squares[k].c[i][j].inset = 0;
							}
						}
					}
				}
			}
		}
	}

	
	//set up map render
	S32 vert_count=0;
	map_vert_list = new RPVertex[9*numSquares];
	map_id_list = new U16[24*numSquares];
	S32 idx_count=0;
	for (cnt=0;cnt<numSquares;cnt++)
	{
		Square *sq = &squares[cnt];
		
		
		SINGLE RVAL,GVAL,BVAL;
		
		RVAL = 1.0;//0.4;
		GVAL = 1.0;//0.3;
		BVAL = 1.0;//0.1;
		
		S32 base_ref=vert_count;
		vert_count+=9;
		S32 cntr_ref=base_ref+4;
		S32 ref;// = vert_count++;
		map_vert_list[cntr_ref].pos.set(sq->x,sq->y,0);
		map_vert_list[cntr_ref].r = RVAL*sq->c[1][1].alpha;
		map_vert_list[cntr_ref].g = GVAL*sq->c[1][1].alpha;
		map_vert_list[cntr_ref].b = BVAL*sq->c[1][1].alpha;

		
#define NEB_SHAPE(x0,y0,x1,y1)\
		{\
		ref = base_ref+x0+y0*3;\
		map_id_list[idx_count++] = ref;\
		map_vert_list[ref].pos.set(sq->x+HFTS*(x0-1),sq->y+HFTS*(y0-1),0);\
		map_vert_list[ref].r = RVAL*sq->c[x0][y0].alpha;\
		map_vert_list[ref].g = GVAL*sq->c[x0][y0].alpha;\
		map_vert_list[ref].b = BVAL*sq->c[x0][y0].alpha;\
		ref = base_ref+x1+y1*3;\
		map_id_list[idx_count++] = ref;\
		map_vert_list[ref].pos.set(sq->x+HFTS*(x1-1),sq->y+HFTS*(y1-1),0);\
		map_vert_list[ref].r = RVAL*sq->c[x1][y1].alpha;\
		map_vert_list[ref].g = GVAL*sq->c[x1][y1].alpha;\
		map_vert_list[ref].b = BVAL*sq->c[x1][y1].alpha;\
		map_id_list[idx_count++] = cntr_ref;\
		}


	//	v[0].set(sq->x+HFTS*(x0-1),sq->y+HFTS*(y0-1),0);\
	//	v[1].set(sq->x+HFTS*(x1-1),sq->y+HFTS*(y1-1),0);\
	//	PB.Color3ub(RVAL*sq->c[x0][y0].alpha,GVAL*sq->c[x0][y0].alpha,BVAL*sq->c[x0][y0].alpha); \
	//	PB.Color3ub(RVAL*sq->c[x1][y1].alpha,GVAL*sq->c[x1][y1].alpha,BVAL*sq->c[x1][y1].alpha); \
	//	}
		
		NEB_SHAPE(1,0,0,0);
		NEB_SHAPE(0,1,0,0);
		NEB_SHAPE(2,1,2,0);
		NEB_SHAPE(1,0,2,0);
		NEB_SHAPE(2,1,2,2);
		NEB_SHAPE(1,2,2,2);
		NEB_SHAPE(0,1,0,2);
		NEB_SHAPE(1,2,0,2);
		
#undef NEB_SHAPE
	}

	map_index_count = idx_count;
	// for now
	map_vert_count = 9*numSquares;

	//assumes counter is initialized to 0
	for (int v=0;v<map_vert_count;v++)
	{
#define MAP_TEX_FACTOR 0.00005
		map_vert_list[v].u = map_vert_list[v].pos.x*MAP_TEX_FACTOR;
		map_vert_list[v].v = map_vert_list[v].pos.y*MAP_TEX_FACTOR;
	}

	//FIX THIS !!!
	Vector centerPos;
	centerPos.set(0.5*(bounds.right+bounds.left),0.5*(bounds.top+bounds.bottom),0);
	SFXMANAGER->Play(hAmbientSound,systemID,&centerPos,SFXPLAYF_LOOPING|SFXPLAYF_NOFOG);
	//for lights
	transform.translation = centerPos;

	result =1;

	map_square = OBJMAP->GetMapSquare(systemID,Vector(squares[0].x,squares[0].y,0));
	U32 flags = 0;
	objMapNode = OBJMAP->AddObjectToMap(this,systemID,map_square,flags);

//Done:
	return result;
}
//------------------------------------------------------------------------
//
U32 AsteroidField::hasSquareAt(S32 x, S32 y)
{
	for(U32 i = 0; i < numSquares; ++i)
	{
		if(squares[i].x == x && squares[i].y == y)
		{
			return 1;
		}
	}
	return 0;
}

void AsteroidField::setTerrainFootprint (struct ITerrainMap * terrainMap)
{
	FootprintInfo info;
	info.missionID = dwMissionID;
	info.height = 0;
	info.flags = TERRAIN_FIELD | TERRAIN_FULLSQUARE;
	terrainMap->SetFootprint(gvec,numSquares,info);
}
void AsteroidField::unsetTerrainFootprint (struct ITerrainMap * terrainMap)
{
	FootprintInfo info;
	info.missionID = dwMissionID;
	info.height = 0;
	info.flags = TERRAIN_FIELD | TERRAIN_FULLSQUARE;
	terrainMap->UndoFootprint(gvec,numSquares,info);
}

struct DACOM_NO_VTABLE Nebula : ObjectMission<
											  ObjectTransform<
															  ObjectFrame<
																		  IField,
																		  NEBULA_SAVELOAD,
																		  NEBULA_INIT
																		 >
															 >
											 >,
											 ISaveLoad, IQuickSaveLoad,
											 BASE_NEBULA_SAVELOAD
{

	//
	// incoming interface map
	//

	BEGIN_MAP_INBOUND(Nebula)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IField)
	END_MAP()


	NebulaArchetype *nArch;

	AnimArchetype *arch;
	NEBTYPE nebType;
	
	U32 menuID;

	CQLight light[2];
	U8 lzone[2];
	S32 offsetX[2],offsetY[2];
	SINGLE dirX[2],dirY[2];
	BOOL32 lActive[2];
	SINGLE timeToLive[2];
	SINGLE counter;
	SINGLE angle,shift;

	BOOL32 bCameraMoved:1;
	BOOL32 bHasSparks:1;
	
	Square *squares;
	IEffectInstance ** cloudEffects;
	
	RECT bounds;
	SINGLE updateTime;
	HSOUND hAmbientSound;
	U32 multiStages;

	//culling vars
	S32 vx_min,vx_max,vy_min,vy_max;

	//map render stuff
	U32 mapTex;
	RPVertex *map_vert_list;
	U16 *map_id_list;
	int map_index_count,map_vert_count;
	SINGLE lastShift;

	//terrainmap stuff
	GRIDVECTOR *gvec;//[MAX_SQUARES];

	int map_square;

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "Nebula");
	}
	
	
	Nebula (void);
	~Nebula (void);

//	BOOL32 LoadData ();

	IDAComponent * GetBase (void)
	{
		return (IEventCallback *) this;
	}

	// IBaseObject methods

	virtual void PhysicalUpdate(SINGLE dt);

	void DrawBillboard(int layer);

	virtual void CastVisibleArea (void);

	virtual BOOL32 Update (void);

	virtual void RenderBackground (void);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual void MapTerrainRender ();

	virtual void View (void);

	virtual Vector GetCenterPos (void);

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual bool GetObjectBox (OBJBOX & box) const
	{
		LONG width = bounds.right - bounds.left;
		LONG height = bounds.bottom - bounds.top;		// backwards!!!?
		CQASSERT(height>=0 && width>=0);

		memset(box, 0, sizeof(box));
		box[0] = __max(width, height) / 2;
		return true;
	}

	virtual SINGLE TestHighlight (const RECT & rect);	// set bHighlight if possible for any part of object to appear within rect,

	// ISaveLoad 

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	/* IQuickSaveLoad methods */
	
	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void)
	{}

	/* IDocumentClient methods */

	//DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message = 0, void *parm = 0);

	virtual BOOL32 Setup();//XYCoord *_squares=0,U32 _numSquares=0);

	BOOL32 OnScreen();

	void GetNewSparkPos(Vector &vec,const Spark &spark);

	virtual BOOL32 Init(HANDLE hArchetype);

	virtual BOOL32 ObjInField (U32 objSystemID,const Vector &pos);

	virtual void SetSystemID (U32 _systemID)
	{
		systemID = _systemID;
	}

	void SetFieldFlags(FIELDFLAGS &flags)
	{
		flags.bIon = (nebType == NEB_ION);
		flags.bAntiMatter = (nebType == NEB_ANTIMATTER);
		flags.bCelsius = (nebType == NEB_CELSIUS);
		flags.bCygnus = (nebType == NEB_CYGNUS);
		flags.bHelious = (nebType == NEB_HELIOUS);
		flags.bHades = (nebType == NEB_HYADES);
		flags.bLithium = (nebType == NEB_LITHIUM);
		flags.damagePerTwentySeconds = nArch->pData->attributes.damage*20.0f;
	}

	virtual void PlaceBaseNuggets();

	virtual void SetAmbientLight (const Vector &pos,const U8_RGB &old_amb);

	SINGLE getEdgeScale (const Vector &pos);

	virtual void setTerrainFootprint (struct ITerrainMap * terrainMap);
	virtual void unsetTerrainFootprint (struct ITerrainMap * terrainMap);

	U32 hasSquareAt(S32 x, S32 y);

	void setupSoftwareQuads(U32 i,U8 fogBits);

	void drawCournerQuad(U8 bits,Vector * vec);
};
//---------------------------------------------------------------------
//

Nebula::Nebula (void)
{
	U32 i;

	numSquares = 0;
/*	for (i=0;i<NUM_SPARKS;i++)
	{
		spark[i].alive = FALSE;
		spark[i].curAngle = 0;
	}*/

	for (i=0;i<2;i++)
	{
		dirX[i] = dirY[i] = 1;
		offsetX[i] = offsetY[i] = 0;
	}

/*	neb.light[0].red = 180;
	neb.light[0].green = 255;
	neb.light[0].blue = 200;
	neb.light[0].size = 6000;
	neb.light[0].speed = 600;
	neb.light[0].pulse = 0.2;
	neb.light[0].life = 10;

	memcpy(&neb.light[1],&neb.light[0],sizeof(neb.light[0]));

	light[0].color.r = neb.light[0].red;
	light[0].color.g = neb.light[0].green;
	light[0].color.b = neb.light[0].blue;
	light[0].range = neb.light[0].size;*/
	light[0].direction.set(0,0,-1);
//	light[0].enable();

/*	light[1].color.r = neb.light[1].red;
	light[1].color.g = neb.light[1].green;
	light[1].color.b = neb.light[1].blue;
	light[1].range = neb.light[1].size;*/
	light[1].direction.set(0,0,-1);
//	light[1].enable();

//	spark[0].alive = TRUE;
//	spark[0].launched = FALSE;

	multiStages = 0xffffffff;
}

Nebula::~Nebula (void)
{
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(systemID, map);
	if(map)
		unsetTerrainFootprint(map);

	SFXMANAGER->CloseHandle(hAmbientSound);

	if (map_vert_list)
		delete [] map_vert_list;
	if (map_id_list)
		delete [] map_id_list;
		
	if (OBJMAP)
		OBJMAP->RemoveObjectFromMap(this,systemID,map_square);

	for(U32 i = 0; i < numSquares; ++i)
	{
		if(cloudEffects[i])
			cloudEffects[i]->Stop();
	}
	delete [] squares;
	delete [] cloudEffects;
	delete [] gvec;
}

BOOL32 Nebula::Setup()//struct XYCoord *_squares,U32 _numSquares)
{
	S32 i,j;
	U32 k,l;
//	U8 zoneCnt = 0;
	COMPTR<ITerrainMap> map;

	CQASSERT(systemID);
	SECTOR->GetTerrainMap(systemID, map);

	if (gvec)
		unsetTerrainFootprint(map);

	if (nArch->numSquares)//_numSquares)
	{
		numSquares = nArch->numSquares;//_numSquares;
		memcpy(squares_xy,nArch->squares,numSquares*sizeof(*nArch->squares));
	}

	squares = new Square[numSquares];
	gvec = new GRIDVECTOR[numSquares];
	cloudEffects = new IEffectInstance*[numSquares];
	for (i=0;i<(int)numSquares;i++)
	{
		squares[i].x = squares_xy[i].x;
		squares[i].y = squares_xy[i].y;
		gvec[i] = Vector(squares[i].x,squares[i].y,0);
		if(nArch->cloudEffect)
		{
			cloudEffects[i] = nArch->cloudEffect->CreateInstance();
			cloudEffects[i]->SetSystemID(systemID);
			cloudEffects[i]->SetTargetPositon(Vector(squares[i].x,squares[i].y,0),0,0);
			cloudEffects[i]->TriggerStartEvent();
		}
		else
			cloudEffects[i] = NULL;
	}
	
	setTerrainFootprint(map);

	int cnt;
	for (cnt=0;cnt<(int)numSquares;cnt++)
	{
		for (i=0;i<3;i++)
		{
			for (j=0;j<3;j++)
			{
				squares[cnt].c[i][j].inset = true;
				squares[cnt].c[i][j].alpha = 0; 
			//	squares[cnt].c[i][j].height = LOW;
				
				S32 intx = squares[cnt].x;
				S32 inty = squares[cnt].y;
				S32 snapX = intx % HFTS;
				S32 snapY = inty % HFTS;

				squares[cnt].c[i][j].u = (SINGLE)(i+(intx%(2*HFTS) == snapX))/2;
				squares[cnt].c[i][j].v = (SINGLE)(j+(inty%(2*HFTS) == snapY))/2;
			}
		}
		squares[cnt].c[1][1].alpha = MAX_ALPHA;
		//height schmeight
		//squares[cnt].c[1][1].height = HIGH;
		SINGLE tmp = 0.0628*(rand()%100);
		squares[cnt].shftX = cos(tmp);
		squares[cnt].shftY = sin(tmp);
	}

	bounds.left = bounds.top = 9999999;
	bounds.right = bounds.bottom = -1;
	for (k=0;k<numSquares;k++)
	{
		if (squares[k].x-HFTS < bounds.left)
			bounds.left = squares[k].x-HFTS;
		if (squares[k].x+HFTS > bounds.right)
			bounds.right = squares[k].x+HFTS;
		if (squares[k].y-HFTS < bounds.top)
			bounds.top = squares[k].y-HFTS;
		if (squares[k].y+HFTS > bounds.bottom)
			bounds.bottom = squares[k].y+HFTS;

		//I don't care about height
	//	squares[k].c[1][1].height = LOW;
	//	squares[k].c[1][1].alpha = MAX_ALPHA;
		for (i=0;i<3;i++)
		{
			for (j=0;j<3;j++)
			{
			/*	if (j != 1 || i != 1)
				{
					squares[k].c[i][j].alpha = 0;
				}*/
				U8 score = 0;
				if (j == 1 || i == 1)
				{
					score = 2;
				}
				else
					score = 1;
				
				for (l=0;l<numSquares;l++)
				{
					if (k != l)
					{
						Square sq1 = squares[k];
						Square sq2 = squares[l];
						if (sq1.x-HFTS+HFTS*i <= sq2.x + HFTS && sq1.x-HFTS+HFTS*i >= sq2.x-HFTS)
						{
							
							if (sq1.y-HFTS+HFTS*j <= sq2.y + HFTS && sq1.y-HFTS+HFTS*j >= sq2.y-HFTS)
							{
								
								if (j == 1 || i == 1)
								{
									if (sq1.x == sq2.x || sq1.y == sq2.y)
									{
										score += 2;
									}
									else
										score++;
								}
								else
								{
									if (sq1.x == sq2.x || sq1.y == sq2.y)
									{
										score++;
									}
									else
									{
										if (abs(sq1.x - sq2.x) == HFTS || abs(sq1.y - sq2.y) == HFTS)
											score += 2;
										else
											score ++;
									}
									
								}
								
								if (score >= 4)
								{
//									U8 cnt;
//									BOOL32 done = FALSE;
									squares[k].c[i][j].alpha = MAX_ALPHA;
									squares[k].c[i][j].height = HIGH;
									
								/*	if (zoneCnt != MAX_SQUARES)
									{
										for (cnt=0;cnt<zoneCnt;cnt++)
										{
											if (zone[cnt].x == squares[k].x+HFTS*i-HFTS && zone[cnt].y == squares[k].y+HFTS*j-HFTS)
												done = TRUE;		
										}
										
										if (!done)
										{
											zone[zoneCnt].x = squares[k].x+HFTS*i-HFTS;
											zone[zoneCnt].y = squares[k].y+HFTS*j-HFTS;
											zoneCnt++;
										}
									}*/
								}
								squares[k].c[i][j].inset = false;
							}
						}
					}
				}
			}
		}
	}
/*	numZones = zoneCnt;
	if (numZones == 0)
		bHasSparks = 0;*/
	
	/*for (i=0;i<2;i++)
	{
		lzone[i] = numZones*(SINGLE)rand()/RAND_MAX;
		Vector vec(zone[lzone[i]].x,zone[lzone[i]].y,3000);
		
		light[i].setColor(neb.light[i].red,neb.light[i].green,neb.light[i].blue);
		light[i].range = neb.light[i].size;
		light[i].direction.set(0,0,-1);
		light[i].setSystem(systemID);
		light[i].set_position(vec);
	}*/
	
/*	for (i=0;i<NUM_SPARKS;i++)
	{
		Vector vec = Vector (zone[0].x, zone[0].y, 1475);

		spark[i].inst.SetPosition(vec);
		spark[i].timeToLive = SPARK_LIFE;
	}*/

//	GetRegions();

	//set up map render
	S32 vert_count=0;
	map_vert_list = new RPVertex[9*numSquares];
	map_id_list = new U16[24*numSquares];
	S32 idx_count=0;
	for (cnt=0;cnt<(int)numSquares;cnt++)
	{
		Square *sq = &squares[cnt];
		
		
		S32 base_ref=vert_count;
		vert_count+=9;
		S32 cntr_ref=base_ref+4;
		S32 ref;// = vert_count++;
		map_vert_list[cntr_ref].pos.set(sq->x,sq->y,0);
		map_vert_list[cntr_ref].r = neb.amb_r*sq->c[1][1].alpha>>8;
		map_vert_list[cntr_ref].g = neb.amb_g*sq->c[1][1].alpha>>8;
		map_vert_list[cntr_ref].b = neb.amb_b*sq->c[1][1].alpha>>8;

		
#define NEB_SHAPE(x0,y0,x1,y1)\
		{\
		ref = base_ref+x0+y0*3;\
		map_id_list[idx_count++] = ref;\
		map_vert_list[ref].pos.set(sq->x+HFTS*(x0-1),sq->y+HFTS*(y0-1),0);\
		map_vert_list[ref].r = (neb.amb_r*sq->c[x0][y0].alpha)>>8;\
		map_vert_list[ref].g = (neb.amb_g*sq->c[x0][y0].alpha)>>8;\
		map_vert_list[ref].b = (neb.amb_b*sq->c[x0][y0].alpha)>>8;\
		ref = base_ref+x1+y1*3;\
		map_id_list[idx_count++] = ref;\
		map_vert_list[ref].pos.set(sq->x+HFTS*(x1-1),sq->y+HFTS*(y1-1),0);\
		map_vert_list[ref].r = (neb.amb_r*sq->c[x1][y1].alpha)>>8;\
		map_vert_list[ref].g = (neb.amb_g*sq->c[x1][y1].alpha)>>8;\
		map_vert_list[ref].b = (neb.amb_b*sq->c[x1][y1].alpha)>>8;\
		map_id_list[idx_count++] = cntr_ref;\
		}


	//	v[0].set(sq->x+HFTS*(x0-1),sq->y+HFTS*(y0-1),0);\
	//	v[1].set(sq->x+HFTS*(x1-1),sq->y+HFTS*(y1-1),0);\
	//	PB.Color3ub(RVAL*sq->c[x0][y0].alpha,GVAL*sq->c[x0][y0].alpha,BVAL*sq->c[x0][y0].alpha); \
	//	PB.Color3ub(RVAL*sq->c[x1][y1].alpha,GVAL*sq->c[x1][y1].alpha,BVAL*sq->c[x1][y1].alpha); \
	//	}
		
		NEB_SHAPE(1,0,0,0);
		NEB_SHAPE(0,1,0,0);
		NEB_SHAPE(2,1,2,0);
		NEB_SHAPE(1,0,2,0);
		NEB_SHAPE(2,1,2,2);
		NEB_SHAPE(1,2,2,2);
		NEB_SHAPE(0,1,0,2);
		NEB_SHAPE(1,2,0,2);
		
#undef NEB_SHAPE
	}

	map_index_count = idx_count;
	// for now
	map_vert_count = 9*numSquares;

	//assumes counter is initialized to 0
	for (int v=0;v<map_vert_count;v++)
	{
		map_vert_list[v].u = map_vert_list[v].pos.x*MAP_TEX_FACTOR;
		map_vert_list[v].v = map_vert_list[v].pos.y*MAP_TEX_FACTOR;
	}
	
	//FIX THIS !!!
	Vector centerPos;
//	centerPos.set(squares[0].x,squares[0].y,0);
	centerPos.set(0.5*(bounds.right+bounds.left),0.5*(bounds.top+bounds.bottom),0);
	SFXMANAGER->Play(hAmbientSound,systemID,&centerPos,SFXPLAYF_LOOPING|SFXPLAYF_NOFOG);
	transform.translation = centerPos;

	map_square = OBJMAP->GetMapSquare(systemID,Vector(squares[0].x,squares[0].y,0));
	U32 flags = 0;
	objMapNode = OBJMAP->AddObjectToMap(this,systemID,map_square,flags);

	return 1;
}
//------------------------------------------------------------------------
//
U32 Nebula::hasSquareAt(S32 x, S32 y)
{
	for(U32 i = 0; i < numSquares; ++i)
	{
		if(squares[i].x == x && squares[i].y == y)
		{
			return 1;
		}
	}
	return 0;
}
//------------------------------------------------------------------------
//
BOOL32 Nebula::Init(HANDLE hArchetype) 
{
	nArch = (NebulaArchetype *)hArchetype;
	FRAME_init(*nArch);
	BT_NEBULA_DATA *data = nArch->pData;
	infoHelpID = nArch->pData->infoHelpID;

	hAmbientSound = SFXMANAGER->Open(data->ambientSFX);

	nebType = data->nebType;

	mapTex = nArch->mapTex;

	return 1;
}

void Nebula::PhysicalUpdate(SINGLE dt)
{

/*	if (updateTime > 0.05)
	{

		updateTime = 0;
		//// OLD FLASHY LIGHT CODE
	for (int i=0;i<2;i++)
	{
		Vector vec;
		if (numZones)
		{
			vec.x = zone[lzone[i]].x;
			vec.y = zone[lzone[i]].y;
		}
		else
		{
			vec.x = squares[0].x;
			vec.y = squares[0].y;
		}
		offsetX[i] += neb.light[i].speed*dirX[i];
		vec.x += offsetX[i] + neb.light[i].x;
		if (abs(offsetX[i]) > ROAM)
			dirX[i] = -dirX[i];
		offsetY[i] += neb.light[i].speed*dirY[i];
		vec.y += offsetY[i] + neb.light[i].y;
		if (abs(offsetY[i]) > ROAM)
			dirY[i] = -dirY[i];
		vec.z = 3000;
		if (i==0)
			light[0].set_position(vec);
		else
			light[1].set_position(vec);
		
		if (neb.light[i].life != 0 && lActive[i])
		{
			light[i].range = (timeToLive[i]/neb.light[i].life)*neb.light[i].size;
			
			if ((timeToLive[i] -= (float)ELAPSED_TIME) < 0)
			{
				lActive[i] = 0;
				light[i].set_On(0);
				
				lzone[i] = numZones*(SINGLE)rand()/RAND_MAX;
				//LIGHT->deactivate_lights(&flash_index[i],1);
			}
		}
		else if ((SINGLE)rand()/RAND_MAX < neb.light[i].pulse)
		{
			lActive[i] = !lActive[i];
			if (lActive[i] && !(neb.light[i].x || neb.light[i].y))
			{
				offsetX[i] = (rand()%(ROAM*2) - ROAM);
				offsetY[i] = (rand()%(ROAM*2) - ROAM);
					
				dirX[i] = (SINGLE)rand()/RAND_MAX;
				dirY[i] = (SINGLE)rand()/RAND_MAX;
				timeToLive[i] = neb.light[i].life;
				lzone[i] = rand()%numZones;
			}
			if (lActive[i])
			{
				light[i].set_On(1);
			}
		}
	}

	/// OLD SPARK CODE
#define NEAR_BND 2.0
#define FAR_BND 1.0
#define MID_BND 1.5
	
	if (bHasSparks)
	{	
		//Launch everyone who wants to
		for (i=0;i<NUM_SPARKS;i++)
		{
			if (spark[i].timeToLive < SPARK_LIFE*0.4 && !spark[i].launched && spark[i].alive)
			{
				spark[i].launched = TRUE;
				S32 slot =0;
				U8 total = 0;
				while (rand()%2==0 && slot != -1 && total !=3)
				{
					total++;
					S32 j = 0;
					slot = -1;
					
					while (j < NUM_SPARKS && slot == -1)
					{
						if (!spark[j].alive)
						{
							slot = j;
						}
						j++;
					}
					
					if (slot == -1)
					{
						slot = -1;
					}
					
					if (slot != -1)
					{
					//	spark[i].launched = TRUE;
						
						spark[slot].alive = TRUE;
						spark[slot].launched = FALSE;
						CQASSERT(i != slot);
						spark[slot].timeToLive = SPARK_LIFE;
						spark[slot].zone = spark[i].zone;
						
						Vector vec;// = ENGINE->get_position(spark[i].index);
						//	ENGINE->set_position(spark[slot].index,vec);
						SINGLE newAngle = spark[i].curAngle - 1.5 + 3*(SINGLE)rand()/RAND_MAX;
						GetNewSparkPos(vec,spark[i]);
						spark[slot].inst.SetPosition(vec);
						spark[slot].curAngle = newAngle;
						spark[slot].inst.SetRotation(newAngle,820,130);
						spark[slot].inst.Restart();
				//		spark[slot].inst.SetColor(0,255,255,255);
						if (abs(vec.x - zone[spark[slot].zone].x) > HFTS/NEAR_BND || abs(vec.y - zone[spark[slot].zone].y) > HFTS/NEAR_BND)
						{
							U8 k;
							spark[slot].alive = FALSE;
							if (abs(vec.x - zone[spark[slot].zone].x) > HFTS/2)
							{
								for (k=0;k<numZones;k++)
								{
									if (abs(vec.x - zone[k].x) < HFTS/FAR_BND && abs(vec.y - zone[k].y) < HFTS/MID_BND)
									{
										if (spark[slot].zone != k)
										{
											spark[slot].alive = TRUE;
											spark[slot].zone = k;
										}
									}
								}
							}
							else
							{
								for (k=0;k<numZones;k++)
								{
									if (abs(vec.y - zone[k].y) < HFTS/FAR_BND && abs(vec.x - zone[k].x) < HFTS/MID_BND)
									{
										if (spark[slot].zone != k)
										{
											spark[slot].alive = TRUE;
											spark[slot].zone = k;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		
		//kill dead folks
		for (i=0;i<NUM_SPARKS;i++)
		{
			if (spark[i].alive && spark[i].timeToLive <0)
			{
				spark[i].alive = FALSE;
			}
		}
		
		//generate new guys
		if (rand()%30==0)
		{
			S32 j= 0,slot = -1;
			
			while (j < NUM_SPARKS && slot == -1)
			{
				if (!spark[j].alive)
				{
					slot = j;
				}
				j++;
			}
			
			if (slot != -1)
			{
				spark[slot].alive = TRUE;
				spark[slot].launched = FALSE;
				spark[slot].timeToLive = SPARK_LIFE;
				spark[slot].zone = rand()%numZones;
				
				Vector vec2 = spark[slot].inst.GetPosition();
				//SINGLE temp = (2.0*HFTS/NEAR_BND)*0.001*(rand()%1000);
				vec2.x = zone[spark[slot].zone].x-HFTS/NEAR_BND + (2.0*HFTS/NEAR_BND)*0.001*(rand()%1000);
				vec2.y = zone[spark[slot].zone].y-HFTS/NEAR_BND + (2.0*HFTS/NEAR_BND)*0.001*(rand()%1000);
				spark[slot].inst.SetPosition(vec2);
				SINGLE newAngle = 2*PI*(SINGLE)rand()/RAND_MAX;
				spark[slot].curAngle = newAngle;
				spark[slot].inst.SetRotation(newAngle,820,130);
				spark[slot].inst.Restart();
				//spark[slot].inst.SetColor(255,0,0,255);
				spark[slot].inst.rate = SPARK_FPS;
			}
		}
	}
	}*/
}

BOOL32 Nebula::OnScreen()
{
	U32 i;
	BOOL32 onScreen = FALSE;
	Vector tl,tr,br,bl;
	CAMERA->PaneToPoints(tl,tr,br,bl);
	for (i=0;i<numSquares;i++)
	{
		if (squares[i].x+HFTS > min(bl.x,tr.x) && squares[i].x-HFTS < max(tl.x,br.x))
		{
			if (squares[i].y+HFTS > min(bl.y,tr.y) && squares[i].y-HFTS < max(tl.y,br.y))
			{
				onScreen = TRUE;
			}
		}
	}

	return onScreen;
}

void Nebula::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	bVisible = FALSE;
	if (GetSystemID() == currentSystem)
	{
		Vector tl,tr,br,bl;
		CAMERA->PaneToPoints(tl,tr,br,bl);
		const Transform *inverseWorldROT = CAMERA->GetWorldTransform();
		tl = inverseWorldROT->rotate_translate(tl);
		tr = inverseWorldROT->rotate_translate(tr);
		br = inverseWorldROT->rotate_translate(br);
		bl = inverseWorldROT->rotate_translate(bl);
		vx_min = min(tl.x,min(bl.x,min(tr.x,br.x)))-4000;
		vx_max = max(tl.x,max(bl.x,max(tr.x,br.x)))+4000;
		vy_min = min(tl.y,min(bl.y,min(tr.y,br.y)))-4000;
		vy_max = max(tl.y,max(bl.y,max(tr.y,br.y)))+4000;
		for (unsigned int i=0;i<numSquares;i++)
		{
			if (squares[i].x+HFTS > vx_min && squares[i].x-HFTS < vx_max)
			{
				if (squares[i].y+HFTS > vy_min && squares[i].y-HFTS < vy_max)
				{
					bVisible = TRUE;
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
SINGLE Nebula::TestHighlight (const RECT & rect)	// set bHighlight if possible for any part of object to appear within rect,
{
	bHighlight = 0;
	// don't highlight in lasso mode
	if (bVisible && rect.left==rect.right && rect.top==rect.bottom)
	{
		if (rect.left > 0)
		{
			Vector pos;
			
			pos.x = rect.left;
			pos.y = rect.top;
			pos.z = 0;
			
			CAMERA->ScreenToPoint(pos.x,pos.y,pos.z);
			
			if (FOGOFWAR->CheckHardFogVisibility(MGlobals::GetThisPlayer(), systemID, pos))
			{
				if (pos.x > bounds.left && pos.x < bounds.right)
				{
					if (pos.y > bounds.top && pos.y < bounds.bottom)
					{
						for (int i=0;i<(int)numSquares;i++)
						{
							SINGLE diffX = pos.x-squares[i].x;
							SINGLE diffY = pos.y-squares[i].y;
							if (fabs(diffX) < HFTS && fabs(diffY) < HFTS)
							{
								bHighlight = TRUE;
								factory->FieldHighlighted(this);
								/*	if (fabs(diffX) < HFTS-INSET && fabs(diffY) < HFTS-INSET)
								{
								return TRUE;
								}
								else
								{
								if (diffX < 0)
								{
								if (diffY < 0)
								TESTSQUARE(0,0,-1,-1)
								else
								TESTSQUARE(0,2,-1,1)
								}
								else
								{
								if (diffY < 0)
								TESTSQUARE(2,0,1,-1)
								else
								TESTSQUARE(2,2,1,1)
								}
							}*/
							}
						}
					}
				}
			}
		}
	}

	return 1e20;		// a large number
}
//--------------------------------------------------------------------------//
//
void Nebula::GetNewSparkPos(Vector &vec,const Spark &spark)
{
	SINGLE cosA,sinA;

	cosA = cos(spark.curAngle);
	sinA = sin(spark.curAngle);

	Vector epos (spark.inst.GetPosition());

	float x = 130;
	float y = 400;
	float pivx = 820;
	float pivy = 130;

	vec.set(epos.x+((x-pivx)*cosA-(y-pivy)*sinA)+pivx,epos.y+((x-pivx)*sinA+(y-pivy)*cosA)+pivy,epos.z);
	vec.x -= 820;
	vec.y -= 130;
//	epos = vec;

//	cosA = cos(newAngle);
//	sinA = sin(newAngle);

//	vec.set(epos.x+((-pivx)*cosA-(0-pivy)*sinA)+pivx,epos.y+((0-pivx)*sinA+(0-pivy)*cosA)+pivy,epos.z);
}

void Nebula::RenderBackground()
{
	if(bVisible)
	{
	}
}

void Nebula::Render()
{
	if (bVisible && CQEFFECTS.bExpensiveTerrain)//systemID == SECTOR->GetCurrentSystem())
	{
	}
}

//---------------------------------------------------------------------------------------
//
void Nebula::CastVisibleArea (void)
{
	SetVisibleToAllies(GetVisibilityFlags());  //sticky visibility flags - must saveload
}
//---------------------------------------------------------------------------------------
//
BOOL32 Nebula::Update()
{
	if(bVisible)
	{
		for(U32 i = 0; i < numSquares; ++i)
		{
			if(cloudEffects[i])
				cloudEffects[i]->Enable();
		}
	}
	else
	{
		for(U32 i = 0; i < numSquares; ++i)
		{
			if(cloudEffects[i])
				cloudEffects[i]->Disable();
		}
	}

	return true;
}
//---------------------------------------------------------------------------
//
void Nebula::MapTerrainRender ()
{
	for(U32 i = 0; i < numSquares; ++i)
	{
		if(mapTex != -1)
			SYSMAP->DrawIcon(gvec[i],GRIDSIZE,mapTex);	
	}
}
//---------------------------------------------------------------------------
//
void Nebula::View (void)
{
	NEBULA_DATA data;
	Vector vec;

	//memset(&data, 0, sizeof(data));
	
	memcpy(&data,&neb,sizeof(neb));
	memcpy(data.name,name, sizeof(data.name));

	if (DEFAULTS->GetUserData("NEBULA_DATA", " ", &data, sizeof(data)))
	{
		memcpy(&neb, &data, sizeof(data));
		memcpy(name, data.name, sizeof(data.name));
		
/*		light[0].setColor(neb.light[0].red,neb.light[0].green,neb.light[0].blue);
		light[0].range = neb.light[0].size;
		light[0].direction.set(0,0,-1);

		light[1].color.r = neb.light[1].red;
		light[1].color.g = neb.light[1].green;
		light[1].color.b = neb.light[1].blue;
		light[1].setColor(neb.light[1].red,neb.light[1].green,neb.light[1].blue);
		light[1].range = neb.light[1].size;
		light[1].direction.set(0,0,-1);

		lzone[0] = lzone[1] = numZones*(SINGLE)rand()/RAND_MAX;
		vec.set(zone[lzone[0]].x,zone[lzone[0]].y,3000);
		
		light[0].set_position(vec);
		light[1].set_position(vec);*/
		
	}
}
//---------------------------------------------------------------------------
//
Vector Nebula::GetCenterPos (void)
{
	Vector vec(0,0,0);
	for (int i=0;i<(int)numSquares;i++)
	{
		vec.x+= squares[i].x;
		vec.y+= squares[i].y;
	}
	vec.x = vec.x/numSquares;
	vec.y = vec.y/numSquares;
	return vec;
}
//---------------------------------------------------------------------------
//
BOOL32 Nebula::Save(IFileSystem *inFile)
{
	DAFILEDESC fdesc = "NEBULA_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	NEBULA_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memcpy(&save, static_cast<BASE_NEBULA_SAVELOAD *>(this), sizeof(BASE_NEBULA_SAVELOAD));

	save.exploredFlags = GetVisibilityFlags();
	FRAME_save(save);

	if (file->WriteFile(0,&save,sizeof(save),&dwWritten,0) == 0)
		goto Done;

	result = 1;

Done:

	return result;

}

BOOL32 Nebula::Load(IFileSystem *inFile)
{
	DAFILEDESC fdesc = "NEBULA_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	NEBULA_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(buffer, 0, sizeof(buffer));
	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("NEBULA_SAVELOAD", buffer, &load);

	CQASSERT(sizeof(NEBULA_SAVELOAD) < 1024);

	memcpy(static_cast<BASE_NEBULA_SAVELOAD *>(this), &load, sizeof(BASE_NEBULA_SAVELOAD));
	FRAME_load(load);
	SetVisibleToAllies(load.exploredFlags);
	UpdateVisibilityFlags();


	Setup();
	result = 1;
	//nebActive = TRUE;

	SYSMAP->InvalidateMap(systemID);

Done:

	return result;

}

void Nebula::ResolveAssociations()
{
}

/* IQuickSaveLoad methods */

void Nebula::QuickSave (struct IFileSystem * file)
{
	DAFILEDESC fdesc = partName;
	HANDLE hFile;

	file->CreateDirectory("MT_QFIELDLOAD");
	if (file->SetCurrentDirectory("MT_QFIELDLOAD") == 0)
		CQERROR0("QuickSave failed");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR0("QuickSave failed");
	}
	else
	{
		MT_QFIELDLOAD qload;
		DWORD dwWritten;

		qload.systemID = systemID;
		qload.numSquares = numSquares;
		for (unsigned int i=0;i<numSquares;i++)
		{
			qload.pos[i] = Vector(squares[i].x,squares[i].y,0);
		}

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//---------------------------------------------------------------------------
//
void Nebula::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_QFIELDLOAD qload;
	MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);

	SetSystemID(qload.systemID);
	numSquares = qload.numSquares;
	dwMissionID = MGlobals::CreateNewPartID(0);

	MGlobals::InitMissionData(this, dwMissionID);
	partName = szInstanceName;

	OBJLIST->AddPartID(this, dwMissionID);

	nArch->numSquares = numSquares;
	for (unsigned int i=0;i<numSquares;i++)
	{
		Vector pos = qload.pos[i];
		nArch->squares[i].x = pos.x;
		nArch->squares[i].y = pos.y;
	}
	Setup();
	PlaceBaseNuggets();
};

#define TESTSQUARE(x,y,xfact,yfact) \
{\
	SINGLE dX = diffX*(xfact);\
	SINGLE dY = diffY*(yfact);\
	if (squares[i].c[x][y].inset)\
	{\
		if (dX > dY && HFTS-dX > dY*(INSETRATIO))\
		{\
			return TRUE;\
		}\
		else if (dY > dX && HFTS-dY > dX*(INSETRATIO))\
		{	\
			return TRUE;\
		}\
	}\
	else\
		return TRUE;\
}

BOOL32 Nebula::ObjInField(U32 objSystemID,const Vector &pos)
{
	if (objSystemID && objSystemID != systemID)
		return FALSE;

	if (pos.x > bounds.left && pos.x < bounds.right)
	{
		if (pos.y > bounds.top && pos.y < bounds.bottom)
		{
			for (int i=0;i<(int)numSquares;i++)
			{
				SINGLE diffX = pos.x-squares[i].x;
				SINGLE diffY = pos.y-squares[i].y;
				if (fabs(diffX) < HFTS && fabs(diffY) < HFTS)
				{
					if (fabs(diffX) < HFTS-INSET && fabs(diffY) < HFTS-INSET)
					{
						return TRUE;
					}
					else
					{
						if (diffX < 0)
						{
							if (diffY < 0)
								TESTSQUARE(0,0,-1,-1)
							else
								TESTSQUARE(0,2,-1,1)
						}
						else
						{
							if (diffY < 0)
								TESTSQUARE(2,0,1,-1)
							else
								TESTSQUARE(2,2,1,1)
						}
					}
				}
			}
		}
	}

	return FALSE;
}

//for setting the ambient light.  how in am I?
SINGLE Nebula::getEdgeScale(const Vector &pos)
{
	if (pos.x > bounds.left && pos.x < bounds.right)
	{
		if (pos.y > bounds.top && pos.y < bounds.bottom)
		{
			for (int i=0;i<(int)numSquares;i++)
			{
				SINGLE diffX = pos.x-squares[i].x;
				SINGLE diffY = pos.y-squares[i].y;
				if (fabs(diffX) < HFTS && fabs(diffY) < HFTS)
				{
					if (fabs(diffX) < HFTS-INSET && fabs(diffY) < HFTS-INSET)
					{
						//point is utterly inside the square and the insets
						return 1.0f;
					}
					else
					{
						int x,y;
						if (pos.x > squares[i].x)
							x = 1;
						else
							x = -1;

						if (pos.y > squares[i].y)
							y = 1;
						else
							y = -1;

						if (squares[i].c[x+1][y+1].alpha == MAX_ALPHA)
							return 1.0f;
						else
						{
							SINGLE dist = sqrt(diffX*diffX+diffY*diffY);
							if (dist < 0.75*HFTS)
								return 1.0f;

							return max(0.0f,((HFTS-dist)/(0.25*HFTS)));
						}
					}
				}
			}
		}
	}

	return 0.0f;
}

void Nebula::setTerrainFootprint (struct ITerrainMap * terrainMap)
{
	FootprintInfo info;
	info.missionID = dwMissionID;
	info.height = 0;
	info.flags = TERRAIN_FIELD | TERRAIN_FULLSQUARE;
	if (nebType == NEB_ANTIMATTER)
		info.flags |= TERRAIN_IMPASSIBLE;
	terrainMap->SetFootprint(gvec,numSquares,info);
}
void Nebula::unsetTerrainFootprint (struct ITerrainMap * terrainMap)
{
	FootprintInfo info;
	info.missionID = dwMissionID;
	info.height = 0;
	info.flags = TERRAIN_FIELD | TERRAIN_FULLSQUARE;
	if (nebType == NEB_ANTIMATTER)
		info.flags |= TERRAIN_IMPASSIBLE;
	terrainMap->UndoFootprint(gvec,numSquares,info);
}

void Nebula::PlaceBaseNuggets()
{
	U32 validNuggets = 0;
	if(nArch->nuggetType[3])
		validNuggets = 4;
	else if(nArch->nuggetType[2])
		validNuggets = 3;
	else if(nArch->nuggetType[1])
		validNuggets = 2;
	else if(nArch->nuggetType[0])
		validNuggets = 1;
	if(validNuggets)
	{
		RandomGen randGen(((U16)dwMissionID >> 3) %255);
		numNuggets = nArch->pData->nuggetsPerSquare*numSquares;
		U32 nuggetID = 1;
		for(U32 i = 0 ; i < numNuggets; ++i)
		{
			CQASSERT((nuggetID != 0x00000080) && "Excessive Number of Nuggets in Nebula");
			U32 square = randGen.rand() % numSquares;
			Vector pos;
			pos.x = this->squares[square].x + GRIDSIZE*((randGen.rand()%1000)/1000.0) - (GRIDSIZE*0.5);
			pos.y = this->squares[square].y + GRIDSIZE*((randGen.rand()%1000)/1000.0) - (GRIDSIZE*0.5);
			pos.z = nArch->pData->nuggetZHeight;

			PARCHETYPE pArch = nArch->nuggetType[randGen.rand()%validNuggets];
			BT_NUGGET_DATA * nData = (BT_NUGGET_DATA *)(ARCHLIST->GetArchetypeData(pArch));
			NUGGETMANAGER->CreateNugget(pArch,systemID,pos,nData->maxSupplies,0,dwMissionID | (nuggetID << 24),false);
			++nuggetID;
		}
	}
}

void Nebula::SetAmbientLight (const Vector &pos,const U8_RGB &old_amb)
{
	BT_NEBULA_DATA::AMBIENT_NEBULA_LIGHT * amb = &nArch->pData->ambient;
	SINGLE ramp=0.0;
	ramp = getEdgeScale(pos);
	LIGHTS->SetAmbientLight(amb->r*ramp+old_amb.r*(1-ramp),amb->g*ramp+old_amb.g*(1-ramp),amb->b*ramp+old_amb.b*(1-ramp));
}

/*void Nebula::GetRegions() 
{
	U32 c,i,j;
	Cloudzone *zone;
	
	zone = new Cloudzone[numSquares];
	
	for (c=0;c<numSquares;c++)
	{
		zone[c].r.left = squares[c].x-HFTS;
		zone[c].r.right = squares[c].x+HFTS;
		zone[c].r.top = squares[c].y-HFTS;
		zone[c].r.bottom = squares[c].y+HFTS;
		
		for (i=0;i<3;i++)
			for (j=0;j<3;j++)
			{
				zone[c].alpha[i*3+j] = squares[c].c[i][j].alpha/255.0;
			}
	}

	FOGOFWAR->AddNebula(zone,numSquares);
}*/

//------------------------------------------------------------------------------//
//  Concept of FieldManager :
//
//  FieldManager will keep track of big unclickable things that nonetheless need
//  to be BaseObjects.  
//
//  FieldManager will persist the entire app.
//
//  FieldManager will call the view function on its children : Fields
//
//  FieldManager is now an objectfactory
/*struct FieldNode
{
	IField *field;
	FieldNode *nextField;
	BOOL32 flag:1;
};*/


/*
struct LARRY
{
	S32 x,y;
};

struct FIELD_PACKET : CREATE_PACKET
{
	LARRY p[MAX_SQUARES];
	U8 numSquares;
};
*/

IField *CreateMinefield(PARCHETYPE);
IField *CreateDebrisfield(PARCHETYPE);

struct DACOM_NO_VTABLE FieldManager : public IFieldManager, IEventCallback, IObjectFactory, ResourceClient<>
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(FieldManager)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IFieldManager)
	END_DACOM_MAP()

	// structure
	struct IField *fieldList;
	U32 eventHandle,factoryHandle;
	BOOL32 bHasFocus;
	HWND dialog;

	// functionality
	BOOL32 editing;
	BOOL32 managing;
	UINT textureID;
	U32 workingSystemID;

	//blast stuff 
	PARCHETYPE pHeliousBlast;
	PARCHETYPE pHadesBlast;

	//child object info
	FieldArchetype<void> *fieldArch;

	//FieldManager methods

	FieldManager (void) 
	{
		RadixSort::set_sort_size(8192);

		resPriority = RES_PRIORITY_MEDIUM;
	
//		anchorX = ANCHOR_OFF;
	}

	~FieldManager();
	
	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}
	void operator delete (void * ptr)
	{
		::free(ptr);
	}

	IDAComponent * GetBase (void)
	{
		return (IEventCallback *) this;
	}

	void RenderBackground ();

	void Render();

	BOOL32 Update();

	void Edit(HANDLE hArchetype);

	void EndEdit();

	void Init();

	static BOOL CALLBACK NameDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
	static BOOL CALLBACK FieldListDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	//IFieldManager
	virtual void DeleteField(IField *field);

	virtual IField *GetFields();

	virtual IField *GetFieldContaining(IBaseObject *obj);

//	virtual void SetShipBits ();

	virtual void FieldHighlighted (struct IField * field);

	virtual void CreateField(char * fieldArchtype,S32 * xPos,S32 * yPos, U32 numberInList, U32 systemID);

	//obj is the pointer to the ship that is in the field
	virtual void SetAttributes(IBaseObject *obj,U32 dwFieldMissionID);

	virtual void AddFieldToList(IField * field);

	virtual void RemoveFieldFromList(IField * field);

	virtual void MapRenderFields(U32 systemID);
	
	virtual void CreateFieldBlast(IBaseObject * target,Vector pos,U32 systemID);

	//IEventCallBack
	DEFMETHOD (Notify) (U32 message, void *param);

	//IObjectFactory
	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);

	void loadTextures (bool bLoad);

	virtual void setHintbox (void)
	{
		if (hintboxID && HINTBOX->GetText() != hintboxID)
			HINTBOX->SetText(hintboxID);
	}

};

//--------------------------------------------------------------------------
//
void FieldManager::loadTextures (bool bLoad)
{
	if (bLoad)
	{
		CQASSERT(textureID==0);
		textureID = TMANAGER->CreateTextureFromFile("edit.tga", TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
	}
	else
	{
		TMANAGER->ReleaseTextureRef(textureID);
		textureID = 0;
	}
}
//--------------------------------------------------------------------------
// FieldManager methods

FieldManager::~FieldManager()
{
	COMPTR<IDAConnectionPoint> connection;
	if (GS)
	{
		
		
		if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}
	if (OBJLIST)
	{
		if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
			connection->Unadvise(factoryHandle);
	}

	RadixSort::free_arrays();
}

void FieldManager::RenderBackground()
{
	IField *fieldPos = fieldList;

	while (fieldPos)
	{
		fieldPos->RenderBackground();
		fieldPos = fieldPos->nextField;
	}
}

BOOL32 FieldManager::Update()
{
//	if (dummy == NULL)
//	{
//		dummy = ReviveDummy();
	//}

	if (editing)
	{
		desiredOwnedFlags = RF_CURSOR;
		if (ownsResources() == 0)
			grabAllResources();
	}

/*	IBaseObject *ptr = OBJLIST->GetObjectList();
	IField *field = fieldList;

	static unsigned int updateCounter;
	updateCounter++;
	updateCounter &= 31;

	while (ptr)
	{
		if (ptr->bVisible || ptr->updateBin == updateCounter)		// assumes that updateBin is 5 bits, unsigned
		{
			U32 systemID = ptr->GetSystemID();
			Vector pos = ptr->GetPosition();
			field = fieldList;
			memset(&ptr->fieldFlags,0,sizeof(FIELDFLAGS));
			while (field)
			{
				if (field->ObjInField(systemID,pos))
				{
					field->SetFieldFlags(ptr->fieldFlags);
				}
				field = field->nextField;
			}
		}

		ptr = ptr->nextField;
	}*/
	
	//
	// check mouse-over
	//
	if (editing==0)
	{
		if (hintboxID)
		{
			desiredOwnedFlags = RF_HINTBOX;
			if (ownsResources()==0)
				grabAllResources();
			else
				setResources();
		}
		else
		{
			desiredOwnedFlags = 0;
			releaseResources();
		}
		
	}
	hintboxID = 0;
	return 1;
}

void FieldManager::FieldHighlighted (struct IField * field)
{
	hintboxID = field->infoHelpID;
}

void FieldManager::CreateField(char * fieldArchtype,S32 * xPos,S32 * yPos, U32 numberInList, U32 systemID)
{
	FieldArchetype<void> * fieldArch = (FieldArchetype<void> *)(ARCHLIST->GetArchetypeHandle(ARCHLIST->LoadArchetype(fieldArchtype)));
	fieldArch->SetUpPosition(xPos,yPos,numberInList);

	IField *obj;
	
	obj = (IField *)ARCHLIST->CreateInstance(fieldArch->pArchetype);
	if (obj==0)
		return;

	MPartNC part = obj;
	part->dwMissionID = MGlobals::CreateNewPartID(0);
	sprintf(part->partName,"#%x#%.20s",part->dwMissionID,obj->name);
	OBJLIST->AddPartID(obj,obj->GetPartID());

	obj->SetSystemID(systemID);
	obj->Setup();//squares,numSquares);
	
	OBJLIST->AddObject(obj);

	obj->PlaceBaseNuggets();
	SYSMAP->InvalidateMap(systemID);
}

void FieldManager::SetAttributes(IBaseObject *obj,U32 dwFieldMissionID)
{
	IField *field = fieldList;
	while (field && field->GetPartID() != dwFieldMissionID)
	{
		field = field->nextField;
		CQASSERT(field && "Invalid Part ID");
	}

	if (field)
		field->SetFieldFlags(obj->fieldFlags);
}

void FieldManager::AddFieldToList(IField * field)
{
	field->nextField = fieldList;
	fieldList = field;
}

void FieldManager::RemoveFieldFromList(IField * field)
{
	IField * prev = NULL;
	IField * search = fieldList;
	while(search)
	{
		if(search == field)
		{
			if(prev)
			{
				prev->nextField = field->nextField;
			}
			else
			{
				fieldList = field->nextField;
			}
			return;
		}
		prev = search;
		search = search->nextField;
	}
}

void FieldManager::MapRenderFields(U32 systemID)
{
	IField * search = fieldList;
	while(search)
	{
		if(search->GetSystemID() == systemID)
			search->MapTerrainRender();
		search = search->nextField;
	}
}

void FieldManager::CreateFieldBlast(IBaseObject * target,Vector pos,U32 systemID)
{
	if(target->fieldFlags.hasBlast())
	{
		if(target->fieldFlags.bHelious)
		{
			TRANSFORM trans;
			trans.set_position(pos);
			IBaseObject * blast = CreateBlast(pHeliousBlast,trans,systemID,1);
			if(blast)
				OBJLIST->AddObject(blast);
		}
	}

	if (target->bVisible && target->fieldFlags.bHades)
	{
		TRANSFORM trans;
		const TRANSFORM &transform = target->GetTransform();
		trans = transform.get_inverse();
		trans.translation.set(0,0,0);
		IBaseObject *obj;
		if ((obj = ARCHLIST->CreateInstance(pHadesBlast)) != 0)
		{
			OBJPTR<IBlast> blast;

			float box[6];
			target->GetObjectBox(box);
			SINGLE scale = 10*(box[BBOX_MAX_X]-box[BBOX_MIN_X])*0.00025;

			if (obj->QueryInterface(IBlastID, blast))
			{
				blast->InitBlast(trans, systemID,target,scale);
			}
		}

		VOLPTR(IExtent) extent = target;
		extent->AddChildBlast(obj);
	}
}

/*void FieldManager::SetShipBits()
{
	IBaseObject *ptr = OBJLIST->GetObjectList();
	IField *field = fieldList;
	
	while (ptr)
	{
		U32 systemID = ptr->GetSystemID();
		Vector pos = ptr->GetPosition();
		field = fieldList;
		memset(&ptr->fieldFlags,0,sizeof(FIELDFLAGS));
		while (field)
		{
			if (field->ObjInField(systemID,pos))
			{
				field->SetFieldFlags(ptr->fieldFlags);
			}
			field = field->nextField;
		}

		ptr = ptr->nextField;
	}
}*/

void FieldManager::Render()
{
	if (CQFLAGS.bGameActive==0)
		return;

	if (editing)
	{
		fieldArch->RenderEdit();
	}
	else if(managing)
	{
		S32 xPos;
		S32 yPos;
		Vector vec;
		IField * highlight = fieldList;
		while(highlight)
		{
			if(highlight->GetSystemID() == SECTOR->GetCurrentSystem())
			{
				vec = highlight->GetCenterPos();
				CAMERA->PointToScreen(vec,&xPos,&yPos);
				DEBUGFONT->StringDraw(0,xPos,yPos, highlight->name);
			}
			highlight = highlight->nextField;
		}
	}
}

void FieldManager::Edit(HANDLE hArchetype)
{
	editing = TRUE;
	fieldArch = (FieldArchetype<void> *)hArchetype;
	fieldArch->Edit();

}

void FieldManager::EndEdit()
{
	desiredOwnedFlags = 0;
	releaseResources();
	editing = FALSE;

	fieldArch->EndEdit();

#if 0
	if (laidSquare == 0)
		return;

	laidSquare = FALSE;

	IField *obj;

	if(((FieldArchetype<BASIC_DATA> *)fieldArch)->pData->objClass == OC_MINEFIELD)
	{
		for(U32 squareIndex = 0; squareIndex < numSquares;++squareIndex)
		{
			obj = (IField *)MGlobals::CreateInstance(fieldArch->pArchetype, MGlobals::CreateNewPartID(MGlobals::GetThisPlayer()));
			if (obj==0)
				return;
			obj->Setup();//&(squares[squareIndex]),1);

			OBJLIST->AddObject(obj);
		}
	}
	else
	{
		obj = (IField *)ARCHLIST->CreateInstance(fieldArch->pArchetype);
		if (obj==0)
			return;

		MPartNC(obj)->dwMissionID = MGlobals::CreateNewPartID(0);
		OBJLIST->AddPartID(obj,obj->GetPartID());

		obj->Setup();//squares,numSquares);

		OBJLIST->AddObject(obj);
	}
#endif
}

void FieldManager::Init()
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &eventHandle);
	}

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}

	initializeResources();
}

BOOL FieldManager::NameDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;
	char *nameStr = (char *) GetWindowLong(hwnd, DWL_USER);


	switch (message)
	{
	case WM_INITDIALOG:
		{
			SetWindowLong(hwnd, DWL_USER, lParam);

			nameStr = (char *)lParam;
			SetDlgItemText(hwnd,IDC_EDIT1,nameStr);
			SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_LIST1:
			{
				switch (HIWORD(wParam))
				{
				case LBN_DBLCLK:
					SendMessage(hwnd, WM_COMMAND, IDOK, 0);
					break;
				}
			}
			break;
		case IDOK:
			{
				char buffer[32];
				GetDlgItemText(hwnd,IDC_EDIT1,buffer,32);
				if (buffer[0])
				{
					strcpy(nameStr,buffer);
				}
				EndDialog(hwnd,0);
			}
			break;
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
		}
	}

	return result;
}

BOOL FieldManager::FieldListDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;
	//FieldNode * fieldList = (FieldNode *)GetWindowLong(hwnd, DWL_USER);
	FieldManager *fieldmgr = (FieldManager *)GetWindowLong(hwnd, DWL_USER);
//	char newName[32];

	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hList = GetDlgItem(hwnd,IDC_LIST1);

			SetWindowLong(hwnd, DWL_USER, lParam);

			fieldmgr = (FieldManager *)lParam;

			fieldmgr->workingSystemID = SECTOR->GetCurrentSystem();
			IField *pos = fieldmgr->fieldList;
			U32 cnt = 0;
			while (pos)
			{	
				if (pos->GetSystemID() == fieldmgr->workingSystemID)
				{
					char buffer[256];
					sprintf(buffer,"#%x#%s",pos->GetPartID(),pos->name);
					SendMessage(hList, LB_INSERTSTRING, cnt, (LPARAM)buffer);
					SendMessage(hList, LB_SETITEMDATA, cnt, cnt);
					cnt++;
					pos->flag = FALSE;
				}
				else
					pos->flag = TRUE;

				pos = pos->nextField;
			}

			SetFocus(GetDlgItem(hwnd, IDC_LIST1));
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_LIST1:
			{
				switch (HIWORD(wParam))
				{
				case LBN_DBLCLK:
					U32 sel;
					HWND hList = GetDlgItem(hwnd,IDC_LIST1);
					IField *pos = fieldmgr->fieldList;
					while (pos && pos->GetSystemID() != fieldmgr->workingSystemID)
						pos = pos->nextField;

					sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
					U32 i;
					for (i=0;i<sel;i++)
					{
						//search through the list to the field in this system we're referring to
						do
						{
							pos = pos->nextField;
						} while (pos && pos->GetSystemID() != fieldmgr->workingSystemID);
					}

					pos->View();
					U32 index= SendMessage(hList,LB_GETITEMDATA,sel,0);
					SendMessage(hList,LB_DELETESTRING,sel,0);
					char buffer[256];
					sprintf(buffer,"#%x#%s",pos->GetPartID(),pos->name);
					SendMessage(hList,LB_INSERTSTRING,sel,(LPARAM)buffer);
					SendMessage(hList, LB_SETITEMDATA, sel, index);
			
					break;
				}
			}
			break;
		case IDDELETE:
			{
				U32 sel;
				HWND hList = GetDlgItem(hwnd,IDC_LIST1);
				
				sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
				SendMessage(hList,LB_DELETESTRING,sel,0);
			}
			break;
		case IDOK:
			{
				if (fieldmgr->managing)
				{
					IField *pos = fieldmgr->fieldList,*last,*nextField;
					while (pos && pos->GetSystemID() != fieldmgr->workingSystemID)
					{
						pos = pos->nextField;
					}

					HWND hList = GetDlgItem(hwnd,IDC_LIST1);
					
					U32 cnt=0,sel,listCnt = 0;
					U32 total = SendMessage(hList,LB_GETCOUNT,0,0);
					
					while (pos && cnt < total)
					{
						sel = SendMessage(hList,LB_GETITEMDATA,cnt,0);
					//	result = SendMessage(hList,LB_GETTEXT,cnt,(LPARAM)newName);
						while (listCnt < sel)
						{
							do
							{
								pos = pos->nextField;
							} while (pos && pos->GetSystemID() != fieldmgr->workingSystemID);
							listCnt++;
						}
					//	strcpy(pos->name,newName);
						pos->flag = TRUE;
						cnt++;
					}
					
					last = NULL;
					pos = fieldmgr->fieldList;
					while (pos)
					{
						nextField = pos->nextField;
						if (!pos->flag)
						{
							if (last)
								last->nextField=pos->nextField;
							else
								fieldmgr->fieldList = pos->nextField;
							
							OBJLIST->RemoveObject(pos);

							delete pos;
						}
						else
							last = pos;
						pos = nextField;
					}
					fieldmgr->managing = FALSE;
					EndDialog(hwnd,1);

					SYSMAP->InvalidateMap(fieldmgr->workingSystemID);

					CQASSERT(HEAP->EnumerateBlocks());
				}
				else 
				{
					MessageBox(hMainWindow,"Your changes have been pre-empted by another user's changes","Bite",MB_OK);
					EndDialog(hwnd,0);
				}
			}
			break;
		case IDCANCEL:
			fieldmgr->managing = FALSE;
			EndDialog(hwnd, 0);
			break;
		}
	}

	return result;
}

void FieldManager::DeleteField(IField *field)
{
	if (managing)
		managing = FALSE;

	IField *pos = fieldList,*last,*nextField;
	last = NULL;
	pos = fieldList;
	while (pos)
	{
		nextField = pos->nextField;
		if (pos == field)
		{
			if (last)
				last->nextField=pos->nextField;
			else
				fieldList = pos->nextField;
			
			OBJLIST->RemoveObject(pos);
			return;
		}
		else
 			last = pos;
		pos = nextField;
	}
}
//--------------------------------------------------------------------------//
//
//
IField * FieldManager::GetFields()
{
	return fieldList;
}
//--------------------------------------------------------------------------//
//
IField *FieldManager::GetFieldContaining(IBaseObject *obj)
{
	CQASSERT(obj);
	U32 systemID = obj->GetSystemID();
	if (systemID == 0)
		return NULL;

	Vector pos = obj->GetPosition();

	IField *fieldPos = fieldList;

	while (fieldPos)
	{
		if (fieldPos->ObjInField(systemID,pos))
		{
			return fieldPos;
		}
		fieldPos = fieldPos->nextField;
	}

	return NULL;
}

//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT FieldManager::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_LEAVING_INGAMEMODE:
		loadTextures(false);
		break;
	case CQE_ENTERING_INGAMEMODE:
		loadTextures(true);
		break;

/*	case WM_MOUSEMOVE:
		mouseX = LOWORD(msg->lParam);
		mouseY = HIWORD(msg->lParam);
		break;*/

	case WM_RBUTTONDOWN:
		if (editing && ownsResources())
		{
			EndEdit();
		}
		break;
		
	case WM_MOUSEMOVE:
	case WM_LBUTTONUP:
	case WM_LBUTTONDOWN:
		if (editing && ownsResources())
		{
			fieldArch->Notify(message,param);
		}
		break;

	case WM_CLOSE:
		loadTextures(false);
		break;
	
	case CQE_DEBUG_HOTKEY:
		if (CQFLAGS.bGameActive)
		switch ((U32)param)
		{
		case IDH_NEBULATE:
			if (!managing)
			{
				managing = TRUE;
				dialog = CreateDialogParam(hResource, MAKEINTRESOURCE(IDD_FIELDLIST), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) FieldListDlgProc, LPARAM(this));
				SetParent(dialog,0);
				ShowWindow(dialog,SW_SHOWNORMAL);
				SetWindowPos(dialog,HWND_NOTOPMOST, 20, 20, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
			}
			break;
	
		}
		break;

	case CQE_NEW_SELECTION:
	case CQE_KILL_FOCUS:
		if (editing)
			EndEdit();

	//	if ((IDAComponent *)param != getBase())
	//	{
		//	bHasFocus = false;
		//	desiredOwnedFlags = 0;
		//	releaseResources();
	//	}
		break;

//	case CQE_SET_FOCUS:
//		bHasFocus = true;
//		break;

	case CQE_UPDATE:
		Update();
		break;

	case CQE_RENDER_LAST3D:
		Render();
		break;

	case CQE_SYSTEM_CHANGED:
		if (managing)
		{
			SendMessage(dialog,WM_COMMAND,IDOK,0);
		}
		break;

	}

	return GR_OK;
}

HANDLE FieldManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	HANDLE result = 0;
	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc;
	
	switch (objClass)
	{
	case OC_MINEFIELD:
		{
			result = Minefield::CreateArchetype(ARCHLIST->GetArchetype(szArchname), objClass, data);
		}
		break;
		
		
	case OC_NEBULA:
		{
			U32 lastTexMem = TEXMEMORYUSED;
			NebulaArchetype *nArch;
			nArch = new NebulaArchetype;
			//nArch->name = szArchname;
			nArch->pData = (BT_NEBULA_DATA *)data;
			nArch->pArchetype = ARCHLIST->GetArchetype(szArchname);
			for(U32 i = 0; i <4; ++i)
			{
				if(nArch->pData->nuggetType[i][0])
					nArch->nuggetType[i] = ARCHLIST->LoadArchetype(nArch->pData->nuggetType[i]);
			}

			if(nArch->pData->cloudEffect[0])
				nArch->cloudEffect = EFFECTPLAYER->LoadEffect(nArch->pData->cloudEffect);
			else
				nArch->cloudEffect = NULL;
	
			nArch->pData->mapTexName;
			if (nArch->pData->mapTexName[0])
			{
				nArch->mapTex = SYSMAP->RegisterIcon(nArch->pData->mapTexName);
			}else
			{
				nArch->mapTex = -1;
			}


			SFXMANAGER->Preload(nArch->pData->ambientSFX);

			result = (HANDLE)nArch;

			if(nArch->pData->nebType == NEB_HELIOUS)
			{
			}

			if(nArch->pData->nebType == NEB_HYADES)
			{
			}
			NEBTEXTUREUSED += (TEXMEMORYUSED-lastTexMem);
			break;
		}
	case OC_FIELD:
		{
			U32 lastTexMem = TEXMEMORYUSED;
			BASE_FIELD_DATA *objData = (BASE_FIELD_DATA *)data;
			switch (objData->fieldClass)
			{
			case FC_ANTIMATTER:
				result = (HANDLE)CreateAntiMatterArchetype(ARCHLIST->GetArchetype(szArchname),(BT_ANTIMATTER_DATA *)data);
				ANTIMATTERTEXMEMUSED += (TEXMEMORYUSED-lastTexMem);
				break;
			case FC_ASTEROIDFIELD:
				{
					BT_ASTEROIDFIELD_DATA *objData = (BT_ASTEROIDFIELD_DATA *)data;
					AsteroidArchetype *dArch;
					dArch = new AsteroidArchetype;
					//dArch->name = szArchname;
					dArch->pData = objData;
					dArch->pArchetype = ARCHLIST->GetArchetype(szArchname);
					int i;
					for(i = 0; i <4; ++i)
					{
						if(dArch->pData->nuggetType[i][0])
							dArch->nuggetType[i] = ARCHLIST->LoadArchetype(dArch->pData->nuggetType[i]);
					}


					


										
					PFenum alphaPF;
					
					if (CQRENDERFLAGS.b32BitTextures)
						alphaPF = PF_4CC_DAA8;
					else
						alphaPF = PF_4CC_DAA4;
					
					const char * fname;
					
					if(CQEFFECTS.bExpensiveTerrain==0)
					{
						fname = dArch->pData->softwareTexClearName;
						if (fname[0])
						{
							dArch->softwareTexClearID = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAOT);
						}
						fname = dArch->pData->softwareTexFogName;
						if (fname[0])
						{
							dArch->softwareTexFogID = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, PF_4CC_DAOT);
						}
					}
					else
					{
						fname = dArch->pData->dustTexName;
						if (fname[0])
						{
							dArch->texID2 = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, alphaPF);
						}
						
						fname = dArch->pData->modTextureName;
						if (fname[0])
						{
							dArch->texID = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, alphaPF);
						}
						
						for (i=0;i<MAX_ASTEROID_TYPES;i++)
						{
							if (objData->fileName[i][0])
							{
								fdesc = objData->fileName[i];
								
								fdesc.lpImplementation = "UTF";
								
								if (OBJECTDIR->CreateInstance(&fdesc, file) != GR_OK)
								{
									CQFILENOTFOUND(fdesc.lpFileName);
									delete dArch;
									result = 0;
									goto Done;
								}
								
								if ((dArch->animArch[i] = ANIM2D->create_archetype(file)) == 0)
								{
									delete dArch;
									result = 0;
									goto Done;
								}
							}
						}
					}

					
/*					fname = dArch->pData->backgroundTexName;
					if (fname[0])
					{
						dArch->BG_tex = TMANAGER->CreateTextureFromFile(fname, TEXTURESDIR, DA::TGA, alphaPF);
					}*/
					if (dArch->pData->mapTexName[0])
					{
						dArch->mapTex = SYSMAP->RegisterIcon(dArch->pData->mapTexName);
					}else
					{
						dArch->mapTex = -1;
					}
					
					result = (HANDLE)dArch;
				}
				FIELDTEXTUREUSED += (TEXMEMORYUSED-lastTexMem);
				break;
				
			}
		}
		break;
	}


Done:

	return result;
}

BOOL32 FieldManager::DestroyArchetype(HANDLE hArchetype)
{
	FieldArchetype<BASIC_DATA> *deadguy = (FieldArchetype<BASIC_DATA> *)hArchetype;
	delete deadguy;
	NEBTEXTUREUSED = FIELDTEXTUREUSED = ANTIMATTERTEXMEMUSED = 0;

	return 1;
}

IBaseObject * FieldManager::CreateInstance(HANDLE hArchetype)
{
	FieldArchetype<BASIC_DATA> *pField = (FieldArchetype<BASIC_DATA> *)hArchetype;
	BASIC_DATA *_data = pField->pData;
	IField * field=NULL;
	
	if (_data->objClass == OC_FIELD)
	{
		BASE_FIELD_DATA *objData = (BASE_FIELD_DATA *)_data;
		switch (objData->fieldClass)
		{
		case FC_ASTEROIDFIELD:
			{
				AsteroidField * obj = new ObjectImpl<AsteroidField>;
				BT_ASTEROIDFIELD_DATA *data = (BT_ASTEROIDFIELD_DATA *)objData;
				obj->fieldType = FC_ASTEROIDFIELD;

				obj->pArchetype = 0;
				obj->objClass = OC_FIELD;
				obj->SetSystemID(SECTOR->GetCurrentSystem());
				obj->attributes = data->attributes;
				strcpy(obj->name, ARCHLIST->GetArchName(pField->pArchetype));
				field = obj;
				obj->Init(hArchetype);
			}
			break;
		case FC_ANTIMATTER:
			{
				field = CreateAntiMatter(pField);
			}
		}
	}
	
	if (_data->objClass == OC_NEBULA)
	{
		Nebula * obj = new ObjectImpl<Nebula>;
		BT_NEBULA_DATA *data = (BT_NEBULA_DATA *)_data;
		obj->pArchetype = 0;
		obj->objClass = OC_NEBULA;
		obj->fieldType = FC_NEBULA;

		obj->attributes = data->attributes;
		strcpy(obj->name,ARCHLIST->GetArchName(pField->pArchetype));
			
		field = obj;
		obj->Init(hArchetype);
		obj->SetSystemID(SECTOR->GetCurrentSystem());
	}

	if (_data->objClass == OC_MINEFIELD)
	{
		IField * obj = Minefield::CreateInstance(hArchetype);
		obj->fieldType = FC_MINEFIELD;
		field = obj;
		obj->objClass = OC_MINEFIELD;

		obj->Init(hArchetype);
	}

	if (field)
	{
		if (!fieldList)
		{
			fieldList = field;
			fieldList->nextField = NULL;
		}
		else
		{
			field->nextField = fieldList;
			fieldList = field;
		}
		field->factory = this;
		
	}

	return field;
}

void FieldManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	Edit(hArchetype);
}

//----------------------------------------------------------------------------------------------
//
struct _cloud : GlobalComponent
{
	struct FieldManager *fieldMgr;
	virtual void Startup (void)
	{
		FIELDMGR = fieldMgr = new DAComponent<FieldManager>;
		AddToGlobalCleanupList(&FIELDMGR);
	}

	virtual void Initialize (void)
	{
		fieldMgr->Init();
	}
};

static _cloud cloud;

//--------------------------------------------------------------------------//
//----------------------------END Cloud.cpp------------------------------//
//--------------------------------------------------------------------------//
