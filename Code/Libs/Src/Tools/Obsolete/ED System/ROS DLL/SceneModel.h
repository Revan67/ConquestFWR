// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SceneModel_h
#define SceneModel_h
// --------------------------------------------------------------------------
#include "RosModel.h"
#include "StringType.h"
#include "ASceneEntity.h"
#include "Scene.h"
#include "ROSDLL.h"
#include "Position.h"
#include "OperationStack.h"
#include "HardPoint.h"
// --------------------------------------------------------------------------
namespace ROS
{
// --------------------------------------------------------------------------
class ADynamicCamera;
class StringList;
// --------------------------------------------------------------------------
//	SceneModel
// --------------------------------------------------------------------------
class CPP_DECL SceneModel: public Model
{
	public:
		enum PerformanceStyle
		{
			kOnce,
			kRepeat,
			kLoop
		};

        typedef void (*Callback)(SceneModel& sceneModel, SceneEvent sceneEvent, const ROSString& sceneEntityName, const ROSString& entityCategoryName, const StringList& descriptionStrings, const void** entity, void** entityUserData, SceneEventFlag* flags);
 
        // --------------------------------------------------------------------------
        //	ExInvalidSceneFile
        // --------------------------------------------------------------------------
        class ExInvalidSceneFile: public std::exception
        {	public:
        		ExInvalidSceneFile()
				{
                };

                virtual const char* what() const throw()
                {
                  return "Invalid Scene file";
                }
        };

    	explicit SceneModel(Callback callback = NULL, void* userData = NULL);
        virtual	~SceneModel();

		void SetPerformanceStyle(PerformanceStyle style);
		PerformanceStyle GetPerformanceStyle() const;

		void SetUseInitialEntityState(bool use);
		bool IsUsingInitialEntityState() const;

		void SceneModel::Read(const ROSString& sceneFileName);

		ROSString GetSceneName() const;
        ROSString GetSceneFileName() const;

		void* GetUserData() const;

		void GetSceneEntities(SceneEntityCollection& sceneEntityCollectionR) const;
        void AddSceneEntity(ASceneEntity& sceneEntity);
        void AddCompoundSceneEntity(const ROSString& entityName, const ROS::ROSString& categoryName, const StringList& descriptionStrings);
        void AddDeformableSceneEntity(const ROSString& entityName, const ROS::ROSString& categoryName, const StringList& descriptionStrings);
        void AddNewPositionMarker();
        void AddNewCamera();
        void AddNewLiveCamera();
		void AddNewAmbientLight();
		void AddNewDirectionalLight();
		void AddNewPointLight();
		void AddNewSpotLight();
		void RemoveSceneEntity(const ASceneEntity& sceneEntityR);
        void RenameSceneEntity(const ROSString& kEntityToRenameR, const ROSString& kNewNameR);

		void SetSelectedSceneEntity(ASceneEntity* sceneEntityP);
		ASceneEntity* GetSelectedSceneEntity() const;

        void LockSceneEntitySelection(bool lock);
        bool IsSceneEntitySelectionLocked() const;

        void SelectedSceneEntityUpdated();

		void SetSecondarySceneEntity(ASceneEntity* sceneEntityP);
		void SecondarySceneEntityUpdated();
		void SecondaryDependentSceneEntityUpdated();
		ASceneEntity* GetSecondarySceneEntity() const;

		void SceneUpdated();

        void SetGameEngineUpdatingMode(bool updateGameEngine);

        Time GetSceneDuration() const;
        void SetSceneDuration(Time duration);
        Time GetCurrentSceneTime() const;
        void SetCurrentSceneTime(Time time);

        void PlayScene();
        void PauseScene();
        void StopScene();
        bool IsScenePlaying() const;
		void UpdateScene(Time timeDelta);

		void SaveScene(const ROSString& fileName, bool saveExtraData = false);

        void SwitchToNewSceneModel(SceneModel* sceneModel);
        SceneModel* GetNewSceneModel();

		void ShutdownSystem();

		void RememberPosition(const Position& position);
		Position RecallPosition() const;

		void RememberHardPoint(const HardPoint& hardPoint);
		HardPoint RecallHardPoint() const;

		void UndoLastOperation();
		void RedoLastOperation();
		void AddUndoableOperation(AOperation* operation);

		ROSString GetUndoOperationName() const;
		ROSString GetRedoOperationName() const;

	private:
		/**# :[Cardinalities = "1..1/"] */
        typedef AggPointer<Scene>	SceneSP;	// Can't use automatic destruction as this needs to be deleted in ~SceneModel() before the (rest of) the body of ~SceneModel()

		static void SceneCallback(const Scene& scene, SceneEvent sceneEvent, const ROSString& sceneEntityName, const ROSString& sceneEntityCategory, const StringList& descriptionStrings, const void** entity, void** entityUserData, SceneEventFlag* flags);
		
		void RemoveFromUndoRedoStacks(const ASceneEntity& entity);
		
		SceneSP 					mSceneSP;
		ROSString					mSceneFileName;
        AssPointer<ASceneEntity>	mSelectedSceneEntityP;
        AssPointer<ASceneEntity>	mSecondarySceneEntityP;
		bool						mSceneEntitySelectionLocked;
        SceneModel*					mNewSceneModel;
		Position					mRememberedPosition;
		HardPoint					mRememberedHardPoint;
		OperationStack				mUndoOperationStack;
		OperationStack				mRedoOperationStack;
};
// --------------------------------------------------------------------------
}
// --------------------------------------------------------------------------
#endif
