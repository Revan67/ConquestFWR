// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ASpotLightState_h
#define ASpotLightState_h
// --------------------------------------------------------------------------
#include "ALightState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	ASpotLightState
// --------------------------------------------------------------------------
class ASpotLightState: public virtual ALightState
{
	public:
		virtual void SetInfinite(bool infinite) = 0;
		virtual void SetRange(float range) = 0;
		virtual void SetCutOff(float cutOff) = 0;

		virtual bool IsInfinite() const = 0;
		virtual float GetRange() const = 0;
		virtual float GetCutOff() const = 0;

		virtual void SetSpotLightState(const ASpotLightState& lightState);

		void Interpolate(const ASpotLightState& nextState, float t, ASpotLightState& tState) const;

	protected:
        ASpotLightState();
		ASpotLightState(const ASpotLightState& lightState);

	private:
		typedef ALightState BaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif