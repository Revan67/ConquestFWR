// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ConstDynamicsStateAccessor_h
#define ConstDynamicsStateAccessor_h
// --------------------------------------------------------------------------
#include "ROSDLL.h"
#include "Force.h"
#include "AngularVelocity.h"
#include "Torque.h"
#include "LinearVelocity.h"
// --------------------------------------------------------------------------
namespace ROS
{
class ADynamicSceneEntity;
class ADynamicsState;
// --------------------------------------------------------------------------
//  ConstDynamicsStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL ConstDynamicsStateAccessor
{
    public:
    	ConstDynamicsStateAccessor(const ADynamicSceneEntity& owner, const ADynamicsState& state);

        Force GetForce() const;
        AngularVelocity GetAngularVelocity() const;
        Torque GetTorque() const;
        LinearVelocity GetLinearVelocity() const;

    protected:
    	const ADynamicsState& GetState() const { return mState; };
    	const ADynamicSceneEntity& GetOwner() const { return mOwner; };

	private :
        const ADynamicsState& mState;
		const ADynamicSceneEntity& mOwner;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif