// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ConstLightStateAccessor_h
#define ConstLightStateAccessor_h
// --------------------------------------------------------------------------
#include "ROSDLL.h"
#include "Color.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ALightState;
class ALight;
// --------------------------------------------------------------------------
//  ConstLightStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL ConstLightStateAccessor
{
    public:
    	ConstLightStateAccessor(const ALight& owner, const ALightState& state);
		
        Color GetColor() const;
        
    protected:
    	const ALight& GetOwner() const;
    	const ALightState& GetState() const;

    private:
        const ALight&		mOwner;
		const ALightState&	mState;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif