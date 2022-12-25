//--------------------------------------------------------------------------//
//                                                                          //
//                               Explosion.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Conquest/App/Src/explosion.cpp 141   10/23/00 11:05p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include <MGlobals.h>
#include "TObject.h"
#include "Camera.h"
#include "Sector.h"
#include "Objlist.h"
#include "sfx.h"
#include "DExplosion.h"
#include "IConnection.h"
#include "Startup.h"
#include "IExplosion.h"
#include "SuperTrans.h"
#include "Anim2d.h"
#include "TObjTrans.h"
#include "IBlast.h"
#include "ArchHolder.h"
#include "FogOfWar.h"
#include "mesh.h"
#include "INugget.h"
#include "UserDefaults.h"
#include "DMBaseData.h"
#include "IMissionActor.h"
#include "IHarvest.h"
#include "OpAgent.h"
#include "MeshExplode.h"
#include "MeshRender.h"
#include "ObjMap.h"
#include "MPart.h"

#include <TComponent.h>
#include <lightman.h>
#include <Engine.h>
#include <FileSys.h>
#include <Vector.h>
#include <Physics.h>
#include <Renderer.h>

#include <stdlib.h>

#define RANGE 6000
#define SMALLRANGE 2000
#define MAX_ANIMS 12

#define FOG_PAUSE 2.0

#define MAX_SHRAPNEL 48
#define MAX_BLASTS 48
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE Explosion : public IBaseObject, IExplosion
{

	BEGIN_MAP_INBOUND(Explosion)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IExplosion)
	END_MAP()

	//------------------------------------------

//	static HSOUND hSound;
//	static soundUsers;
//	static HSOUND sm_sound;
//	static sm_soundUsers;

	SINGLE timeToLive,totalTime;
//	BaseLight flash;
	U32 range;
	OBJPTR<IExplosionOwner> owner;
	INSTANCE_INDEX ownerIdx;
	AnimInstance *anim[MAX_ANIMS];
	BT_MESH_EXPLOSION *data;
	U8 cnt,num_booms;
	MeshChain *children;
	int numChildren;
	IMeshInfoTree *mesh_info;

    PARCHETYPE secondaryBlastType;
    PARCHETYPE pieceBlastType;
    PARCHETYPE catastrophicBlastType;
	PARCHETYPE rippleBlastType;
	ARCHETYPE_INDEX fireTrail;

	//------------------------------------------

	//
	// explosion data
	//

	S32 deathPause;
	SINGLE explosionTime;

	S32 explosionCounter;
//	SINGLE rollTarget, pitchTarget;
//	SINGLE listTarget;
	SINGLE lst_roll,lst_pitch,lst_list;
	SINGLE bounding_box[6];
	S16 num_first_children;
	U16 boom_count;
	Vector boom_axis;
	SINGLE boom_length;
	Vector fall_vel;

	Vector boom_pos;
	HEXPLODE handle;

	Vector blast_vel;
	U32 blast_speed;

	MeshChain * shrapnel[MAX_SHRAPNEL];
	IBaseObject *blasts[MAX_BLASTS];
	PHYS_CHUNK shrap_phys[MAX_SHRAPNEL]; //big array
	U16 shrapnel_cnt;
	U16 blast_cnt;
	SINGLE total_mass;
	Vector c_of_mass;

	U32 systemID;
	U32 playerID;
	U32 dwMissionID;
	U16 sensorRadius;
	bool bRipple:1;
	bool bChunk:1;
	bool bListing:1;
	bool bSmallBooms:1;
	bool bSpinner:1;
	bool dying:1;
	bool minimal:1;
	bool notOnObjlist:1;
	bool dead:1;

	U32 objMapSquare;
	U32 objMapSystemID;
 
	//------------------------------------------

	Explosion (void) //: flash(ENGINE)
	{
		timeToLive = totalTime = 1.4F;
		EXPCOUNT++;
		//timeToLive = 4004040;
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual ~Explosion (void);	// See ObjList.cpp

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt);

	virtual SINGLE TestHighlight (const RECT & rect)	// set bHighlight if possible for any part of object to appear within rect
	{
		bHighlight = FALSE;
		return 0.0f;
	}

	virtual void Render (void);
	
	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual U32 GetPlayerID (void) const
	{
		return playerID;
	}

	virtual U32 GetPartID (void) const
	{
		return dwMissionID;
	}

	virtual const TRANSFORM & GetTransform (void) const
	{
		return owner.Ptr()->GetTransform();
	}

	void RevealFog (const U32 currentSystem);

	virtual void CastVisibleArea (void);				// set visible flags of objects nearby

	/* IExplosion methods */

	void doExplode (void);

	void doRipple (void);

	void doChunk (void);

	void Catastrophic (void);

	void RationalSpeed(INSTANCE_INDEX id);

//	AnimInstance *MakeAnimInst();

	virtual void Bust (IMeshInfoTree *tree);

	virtual void Separate (IMeshInfoTree *tree);

	virtual void SpreadShrapnel (void);

	virtual BOOL32 InitExplosion (IBaseObject * _owner,U32 playerID,U16 sensorRadius,BOOL32 _minimal = 0,
		BOOL32 _notOnObjlist = 0);

	virtual BOOL32 ShouldRenderParent();

	static U32 calculateNumNuggets (IExplosionOwner * obj);
};

//HSOUND Explosion::hSound = 0,Explosion::sm_sound = 0;
//int Explosion::soundUsers = 0,Explosion::sm_soundUsers =0;

//----------------------------------------------------------------------------------
//
Explosion::~Explosion (void)
{
//	if (--soundUsers <= 0)
//	{
//		SFXMANAGER->CloseHandle(hSound);
//		hSound = 0;
//	}

//	if (--sm_soundUsers <=0)
//	{
//		SFXMANAGER->CloseHandle(sm_sound);
//		sm_sound = 0;
//	}

	//if (handle)
	//	CloseExplodeHandle(handle);

	int i;
	for (i=0;i<blast_cnt;i++)
	{
		delete blasts[i];
	}

	if(!notOnObjlist)
		delete owner.Ptr();		// we grabbed ownership of our owner

	if (objMapSystemID)
		OBJMAP->RemoveObjectFromMap(this,objMapSystemID,objMapSquare);

	for (i=0;i<shrapnel_cnt;i++)
	{
		shrapnel[i]->bOwnsChildren = true;
		delete shrapnel[i];
	}

	EXPCOUNT--;
	CQASSERT(EXPCOUNT>=0);
}
//----------------------------------------------------------------------------------
//
BOOL32 Explosion::Update (void)
{
	//remove
	/*if (!dead)
	{
		Bust(mesh_info);
		dead = true;
		timeToLive = 0;
	}*/
	//end remove 

	int i;
	BOOL32 allDone;
	allDone = TRUE;

	if (!dead && !notOnObjlist)
		owner.Ptr()->Update();

//	if (handle)
//		ContinueExplodeInstance(handle,NULL,NULL,0,0,0);

	explosionCounter++;
/*	for (i=0;i<MAX_ANIMS;i++)
	{
		if (anim[i])
		{
			if (anim[i]->update(ELAPSED_TIME) == 0)
			{
				delete anim[i];
				anim[i] = 0;
			}
			else
			{
				allDone = FALSE;
			}
		}
	}*/

	for (i=0;i<blast_cnt;i++)
	{
		if (blasts[i])
		{
			if (blasts[i]->Update() == 0)
			{
				delete blasts[i];
				blasts[i] = 0;
			}
			else
			{
				allDone = FALSE;
			}
		}
	}

	timeToLive -= ELAPSED_TIME;
	if (timeToLive <= 0.0)
	{
		if (dying)
		{
			deathPause--;
			if (deathPause == 0)
			{
				if (!minimal)
				{
			//		owner->SetColors();
					Bust(mesh_info);
					mesh_info = 0;
					VOLPTR(IExtent) extentObj;
					extentObj = owner.Ptr();
					CQASSERT(extentObj);
//					extentObj->SetMeshInfoTree(0);
					//	owner = 0;
					
					if (shrapnel_cnt)
					{
						SpreadShrapnel();
					}
				}

				// die used to be here

				dead = TRUE;
				dying = FALSE;
			}
			allDone = FALSE;
		}
		else
		if (!dead)
		{
			Catastrophic();//ownerIdx);
			allDone = FALSE;
		
		//	OBJLIST->AddObject(this);
//			NETPACKET->SendStateChange(this, 0, 0);
		//	return 0;
			//		bDeleteRequested = TRUE;
		}
	}
	else
	{
		doExplode();
		if (bRipple)
			doRipple();
		if (bChunk)
			doChunk();
	}
	
	return (timeToLive > 0 || !allDone);
}

void Explosion::PhysicalUpdate(SINGLE dt)
{
	int i;
	for (i=0;i<blast_cnt;i++)
	{
		if (blasts[i])
		{
			blasts[i]->PhysicalUpdate(dt);
		}
	}

	if (!dead && !notOnObjlist)
		owner.Ptr()->PhysicalUpdate(dt);

	for (i=0;i<shrapnel_cnt;i++)
	{
		Vector velocity = ENGINE->get_velocity(shrapnel[i]->mi[0]->instanceIndex);
		Vector ang_velocity = ENGINE->get_angular_velocity(shrapnel[i]->mi[0]->instanceIndex);

		Quaternion q;
		Vector x;
		Transform T = ENGINE->get_transform(shrapnel[i]->mi[0]->instanceIndex);

		q.set(T);
		x = T.translation;
		Matrix *R = (Matrix *)&T;

		// recalc center_of_mass
		x -= *R * shrap_phys[i].arm;
		
		// update position.
		x += velocity * dt;

		// update ang position
		Quaternion qw(ang_velocity);
		Quaternion qdot = qw * q * 0.5;
		q.w += qdot.w * dt;
		q.x += qdot.x * dt;
		q.y += qdot.y * dt;
		q.z += qdot.z * dt;
		q.normalize();

	// be sure to sync rotation matrix with quaternion:
		T = q;
		*R = q;

		// now resync position
		T.translation = x;
		
		T.translation += *R * shrap_phys[i].arm;

		ENGINE->set_transform(shrapnel[i]->mi[0]->instanceIndex,T);

		ENGINE->update_instance(shrapnel[i]->mi[0]->instanceIndex,0,0);
	}
}

void Explosion::RationalSpeed(INSTANCE_INDEX id)
{
	Vector vel = ENGINE->get_velocity(id);
	float speed = vel.magnitude();

	if (speed > 1e6)
		CQTRACE31("Silly speed : %f",speed); //which severity level is which?
	
	//for now, not moving is ok
	/*if (speed == 0)
	{
		vel.set(rand(),rand(),rand());
		vel.normalize();
		vel *= 250;
		speed = 250;
		ENGINE->set_velocity(id, vel);
	}

	if (speed < 250)
	{
		vel *= 250.0 / speed;
		ENGINE->set_velocity(id, vel);
	}*/

//	CQASSERT(speed < 2500);
	
	if (speed > 2000)
	{
		vel *= 2000.0 / speed;
		ENGINE->set_velocity(id, vel);
		Vector ang = ENGINE->get_angular_velocity(id);
		// NOTE: don't use normalize here, because ang may be 0 vector
	/*	SINGLE mag = ang.magnitude();
		if (mag)
			ang *= (10 / mag);
		ENGINE->set_angular_velocity(id, ang);*/
	}
}

