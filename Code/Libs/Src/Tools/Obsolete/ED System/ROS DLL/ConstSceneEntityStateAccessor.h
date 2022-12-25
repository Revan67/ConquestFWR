// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ConstSceneEntityStateAccessor_h
#define ConstSceneEntityStateAccessor_h
// --------------------------------------------------------------------------
#include "StringType.h"
#include "ROSDLL.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ARole;
class ASceneEntity;
class SceneEntityState;
class IntersectInfo;
class DABaseCamera;
class IntersectInfo;
// --------------------------------------------------------------------------
//  ConstSceneEntityStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL ConstSceneEntityStateAccessor
{
    public:
							ConstSceneEntityStateAccessor(const ASceneEntity& sceneEntity, const SceneEntityState& state);

		ROSString			GetName() const;
		bool				IsVisible() const;
							
		void				SetUserData(void* userData) const;
		void*				GetUserData() const;

        const ARole&		GetRole(unsigned int roleIndex) const;

		unsigned int		GetRoleCount() const;

		bool				FindIntersect(const IntersectInfo& intersectInfo, float* distance) const;

        void				Draw(const ROS::DABaseCamera* camera) const;
		bool				Intersect(const IntersectInfo& intersectInfo, float* distance) const;
										
	private:
		const ASceneEntity&		mSceneEntity;
		const SceneEntityState&	mState;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
