// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "DADynamicSpotLightState.h"
#include "CodeMsg.h"
#include "Char.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kForce,
	kTorque,
	kLinearVelocity,
	kAngularVelocity,
	kColor,
	kIsInfinite,
	kRange,
	kCutOff
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
DADynamicSpotLightState::DADynamicSpotLightState()
: mDABaseLight(CharGetEngine())
{
	Orientation	orient;

	orient.MakeRotationMatrix(Vector(1.0, 0.0, 0.0), 180.0);

	SetPosition(Position(Location(0.0, 10.0, 0.0), orient));

	SetColor(Color(1.0, 1.0, 1.0, 1.0));
	SetInfinite(false);
	SetRange(100);
	SetCutOff(45);

	mDABaseLight.set_On(TRUE);

	ILightManager*	lightManager = CharGetLightManager();
	ASSERT(lightManager != NULL);

	ILight*	light = &mDABaseLight;
	
	lightManager->activate_lights(&light, 1);
}
// --------------------------------------------------------------------------
DADynamicSpotLightState::~DADynamicSpotLightState()
{
	ILightManager*	lightManager = CharGetLightManager();
	ASSERT(lightManager != NULL);

	ILight*	light = &mDABaseLight;

	lightManager->deactivate_lights(&light, 1);

	mDABaseLight.set_On(FALSE);
}
// --------------------------------------------------------------------------
void DADynamicSpotLightState::SetColor(const Color& color)
{
	mDABaseLight.color.r = color.GetRed() * 255;
	mDABaseLight.color.g = color.GetGreen() * 255;
	mDABaseLight.color.b = color.GetBlue() * 255;
}
// --------------------------------------------------------------------------
Color DADynamicSpotLightState::GetColor() const
{
	return Color(mDABaseLight.color.r / 255.0, mDABaseLight.color.g / 255.0
					, mDABaseLight.color.b / 255.0, 1.0);

}
// --------------------------------------------------------------------------
void DADynamicSpotLightState::SetRange(float range)
{
	ASSERT(range >= 0);

	mDABaseLight.range = range;
}
// --------------------------------------------------------------------------
void DADynamicSpotLightState::SetInfinite(bool infinite)
{
	mDABaseLight.infinite = infinite ? TRUE : FALSE;
}
// --------------------------------------------------------------------------
void DADynamicSpotLightState::SetCutOff(float cutOff)
{
	ASSERT(0 <= cutOff && cutOff <= 180);

	mDABaseLight.cutoff = cutOff;
}
// --------------------------------------------------------------------------
float DADynamicSpotLightState::GetRange() const
{
	return mDABaseLight.GetRange();
}
// --------------------------------------------------------------------------
bool DADynamicSpotLightState::IsInfinite() const
{
	return mDABaseLight.IsInfinite();
}
// --------------------------------------------------------------------------
float DADynamicSpotLightState::GetCutOff() const
{
	return mDABaseLight.GetCutoff();
}
// --------------------------------------------------------------------------
Position DADynamicSpotLightState::GetPosition() const
{
	const Vector	pos = mDABaseLight.get_position();
	const ::Matrix	orient = mDABaseLight.get_orientation();

	return Position(Location(pos), Orientation(orient.get_i(), orient.get_j(), orient.get_k()));
}
// --------------------------------------------------------------------------
void DADynamicSpotLightState::SetPosition(const Position& kPositionR)
{
	mDABaseLight.set_position(kPositionR.GetLocation().GetVector());

	const Orientation	orient = kPositionR.GetOrientation();
	::Matrix orientMat(orient.GetI(), orient.GetJ(), orient.GetK());
		
	mDABaseLight.set_orientation(orientMat);
	mDABaseLight.direction = orientMat.get_k();

	{
		char	direction[100];
	
		sprintf(direction, "x=%f, y=%f, z=%f\n", mDABaseLight.direction.x, mDABaseLight.direction.y, mDABaseLight.direction.z);
		//OutputDebugString(direction);
	}
}
// --------------------------------------------------------------------------
void DADynamicSpotLightState::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void DADynamicSpotLightState::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void DADynamicSpotLightState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	oWiz.Put(kForce, mForce);
	oWiz.Put(kTorque, mTorque);
	oWiz.Put(kLinearVelocity, mLinearVelocity);
	oWiz.Put(kAngularVelocity, mAngularVelocity);

	// Write BaseLight properties
	const Color	color = GetColor();
	const bool	infinite = IsInfinite();
	const float	range = GetRange();
	const float	cutOff = GetCutOff();
	
	oWiz.Put(kColor, color);
	oWiz.Put(kIsInfinite, infinite);
	oWiz.Put(kRange, range);
	oWiz.Put(kCutOff, cutOff);
}
// --------------------------------------------------------------------------
void DADynamicSpotLightState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	mForce = Force();
	mTorque = Torque();
	mLinearVelocity = LinearVelocity();
	mAngularVelocity = AngularVelocity();

	if(iWiz.Has(kForce))
	{
		iWiz.Get(kForce, mForce);
	}

	if(iWiz.Has(kTorque))
	{
		iWiz.Get(kTorque, mTorque);
	}

	if(iWiz.Has(kLinearVelocity))
	{
		iWiz.Get(kLinearVelocity, mLinearVelocity);
	}

	if(iWiz.Has(kAngularVelocity))
	{
		iWiz.Get(kAngularVelocity, mAngularVelocity);
	}

	// Read BaseLight properties
	Color	color = Color(1.0, 1.0, 1.0, 1.0);
	bool	infinite = false;
	float	range = 100;
	float	cutOff = 45;
	
	if(iWiz.Has(kColor))
	{
		iWiz.Get(kColor, color);
	}

	if(iWiz.Has(kIsInfinite))
	{
		iWiz.Get(kIsInfinite, infinite);
	}

	if(iWiz.Has(kRange))
	{
		iWiz.Get(kRange, range);
	}

	if(iWiz.Has(kCutOff))
	{
		iWiz.Get(kCutOff, cutOff);
	}

	SetColor(color);
	SetInfinite(infinite);
	SetRange(range);
	SetCutOff(cutOff);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

