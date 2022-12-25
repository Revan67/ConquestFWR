#ifndef TOBJFCONTROL_H
#define TOBJFCONTROL_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               TObjFControl.h                              //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjFControl.h 3     5/18/00 9:13a Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef SUPERTRANS_H
#include "SuperTrans.h"
#endif

#ifndef DBASEDATA_H
#include "DBaseData.h"
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
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#define ObjectFControl _CoFC

template <class Base> 
struct _NO_VTABLE ObjectFControl : public Base
{
	struct UpdateNode       updateNode;
	struct InitNode			initNode;
	struct PhysUpdateNode	physUpdateNode;

	typename typedef Base::INITINFO CONTROLINITINFO;

	SINGLE MAX_FORWARD_VELOCITY;

private:
    SINGLE FORWARD_ACCELERATION;
    SINGLE MAX_ANG_VELOCITY;				// used for yaw

	bool bRelVecValid;
	bool bThrustEnabled;					// go forward using acceleration
	bool bRelRotateValid;
	Vector relVec;
	SINGLE relYaw, relRoll, relPitch;

public:

	//----------------------------------
	
	ObjectFControl (void);

	/* ObjectFControl methods */

	BOOL32 updateDynamics (void);

	void initDynamics (const CONTROLINITINFO & data);

	void updateDynamics (SINGLE dt);

	/* higher level control methods */

	void rotateShip (SINGLE relYaw, SINGLE relRoll, SINGLE relPitch);

	void setAltitude (SINGLE relAltitude);

	void setPosition (const Vector & relPosition);

	void enableThrusters (void)
	{
		bThrustEnabled = true;
	}

	bool isThrustersEnabled (void) const
	{
		return bThrustEnabled;
	}
};

//---------------------------------------------------------------------------
//
template <class Base> 
ObjectFControl< Base >::ObjectFControl (void) :
					updateNode(this, UpdateProc(&ObjectFControl::updateDynamics)),
					initNode(this, InitProc(&ObjectFControl::initDynamics)),
					physUpdateNode(this, PhysUpdateProc(&ObjectFControl::updateDynamics))
{
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectFControl< Base >::initDynamics (const CONTROLINITINFO & data)
{
	MAX_FORWARD_VELOCITY = data.pData->dynamicsData.maxLinearVelocity;
    FORWARD_ACCELERATION = data.pData->dynamicsData.linearAcceleration * ELAPSED_TIME;
    MAX_ANG_VELOCITY = data.pData->dynamicsData.maxAngVelocity;
}
//---------------------------------------------------------------------------
//
template <class Base> 
BOOL32 ObjectFControl< Base >::updateDynamics (void)
{
	/*
	if (bThrustEnabled)
	{
		Vector tmp = -transform.get_k();
		SINGLE vel = dot_product(velocity, tmp);		
		double new_accel_mag=vel;
		
		new_accel_mag += FORWARD_ACCELERATION;

		if (new_accel_mag > MAX_FORWARD_VELOCITY)
			new_accel_mag = MAX_FORWARD_VELOCITY;
		else
		if (new_accel_mag < -MAX_FORWARD_VELOCITY)
			new_accel_mag = -MAX_FORWARD_VELOCITY;

		tmp *= new_accel_mag;
		velocity = tmp;
		bThrustEnabled = false;
	}
	*/
	bThrustEnabled = false;

	return 1;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectFControl< Base >::rotateShip (SINGLE _relYaw, SINGLE _relRoll, SINGLE _relPitch)
{
	relYaw = _relYaw;
	relRoll = _relRoll;
	relPitch = _relPitch;
	bRelRotateValid = true;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectFControl< Base >::setAltitude (SINGLE relAltitude)
{
	bRelVecValid = true;
	relVec.z = relAltitude;
}
//---------------------------------------------------------------------------
// move the ship using thrusters
// return TRUE if we are there.
//
template <class Base> 
void ObjectFControl< Base >::setPosition (const Vector & relPosition)
{
	bRelVecValid = true;
	relVec = relPosition;
}
//---------------------------------------------------------------------------
//
template <class Base> 
void ObjectFControl< Base >::updateDynamics (SINGLE dt)
{
	if (bThrustEnabled)
	{
		velocity = transform.get_j() * -MAX_FORWARD_VELOCITY;
		transform.translation += (velocity * dt);
		bRelVecValid = false;
	}
	else
	if (bRelVecValid)
	{
		SINGLE relMag = relVec.fast_magnitude();
		if (relMag > 0)
		{
			SINGLE speed =  relMag / dt;
			if (speed > MAX_FORWARD_VELOCITY)
				speed = MAX_FORWARD_VELOCITY;
			else
				bRelVecValid = false;

			velocity = relVec * (speed / relMag);
			Vector rel = velocity * dt;

			transform.translation += rel;
			relVec -= rel;
		}
		else
			bRelVecValid = false;
	}

	if (bRelRotateValid)
	{
		if (relYaw != 0)
		{
			SINGLE speed = relYaw / dt;
			if (speed > MAX_ANG_VELOCITY)
			{
				speed = MAX_ANG_VELOCITY;
				SINGLE angle = speed * dt;
				relYaw -= angle;
				transform.rotate_about_k(angle);
			}
			else
			if (speed < -MAX_ANG_VELOCITY)
			{
				speed = -MAX_ANG_VELOCITY;
				SINGLE angle = speed * dt;
				relYaw -= angle;
				transform.rotate_about_k(angle);
			}
			else
			{
				transform.rotate_about_k(relYaw);
				relYaw = 0;
			}
		}
		if (relRoll != 0)
		{
			SINGLE speed = relRoll / dt;
			if (speed > MAX_ANG_VELOCITY)
			{
				speed = MAX_ANG_VELOCITY;
				SINGLE angle = speed * dt;
				relRoll -= angle;
				transform.rotate_about_j(angle);
			}
			else
			if (speed < -MAX_ANG_VELOCITY)
			{
				speed = -MAX_ANG_VELOCITY;
				SINGLE angle = speed * dt;
				relRoll -= angle;
				transform.rotate_about_j(angle);
			}
			else
			{
				transform.rotate_about_j(relRoll);
				relRoll = 0;
			}
		}
		if (relPitch != 0)
		{
			SINGLE speed = relPitch / dt;
			if (speed > MAX_ANG_VELOCITY)
			{
				speed = MAX_ANG_VELOCITY;
				SINGLE angle = speed * dt;
				relPitch -= angle;
				transform.rotate_about_i(angle);
			}
			else
			if (speed < -MAX_ANG_VELOCITY)
			{
				speed = -MAX_ANG_VELOCITY;
				SINGLE angle = speed * dt;
				relPitch -= angle;
				transform.rotate_about_i(angle);
			}
			else
			{
				transform.rotate_about_i(relPitch);
				relPitch = 0;
			}
		}
		
		if (relYaw==0 && relRoll==0 && relPitch==0)
			bRelRotateValid = false;
	}

}
//---------------------------------------------------------------------------
//------------------------End TObjFControl.h----------------------------------
//---------------------------------------------------------------------------
#endif