// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "LightState.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kColor
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
LightState::LightState()
: mColor(1.0, 1.0, 1.0, 1.0)
{
}
// --------------------------------------------------------------------------
LightState::LightState(const ALightState& lightState)
: mColor(lightState.GetColor())
{
}
// --------------------------------------------------------------------------
LightState::LightState( const Color& color)
: mColor(color)
{
}
// --------------------------------------------------------------------------
LightState& LightState::operator=(const LightState& lightState)
{
	SetLightState(lightState);

	return *this;
}
// --------------------------------------------------------------------------
void LightState::SetColor(const Color& color)
{
	mColor = color;
}
// --------------------------------------------------------------------------
Color LightState::GetColor() const
{
	return mColor;
}
// --------------------------------------------------------------------------
void LightState::Write(std::ostream& oStream) const
{
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void LightState::Read(std::istream& iStream)
{
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void LightState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	oWiz.Put(kColor, mColor);
}
// --------------------------------------------------------------------------
void LightState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
	
	iWiz.Get(kColor, mColor);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
