// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "DeformableEntityStaticsState.h"
#include "DeformableSceneEntity.h"
#include "Utils.h"
#include "DADeformableObject.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kPosition
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
DeformableEntityStaticsState::DeformableEntityStaticsState(const DADeformableObject* dADeformableObject)
: mDADeformableObject(dADeformableObject)
{
}
// --------------------------------------------------------------------------
DeformableEntityStaticsState::~DeformableEntityStaticsState()
{
	ASSERT(mDADeformableObject == NULL);
}
// --------------------------------------------------------------------------
void DeformableEntityStaticsState::SetDADeformableObject(const DADeformableObject* dADeformableObject)
{
	mDADeformableObject = dADeformableObject;
}
// --------------------------------------------------------------------------
const DADeformableObject* DeformableEntityStaticsState::GetDADeformableObject() const
{
	return mDADeformableObject;
}
// --------------------------------------------------------------------------
Position DeformableEntityStaticsState::GetPosition() const
{
	ASSERT(mDADeformableObject);

    Vector		location;
    Orientation	orientation;

    DeformableObjectGetPosition(mDADeformableObject, location);
	DeformableObjectGetOrientation(mDADeformableObject, orientation);

    return Position(Location(location), orientation);
}
// --------------------------------------------------------------------------
void DeformableEntityStaticsState::SetPosition(const Position& position)
{
	ASSERT(mDADeformableObject);

    DeformableObjectSetPosition(mDADeformableObject, position.GetLocation().GetVector());
    DeformableObjectSetOrientation(mDADeformableObject, position.GetOrientation());
}
// --------------------------------------------------------------------------
void DeformableEntityStaticsState::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

    ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void DeformableEntityStaticsState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
	
	Position	position;

	iWiz.Get(kPosition, position);

	// Had to read it back since we wrote it, but don't want to set to the state
	// SetPosition(position);
}
// --------------------------------------------------------------------------
void DeformableEntityStaticsState::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);

    WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void DeformableEntityStaticsState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	const Position	position = GetPosition();
	
	oWiz.Put(kPosition, position);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

