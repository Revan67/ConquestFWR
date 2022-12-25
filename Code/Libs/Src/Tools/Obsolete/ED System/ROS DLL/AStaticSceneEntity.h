// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef AStaticSceneEntity_h
#define AStaticSceneEntity_h
// --------------------------------------------------------------------------
#include "ASceneEntity.h"
#include <Memory>

#include "Location.h"
#include "Orientation.h"
#include "AStaticsState.h"
#include "TimeType.h"
#include "ROSDLL.h"
#include "OrientationMemento.h"
#include "LocationMemento.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ConstStaticsStateAccessor;
class StaticsStateAccessor;
class Scene;
class ARole;
class AMarker;
// --------------------------------------------------------------------------
//  AStaticSceneEntity
// --------------------------------------------------------------------------
class CPP_DECL AStaticSceneEntity: public virtual ASceneEntity
{
    friend class ConstStaticsStateAccessor;
    friend class StaticsStateAccessor;

    public:
		enum InterpolationType
		{
			kLinearFixed,
			kSplineFixed,
			kLinearBlend,
			kSplineBlend
		};

		virtual void Delete() = 0;

        virtual const std::auto_ptr<ConstStaticsStateAccessor> GetConstStaticsStateAccessor() const;
        virtual std::auto_ptr<StaticsStateAccessor> GetStaticsStateAccessor();

    protected:
        AStaticSceneEntity();

        virtual ~AStaticSceneEntity() = 0;

		virtual void Respond(const SceneEntityEvent& event);

		virtual void Goto(Time time);

		virtual void GotoForLocationRole(Time time);
		virtual void GotoForOrientationRole(Time time);
		
		virtual void StateUpdated(Update::ID update);
		virtual void StateUpdated(Update::ID id, Time time);

		virtual void LocationStateUpdated(Time time);
		virtual void OrientationStateUpdated(Time time);

        Orientation GetOrientationInWorld() const;
        Location    GetLocationInWorld() const;

		virtual OrientationMemento GetOrientationMemento(Time time) const;
		virtual void SetOrientationMemento(const OrientationMemento& memento);

		virtual LocationMemento GetLocationMemento(Time time) const;
		virtual void SetLocationMemento(const LocationMemento& memento);

		virtual InterpolationType GetLocationInterpolationType(Time time) const;

		virtual void SetStaticsPathVisible(bool visible);
		virtual bool IsStaticsPathVisible() const;
		
		void UpdateMotionPathMarkers();

		virtual Time GetLocationTime(unsigned int keyPointIndex) const;
		virtual Time GetOrientationTime(unsigned int keyPointIndex) const;

		virtual Location GetLocation(Time time) const;
		virtual Orientation GetOrientation(Time time) const;

		virtual void SetLocation(const Location& location, Time time);
		virtual void SetOrientation(const Orientation& orientation, Time time);

		virtual void RemoveLocation(Time time);
		virtual void RemoveOrientation(Time time);

		virtual bool IsAtKeyLocation() const;
		virtual bool IsAtKeyOrientation() const;

		virtual int GetLocationRoleIndex() const = 0;
		virtual int GetOrientationRoleIndex() const = 0;

        virtual void SetupPosition() const;

		virtual void RoleUpdated();

    private:
		void UpdateForDeletedMarker(AMarker* marker);

        typedef ASceneEntity BaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif