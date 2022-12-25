// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Marker_h
#define Marker_h
// --------------------------------------------------------------------------
#include <Memory>

#include "AMarker.h"
#include "Location.h"
#include "Orientation.h"
#include "TimeType.h"
#include "ROSDLL.h"
#include "OrientationMemento.h"
#include "LocationMemento.h"
#include "SceneEntityState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class AStaticsState;
// --------------------------------------------------------------------------
//  Marker
// --------------------------------------------------------------------------
class CPP_DECL Marker: public AMarker
{
    public:
        Marker(const ROSString& name, bool makeNameUnique, Scene& scene, AStaticsState& staticsState);
        Marker(std::istream& iStream, Scene& scene, AStaticsState& staticsState);

		virtual void Delete();

		void SetNextMarker(Marker* nextMarker);

		const Marker* GetPreviousMarker() const;
		Marker* GetPreviousMarker();

		const Marker* GetNextMarker() const;
		Marker* GetNextMarker();

        virtual ROSString GetArchetypeName() const;
        static ROSString GetMarkerArchetypeName();

		virtual OrientationMemento GetOrientationMemento(Time time) const;
		virtual void SetOrientationMemento(const OrientationMemento& memento);

		virtual LocationMemento GetLocationMemento(Time time) const;
		virtual void SetLocationMemento(const LocationMemento& memento);

		virtual InterpolationType GetLocationInterpolationType(Time time) const;

		virtual bool IsPersistent() const;
        virtual void Write(std::ostream& oStream) const;
        virtual void Read(std::istream& iStream);

    protected:
		virtual ~Marker();

		virtual SceneEntityState& GetSceneEntityState();
		virtual const SceneEntityState& GetSceneEntityState() const;
		virtual APhysicalState& GetPhysicalState();
		virtual const APhysicalState& GetPhysicalState() const;

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

		virtual void GotoForLocationRole(Time time);
		virtual void GotoForOrientationRole(Time time);

		virtual void LocationStateUpdated(Time time);
		virtual void OrientationStateUpdated(Time time);

		virtual int GetLocationRoleIndex() const;
		virtual int GetOrientationRoleIndex() const;

		virtual void Render(const ROS::DABaseCamera* camera) const;

		virtual void RenderMarker(const ROS::DABaseCamera* camera) const;
		virtual void RenderPathToNextMarker(const ROS::DABaseCamera* camera) const;

    private:
        typedef AMarker BaseClass;

		void ReadSubObject(std::istream& iStream);
		void WriteSubObject(std::ostream& oStream) const;

		SceneEntityState	mSceneEntityState;
		AStaticsState*		mStaticsState;
		Marker*				mPreviousMarker;
		Marker*				mNextMarker;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif