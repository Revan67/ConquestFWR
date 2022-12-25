// Author: Shaival Varma
//---------------------------------------------------------------------------
#include "PCH.h"
#include "IKState.h"
#include "AStaticSceneEntity.h"
#include "ConstSceneEntityStateAccessor.h"
//---------------------------------------------------------------------------
namespace ROS
{
//---------------------------------------------------------------------------
void IKState::WriteSubObject(std::ostream& oStream) const
{
	ASSERT(mTargetEntity);

	OStreamWiz<FieldID>	oWiz(oStream);

	oWiz.Put(kEndEffectorName, mEndEffectorName);
	oWiz.Put(kCountToRoot, mCountToRootEffector);
	oWiz.Put(kTargetEntityName, mTargetEntity->GetConstSceneEntityStateAccessor()->GetName());
	oWiz.Put(kDampingFactor, mDampingFactor);

	unsigned int	axis;
	
	axis = mEndEffectorAxis;
	oWiz.Put(kEndEffectorAxis, axis);

	axis = mEndEffectorUpAxis;
	oWiz.Put(kEndEffectorUpAxis, axis);

	oWiz.Put(kPointAtFlag, mPointAt);
	oWiz.Put(kMoveToFlag, mMoveTo);
}
//---------------------------------------------------------------------------
void IKState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	iWiz.Get(kEndEffectorName, mEndEffectorName);
	iWiz.Get(kCountToRoot, mCountToRootEffector);

	ROSString	targetEntityName;

	iWiz.Get(kTargetEntityName, targetEntityName);

	SceneEntityRemapper::Add(targetEntityName, new SceneEntityRemap<const AStaticSceneEntity*>(&mTargetEntity));
	
	iWiz.Get(kDampingFactor, mDampingFactor);

	if(iWiz.Has(kEndEffectorAxis))
	{
		unsigned int	axis;

		iWiz.Get(kEndEffectorAxis, axis);

		mEndEffectorAxis = static_cast<Axis>(axis);
	}
	else
	{
		mEndEffectorAxis = kZAxis;
	}

	if(iWiz.Has(kEndEffectorUpAxis))
	{
		unsigned int	axis;

		iWiz.Get(kEndEffectorUpAxis, axis);

		mEndEffectorUpAxis = static_cast<Axis>(axis);
	}
	else
	{
		mEndEffectorUpAxis = kYAxis;
	}

	if (iWiz.Has(kPointAtFlag))
	{
		iWiz.Get(kPointAtFlag, mPointAt);
	}
	else
	{
		mPointAt = false;
	}

	if (iWiz.Has(kMoveToFlag))
	{
		iWiz.Get(kMoveToFlag, mMoveTo);
	}
	else
	{
		mMoveTo = true;
	}
}
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
