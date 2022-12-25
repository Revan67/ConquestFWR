// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Scene_h
#define Scene_h

#include <exception>

#include "Environment.h"
#include "SceneState.h"
#include "SceneEntityList.h"
#include "Links.h"
#include "TimeType.h"
#include "TimeTag.h"
#include "ROSDLL.h"
#include "SceneEvent.h"
#include "EntityDescription.h"
#include "SceneEntityRemapper.h"
// --------------------------------------------------------------------------
namespace ROS
{
class State;
class ADynamicCamera;
class Screenplay;
class DACompoundObject;
class DADeformableObject;
class DAAudioObject;
class StringList;
class AStaticSceneEntity;
// --------------------------------------------------------------------------
//	Scene
// --------------------------------------------------------------------------
 /**# :[Note = "The Scene class uses the SceneEntityList class."] */
class CPP_DECL Scene
{
	public:
		typedef void (*Callback)(const Scene& scene, SceneEvent sceneEvent, const ROSString& sceneEntityName, const ROSString& categoryName, const StringList& descriptionStrings, const void** entity, void** entityUserData, SceneEventFlag* flags);

	   	enum StreamType
        {
			kSceneData,
        	kSceneList
        };

		enum EntityType
		{
			kCompound,
			kDeformable,
			kAudio,
			kEvent
		};

		enum PerformanceStyle
		{
			kOnce,
			kRepeat,
			kLoop
		};

		Scene(Callback callback, void* userData);

    	~Scene();

		void SetPerformanceStyle(PerformanceStyle style);
		PerformanceStyle GetPerformanceStyle() const;

        const ROSString& GetName() const;

		void* GetUserData() const;

        void AddSceneEntity(ASceneEntity& sceneEntityR);
        void RemoveSceneEntity(const ASceneEntity& sceneEntityR);
        void RenameSceneEntity(const ROSString& kEntityToRenameR, const ROSString& kNewNameR);
		void GetSceneEntities(SceneEntityCollection& sceneEntityCollectionR) const;

        void SetGameEngineUpdatingMode(bool updateGameEngine);
        bool IsUpdatingGameEngine() const;

		void SetUseInitialEntityState(bool use);
		bool IsUsingInitialEntityState() const;

		void Perform(Time startTime);
        void Update(Time timeDelta);
        void Stop();
        void Pause();
        void Resume();
        bool IsPerforming() const;
		bool IsFirstIteration() const;

        Time GetDuration() const;
        void SetDuration(Time duration);
        Time GetCurrentTimePoint() const;
        void SetCurrentTimePoint(Time time);

		const DACompoundObject* CreateDACompoundObject(const ROSString& sceneEntityName, const ROSString& categoryName, const StringList& descriptionStrings, void** entityData, SceneEventFlag* flags) const;
		void DestroyDACompoundObject(const DACompoundObject* dACompoundObj, const ROSString& sceneEntityName, const ROSString& categoryName, const StringList& descriptionStrings, void** entityData) const;
		const DADeformableObject* CreateDADeformableObject(const ROSString& sceneEntityName, const ROSString& categoryName, const StringList& descriptionStrings, void** entityData, SceneEventFlag* flags) const;
		void DestroyDADeformableObject(const DADeformableObject* dADeformableObj, const ROSString& sceneEntityName, const ROSString& categoryName, const StringList& descriptionStrings, void** entityData) const;
		const DABaseCamera* CreateDABaseCamera(const ROSString& sceneEntityName, void** entityData, SceneEventFlag* flags) const;
		void DestroyDABaseCamera(const DABaseCamera* dABaseCamera, const ROSString& sceneEntityName, void** entityData) const;
		const DABaseCamera* CreateInternalDABaseCamera(const ROSString& sceneEntityName, void** entityData, SceneEventFlag* flags) const;
		void DestroyInternalDABaseCamera(const DABaseCamera* dABaseCamera, const ROSString& sceneEntityName, void** entityData) const;
		const DAAudioObject* CreateDAAudioObject(const ROSString& soundName, Time startTime, const AStaticSceneEntity* staticSE) const;
		void DestroyDAAudioObject(const DAAudioObject* audioObject) const;
		void PlayDAAudioObject(const DAAudioObject* audioObject, Time startTime) const;
		void StopDAAudioObject(const DAAudioObject* audioObject) const;

		void UpdateDescriptionStrings(const ROSString& entityName, EntityType entityType, ROSString& categoryName, StringList& descriptionStrings) const;

        void Write(std::ostream& oStreamR) const;
        void Read(std::istream& iStreamR, StreamType streamType);
        // --------------------------------------------------------------------------
        //	ExInvalidSceneStream
        // --------------------------------------------------------------------------
        class ExInvalidSceneStream: public std::exception
        {
			public:
        		ExInvalidSceneStream()
				{
                };

                virtual const char* what() const throw()
                {
                  return "Invalid Scene stream";
                }
        };

        class ExInvalidSceneEntityInStream: public std::exception
        {
			public:
				ExInvalidSceneEntityInStream(const ROSString& entityTypeName)
				: mMessage(ROSString("Unrecognized Scene Entity Type: ") + entityTypeName)
				{
				}
					
				virtual const char* what() const throw()
				{
				  return mMessage.c_str();
				}

			private:
				ROSString	mMessage;
        };

        class ExSceneCreationFailed: public std::exception
        {
			public:
        		ExSceneCreationFailed()
				{
                };

                virtual const char* what() const throw()
                {
                  return "Scene creation failed";
                }
        };

	private:
        typedef SceneEntityList 	EntityList;

		void WriteSubObject(std::ostream& ostreamR) const;
		void ReadSubObject(std::istream& iStreamR, StreamType streamType);

        void Goto(Time currentTime);
		void UpdateTimeTagDataDuration();
        ADynamicCamera* FindFirstCamera();
		bool HasLiveCameras() const;

		void RemapAllSceneEntityReferences();

        ROSString mName;
        /**# :[Cardinalities = "1..1/"] */
        AggPointer<Environment> mEnvironment;
        /**# :[Cardinalities = "1..1/"] */
        AggPointer<SceneState> mInitialSceneState;
        /**# :[Cardinalities = "1..1/"] */
        AggPointer<SceneState> mCurrentSceneState;
        /**# :[Cardinalities = "1..1/"] */
        AggAPointer<EntityList> mSceneEntityList;
        /**# :[Cardinalities = "1..1/"] */
		AggPointer<Screenplay>	mScreenplay;

//        const ROS::DAChannel*   mDAChannelP;

        Time                    mDuration;
        TimeTagList				mTimeTags;
        bool					mIsPerforming;
        Time					mCurrentTime;
        mutable void*			mUserData;
        Callback                mCallback;
        bool                    mUpdateGameEngine;
		bool					mUseInitialState;
		bool					mHasLiveCamera;
		PerformanceStyle		mPerformanceStyle;
		bool					mIsFirstIteration;
		SceneEntityRemapper		mSceneEntityRemapper;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
