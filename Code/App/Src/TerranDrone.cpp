//--------------------------------------------------------------------------//
//                                                                          //
//                             TerranDrone.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TerranDrone.cpp 12    10/04/00 8:35p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObjTrans.h"
#include "TObjFControl.h"
#include "TObjFrame.h"
#include "TObjRender.h"
#include "IBuildShip.h"  //ITerranDrone
#include <DShipSave.h>
#include "TObject.h"
#include "Objlist.h"
#include <DSpaceShip.h>
#include "Startup.h"
#include "Mission.h"
#include "IFabricator.h"
#include "IAnim.h"
#include "IExplosion.h"
#include "IBlast.h"
#include "anim2d.h"
#include "archHolder.h"
#include "Sector.h"
#include "Camera.h"
#include "UserDefaults.h"
#include "MGlobals.h"
#include "Fogofwar.h"
#include "MPart.h"

#include "CQLight.h"

#include <IHardPoint.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TSmartPointer.h>
#include <TComponent.h>
#include <ITextureLibrary.h>

#define SPIN_ACCEL 700.0
#define SPIN_VEL 200.0
#define SCUTTLE_VEL 400.0

enum TDAction
{
	TD_IDLE,
	TD_BUILDING,
	TD_MOVING
};

struct TERRANDRONE_INIT : BUILDSHIP_INIT
{
	struct EngineTerranDrone *engineTerranDrones;
	const BT_BUILDERSHIP_DATA * pData;
	PARCHETYPE pArchetype;
	//	PARCHETYPE pEngineArchetype;
	S32 archIndex;
	S32 animArchetype;
	IMeshArchetype * meshArch;
	
	PARCHETYPE pExplosion;
	AnimArchetype * sparkAnmArch;
	
	int numChildren;
	struct IMeshInfoTree *mesh_info;
	struct IMeshRender **mr;
	
	~TERRANDRONE_INIT()
	{
		for (int i=0;i<numChildren;i++)
		{
			mr[i]->Release();
		}
		delete [] mr;
	}
};

//false but adequate
struct ARCHNODE 
{
	struct ARCHNODE * prev, * next;
	U32 usage;
};


struct TERRANDRONE_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
};

struct EngineTerranDrone : public ObjectRender<ObjectTransform<ObjectFrame<IBaseObject,TERRANDRONE_SAVELOAD,TERRANDRONE_INIT> > >
{	
	BEGIN_MAP_INBOUND(EngineTerranDrone)
	_INTERFACE_ENTRY(IBaseObject)
	END_MAP()

		// the archtype for the explosion/fade
	PARCHETYPE pExplosion;

	S32 buildAnimIndex;

	TERRANDRONE_INIT *arch;

		AnimInstance * sparkAnim;
	HardpointInfo  sparkHP;
	INSTANCE_INDEX sparkHPIndex;

	EngineTerranDrone()
	{
		buildAnimIndex = -1;
	}
	
	~EngineTerranDrone()
	{
		if(buildAnimIndex != -1)
		{
			ANIM->release_script_inst(buildAnimIndex);
			buildAnimIndex = -1;
		}

		//here or in engine drone?
		if(sparkAnim)
			delete sparkAnim;
	}

	bool initTerranDrone (TERRANDRONE_INIT * data);
	
	BOOL32 updateMove (SINGLE dt);

	virtual void Render();
};

struct _NO_VTABLE TerranDrone : public ObjectFControl<ObjectTransform<ObjectFrame<IBaseObject,SPACESHIP_SAVELOAD,TERRANDRONE_INIT> > >, ITerranDrone
{
	BEGIN_MAP_INBOUND(TerranDrone)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ITerranDrone)
	_INTERFACE_ENTRY(IBuildShip)
	END_MAP()

	EngineTerranDrone *engineTerranDrones;

	HardpointInfo  hardpointinfo;
	INSTANCE_INDEX dockIndex;

	//SINGLE sparkDelay;
	SINGLE sparkTime;

	SINGLE visTimer;
	IBaseObject * owner;

	SINGLE moveTimer;
	TDAction action;

	//Spin physics?????
//	Vector travelDir,vertDir;
	Vector goalDir;
	SINGLE vertFactor,vertAccel;
	Vector goalPosition;
//	Vector velocity,position;
	int engineDroneRef;



	BOOL32 bDeleteRequested:1;


	TerranDrone()
	{
		engineDroneRef = rand()%3;
		dockIndex = -1;
	}
	
	~TerranDrone()
	{
	}

	/* IBaseObject */

	virtual void Render();

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void GotoPosition (const Vector & pos);

	//  ITerranDrone

	virtual void BuildAtPos (const Vector &pos, const Vector & dir);

	virtual void IdleAtPos (const Vector &pos);

	// IBuildShip

	virtual void InitBuildShip (IBaseObject * owner);
	
	virtual void Return ()
	{
		/*if(bWorking)
		{
			if(buildAnimIndex != -1)
			{
				ANIM->script_stop(buildAnimIndex);
				ANIM->script_start(buildAnimIndex);
			}
			bWorking = false;
		}*/
	//	action = BS_RETURNING;
		GotoPosition(getStartPos().get_position());
	}

	virtual void SetTransform (const TRANSFORM & trans)
	{
		transform = trans;	
	}

	virtual void SetPosition (const Vector & pos)
	{
		transform.translation = pos;
	}

	virtual void WorkAtShipPos(IBaseObject * targ, Vector pos, Vector dir,SINGLE offSet = 0)
	{}

	virtual void SetStartHardpoint (HardpointInfo  _hardpointinfo, INSTANCE_INDEX _dockIndex);

	/* TerranDrone methods */

	TRANSFORM getStartPos();

	BOOL32 updateMove (SINGLE dt);

	BOOL32 craneMove ();
	
	void doBuild ();

	bool initTerranDrone (TERRANDRONE_INIT *init);//PARCHETYPE pArchetype);
};

//---------------------------------------------------------------------------
//
void TerranDrone::BuildAtPos (const Vector &pos, const Vector &dir)
{
	action = TD_BUILDING;
	GotoPosition(pos);
	goalDir = dir;
}
//---------------------------------------------------------------------------
//
void TerranDrone::IdleAtPos (const Vector &pos)
{
	action = TD_IDLE;
	GotoPosition(pos);
}
//---------------------------------------------------------------------------
//
void TerranDrone::InitBuildShip (IBaseObject * _owner)
{
	owner = _owner;
	MPartNC ownerPart(owner);

	bDeleteRequested = false;
}

//---------------------------------------------------------------------------
//
void TerranDrone::Render (void)
{	
	/*if(!bVisible)
	{
		visTimer=0;
		return;
	}*/

	if (visTimer)
	{
		updateMove(visTimer);
		visTimer=0;
	}
	
	engineTerranDrones[engineDroneRef].Render();
}

void EngineTerranDrone::Render (void)
{

	LIGHT->deactivate_all_lights();
	LIGHTS->ActivateBestLights(transform.translation,8,1000);

  	ENGINE->update_instance(instanceIndex,0,0);
//	ENGINE->render_instance(MAINCAM, instanceIndex,0,LODPERCENT,0,0);
	TreeRender(mc.mi,mc.numChildren);

}

//---------------------------------------------------------------------------
//
void TerranDrone::GotoPosition (const Vector & pos)
{
/*	if(bLockedToHardpoint)
	{
		bLockedToShip = false;
		bLockedToHardpoint = false;

	}
	else if(seekTarget && bLockedToShip)
	{

	}
	bLockedToShip = false;*/
	goalPosition = pos;
	action = TD_MOVING;
//	zCenter = pos.z;
}
	
void TerranDrone::PhysicalUpdate (SINGLE dt)
{
	visTimer += dt;

}


/*void TerranDrone::SetPosition (const Vector &pos)
{
	position = pos;
}*/
//---------------------------------------------------------------------------
//
bool TerranDrone::initTerranDrone (TERRANDRONE_INIT *init)//PARCHETYPE _pArchetype)
{
	pArchetype = init->pArchetype;
	objClass = OC_SPACESHIP;
	engineTerranDrones = init->engineTerranDrones;

	FRAME_init(*init);

	return true;
}

bool EngineTerranDrone::initTerranDrone (TERRANDRONE_INIT * data)
{
	FRAME_init(*data);

	arch = data;

//	pArchetype = data->pEngineArchetype;
	pExplosion = data->pExplosion;
	objClass = OC_SPACESHIP;

	if (data->pData->workAnimation[0])
	{
		buildAnimIndex = ANIM->create_script_inst(data->animArchetype, instanceIndex, data->pData->workAnimation);
		ANIM->script_start(buildAnimIndex, Animation::LOOP);
	}

	if(data->sparkAnmArch)
	{
		sparkAnim = new AnimInstance;
		if (sparkAnim)
		{
			sparkAnim->Init(data->sparkAnmArch);
			sparkAnim->SetRotation(2*PI*(SINGLE)rand()/RAND_MAX);
			sparkAnim->SetWidth(data->pData->sparkWidth);
			sparkAnim->loop = false;
			sparkAnim->delay = data->pData->sparkDelay ;
			//sparkDelay = data->pData->sparkDelay;
		}
	}

	if (data->pData->sparkHardpoint[0])
	{
		FindHardpoint(data->pData->sparkHardpoint, sparkHPIndex, sparkHP, instanceIndex);
	}

	return true;
}

//---------------------------------------------------------------------------
//
BOOL32 EngineTerranDrone::updateMove (SINGLE dt)
{
	ANIM->update_instance(instanceIndex,dt);
	//visTimer = 0;

	return 1;
}

BOOL32 TerranDrone::updateMove (SINGLE dt)
{
	if (action != TD_IDLE)
	{
		switch (action)
		{
		case TD_MOVING:
			craneMove();
			break;
		case TD_BUILDING:
			doBuild();
			break;
		}

	//	position += velocity*dt;
	}
	
	FRAME_physicalUpdate(dt);

	engineTerranDrones[engineDroneRef].updateMove(dt);

/*	Vector i,j,k;
	j = position-spinCenter;
	j.fast_normalize();
	k = cross_product(j,spin_i);*/
	TRANSFORM &trans = engineTerranDrones[engineDroneRef].transform;
	/*transform.set_i(spin_i);
	transform.set_j(j);
	transform.set_k(k);*/
	trans = transform;
	
	return 1;
}
//---------------------------------------------------------------------------
//
BOOL32 TerranDrone::craneMove (void)
{
	Vector goal = goalPosition-GetPosition();

	setPosition(goal);
	
	SINGLE yaw   = transform.get_yaw();
	SINGLE relPitch = -transform.get_pitch();
	SINGLE relRoll = -transform.get_roll();
	
	goal = -goalDir;
	SINGLE pitch = transform.get_pitch();
	relPitch = get_angle(goal.y,goal.z) -pitch-(PI/2);
	if (relPitch < -PI)
		relPitch += PI*2;
	else
		if (relPitch > PI)
			relPitch -= PI*2;
		if(relPitch > (PI/2))
			relPitch = (PI/2)-relPitch;
		if(relPitch < -(PI/2))
			relPitch = (-(PI/2))-relPitch;
		if(relPitch+pitch > (PI/4))
			relPitch = (PI/4)-pitch;
		if(relPitch+pitch < -(PI/4))
			relPitch = -(PI/4)-pitch;
		
		SINGLE relYaw = get_angle(goal.x, goal.y) - yaw;
		
	if (relYaw < -PI)
		relYaw += PI*2;
	else
	if (relYaw > PI)
		relYaw -= PI*2;
	rotateShip(relYaw, relRoll,relPitch);
	
	return 1;
}

