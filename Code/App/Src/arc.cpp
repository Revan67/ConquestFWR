//--------------------------------------------------------------------------//
//                                                                          //
//                                 ArcCannon.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Jasony $

    $Header: /Conquest/App/Src/arc.cpp 97    11/02/00 1:24p Jasony $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "Camera.h"
#include "Sector.h"
#include "Objlist.h"
#include "sfx.h"
#include "IConnection.h"
#include "Startup.h"
#include "SuperTrans.h"
#include "DWeapon.h"
#include "IWeapon.h"
#include "IExplosion.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "Mesh.h"
#include "ArchHolder.h"
#include "Anim2d.h"
#include "FogOfWar.h"
#include "UserDefaults.h"
#include "Mission.h"
#include "OpAgent.h"
#include "IMissionActor.h"
#include <MGlobals.h>
#include "MPart.h"
#include "GridVector.h"
#include "ObjMapIterator.h"

#include <Renderer.h>
#include <TComponent.h>
#include <Engine.h>
#include <Vector.h>
#include <Physics.h>
#include <Matrix.h>
#include <IHardpoint.h>
#include <stdlib.h>
#include <FileSys.h>
#include <ICamera.h>
#include <Collision.h>
#include <Pixel.h>
//#include <RPUL\PrimitiveBuilder.h>
#include <IRenderPrimitive.h>
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
/*static void Transform_to_4x4 (float m[16], const Transform &t)
{
        m[ 0] = t.element[0][0];
        m[ 1] = t.element[1][0];
        m[ 2] = t.element[2][0];
        m[ 3] = 0;

        m[ 4] = t.element[0][1];
        m[ 5] = t.element[1][1];
        m[ 6] = t.element[2][1];
        m[ 7] = 0;

        m[ 8] = t.element[0][2];
        m[ 9] = t.element[1][2];
        m[10] = t.element[2][2];
        m[11] = 0;

        m[12] = t.element[0][3];
        m[13] = t.element[1][3];
        m[14] = t.element[2][3];
        m[15] = 1;
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct MeshSpark
{
	S32 start_ref,end_ref;
	S32 frame_ref;
};

#define MESH_SPARKS 9
#define MAX_TENDRILS 5 /*MAX_AOE_VICTIMS*/
#define MAX_SEGMENTS 35
#define BEAM_TICKS   6
#define DISTANCE_TICKS 4;

struct Tendril
{
	Vector start,end;
	OBJPTR<IWeaponTarget> target;
	OBJPTR<IBaseObject> source;
	TRANSFORM tobject_to_world;
	SINGLE offset[MAX_SEGMENTS];
	S32 frame_ref[MAX_SEGMENTS];
	int segments;
	MeshSpark meshSpark[MESH_SPARKS];
	int meshSparkCnt;

	SINGLE length;
	U8 zapLevel;
	BOOL32 bFeedBack:1;
	AnimArchetype *animArch;
	S32 damage;
	S32 targetID;
	S32 distanceCounter;

	//extent map stuff
	const RECT *extents;
	SINGLE z_step,z_min;
	U8 slices;

	void Render();
	void Update();
	void GenerateSparks();
	void Init();
};

void Tendril::Render()
{
	if (target == 0 || source == 0)
		return;

	U32 systemID = SECTOR->GetCurrentSystem();

	if (target.Ptr()->GetSystemID() != systemID || source->GetSystemID() != systemID)
		return;

	start = source->GetPosition();
	end = target.Ptr()->GetPosition();
	Vector direction = end-start;
	length = direction.magnitude();
	if(!length)
	{
		direction = Vector(0,0,1);
		length = 1;
	}
	direction /= length;
	segments = min(ceil(length*0.0012),MAX_SEGMENTS);
	
	if (distanceCounter > 1)
	{
		length -= length*distanceCounter/5;
		distanceCounter--;
	}

	if (FOGOFWAR->CheckVisiblePosition(start) || target.Ptr()->IsVisibleToPlayer(MGlobals::GetThisPlayer()) ||
	    (DEFAULTS->GetDefaults()->bVisibilityRulesOff || DEFAULTS->GetDefaults()->bEditorMode) )
	{
#define TOLERANCE 0.00001f
		
		Vector v0,v1,v2,v3;
		Vector cpos = MAINCAM->get_position();
		
#define BM_WDTH 140.0  // value was 255.0
		SINGLE width = BM_WDTH;
		//	width += (U8)(12*test);
		//	SINGLE mag,mag2;
		
		Vector look (cpos - start);
	
		//this is the draw code that makes the arcCannon always visible
		//look = direction;
		
		Vector i = cross_product(look,direction);//(look.y, -look.x, 0);
		
		if (fabs (i.x) < TOLERANCE && fabs (i.y) < TOLERANCE)
		{
			i.x = 1.0f;
		}
		i.normalize ();
		
			
		U8 c;

		//	PrimitiveBuilder PB(PIPE);
		PB.Color3ub(255,255,255);

		if (!bFeedBack)
		{
			Vector control;
			control = start + direction*length/3;
			control.z += 1000;

			Vector ptBegin;
			Vector ptEnd;

			for (c = 0; c < segments; c++)
			{
				SINGLE t = c/float(segments);
				SINGLE u = (c+1)/float(segments);

				// quadratic bezier
				ptBegin = (1-t)*(1-t)*start + 2*(1-t)*t*control + t*t*end;
				ptEnd = (1-u)*(1-u)*start + 2*(1-u)*u*control + u*u*end;
				
				v0 = (ptBegin - (i * (width-offset[c]+(SINGLE)rand()/RAND_MAX*20)));
				v1 = (ptBegin + (i * (width+offset[c]+(SINGLE)rand()/RAND_MAX*20)));
				v2 = (ptEnd + (i * (width+offset[c+1]+(SINGLE)rand()/RAND_MAX*20)));
				v3 = (ptEnd - (i * (width-offset[c+1]+(SINGLE)rand()/RAND_MAX*20)));
				
				AnimFrame *frame = &animArch->frames[frame_ref[c]];
////			BATCH->set_texture_stage_texture( 0,frame->texture);

				BATCH->set_state(RPR_STATE_ID,frame->texture);
				SetupDiffuseBlend(frame->texture,TRUE);

				//stupid correction factor here to eliminate seams
				PB.Begin(PB_QUADS);
				PB.TexCoord2f(frame->x0,frame->y0);     PB.Vertex3f(v0.x,v0.y,v0.z);
				PB.TexCoord2f(frame->x1,frame->y0);		PB.Vertex3f(v1.x,v1.y,v1.z);
				PB.TexCoord2f(frame->x1,frame->y1);		PB.Vertex3f(v2.x,v2.y,v2.z);
				PB.TexCoord2f(frame->x0,frame->y1);		PB.Vertex3f(v3.x,v3.y,v3.z);

				PB.End();
			}
		}
		
		//	Transform object_to_world = ENGINE->get_transform(target.ptr->GetObjectIndex());
		tobject_to_world = target.Ptr()->GetTransform();
		//	CAMERA->SetGLRenderVolume(&object_to_world);
		for (c=0;c<MESH_SPARKS;c++)
		{
			Vector start;
			S32 slice = meshSpark[c].start_ref >> 16;
			switch (meshSpark[c].start_ref & 0xffff)
			{
			case 0:
				start.x = extents[slice].left;
				start.y = extents[slice].top;
				break;
			case 1:
				start.x = extents[slice].right;
				start.y = extents[slice].top;
				break;
			case 2:
				start.x = extents[slice].right;
				start.y = extents[slice].bottom;
				break;
			case 3:
				start.x = extents[slice].left;
				start.y = extents[slice].bottom;
				break;
			}
			start.z = slice*z_step+z_min;
			
			slice = meshSpark[c].end_ref >> 16;
			switch (meshSpark[c].end_ref & 0xffff)
			{
			case 0:
				end.x = extents[slice].left;
				end.y = extents[slice].top;
				break;
			case 1:
				end.x = extents[slice].right;
				end.y = extents[slice].top;
				break;
			case 2:
				end.x = extents[slice].right;
				end.y = extents[slice].bottom;
				break;
			case 3:
				end.x = extents[slice].left;
				end.y = extents[slice].bottom;
				break;
			}
			end.z = slice*z_step+z_min;
			start = tobject_to_world.rotate_translate(start);
			end = tobject_to_world.rotate_translate(end);
			
			//width = (BM_WDTH/400.0)*(end-start).magnitude();
			look = cpos-start;
			
			//i.set (look.y, -look.x, 0);
			i = cross_product(look,(end-start));
			
			if (fabs (i.x) < TOLERANCE && fabs (i.y) < TOLERANCE)
			{
				i.x = 1.0f;
			}
			i.normalize ();
			
			v0 = (start - (i * (width+(SINGLE)rand()/RAND_MAX*2)));
			v1 = (start + (i * (width+(SINGLE)rand()/RAND_MAX*2)));
			v2 = (end + (i * (width+(SINGLE)rand()/RAND_MAX*2)));
			v3 = (end - (i * (width+(SINGLE)rand()/RAND_MAX*2)));
			
			
			
			AnimFrame *frame = &animArch->frames[meshSpark[c].frame_ref];
//			BATCH->set_texture_stage_texture( 0,frame->texture);
			SetupDiffuseBlend(frame->texture,TRUE);
			PB.Begin(PB_QUADS);
			PB.TexCoord2f(frame->x0,frame->y0);     PB.Vertex3f(v0.x,v0.y,v0.z);
			PB.TexCoord2f(frame->x1,frame->y0);		PB.Vertex3f(v1.x,v1.y,v1.z);
			PB.TexCoord2f(frame->x1,frame->y1);		PB.Vertex3f(v2.x,v2.y,v2.z);
			PB.TexCoord2f(frame->x0,frame->y1);		PB.Vertex3f(v3.x,v3.y,v3.z);
			PB.End();
		}
		
	}
#undef TOLERANCE
}

void Tendril::Update()
{
	if (target == 0 || source == 0)
		return;

	if (rand()%2)
	{
		U8 i;
		if (!bFeedBack)
		{
			length = (end-start).magnitude();
			segments = min(ceil(length*0.0012),MAX_SEGMENTS);
			frame_ref[0] = rand()%3;
			for (i=1;i<segments;i++)
			{
				offset[i] = 400.0*(SINGLE)rand()/RAND_MAX-200.0;
				frame_ref[i] = rand()%3;
			}
		}
		for (i=0;i<MESH_SPARKS;i++)
		{
			meshSpark[i].frame_ref = rand()%3;
		}

	//	segment_length = length/segments;
	}
}

void Tendril::GenerateSparks ()
{
	//current code, hi bits = slice, lo bits = corner
	meshSpark[0].start_ref = 0;
	S32 j;
	j = 0;
	while (j<MESH_SPARKS)
	{
		meshSpark[j].end_ref = meshSpark[j].start_ref + 0x10001;
		meshSpark[j].end_ref &= 0xff0003;
			
		j++;
		if (j<MESH_SPARKS)
			meshSpark[j].start_ref = meshSpark[j-1].end_ref;
	}
}

void Tendril::Init ()
{


	distanceCounter = DISTANCE_TICKS;

	if (!bFeedBack)
	{
//		direction = end-start;
//		direction.normalize();
		//	HARCH archIndex = target.ptr->GetObjectIndex();
		//	enemyMesh = REND->get_archetype_mesh(archIndex);
		
		//	GenerateSparkMesh(target->GetObjectIndex());
		//	GenerateExtents(target->GetObjectIndex());
		
		
		length = (end-start).magnitude();
		segments = min(ceil(length*0.0012),MAX_SEGMENTS);
//		segment_length = length/segments;
		
		offset[0] = 0;
		frame_ref[0] = rand()%3;
		for (int i=1;i<segments;i++)
		{
			offset[i] = 400.0*(SINGLE)rand()/RAND_MAX-200.0;
			frame_ref[i] = rand()%3;
		}
		
	}

	OBJPTR<IExtent> extentObj;
	target->QueryInterface(IExtentID,extentObj);
	CQASSERT(extentObj);
	extentObj->GetExtentInfo(extents,&z_step,&z_min,&slices);
	CQASSERT(extents && "No extent info!?");
	GenerateSparks();
}

/*
void Tendril::DamageTarget()
{
	Vector dummy;
	dummy.zero();
	target->ApplyDamage(NULL, dummy, dummy, data->damage,0);
}
*/


struct ARC_INIT
{
	S32 archIndex;
	IMeshArchetype * meshArch;
	PARCHETYPE pArchetype;
	AnimArchetype *animArch;
};

struct _NO_VTABLE ArcCannon : public ObjectTransform<ObjectFrame<IBaseObject,struct ARC_SAVELOAD,struct ARC_INIT> >, IAOEWeapon
{
	BEGIN_MAP_INBOUND(ArcCannon)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IAOEWeapon)
	END_MAP()

	//------------------------------------------
	U32 ownerID;
	U32 systemID;
	
	SINGLE time;
	AnimArchetype *animArch;
	const BT_ARC_DATA * data;
	HSOUND hSound;
	Tendril tendril[MAX_TENDRILS];
	S32 numTendrils;
	S32 beamCounter;
	SINGLE timePassed;
	BOOL32 bFeedBack:1;
	U32 *_partIDs;
	U32 *_damage;
	
	
	Mesh *enemyMesh;
	//mesh map stuff

#define R_STEPS 3
#define T_STEPS 9
	S32 meshMap[R_STEPS][T_STEPS];


	//------------------------------------------

	ArcCannon (void) 
	{
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~ArcCannon (void);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual void Render (void);

	virtual U32 GetSystemID (void) const;

	virtual U32 GetPlayerID (void) const
	{
		return MGlobals::GetPlayerFromPartID(ownerID);
	}

	/* IArcWeapon methods */

	virtual void InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * obj, U32 flags, const class Vector * pos=0);

	// weapon should determine who it will damage, and how much, then return the result to the caller
	virtual U32 GetAffectedUnits (U32 partIDs[MAX_AOE_VICTIMS], U32 damage[MAX_AOE_VICTIMS]);

	// caller has determined who it will damage, and how much.
	virtual void SetAffectedUnits (const U32 partIDs[MAX_AOE_VICTIMS], const U32 damage[MAX_AOE_VICTIMS]);


	///////////////////

	void init (ARC_INIT &initData);
	
//	void GenerateSparkMesh(INSTANCE_INDEX index);

//	void GenerateExtents(INSTANCE_INDEX index);

//	void GenerateSparks();
};

//----------------------------------------------------------------------------------
//
ArcCannon::~ArcCannon (void)
{
//	TXMLIB->release_texture(textureID);
/*	for (int i=0;i<3;i++)
		textureID[i] = 0;*/
	if (hSound)
		SFXMANAGER->CloseHandle(hSound);
}
//----------------------------------------------------------------------------------
//
U32 ArcCannon::GetSystemID (void) const
{
	return systemID;
}
//----------------------------------------------------------------------------------
//
void ArcCannon::PhysicalUpdate (SINGLE dt)
{
	time-= dt;

	timePassed += dt;
	if (timePassed > 0.1)
	{
		timePassed = 0;
		for (int i=0;i<numTendrils;i++)
		{
			tendril[i].Update();
		}
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 ArcCannon::Update (void)
{
	BOOL32 result = 1;

	if (time <= 0)
	{
		OBJPTR<IWeaponTarget> target;
		for (int i=0;i<numTendrils;i++)
		{
			Vector dummy;
		//	tendril[i].DamageTarget();
			dummy.zero();
			IBaseObject *obj = OBJLIST->FindObject(tendril[i].targetID);
			if (obj && obj->QueryInterface(IWeaponTargetID,target,NONSYSVOLATILEPTR) != 0)
			{
				if (target)
				{
					if(data->drainSupplies)
					{
						if(THEMATRIX->IsMaster())
						{
							MPartNC part = target.Ptr();
							if(part.isValid())
							{
								U32 ownerSupplies;
								if(part->supplies >= tendril[i].damage)
									ownerSupplies = tendril[i].damage;
								else
									ownerSupplies = part->supplies;

								S32 newSupplies = part->supplies-tendril[i].damage;
								if(newSupplies < 0)
									newSupplies = 0;
								part->supplies = newSupplies;
							}
						}
					}
					else
					{
						//target->ApplyDamage(this, ownerID, dummy, dummy,tendril[i].damage&~KILL_BIT,0);
						target->ApplyAOEDamage(ownerID,tendril[i].damage);
					}
				}
			}
		}
			
		result = 0;            //we are done!
	}

	return result;
}
//----------------------------------------------------------------------------------
//
void ArcCannon::Render (void)
{
	if (bVisible && numTendrils)
	{
		BATCH->set_state(RPR_BATCH,TRUE);
		CAMERA->SetModelView();
		
		BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
		BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_ONE);
		BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
		BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
		
		int nTend = 1;

		if (beamCounter < 0)
		{
			nTend = numTendrils;
		}
		else
		{
			beamCounter--;  //this is pretty much random nowadays
		}

		for (int i = 0; i < nTend; i++)
		{
			tendril[i].Render();
		}
		BATCH->set_state(RPR_STATE_ID,0);
	}
}
//----------------------------------------------------------------------------------
//
void ArcCannon::InitWeapon (IBaseObject * owner, const class TRANSFORM & orientation, IBaseObject * _target, U32 flags, const class Vector * pos)
{
	//Correct arcCannon to fire at target regardless of gun barrel
//	SINGLE curPitch, desiredPitch, relPitch;
//	Vector goal = (_target)? _target->GetPosition() : *pos;

	systemID = owner->GetSystemID();
	ownerID = owner->GetPartID();
	U32 visFlags = owner->GetTrueVisibilityFlags();

	if (_target)
	{
		_target->QueryInterface(IWeaponTargetID, tendril[0].target, NONSYSVOLATILEPTR);
		CQASSERT(tendril[0].target!=0);
		visFlags |= _target->GetTrueVisibilityFlags();
		owner->SetVisibleToAllies(1 << (_target->GetPlayerID()-1));
		_target->SetVisibleToAllies(1 << (owner->GetPlayerID()-1));
	}
	owner->QueryInterface(IBaseObjectID,tendril[0].source,NONSYSVOLATILEPTR);
	SetVisibilityFlags(visFlags);

/*	curPitch = orientation.get_pitch();
	goal -= orientation.get_position();

	desiredPitch = sqrt(goal.x * goal.x  + goal.y * goal.y);
	desiredPitch = get_angle(goal.z, desiredPitch);

	relPitch = desiredPitch - curPitch;

	transform = orientation;
	transform.rotate_about_i(relPitch);*/

	tendril[0].start = orientation.get_position();
//	tendril[0].direction = -transform.get_k();
	Vector start = orientation.get_position();

	hSound = SFXMANAGER->Open(data->launchSfx);
	SFXMANAGER->Play(hSound,systemID,&start);

//	tendril[0].data = data;
	tendril[0].zapLevel = 0;
	

//	tendril[0].length = (tendril[0].end-tendril[0].start).magnitude();
//	CQASSERT(tendril[0].length > 0);
//	if (tendril[0].length < data->feedbackRange)
//		bFeedBack = TRUE;
	//tendril[0].Init();
//	numTendrils = 1;
	
}
//---------------------------------------------------------------------------
//
U32 ArcCannon::GetAffectedUnits (U32 partIDs[MAX_AOE_VICTIMS], U32 damage[MAX_AOE_VICTIMS])
{
	CQASSERT(tendril[0].target);

	const U32 allyMask = MGlobals::GetAllyMask(ownerID & PLAYERID_MASK);

	beamCounter = BEAM_TICKS;

	tendril[0].targetID = tendril[0].target.Ptr()->GetPartID();
	tendril[0].end = tendril[0].target.Ptr()->GetPosition();
	tendril[0].damage = data->damage;
	partIDs[0] = tendril[0].targetID;
	if(damage)
		damage[0] = data->damage;

	// need a vector and a float to help define the plane.  Other targets, *must* be on the positvie side of this
	// plane to be hit 
	Vector norm = tendril[0].end - tendril[0].start;
	norm.z = 0;
	norm.normalize();
	const SINGLE fDist = dot_product(tendril[0].end, norm) - 0.20f;

	numTendrils = 1;

	//numTendrils will increase as this loop runs
	if(tendril[0].target.Ptr()->GetSystemID() == systemID)
	{
		for (int i = 0; i < numTendrils; i++)
		{
			if (tendril[i].zapLevel < data->maxTendrilStages && numTendrils < (int)data->maxTargets)
			{
				ObjMapIterator it(tendril[0].target.Ptr()->GetSystemID(), tendril[0].target.Ptr()->GetPosition(), 5.0f*GRIDSIZE);

				while (it && (it->flags & OM_TARGETABLE) == 0)
				{
					++it;
				}
				
				while (it && numTendrils < MAX_TENDRILS)
				{
					// make sure that we are not hitting an ally
					const U32 hisPlayerID = it.GetApparentPlayerID(allyMask);//it->dwMissionID & PLAYERID_MASK;

					// if we are enemies
					if (hisPlayerID && (((1 << (hisPlayerID-1)) & allyMask) == 0))
					{
						GRIDVECTOR gVect;
						gVect = tendril[i].end;
						SINGLE dist = it->obj->GetGridPosition() - gVect;
						
						gVect = tendril[0].start;
						SINGLE totalDist = it->obj->GetGridPosition() - gVect;
						
						// test to see if the object is on the proper side of the plane to be a valid target
						bool bValidTarget = (dot_product(norm, it->obj->GetPosition()) > fDist);

						if (dist < data->jumpRange && totalDist < data->jumpRange*MAX_TENDRILS && bValidTarget)
						{
							if (it->obj->GetPartID() != ownerID)
							{
								S8 tagged = -1;
								for (int c=0; c<numTendrils; c++)
								{
									if (tendril[c].target.Ptr() == it->obj)
										tagged = c;
								}
								
								if (tagged == -1)
								{
									if (numTendrils < MAX_TENDRILS)
									{
										if (it->obj->QueryInterface(IWeaponTargetID,tendril[numTendrils].target, NONSYSVOLATILEPTR))
										{
											if (damage)
												damage[numTendrils] = 0;
											if (tendril[numTendrils].target.Ptr()->GetSystemID() == systemID)
											{
												tendril[i].target.Ptr()->QueryInterface(IBaseObjectID,tendril[numTendrils].source,NONSYSVOLATILEPTR);
												CQASSERT(tendril[numTendrils].target != 0);
												
												tendril[numTendrils].length = dist;
												tendril[numTendrils].end = it->obj->GetPosition();
												tendril[numTendrils].zapLevel = tendril[i].zapLevel + 1;
												tendril[numTendrils].start = tendril[i].end;
												
												partIDs[numTendrils] = it->obj->GetPartID();
												tendril[numTendrils].targetID = partIDs[numTendrils];
												tendril[numTendrils].damage = data->damage*(1-tendril[numTendrils].zapLevel*data->damageDrop);
												if(damage)
													damage[numTendrils] = tendril[numTendrils].damage;
											}
											numTendrils++;
										}
									}
								}
								else
								{
									if (dist && dist < tendril[tagged].length && tendril[i].zapLevel <= tendril[tagged].zapLevel)
									{
										//this code will reassign an already targeted ship to be hit by a different piece of lightning
										//tendril[tagged].target = obj;  //should be redundant
										tendril[tagged].length = dist;
										tendril[tagged].end = it->obj->GetPosition();
										tendril[tagged].zapLevel = tendril[i].zapLevel + 1;
										tendril[tagged].start = tendril[i].end;
										tendril[i].target.Ptr()->QueryInterface(IBaseObjectID,tendril[tagged].source,NONSYSVOLATILEPTR);
									}
								}
							}
						}
					}
					do
					{
						++it;
					}
					while (it && (it->flags & OM_TARGETABLE) == 0);
				}
			}
		}
	}

	S32 result = numTendrils;
	
	if (bFeedBack && numTendrils < MAX_TENDRILS)
	{
		tendril[numTendrils].bFeedBack = TRUE;
		IBaseObject *owner = OBJLIST->FindObject(ownerID);
		CQASSERT(owner && "Something is very wrong");
		if (owner)
		{
			if (owner->QueryInterface(IWeaponTargetID,tendril[numTendrils].target, NONSYSVOLATILEPTR))
			{
				tendril[numTendrils].targetID = tendril[numTendrils].target.Ptr()->GetPartID();
				CQASSERT(tendril[numTendrils].target != 0);
				numTendrils++;
			}
		}
	}

	for (int i=0; i < numTendrils; i++)
	{
		tendril[i].Init();
		tendril[i].animArch = animArch;
	}

	if (numTendrils < MAX_AOE_VICTIMS)
		partIDs[numTendrils] = 0;

	return result;
}
//---------------------------------------------------------------------------
//
void ArcCannon::SetAffectedUnits (const U32 partIDs[MAX_AOE_VICTIMS], const U32 damage[MAX_AOE_VICTIMS])
{
	int cnt=0;
	numTendrils = 0;
	
	beamCounter = BEAM_TICKS;

	IBaseObject *obj=0;
	while (obj == 0 && partIDs[cnt] && cnt < MAX_AOE_VICTIMS)
	{
		obj = OBJLIST->FindObject(partIDs[cnt]);
		if (obj)
		{
			tendril[0].targetID = partIDs[cnt];
			if (obj->QueryInterface(IWeaponTargetID,tendril[0].target, NONSYSVOLATILEPTR))
			{
				if(damage)
					tendril[0].damage = damage[cnt];
				else
					tendril[0].damage = 0;
				if(tendril[0].source)
					tendril[0].start = tendril[0].source->GetPosition();
				else
					tendril[0].start = tendril[0].target.Ptr()->GetPosition();
				tendril[0].end = tendril[0].target.Ptr()->GetPosition();
			}
		}
		cnt++;
	}

	//No valid targets left?
	if (obj == 0)
	{
		time = 0;
		return;
	}

	CQASSERT(tendril[0].target);

	numTendrils = 1;
	
	int i;
	for (i=0;i<numTendrils;i++)
	{
		cnt = 1;
		//0 indicates the end of the submitted list
		while (partIDs[cnt] && cnt < MAX_AOE_VICTIMS)
		{
			IBaseObject *obj = OBJLIST->FindObject(partIDs[cnt]);
			if (obj)
			{
				Vector diff = obj->GetPosition()-tendril[i].end;
				diff.z = 0;
				SINGLE dist=diff.magnitude();
				diff = obj->GetPosition()-tendril[0].start;
				diff.z = 0;
				//	SINGLE totalDist = diff.magnitude();
				
				S8 tagged = -1;
				for (int c=0;c<numTendrils;c++)
				{
					if (tendril[c].target.Ptr() == obj)
						tagged = c;
				}
				
				if (tagged == -1)
				{
					CQASSERT(i == 0);
					CQASSERT(numTendrils < MAX_TENDRILS);
					obj->QueryInterface(IWeaponTargetID,tendril[numTendrils].target, NONSYSVOLATILEPTR);
					tendril[numTendrils].targetID = partIDs[cnt];
					tendril[i].target.Ptr()->QueryInterface(IBaseObjectID,tendril[numTendrils].source,NONSYSVOLATILEPTR);
					CQASSERT(tendril[numTendrils].target != 0);
					tendril[numTendrils].length = dist;
					tendril[numTendrils].end = obj->GetPosition();
					tendril[numTendrils].zapLevel = tendril[i].zapLevel + 1;
					tendril[numTendrils].start = tendril[i].end;
					if(damage)
						tendril[numTendrils].damage = damage[cnt];
					else
						tendril[numTendrils].damage = 0;
					
					numTendrils++;
				}
				else
				{
					if (dist && dist < tendril[tagged].length && tendril[i].zapLevel <= tendril[tagged].zapLevel)
					{
						//this code will reassign an already targeted ship to be hit by a different piece of lightning
						//tendril[tagged].target = obj;  //should be redundant
						tendril[tagged].length = dist;
						tendril[tagged].end = obj->GetPosition();
						tendril[tagged].zapLevel = tendril[i].zapLevel + 1;
						tendril[tagged].start = tendril[i].end;
						tendril[i].target.Ptr()->QueryInterface(IBaseObjectID,tendril[tagged].source,NONSYSVOLATILEPTR);
					}
				}
			}
			cnt++;
		}
	}

	if (bFeedBack && numTendrils < MAX_TENDRILS)
	{
		tendril[numTendrils].bFeedBack = TRUE;
		IBaseObject *owner =OBJLIST->FindObject(ownerID);
		if (owner)
		{
			owner->QueryInterface(IWeaponTargetID,tendril[numTendrils].target, NONSYSVOLATILEPTR);
			tendril[numTendrils].targetID = tendril[numTendrils].target.Ptr()->GetPartID();
			CQASSERT(tendril[numTendrils].target != 0);
			numTendrils++;
		}
		else
			bFeedBack = FALSE;
	}

	for (i=0;i<numTendrils;i++)
	{
		tendril[i].Init();
		tendril[i].animArch = animArch;
	}
}
//---------------------------------------------------------------------------
//
void ArcCannon::init (ARC_INIT &initData)
{
	FRAME_init(initData);
	data = (const BT_ARC_DATA *) ARCHLIST->GetArchetypeData(initData.pArchetype);

	CQASSERT(data);
	CQASSERT(data->wpnClass == WPN_ARC);
	CQASSERT(data->objClass == OC_WEAPON);

	time = data->period;
	/*if ((textureID = TXMLIB->get_texture_id(data->fileName)) == 0)
	{
		CQASSERT(data->fileName[0]);
		if ((textureID = CreateTextureFromFile(data->fileName, TEXTURESDIR, DA::TGA, 3)) != 0)
		{
			glBindTexture(GL_TEXTURE_2D,textureID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D,0);
			TXMLIB->set_texture_id(data->fileName, textureID);
			TXMLIB->get_texture_id(data->fileName);	// add 1 reference
		}
	}*/
	
	animArch = initData.animArch;
	pArchetype = initData.pArchetype;
	objClass = OC_WEAPON;
}
//---------------------------------------------------------------------------
//
/*void CreateArcCannonArchetype()
{
	if (!arch)
	{
		COMPTR<IFileSystem> file;
		DAFILEDESC fdesc = "lancerbolt.anm";
		
		fdesc.lpImplementation = "UTF";
		
		
		if (OBJECTDIR->CreateInstance(&fdesc, file) != GR_OK)
		{
			CQERROR1("Failed to open file %s", fdesc.lpFileName);
			return;
		}
		
		if ((arch = ANIM2D->create_archetype(file)) == 0)
		{
			CQERROR0("Failed to create arc cannon archetype");
			return;
		}
	}
}*/
//------------------------------------------------------------------------------------------
//---------------------------ArcCannon Factory----------------------------------------------
//------------------------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE ArcCannonFactory : public IObjectFactory
{
	struct OBJTYPE : ARC_INIT
	{
		void * operator new (size_t size)
		{
			return calloc(size,1);
		}
		
		OBJTYPE (void)
		{
			archIndex = -1;
		}
		
		~OBJTYPE (void)
		{
			if (animArch)
			{
				delete animArch;
				animArch = 0;
			}
		}
	};
	//------------------------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ArcCannonFactory)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

	ArcCannonFactory (void) { }

	~ArcCannonFactory (void);

	void init (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	// IObjectFactory methods 

	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);

	// ArcCannonFactory methods 

	IDAComponent * getBase (void)
	{
		return static_cast<IObjectFactory *> (this);
	}
};
//--------------------------------------------------------------------------//
//
ArcCannonFactory::~ArcCannonFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST && OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//--------------------------------------------------------------------------//
//
void ArcCannonFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
		connection->Advise(getBase(), &factoryHandle);
}
//-----------------------------------------------------------------------------
//
HANDLE ArcCannonFactory::CreateArchetype (const char *szArchname, OBJCLASS objClass, void *_data)
{
	OBJTYPE * result = 0;

	if (objClass == OC_WEAPON)
	{
		BT_ARC_DATA * data = (BT_ARC_DATA *)_data;
		if (data->wpnClass == WPN_ARC)
		{
			result = new OBJTYPE;
			
			
//			newguy->name = szArchname;
			//result->data = data;
			result->pArchetype = ARCHLIST->GetArchetype(szArchname);
			//
			// force preload of sound effect
			// 
			SFXMANAGER->Preload(data->launchSfx);
			
			COMPTR<IFileSystem> file;
			DAFILEDESC fdesc = data->fileName;//"lancerbolt.anm";

			fdesc.lpImplementation = "UTF";
			
			
			if (OBJECTDIR->CreateInstance(&fdesc, file) != GR_OK)
			{
				CQERROR1("Failed to open file %s", fdesc.lpFileName);
				goto Error;
			}
			
			if ((result->animArch = ANIM2D->create_archetype(file)) == 0)
			{
				CQERROR0("Failed to create arc cannon archetype");
				goto Error;
			}

			goto Done;
		}
	}

Error:
	delete result;
	result = 0;
Done:
	return (HANDLE) result;
}
//-------------------------------------------------------------------
//
BOOL32 ArcCannonFactory::DestroyArchetype (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

//	EditorStopObjectInsertion(objtype->pArchetype);		(does not allow for this)
	delete objtype;

	return 1;
}
//-------------------------------------------------------------------
//
IBaseObject * ArcCannonFactory::CreateInstance (HANDLE hArchetype)
{
	OBJTYPE * objtype = (OBJTYPE *)hArchetype;

	ArcCannon * arcCannon = new ObjectImpl<ArcCannon>;

	arcCannon->init(*objtype);

	return arcCannon;
}
//-------------------------------------------------------------------
//
void ArcCannonFactory::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
	// does not support this!
}
//---------------------------------------------------------------------------
//-------------------------GLOBAL STARTUP------------------------------------
//---------------------------------------------------------------------------
//
struct _arcCannon : GlobalComponent
{
	ArcCannonFactory * sfactory;

	virtual void Startup (void)
	{
		sfactory = new DAComponent<ArcCannonFactory>;
		AddToGlobalCleanupList((IDAComponent **) &sfactory);
	}

	virtual void Initialize (void)
	{
		sfactory->init();
	}
};

static _arcCannon __aCannon;
//---------------------------------------------------------------------------
//------------------------End Arc.cpp----------------------------------------
//---------------------------------------------------------------------------


