// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "DynamicSceneEntityState.h"
#include "CodeMsg.h"
// --------------------------------------------------------------------------
enum FieldID
{
	kStaticsState,
	kDynamicsState
};
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
DynamicSceneEntityState::DynamicSceneEntityState()
{
}
// --------------------------------------------------------------------------
void DynamicSceneEntityState::Write(std::ostream& oStream) const
{
	WriteSubObject(oStream);
}
// --------------------------------------------------------------------------
void DynamicSceneEntityState::Read(std::istream& iStream)
{
	ReadSubObject(iStream);
}
// --------------------------------------------------------------------------
void DynamicSceneEntityState::WriteSubObject(std::ostream& oStream) const
{
	OStreamWiz<FieldID>	oWiz(oStream);

	oWiz.Put(kStaticsState, mStaticsState);
	oWiz.Put(kDynamicsState, mDynamicsState);
}
// --------------------------------------------------------------------------
void DynamicSceneEntityState::ReadSubObject(std::istream& iStream)
{
	IStreamWiz<FieldID>	iWiz(iStream);

	iWiz.Get(kStaticsState, mStaticsState);
	iWiz.Get(kDynamicsState, mDynamicsState);
}
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
