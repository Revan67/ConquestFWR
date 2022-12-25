// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "SceneEntityStateAccessor.h"
#include "ASceneEntity.h"
#include "SceneEntityState.h"
#include "Update.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
SceneEntityStateAccessor::SceneEntityStateAccessor(ASceneEntity& sceneEntity, SceneEntityState& state)
: mSceneEntity(sceneEntity), mState(state)
{
}
// --------------------------------------------------------------------------
ROSString SceneEntityStateAccessor::GetName() const
{
	return mState.GetName();
}
// --------------------------------------------------------------------------
bool SceneEntityStateAccessor::IsVisible() const
{
	return mState.IsVisible();
}
// --------------------------------------------------------------------------
void SceneEntityStateAccessor::SetName(const ROSString& name)
{
	mState.SetName(name);
	mSceneEntity.StateUpdated(Update::kName);
}
// --------------------------------------------------------------------------
void SceneEntityStateAccessor::SetVisible(bool isVisible)
{
	mState.SetVisible(isVisible);
	mSceneEntity.StateUpdated(Update::kVisibility);
}
// --------------------------------------------------------------------------
void SceneEntityStateAccessor::SetUserData(void* userData) const
{
	mState.SetUserData(userData);
	mSceneEntity.StateUpdated(Update::kUserData);
}
// --------------------------------------------------------------------------
void* SceneEntityStateAccessor::GetUserData() const
{
	return mState.GetUserData();
}
// --------------------------------------------------------------------------
void SceneEntityStateAccessor::SetTrackId(long trackId)
{
	mState.SetTrackId(trackId);
}
// --------------------------------------------------------------------------
long SceneEntityStateAccessor::GetTrackId() const
{
	return mState.GetTrackId();
}
// --------------------------------------------------------------------------
const ARole& SceneEntityStateAccessor::GetRole(unsigned int roleIndex) const
{
	return mState.GetRole(roleIndex);
}
// --------------------------------------------------------------------------
ARole& SceneEntityStateAccessor::GetRole(unsigned int roleIndex)
{
	return mState.GetRole(roleIndex);
}
// --------------------------------------------------------------------------
unsigned int SceneEntityStateAccessor::GetRoleCount() const
{
	return mState.GetRoleCount();
}
// --------------------------------------------------------------------------
void SceneEntityStateAccessor::RoleUpdated()
{
	mSceneEntity.RoleUpdated();
}
// --------------------------------------------------------------------------
void SceneEntityStateAccessor::Goto(Time time)
{
	mSceneEntity.Goto(time);
}
// --------------------------------------------------------------------------
void SceneEntityStateAccessor::Draw(const ROS::DABaseCamera* camera) const
{
	mSceneEntity.Draw(camera);
}
// --------------------------------------------------------------------------
bool SceneEntityStateAccessor::Intersect(const IntersectInfo& intersectInfo, float* distance) const
{
	return mSceneEntity.Intersect(intersectInfo, distance);
}
// --------------------------------------------------------------------------
ASceneEntity* SceneEntityStateAccessor::GetDependentEntity()
{
	return mSceneEntity.GetDependentEntity();
}
// --------------------------------------------------------------------------
void SceneEntityStateAccessor::AddListener(ASceneEntityEventListener& listener)
{
	mState.AddListener(listener);
}
// --------------------------------------------------------------------------
void SceneEntityStateAccessor::RemoveListener(ASceneEntityEventListener& listener)
{
	mState.RemoveListener(listener);
}
// --------------------------------------------------------------------------
}
