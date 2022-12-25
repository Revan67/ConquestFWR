#ifndef TOBJCONTROL_H
#define TOBJCONTROL_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               TObjControl.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TObjControl.h 30    8/30/00 11:24p Jasony $
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

#define MAX_UPDOWN_VELOCITY 150.0F
#define ANG_SCALE (0.3)          // angVelocity * scale = angVelocity for pitch, roll
#define ZXLATE_SCALE (0.4)		 // scales z accel, velocity
#define ObjectControl _CoC

template <class Base> 
struct _NO_VTABLE ObjectControl : public Base, DYNAMICS_DATA
{
	struct UpdateNode       updateNode;
	struct InitNode			initNode;
	struct PhysUpdateNode	physUpdateNode;

	typename typedef Base::INITINFO CONTROLINITINFO;

	//----------------------------------
	
	ObjectControl (void);

	/* ObjectControl methods */

	bool rotateShip (SINGLE relYaw, SINGLE relRoll, SINGLE relPitch)
	{
		targetYaw   = fixAngle(transform.get_yaw() + relYaw);
		targetRoll  = fixAngle(transform.get_roll() + relRoll);
		targetPitch = fixAngle(transform.get_pitch() + relPitch);
		bAngleValid = true;

		if (fabs(relYaw) > 0.1 || fabs(relRoll) > 0.1 || fabs(relPitch) > 0.1)
			return false;
		return true;
	}

	void rotateTo (SINGLE absYaw, SINGLE absRoll, SINGLE absPitch)
	{
		targetYaw   = absYaw;
		targetRoll  = absRoll;
		targetPitch = absPitch;
		bAngleValid = true;
	}

	void setAltitude (SINGLE relAltitude)
	{
		if (bPositionValid == false)
		{
			targetPos = transform.translation;
			targetPos.z += relAltitude;
		}
		else
			targetPos.z = transform.translation.z + relAltitude;

		bPositionValid = true;
	}

	bool setPosition (const Vector & relPosition)
	{
		targetPos = transform.translation;
		targetPos += relPosition;
		bPositionValid = true;

		return (relPosition.fast_magnitude() < 50);
	}

	bool setPosition (const Vector & relPosition, SINGLE _finalVel)
	{
		bool result = setPosition(relPosition);
		finalVelMag = _finalVel;
		bFinalVelocityValid = true;
		return result;
	}

	void moveTo (const Vector & absPosition)
	{
		targetPos = absPosition;
		bPositionValid = true;
	}
	
	void moveTo (const Vector & absPosition, SINGLE _finalVel)
	{
		moveTo(absPosition);
		finalVelMag = _finalVel;
		bFinalVelocityValid = true;
	}

	void setThrustersOn (void)
	{
		cEnginesOn = 2;
	}

	bool areThrustersOn (void) const
	{
		return (cEnginesOn > 0);
	}

	const DYNAMICS_DATA & getDynamicsData (void) const
	{
		return *static_cast<const DYNAMICS_DATA *>(this);
	}

	void setDynamicsData (const DYNAMICS_DATA & data)
	{
		*static_cast<DYNAMICS_DATA *>(this) = data;
	}

	void restoreDynamicsData (void)
	{
		*static_cast<DYNAMICS_DATA *>(this) = *pOrigData;
	}

    void restoreLinearDynamicsData (void)
	{
		linearAcceleration = pOrigData->linearAcceleration;
		maxLinearVelocity = pOrigData->maxLinearVelocity;
	}

	void restoreAngDynamicsData (void)
	{
		angAcceleration = pOrigData->angAcceleration;
		maxAngVelocity = pOrigData->maxAngVelocity;
	}

	static SINGLE fixAngle (SINGLE angle)
	{
		if (angle < -PI)
			angle += PI*2;
		else
		if (angle > PI)
			angle -= PI*2;
		return angle;
	}

	// override methods 
	void setAbsOverridePosition (const Vector & position)
	{
		OtargetPos = position;
		bOPositionValid = true;
	}

	void setAbsOverridePosition (const Vector & position, SINGLE _finalVel)
	{
		setAbsOverridePosition(position);
		OfinalVelMag = _finalVel;
		bOFinalVelocityValid = true;
	}

	void overrideAbsShipRotate (SINGLE yaw, SINGLE roll, SINGLE pitch)
	{
		OtargetYaw   = fixAngle(yaw);
		OtargetRoll  = fixAngle(roll);
		OtargetPitch = fixAngle(pitch);
		bOAngleValid = true;
	}

	void setDestabilize (void)
	{
		bDestabilize = true;
	}

	// avoid "floating away assert" when you are floating on purpose
	void DEBUG_resetInputCounter (void)
	{
		noInputCountA = noInputCountB = 0;
	}

    static SINGLE calculateCoastingDistance (SINGLE initialVel, SINGLE friction, SINGLE timeInterval);

	static SINGLE calculateCoastingDistance (SINGLE finalVel, SINGLE initialVel, SINGLE friction, SINGLE timeInterval);

private:

	const DYNAMICS_DATA * pOrigData;
	Vector targetPos;
	SINGLE finalVelMag;
	SINGLE targetYaw, targetRoll, targetPitch;
	bool bAngleValid;
	bool bPositionValid;
	bool bFinalVelocityValid;
	bool bPhysicalUpdateHappened;
	C8	 cEnginesOn;
	//
	// data for external overrides (fields that push things around)
	//
	bool bOAngleValid;
	bool bOPositionValid;
	bool bOFinalVelocityValid;
	bool bDestabilize;
	Vector OtargetPos;
	SINGLE OfinalVelMag;
	SINGLE OtargetYaw, OtargetRoll, OtargetPitch;
	//
	// debugging
	//
	int noInputCountA, noInputCountB;	// incrmemented when no inputs were received


	/* private methods */

	void initControl (const CONTROLINITINFO & data)
	{
		*static_cast<DYNAMICS_DATA *>(this) = data.pData->dynamicsData;
		pOrigData = & data.pData->dynamicsData;
	}

	BOOL32 updateControl (void);

	void physUpdateControl (SINGLE dt);

	static SINGLE limitAcceleration (SINGLE oldVel, SINGLE desiredVel, SINGLE acceleration, SINGLE dt);
};
//---------------------------------------------------------------------------
//
template <class Base> 
ObjectControl< Base >::ObjectControl (void) :
					updateNode(this, UpdateProc(&ObjectControl::updateControl)),
					initNode(this, InitProc(&ObjectControl::initControl)),
					physUpdateNode(this, PhysUpdateProc(&ObjectControl::physUpdateControl))
{
}
//---------------------------------------------------------------------------
//------------------------End TObjControl.h----------------------------------
//---------------------------------------------------------------------------
#endif