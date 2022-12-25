// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DACameraDynamicsState_h
#define DACameraDynamicsState_h

#include "ACameraDynamicsState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class DABaseCamera;
// --------------------------------------------------------------------------
//  DACameraDynamicsState
// --------------------------------------------------------------------------
class DACameraDynamicsState: public ACameraDynamicsState
{
    friend DACameraDynamicsState Interpolate(const DACameraDynamicsState& previousState, const DACameraDynamicsState& nextState, float t);

    public:
        explicit DACameraDynamicsState(const DABaseCamera* camera = NULL);

		void SetDABaseCamera(const DABaseCamera* camera);
		const DABaseCamera* GetDABaseCamera() const;

        virtual Position GetPosition() const;
        virtual Force GetForce() const;
        virtual AngularVelocity GetAngularVelocity() const;
        virtual Torque GetTorque() const;
        virtual LinearVelocity GetLinearVelocity() const;
        virtual float GetHorizontalFOV() const;
        virtual float GetVerticalFOV() const;

        virtual void SetPosition(const Position& position);
        virtual void SetForce(const Force& force);
        virtual void SetAngularVelocity(const AngularVelocity& angularVelocity);
        virtual void SetTorque(const Torque& torque);
        virtual void SetLinearVelocity(const LinearVelocity& linearVelocity);
        virtual void SetHorizontalFOV(float hFOV);
        virtual void SetVerticalFOV(float vFOV);

		DACameraDynamicsState Interpolate(const DACameraDynamicsState& nextState, float t) const;

        virtual void Write(std::ostream& oStream) const;
        virtual void Read(std::istream& iStream);

    protected:
#if 0
        DACameraDynamicsState& operator=(const AStaticsState& staticsState);
#endif

//        DACamera& GetOwner();
//        const DACamera& GetOwner() const;

    private:
        typedef ACameraDynamicsState BaseClass;

        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

        const ROS::DABaseCamera*    mDABaseCamera;
        Force						mForce;
        Torque						mTorque;
        LinearVelocity				mLinearVelocity;
        AngularVelocity				mAngularVelocity;
};
// --------------------------------------------------------------------------
inline DACameraDynamicsState DACameraDynamicsState::Interpolate(const DACameraDynamicsState& nextState, float t) const
{
	if(t == 0)
    {	
		return *this;
    }
	else if(t == 1)
    {	
		return nextState;
    }
    else
    {  	
		DACameraDynamicsState	tState(GetDABaseCamera());

		tState.SetPosition(GetPosition().Interpolate(nextState.GetPosition(), t));
        tState.SetForce(GetForce().Interpolate(nextState.GetForce(), t));
        tState.SetAngularVelocity(GetAngularVelocity().Interpolate(nextState.GetAngularVelocity(), t));
        tState.SetTorque(GetTorque().Interpolate(nextState.GetTorque(), t));
        tState.SetLinearVelocity(GetLinearVelocity().Interpolate(nextState.GetLinearVelocity(), t));

		return tState;
    }
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
