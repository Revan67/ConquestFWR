// Author: Shaival Varma
//---------------------------------------------------------------------------
#ifndef CameraStateAccessor_h
#define CameraStateAccessor_h
// --------------------------------------------------------------------------
#include "DynamicsStateAccessor.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ACamera;
class ACameraState;
// --------------------------------------------------------------------------
//  CameraStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL CameraStateAccessor
{
    public:
		CameraStateAccessor(ACamera& owner, ACameraState& state);

        float GetHorizontalFOV() const;
        float GetVerticalFOV() const;

        void SetHorizontalFOV(float hFOV);
        void SetVerticalFOV(float vFOV);

    	ACamera& GetOwner();
    	const ACamera& GetOwner() const;

    	ACameraState& GetState();
    	const ACameraState& GetState() const;

		void OwnerStateUpdated(Update::ID update);

    private :
        ACamera&		mOwner;
        ACameraState&	mState;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif