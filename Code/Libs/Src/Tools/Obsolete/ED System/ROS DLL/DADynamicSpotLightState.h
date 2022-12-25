// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DADynamicSpotLightState_h
#define DADynamicSpotLightState_h
// --------------------------------------------------------------------------
#include "ADynamicSpotLightState.h"
#include "DynamicsState.h"
#include "BaseLight.h"
#include "Position.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	DADynamicSpotLightState
// --------------------------------------------------------------------------
class DADynamicSpotLightState: public ADynamicSpotLightState
{
	public:
		DADynamicSpotLightState();
		~DADynamicSpotLightState();

		virtual void SetColor(const Color& color);
		virtual Color GetColor() const;

		virtual void SetInfinite(bool infinite);
		virtual void SetRange(float range);
		virtual void SetCutOff(float cutOff);

		virtual bool IsInfinite() const;
		virtual float GetRange() const;
		virtual float GetCutOff() const;

        virtual Position GetPosition() const;
        virtual void SetPosition(const Position& position);

		virtual Force GetForce() const { return mForce; }
        virtual AngularVelocity GetAngularVelocity() const  { return mAngularVelocity; }
        virtual Torque GetTorque() const { return mTorque; }
        virtual LinearVelocity GetLinearVelocity() const { return mLinearVelocity; }
        virtual void SetForce(const Force& force)  { mForce = force; }
        virtual void SetAngularVelocity(const AngularVelocity& angularVelocity) { mAngularVelocity = angularVelocity; }
        virtual void SetTorque(const Torque& torque) { mTorque = torque; }
        virtual void SetLinearVelocity(const LinearVelocity& linearVelocity) { mLinearVelocity = linearVelocity; }

		virtual void Write(std::ostream& oStream) const;
		virtual void Read(std::istream& iStream);

	private:
		typedef ADynamicSpotLightState BaseClass;

		void WriteSubObject(std::ostream& oStream) const;
		void ReadSubObject(std::istream& iStream);

        Force			mForce;
        Torque			mTorque;
        LinearVelocity	mLinearVelocity;
        AngularVelocity mAngularVelocity;
		BaseLight		mDABaseLight;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif