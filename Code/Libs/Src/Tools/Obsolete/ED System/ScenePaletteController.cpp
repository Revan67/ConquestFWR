// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ScenePaletteController.h"
#include "SceneModel.h"
#include "Utils.h"
#include "SceneView.h"
// --------------------------------------------------------------------------
/**# implementation ScenePaletteController:: id(C_0888876533)
*/
// --------------------------------------------------------------------------
ScenePaletteController::ScenePaletteController(ROS::SceneModel& sceneModelR, UpdateCB& updateCallbackR)
:mSceneModelSP(&sceneModelR), mUpdateCB(updateCallbackR)
{
	sceneModelR.Attach(*this);
}
// --------------------------------------------------------------------------
ScenePaletteController::~ScenePaletteController()
{
	delete mSceneModelSP.reset(NULL);
}
// --------------------------------------------------------------------------
ROS::SceneModel& ScenePaletteController::GetSceneModel()
{
    return *mSceneModelSP;
}
// --------------------------------------------------------------------------
ROS::ROSString ScenePaletteController::GetSceneName() const
{
	if(IsScenePresent())
    {
		return mSceneModelSP->GetSceneName();
    }
    else
    {
		return "";
    }
}
// --------------------------------------------------------------------------
ROS::ROSString ScenePaletteController::GetSceneFileName() const
{
	if(IsScenePresent())
    {
		return mSceneModelSP->GetSceneFileName();
    }
    else
    {
		return "";
    }
}
// --------------------------------------------------------------------------
bool ScenePaletteController::IsScenePresent() const
{
	return IsNotNull(mSceneModelSP);
}
// --------------------------------------------------------------------------
void ScenePaletteController::SaveScene(const ROS::ROSString& fileName, bool saveAndConvert) const
{
	mSceneModelSP->SaveScene(fileName, saveAndConvert);
}
// --------------------------------------------------------------------------
void ScenePaletteController::PauseScene()
{
	mSceneModelSP->PauseScene();
}
// --------------------------------------------------------------------------
void ScenePaletteController::SetCurrentSceneTime(ROS::Time time)
{
	mSceneModelSP->SetCurrentSceneTime(time);
}
// --------------------------------------------------------------------------
ROS::Time ScenePaletteController::GetCurrentSceneTime() const
{
	return mSceneModelSP->GetCurrentSceneTime();
}
// --------------------------------------------------------------------------
void ScenePaletteController::GetSceneEntities(ROS::SceneEntityCollection& sceneEntityCollectionR) const
{
	mSceneModelSP->GetSceneEntities(sceneEntityCollectionR);
}
// --------------------------------------------------------------------------
void ScenePaletteController::HandleEvent(const ROS::Event& kEventR)
{

}
// --------------------------------------------------------------------------
void ScenePaletteController::Update(int updateID)
{
    if(updateID == ModelNS::kSceneModelReplaced)
    {
		mSceneModelSP.reset(mSceneModelSP->GetNewSceneModel());
    }
    else if(mUpdateCB)
    {
		if(updateID != ModelNS::kSceneSystemShutdown)
        {
			mUpdateCB(updateID);
        }
		else
		{
			mSceneModelSP->Detach(*this);
			mSceneModelSP.reset(NULL);
		}
    }

#if 0
    if(updateID == ModelNS::kSceneModelDeleted)
    {
		mSceneModelSP->Detach(*this);
    }
#endif
}
// --------------------------------------------------------------------------
void ScenePaletteController::SetSelectedSceneEntity(ROS::ASceneEntity* sceneEntityP)
{
	mSceneModelSP->SetSelectedSceneEntity(sceneEntityP);
}
// --------------------------------------------------------------------------
ROS::ASceneEntity* ScenePaletteController::GetSelectedSceneEntity() const
{
	return mSceneModelSP->GetSelectedSceneEntity();
}
// --------------------------------------------------------------------------
ROS::ASceneEntity* ScenePaletteController::GetSecondarySceneEntity() const
{
	return mSceneModelSP->GetSecondarySceneEntity();
}
// --------------------------------------------------------------------------
void ScenePaletteController::SelectedSceneEntityUpdated()
{
	mSceneModelSP->SelectedSceneEntityUpdated();
}
// --------------------------------------------------------------------------
void ScenePaletteController::LockSceneEntitySelection(bool lock)
{
	mSceneModelSP->LockSceneEntitySelection(lock);
}
// --------------------------------------------------------------------------
bool ScenePaletteController::IsSceneEntitySelectionLocked() const
{
	return mSceneModelSP->IsSceneEntitySelectionLocked();
}
// --------------------------------------------------------------------------
void ScenePaletteController::AddSceneView(int screenX, int screenY, HWND parentWndH)
{
	if(IsNotNull(mSceneModelSP))
    {
		new SceneView(*mSceneModelSP, screenX, screenY, parentWndH);
		// We do not need to maintain any information about the View; It takes care of itself.
    }
}
// --------------------------------------------------------------------------
void ScenePaletteController::AddSceneEntity(ROS::ASceneEntity& sceneEntity)
{
	if(IsNotNull(mSceneModelSP))
    {
		mSceneModelSP->AddSceneEntity(sceneEntity);
    }
}
// --------------------------------------------------------------------------
void ScenePaletteController::AddCompoundSceneEntity(const ROS::ROSString& entityName, const ROS::ROSString& categoryName, const ROS::StringList& descriptionStrings)
{
	if(IsNotNull(mSceneModelSP))
    {
		mSceneModelSP->AddCompoundSceneEntity(entityName, categoryName, descriptionStrings);
    }
}
// --------------------------------------------------------------------------
void ScenePaletteController::AddDeformableSceneEntity(const ROS::ROSString& entityName, const ROS::ROSString& categoryName, const ROS::StringList& descriptionStrings)
{
	if(IsNotNull(mSceneModelSP))
    {
		mSceneModelSP->AddDeformableSceneEntity(entityName, categoryName, descriptionStrings);
    }
}
// --------------------------------------------------------------------------
void ScenePaletteController::AddNewPositionMarker()
{
	if(IsNotNull(mSceneModelSP))
    {
		mSceneModelSP->AddNewPositionMarker();
    }
}
// --------------------------------------------------------------------------
void ScenePaletteController::AddNewCamera()
{
	if(IsNotNull(mSceneModelSP))
    {
		mSceneModelSP->AddNewCamera();
    }
}
// --------------------------------------------------------------------------
void ScenePaletteController::AddNewLiveCamera()
{
	if(IsNotNull(mSceneModelSP))
    {
		mSceneModelSP->AddNewLiveCamera();
    }
}
// --------------------------------------------------------------------------
void ScenePaletteController::AddNewAmbientLight()
{
	if(IsNotNull(mSceneModelSP))
    {
		mSceneModelSP->AddNewAmbientLight();
    }
}
// --------------------------------------------------------------------------
void ScenePaletteController::AddNewDirectionalLight()
{
	if(IsNotNull(mSceneModelSP))
    {
		mSceneModelSP->AddNewDirectionalLight();
    }
}
// --------------------------------------------------------------------------
void ScenePaletteController::AddNewPointLight()
{
	if(IsNotNull(mSceneModelSP))
    {
		mSceneModelSP->AddNewPointLight();
    }
}
// --------------------------------------------------------------------------
void ScenePaletteController::AddNewSpotLight()
{
	if(IsNotNull(mSceneModelSP))
    {
		mSceneModelSP->AddNewSpotLight();
    }
}
// --------------------------------------------------------------------------
void ScenePaletteController::RenameSceneEntity(const ROS::ROSString& kEntityToRenameR, const ROS::ROSString& kNewNameR)
{
    mSceneModelSP->RenameSceneEntity(kEntityToRenameR, kNewNameR);
}
// --------------------------------------------------------------------------
void ScenePaletteController::RemoveSceneEntity(const ROS::ASceneEntity& sceneEntityR)
{
	mSceneModelSP->RemoveSceneEntity(sceneEntityR);
}
// --------------------------------------------------------------------------
void ScenePaletteController::SceneUpdated()
{
	mSceneModelSP->SceneUpdated();
}
// --------------------------------------------------------------------------
void ScenePaletteController::ReplaceSceneModel(ROS::SceneModel* sceneModel)
{
    mSceneModelSP->SwitchToNewSceneModel(sceneModel);
}
// --------------------------------------------------------------------------
void ScenePaletteController::UndoLastOperation()
{
	mSceneModelSP->UndoLastOperation();
}
// --------------------------------------------------------------------------
void ScenePaletteController::RedoLastOperation()
{
	mSceneModelSP->RedoLastOperation();
}
// --------------------------------------------------------------------------
ROS::ROSString ScenePaletteController::GetUndoOperationName() const
{
	return mSceneModelSP->GetUndoOperationName();
}
// --------------------------------------------------------------------------
ROS::ROSString ScenePaletteController::GetRedoOperationName() const
{
	return mSceneModelSP->GetRedoOperationName();
}
// --------------------------------------------------------------------------
void ScenePaletteController::ShutdownSystem()
{
    mSceneModelSP->ShutdownSystem();
}
// --------------------------------------------------------------------------
