// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "CameraState.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kHorizontalFOV,
	kVerticalFOV
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CameraState::CameraState(const ACameraState& cameraState)
:mHorFOV(cameraState.GetHorizontalFOV()), mVerFOV(cameraState.GetVerticalFOV())
{
}
// --------------------------------------------------------------------------
CameraState::CameraState(float hFOV, float vFOV)
:mHorFOV(hFOV), mVerFOV(vFOV)
{
}
// --------------------------------------------------------------------------
float CameraState::GetHorizontalFOV() const
{
	return mHorFOV;
}
// --------------------------------------------------------------------------
float CameraState::GetVerticalFOV() const
{
	return mVerFOV;
}
// --------------------------------------------------------------------------
void CameraState::SetHorizontalFOV(float hFOV)
{
	mHorFOV = hFOV;
}
// --------------------------------------------------------------------------
void CameraState::SetVerticalFOV(float vFOV)
{
	mVerFOV = vFOV;
}
// --------------------------------------------------------------------------
void CameraState::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void CameraState::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void CameraState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	oWiz.Put(kHorizontalFOV, mHorFOV);
	oWiz.Put(kVerticalFOV, mVerFOV);
}
// --------------------------------------------------------------------------
void CameraState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	iWiz.Get(kHorizontalFOV, mHorFOV);
	iWiz.Get(kVerticalFOV, mVerFOV);
}
// --------------------------------------------------------------------------
}
