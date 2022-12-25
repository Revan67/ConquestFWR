// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include <iostream>
#include <windows.h>
#include "ASceneEntity.h"
#include "DARenderPipeline.h"
#include "Utils.h"
#include "CodeMsg.h"
#include "ARole.h"
#include "Scene.h"
#include "CodeMsg.h"
#include "ConstSceneEntityStateAccessor.h"
#include "SceneEntityStateAccessor.h"
#include "SceneEntityState.h"
// --------------------------------------------------------------------------
/**# implementation ASceneEntity:: id(C_0886784890)
*/
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN void ASceneEntity::Delete()
{
	// Inform all listeners that the owner entity is going away.
	SceneEntityState&	state = GetSceneEntityState();
	
	state.FireToListeners(SceneEntityEvent(SceneEntityEvent::kSourceEntityDeleted, *this));
	state.RemoveAllListeners();
}
// --------------------------------------------------------------------------
CPP_DEFN ASceneEntity::~ASceneEntity()
{
}
// --------------------------------------------------------------------------
CPP_DEFN ASceneEntity::ASceneEntity()
{
}
// --------------------------------------------------------------------------
CPP_DEFN void ASceneEntity::Respond(const SceneEntityEvent& event)
{
}
// --------------------------------------------------------------------------
CPP_DEFN bool ASceneEntity::IsPersistent() const
{
	return true;
}
// --------------------------------------------------------------------------
CPP_DEFN void ASceneEntity::Write(std::ostream& oStream) const
{
}
// --------------------------------------------------------------------------
CPP_DEFN void ASceneEntity::Read(std::istream& iStream)
{
}
// --------------------------------------------------------------------------
CPP_DEFN void ASceneEntity::Draw(const ROS::DABaseCamera* camera) const
{
	ASSERT(PIPE);

    if(GetSceneEntityState().IsVisible())
	{	// Preserve
		Transform	oldModelView;
    
		PIPE->get_modelview(oldModelView);

		SetupPosition();

		Render(camera);

		// Restore
		PIPE->set_modelview(oldModelView);
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void ASceneEntity::SetupPosition() const
{
}
// --------------------------------------------------------------------------
CPP_DEFN void ASceneEntity::Render(const ROS::DABaseCamera* camera) const
{
}
// --------------------------------------------------------------------------
CPP_DEFN bool ASceneEntity::FindIntersect(const IntersectInfo& intersectInfo, float* distance) const
{
	return false;
}
// --------------------------------------------------------------------------
CPP_DEFN bool ASceneEntity::Intersect(const IntersectInfo& intersectInfo, float* distance) const
{
	if(GetSceneEntityState().IsVisible())
	{
		return FindIntersect(intersectInfo, distance);
	}
	else
	{
		return false;
	}
}
// --------------------------------------------------------------------------
void ASceneEntity::Goto(Time time)
{
}
// --------------------------------------------------------------------------
const std::auto_ptr<ConstSceneEntityStateAccessor> ASceneEntity::GetConstSceneEntityStateAccessor() const
{
	return std::auto_ptr<ConstSceneEntityStateAccessor>(new ConstSceneEntityStateAccessor(*this, GetSceneEntityState()));
}
// --------------------------------------------------------------------------
std::auto_ptr<SceneEntityStateAccessor> ASceneEntity::GetSceneEntityStateAccessor()
{
	return std::auto_ptr<SceneEntityStateAccessor>(new SceneEntityStateAccessor(*this, GetSceneEntityState()));
}
// --------------------------------------------------------------------------
bool ASceneEntity::Performing() const
{
	const Scene&	scene = GetSceneEntityState().GetScene();

	return scene.IsPerforming();
}
// --------------------------------------------------------------------------
Time ASceneEntity::GetCurrentTimePoint() const
{
	return GetSceneEntityState().GetScene().GetCurrentTimePoint();
}
// --------------------------------------------------------------------------
void ASceneEntity::StateUpdated(Update::ID id)
{
	if(!Performing())
    {
		StateUpdated(id, GetCurrentTimePoint());
    }
}
// --------------------------------------------------------------------------
void ASceneEntity::StateUpdated(Update::ID id, Time time)
{
}
// --------------------------------------------------------------------------
void ASceneEntity::RoleUpdated()
{
	Goto(GetCurrentTimePoint());
}
// --------------------------------------------------------------------------
ASceneEntity* ASceneEntity::GetDependentEntity()
{
	return NULL;
}
// --------------------------------------------------------------------------
void ASceneEntity::AddListener(ASceneEntityEventListener& listener)
{
	GetSceneEntityState().AddListener(listener);
}
// --------------------------------------------------------------------------
void ASceneEntity::RemoveListener(ASceneEntityEventListener& listener)
{
	GetSceneEntityState().RemoveListener(listener);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

