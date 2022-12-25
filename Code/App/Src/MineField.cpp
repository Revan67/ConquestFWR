//--------------------------------------------------------------------------//
//                                                                          //
//                             Minefield.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MineField.cpp 77    10/18/00 1:41a Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Minefield.h"
#include "IMissionActor.h"
#include "IConnection.h"
#include "DMinefield.h"
#include "Field.h"
#include "DField.h"
#include "Anim2d.h"
#include "Camera.h"
#include "Objlist.h"
#include "IWeapon.h"
#include "MGlobals.h"
#include "Startup.h"
#include "IBlast.h"
#include "Sector.h"
#include "Mission.h"
#include "UserDefaults.h"
#include "FogOfWar.h"
#include "OpAgent.h"
#include "IBlast.h"
#include "MPart.h"
#include "ObjMapIterator.h"
#include "ObjMap.h"
#include "Sysmap.h"

#include <FileSys.h>

#define MINE_ACCELERATION 50

#define FADE_INC 25

#define Z_SIZE 200

struct MinefieldArchetype : DefaultArchetype<BT_MINEFIELD_DATA>
{
	~MinefieldArchetype (void)
	{
		if(regAnimArchetype)
			delete regAnimArchetype;
		if(blastType)
			ARCHLIST->Release(blastType, OBJREFNAME);
	}

	U32 currentSquare;
	AnimArchetype * regAnimArchetype;
	PARCHETYPE blastType;

	virtual void EndEdit();

	virtual void Edit();
};

void MinefieldArchetype::EndEdit()
{	
	if (laidSquare == 0)
		return;
	
	laidSquare = FALSE;
	
	IField *obj;
	currentSquare = 0;
	for(U32 squareIndex = 0; squareIndex < numSquares;++squareIndex)
	{
		obj = (IField *)MGlobals::CreateInstance(pArchetype, MGlobals::CreateNewPartID(MGlobals::GetThisPlayer()));
		if (obj==0)
			return;
		obj->Setup();
		++currentSquare;
		OBJLIST->AddObject(obj);
	}
}

void MinefieldArchetype::Edit()
{
	snapping = true;
	snapX = 0;
	snapY = 0;
	numSquares = 0;
}

//---------------------------------------------------------------------------
//
Minefield::Minefield (void) :
		genSyncNode(this, SyncGetProc(&Minefield::getSyncData), SyncPutProc(&Minefield::putSyncData)),
		explodeNode(this, ExplodeProc(&Minefield::explodeMines))
{
}
//---------------------------------------------------------------------------
//
Minefield::~Minefield()
{
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(systemID, map);
	unsetTerrainFootprint(map);

	if(mines)
		delete mines;
}
//---------------------------------------------------------------------------
//
BOOL32 Minefield::Update (void)
{
	if(THEMATRIX->IsMaster())
	{
		if(updateCounter > 3)
		{	
			updateCounter = 0;
			COMPTR<ITerrainMap> map;
			SECTOR->GetTerrainMap(systemID,map);
	
			map->TestSegment(gridPos,gridPos,this);
		}else
			++updateCounter;
	}

	if(IsVisibleToPlayer(MGlobals::GetThisPlayer()))
	{
		if(fade != 255)
		{
			if(bVisible)
			{
				if(fade <= 255-FADE_INC)
					fade += FADE_INC;
				else 
					fade = 255;
			}else
			{
				fade = 255;
			}
			U32 mineIndex = 0;
			U32 mineLimit = archetype->pData->maxMineNumber;
			while(mineIndex < mineLimit)
			{
				if(mines[mineIndex].bActive)
					mines[mineIndex].SetFade(fade);
				++mineIndex;
			}
		}
	}
	else
	{
		if(fade != 0)
		{
			fade = 0;
			U32 mineIndex = 0;
			U32 mineLimit = archetype->pData->maxMineNumber;
			while(mineIndex < mineLimit)
			{
				mines[mineIndex].SetFade(fade);
				++mineIndex;
			}
		}
	}
	//supplies in a minefield are actualy the number of active mines
	//Mines may be needed to fade in or out based on this
	if(bUpdateFading)
	{
		bUpdateFading = false;
		U32 mineIndex = 0;
		U32 mineLimit = archetype->pData->maxMineNumber;
		while((supplies > hullPoints) && (mineIndex < mineLimit))
		{
			Mine * mine = &(mines[mineIndex]);
			if((mine->bActive) )
			{
				--supplies;
			}
			++mineIndex;
		}
		mineIndex = 0;
		while((supplies < hullPoints) && (mineIndex < mineLimit))
		{
			Mine * mine = &(mines[mineIndex]);
			if(!(mine->bActive))
			{
				mine->bActive = true;
				mine->velocity.x = (rand()%2000)*0.001 - 1.0;
				mine->velocity.y = (rand()%2000)*0.001 - 1.0;
				mine->velocity.z = (rand()%2000)*0.001 - 1.0;
				if((mine->velocity.x == 0) && (mine->velocity.y == 0) && (mine->velocity.z == 0))
					mine->velocity = Vector(0.5,0.5,1);
				if(mine->velocity.z <= 0.2)
					mine->velocity.z = 0.5;
				mine->velocity.normalize();
				mine->velocity.x *= rand()%archetype->pData->maxHorizontalVelocity;
				mine->velocity.y *= rand()%archetype->pData->maxHorizontalVelocity;
				mine->velocity.z *= rand()%archetype->pData->maxVerticalVelocity;
				Vector tmpVect;
				Vector centVect = gridPos;
				tmpVect.x = centVect.x+(rand()%(GRIDSIZE))-((SINGLE)(GRIDSIZE/2));
				tmpVect.y = centVect.y+(rand()%(GRIDSIZE))-((SINGLE)(GRIDSIZE/2));
				tmpVect.z = (rand()%(Z_SIZE*2))-((SINGLE)(Z_SIZE));
				mine->regAnim.SetPosition(tmpVect);
				mine->mineCenter = tmpVect;
				++supplies;
			}
			++mineIndex;
		}
	}
	if((hullPoints == 0) && (layerRef == 0) && THEMATRIX->IsMaster())
		THEMATRIX->ObjectTerminated(dwMissionID, 0);

	return 1;
}
//---------------------------------------------------------------------------
//
bool Minefield::TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
{
	if ((info.flags & (TERRAIN_PARKED|TERRAIN_MOVING)) != 0 && (info.flags & TERRAIN_DESTINATION)==0)
	{
		IBaseObject * obj = OBJLIST->FindObject(info.missionID);
		if (obj)
			return hitObject(obj);
	}
	return true;
}
//---------------------------------------------------------------------------
//
void Minefield::PhysicalUpdate (SINGLE dt)
{
	dt += storedDelay;
	if(bVisible || (dt > 0.5))
	{
		storedDelay = 0;
		for(U32 mineIndex = 0; mineIndex < archetype->pData->maxMineNumber; ++mineIndex)
		{
			Mine * mine = &(mines[mineIndex]);
			if(mine->bActive)
			{
				if(archetype->regAnimArchetype)
				{
					mine->regAnim.update(dt);
					if(bVisible || layerRef)
					{
						Vector vect = mine->regAnim.GetPosition();
						vect.x += mine->velocity.x*dt;
						vect.y += mine->velocity.y*dt;
						vect.z += mine->velocity.z*dt;
						mine->regAnim.SetPosition(vect);
						SINGLE maxHVel = archetype->pData->maxHorizontalVelocity;
						SINGLE acceleration = archetype->pData->mineAcceleration*dt;
						SINGLE maxVVel = archetype->pData->maxVerticalVelocity;
						if((vect.x >= mine->mineCenter.x) && (mine->velocity.x > -maxHVel))
						{
							if(mine->velocity.x > maxHVel)
								mine->velocity.x -= acceleration*10;
							mine->velocity.x -= acceleration;
							if(mine->velocity.x < -maxHVel)
								mine->velocity.x = -maxHVel;
						}else if((vect.x < mine->mineCenter.x) && (mine->velocity.x < maxHVel))
						{
							if(mine->velocity.x < -maxHVel)
								mine->velocity.x += acceleration*10;
							mine->velocity.x += acceleration;
							if(mine->velocity.x > maxHVel)
								mine->velocity.x = maxHVel;
						}
						if((vect.y >= mine->mineCenter.y) && (mine->velocity.y > -maxHVel))
						{
							if(mine->velocity.y > maxHVel)
								mine->velocity.y -= acceleration*10;
							mine->velocity.y -= acceleration;
							if(mine->velocity.y < -maxHVel)
								mine->velocity.y = -maxHVel;
						}else if((vect.y < mine->mineCenter.y) && (mine->velocity.y < maxHVel))
						{
							if(mine->velocity.y < -maxHVel)
								mine->velocity.y += acceleration*10;
							mine->velocity.y += acceleration;
							if(mine->velocity.y > maxHVel)
								mine->velocity.y = maxHVel;
						}
						if((vect.z >= mine->mineCenter.z) && (mine->velocity.z > -maxVVel))
						{
							if(mine->velocity.z > maxVVel)
								mine->velocity.z -= acceleration*10;
							mine->velocity.z -= acceleration;
							if(mine->velocity.z < -maxVVel)
								mine->velocity.z = -maxVVel;
						}else if((vect.z < mine->mineCenter.z) && (mine->velocity.z < maxVVel))
						{
							if(mine->velocity.z < -maxVVel)
								mine->velocity.z += acceleration*10;
							mine->velocity.z += acceleration;
							if(mine->velocity.z > maxVVel)
								mine->velocity.z = maxVVel;
						}
					}
				}
			}
			
		}
	}
	else
	{
		storedDelay = dt;
	}
}
//---------------------------------------------------------------------------
//
bool Minefield::hitObject(IBaseObject * obj)
{
	if((obj->GetPlayerID() == 0) || MGlobals::AreAllies(obj->GetPlayerID(),GetPlayerID()))
	{
		return true;
	}
	bDetonating = true;
	OBJPTR<IWeaponTarget> targ;
	obj->QueryInterface(IWeaponTargetID,targ);
	if(targ != 0)
	{
		Vector collide_point,dir;
		Vector minePos;
		
		for(int mineIndex = 0; mineIndex < (S32)archetype->pData->maxMineNumber; ++mineIndex)
		{
			Mine * mine = &(mines[mineIndex]);
			if(mine->bActive)
			{				
				minePos = mine->regAnim.GetPosition();
				supplies--;
				mine->bActive = false;
				if(archetype->blastType != 0)
				{
					IBaseObject * obj = ARCHLIST->CreateInstance(archetype->blastType);
					if(obj)
					{
						OBJPTR<IBlast> blast;
						obj->QueryInterface(IBlastID,blast);
						if(blast != 0)
						{
							Transform blastTrans;
							blastTrans.set_position(mine->regAnim.GetPosition());
							blast->InitBlast(blastTrans,systemID,NULL);
							OBJLIST->AddObject(obj);
						}
					}
				}
						
				ObjMapIterator iter(systemID,gridPos,archetype->pData->explosionRange*GRIDSIZE,0);
				while(iter)
				{
					if(iter->flags & OM_TARGETABLE)
					{
						SINGLE dist = gridPos - iter->obj->GetGridPosition();
						if(dist <= archetype->pData->explosionRange)
						{
							iter->obj->QueryInterface(IWeaponTargetID,targ);
							Vector look = iter->obj->GetPosition()-minePos;
							look.normalize();
							if (targ)
								targ->ApplyDamage(this,GetPartID(),minePos,look,archetype->pData->damagePerHit,true);
							MPartNC part(iter->obj);
							if(part.isValid())
							{
								if(part->supplies < archetype->pData->supplyDamagePerHit)
									part->supplies = 0;
								else
									part->supplies -= archetype->pData->supplyDamagePerHit;
							}			
						}
					}
					++iter;
				}
			}
		}
	}
	hullPoints = 0;
	return false;
}
//---------------------------------------------------------------------------
//
void Minefield::Render (void)
{
	if ((systemID == SECTOR->GetCurrentSystem()) && bVisible)
	{
		//draw existing mines
		CAMERA->SetModelView();
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		BATCH->set_state(RPR_BATCH,FALSE);

		if(archetype->regAnimArchetype)
		{
			ANIM2D->start_batch(archetype->regAnimArchetype->frames[0].texture);
			for(U32 mineIndex = 0; mineIndex < archetype->pData->maxMineNumber; ++mineIndex)
			{
				if(mines[mineIndex].bActive)
				{
					ANIM2D->render_instance(&(mines[mineIndex].regAnim));
				}
			}
			ANIM2D->end_batch();
		}
	}
}
//---------------------------------------------------------------------------
//
void Minefield::MapRender(bool bPing)
{
}
//---------------------------------------------------------------------------
//

