//--------------------------------------------------------------------------//
//                                                                          //
//                             BuildShip.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/BuildShip.cpp 66    10/04/00 8:34p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObjTrans.h"
#include "TObjFControl.h"
#include "TObjFrame.h"
#include "IBuildShip.h"
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
#include "MeshExplode.h"
#include "TObjRender.h"

#include "CQLight.h"

#include <IHardPoint.h>
#include <FileSys.h>
#include <IConnection.h>
#include <TSmartPointer.h>
#include <TComponent.h>
#include <ITextureLibrary.h>

enum BSAction
{
	BS_RETURNING,
	BS_BUILDING,
	BS_SEEK,
	BS_IDLE,
	BS_IDLEING, //means activly waiting (could still be moving)
	BS_WORK_RELATIVE,
	BS_RELATIVE_IDLE,//still staying relative to target
};

enum BSStage
{
	BSS_NONE
};

struct SHUTTLE_INIT : BUILDSHIP_INIT, RenderArch
{
	const BT_BUILDERSHIP_DATA * pData;
	PARCHETYPE pArchetype;
	S32 archIndex;
	IMeshArchetype * meshArch;
	S32 animArchetype;

	PARCHETYPE pExplosion;
	AnimArchetype * sparkAnmArch;
};

struct _NO_VTABLE BuildShip : public ObjectRender<ObjectFControl<ObjectTransform<ObjectFrame<IBaseObject,SPACESHIP_SAVELOAD,SHUTTLE_INIT> > > >, IShuttle//, ROCKING_DATA
{
	BEGIN_MAP_INBOUND(BuildShip)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IBuildShip)
	_INTERFACE_ENTRY(IShuttle)
	END_MAP()

	HardpointInfo  hardpointinfo;
	INSTANCE_INDEX dockIndex;

	OBJPTR<IDebris> debris;
	bool isFading;
 
	S32 buildAnimIndex;
	AnimInstance * sparkAnim;
	HardpointInfo  sparkHP;
	INSTANCE_INDEX sparkHPIndex;

	U32 systemID;
	BSAction action;
	BSStage stage;
	IBaseObject * owner;
	OBJPTR<IBaseObject> seekTarget;
	SINGLE zCenter;
	Vector goalDir;
	Vector relPos;
	Vector relDir;

	Vector goalPosition;

	// the archtype for the explosion/fade
	PARCHETYPE pExplosion;

	BOOL32 bDeleteRequested:1;
	bool bLockedToHardpoint:1;
	bool bWorking:1;
	bool bLockedToShip:1;
	bool bRollUp:1;
	bool bAltUp:1;

	BuildShip()
	{
		buildAnimIndex = -1;
		debris = NULL;
		isFading = FALSE;
	}
	
	~BuildShip()
	{
		if(buildAnimIndex != -1)
		{
			ANIM->release_script_inst(buildAnimIndex);
			buildAnimIndex = -1;
		}
		if(sparkAnim)
			delete sparkAnim;
	}

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual void Render();

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt)
	{
		if(!owner)
			return;

		if(bWorking && sparkAnim)
		{
			if(buildAnimIndex != -1)
			{
				SINGLE sparkTime = ANIM->get_current_time(buildAnimIndex);
				sparkAnim->SetTime(sparkTime);
			}
			else
			{
				sparkAnim->update(dt);
			}
		}

		FRAME_physicalUpdate(dt);
	}

	virtual void SetTransform (const TRANSFORM &trans);

	virtual void SetPosition (const Vector &pos);

	virtual void SetStartHardpoint (HardpointInfo  _hardpointinfo, INSTANCE_INDEX _dockIndex);

	virtual void GotoPosition (const Vector & pos);

	virtual void SetSystemID(U32 _systemID)
	{
		systemID = _systemID;
	}

	/* IBaseObject */

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	/* BuildShip methods */


	virtual void FadeBuildShip(bool bExplode);

	virtual BOOL IsFadeComplete();

	virtual void InitBuildShip (IBaseObject * owner);

	virtual void GoAway();

	virtual void Return();

	virtual void SeekShip(IBaseObject * targ);

	virtual void BuildAtPos (Vector pos, Vector  dir);

	virtual void IdleAtPos (Vector pos);

	virtual void WorkAtShipPos(IBaseObject * targ, Vector pos, Vector dir, SINGLE offset = 0);

	virtual bool AtTarget();

	bool initBuildShip (const SHUTTLE_INIT & data);

	void doReturn();

	void doBuild();

	void doSeek();

	void doWorkRel();

	void doIdeling();

	TRANSFORM getStartPos();

	SINGLE getCircleYaw(const Vector & relVec, SINGLE yaw) const;

	// hijacked move methods

	BOOL32 updateMove (void);

	SINGLE getRelPitch (SINGLE pitch, SINGLE relAltitude)
	{
		if (relAltitude < -500)
			return (-20 * MUL_DEG_TO_RAD) - pitch;
		else
		if (relAltitude >  500)
			return (20 * MUL_DEG_TO_RAD) - pitch;
		else
			return 0 - pitch;
	}

	void setColors(U32 playerID);
};
//---------------------------------------------------------------------------
//
void BuildShip::FadeBuildShip(bool bExplode)
{
	isFading = TRUE;

	// create the debris and set up the debris pointer
	IBaseObject* obj = NULL;

	PHYS_CHUNK phys;
	phys.arm = -GetCenterOfMass(mc.mi[0]);  //??

	if (bExplode)
	{
		obj = CreateDebris(1, true, instanceIndex, pExplosion, systemID,&phys);
	}
	else
	{
		obj = CreateDebris(1, true, instanceIndex, NULL, systemID,&phys);
	}

	if (obj)
	{
		obj->QueryInterface(IDebrisID, debris, NONSYSVOLATILEPTR);
		dockIndex = INVALID_INSTANCE_INDEX;

//		CQASSERT(obj);	
		OBJLIST->AddObject(obj);
	}
}
//---------------------------------------------------------------------------
//
BOOL BuildShip::IsFadeComplete()
{
	if (isFading)
	{
		return (debris == NULL);
	} 

	return FALSE;
}
//---------------------------------------------------------------------------
//
void BuildShip::InitBuildShip (IBaseObject * _owner)
{
	owner = _owner;
	dockIndex = -1;

	systemID = owner->GetSystemID();

	action = BS_IDLE;
	stage = BSS_NONE;
	seekTarget = NULL;
	bDeleteRequested = false;
	bLockedToHardpoint = true;

	setColors(_owner->GetPlayerID());
}   
//---------------------------------------------------------------------------
//
void BuildShip::Render (void)
{	
	if(!bVisible)
		return;

//	ILight * lights[8];
	LIGHT->deactivate_all_lights();
	//U32 numLights = LIGHT->get_best_lights(lights,8, GetTransform().translation,4000);
	//LIGHT->activate_lights(lights,numLights);
	LIGHTS->ActivateBestLights(transform.translation,8,4000);

	TreeRender(mc);
//	ENGINE->render_instance(MAINCAM, instanceIndex,0,LODPERCENT,0,0);

	if(sparkAnim && bWorking)
	{		
//		updateChildPositions();

		TRANSFORM result;
		result.TRANSFORM::TRANSFORM(sparkHP.orientation, sparkHP.point);
		result = ENGINE->get_transform(sparkHPIndex).multiply(result);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		ANIM2D->render(sparkAnim,&(result));
	}

}
//---------------------------------------------------------------------------
//
BOOL32 BuildShip::Update (void)
{
	//TEMP
	if (owner==0)
		return 0;

	BOOL32 bResult = TRUE;

	switch (action)
	{
	case BS_RETURNING:
		doReturn();
		break;
	case BS_BUILDING:
		doBuild();
		break;
	case BS_SEEK:
		doSeek();
		break;
	case BS_IDLEING:
		doIdeling();
		break;
	case BS_RELATIVE_IDLE:
	case BS_WORK_RELATIVE:
		doWorkRel();
		break;
	}
	return !bDeleteRequested && bResult && FRAME_update();
}
//---------------------------------------------------------------------------
//
void BuildShip::SetTransform (const TRANSFORM &trans)
{
	transform = trans;
	zCenter = transform.translation.z;
}

void BuildShip::SetPosition (const Vector &pos)
{
	transform.translation = pos;
	zCenter = pos.z;
}
//---------------------------------------------------------------------------
//
void BuildShip::SetStartHardpoint (HardpointInfo  _hardpointinfo, INSTANCE_INDEX _dockIndex)
{
	hardpointinfo = _hardpointinfo;
	dockIndex = _dockIndex;
	bLockedToHardpoint = true;
	SetTransform(getStartPos());
}
//---------------------------------------------------------------------------
//
void BuildShip::Return()
{
	seekTarget = NULL;
	if(bWorking)
	{
		if(buildAnimIndex != -1)
		{
			ANIM->script_stop(buildAnimIndex);
			ANIM->script_start(buildAnimIndex);
		}
		bWorking = false;
	}
	action = BS_RETURNING;
	GotoPosition(getStartPos().get_position());
}
//---------------------------------------------------------------------------
//
void BuildShip::SeekShip(IBaseObject * targ)
{
	if(bWorking)
	{
		if(buildAnimIndex != -1)
		{
			ANIM->script_stop(buildAnimIndex);
			ANIM->script_start(buildAnimIndex);
		}
		bWorking = false;
	}
	if (targ)
		targ->QueryInterface(IBaseObjectID, seekTarget, SYSVOLATILEPTR);
	else
		seekTarget = 0;
	GotoPosition(seekTarget->GetPosition());
	action = BS_SEEK;
}
//---------------------------------------------------------------------------
//
void BuildShip::BuildAtPos (Vector pos, Vector dir)
{
	seekTarget = NULL;
	action = BS_BUILDING;
	GotoPosition(pos);
	goalDir = dir;
}
//---------------------------------------------------------------------------
//
void BuildShip::IdleAtPos (Vector pos)
{
	seekTarget = NULL;
	if(bWorking)
	{
		if(buildAnimIndex != -1)
		{
			ANIM->script_stop(buildAnimIndex);
			ANIM->script_start(buildAnimIndex);
		}
		bWorking = false;
	}
	action = BS_IDLEING;
	GotoPosition(pos);
}
//---------------------------------------------------------------------------
//
void BuildShip::WorkAtShipPos(IBaseObject * targ, Vector pos, Vector dir,SINGLE offSet)
{
	if (targ)
		targ->QueryInterface(IBaseObjectID, seekTarget, SYSVOLATILEPTR);
	else
		seekTarget = 0;
	action = BS_WORK_RELATIVE;
	relPos = pos+(dir*offSet);
	relDir = dir;
	pos = targ->GetTransform().rotate_translate(pos);
	goalDir = targ->GetTransform().rotate(dir);
	GotoPosition(pos);
}
//---------------------------------------------------------------------------
//
bool BuildShip::AtTarget()
{
	return (action == BS_IDLE) || (action == BS_RELATIVE_IDLE);
}
//---------------------------------------------------------------------------
//
void BuildShip::GotoPosition (const Vector & pos)
{
	if(bLockedToHardpoint)
	{
		bLockedToShip = false;
		bLockedToHardpoint = false;
/*
		bits.thrustUp = 1;
		if(owner)
		{
			velocity = (GetPosition()-owner->GetPosition());
			if(velocity.x != 0 ||velocity.y != 0 ||velocity.z != 0)
			{
				velocity.normalize();
				velocity = velocity*MANEUVER_ACCELERATION*10;
			}
		}
*/
	}
	else if(seekTarget && bLockedToShip)
	{
/*
		velocity = (GetPosition()-seekTarget->GetPosition());
		if(velocity.x != 0 ||velocity.y != 0 ||velocity.z != 0)
		{
			velocity.normalize();
			velocity = velocity*MANEUVER_ACCELERATION*10;
		}
*/
	}
	bLockedToShip = false;
	goalPosition = pos;
	zCenter = pos.z;
//	DYNAMICS_DATA::MAX_SIDEWAYS_VELOCITY = 1500;
//	DYNAMICS_DATA::MAX_FORWARD_VELOCITY = 1000;
//	DYNAMICS_DATA::MAX_MANEUVER_VELOCITY = 1000;
}
//---------------------------------------------------------------------------
//
void BuildShip::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	bVisible = 0;
	if(owner)
	{
		systemID = owner->GetSystemID();
		if(bLockedToHardpoint)
		{
			bVisible = owner->bVisible;
			SetTransform(getStartPos());
			return;//don't want to test for visible any more;
		}
	}

	if (seekTarget && bLockedToShip)
	{
		Vector pos = seekTarget->GetTransform().rotate_translate(relPos);
		goalDir = seekTarget->GetTransform().rotate(relDir);
		SetPosition(pos);
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
}
//---------------------------------------------------------------------------
//
bool BuildShip::initBuildShip (const SHUTTLE_INIT & data)
{
	pArchetype = data.pArchetype;
	pExplosion = data.pExplosion;
	objClass = OC_SPACESHIP;
	transform = TRANSFORM::WORLD;

//	*static_cast<ROCKING_DATA *> (this) = data.pData->rockingData;

/*	rockLinearAccel *= ELAPSED_TIME;
	rockAngAccel *= ELAPSED_TIME;		// speed of rocking
	bRollUp = ((rand() & 1) == 0);
	bAltUp = ((rand() & 1) == 0);*/

	if (data.pData->workAnimation[0])
	{
		buildAnimIndex = ANIM->create_script_inst(data.animArchetype, instanceIndex, data.pData->workAnimation);
	}

	if(data.sparkAnmArch)
	{
		sparkAnim = new AnimInstance;
		if (sparkAnim)
		{
			sparkAnim->Init(data.sparkAnmArch);
			sparkAnim->SetRotation(2*PI*(SINGLE)rand()/RAND_MAX);
			sparkAnim->SetWidth(data.pData->sparkWidth);
			sparkAnim->loop = false;
			sparkAnim->delay = data.pData->sparkDelay ;
//			sparkDelay = data.pData->sparkDelay;
		}
	}

	if (data.pData->sparkHardpoint[0])
	{
		FindHardpoint(data.pData->sparkHardpoint, sparkHPIndex, sparkHP, instanceIndex);
	}
	return true;
}
//---------------------------------------------------------------------------
//
void BuildShip::GoAway()
{
	bDeleteRequested = TRUE;
}

//---------------------------------------------------------------------------
//
void BuildShip::doReturn()
{
	updateMove();
	TRANSFORM startPos = getStartPos();
	if(((startPos.get_position()-GetPosition()).magnitude_squared() <= 1600) && (transform.get_yaw()-startPos.get_yaw() <= 0.1)
		&& (transform.get_pitch()-startPos.get_pitch() <= 0.15))
	{
		action = BS_IDLE;
		bLockedToHardpoint = true;
	}else
	{
		GotoPosition(startPos.get_position());
		updateMove();
	}
}
//---------------------------------------------------------------------------
//
void BuildShip::doBuild (void)
{
	if((goalPosition-GetPosition()).magnitude_squared() <= 2500)
	{
		if(!bWorking)
		{
			if(buildAnimIndex != -1)
				ANIM->script_start(buildAnimIndex, Animation::LOOP);
			if(sparkAnim)
				sparkAnim->Restart();
//			sparkTime = 0;
			bWorking = true;
		}
		updateMove();
	}
	else
	{
		updateMove();
	}
}

//---------------------------------------------------------------------------
//
void BuildShip::doSeek()
{
	if(seekTarget)
	{
		updateMove();
		Vector targPos = seekTarget->GetPosition();
		if((targPos - goalPosition).magnitude() >50)
			GotoPosition(targPos);
		else if((goalPosition-GetPosition()).magnitude_squared() <= 10000)
			action = BS_IDLE;
	}
	else
		action = BS_IDLE;
}
//---------------------------------------------------------------------------
//
void BuildShip::doWorkRel()
{
/*	if (seekTarget && bLockedToShip)
	{
		// want to stay at the same relative direction and distance from the ship we're locked too
		updateMove();
		Vector pos = seekTarget->GetTransform().rotate_translate(relPos);
		goalDir = seekTarget->GetTransform().rotate(relDir);
//		SetPosition(pos);
	}
*/
	if(seekTarget)
	{
		updateMove();
		Vector pos = seekTarget->GetTransform().rotate_translate(relPos);
		goalDir = seekTarget->GetTransform().rotate(relDir);

		GotoPosition(pos);
		if((goalPosition-GetPosition()).magnitude_squared() <= 250000)
		{
			if(!bWorking)
			{
				if(buildAnimIndex != -1)
					ANIM->script_start(buildAnimIndex, Animation::LOOP);
				if(sparkAnim)
					sparkAnim->Restart();
//				sparkTime = 0;
				bWorking = true;
			}
			bLockedToShip = true;
			action = BS_RELATIVE_IDLE;
		}
	}
	else
		action = BS_IDLE;
}
//---------------------------------------------------------------------------
//
void BuildShip::doIdeling()
{
	updateMove();
	if((goalPosition-GetPosition()).magnitude_squared() <= 250000)
	{
		action = BS_IDLE;
	}
}
//---------------------------------------------------------------------------
//
BOOL32 BuildShip::updateMove (void)
{
	Vector goal = goalPosition-GetPosition();

	if (seekTarget)
	{
		setPosition(goal);
	}
	else
	{
		setPosition(goal);
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
	rotateShip(relYaw, relRoll,relPitch);
	
	return 1;
}
//---------------------------------------------------------------------------
//
TRANSFORM BuildShip::getStartPos()
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

void BuildShip::setColors (U32 playerID)
{	
	COLORREF color = COLORTABLE[MGlobals::GetColorID(playerID)];
	
	for (int c=0;c<mc.numChildren;c++)
	{	
		if (mc.mi[c]->bHasMesh)
		{
			IRenderMaterial *irm=0;
			
			while((irm = mc.mi[c]->mr->GetNextFaceGroup(irm)) != 0)
			{
				FaceGroupInfo *fgi = &mc.mi[c]->fgi[irm->fg_idx];

				if (irm->flags & MM_MARKINGS)
				{
					color = COLORTABLE[MGlobals::GetColorID(playerID)];
					
					fgi->diffuse.r = GetRValue(color);
					fgi->diffuse.g = GetGValue(color);
					fgi->diffuse.b = GetBValue(color);
					
					fgi->emissive.r = 0.3*fgi->diffuse.r;
					fgi->emissive.g = 0.3*fgi->diffuse.g;
					fgi->emissive.b = 0.3*fgi->diffuse.b;
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createBuildShip (const SHUTTLE_INIT & data)
{
	BuildShip * obj = new ObjectImpl<BuildShip>;
	obj->FRAME_init(data);

	if (obj->initBuildShip(data))
	{
		return obj;
	}

	delete obj;
	return 0;
}

//------------------------------------------------------------------------------------------
//---------------------------BuildShip Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE BuildShipFactory : public IObjectFactory
{
	struct OBJTYPE : SHUTTLE_INIT
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
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(BuildShipFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	BuildShipFactory (void) { }

	~BuildShipFactory (void);

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

	/* BuildShipFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
BuildShipFactory::~BuildShipFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void BuildShipFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE BuildShipFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_SPACESHIP)
	{
		BT_BUILDERSHIP_DATA * data = (BT_BUILDERSHIP_DATA *) _data;

		if (data->type == SSC_BUILDSHIP)	   
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
					result->sparkAnmArch->delay = data->sparkDelay;
				}
				else 
				{
					CQFILENOTFOUND(fdesc.lpFileName);
					result->sparkAnmArch =0;
				}
			}		
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
BOOL32 BuildShipFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		(does not allow for this)
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * BuildShipFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createBuildShip(*objtype);
}
//-------------------------------------------------------------------
//
void BuildShipFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _buildship : GlobalComponent
{
	BuildShipFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<BuildShipFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _buildship __ship;
//---------------------------------------------------------------------------
//--------------------------End BuildShip.cpp--------------------------------
//---------------------------------------------------------------------------