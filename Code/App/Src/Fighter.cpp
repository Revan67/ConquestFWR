//--------------------------------------------------------------------------//
//                                                                          //
//                              Fighter.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Fighter.cpp 115   11/14/00 12:29p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include <MGlobals.h>
#include "Sector.h"
#include "IFighter.h"
#include "IWeapon.h"
#include "TObjTrans.h"
#include "TObjControl.h"
#include "TObjPhys.h"
#include "TObjFrame.h"
#include <DFighter.h>
#include <DWeapon.h>
#include "TObject.h"
#include "Objlist.h"
#include "Startup.h"
#include "Mission.h"
#include "TerrainMap.h"
#include "UserDefaults.h"
#include "FogOfWar.h"
#include "SFX.h"
#include "ArchHolder.h"
#include "Camera.h"
#include "DMBaseData.h"
#include "IEngineTrail.h"
#include "Field.h"
#include "MPart.h"
#include "GridVector.h"
#include "ObjMap.h"
#include "CQExtent.h"
#include "Sysmap.h"
#include "IAdmiral.h"
#include "TObjRender.h"
#include "EventScheduler.h"

//material stuff
#include <Renderer.h>
//--------------------

#include <FileSys.h>
#include <IConnection.h>
#include <TSmartPointer.h>
#include <TComponent.h>
#include <IRenderPrimitive.h>
#include <BaseCam.h>


#define WINGX  100.0
#define WINGY  100.0
#define WINGZ  50.0
struct LovelyVector
{
	SINGLE x, y, z;

	operator const Vector & (void)
	{
		return *((const Vector *)this);
	}
};

typedef LovelyVector LovelyFormation[6];


static LovelyFormation formation[] = {
								{ {0.0F,0.0F,0.0F},
								{-WINGX,   0, +WINGZ},
								{ WINGX*2, 0, 0},
								{-WINGX*3, 0, +WINGZ},
								{+WINGX*2, 0, 0},
								{+WINGX*2, 0, 0} },

								{ {0.0F,0.0F,0.0F},
								{-WINGX,   0, +WINGZ},
								{ WINGX*2, 0, 0},
								{-WINGX*1, 0, +WINGZ},
								{ 0, -WINGY*1, -WINGZ},
								{ 0, +WINGY*2, 0} },

								{ {0.0F,0.0F,0.0F},
								{-WINGX,   0, +WINGZ},
								{ WINGX*2, 0, 0},
								{+WINGX*1, WINGY, +WINGZ},
								{-WINGX*1, 0, +WINGZ},
								{+WINGX*2, 0, 0} },

								{ {0.0F,0.0F,0.0F},
								{ 0,   WINGY, WINGZ},
								{ WINGX*2, -WINGY, -WINGZ},
								{ 0, WINGY, WINGZ},
								{ 0, -WINGY, +WINGZ},
								{ -WINGX*2, 0, 0} }

								};

// 
// patrol stages
//
#define LAUNCHING   0		// leaving the flight deck
#define CIRCLING	1		// circling the ship
#define RETURNING	2		// lining up with runway
#define ENTERING	3		// lined up, going back into the ship
#define ENTERING2	4		// lined up, going back into the ship
#define ATTACKING	5		// going after an enemy
#define KAMIKAZE	6		// dive into target ship
//
// formation stages
//
#define LOCKED		0		// locked behind leader
#define BREAKING	1		// breaking away from leader
#define LOOSE		2		// acting individually
#define FORMING		3		// forming up with leader

// batch render states
#define BATCH_RENDER_STATE_NUMBER 1
#define BATCH_RENDER_FIGHTER 0

struct DUMMY_INIT : RenderArch
{
};

struct DUMMY_SAVELOAD
{};

struct DummyDummy : IBaseObject
{
	INSTANCE_INDEX instanceIndex;
};

struct DummyFighter : ObjectRender<ObjectFrame<DummyDummy,DUMMY_SAVELOAD,DUMMY_INIT> >
{
	BEGIN_MAP_INBOUND(DummyFighter)
	_INTERFACE_ENTRY(IBaseObject)
	END_MAP()

	~DummyFighter()
	{
		ENGINE->destroy_instance(instanceIndex);
	}
};

struct FIGHTER_INIT : BASE_FIGHTER_INIT
{
	DUMMY_INIT *dummyInit;
	DummyFighter *dummyFighter;
};



struct Ellipse fighterBubble(Vector(0,0,0),50,Vector(1.0,1.0,1.0),Transform());

//static Material *fighter_mat=NULL;
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
struct _NO_VTABLE Fighter : public ObjectPhysics<ObjectControl<ObjectTransform<ObjectFrame<IBaseObject,FIGHTER_SAVELOAD,BASE_FIGHTER_INIT> > > >, IFighter, IWeaponTarget, BASE_FIGHTER_SAVELOAD
{
	BEGIN_MAP_INBOUND(Fighter)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IFighter)
	_INTERFACE_ENTRY(IWeaponTarget)
	END_MAP()

	//---------------------------------------------------------------------------
	//
	struct TCallback : ITerrainSegCallback
	{
		SINGLE maxY;
		U32 highest, kamikazeTarget;
		IBaseObject * const owner;

		TCallback (IBaseObject * _owner) : owner(_owner)
		{
			highest = kamikazeTarget = 0;
			maxY = 0;
		}

		virtual bool TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos);		// return false to stop callback
	};


	//
	// fighter runtime data
	//
	Fighter *nextFighter, *prevFighter;		// fighter list nodes
	
	OBJPTR<IFighterOwner> owner;
	OBJPTR<IWeaponTarget> target;
	OBJPTR<IEngineTrail> trail;
	S32 index;		// which fighter am I?
	U32 refirePeriod;			// time between shots
	U32 patrolPeriod;			// time flying around aimlessly
	U32 kamikazeDamage;
	SINGLE   patrolRadius;
	SINGLE   patrolAltitude;
	SINGLE   weaponRange, weaponVelocity;
	PARCHETYPE pWeaponType, pExplosionType;
	SINGLE baseAirAccuracy;		// from 0 to 1.0
	SINGLE baseGroundAccuracy;		// from 0 to 1.0
	S32 sensorRadius;	// for FogOfWar
	S32 cloakedSensorRadius; // for detecting cloaking
	S32 hullPoints;
	const BT_FIGHTER_DATA * pFighterData;
	MeshInfo *fighterMesh;

	// sound data
	HSOUND hKamikazeYell;			// when diving into enemy ship
	HSOUND hFighterwhoosh;			// When fighters fly by Capital ships 

	//objmap thing
	int map_square;
	U32 mapSysID;

	//
	// saveable data goes in BASE_FIGHTER_SAVELOAD
	//

	virtual ~Fighter (void);

	virtual void Render (void);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual Vector GetVelocity (void);
	
	virtual struct GRIDVECTOR GetGridPosition (void) const;

	virtual const TRANSFORM & GetTransform (void) const;

	virtual U32 GetPartID (void) const;

	virtual U32 GetPlayerID (void) const;	// return ID of player who owns fighter

	virtual void RevealFog (const U32 currentSystem);

	virtual void CastVisibleArea (void);

	virtual U32 GetSystemID (void) const;

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

 	/* IFighter methods */

//	virtual struct Mesh *GetMesh();

	virtual void BatchRender (U32 stage,BATCHDESC &desc);

	virtual void SetupBatchRender(U32 stage,BATCHDESC &desc,int fighter_cnt);

	virtual void FinishBatchRender(U32 stage,BATCHDESC &desc,int fighter_cnt);

	virtual U32 GetBatchRenderStateNumber();

	virtual void InitFighter (IBaseObject * owner, S32 index, SINGLE _baseAirAccuracy, SINGLE _baseGroundAccuracy);

	virtual void LaunchFighter (const class TRANSFORM & orientation, const class Vector & initialVelocity, U32 formationIndex, S32 fighterIndex, S32 parentIndex, S32 childIndex);

	virtual void ReturnToCarrier (void);

	virtual void SetFighterSupplies (U32 supplies);

	virtual U32  GetFighterSupplies (void) const;

	virtual FighterState GetState (void) const;

	virtual void SetTarget (IBaseObject * target);

	virtual void SetFormationState (U8 state);

	virtual U8 GetFormationState (void);

	virtual void SetPatrolState (U8 state);

//	virtual void * __fastcall GetNextFighter (OBJPTR<IFighter> & pInterface) const;

	virtual bool IsLeader (void) const;  // true if has no leader itself

	virtual bool IsRelated (S32 fighterIndex) const;	// return true if "fighterIndex" is parent or child

	virtual void GetFighterInfo (FighterInfo & info) const;

	virtual void * __fastcall GetChildFighter (OBJPTR<IFighter> & pInterface);

	virtual U32 GetFighterOwner (void) const
	{
		return ownerID;
	}

	virtual IEngineTrail * GetTrail (void)
	{
		return trail;
	}

	virtual void SetRadarSigniture (void);

	virtual S32 GetHullPoints (void);

	virtual bool IsVisible (void) const;

	virtual void SetMissionID (U32 _dwMissionID)
	{
		dwMissionID = _dwMissionID;
	}

	/* IWeaponTarget methods */
	
	virtual BOOL32 ApplyDamage (IBaseObject * collider, U32 _owner, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit=0);

	virtual BOOL32 ApplyVisualDamage (IBaseObject * collider, U32 _owner, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit=0);

	virtual BOOL32 GetModelCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction);

	virtual BOOL32 GetCollisionPosition (Vector &collide_point,Vector &dir,const Vector &start,const Vector &direction);

	virtual void AttachBlast(PARCHETYPE pBlast,const Vector &pos,const Vector &dir);

	virtual void AttachBlast(PARCHETYPE pBlast,const Transform & baseTrans);

