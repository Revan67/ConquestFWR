// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ConstDynamicsStateAccessor.h"
#include "ADynamicSceneEntity.h"
#include "ADynamicsState.h"
// --------------------------------------------------------------------------

namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN ConstDynamicsStateAccessor::ConstDynamicsStateAccessor(const ADynamicSceneEntity& owner, const ADynamicsState& state)
: mOwner(owner), mState(state)
{
}
// --------------------------------------------------------------------------
CPP_DEFN Force ConstDynamicsStateAccessor::GetForce() const
{
	return mState.GetForce();
}
// --------------------------------------------------------------------------
CPP_DEFN AngularVelocity ConstDynamicsStateAccessor::GetAngularVelocity() const
{
	return mState.GetAngularVelocity();
}
// --------------------------------------------------------------------------
CPP_DEFN Torque ConstDynamicsStateAccessor::GetTorque() const
{
	return mState.GetTorque();
}
// --------------------------------------------------------------------------
CPP_DEFN LinearVelocity ConstDynamicsStateAccessor::GetLinearVelocity() const
{
	return mState.GetLinearVelocity();
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