//---------------------------------------------------------------------------
//
void TerranDrone::doBuild (void)
{
	if((goalPosition-GetPosition()).magnitude_squared() <= 2500)
	{
//		if(!bWorking)
//		{
		//	if(buildAnimIndex != -1)
		//		ANIM->script_start(buildAnimIndex, Animation::LOOP);
		//	if(sparkAnim)
		//		sparkAnim->Restart();
//			bWorking = true;
//		}
//		updateMove();
	}
	else
	{
//		updateMove();
	}
}
//---------------------------------------------------------------------------
//
TRANSFORM TerranDrone::getStartPos()
{
	if(dockIndex != -1)
	{
		TRANSFORM result;
		result.TRANSFORM::TRANSFORM(hardpointinfo.orientation, hardpointinfo.point);
		result = ENGINE->get_transform(dockIndex).multiply(result);
		return result;
	}
	else
	{
		return owner->GetTransform();
	}
}
//---------------------------------------------------------------------------
//
void TerranDrone::SetStartHardpoint (HardpointInfo  _hardpointinfo, INSTANCE_INDEX _dockIndex)
{
	hardpointinfo = _hardpointinfo;
	dockIndex = _dockIndex;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createTerranDrone (TERRANDRONE_INIT * data)
{
	EngineTerranDrone * obj = new ObjectImpl<EngineTerranDrone>;


	if (obj->initTerranDrone(data))
	{
		return obj;
	}

	delete obj;
	return 0;
}

//------------------------------------------------------------------------------------------
//---------------------------TerranDrone Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE TerranDroneFactory : public IObjectFactory, IBuildShipFactory, IEventCallback
{
	struct OBJTYPE : TERRANDRONE_INIT
	{
		void * operator new (size_t size)
		{
			return calloc(size,1);
		}
		
		void * operator new [] (size_t size)
		{
			return calloc(size,1);
		}
		
		void   operator delete (void *ptr)
		{
			::free(ptr);
		}

		void   operator delete[] (void *ptr)
		{
			::free(ptr);
		}
		
		OBJTYPE (void)
		{
			animArchetype = archIndex = -1;
		}

		~OBJTYPE (void)
		{
			if (animArchetype != -1)
				ANIM->release_script_set_arch(animArchetype);
			if (archIndex != -1)
				ENGINE->release_archetype(archIndex);
			if(sparkAnmArch)
				delete sparkAnmArch;
			if (pExplosion)
				ARCHLIST->Release(pExplosion, OBJREFNAME);
			delete [] engineTerranDrones;
			engineTerranDrones = 0;
		}
	};

	struct ShipNode
	{
		struct ShipNode *next;
		struct TerranDrone *drones;
		U8 numDrones;
		bool bTaken;
		PARCHETYPE pArchetype;

		~ShipNode()
		{
			delete [] drones;
		}
	};

	ShipNode *shipNodeList;
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback
	U32 eventHandle;

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(TerranDroneFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	DACOM_INTERFACE_ENTRY(IBuildShipFactory)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	TerranDroneFactory (void) { }

	~TerranDroneFactory (void);

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

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param);

	IDAComponent * GetBase (void)
	{
		return (IEventCallback *) this;
	}

	//IBuildShipFactory methods

	virtual void GetBuilderShips (struct IBuildShip **ships,int numShips,PARCHETYPE _pArchetype,IBaseObject * _owner);

	virtual void ReleaseShips (const IBuildShip * firstShip);

	/* TerranDroneFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
TerranDroneFactory::~TerranDroneFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);

	if (GS)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}
}
//--------------------------------------------------------------------------//
//
void TerranDroneFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);

	if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
	{
		CQASSERT(eventHandle==0);
		connection->Advise(GetBase(), &eventHandle);
	}
}
//-----------------------------------------------------------------------------
//
HANDLE TerranDroneFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_SPACESHIP)
	{
		BT_BUILDERSHIP_DATA * data = (BT_BUILDERSHIP_DATA *) _data;

		if (data->type == SSC_TERRANDRONE)	   
		{
			result = new OBJTYPE;
			result->pData = data;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
			DAFILEDESC fdesc = data->fileName;
			COMPTR<IFileSystem> objFile;

			// load the explosion
			if (data->explosionType[0])
			{
				result->pExplosion = ARCHLIST->LoadArchetype(data->explosionType);
				ARCHLIST->AddRef(result->pExplosion, OBJREFNAME);
			}
 
			if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
				TEXLIB->load_library(objFile, 0);
			else
				goto Error;

			if ((result->archIndex = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX)
				goto Error;

			result->animArchetype = ANIM->create_script_set_arch(objFile);

			if (data->sparkAnim[0])
			{
				DAFILEDESC fdesc;
				COMPTR<IFileSystem> objFile;
				fdesc.lpFileName = data->sparkAnim;
				if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
				{
					result->sparkAnmArch = ANIM2D->create_archetype(objFile);
				}
				else 
				{
					CQFILENOTFOUND(fdesc.lpFileName);
					result->sparkAnmArch =0;
				}
			}	
			
			result->factory = this;
			
			
			typedef ObjectImpl<EngineTerranDrone> DroneObj;
			result->engineTerranDrones = new DroneObj[3];
			for (int i=0;i<3;i++)
			{
				result->engineTerranDrones[i].initTerranDrone(result);
			}
		//	result->pEngineArchetype->usage += 3;
		}
	}

	goto Done;

Error:
	delete result;
	result = 0;
Done:
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 TerranDroneFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		(does not allow for this)
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * TerranDroneFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createTerranDrone(objtype);
}
//-------------------------------------------------------------------
//
void TerranDroneFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT TerranDroneFactory::Notify (U32 message, void *param)
{
	switch (message)
	{
	case CQE_MISSION_CLOSE:
		if (param == 0)
		{
			ShipNode * pos = shipNodeList;
			ShipNode * nextShip;
		
			while (pos)
			{
				nextShip = pos->next;
				delete pos;
				pos = nextShip;
			}

			shipNodeList = 0;
		}
		break;
	}

	return GR_OK;
}
//-------------------------------------------------------------------
//
void TerranDroneFactory::GetBuilderShips (struct IBuildShip **ships,int numShips,PARCHETYPE _pArchetype,IBaseObject * _owner)
{
	ShipNode * pos = shipNodeList;
	ShipNode * freeNode=0;
	
	while (pos && freeNode == 0)
	{
		if ((!pos->bTaken) && numShips == pos->numDrones && pos->pArchetype == _pArchetype)
		{
			freeNode = pos;
		}
		pos = pos->next;
	}

	if (freeNode == 0)
	{
		TERRANDRONE_INIT *init = (TERRANDRONE_INIT *)ARCHLIST->GetArchetypeHandle(_pArchetype);
		freeNode = new ShipNode;
		freeNode->next = shipNodeList;
		shipNodeList = freeNode;
		typedef ObjectImpl<TerranDrone> DroneObj;
		freeNode->drones = new DroneObj[numShips];
		for (int i=0;i<numShips;i++)
		{
			freeNode->drones[i].initTerranDrone(init);
		}
		_pArchetype->usage += numShips;
		freeNode->numDrones = numShips;
		freeNode->pArchetype = _pArchetype;
	}

	freeNode->bTaken = true;

	for (int i=0;i<numShips;i++)
	{
		freeNode->drones[i].InitBuildShip(_owner);
		ships[i] = &freeNode->drones[i];
	}

}
//-------------------------------------------------------------------
//
void TerranDroneFactory::ReleaseShips (const IBuildShip * firstShip)
{
	ShipNode * pos = shipNodeList;
	ShipNode * node=0;
	
	if (pos && firstShip)
	{
		while (pos && node == 0)
		{
			if (pos->drones == firstShip)
			{
				node = pos;
			}
			pos = pos->next;
		}
		
		CQASSERT(node);
		CQASSERT(node->bTaken);
		node->bTaken = false;
	}
}
//---------------------------------------------------------------------------
//---------------------------GLOBAL STARTUP----------------------------------
//---------------------------------------------------------------------------
//
struct _terrandrone : GlobalComponent
{
	TerranDroneFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<TerranDroneFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _terrandrone __terrandrone;
//---------------------------------------------------------------------------
//--------------------------End TerranDrone.cpp--------------------------------
//---------------------------------------------------------------------------