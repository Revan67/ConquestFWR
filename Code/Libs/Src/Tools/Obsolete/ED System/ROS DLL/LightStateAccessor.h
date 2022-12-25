// --------------------------------------------------------------------------
// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef LightStateAccessor_h
#define LightStateAccessor_h
// --------------------------------------------------------------------------
#include "ROSDLL.h"
#include "Color.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ALight;
class ALightState;
namespace Update
{
enum ID;
}
// --------------------------------------------------------------------------
//  LightStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL LightStateAccessor
{
    public:
    	LightStateAccessor(ALight& owner, ALightState& state)
		: mOwner(owner), mState(state)
		{
		}

        Color GetColor() const;
        
		void SetColor(const Color& color);

    protected:
    	ALight& GetOwner();
    	const ALight& GetOwner() const;

    	ALightState& GetState();
    	const ALightState& GetState() const;

		void OwnerStateUpdated(Update::ID update);

    private:
        ALight&			mOwner;
        ALightState&	mState;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif