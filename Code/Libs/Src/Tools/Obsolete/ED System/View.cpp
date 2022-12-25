// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "View.h"
#include "SceneModel.h"
#include "Utils.h"
// --------------------------------------------------------------------------
View::View (ROS::SceneModel& sceneModel)
:mSceneModel(&sceneModel)
{
    mSceneModel->Attach(*this);
}
// --------------------------------------------------------------------------
View::~View()
{
	mSceneModel->Detach(*this);
	mSceneModel.reset(NULL);
}
// --------------------------------------------------------------------------
void View::SetSelectedSceneEntity (ROS::ASceneEntity* sceneEntityP)
{
	mSceneModel->SetSelectedSceneEntity(sceneEntityP);
}
// --------------------------------------------------------------------------
void View::SelectedSceneEntityUpdated()
{
	mSceneModel->SelectedSceneEntityUpdated();
}
// --------------------------------------------------------------------------
void View::LockSceneEntitySelection(bool lock)
{
    mSceneModel->LockSceneEntitySelection(lock);
}
// --------------------------------------------------------------------------
bool View::IsSceneEntitySelectionLocked() const
{
    return mSceneModel->IsSceneEntitySelectionLocked();
}
// --------------------------------------------------------------------------
void View::GetSceneEntities(ROS::SceneEntityCollection& sceneEntityCollectionR) const
{
    mSceneModel->GetSceneEntities(sceneEntityCollectionR);
}
// --------------------------------------------------------------------------
ROS::ASceneEntity* View::GetSelectedSceneEntity()
{
    return mSceneModel->GetSelectedSceneEntity();
}
// --------------------------------------------------------------------------
void View::SetSecondarySceneEntity(ROS::ASceneEntity* sceneEntityP)
{
	mSceneModel->SetSecondarySceneEntity(sceneEntityP);
}
// --------------------------------------------------------------------------
void View::SecondarySceneEntityUpdated()
{
	mSceneModel->SecondarySceneEntityUpdated();
}
// --------------------------------------------------------------------------
void View::SceneUpdated()
{
	mSceneModel->SceneUpdated();
}
// --------------------------------------------------------------------------
ROS::Time View::GetCurrentSceneTime() const
{
	return mSceneModel->GetCurrentSceneTime();
}
// --------------------------------------------------------------------------
void View::AddUndoableOperation(ROS::AOperation* operation)
{
	mSceneModel->AddUndoableOperation(operation);
}
// --------------------------------------------------------------------------
void View::UndoLastOperation()
{
	mSceneModel->UndoLastOperation();
}
// --------------------------------------------------------------------------
void View::RedoLastOperation()
{
	mSceneModel->RedoLastOperation();
}
// --------------------------------------------------------------------------
void View::RememberPosition(const ROS::Position& position)
{
	mSceneModel->RememberPosition(position);
}
// --------------------------------------------------------------------------
ROS::Position View::RecallPosition() const
{
	return mSceneModel->RecallPosition();
}
// --------------------------------------------------------------------------
void View::RememberHardPoint(const ROS::HardPoint& hardPoint)
{
	mSceneModel->RememberHardPoint(hardPoint);
}
// --------------------------------------------------------------------------
ROS::HardPoint View::RecallHardPoint() const
{
	return mSceneModel->RecallHardPoint();
}
// --------------------------------------------------------------------------
void View::Update(int updateID)
{
    if(updateID == ModelNS::kSceneModelReplaced)
    {
		ROS::SceneModel* sceneModel = mSceneModel->GetNewSceneModel();

        mSceneModel.reset(sceneModel);
    }
}
// --------------------------------------------------------------------------
