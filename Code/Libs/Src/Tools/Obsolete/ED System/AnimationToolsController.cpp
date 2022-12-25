// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "AnimationToolsController.h"
#include "SceneModel.h"
#include "Utils.h"
#include "DAAudioObject.h"
// --------------------------------------------------------------------------
/**# implementation AnimationToolsController:: id(C_0897405670)
*/
// --------------------------------------------------------------------------
AnimationToolsController::AnimationToolsController(ROS::SceneModel& sceneModel, UpdateCB& updateCB)
: mSceneModelSP(&sceneModel), mUpdateCB(updateCB)
{
    mSceneModelSP->Attach(*this);
}
// --------------------------------------------------------------------------
AnimationToolsController::~AnimationToolsController()
{
	if(IsNotNull(mSceneModelSP))
	{
		mSceneModelSP->Detach(*this);
		mSceneModelSP.reset(NULL);
	}
}
// --------------------------------------------------------------------------
void AnimationToolsController::Update(int updateID)
{
    if(updateID == ModelNS::kSceneModelReplaced)
    {
		mSceneModelSP.reset(mSceneModelSP->GetNewSceneModel());
    }
	else 
	{
		if(updateID == ModelNS::kSceneSystemShutdown)
        {
			mSceneModelSP->Detach(*this);
			mSceneModelSP.reset(NULL);
        }
		else
		{
			mUpdateCB(updateID);
		}
	}
}
// --------------------------------------------------------------------------
ROS::ROSString AnimationToolsController::GetSceneFileName() const
{
    return mSceneModelSP->GetSceneFileName();
}
// --------------------------------------------------------------------------
ROS::Time AnimationToolsController::GetSceneDuration() const
{
    return mSceneModelSP->GetSceneDuration();
}
// --------------------------------------------------------------------------
void AnimationToolsController::SetSceneDuration(ROS::Time duration)
{
    mSceneModelSP->SetSceneDuration(duration);
}
// --------------------------------------------------------------------------
void AnimationToolsController::PlayScene()
{
	mSceneModelSP->SetCurrentSceneTime(ROS::Time(0));
    mTimer.Run();
    mSceneModelSP->PlayScene();
}
// --------------------------------------------------------------------------
void AnimationToolsController::PauseScene()
{
	mTimer.Stop();
    mSceneModelSP->PauseScene();
}
// --------------------------------------------------------------------------
bool AnimationToolsController::IsScenePlaying() const
{
	return mSceneModelSP->IsScenePlaying();
}
// --------------------------------------------------------------------------
void AnimationToolsController::SaveScene(const ROS::ROSString& fileName) const
{
	mSceneModelSP->SaveScene(fileName);
}
// --------------------------------------------------------------------------
void AnimationToolsController::GetSceneEntities(ROS::SceneEntityCollection& sceneEntityCollectionR) const
{
    mSceneModelSP->GetSceneEntities(sceneEntityCollectionR);
}
// --------------------------------------------------------------------------
ROS::ASceneEntity* AnimationToolsController::GetSelectedSceneEntity() const
{
    return mSceneModelSP->GetSelectedSceneEntity();
}
// --------------------------------------------------------------------------
void AnimationToolsController::UpdateScene()
{
	if(IsScenePlaying())
	{
		float timeDelta = mTimer.GetElapsedTime();

     	mTimer.Reset();
	    mTimer.Run();

        mSceneModelSP->UpdateScene(ROS::Time(timeDelta));

		AudioObjectSystemUpdate();
    }
}
// --------------------------------------------------------------------------

