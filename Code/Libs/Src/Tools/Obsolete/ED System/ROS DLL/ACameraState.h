// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ACameraState_h
#define ACameraState_h
// --------------------------------------------------------------------------
#include "APhysicalState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  ACameraState
// --------------------------------------------------------------------------
class ACameraState: public virtual APhysicalState
{
    public:
        virtual float GetHorizontalFOV() const = 0;
        virtual float GetVerticalFOV() const = 0;

        virtual void SetHorizontalFOV(float hFOV) = 0;
        virtual void SetVerticalFOV(float vFOV) = 0;

		virtual void SetCameraState(const ACameraState& cameraState);
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
