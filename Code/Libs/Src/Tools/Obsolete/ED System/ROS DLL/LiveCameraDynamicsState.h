// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef LiveCameraDynamicsState_h
#define LiveCameraDynamicsState_h
// --------------------------------------------------------------------------
#include "ACameraDynamicsState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ADynamicCamera;
// --------------------------------------------------------------------------
//  LiveCameraDynamicsState
// --------------------------------------------------------------------------
class LiveCameraDynamicsState: public ACameraDynamicsState
{
    public:
		explicit LiveCameraDynamicsState(ADynamicCamera* liveCamera);

		void SetLiveCamera(ADynamicCamera* liveCamera);
		ADynamicCamera* GetLiveCamera();

        virtual float GetHorizontalFOV() const;
        virtual float GetVerticalFOV() const;

        virtual void SetHorizontalFOV(float hFOV);
        virtual void SetVerticalFOV(float vFOV);
	
        virtual Position GetPosition() const;
        virtual void SetPosition(const Position& position);

        virtual Force GetForce() const;
        virtual AngularVelocity GetAngularVelocity() const;
        virtual Torque GetTorque() const;
        virtual LinearVelocity GetLinearVelocity() const;
        virtual void SetForce(const Force& force);
        virtual void SetAngularVelocity(const AngularVelocity& angularVelocity);
        virtual void SetTorque(const Torque& torque);
        virtual void SetLinearVelocity(const LinearVelocity& linearVelocity);

		virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& iStream);

	private:
		typedef ACameraDynamicsState BaseClass;

		void WriteSubObject(std::ostream& oStream) const;
		void ReadSubObject(std::istream& iStream);

		ADynamicCamera*	mLiveCamera;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif