// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "TrackController.h"
#include "Utils.h"
#include "SceneModel.h"
// --------------------------------------------------------------------------
/**# implementation TrackController:: id(C_0897418418)
*/
// --------------------------------------------------------------------------
TrackController::TrackController(ROS::SceneModel& sceneModel, const UpdateCB& updateCB)
: BaseClass(sceneModel), mUpdateCB(updateCB)
{
}
// --------------------------------------------------------------------------
TrackController::~TrackController ()
{
}
// --------------------------------------------------------------------------
void TrackController::Update (int updateID)
{
	BaseClass::Update(updateID);

    if(updateID != ModelNS::kSceneModelReplaced)
    {   
		if(updateID != ModelNS::kSceneSystemShutdown)
        {
			mUpdateCB(updateID);
        }
    }
}
// --------------------------------------------------------------------------

