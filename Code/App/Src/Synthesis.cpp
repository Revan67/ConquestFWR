//--------------------------------------------------------------------------//
//                                                                          //
//                                 Synthesis.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Synthesis.cpp 32    9/13/01 10:01a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "IConnection.h"
#include "Camera.h"
#include "DEffect.h"
#include "Objlist.h"
#include "Sfx.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "TObjPhys.h"
#include "TObjControl.h"
#include "Startup.h"
#include "IWeapon.h"
#include "IFighter.h"
#include "ILauncher.h"
#include "Mission.h"
#include "ICloak.h"
#include "ArchHolder.h"
#include "MGlobals.h"
#include "MPart.h"
#include "Sector.h"
#include "CQLight.h"
#include "Anim2d.h"
#include "IShipDamage.h"
#include "SimpleMesh.h"
#include "CQBatch.h"
#include "MeshRender.h"
#include "Opagent.h"
#include "IBanker.h"
#include "IShipMove.h"

#include <IHardpoint.h>
#include <DSpecial.h>
#include <DMBaseData.h>
#include <TSmartPointer.h>
#include <Mesh.h>
#include <FileSys.h>
#include <Engine.h>
#include <IRenderPrimitive.h>
#include <Renderer.h>
#include <IAnim.h>


struct SynthesisArchetype
{
	const char *name;
	BT_SYNTHESIS_DATA *data;
	S32 archIndex;
	IMeshArchetype * meshArch;
	//PARCHETYPE pImpactBlast;
	Vector rigidBodyArm;
	AnimArchetype *animArch;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	SynthesisArchetype (void)
	{
		meshArch = NULL;
		archIndex = INVALID_INSTANCE_INDEX;
	}

	~SynthesisArchetype (void)
	{
		ENGINE->release_archetype(archIndex);
		delete animArch;
//		if (pImpactBlast)
//			ARCHLIST->Release(pImpactBlast);
	}

};


U16 zapIndices[24] = {1,0,3, 1,3,4,
					  2,1,4, 2,4,5,
					  4,3,6, 4,6,7,
					  5,4,8, 4,7,8};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE Synthesis : public ObjectPhysics<ObjectTransform<ObjectFrame<IBaseObject,SYNTHESIS_SAVELOAD,SynthesisArchetype> > >,  ISaveLoad, ILauncher, BASE_SYNTHESIS_SAVELOAD
{

	BEGIN_MAP_INBOUND(Synthesis)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(ISaveLoad)
	_INTERFACE_ENTRY(ILauncher)
	END_MAP()

	//------------------------------------------

	BT_SYNTHESIS_DATA *data;
	OBJPTR<IWeaponTarget> target;
	SynthesisArchetype *arch;
	U32 systemID;

	OBJPTR<ILaunchOwner> owner;
	MeshChain *mc;

	U32 dwMissionID;
	AnimInstance anim;
	SINGLE shipLength;

	SINGLE timer;

	bool bActive:1;

	//----------------------------------
	// hardpoint data
	//----------------------------------
	HardpointInfo  hardpointinfo;
	INSTANCE_INDEX barrelIndex;
	//------------------------------------------

	//
	// sync info
	// 
	struct SYNC_PACKET
	{
		U32 targetID;
	};

	Synthesis (void)
	{
		barrelIndex = instanceIndex = INVALID_INSTANCE_INDEX;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~Synthesis (void);	// See ObjList.cpp

	/* IBaseObject methods */

	virtual void CastVisibleArea (void);

	virtual void PhysicalUpdate (SINGLE dt);
	
	virtual BOOL32 Update ();
	
	virtual void Render (void);

	//---------------------------------------------------------------------------
	//
	virtual U32 GetPartID (void) const
	{
		if (owner)
		{
			return owner.Ptr()->GetPartID();
		}
		else
			return dwMissionID;
	}

	virtual U32 GetPlayerID () const
	{
		return MGlobals::GetPlayerFromPartID(dwMissionID);
	}

	virtual U32 GetSystemID() const
	{
		return systemID;
	}
	
	// ILauncher

	virtual void InitLauncher (IBaseObject * owner, S32 ownerIndex, S32 animArcheIndex, SINGLE range);

	virtual void AttackPosition (const struct GRIDVECTOR * position, bool bSpecial);

	virtual void AttackObject (IBaseObject * obj);

	virtual void AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID);

	virtual void WormAttack (IBaseObject * obj)
	{};

	virtual void DoCreateWormhole(U32 systemID)
	{}

	virtual const bool TestFightersRetracted (void) const; // return true if fighters are retracted

	virtual void SetFighterStance(FighterStance stance)
	{
	}

	virtual void HandlePreTakeover (U32 newMissionID, U32 troopID)
	{
		CQASSERT(bActive == 0 && "See Rob");
	}

	virtual void TakeoverSwitchID (U32 newMissionID)
	{
	}

	// the following methods are for network synchronization of realtime objects
	virtual U32 GetSyncDataSize (void) const;

	virtual U32 GetSyncData (void * buffer);			// buffer points to use supplied memory

	virtual void PutSyncData (void * buffer, U32 bufferSize);

	virtual void DoSpecialAbility (U32 specialID);

	virtual void DoSpecialAbility (IBaseObject * obj);

	virtual void DoCloak (void)
	{
	}
	
	virtual void SpecialAttackObject (IBaseObject * obj);

	virtual void  GetSpecialAbility (UNIT_SPECIAL_ABILITY & ability, bool & bEnabled)
	{
		MPart part(owner.Ptr());
		ability = USA_SYNTHESIS;
		bEnabled = part->caps.synthesisOk;
	}

	virtual const U32 GetApproxDamagePerSecond (void) const
	{
		return 0;
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

	/* ISaveLoad methods */

	virtual BOOL32 Save (struct IFileSystem * file);

	virtual BOOL32 Load (struct IFileSystem * file);

	virtual void ResolveAssociations ();

	// Synthesis methods

//	bool checkSupplies ();

	void init (SynthesisArchetype *arch);

	void renderZap ();

	void renderShield ();

	void shootTarget ();
};

//----------------------------------------------------------------------------------
//
Synthesis::~Synthesis (void)
{
	ENGINE->destroy_instance(instanceIndex);
	if (bActive)
		delete owner.Ptr();
}
//---------------------------------------------------------------------------------------
//
void Synthesis::CastVisibleArea (void)
{
	if (bActive)
	{
		// propogate visibility
		SetVisibleToAllies(GetVisibilityFlags());
	}
}
//----------------------------------------------------------------------------------
//
void Synthesis::PhysicalUpdate (SINGLE dt)
{
	if (bActive && target)
	{
		
		if (instanceIndex != INVALID_INSTANCE_INDEX)
		{
			ENGINE->update_instance(instanceIndex,0,dt);
			ANIM->update_instance(instanceIndex,dt);
		}
		FRAME_physicalUpdate(dt);
		if (stage == SYN_ZAP || stage == SYN_ABSORB)
			timer += dt;
		
#define ZAP_TIME 1.0f
		
		if (stage == SYN_ZAP)
		{
			SINGLE progress = min(timer/ZAP_TIME,1.0f);
			if (progress == 1.0f)
				stage = SYN_APPROACH;
		}

		if (stage == SYN_ROTATE)
			owner.Ptr()->PhysicalUpdate(dt);
	}

}

BOOL32 Synthesis::Update ()
{
	MPartNC part(owner.Ptr());
	if(part.isValid())
	{
		if(!(part->caps.synthesisOk))
		{
			BT_SYNTHESIS_DATA * data = (BT_SYNTHESIS_DATA *)(ARCHLIST->GetArchetypeData(pArchetype));
			if(data->neededTech.raceID)
			{
				if(MGlobals::GetCurrentTechLevel(owner.Ptr()->GetPlayerID()).HasTech(data->neededTech))
				{
					part->caps.synthesisOk = true;
				}
			}
			else
			{
				part->caps.synthesisOk = true;
			}
		}
	}
	if (bActive)
	{
		if (!target)
		{
			if (owner)
			{
				//owner->LauncherCancelAttack();
				if(THEMATRIX->IsMaster())
				{
					MPart part(owner.Ptr());
					if(part.isValid() && part->bUnderCommand)
					{
						BANKER->FreeCommandPt(owner.Ptr()->GetPlayerID(),part.pInit->resourceCost.commandPt);
					}
				}
				delete owner.Ptr();
				owner = 0;
			}
			return 0;
		}
		
		Vector targetPos = target.Ptr()->GetPosition();
		Vector diff = targetPos-transform.translation;

		if (stage == SYN_ROTATE)
		{
			owner.Ptr()->Update();
			transform = owner.Ptr()->GetTransform();
			SINGLE yaw = transform.get_yaw();
			SINGLE relYaw = get_angle(diff.x, diff.y) - yaw;
			
			if (relYaw < -PI)
				relYaw += PI*2;
			else if (relYaw > PI)
				relYaw -= PI*2;
			VOLPTR(IExplosionOwner) eo=owner.Ptr();
			CQASSERT(eo);
			eo->RotateShip(relYaw, 0,0,0);
		//	ang_velocity.set(0,0,-4*relYaw);

			if (fabs(relYaw) < 0.1)
			{
				//danger to debuggers!
				Transform trans = owner.Ptr()->GetTransform();

				ENGINE->set_instance_handler(owner.Ptr()->GetObjectIndex(),this);
				ENGINE->set_transform(instanceIndex,trans);
				ang_velocity.set(0,0,0);
				stage = SYN_ZAP;
				timer = 0;
			}
		} 
		else if (stage == SYN_APPROACH)
		{
			SINGLE mag;
			Vector pt,dir;
			if (target->GetCollisionPosition(pt,dir,transform.translation,diff))
			{
				diff = pt-transform.translation;
				mag = diff.magnitude();
#define SPEED 1500
				if (mag < SPEED*(0.3f+ELAPSED_TIME))
				{
					stage = SYN_ABSORB;
					timer = 0;
				//	return 0;
				}
			}
			else
				mag = diff.magnitude();
			
			velocity = diff*SPEED/mag;
		}

	}

	FRAME_update();

	#define ABSORB_TIME 1.0f
	if (stage == SYN_ABSORB)
	{
		SINGLE progress = min(timer/ABSORB_TIME,1.0f);
		if (progress == 1.0f)
		{
			//owner->LauncherCancelAttack();
			if(THEMATRIX->IsMaster())
			{
				MPart part(owner.Ptr());
				if(part.isValid() && part->bUnderCommand)
				{
					BANKER->FreeCommandPt(owner.Ptr()->GetPlayerID(),part.pInit->resourceCost.commandPt);
				}
			}
			delete owner.Ptr();
			owner = 0;
			MPartNC targetPart(target.Ptr());
			//finish
			targetPart->hullPoints = min(targetPart->hullPointsMax,targetPart->hullPoints+(hullPointsPer*targetPart->hullPointsMax));
			targetPart->supplies = min(targetPart->supplyPointsMax,targetPart->supplies+(suppliesPer*targetPart->supplyPointsMax));
			return 0;
		}
	}

	return 1;
}
//----------------------------------------------------------------------------------
//
void Synthesis::Render (void)
{
	if (!target || !owner)
		return;

	if (bActive && systemID == SECTOR->GetCurrentSystem() && bVisible)
	{
		BATCH->set_state(RPR_BATCH,TRUE);
		LIGHTS->ActivateBestLights(transform.translation,8,1000);
		//ENGINE->render_instance(MAINCAM,instanceIndex, 0, LODPERCENT, 0, 0);

		if (stage == SYN_ZAP || stage == SYN_APPROACH)
		{	
			SINGLE dt = OBJLIST->GetRealRenderTime();
			anim.update(dt);
			renderZap();
		}
		else if (stage == SYN_ABSORB)
			renderShield();

		if (stage == SYN_ABSORB)
		{
			SINGLE progress = min(timer/ABSORB_TIME,1.0f);
			Vector split_z = Vector(0,0,-shipLength/2+shipLength*progress);
			split_z = transform*split_z;
			TreeRenderPortionZAlign(mc->mi,mc->numChildren,split_z);
		}
		else
			TreeRender(mc->mi,mc->numChildren);
	}
}
//---------------------------------------------------------------------------
//
void Synthesis::renderZap()
{
	if (!target || !owner)
		return;

	CAMERA->SetModelView();
	
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
	//DisableTextures();

	const AnimFrame *frame = anim.retrieve_current_frame();
	SetupDiffuseBlend(frame->texture,FALSE);
	BATCH->set_state(RPR_STATE_ID,frame->texture);

	BATCHDESC desc;
	desc.type = D3DPT_TRIANGLELIST;
	desc.vertex_format = D3DFVF_RPVERTEX;
	desc.num_verts = 9;
	desc.num_indices = 24;
	CQBATCH->GetPrimBuffer(&desc);
	desc.num_verts = 6;
	desc.num_indices = 12;
	RPVertex *v_list = (RPVertex *)desc.verts;



	Vector start,end;
	if (barrelIndex != -1)
	{
		start = ENGINE->get_transform(barrelIndex)*hardpointinfo.point;
	}
	else
		start = ENGINE->get_transform(instanceIndex).translation;

	end = target.Ptr()->GetPosition();

	SINGLE progress = min(timer/ZAP_TIME,1.0);
#define FLARE 0.7

	Vector dir= end-start;
	SINGLE length = dir.magnitude();
	dir /= length;

	Vector cpos = CAMERA->GetPosition();
	Vector look (cpos - start);
	Vector i;// (look.y, -look.x, 0);
	i = cross_product(look,dir);

#define TOLERANCE 1e-3
#define BM_WIDTH 100
	if (fabs (i.x) < TOLERANCE && fabs (i.y) < TOLERANCE)
	{
		i.x = 1.0f;
	}
	
	i.normalize ();

#define TILE 0.01

	SINGLE dist = min(progress,FLARE)*length;
	//draw some
	v_list[0].pos = start-i*BM_WIDTH;
	v_list[0].color = 0;
	v_list[0].u = 0;
	v_list[0].v = -BM_WIDTH*TILE;
	v_list[1].pos = start;
	v_list[1].color = 0xffffffff;
	v_list[1].u = 0;
	v_list[1].v = 0;
	v_list[2].pos = start+i*BM_WIDTH;
	v_list[2].color = 0;
	v_list[2].u = 0;
	v_list[2].v = BM_WIDTH*TILE;
	v_list[3].pos = start+dir*dist-i*BM_WIDTH;
	v_list[3].color = 0;
	v_list[3].u = dist*TILE;
	v_list[3].v = -BM_WIDTH*TILE;
	v_list[4].pos = start+dir*dist;
	v_list[4].color = 0xffffffff;
	v_list[4].u = dist*TILE;
	v_list[4].v = 0;
	v_list[5].pos = start+dir*dist+i*BM_WIDTH;
	v_list[5].color = 0;
	v_list[5].u = dist*TILE;
	v_list[5].v = BM_WIDTH*TILE;
	
	if (progress > FLARE)
	{
		dist = progress*length-dist;
		SINGLE ratio = 4*dist/((1.0-FLARE)*length);

		//draw the flare
		v_list[6].pos = v_list[4].pos+dir*dist-i*ratio*BM_WIDTH;
		v_list[6].color = 0;
		v_list[6].u = progress*length*TILE;
		v_list[6].v = -ratio*BM_WIDTH*TILE;
		v_list[7].pos = v_list[4].pos+dir*dist;
		v_list[7].color = 0;
		v_list[7].u = progress*length*TILE;
		v_list[7].v = 0;
		v_list[8].pos = v_list[4].pos+dir*dist+i*ratio*BM_WIDTH;
		v_list[8].color = 0;
		v_list[8].u = progress*length*TILE;
		v_list[8].v = ratio*BM_WIDTH*TILE;

		
		desc.num_verts = 9;
		desc.num_indices = 24;
	}

	memcpy(desc.indices,zapIndices,sizeof(U16)*desc.num_indices);
	CQBATCH->ReleasePrimBuffer(&desc);
	BATCH->set_state(RPR_STATE_ID,0);

	if (progress > FLARE)
	{
		//draw the shield
		renderShield();
	}

}
//---------------------------------------------------------------------------
//
void Synthesis::renderShield()
{
	VOLPTR(IShipDamage) shipDamage = target;
	SMesh *smesh = shipDamage->GetShieldMesh();
	if (smesh == 0)
		return;

	Vector v[3];

	Mesh *mesh = REND->get_archetype_mesh(arch->archIndex,1.0);  //this 3db should match the shield mesh 3db
	
	Vector shieldScale(1.2,1.2,1.2);
	TRANSFORM scaleTrans;
	scaleTrans.d[0][0] = shieldScale.x;
	scaleTrans.d[1][1] = shieldScale.y;
	scaleTrans.d[2][2] = shieldScale.z;
	
	TRANSFORM trans = target.Ptr()->GetTransform()*scaleTrans;
	CAMERA->SetModelView(&trans);
	
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
//	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
//	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	PB.Color4ub(255,255,255,255);
	
	Vector look = target.Ptr()->GetTransform().translation-CAMERA->GetPosition();
	look = trans.inverse_rotate(look);
	look.normalize();


	const AnimFrame *frame = anim.retrieve_current_frame();
	int f;
/*	DisableTextures();
	BATCH->set_state(RPR_STATE_ID,2);
	PB.Begin(PB_LINES);
	for (f=0;f<smesh->f_cnt;f++)
	{
		for (int vv=0;vv<3;vv++)
		{
			Vector n = smesh->v_list[smesh->f_list[f].v[vv]].n;
			v[vv] = smesh->v_list[smesh->f_list[f].v[vv]].pt;
			SINGLE dot = fabs(dot_product(smesh->v_list[smesh->f_list[f].v[vv]].n,look));
			SINGLE fade = 1.0;
			if (dot < 0.8)
				fade = (dot-0.5) / 0.3;
			if (fade < 0.0)
				fade = 0.0;
		//	PB.Color4ub(255,255,255,F2LONG(255.0*fade));
		//	PB.TexCoord2f(v[vv].x*0.001,v[vv].y*0.001);
			PB.Color4ub(255,0,0,F2LONG(255.0*fade));
			PB.Vertex3f(v[vv].x,v[vv].y,v[vv].z);
			PB.Color4ub(0,255,0,F2LONG(255.0*fade));
			PB.Vertex3f(v[vv].x+n.x*100,v[vv].y+n.y*100,v[vv].z+n.z*100);
		}

	}
	PB.End();
	BATCH->set_state(RPR_STATE_ID,0);*/

	//FaceGroup *fg = mesh->face_groups;
	SetupDiffuseBlend(frame->texture,FALSE);
	BATCH->set_state(RPR_STATE_ID,frame->texture);

	BATCHDESC desc;
	desc.type = D3DPT_TRIANGLELIST;
	desc.vertex_format = D3DFVF_RPVERTEX;
	desc.num_verts = smesh->f_cnt*3;  //non-optimal, but problems force it for forseeable future
	desc.num_indices = smesh->f_cnt*3;
	CQBATCH->GetPrimBuffer(&desc);
	RPVertex *v_list = (RPVertex *)desc.verts;
	U16 *id_list = desc.indices;

/*	for (int vv=0;vv<smesh->v_cnt;vv++)
	{
		Vector n = smesh->v_list[vv].n;
		v_list[vv].pos = smesh->v_list[vv].pt;
		SINGLE dot = -dot_product(smesh->v_list[vv].n,look);
		SINGLE fade = 1.0;
		if (dot < 0.6)
			fade = (dot-0.5) / 0.1;
		if (fade < 0.0)
			fade = 0.0;
		v_list[vv].color = RGB(F2LONG(255.0*fade),F2LONG(255.0*fade),F2LONG(255.0*fade)) | 0xff000000;

		//this 3db should match the shield mesh 3db - but if it doesn't I did a mod
		U32 tref;
		tref = mesh->texture_batch_list[vv%mesh->vertex_batch_cnt];
		v_list[vv].u = mesh->texture_vertex_list[tref].u;
		v_list[vv].v = mesh->texture_vertex_list[tref].v;
	}*/

	int vcnt=0;
	int idx_cnt=0;
	for (f=0;f<smesh->f_cnt;f++)
	{
		SINGLE f_dot = dot_product(smesh->f_list[f].n,look);
		if (f_dot < 0)
		{
			for (int vv=0;vv<3;vv++)
			{
				Vector n = smesh->v_list[smesh->f_list[f].v[vv]].n;
				v_list[vcnt].pos = smesh->v_list[smesh->f_list[f].v[vv]].pt;
				SINGLE dot = -dot_product(n,look);
				SINGLE fade = 1.0;
				if (dot < 0.6)
					fade = (dot-0.5) / 0.1;
				if (fade < 0.0)
					fade = 0.0;
				v_list[vcnt].color = RGB(F2LONG(255.0*fade),F2LONG(255.0*fade),F2LONG(255.0*fade)) | 0xff000000;
				
				//this 3db should match the shield mesh 3db - but if it doesn't I did a mod
				U32 tref;
				tref = mesh->texture_batch_list[vv%3];//mesh->vertex_batch_cnt];
				v_list[vcnt].u = mesh->texture_vertex_list[tref].u;
				v_list[vcnt].v = mesh->texture_vertex_list[tref].v;
				
				id_list[idx_cnt++] = vcnt;
				vcnt++;
			}
		}
	}

	desc.num_verts = desc.num_indices = idx_cnt;
	CQBATCH->ReleasePrimBuffer(&desc);
	BATCH->set_state(RPR_STATE_ID,0);
}

// ILauncher

void Synthesis::InitLauncher (IBaseObject * _owner, S32 ownerIndex, S32 animArcheIndex, SINGLE _range)
{
	CQASSERT(_owner);
	_owner->QueryInterface(ILaunchOwnerID,owner,LAUNCHVOLATILEPTR);

	VOLPTR(IExtent) extentObj = owner;
	CQASSERT(extentObj);

//	mc = &extentObj->GetMeshChain();
	float box[6];
	extentObj.Ptr()->GetObjectBox(box);
	shipLength = box[BBOX_MAX_Z]-box[BBOX_MIN_Z];
}

void Synthesis::AttackPosition (const struct GRIDVECTOR * position, bool bSpecial)
{
//	CQBOMB0("AttackPosition not supported");
}

void Synthesis::AttackObject (IBaseObject * obj)
{
//	CQBOMB0("AttackObject not supported");
}
 
void Synthesis::AttackMultiSystem (struct GRIDVECTOR * position, U32 targSystemID)
{
}

const bool Synthesis::TestFightersRetracted (void) const
{
	return true;
}  // return true or else

// the following methods are for network synchronization of realtime objects
U32 Synthesis::GetSyncDataSize (void) const
{
	return sizeof(SYNC_PACKET);
}

U32 Synthesis::GetSyncData (void * buffer)
{
	if (target && (owner.Ptr()->effectFlags.canShoot()) && (!owner.Ptr()->fieldFlags.suppliesLocked()))
	{
		if ((target.Ptr()->GetPosition()-owner.Ptr()->GetPosition()).magnitude() > owner->GetWeaponRange())
			return 0;

		SYNC_PACKET * data = (SYNC_PACKET *) buffer;
		data->targetID = targetID;
		shootTarget();
		return sizeof(*data);
	}

	return 0;
}

void Synthesis::PutSyncData (void * buffer, U32 bufferSize)
{
	SYNC_PACKET data;
	memset((void *)(&data),0,sizeof(SYNC_PACKET));
	if(bufferSize != 1)
		memcpy(&data,buffer,bufferSize);

	targetID = data.targetID;
	shootTarget();
}
//---------------------------------------------------------------------------
//
void Synthesis::DoSpecialAbility (U32 specialID)
{
}
//---------------------------------------------------------------------------
//
void Synthesis::DoSpecialAbility (IBaseObject * obj)
{
}
//---------------------------------------------------------------------------
//
void Synthesis::SpecialAttackObject (IBaseObject * obj)
{
	if (obj == 0)
		target = 0;

	if (owner == 0)
	{
		CQBOMB0("Bring to the attention of Rob Marr - ignorable");
		return;
	}

	if(obj)
	{
		obj->QueryInterface(IWeaponTargetID,target,GetPlayerID());
		targetID = obj->GetPartID();
	}

//	zealotArchetypeID = ARCHLIST->GetArchetypeDataID(owner.ptr->pArchetype);
}
//---------------------------------------------------------------------------
//
BOOL32 Synthesis::Save (struct IFileSystem * outFile)
{
	DAFILEDESC fdesc = "SYNTHESIS_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwWritten;
	SYNTHESIS_SAVELOAD save;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	memset(&save, 0, sizeof(save));
	memcpy(&save, static_cast<BASE_SYNTHESIS_SAVELOAD *>(this), sizeof(BASE_SYNTHESIS_SAVELOAD));

	FRAME_save(save);
	
	file->WriteFile(0, &save, sizeof(save), &dwWritten, 0);
	
	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Synthesis::Load (struct IFileSystem * inFile)
{
	DAFILEDESC fdesc = "SYNTHESIS_SAVELOAD";
	COMPTR<IFileSystem> file;
	BOOL32 result = 0;
	DWORD dwRead;
	SYNTHESIS_SAVELOAD load;
	U8 buffer[1024];

	fdesc.lpImplementation = "DOS";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("SYNTHESIS_SAVELOAD", buffer, &load);

	*static_cast<BASE_SYNTHESIS_SAVELOAD *>(this) = load;

	FRAME_load(load);

	result = 1;

Done:	
	return result;
}
//---------------------------------------------------------------------------
//
void Synthesis::ResolveAssociations ()
{
	OBJLIST->FindObject(targetID,NONSYSVOLATILEPTR,target,IWeaponTargetID);
	if(target)
	{
		bActive = true;
	}
}
//---------------------------------------------------------------------------
//
void Synthesis::init (SynthesisArchetype *_arch)
{
	arch = _arch;
	data = arch->data;
	anim.Init(arch->animArch);
}


void Synthesis::shootTarget()
{
	IBaseObject *obj = OBJLIST->FindObject(targetID);
	if (owner)
		owner->LauncherCancelAttack();
	if(obj)
	{
		U32 visFlags = owner.Ptr()->GetTrueVisibilityFlags();
		visFlags |= obj->GetTrueVisibilityFlags();
		SetVisibilityFlags(visFlags);
		
		dwMissionID = owner.Ptr()->GetPartID();
		
		UnregisterWatchersForObject(this);
		OBJLIST->AddObject(this);
		//danger to debuggers!
	//	Transform trans = owner.ptr->GetTransform();
		
	//	ENGINE->set_instance_handler(owner.ptr->GetObjectIndex(),this);
	//	ENGINE->set_transform(instanceIndex,trans);

		VOLPTR(IShipMove) smove = owner;
		CQASSERT(smove);
		smove->RemoveFromMap();

		OBJLIST->RemoveObject(owner.Ptr());
		obj->QueryInterface(IWeaponTargetID,target,GetPlayerID());
		bActive = TRUE;
		stage = SYN_ROTATE;

		EVENTSYS->Send(CQE_OBJECT_DESTROYED, (void *) dwMissionID);
		
		systemID = owner.Ptr()->GetSystemID();
		
		MPart ownerPart(owner.Ptr());
		hullPointsPer = ownerPart->hullPoints/((SINGLE)(ownerPart->hullPointsMax));
		suppliesPer = ownerPart->supplies/((SINGLE)(ownerPart->supplyPointsMax));
	}
	else if(!THEMATRIX->IsMaster())
	{
		OBJLIST->RemoveObject(owner.Ptr());
		VOLPTR(IShipMove) smove = owner;
		CQASSERT(smove);
		smove->RemoveFromMap();
		delete owner.Ptr();
	}
	//temp
//	time = 100;
}
//----------------------------------------------------------------------------------
//---------------------------------Synthesis Factory------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

struct DACOM_NO_VTABLE SynthesisManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(SynthesisManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	U32 factoryHandle;


	//child object info
	SynthesisArchetype *pArchetype;

	//SynthesisManager methods

	SynthesisManager (void) 
	{
	}

	~SynthesisManager();
	
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
// SynthesisManager methods

SynthesisManager::~SynthesisManager()
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
void SynthesisManager::init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}
//--------------------------------------------------------------------------
//
HANDLE SynthesisManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	SynthesisArchetype *newguy = 0;
	if (objClass == OC_WEAPON)
	{
		BT_SYNTHESIS_DATA *objData = (BT_SYNTHESIS_DATA *)data;
		if (objData->wpnClass == WPN_SYNTHESIS)
		{
			newguy = new SynthesisArchetype;
			newguy->name = szArchname;
			newguy->data = objData;
			newguy->rigidBodyArm.set(0,0,0);
			
			/*if (objData->impactBlastType[0])
			{
				newguy->pImpactBlast = ARCHLIST->LoadArchetype(objData->impactBlastType);
				if (newguy->pImpactBlast)
					ARCHLIST->AddRef(newguy->pImpactBlast);
			}*/

			newguy->animArch = ANIM2D->create_archetype(objData->animName);
			
			DAFILEDESC fdesc;
			COMPTR<IFileSystem> objFile;
			
			fdesc.lpFileName = "geo_sphere_tex.3db";
			if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
			{
				if ((newguy->archIndex = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX)
				{
					goto Error;
				}
			}
			else 
			{
				CQFILENOTFOUND(fdesc.lpFileName);
				goto Error;
			}
			

			goto Done;
		}
	}

Error:
	delete newguy;
	newguy = 0;

Done:
	return newguy;
}
//--------------------------------------------------------------------------
//
BOOL32 SynthesisManager::DestroyArchetype(HANDLE hArchetype)
{
	SynthesisArchetype *deadguy = (SynthesisArchetype *)hArchetype;

	delete deadguy;
	return 1;
}
//--------------------------------------------------------------------------
//
IBaseObject * SynthesisManager::CreateInstance(HANDLE hArchetype)
{
	SynthesisArchetype *pSynthesis = (SynthesisArchetype *)hArchetype;
	
	Synthesis * obj = new ObjectImpl<Synthesis>;
	obj->objClass = OC_WEAPON;
	obj->init(pSynthesis);

	return obj;
	
}
//--------------------------------------------------------------------------
//
void SynthesisManager::EditorCreateInstance(HANDLE hArchetype, const MISSION_INFO & info)
{
	// not supported
}

struct SynthesisManager *SynthesisMgr;
//----------------------------------------------------------------------------------------------
//
struct _ssc : GlobalComponent
{


	virtual void Startup (void)
	{
		SynthesisMgr = new DAComponent<SynthesisManager>;
		AddToGlobalCleanupList((IDAComponent **) &SynthesisMgr);
	}

	virtual void Initialize (void)
	{
		SynthesisMgr->init();
	}
};

static _ssc ssc;
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

//---------------------------------------------------------------------------
//--------------------------End Synthesis.cpp------------------------------------
//---------------------------------------------------------------------------
