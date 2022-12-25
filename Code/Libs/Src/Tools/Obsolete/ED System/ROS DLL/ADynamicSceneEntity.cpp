// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "ADynamicSceneEntity.h"
#include "SceneEntityState.h"
#include "DynamicsRole.h"
#include "Update.h"

/**# implementation ADynamicScneneEntity:: id(C_0890924376) 
*/
// --------------------------------------------------------------------------
namespace ROS
{
CPP_DEFN ADynamicSceneEntity::ADynamicSceneEntity()
{
}
// --------------------------------------------------------------------------
CPP_DEFN ADynamicSceneEntity::~ADynamicSceneEntity()
{
}
// --------------------------------------------------------------------------
const std::auto_ptr<ConstDynamicsStateAccessor> ADynamicSceneEntity::GetConstDynamicsStateAccessor() const
{
	const APhysicalState*	state = &GetPhysicalState();
	const ADynamicsState*	dState = dynamic_cast<const ADynamicsState*>(state);
	ASSERT(dState);

   return std::auto_ptr<ConstDynamicsStateAccessor>(new ConstDynamicsStateAccessor(*this, *dState));
}
// --------------------------------------------------------------------------
std::auto_ptr<DynamicsStateAccessor> ADynamicSceneEntity::GetDynamicsStateAccessor()
{
	APhysicalState*	state = &GetPhysicalState();
	ADynamicsState*	dState = dynamic_cast<ADynamicsState*>(state);
	ASSERT(dState);

   return std::auto_ptr<DynamicsStateAccessor>(new DynamicsStateAccessor(*this, *dState));
}
// --------------------------------------------------------------------------
void ADynamicSceneEntity::Goto(Time time)
{
	BaseClass::Goto(time);
	
	GotoForDynamicRole(time);
}
// --------------------------------------------------------------------------
void ADynamicSceneEntity::GotoForDynamicRole(Time time)
{
	const int	dynamicRoleIndex = GetDynamicRoleIndex();
	
	if(dynamicRoleIndex >= 0)
	{
		const ARole& aRole = GetSceneEntityState().GetRole(dynamicRoleIndex);

		const DynamicsRole* dRole = dynamic_cast<const DynamicsRole*>(&aRole);

		ASSERT(dRole);

		DynamicsState	state = dRole->GetState(time);

		APhysicalState&	aState = GetPhysicalState();

		ADynamicsState*	dState = dynamic_cast<ADynamicsState*>(&aState);
		ASSERT(dState);
        
		dState->SetDynamicsState(state);
	}
}
// --------------------------------------------------------------------------
void ADynamicSceneEntity::StateUpdated(Update::ID id)
{
	BaseClass::StateUpdated(id);
}
// --------------------------------------------------------------------------
void ADynamicSceneEntity::StateUpdated(Update::ID id, Time time)
{
	BaseClass::StateUpdated(id, time);

    if(id == Update::kDynamic)
	{
		DynamicStateUpdated(time);
	}
}
// --------------------------------------------------------------------------
void ADynamicSceneEntity::DynamicStateUpdated(Time time)
{
	const int	dynamicRoleIndex = GetDynamicRoleIndex();
	
	if(dynamicRoleIndex >= 0)
	{
		ARole& aRole = GetSceneEntityState().GetRole(dynamicRoleIndex);

		DynamicsRole*   dRole = dynamic_cast<DynamicsRole*>(&aRole);
		ASSERT(dRole);

		const APhysicalState&	aState = GetPhysicalState();
		const ADynamicsState*	dState = dynamic_cast<const ADynamicsState*>(&aState);
		ASSERT(dState);

		DynamicsState	state(*dState);

		dRole->StateUpdated(state, time);
	}
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