void Minefield::View (void)
{
	MINEFIELD_VIEW mView;
	mView.mineNumber = hullPoints;
	if (DEFAULTS->GetUserData("MINEFIELD_VIEW", " ", &mView, sizeof(mView)))
	{
		if( mView.mineNumber <= 0)
			mView.mineNumber = 1;
		else if(mView.mineNumber > archetype->pData->maxMineNumber)
			mView.mineNumber = archetype->pData->maxMineNumber;
		if(mView.mineNumber < (U32)hullPoints)//removeing mines
		{
			int diff = hullPoints-mView.mineNumber;
			U32 mineIndex = 0;
			while((mineIndex <archetype->pData->maxMineNumber) && diff)
			{
				if(mines[mineIndex].bActive)
				{
					--diff;
					mines[mineIndex].bActive = false;
					--supplies;
				}
				++mineIndex;
			}
		}else if(mView.mineNumber > (U32)hullPoints)//adding mines
		{
			int diff = mView.mineNumber-hullPoints;
			U32 mineIndex = 0;
			while((mineIndex <archetype->pData->maxMineNumber) && diff)
			{
				if(!mines[mineIndex].bActive)
				{
					--diff;
					++supplies;
					mines[mineIndex].bActive = true;
					mines[mineIndex].velocity.x = (rand()%2000)*0.001 - 1.0;
					mines[mineIndex].velocity.y = (rand()%2000)*0.001 - 1.0;
					mines[mineIndex].velocity.z = (rand()%2000)*0.001 - 1.0;
					if((mines[mineIndex].velocity.x == 0) && 
							(mines[mineIndex].velocity.y == 0) && 
							(mines[mineIndex].velocity.z == 0))
						mines[mineIndex].velocity = Vector(1,1,0.5);
					if(mines[mineIndex].velocity.z <= 0.2)
						mines[mineIndex].velocity.z = 0.5;
					mines[mineIndex].velocity.normalize();

					mines[mineIndex].velocity.x *= rand()%archetype->pData->maxHorizontalVelocity;
					mines[mineIndex].velocity.y *= rand()%archetype->pData->maxHorizontalVelocity;
					mines[mineIndex].velocity.z *= rand()%archetype->pData->maxVerticalVelocity;
					Vector myPos = gridPos;
					Vector tmpVect;
					tmpVect.x = myPos.x+(rand()%(GRIDSIZE))-((SINGLE)(GRIDSIZE/2));
					tmpVect.y = myPos.y+(rand()%(GRIDSIZE))-((SINGLE)(GRIDSIZE/2));
					tmpVect.z = (rand()%(Z_SIZE*2))-((SINGLE)(Z_SIZE));
					mines[mineIndex].regAnim.SetPosition(tmpVect);
					mines[mineIndex].mineCenter = tmpVect;
				}
				++mineIndex;
			}
		}
		hullPoints = mView.mineNumber;
	}

}
//---------------------------------------------------------------------------
//
void Minefield::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	bVisible = (GetSystemID() == currentSystem &&
			   (IsVisibleToPlayer(currentPlayer) ||
				 defaults.bVisibilityRulesOff ||
				 defaults.bEditorMode) );	
	if(bVisible)
	{
		//as long as the cammera never goes on it's side this will work
		RECT screenRect = { 0, 0, SCREENRESX, SCREENRESY };
		RECT rect;
		S32 xCent,yCent,zXLoc,zYLoc;
		Vector myPos = gridPos;
		CAMERA->PointToScreen(Vector(myPos.x,myPos.y,0),&xCent,&yCent);
		CAMERA->PointToScreen(Vector(myPos.x,myPos.y,Z_SIZE*2),&zXLoc,&zYLoc);
		S32 width =(yCent-zYLoc)*2;
		width = width*width;
		rect.top = yCent-width;
		rect.left = xCent-width;
		rect.bottom = yCent+width;
		rect.right = xCent+width;
		bVisible = RectIntersects(rect, screenRect);
	}
}
//---------------------------------------------------------------------------
//
void Minefield::CastVisibleArea()
{
	const U32 mask = MGlobals::GetAllyMask(playerID);
	SetVisibleToAllies(mask);
}
//---------------------------------------------------------------------------
//
void Minefield::setTerrainFootprint (struct ITerrainMap * terrainMap)
{
	FootprintInfo info;
	info.missionID = dwMissionID;
	info.height = 0;
	info.flags = TERRAIN_FIELD | TERRAIN_FULLSQUARE;
	terrainMap->SetFootprint(&gridPos,1,info);

	if(map_square == -1)
	{
		map_square = OBJMAP->GetMapSquare(systemID,gridPos);
		OBJMAP->AddObjectToMap(this,systemID,map_square,0);
	}
}
//---------------------------------------------------------------------------
//
void Minefield::unsetTerrainFootprint (struct ITerrainMap * terrainMap)
{
	if (gridPos.isZero() == 0)
	{
		FootprintInfo info;
		info.missionID = dwMissionID;
		info.height = 0;
		info.flags = TERRAIN_FIELD | TERRAIN_FULLSQUARE;
		terrainMap->UndoFootprint(&gridPos,1,info);

		if(map_square != -1)
		{
			OBJMAP->RemoveObjectFromMap(this,systemID,map_square);
			map_square = -1;
		}
	}
}
//---------------------------------------------------------------------------
//
Vector Minefield::GetCenterPos(void)
{
	Vector vec;
	vec = gridPos;
	return vec;
}
//---------------------------------------------------------------------------
//
BOOL32 Minefield::ObjInField (IBaseObject *obj)
{
	return false;
}
//---------------------------------------------------------------------------
//

BOOL32 Minefield::Save(IFileSystem *outFile)
{
	DAFILEDESC fdesc;
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;

	DWORD dwWritten;

	MINEFIELD_SAVELOAD save;
	U32 saveableMines = 0;
	U32 index;

	fdesc.lpFileName = "MINEFIELD_SAVELOAD";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memcpy(&save, static_cast<BASE_MINEFIELD_SAVELOAD *>(this), sizeof(BASE_MINEFIELD_SAVELOAD));
	FRAME_save(save);

	if (file->WriteFile(0,&save,sizeof(save),&dwWritten,0) == 0)
		goto Done;
	
	fdesc.lpFileName = "Data";
	
	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	for(index = 0; index < archetype->pData->maxMineNumber; ++index)
	{
		Mine * mine = &(mines[index]);
		if((mine->bActive)  && (!mine->bFadeOut))
			++saveableMines;
	}
	if (file->WriteFile(0, &saveableMines, sizeof(saveableMines), &dwWritten, 0) == 0)
		goto Done;

	for(index = 0; index < archetype->pData->maxMineNumber; ++index)
	{
		Mine * mine = &(mines[index]);
		if((mine->bActive)  && (!mine->bFadeOut))
		{
			if (file->WriteFile(0, &(mine->mineCenter), sizeof(mine->mineCenter), &dwWritten, 0) == 0)
				goto Done;
		}
	}

	file.free();
	
	result = 1;

Done:

	return result;
}
//---------------------------------------------------------------------------
//

BOOL32 Minefield::Load(IFileSystem *inFile)
{
	MINEFIELD_SAVELOAD load;
	DAFILEDESC fdesc;
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	U32 loadableMines = 0;
	U32 index;
	DWORD dwRead;
	Vector pos;
	U8 buffer[sizeof(MINEFIELD_SAVELOAD)*2];

	fdesc.lpFileName = "MINEFIELD_SAVELOAD";
	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
	{
		goto Done;
	}

	memset(buffer, 0, sizeof(buffer));
	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("MINEFIELD_SAVELOAD", buffer, &load);

	memcpy(static_cast<BASE_MINEFIELD_SAVELOAD *>(this), &load, sizeof(BASE_MINEFIELD_SAVELOAD));
	FRAME_load(load);

	fdesc.lpFileName = "Data";
	
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;
	
	if (file->ReadFile(0, &loadableMines, sizeof(loadableMines), &dwRead, 0) == 0)
		goto Done;
	supplies = 0;
	for(index = 0; index < loadableMines; ++index)
	{
		Mine * mine = &(mines[index]);
		Vector vect;
		if (file->ReadFile(0, &(mine->mineCenter), sizeof(mine->mineCenter), &dwRead, 0) == 0)
			goto Done;
		mine->regAnim.SetPosition(mine->mineCenter);
		mine->velocity.x = (rand()%2000)*0.001 - 1.0;
		mine->velocity.y = (rand()%2000)*0.001 - 1.0;
		mine->velocity.z = (rand()%2000)*0.001 - 1.0;
		if((mine->velocity.x == 0) && 
				(mine->velocity.y == 0) && 
				(mine->velocity.z == 0))
			mine->velocity = Vector(1,1,0.5);
		if(fabs(mine->velocity.z) <= 0.2)
			mine->velocity.z = 0.5;
		mine->velocity.normalize();

		mine->velocity.x *= rand()%archetype->pData->maxHorizontalVelocity;
		mine->velocity.y *= rand()%archetype->pData->maxHorizontalVelocity;
		mine->velocity.z *= rand()%archetype->pData->maxVerticalVelocity;

		mine->bActive = true;
		mine->bFadeIn = false;
		mine->bFadeOut = false;
		mine->fade = 255;
		mine->SetFade();
		++supplies;
	}

	strcpy(name,partName);

	{
		COMPTR<ITerrainMap> map;
		SECTOR->GetTerrainMap(systemID, map);
		setTerrainFootprint(map);
	}

	netHullPoints = hullPoints;

	result = 1;

Done:

	return result;
}
//---------------------------------------------------------------------------
//
U32 Minefield::getSyncData (void * buffer)
{
	if(bDetonating)
	{
		bDetonating = false;
		((U8 *)buffer)[0] = 255;
		return 1;
	}
	else if(hullPoints != netHullPoints)
	{
		((U8 *)buffer)[0] = hullPoints;
		netHullPoints = hullPoints;
		return 1;		
	}
	return 0;
}
//---------------------------------------------------------------------------
//
void Minefield::putSyncData (void * buffer, U32 bufferSize, bool bLateDelivery)
{
	CQASSERT(bLateDelivery==false);
	U8 value = ((U8 *)buffer)[0];
	if(value == 255)
	{
		Vector minePos;
		
		for(int mineIndex = 0; mineIndex < (S32)archetype->pData->maxMineNumber; ++mineIndex)
		{
			Mine * mine = &(mines[mineIndex]);
			if(mine->bActive)
			{				
				minePos = mine->regAnim.GetPosition();
				supplies--;
				mine->bActive = false;
				if(archetype->blastType != 0)
				{
					IBaseObject * obj = ARCHLIST->CreateInstance(archetype->blastType);
					if(obj)
					{
						OBJPTR<IBlast> blast;
						obj->QueryInterface(IBlastID,blast);
						if(blast != 0)
						{
							Transform blastTrans;
							blastTrans.set_position(mine->regAnim.GetPosition());
							blast->InitBlast(blastTrans,systemID,NULL);
							OBJLIST->AddObject(obj);
						}
					}
				}
			}
		}
		hullPoints = 0;
	}
	else
	{
		hullPoints = value;
		bUpdateFading = true;
	}
}
//---------------------------------------------------------------------------
//
void Minefield::explodeMines (bool bExplode)
{
	Vector minePos;
	
	for(int mineIndex = 0; mineIndex < (S32)archetype->pData->maxMineNumber; ++mineIndex)
	{
		Mine * mine = &(mines[mineIndex]);
		if(mine->bActive)
		{				
			minePos = mine->regAnim.GetPosition();
			supplies--;
			mine->bActive = false;
			if(archetype->blastType != 0)
			{
				IBaseObject * obj = ARCHLIST->CreateInstance(archetype->blastType);
				if(obj)
				{
					OBJPTR<IBlast> blast;
					obj->QueryInterface(IBlastID,blast);
					if(blast != 0)
					{
						Transform blastTrans;
						blastTrans.set_position(mine->regAnim.GetPosition());
						blast->InitBlast(blastTrans,systemID,NULL);
						OBJLIST->AddObject(obj);
					}
				}
			}
		}
	}
	hullPoints = 0;
}
//---------------------------------------------------------------------------
//
void Minefield::ApplyAOEDamage (U32 ownerID, U32 damageAmount)
{
	SYSMAP->RegisterAttack(systemID,GetGridPosition(),playerID);
	if(THEMATRIX->IsMaster())
	{
		bDetonating = true;
		Vector collide_point,dir;
		Vector minePos;
		
		for(int mineIndex = 0; mineIndex < (S32)archetype->pData->maxMineNumber; ++mineIndex)
		{
			Mine * mine = &(mines[mineIndex]);
			if(mine->bActive)
			{				
				minePos = mine->regAnim.GetPosition();
				supplies--;
				mine->bActive = false;
				if(archetype->blastType != 0)
				{
					IBaseObject * obj = ARCHLIST->CreateInstance(archetype->blastType);
					if(obj)
					{
						OBJPTR<IBlast> blast;
						obj->QueryInterface(IBlastID,blast);
						if(blast != 0)
						{
							Transform blastTrans;
							blastTrans.set_position(mine->regAnim.GetPosition());
							blast->InitBlast(blastTrans,systemID,NULL);
							OBJLIST->AddObject(obj);
						}
					}
				}
						
				ObjMapIterator iter(systemID,gridPos,archetype->pData->explosionRange*GRIDSIZE,0);
				while(iter)
				{
					if(iter->flags & OM_TARGETABLE)
					{
						SINGLE dist = gridPos - iter->obj->GetGridPosition();
						if(dist <= archetype->pData->explosionRange)
						{
							OBJPTR<IWeaponTarget> targ;
							iter->obj->QueryInterface(IWeaponTargetID,targ);
							Vector look = iter->obj->GetPosition()-minePos;
							look.normalize();
							if (targ)
								targ->ApplyDamage(this,GetPartID(),minePos,look,archetype->pData->damagePerHit,true);
							MPartNC part(iter->obj);
							if(part.isValid())
							{
								if(part->supplies < archetype->pData->supplyDamagePerHit)
									part->supplies = 0;
								else
									part->supplies -= archetype->pData->supplyDamagePerHit;
							}			
						}
					}
					++iter;
				}
			}
		}
		hullPoints = 0;
	}
	return;
}	
//---------------------------------------------------------------------------
//
BOOL32 Minefield::ApplyDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 shieldHit)
{
	SYSMAP->RegisterAttack(systemID,GetGridPosition(),playerID);
	if(THEMATRIX->IsMaster())
	{
		bDetonating = true;
		Vector collide_point,dir;
		Vector minePos;
		
		for(int mineIndex = 0; mineIndex < (S32)archetype->pData->maxMineNumber; ++mineIndex)
		{
			Mine * mine = &(mines[mineIndex]);
			if(mine->bActive)
			{				
				minePos = mine->regAnim.GetPosition();
				supplies--;
				mine->bActive = false;
				if(archetype->blastType != 0)
				{
					IBaseObject * obj = ARCHLIST->CreateInstance(archetype->blastType);
					if(obj)
					{
						OBJPTR<IBlast> blast;
						obj->QueryInterface(IBlastID,blast);
						if(blast != 0)
						{
							Transform blastTrans;
							blastTrans.set_position(mine->regAnim.GetPosition());
							blast->InitBlast(blastTrans,systemID,NULL);
							OBJLIST->AddObject(obj);
						}
					}
				}
						
				ObjMapIterator iter(systemID,gridPos,archetype->pData->explosionRange*GRIDSIZE,0);
				while(iter)
				{
					if(iter->flags & OM_TARGETABLE)
					{
						SINGLE dist = gridPos - iter->obj->GetGridPosition();
						if(dist <= archetype->pData->explosionRange)
						{
							OBJPTR<IWeaponTarget> targ;
							iter->obj->QueryInterface(IWeaponTargetID,targ);
							Vector look = iter->obj->GetPosition()-minePos;
							look.normalize();
							if (targ)
								targ->ApplyDamage(this,GetPartID(),minePos,look,archetype->pData->damagePerHit,true);
							MPartNC part(iter->obj);
							if(part.isValid())
							{
								if(part->supplies < archetype->pData->supplyDamagePerHit)
									part->supplies = 0;
								else
									part->supplies -= archetype->pData->supplyDamagePerHit;
							}			
						}
					}
					++iter;
				}
			}
		}
		hullPoints = 0;
	}
	return false;
}
//---------------------------------------------------------------------------
//
BOOL32 Minefield::ApplyVisualDamage (IBaseObject * collider, U32 ownerID, const Vector & pos, const Vector &dir,U32 amount,BOOL32 shieldHit)
{
	return false;
}
//---------------------------------------------------------------------------
//
BOOL32 Minefield::GetCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction)
{
	return false;
}
//---------------------------------------------------------------------------
//
BOOL32 Minefield::GetModelCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction)
{
	return false;
}
//---------------------------------------------------------------------------
//
void Minefield::AttachBlast(PARCHETYPE pBlast,const Vector &pos,const Vector &dir)
{
}
//---------------------------------------------------------------------------
//
void Minefield::AttachBlast(PARCHETYPE pBlast,const Transform & baseTrans)
{
}
//---------------------------------------------------------------------------
//
void Minefield::AddLayerRef()
{
	++layerRef;
}
//---------------------------------------------------------------------------
//
U32 Minefield::GetLayerRef()
{
	return layerRef;
}
//---------------------------------------------------------------------------
//
void Minefield::SubLayerRef()
{
	--layerRef;
}
//---------------------------------------------------------------------------
//
void Minefield::AddMine(Vector & pos,Vector & velocity)
{
	if(hullPoints < ((S32)archetype->pData->maxMineNumber))
	{
		++hullPoints;
		++supplies;
		U32 mineIndex = 0;
		while(mineIndex <archetype->pData->maxMineNumber)
		{
			if(!mines[mineIndex].bActive)
			{
				mines[mineIndex].bActive = true;
				mines[mineIndex].bFadeOut = false;
				mines[mineIndex].bFadeIn = false;
				mines[mineIndex].fade = 255;
				mines[mineIndex].SetFade();
				mines[mineIndex].velocity = velocity;
				mines[mineIndex].regAnim.SetPosition(pos);
				Vector tmpVect;
				Vector centVect = gridPos;
				tmpVect.x = centVect.x+(rand()%(GRIDSIZE))-((SINGLE)(GRIDSIZE/2));
				tmpVect.y = centVect.y+(rand()%(GRIDSIZE))-((SINGLE)(GRIDSIZE/2));
				tmpVect.z = (rand()%(Z_SIZE*2))-((SINGLE)(Z_SIZE));
				mines[mineIndex].mineCenter = tmpVect;
				break;
			}
			++mineIndex;
		}
	}
}
//---------------------------------------------------------------------------
//
void Minefield::SetMineNumber(U32 mineNumber)
{
	hullPoints = mineNumber;//mines should fade automaticaly as needed
}
//---------------------------------------------------------------------------
//
void Minefield::InitMineField( GRIDVECTOR _gridPos, U32 _systemID)
{
	COMPTR<ITerrainMap> map;
	systemID = _systemID;
	SECTOR->GetTerrainMap(systemID, map);
	unsetTerrainFootprint(map);
	gridPos = _gridPos;
	setTerrainFootprint(map);
	M_STRING buffer;
	_itoa(dwMissionID,buffer,10);
	partName = ARCHLIST->GetArchName(archetype->pArchetype);
	strcat(partName," ");
	strcat(partName,buffer);
	strcpy(name,partName);
}
//---------------------------------------------------------------------------
//
U32 Minefield::MaxMineNumber ()
{
	return archetype->pData->maxMineNumber;
}
//---------------------------------------------------------------------------
//
BOOL32 Minefield::AtPosition(GRIDVECTOR _gridPos)
{
	return gridPos == _gridPos;
}
//---------------------------------------------------------------------------
//
void Minefield::initMission ()
{
	supplies = 0;
	hullPoints = archetype->pData->maxMineNumber;

	M_STRING buffer;
	_itoa(dwMissionID,buffer,10);

	partName = ARCHLIST->GetArchName(archetype->pArchetype);
	strcat(partName," ");
	strcat(partName,buffer);
	systemID = SECTOR->GetCurrentSystem();

}
//---------------------------------------------------------------------------
//
BOOL32 Minefield::Init (HANDLE _hArchetype)
{
	archetype = (MinefieldArchetype *) _hArchetype;
	pArchetype = archetype->pArchetype;
	FRAME_init(*archetype);
	storedDelay = 0;
	updateCounter = 0;
	hullPoints = 0;
	supplies = 0;
	bUpdateFading = false;
	bCloaked = true;
	fade = 255;
	map_square = -1;

	mines = new Mine[archetype->pData->maxMineNumber];
	for(U32 mineIndex = 0;mineIndex <  archetype->pData->maxMineNumber;++mineIndex)
	{
		mines[mineIndex].bActive = false;
		if(archetype->regAnimArchetype)
		{
			mines[mineIndex].regAnim.Init(archetype->regAnimArchetype);
			mines[mineIndex].regAnim.delay = 0;
			mines[mineIndex].regAnim.SetRotation(2*PI*(SINGLE)rand()/RAND_MAX);
			mines[mineIndex].regAnim.SetWidth(archetype->pData->mineWidth);
			mines[mineIndex].regAnim.loop = TRUE;
			mines[mineIndex].regAnim.Randomize();
		}
	}
	return true;
}

