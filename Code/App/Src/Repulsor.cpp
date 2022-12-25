//--------------------------------------------------------------------------//
//                                                                          //
//                                 Repulsor.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Repulsor.cpp 48    9/13/01 10:01a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "Camera.h"
#include "Objlist.h"
#include "sfx.h"
#include "IConnection.h"
#include "Startup.h"
#include "SuperTrans.h"
#include "DSpecial.h"
#include "ILauncher.h"
#include "IWeapon.h"
#include "CQLight.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "Mission.h"
#include "MGlobals.h"
#include "IMissionActor.h"
#include "Anim2d.h"
#include "UserDefaults.h"
#include "IBlast.h"
#include "MPart.h"
#include "IShipMove.h"

#include <TComponent.h>
#include <lightman.h>
#include <Engine.h>
#include <Vector.h>
#include <Matrix.h>
#include <IHardpoint.h>
#include <stdlib.h>
#include <FileSys.h>
#include <ICamera.h>
#include <Pixel.h>
//#include <RPUL\PrimitiveBuilder.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
struct RepulsorArchetype
{
	const char *name;
	BT_REPULSOR_DATA *data;
	PARCHETYPE pArchetype;
	PARCHETYPE pContactBlast;
	AnimArchetype *animArch;

	RepulsorArchetype (void)
	{
		memset(this, 0, sizeof(*this));
	}

	~RepulsorArchetype (void)
	{
		if (animArch)
			delete animArch;
		if (pContactBlast)
			ARCHLIST->Release(pContactBlast, OBJREFNAME);
	}
};


struct _NO_VTABLE Repulsor : IBaseObject, ISaveLoad, ILauncher, REPULSOR_SAVELOAD
{
	BEGIN_MAP_INBOUND(Repulsor)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ILauncher)
	_INTERFACE_ENTRY(ISaveLoad)
	END_MAP()

	//------------------------------------------
	unsigned int textureID;
	RepulsorArchetype *arch;
	BT_REPULSOR_DATA *data;
	OBJPTR<IShipMove> target;
	OBJPTR<ILaunchOwner> owner;
	OBJPTR<IBlast> contactBlast;
	HSOUND hSound;

	SINGLE timer;

	//----------------------------------
	// hardpoint data
	//----------------------------------
	HardpointInfo  hardpointinfo;
	INSTANCE_INDEX barrelIndex;
	//------------------------------------------

	Repulsor (void);
	virtual ~Repulsor (void);	// See ObjList.cpp

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

//	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual U32 GetPartID (void) const
	{
		return ownerID;
	}

	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	/* IWeapon methods */

//	virtual void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * obj, U32 flags, const class Vector * pos=0);

	// IRecon

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial) {}

	virtual void AttackObject (IBaseObject * obj) {}

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID) {}

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void DoCreateWormhole(U32 systemID)
	{}

	virtual const bool TestFightersRetracted (void) const {return true;}   // return true if fighters are retracted

	virtual void SetFighterStance(FighterStance stance)
	{
	}

	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID)
	{
		target = 0;
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const {return 0;}

	virtual U32 GetSyncData (void * buffer) {return 0;}			// buffer points to use supplied memory

	virtual void PutSyncData (void * buffer, U32 bufferSize) {}

	virtual void DoSpecialAbility (U32 specialID) {}

	virtual void DoSpecialAbility (IBaseObject *obj) {}

	virtual void DoCloak (void)
	{
	}
	
	virtual void SpecialAttackObject (IBaseObject * obj);

	/* what special attack (if any) does this unit have, and can we use it?  Variables get overwritten */

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bEnabled)
	{
		MPartNC part(owner.Ptr());
		ability = USA_REPULSOR;
		bEnabled = part->caps.specialAttackShipOk;
	} 

	virtual void InformOfCancel() {};
	
	virtual void LauncherOpCreated(U32 agentID, void * buffer, U32 bufferSize) {};

	virtual void LauncherReceiveOpData(U32 agentID, void * buffer, U32 bufferSize) {};

	virtual void LauncherOpCompleted(U32 agentID) {};

	virtual bool CanCloak(){return false;};

	virtual bool IsToggle() {return false;};

	virtual bool CanToggle(){return false;};

	virtual bool IsOn() {return false;};

	virtual void OnAllianceChange (U32 allyMask)
	{
	}

	virtual IBaseObject * FindChildTarget(U32 childID){return NULL;};

	void init (RepulsorArchetype * data);

	/* ISaveLoad */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations();

	virtual const U32 GetApproxDamagePerSecond (void) const
	{ return 0; }
};

//----------------------------------------------------------------------------------
//
Repulsor::Repulsor (void) 
{
	time = 30;
}

//----------------------------------------------------------------------------------
//
Repulsor::~Repulsor (void)
{
	textureID = 0;
	if (target)
		target->ReleaseShipControl(ownerID);

	delete contactBlast.Ptr();
}

//---------------------------------------------------------------------------
//
BOOL32 Repulsor::Save (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "REPULSOR_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	REPULSOR_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	if (target==0)
		targetID = 0;
	memset(&save, 0, sizeof(save));
	memcpy(&save, static_cast<REPULSOR_SAVELOAD *>(this), sizeof(REPULSOR_SAVELOAD));

	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Repulsor::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "REPULSOR_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	REPULSOR_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("REPULSOR_SAVELOAD", buffer, &load);

	*static_cast<REPULSOR_SAVELOAD *>(this) = load;

	result = 1;

Done:	
	return result;
}

//---------------------------------------------------------------------------
//

void Repulsor::ResolveAssociations()
{
	if (targetID)
	{
		IBaseObject* targetObject = OBJLIST->FindObject(targetID);
		CQASSERT(targetObject!=0);

		if (targetObject) 
			targetObject->QueryInterface(IShipMoveID, target, SYSVOLATILEPTR);
		CQASSERT(target!=0);
	}

	if (ownerID)
	{
		IBaseObject *ownerObject = OBJLIST->FindObject(ownerID);
		CQASSERT(ownerObject!=0);

		if (ownerObject)
			ownerObject->QueryInterface(ILaunchOwnerID,owner,LAUNCHVOLATILEPTR);
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 Repulsor::Update (void)
{
	BOOL32 result=1;
	MPartNC part(owner.Ptr());
	if(part.isValid())
	{
		if(!(part->caps.specialAttackShipOk))
		{
			BT_REPULSOR_DATA * data = (BT_REPULSOR_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
			if(data->neededTech.raceID)
			{
				if(MGlobals::GetCurrentTechLevel(owner.Ptr()->GetPlayerID()).HasTech(data->neededTech))
				{
					part->caps.specialAttackShipOk = true;
				}
			}
			else
			{
				part->caps.specialAttackShipOk = true;
			}
		}
	}
	if (target)
	{
		if (owner==0 || target==0)
			return 0;
		
		if (mass > data->minimumMass && (owner.Ptr()->GetSystemID() == target.Ptr()->GetSystemID()))
		{
			
			Vector dir = owner.Ptr()->GetPosition()-target.Ptr()->GetPosition();//pushPos;
																		/*	if (dir.magnitude() > 5000)
																		{
																		dir.normalize();
																		dir *= 5000;
		}*/
			
			dir.z = 0;
			if (fabs(dir.x) < 1e-3 && fabs(dir.y) < 1e-3)
			{
				dir.set(1,0,0);
			}
			else
				dir.normalize();
			
			SINGLE speed = (data->basePushPower+data->pushPerMass*mass);
		//	ENGINE->set_velocity(target.ptr->GetObjectIndex(),vel);
			target->PushShip(ownerID,-dir,speed);
		}

		if (time-- == 0)
		{
			if (target)
				target->ReleaseShipControl(ownerID);
			target = 0;
			if (contactBlast)
			{
				delete contactBlast.Ptr();
				contactBlast = 0;
			}
		}
	}

	if (contactBlast)
	{
		contactBlast.Ptr()->Update();
	}

	return result;
}

void Repulsor::PhysicalUpdate(SINGLE dt)
{
//	offset -= dt*20;
}
//----------------------------------------------------------------------------------
//
/*void Repulsor::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	IBaseObject *owner = OBJLIST->FindObject(ownerID);
	
	bVisible = 0;

	if (owner && target!=0)
	{
		bVisible = (GetSystemID() == currentSystem &&
			   (IsVisibleToPlayer(currentPlayer) ||
			   (target.ptr->IsVisibleToPlayer(currentPlayer)) ||
			     defaults.bVisibilityRulesOff ||
			     defaults.bEditorMode) );
	}
}*/
//----------------------------------------------------------------------------------
//
void Repulsor::Render (void)
{
	if (target == 0 || owner==0)
		return;

	if (target.Ptr()->GetSystemID() != owner.Ptr()->GetSystemID())
		return;

	SINGLE dt = OBJLIST->GetRealRenderTime();
	timer += dt;
	timer = fmod(timer,1.0f);
	CAMERA->SetModelView();
	SINGLE test = (SINGLE)rand()/RAND_MAX;
	
	//visibility code
	//Vector epos (ENGINE->get_position (instance));
	#define TOLERANCE 0.00001f

	Vector v0,v1,v2,v3;
	Vector cpos = MAINCAM->get_position();
	
#define BM_WDTH 200
#define SEG_LENGTH (16*BM_WDTH)

	int width = BM_WDTH;
	width += (U8)(12*test);
//	SINGLE mag,mag2;

	Vector start = owner.Ptr()->GetPosition();
	if (barrelIndex != -1)
	{
		start = ENGINE->get_transform(barrelIndex)*hardpointinfo.point;
	}
	Vector end = target.Ptr()->GetPosition();
	SINGLE length = (end-start).magnitude();
	Vector direction = (end-start)/length;
//	int numSegments = 0.5*length/SEG_LENGTH;

	Vector look (cpos - start);
	//The following code is all to keep beams in the distance looking good.
/*	if ((mag = look.magnitude()) > 7000)
	{
		look = (cpos - end);
		if ((mag2 = look.magnitude()) > 7000)
		{
			if (mag > mag2)
			{
				mag = mag2;
			}
			else
				look = cpos-start;
			look.normalize();
			mag *= 1.0f - 0.55f*(SINGLE)fabs(dot_product(look,direction));
			if (mag > 3000)
				width = BM_WDTH+(U8)((mag-3000)/300);
		}
	}*/

	//this is the draw code that makes the beam always visible
	
	Vector i;// (look.y, -look.x, 0);
	i = cross_product(look,direction);

	if (fabs (i.x) < TOLERANCE && fabs (i.y) < TOLERANCE)
	{
		i.x = 1.0f;
	}
	
	i.normalize ();
	
	BATCH->set_state(RPR_BATCH,TRUE);
	CAMERA->SetModelView();
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);

//	BATCH->set_texture_stage_texture(0,animArch->frames[0].texture);
	SetupDiffuseBlend(arch->animArch->frames[0].texture,FALSE);
	BATCH->set_state(RPR_STATE_ID,arch->animArch->frames[0].texture);

	PB.Color3ub(255,255,255);	
	PB.Begin(PB_QUADS);


//	SINGLE step = 1.0/numSegments;
		
	v0 = start - (i *width);
	v1 = start + (i *width);
	v2 = end + (i * width);
	v3 = end - (i * width);

	AnimFrame *frame = &arch->animArch->frames[F2LONG(timer*8)%4];
		
	//stupid correction factor here to eliminate seams  !!!clamp???
	PB.TexCoord2f(-5*timer,frame->y0);     PB.Vertex3f(v0.x,v0.y,v0.z);
	PB.TexCoord2f(-5*timer,frame->y1);		PB.Vertex3f(v1.x,v1.y,v1.z);
	PB.TexCoord2f(length/SEG_LENGTH-5*timer,frame->y1);		PB.Vertex3f(v2.x,v2.y,v2.z);
	PB.TexCoord2f(length/SEG_LENGTH-5*timer,frame->y0);		PB.Vertex3f(v3.x,v3.y,v3.z);

	PB.End();  //GL_QUADS

	BATCH->set_state(RPR_STATE_ID,0);

	if (contactBlast)
	{
		Vector j = cross_product(i,-direction);
		Transform t;
		t.set_i(i);
		t.set_j(j);
		t.set_k(-direction);
		t.translation = end;
		contactBlast.Ptr()->PhysicalUpdate(dt);
		contactBlast->SetRelativeTransform(t);
		contactBlast.Ptr()->Render();
	}

}
//----------------------------------------------------------------------------------
//
void Repulsor::SpecialAttackObject(IBaseObject *obj)
{
	if (obj)
	{
		time = data->pushTime*REALTIME_FRAMERATE;
		systemID = owner.Ptr()->GetSystemID();
		U32 targetSystemID = obj->GetSystemID();
		CQASSERT(systemID == targetSystemID);
		
		obj->QueryInterface(IShipMoveID, target, SYSVOLATILEPTR);
		CQASSERT(target!=0);
		
		U32 visFlags = owner.Ptr()->GetVisibilityFlags();
		targetID = target.Ptr()->GetPartID();
		visFlags |= obj->GetVisibilityFlags();
		owner.Ptr()->SetVisibleToAllies(1 << (obj->GetPlayerID()-1));
		
		SetVisibilityFlags(visFlags);
		
		Vector pos = obj->GetPosition();
		
		if (hSound==0)
			hSound = SFXMANAGER->Open(data->launchSfx);
		SFXMANAGER->Play(hSound,systemID,&pos);
		
		mass = data->minimumMass*2+200;
		
		IBaseObject * b_obj;
		b_obj = ARCHLIST->CreateInstance(arch->pContactBlast);
		if (b_obj)
		{
			if (contactBlast)
			{
				delete contactBlast.Ptr();
				contactBlast = 0;
			}

			b_obj->QueryInterface(IBlastID,contactBlast,NONSYSVOLATILEPTR);
			CQASSERT(contactBlast && "Not a blast!!");
			TRANSFORM ownerTrans = owner.Ptr()->GetTransform();
			//!!!! might want to set the orientation here someday
			TRANSFORM trans;
			trans.translation = target.Ptr()->GetPosition();
			contactBlast->InitBlast(trans,owner.Ptr()->GetSystemID(),0);
			contactBlast->SetVisible(true);
		}
	}
}

void Repulsor::InitLauncher(IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range)
{
	_owner->QueryInterface(ILaunchOwnerID,owner,LAUNCHVOLATILEPTR);

	ownerID = owner.Ptr()->GetPartID();

	barrelIndex = -1;
	if (data->hardpoint[0])
	{
		FindHardpoint(data->hardpoint, barrelIndex, hardpointinfo, ownerIndex);
		if (barrelIndex == -1)
			CQERROR1("Hardpoint '%s' not found!", data->hardpoint);
	}
}
//---------------------------------------------------------------------------
//
void Repulsor::init (RepulsorArchetype * _arch)
{
	arch = _arch;
	CQASSERT(arch->data->wpnClass == WPN_REPULSOR);
	CQASSERT(arch->data->objClass == OC_WEAPON);

	data = (BT_REPULSOR_DATA *)arch->data;

	pArchetype = arch->pArchetype;
	objClass = OC_WEAPON;
//	animArch = arch->animArch;
}
//---------------------------------------------------------------------------
//
static Repulsor * CreateRepulsor (RepulsorArchetype * data)
{
	Repulsor * beam = new ObjectImpl<Repulsor>;

//	beam->FRAME_init(data);
	beam->init(data);

	return beam;
}
//----------------------------------------------------------------------------------------------
//-------------------------------class RepulsorManager--------------------------------------------
//----------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE RepulsorManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(RepulsorManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	U32 factoryHandle;

	//child object info
	RepulsorArchetype *pArchetype;

	//RepulsorManager methods

	RepulsorManager (void) 
	{
	}

	~RepulsorManager();
	
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	IDAComponent * GetBase (void)
	{
		return (IObjectFactory *) this;
	}

	void init();

	//IObjectFactory
	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);
};


//--------------------------------------------------------------------------
// RepulsorManager methods

RepulsorManager::~RepulsorManager()
{
	COMPTR<IDAConnectionPoint> connection;
	if (OBJLIST)
	{
		if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
			connection->Unadvise(factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
void RepulsorManager::init()
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(GetBase(), &factoryHandle);
}
//--------------------------------------------------------------------------
//
HANDLE RepulsorManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *_data)
{
	RepulsorArchetype *newguy=0;
	if (objClass == OC_WEAPON)
	{
		BT_REPULSOR_DATA * data = (BT_REPULSOR_DATA *)_data;
		if (data->wpnClass == WPN_REPULSOR)
		{
			
			newguy = new RepulsorArchetype;
			
			newguy->name = szArchname;
			newguy->data = data;
			newguy->pArchetype = ARCHLIST->GetArchetype(szArchname);
				
			COMPTR<IFileSystem> file;
			DAFILEDESC fdesc = data->fileName;
			
			fdesc.lpImplementation = "UTF";
			
			
			if (OBJECTDIR->CreateInstance(&fdesc, file) != GR_OK)
			{
				CQERROR1("Failed to open file %s", fdesc.lpFileName);
				goto Error;
			}
			
			if ((newguy->animArch = ANIM2D->create_archetype(file)) == 0)
			{
				CQERROR0("Failed to create repulsor archetype");
				goto Error;
			}
			
			if (data->contactBlastType[0])
			{
				newguy->pContactBlast = ARCHLIST->LoadArchetype(data->contactBlastType);
				CQASSERT(newguy->pContactBlast);
				ARCHLIST->AddRef(newguy->pContactBlast, OBJREFNAME);
			}

			//
			// force preload of sound effect
			// 
			SFXMANAGER->Preload(data->launchSfx);
		
			goto Done;
		}
	}

Error:
	if (newguy)
		delete newguy;
	newguy = 0;

Done:
	return newguy;
}
//--------------------------------------------------------------------------
//
BOOL32 RepulsorManager::DestroyArchetype(HANDLE hArchetype)
{
	RepulsorArchetype *pWeapon = (RepulsorArchetype *)hArchetype;
	BT_REPULSOR_DATA *objData = pWeapon->data;
	
	if (objData->objClass == OC_WEAPON)
	{
//		IBaseObject * obj = NULL;
	/*	switch (objData->wpnClass)
		{
		case WPN_ARC:
			ReleaseArcCannonArchetype();
			break;
		}*/
	}
	RepulsorArchetype *deadguy = (RepulsorArchetype *)hArchetype;
	delete deadguy;

	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * RepulsorManager::CreateInstance(HANDLE hArchetype)
{
	RepulsorArchetype *pWeapon = (RepulsorArchetype *)hArchetype;
	BT_REPULSOR_DATA *objData = pWeapon->data;
//	INSTANCE_INDEX index=-1;
	
	if (objData->objClass == OC_WEAPON)
	{
		IBaseObject * obj = NULL;
		switch (objData->wpnClass)
		{
		case WPN_REPULSOR:
			{
				obj = CreateRepulsor(pWeapon);
			}
			break;
		}

		return obj;
	}
	
	return 0;
}
//--------------------------------------------------------------------------
//
void RepulsorManager::EditorCreateInstance(HANDLE hArchetype,const MISSION_INFO & info)
{
	// not supported
}

static struct RepulsorManager *REPULSORMGR;
//----------------------------------------------------------------------------------------------
//
struct _repulsors : GlobalComponent
{
	virtual void Startup (void)
	{
		struct RepulsorManager *repulsorMgr = new DAComponent<RepulsorManager>;
		REPULSORMGR = repulsorMgr;
		AddToGlobalCleanupList((IDAComponent **) &REPULSORMGR);
	}

	virtual void Initialize (void)
	{
		REPULSORMGR->init();
	}
};

static _repulsors repulsors;

//--------------------------------------------------------------------------//
//------------------------------END Repulsor.cpp--------------------------------//
//--------------------------------------------------------------------------//
