// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ACameraDynamicsState_h
#define ACameraDynamicsState_h
// --------------------------------------------------------------------------
#include "ACameraState.h"
#include "AStaticsState.h"
#include "ADynamicsState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
//  ACameraDynamicsState
// --------------------------------------------------------------------------
class ACameraDynamicsState: public virtual ACameraState, public virtual AStaticsState, public virtual ADynamicsState
{
    public:
		virtual void SetCameraDynamicsState(const ACameraDynamicsState& cameraDynamicsState);
	
	private:
		typedef ACameraState CameraStateBaseClass;
		typedef AStaticsState StaticsStateBaseClass;
		typedef ADynamicsState DynamicsStateBaseClass;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif