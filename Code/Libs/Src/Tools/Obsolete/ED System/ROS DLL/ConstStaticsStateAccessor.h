// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ConstStaticsStateAccessor_h
#define ConstStaticsStateAccessor_h
// --------------------------------------------------------------------------
#include "Location.h"
#include "Orientation.h"
#include "LocationMemento.h"
#include "OrientationMemento.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class AStaticsState;
class AStaticSceneEntity;
// --------------------------------------------------------------------------
//  ConstStaticsStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL ConstStaticsStateAccessor
{
    public:
		enum InterpolationType
		{
			kLinearFixed,
			kSplineFixed,
			kLinearBlend,
			kSplineBlend
		};

    	ConstStaticsStateAccessor(const AStaticSceneEntity& owner, const AStaticsState& state);

        Location GetLocation() const;
        Orientation GetOrientation() const;

		OrientationMemento GetOrientationMemento(Time time) const;
		LocationMemento GetLocationMemento(Time time) const;

		Time GetLocationTime(unsigned int keyPointIndex) const;
		Time GetOrientationTime(unsigned int keyPointIndex) const;

		Location GetLocation(Time time) const;
		Orientation GetOrientation(Time time) const;

		InterpolationType GetLocationInterpolationType(Time time) const;

		bool IsStaticsPathVisible() const;

        Location    GetLocationInWorld() const;
        Orientation GetOrientationInWorld() const;

    protected:
    	const AStaticsState& GetState() const;
    	const AStaticSceneEntity& GetOwner() const;

    private :
        const AStaticsState& mState;
		const AStaticSceneEntity& mOwner;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif