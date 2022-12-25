// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "OrientationKeyPointMarker.h"
#include "OrientationKeyPointStaticsState.h"
#include "GLUtils.h"
#include "SceneEntityStateAccessor.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN OrientationKeyPointMarker::OrientationKeyPointMarker(const ROSString& name, bool makeNameUnique, Scene& scene, AStaticSceneEntity& entity, unsigned int keyPointIndex)
:BaseClass(name, makeNameUnique, scene, *new OrientationKeyPointStaticsState(entity, keyPointIndex)), mBaseName(name)
{
	SetName();

	entity.GetSceneEntityStateAccessor()->AddListener(*this);	// So that this object gets informed when the entity is deleted
	GetSceneEntityState().AddSource(entity);
}
// --------------------------------------------------------------------------
CPP_DEFN OrientationKeyPointMarker::~OrientationKeyPointMarker()
{
	// Decrement the key point indices of the OrientationKeyPointMarker instances that follow
	Marker*	marker = GetNextMarker();

	while(marker != NULL)
	{
		OrientationKeyPointMarker*	oMarker = dynamic_cast<OrientationKeyPointMarker*>(marker);
		ASSERT(oMarker);

		oMarker->SetKeyPointIndex(oMarker->GetKeyPointIndex() - 1);

		marker = oMarker->GetNextMarker();
	}
}
// --------------------------------------------------------------------------
CPP_DEFN unsigned int OrientationKeyPointMarker::GetKeyPointIndex() const
{
	const APhysicalState*					pState = &GetPhysicalState();
	const OrientationKeyPointStaticsState*	oState = dynamic_cast<const OrientationKeyPointStaticsState*>(pState);
	ASSERT(oState);

	return oState->GetKeyPointIndex();
}
// --------------------------------------------------------------------------
CPP_DEFN void OrientationKeyPointMarker::SetKeyPointIndex(unsigned int keyPointIndex)
{
	APhysicalState*						pState = &GetPhysicalState();
	OrientationKeyPointStaticsState*	oState = dynamic_cast<OrientationKeyPointStaticsState*>(pState);
	ASSERT(oState);

	oState->SetKeyPointIndex(keyPointIndex);

	SetName();
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString OrientationKeyPointMarker::GetArchetypeName() const
{
	return GetOrientationKeyPointMarkerArchetypeName();
}
// --------------------------------------------------------------------------
CPP_DEFN ROSString OrientationKeyPointMarker::GetOrientationKeyPointMarkerArchetypeName()
{
	return "OrientationKeyPointMarker";
}
// --------------------------------------------------------------------------
CPP_DEFN void OrientationKeyPointMarker::Render(const ROS::DABaseCamera* camera) const
{
	GL::Draw3dAxes(1.0, 1.0);
}
// --------------------------------------------------------------------------
CPP_DEFN void OrientationKeyPointMarker::SetName()
{
	char	indexStr[100];

	GetSceneEntityState().SetName(mBaseName + ROSString(" Orientation ") + itoa(GetKeyPointIndex() + 1, indexStr, 10));
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
