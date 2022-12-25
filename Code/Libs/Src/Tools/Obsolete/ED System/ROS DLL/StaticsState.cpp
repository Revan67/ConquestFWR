// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "StaticsState.h"
// --------------------------------------------------------------------------
/**# implementation StaticsState:: id(C_0887645182)
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
CPP_DEFN StaticsState::StaticsState()
{
}
// --------------------------------------------------------------------------
CPP_DEFN StaticsState::StaticsState(const Position& kPositionR)
: mPosition(kPositionR)
{
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsState::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);

    WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID> oWiz(oStream);

	oWiz.Put(kPosition, mPosition);
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsState::Read(std::istream& iStream)
{
    BaseClass::Read(iStream);

    ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID> iWiz(iStream);

	iWiz.Get(kPosition, mPosition);
}
// --------------------------------------------------------------------------
CPP_DEFN Position StaticsState::GetPosition () const
{
    return mPosition;
}
// --------------------------------------------------------------------------
CPP_DEFN void StaticsState::SetPosition (const Position& kPositionR)
{
    mPosition = kPositionR;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

