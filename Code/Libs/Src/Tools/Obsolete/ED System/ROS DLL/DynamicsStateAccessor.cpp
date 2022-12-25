// Author: Shaival Varma
//---------------------------------------------------------------------------
#include "PCH.h"
#include "DynamicsStateAccessor.h"
#include "ADynamicSceneEntity.h"
#include "ADynamicsState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN DynamicsStateAccessor::DynamicsStateAccessor(ADynamicSceneEntity& owner, ADynamicsState& state)
: mOwner(owner), mState(state)
{
}
// --------------------------------------------------------------------------
CPP_DEFN Force DynamicsStateAccessor::GetForce() const
{
	return mState.GetForce();
}
// --------------------------------------------------------------------------
CPP_DEFN AngularVelocity DynamicsStateAccessor::GetAngularVelocity() const
{
	return mState.GetAngularVelocity();
}
// --------------------------------------------------------------------------
CPP_DEFN Torque DynamicsStateAccessor::GetTorque() const
{
	return mState.GetTorque();
}
// --------------------------------------------------------------------------
CPP_DEFN LinearVelocity DynamicsStateAccessor::GetLinearVelocity() const
{
	return mState.GetLinearVelocity();
}
// --------------------------------------------------------------------------
CPP_DEFN void DynamicsStateAccessor::SetForce(const Force& force)
{
	mState.SetForce(force);

	OwnerStateUpdated(Update::kDynamic);
}
// --------------------------------------------------------------------------
CPP_DEFN void DynamicsStateAccessor::SetAngularVelocity(const AngularVelocity& angularVelocity)
{
	mState.SetAngularVelocity(angularVelocity);

	OwnerStateUpdated(Update::kDynamic);
}
// --------------------------------------------------------------------------
CPP_DEFN void DynamicsStateAccessor::SetTorque(const Torque& torque)
{
	mState.SetTorque(torque);

	OwnerStateUpdated(Update::kDynamic);
}
// --------------------------------------------------------------------------
CPP_DEFN void DynamicsStateAccessor::SetLinearVelocity(const LinearVelocity& linearVelocity)
{
	mState.SetLinearVelocity(linearVelocity);

	OwnerStateUpdated(Update::kDynamic);
}
// --------------------------------------------------------------------------
CPP_DEFN ADynamicSceneEntity& DynamicsStateAccessor::GetOwner()
{
	return mOwner;
}
// --------------------------------------------------------------------------
CPP_DEFN const ADynamicSceneEntity& DynamicsStateAccessor::GetOwner() const
{
	return mOwner;
}
// --------------------------------------------------------------------------
CPP_DEFN ADynamicsState& DynamicsStateAccessor::GetState()
{
	return mState;
}
// --------------------------------------------------------------------------
CPP_DEFN const ADynamicsState& DynamicsStateAccessor::GetState() const
{
	return mState;
}
// --------------------------------------------------------------------------
CPP_DEFN void DynamicsStateAccessor::OwnerStateUpdated(Update::ID id)
{
    mOwner.StateUpdated(id);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
