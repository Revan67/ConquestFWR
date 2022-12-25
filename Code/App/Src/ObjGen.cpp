//--------------------------------------------------------------------------//
//                                                                          //
//                                ObjGen.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ObjGen.cpp 71    10/18/00 10:32a Jasony $
*/			   
//---------------------------------------------------------------------------
#include "pch.h"
#include <globals.h>

#include <MGlobals.h>
#include "TResClient.h"
#include "IObject.h"
#include "Objlist.h"
#include "Startup.h"
#include "Camera.h"
#include "Resource.h"
#include "SysMap.h"
#include "SuperTrans.h"
#include "Sector.h"
#include "IPlanet.h"
#include "IFabricator.h"		// IPlatform
#include "MPart.h"
#include <DSpaceship.h>
#include <DPlatform.h>
#include <DPlanet.h>
#include "OpAgent.h"
#include "IMovieCamera.h"
#include "GridVector.h"
#include "TerrainMap.h"
#include "INugget.h"
#include "DNugget.h"
#include "IBanker.h"
#include "IJumpgate.h"

#include <Renderer.h>
#include <3DMath.h>
#include <Engine.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <EventSys.h>
#include <TComponent.h>
#include <IConnection.h>
#include <TConnPoint.h>
#include <TConnContainer.h>
#include <WindowManager.h>

#include <stdlib.h>
#include <stdio.h>
//--------------------------------------------------------------------------//
//----------------------------//
//
enum CURSOR_MODE
{
	NOT_OWNED=0,
	INSERT,
	BAN
};

static struct ObjGenerator * gen;

//--------------------------------------------------------------------------//
//------------------------------ObjectList Class----------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE ObjGenerator : public IEventCallback, ResourceClient<>
{
	U32 eventHandle;		// handles to callback
	BOOL32 bHasFocus;
	bool bDropOk;
	CURSOR_MODE cursorMode, desiredMode;
	PARCHETYPE objToPlace;
	MISSION_INFO info;		// saved info to hand off to mission system
	bool bButtonValid;
	bool bObjIsNeutral;

	int mouseX,mouseY;
	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ObjGenerator)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	END_DACOM_MAP()

	ObjGenerator (void);

	~ObjGenerator (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* ObjGenerator methods */

	void setCursorMode (CURSOR_MODE newMode);

	static BOOL32 insideArea (void);

	void getObjectDropRect (struct tagRECT &rect,Vector &pos);

	void renderEdit ();

	void update (void);

	void init (void);

	void editorAddObject (S32 x, S32 y);

	void editorInitObject (IBaseObject * obj);

	static bool isNeutral (BASIC_DATA * data);
	static bool isAdmiral (BASIC_DATA * data);

	IDAComponent * getBase (void)
	{
		return (IEventCallback *) this;
	}
};
//--------------------------------------------------------------------------//
//
ObjGenerator::ObjGenerator (void)
{
	bHasFocus = 1;
	resPriority = RES_PRIORITY_MEDIUM;
}
//--------------------------------------------------------------------------//
//
ObjGenerator::~ObjGenerator (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);
}
//-------------------------------------------------------------------
// receive notifications from event system
//
GENRESULT ObjGenerator::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_KILL_FOCUS:
		if ((IDAComponent *)param != getBase())
		{
			bHasFocus = FALSE;
			desiredOwnedFlags = 0;
			releaseResources();
		}
		break;
	case CQE_SET_FOCUS:
		bHasFocus = TRUE;
		break;

	case CQE_UPDATE:
		update();
		break;

	case CQE_RENDER_LAST3D:
		if (cursorMode == INSERT)
		{
			renderEdit();
		}
		break;

	case WM_LBUTTONDOWN:
		bButtonValid = (cursorMode == INSERT && ownsResources() && bDropOk);
		break;
		
	case WM_LBUTTONUP:
		if (bButtonValid && cursorMode == INSERT && ownsResources())
		{
			bButtonValid = false;
			editorAddObject(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)));
		}
		break;

	case WM_RBUTTONDOWN:
		if (ownsResources())
		{
			desiredMode = NOT_OWNED;
			setCursorMode(NOT_OWNED);
		}
		break;

	case WM_MOUSEMOVE:
		mouseX = short(LOWORD(msg->lParam));
		mouseY = short(HIWORD(msg->lParam));
		break;

	case CQE_NEW_SELECTION:
		if (param != this)
		{
			desiredMode = NOT_OWNED;
			setCursorMode(NOT_OWNED);
		}
		break;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void ObjGenerator::setCursorMode (CURSOR_MODE newMode)
{
	switch (newMode)
	{
	case INSERT:
		desiredOwnedFlags = RF_CURSOR | RF_STATUS;
		if (statusTextID == IDS_INSERT3DOBJECT && cursorID == IDC_CURSOR_PLACEOBJECT)
		{
			if (ownsResources() == 0)
				grabAllResources();

			// for testing purposes
			// get the grid vector associated with the mouse control
			if (objToPlace)
			{
				Vector vec(mouseX, mouseY, 0);
				GRIDVECTOR grid;


				CAMERA->ScreenToPoint(vec.x, vec.y, vec.z);
				grid = vec;
				
				// is the object we're about to drop a fullgrid or not?
				BASIC_DATA * _data = (BASIC_DATA *) ARCHLIST->GetArchetypeData(objToPlace);

				if (_data->objClass == OC_SPACESHIP)
				{
					SPACESHIP_INIT<BASE_SPACESHIP_DATA> *arch;
					arch = static_cast<SPACESHIP_INIT<BASE_SPACESHIP_DATA> *>(ARCHLIST->GetArchetypeHandle(objToPlace));
					
					if (arch->fp_radius)
					{
						if (arch->fp_radius > HALFGRID)
						{
							grid.centerpos();
						}
						else
						{
							grid.quarterpos();
						}
					}
					
					wchar_t szString[64];
					swprintf(szString, L"Press left click to drop object at %.2f, %.2f", grid.getX(), grid.getY());
					STATUS->SetTextString(szString);
				}
			}
		}
		else
		{
			statusTextID = IDS_INSERT3DOBJECT;
			cursorID = IDC_CURSOR_PLACEOBJECT;
			if (ownsResources())
				setResources();
			else
				grabAllResources();
		}
		break;

	case BAN:
		desiredOwnedFlags = RF_CURSOR | RF_STATUS;
		if (statusTextID == IDS_OUTOFBOUNDS && cursorID == IDC_CURSOR_BAN)
		{
			if (ownsResources() == 0)
				grabAllResources();
		}
		else
		{
			statusTextID = IDS_OUTOFBOUNDS;
			cursorID = IDC_CURSOR_BAN;
			if (ownsResources())
				setResources();
			else
				grabAllResources();
		}
		break;

	case NOT_OWNED:
		desiredOwnedFlags = 0;
		releaseResources();
		break;
	};


	cursorMode = newMode;
}
//-------------------------------------------------------------------
//
BOOL32 ObjGenerator::insideArea (void)
{
	S32 x, y;

	WM->GetCursorPos(x, y);

	return (y <= CAMERA->GetPane()->y1);
}
//-------------------------------------------------------------------
//
void ObjGenerator::getObjectDropRect (struct tagRECT &rect,Vector &pos)
{
	int i,j;
	Vector position;

	position.x = mouseX;
	position.y = mouseY;
	position.z = 0;

	RECT rc;
	SECTOR->GetSystemRect(SECTOR->GetCurrentSystem(), &rc);
	int numGrids;
	numGrids = (rc.right - rc.left)/GRIDSIZE;

	BASIC_DATA * _data = (BASIC_DATA *) ARCHLIST->GetArchetypeData(objToPlace);
	if (CAMERA->ScreenToPoint(position.x, position.y, position.z))
	{
		switch (_data->objClass)
		{
		case OC_PLANETOID:
			{
				BT_PLANET_DATA * planetData = (BT_PLANET_DATA *) _data;
				if(planetData->bMoon)
				{
					//snap to the closes grid center
					i = floor(position.x/GRIDSIZE+1.0);
					j = floor(position.y/GRIDSIZE+1.0);
					
					if (i > numGrids)
						i = numGrids;
					if (j > numGrids)
						j = numGrids;
					if (i < 1)
						i = 1;
					if (j < 1)
						j = 1;
					
					pos.x = i*GRIDSIZE-HALFGRID;
					pos.y = j*GRIDSIZE-HALFGRID;
					
					rect.left = pos.x-HALFGRID;
					rect.top = pos.y-HALFGRID;
					rect.right = pos.x+HALFGRID;
					rect.bottom = pos.y+HALFGRID;
				}
				else
				{
					// correct the planet's position to snap to the closet grid apex
					i = floor(position.x/GRIDSIZE+0.5);
					j = floor(position.y/GRIDSIZE+0.5);
					
					if (i > numGrids-2)
						i = numGrids-2;
					if (j > numGrids-2)
						j = numGrids-2;
					if (i < 1)
						i = 1;
					if (j < 1)
						j = 1;
					
					pos.x = i*GRIDSIZE;
					pos.y = j*GRIDSIZE;
					
					rect.left = pos.x-GRIDSIZE;
					rect.top = pos.y-GRIDSIZE;
					rect.right = pos.x+GRIDSIZE;
					rect.bottom = pos.y+GRIDSIZE;
				}
			}
			break;

		case OC_PLATFORM:
			{
				PLATFORM_INIT<BASE_PLATFORM_DATA> *arch;
				arch = static_cast<PLATFORM_INIT<BASE_PLATFORM_DATA> *>(ARCHLIST->GetArchetypeHandle(objToPlace));
				if(arch->pData->slotsNeeded == 0)
				{
					if (arch->fp_radius)
					{
						if (arch->fp_radius > HALFGRID)
						{
							//snap a big ship to the center of a square
							i = floor(position.x/GRIDSIZE+1.0);
							j = floor(position.y/GRIDSIZE+1.0);
							
							if (i > numGrids)
								i = numGrids;
							if (j > numGrids)
								j = numGrids;
							if (i < 1)
								i = 1;
							if (j < 1)
								j = 1;
							
							pos.x = i*GRIDSIZE-HALFGRID;
							pos.y = j*GRIDSIZE-HALFGRID;
							
							rect.left = pos.x-HALFGRID;
							rect.top = pos.y-HALFGRID;
							rect.right = pos.x+HALFGRID;
							rect.bottom = pos.y+HALFGRID;
						}
						else
						{
							//snap a little ship to the center of a square
							i = floor(position.x/HALFGRID+1.0);
							j = floor(position.y/HALFGRID+1.0);
							
							numGrids = (rc.right - rc.left)/HALFGRID;

							if (i > numGrids)
								i = numGrids;
							if (j > numGrids)
								j = numGrids;
							if (i < 1)
								i = 1;
							if (j < 1)
								j = 1;
							
							pos.x = i*HALFGRID-(HALFGRID>>1);
							pos.y = j*HALFGRID-(HALFGRID>>1);
							
							rect.left = pos.x-(HALFGRID>>1);
							rect.top = pos.y-(HALFGRID>>1);
							rect.right = pos.x+(HALFGRID>>1);
							rect.bottom = pos.y+(HALFGRID>>1);
						}
					}
				}
				else
				{
					SetRect(&rect, 0, 0, 0, 0);
				}
			}
			break;
		case OC_SPACESHIP:
			SPACESHIP_INIT<BASE_SPACESHIP_DATA> *arch;
			arch = static_cast<SPACESHIP_INIT<BASE_SPACESHIP_DATA> *>(ARCHLIST->GetArchetypeHandle(objToPlace));
			if (arch->fp_radius)
			{
				if (arch->fp_radius > HALFGRID)
				{
					//snap a big ship to the center of a square
					i = floor(position.x/GRIDSIZE+1.0);
					j = floor(position.y/GRIDSIZE+1.0);
					
					if (i > numGrids)
						i = numGrids;
					if (j > numGrids)
						j = numGrids;
					if (i < 1)
						i = 1;
					if (j < 1)
						j = 1;
					
					pos.x = i*GRIDSIZE-HALFGRID;
					pos.y = j*GRIDSIZE-HALFGRID;
					
					rect.left = pos.x-HALFGRID;
					rect.top = pos.y-HALFGRID;
					rect.right = pos.x+HALFGRID;
					rect.bottom = pos.y+HALFGRID;
				}
				else
				{
					//snap a little ship to the center of a square
					i = floor(position.x/HALFGRID+1.0);
					j = floor(position.y/HALFGRID+1.0);
					
					numGrids = (rc.right - rc.left)/HALFGRID;

					if (i > numGrids)
						i = numGrids;
					if (j > numGrids)
						j = numGrids;
					if (i < 1)
						i = 1;
					if (j < 1)
						j = 1;
					
					pos.x = i*HALFGRID-(HALFGRID>>1);
					pos.y = j*HALFGRID-(HALFGRID>>1);
					
					rect.left = pos.x-(HALFGRID>>1);
					rect.top = pos.y-(HALFGRID>>1);
					rect.right = pos.x+(HALFGRID>>1);
					rect.bottom = pos.y+(HALFGRID>>1);
				}
			}
			break;

		default:
			SetRect(&rect, 0, 0, 0, 0);
			break;
		}
	}
}
//-------------------------------------------------------------------
//
void ObjGenerator::renderEdit ()
{
	RECT rect;
	Vector pos;

	getObjectDropRect(rect,pos);

	Vector v[4];

	v[3].x = v[0].x = rect.left;
	v[0].y = v[1].y = rect.top;
	v[1].x = v[2].x = rect.right;
	v[2].y = v[3].y = rect.bottom;
	
	BATCH->set_state(RPR_BATCH,false);
	
	CAMERA->SetPerspective();
	CAMERA->SetModelView();
	DisableTextures();
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);


	if (bDropOk)
	{
		PB.Color3ub(55,55,55);
	}
	else
	{
		PB.Color3ub(180,0,0);
	}
	
	PB.Begin(PB_QUADS);
	PB.Vertex3f(v[0].x,v[0].y,0);
	PB.Vertex3f(v[1].x,v[1].y,0);
	PB.Vertex3f(v[2].x,v[2].y,0);
	PB.Vertex3f(v[3].x,v[3].y,0);
	PB.End();
}
//-------------------------------------------------------------------
//
void ObjGenerator::update (void)
{
	if (desiredMode != NOT_OWNED)
	{
		if (insideArea() && bHasFocus)
		{
			//
			// check to make sure mouse is within the system boundary
			//
			Vector pos;
			RECT rect;
			S32 x, y;

			WM->GetCursorPos(x, y);

			pos.x = x;
			pos.y = y;
			pos.z = 0;

			CAMERA->ScreenToPoint(pos.x, pos.y, 0);
			SECTOR->GetCurrentRect(&rect);
			Vector temp((rect.left-rect.right)/2, (rect.bottom-rect.top)/2, 0);
			SINGLE dist = (temp.x * temp.x);// + (temp.y * temp.y);
			temp += pos;

			// if we are are already over something, than put up the ban
			COMPTR<ITerrainMap> map;
			SECTOR->GetTerrainMap(SECTOR->GetCurrentSystem(), map);
			if (map != NULL)
			{
				GRIDVECTOR grid;
				grid = pos;
				getObjectDropRect(rect, pos);

				if ((rect.right - rect.left)/GRIDSIZE > 1)
				{
					// the 4-grid case, make sure all 4 grids are open before we place the object
					bool bPass = true;
					Vector v[4];
					v[0] = pos;
					v[1] = Vector(pos.x - GRIDSIZE, pos.y, pos.z);
					v[2] = Vector(pos.x - GRIDSIZE, pos.y - GRIDSIZE, pos.z);
					v[3] = Vector(pos.x, pos.y - GRIDSIZE, pos.z);

					for (int i = 0; i < 4; i++)
					{
						grid = v[i];
						if (map->IsGridEmpty(grid, 0, true) == false)
						{
							bPass = false;
							break;
						}
					}
					bDropOk = bPass;
				}
				else if (rect.right - rect.left)
				{
					// the one quarter or one full square case
					bool bFullSquare = (rect.right - rect.left) > HALFGRID;
					
					bDropOk = map->IsGridEmpty(grid, 0, bFullSquare);
				}
				else
				{
					bDropOk = true;
				}
			}
			
			if (dist < ((temp.x * temp.x) + (temp.y * temp.y)))
			{
				cursorMode = BAN;
			}
			else // ok
			{
				cursorMode = desiredMode;
			}
		}
		else
			cursorMode = NOT_OWNED;

		setCursorMode(cursorMode);
	}
}
//-------------------------------------------------------------------
//
bool ObjGenerator::isNeutral (BASIC_DATA * _data)
{
	if (_data->objClass == OC_SPACESHIP)
	{
		BASE_SPACESHIP_DATA	* data  = (BASE_SPACESHIP_DATA	*) _data;
		return (data->missionData.race == M_NO_RACE);
	}
	else
	if (_data->objClass == OC_PLATFORM)
	{
		BASE_PLATFORM_DATA	* data  = (BASE_PLATFORM_DATA	*) _data;
		return (data->missionData.race == M_NO_RACE);
	}
	return ((_data->objClass & CF_PLAYERALIGNED) == 0);
}
//-------------------------------------------------------------------
//
bool ObjGenerator::isAdmiral (BASIC_DATA * _data)
{
	BASE_SPACESHIP_DATA	* data  = (BASE_SPACESHIP_DATA	*) _data;
	return (data->objClass == OC_SPACESHIP && data->type == SSC_FLAGSHIP);
}
//-------------------------------------------------------------------
//
void ObjGenerator::editorAddObject (S32 x, S32 y)
{
	U32 systemID = SECTOR->GetCurrentSystem();
	SYSMAP->InvalidateMap(systemID);
	BASIC_DATA * _data = (BASIC_DATA *) ARCHLIST->GetArchetypeData(objToPlace);

	if(_data->objClass == OC_NUGGET)
	{
		Vector position;
		position.x = x;
		position.y = y;
		position.z = 0;

		if (CAMERA->ScreenToPoint(position.x, position.y, position.z))
		{
			BT_NUGGET_DATA * nData = (BT_NUGGET_DATA *)(_data);
			NUGGETMANAGER->CreateNugget(objToPlace,SECTOR->GetCurrentSystem(),position,nData->maxSupplies,0,MGlobals::CreateNewPartID(0),false);
		}
		return;
	}

	U32 partID = MGlobals::CreateNewPartID(isNeutral(_data)?0:MGlobals::GetThisPlayer());
	if (isAdmiral(_data))
		partID |= ADMIRAL_MASK;
	IBaseObject * rtObject = MGlobals::CreateInstance(objToPlace, partID);

	if (rtObject)
	{
		Vector position;
		VOLPTR(IPhysicalObject) physObj;
		VOLPTR(IMovieCamera) cameraObj;
	
		position.x = x;
		position.y = y;
		position.z = 0;

		editorInitObject(rtObject);

		OBJLIST->AddObject(rtObject);

		physObj = rtObject;
		cameraObj = rtObject;

		if (CAMERA->ScreenToPoint(position.x, position.y, position.z))
		{
			VOLPTR(IPlatform) platform;
			
			platform = rtObject;
			if (platform && (!platform->IsDeepSpacePlatform()))		// yes! we are a platform
			{
				if(platform->IsJumpPlatform())
				{
					IBaseObject * bestJumpgate = NULL;
					IBaseObject * obj = OBJLIST->GetTargetList();     // nav hazard list
					SINGLE bestDistance=10e8;
					while(obj)
					{
						if(obj->GetSystemID() == systemID && obj->objClass == OC_JUMPGATE)
						{
							OBJPTR<IJumpGate> jumpgate;
							obj->QueryInterface(IJumpGateID,jumpgate);
							CQASSERT(jumpgate);
							if(!jumpgate->GetPlayerOwner())
							{
								SINGLE newDist = (position-obj->GetPosition()).magnitude_squared();
								if(newDist < bestDistance)
								{
									bestJumpgate = obj;
									bestDistance = newDist;
								}
							}
						}
						obj = obj->nextTarget;
					}
					if(bestJumpgate)
					{
						platform->ParkYourself(bestJumpgate);
					}
					else
					{
						CQASSERT(0 && "Could Not Place JumpPlat");
					}
				}
				else
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
							U32 newSlot = planet->FindBestSlot(objToPlace,&position);
							if(newSlot)
							{
								SINGLE newDist = (position-planet->GetSlotTransform(newSlot).translation).magnitude();
								if(newDist < bestDistance)
								{
									bestDistance = newDist;
									bestSlot = newSlot;
									bestPlanet = planet;
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
						physObj->SetPosition(position, SECTOR->GetCurrentSystem());
						ENGINE->update_instance(physObj.Ptr()->GetObjectIndex(),0,0);
					}
				}
			}
			else if(platform && platform->IsJumpPlatform())
			{
				IBaseObject * jump = NULL;
				SINGLE dist = 0;
				IBaseObject * obj = OBJLIST->GetObjectList();
				while(obj)
				{
					if(obj->objClass == OC_JUMPGATE && obj->GetSystemID() == SECTOR->GetCurrentSystem())
					{
						if(jump)
						{
							SINGLE newDist =  (position-obj->GetPosition()).magnitude_squared();
							if(newDist < dist)
							{
								jump = obj;
								dist = newDist;
							}
						}
						else
						{
							jump = obj;
							dist =  (position-obj->GetPosition()).magnitude_squared();
						}
					}
					obj = obj->next;
				}
				if(jump)
				{
					platform->ParkYourself(jump);
					ENGINE->update_instance(platform.Ptr()->GetObjectIndex(),0,0);
				}
			}
			else if (cameraObj)
			{
				cameraObj->InitCamera();
				physObj->SetSystemID(SECTOR->GetCurrentSystem());
				ENGINE->update_instance(physObj.Ptr()->GetObjectIndex(),0,0);
			}
			else if (physObj)
			{
				RECT rect;
				getObjectDropRect(rect,position);

				physObj->SetPosition(position, SECTOR->GetCurrentSystem());
				ENGINE->update_instance(physObj.Ptr()->GetObjectIndex(),0,0);
			}

			if(rtObject->GetPlayerID())
				SECTOR->RevealSystem(SECTOR->GetCurrentSystem(), rtObject->GetPlayerID());
		}

		// the object has to initialize its footprint
		COMPTR<ITerrainMap> map;
		SECTOR->GetTerrainMap(SECTOR->GetCurrentSystem(), map);
		if (map)
		{
			rtObject->SetTerrainFootprint(map);
		}
	}
}
//-------------------------------------------------------------------
//
void ObjGenerator::editorInitObject (IBaseObject * obj)
{
	//
	// set initial supplies, hull points
	//
	MPartNC part = obj;

	if (part.isValid())
	{
		if(MGlobals::GetPlayerFromPartID(obj->GetPartID()) && THEMATRIX->IsMaster())
			BANKER->UseCommandPt(MGlobals::GetPlayerFromPartID(obj->GetPartID()),part.pInit->resourceCost.commandPt);
		part->bUnderCommand = true;
		part->hullPoints = part->hullPointsMax;
		if ((part->mObjClass != M_HARVEST) && (part->mObjClass != M_GALIOT) &&(part->mObjClass != M_SIPHON))
			part->supplies   = part->supplyPointsMax;
		obj->SetReady(true);

		if (part->playerID != 0)
		{
			obj->SetVisibleToPlayer(part->playerID);
			obj->UpdateVisibilityFlags();
		}

		part->systemID = SECTOR->GetCurrentSystem();
	}
}
//-------------------------------------------------------------------
//
void ObjGenerator::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Advise(getBase(), &eventHandle);

	initializeResources();
}
//-------------------------------------------------------------------
//
void __stdcall EditorStartObjectInsertion (PARCHETYPE pArchetype, const struct MISSION_INFO & _info)
{
	ObjGenerator * _this;
	
	if ((_this = gen) != 0)
	{
		_this->objToPlace = pArchetype;
		_this->desiredMode = INSERT;
		_this->setCursorMode(INSERT);
		_this->info = _info;
	}
}
//-------------------------------------------------------------------
//
void __stdcall EditorStopObjectInsertion (PARCHETYPE pArchetype)
{
	ObjGenerator * _this;
	
	if ((_this = gen) != 0)
	{
		if (pArchetype == _this->objToPlace)
		{
			_this->objToPlace = 0;
			if (_this->desiredMode == INSERT)
			{
				_this->desiredMode = NOT_OWNED;
				_this->setCursorMode(NOT_OWNED);
			}
		}
	}
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
struct _gen : GlobalComponent
{
	virtual void Startup (void)
	{
		gen = new DAComponent<ObjGenerator>;
		AddToGlobalCleanupList((IDAComponent **) &gen);
	}

	virtual void Initialize (void)
	{
		gen->init();
	}
};

static _gen generator;

//-------------------------------------------------------------------
//-------------------------END ObjGen.cpp---------------------------
//-------------------------------------------------------------------
