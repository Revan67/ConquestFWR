// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SceneEntityStateAccessor_h
#define SceneEntityStateAccessor_h
// --------------------------------------------------------------------------
#include "StringType.h"
#include "ROSDLL.h"
#include "ASceneEntityEventListener.h"
namespace ROS
{
// --------------------------------------------------------------------------
class ARole;
class SceneEntityState;
class ASceneEntity;
class DABaseCamera;
class IntersectInfo;
// --------------------------------------------------------------------------
//  SceneEntityStateAccessor
// --------------------------------------------------------------------------
class CPP_DECL SceneEntityStateAccessor
{
    public:
							SceneEntityStateAccessor(ASceneEntity& sceneEntity, SceneEntityState& state);

		ROSString			GetName() const;
		bool				IsVisible() const;
							
        void				SetName(const ROSString& name);
		void				SetVisible(bool isVisible);

		void				SetUserData(void* userData) const;
		void*				GetUserData() const;

		void				SetTrackId(long trackId);
		long				GetTrackId() const;

        const ARole&		GetRole(unsigned int roleIndex) const;
        ARole&				GetRole(unsigned int roleIndex);

		unsigned int		GetRoleCount() const;

		void				RoleUpdated();

        void				Goto(Time time);

        void				Draw(const ROS::DABaseCamera* camera) const;
		bool				Intersect(const IntersectInfo& intersectInfo, float* distance) const;

		ASceneEntity*		GetDependentEntity();
	
		void				AddListener(ASceneEntityEventListener& listener);
		void				RemoveListener(ASceneEntityEventListener& listener);

	private:
		ASceneEntity&		mSceneEntity;
		SceneEntityState&	mState;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
