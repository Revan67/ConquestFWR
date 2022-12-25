// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "LocationKeyPointStaticsState.h"
#include "AStaticSceneEntity.h"
#include "ConstStaticsStateAccessor.h"
#include "StaticsStateAccessor.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kLocation
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN LocationKeyPointStaticsState::LocationKeyPointStaticsState(AStaticSceneEntity& entity, unsigned int keyPointIndex)
:mStaticSceneEntity(entity), mKeyPointIndex(keyPointIndex)
{
}
// --------------------------------------------------------------------------
CPP_DEFN const AStaticSceneEntity& LocationKeyPointStaticsState::GetSceneEntity() const
{
	return mStaticSceneEntity;
}
// --------------------------------------------------------------------------
CPP_DEFN AStaticSceneEntity& LocationKeyPointStaticsState::GetSceneEntity()
{
	return mStaticSceneEntity;
}
// --------------------------------------------------------------------------
CPP_DEFN unsigned int LocationKeyPointStaticsState::GetKeyPointIndex() const
{
	return mKeyPointIndex;
}
// --------------------------------------------------------------------------
CPP_DEFN void LocationKeyPointStaticsState::SetKeyPointIndex(unsigned int keyPointIndex)
{
	mKeyPointIndex = keyPointIndex;
}
// --------------------------------------------------------------------------
CPP_DEFN LocationKeyPointStaticsState::InterpolationType LocationKeyPointStaticsState::GetInterpolationType() const
{
	std::auto_ptr<ConstStaticsStateAccessor>			access = mStaticSceneEntity.GetConstStaticsStateAccessor();
	const ConstStaticsStateAccessor::InterpolationType	type = access->GetLocationInterpolationType(access->GetLocationTime(mKeyPointIndex));

	switch(type)
	{
		case ConstStaticsStateAccessor::kLinearFixed:
			return kLinearFixed;
		case ConstStaticsStateAccessor::kSplineFixed:
			return kSplineFixed;
		case ConstStaticsStateAccessor::kLinearBlend:
			return kLinearBlend;
		case ConstStaticsStateAccessor::kSplineBlend:
			return kSplineBlend;
		default:
			ASSERT(0);	// Unknown case!
			return kLinearFixed;	// Just to keep the compiler happy
	}
}
// --------------------------------------------------------------------------
CPP_DEFN void LocationKeyPointStaticsState::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);

    WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void LocationKeyPointStaticsState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	std::auto_ptr<ConstStaticsStateAccessor>	access = mStaticSceneEntity.GetConstStaticsStateAccessor();
	Location	location = access->GetLocation(access->GetLocationTime(mKeyPointIndex));
    
	oWiz.Put(kLocation, location);
}
// --------------------------------------------------------------------------
CPP_DEFN void LocationKeyPointStaticsState::Read(std::istream& iStream)
{
    BaseClass::Read(iStream);

    ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void LocationKeyPointStaticsState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);
	
	Location	location;
	
	iWiz.Get(kLocation, location);
	
	std::auto_ptr<StaticsStateAccessor>	access = mStaticSceneEntity.GetStaticsStateAccessor();
    
    access->SetLocation(location, access->GetLocationTime(mKeyPointIndex));
}
// --------------------------------------------------------------------------
CPP_DEFN Position LocationKeyPointStaticsState::GetPosition() const
{
	std::auto_ptr<ConstStaticsStateAccessor>	access = mStaticSceneEntity.GetConstStaticsStateAccessor();

	Location	location = access->GetLocation(access->GetLocationTime(mKeyPointIndex));
    
	return Position(location, Orientation());
}
// --------------------------------------------------------------------------
CPP_DEFN void LocationKeyPointStaticsState::SetPosition(const Position& position)
{
    std::auto_ptr<StaticsStateAccessor>	access = mStaticSceneEntity.GetStaticsStateAccessor();
	access->SetLocation(position.GetLocation(), access->GetLocationTime(mKeyPointIndex));
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

