#ifndef TOBJTRANS_H
#define TOBJTRANS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 TObjTrans.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjTrans.h 25    6/30/00 3:02p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef SUPERTRANS_H
#include "SuperTrans.h"
#endif

#ifndef ENGINE_H
#include <Engine.h>
#endif

#ifndef _3DMATH_H
#include <3DMath.h>
#endif

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

#ifndef DBASEDATA_H
#include <DBaseData.h>
#endif

#include <IMeshManager.h>

#define ObjectTransform _CoT
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
template <class Base=IBaseObject> 
struct _NO_VTABLE ObjectTransform : public Base, IEngineInstance, IPhysicalObject, IMeshCallback
{
	S32  instanceIndex;		

	IMeshInstance * instanceMesh;

	TRANSFORM transform;
	Vector velocity, ang_velocity;

private:
	bool initialized;
	SINGLE instanceRadius;
	Vector instanceCenter;
public:

	typename typedef Base::SAVEINFO TRANSSAVEINFO;
	typename typedef Base::INITINFO TRANSINITINFO;

	struct SaveNode			saveNode;
	struct LoadNode         loadNode;
	struct InitNode			initNode;

	//----------------------------------
	
	ObjectTransform (void);

	~ObjectTransform (void);

	/* IEngineInstance methods */

	virtual void COMAPI initialize_instance (INSTANCE_INDEX index)
	{
	//	CQASSERT(instanceIndex == -1 && initialized==false);
		instanceIndex = index;
		
		initialized = true;
	}

	virtual void  COMAPI create_instance (INSTANCE_INDEX index)
	{
		CQASSERT(index == instanceIndex);
	}

	virtual void   COMAPI destroy_instance (INSTANCE_INDEX index)
	{
		CQASSERT(index == instanceIndex);
		instanceIndex = -1;
		initialized = false;
	}
	virtual void   COMAPI set_position (INSTANCE_INDEX index, const Vector & position)
	{
		transform.set_position(position);
	}
	virtual const Vector & COMAPI get_position (INSTANCE_INDEX index) const
	{
		return transform.get_position();
	}
	virtual void   COMAPI set_orientation (INSTANCE_INDEX index, const Matrix & orientation)
	{
		transform.set_orientation(orientation);
	}
	virtual const Matrix & COMAPI get_orientation (INSTANCE_INDEX index) const
	{
		return transform.get_orientation();
	}
	virtual void      COMAPI set_transform (INSTANCE_INDEX index, const Transform & _transform)
	{
		transform = _transform;
	}
	virtual const Transform & COMAPI get_transform (INSTANCE_INDEX index) const
	{
		return transform;
	}
	virtual const Vector & COMAPI get_velocity (INSTANCE_INDEX object) const
	{
		return velocity;
	}
	virtual const Vector & COMAPI get_angular_velocity (INSTANCE_INDEX object) const
	{
		return ang_velocity;
	}

	virtual void COMAPI set_velocity (INSTANCE_INDEX object, const Vector & vel)
	{
		CQASSERT(vel.magnitude() < 1e6 && "Bother Rob");
		velocity = vel;
	}
	virtual void COMAPI set_angular_velocity (INSTANCE_INDEX object, const Vector & ang)
	{
		ang_velocity = ang;
	}
	virtual void COMAPI get_centered_radius (INSTANCE_INDEX object,SINGLE *r,Vector *c) const
	{
		*r = instanceRadius;
		*c = instanceCenter;
	}
	virtual void COMAPI set_centered_radius (INSTANCE_INDEX object, const SINGLE r,const Vector &c)
	{
		instanceRadius = r;
		instanceCenter = c;
	}

	/* IMeshCallback */
	virtual void AnimationCue(struct IMeshInstance * meshInstance, const char * cueName) {};



	/* IBaseObject methods */

	virtual S32 GetObjectIndex (void) const
	{
		return instanceIndex;
	}
	
	virtual const TRANSFORM & GetTransform (void) const
	{
		return transform;
	}

	virtual Vector GetVelocity (void)
	{
		return velocity;
	}

	/* IPhysicalObject methods */

	virtual void SetSystemID (U32 newSystemID)
	{
	}

	virtual void SetPosition (const Vector & position, U32 newSystemID)
	{
		transform.translation = position;
	}

	virtual void SetTransform (const TRANSFORM & _transform, U32 newSystemID)
	{
		transform = _transform;
	}

	virtual void SetVelocity (const Vector & _velocity)
	{
		velocity = _velocity;
	}

	virtual void SetAngVelocity (const Vector & angVelocity)
	{
		ang_velocity = angVelocity;
	}

	static int makeChildrenIntangible (INSTANCE_INDEX index);

	void saveTransform (TRANSSAVEINFO & saveStruct);
	void loadTransform (TRANSSAVEINFO & saveStruct);
	void initTransform (const TRANSINITINFO & data);
};

//---------------------------------------------------------------------------
//
template <class Base> 
ObjectTransform< Base >::ObjectTransform (void) :
					saveNode(this, SaveLoadProc(&ObjectTransform::saveTransform)),
					loadNode(this, SaveLoadProc(&ObjectTransform::loadTransform)),
					initNode(this, InitProc(&ObjectTransform::initTransform))
{
	instanceMesh = 0;
	instanceIndex = -1;
}
//---------------------------------------------------------------------------
//
template <class Base> 
ObjectTransform< Base >::~ObjectTransform (void) 
{
	if(instanceMesh)
	{
		MESHMAN->DestroyMesh(instanceMesh);
		instanceMesh = NULL;
		instanceIndex = -1;
	}
	else if (instanceIndex != -1)
	{
		CQASSERT(initialized);
		ENGINE->destroy_instance(instanceIndex);
	}
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectTransform< Base >::saveTransform (TRANSSAVEINFO & save)
{
	save.trans_SL.position = transform.translation;
	save.trans_SL.ang_position = transform.get_yaw();
	save.trans_SL.velocity = velocity;
	save.trans_SL.ang_velocity = ang_velocity;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectTransform< Base >::loadTransform (TRANSSAVEINFO & load)
{
	transform.translation = load.trans_SL.position;
	transform.rotate_about_k(load.trans_SL.ang_position);
	velocity = load.trans_SL.velocity;
	ang_velocity = load.trans_SL.ang_velocity;
	ENGINE->update_instance(instanceIndex,0,0);
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectTransform< Base >::initTransform (const TRANSINITINFO & data)
{
	if (data.archIndex != -1)
	{
		ENGINE->create_instance2(data.archIndex, this);
		CQASSERT(instanceIndex != -1);
	}
	else if (data.meshArch)
	{
		instanceMesh = MESHMAN->CreateMesh(data.meshArch,this);
		if(instanceMesh)
			instanceMesh->SetCallback(this);
		CQASSERT(instanceIndex != -1);
	}
	else
		initialized = true;
}
//---------------------------------------------------------------------------
//------------------------End TObjTrans.h----------------------------------
//---------------------------------------------------------------------------
#endif