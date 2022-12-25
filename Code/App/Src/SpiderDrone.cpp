//--------------------------------------------------------------------------//
//                                                                          //
//                             SpiderDrone.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/SpiderDrone.cpp 14    10/04/00 8:35p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObjTrans.h"
#include "TObjControl.h"
#include "TObjFrame.h"
#include "IBuildShip.h"  //ISpiderDrone
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
#include "TObjRender.h"

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

enum SDAction
{
	SD_IDLE,
	SD_SPIN,
	SD_SCUTTLE
};

struct SPIDERDRONE_INIT : BUILDSHIP_INIT
{
	const BT_BUILDERSHIP_DATA * pData;
	PARCHETYPE pArchetype;
	S32 archIndex;
	S32 animArchetype;
	IMeshArchetype * meshArch;

	PARCHETYPE pExplosion;
	AnimArchetype * sparkAnmArch;
	struct EngineSpiderDrone *engineSpiderDrones;

	virtual ~SPIDERDRONE_INIT()
	{
	}
};

struct ENGINE_SPIDERDRONE_INIT : SPIDERDRONE_INIT, RenderArch
{
	virtual ~ENGINE_SPIDERDRONE_INIT()
	{}
};

//false but adequate
struct ARCHNODE 
{
	struct ARCHNODE * prev, * next;
	U32 usage;
};


/*struct SpiderVisual
{

	inline void scuttleTo (const Vector & pos,const Transform &trans);

	inline void spinTo (const Vector & pos,const Transform &trans);

	inline BOOL32 updateMove (SINGLE dt);

	inline void doSpin (SINGLE dt);

	inline void doScuttle (SINGLE dt);	
};*/

struct SPIDERDRONE_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
};

struct EngineSpiderDrone : public ObjectRender<ObjectTransform<ObjectFrame<IBaseObject,SPIDERDRONE_SAVELOAD,ENGINE_SPIDERDRONE_INIT> > >
{	
	BEGIN_MAP_INBOUND(EngineSpiderDrone)
	_INTERFACE_ENTRY(IBaseObject)
	END_MAP()

	EngineSpiderDrone()
	{
		buildAnimIndex = -1;
	}
	
	~EngineSpiderDrone()
	{
		if(buildAnimIndex != -1)
		{
			ANIM->release_script_inst(buildAnimIndex);
			buildAnimIndex = -1;
		}
//		if(sparkAnim)
//			delete sparkAnim;
	}

	// the archtype for the explosion/fade
	PARCHETYPE pExplosion;

	S32 buildAnimIndex;

	bool initSpiderDrone (const ENGINE_SPIDERDRONE_INIT & data);
	
	BOOL32 updateMove (SINGLE dt);

	virtual void Render();
};

struct _NO_VTABLE SpiderDrone : public ObjectFrame<IBaseObject,SPACESHIP_SAVELOAD,SPIDERDRONE_INIT>, ISpiderDrone
{
	BEGIN_MAP_INBOUND(SpiderDrone)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISpiderDrone)
	_INTERFACE_ENTRY(IBuildShip)
	END_MAP()

	HardpointInfo  hardpointinfo;
	INSTANCE_INDEX dockIndex;

//	OBJPTR<IDebris, false> debris;
//	bool isFading;
 

//	AnimInstance * sparkAnim;
//	HardpointInfo  sparkHP;
//	INSTANCE_INDEX sparkHPIndex;
//	SINGLE sparkDelay;
//	SINGLE sparkTime;

	SINGLE visTimer;
	//U32 systemID;
	IBaseObject * owner;

	SINGLE moveTimer;
	SDAction action;
	//Spin physics
	Vector spinCenter,spin_i;
	Vector travelDir,vertDir;
	SINGLE vertFactor,vertAccel;
	Vector goalPosition;
	Vector velocity,position;
	int engineSpiderRef;

	EngineSpiderDrone *engineSpiderDrones;


	BOOL32 bDeleteRequested:1;


	SpiderDrone()
	{
		engineSpiderRef = rand()%3;
		dockIndex = -1;
	}
	
	~SpiderDrone()
	{
	}

	/* IBaseObject */

//	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

/*	virtual U32 GetSystemID (void) const
	{
		CQASSERT(0);
		return owner->GetSystemID();
	}*/

	virtual void Render();

//	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	//  ISpiderDrone

	virtual void ScuttleTo (const Vector & pos,bool bOverride=0);

	virtual void SpinTo (const Vector & pos);

	virtual void IdleAt (const Vector & pos);

	// IBuildShip

	virtual void InitBuildShip (IBaseObject * owner);

	virtual void Return ()
	{
		ScuttleTo(getStartPos().translation,true);
	}

	virtual void SetTransform (const TRANSFORM & trans)
	{
		position = trans.translation;
	}

	virtual void SetPosition (const Vector & pos);

/*	virtual void SetSystemID(U32 _systemID)
	{
		CQASSERT("ouch!");
		//systemID = _systemID;
	}*/

	virtual void WorkAtShipPos(IBaseObject * targ, Vector pos, Vector dir,SINGLE offSet = 0)
	{}

	virtual void SetStartHardpoint (HardpointInfo  _hardpointinfo, INSTANCE_INDEX _dockIndex);

	/* SpiderDrone methods */

	TRANSFORM getStartPos();

	BOOL32 updateMove (SINGLE dt);

	void doSpin (SINGLE dt);

	void doScuttle (SINGLE dt);
		
	bool initSpiderDrone (SPIDERDRONE_INIT *init);
};

//---------------------------------------------------------------------------
//
/*void SpiderDrone::FadeBuildShip(bool bExplode)
{
	isFading = TRUE;

	// create the debris and set up the debris pointer
	IBaseObject* obj = NULL;

	if (bExplode)
	{
		obj = CreateDebris(1, true, instanceIndex, pExplosion, systemID);
	}
	else
	{
		obj = CreateDebris(1, true, instanceIndex, NULL, systemID);
	}
	obj->QueryInterface(IDebrisID, debris);

	dockIndex = INVALID_INSTANCE_INDEX;

	CQASSERT(obj);	
	OBJLIST->AddObject(obj);
}
//---------------------------------------------------------------------------
//
BOOL SpiderDrone::IsFadeComplete()
{
	if (isFading)
	{
		return (debris == NULL);
	} 

	return FALSE;
}*/
//---------------------------------------------------------------------------
//
void SpiderDrone::InitBuildShip (IBaseObject * _owner)
{
	owner = _owner;
	MPartNC ownerPart(owner);


	//systemID = owner->GetSystemID();

//	action = SD_IDLE;
	bDeleteRequested = false;
}

void SpiderDrone::ScuttleTo(const Vector & pos,bool bOverride)
{
	if (action == SD_IDLE || bOverride)
	{
		action = SD_SCUTTLE;
		vertDir = pos-spinCenter;
		SINGLE mag = vertDir.fast_magnitude();
		if (mag < 1e-3)
		{
			action = SD_IDLE;
			return;
		}
		vertDir /= mag;
		travelDir = pos-position;
		mag = travelDir.fast_magnitude();
		travelDir /= mag;
		moveTimer = mag/SCUTTLE_VEL;
		spin_i = cross_product(vertDir,-travelDir);
		mag = spin_i.fast_magnitude();
		if (mag < 1e-3)
			spin_i.set(1,0,0);
		else
			spin_i /= mag;

	}
}

void SpiderDrone::SpinTo(const Vector & pos)
{
	if (action == SD_IDLE)
	{
		action = SD_SPIN;
	/*	startVel=transform.translation-spinCenter;
		startVel.fast_normalize();
		Vector end=-(pos-spinCenter);
		end.fast_normalize();
		angle = acos(dot_product(startVel,end));
		angle = 2*PI-angle;
		spinAxis = cross_product(startVel,end);
		SINGLE mag = spinAxis.fast_magnitude();
		if (mag < 1e-3)
		{
			action = SD_IDLE;
			return;
		}
		spinAxis /= mag;
		startVel *= 500;
		timer = 0;*/
		vertFactor = SPIN_ACCEL;
		vertDir = pos-spinCenter;
		SINGLE mag = vertDir.fast_magnitude();
		if (mag < 1e-3)
		{
			action = SD_IDLE;
			return;
		}
		vertDir /= mag;
		travelDir = pos-position;
		mag = travelDir.fast_magnitude();
		travelDir /= mag;
		vertAccel = 2*SPIN_ACCEL*SPIN_VEL/mag;
		spin_i = cross_product(vertDir,-travelDir);
		mag = spin_i.fast_magnitude();
		if (mag < 1e-3)
			spin_i.set(1,0,0);
		else
			spin_i /= mag;
	}
}

void SpiderDrone::IdleAt(const Vector & pos)
{
	position = pos;
}
//---------------------------------------------------------------------------
//
void SpiderDrone::Render (void)
{	
	/*if(!bVisible)
	{
		visTimer=0;
		return;
	}*/

	updateMove(visTimer);
	visTimer=0;
	
	engineSpiderDrones[engineSpiderRef].Render();
}

void EngineSpiderDrone::Render (void)
{

//	ILight * lights[8];
	LIGHT->deactivate_all_lights();
//	U32 numLights = LIGHT->get_best_lights(lights,8, transform.translation,4000);
//	LIGHT->activate_lights(lights,numLights);
	LIGHTS->ActivateBestLights(transform.translation,8,4000);

  	ENGINE->update_instance(instanceIndex,EN_UPDATE_WO_COMPONENTS,0);
	TreeRender(mc);
}
//---------------------------------------------------------------------------
//
/*BOOL32 SpiderDrone::Update (void)
{
	//TEMP
	if (owner==0)
		return 0;

	BOOL32 bResult = TRUE;


	return !bDeleteRequested && bResult && FRAME_update();
}*/
	
void SpiderDrone::PhysicalUpdate (SINGLE dt)
{
	visTimer += dt;
//	if (bVisible)
//		updateMove(dt);

/*	switch (action)
	{
	case SD_SPIN:
		doSpin(dt);
		break;
	case SD_SCUTTLE:
		doScuttle(dt);
		break;
	}*/

//	FRAME_physicalUpdate(dt);
}
//---------------------------------------------------------------------------
//
/*void SpiderDrone::SetTransform (const TRANSFORM &trans)
{
	transform = trans;
//	zCenter = transform.translation.z;
}*/

void SpiderDrone::SetPosition (const Vector &pos)
{
	spinCenter = position = pos;
//	zCenter = pos.z;
}
//---------------------------------------------------------------------------
//
/*void SpiderDrone::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	bVisible = 0;
	if(owner)
	{
		systemID = owner->GetSystemID();

	}

	if (systemID == currentSystem)
	{
		S32 screenX, screenY;
		if (CAMERA->PointToScreen(transform.translation, &screenX, &screenY, 0) != BEHIND_CAMERA)
		{
			PANE * pane = CAMERA->GetPane();
			const int offset = REAL2IDEALX(100);		// enough room for trail
			
			if (screenX+offset >= pane->x0 && screenX-offset <= pane->x1 &&
				screenY+offset >= pane->y0 && screenY-offset <= pane->y1)
			{
				if (defaults.bVisibilityRulesOff ||
					MGlobals::AreAllies(owner->GetPlayerID(),currentPlayer) ||
					FOGOFWAR->CheckVisiblePosition(transform.translation))
				{
					bVisible = 1;
				}
			}
		}
	}
}*/
//---------------------------------------------------------------------------
//
bool SpiderDrone::initSpiderDrone (SPIDERDRONE_INIT *init)
{
	pArchetype = init->pArchetype;
	objClass = OC_SPACESHIP;

	engineSpiderDrones = init->engineSpiderDrones;

	FRAME_init(*init); //????

	return true;
}

bool EngineSpiderDrone::initSpiderDrone (const ENGINE_SPIDERDRONE_INIT & data)
{
	FRAME_init(data);

//	pArchetype = data.pArchetype;
	pExplosion = data.pExplosion;
	objClass = OC_SPACESHIP;

	if (data.pData->workAnimation[0])
	{
		buildAnimIndex = ANIM->create_script_inst(data.animArchetype, instanceIndex, data.pData->workAnimation);
		ANIM->script_start(buildAnimIndex, Animation::LOOP);
	}

/*	if(data.sparkAnmArch)
	{
		sparkAnim = new AnimInstance;
		if (sparkAnim)
		{
			sparkAnim->Init(data.sparkAnmArch);
			sparkAnim->SetRotation(2*PI*(SINGLE)rand()/RAND_MAX);
			sparkAnim->SetWidth(data.pData->sparkWidth);
			sparkAnim->loop = false;
			sparkAnim->delay = data.pData->sparkDelay ;
			sparkDelay = data.pData->sparkDelay;
		}
	}

	if (data.pData->sparkHardpoint[0])
	{
		FindHardpoint(data.pData->sparkHardpoint, sparkHPIndex, sparkHP, instanceIndex);
	}*/
	return true;
}

//---------------------------------------------------------------------------
//
BOOL32 EngineSpiderDrone::updateMove (SINGLE dt)
{
	ANIM->update_instance(instanceIndex,dt);
	//visTimer = 0;

	return 1;
}

BOOL32 SpiderDrone::updateMove (SINGLE dt)
{
	if (action != SD_IDLE)
	{
		switch (action)
		{
		case SD_SPIN:
			doSpin(dt);
			break;
		case SD_SCUTTLE:
			doScuttle(dt);
			break;
		}

		position += velocity*dt;
	}
	
	Vector i,j,k;
	j = position-spinCenter;
	j.fast_normalize();
	k = cross_product(j,spin_i);
	TRANSFORM &transform = engineSpiderDrones[engineSpiderRef].transform;
	transform.set_i(spin_i);
	transform.set_j(j);
	transform.set_k(k);
	transform.translation = position;
	
	return 1;
}
//---------------------------------------------------------------------------
//
void SpiderDrone::doSpin (SINGLE dt)
{
//	SINGLE angleRate = angle;
	//Quaternion bob(spinAxis,timer*angleRate);
	//velocity = bob*startVel;
	velocity = travelDir*SPIN_VEL+vertFactor*vertDir;

	vertFactor -= vertAccel*dt;
	if (vertFactor < -SPIN_ACCEL)
	{
		action = SD_IDLE;
		velocity.set(0,0,0);
	}
}
//---------------------------------------------------------------------------
//
void SpiderDrone::doScuttle (SINGLE dt)
{
	velocity = travelDir*SCUTTLE_VEL;

	moveTimer -= dt;
	if (moveTimer < 0)
	{
		action = SD_IDLE;
		velocity.set(0,0,0);
	}
}
//---------------------------------------------------------------------------
//
/*BOOL32 SpiderDrone::spiderMove (void)
{
	Vector goal = goalPosition-GetPosition();
	velocity = goal;
	if(velocity.x != 0 ||velocity.y != 0 ||velocity.z != 0)
	{
		velocity.normalize();
		velocity = velocity*MANEUVER_ACCELERATION*2;
	}

	goal = -goalDir;
	SINGLE pitch = transform.get_pitch();
	SINGLE relPitch = get_angle(goal.y,goal.z) -pitch-(PI/2);
	if (relPitch < -PI)
		relPitch += PI*2;
	else
	if (relPitch > PI)
		relPitch -= PI*2;

	rotateShip(transform, 0,0,relPitch);

	return 1;
}*/
//---------------------------------------------------------------------------
//
/*BOOL32 SpiderDrone::craneMove (void)
{
	Vector goal = goalPosition-GetPosition();

	if (seekTarget)
	{
		setPosition(transform, goal, seekTarget.ptr->GetVelocity());
	}
	else
	{
		setPosition(transform,goal);
	}
	
	SINGLE yaw   = transform.get_yaw();
	SINGLE relPitch = -transform.get_pitch();
	SINGLE relRoll = -transform.get_roll();
	if(action == BS_RETURNING)
	{
		TRANSFORM dockTrans = getStartPos();
		if((dockTrans.get_position()- GetPosition()).magnitude_squared() <= 100000000.0)
		{
			goal = -dockTrans.get_k();			
		}
	}else if((action == BS_BUILDING) || (action == BS_WORK_RELATIVE) || (action == BS_RELATIVE_IDLE))
	{
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
	}
	SINGLE relYaw = get_angle(goal.x, goal.y) - yaw;

	if (relYaw < -PI)
		relYaw += PI*2;
	else
	if (relYaw > PI)
		relYaw -= PI*2;
	rotateShip(transform, relYaw, relRoll,relPitch);
	
	return 1;
}*/
//---------------------------------------------------------------------------
//
TRANSFORM SpiderDrone::getStartPos()
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
void SpiderDrone::SetStartHardpoint (HardpointInfo  _hardpointinfo, INSTANCE_INDEX _dockIndex)
{
	hardpointinfo = _hardpointinfo;
	dockIndex = _dockIndex;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createSpiderDrone (const ENGINE_SPIDERDRONE_INIT & data)
{
	EngineSpiderDrone * obj = new ObjectImpl<EngineSpiderDrone>;


	if (obj->initSpiderDrone(data))
	{
		return obj;
	}

	delete obj;
	return 0;
}

//------------------------------------------------------------------------------------------
//---------------------------SpiderDrone Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE SpiderDroneFactory : public IObjectFactory, IBuildShipFactory, IEventCallback
{
	struct OBJTYPE : ENGINE_SPIDERDRONE_INIT
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
			delete [] engineSpiderDrones;
			engineSpiderDrones = 0;
		}
	};

	struct ShipNode
	{
		struct ShipNode *next;
		struct SpiderDrone *drones;
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

	BEGIN_DACOM_MAP_INBOUND(SpiderDroneFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	DACOM_INTERFACE_ENTRY(IBuildShipFactory)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	SpiderDroneFactory (void) { }

	~SpiderDroneFactory (void);

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

	/* SpiderDroneFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
SpiderDroneFactory::~SpiderDroneFactory (void)
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
void SpiderDroneFactory::init (void)
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
HANDLE SpiderDroneFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_SPACESHIP)
	{
		BT_BUILDERSHIP_DATA * data = (BT_BUILDERSHIP_DATA *) _data;

		if (data->type == SSC_SPIDERDRONE)	   
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
			
			
			typedef ObjectImpl<EngineSpiderDrone> SpiderObj;
			result->engineSpiderDrones = new SpiderObj[3];
			for (int i=0;i<3;i++)
			{
				result->engineSpiderDrones[i].initSpiderDrone(*result);
			}
			//result->pArchetype->usage += 3;
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
BOOL32 SpiderDroneFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		(does not allow for this)
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * SpiderDroneFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createSpiderDrone(*objtype);
}
//-------------------------------------------------------------------
//
void SpiderDroneFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT SpiderDroneFactory::Notify (U32 message, void *param)
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
void SpiderDroneFactory::GetBuilderShips (struct IBuildShip **ships,int numShips,PARCHETYPE _pArchetype,IBaseObject * _owner)
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
		SPIDERDRONE_INIT *init = (SPIDERDRONE_INIT *)ARCHLIST->GetArchetypeHandle(_pArchetype);
		freeNode = new ShipNode;
		freeNode->next = shipNodeList;
		shipNodeList = freeNode;
		typedef ObjectImpl<SpiderDrone> SpiderObj;
		freeNode->drones = new SpiderObj[numShips];
		for (int i=0;i<numShips;i++)
		{
			freeNode->drones[i].initSpiderDrone(init);
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
void SpiderDroneFactory::ReleaseShips (const IBuildShip * firstShip)
{
	if (firstShip == 0)
		return;

	ShipNode * pos = shipNodeList;
	ShipNode * node=0;
	
	if (pos)
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
struct _spiderdrone : GlobalComponent
{
	SpiderDroneFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<SpiderDroneFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _spiderdrone __spiderdrone;
//---------------------------------------------------------------------------
//--------------------------End SpiderDrone.cpp--------------------------------
//---------------------------------------------------------------------------