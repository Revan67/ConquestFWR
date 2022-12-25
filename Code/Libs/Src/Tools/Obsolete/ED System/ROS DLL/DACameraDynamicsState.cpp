// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "DACameraDynamicsState.h"
#include "DABaseCamera.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kPosition,
	kForce,
	kTorque,
	kLinearVelocity,
	kAngularVelocity
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
DACameraDynamicsState::DACameraDynamicsState(const DABaseCamera* camera)
: mDABaseCamera(camera)
{
}
// --------------------------------------------------------------------------
void DACameraDynamicsState::SetDABaseCamera(const DABaseCamera* camera)
{
	mDABaseCamera = camera;
}
// --------------------------------------------------------------------------
const DABaseCamera* DACameraDynamicsState::GetDABaseCamera() const
{
	return mDABaseCamera;
}
/*
// --------------------------------------------------------------------------
DACameraDynamicsState::DACameraDynamicsState(DACamera& owner, const Position& position)
: mDABaseCamera(NULL)
{
    mOwner.SetDACameraLocation(position.GetLocation());
    mOwner.SetDACameraOrientation(position.GetOrientation());
}
*/
// --------------------------------------------------------------------------
Position DACameraDynamicsState::GetPosition() const
{
	ASSERT(mDABaseCamera);

	Vector		location;
	Orientation	orient;

	CameraGetPosition(mDABaseCamera, location);
	CameraGetOrientation(mDABaseCamera, orient);

	return Position(Location(location), orient);
}
// --------------------------------------------------------------------------
Force DACameraDynamicsState::GetForce() const
{
    return mForce;
}
// --------------------------------------------------------------------------
AngularVelocity DACameraDynamicsState::GetAngularVelocity() const
{
    return mAngularVelocity;
}
// --------------------------------------------------------------------------
Torque DACameraDynamicsState::GetTorque() const
{
    return mTorque;
}
// --------------------------------------------------------------------------
LinearVelocity DACameraDynamicsState::GetLinearVelocity() const
{
    return mLinearVelocity;
}
// --------------------------------------------------------------------------
float DACameraDynamicsState::GetHorizontalFOV() const
{
	ASSERT(mDABaseCamera);

    return CameraGetHorizontalFOV(mDABaseCamera);
}
// --------------------------------------------------------------------------
float DACameraDynamicsState::GetVerticalFOV() const
{
	ASSERT(mDABaseCamera);

	return CameraGetVerticalFOV(mDABaseCamera);
}
// --------------------------------------------------------------------------
void DACameraDynamicsState::SetPosition(const Position& position)
{
	ASSERT(mDABaseCamera);

	const Location	location = position.GetLocation();
	Vector			loc(location.GetX(), location.GetY(), location.GetZ());

	CameraSetPosition(mDABaseCamera, loc);
	CameraSetOrientation(mDABaseCamera, position.GetOrientation());
}
// --------------------------------------------------------------------------
void DACameraDynamicsState::SetForce(const Force& force)
{
	ASSERT(mDABaseCamera);

    mForce = force;
}
// --------------------------------------------------------------------------
void DACameraDynamicsState::SetAngularVelocity(const AngularVelocity& angularVelocity)
{
	ASSERT(mDABaseCamera);

    mAngularVelocity = angularVelocity;
}
// --------------------------------------------------------------------------
void DACameraDynamicsState::SetTorque(const Torque& torque)
{
	ASSERT(mDABaseCamera);

    mTorque = torque;
}
// --------------------------------------------------------------------------
void DACameraDynamicsState::SetLinearVelocity(const LinearVelocity& linearVelocity)
{
	ASSERT(mDABaseCamera);

    mLinearVelocity = linearVelocity;
}
// --------------------------------------------------------------------------
void DACameraDynamicsState::SetHorizontalFOV(float hFOV)
{
	ASSERT(mDABaseCamera);

	CameraSetHorizontalFOV(mDABaseCamera, hFOV);
}
// --------------------------------------------------------------------------
void DACameraDynamicsState::SetVerticalFOV(float vFOV)
{
	ASSERT(mDABaseCamera);

	CameraSetVerticalFOV(mDABaseCamera, vFOV);
}
#if 0
// --------------------------------------------------------------------------
void DACameraDynamicsState::SetDynamicsState(const ADynamicsState& dynamicsState)
{
	ASSERT(mDABaseCamera);

	SetStaticsState(dynamicsState);

	SetForce(dynamicsState.GetForce());
	SetTorque(dynamicsState.GetTorque());
	SetLinearVelocity(dynamicsState.GetLinearVelocity());
	SetAngularVelocity(dynamicsState.GetAngularVelocity());
}
// --------------------------------------------------------------------------
void DACameraDynamicsState::SetCameraDynamicsState(const ACameraDynamicsState& cameraDynamicsState)
{
	ASSERT(mDABaseCamera);

	SetDynamicsState(cameraDynamicsState);

	SetHorizontalFOV(cameraDynamicsState.GetHorizontalFOV());
	SetVerticalFOV(cameraDynamicsState.GetVerticalFOV());
}
#endif
// --------------------------------------------------------------------------
void DACameraDynamicsState::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);

    WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void DACameraDynamicsState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);
	
    const Position    pos = GetPosition();

    oWiz.Put(kPosition, pos);

	// Had to read it back since we wrote it, but don't want to set to the state
    // SetPosition(pos);
    
	oWiz.Put(kForce, mForce);
	oWiz.Put(kTorque, mTorque);
	oWiz.Put(kLinearVelocity, mLinearVelocity);
    oWiz.Put(kAngularVelocity, mAngularVelocity);
}
// --------------------------------------------------------------------------
void DACameraDynamicsState::Read(std::istream& iStream)
{
    BaseClass::Read(iStream);

    ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void DACameraDynamicsState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
	

    Position    pos;
	iWiz.Get(kPosition, pos);

	// Had to read it back since we wrote it, but don't want to set to the state
    // SetPosition(pos);
    
	iWiz.Get(kForce, mForce);
	iWiz.Get(kTorque, mTorque);
	iWiz.Get(kLinearVelocity, mLinearVelocity);
    iWiz.Get(kAngularVelocity, mAngularVelocity);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

