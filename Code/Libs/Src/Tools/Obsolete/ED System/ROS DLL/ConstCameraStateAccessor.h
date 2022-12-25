// Author: Shaival Varma
//---------------------------------------------------------------------------
#ifndef ConstCameraStateAccessor_h
#define ConstCameraStateAccessor_h
// --------------------------------------------------------------------------
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ACameraState;
class ACamera;
// --------------------------------------------------------------------------
//  ConstCameraStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL ConstCameraStateAccessor
{
    public:
		ConstCameraStateAccessor(const ACamera& owner, const ACameraState& state);

        float GetHorizontalFOV() const;
        float GetVerticalFOV() const;

    protected:
    	const ACamera& GetOwner() const { return mOwner; };
    	const ACameraState& GetState() const { return mState; };

    private :
		const ACamera& mOwner;
        const ACameraState& mState;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif