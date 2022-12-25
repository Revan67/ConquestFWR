// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ConstSceneEntityStateAccessor.h"
#include "ASceneEntity.h"
#include "SceneEntityState.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
ConstSceneEntityStateAccessor::ConstSceneEntityStateAccessor(const ASceneEntity&	entity, const SceneEntityState& state)
: mSceneEntity(entity), mState(state)
{
}
// --------------------------------------------------------------------------
ROSString ConstSceneEntityStateAccessor::GetName() const
{
	return mState.GetName();
}
// --------------------------------------------------------------------------
bool ConstSceneEntityStateAccessor::IsVisible() const
{
	return mState.IsVisible();
}
// --------------------------------------------------------------------------
void* ConstSceneEntityStateAccessor::GetUserData() const
{
	return mState.GetUserData();
}
// --------------------------------------------------------------------------
const ARole& ConstSceneEntityStateAccessor::GetRole(unsigned int roleIndex) const
{
	return mState.GetRole(roleIndex);
}
// --------------------------------------------------------------------------
unsigned int ConstSceneEntityStateAccessor::GetRoleCount() const
{
	return mState.GetRoleCount();
}
// --------------------------------------------------------------------------
bool ConstSceneEntityStateAccessor::Intersect(const IntersectInfo& intersectInfo, float* distance) const
{
	return mSceneEntity.Intersect(intersectInfo, distance);
}
// --------------------------------------------------------------------------
void ConstSceneEntityStateAccessor::Draw(const ROS::DABaseCamera* camera) const
{
	mSceneEntity.Draw(camera);
}
// --------------------------------------------------------------------------
}
