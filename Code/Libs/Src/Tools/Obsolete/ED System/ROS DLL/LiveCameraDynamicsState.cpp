// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "LiveCameraDynamicsState.h"
#include "ADynamicCamera.h"
#include "ConstCameraStateAccessor.h"
#include "CameraStateAccessor.h"
#include "ConstStaticsStateAccessor.h"
#include "StaticsStateAccessor.h"
#include "ConstDynamicsStateAccessor.h"
#include "DynamicsStateAccessor.h"
// --------------------------------------------------------------------------
enum FieldID
{
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
LiveCameraDynamicsState::LiveCameraDynamicsState(ADynamicCamera* liveCamera)
:mLiveCamera(liveCamera)
{
}
// --------------------------------------------------------------------------
float LiveCameraDynamicsState::GetHorizontalFOV() const
{
	if(mLiveCamera)
	{
		return mLiveCamera->GetConstCameraStateAccessor()->GetHorizontalFOV();
	}
	else
	{
		return 45.0000;
	}
}
// --------------------------------------------------------------------------
float LiveCameraDynamicsState::GetVerticalFOV() const
{
	if(mLiveCamera)
	{
		return mLiveCamera->GetConstCameraStateAccessor()->GetVerticalFOV();
	}
	else
	{
		return 36.8699;
	}
}
// --------------------------------------------------------------------------
void LiveCameraDynamicsState::SetHorizontalFOV(float hFOV)
{
	if(mLiveCamera)
	{
		mLiveCamera->GetCameraStateAccessor()->SetHorizontalFOV(hFOV);
	}
}
// --------------------------------------------------------------------------
void LiveCameraDynamicsState::SetVerticalFOV(float vFOV)
{
	if(mLiveCamera)
	{
		mLiveCamera->GetCameraStateAccessor()->SetVerticalFOV(vFOV);
	}
}
// --------------------------------------------------------------------------
Position LiveCameraDynamicsState::GetPosition() const
{
	if(mLiveCamera)
	{
		std::auto_ptr<ConstStaticsStateAccessor>	access = mLiveCamera->GetConstStaticsStateAccessor();

		const Location		location = access->GetLocation();
		const Orientation	orientation = access->GetOrientation();

		return Position(location, orientation);
	}
}
// --------------------------------------------------------------------------
void LiveCameraDynamicsState::SetPosition(const Position& position)
{
	if(mLiveCamera)
	{
		std::auto_ptr<StaticsStateAccessor>	access = mLiveCamera->GetStaticsStateAccessor();
		
		access->SetLocation(position.GetLocation());
		access->SetOrientation(position.GetOrientation());
	}
}
// --------------------------------------------------------------------------
Force LiveCameraDynamicsState::GetForce() const
{
	if(mLiveCamera)
	{
		return mLiveCamera->GetConstDynamicsStateAccessor()->GetForce();
	}
	else
	{
		return Force();
	}
}
// --------------------------------------------------------------------------
AngularVelocity LiveCameraDynamicsState::GetAngularVelocity() const
{
	if(mLiveCamera)
	{
		return mLiveCamera->GetConstDynamicsStateAccessor()->GetAngularVelocity();
	}
	else
	{
		return AngularVelocity();
	}
}
// --------------------------------------------------------------------------
Torque LiveCameraDynamicsState::GetTorque() const
{
	if(mLiveCamera)
	{
		return mLiveCamera->GetConstDynamicsStateAccessor()->GetTorque();
	}
	else
	{
		return Torque();
	}
}
// --------------------------------------------------------------------------
LinearVelocity LiveCameraDynamicsState::GetLinearVelocity() const
{
	if(mLiveCamera)
	{
		return mLiveCamera->GetConstDynamicsStateAccessor()->GetLinearVelocity();
	}
	else
	{
		return LinearVelocity();
	}
}
// --------------------------------------------------------------------------
void LiveCameraDynamicsState::SetForce(const Force& force)
{
	if(mLiveCamera)
	{
		mLiveCamera->GetDynamicsStateAccessor()->SetForce(force);
	}
}
// --------------------------------------------------------------------------
void LiveCameraDynamicsState::SetAngularVelocity(const AngularVelocity& angularVelocity)
{
	if(mLiveCamera)
	{
		mLiveCamera->GetDynamicsStateAccessor()->SetAngularVelocity(angularVelocity);
	}
}
// --------------------------------------------------------------------------
void LiveCameraDynamicsState::SetTorque(const Torque& torque)
{
	if(mLiveCamera)
	{
		mLiveCamera->GetDynamicsStateAccessor()->SetTorque(torque);
	}
}
// --------------------------------------------------------------------------
void LiveCameraDynamicsState::SetLinearVelocity(const LinearVelocity& linearVelocity)
{
	if(mLiveCamera)
	{
		mLiveCamera->GetDynamicsStateAccessor()->SetLinearVelocity(linearVelocity);
	}
}
// --------------------------------------------------------------------------
void LiveCameraDynamicsState::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void LiveCameraDynamicsState::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void LiveCameraDynamicsState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);
}
// --------------------------------------------------------------------------
void LiveCameraDynamicsState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
