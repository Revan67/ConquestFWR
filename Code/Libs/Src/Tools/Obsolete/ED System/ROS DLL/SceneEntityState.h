// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SceneEntityState_h
#define SceneEntityState_h
// --------------------------------------------------------------------------
#include <vector>

#include "SequenceGenerator.h"
#include "ARole.h"
#include "Utils.h"
#include "Links.h"
#include "ASceneEntityEventListener.h"
#include "ASceneEntityEventSource.h"
#include "EventListenerTracker.h"
#include "EventSourceTracker.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class Scene;
// --------------------------------------------------------------------------
//  ASceneEntityState
// --------------------------------------------------------------------------
class SceneEntityState
{
    public:
		SceneEntityState(ASceneEntity& ownerEntity, const ROSString& name, bool makeNameUnique);
		SceneEntityState(ASceneEntity& ownerEntity, std::istream& iStream);
		
		~SceneEntityState();

        ROSString GetName() const;
        void SetName(const ROSString& name);

		bool IsVisible() const;
		void SetVisible(bool isVisible);
							
        unsigned int GetRoleCount() const;
        unsigned int AddRole(ARole& role);
        const ARole& GetRole(unsigned int roleIndex) const;
        ARole& GetRole(unsigned int roleIndex);

		void SetUserData(void* userData) const;
		void* GetUserData() const;

 		void SetTrackId(long trackId);
		long GetTrackId() const;

       ASceneEntity* GetParentEntity();
        const ASceneEntity* GetParentEntity() const;

        void SetScene(Scene* scene);
        Scene& GetScene();
        const Scene& GetScene() const;

		void SetStaticsPathVisible(bool visible);
		bool IsStaticsPathVisible() const;

		void AddListener(ASceneEntityEventListener& listener);
		void RemoveListener(ASceneEntityEventListener& listener);
		void RemoveAllListeners();
		void FireToListeners(const SceneEntityEvent& event);
		unsigned int GetListenerCount() const;
		ASceneEntityEventListener& GetListener(unsigned int listenerIndex) const;

		void AddSource(ASceneEntityEventSource& source);
		void RemoveSource(ASceneEntityEventSource& source);
		unsigned int GetSourceCount() const;
		ASceneEntityEventSource& GetSource(unsigned int sourceIndex) const;
		void RemoveFromAllSources(ASceneEntityEventListener& listener);
		
		void Write(std::ostream& oStream) const;
        void Read(std::istream& iStream);

	private:
		typedef std::vector<ARole*> RoleList;
		typedef EventListenerTracker<ASceneEntityEventListener, SceneEntityEvent> SceneEntityEventListenerTracker;
		typedef EventSourceTracker<ASceneEntityEventSource, ASceneEntityEventListener> SceneEntityEventSourceTracker;

        void WriteSubObject(std::ostream& oStream) const;
        void ReadSubObject(std::istream& iStream);

        ROSString								mName;
        bool									mIsVisible;
        /**#: [Cardinalities = "0..1/"]*/
		/********** NOTE: Store the parent's identifier instead of storing a pointer.**********/
        AssPointer<ASceneEntity>				mOwnerEntity;
        ASceneEntity*							mParentEntity;
        /**#: [Cardinalities = "1..1/"]*/
		mutable void*							mUserData;
		long									mTrackId;
        AssPointer<Scene>						mScene;
		RoleList								mRoles;
		bool									mIsStaticsPathVisible;
		SceneEntityEventListenerTracker			mEventListenerTracker;
		SceneEntityEventSourceTracker			mEventSourceTracker;

        static SequenceGenerator<unsigned int>	mNameSeqGen;
};
// --------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& oStream, const ROS::SceneEntityState& state)
{
	state.Write(oStream);

	return oStream;
}
//---------------------------------------------------------------------------
inline std::istream& operator>>(std::istream& iStream, ROS::SceneEntityState& state)
{
	state.Read(iStream);

	return iStream;
}
// --------------------------------------------------------------------------
#endif