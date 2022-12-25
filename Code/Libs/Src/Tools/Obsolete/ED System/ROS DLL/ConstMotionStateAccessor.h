// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ConstMotionStateAccessor_h
#define ConstMotionStateAccessor_h
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
class ACompoundSceneEntityState;
class HardPointHost;
// --------------------------------------------------------------------------
//  ConstMotionStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL ConstMotionStateAccessor
{
    public:
    	ConstMotionStateAccessor(const ACompoundSceneEntity& owner, const ACompoundSceneEntityState& state);

		bool                    IsSkeletonShowing() const;
		bool					AreHardPointsShowing() const;
								
        unsigned int			GetHardPointCount() const;
        ROSString				GetHardPointName(unsigned int idx) const;
        Location				GetHardPointLocation(unsigned int idx) const;
        Orientation				GetHardPointOrientation(unsigned int idx) const;
		const HardPointHost*	GetHardPointHost(unsigned int idx) const;

        unsigned int			GetMotionCount() const;
        ROSString				GetMotionName(int motionIdx) const;
        Time					GetMotionLength(const ROSString& motionName) const;
        ROSString				GetCurrentMotionName() const;
								
		IKState					GetIKState(Time startTime) const;

        Time        			GetCurrentMotionTime() const;
		long					GetRootEngineIndex () const;

	protected:
    	const ACompoundSceneEntity&			GetOwner() const;

    	const ACompoundSceneEntityState&	GetState() const;

	private:
		const ACompoundSceneEntity&			mOwner;
		const ACompoundSceneEntityState&	mState;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
