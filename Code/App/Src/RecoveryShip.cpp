//--------------------------------------------------------------------------//
//                                                                          //
//                             RecoveryShip.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/RecoveryShip.cpp 13    3/18/00 12:30a Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Sector.h"
#include "TSpaceShip.h"
#include <DSpaceShip.h>
#include "TObject.h"
#include "Objlist.h"
#include "Iplanet.h"
#include "Startup.h"
#include "Mission.h"
#include "SysMap.h"
#include "CommPacket.h"
#include "MPart.h"
#include "ObjSet.h"
#include "IRecoverShip.h"
#include "ILight.h"
#include "CQLight.h"
#include "DRecoveryShipSave.h"
#include "DRecoveryShip.h"

#include <IConnection.h>
#include <TComponent.h>
#include <TSmartPointer.h>
#include <FileSys.h>


//this is so I can add HasOldPackets easily.
#define HOST_CHECK THEMATRIX->IsMaster()

#define COMP_OP(operation) { U32 tempID = workingID; workingID = 0; THEMATRIX->OperationCompleted(tempID,operation);}

struct _NO_VTABLE RecoveryShip : public SpaceShip<RECOVERYSHIP_SAVELOAD, RECOVERYSHIP_INIT>,  
								BASE_RECOVERYSHIP_SAVELOAD, IRecoverShip
{
	BEGIN_MAP_INBOUND(RecoveryShip)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IGotoPos)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(IQuickSaveLoad)
	_INTERFACE_ENTRY(IExplosionOwner)
	_INTERFACE_ENTRY(IMissionActor)
	_INTERFACE_ENTRY(IWeaponTarget)
	_INTERFACE_ENTRY(IBuild)
	_INTERFACE_ENTRY(IPhysicalObject)
	_INTERFACE_ENTRY(IRepairee)
	_INTERFACE_ENTRY(IShipDamage)
	_INTERFACE_ENTRY(IExtent)
	_INTERFACE_ENTRY(IRecoverShip)
	END_MAP()

	const BT_RECOVERYSHIP_DATA * pData;

	UpdateNode updateNode;
	SaveNode	saveNode;
	LoadNode	loadNode;
	InitNode		initNode;
	PhysUpdateNode  physUpdateNode;
	RenderNode	renderNode;
	OnOpCancelNode	onOpCancelNode;
	ReceiveOpDataNode	receiveOpDataNode;
	PreDestructNode	destructNode;

	OBJPTR<IBaseObject> recoveryTarget;
	HardpointInfo  beamPointInfo1;
	HardpointInfo  beamPointInfo2;
	U32 mineTex;
		
	RecoveryShip (void);

	virtual ~RecoveryShip (void);

	/* IGotoPos methods */


	/* IRecoverShip */

	virtual void RecoverWreck(IBaseObject * object, U32 agentID);

	/* SpaceShip methods */
	
	virtual void ResolveAssociations (void);

	virtual const char * getSaveStructName (void) const
	{
		return "RECOVERYSHIP_SAVELOAD";
	}

	virtual void * getViewStruct (void)	
	{
		return static_cast<BASE_RECOVERYSHIP_SAVELOAD *>(this);
	}
	/* IMissionActor */

	// the following are process related
	// called in response to OpAgent::CreateOperation()
	virtual void OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize);

	// called in response to OpAgent::SendOperationData()
	void receiveOperationData (U32 agentID, void *buffer, U32 bufferSize);

	// user has requested a different action
	void onOperationCancel (U32 agentID);
		
	void preSelfDestruct (void);

	virtual void OnStopRequest (U32 agentID);

	virtual void OnMasterChange (bool bIsMaster);

	/* recovery ship methods */

	BOOL32 updateRecovery (void);

	void physUpdateRecovery  (SINGLE dt);

	void renderRecovery ();

	void initRecovery (const RECOVERYSHIP_INIT & data);

	void save (RECOVERYSHIP_SAVELOAD & save);
	void load (RECOVERYSHIP_SAVELOAD & load);
};
//---------------------------------------------------------------------------
//
RecoveryShip::RecoveryShip (void) : 
	updateNode(this, UpdateProc(updateRecovery)),
    physUpdateNode(this, PhysUpdateProc(physUpdateRecovery)),
	saveNode(this, CASTSAVELOADPROC(save)),
	loadNode(this, CASTSAVELOADPROC(load)),
	initNode(this, CASTINITPROC(initRecovery)),
	renderNode(this,RenderProc(renderRecovery)),
	onOpCancelNode(this, OnOpCancelProc(onOperationCancel)),
	receiveOpDataNode(this, ReceiveOpDataProc(receiveOperationData)),
	destructNode(this, PreDestructProc(preSelfDestruct))
{
}
//---------------------------------------------------------------------------
//
RecoveryShip::~RecoveryShip (void)
{
}
//---------------------------------------------------------------------------
//
void RecoveryShip::initRecovery (const RECOVERYSHIP_INIT & data)
{
	pData = data.pData;
	mineTex = data.mineTex;

	INSTANCE_INDEX dummy = -1;
	if(data.pData->beamPointName1[0])
		FindHardpoint(data.pData->beamPointName1, dummy, beamPointInfo1, instanceIndex);
	else
		beamPointInfo1.point = Vector(0,0,0);

	dummy = -1;
	if(data.pData->beamPointName2[0])
		FindHardpoint(data.pData->beamPointName2, dummy, beamPointInfo2, instanceIndex);
	else
		beamPointInfo2.point = Vector(0,0,0);
}
//---------------------------------------------------------------------------
//
BOOL32 RecoveryShip::updateRecovery (void)
{
	if(recoveryTargetID)
	{
		switch(mode)
		{
		case MOVING_TO_RECOVERY:
			{
				if(!isMoveActive())
				{
					mode = RECOVERING;
					recoverTimer = pData->recoverTime;
				}
			}
			break;
		case RECOVERING:
			{
				recoverTimer -= ELAPSED_TIME;
				if(recoverTimer <0)
				{
					mode = TOWING;
				}
			}
			break;
		}
	}
	return true;
}
//---------------------------------------------------------------------------
//
void RecoveryShip::physUpdateRecovery (SINGLE dt)
{
	if(mode == TOWING)
	{
		CQASSERT(recoveryTarget);

		SINGLE recoverDist = pData->recoveryRadius*GRIDSIZE;
		recoverDist = recoverDist*recoverDist;
		Vector pos = recoveryTarget->GetPosition();
		Vector dir = pos-GetPosition();
		if(dir.magnitude_squared() > recoverDist)
		{
			OBJPTR<IPhysicalObject> physTarg;
			recoveryTarget->QueryInterface(IPhysicalObjectID,physTarg);
			CQASSERT(physTarg);
			dir.normalize();
			physTarg->SetSystemID(systemID);
			physTarg->SetPosition(GetPosition()+dir*(pData->recoveryRadius*GRIDSIZE));
		}
	}
}
//---------------------------------------------------------------------------
//
void RecoveryShip::renderRecovery ()
{
	if (recoveryTarget && mode == TOWING)
	{
		BATCH->set_state(RPR_BATCH,TRUE);
		SetupDiffuseBlend(mineTex,TRUE);
		BATCH->set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRENDERSTATE_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRENDERSTATE_DESTBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRENDERSTATE_ZENABLE,true);
		BATCH->set_render_state(D3DRENDERSTATE_ZWRITEENABLE, false);
		CAMERA->SetModelView();
		
		Vector dir,dir2;
		Vector cpos = CAMERA->GetPosition();
		Vector look = recoveryTarget->GetPosition()-cpos;
		look.normalize();
		
		
		dir = transform.translation-recoveryTarget->GetPosition();
		dir.normalize();
		dir2 = cross_product(look,dir);
		dir2.normalize();

		Vector v0,v1,v2,v3,v4,v5;
		
		v0 = v1 = v4 = recoveryTarget->GetPosition();
		v0 += (100)*dir2;
		v1 -= (100)*dir2;
		v2 = v3 = v5 = transform*beamPointInfo1.point;
		v2 -= 20*dir2;
		v3 += 20*dir2;

		
		PB.Color3ub(255,255,255);
		PB.Begin(PB_QUADS);
		PB.TexCoord2f(0.1,0);  PB.Vertex3f(v0.x,v0.y,v0.z);
		PB.TexCoord2f(0.5,0);  PB.Vertex3f(v4.x,v4.y,v4.z);
		PB.TexCoord2f(0.4,1);  PB.Vertex3f(v5.x,v5.y,v5.z);
		PB.TexCoord2f(0,1); PB.Vertex3f(v3.x,v3.y,v3.z);

		PB.TexCoord2f(0.9,0);    PB.Vertex3f(v1.x,v1.y,v1.z);
		PB.TexCoord2f(0.5,0);  PB.Vertex3f(v4.x,v4.y,v4.z);
		PB.TexCoord2f(0.6,1);  PB.Vertex3f(v5.x,v5.y,v5.z);
		PB.TexCoord2f(1,1);    PB.Vertex3f(v2.x,v2.y,v2.z);
		PB.End();

		v2 = v3 = v5 = transform*beamPointInfo2.point;
		v2 -= 20*dir2;
		v3 += 20*dir2;

		
		PB.Color3ub(255,255,255);
		PB.Begin(PB_QUADS);
		PB.TexCoord2f(0.1,0);  PB.Vertex3f(v0.x,v0.y,v0.z);
		PB.TexCoord2f(0.5,0);  PB.Vertex3f(v4.x,v4.y,v4.z);
		PB.TexCoord2f(0.4,1);  PB.Vertex3f(v5.x,v5.y,v5.z);
		PB.TexCoord2f(0,1); PB.Vertex3f(v3.x,v3.y,v3.z);

		PB.TexCoord2f(0.9,0);    PB.Vertex3f(v1.x,v1.y,v1.z);
		PB.TexCoord2f(0.5,0);  PB.Vertex3f(v4.x,v4.y,v4.z);
		PB.TexCoord2f(0.6,1);  PB.Vertex3f(v5.x,v5.y,v5.z);
		PB.TexCoord2f(1,1);    PB.Vertex3f(v2.x,v2.y,v2.z);
		PB.End();
		
	}
}
//---------------------------------------------------------------------------
//
void RecoveryShip::RecoverWreck(IBaseObject * object, U32 agentID)
{
	if(object)
	{
		recoveryTarget = object;
		recoveryTargetID = object->GetPartID();
		Vector pos = object->GetPosition();
		Vector dir = GetPosition()-pos;
		dir.normalize();
		GRIDVECTOR vec;
		vec = pos+dir*(pData->recoveryRadius*GRIDSIZE);
		GotoPosition(vec,0);
		mode = MOVING_TO_RECOVERY;
	}
	THEMATRIX->OperationCompleted(agentID,dwMissionID);
}
//---------------------------------------------------------------------------
//
void RecoveryShip::OnStopRequest (U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void RecoveryShip::OnMasterChange (bool bIsMaster)
{
}
//---------------------------------------------------------------------------
//
void RecoveryShip::OnOperationCreation (U32 agentID, void *buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void RecoveryShip::receiveOperationData (U32 agentID, void *buffer, U32 bufferSize)
{
}
//---------------------------------------------------------------------------
//
void RecoveryShip::onOperationCancel (U32 agentID)
{
}
//---------------------------------------------------------------------------
//
void RecoveryShip::preSelfDestruct (void)
{
}
//---------------------------------------------------------------------------
//
void RecoveryShip::save (RECOVERYSHIP_SAVELOAD & save)
{
	save.baseSaveLoad = *static_cast<BASE_RECOVERYSHIP_SAVELOAD *>(this);
}
//---------------------------------------------------------------------------
//
void RecoveryShip::load (RECOVERYSHIP_SAVELOAD & load)
{
	*static_cast<BASE_RECOVERYSHIP_SAVELOAD *>(this) = load.baseSaveLoad;
}
//---------------------------------------------------------------------------
//
void RecoveryShip::ResolveAssociations (void)
{
	if(recoveryTargetID)
		recoveryTarget = OBJLIST->FindObject(recoveryTargetID);
	FRAME_resolve();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createRecoveryShip (const RECOVERYSHIP_INIT & data)
{
	RecoveryShip * obj = new ObjectImpl<RecoveryShip>;

	obj->FRAME_init(data);
	return obj;
}

//------------------------------------------------------------------------------------------
//---------------------------RecoveryShip Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE RecoveryShipFactory : public IObjectFactory
{
	struct OBJTYPE : RECOVERYSHIP_INIT
	{
		OBJTYPE ()
		{
			mineTex = 0;
		}

		~OBJTYPE (void)
		{
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(RecoveryShipFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	RecoveryShipFactory (void) { }

	~RecoveryShipFactory (void);

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

	/* FabricatorFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
RecoveryShipFactory::~RecoveryShipFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void RecoveryShipFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE RecoveryShipFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_SPACESHIP)
	{
		BT_RECOVERYSHIP_DATA * data = (BT_RECOVERYSHIP_DATA *) _data;


		if (data->type == SSC_RECOVERYSHIP)	   
		{
			result = new OBJTYPE;

			if (result->loadSpaceshipArchetype(data, ARCHLIST->GetArchetype(szArchname)) == false)
				goto Error;

			if (result->mineTex == 0)
				result->mineTex = TMANAGER->CreateTextureFromFile("nuggetbeam.tga",TEXTURESDIR, DA::TGA,PF_4CC_DAA4);
			
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
BOOL32 RecoveryShipFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	EditorStopObjectInsertion(objtype->pArchetype);
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * RecoveryShipFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createRecoveryShip(*objtype);
}
//-------------------------------------------------------------------
//
void RecoveryShipFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	EditorStartObjectInsertion(objtype->pArchetype, info);
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _recoveryship : GlobalComponent
{
	RecoveryShipFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<RecoveryShipFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _recoveryship __ship;

//---------------------------------------------------------------------------
//--------------------------End RecoveryShip.cpp--------------------------------
//---------------------------------------------------------------------------