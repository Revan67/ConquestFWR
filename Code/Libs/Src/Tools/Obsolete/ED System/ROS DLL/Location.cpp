// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "Location.h"

/**# implementation Location:: id(C_0886790563)
*/
// --------------------------------------------------------------------------
enum FieldID
{
	kX,
	kY,
	kZ
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN void Location::Write(std::ostream& oStream) const
{
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void Location::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

    float x = GetX();
    float y = GetY();
    float z = GetZ();

    oWiz.Put(kX, x);
    oWiz.Put(kY, y);
    oWiz.Put(kZ, z);
}
// --------------------------------------------------------------------------
CPP_DEFN void Location::Read(std::istream& iStream)
{
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void Location::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

    float x, y, z;

    iWiz.Get(kX, x);
    iWiz.Get(kY, y);
    iWiz.Get(kZ, z);

    SetX(x);
    SetY(y);
    SetZ(z);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