void Explosion::doExplode (void)
{
	fall_vel = 0.98*fall_vel;
//  Vector vel = ENGINE->get_velocity(ownerIdx);
	Vector owner_vel = owner.Ptr()->GetVelocity();
	if (owner_vel.magnitude() < fall_vel.magnitude())
		ENGINE->set_velocity(ownerIdx,fall_vel);
	
	const TRANSFORM & transform = owner.Ptr()->GetTransform();
	
	if (bListing)
	{
	/*			float roll = transform.get_roll();
	float pitch = transform.get_pitch();
		owner->RotateShip(0, (rollTarget - roll) / 2.0, (pitchTarget - pitch) / 2.0, listTarget - transform.get_position().z);*/
	//	owner->RotateShip(0,lst_roll,lst_pitch,lst_list);
	}
	
	if (bSpinner)
	{
		Vector vel(0,0.5,0.8);//explosionCounter/35.0);
		ENGINE->set_angular_velocity(ownerIdx,vel);
	}
	
	if (ELAPSED_TIME > 0.0)//bSmallBooms)
	{
		S32 percent = F2LONG(8000.0*ELAPSED_TIME/2.0);
		if (percent > (rand()%10000))
		{
			Vector pos = transform.get_position();
			Vector offset;
			//	offset.x = bounding_box[1] + rand() * (bounding_box[0] - bounding_box[1]) / RAND_MAX;
			//	offset.y = bounding_box[3] + rand() * (bounding_box[2] - bounding_box[3]) / RAND_MAX;
			//	offset.z = bounding_box[5] + rand() * (bounding_box[4] - bounding_box[5]) / RAND_MAX;
			Vector dim;
			dim.x = bounding_box[0] - bounding_box[1];
			dim.y = bounding_box[2] - bounding_box[3];
			dim.z = bounding_box[4] - bounding_box[5];
			
			offset.x = (rand() * dim.x *0.25 / RAND_MAX) - dim.x*0.125;
			offset.y = (rand() * dim.y *0.25 / RAND_MAX) - dim.y*0.125;
			offset.z = (rand() * dim.z *0.25 / RAND_MAX) - dim.z*0.125;
			
			boom_pos += offset;
			if (boom_pos.x < bounding_box[1]+100 || boom_pos.x > bounding_box[0]-100)
				boom_pos.x -= 2*offset.x;
			//	if (boom_pos.y < bounding_box[3] || boom_pos.y > bounding_box[2])
			//		boom_pos.y -= 2*offset.y;
			if (boom_pos.z < bounding_box[5]+100 || boom_pos.z > bounding_box[4]-100)
				boom_pos.z -= 2*offset.z;
			
			OBJPTR<IExtent> extentObj;
			owner->QueryInterface(IExtentID,extentObj);
			CQASSERT(extentObj);
			
			const RECT *rect = extentObj->GetExtentRect(boom_pos.z);
			boom_pos.y = rect->top+50;
			
		//	Matrix R = transform.get_orientation();
		//	Vector loc = pos + R * boom_pos;
			TRANSFORM trans;// = transform;
			trans.set_position(boom_pos);
			
			IBaseObject * explosion;
			
			if ( (blast_cnt < MAX_BLASTS) && (explosion = ARCHLIST->CreateInstance(secondaryBlastType)) !=0)
			{
				OBJPTR<IBlast> blast;
				if (explosion->QueryInterface(IBlastID,blast))
				{
					blast->InitBlast(trans, systemID,this,0.2);
					//ENGINE->set_velocity(explosion->GetObjectIndex(),ENGINE->get_velocity(ownerIdx));
					Vector bob = Vector(0,0,1)*50;
					blasts[blast_cnt++] = explosion;
					
					OBJPTR<IFireball> fire;
					blast->GetFireball(fire);
					CQASSERT(fire);
#ifdef _DEBUG
					if (fire)
#endif
						fire->SetSpread(bob,-bob,2);
				}
			}
#ifdef _ROB
			else
			{
				CQTRACE10("Out of blasts or failed to create blast");
			}
#endif
		}
		
		if (num_first_children && !minimal)
		{
			S32 percent = F2LONG(8000.0*ELAPSED_TIME/4.0);

			if (percent > (rand()%10000))
			{
				// Disconnect and explode random child object.
				
				int child_to_explode = rand() * num_first_children / RAND_MAX;
				int cnt = 0;
				
				INSTANCE_INDEX child = ENGINE->get_instance_child_next(ownerIdx,0,INVALID_INSTANCE_INDEX);
				while ((child != INVALID_INSTANCE_INDEX) && (cnt++ < child_to_explode))
				{
					child = ENGINE->get_instance_child_next(ownerIdx, EN_DONT_RECURSE, child);
				}
				
				if (child != INVALID_INSTANCE_INDEX)
				{
					for (int i=0;i<numChildren;i++)
					{
						if (children->mi[i]->instanceIndex == child && children->mi[i]->bWhole)
						{
							SINGLE mass = GetMass(children->mi[i]);
							if (mass < 1000)
							{
								PHYS_CHUNK chunk;
								Vector c_mass = chunk.arm = GetCenterOfMass(children->mi[i]);
								children->mi[i]->instanceIndex = INVALID_INSTANCE_INDEX;
								TRANSFORM trans = ENGINE->get_transform(ownerIdx);
								TRANSFORM childTrans = ENGINE->get_transform(child);
								childTrans.translation = trans.inverse_rotate_translate(childTrans.translation);
								ENGINE->destroy_joint(ownerIdx, child);
								owner->OnChildDeath(child);
//								PHYSICS->set_dynamic(child, DS_DYNAMIC);
								Vector ang = ENGINE->get_angular_velocity(ownerIdx);
							//	Vector c_mass = GetCenterOfMass(child)-trans.rotate_translate(c_of_mass);


								Vector impulse;
								impulse.set(rand(),rand(),rand());
								impulse.normalize();
								
								SINGLE massFactor = 100+(1000-mass);
								Vector vel = ENGINE->get_velocity(ownerIdx)+cross_product(ang,c_mass)+impulse*0.30*massFactor;
					//			CQASSERT(vel.magnitude() < 3000 && "Bother Rob");
								ENGINE->set_velocity(child,vel);
								ENGINE->set_angular_velocity(child,ang+impulse*massFactor*0.008*((SINGLE)rand()/RAND_MAX));
								IBaseObject * obj = CreateDebris(0.02*(rand()%100), TRUE, child,pieceBlastType, systemID, &chunk);
								
								if (obj)
								{
									obj->pArchetype = 0;
									obj->objClass = OC_SHRAPNEL;
									OBJLIST->AddObject(obj);
									if (blast_cnt < MAX_BLASTS)
									{
										IBaseObject * explosion = ARCHLIST->CreateInstance(pieceBlastType);
										if (explosion)
										{	
											OBJPTR<IBlast> blast;
											if (explosion->QueryInterface(IBlastID,blast))
											{
												blast->InitBlast(childTrans, systemID,this);
											}
											blasts[blast_cnt++] = explosion;
										}
									}
#ifdef _ROB
									else
									{
										CQTRACE10("Out of blasts");
									}
#endif
								}
							}
						}
					}
				}
			}
		}
	}
}

void Explosion::doRipple(void)
{
//	ENGINE->set_velocity(ownerIdx,blast_vel);

	VOLPTR(IExtent) extentObj;
	extentObj = owner.Ptr();
	CQASSERT(extentObj);

	S32 percent = F2LONG(8000.0*ELAPSED_TIME/2.0);
	if (percent > (rand()%10000))
	{
		SINGLE fudge = ((SINGLE)(rand()-RAND_MAX/2)/RAND_MAX)*300;
		boom_count++;


//		INSTANCE_INDEX newIndex1,newIndex2;
		PHYS_CHUNK phys1,phys2;
	//	owner->SetColors();
	//	owner.ptr->bNoMeshRender = true;
		IMeshInfoTree *out0,*out1;
		//I hereby define split_n to point in the direction of out1
		//I also proclaim that out1 will be the piece that is thrown away
		if (SplitInstance(ownerIdx,mesh_info,boom_axis,boom_length/2-(boom_count*boom_length/12)+fudge,&out0,&out1,&phys1,&phys2))
		{
			IEngineInstance *eng_inst;
			ENGINE->get_instance_handler(ownerIdx,&eng_inst);
			ENGINE->destroy_instance(ownerIdx);
			DestroyMeshInfoTree(mesh_info);
//			extentObj->SetMeshInfoTree(out0);
			mesh_info = out0;
			CQASSERT(eng_inst);
			children->numChildren = out0->ListChildren(children->mi);
			ENGINE->set_instance_handler(children->mi[0]->instanceIndex,eng_inst);
			ownerIdx = children->mi[0]->instanceIndex;
			Transform ownerTrans = ENGINE->get_transform(ownerIdx);
			//ENGINE->set_transform(newIndex,trans);
		//	INSTANCE_INDEX index0 = out1->GetMeshInfo()->instanceIndex;
			Vector pos=GetCenterOfMass(out1->GetMeshInfo());
			if (blast_cnt < MAX_BLASTS)
			{
				IBaseObject *obj = ARCHLIST->CreateInstance(data->rippleBlastType);
				if(obj)
				{
					OBJPTR<IBlast> blast;
					if (obj->QueryInterface(IBlastID,blast))
					{
						Transform trans;
						//trans.set_position(pos);
						trans.set_identity();
						trans.translation = pos;
						blast->InitBlast(trans, systemID,this);
						//OBJLIST->AddObject(obj);
						blasts[blast_cnt++] = obj;
						Vector temp = ENGINE->get_velocity(ownerIdx);
						ENGINE->set_velocity(obj->GetObjectIndex(),ENGINE->get_velocity(ownerIdx));
					}
				}
			}
#ifdef _ROB
			else
			{
				CQTRACE10("Out of blasts or failed to create blast");
			}
#endif
			
			Separate(out1);
			out1=0;

			blast_speed = 25;
			blast_vel = -ownerTrans.get_k()*blast_speed;
			Vector vel = ENGINE->get_velocity(ownerIdx);
			vel += blast_vel;
			ENGINE->set_velocity(ownerIdx,vel);
		
		}
//		ENGINE->destroy_instance(newIndex1);
	}
}

void Explosion::doChunk(void)
{
//	ENGINE->set_velocity(ownerIdx,blast_vel);
	VOLPTR(IExtent) extentObj;
	extentObj = owner.Ptr();
	CQASSERT(extentObj);

	S32 percent = F2LONG(8000.0*ELAPSED_TIME/4.0);
	if (percent > (rand()%10000))
	{
		SINGLE fudge = ((SINGLE)(rand()-RAND_MAX/2)/RAND_MAX)*300;
		boom_count++;


		PHYS_CHUNK phys1,phys2;

		//should be utterly unnecessary to set this here
	//	owner->SetColors();

	//	owner.ptr->bNoMeshRender = true;
		//if (SplitInstance(ownerIdx,boom_axis,(boom_count*boom_length/12)-boom_length/2+fudge,&newIndex1,&newIndex2,&phys1,&phys2))
		IMeshInfoTree *out0,*out1;
		//I hereby define split_n to point in the direction of out1
		//I also proclaim that out1 will be the piece that is thrown away
		if (SplitInstance(ownerIdx,mesh_info,boom_axis,boom_length/2-(boom_count*boom_length/12)+fudge,&out0,&out1,&phys1,&phys2))
		{
			IEngineInstance *eng_inst;
			ENGINE->get_instance_handler(ownerIdx,&eng_inst);
			ENGINE->destroy_instance(ownerIdx);
			DestroyMeshInfoTree(mesh_info);
//			extentObj->SetMeshInfoTree(out0);
			mesh_info = out0;
			CQASSERT(eng_inst);
			children->numChildren = out0->ListChildren(children->mi);
			ENGINE->set_instance_handler(children->mi[0]->instanceIndex,eng_inst);
			ownerIdx = children->mi[0]->instanceIndex;
			Transform ownerTrans = ENGINE->get_transform(ownerIdx);
			//ENGINE->set_transform(newIndex,trans);
			//INSTANCE_INDEX index0 = out1->GetMeshInfo()->instanceIndex;
			Vector pos=GetCenterOfMass(out1->GetMeshInfo());

			if (blast_cnt < MAX_BLASTS)
			{
				IBaseObject *obj = ARCHLIST->CreateInstance(data->rippleBlastType);
				if (obj)
				{
					OBJPTR<IBlast> blast;
					if (obj->QueryInterface(IBlastID,blast))
					{
						Transform trans;
						trans.set_identity();
						trans.translation = pos;
						blast->InitBlast(trans, systemID,this);
						//OBJLIST->AddObject(obj);
						blasts[blast_cnt++] = obj;
						
						//we don't give the blast a velocity because it is attached to us.
						//Vector temp = ENGINE->get_velocity(ownerIdx);
						//ENGINE->set_velocity(obj->GetObjectIndex(),owner.ptr->GetVelocity());
					}
				}
			}
#ifdef _ROB
			else
			{
				CQTRACE10("Out of blasts or failed to create blast");
			}
#endif
			
			Separate(out1);
			out1=0; //got destroyed in separate
			
			SpreadShrapnel();

			blast_speed = 25;
			blast_vel = ownerTrans.rotate(boom_axis)*(blast_speed);
			Vector vel = owner.Ptr()->GetVelocity();
			vel -= blast_vel;
			ENGINE->set_velocity(ownerIdx,vel);
		
		//	ENGINE->destroy_instance(newIndex1);
		}
	}

}

/*AnimInstance *Explosion::MakeAnimInst()
{
	U32 i=0;
	
	while (i<12)
	{
		if (!anim[i])
		{
			anim[i] = new AnimInstance;
			return anim[i];
		}
		i++;
	}

	return 0;
}*/

void Explosion::Catastrophic (void)
{
	//	if (cnt%5 ==0)
	//	{
	
//	if (!Explosion::hSound)
//	{
//		Explosion::hSound = SFXMANAGER->Open(SFX::EXPLOSION);
//	}
//	Explosion::soundUsers++;
//	SFXMANAGER->Play(Explosion::hSound);
	deathPause = 1;//20;
	/*float box[6];
	HARCH hArche = ownerIdx;

	if (REND->get_archetype_bounding_box(hArche, LODPERCENT, box))
	{
//		SINGLE dz = box[4]-box[5];
		
		TRANSFORM trans;
		Vector k(0,0,-1);
		trans.translation = c_of_mass;
		if (IBaseObject *obj = ARCHLIST->CreateInstance(catastrophicBlastType))
		{
			OBJPTR<IBlast> blast;
			if (obj->QueryInterface(IBlastID,blast))
			{
				blast->InitBlast(trans,systemID,this);
				ENGINE->set_velocity(obj->GetObjectIndex(),ENGINE->get_velocity(ownerIdx));
				
				Vector bob = k*(dz/7.0);
				
				blasts[blast_cnt++] = obj;
				OBJPTR<IFireball> fire;
				blast->GetFireball(fire);
				CQASSERT(fire);
#ifdef _DEBUG
				if (fire)
#endif
					fire->SetSpread(bob,-bob,ceil(dz/1200.0));
			}
		}
	//	deathPause = max(20*(dz/4000.0),8);
	}*/
	//	}

//	cnt++;

//	if (cnt == 5*num_booms)
//	{
	

		dying = TRUE;
	
	//	dead = TRUE;
	
		
//	}

}
	
void Explosion::Separate (IMeshInfoTree *tree)
{
	//just a token separation of pieces
//	INSTANCE_INDEX index = tree->GetMeshInfo()->instanceIndex;
	//ENGINE->destroy_instance(index);
	SINGLE force = GetMass(tree->GetMeshInfo());
	int shrapnel_room = min(MAX_SHRAPNEL-shrapnel_cnt,6);
	if (shrapnel_room)
	{
		U32 numDebris = ExplodeInstance( tree, force, shrapnel_room, shrapnel_room, &shrapnel[shrapnel_cnt],&shrap_phys[shrapnel_cnt]);
		shrapnel_cnt += numDebris;
	}
}

void Explosion::SpreadShrapnel(void)
{
	SINGLE strength = 20 * total_mass;
	
	int i;
	for (i = 0; i < shrapnel_cnt; i++)
	{
		BOOL32 bExplodeLater=0;
		BOOL32 bAddFireTrail = (rand()%2 == 0);

		SINGLE dx,dy,dz;
		dx = dy = dz = shrapnel[i]->mi[0]->radius;  //!!!!!replace with real dimensions when possible
		/*float box[6];
		if (REND->get_archetype_bounding_box(hArch, LODPERCENT,box))
		{
		float dx = fabs(box[0] - box[1]);
		float dy = fabs(box[2] - box[3]);
		float dz = fabs(box[4] - box[5]);
		float size = __max(dx, __max(dy, dz));*/
		
		SINGLE size = shrapnel[i]->mi[0]->radius;
		
		// Now apply explosion impulse.
		Vector pos = ENGINE->get_transform(shrapnel[i]->mi[0]->instanceIndex)*shrapnel[i]->mi[0]->sphere_center;  //was GetCenterOfMass
		Matrix orient (ENGINE->get_orientation (shrapnel[i]->mi[0]->instanceIndex));
		
		TRANSFORM trans = ENGINE->get_transform(ownerIdx);
		//	Vector bud = trans.rotate_translate(c_of_mass);
		Vector impulse = pos;
		SINGLE mag = impulse.magnitude();
		if (mag > 0)
			impulse /= mag;
		else
			impulse.set(1,0,0);
		
		impulse *= strength;
		
		// Apply impulse at a random point on the chunk.
		
		float vertx = dx*rand()/float(RAND_MAX) - dx/2.0;
		float verty = dy*rand()/float(RAND_MAX) - dy/2.0;
		float vertz = dz*rand()/float(RAND_MAX) - dz/2.0;
		
		Vector pt = pos + orient * Vector (vertx, verty, vertz);
		
		//				SINGLE mass = dummy_get_mass(shrapnel[i]);
		//				PHYSICS->add_impulse_at_point(shrapnel[i], impulse, pt);
		if (size > 450.0 && rand()%2 == 0)
		{
			// Make big debris explode after a while.
			bExplodeLater = 1;
		}
		else
		{
			bExplodeLater = 0;
		}
		
		IBaseObject * debris;
		float lifeTime = 1.0 +(SINGLE)rand()/RAND_MAX;
		if (bAddFireTrail && !bExplodeLater)
		{
			// Pieces with fire trails that won't explode need to last longer.
			lifeTime *= 2.0;
		}
		
		RationalSpeed(shrapnel[i]->mi[0]->instanceIndex);
		//we're becoming a mesh chain.  we have an owner so we can't own our instance
		//shrapnel[i]->bOwnsInstance = false;
		shrapnel[i]->bOwnsChildren = true;
		if ((debris = CreateDebris(lifeTime, bExplodeLater,shrapnel[i], pieceBlastType, systemID,&shrap_phys[i])) != 0)
		{
			shrapnel[i] = 0;
			//debris->instanceIndex = shrapnel[i];
			//				debris->archIndex = INVALID_ARCHETYPE_INDEX;
			debris->objClass = OC_SHRAPNEL;
			
			OBJLIST->AddObject(debris);
			//				PHYSICS->set_dynamic(shrapnel[i], DS_DYNAMIC);
			
			//	Vector vec = ENGINE->get_position(shrapnel[i]);
			
			// particle explosion
			//IBaseObject * ps = CreateParticleObject(vec, &pData);
			
			// Randomly add fire trail.
			if (bAddFireTrail)
			{
				
				INSTANCE_INDEX index = ENGINE->create_instance2(fireTrail,0);
				if (index != INVALID_INSTANCE_INDEX)
				{
					OBJPTR<IDebris> deb;
					//OBJLIST->AddObject(index);
					
					//	JointInfo info(JT_FIXED);									    
					//	REND->get_centroid(hArch, info.rel_position);
					//	info.rel_orientation.set_identity();
					//	Joint joint(info, shrapnel[i], index);
					//	MODEL->connect(&joint);
					Transform fireTrans;
					Vector i,j,k;
					k = debris->GetVelocity();
					if (k.y == 0 && k.x == 0)
					{
						j.set(0,1,0);
						k.set(0,0,1);
					}
					else
					{
						k.normalize();
						j.set(k.y,-k.x,0);
						j.normalize();
					}
					i = cross_product(j,k);
					fireTrans.set_i(i);
					fireTrans.set_j(j);
					fireTrans.set_k(k);
					ENGINE->set_transform(index,fireTrans);
					ENGINE->update_instance(index,0,0);
					debris->QueryInterface(IDebrisID,deb);
					CQASSERT(deb);
					deb->RegisterChild(index);
				}
			}
		}
	}
	shrapnel_cnt =0;
}

void Explosion::Bust (IMeshInfoTree *tree)
{
	S32 i, numDebris;

	MeshChain * objArray[12];
	PHYS_CHUNK physChunks[12];

	if (handle)
	{
		numDebris = 0;
	//	ContinueExplodeInstance(handle,&numDebris,objArray,physChunks,12,TRUE);
	//	CloseExplodeHandle(handle);
	//	handle = 0;
	}
	else
	{
		U8 desiredChunks=4;
		SINGLE force = 5 * GetMass(tree->GetMeshInfo());
		numDebris = ExplodeInstance( tree, force, desiredChunks, 12, objArray,physChunks);
		
	}
	
	
	// add new objects to the list
	for (i = 0; i < numDebris; i++)
	{
		BOOL32 bExplodeLater=0;
		BOOL32 bAddFireTrail = rand()%2==0;
		SINGLE size = objArray[i]->mi[0]->radius;

		if (size > 450.0 && rand()%2 == 0)
		{
			// Make big debris explode after a while.
			bExplodeLater = 1;
		}
		else
		{
			bExplodeLater = 0;
		}
		
		IBaseObject * debris;
		float lifeTime = 1.0+0.01*(rand()%100);
		if (bAddFireTrail && !bExplodeLater)
		{
			// Pieces with fire trails that won't explode need to last longer.
			lifeTime *= 2.0;
		}
		
		RationalSpeed(objArray[i]->mi[0]->instanceIndex);
		//we're becoming a mesh chain.  we have an owner so we can't own our instance
		objArray[i]->bOwnsChildren = true;
		if ((debris = CreateDebris(lifeTime, bExplodeLater,objArray[i],pieceBlastType, systemID,&physChunks[i])) != 0)
		{
			//			debris->instanceIndex = objArray[i];
			//				debris->archIndex = INVALID_ARCHETYPE_INDEX;
			debris->objClass = OC_SHRAPNEL;
			
			OBJLIST->AddObject(debris);
			//				PHYSICS->set_dynamic(objArray[i], DS_DYNAMIC);
			
			
			// Randomly add fire trail.
			if (bAddFireTrail)
			{
				
				Vector vec = ENGINE->get_position(objArray[i]->mi[0]->instanceIndex);
				
				// particle explosion
				//IBaseObject * ps = CreateParticleObject(vec, &pData);
				
				INSTANCE_INDEX index = ENGINE->create_instance2(fireTrail,0);
				if (index != INVALID_INSTANCE_INDEX)
				{
					//OBJLIST->AddObject(index);
					OBJPTR<IDebris> deb;
					
					//	JointInfo info(JT_FIXED);									    
					//	REND->get_centroid(hArch, info.rel_position);
					
					//	info.rel_orientation.set_identity();
					//	{
					Transform fireTrans;
					Vector i,j,k;
					k = debris->GetVelocity();
					// NOTE: don't use normalize here, because vel may be 0 vector
					// ??????? should be ok now
					/*	SINGLE mag = vel.magnitude();
					if (mag)
					vel *= (1.0F / mag);
					k.normalize();*/
					if (k.y == 0 && k.x == 0)
					{
						j.set(0,1,0);
					}
					else
					{
						j.set(k.y,-k.x,0);
						j.normalize();
					}
					i = cross_product(j,k);
					fireTrans.set_i(i);
					fireTrans.set_j(j);
					fireTrans.set_k(k);
					//	}
					//	Joint joint(info, objArray[i], index);
					//	MODEL->connect(&joint);
					ENGINE->set_transform(index,fireTrans);
					ENGINE->update_instance(index,0,0);
					debris->QueryInterface(IDebrisID,deb);
					CQASSERT(deb);
					deb->RegisterChild(index);
				}
			}
		}
	}
}
//----------------------------------------------------------------------------------
//
void Explosion::Render (void)
{
	BATCH->set_state(RPR_BATCH,true);
	if (bVisible || notOnObjlist)  //this seems to be necessary because otherwise explosions in OTHER SYSTEMS will appear.
	{
		int i;
		if (!dead && !notOnObjlist)
			owner.Ptr()->Render();
		
		for (i=0;i<shrapnel_cnt;i++)
		{
			TreeRender(shrapnel[i]->mi,shrapnel[i]->numChildren);
		}

		const U32 currentSystem = SECTOR->GetCurrentSystem();
		const USER_DEFAULTS & defaults = *DEFAULTS->GetDefaults();
		const U32 currentPlayer = MGlobals::GetThisPlayer();
		
		for (i=0;i<blast_cnt;i++)
		{
			if (blasts[i])
			{
				blasts[i]->TestVisible(defaults, currentSystem, currentPlayer);
				if (blasts[i]->bVisible)
					blasts[i]->Render();
			}
		}
	}
}

void Explosion::RevealFog (const U32 currentSystem)
{
	if (systemID==currentSystem && playerID && MGlobals::AreAllies(playerID, MGlobals::GetThisPlayer()) && timeToLive+FOG_PAUSE > 0)
	{
		SINGLE fogFactor = min((timeToLive+FOG_PAUSE)/totalTime,1);
		FOGOFWAR->RevealZone(this, sensorRadius*fogFactor,0);
	}
}
//----------------------------------------------------------------------------------
//
void Explosion::CastVisibleArea (void)
{
	if (playerID)
	{
		if (!dead && !notOnObjlist)
			OBJLIST->CastVisibleArea(playerID, systemID, owner.Ptr()->GetTransform().translation, fieldFlags, sensorRadius, 0);
		SetVisibleToPlayer(playerID);
	}
}
//----------------------------------------------------------------------------------
//
BOOL32 Explosion::InitExplosion (IBaseObject * _owner,U32 _playerID,U16 _sensorRadius,BOOL32 _minimal,BOOL32 _notOnObjlist)
{
/*	CQASSERT(_owner);
	notOnObjlist = (_notOnObjlist==1);
	if(!notOnObjlist)
		OBJLIST->RemoveObject(_owner);
	_owner->QueryInterface(IExplosionOwnerID, owner,LAUNCHVOLATILEPTR);
	CQASSERT(owner!=0);
	systemID = _owner->GetSystemID();
	ownerIdx = _owner->GetObjectIndex();
	fieldFlags = owner.Ptr()->fieldFlags;
	SetVisibilityFlags(owner.Ptr()->GetVisibilityFlags());
	TRANSFORM trans = _owner->GetTransform();
	VOLPTR(IExtent) extentObj=_owner;
	SINGLE mass = 4000;
	CQASSERT (extentObj);
	mass = 1000;//GetMass(extentObj->GetMeshInfoTree()->GetMeshInfo());
	playerID = _playerID;
	sensorRadius = _sensorRadius;
	dwMissionID = owner->GetFirstNuggetID();
	CQASSERT(dwMissionID);
	//
	// init objmap stuff
	//

	CQASSERT(objMapNode==0);
	objMapSquare = OBJMAP->GetMapSquare(systemID, trans.translation);
	objMapSystemID = systemID;
	objMapNode = OBJMAP->AddObjectToMap(this, objMapSystemID, objMapSquare, OM_EXPLOSION);
	CQASSERT(objMapNode);

	//fall_vel = ENGINE->get_velocity(ownerIdx);
	fall_vel = owner.Ptr()->GetVelocity();

	minimal = 0;//_minimal;
	HARCH hArch = ownerIdx;
	REND->get_archetype_bounding_box(hArch, LODPERCENT, bounding_box);

	num_first_children = 0;
	INSTANCE_INDEX child = ENGINE->get_instance_child_next(ownerIdx,0,INVALID_INSTANCE_INDEX);
	while (child != INVALID_INSTANCE_INDEX)
	{
		num_first_children++;
		child = ENGINE->get_instance_child_next(ownerIdx, EN_DONT_RECURSE, child);
	}

	children = &extentObj->GetMeshChain();
	mesh_info = extentObj->GetMeshInfoTree();

	//setup fancy effects params
	float dx = bounding_box[BBOX_MAX_X] - bounding_box[BBOX_MIN_X];
	float dy = bounding_box[BBOX_MAX_Y] - bounding_box[BBOX_MIN_Y];
	float dz = bounding_box[BBOX_MAX_Z] - bounding_box[BBOX_MIN_Z];

	explosionTime = 4.0 + rand() * 2.0 / RAND_MAX;
	explosionCounter = 0;
	bRipple = bListing = bSmallBooms = bSpinner = bChunk = FALSE;
	if (!minimal)
	{
		// testing!!!!
		if (rand()%100 > 15)
		{
			if (rand()%100 > 50)
				bChunk = TRUE;
			else
				bRipple = TRUE;
		}

		if (rand() > RAND_MAX*0.25)
			bListing = TRUE;
		if (rand() > RAND_MAX*0.25)
			bSmallBooms = TRUE;
		
		if (mass < 2000 && rand() > RAND_MAX*0.5)
		{
			Vector vel = ENGINE->get_velocity(ownerIdx);
			if (vel.magnitude() > 100)
			{
				bSpinner = TRUE;
				//dynamicsData.MAX_ANG_VELOCITY = 			10.0; 
			}
		}
		
		if (bRipple || bChunk)
		{
			timeToLive = totalTime = 2.0+0.03*(rand()%100);
			switch (rand()%8)
			{
			case 0:
				boom_axis.set(1,0,0);
				boom_length = dx;
				timeToLive *= dx/2900.0;
				break;
			case 1:
				boom_axis.set(-1,0,0);
				boom_length = dx;
				timeToLive *= dx/2900.0;
				break;
			case 2:
				boom_axis.set(0,1,0);
				boom_length = dy;
				timeToLive *= dy/2900.0;
				break;
			case 3:
				boom_axis.set(0,-1,0);
				boom_length = dy;
				timeToLive *= dy/2900.0;
				break;
			case 4:
			case 6:
				boom_axis.set(0,0,1);
				boom_length = dz;
				timeToLive *= dz/2900.0;
				break;
			case 5:
			case 7:
				boom_axis.set(0,0,-1);
				boom_length = dz;
				timeToLive *= dz/2900.0;
				break;
			}
			timeToLive += 2.0;
			totalTime = timeToLive;
			//	bListing = FALSE;
		}
		else
		{
		//	owner->SetColors();
		//	SINGLE force = 90 * GetMass(ownerIdx);
			handle = 0;//temp? //StepExplodeInstance(ownerIdx,force,8);
		}

		TRANSFORM trans;
		Vector k(0,0,-1);
		trans.translation = c_of_mass;
		if (blast_cnt < MAX_BLASTS)
		{
			IBaseObject *obj = ARCHLIST->CreateInstance(catastrophicBlastType);
			if (obj)
			{
				OBJPTR<IBlast> blast;
				if (obj->QueryInterface(IBlastID,blast))
				{
					blast->InitBlast(trans,systemID,this,1.0,timeToLive);
					
					float box[6];
					HARCH hArche = ownerIdx;
					
					if (REND->get_archetype_bounding_box(hArche, LODPERCENT, box))
					{
						SINGLE dz = box[4]-box[5];
						Vector bob = k*(dz/7.0);
						
						
						blasts[blast_cnt++] = obj;
						OBJPTR<IFireball> fire;
						blast->GetFireball(fire);
						CQASSERT(fire);
#ifdef _DEBUG
						if (fire)
#endif
							fire->SetSpread(bob,-bob,ceil(dz/1200.0));
					}
				}
			}
		}
#ifdef _ROB
		else
		{
			CQTRACE10("Out of blasts or failed to create blast");
		}
#endif
	}
	boom_count = 1;
	
	//	const float half = 3.14159 / 4.0;

#define MAX_MASS 250000
	SINGLE massFactor = 2.0*(1.2-mass/MAX_MASS);
	CQASSERT(massFactor > 0);
//	lst_roll = massFactor*UPDATEFACTOR*(PI/40.0-(PI/20.0)*((SINGLE)rand()/RAND_MAX));
//	lst_pitch = massFactor*UPDATEFACTOR*(PI/40.0-(PI/20.0)*((SINGLE)rand()/RAND_MAX));
//	lst_list = massFactor*UPDATEFACTOR*(12-24.0*((SINGLE)rand()/RAND_MAX));
	Vector ang_vel;
	ang_vel.set(rand()%400-200,rand()%400-200,rand()%400-200);
	SINGLE mag = ang_vel.magnitude();
	if (mag)
		ang_vel *= massFactor*0.1/mag;
	ang_vel += ENGINE->get_angular_velocity(ownerIdx);
	ENGINE->set_angular_velocity(ownerIdx, ang_vel);

	REND->get_archetype_centroid(HARCH(ownerIdx),LODPERCENT,c_of_mass);//GetCenterOfMass(ownerIdx);
//	c_of_mass = trans.inverse_rotate_translate(c_of_mass);
	total_mass = max(mass,0);

	{
		OBJPTR<IExtent> extentObj;
		owner->QueryInterface(IExtentID,extentObj);
		CQASSERT(extentObj);
		
		boom_pos.x = bounding_box[1] + rand() * (bounding_box[0] - bounding_box[1]) / RAND_MAX;
		//	boom_pos.y = bounding_box[3] + rand() * (bounding_box[2] - bounding_box[3]) / RAND_MAX;
		boom_pos.z = bounding_box[5] + rand() * (bounding_box[4] - bounding_box[5]) / RAND_MAX;
		const RECT *rect = extentObj->GetExtentRect(boom_pos.z);
		boom_pos.y = rect->top+50;
	}

	IBaseObject::MDATA mdata;

	owner.Ptr()->GetMissionData(mdata);
		
//	race = mdata.pSaveData->race;

	CreateDebrisNuggets(owner.Ptr());
	RealizeDebrisNuggets(owner.Ptr(),timeToLive);
*/
	return 1;
}
//----------------------------------------------------------------------------------
//
BOOL32 Explosion::ShouldRenderParent()
{
	return !dead && notOnObjlist;
}
//----------------------------------------------------------------------------------
//
U32 Explosion::calculateNumNuggets (IExplosionOwner * obj)
{
	U32 numNuggets = MAX_NUGGETS, maxNuggets = MAX_NUGGETS;
	U32 scrapValue = obj->GetScrapValue();

//removed because of changes to havester, recode this when harvester is finished	
/*	if (harvester)
	{
		IHarvest::HarvestMaterials materials;

		harvester->GetMaterials(materials);
		
		scrapValue += materials.metalsComplete;
		scrapValue += materials.radioactivesComplete;
		scrapValue += materials.organicsComplete;
		scrapValue += materials.crystalsComplete;
	}
*/

	//
	// figure out how many nuggets we will need to create
	// try to get nuggets of equal value, around 150 / nugget
	//
	if (scrapValue==0)
		numNuggets = scrapValue = 0;
	else
	{
		U32 unitValue = scrapValue / maxNuggets;

		while (maxNuggets>1 && unitValue < 150)		// too many nuggets?
		{
			maxNuggets--;
			unitValue = scrapValue / maxNuggets;
		}

		numNuggets = maxNuggets;
	}

	return numNuggets;
}
//----------------------------------------------------------------------------------
//
void IExplosion::CreateDebrisNuggets (IBaseObject * owner)
{
	U32 dwMissionID = owner->GetPartID();
	// child objects (and child jumpplat) cannot create nuggets
	if ((dwMissionID & SUBORDID_MASK) == 0 && THEMATRIX->IsMaster())
	{
		VOLPTR(IExplosionOwner) obj = owner;
		U32 numNuggets = Explosion::calculateNumNuggets(obj);
		U32 systemID = owner->GetSystemID();

		if (numNuggets > 0 && systemID <= MAX_SYSTEMS)		// no nuggets in hyperspace!
		{
			MPart partObj(owner);
			PARCHETYPE pArchetype = NULL;
			if(partObj.isValid())
			{
				if(partObj->race == M_MANTIS)
					pArchetype = ARCHLIST->LoadArchetype("NUGGET!!M_Scrap");
				else if(partObj->race == M_SOLARIAN)
					pArchetype = ARCHLIST->LoadArchetype("NUGGET!!S_Scrap");
				else if(partObj->race == M_VYRIUM)
					pArchetype = ARCHLIST->LoadArchetype("NUGGET!!V_Scrap");
				else
					pArchetype = ARCHLIST->LoadArchetype("NUGGET!!Scrap");
			}
			else
			{
				pArchetype = ARCHLIST->LoadArchetype("NUGGET!!Scrap");
			}

			S32 scrapPer = obj->GetScrapValue() / numNuggets;
			U32 nuggetID = MGlobals::GetNextSubPartID(dwMissionID);
			nuggetID = MGlobals::GetNextSubPartID(nuggetID);		// skip the first subpart
			CQASSERT(nuggetID);
			U32 i;

			for (i=0; i < numNuggets; i++)
			{
				Vector pos = owner->GetPosition();
				pos.x += rand()%1000-500;
				pos.y += rand()%1000-500;
				pos.z += rand()%1000-500;
				Vector stop_pos = pos;
				stop_pos.x += rand()%400-200;
				stop_pos.y += rand()%400-200;
				stop_pos.z = 700+rand()%500;
				NUGGETMANAGER->CreateNugget(pArchetype,systemID,stop_pos,scrapPer,60*5,nuggetID,true);
				nuggetID = MGlobals::GetNextSubPartID(nuggetID);
			}
		}
	}
}
//----------------------------------------------------------------------------------
//
void IExplosion::RealizeDebrisNuggets (IBaseObject * owner,SINGLE delay)
{
	U32 numNuggets = Explosion::calculateNumNuggets(VOLPTR(IExplosionOwner)(owner));
	U32 systemID = owner->GetSystemID();
	U32 dwMissionID = owner->GetPartID();

	if (numNuggets > 0 && systemID <= MAX_SYSTEMS)		// no nuggets in hyperspace!
	{
		U32 nuggetID = MGlobals::GetNextSubPartID(dwMissionID);
		nuggetID = MGlobals::GetNextSubPartID(nuggetID);		// skip the first subpart
		// nuggetID &= ~PLAYERID_MASK;
		CQASSERT(nuggetID);
		U32 i;

		for (i=0; i < numNuggets; i++)
		{
			Vector pos = owner->GetPosition();
			pos.x += rand()%1000-500;
			pos.y += rand()%1000-500;
			pos.z += rand()%1000-500;
			Vector stop_pos = pos;
			stop_pos.x += rand()%400-200;
			stop_pos.y += rand()%400-200;
			stop_pos.z = 700+rand()%500;
			NUGGETMANAGER->RealizeNugget(nuggetID, stop_pos, systemID, delay);
			nuggetID = MGlobals::GetNextSubPartID(nuggetID);
		}
	}
}
//----------------------------------------------------------------------------------
//---------------------------------Explosion Factory--------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

struct ExplosionArchetype
{
	const char *name;
	BT_MESH_EXPLOSION *data;
    PARCHETYPE secondaryBlastType;
    PARCHETYPE pieceBlastType;
    PARCHETYPE catastrophicBlastType;
	PARCHETYPE rippleBlastType;
	ARCHETYPE_INDEX fireTrail;
	
	void * operator new (size_t size)
	{
		return calloc(size,1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	ExplosionArchetype (void)
	{
		fireTrail = -1;
	}

	~ExplosionArchetype (void)
	{
		if (secondaryBlastType)
			ARCHLIST->Release(secondaryBlastType, OBJREFNAME);
		if (pieceBlastType)
			ARCHLIST->Release(pieceBlastType, OBJREFNAME);
		if (catastrophicBlastType)
			ARCHLIST->Release(catastrophicBlastType, OBJREFNAME);
		if (rippleBlastType)
			ARCHLIST->Release(rippleBlastType, OBJREFNAME);

		ENGINE->release_archetype(fireTrail);
	}
};

struct DACOM_NO_VTABLE ExplosionManager : public IObjectFactory
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(ExplosionManager)
	DACOM_INTERFACE_ENTRY(IObjectFactory)
	END_DACOM_MAP()

//	struct ExplosionNode *explosionList;
	U32 factoryHandle;


	//child object info
	ExplosionArchetype *pArchetype;

	//ExplosionManager methods

	ExplosionManager (void) 
	{
	}

	~ExplosionManager();
	
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

	void Init();

	//IObjectFactory
	DEFMETHOD_(HANDLE,CreateArchetype) (const char *szArchname, OBJCLASS objClass, void *data);

	DEFMETHOD_(BOOL32,DestroyArchetype) (HANDLE hArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (HANDLE hArchetype);

	DEFMETHOD_(void,EditorCreateInstance) (HANDLE hArchetype, const MISSION_INFO & info);
};


//--------------------------------------------------------------------------
// ExplosionManager methods

ExplosionManager::~ExplosionManager()
{
	/*ExplosionNode 
	if (explosionList)
	{
	}*/

	COMPTR<IDAConnectionPoint> connection;
	if (OBJLIST)
	{
		if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
			connection->Unadvise(factoryHandle);
	}
}

void ExplosionManager::Init()
{
	COMPTR<IDAConnectionPoint> connection;


	if (OBJLIST->QueryOutgoingInterface("IObjectFactory", connection) == GR_OK)
	{
		connection->Advise(GetBase(), &factoryHandle);
	}
}

HANDLE ExplosionManager::CreateArchetype(const char *szArchname, OBJCLASS objClass, void *data)
{
	if (objClass == OC_MEXPLODE)
	{
		ExplosionArchetype *newguy = new ExplosionArchetype;
		newguy->name = szArchname;
		newguy->data = (BT_MESH_EXPLOSION *)data;

		if ((newguy->catastrophicBlastType = ARCHLIST->LoadArchetype(newguy->data->catastrophicBlastType)) != 0)
			ARCHLIST->AddRef(newguy->catastrophicBlastType, OBJREFNAME);
		if ((newguy->secondaryBlastType = ARCHLIST->LoadArchetype(newguy->data->secondaryBlastType)) != 0)
			ARCHLIST->AddRef(newguy->secondaryBlastType, OBJREFNAME);
		if ((newguy->pieceBlastType = ARCHLIST->LoadArchetype(newguy->data->pieceBlastType)) != 0)
			ARCHLIST->AddRef(newguy->pieceBlastType, OBJREFNAME);
		if ((newguy->rippleBlastType = ARCHLIST->LoadArchetype(newguy->data->rippleBlastType)) != 0)
			ARCHLIST->AddRef(newguy->rippleBlastType, OBJREFNAME);

		DAFILEDESC fdesc = ((BT_MESH_EXPLOSION *)data)->fireTrail;
		COMPTR<IFileSystem> file;
		
		if (fdesc.lpFileName[0] && OBJECTDIR->CreateInstance(&fdesc,file)==GR_OK)
		{
			TEXLIB->load_library(file, 0);
			newguy->fireTrail = ENGINE->create_archetype(fdesc.lpFileName, file);
		}

		return newguy;
	}

	return 0;
}

BOOL32 ExplosionManager::DestroyArchetype(HANDLE hArchetype)
{
	ExplosionArchetype *deadguy = (ExplosionArchetype *)hArchetype;
	delete deadguy;

	return 1;
}

IBaseObject * ExplosionManager::CreateInstance(HANDLE hArchetype)
{
	ExplosionArchetype *pExplosion = (ExplosionArchetype *)hArchetype;
	BT_MESH_EXPLOSION *objData = ((ExplosionArchetype *)hArchetype)->data;
	
	if (objData->objClass == OC_MEXPLODE)
	{
			Explosion * obj = new ObjectImpl<Explosion>;
			obj->objClass = OC_MEXPLODE;
#if 0
			obj->range = objData->range;
#endif
			obj->timeToLive = obj->totalTime = 3.0;//objData->lifeTime;

			obj->data = objData;
			
			obj->secondaryBlastType = pExplosion->secondaryBlastType;
			obj->pieceBlastType  = pExplosion->pieceBlastType;
			obj->catastrophicBlastType = pExplosion->catastrophicBlastType;
			obj->rippleBlastType = pExplosion->rippleBlastType;
			obj->fireTrail = pExplosion->fireTrail;
			return obj;
	}
	
	return 0;
}

void ExplosionManager::EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info)
{
}

//----------------------------------------------------------------------------------------------
//
struct _bang : GlobalComponent
{
	struct ExplosionManager *explosionMgr;

	virtual void Startup (void)
	{
		explosionMgr = new DAComponent<ExplosionManager>;
		AddToGlobalCleanupList((IDAComponent **) &explosionMgr);
	}

	virtual void Initialize (void)
	{
		explosionMgr->Init();
	}
};

static _bang bang;

/*void ExplodeInstance(IBaseObject *obj,PARCHETYPE arch)
{
	IBaseObject *explosion;
	if ((explosion = ARCHLIST->CreateInstance(arch)) != 0)
	{
		OBJPTR<IExplosion> explode;

		if (explosion->QueryInterface(IExplosionID, explode))
		{
			OBJLIST->AddObject(explosion);
			Vector pos = ENGINE->get_position(obj->instanceIndex);
			explode->Bust();

		//	explode->InitExplosion(pos, dummy_get_mass(obj->instanceIndex), obj);
		}
	}
}*/
//---------------------------------------------------------------------------
//------------------------End Explosion.cpp----------------------------------
//---------------------------------------------------------------------------
