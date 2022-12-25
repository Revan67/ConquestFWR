// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ACompoundSceneEntity_h
#define ACompoundSceneEntity_h
// --------------------------------------------------------------------------
#include <istream>
#include <set>
#include "AStaticSceneEntity.h"
#include "ParentRole.h"
#include "IKState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class Scene;
class HardPoint;
class HardPointHost;
// --------------------------------------------------------------------------
//  ACompoundSceneEntity
// --------------------------------------------------------------------------
class CPP_DECL ACompoundSceneEntity: public virtual AStaticSceneEntity
{
	friend class MotionStateAccessor;
	friend class ConstMotionStateAccessor;

	public:
        virtual const std::auto_ptr<ConstMotionStateAccessor> GetConstMotionStateAccessor() const;
        virtual std::auto_ptr<MotionStateAccessor> GetMotionStateAccessor();

		virtual void Read(std::istream& iStream);

		virtual void Respond(const SceneEntityEvent& event);

	protected:
		// --------------------------------------------------------------------------
		// ParentRoleCallback
		// --------------------------------------------------------------------------
		class ParentRoleCallback: public ParentRole::UpdateCallback
		{
			public:
				ParentRoleCallback(ACompoundSceneEntity& aCompoundSE);

				virtual void RemoveStarted(const ParentRole::UpdateCallback::RoleType& role, Time time);
				virtual void RemoveFinished(const ParentRole::UpdateCallback::RoleType& role, Time time);

				virtual void RemoveStarted(const ParentRole::UpdateCallback::RoleType& role, unsigned int timePointIndex);
				virtual void RemoveFinished(const ParentRole::UpdateCallback::RoleType& role, unsigned int timePointIndex);

			private:
				void RemoveParent();

				ACompoundSceneEntity&	mACompoundSE;
				ACompoundSceneEntity*	mLastParentRemoved;
		};

		friend class ParentRoleCallback;

        virtual void					ShowSkeleton(bool show) = 0;
        virtual bool					IsSkeletonShowing() const = 0;
										
        virtual void					ShowHardPoints(bool show) = 0;
        virtual bool					AreHardPointsShowing() const = 0;
										
        virtual unsigned int			GetHardPointCount() const = 0;
        virtual ROSString				GetHardPointName(unsigned int idx) const = 0;
        virtual Location				GetHardPointLocation(unsigned int idx) const = 0;
        virtual Orientation				GetHardPointOrientation(unsigned int idx) const = 0;
		virtual const HardPointHost*	GetHardPointHost(unsigned int idx) const = 0;
		virtual void					AttachHardPointToParent(unsigned int hardPointIndex, const HardPoint& parent);
		virtual void					DetachHardPointFromParent(unsigned int hardPointIndex, const HardPoint& parent);

        virtual unsigned int			GetMotionCount() const = 0;
		virtual ROSString				GetMotionName(int motionIdx) const = 0;
        virtual Time					GetMotionLength(const ROSString& motionName) const = 0;
										
		virtual ROSString				GetCurrentMotionName() const = 0;
        virtual void					SetCurrentMotionName(const ROSString& motionName) = 0;
        virtual Time					GetCurrentMotionTime() const = 0;
										
        virtual void    				Start(const ROSString& motionName, Time startTime, Time transition) = 0;
        virtual void    				Loop(const ROSString& motionName, Time startTime, Time transition) = 0;
        virtual void    				Pause(const ROSString& motionName) = 0;
        virtual void    				Resume(const ROSString& motionName) = 0;
        virtual void    				Stop(const ROSString& motionName) = 0;

		virtual void					StartIK(const ROSString& endEffectorName, unsigned int countToRootEffector, AStaticSceneEntity& targetEntity, Time transition) = 0;
		virtual IKState					GetIKState(Time startTime) const = 0;
		virtual void					SetIKState(const IKState& iKState, Time startTime) = 0;

        virtual void					Goto(Time time);
										
		// *** NOTE: This should probably be returning an object that can be used to access the heirarchy
		// *** of the compound object. For now, this is the associated engine index that can be used with the
		// *** DA Library code to retrieve the needed information.
		// *** TODO: Make a skeleton interface and return an object here that implements that interface.
		virtual long					GetRootEngineIndex () const = 0;

    protected:
		typedef std::set<Time>			ParentEventFlags;

										ACompoundSceneEntity();

		virtual int						GetParentRoleIndex() const = 0;

		virtual void					AttachHardPointToParent(unsigned int childHardPointIndex, const HardPointHost* parentHardPointHost, const ROSString& parentHardPointName) = 0;
		virtual void					DetachHardPointFromParent(unsigned int childHardPointIndex, const HardPointHost* parentHardPointHost, const ROSString& parentHardPointName) = 0;

		virtual ParentEventFlags&		GetParentEventFlags() = 0;
		virtual const ParentEventFlags&	GetParentEventFlags() const = 0;

        virtual void					GotoForMotionRole(Time time) = 0;
		virtual void					GotoForParentRole(Time time);

    private:
		void 							InitParentEventFlags();
        void							SetParentEventFlag(Time startTime, bool isHandled);
        bool							IsParentEventFlagSet(Time startTime) const;

		void							RemoveParent(ACompoundSceneEntity& aCompoundSE);

        typedef AStaticSceneEntity BaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
