//--------------------------------------------------------------------------//
//                                                                          //
//                                JumpLauncher.cpp                          //
//                                                                          //
//                  COPYRIGHT (C) 2004 BY Warthog                           //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header:
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "SuperTrans.h"
#include "ObjList.h"
#include "sfx.h"

#include "Mission.h"
#include "IMissionActor.h"
#include "Startup.h"
#include "ArchHolder.h"
#include "TerrainMap.h"
#include "MPart.h"
#include "TObject.h"
#include <DJumpLauncher.h>
#include "ILauncher.h"
#include "IWeapon.h"
#include "DSpaceShip.h"
#include "IGotoPos.h"
#include "OpAgent.h"
#include "IShipMove.h"
#include "ObjMapIterator.h"
#include "CommPacket.h"
#include "sector.h"
#include "ObjSet.h"
#include "camera.h"
#include "MeshRender.h"
#include "Anim2d.h"

#include <Engine.h>
#include <TSmartPointer.h>
#include <IAnim.h>
#include <FileSys.h>
#include <TComponent.h>
#include <IConnection.h> 
#include <MGlobals.h>
#include <DMTechNode.h>
#include <renderer.h>

#include <stdlib.h>

// so that we can use the techtree stuff
using namespace TECHTREE;

struct JumpLauncherArchetype
{
	AnimArchetype * jumpRingAnim;
};

const SINGLE JUMP_OUT_TIME = 1.0f;
const SINGLE JUMP_IN_TIME = 1.0f;

//network commands
const U8 JL_NET_JUMP_IN = 0;

//particle goodness
struct JumpPart
{
	Vector pos;
	SINGLE lifetime;
	SINGLE startTime;
	bool bAlive:1;
};

#define MAX_JUMP_PARTICLES 100
#define JUMP_RING_SIZE 1000

struct JumpParticleRing
{
	bool bInitialize;
	SINGLE frequency;
	SINGLE time;
	SINGLE createTime;
	AnimArchetype * animArch;
	U32 systemID;

	U8 red;
	U8 green;
	U8 blue;

	JumpPart particles[MAX_JUMP_PARTICLES];

	JumpParticleRing()
	{
		bInitialize = false;
	}

	void Init(SINGLE freq, AnimArchetype * _animArch, U8 _red, U8 _green,U8 _blue, U32 _systemID);
	
	void Render(const Vector & hitDir, const Vector & position);

	void PhysUpdate(SINGLE progress, SINGLE dt);
};

void JumpParticleRing::Init(SINGLE freq, AnimArchetype * _animArch, U8 _red, U8 _green,U8 _blue, U32 _systemID)
{
	if(_animArch)
	{
		bInitialize  = true;
		systemID = _systemID;
		red = _red;
		green = _green;
		blue = _blue;
		animArch = _animArch;
		frequency = freq;
		time = 0;
		createTime = 0;
		for(U32 i = 0; i < MAX_JUMP_PARTICLES; ++i)
		{
			particles[i].bAlive = false;
		}
	}
}

void JumpParticleRing::Render(const Vector & hitDir, const Vector & position)
{
	if(!bInitialize )
		return;
	Vector j(0,0,1);
	Vector i = cross_product(j,hitDir);
	i.fast_normalize();
	j = cross_product(hitDir,i);
	j.fast_normalize();

	TRANSFORM trans;
	trans.set_i(i);
	trans.set_j(j);
	trans.set_k(hitDir);
	trans.translation = position;

	Vector cpos (CAMERA->GetPosition());

	Vector lookAt (CAMERA->GetLookAtPosition());

	Vector k = lookAt-cpos;
	k.fast_normalize();
	
	Vector tmpUp(0,0,1);

	j = Vector(cross_product(k,tmpUp));
	j.fast_normalize();

	i = Vector(cross_product(j,k));
	i.fast_normalize();

	CAMERA->SetPerspective();

	CAMERA->SetModelView();
	
	BATCH->set_state(RPR_BATCH,true);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	BATCH->set_state(RPR_STATE_ID,animArch->frames[0].texture);
	
	BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	SetupDiffuseBlend(animArch->frames[0].texture,TRUE);

	PB.Begin(PB_QUADS);
	PB.Color4ub(red,green,blue,255);
	for(U32 counter = 0; counter < MAX_JUMP_PARTICLES; ++counter)
	{
		if(particles[counter].bAlive)
		{
			Vector pos = trans.rotate_translate(particles[counter].pos);
			SINGLE size = 400*(1.0-((time - particles[counter].startTime)/(particles[counter].lifetime-particles[counter].startTime)));
	
			U32 totalTimeMS = (animArch->frame_cnt*1000)/animArch->capture_rate;
			U32 frameTimeMS = (time+particles[counter].startTime)*1000;
			frameTimeMS = (frameTimeMS+(F2LONG((counter*1000)*animArch->capture_rate)))%totalTimeMS;
			U32 frame = ((frameTimeMS*animArch->capture_rate)/1000);
			frame = frame%animArch->frame_cnt;//just in case
		
			const AnimFrame* anim = &(animArch->frames[frame]);

			Vector v0 = pos - i*size - j*size;
			Vector v1 = pos + i*size - j*size;
			Vector v2 = pos + i*size + j*size;
			Vector v3 = pos - i*size + j*size;

			PB.TexCoord2f (anim->x0, anim->y0);
			PB.Vertex3f (v0.x, v0.y, v0.z);
			PB.TexCoord2f (anim->x1, anim->y0);
			PB.Vertex3f (v1.x, v1.y, v1.z);
			PB.TexCoord2f (anim->x1, anim->y1);
			PB.Vertex3f (v2.x, v2.y, v2.z);
			PB.TexCoord2f (anim->x0, anim->y1);
			PB.Vertex3f (v3.x, v3.y, v3.z);

		}
	}
	PB.End();
	BATCH->set_state(RPR_BATCH, FALSE);
}

void JumpParticleRing::PhysUpdate(SINGLE progress, SINGLE dt)
{
	if(!bInitialize )
		return;
	time += dt;
	createTime += dt;
	U32 numToCreate = 0;
	numToCreate = createTime*frequency;
	if(numToCreate)
	{
		createTime = createTime - (numToCreate/frequency);
	}
	bInitialize = false;
	for(U32 i = 0; i < MAX_JUMP_PARTICLES; ++i)
	{
		if(particles[i].bAlive)
		{
			bInitialize = true;
			if(particles[i].lifetime < time)
			{
				particles[i].bAlive = false;
			}
		}
		if(numToCreate && (!particles[i].bAlive))
		{
			bInitialize = true;
			particles[i].bAlive = true;
			particles[i].startTime = time;
			particles[i].lifetime = 0.2f+(((SINGLE)(rand()%10000))/10000.0f)*0.3f+time;
			SINGLE angle = (((SINGLE)(rand()%10000))/10000.0f)*2*PI;
			particles[i].pos.z = JUMP_RING_SIZE *((progress*2)-1);
			SINGLE radius = (((SINGLE)(rand()%10000))/10000.0f)*sqrt((JUMP_RING_SIZE *JUMP_RING_SIZE )-(particles[i].pos.z*particles[i].pos.z));
			particles[i].pos.x = cos(angle)*radius;
			particles[i].pos.y = sin(angle)*radius;
			--numToCreate;
		}
	}
}

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE JumpLauncher : IBaseObject, ILauncher, ISaveLoad, JUMP_LAUNCHER_SAVELOAD
{
	BEGIN_MAP_INBOUND(JumpLauncher)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	BT_JUMP_LAUNCHER * pData;

	OBJPTR<ILaunchOwner> owner;	// person who created us

	MeshChain *mc;
	SINGLE shipLength;

	JumpParticleRing ring;

	//----------------------------------
	//----------------------------------
	
	JumpLauncher (void);

	~JumpLauncher (void);	

	/* IBaseObject methods */

	virtual const TRANSFORM & GetTransform (void) const;

	virtual BOOL32 Update (void);	// returning FALSE causes destruction
	
	virtual void PhysicalUpdate (SINGLE dt);

	virtual S32 GetObjectIndex (void) const;

	virtual U32 GetPartID (void) const;

	virtual U32 GetSystemID (void) const
	{
		return owner.Ptr()->GetSystemID();
	}

	virtual U32 GetPlayerID (void) const
	{
		return owner.Ptr()->GetPlayerID();
	}

	virtual void Render();

	/* ILauncher methods */

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);
	

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial);

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
	{}

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void DoCreateWormhole(U32 systemID){};

	virtual const bool TestFightersRetracted (void) const 
	{ 
		return true;
	}

	virtual void SetFighterStance(FighterStance stance)
	{
	}

	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID)
	{
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	virtual void DoSpecialAbility (U32 specialID);

	virtual void DoSpecialAbility (IBaseObject *obj)
	{
	}

	virtual void DoCloak (void)
	{
	}
	
	virtual void SpecialAttackObject (IBaseObject * obj)
	{
	}

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bSpecialEnabled)
	{
		MPart part = owner.Ptr();

		ability = USA_JUMP;
		bSpecialEnabled = part->caps.specialEOAOk && checkSupplies();
	}

	virtual const U32 GetApproxDamagePerSecond (void) const
	{
		return 0;
	}

	virtual void InformOfCancel();

	virtual void LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize);

	virtual void LauncherReceiveOpData(U32 agentID, void * buffer, U32 bufferSize);

	virtual void LauncherOpCompleted(U32 agentID);

	virtual bool CanCloak(){return false;};

	virtual bool IsToggle() {return false;};

	virtual bool CanToggle(){return false;};

	virtual bool IsOn() {return false;};

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const
	{
		return 0;
	}

	virtual U32 GetSyncData (void * buffer)
	{
		return 0;
	}

	virtual void PutSyncData (void * buffer, U32 bufferSize)
	{
	}

	virtual void OnAllianceChange (U32 allyMask)
	{
	}

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file)
	{
		return TRUE;
	}

	virtual BOOL32 Load (struct IFileSystem * file)
	{
		return FALSE;
	}
	
	virtual void ResolveAssociations()
	{
	}

	/* JumpLauncher methods */
	
	void init (PARCHETYPE pArchetype);

	bool checkSupplies();
};

//---------------------------------------------------------------------------
//
JumpLauncher::JumpLauncher (void) 
{
}
//---------------------------------------------------------------------------
//
JumpLauncher::~JumpLauncher (void)
{
}
//---------------------------------------------------------------------------
//
void JumpLauncher::init (PARCHETYPE _pArchetype)
{
	pData = (BT_JUMP_LAUNCHER *) ARCHLIST->GetArchetypeData(_pArchetype);

	CQASSERT(pData);
	CQASSERT(pData->type == LC_JUMP_LAUNCH);
	CQASSERT(pData->objClass == OC_LAUNCHER);

	pArchetype = _pArchetype;
	objClass = OC_LAUNCHER;
	jumpMode = JUMP_LAUNCHER_SAVELOAD::NO_JUMP;
}
//---------------------------------------------------------------------------
//
void JumpLauncher::DoSpecialAbility (U32 specialID)
{

}
//---------------------------------------------------------------------------
//
S32 JumpLauncher::GetObjectIndex (void) const
{
	// not too sure about this...
	return owner.Ptr()->GetObjectIndex();
}
//---------------------------------------------------------------------------
//
U32 JumpLauncher::GetPartID (void) const
{
	return owner.Ptr()->GetPartID();
}
//---------------------------------------------------------------------------
//
void JumpLauncher::Render()
{
	if(owner.Ptr()->bSpecialRender && jumpMode != JUMP_LAUNCHER_SAVELOAD::NO_JUMP)
	{
		SINGLE progress = 1.0;
		if(jumpMode == JUMP_LAUNCHER_SAVELOAD::JUMPING_OUT)
		{
			progress = (timer/JUMP_OUT_TIME);
			progress = __min(1.0,progress);
		}
		else //jumping in
		{
			progress = 1.0-(timer/JUMP_OUT_TIME);
			progress = __max(0,progress);
		}

		TRANSFORM transform = owner.Ptr()->GetTransform();
		Vector split_z = Vector(0,0,-shipLength/2+shipLength*progress);
		split_z = transform*split_z;
		TreeRenderPortionZAlign(mc->mi,mc->numChildren,split_z);
		
		ring.Render(transform.get_k(),transform.translation);
	}
}
//---------------------------------------------------------------------------
//
BOOL32 JumpLauncher::Update (void)
{
	if(jumpMode == JUMP_LAUNCHER_SAVELOAD::JUMPING_OUT)
	{
		SINGLE progress = 1.0;
		if(jumpMode == JUMP_LAUNCHER_SAVELOAD::JUMPING_OUT)
		{
			progress = (timer/JUMP_OUT_TIME);
			progress = __min(1.0,progress);
		}
		else //jumping in
		{
			progress = 1.0-(timer/JUMP_OUT_TIME);
			progress = __max(0,progress);
		}

		if(THEMATRIX->IsMaster())
		{
			if(timer > JUMP_OUT_TIME)
			{
				U8 command = JL_NET_JUMP_IN;
				U8 buffer[100];
				buffer[0] = command;
				owner->LauncherSendOpData(workingID,buffer,sizeof(U8));

				jumpMode = JUMP_LAUNCHER_SAVELOAD::JUMPING_IN;
				timer = 0;
				OBJPTR<IPhysicalObject> phys;
				owner.Ptr()->QueryInterface(IPhysicalObjectID,phys);
				phys->SetPosition(targetPos,owner.Ptr()->GetSystemID());

			}
		}
	}
	else if(jumpMode == JUMP_LAUNCHER_SAVELOAD::JUMPING_IN)
	{
		if(THEMATRIX->IsMaster())
		{
			if(timer > JUMP_IN_TIME)
			{
				jumpMode = JUMP_LAUNCHER_SAVELOAD::NO_JUMP;
				owner->LaunchOpCompleted(this,workingID);
				workingID = 0;
				owner.Ptr()->bSpecialRender = false;

				USR_PACKET<USRMOVE> packet;
				packet.objectID[0] = owner.Ptr()->GetPartID();
				packet.position.init(targetPos,owner.Ptr()->GetSystemID());
				packet.init(1);
				NETPACKET->Send(HOSTID,0,&packet);
			}
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
//
void JumpLauncher::PhysicalUpdate (SINGLE dt)
{
	if(jumpMode != JUMP_LAUNCHER_SAVELOAD::NO_JUMP)
	{
		timer += dt;
		SINGLE progress = 1.0;
		if(jumpMode == JUMP_LAUNCHER_SAVELOAD::JUMPING_OUT)
		{
			progress = (timer/JUMP_OUT_TIME);
			progress = __min(1.0,progress);
		}
		else //jumping in
		{
			progress = 1.0-(timer/JUMP_OUT_TIME);
			progress = __max(0,progress);
		}
		ring.PhysUpdate(progress,dt);
	}
}
//---------------------------------------------------------------------------
//
void JumpLauncher::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID, owner, LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);

	VOLPTR(IExtent) extentObj = owner;
	CQASSERT(extentObj);

//	mc = &extentObj->GetMeshChain();
	float box[6];
	extentObj.Ptr()->GetObjectBox(box);
	shipLength = box[BBOX_MAX_Z]-box[BBOX_MIN_Z];
}
//---------------------------------------------------------------------------
//
void JumpLauncher::InformOfCancel()
{
}
//---------------------------------------------------------------------------
//
void JumpLauncher::LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize)
{
	if(owner)
		owner.Ptr()->bSpecialRender = true;
	workingID = agentID;
	targetPos = *((GRIDVECTOR*)buffer);
	jumpMode = JUMP_LAUNCHER_SAVELOAD::JUMPING_OUT;
	timer = 0;
	ring.Init(50,((JumpLauncherArchetype *)(ARCHLIST->GetArchetypeHandle(pArchetype)))->jumpRingAnim,255,255,255,owner.Ptr()->GetSystemID());
}
//---------------------------------------------------------------------------
//
void JumpLauncher::LauncherReceiveOpData(U32 workingID, void * buffer, U32 bufferSize)
{
	U8 command = *((U8*)buffer);
	if(command == JL_NET_JUMP_IN)
	{
		jumpMode = JUMP_LAUNCHER_SAVELOAD::JUMPING_IN;
		timer = 0;
		OBJPTR<IPhysicalObject> phys;
		owner.Ptr()->QueryInterface(IPhysicalObjectID,phys);
		phys->SetPosition(targetPos,owner.Ptr()->GetSystemID());
	}
}
//---------------------------------------------------------------------------
//
void JumpLauncher::LauncherOpCompleted(U32 agentID)
{
	if(owner)
		owner.Ptr()->bSpecialRender = false;
	jumpMode = JUMP_LAUNCHER_SAVELOAD::NO_JUMP;
	workingID = 0;
}
//---------------------------------------------------------------------------
//
void JumpLauncher::AttackPosition (const struct GRIDVECTOR * position, bool bSpecial)
{
	if(owner)
		owner->LauncherCancelAttack();//force the completion of the attack operation.
	if(THEMATRIX->IsMaster())
	{
		if(owner->UseSupplies(pData->supplyCost,true))
		{
			owner.Ptr()->bSpecialRender = true;
			targetPos = *position;
			jumpMode = JUMP_LAUNCHER_SAVELOAD::JUMPING_OUT;
			ring.Init(50,((JumpLauncherArchetype *)(ARCHLIST->GetArchetypeHandle(pArchetype)))->jumpRingAnim,255,255,255,owner.Ptr()->GetSystemID());

			timer = 0;

			ObjSet set;
			set.objectIDs[0] = owner.Ptr()->GetPartID();
			set.numObjects = 1;
			workingID = owner->CreateLauncherOp(this,set,&targetPos,sizeof(GRIDVECTOR));
			THEMATRIX->SetCancelState(workingID,false);
		}
	}
}
//---------------------------------------------------------------------------
//
void JumpLauncher::AttackObject (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
const TRANSFORM & JumpLauncher::GetTransform (void) const
{
	return owner.Ptr()->GetTransform();
}
//---------------------------------------------------------------------------
//
bool JumpLauncher::checkSupplies()
{
	MPart part = owner.Ptr();

	return pData->supplyCost <= part->supplies;
}
//---------------------------------------------------------------------------
//
inline IBaseObject * createJumpLauncher (PARCHETYPE pArchetype)
{
	JumpLauncher * jumpLauncher = new ObjectImpl<JumpLauncher>;

	jumpLauncher->init(pArchetype);

	return jumpLauncher;
}
//------------------------------------------------------------------------------------------
//---------------------------JumpLauncher Factory------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE JumpLauncherFactory : public IObjectFactory
{
	struct OBJTYPE : JumpLauncherArchetype
	{
		PARCHETYPE pArchetype;

		void * operator new (size_t size)
		{
			return calloc(size,1);
		}

		OBJTYPE (void)
		{
		}

		~OBJTYPE (void)
		{
			if(jumpRingAnim)
			{
				delete jumpRingAnim;
				jumpRingAnim = NULL;
			}
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(JumpLauncherFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	JumpLauncherFactory (void) { }

	~JumpLauncherFactory (void);

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

	/* JumpLauncherFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
JumpLauncherFactory::~JumpLauncherFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void JumpLauncherFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE JumpLauncherFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_LAUNCHER)
	{
		BT_JUMP_LAUNCHER * data = (BT_JUMP_LAUNCHER *) _data;

		if (data->type == LC_JUMP_LAUNCH)	   
		{
			result = new OBJTYPE;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);

			if(data->jumpParticle[0])
			{
				DAFILEDESC fdesc = data->jumpParticle;
				
				fdesc.lpImplementation = "UTF";

				COMPTR<IFileSystem> file;
				
				if (OBJECTDIR->CreateInstance(&fdesc, file) == GR_OK)
				{
					 result->jumpRingAnim = ANIM2D->create_archetype(file);
				}
			}
		}
	}

	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 JumpLauncherFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		// not supported
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * JumpLauncherFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createJumpLauncher(objtype->pArchetype);
}
//-------------------------------------------------------------------
//
void JumpLauncherFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _jumpLauncher : GlobalComponent
{
	JumpLauncherFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<JumpLauncherFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _jumpLauncher __jumpLauncher;

//---------------------------------------------------------------------------------------------
//-------------------------------End JumpLauncher.cpp------------------------------------------------
//---------------------------------------------------------------------------------------------