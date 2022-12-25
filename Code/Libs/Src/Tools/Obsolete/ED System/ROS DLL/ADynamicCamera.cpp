// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ADynamicCamera.h"
#include "ACameraDynamicsState.h"
// --------------------------------------------------------------------------
/**# implementation ADynamicCamera:: id(C_0901741906)
*/
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
ADynamicCamera::ADynamicCamera()
{
}
// --------------------------------------------------------------------------
void ADynamicCamera::Goto(Time time)
{
	ACameraBaseClass::Goto(time);
	ADynamicSceneEntityBaseClass::Goto(time);
}
// --------------------------------------------------------------------------
void ADynamicCamera::SetState(const ADynamicCamera& camera)
{
	const APhysicalState*		aSrcState = &(camera.GetPhysicalState());
	const ACameraDynamicsState*	srcState = dynamic_cast<const ACameraDynamicsState*>(aSrcState);
	ASSERT(srcState);

	APhysicalState*			aDstState = &GetPhysicalState();
	ACameraDynamicsState*	dstState = dynamic_cast<ACameraDynamicsState*>(aDstState);
	ASSERT(dstState);

	dstState->SetCameraDynamicsState(*srcState);
}
// --------------------------------------------------------------------------
void ADynamicCamera::StateUpdated(Update::ID id)
{
	ACameraBaseClass::StateUpdated(id);
	ADynamicSceneEntityBaseClass::StateUpdated(id);
}
// --------------------------------------------------------------------------
void ADynamicCamera::StateUpdated(Update::ID id, Time time)
{
	ACameraBaseClass::StateUpdated(id, time);
	ADynamicSceneEntityBaseClass::StateUpdated(id, time);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
