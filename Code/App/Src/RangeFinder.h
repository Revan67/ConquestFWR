#ifndef RANGEFINDER_H
#define RANGEFINDER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               RangeFinder.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/RangeFinder.h 9     4/17/00 8:35a Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef MGLOBALS_H
#include <MGlobals.h>
#endif

#ifndef OBJLIST_H
#include "Objlist.h"
#endif

#ifndef __INV_SQRT
#include <inv_sqrt.h>
#endif

//--------------------------------------------------------------------------//
//
struct RangeFinder : RangeFinderSaveLoad
{
	OBJPTR<IBaseObject> closestAdmiral;

	SINGLE calcRangeError (IBaseObject * const _this, IBaseObject * target, IBaseObject * owner)
	{
		U32 id;
		const Vector targetVel = target->GetVelocity();
		const Vector myVel = owner->GetVelocity();
		bool bMoving = (fabs(targetVel.x) > 50 || fabs(targetVel.y) > 50 || fabs(myVel.x) > 50 || fabs(myVel.y) > 50);

		if ((id = target->GetPartID()) != targetID || bMoving)
		{
			// new target!
			targetID = id;

			SINGLE dist = (target->GetTransform().translation - _this->GetTransform().translation).magnitude();

			if ((rand() & 7) == 0)
			{
				IBaseObject * ptr = OBJLIST->FindClosestAdmiral(owner);
				InitObjectPointer(closestAdmiral, SYSVOLATILEPTR,ptr,0);
			}

			accuracy = MGlobals::GetBaseTargetingAccuracy(owner, target, closestAdmiral);

			int sum = (rand() & 511) + (rand() & 511) + (rand() & 511) + (rand() & 511);
			rangeError = dist * ( (float(sum) / 2048) * (1 - accuracy) );
		}
		else  // close in on target
		{
			rangeError *=  ( (float(rand() & 2047) / 2048) * (1 - accuracy) );
		}

		return rangeError;
	}

	Vector calculateErrorPosition (const Vector & pos)
	{
		Vector result;
		SINGLE angle = (float(rand() & 2047) / 2048) * 2 * PI;

		result.x = cos(angle) * rangeError;
		result.y = sin(angle) * rangeError;
		result.z = ((float(rand() & 2047) / 2048) - 0.5F) * rangeError;

		return result + pos;
	}

	void saveRangeFinder (RangeFinderSaveLoad & save)
	{
		save = *static_cast<RangeFinderSaveLoad *>(this);
	}

	void loadRangeFinder (const RangeFinderSaveLoad & load)
	{
		*static_cast<RangeFinderSaveLoad *>(this)= load;
	}

	static Vector leadTarget (const Vector & _startPos, const Vector & _targetPos, const Vector & _targetVel, const SINGLE _weaponVelocity);

	static Vector leadTargetFast (const Vector & _startPos, const Vector & _targetPos, const Vector & _targetVel, const SINGLE _weaponVelocity);

};
//---------------------------------------------------------------------------------------------
// return lead vector relative to target
//
#define MAX_LEAD_ITERATIONS 32
inline Vector RangeFinder::leadTarget (const Vector & _startPos, const Vector & _targetPos, const Vector & _targetVel, const SINGLE _weaponVelocity)
{
	Vector startPos=_startPos;
	Vector targetPos=_targetPos;
	const Vector targetVel=_targetVel/16;
	const SINGLE weaponVelocity = _weaponVelocity / 16;
	int i=0;
	SINGLE olddist=0;
	Vector straight;

	while (i < MAX_LEAD_ITERATIONS)
	{
		Vector dir = targetPos - startPos;
		SINGLE dist = dir.fast_magnitude();

		if (i && dist > olddist)
		{
			// back up one
			startPos -= straight*weaponVelocity;				
			break;
		}

		if (dist)
			startPos += (weaponVelocity/dist) * dir;		// move in direction of target
		targetPos += targetVel;
		//
		// calculate real startPos given straight-line travel
		// 
		Vector temp = startPos - _startPos;
		if (temp.fast_magnitude())
		{
			straight = temp.fast_normalize();
			if (i)
				startPos = straight*((i+1)*weaponVelocity) + _startPos;
		}

		olddist = dist;
		i++;
	}

	return (startPos - _targetPos);
}
//---------------------------------------------------------------------------------------------
//
inline Vector RangeFinder::leadTargetFast (const Vector & _startPos, const Vector & _targetPos, const Vector & _targetVel,
										   const SINGLE _weaponVelocity)
{
	Vector k = _startPos-_targetPos;
	SINGLE c = k.x*k.x+k.y*k.y+k.z*k.z;
	SINGLE b = 2*(k.x*_targetVel.x+k.y*_targetVel.y+k.z*_targetVel.z);
	SINGLE a = _targetVel.x*_targetVel.x+_targetVel.y*_targetVel.y+_targetVel.z*_targetVel.z-(_weaponVelocity*_weaponVelocity);

	SINGLE det = (b*b)-(4*a*c);
	SINGLE t;
	if(det < 0)
		return Vector(0,0,0);
	else if(det == 0)
		t = (-b)/(2*a);
	else
	{
		SINGLE sq = SQRT.Sqrt(det);
		SINGLE t1 = ((-b)+sq)/(2*a);
		SINGLE t2 = ((-b)-sq)/(2*a);
		if(t1 < 0)
		{
			if(t2 <0)
			{
				return Vector(0,0,0);
			}
			else
			{
				t = t2;
			}
		}
		else
		{
			if(t2 <0)
			{
				t = t1;
			}
			else
			{
				t = (t1<t2)?t1:t2;
			}
		}
	}
	return _targetVel*t;
}

//---------------------------------------------------------------------------------------------
//-------------------------------End RangeFinder.h---------------------------------------------
//---------------------------------------------------------------------------------------------
#endif