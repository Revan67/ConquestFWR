// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "DynamicSpotLightState.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kPosition,
	kDynamicsState,
	kSpotLightState
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
DynamicSpotLightState::DynamicSpotLightState()
{
}
// --------------------------------------------------------------------------
DynamicSpotLightState::DynamicSpotLightState(const ADynamicSpotLightState& state)
{
	SetDynamicSpotLightState(state);
}
// --------------------------------------------------------------------------
DynamicSpotLightState& DynamicSpotLightState::operator=(const ADynamicSpotLightState& lightState)
{
	SetDynamicSpotLightState(lightState);

	return *this;
}
// --------------------------------------------------------------------------
void DynamicSpotLightState::Write(std::ostream& oStream) const
{
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void DynamicSpotLightState::Read(std::istream& iStream)
{
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void DynamicSpotLightState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	oWiz.Put(kPosition, mPosition);
	oWiz.Put(kDynamicsState, mDynamicsState);
	oWiz.Put(kSpotLightState, mSpotLightState);
}
// --------------------------------------------------------------------------
void DynamicSpotLightState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	iWiz.Get(kPosition, mPosition);
	iWiz.Get(kDynamicsState, mDynamicsState);
	iWiz.Get(kSpotLightState, mSpotLightState);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
