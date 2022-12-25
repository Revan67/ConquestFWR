//--------------------------------------------------------------------------//
//                                                                          //
//                                 Debris.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "TObject.h"
#include "Camera.h"
#include "ObjList.h"
#include "filesys.h"
#include "TObjTrans.h"
#include "TObjFrame.h"
#include "TObjPhys.h"
#include "Material.h"
#include "MeshRender.h"
#include "IExplosion.h"
#include "ArchHolder.h"
#include "UserDefaults.h"
#include "FogOfWar.h"
#include "MeshExplode.h"
#include "CQLight.h"
#include "ICamera.h"

#include <View2d.h>
#include <TSmartPointer.h>
#include <IParticleSystem.h>
#include <Physics.h>
#include <3DMath.h>
#include <Engine.h>
#include <Renderer.h>

//--------------------------------------------------------------------------//
//
struct DEBRIS_SAVELOAD
{
	TRANS_SAVELOAD trans_SL;
};
struct DEBRIS_ARCHETYPE
{
	S32 archIndex;
	IMeshArchetype * meshArch;

	Vector rigidBodyArm;

	DEBRIS_ARCHETYPE (void)
	{
		meshArch = NULL;
		archIndex = -1;
		rigidBodyArm.zero();
	}
};
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE Debris : public ObjectPhysics<ObjectTransform<ObjectFrame<IBaseObject,DEBRIS_SAVELOAD,DEBRIS_ARCHETYPE> > >, IDebris
{
	BEGIN_MAP_INBOUND(Debris)
	_INTERFACE_ENTRY(IDebris)
	_INTERFACE_ENTRY(IBaseObject)
	_INTERFACE_ENTRY(IPhysicalObject)
	END_MAP()

	//------------------------------------------

	SINGLE timeToLive,_lifeTime;
	PARCHETYPE pBlastType;
	INSTANCE_INDEX fireIndex;
	IBaseObject *blast;
	bool bBlasting:1;
	bool bExplode:1;
// the following should be retrieved when needed (jy)
/*
	Material *mat;
	S32 mat_cnt;
*/
//	SINGLE r,g,b;
	Vector rgb;
	U32 systemID;
	DEBRIS_ARCHETYPE debrisArchetype;
	MeshChain *mc;

//	BOOL32 bFireVisible;

	//------------------------------------------

	Debris (void)
	{
		timeToLive = 5;
		bExplode = false;
		fireIndex = INVALID_INSTANCE_INDEX;
	}

	virtual ~Debris (void);	// See ObjList.cpp

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	bool get_pbs(float & cx,float & cy,float & radius,float & depth);

	virtual BOOL32 Update (void);

	virtual void PhysicalUpdate (SINGLE dt)
	{
		if (bBlasting == 0)
		{
			timeToLive -= dt;
			if (timeToLive < 0)
				timeToLive = 0;
		}

		if (fireIndex != INVALID_INSTANCE_INDEX)
			ENGINE->update_instance(fireIndex, 0, dt);

		FRAME_physicalUpdate(dt);

		ENGINE->update_instance(instanceIndex,0,0);

		if (blast)
			blast->PhysicalUpdate(dt);
	}

	virtual void CastVisibleArea (void);				// set visible flags of objects nearby

	virtual U32 GetSystemID (void) const
	{
		return systemID;
	}

	virtual void Render ();

	virtual void TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer);

	void explode (void);

	virtual void RegisterChild (INSTANCE_INDEX _child);
};

//----------------------------------------------------------------------------------
//
Debris::~Debris (void)
{
	if (fireIndex != INVALID_INSTANCE_INDEX)
		ENGINE->destroy_instance(fireIndex);
	if (pBlastType)
		ARCHLIST->Release(pBlastType, OBJREFNAME);
	if (blast)
		delete blast;
	if (mc)
		delete mc;
}

bool Debris::get_pbs(float & cx,float & cy,float & radius,float & depth)
{
	bool result = false;
	CQASSERT(instanceIndex != INVALID_INSTANCE_INDEX);
	float obj_rad = mc->mi[0]->radius;
	Vector wcenter = mc->mi[0]->sphere_center;
	const Transform *cam2world = CAMERA->GetTransform();

	Vector vcenter = cam2world->inverse_rotate_translate(transform*wcenter);
				
	// Make sure object is in front of near plane.
	if (vcenter.z < -MAINCAM->get_znear())
	{
		const struct ViewRect * pane = MAINCAM->get_pane();
		
		float x_screen_center = float(pane->x1 - pane->x0) * 0.5f;
		float y_screen_center = float(pane->y1 - pane->y0) * 0.5f;
		float screen_center_x = pane->x0 + x_screen_center;
		float screen_center_y = pane->y0 + y_screen_center;
		
		float w = -1.0 / vcenter.z;
		float sphere_center_x = vcenter.x * w;
		float sphere_center_y = vcenter.y * w;
		
		cx = screen_center_x + sphere_center_x * MAINCAM->get_hpc()*MAINCAM->get_znear();
		cy = screen_center_y + sphere_center_y * MAINCAM->get_vpc()*MAINCAM->get_znear();
		
		float center_distance = vcenter.magnitude();
		
		if(center_distance >= obj_rad)
		{
			float dx = fabs(cx - screen_center_x);
			float dy = fabs(cy - screen_center_y);
			
			//changes 1/26 - rmarr
			//function should now not return TRUE with obscene radii
			float outer_angle = asin(obj_rad / center_distance);
			sphere_center_x = fabs(sphere_center_x);
			float inner_angle = atan(sphere_center_x);
			
			//	float near_plane_radius = tan(inner_angle + outer_angle);
			//	near_plane_radius -= sphere_center_x;
			//	radius = near_plane_radius * camera->get_hpc();
			
			float near_plane_radius = tan(inner_angle - outer_angle);
			near_plane_radius = sphere_center_x-near_plane_radius;
			radius = near_plane_radius * MAINCAM->get_hpc()*MAINCAM->get_znear();
			
			int view_w = (pane->x1 - pane->x0 + 1) >> 1;
			int view_h = (pane->y1 - pane->y0 + 1) >> 1;
			
			if ((dx < (view_w + radius)) && (dy < (view_h + radius)))
			{
				depth = -vcenter.z;
				result = true;
			}
		}
	}
				
	return result;
}
//----------------------------------------------------------------------------------
//
void Debris::TestVisible (const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer)
{
	float center_x, center_y, radius, depth;
	
	bVisible = 0;
	if (GetSystemID() == currentSystem)
	{
		bool bFireVisible=0;
		if (fireIndex != INVALID_INSTANCE_INDEX)
		{
			ENGINE->set_position(fireIndex,transform.translation);
			bFireVisible = (REND->get_instance_projected_bounding_sphere(fireIndex, MAINCAM, LODPERCENT, center_x, center_y, radius, depth) != 0);
		}

		if (bBlasting)
			bVisible = bFireVisible;
		else
			bVisible = ((get_pbs( center_x, center_y, radius, depth) || bFireVisible) &&
				(FOGOFWAR->CheckVisiblePosition(transform.translation) || defaults.bVisibilityRulesOff) );
		
		if (blast)
		{
			blast->TestVisible(defaults,currentSystem,currentPlayer);
			bVisible |= blast->bVisible;
		}
	}
}
//----------------------------------------------------------------------------------
//
void Debris::CastVisibleArea (void)
{
	SetVisibilityFlags(GetVisibilityFlags());
}
//----------------------------------------------------------------------------------
//
BOOL32 Debris::Update (void)
{
	BOOL32 bHangAround = TRUE;
	
	if (bBlasting)
	{
		if (blast)
			bHangAround = blast->Update();
		else
			bHangAround = FALSE;
	}
	else
	{
		CQASSERT (velocity.magnitude() < 1e6);

		if (timeToLive <= 0)
		{
			if (bExplode)
			{
				explode();
			}
			else
				bHangAround = FALSE;
		}
	}
	return bHangAround;
}

void Debris::Render (void)
{
	if (bVisible)
	{
		if (!bBlasting)
		{
//			BATCH->set_render_state(D3DRS_ZENABLE,TRUE);
//			BATCH->set_render_state(D3DRS_ZWRITEENABLE,TRUE);
			
			if (!bExplode)
			{
				for (int c=0;c<mc->numChildren;c++)
				{
					for (int i=0;i<mc->mi[c]->faceGroupCnt;i++)
					{
						if (mc->mi[c]->bHasMesh)
							mc->mi[c]->fgi[i].a = 255*timeToLive/_lifeTime;
					}
				}
			}

		//	LIGHTS->ActivateBestLights(transform.translation,4,4000);
			TreeRender(mc->mi,mc->numChildren);
		}
		//Transform trans = ENGINE->get_transform(instanceIndex);
		
		/*	if (fireIndex != INVALID_INSTANCE_INDEX)
		{
		
		  COMPTR<IParticleSystem> IPS;
		  
			if(ENGINE->query_instance_interface( _child, IID_IParticleSystem, (IDAComponent **)(void **)IPS ) == GR_OK)
			{
			ParticleSystemParameters psp;
			IPS->get_parameters( &psp );
			
			  psp.bounding_sphere_radius=1000;
			  psp.
			  }
			  else
			  CQBOMB0("query_instance_interface() failed");
			  EventDef eventDef;
			  ENGINE->get_instance_property(fireIndex,"EventDef",&eventDef);
			  eventDef.color = rgb*timeToLive/_lifeTime; //*= 0.98;
			  eventDef.radius = 1000;
			  
				ENGINE->set_instance_property(fireIndex,"EventDef",&eventDef);
				ENGINE->set_position(fireIndex,PHYSICS->get_center_of_mass(instanceIndex));
				ENGINE->render_instance(MAINCAM, fireIndex, LODPERCENT, 0,0);
				}*/
	}
	
	if (blast)
		blast->Render();
}
//----------------------------------------------------------------------------------
//
void Debris::explode (void)
{
	mc->bOwnsChildren = false;
	CQASSERT (bBlasting == false);

	U32 i;
	MeshChain *objArray[2];
	PHYS_CHUNK physChunks[2];
//	SINGLE mass = PHYSICS->get_mass(instanceIndex);

//	Vector pos = ENGINE->get_position(instanceIndex);
	TRANSFORM trans = transform;

	if (fireIndex != INVALID_INSTANCE_INDEX)
	{
		ENGINE->destroy_instance(fireIndex);
		fireIndex = INVALID_INSTANCE_INDEX;
	}

	SINGLE force = 60 * 1000;	// PHYSICS->get_mass(instanceIndex);
	U32 numDebris = ExplodeInstance(mc, force, 2, 2, objArray,physChunks);
	for (i=0;i<numDebris;i++)
	{
		Vector vel = ENGINE->get_velocity(objArray[i]->mi[0]->instanceIndex);
		float speed = vel.magnitude();
		if (speed > 1500)
		{
			vel *= 1500.0 / speed;
			ENGINE->set_velocity(objArray[i]->mi[0]->instanceIndex, vel);
			Vector ang = ENGINE->get_angular_velocity(objArray[i]->mi[0]->instanceIndex);
			// NOTE: don't use normalize here, because ang may be 0 vector
			SINGLE mag = ang.magnitude();
			if (mag)
				ang *= (10 / mag);
			ENGINE->set_angular_velocity(objArray[i]->mi[0]->instanceIndex, ang);
		}
		objArray[i]->bOwnsChildren = true;
		IBaseObject *obj = CreateDebris(1,FALSE,objArray[i],0, systemID,&physChunks[i]);
		if (obj)
		{
			obj->pArchetype = 0;
			obj->objClass = OC_SHRAPNEL;
			OBJLIST->AddObject(obj);
		
//			PHYSICS->set_dynamic(objArray[i], DS_DYNAMIC);
		}
	}

	bBlasting = TRUE;
	//CQASSERT(pBlastType);
	
	HARCH arch = instanceIndex;
	if (arch != INVALID_ARCHETYPE_INDEX)
	{
		Vector centroid;
		if (REND->get_archetype_centroid(arch, LODPERCENT, centroid))
			//com = PHYSICS->get_center_of_mass(arch);
		{
			//what does this code do?
			const Matrix & R = transform;
			Vector pos = trans.get_position();
			pos += R * centroid;
			trans.set_position(pos);
			
			if (pBlastType)
			{
				if (IBaseObject *explosion = CreateBlast(pBlastType,trans, systemID))
				{
					//	OBJLIST->AddObject(explosion);
					blast = explosion;
				//	bBlasting = TRUE;
				}
			}
			else
			{
				blast=0;
			//	bBlasting=TRUE;
			}
		}
	}
	
//	ENGINE->destroy_instance(instanceIndex);
//	instanceIndex = -1;

}

void Debris::RegisterChild (INSTANCE_INDEX _child)
{
	fireIndex = _child;
/*	CQASSERT(_child != INVALID_INSTANCE_INDEX);

	COMPTR<IParticleSystem> IPS;

	if(ENGINE->query_instance_interface( _child, IID_IParticleSystem, (IDAComponent **)(void **)IPS ) == GR_OK)
	{
		ParticleSystemParameters psp;
		IPS->get_parameters( &psp );

		rgb = psp.color;
	}
	else
		CQBOMB0("query_instance_interface() failed");*/
}
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//
struct IBaseObject * __stdcall CreateDebris (SINGLE lifeTime, BOOL32 bExplodeUponExpiration,MeshChain *_mc, PARCHETYPE _pBlastType, U32 systemID,PHYS_CHUNK *chunk)
{
	Debris * obj = new ObjectImpl<Debris>;
	CQASSERT(obj);
	if (obj)
	{
		obj->mc = _mc;
		obj->debrisArchetype.rigidBodyArm = chunk->arm;
		obj->FRAME_init(obj->debrisArchetype);
		Vector test_vel = ENGINE->get_velocity(_mc->mi[0]->instanceIndex);
		Vector test_ang_vel = ENGINE->get_angular_velocity(_mc->mi[0]->instanceIndex);
		ENGINE->set_instance_handler(_mc->mi[0]->instanceIndex,obj);
		CQASSERT(_mc->mi[0]->instanceIndex != -1);
		test_vel = ENGINE->get_velocity(_mc->mi[0]->instanceIndex);
		test_ang_vel = ENGINE->get_angular_velocity(_mc->mi[0]->instanceIndex);

		obj->timeToLive = lifeTime;
		obj->_lifeTime = lifeTime;
		obj->bExplode = (bExplodeUponExpiration != 0);
		obj->systemID = systemID;
		if ((obj->pBlastType = _pBlastType) != 0)
			ARCHLIST->AddRef(_pBlastType, OBJREFNAME);
		return obj;
	}
	return 0;
}


struct IBaseObject * __stdcall CreateDebris (SINGLE lifeTime, BOOL32 bExplodeUponExpiration,INSTANCE_INDEX _index, PARCHETYPE _pBlastType, U32 systemID,PHYS_CHUNK *chunk)
{
	CQTRACE10("Obsolete CreateDebris call");
	ENGINE->destroy_instance(_index);
	return 0;
}

//---------------------------------------------------------------------------
//--------------------------End TestObj.cpp----------------------------------
//---------------------------------------------------------------------------
