// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "Controller.h"
#include "Utils.h"
#include "SceneModel.h"
// --------------------------------------------------------------------------
Controller::Controller(ROS::SceneModel& sceneModel)
: mSceneModelP(&sceneModel)
{
	mSceneModelP->Attach(*this);
}
// --------------------------------------------------------------------------
Controller::~Controller ()
{
}
// --------------------------------------------------------------------------
bool Controller::IsScenePresent() const
{
    return IsNotNull(mSceneModelP);
}
// --------------------------------------------------------------------------
void Controller::GetSceneEntities(ROS::SceneEntityCollection& sceneEntityCollectionR) const
{
    mSceneModelP->GetSceneEntities(sceneEntityCollectionR);
}
// --------------------------------------------------------------------------
ROS::ASceneEntity* Controller::GetSelectedSceneEntity() const
{
    return mSceneModelP->GetSelectedSceneEntity();
}
// --------------------------------------------------------------------------
void Controller::SetSelectedSceneEntity(ROS::ASceneEntity* sceneEntity)
{
	mSceneModelP->SetSelectedSceneEntity(sceneEntity);
}
// --------------------------------------------------------------------------
void Controller::SetSecondarySceneEntity(ROS::ASceneEntity* sceneEntityP)
{
	mSceneModelP->SetSecondarySceneEntity(sceneEntityP);
}
// --------------------------------------------------------------------------
void Controller::LockSceneEntitySelection(bool lock)
{
	mSceneModelP->LockSceneEntitySelection(lock);
}
// --------------------------------------------------------------------------
ROS::ASceneEntity* Controller::GetSecondarySceneEntity() const
{
    return mSceneModelP->GetSecondarySceneEntity();
}
// --------------------------------------------------------------------------
void Controller::SecondarySceneEntityUpdated()
{
	mSceneModelP->SecondarySceneEntityUpdated();
}
// --------------------------------------------------------------------------
void Controller::SceneUpdated()
{
	mSceneModelP->SceneUpdated();
}
// --------------------------------------------------------------------------
ROS::Time Controller::GetSceneDuration() const
{
    return mSceneModelP->GetSceneDuration();
}
// --------------------------------------------------------------------------
void Controller::SetCurrentSceneTime(ROS::Time time)
{
	mSceneModelP->SetCurrentSceneTime(time);
}
// --------------------------------------------------------------------------
ROS::Time Controller::GetCurrentSceneTime() const
{
	return mSceneModelP->GetCurrentSceneTime();
}
// --------------------------------------------------------------------------
void Controller::AddUndoableOperation(ROS::AOperation* operation)
{
	mSceneModelP->AddUndoableOperation(operation);
}
// --------------------------------------------------------------------------
void Controller::UndoLastOperation()
{
	mSceneModelP->UndoLastOperation();
}
// --------------------------------------------------------------------------
void Controller::RedoLastOperation()
{
	mSceneModelP->RedoLastOperation();
}
// --------------------------------------------------------------------------
void Controller::Update(int updateID)
{
    if(updateID == ModelNS::kSceneModelReplaced)
    {
		mSceneModelP.reset(mSceneModelP->GetNewSceneModel());
    }
    else
    {   
		if(updateID == ModelNS::kSceneSystemShutdown)
		{	
			mSceneModelP->Detach(*this);
			mSceneModelP.reset(NULL);
		}
    }
}
// --------------------------------------------------------------------------

