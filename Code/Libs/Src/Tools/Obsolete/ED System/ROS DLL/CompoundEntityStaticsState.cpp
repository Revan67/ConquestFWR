// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "CompoundEntityStaticsState.h"
#include "Utils.h"
#include "DACompoundObject.h"
// --------------------------------------------------------------------------
/**# implementation CompoundEntityStaticsState:: id(C_0892667255)
*/
// --------------------------------------------------------------------------
enum FieldID
{
	kPosition
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CompoundEntityStaticsState::CompoundEntityStaticsState(const DACompoundObject* dACompoundObject)
: mDACompoundObject(dACompoundObject)
{
}
// --------------------------------------------------------------------------
CompoundEntityStaticsState::~CompoundEntityStaticsState()
{
	ASSERT(IsNull(mDACompoundObject));
}
// --------------------------------------------------------------------------
void CompoundEntityStaticsState::SetDACompoundObject(const DACompoundObject* dACompoundObject)
{
	mDACompoundObject = dACompoundObject;
}
// --------------------------------------------------------------------------
const DACompoundObject* CompoundEntityStaticsState::GetDACompoundObject() const
{
	return mDACompoundObject;
}
// --------------------------------------------------------------------------
Position CompoundEntityStaticsState::GetPosition() const
{
    ASSERT(mDACompoundObject);

	Vector		location;
	Orientation orientation;

	CompoundObjectGetPosition(mDACompoundObject, location);
	CompoundObjectGetOrientation(mDACompoundObject, orientation);

	return Position(Location(location), orientation);
}
// --------------------------------------------------------------------------
void CompoundEntityStaticsState::SetPosition(const Position& position)
{
    ASSERT(mDACompoundObject);
    
	CompoundObjectSetPosition(mDACompoundObject, position.GetLocation().GetVector());
    CompoundObjectSetOrientation(mDACompoundObject, position.GetOrientation());
}
// --------------------------------------------------------------------------
void CompoundEntityStaticsState::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

    ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void CompoundEntityStaticsState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	Position	position;
	
	iWiz.Get(kPosition, position);

    // Had to read it back since we wrote it, but don't want to set to the state
	// SetPosition(position);
}
// --------------------------------------------------------------------------
void CompoundEntityStaticsState::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);

    WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void CompoundEntityStaticsState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	Position	position = GetPosition();

    oWiz.Put(kPosition, position);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

