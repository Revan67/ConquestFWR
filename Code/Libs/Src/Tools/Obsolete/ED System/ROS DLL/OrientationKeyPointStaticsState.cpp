// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "OrientationKeyPointStaticsState.h"
#include "AStaticSceneEntity.h"
#include "ConstStaticsStateAccessor.h"
#include "StaticsStateAccessor.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kOrientation
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN OrientationKeyPointStaticsState::OrientationKeyPointStaticsState(AStaticSceneEntity& entity, unsigned int keyPointIndex)
:mStaticSceneEntity(entity), mKeyPointIndex(keyPointIndex)
{
}
// --------------------------------------------------------------------------
CPP_DEFN const AStaticSceneEntity& OrientationKeyPointStaticsState::GetSceneEntity() const
{
	return mStaticSceneEntity;
}
// --------------------------------------------------------------------------
CPP_DEFN AStaticSceneEntity& OrientationKeyPointStaticsState::GetSceneEntity()
{
	return mStaticSceneEntity;
}
// --------------------------------------------------------------------------
CPP_DEFN unsigned int OrientationKeyPointStaticsState::GetKeyPointIndex() const
{
	return mKeyPointIndex;
}
// --------------------------------------------------------------------------
CPP_DEFN void OrientationKeyPointStaticsState::SetKeyPointIndex(unsigned int keyPointIndex)
{
	mKeyPointIndex = keyPointIndex;
}
// --------------------------------------------------------------------------
CPP_DEFN void OrientationKeyPointStaticsState::Write(std::ostream& oStream) const
{
    BaseClass::Write(oStream);

    WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void OrientationKeyPointStaticsState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	std::auto_ptr<ConstStaticsStateAccessor>	access = mStaticSceneEntity.GetConstStaticsStateAccessor();
	
	Orientation	orientation = access->GetOrientation(access->GetOrientationTime(mKeyPointIndex));
    
	oWiz.Put(kOrientation, orientation);
}
// --------------------------------------------------------------------------
CPP_DEFN void OrientationKeyPointStaticsState::Read(std::istream& iStream)
{
    BaseClass::Read(iStream);

    ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void OrientationKeyPointStaticsState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	std::auto_ptr<StaticsStateAccessor>	access = mStaticSceneEntity.GetStaticsStateAccessor();
		
	Orientation	orientation;
	
	iWiz.Get(kOrientation, orientation);

    access->SetOrientation(orientation, access->GetOrientationTime(mKeyPointIndex));
}
// --------------------------------------------------------------------------
CPP_DEFN Position OrientationKeyPointStaticsState::GetPosition() const
{
	std::auto_ptr<ConstStaticsStateAccessor>	access = mStaticSceneEntity.GetConstStaticsStateAccessor();
	
	const Time			time = access->GetOrientationTime(mKeyPointIndex);
	const Location		location = access->GetLocation(time);
	const Orientation	orientation = access->GetOrientation(time);
    
	return Position(location, orientation);
}
// --------------------------------------------------------------------------
CPP_DEFN void OrientationKeyPointStaticsState::SetPosition(const Position& position)
{
	std::auto_ptr<StaticsStateAccessor>	access = mStaticSceneEntity.GetStaticsStateAccessor();

    access->SetOrientation(position.GetOrientation(), access->GetOrientationTime(mKeyPointIndex));
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------

