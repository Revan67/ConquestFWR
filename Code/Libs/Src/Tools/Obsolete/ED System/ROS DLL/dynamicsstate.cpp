// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "DynamicsState.h"
// --------------------------------------------------------------------------
/**# implementation DynamicsState:: id(C_0887147031)
*/
// --------------------------------------------------------------------------
enum FieldID
{
	kForce,
	kTorque,
	kLinearVelocity,
	kAngularVelocity
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN DynamicsState::DynamicsState()
{
}
// --------------------------------------------------------------------------
CPP_DEFN DynamicsState::DynamicsState(const ADynamicsState& aDynamicsState)
{
    SetForce(aDynamicsState.GetForce());
    SetAngularVelocity(aDynamicsState.GetAngularVelocity());
    SetTorque(aDynamicsState.GetTorque());
    SetLinearVelocity(aDynamicsState.GetLinearVelocity());
}
// --------------------------------------------------------------------------
CPP_DEFN Force DynamicsState::GetForce() const
{
    return mForce;
}
// --------------------------------------------------------------------------
CPP_DEFN AngularVelocity DynamicsState::GetAngularVelocity() const
{
    return mAngularVelocity;
}
// --------------------------------------------------------------------------
CPP_DEFN Torque DynamicsState::GetTorque() const
{
    return mTorque;
}
// --------------------------------------------------------------------------
CPP_DEFN LinearVelocity DynamicsState::GetLinearVelocity() const
{
    return mLinearVelocity;
}
// --------------------------------------------------------------------------
CPP_DEFN void DynamicsState::SetForce(const Force& kForceR)
{
    mForce = kForceR;
}
// --------------------------------------------------------------------------
CPP_DEFN void DynamicsState::SetAngularVelocity(const AngularVelocity& kAngularVelocityR)
{
    mAngularVelocity = kAngularVelocityR;
}
// --------------------------------------------------------------------------
CPP_DEFN void DynamicsState::SetTorque(const Torque& kTorqueR)
{
    mTorque = kTorqueR;
}
// --------------------------------------------------------------------------
CPP_DEFN void DynamicsState::SetLinearVelocity(const LinearVelocity& kLinearVelocityR)
{
    mLinearVelocity = kLinearVelocityR;
}
// --------------------------------------------------------------------------
CPP_DEFN void DynamicsState::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);

    WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void DynamicsState::WriteSubObject(std::ostream& oStream) const
{
 	OStreamWiz<FieldID>	oWiz(oStream);
	
	oWiz.Put(kForce, mForce);
	oWiz.Put(kTorque, mTorque);
	oWiz.Put(kLinearVelocity, mLinearVelocity);
	oWiz.Put(kAngularVelocity, mAngularVelocity);
}
// --------------------------------------------------------------------------
CPP_DEFN void DynamicsState::Read(std::istream& iStream)
{
    BaseClass::Read(iStream);

    ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void DynamicsState::ReadSubObject(std::istream& iStream)
{
 	IStreamWiz<FieldID>	iWiz(iStream);
	
	iWiz.Get(kForce, mForce);
	iWiz.Get(kTorque, mTorque);
	iWiz.Get(kLinearVelocity, mLinearVelocity);
	iWiz.Get(kAngularVelocity, mAngularVelocity);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

