//--------------------------------------------------------------------------//
//                                                                          //
//                               PlayerBomb.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/PlayerBomb.cpp 48    7/17/00 7:49p Tmauer $
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
#include "SuperTrans.h"
#include "UserDefaults.h"
#include "Mission.h"
#include "IMissionActor.h"
#include "MGlobals.h"
#include "Startup.h"
#include "DrawAgent.h"
#include "TerrainMap.h"
#include "Anim2d.h"
#include "GenData.h"
#include <DPlayerBomb.h>
#include "MPart.h"
#include "OpAgent.h"
#include "DSpaceship.h"
#include "ifabricator.h"
#include "IPlanet.h"
#include "DPlatform.h"
#include "Sector.h"
#include "CQGAME.h"
#include "CommPacket.h"
#include "MScript.h"
#include "DQuickSave.h"
#include "IBanker.h"

#include <WindowManager.h>
#include <Engine.h>
#include <TSmartPointer.h>
#include <Physics.h>
#include <3DMath.h>
#include <FileSys.h>
#include <IConnection.h>

using namespace CQGAMETYPES;

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
struct PLAYERBOMB_INIT
{
	BT_PLAYERBOMB_DATA * pData;
	PARCHETYPE pArchetype;
	AnimArchetype *animArch;

	S32 archIndex;	// not used, should always be -1
	IMeshArchetype * meshArch;
};

struct _NO_VTABLE PlayerBomb : ObjectMission<
									ObjectTransform<
										ObjectFrame<IBaseObject,PLAYERBOMB_SAVELOAD,PLAYERBOMB_INIT>
												   >
										>, 
						   ISaveLoad, IQuickSaveLoad, IPlayerBomb, BASE_PLAYERBOMB_SAVELOAD
{
	BEGIN_MAP_INBOUND(PlayerBomb)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IPlayerBomb)
	END_MAP()

	BT_PLAYERBOMB_DATA * pData;

	AnimInstance anim;
	S32 animWidth;
	COMPTR<IFontDrawAgent> pFont;
	bool bNoExplode;

	PlayerBomb (void);

	virtual ~PlayerBomb (void);	// See ObjList.cpp

	virtual SINGLE TestHighlight (const RECT &rect);

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual void MapRender (bool bPing);

	virtual void DrawHighlighted (void);

	virtual void DrawSelected (void);

	virtual void SetPosition (const Vector &position, U32 newSystemID)
	{
		systemID = newSystemID;
		CQASSERT(systemID);
		ObjectTransform<ObjectFrame<IBaseObject,PLAYERBOMB_SAVELOAD,PLAYERBOMB_INIT> >::SetPosition(position, systemID);
	}

	virtual void SetTransform (const TRANSFORM & transform, U32 newSystemID)
	{
		systemID = newSystemID;
		CQASSERT(systemID);
		ObjectTransform<ObjectFrame<IBaseObject,PLAYERBOMB_SAVELOAD,PLAYERBOMB_INIT> >::SetTransform(transform, newSystemID);
	}

	/* ISaveLoad */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	/* IPlayerBomb */

	virtual void ExplodePlayerBomb (void);


	/* IMissionActor methods */

	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize);

	/* IQuickSaveLoad methods */

	virtual void QuickSave (struct IFileSystem * file);

	virtual void QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize);

	virtual void QuickResolveAssociations (void);

	/* planetBomb */

	void addObject (U32 archID, U32 partID);

	bool isAdmiral (PARCHETYPE pArchetype);

	void editorInitObject (IBaseObject * obj);

	void doBomb (bool bNetworked);
};

//---------------------------------------------------------------------------
//
PlayerBomb::PlayerBomb (void) 
{
	bNoExplode = false;
}
//---------------------------------------------------------------------------
//
PlayerBomb::~PlayerBomb (void)
{
	OBJLIST->RemovePartID(this, dwMissionID);
}
//---------------------------------------------------------------------------
//
void PlayerBomb::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	IBaseObject::TestVisible(defaults,currentSystem,currentPlayer);
	bVisible = (bVisible && defaults.bEditorMode!=0);
}
//---------------------------------------------------------------------------
//
SINGLE PlayerBomb::TestHighlight (const RECT &rect)
{
	bHighlight = 0;
	if (bVisible)
	{
		const BOOL32 bEditorMode = DEFAULTS->GetDefaults()->bEditorMode;
		
		if (bEditorMode)
		{
			S32 screenX,screenY;
			CAMERA->PointToScreen(transform.translation,&screenX,&screenY);
			
			
			// only highlight when mouse is over us
			if (rect.left == rect.right && rect.top == rect.bottom || bEditorMode)
			{
				if (screenX-15 < rect.right && screenX+15 > rect.left && screenY-15 < rect.bottom && screenY+15 > rect.top)
				{
					bHighlight = TRUE;
				}
			}
		}
	}

	return 0.0f;
}
//---------------------------------------------------------------------------
//
void PlayerBomb::DrawHighlighted (void)
{
	Vector point;

	point.x = 0;
	point.y = -animWidth;
	point.z = 0;

	S32 x,y;
	CAMERA->PointToScreen(point, &x, &y, &transform);

	PANE * pane = CAMERA->GetPane();
	SINGLE supplies = this->supplies;
	
	if (supplies > 0)
	{
#if 0
		char buffer[10];
#else
		int TBARLENGTH = 80;
		S32 suppliesMax = supplyPointsMax;
		supplies /= suppliesMax;
		if (suppliesMax < 1500)
		{
			if (suppliesMax < 500)
				TBARLENGTH = 30;
			else
			{
				TBARLENGTH = 30 + (((suppliesMax - 500)*50) / 1000);
			}
		}
		TBARLENGTH = IDEAL2REALX(TBARLENGTH);
#endif
		//
		// draw the blue (supplies) bar RGB(50,50,200)
		//
		
		if (DEFAULTS->GetDefaults()->bEditorMode || playerID == 0)
		{
			if (OBJLIST->GetHighlightedList()==this && nextHighlighted==0)
			{
#if 0
				sprintf(buffer, "%d", S32(supplies));
				DEBUGFONT->StringDraw(pane, x, y, buffer, RGB(180,180,255));
#else
				DA::RectangleHash(pane, x-(TBARLENGTH/2), y+5, x+(TBARLENGTH/2), y+5+2, RGB(128,128,128));
				DA::RectangleFill(pane, x-(TBARLENGTH/2), y+5, x-(TBARLENGTH/2)+S32(TBARLENGTH*supplies), y+5+2, RGB(0,128,255));
#endif
			}
		}
	}

/*	if (nextHighlighted==0 && OBJLIST->GetHighlightedList()==this)
	{
		pFont->SetFontColor(RGB(180,180,180) | 0xFF000000, 0);
		wchar_t temp[M_MAX_STRING];
		_localAnsiToWide(partName, temp, sizeof(temp));

		pFont->StringDraw(pane, x-20, y+10, temp);		// fix this!!! GetDisplayName()
	}*/
}

void PlayerBomb::DrawSelected (void)
{
	if (bVisible==0)
		return;
	
	TRANSFORM trans;
	trans.rotate_about_i(PI/2);
	trans.translation = transform.translation;
	
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
//---------------------------------------------------------------------------
//
void PlayerBomb::doBomb (bool bNetworked)
{
	if(MGlobals::GetThisPlayer() == playerID)
	{
		SECTOR->SetCurrentSystem(systemID);
		CAMERA->SetLookAtPosition(GetPosition());
	}

	if( bNoExplode )
	{
		return;
	}

	U32 workingID;
	if (MGlobals::IsPlayerInGame(playerID))
	{
		CQGAME game = MGlobals::GetGameSettings();
		U32 race = MGlobals::GetPlayerRace(playerID)-1;

		if(game.units == UNITS_MINIMAL)
		{
			for(int i = 0; i < 4 ; ++i)
			{
				if(pData->race[race].minBombType[i].archetypeName[0])
				{
					U32 archID = ARCHLIST->GetArchetypeDataID(ARCHLIST->LoadArchetype(pData->race[race].minBombType[i].archetypeName));
					if(archID)
					{
						U32 partID = MGlobals::CreateNewPartID(playerID);
						
						if (bNetworked)
						{
							U32 buffer[2];
							buffer[0] = archID;
							buffer[1] = partID;
							workingID = THEMATRIX->CreateOperation(dwMissionID,buffer,sizeof(buffer));
							THEMATRIX->OperationCompleted(workingID,dwMissionID);
						}

						addObject(archID,partID);
					}
				}
			}
		}
		else if(game.units == UNITS_MEDIUM)
		{
			for(int i = 0; i < 4 ; ++i)
			{
				if(pData->race[race].bombType[i].archetypeName[0])
				{
					U32 archID = ARCHLIST->GetArchetypeDataID(ARCHLIST->LoadArchetype(pData->race[race].bombType[i].archetypeName));
					if(archID)
					{
						U32 partID = MGlobals::CreateNewPartID(playerID);
						
						if (bNetworked)
						{
							U32 buffer[2];
							buffer[0] = archID;
							buffer[1] = partID;
							workingID = THEMATRIX->CreateOperation(dwMissionID,buffer,sizeof(buffer));
							THEMATRIX->OperationCompleted(workingID,dwMissionID);
						}

						addObject(archID,partID);
					}
				}
			}
		}
		else if(game.units == UNITS_LARGE)
		{
			for(int i = 0; i < 8 ; ++i)
			{
				if(pData->race[race].largeBombType[i].archetypeName[0])
				{
					U32 archID = ARCHLIST->GetArchetypeDataID(ARCHLIST->LoadArchetype(pData->race[race].largeBombType[i].archetypeName));
					if(archID)
					{
						U32 partID = MGlobals::CreateNewPartID(playerID);
						
						if (bNetworked)
						{
							U32 buffer[2];
							buffer[0] = archID;
							buffer[1] = partID;
							workingID = THEMATRIX->CreateOperation(dwMissionID,buffer,sizeof(buffer));
							THEMATRIX->OperationCompleted(workingID,dwMissionID);
						}

						addObject(archID,partID);
					}
				}
			}
		}
	}

	bDeployedPlayer = true;
	if (bNetworked)
	{
		workingID = THEMATRIX->CreateOperation(dwMissionID,0,0);//the deleteion yourself op;
		THEMATRIX->OperationCompleted(workingID,dwMissionID);
	}
	OBJLIST->DeferredDestruction(dwMissionID);

}
//---------------------------------------------------------------------------
//
BOOL32 PlayerBomb::Update (void)
{
	if(THEMATRIX->IsMaster() && (!bDeployedPlayer) && (MGlobals::GetUpdateCount() <= 8))
	{
		doBomb(true);
	}
	return 1;
}
//-------------------------------------------------------------------
//
void PlayerBomb::ExplodePlayerBomb (void)
{
	doBomb(false);
}
//-------------------------------------------------------------------
//
void PlayerBomb::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
{
	bDeployedPlayer = true;
	if(bufferSize)
	{
		U32 * buf = (U32 *)buffer;
		addObject(buf[0],buf[1]);	
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
	}
	else
	{
		if(MGlobals::GetThisPlayer() == playerID)
		{
			SECTOR->SetCurrentSystem(systemID);
			CAMERA->SetLookAtPosition(GetPosition());
		}
		THEMATRIX->OperationCompleted(agentID,dwMissionID);
		OBJLIST->DeferredDestruction(dwMissionID);
	}
}
//-------------------------------------------------------------------
//
bool PlayerBomb::isAdmiral (PARCHETYPE pArchetype)
{
	BASE_SPACESHIP_DATA	* data  = (BASE_SPACESHIP_DATA	*) ARCHLIST->GetArchetypeData(pArchetype);

	return (data->objClass == OC_SPACESHIP && data->type == SSC_FLAGSHIP);
}
//-------------------------------------------------------------------
//
void PlayerBomb::editorInitObject (IBaseObject * obj)
{
	//
	// set initial supplies, hull points
	//
	MPartNC part = obj;


	if (part.isValid())
	{
//		if(THEMATRIX->IsMaster())
		CQASSERT(!(CQFLAGS.bGameActive));
		BANKER->UseCommandPt(playerID,part.pInit->resourceCost.commandPt);
		part->bUnderCommand = true;
		part->hullPoints = part->hullPointsMax;
		if ((part->mObjClass != M_HARVEST) && (part->mObjClass != M_GALIOT) &&(part->mObjClass != M_SIPHON))
			part->supplies   = part->supplyPointsMax;
		obj->SetReady(true);
	}

	obj->SetVisibleToAllies(MGlobals::GetAllyMask(playerID));
	obj->UpdateVisibilityFlags();
}

//-------------------------------------------------------------------
//
void PlayerBomb::addObject (U32 archID, U32 partID)
{
	PARCHETYPE objToPlace = ARCHLIST->LoadArchetype(archID);

	if (isAdmiral(objToPlace))
		partID |= ADMIRAL_MASK;
	IBaseObject * rtObject = MGlobals::CreateInstance(objToPlace, partID);

	if (rtObject)
	{
		Vector position;
		VOLPTR(IPhysicalObject) physObj;
	
		position = GetPosition();

		editorInitObject(rtObject);

		OBJLIST->AddObject(rtObject);

		physObj = rtObject;

		VOLPTR(IPlatform) platform;
		
		platform = rtObject;
		if (platform)		// yes! we are a platform
		{
			//
			// find nearest open slot
			//
			IBaseObject * obj = OBJLIST->GetTargetList();     // nav hazard list
			S32 bestSlot=-1;
			SINGLE bestDistance=10e8;
			OBJPTR<IPlanet> planet, bestPlanet;
			U32 dwPlatformID = platform.Ptr()->GetPartID();

			while (obj)
			{
				if (obj->GetSystemID() == systemID && obj->QueryInterface(IPlanetID, planet)!=0)
				{
					//
					// object is a planet, find the closest open slot
					//
					U32 i, maxSlots;

					maxSlots = planet->GetMaxSlots();
					U32 numSlots = ((BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(objToPlace)))->slotsNeeded;
					U32 slotMask = 0x00000001;
					while(!(slotMask & (0x00000001 << (numSlots-1))))
					{
						slotMask = (slotMask << 1) | 0x00000001;
					}
					for (i=0; i < maxSlots; i++)
					{
						U16 slot = planet->AllocateBuildSlot(dwPlatformID, slotMask,true);
						if (slot!=0)
						{
							SINGLE distance;
							distance = (position-planet->GetSlotTransform(slotMask).translation).magnitude();
							planet->DeallocateBuildSlot(dwPlatformID, slotMask);
							if (distance < bestDistance)
							{
								bestDistance = distance;
								bestSlot = slotMask;
								bestPlanet = planet;
							}
						}
						slotMask = slotMask << 1;
						if(slotMask & (0x00000001 << maxSlots))
						{
							slotMask = (slotMask & (~(0x00000001 << maxSlots))) | 0x00000001; 
						}
					}
				} // end obj->GetSystemID() ...
				
				obj = obj->nextTarget;
			} // end while()

			if (bestPlanet && bestDistance < 10000)
			{
				CQASSERT(bestSlot);
				bestSlot = bestPlanet->AllocateBuildSlot(dwPlatformID, bestSlot);
				TRANSFORM trans = bestPlanet->GetSlotTransform(bestSlot);
				position = trans.translation;

				platform->ParkYourself(trans, bestPlanet.Ptr()->GetPartID(), bestSlot);
			}
			
			if(physObj)
			{
				physObj->SetPosition(position, systemID);
				ENGINE->update_instance(physObj.Ptr()->GetObjectIndex(),0,0);
			}
		}
		else if (physObj)
		{
			physObj->SetPosition(position, systemID);
			ENGINE->update_instance(physObj.Ptr()->GetObjectIndex(),0,0);

			if(THEMATRIX->IsMaster())
			{
				USR_PACKET<USRMOVE> packet;

				packet.objectID[0] = rtObject->GetPartID();
				packet.position.init(GetGridPosition(),systemID);
				packet.init(1);
				NETPACKET->Send(HOSTID,0,&packet);
			}
		}

		MScript::RunProgramsWithEvent(CQPROGFLAG_OBJECTCONSTRUCTED, rtObject->GetPartID());
//			if(rtObject->GetPlayerID())
//				SECTOR->RevealSystem(SECTOR->GetCurrentSystem(), rtObject->GetPlayerID());
	}
}
//---------------------------------------------------------------------------
//
void PlayerBomb::PhysicalUpdate (SINGLE dt)
{
	if (bVisible)
		anim.update(dt);
}
//---------------------------------------------------------------------------
//
void PlayerBomb::Render (void)
{
	if (bVisible)
	{
		BATCH->set_state(RPR_BATCH,true);
		//for now ZENABLE and ZWRITEENABLE are not batched
		//can we get away with alpha testing?
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
		BATCH->set_render_state(D3DRS_ALPHATESTENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ALPHAREF,8);
		BATCH->set_render_state(D3DRS_ALPHAFUNC,D3DCMP_GREATER);

		//TRANSFORM trans;
		ANIM2D->render(&anim,&transform);

		BATCH->set_render_state(D3DRS_ALPHATESTENABLE,FALSE);
	}
}
//---------------------------------------------------------------------------
//
void PlayerBomb::MapRender (bool bPing)
{
}
//---------------------------------------------------------------------------
//
BOOL32 PlayerBomb::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "PLAYERBOMB_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	PLAYERBOMB_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));

	memcpy(&save, static_cast<BASE_PLAYERBOMB_SAVELOAD *>(this), sizeof(BASE_PLAYERBOMB_SAVELOAD));

	FRAME_save(save);

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 PlayerBomb::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "PLAYERBOMB_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	PLAYERBOMB_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("PLAYERBOMB_SAVELOAD", buffer, &load);

	memcpy(static_cast<BASE_PLAYERBOMB_SAVELOAD *>(this), &load, sizeof(BASE_PLAYERBOMB_SAVELOAD));

	FRAME_load(load);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void PlayerBomb::ResolveAssociations()
{
}
//--------------------------------------------------------------------------------------
//
void PlayerBomb::QuickSave (struct IFileSystem * file)
{
	DAFILEDESC fdesc = partName;
	HANDLE hFile;

	file->CreateDirectory("MT_PLAYERBOMB_QLOAD");
	if (file->SetCurrentDirectory("MT_PLAYERBOMB_QLOAD") == 0)
		CQERROR0("QuickSave failed on directory 'MT_PLAYERBOMB_QLOAD'");

	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	if ((hFile = file->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		CQERROR1("QuickSave failed on part '%s'", fdesc.lpFileName);
	}
	else
	{
		MT_PLAYERBOMB_QLOAD qload;
		DWORD dwWritten;
		
		qload.dwMissionID = dwMissionID;
		qload.position.init(GetGridPosition(),systemID);
		qload.bNoExplode = true;

		file->WriteFile(hFile, &qload, sizeof(qload), &dwWritten, 0);
		file->CloseHandle(hFile);
	}
}
//--------------------------------------------------------------------------------------
//
void PlayerBomb::QuickLoad (const char *szSaveLoadType, const char *szInstanceName, void * buffer, U32 bufferSize)
{
	MT_PLAYERBOMB_QLOAD qload;
	MISSION->CorrelateSymbol(szSaveLoadType, buffer, &qload);

	SetPosition(qload.position, qload.position.systemID);
	MGlobals::InitMissionData(this, MGlobals::CreateNewPartID(MGlobals::GetPlayerFromPartID(qload.dwMissionID)));
	partName = szInstanceName;
	SetReady(true);

	OBJLIST->AddPartID(this, dwMissionID);

	bNoExplode = qload.bNoExplode;
}
//--------------------------------------------------------------------------------------
//
void PlayerBomb::QuickResolveAssociations (void)
{
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createPlayerBomb (const PLAYERBOMB_INIT & data)
{
	PlayerBomb * obj = new ObjectImpl<PlayerBomb>;

	obj->pArchetype = data.pArchetype;
	obj->objClass = OC_PLAYERBOMB;
	obj->anim.Init(data.animArch);
	S32 animSize = data.pData->animSize;
	if (animSize == 0)
		animSize = 1000;
	obj->anim.SetWidth(animSize);
	obj->anim.Randomize();
	obj->animWidth = animSize;
	obj->bDeployedPlayer = false;
	obj->pData = data.pData;

	obj->FRAME_init(data);

	return obj;
}
//------------------------------------------------------------------------------------------
//---------------------------PLAYERBOMB Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE PlayerBombFactory : public IObjectFactory
{
	struct OBJTYPE : PLAYERBOMB_INIT
	{
		
		void * operator new (size_t size)
		{
			return calloc(size,1);
		}

		OBJTYPE (void)
		{
			archIndex = -1;		// not used
		}

		~OBJTYPE (void)
		{
			if (animArch)
				delete animArch;
		}
	};

	U32 factoryHandle;		// handles to callback
	U32 eventHandle;
	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(PlayerBombFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	PlayerBombFactory (void) { }

	~PlayerBombFactory (void);

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

	/* PlayerBombFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
PlayerBombFactory::~PlayerBombFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);

}
//--------------------------------------------------------------------------//
//
void PlayerBombFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);

}
//-----------------------------------------------------------------------------
//
HANDLE PlayerBombFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_PLAYERBOMB)
	{
		BT_PLAYERBOMB_DATA * data = (BT_PLAYERBOMB_DATA *) _data;
		result = new OBJTYPE;
		result->pData = data;
		result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		
		{
			COMPTR<IFileSystem> file;
			DAFILEDESC fdesc = data->playerBomb_anim2D;
			
			fdesc.lpImplementation = "UTF";
			
			if (OBJECTDIR->CreateInstance(&fdesc, file) != GR_OK)
			{
				CQFILENOTFOUND(fdesc.lpFileName);
				goto Error;
			}
			
			if ((result->animArch = ANIM2D->create_archetype(file)) == 0)
				goto Error;
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
BOOL32 PlayerBombFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * PlayerBombFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createPlayerBomb(*objtype);
}
//-------------------------------------------------------------------
//
void PlayerBombFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}

//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _playerbomb : GlobalComponent
{
	PlayerBombFactory * playBomb;

	virtual void Startup (void)
	{
		playBomb = new DAComponent<PlayerBombFactory>;
		AddToGlobalCleanupList((IDAComponent **) &playBomb);
	}

	virtual void Initialize (void)
	{
		playBomb->init();
	}
};

static _playerbomb pbomb;
//---------------------------------------------------------------------------
//--------------------------End PlayerBomb.cpp--------------------------------
//---------------------------------------------------------------------------
