// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SpotLightStateAccessor_h
#define SpotLightStateAccessor_h
// --------------------------------------------------------------------------
#include "ROSDLL.h"
#include "LightStateAccessor.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ASpotLight;
class ASpotLightState;
// --------------------------------------------------------------------------
//  SpotLightStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL SpotLightStateAccessor: public LightStateAccessor
{
    public:
    	SpotLightStateAccessor(ASpotLight& owner, ASpotLightState& state);
        
		void SetInfinite(bool infinite);
		void SetRange(float range);
		void SetCutOff(float cutOff);
		
		bool IsInfinite() const;
		float GetRange() const;
		float GetCutOff() const;

    private:
		typedef LightStateAccessor	BaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif