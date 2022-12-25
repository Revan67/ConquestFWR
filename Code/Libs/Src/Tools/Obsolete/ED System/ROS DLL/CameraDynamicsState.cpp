// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "CameraDynamicsState.h"
//---------------------------------------------------------------------------
enum FieldID
{
	kDynamicsState,
	kHorizontalFOV,
	kVerticalFOV
};
//---------------------------------------------------------------------------
namespace ROS
{
//---------------------------------------------------------------------------
CameraDynamicsState::CameraDynamicsState()
:mHorizontalFOV(45.0000), mVerticalFOV(36.8699)
{
}
//---------------------------------------------------------------------------
CameraDynamicsState::CameraDynamicsState(const ACameraDynamicsState& cameraDynamicsState)
{
	SetCameraDynamicsState(cameraDynamicsState);
}
//---------------------------------------------------------------------------
CameraDynamicsState::CameraDynamicsState(const Position& position, float horizontalFOV, float verticalFOV)
:mPosition(position), mHorizontalFOV(horizontalFOV), mVerticalFOV(verticalFOV)
{
}
//---------------------------------------------------------------------------
Position CameraDynamicsState::GetPosition() const
{
	return mPosition;
}
//---------------------------------------------------------------------------
Force CameraDynamicsState::GetForce() const
{
	return mDynamicsState.GetForce();
}
//---------------------------------------------------------------------------
AngularVelocity CameraDynamicsState::GetAngularVelocity() const
{
	return mDynamicsState.GetAngularVelocity();
}
//---------------------------------------------------------------------------
Torque CameraDynamicsState::GetTorque() const
{
	return mDynamicsState.GetTorque();
}
//---------------------------------------------------------------------------
LinearVelocity CameraDynamicsState::GetLinearVelocity() const
{
	return mDynamicsState.GetLinearVelocity();
}
//---------------------------------------------------------------------------
float CameraDynamicsState::GetHorizontalFOV() const
{
	return mHorizontalFOV;
}
//---------------------------------------------------------------------------
float CameraDynamicsState::GetVerticalFOV() const
{
	return mVerticalFOV;
}
//---------------------------------------------------------------------------
StaticsState CameraDynamicsState::GetStaticsState() const
{
	return StaticsState(mPosition);
}
//---------------------------------------------------------------------------
DynamicsState CameraDynamicsState::GetDynamicsState()const
{
	return mDynamicsState;
}
//---------------------------------------------------------------------------
void CameraDynamicsState::SetPosition(const Position& position)
{
	mPosition = position;
}
//---------------------------------------------------------------------------
void CameraDynamicsState::SetForce(const Force& force)
{
	mDynamicsState.SetForce(force);
}
//---------------------------------------------------------------------------
void CameraDynamicsState::SetAngularVelocity(const AngularVelocity& angularVelocity)
{
	mDynamicsState.SetAngularVelocity(angularVelocity);
}
//---------------------------------------------------------------------------
void CameraDynamicsState::SetTorque(const Torque& torque)
{
	mDynamicsState.SetTorque(torque);
}
//---------------------------------------------------------------------------
void CameraDynamicsState::SetLinearVelocity(const LinearVelocity& linearVelocity)
{
	mDynamicsState.SetLinearVelocity(linearVelocity);
}
//---------------------------------------------------------------------------
void CameraDynamicsState::SetHorizontalFOV(float hFOV)
{
	mHorizontalFOV = hFOV;
}
//---------------------------------------------------------------------------
void CameraDynamicsState::SetVerticalFOV(float vFOV)
{
	mVerticalFOV = vFOV;
}
//---------------------------------------------------------------------------
void CameraDynamicsState::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);

    WriteSubObject(oStream);
}
//---------------------------------------------------------------------------
void CameraDynamicsState::Read(std::istream& iStream)
{
    BaseClass::Read(iStream);

    ReadSubObject(iStream);
}
//---------------------------------------------------------------------------
void CameraDynamicsState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

    oWiz.Put(kDynamicsState, mDynamicsState);

	const float	horFOV = GetHorizontalFOV();
	const float	verFOV = GetVerticalFOV();

    oWiz.Put(kHorizontalFOV, horFOV);
    oWiz.Put(kVerticalFOV, verFOV);
}
//---------------------------------------------------------------------------
void CameraDynamicsState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
	
	iWiz.Get(kDynamicsState, mDynamicsState);

	float	horFOV;
	float	verFOV;

	iWiz.Get(kHorizontalFOV, horFOV);
	iWiz.Get(kVerticalFOV, verFOV);

	SetHorizontalFOV(horFOV);
	SetVerticalFOV(verFOV);
}
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
