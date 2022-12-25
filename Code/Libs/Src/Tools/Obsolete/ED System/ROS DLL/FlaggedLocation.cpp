// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "FlaggedLocation.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kInterpolationType
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN void FlaggedLocation::Write(std::ostream& oStream) const
{
	BaseClass::Write(oStream);

	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void FlaggedLocation::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

    int type = GetInterpolationType();

    oWiz.Put(kInterpolationType, type);
}
// --------------------------------------------------------------------------
CPP_DEFN void FlaggedLocation::Read(std::istream& iStream)
{
	BaseClass::Read(iStream);

	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void FlaggedLocation::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

    int	type;

    iWiz.Get(kInterpolationType, type);

	SetInterpolationType((InterpolationType)type);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