//---------------------------------------------------------------------------
//
BOOL32 Minefield::Setup()//XYCoord *_squares,U32 _numSquares)
{
	CQASSERT(archetype->numSquares > archetype->currentSquare);
	COMPTR<ITerrainMap> map;

	initMission();

	Vector pos = Vector(archetype->squares[archetype->currentSquare].x,archetype->squares[archetype->currentSquare].y,0);

	if(systemID)
	{
		SECTOR->GetTerrainMap(systemID, map);
		if (map)
			unsetTerrainFootprint(map);
		gridPos = pos;
		if (map)
			setTerrainFootprint(map);
	}

	strcpy(name,partName);
	if(hullPoints)
	{
		for(int mineIndex = 0; mineIndex < hullPoints; ++mineIndex)
		{
			++supplies;
			mines[mineIndex].bActive = true;
			mines[mineIndex].bFadeOut = false;
			mines[mineIndex].bFadeIn = false;
			mines[mineIndex].fade = 255;
			mines[mineIndex].SetFade();
			mines[mineIndex].velocity.x = (rand()%2000)*0.001 - 1.0;
			mines[mineIndex].velocity.y = (rand()%2000)*0.001 - 1.0;
			mines[mineIndex].velocity.z = (rand()%2000)*0.001 - 1.0;
			if((mines[mineIndex].velocity.x == 0) && (mines[mineIndex].velocity.y == 0) && (mines[mineIndex].velocity.z == 0))
				mines[mineIndex].velocity = Vector(1,1,0.5);
			if(fabs(mines[mineIndex].velocity.z) <= 0.2)
				mines[mineIndex].velocity.z = 0.5;
			mines[mineIndex].velocity.normalize();
			mines[mineIndex].velocity.x *= rand()%archetype->pData->maxHorizontalVelocity;
			mines[mineIndex].velocity.y *= rand()%archetype->pData->maxHorizontalVelocity;
			mines[mineIndex].velocity.z *= rand()%archetype->pData->maxVerticalVelocity;
			Vector tmpVect;
			Vector centVect = gridPos;
			tmpVect.x = centVect.x+(rand()%(GRIDSIZE))-((SINGLE)(GRIDSIZE/2));
			tmpVect.y = centVect.y+(rand()%(GRIDSIZE))-((SINGLE)(GRIDSIZE/2));
			tmpVect.z = (rand()%(Z_SIZE*2))-((SINGLE)(Z_SIZE));
			mines[mineIndex].regAnim.SetPosition(tmpVect);
			mines[mineIndex].mineCenter = tmpVect;
		}
	}
	return true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
HANDLE Minefield::CreateArchetype (PARCHETYPE pArchetype, OBJCLASS objClass, void *data)
{
	MinefieldArchetype *mArch;
	mArch = new MinefieldArchetype;
	mArch->pArchetype = pArchetype;
	mArch->pData = (BT_MINEFIELD_DATA *)data;

	DAFILEDESC fdesc;
	COMPTR<IFileSystem> objFile;
	fdesc.lpFileName = mArch->pData->regAnimation;
	if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
	{
		mArch->regAnimArchetype = ANIM2D->create_archetype(objFile);
	}
	else 
	{
		CQFILENOTFOUND(fdesc.lpFileName);
		mArch->regAnimArchetype =0;
	}

	if (mArch->pData->blastType[0])
	{
		mArch->blastType = ARCHLIST->LoadArchetype(mArch->pData->blastType);
		CQASSERT(mArch->blastType);
		ARCHLIST->AddRef(mArch->blastType, OBJREFNAME);
	}

/*	fdesc.lpFileName = mArch->pData->explodingAnimation;
	if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
	{
		mArch->explodeAnimArchetype = ANIM2D->create_archetype(objFile);
	}
	else 
	{
		CQFILENOTFOUND(fdesc.lpFileName);
		mArch->explodeAnimArchetype =0;
	}

	fdesc.lpFileName = mArch->pData->haloExplosionAnimation;
	if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
	{
		mArch->explodeHaloAnimArchetype = ANIM2D->create_archetype(objFile);
	}
	else 
	{
		CQFILENOTFOUND(fdesc.lpFileName);
		mArch->explodeHaloAnimArchetype =0;
	}
*/
	return mArch;
}
//---------------------------------------------------------------------------
//
Minefield * Minefield::CreateInstance (HANDLE hArchetype)
{
	Minefield * obj = new ObjectImpl<Minefield>;
	return obj;
}





//------------------------------------------------------------------------------//
//------------------------------End Minefield.cpp-------------------------------//
//------------------------------------------------------------------------------//
