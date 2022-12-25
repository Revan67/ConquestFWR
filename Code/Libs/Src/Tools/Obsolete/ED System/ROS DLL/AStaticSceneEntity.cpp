// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "AStaticSceneEntity.h"
#include "CodeMsg.h"
#include "Matrix.h"
#include "ConstStaticsStateAccessor.h"
#include "StaticsStateAccessor.h"
#include "ARole.h"
#include "LocationRole.h"
#include "OrientationRole.h"
#include "DARenderPipeline.h"
#include "RPUL.h"
#include "TransformUtil.h"
#include "SceneEntityState.h"
#include "Position.h"
#include "Update.h"
#include "LocationKeyPointMarker.h"
#include "OrientationKeyPointMarker.h"
#include "Scene.h"
// --------------------------------------------------------------------------
/**# implementation AStaticSceneEntity:: id(C_0890924256)
*/
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN AStaticSceneEntity::AStaticSceneEntity()
{
}
// --------------------------------------------------------------------------
CPP_DEFN AStaticSceneEntity::~AStaticSceneEntity()
{
}
// --------------------------------------------------------------------------
CPP_DEFN void AStaticSceneEntity::Delete()
{
	/******** SHOULD NOT BE NECESSARY TO MAKE PATH INVISIBLE. TRY CODE WITHOUT IT*********/
	if(IsStaticsPathVisible())
	{
		SetStaticsPathVisible(false);
	}

	BaseClass::Delete();
}
// --------------------------------------------------------------------------
CPP_DEFN const std::auto_ptr<ConstStaticsStateAccessor> AStaticSceneEntity::GetConstStaticsStateAccessor() const
{
	const APhysicalState*	state = &GetPhysicalState();
	const AStaticsState*	sState = dynamic_cast<const AStaticsState*>(state);
	ASSERT(sState);

    return std::auto_ptr<ConstStaticsStateAccessor>(new ConstStaticsStateAccessor(*this, *sState));
}
// --------------------------------------------------------------------------
CPP_DEFN std::auto_ptr<StaticsStateAccessor> AStaticSceneEntity::GetStaticsStateAccessor()
{
	APhysicalState*	state = &GetPhysicalState();
	AStaticsState*	sState = dynamic_cast<AStaticsState*>(state);
	ASSERT(sState);

    return std::auto_ptr<StaticsStateAccessor>(new StaticsStateAccessor(*this, *sState));
}
// --------------------------------------------------------------------------
CPP_DEFN Orientation AStaticSceneEntity::GetOrientationInWorld() const
{
    const AStaticSceneEntity* parentEntity = dynamic_cast<const AStaticSceneEntity*>(GetSceneEntityState().GetParentEntity());

	const APhysicalState*	state = &GetPhysicalState();
	const AStaticsState*	sState = dynamic_cast<const AStaticsState*>(state);
	ASSERT(sState);

	const Orientation	orientation = sState->GetPosition().GetOrientation();

    if(parentEntity)
    {
		return parentEntity->GetOrientationInWorld() * orientation;
    }
    else
    {
		return orientation;
    }
}
// --------------------------------------------------------------------------
CPP_DEFN Location AStaticSceneEntity::GetLocationInWorld() const
{
    const AStaticSceneEntity* parentEntity = dynamic_cast<const AStaticSceneEntity*>(GetSceneEntityState().GetParentEntity());

	const APhysicalState*	state = &GetPhysicalState();
	const AStaticsState*	sState = dynamic_cast<const AStaticsState*>(state);
	ASSERT(sState);

	const Location	locationInLocal = sState->GetPosition().GetLocation();

    if(parentEntity)
    {
		ASSERT(0);  // Take parent transformations into account

        return locationInLocal;
    }
    else
    {
        return locationInLocal;
    }
}
// --------------------------------------------------------------------------
CPP_DEFN void AStaticSceneEntity::SetupPosition() const
{
    ASSERT(PIPE);
	
	// Set up transformations
    const std::auto_ptr<ConstStaticsStateAccessor> stateAccess = GetConstStaticsStateAccessor();
    const Location      loc = stateAccess->GetLocation();
    const Orientation   orient = stateAccess->GetOrientation();

	Vector		location(loc.GetX(), loc.GetY(), loc.GetZ());
	::Matrix	orientation(orient.GetI(), orient.GetJ(), orient.GetK());

	Transform	tr(orientation, location);

	Transform	oldModelView;

	PIPE->get_modelview(oldModelView);

	tr = oldModelView * tr;
	
	PIPE->set_modelview(tr);
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::Respond(const SceneEntityEvent& event)
{
	BaseClass::Respond(event);

	if(IsStaticsPathVisible())
	{
		if(event.GetID() == SceneEntityEvent::kSourceEntityDeleted)
		{
			ASceneEntity*	source = &event.GetSourceEntity();
			AMarker*		marker = dynamic_cast<AMarker*>(source);

			if(marker)
			{
				UpdateForDeletedMarker(marker);
			}	
		}
	}
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::Goto(Time time)
{
	BaseClass::Goto(time);
	
	GotoForLocationRole(time);
	GotoForOrientationRole(time);
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::GotoForLocationRole(Time time)
{
    const int	locationRoleIndex = GetLocationRoleIndex();

	if(locationRoleIndex >= 0)
	{
		const ARole& aRole = GetSceneEntityState().GetRole(locationRoleIndex);

		const LocationRole* lRole = dynamic_cast<const LocationRole*>(&aRole);

		ASSERT(lRole);

		Location	location = lRole->GetState(time);

		APhysicalState&	aState = GetPhysicalState();
		AStaticsState*	sState = dynamic_cast<AStaticsState*>(&aState);
		ASSERT(sState);

		Position	position = sState->GetPosition();

		position.SetLocation(location);

		sState->SetPosition(position);
	}
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::GotoForOrientationRole(Time time)
{
	const int	orientationRoleIndex = GetOrientationRoleIndex();

	if(orientationRoleIndex >= 0)
	{
		const ARole& aRole = GetSceneEntityState().GetRole(orientationRoleIndex);
		
		const OrientationRole* oRole = dynamic_cast<const OrientationRole*>(&aRole);

		ASSERT(oRole);

		Orientation	orientation = oRole->GetState(time);

		APhysicalState&	aState = GetPhysicalState();
		AStaticsState*	sState = dynamic_cast<AStaticsState*>(&aState);
		ASSERT(sState);

		Position	position = sState->GetPosition();

		position.SetOrientation(orientation);

		sState->SetPosition(position);
	}
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::StateUpdated(Update::ID id)
{
	BaseClass::StateUpdated(id);
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::StateUpdated(Update::ID id, Time time)
{
	BaseClass::StateUpdated(id, time);

	switch(id)
	{
		case Update::kLocation:
			LocationStateUpdated(time);
			break;

		case Update::kOrientation:
			OrientationStateUpdated(time);
			break;
	}
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::LocationStateUpdated(Time time)
{
	const int	locationRoleIndex = GetLocationRoleIndex();

	if(locationRoleIndex >= 0)
	{
		ARole& aRole = GetSceneEntityState().GetRole(locationRoleIndex);

		LocationRole*   lRole = dynamic_cast<LocationRole*>(&aRole);
		ASSERT(lRole);
		
		const APhysicalState&	aState = GetPhysicalState();
		const AStaticsState*	sState = dynamic_cast<const AStaticsState*>(&aState);
		ASSERT(sState);

		const bool	existingKeyPoint = lRole->HasTime(time);

		const FlaggedLocation	fLocation(sState->GetPosition().GetLocation(), lRole->GetState(time).GetInterpolationType());
		
		lRole->StateUpdated(fLocation, time);

		if(!existingKeyPoint)
		{
			UpdateMotionPathMarkers();
		}
	}
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::OrientationStateUpdated(Time time)
{
	const int	orientationRoleIndex = GetOrientationRoleIndex();
		
	if(orientationRoleIndex >= 0)
	{
		ARole& aRole = GetSceneEntityState().GetRole(orientationRoleIndex);

		OrientationRole*   oRole = dynamic_cast<OrientationRole*>(&aRole);
		ASSERT(oRole);
		
		const APhysicalState&	aState = GetPhysicalState();
		const AStaticsState*	sState = dynamic_cast<const AStaticsState*>(&aState);
		ASSERT(sState);
		
		const bool	existingKeyPoint = oRole->HasTime(time);

		const FlaggedOrientation	currentOrientation = oRole->GetState(time);

		oRole->StateUpdated(FlaggedOrientation(sState->GetPosition().GetOrientation(), currentOrientation.GetInterpolationType(), currentOrientation.GetTargetEntity()), time);

		if(!existingKeyPoint)
		{
			UpdateMotionPathMarkers();
		}
	}
}
// --------------------------------------------------------------------------
CPP_DEFN OrientationMemento AStaticSceneEntity::GetOrientationMemento(Time time) const
{
	const Orientation	orientation = GetConstStaticsStateAccessor()->GetOrientation(time);

	const int		roleIndex = GetOrientationRoleIndex();
	ASSERT(roleIndex >= 0);
	const ARole&	role = GetSceneEntityState().GetRole(roleIndex);

	return OrientationMemento(orientation, role.HasTime(time), time);
}
// --------------------------------------------------------------------------
CPP_DEFN void AStaticSceneEntity::SetOrientationMemento(const OrientationMemento& memento)
{
	const Time	mementoTime = memento.GetTime();

	GetStaticsStateAccessor()->SetOrientation(memento.GetOrientation(), mementoTime);

	if(!memento.IsKeyPoint())
	{
		const int	orientationRoleIndex = GetOrientationRoleIndex();
		ASSERT(orientationRoleIndex >= 0);

		ARole&	role = GetSceneEntityState().GetRole(orientationRoleIndex);

        role.Remove(mementoTime);
	}
}
// --------------------------------------------------------------------------
CPP_DEFN LocationMemento AStaticSceneEntity::GetLocationMemento(Time time) const
{
	const Location	location = GetConstStaticsStateAccessor()->GetLocation(time);

	const int		roleIndex = GetLocationRoleIndex();
	ASSERT(roleIndex >= 0);
	const ARole&	role = GetSceneEntityState().GetRole(roleIndex);

	return LocationMemento(location, role.HasTime(time), time);
}
// --------------------------------------------------------------------------
CPP_DEFN void AStaticSceneEntity::SetLocationMemento(const LocationMemento& memento)
{
	const Time	mementoTime = memento.GetTime();

	GetStaticsStateAccessor()->SetLocation(memento.GetLocation(), mementoTime);

	if(!memento.IsKeyPoint())
	{
		ARole&	role = GetSceneEntityState().GetRole(GetLocationRoleIndex());

        role.Remove(mementoTime);
	}
}
// --------------------------------------------------------------------------
CPP_DEFN AStaticSceneEntity::InterpolationType AStaticSceneEntity::GetLocationInterpolationType(Time time) const
{
	const int			locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
    const ARole&		aRole = GetSceneEntityState().GetRole(locationRoleIndex);
    const LocationRole*	lRole = dynamic_cast<const LocationRole*>(&aRole);
    ASSERT(lRole);

	const FlaggedLocation::InterpolationType	type = lRole->GetState(time).GetInterpolationType();

	switch(type)
	{
		case FlaggedLocation::kLinearFixed:
			return kLinearFixed;
		case FlaggedLocation::kSplineFixed:
			return kSplineFixed;
		case FlaggedLocation::kLinearBlend:
			return kLinearBlend;
		case FlaggedLocation::kSplineBlend:
			return kSplineBlend;
		default:
			ASSERT(0);	// Unknown type!
			return kLinearFixed;	// Just for the compiler's pleasure
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void AStaticSceneEntity::SetStaticsPathVisible(bool visible)
{
	const bool	isVisible = IsStaticsPathVisible();

	if((visible && isVisible) || (!visible && !isVisible))
	{
		// Nothing to do!
		return;
	}

	SceneEntityState&	state = GetSceneEntityState();
	Scene&				scene = state.GetScene();
	
	state.SetStaticsPathVisible(visible);

	if(visible)
	{
		// Add markers for the path. First location, then orientation
		ROSString	name = state.GetName();

		// Add all the location markers
		const int			locationRoleIndex = GetLocationRoleIndex();

		if(locationRoleIndex >= 0)
		{
			ARole*				aRole = &state.GetRole(locationRoleIndex);
			LocationRole*		lRole = dynamic_cast<LocationRole*>(aRole);
			ASSERT(lRole);
			const unsigned int	lCount = lRole->CountTimePoints();

			Marker*	lastMarker = NULL;
			
			for(unsigned int lIdx = 0; lIdx < lCount; ++lIdx)
			{	
				Marker*	marker = new LocationKeyPointMarker(name, false, scene, *this, lIdx);

				if(lastMarker)
				{
					lastMarker->SetNextMarker(marker);
				}

				marker->GetSceneEntityStateAccessor()->AddListener(*this);	// So that this object gets informed if the marker is deleted.
				state.AddSource(*marker);

				scene.AddSceneEntity(*marker);

				lastMarker = marker;
			}

			// Add all the orientation markers
			const int	orientationRoleIndex = GetOrientationRoleIndex();

			if(orientationRoleIndex >= 0)
			{
				aRole = &state.GetRole(orientationRoleIndex);
				OrientationRole*	oRole = dynamic_cast<OrientationRole*>(aRole);
				ASSERT(oRole);
				const unsigned int	oCount = oRole->CountTimePoints();

				lastMarker = NULL;

				for(unsigned int oIdx = 0; oIdx < oCount; ++oIdx)
				{	
					Marker*	marker = new OrientationKeyPointMarker(name, false, scene, *this, oIdx);

					if(lastMarker)
					{
						lastMarker->SetNextMarker(marker);
					}

					marker->GetSceneEntityStateAccessor()->AddListener(*this);	// So that this object gets informed if the marker is deleted.
					state.AddSource(*marker);
					
					scene.AddSceneEntity(*marker);

					lastMarker = marker;
				}
			}
		}
	}
	else
	{
		// Remove markers for the path
		unsigned int	sourceCount = state.GetSourceCount();

		for(unsigned int sourceIdx = 0; sourceIdx < sourceCount;)
		{
			ASceneEntityEventSource*	source = &state.GetSource(sourceIdx);

			AMarker*	marker = dynamic_cast<AMarker*>(source);

			if(marker)
			{
				scene.RemoveSceneEntity(*marker);

				state.RemoveSource(*marker);
				marker->GetSceneEntityStateAccessor()->RemoveListener(*this);	// So that this object does not get informed when the marker is deleted in the next statement.

				marker->Delete();

				--sourceCount;
			}
			else
			{
				++sourceIdx;
			}
		}
	}
}
// --------------------------------------------------------------------------
CPP_DEFN bool AStaticSceneEntity::IsStaticsPathVisible() const
{
	return GetSceneEntityState().IsStaticsPathVisible();
}
// --------------------------------------------------------------------------
CPP_DEFN void AStaticSceneEntity::UpdateMotionPathMarkers()
{
	if(IsStaticsPathVisible())
	{
		SetStaticsPathVisible(false);
		SetStaticsPathVisible(true);
	}
}
// --------------------------------------------------------------------------
Time AStaticSceneEntity::GetLocationTime(unsigned int keyPointIndex) const
{
	const int			locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
	const ARole&		aRole = GetSceneEntityState().GetRole(locationRoleIndex);
	const LocationRole*	lRole = dynamic_cast<const LocationRole*>(&aRole);
	ASSERT(lRole);

	return lRole->GetTime(keyPointIndex);	
}
// --------------------------------------------------------------------------
Time AStaticSceneEntity::GetOrientationTime(unsigned int keyPointIndex) const
{
	const int				orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);
	const ARole&			aRole = GetSceneEntityState().GetRole(orientationRoleIndex);
	const OrientationRole*	oRole = dynamic_cast<const OrientationRole*>(&aRole);
	ASSERT(oRole);

	return oRole->GetTime(keyPointIndex);	
}
// --------------------------------------------------------------------------
Location AStaticSceneEntity::GetLocation(Time time) const
{
	const int			locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
	const ARole&		aRole = GetSceneEntityState().GetRole(locationRoleIndex);
	const LocationRole*	lRole = dynamic_cast<const LocationRole*>(&aRole);
	ASSERT(lRole);

	return lRole->GetState(time);	
}
// --------------------------------------------------------------------------
Orientation AStaticSceneEntity::GetOrientation(Time time) const
{
	const int				orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);
	const ARole&			aRole = GetSceneEntityState().GetRole(orientationRoleIndex);
	const OrientationRole*	oRole = dynamic_cast<const OrientationRole*>(&aRole);
	ASSERT(oRole);

	return oRole->GetState(time);	
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::SetLocation(const Location& location, Time time)
{
	const int		locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
	ARole&			aRole = GetSceneEntityState().GetRole(locationRoleIndex);
	LocationRole*	lRole = dynamic_cast<LocationRole*>(&aRole);
	ASSERT(lRole);

	const FlaggedLocation	fLocation(location, lRole->GetState(time).GetInterpolationType());

	lRole->StateUpdated(fLocation, time);

	GotoForLocationRole(GetCurrentTimePoint());
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::SetOrientation(const Orientation& orientation, Time time)
{
	const int			orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);
	ARole&				aRole = GetSceneEntityState().GetRole(orientationRoleIndex);
	OrientationRole*	oRole = dynamic_cast<OrientationRole*>(&aRole);
	ASSERT(oRole);

	const FlaggedOrientation	currentOrientation = oRole->GetState(time);

	oRole->StateUpdated(FlaggedOrientation(orientation, currentOrientation.GetInterpolationType(), currentOrientation.GetTargetEntity()), time);	

	GotoForOrientationRole(GetCurrentTimePoint());
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::RemoveLocation(Time time)
{
	const int	locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
	ARole&		aRole = GetSceneEntityState().GetRole(locationRoleIndex);
	ASSERT(dynamic_cast<LocationRole*>(&aRole));

	aRole.Remove(time);

	GotoForLocationRole(GetCurrentTimePoint());
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::RemoveOrientation(Time time)
{
	const int	orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);
	ARole&		aRole = GetSceneEntityState().GetRole(orientationRoleIndex);
	ASSERT(dynamic_cast<OrientationRole*>(&aRole));

	aRole.Remove(time);	

	GotoForOrientationRole(GetCurrentTimePoint());
}
// --------------------------------------------------------------------------
bool AStaticSceneEntity::IsAtKeyLocation() const
{
	const int		locationRoleIndex = GetLocationRoleIndex();
	ASSERT(locationRoleIndex >= 0);
	const ARole&	aRole = GetSceneEntityState().GetRole(locationRoleIndex);
	ASSERT(dynamic_cast<const LocationRole*>(&aRole));

	return aRole.HasTime(GetCurrentTimePoint());
}
// --------------------------------------------------------------------------
bool AStaticSceneEntity::IsAtKeyOrientation() const
{
	const int		orientationRoleIndex = GetOrientationRoleIndex();
	ASSERT(orientationRoleIndex >= 0);
	const ARole&	aRole = GetSceneEntityState().GetRole(orientationRoleIndex);
	ASSERT(dynamic_cast<const OrientationRole*>(&aRole));

	return aRole.HasTime(GetCurrentTimePoint());
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::RoleUpdated()
{
	BaseClass::RoleUpdated();

	UpdateMotionPathMarkers();
}
// --------------------------------------------------------------------------
void AStaticSceneEntity::UpdateForDeletedMarker(AMarker* marker)
{
	// Check if its an Orientation marker
	OrientationKeyPointMarker*	oMarker = dynamic_cast<OrientationKeyPointMarker*>(marker);

	if(oMarker)
	{
		// Update the orientation role for the deleted time point
		const unsigned int	index = oMarker->GetKeyPointIndex();
		
		const int			orientationRoleIndex = GetOrientationRoleIndex();
		ASSERT(orientationRoleIndex >= 0);
		ARole*				aRole = &GetSceneEntityState().GetRole(orientationRoleIndex);
		OrientationRole*	oRole = dynamic_cast<OrientationRole*>(aRole);
		ASSERT(oRole);

		oRole->Remove(index);

		GetSceneEntityState().RemoveSource(*oMarker);

		return;
	}
	

	// Check if its an Location marker
	LocationKeyPointMarker*	lMarker = dynamic_cast<LocationKeyPointMarker*>(marker);

	if(lMarker)
	{
		// Update the location role for the deleted time point
		const unsigned int	index = lMarker->GetKeyPointIndex();
		
		const int		locationRoleIndex = GetLocationRoleIndex();
		ASSERT(locationRoleIndex >= 0);
		ARole*			aRole = &GetSceneEntityState().GetRole(locationRoleIndex);
		LocationRole*	lRole = dynamic_cast<LocationRole*>(aRole);
		ASSERT(lRole);

		lRole->Remove(index);

		GetSceneEntityState().RemoveSource(*lMarker);

		return;
	}

	ASSERT(0); // Unknown marker type!	
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
