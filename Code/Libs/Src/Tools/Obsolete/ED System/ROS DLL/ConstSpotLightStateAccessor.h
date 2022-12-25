// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ConstSpotLightStateAccessor_h
#define ConstSpotLightStateAccessor_h
// --------------------------------------------------------------------------
#include "ROSDLL.h"
#include "ConstLightStateAccessor.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ASpotLight;
class ASpotLightState;
// --------------------------------------------------------------------------
//  SpotLightStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL ConstSpotLightStateAccessor: public ConstLightStateAccessor
{
    public:
    	ConstSpotLightStateAccessor(const ASpotLight& owner, const ASpotLightState& state);
        
		bool IsInfinite() const;
		float GetRange() const;
		float GetCutOff() const;

    private:
		typedef ConstLightStateAccessor	BaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif