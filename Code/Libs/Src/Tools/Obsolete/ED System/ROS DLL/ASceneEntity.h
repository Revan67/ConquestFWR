// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ASceneEntity_h
#define ASceneEntity_h
// --------------------------------------------------------------------------
#include <list>
#include "Links.h"
#include "StringType.h"
#include "TimeType.h"
#include "ROSDLL.h"
#include "SceneEntityEvent.h"
#include "ASceneEntityEventListener.h"
#include "ASceneEntityEventSource.h"
// --------------------------------------------------------------------------

namespace ROS
{
// --------------------------------------------------------------------------
class DABaseCamera;
class Scene;
class IntersectInfo;
class ConstSceneEntityStateAccessor;
class SceneEntityState;
class APhysicalState;

namespace Update
{
enum ID;
}
// --------------------------------------------------------------------------
//  ASceneEntity
// --------------------------------------------------------------------------
class CPP_DECL ASceneEntity: public ASceneEntityEventListener, public ASceneEntityEventSource
{
	friend class ConstSceneEntityStateAccessor;
	friend class SceneEntityStateAccessor;

    public:
		virtual void					Delete() = 0;

        virtual ROSString				GetArchetypeName()  const  = 0;

		virtual void					Respond(const SceneEntityEvent& event);

		virtual bool					IsPersistent() const;
        virtual void					Write(std::ostream& oStream) const = 0;
        virtual void					Read(std::istream& iStream) = 0;
										
        virtual const std::auto_ptr<ConstSceneEntityStateAccessor> GetConstSceneEntityStateAccessor() const;
        virtual std::auto_ptr<SceneEntityStateAccessor> GetSceneEntityStateAccessor();

    protected:							
										ASceneEntity();

		virtual							~ASceneEntity();
										
        virtual void					Draw(const ROS::DABaseCamera* camera) const;
		virtual bool					Intersect(const IntersectInfo& intersectInfo, float* distance) const;
										
        virtual void					Goto(Time time) = 0;
										
        virtual void					SetupPosition() const;
        virtual void					Render(const ROS::DABaseCamera* camera) const;
		virtual bool					FindIntersect(const IntersectInfo& intersectInfo, float* distance) const;
										
		bool							Performing() const;
        Time							GetCurrentTimePoint() const;

		virtual SceneEntityState&		GetSceneEntityState() = 0;
		virtual const SceneEntityState&	GetSceneEntityState() const = 0;
		virtual APhysicalState&			GetPhysicalState() = 0;
		virtual const APhysicalState&	GetPhysicalState() const = 0;
		
		virtual void					StateUpdated(Update::ID update);
		virtual void					StateUpdated(Update::ID update, Time time);

		virtual void					RoleUpdated();

		virtual ASceneEntity*			GetDependentEntity();

		virtual void					AddListener(ASceneEntityEventListener& listener);
		virtual void					RemoveListener(ASceneEntityEventListener& listener);
};
// --------------------------------------------------------------------------
typedef std::list<ASceneEntity*> SceneEntityCollection;
// --------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ASceneEntity& sceneEntity)
{
	sceneEntity.Write(oStream);

	return oStream;
}
// --------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ASceneEntity& sceneEntity)
{
	sceneEntity.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
