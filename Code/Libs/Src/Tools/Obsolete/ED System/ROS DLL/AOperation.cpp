// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "AOperation.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN AOperation::AOperation(const ROSString& name, ASceneEntity* entity)
:mName(name), mEntity(entity)
{
}
// --------------------------------------------------------------------------
CPP_DEFN AOperation::~AOperation()
{
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString AOperation::GetName() const
{
	return mName;
}
// --------------------------------------------------------------------------
CPP_DEFN void AOperation::SetName(const ROSString& name)
{
	mName = name;
}
// --------------------------------------------------------------------------
CPP_DEFN const ASceneEntity* AOperation::GetEntity() const
{
	return mEntity;
}
// --------------------------------------------------------------------------
CPP_DEFN ASceneEntity* AOperation::GetEntity()
{
	return mEntity;
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
