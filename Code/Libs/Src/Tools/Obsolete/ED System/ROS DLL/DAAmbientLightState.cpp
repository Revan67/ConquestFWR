// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "DAAmbientLightState.h"
#include "Char.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kColor
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
DAAmbientLightState::DAAmbientLightState()
{
//    SetColor(Color(1, 1, 1, 1));	Don't want to set the color by default
}
// --------------------------------------------------------------------------
DAAmbientLightState::DAAmbientLightState(const Color& color)
{
	SetColor(color);
}
// --------------------------------------------------------------------------
DAAmbientLightState::~DAAmbientLightState()
{
    SetColor(Color(0, 0, 0, 1));
}
// --------------------------------------------------------------------------
void DAAmbientLightState::SetColor(const Color& color)
{
    WorldSetAmbientLight(color.GetRed() * 255, color.GetGreen() * 255, color.GetBlue() * 255);
}
// --------------------------------------------------------------------------
Color DAAmbientLightState::GetColor() const
{
	unsigned int	red, green, blue;
	
	WorldGetAmbientLight(red, green, blue);
	
	return Color(red / 255.0, green / 255.0, blue / 255.0, 1.0);
}
// --------------------------------------------------------------------------
void DAAmbientLightState::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void DAAmbientLightState::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void DAAmbientLightState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	const Color	color = GetColor();

	oWiz.Put(kColor, color);
}
// --------------------------------------------------------------------------
void DAAmbientLightState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	Color	color;

	iWiz.Get(kColor, color);

	SetColor(color);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
