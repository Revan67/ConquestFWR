// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef MotionStateAccessor_h
#define MotionStateAccessor_h
// --------------------------------------------------------------------------
#include "StringType.h"
#include "TimeType.h"
#include "TimeTag.h"
#include "ROSDLL.h"
#include "IKState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ACompoundSceneEntity;
class HardPoint;
// --------------------------------------------------------------------------
//  MotionStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL MotionStateAccessor
{
    public:
    	MotionStateAccessor(ACompoundSceneEntity& owner, AStaticsState& state);

		void            ShowSkeleton(bool show);
		void			ShowHardPoints(bool show);

        unsigned int    GetHardPointCount() const;
        ROSString       GetHardPointName(unsigned int idx) const;
        Location        GetHardPointLocation(unsigned int idx) const;
        Orientation     GetHardPointOrientation(unsigned int idx) const;

		void			AttachHardPointToParent(unsigned int hardPointIndex, const HardPoint& parent);

        void    	    Start(const ROSString& motionName, Time startTime, Time transition);
        void    	    Loop(const ROSString& motionName, Time startTime, Time transition);
        void    	    Pause(const ROSString& motionName);
        void    	    Resume(const ROSString& motionName);
        void    	    Stop(const ROSString& motionName);

		void			StartIK(const ROSString& endEffectorName, unsigned int countToRootEffector, AStaticSceneEntity& targetEntity, Time transition);
		IKState			GetIKState(Time startTime) const;
		void			SetIKState(const IKState& iKState, Time startTime);

        unsigned int    GetMotionCount() const;
        ROSString       GetMotionName(unsigned int motionIdx) const;
        Time            GetMotionLength(const ROSString& motionName) const;
        ROSString	    GetCurrentMotionName() const;
        void            SetCurrentMotionName(const ROSString& motionName);

        Time            GetCurrentMotionTime() const;
		long            GetRootEngineIndex () const;

	protected:
    	ACompoundSceneEntity& GetOwner();
    	const ACompoundSceneEntity& GetOwner() const;

    	AStaticsState& GetState();
    	const AStaticsState& GetState() const;

		void OwnerStateUpdated(Update::ID update);

	private:
		ACompoundSceneEntity&	mOwner;
		AStaticsState&			mState;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
