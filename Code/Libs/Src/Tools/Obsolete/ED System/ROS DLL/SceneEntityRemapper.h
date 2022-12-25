// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SceneEntityRemapper_h
#define SceneEntityRemapper_h
// --------------------------------------------------------------------------
#include "StringType.h"
#include "Remapper.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ASceneEntity;
// --------------------------------------------------------------------------
// SceneEntityRemapper
// --------------------------------------------------------------------------
typedef Remapper<ROSString, ASceneEntity*> SceneEntityRemapper;
// --------------------------------------------------------------------------
// SceneEntityRemapClass
// --------------------------------------------------------------------------
template<class TRemapDescendantType>
class SceneEntityRemap: public TARemapClass<ASceneEntity*>
{
	public:
		SceneEntityRemap(TRemapDescendantType* instanceToRemap)
		: mInstanceToRemap(instanceToRemap)
		{
		}

	protected:
		typedef ASceneEntity* CPtr;

		virtual void Remap(const CPtr& value) const
		{
			ASSERT(dynamic_cast<const TRemapDescendantType>(value));

			*mInstanceToRemap = dynamic_cast<TRemapDescendantType>(value);
		}

	private:
		TRemapDescendantType*	mInstanceToRemap;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif