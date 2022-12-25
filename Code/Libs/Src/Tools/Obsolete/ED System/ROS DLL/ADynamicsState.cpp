// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ADynamicsState.h"
// --------------------------------------------------------------------------
/**# implementation ADynamicsState:: id(C_0892672952)
*/
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN ADynamicsState::ADynamicsState()
{
}
// --------------------------------------------------------------------------
CPP_DEFN void ADynamicsState::SetDynamicsState(const ADynamicsState& kDynamicsStateR)
{
    SetForce(kDynamicsStateR.GetForce());
    SetAngularVelocity(kDynamicsStateR.GetAngularVelocity());
    SetTorque(kDynamicsStateR.GetTorque());
    SetLinearVelocity(kDynamicsStateR.GetLinearVelocity());
}
// --------------------------------------------------------------------------
CPP_DEFN void ADynamicsState::Interpolate(const ADynamicsState& nextState, float t, ADynamicsState& tState) const
{
	if(t == 0)
    {
		tState.SetDynamicsState(*this);
    }
	else if(t == 1)
    {	
		tState.SetDynamicsState(nextState);
    }
    else
    {  	
        tState.SetForce(GetForce().Interpolate(nextState.GetForce(), t));
        tState.SetAngularVelocity(GetAngularVelocity().Interpolate(nextState.GetAngularVelocity(), t));
        tState.SetTorque(GetTorque().Interpolate(nextState.GetTorque(), t));
        tState.SetLinearVelocity(GetLinearVelocity().Interpolate(nextState.GetLinearVelocity(), t));
    }
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

