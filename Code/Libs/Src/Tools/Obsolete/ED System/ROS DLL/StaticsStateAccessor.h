// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef StaticsStateAccessor_h
#define StaticsStateAccessor_h
// --------------------------------------------------------------------------
#include "Location.h"
#include "Orientation.h"
#include "ROSDLL.h"
#include "OrientationMemento.h"
#include "LocationMemento.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class AStaticSceneEntity;
class AStaticsState;
namespace Update
{
enum ID;
}
// --------------------------------------------------------------------------
//  StaticsStateAccessor
// --------------------------------------------------------------------------
/**# :[Note = "Consider a base class AStaticsStateAccessor. Don't know if it is really needed."]
*/
class CPP_DECL StaticsStateAccessor
{
    public:
		enum InterpolationType
		{
			kLinearFixed,
			kSplineFixed,
			kLinearBlend,
			kSplineBlend
		};

    	StaticsStateAccessor(AStaticSceneEntity& owner, AStaticsState& state): mOwner(owner), mState(state){}

        Location GetLocation() const;
        Orientation GetOrientation() const;

		//NOTE: SIMPLIFY BY CREATING ACCESSOR FOR Position IN ORDER TO SET Location AND Orientation
        void SetLocation(const Location& location);
        void SetOrientation(const Orientation& orientation);

		OrientationMemento GetOrientationMemento(Time time) const;
		void SetOrientationMemento(const OrientationMemento& memento);

		LocationMemento GetLocationMemento(Time time) const;
		void SetLocationMemento(const LocationMemento& memento);

		Location GetLocation(Time time) const;
		Orientation GetOrientation(Time time) const;

		Time GetLocationTime(unsigned int keyPointIndex) const;
		Time GetOrientationTime(unsigned int keyPointIndex) const;

		void SetLocation(const Location& location, Time time);
		void SetOrientation(const Orientation& orientation, Time time);

		void RemoveLocation(Time time);
		void RemoveOrientation(Time time);

		bool IsAtKeyLocation() const;
		bool IsAtKeyOrientation() const;

		InterpolationType GetLocationInterpolationType(Time time) const;

		void SetStaticsPathVisible(bool visible);
		bool IsStaticsPathVisible() const;

        Location    GetLocationInWorld() const;
        Orientation GetOrientationInWorld() const;

    protected:
    	AStaticSceneEntity& GetOwner();
    	const AStaticSceneEntity& GetOwner() const;

    	AStaticsState& GetState();
    	const AStaticsState& GetState() const;

		void OwnerStateUpdated(Update::ID update);

    private :
        AStaticSceneEntity& mOwner;
        AStaticsState& mState;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif