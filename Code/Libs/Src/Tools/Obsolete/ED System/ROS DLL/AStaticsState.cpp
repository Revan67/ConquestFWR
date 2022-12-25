// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "AStaticsState.h"
// --------------------------------------------------------------------------
/**# implementation AStaticsState:: id(C_0892666840) 
*/
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN AStaticsState::~AStaticsState()
{
}
//---------------------------------------------------------------------------
void AStaticsState::SetStaticsState(const AStaticsState& staticsState)
{
    SetPosition(staticsState.GetPosition());
}
// --------------------------------------------------------------------------
void AStaticsState::Interpolate(const AStaticsState& nextState, float t, AStaticsState& tState) const
{
	tState.SetPosition(GetPosition().Interpolate(nextState.GetPosition(), t));
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