//	virtual void GetExtentInfo (RECT **extents,SINGLE *z_step,SINGLE *z_min,U8 *slices);

	virtual U32 PendingAOEDamage (U32 damageAmount);

	virtual void ApplyAOEDamage (U32 ownerID, U32 damageAmount);

	// Fighter methods

	bool init (const FIGHTER_INIT & data);

	void doLaunch (IFighter * parent);

	void doCircle (IFighter * parent);

	void doReturn (IFighter * parent);

	void doEnter (IFighter * parent);

	void doEnter2 (IFighter * parent);

	void doAttack (IFighter * parent);

	void doKamikaze (IFighter * parent);

	SINGLE getCircleYaw (const Vector & relVec, SINGLE yaw) const;

	SINGLE getAttackYaw (IFighter * parent, const Vector & relVec, SINGLE yaw, bool bOverTarget);

	IFighter * getFighter (S8 & fighterIndex);		// returns NULL on failure

	void explode (void);

	IBaseObject * checkFutureCollision (void);

	static void getVFXPoints (GRIDVECTOR points[2], const TRANSFORM & transform, SINGLE distance);

	bool updateWeapon (void);

	bool calculateTargetTransform (TRANSFORM & result, Vector & targetPos, bool & bAlwaysHit);

	bool formationMove (IFighter *parent);	// return TRUE if handled movement for this turn

	void setPatrolState (IFighter *parent, U8 newState)
	{
		if (parent==0)
		{
			IFighter * child = getFighter(childIndex);

			patrolState = newState;
			generalCounter = REALTIME_FRAMERATE+1;
			if (newState==ATTACKING)
				formationType = (race == M_MANTIS) ? FT_MANTIS_ATTACK : FT_HUMAN_ATTACK;
			else
				formationType = (race == M_MANTIS) ? FT_MANTIS_PATROL : FT_HUMAN_PATROL;
			if (child)
				child->SetPatrolState(newState);
		}
	}

	void setFormationState (IFighter *parent, U8 newState)
	{
		if (parent==0)
		{
			IFighter * child = getFighter(childIndex);

			formationState = newState;
			if (child)
				child->SetFormationState(newState);
		}
	}

	SINGLE getRelPitch (SINGLE pitch, SINGLE relAltitude)
	{
		if (relAltitude < -500)
			return (-20 * MUL_DEG_TO_RAD) - pitch;
		else
		if (relAltitude >  500)
			return (20 * MUL_DEG_TO_RAD) - pitch;
		else
		if (relAltitude != 0)
			return ((relAltitude / 500)*(20 * MUL_DEG_TO_RAD)) - pitch;
		else
			return 0 - pitch;
	}

	void setThrustersOn (void)
	{
		// make sure relVec is far enough ahead so that cheap movement won't stop short
		Vector relVec = transform.get_k() * (-10.0 * maxLinearVelocity);
		setPosition(relVec, maxLinearVelocity);

		ObjectControl<ObjectTransform
					    <ObjectFrame<struct IBaseObject,FIGHTER_SAVELOAD,BASE_FIGHTER_INIT> >
						>::setThrustersOn();
	}

	// fighter list stuff
	static Fighter * fighterList;
	static void AddFighter (Fighter * fighter);
	static void RemoveFighter (Fighter * fighter);
};
Fighter * Fighter::fighterList;
//---------------------------------------------------------------------------
//
Fighter::~Fighter (void)
{
	if (state == PATROLLING)
		RemoveFighter(this);

	SFXMANAGER->CloseHandle(hKamikazeYell);
	SFXMANAGER->CloseHandle(hFighterwhoosh);

	if(trail != 0)
	{
		delete trail.Ptr();
		trail = 0;
	}

	if (mapSysID)
		OBJMAP->RemoveObjectFromMap(this,mapSysID,map_square);
}
//---------------------------------------------------------------------------
//
Vector Fighter::GetVelocity (void)
{
	return velocity;
}
//---------------------------------------------------------------------------
//
const TRANSFORM & Fighter::GetTransform (void) const
{
	return transform;
}
//---------------------------------------------------------------------------
//
struct GRIDVECTOR Fighter::GetGridPosition (void) const
{
	Vector pos = transform.translation;

	// clamp the result
	if (pos.x < 0)
		pos.x = 0;
	if (pos.y < 0)
		pos.y = 0;

	GRIDVECTOR result;
	result = pos;
	return result;
}
//---------------------------------------------------------------------------
//
U32 Fighter::GetPartID (void) const
{
	return dwMissionID;
}
//---------------------------------------------------------------------------
//
void Fighter::RevealFog (const U32 currentSystem)
{
	if (currentSystem == systemID)
	{
		if (parentIndex == -1 || formationState != LOCKED)
		{
			if (MGlobals::AreAllies(playerID, MGlobals::GetThisPlayer()))
				FOGOFWAR->RevealZone(this, sensorRadius, cloakedSensorRadius);
		}
	}
}
//---------------------------------------------------------------------------
//
void Fighter::SetRadarSigniture (void)
{
	CQASSERT(systemID && systemID <= MAX_SYSTEMS);

	if (parentIndex == -1 || formationState != LOCKED)
	{
		// if we are the leader or we are not in formation...
		int new_map_square = OBJMAP->GetMapSquare(systemID,transform.translation);
		if (new_map_square != map_square || mapSysID != systemID)
		{
			OBJMAP->RemoveObjectFromMap(this,mapSysID,map_square);
			map_square = new_map_square;
			mapSysID = systemID;
			OBJMAP->AddObjectToMap(this,mapSysID,map_square,OM_AIR);
		}
	}
	else
	{
		// we are not the leader, remove ourselves from the map (optimization)
		if (mapSysID)
		{
			OBJMAP->RemoveObjectFromMap(this,mapSysID,map_square);
			mapSysID = 0;
		}
	}
}
//---------------------------------------------------------------------------
//
bool Fighter::IsVisible (void) const
{
	return (bVisible!=0);
}
//---------------------------------------------------------------------------
//
void Fighter::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	bVisible = 0;
	if (systemID == currentSystem && (patrolState != ENTERING2))
	{
		IFighter * parent = getFighter(parentIndex);

		if (parent && (playerID==currentPlayer || defaults.bVisibilityRulesOff || MGlobals::AreAllies(playerID,currentPlayer)))
		{
			bVisible = parent->IsVisible();		// use parent's visibility
		}
		else
		{
			S32 screenX, screenY;
			if (CAMERA->PointToScreen(transform.translation, &screenX, &screenY, 0) != BEHIND_CAMERA)
			{
				PANE * pane = CAMERA->GetPane();
				const int offset = REAL2IDEALX(100);		// enough room for trail
				
				if (screenX+offset >= pane->x0 && screenX-offset <= pane->x1 &&
					screenY+offset >= pane->y0 && screenY-offset <= pane->y1)
				{
					if (playerID==currentPlayer || 
						defaults.bVisibilityRulesOff ||
						MGlobals::AreAllies(playerID,currentPlayer) ||
						FOGOFWAR->CheckVisiblePosition(transform.translation) )
					{
						bVisible = 1;
					}
				}
			}
		}
	}

	if (bVisible)
		SetVisibleToAllies( (1 << (currentPlayer-1)) | (1 << (playerID-1)) );
}
//---------------------------------------------------------------------------
//
void Fighter::CastVisibleArea (void)
{
	// if we are a leader
	if (parentIndex == -1)
	{
		if (formationState==LOOSE && target!=0 && patrolState==ATTACKING)
			target.Ptr()->SetVisibleToPlayer(playerID);
		OBJLIST->CastVisibleArea(playerID, systemID, GetGridPosition(), fieldFlags, sensorRadius, cloakedSensorRadius);
	}
	
	UpdateVisibilityFlags();
}
//---------------------------------------------------------------------------
//
U32 Fighter::GetSystemID (void) const
{
	return systemID;
}
//---------------------------------------------------------------------------
//
void Fighter::Render (void)
{
	if (bVisible)
	{
//		FaceGroup *fg = mesh->face_groups;
	//	BATCH->set_state(RPR_BATCH,FALSE);

	//	CAMERA->SetModelView();
		//no lighting or texture effects desired?
		//		PIPE->set_render_state(D3DRS_TEXTUREMAPBLEND,D3DTBLEND_DECALALPHA);
	/*	PIPE->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
		PIPE->set_render_state(D3DRS_ZENABLE,TRUE);
		PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);*/

		SetupDiffuseBlend(fighterMesh->fgi->texture_id,FALSE);

		PB.Begin(PB_TRIANGLES);
		PB.Color3ub(255,255,255);
		IRenderMaterial *irm=fighterMesh->mr->GetNextFaceGroup(0);
		RPVertex *verts = (RPVertex *)irm->src_verts_buffer;
		for (int i=0;i<fighterMesh->mr->face_cnt;i++)
		{
			U32 ref[3];
			ref[0] = irm->index_list[i*3];
			ref[1] = irm->index_list[i*3+1];
			ref[2] = irm->index_list[i*3+2];
			
			Vector v0,v1,v2;
			v0 = transform.rotate_translate(verts[ref[0]].pos);
			v1 = transform.rotate_translate(verts[ref[1]].pos);
			v2 = transform.rotate_translate(verts[ref[2]].pos);
			
			
			PB.TexCoord2f(verts[ref[0]].u,verts[ref[0]].v);
			PB.Vertex3f_NC(v0);
			PB.TexCoord2f(verts[ref[1]].u,verts[ref[1]].v);
			PB.Vertex3f_NC(v1);
			PB.TexCoord2f(verts[ref[2]].u,verts[ref[2]].v);
			PB.Vertex3f_NC(v2);
		}

		if(trail != 0)
		{
			//			BATCH->set_state(RPR_BATCH,FALSE);
			trail->Render();
			//			BATCH->set_state(RPR_BATCH,TRUE);
		}
	}
}

void Fighter::BatchRender(U32 stage,BATCHDESC &desc)
{
	if (bVisible)
	switch(stage)
	{
	case BATCH_RENDER_FIGHTER:
		{
		/*	vis_state result = VS_UNKNOWN;
			const Transform &world_to_view = MAINCAM->get_inverse_transform() ;
			Vector view_pos (world_to_view.rotate_translate(transform.translation));
			result = MAINCAM->object_visibility(view_pos, fighterMesh->radius);
			if (result == VS_NOT_VISIBLE || result == VS_SUB_PIXEL || result == VS_PARTIALLY_VISIBLE)
			{
				BATCH->set_render_state(D3DRS_CLIPPING,TRUE);
			}*/
			
			CQASSERT(fighterMesh->faceGroupCnt == 1 && "Fighters should have one face group");
			
			int v_offset = desc.num_verts;
			int i;
			IRenderMaterial *irm=fighterMesh->mr->GetNextFaceGroup(0);
			RPVertex *verts = (RPVertex *)irm->src_verts_buffer;
			for (i=0;i<fighterMesh->mr->face_cnt;i++)
			{
				desc.indices[desc.num_indices] = v_offset+irm->index_list[i*3];
				desc.indices[desc.num_indices+1] = v_offset+irm->index_list[i*3+1];
				desc.indices[desc.num_indices+2] = v_offset+irm->index_list[i*3+2];
				desc.num_indices += 3;
			}

			RPVertex *v_list = (RPVertex *)desc.verts;
			for (i=0;i<fighterMesh->mr->pos_cnt;i++)
			{
				RPVertex *vert=&verts[i];
				Vector v;
				v = transform.rotate_translate(vert->pos);
				v_list[desc.num_verts].u = vert->u;
				v_list[desc.num_verts].v = vert->v;
				v_list[desc.num_verts].pos = v;
				v_list[desc.num_verts].color = 0xffffffff;
				desc.num_verts++;
			}
		}
		break;
	default:
		{
			if(trail)
				trail->BatchRender(stage-BATCH_RENDER_STATE_NUMBER);
		}
		break;
	}
}
//---------------------------------------------------------------------------
//
void Fighter::SetupBatchRender(U32 stage,BATCHDESC &desc,int fighter_cnt)
{
	switch(stage)
	{
	case BATCH_RENDER_FIGHTER:
		{
			BATCH->set_state(RPR_BATCH,TRUE);
			BATCH->set_state(RPR_STATE_ID,0);
//			BATCH->set_render_state(D3DRS_CLIPPING,FALSE);
			//no lighting or texture effects desired?
			BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
			BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
			//fighters wrap
			SetupDiffuseBlend(fighterMesh->fgi->texture_id,FALSE);
			//this could be used to do the glint, except it's not noticable
			//	BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_ADD );
			BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,FALSE);
			//	PB.Begin(PB_TRIANGLES,fighter_cnt*fighterMesh->mr->face_cnt*3);
			desc.type = D3DPT_TRIANGLELIST;
			desc.vertex_format = D3DFVF_RPVERTEX;
			desc.num_verts = fighter_cnt*fighterMesh->mr->pos_cnt;
			desc.num_indices = fighter_cnt*fighterMesh->mr->face_cnt*3;
			CQBATCH->GetPrimBuffer(&desc);
			desc.num_indices = desc.num_verts = 0;
			break;
		}
	default:
		{
			if(trail)
				trail->SetupBatchRender(stage - BATCH_RENDER_STATE_NUMBER);
			break;
		}
	}
}
//---------------------------------------------------------------------------
//
void Fighter::FinishBatchRender(U32 stage,BATCHDESC &desc,int fighter_cnt)
{
	switch(stage)
	{
	case BATCH_RENDER_FIGHTER:
		{
			CQBATCH->ReleasePrimBuffer(&desc);
//			BATCH->set_render_state(D3DRS_CLIPPING,TRUE);
			BATCH->set_state(RPR_BATCH,TRUE);
			break;
		}
	default:
		{
			if(trail)
				trail->FinishBatchRender(stage - BATCH_RENDER_STATE_NUMBER);
			break;
		}
	}
}
//---------------------------------------------------------------------------
//
U32 Fighter::GetBatchRenderStateNumber()
{
	if(trail)
	{
		return BATCH_RENDER_STATE_NUMBER + trail->GetBatchRenderStateNumber();
	}
	return BATCH_RENDER_STATE_NUMBER ;
}
//---------------------------------------------------------------------------
//
BOOL32 Fighter::Update (void)
{
	if (state == DEAD)
		return 0;

	FRAME_update();

	IFighter * parent = getFighter(parentIndex);
	
	if (owner!=0)
	{
		MPart part = owner.Ptr();
		if (part->playerID != playerID || systemID != part->systemID)
		{
			if (part->playerID != playerID && target==0)
			{
				// attack former home base
				OBJLIST->FindObject(part->dwMissionID, playerID, target, IWeaponTargetID);
			}
			parent = 0;
			if (patrolState != KAMIKAZE)
				setPatrolState(parent, KAMIKAZE);		// owner jumped out without us!
		}
	}

//	bNoDynamics = (parent!=0 && formationState==LOCKED);

	if (bKamikaziComplete)
	{
		explode(); 
		return 0;
	}

	switch (patrolState)
	{
	case LAUNCHING:
		if (owner!=0)
			doLaunch(parent);
		else
			setPatrolState(parent, KAMIKAZE);		
		break;

	case CIRCLING:
		if (owner!=0)
			doCircle(parent);
		else
			setPatrolState(parent, KAMIKAZE);		
		break;

	case RETURNING:
		if (owner!=0)
			doReturn(parent);
		else
			setPatrolState(parent, KAMIKAZE);		
		break;

	case ENTERING:
		if (owner!=0)
			doEnter(parent);
		else
			setPatrolState(parent, KAMIKAZE);		
		break;

	case ENTERING2:
		if (owner!=0)
			doEnter2(parent);
		else
			setPatrolState(parent, KAMIKAZE);		
		break;

	case ATTACKING:
		doAttack(parent);
		break;

	case KAMIKAZE:
		doKamikaze(parent);
		break;
	}

	if(trail != 0)
		trail->Update();

	return 1;
}
//---------------------------------------------------------------------------
//
void Fighter::PhysicalUpdate (SINGLE dt)
{
	if (state == PATROLLING)
	{
		if (parentIndex!=-1 && formationState==LOCKED && bVisible==false)
		{
		}
		else
		{
			FRAME_physicalUpdate(dt);
			if(trail != 0)
				trail->PhysicalUpdate(dt);
		}
	}
}
//---------------------------------------------------------------------------
//
void Fighter::InitFighter (IBaseObject * _owner, S32 _index, SINGLE _baseAirAccuracy, SINGLE _baseGroundAccuracy)
{
	CQASSERT(_owner);
	_owner->QueryInterface(IFighterOwnerID, owner, NONSYSVOLATILEPTR);
	CQASSERT(owner!=0);
	systemID = _owner->GetSystemID();

	index = _index;
	baseAirAccuracy = _baseAirAccuracy;
	baseGroundAccuracy = _baseGroundAccuracy;
}
//---------------------------------------------------------------------------
//
void Fighter::LaunchFighter (const class TRANSFORM & orientation, const class Vector & initialVelocity, U32 _formationIndex, S32 _fighterIndex, S32 _parentIndex, S32 _childIndex)
{
	CQASSERT(dwMissionID != 0);		// must already be set
	transform = orientation;
	formationIndex = _formationIndex;
	parentIndex = _parentIndex;		// from fighter pool of parent
	childIndex = _childIndex;		// from fighter pool of parent
	myIndex = _fighterIndex;		// from fighter pool of parent

	// don't start in formation
//	transform.translation = orientation.rotate_translate(formation[_formationIndex]);

	velocity = transform.get_k() * -getDynamicsData().maxLinearVelocity;
	velocity += initialVelocity;

	patrolTime = patrolPeriod;
	state = PATROLLING;
	patrolState = LAUNCHING;
	generalCounter = REALTIME_FRAMERATE+1;
	bCirclePatrol = (rand() & 1);
	AddFighter(this);
	target = 0;
	formationState = LOCKED;
	refireTime = rand() % refirePeriod;
	systemID = owner.Ptr()->GetSystemID();
	CQASSERT(systemID && systemID <= MAX_SYSTEMS);
	ownerID = owner.Ptr()->GetPartID();
	playerID = owner.Ptr()->GetPlayerID();	// save this for air-defense's benefit
	dwMissionID = (dwMissionID & ~PLAYERID_MASK) | playerID;		// update missionID for current player (takeover)
	ClearVisibilityFlags();
		
	IBaseObject::MDATA mdata;
	if (owner.Ptr()->GetMissionData(mdata))
		race = mdata.pSaveData->race;
	formationType = (race == M_MANTIS) ? FT_MANTIS_PATROL : FT_HUMAN_PATROL;
	//
	// reset kamikazi data
	//
	bKamikaziTargetSelected = bKamikaziComplete = bKamikaziYellComplete = false;
	kamikazeTimer = 0;

	int tech = MGlobals::GetUpgradeLevel(owner.Ptr()->GetPlayerID(),UG_FIGHTER,race);

	//reset trail data
	if(trail != 0)
	{
		trail->SetLengthModifier(tech*0.5+1);
		trail->Reset();
	}

	hullPoints = pFighterData->hullPointsMax;
}
//---------------------------------------------------------------------------
//
void Fighter::ReturnToCarrier (void)
{
	setPatrolState(getFighter(parentIndex), RETURNING);
}
//---------------------------------------------------------------------------
//
void Fighter::SetFighterSupplies (U32 _supplies)
{
	supplies = _supplies;
}
//---------------------------------------------------------------------------
//
U32 Fighter::GetFighterSupplies (void) const
{
	return supplies;
}
//---------------------------------------------------------------------------
//
FighterState Fighter::GetState (void) const
{
	return state;
}
//---------------------------------------------------------------------------
//
void Fighter::SetTarget (IBaseObject * _target)
{
	if (systemID == owner.Ptr()->GetSystemID())		// ignore commands from base if we are in another system
	{
		if (_target)
		{
			_target->QueryInterface(IWeaponTargetID, target, playerID);
#ifndef FINAL_RELEASE
			if (target==0)
			{
				MPart part = _target;
				if (part.isValid())
				{
					CQBOMB1("Target \"%s\" doesn't support IWeaponTarget!", (const char *)part->partName);
				}
				else
					CQBOMB1("Target objClass=0x%X doesn't support IWeaponTarget!", _target->objClass);
			}
#endif
		}
		else
			target = 0;

		IFighter * ourChild = getFighter(childIndex);

		if (ourChild)
		{
			OBJPTR<IFighter> childTarget;
			IBaseObject * __target=0;

			if (_target && _target->QueryInterface(IFighterID, childTarget))
				__target = (IBaseObject *)childTarget->GetChildFighter(childTarget);
			if (__target)
				_target = __target;
					//_target = childTarget.ptr;

			ourChild->SetTarget(_target);
		}

		if (supplies > 0 && patrolState != LAUNCHING && target!=0 && target.Ptr()->GetSystemID() == systemID)
			setPatrolState(getFighter(parentIndex), ATTACKING);	
	}
}
//---------------------------------------------------------------------------
//
void Fighter::SetFormationState (U8 state)
{
	IFighter * child = getFighter(childIndex);

	formationState = state;
	if (child)
		child->SetFormationState(state);
}
//---------------------------------------------------------------------------
//
U8 Fighter::GetFormationState (void)
{
	return formationState;
}
//---------------------------------------------------------------------------
//
void Fighter::SetPatrolState (U8 state)
{
	IFighter * child = getFighter(childIndex);

	switch (state)
	{
	case ENTERING:		// parent is entering the mother ship, we must go it alone
		parentIndex = -1;
		break;
	case RETURNING:
		if (formationState==LOOSE)
		{
			parentIndex = -1;		// don't return with parent
			break;
		}

		// fall through intentional

	default:
		patrolState = state;
		if (state==ATTACKING)
			formationType = (race == M_MANTIS) ? FT_MANTIS_ATTACK : FT_HUMAN_ATTACK;
		else
			formationType = (race == M_MANTIS) ? FT_MANTIS_PATROL : FT_HUMAN_PATROL;
		generalCounter = REALTIME_FRAMERATE+1;
		break;
	}
	if (child)
		child->SetPatrolState(state);
}
//---------------------------------------------------------------------------
//
/*void * Fighter::GetNextFighter (OBJPTR<IFighter> & pInterface) const
{
	pInterface = static_cast<IBaseObject *>((Fighter *)nextFighter);
	pInterface.offset = cqoffsetofclass(IFighter, Fighter) - cqoffsetofclass(IBaseObject, Fighter);
	return pInterface.ptr;
}*/
//---------------------------------------------------------------------------
//
bool Fighter::IsLeader (void) const
{
	return (parentIndex == -1 || formationState != LOCKED);
}
//---------------------------------------------------------------------------
//
bool Fighter::IsRelated (S32 fighterIndex) const
{
	return (fighterIndex==childIndex || fighterIndex==parentIndex);
}
//---------------------------------------------------------------------------
//
U32 Fighter::GetPlayerID (void) const
{
	return playerID;
}
//---------------------------------------------------------------------------
//
void Fighter::GetFighterInfo (FighterInfo & info) const
{
	info.bIsLeader = (parentIndex == -1 || formationState != LOCKED);
	info.playerID = playerID;
	info.systemID = systemID;
	info.pFighterData = pFighterData;
}
//---------------------------------------------------------------------------
//
void * Fighter::GetChildFighter (OBJPTR<IFighter> & pInterface)
{
	void *result = static_cast<IBaseObject *>((Fighter *)getFighter(childIndex));
//	pInterface.offset = cqoffsetofclass(IFighter, Fighter) - cqoffsetofclass(IBaseObject, Fighter);
//	return pInterface.ptr;
	return result;
}
//---------------------------------------------------------------------------
//
BOOL32 Fighter::ApplyDamage (IBaseObject * collider, U32 _owner, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit)
{
	SYSMAP->RegisterAttack(systemID,GetGridPosition(),playerID);

	SINGLE admiralMod = 0;
	IBaseObject * obj = OBJLIST->FindObject(GetFighterOwner());
	if(obj)
	{
		MPart fighterOwner = obj;
		if(fighterOwner.isValid() && fighterOwner->fleetID)
		{
			VOLPTR(IAdmiral) flagship;
			OBJLIST->FindObject(fighterOwner->fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
			if(flagship.Ptr())
			{
				admiralMod = flagship->GetFighterDodgeBonus();
			}
		}
	}
	
	if(((rand() %1000)/1000.0) > (pFighterData->dodge+admiralMod))
	{
		hullPoints -= 1;
		if(hullPoints <= 0)
			explode();
	}
	return TRUE;  //don't want a hull hit
}
//---------------------------------------------------------------------------
//
BOOL32 Fighter::ApplyVisualDamage (IBaseObject * collider, U32 _owner, const Vector & pos, const Vector &dir,U32 amount,BOOL32 bShieldHit)
{
	return false;
}
//---------------------------------------------------------------------------
//
U32 Fighter::PendingAOEDamage (U32 damageAmount)
{
	CQBOMB0("AOE damage not supported!");
	return 0;
}
//---------------------------------------------------------------------------
//
void Fighter::ApplyAOEDamage (U32 ownerID, U32 damageAmount)
{
	SYSMAP->RegisterAttack(systemID,GetGridPosition(),playerID);

	SINGLE admiralMod = 0;
	IBaseObject * obj = OBJLIST->FindObject(GetFighterOwner());
	if(obj)
	{
		MPart fighterOwner = obj;
		if(fighterOwner.isValid() && fighterOwner->fleetID)
		{
			VOLPTR(IAdmiral) flagship;
			OBJLIST->FindObject(fighterOwner->fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
			if(flagship.Ptr())
			{
				admiralMod = flagship->GetFighterDodgeBonus();
			}
		}
	}

	if(((rand() %1000)/1000.0) > (pFighterData->dodge+admiralMod))
	{
		hullPoints -= 1;
		if(hullPoints <= 0)
			explode();
	}
}
//---------------------------------------------------------------------------
//
BOOL32 Fighter::GetModelCollisionPosition (Vector &collide_point, Vector &dir, const Vector &start, const Vector &direction)
{
	return 0;
}
//---------------------------------------------------------------------------
//
BOOL32 Fighter::GetCollisionPosition (Vector &collide_point, Vector &dir, const Vector &start, const Vector &direction)
{
	BOOL32 result = 0;

	fighterBubble.transform(&fighterBubble,transform.translation,transform);

	if (RayEllipse(collide_point,dir,start,direction,fighterBubble))
		result = 1;

	return result;
}
//---------------------------------------------------------------------------
//
void Fighter::AttachBlast(PARCHETYPE pBlast,const Vector &pos,const Vector &dir)
{
	return;
}
//---------------------------------------------------------------------------
//
void Fighter::AttachBlast(PARCHETYPE pBlast,const Transform & baseTrans)
{
	return;
}

//---------------------------------------------------------------------------
//
/*void Fighter::GetExtentInfo (RECT **extents,SINGLE *z_step,SINGLE *z_min,U8 *slices)
{
	CQASSERT(0 && "Fighters don't have extents!");
	*extents=0;
	*z_step=0;
	*z_min=0;
	*slices=0;
}*/
//---------------------------------------------------------------------------
// traveling in formation away from mother ship
//
void Fighter::doLaunch (IFighter * parent)
{
	if (--generalCounter <= 0)
	{
		if (target && target.Ptr()->GetSystemID() == systemID)
			setPatrolState(parent, ATTACKING);
		else
			setPatrolState(parent, CIRCLING);
	}

	if (parent==0 || formationMove(parent)==false)
	{
		rotateShip(0, 0, (10*MUL_DEG_TO_RAD) - transform.get_pitch());
		setThrustersOn();
	}
}
//---------------------------------------------------------------------------
// circle around the carrier
//
void Fighter::doCircle (IFighter * parent)
{
	if (supplies > 0 && target!=0 && target.Ptr()->GetSystemID() == systemID)
		setPatrolState(parent, ATTACKING);	
	else
	if (--patrolTime <= 0)
		setPatrolState(parent, RETURNING);

	if (parent==0 || formationMove(parent)==false)
	{
		SINGLE yaw, pitch, relYaw, relRoll, relPitch, relAlt;

		yaw = transform.get_yaw();
		pitch = transform.get_pitch();

		if (fabs(pitch) > (45 * MUL_DEG_TO_RAD))
		{
			relYaw = 0;
			relRoll = 0;
			relPitch = 0 - pitch;
			relAlt = patrolAltitude - transform.translation.z;
		}
		else
		{
			SINGLE alt = patrolAltitude;
			IBaseObject * collider;

			if ((collider = checkFutureCollision()) != 0)
			{
				OBJBOX tbox;
				if (collider->GetObjectBox(tbox))
				{
					alt = tbox[2] + collider->GetTransform().translation.z;
					if (alt < patrolAltitude)
						alt = patrolAltitude;
				}
			}

			Vector relVec = owner.Ptr()->GetPosition();
			relVec -= transform.get_position();
			relVec.z = 0;

			relYaw = getCircleYaw(relVec, yaw);
			relRoll = 0 - transform.get_roll();
			relAlt = alt - transform.translation.z;
			relPitch = getRelPitch(pitch, relAlt);
		}

		rotateShip(relYaw, relRoll, relPitch);
		setAltitude(relAlt);
		setThrustersOn();
	}
}
//---------------------------------------------------------------------------
//
SINGLE Fighter::getCircleYaw (const Vector & relVec, SINGLE yaw) const
{
	SINGLE relYaw;

	//
	// calculate relYaw for collider ship
	//
	if (bCirclePatrol)
	{
		SINGLE closeness;
		SINGLE offangle = PI/2;		// 90 degrees off from radius vector

		closeness = relVec.magnitude() / patrolRadius;
		if (closeness < 1)
			offangle *= closeness;
		else
		if (closeness < 2)
			offangle += ((closeness-1) * (PI/2));
		else
			offangle = PI;

		relYaw = get_angle(-relVec.x, -relVec.y) + offangle;	// rotate 90 degrees
		if (relYaw < -PI)
			relYaw += PI*2;
		else
		if (relYaw > PI)
			relYaw -= PI*2;

		relYaw -= yaw;
		if (relYaw < -PI)
			relYaw += PI*2;
		else
		if (relYaw > PI)
			relYaw -= PI*2;
	}
	else
	{
		relYaw = get_angle(relVec.x, relVec.y) - yaw;		// turn needed to head back to base
		if (relYaw < -PI)
			relYaw += PI*2;
		else
		if (relYaw > PI)
			relYaw -= PI*2;

		if (fabs(relYaw) > (45*MUL_DEG_TO_RAD) && relVec.magnitude() < patrolRadius)
		{
			if (relYaw < 0)
				relYaw += PI;
			else
				relYaw -= PI;
		}
	}
	return relYaw;
}
//---------------------------------------------------------------------------
//
void Fighter::doReturn (IFighter * parent)
{
	Vector relVec = owner.Ptr()->GetPosition();
	relVec.x -= transform.translation.x;
	relVec.y -= transform.translation.y;
	relVec.z = patrolAltitude - transform.translation.z;

	if (parent==0 || formationMove(parent)==false)
	{
		SINGLE yaw, pitch, relYaw, relRoll;

		yaw = transform.get_yaw();
		pitch = transform.get_pitch();

		if (fabs(pitch) > (45 * MUL_DEG_TO_RAD))
		{
			relYaw = 0;
			relRoll = 0;
			setThrustersOn();
			rotateShip(relYaw, relRoll, 0 - pitch);
		}
		else
		{
			SINGLE relPitch;
			SINGLE alt = patrolAltitude;
			IBaseObject * collider;

			if ((collider = checkFutureCollision()) != 0)
			{
				OBJBOX tbox;
				if (collider->GetObjectBox(tbox))
				{
					alt = tbox[2] + collider->GetTransform().translation.z;
					if (alt < patrolAltitude)
						alt = patrolAltitude;
				}
			}
			
			relYaw = get_angle(relVec.x, relVec.y) - yaw;		// turn needed to head back to base
			if (relYaw < -PI)
				relYaw += PI*2;
			else
			if (relYaw > PI)
				relYaw -= PI*2;

			relRoll = 0 - transform.get_roll();
			relVec.z = alt - transform.translation.z;
			relPitch = getRelPitch(pitch, relVec.z);
			rotateShip(relYaw, relRoll, relPitch);
			setThrustersOn();
			// setPosition(relVec);

			if (relVec.fast_magnitude() < getDynamicsData().maxLinearVelocity*4)
			{
				setPatrolState(parent, ENTERING);
				generalCounter = (8*REALTIME_FRAMERATE)+1;
				owner->GetLandingClearance();
			}
		}
	}
/*
	if (relVec.magnitude() <= MAX_FORWARD_VELOCITY*ELAPSED_TIME*4.0F)
	{
		setPatrolState(parent, ENTERING);
		owner->GetLandingClearance();
	}
*/
}
//---------------------------------------------------------------------------
//
void Fighter::doEnter (IFighter * parent)
{
	if (--generalCounter < 0)
		patrolState = ENTERING2;

//	SINGLE pitch = transform.get_pitch();
	Vector relVec = owner.Ptr()->GetPosition();
	relVec -= transform.get_position();
	SINGLE relYaw = fixAngle(TRANSFORM::get_yaw(relVec) - transform.get_yaw());

	rotateShip(relYaw, 0, 0);

	if (setPosition(relVec, owner.Ptr()->GetVelocity().magnitude()))
		patrolState = ENTERING2;

}
//---------------------------------------------------------------------------
//
void Fighter::doEnter2 (IFighter * parent)
{
	state = ONDECK;
	RemoveFighter(this);
	UnregisterWatchersForObject(this);
	OBJMAP->RemoveObjectFromMap(this,mapSysID,map_square);
	mapSysID = 0;
	owner->OnFighterLanding(index);
}
//---------------------------------------------------------------------------
//
void Fighter::doAttack (IFighter * parent)
{
	if (target!=0 && target.Ptr()->GetSystemID() != systemID)
	{
		setPatrolState(parent, CIRCLING);
		DEBUG_resetInputCounter();
		return;
	}

	if (--patrolTime <= 0)
		patrolTime = 0;

	if (target==0)
	{
		if (patrolTime<=0 || supplies<=0)
			setPatrolState(parent, RETURNING);
		else
			setPatrolState(parent, CIRCLING);

		if (formationState != LOCKED)
		{
			if (parent==0)
			{
				setFormationState(parent, FORMING);
				formationState = LOCKED;
			}
			else
				formationState = FORMING;
		}
		DEBUG_resetInputCounter();
		return;
	}

	if (owner==0)
		setPatrolState(parent, KAMIKAZE);
	else
	if (supplies<=0)
		setPatrolState(parent, RETURNING);

	if (parent==0 || formationMove(parent)==false)
	{
		SINGLE yaw, pitch, relYaw, relRoll, relPitch, relAlt;

		yaw = transform.get_yaw();
		pitch = transform.get_pitch();

		if (fabs(pitch) > (45 * MUL_DEG_TO_RAD))
		{
			relYaw = 0;
			relRoll = 0;
			relPitch = 0 - pitch;
			relAlt = patrolAltitude - transform.translation.z;
		}
		else
		{
			SINGLE alt = patrolAltitude;
			IBaseObject * collider;
			bool bOverTarget=false;

			if ((collider = checkFutureCollision()) != 0)
			{
				OBJBOX tbox;
				if (collider->GetObjectBox(tbox))
				{
					alt = tbox[2] + collider->GetTransform().translation.z;
					if (collider != target.Ptr() && alt < patrolAltitude)		// swoop down on target
						alt = patrolAltitude;
					if (collider == target.Ptr())
						bOverTarget=true;
				}
			}

			Vector relVec = target.Ptr()->GetPosition();
			relVec -= transform.get_position();
			relVec.z = 0;

			relYaw = getAttackYaw(parent, relVec, yaw, bOverTarget);
			relRoll = 0 - transform.get_roll();
			relAlt = alt - transform.translation.z;
			relPitch = getRelPitch(pitch, relAlt);
		}

		rotateShip(relYaw, relRoll, relPitch);
		setAltitude(relAlt);
		setThrustersOn();
	}

	updateWeapon();
}
//---------------------------------------------------------------------------
// there's no hope! ram into target ship
//
void Fighter::doKamikaze (IFighter * parent)
{
//	CQASSERT(parent==0);

	if (kamikazeTimer>0)
	{
		if (--kamikazeTimer==0)
		{
			bKamikaziComplete=true;
		}
	}
	else
	{
		kamikazeTimer = (REALTIME_FRAMERATE*8)+1;	// no more than 8.5 seconds
	}

	if (target==0 || target.Ptr()->IsVisibleToPlayer(playerID)==false || target.Ptr()->GetSystemID()!=systemID)
	{
		rotateShip(0, 0 - transform.get_roll(), (70 * MUL_DEG_TO_RAD) - transform.get_pitch());
		setThrustersOn();

		// if we have gone past the camera, our job is finished
		if (CAMERA->GetTransform()->translation.z < transform.translation.z)
			bKamikaziComplete = true;
		return;
	}

	if (bKamikaziTargetSelected==0)
	{
		OBJBOX tbox;
	
		if (target.Ptr()->GetObjectBox(tbox))
		{
			kamikaziTarget.x = ((tbox[0]-tbox[1]) * (float(rand()) / float(RAND_MAX))) + tbox[1];
			kamikaziTarget.y = ((tbox[2]-tbox[3]) * (float(rand()) / float(RAND_MAX))) + tbox[3];
			kamikaziTarget.z = ((tbox[4]-tbox[5]) * (float(rand()) / float(RAND_MAX))) + tbox[5];
		}
		else
			kamikaziTarget.zero();

		bKamikaziTargetSelected=true;
	}

	Vector relVec = target.Ptr()->GetTransform().rotate_translate(kamikaziTarget);
	relVec.x -= transform.translation.x;
	relVec.y -= transform.translation.y;
	relVec.z -= transform.translation.z;
 	SINGLE yaw, pitch, relYaw, relRoll;

 	yaw = transform.get_yaw();
 	pitch = transform.get_pitch();

	if (fabs(pitch) > (45 * MUL_DEG_TO_RAD))
	{
		relYaw = 0;
		relRoll = 0;
		setThrustersOn();
		rotateShip(relYaw, relRoll, 0 - pitch);
	}
	else
	{
		SINGLE relPitch;
		SINGLE alt = relVec.z + transform.translation.z;
		IBaseObject * collider;

		if ((collider = checkFutureCollision()) != 0 && collider != target.Ptr())
		{
			OBJBOX tbox;
			if (collider->GetObjectBox(tbox))
			{
				alt = tbox[2] + collider->GetTransform().translation.z;
				if (alt < relVec.z + transform.translation.z)
					alt = relVec.z + transform.translation.z;
			}
		}
		
		relYaw = get_angle(relVec.x, relVec.y) - yaw;		// turn needed to head back to base
		if (relYaw < -PI)
			relYaw += PI*2;
		else
		if (relYaw > PI)
			relYaw -= PI*2;

		relRoll = 0 - transform.get_roll();
		relVec.z = alt - transform.translation.z;
		relPitch = getRelPitch(pitch, relVec.z);
		rotateShip(relYaw, relRoll, relPitch);
		setPosition(relVec);

		if (bKamikaziYellComplete==false && fabs(relYaw) < 5 * MUL_DEG_TO_RAD)
		{
			bKamikaziYellComplete = true;
			hKamikazeYell  = SFXMANAGER->Open(pFighterData->kamikazeYell);
			SFXMANAGER->Play(hKamikazeYell, systemID, &transform.translation);
		}
	}

	//
	// check to see if we have hit the target
	//

	if (bKamikaziYellComplete && fabs(relYaw) > 50 * MUL_DEG_TO_RAD)	// have we gone past the target?
		bKamikaziComplete = true;
	else
	{
		Vector collide_point,dir,direction;

		direction = -transform.get_k();
		if (target->GetCollisionPosition(collide_point,dir,transform.translation, direction))
		{
			Vector len = collide_point;
			len -= transform.translation;
			SINGLE collide_dist = len.magnitude();

			if (getDynamicsData().maxLinearVelocity >= collide_dist)		// we have a winner!
			{
				if (supplies>0)
				{
					supplies--;
					//target->ApplyDamage(this, ownerID, collide_point, dir, kamikazeDamage);
					target->ApplyDamage(this, ownerID, transform.translation-direction*300, direction, kamikazeDamage);

				}
				bKamikaziComplete = true;
			}
		}
	}

	if (target)
		updateWeapon();
}
//---------------------------------------------------------------------------
//
SINGLE Fighter::getAttackYaw (IFighter * parent, const Vector & relVec, SINGLE yaw, bool bOverTarget)
{
	SINGLE relYaw;

	//
	// calculate relYaw for target ship
	//
	relYaw = get_angle(relVec.x, relVec.y) - yaw;		// turn needed to head back to target
	if (relYaw < -PI)
		relYaw += PI*2;
	else
	if (relYaw > PI)
		relYaw -= PI*2;

	//
	// do we need to head away from target?
	//
	if (fabs(relYaw) > (45*MUL_DEG_TO_RAD) && relVec.magnitude() < weaponRange)
	{
		if (formationState == LOCKED && bOverTarget)
		{
			setFormationState(parent, BREAKING);
			if (parent == 0)
			{
				hFighterwhoosh  = SFXMANAGER->Open(pFighterData->fighterwhoosh);
				SFXMANAGER->Play(hFighterwhoosh, systemID, &transform.translation);
			}
		}

		if (relYaw < 0)
			relYaw += PI;
		else
			relYaw -= PI;
	}
	else
	{
		if (formationState == BREAKING)
			setFormationState(parent, LOOSE);
	}

	return relYaw;
}
//---------------------------------------------------------------------------
//
IFighter * Fighter::getFighter (S8 & fighterIndex)
{
	IFighter * result=0;

	if (owner!=0 && fighterIndex >= 0)
		if ((result = owner->GetFighter(fighterIndex)) == 0 || result->IsRelated(myIndex)==false)
			fighterIndex = -1;

	return result;
}
//---------------------------------------------------------------------------
//
bool Fighter::init (const FIGHTER_INIT & data)
{
	pFighterData = data.pData;

	refirePeriod = (data.pData->refirePeriod * REALTIME_FRAMERATE)+1;			// time between shots
	patrolPeriod = (data.pData->patrolPeriod * REALTIME_FRAMERATE)+1;			// time flying around aimlessly
	patrolRadius = data.pData->patrolRadius * GRIDSIZE;
	sensorRadius = data.pData->sensorRadius;
	cloakedSensorRadius = data.pData->cloakedSensorRadius;
	patrolAltitude= data.pData->patrolAltitude;
	refireTime = rand() % refirePeriod;
	kamikazeDamage = data.pData->kamikazeDamage;
	baseAirAccuracy = 1.0F;
	baseGroundAccuracy = 1.0F;

	pArchetype = data.pArchetype;
	objClass = OC_FIGHTER;
	supplies = 0;//data.pData->maxSupplies;

	pWeaponType = data.pWeaponType;
	pExplosionType = data.pExplosionType;

	BT_PROJECTILE_DATA * weapon = (BT_PROJECTILE_DATA *) ARCHLIST->GetArchetypeData(data.pWeaponType);

	if(data.pData->attackRange)
		weaponRange = data.pData->attackRange*GRIDSIZE;
	else
		weaponRange = weapon->maxVelocity/2;

	weaponVelocity = weapon->maxVelocity;

	if(data.pEngineTrailType != 0)
	{
		IBaseObject * obj = ARCHLIST->CreateInstance(data.pEngineTrailType);
		if(obj)
		{
			obj->QueryInterface(IEngineTrailID,trail,NONSYSVOLATILEPTR);
			if(trail != 0)
			{
				trail->InitEngineTrail(this,instanceIndex);
			}
		}

	}

	fighterMesh = data.dummyFighter->mc.mi[0];

	return true;
}
//---------------------------------------------------------------------------
//
inline void Fighter::getVFXPoints (GRIDVECTOR points[2], const TRANSFORM & transform, SINGLE distance)
{
	//
	// calculate points (in clock-wise order) for terrain foot-pad system
	//
	Vector pt, wpt;

	pt.y = 0;
	pt.x = 0;		// _box[0];	// maxx
	pt.z = distance*-2;		//box[5];	// minz

	wpt = transform.rotate_translate(pt);
	points[0] = wpt;

	pt.z = distance;		//box[4];	// maxz

	wpt = transform.rotate_translate(pt);
	points[1] = wpt;
}
//---------------------------------------------------------------------------
// typedef float OBJBOX[6];	// maxx,minx,maxy,miny,maxz,minz  in object coordinates
//
IBaseObject * Fighter::checkFutureCollision (void)
{
	GRIDVECTOR points[2];
	COMPTR<ITerrainMap> map;
	TCallback callback(this);

	SECTOR->GetTerrainMap(systemID, map);
	getVFXPoints(points, transform, getDynamicsData().maxLinearVelocity);
	if (patrolState == KAMIKAZE)
		callback.kamikazeTarget = (target!=0) ? target.Ptr()->GetPartID() : 0;

	map->TestSegment(points[0], points[1], &callback);

	return OBJLIST->FindObject(callback.highest);
}
//---------------------------------------------------------------------------
// return TRUE if we fired weapon
//
bool Fighter::updateWeapon (void)
{
	bool result=0;
	if (refireTime < 0 || --refireTime==0)
	{
		TRANSFORM firing;
		Vector targetPos;
		bool bAlwaysHit;	// set by calc function

		if (supplies>0 && calculateTargetTransform(firing, targetPos, bAlwaysHit))
		{
			//do the real damage
			U32 damage = 0;
			U32 weaponVelocity = 0;
			BASE_WEAPON_DATA * pBaseWeapon = (BASE_WEAPON_DATA *) ARCHLIST->GetArchetypeData(pWeaponType);

			switch (pBaseWeapon->wpnClass)
			{
			case WPN_MISSILE:
			case WPN_BOLT:
			case WPN_PLASMABOLT:
			case WPN_ANMBOLT:
				damage = ((BT_PROJECTILE_DATA *)pBaseWeapon)->damage;
				weaponVelocity = ((BT_PROJECTILE_DATA *)pBaseWeapon)->maxVelocity;
				break;
			case WPN_GATTLINGBEAM:
			case WPN_BEAM:
				damage = ((BT_BEAM_DATA *)pBaseWeapon)->damage;
				weaponVelocity = 20000000;//((BT_BEAM_DATA *)pBaseWeapon)->velocity;
				break;
			case WPN_SPECIALBOLT:
				damage = ((BT_SPECIALBOLT_DATA *)pBaseWeapon)->damage;
				weaponVelocity = ((BT_SPECIALBOLT_DATA *)pBaseWeapon)->maxVelocity;
				break;
			case WPN_LASERSPRAY:
				weaponVelocity = ((BT_LASERSPRAY_DATA *)pBaseWeapon)->velocity;
				damage = ((BT_LASERSPRAY_DATA *)pBaseWeapon)->damage;
				break;
			default:
				CQBOMB0("Unhandled weapon type");
				damage = 0;
				break;
			}
			if(damage && owner.Ptr())
			{
				SINGLE dist = (targetPos-firing.translation).fast_magnitude();

				SINGLE time = dist/weaponVelocity;
				SCHEDULER->QueueDamageEvent(time,owner.Ptr()->GetPartID(), target.Ptr()->GetPartID(), damage);
			}		
			//do the visual

			if(bVisible || target.Ptr()->bVisible)
			{
				if(OBJLIST->CreateProjectile())
				{
		 			IBaseObject * obj;
		 			VOLPTR(IWeapon) bolt;

					obj = ARCHLIST->CreateInstance(pWeaponType);
					if (obj)
					{
						OBJLIST->AddObject(obj);
						if ((bolt = obj) != 0)
							bolt->InitWeapon(this, firing, target.Ptr(), (bAlwaysHit)?IWF_ALWAYS_HIT:IWF_ALWAYS_MISS, &targetPos);
					}
					else
					{
						OBJLIST->ReleaseProjectile();
					}
				}
			}
			refireTime = refirePeriod;
			supplies--;
			result=true;
		}
	}
	return result;
}
//---------------------------------------------------------------------------
// typedef float OBJBOX[6];	// maxx,minx,maxy,miny,maxz,minz  in object coordinates
//
bool Fighter::calculateTargetTransform (TRANSFORM & result, Vector & targetPos, bool & bAlwaysHit)
{
	bool bInRange=false;
	OBJBOX box;
	Vector offset, relVec;
	SINGLE distance;
	
	if (target.Ptr()->GetObjectBox(box))
	{
		offset.x = ((box[0]-box[1]) * (float(rand()) / float(RAND_MAX))) + box[1];
		offset.y = ((box[2]-box[3]) * (float(rand()) / float(RAND_MAX))) + box[3];
		offset.z = ((box[4]-box[5]) * (float(rand()) / float(RAND_MAX))) + box[5];
	}
	else
		offset.zero();

	targetPos = target.Ptr()->GetTransform().rotate_translate(offset);		// convert to world coordinates
	bAlwaysHit = false;

	relVec = targetPos - transform.translation;
	distance = relVec.magnitude();

	SINGLE relYaw = get_angle(relVec.x, relVec.y) - transform.get_yaw();

	if (relYaw < -PI)
		relYaw += PI*2;
	else
	if (relYaw > PI)
		relYaw -= PI*2;

	// in range and facing the right direction
	if (distance < weaponRange && fabs(relYaw) < 45*MUL_DEG_TO_RAD)
	{
		//
		// calculate lead
		//
		SINGLE time = distance / weaponVelocity;
		Vector diff = (target.Ptr()->GetVelocity() * time);
		targetPos += diff;
		relVec += diff;
		SINGLE angle = get_angle(relVec.x, relVec.y);

		int num = rand() & 255;
		SINGLE upgradeAccuracy;
		SINGLE admiralMod = 0;
		IBaseObject * obj = OBJLIST->FindObject(GetFighterOwner());
		if(obj)
		{
			MPart fighterOwner = obj;
			if(fighterOwner.isValid() && fighterOwner->fleetID)
			{
				VOLPTR(IAdmiral) flagship;
				OBJLIST->FindObject(fighterOwner->fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
				if(flagship.Ptr())
				{
					admiralMod = flagship->GetFighterTargetingBonus();
				}
			}
		}
		if(target.Ptr()->objClass == OC_SPACESHIP || target.Ptr()->objClass == OC_PLATFORM)
			upgradeAccuracy = baseGroundAccuracy * MGlobals::GetFighterUpgrade(playerID,MGlobals::GetPlayerRace(playerID)) + admiralMod;
		else
			upgradeAccuracy = baseAirAccuracy * MGlobals::GetFighterUpgrade(playerID,MGlobals::GetPlayerRace(playerID)) + admiralMod;
		if (num > int(upgradeAccuracy * 256))	// failed our competency check
		{
			if (num & 1)
				angle += 10 * MUL_DEG_TO_RAD;
			else
				angle -= 10 * MUL_DEG_TO_RAD;
		}
		else
			bAlwaysHit = true;


		result = TRANSFORM::WORLD;		// .rotate_about_i(90*MUL_DEG_TO_RAD);
		result.rotate_about_j(angle);
		result.translation = transform.translation;

		if (relVec.magnitude() < weaponRange)
			bInRange = true;
	}

	return bInRange;
}
//---------------------------------------------------------------------------
//
void Fighter::explode (void)
{
	state = DEAD;
	RemoveFighter(this);

	if (owner!=0)
	{
		UnregisterWatchersForObject(this);
		owner->OnFighterDestruction(index);	 // fighter hit by weapon
		OBJMAP->RemoveObjectFromMap(this,mapSysID,map_square);
		mapSysID = 0;
	}
	
	if (bVisible && pExplosionType && (owner!=0 || target!=0))
	{
		IBaseObject * obj = CreateBlast(pExplosionType, transform, systemID);
		CQASSERT(obj);
		OBJLIST->AddObject(obj);
	}
}
//---------------------------------------------------------------------------
// return TRUE if handled movement for this turn
//#define LOCKED		0		// locked behind leader
//#define BREAKING	1		// breaking away from leader
//#define LOOSE		2		// acting individually
//#define FORMING		3		// forming up with leader
//
bool Fighter::formationMove (IFighter * parent)
{
	bool result = false;
	
	switch (formationState)
	{
	case FORMING:
		{
			const TRANSFORM & trans = parent->GetTransform();
			Vector goal = trans.rotate_translate(formation[formationType][formationIndex]) - transform.translation;
			SINGLE relYaw = get_angle(goal.x, goal.y) - transform.get_yaw();
			if (relYaw < -PI)
				relYaw += PI*2;
			else
			if (relYaw > PI)
				relYaw -= PI*2;
			SINGLE relPitch = sqrt(goal.x * goal.x  + goal.y * goal.y);
			relPitch = get_angle(goal.z, relPitch) - transform.get_pitch();

			rotateShip(relYaw, 0 - transform.get_roll(), relPitch);

			// if heading away from leader, or leader heading towards us
			if (fabs(relYaw) > 10 * MUL_DEG_TO_RAD || dot_product(trans.get_k(), transform.get_k()) < 0)
			{
				setThrustersOn();
			}
			else
			{
				DEBUG_resetInputCounter();
				velocity = transform.get_k() * (-getDynamicsData().maxLinearVelocity * 1.333);
				if (goal.magnitude() < getDynamicsData().maxLinearVelocity * 1.333)
				{
					formationState = LOCKED;	//setFormationState(parent, LOCKED);
				}
			}

			result = true;
		}
		break;
		
	case LOCKED:
		{
			const TRANSFORM & trans = parent->GetTransform();
			DEBUG_resetInputCounter();

			if (bVisible)
			{
				Vector parentVel = parent->GetVelocity();
//				SINGLE parentVelMag = parentVel.fast_magnitude();

				SINGLE relYaw, relRoll, relPitch;
				relYaw = trans.get_yaw() - transform.get_yaw();
				if (relYaw < -PI)
					relYaw += PI*2;
				else
				if (relYaw > PI)
					relYaw -= PI*2;
				relPitch = trans.get_pitch() - transform.get_pitch();
				if (relPitch < -PI)
					relPitch += PI*2;
				else
				if (relPitch > PI)
					relPitch -= PI*2;
				relRoll = 0 - transform.get_roll();
				rotateShip(relYaw, relRoll, relPitch);

				Vector relVec = trans.rotate_translate(formation[formationType][formationIndex]) - transform.translation;
				relVec += (parentVel * ELAPSED_TIME);
				SINGLE relMag = relVec.fast_magnitude();
				if (relMag > 0)
				{
					SINGLE speed =  relMag;		 // or / ELAPSED_TIME;
					if (speed > getDynamicsData().maxLinearVelocity*1.333)
						speed = getDynamicsData().maxLinearVelocity*1.333;

					velocity = transform.get_k() * -speed;
					velocity += relVec * (getDynamicsData().maxLinearVelocity * 0.25 / relMag);
				}
			} // else not visible, just cheat
			else
			{
				transform = *static_cast<const Matrix*>(&trans);	  // do not copy translation
				transform.translation = trans.rotate_translate(formation[formationType][formationIndex]);
				velocity = parent->GetVelocity();
			}

			result = true;
		}
		break;

	case BREAKING:
		{
			const TRANSFORM & trans = parent->GetTransform();
			bool bRight = ((formationIndex & 1) != 0); // (formation[formationType][formationIndex].x > 0);
			Vector k = trans.get_k();
			SINGLE parentAngle = get_angle(-k.x, -k.y);
			k = transform.get_k();
			SINGLE myAngle = get_angle(-k.x, -k.y);
			SINGLE relYaw, relRoll, relPitch;
			
			if (bRight)
			{
				parentAngle += 30 * MUL_DEG_TO_RAD;
				relRoll = (0 * MUL_DEG_TO_RAD) - transform.get_roll();
			}
			else
			{
				parentAngle -= 30 * MUL_DEG_TO_RAD;
				relRoll = (-0 * MUL_DEG_TO_RAD) - transform.get_roll();
			}

			relYaw = parentAngle - myAngle;
			relPitch = trans.get_pitch() - transform.get_pitch();

			rotateShip(relYaw, relRoll, relPitch);
			setThrustersOn();

			// testing!!! fix this one
			if (fabs(relYaw) < 3 * MUL_DEG_TO_RAD)
				formationState = LOOSE;
			result = true;
		}
		break;
	}

	return result;
}
//---------------------------------------------------------------------------
//
void Fighter::AddFighter (Fighter * fighter)
{
	CQASSERT(fighter->prevFighter == 0);
	CQASSERT(fighter->nextFighter == 0);
	CQASSERT(fighter != fighterList);

	fighter->nextFighter = fighterList;

	if (fighterList)
		fighterList->prevFighter = fighter;

	fighterList = fighter;

//	OBJLIST->SetFighterList(fighterList);		// maintain global copy of ptr
}
//---------------------------------------------------------------------------
//
void Fighter::RemoveFighter (Fighter * fighter)
{
	if (fighter == fighterList)
	{
		fighterList = fighterList->nextFighter;
//		OBJLIST->SetFighterList(fighterList);		// maintain global copy of ptr
	}
	if (fighter->prevFighter)
		fighter->prevFighter->nextFighter = fighter->nextFighter;
	if (fighter->nextFighter)
		fighter->nextFighter->prevFighter = fighter->prevFighter;
	fighter->prevFighter = fighter->nextFighter = 0;
}
//---------------------------------------------------------------------------
//
IFighter * IFighter::FindFighter (U32 dwMissionID)
{
	Fighter * fighter = Fighter::fighterList;

	while (fighter)
	{
		if (fighter->dwMissionID == dwMissionID)
			break;
		fighter = fighter->nextFighter;
	}

	return fighter;
}
//---------------------------------------------------------------------------
//
S32 Fighter::GetHullPoints (void)
{
	return hullPoints;
}
//---------------------------------------------------------------------------
//
bool Fighter::TCallback::TerrainCallback (const struct FootprintInfo & info, struct GRIDVECTOR & pos)
{
	if (kamikazeTarget && kamikazeTarget == info.missionID)
		return false;	// don't look beyond kamikaze target

	if ((info.flags & (TERRAIN_PARKED|TERRAIN_BLOCKLOS)) != 0)
	{
		if (highest==0)
		{
			maxY = info.height;
			highest = info.missionID;
		}
		else
		{
			if (info.height > maxY)
			{
				maxY = info.height;
				highest = info.missionID;
			}
		}
	}

	return true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
inline struct IBaseObject * createFighter (const FIGHTER_INIT & data)
{
	Fighter * obj = new ObjectImpl<Fighter>;

	obj->FRAME_init(data);
	if (obj->init(data))
	{
		return obj;
	}

	delete obj;
	return 0;
}

//------------------------------------------------------------------------------------------
//---------------------------Fighter Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE FighterFactory : public IObjectFactory
{
	struct OBJTYPE : FIGHTER_INIT
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
			archIndex = -1;
		}

		~OBJTYPE (void)
		{
			delete dummyFighter;
			delete dummyInit;
			if (archIndex != -1)
				ENGINE->release_archetype(archIndex);
			if (pWeaponType)
				ARCHLIST->Release(pWeaponType, OBJREFNAME);
			if (pExplosionType)
				ARCHLIST->Release(pExplosionType, OBJREFNAME);
			if (pEngineTrailType)
				ARCHLIST->Release(pEngineTrailType, OBJREFNAME);
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(FighterFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	FighterFactory (void) { }

	~FighterFactory (void);

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

	/* FighterFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
FighterFactory::~FighterFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void FighterFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE FighterFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_FIGHTER)
	{
		BT_FIGHTER_DATA * data = (BT_FIGHTER_DATA *) _data;

		result = new OBJTYPE;
		result->pData = data;
		result->pArchetype = ARCHLIST->GetArchetype(szArchname);
		DAFILEDESC fdesc = data->fileName;
		COMPTR<IFileSystem> objFile;

		if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
			TEXLIB->load_library(objFile, 0);
		else
			goto Error;

		if ((result->archIndex = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX)
			goto Error;

		result->pWeaponType = ARCHLIST->LoadArchetype(data->weaponType);
		CQASSERT(result->pWeaponType);
		ARCHLIST->AddRef(result->pWeaponType, OBJREFNAME);

		if (data->explosionType[0])
		{
			result->pExplosionType = ARCHLIST->LoadArchetype(data->explosionType);
			CQASSERT(result->pExplosionType);
			ARCHLIST->AddRef(result->pExplosionType, OBJREFNAME);
		}

		if (data->engineTrailType[0])
		{
			result->pEngineTrailType = ARCHLIST->LoadArchetype(data->engineTrailType);
			CQASSERT(result->pEngineTrailType);
			ARCHLIST->AddRef(result->pEngineTrailType, OBJREFNAME);
		}

		SFXMANAGER->Preload(data->kamikazeYell);
		SFXMANAGER->Preload(data->fighterwhoosh);

		result->dummyInit = new DUMMY_INIT;
		result->dummyFighter = new ObjectImpl<DummyFighter>;
		result->dummyFighter->instanceIndex = ENGINE->create_instance2(result->archIndex,NULL);
		result->dummyFighter->FRAME_init(*result->dummyInit);
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
BOOL32 FighterFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		(does not allow for this)
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * FighterFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;
	return createFighter(*objtype);
}
//-------------------------------------------------------------------
//
void FighterFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _fighter : GlobalComponent
{
	FighterFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<FighterFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _fighter __ship;
//---------------------------------------------------------------------------
//--------------------------End Fighter.cpp----------------------------------
//---------------------------------------------------------------------------