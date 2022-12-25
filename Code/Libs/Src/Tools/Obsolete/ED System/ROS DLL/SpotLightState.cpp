// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <float.h>
#include "SpotLightState.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kLightState,
	kIsInfinite,
	kRange,
	kCutOff
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
SpotLightState::SpotLightState()
{
	SetInfinite(false);
	SetRange(100);
	SetCutOff(45);
}
// --------------------------------------------------------------------------
SpotLightState::SpotLightState(const ASpotLightState& spotLightState)
{
	SetSpotLightState(spotLightState);
}
// --------------------------------------------------------------------------
SpotLightState& SpotLightState::operator=(const ASpotLightState& spotLightState)
{
	SetSpotLightState(spotLightState);

	return *this;
}
// --------------------------------------------------------------------------
void SpotLightState::SetRange(float range)
{
	ASSERT(range >= 0);

	mRange = range;
}
// --------------------------------------------------------------------------
void SpotLightState::SetInfinite(bool infinite)
{
	mIsInfinite = infinite;
}
// --------------------------------------------------------------------------
void SpotLightState::SetCutOff(float cutOff)
{
	ASSERT(0 <= cutOff && cutOff <= 180);

	mCutOff = cutOff;
}
// --------------------------------------------------------------------------
float SpotLightState::GetRange() const
{
	return mRange;
}
// --------------------------------------------------------------------------
bool SpotLightState::IsInfinite() const
{
	return mIsInfinite;
}
// --------------------------------------------------------------------------
float SpotLightState::GetCutOff() const
{
	return mCutOff;
}
// --------------------------------------------------------------------------
void SpotLightState::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void SpotLightState::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void SpotLightState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	oWiz.Put(kLightState, mLightState);
	oWiz.Put(kIsInfinite, mIsInfinite);
	oWiz.Put(kRange, mRange);
	oWiz.Put(kCutOff, mCutOff);
}
// --------------------------------------------------------------------------
void SpotLightState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	mLightState = LightState();
	mIsInfinite = false;
	mRange = 100;
	mCutOff = 45;

	if(iWiz.Has(kLightState))
	{
		iWiz.Get(kLightState, mLightState);
	}

	if(iWiz.Has(kIsInfinite))
	{
		iWiz.Get(kIsInfinite, mIsInfinite);
	}

	if(iWiz.Has(kRange))
	{
		iWiz.Get(kRange, mRange);
	}

	if(iWiz.Has(kCutOff))
	{
		iWiz.Get(kCutOff, mCutOff);
	}
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
