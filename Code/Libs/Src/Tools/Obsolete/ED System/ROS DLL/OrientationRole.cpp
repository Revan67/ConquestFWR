// Author: Shaival Varma
// --------------------------------------------------------------------------

#include "PCH.h"
#include "OrientationRole.h"
#include "Spline.h"
#include "AStaticSceneEntity.H"
#include "3DMath.h"

// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
FlaggedOrientation LinearInterpolate(const OrientationRole& role, OrientationRole::TimeStateIterator& previousIterator, OrientationRole::TimeStateIterator& nextIterator, Time currentTime, const OrientationRole::TimeStateIterator& begin, const OrientationRole::TimeStateIterator& end, LocationRole* locationRole)
{
	const Time									previousTime = (*previousIterator)->GetTime();
	const FlaggedOrientation					previousOrientation = (*previousIterator)->GetState();
												
	const Time									nextTime = (*nextIterator)->GetTime();
	const FlaggedOrientation					nextOrientation = (*nextIterator)->GetState();

	const FlaggedOrientation::InterpolationType	interpolationType = previousOrientation.GetInterpolationType();
	
	
	Orientation	tLinearInterpolatedOrientation;

	if(interpolationType == FlaggedOrientation::kLinear || interpolationType == FlaggedOrientation::kTangent)
	{
		// In either case, we need the linear interpolated orientation at the current time

		if(previousTime == nextTime)
		{
			tLinearInterpolatedOrientation = previousOrientation;
		}
		else
		{
			const float t = (currentTime - previousTime).GetTime() / (nextTime - previousTime).GetTime();

			// We need to interpolate to find the orientation.
			// The orientation at the nextTime may not be linear, so we need to compute it.
			const FlaggedOrientation	trueNextOrientation = role.GetState(nextTime);

			tLinearInterpolatedOrientation = Interpolate(previousOrientation, trueNextOrientation, t);
		}
	}

	if(interpolationType == FlaggedOrientation::kLinear)
	{
		// Nothing more to do
		return FlaggedOrientation(tLinearInterpolatedOrientation,  previousOrientation.GetInterpolationType(), previousOrientation.GetTargetEntity());
	}

	// Now only the Spline, LookAt and Tangent cases remain

	const Vector	zeroVector(0, 0, 0);
	Vector			tTangentVector;

	if(interpolationType == FlaggedOrientation::kSpline)
	{
		if(previousTime == nextTime)
		{
			return previousOrientation;
		}

		Quaternion	q0, q1, q2, q3;

		const ::Matrix	matrix1(previousOrientation.GetI(), previousOrientation.GetJ(), previousOrientation.GetK());
		const ::Matrix	matrix2(nextOrientation.GetI(), nextOrientation.GetJ(), nextOrientation.GetK());

		q1.set(matrix1);
		q2.set(matrix2);

		if(previousIterator != begin)
		{
			const Orientation	orientation0 = role.GetState((*(previousIterator - 1))->GetTime());
			const ::Matrix		matrix0(orientation0.GetI(), orientation0.GetJ(), orientation0.GetK());

			q0.set(matrix0);
		}
		else
		{
			q0 = q1;
		}

		if((nextIterator + 1) != end)
		{
			const Orientation	orientation3 = role.GetState((*(nextIterator + 1))->GetTime());
			const ::Matrix		matrix3(orientation3.GetI(), orientation3.GetJ(), orientation3.GetK());

			q3.set(matrix3);
		}
		else
		{
			q3 = q2;
		}

		const float t = (currentTime - previousTime).GetTime() / (nextTime - previousTime).GetTime();

		const Quaternion	qT = spline_squad(q0, q1, q2, q3, t);
		const ::Matrix		matrixT(qT);
	
		const Orientation	orientationT(matrixT.get_i(), matrixT.get_j(), matrixT.get_k());

		return FlaggedOrientation(orientationT, previousOrientation.GetInterpolationType(), previousOrientation.GetTargetEntity());
	}

	// Now only the LookAt and Tangent cases remain

	if(interpolationType == FlaggedOrientation::kLookAt)
	{
		// We will compute a vector from the source to the target, and then allow the tangent algorithm take care of the rest.
		ASSERT(locationRole);
		ASSERT(previousOrientation.GetTargetEntity());

		const Location	targetLocation = previousOrientation.GetTargetEntity()->GetConstStaticsStateAccessor()->GetLocation();
		const Location	sourceLocation = locationRole->GetState(currentTime);

		tTangentVector = (targetLocation - sourceLocation).GetVector();
	}
	else
	{
		ASSERT(interpolationType == FlaggedOrientation::kTangent);
		ASSERT(locationRole);

		
		// Find the tangent with the help of the location role
		if(locationRole->CountTimePoints() >= 2)
		{
			tTangentVector = GetTangent(*locationRole, currentTime);

			if(tTangentVector.equal(zeroVector, 0.001))
			{
				return FlaggedOrientation(tLinearInterpolatedOrientation, previousOrientation.GetInterpolationType(), previousOrientation.GetTargetEntity());
			}
		}
		else
		{
			tTangentVector.set(0, 0, 1);
		}
	}

	// Now that we have a direction for the z axis, let's compute the orientation
	tTangentVector.normalize();
	tTangentVector = -tTangentVector;

	Vector			iVector = tLinearInterpolatedOrientation.GetI();
	Vector			jVector = tLinearInterpolatedOrientation.GetJ();
	const Vector	kVector = tTangentVector;

	const Vector	jkNormalVector = cross_product(jVector, kVector);

	if(!(jkNormalVector.equal(zeroVector, 0.001)))
	{
		// The jkNormalVector is usable
		iVector = jkNormalVector;
		iVector.normalize();

		jVector = cross_product(kVector, iVector);
		jVector.normalize();
	}
	else
	{
		// jVector and kVector are parallel. Compute with iVector instead

		jVector = cross_product(kVector, iVector);
		jVector.normalize();

		iVector = cross_product(jVector, kVector);
		iVector.normalize();
	}

	return FlaggedOrientation(Orientation(iVector, jVector, kVector), previousOrientation.GetInterpolationType(), previousOrientation.GetTargetEntity());
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
