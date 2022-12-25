// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef CameraDynamicsStateH
#define CameraDynamicsStateH
// --------------------------------------------------------------------------
#include "ACameraDynamicsState.h"
#include "DynamicsState.h"
#include "StaticsState.h"
#include "TimeType.h"
//---------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	CameraDynamicsState
// --------------------------------------------------------------------------
class CameraDynamicsState: public ACameraDynamicsState
{
	public:
        CameraDynamicsState();
        CameraDynamicsState(const ACameraDynamicsState& cameraDynamicsState);
        CameraDynamicsState(const Position& position, float horizontalFOV, float verticalFOV);
        
		virtual Position GetPosition() const;
		virtual Force GetForce() const;
        virtual AngularVelocity GetAngularVelocity() const;
        virtual Torque GetTorque() const;
        virtual LinearVelocity GetLinearVelocity() const;
        virtual float GetHorizontalFOV() const;
        virtual float GetVerticalFOV() const;

        virtual StaticsState GetStaticsState() const;
        virtual DynamicsState GetDynamicsState()const;

		virtual void SetPosition(const Position& kPosition);
        virtual void SetForce(const Force& kForce);
        virtual void SetAngularVelocity(const AngularVelocity& kAngularVelocity);
        virtual void SetTorque(const Torque& kTorque);
        virtual void SetLinearVelocity(const LinearVelocity& kLinearVelocity);
        virtual void SetHorizontalFOV(float hFOV);
        virtual void SetVerticalFOV(float vFOV);

		CameraDynamicsState Interpolate(const CameraDynamicsState& nextState, float t) const;


        virtual void Write(std::ostream& oStream) const;
        virtual void Read(std::istream& iStream);

	private:
		typedef ACameraDynamicsState BaseClass;

        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

		Position		mPosition;
		DynamicsState	mDynamicsState;
        float           mHorizontalFOV;
        float           mVerticalFOV;
};
// --------------------------------------------------------------------------
inline CameraDynamicsState CameraDynamicsState::Interpolate(const CameraDynamicsState& nextState, float t) const
{
	CameraDynamicsState	cameraState;
	DynamicsState		dynamicsState;
	Position			position;
    float               horFOV, verFOV;
	float               diff = 1 - t;

    horFOV = (mHorizontalFOV * diff) + nextState.mHorizontalFOV * t;
    verFOV = (mVerticalFOV * diff) + nextState.mVerticalFOV * t;

	position = mPosition.Interpolate(nextState.mPosition, t);
	mDynamicsState.Interpolate(nextState.mDynamicsState, t, dynamicsState);

    cameraState.mHorizontalFOV = horFOV;
    cameraState.mVerticalFOV = verFOV;
	cameraState.SetPosition(position);
	cameraState.SetForce(dynamicsState.GetForce());
	cameraState.SetAngularVelocity(dynamicsState.GetAngularVelocity());
	cameraState.SetTorque(dynamicsState.GetTorque());
	cameraState.SetLinearVelocity(dynamicsState.GetLinearVelocity());

	return cameraState;
}
// --------------------------------------------------------------------------
inline CameraDynamicsState LinearInterpolate(const CameraDynamicsState& previousState, Time previousTime, const CameraDynamicsState& nextState, Time nextTime, Time currentTime)
{
    if(nextTime == previousTime)
    {
		return previousState;
    }
    else
    {
		float t = (currentTime - previousTime).GetTime() / (nextTime - previousTime).GetTime();

        return previousState.Interpolate(nextState, t);
    }
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
