// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "LocationRole.h"
#include "Spline.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
float CalculateSegmentLength(LocationRole::TimeStateIterator& iterator, const LocationRole::TimeStateIterator& begin, const LocationRole::TimeStateIterator& end)
{
	ASSERT(iterator != end);

	// Get location of spline points
	Vector	p0, p1, p2, p3;
	
	const LocationRole::TimeStateIterator p2Iterator = iterator + 1;

	if(p2Iterator != end)
	{
		p2 = (*p2Iterator)->GetState().GetVector();
	}
	else
	{
		// The iterator is at the end
		return 0;
	}

	const LocationRole::TimeStateIterator p3Iterator = p2Iterator + 1;

	if(p3Iterator != end)
	{
		p3 = (*p3Iterator)->GetState().GetVector();
	}
	else
	{
		p3 = p2;
	}

	p1 = (*iterator)->GetState().GetVector();
	
	if(iterator != begin)
	{
		p0 = (*(iterator - 1))->GetState().GetVector();
	}
	else
	{
		p0 = p1;
	}

	Spline	spline(p0, p1, p2, p3);

	return spline.approx_length();
}
// --------------------------------------------------------------------------
FlaggedLocation Interpolate(LocationRole::TimeStateIterator& previousIterator, LocationRole::TimeStateIterator& nextIterator, Time currentTime, const LocationRole::TimeStateIterator& begin, const LocationRole::TimeStateIterator& end)
{
	const Time				previousTime = (*previousIterator)->GetTime();
	const FlaggedLocation	previousLocation = (*previousIterator)->GetState();

	const Time				nextTime = (*nextIterator)->GetTime();
	const FlaggedLocation	nextLocation = (*nextIterator)->GetState();

    if(nextTime == previousTime)
    {
		return previousLocation;
    }
    else
    {
		const FlaggedLocation::InterpolationType	interpolationType = previousLocation.GetInterpolationType();
		float										t = (currentTime - previousTime).GetTime() / (nextTime - previousTime).GetTime();

		if(interpolationType == FlaggedLocation::kLinearBlend || interpolationType == FlaggedLocation::kSplineBlend)
		{
			// We have to reinterpret t

			// Compute the speed over the previous and next sections
			float previousSpeed, nextSpeed;

			if(previousIterator != begin)
			{
				// There is a previous section
				LocationRole::TimeStateIterator priorIterator = previousIterator - 1;

				const float	length = CalculateSegmentLength(priorIterator, begin, end);

				previousSpeed = length / (previousTime - (*priorIterator)->GetTime()).GetTime();
			}
			else
			{
				// No previous section
				previousSpeed = 0;
			}

			if(nextIterator != end)
			{
				// There is a next section
				LocationRole::TimeStateIterator followingIterator = nextIterator + 1;

				if(followingIterator != end)
				{
					const float	length = CalculateSegmentLength(nextIterator, begin, end);

					nextSpeed = length / ((*followingIterator)->GetTime() - nextTime).GetTime();
				}
				else
				{
					nextSpeed = 0;
				}
			}
			else
			{
				nextSpeed = 0;
			}

			// Compute the mid-point speed
			const float	totalLength = CalculateSegmentLength(previousIterator, begin, end);
			
#if 0
			if(totalLength != 0)
			{
				const float totalTime = (nextTime - previousTime).GetTime();
				const float midSpeed = (((4 * totalLength) / totalTime) - previousSpeed - nextSpeed) / 2;
				const float midTime = totalTime / 2;

				// Now compute the distance travelled
				const float	tTime = (currentTime - previousTime).GetTime();

				float tLength;

				if(tTime < midTime)
				{
					// Haven't hit the mid-point yet
					const float acceleration = (midSpeed - previousSpeed) / midTime;
					
					tLength = (previousSpeed * tTime) + (0.5 * acceleration * tTime * tTime);	// d = u * t + 0.5 * a * t^2
				}
				else
				{
					// We are at the mid-point or beyond
					const float acceleration = (nextSpeed - midSpeed) / midTime;
					const float tTimeAdjusted = tTime - midTime;
					
					tLength = (previousSpeed * midTime) + (0.5 * acceleration * midTime * midTime)	// d = u * t + 0.5 * a * t^2 (first half of time)
								+ (midSpeed * tTimeAdjusted) + (0.5 * acceleration * tTimeAdjusted * tTimeAdjusted);	// d = u * t + 0.5 * a * t^2 (second half of time)
				}

				// This gives us the new t
				t = tLength / totalLength;
			}
#else
#if 1
			if(totalLength != 0)
			{
				const float totalTime = (nextTime - previousTime).GetTime();
				const float totalTimeSquared = totalTime * totalTime;
				const float totalTimeCubed = totalTimeSquared * totalTime;

				const float m = (nextSpeed - previousSpeed) * 2 / totalTimeSquared;
				const float cA = (totalLength - 0.25 * m * totalTimeCubed) / totalTime;

				const float	tTime = (currentTime - previousTime).GetTime();
				const float tTimeSquared = tTime * tTime;
				const float tTimeCubed = tTimeSquared * tTime;

				const float tLength = 0.25 * m * tTimeCubed + cA * tTime;

				// This gives us the new t
				t = tLength / totalLength;
			}
#else
			if(totalLength != 0)
			{
				const float dV = nextSpeed - previousSpeed;
				const float dS = totalLength;
				const float dT = (nextTime - previousTime).GetTime();
				const float v0 = previousSpeed;

				const float B = (dV / ((dS - v0 * dT) * dT)) + 1;
				const float A = (dV * B) / pow(dT, B - 1);

				const float currentDT = (currentTime - previousTime).GetTime();

				const float tLength = (v0 * currentDT) + ((A * pow(currentDT, B - 2)) / (B * (B - 1)));

				// This gives us the new t
				t = tLength / totalLength;
			}
#endif
#endif
		}

		if(interpolationType == FlaggedLocation::kLinearFixed || interpolationType == FlaggedLocation::kLinearBlend)
		{
			Location	location = previousLocation.Interpolate(nextLocation, t);
			
			return FlaggedLocation(location, interpolationType);
		}
		else
		{
			ASSERT(interpolationType == FlaggedLocation::kSplineFixed || interpolationType == FlaggedLocation::kSplineBlend)

			// Get location of spline points
			Vector	p0, p1, p2, p3, tVector;
			
			p1 = previousLocation.GetVector();

			p2 = nextLocation.GetVector();

			if(previousIterator != begin)
			{
				p0 = (*(previousIterator - 1))->GetState().GetVector();
			}
			else
			{
				p0 = p1;
			}

			
			if((nextIterator + 1) != end)
			{
				p3 = (*(nextIterator + 1))->GetState().GetVector();
			}
			else
			{
				p3 = p2;
			}

			// Calculate t
			calculateCRSpline(&p0, &p1, &p2, &p3, t, &tVector);

			return FlaggedLocation(Location(tVector.x, tVector.y, tVector.z), interpolationType);
		}
    }
}
// --------------------------------------------------------------------------
Vector GetTangent(const LocationRole& locationRole, Time time)
{
	ASSERT(locationRole.CountTimePoints() >= 2);

	const FlaggedLocation						tLocation = locationRole.GetState(time);
	const FlaggedLocation::InterpolationType	tType = tLocation.GetInterpolationType();
	const unsigned int							keyPointCount = locationRole.CountTimePoints();

	// Compute a direction
	// First find a time point right before or at the current time
	int	previousTimePointIndex = keyPointCount - 1;

	while((previousTimePointIndex >= 0) && (locationRole.GetTime(previousTimePointIndex) > time))
	{
		--previousTimePointIndex;
	}

	// Is it linear or spline?
	if(tType == FlaggedLocation::kLinearFixed || tType == FlaggedLocation::kLinearBlend)
	{
		if(previousTimePointIndex < 0)
		{
			// All key points are at greater times. Use first two key points to establish direction
			previousTimePointIndex = 0;
		}
		else if(previousTimePointIndex == (keyPointCount - 1))
		{
			// All key points are at lesser time. Use last two key points to establish direction
			--previousTimePointIndex;
		}

		ASSERT(previousTimePointIndex >= 0);

		const unsigned int	prevTimePointIdx = previousTimePointIndex;

		const Location	previousLocation = locationRole.GetState(prevTimePointIdx);
		const Location	nextLocation = locationRole.GetState(prevTimePointIdx + 1);

		return (nextLocation - previousLocation).GetVector();
	}
	else
	{
		ASSERT(tType == FlaggedLocation::kSplineFixed || tType == FlaggedLocation::kSplineBlend);

		unsigned int	i0, i1, i2, i3;
		float			t;
		// Lets check if the time point is before or after all the key points
		
		if(previousTimePointIndex < 0)
		{
			if(keyPointCount == 2)
			{
				const Location	previousLocation = locationRole.GetState(0);
				const Location	nextLocation = locationRole.GetState(1);

				return (nextLocation - previousLocation).GetVector();
			}
			else
			{
				ASSERT(keyPointCount >= 3);

				i0 = 0;
				i1 = 0;
				i2 = 1;
				i3 = 2;

				t = 0;
			}
		}
		else if(previousTimePointIndex == (keyPointCount - 1))
		{
			if(keyPointCount == 2)
			{
				const Location	previousLocation = locationRole.GetState(0);
				const Location	nextLocation = locationRole.GetState(1);

				return (nextLocation - previousLocation).GetVector();
			}
			else
			{
				ASSERT(keyPointCount >= 3);

				i0 = keyPointCount - 3;
				i1 = keyPointCount - 2;
				i2 = keyPointCount - 1;
				i3 = keyPointCount - 1;

				t = 1;
			}
		}
		else
		{
			i0 = (previousTimePointIndex > 0) ? (previousTimePointIndex - 1) : 0;
			i1 = previousTimePointIndex;
			i2 = previousTimePointIndex + 1;
			i3 = (previousTimePointIndex < (keyPointCount - 2)) ? previousTimePointIndex + 2 : keyPointCount - 1;

			const Time	previousTime = locationRole.GetTime(i1);
			const Time	nextTime = locationRole.GetTime(i2);

			ASSERT(previousTime <= time && time >= time);

			t = (time - previousTime).GetTime() / (nextTime - previousTime).GetTime();
		}

		Vector	v0 = locationRole.GetState(i0).GetVector();
		Vector	v1 = locationRole.GetState(i1).GetVector();
		Vector	v2 = locationRole.GetState(i2).GetVector();
		Vector	v3 = locationRole.GetState(i3).GetVector();
		Vector	tTangentVector;

		calculateCRSplineTangent(&v0, &v1, &v2, &v3, t, &tTangentVector);

		return tTangentVector;
	}
}
// --------------------------------------------------------------------------
}
