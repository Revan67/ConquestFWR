// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ADynamicSpotLightState_h
#define ADynamicSpotLightState_h
// --------------------------------------------------------------------------
#include "AStaticsState.h"
#include "ADynamicsState.h"
#include "ASpotLightState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//	ADynamicSpotLightState
// --------------------------------------------------------------------------
class ADynamicSpotLightState: public virtual AStaticsState, public virtual ADynamicsState, public virtual ASpotLightState
{
	public:
		virtual void SetDynamicSpotLightState(const ADynamicSpotLightState& lightState);

		void Interpolate(const ADynamicSpotLightState& nextState, float t, ADynamicSpotLightState& tState) const;

	protected:
        ADynamicSpotLightState();
		ADynamicSpotLightState(const ADynamicSpotLightState& lightState);

	private:
		typedef ADynamicsState ADynamicsStateBaseClass;
		typedef ASpotLightState ASpotLightStateBaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif