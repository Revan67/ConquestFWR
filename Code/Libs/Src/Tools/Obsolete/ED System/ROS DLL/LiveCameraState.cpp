// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "LiveCameraState.h"
#include "ADynamicCamera.h"
#include "SceneEntityRemapper.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kName
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
CPP_DEFN LiveCameraState::LiveCameraState()
:mRollingCamera(NULL)
{
}
// --------------------------------------------------------------------------
CPP_DEFN LiveCameraState::LiveCameraState(ADynamicCamera* rollingCamera)
:mRollingCamera(rollingCamera)
{
}
// --------------------------------------------------------------------------
CPP_DEFN void LiveCameraState::SetRollingCamera(ADynamicCamera* rollingCamera)
{
	mRollingCamera = rollingCamera;
}
// --------------------------------------------------------------------------
CPP_DEFN const ADynamicCamera* LiveCameraState::GetRollingCamera() const
{
	return mRollingCamera;
}
// --------------------------------------------------------------------------
CPP_DEFN ADynamicCamera* LiveCameraState::GetRollingCamera()
{
	return mRollingCamera;
}
// --------------------------------------------------------------------------
CPP_DEFN void LiveCameraState::Write(std::ostream& oStream) const
{
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void LiveCameraState::Read(std::istream& iStream)
{
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
CPP_DEFN void LiveCameraState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	oWiz.Put(kName, mRollingCamera->GetSceneEntityStateAccessor()->GetName());
}
// --------------------------------------------------------------------------
CPP_DEFN void LiveCameraState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	ROSString	name;

	iWiz.Get(kName, name);

	SceneEntityRemapper::Add(name, new SceneEntityRemap<ADynamicCamera*>(&mRollingCamera));
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
