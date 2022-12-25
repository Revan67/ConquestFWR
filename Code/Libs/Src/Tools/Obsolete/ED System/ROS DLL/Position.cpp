// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "Position.h"
// --------------------------------------------------------------------------
/**# implementation Position:: id(C_0886790533)
*/
// --------------------------------------------------------------------------
enum FieldID
{
	kOrientation,
	kLocation
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN const Location& Position::GetLocation () const
{
    return mLocation;
}
// --------------------------------------------------------------------------
CPP_DEFN const Orientation& Position::GetOrientation () const
{
    return mOrientation;
}
// --------------------------------------------------------------------------
CPP_DEFN void Position::SetLocation (const Location& kLocationR)
{
    mLocation = kLocationR;
}
// --------------------------------------------------------------------------
CPP_DEFN void Position::SetOrientation (const Orientation& kOrientationR)
{
    mOrientation = kOrientationR;
}
// --------------------------------------------------------------------------
CPP_DEFN void Position::Write(std::ostream& oStream) const
{
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void Position::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

    oWiz.Put(kOrientation, mOrientation);
    oWiz.Put(kLocation, mLocation);
}
// --------------------------------------------------------------------------
CPP_DEFN void Position::Read(std::istream& iStream)
{
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void Position::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

    iWiz.Get(kOrientation, mOrientation);
    iWiz.Get(kLocation, mLocation);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

