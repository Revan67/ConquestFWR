// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef DynamicSpotLightState_h
#define DynamicSpotLightState_h
// --------------------------------------------------------------------------
#include "ADynamicSpotLightState.h"
#include "DynamicsState.h"
#include "SpotLightState.h"
#include "Position.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	DynamicSpotLightState
// --------------------------------------------------------------------------
class DynamicSpotLightState: public ADynamicSpotLightState
{
	public:
		DynamicSpotLightState();
		DynamicSpotLightState(const ADynamicSpotLightState& state);

		virtual void SetColor(const Color& color) { mSpotLightState.SetColor(color); }
		virtual Color GetColor() const { return mSpotLightState.GetColor(); }

		virtual void SetInfinite(bool infinite) { mSpotLightState.SetInfinite(infinite); }
		virtual void SetRange(float range) { mSpotLightState.SetRange(range); }
		virtual void SetCutOff(float cutOff) { mSpotLightState.SetCutOff(cutOff); }

		virtual bool IsInfinite() const { return mSpotLightState.IsInfinite(); }
		virtual float GetRange() const { return mSpotLightState.GetRange(); }
		virtual float GetCutOff() const { return mSpotLightState.GetCutOff(); }

        virtual Position GetPosition() const { return mPosition; }
        virtual void SetPosition(const Position& position) { mPosition = position; }

		virtual Force GetForce() const { return mDynamicsState.GetForce(); }
        virtual AngularVelocity GetAngularVelocity() const  { return mDynamicsState.GetAngularVelocity(); }
        virtual Torque GetTorque() const { return mDynamicsState.GetTorque(); }
        virtual LinearVelocity GetLinearVelocity() const { return mDynamicsState.GetLinearVelocity(); }
        virtual void SetForce(const Force& force)  { mDynamicsState.SetForce(force); }
        virtual void SetAngularVelocity(const AngularVelocity& angularVelocity) { mDynamicsState.SetAngularVelocity(angularVelocity); }
        virtual void SetTorque(const Torque& kTorqueR) { mDynamicsState.SetTorque(kTorqueR); }
        virtual void SetLinearVelocity(const LinearVelocity& linearVelocity) { mDynamicsState.SetLinearVelocity(linearVelocity); }

		DynamicSpotLightState& operator=(const ADynamicSpotLightState& lightState);

        virtual void Write(std::ostream& oStream) const;
    	virtual void Read(std::istream& iStream);

	private:
		typedef ADynamicSpotLightState BaseClass;

    	void WriteSubObject(std::ostream& oStream) const;
    	void ReadSubObject(std::istream& iStream);

		Position		mPosition;
		DynamicsState	mDynamicsState;
		SpotLightState	mSpotLightState;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif