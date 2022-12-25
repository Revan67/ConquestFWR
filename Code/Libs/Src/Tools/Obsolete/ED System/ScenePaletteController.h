// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ScenePaletteController_h
#define ScenePaletteController_h
// --------------------------------------------------------------------------
#include <afx.h>
#include "AController.h"
#include "Links.h"
#include "Callback.hpp"
#include "StringType.h"
#include "ModelNS.h"
// --------------------------------------------------------------------------
class ScenePaletteView;
namespace ROS
{
class ADynamicCamera;
class ASceneEntity;
class SceneModel;
class StringList;
}
// --------------------------------------------------------------------------
//	ScenePaletteController
// --------------------------------------------------------------------------
class ScenePaletteController : public ROS::AController
{
	public:
    	typedef CBFunctor1<int> UpdateCB;

        /**# :[Description = "Calls Model::Detach()"] */
        ScenePaletteController(ROS::SceneModel& sceneModelR, UpdateCB& updateCallbackR);
        /**# :[Description = "Calls Model::Detach()"] */
        virtual ~ScenePaletteController();

		ROS::ROSString GetSceneName() const;
		ROS::ROSString GetSceneFileName() const;
		
		ROS::SceneModel& GetSceneModel();    /********NOTE: SHOULD NOT BE HANDING OUT SceneModel**********/

        bool IsScenePresent() const;
		void SaveScene(const ROS::ROSString& fileName, bool saveAndConvert = false) const;

        void PauseScene();

        void SetCurrentSceneTime(ROS::Time time);
        ROS::Time GetCurrentSceneTime() const;

        void GetSceneEntities(ROS::SceneEntityCollection& sceneEntityCollectionR) const;
        void AddSceneEntity(ROS::ASceneEntity& sceneEntity);
        void AddCompoundSceneEntity(const ROS::ROSString& entityName, const ROS::ROSString& categoryName, const ROS::StringList& descriptionStrings);
        void AddDeformableSceneEntity(const ROS::ROSString& entityName, const ROS::ROSString& categoryName, const ROS::StringList& descriptionStrings);
		void AddNewPositionMarker();
		void AddNewCamera();
		void AddNewLiveCamera();
		void AddNewAmbientLight();
		void AddNewDirectionalLight();
		void AddNewPointLight();
		void AddNewSpotLight();
		void RemoveSceneEntity(const ROS::ASceneEntity& sceneEntityR);
        void RenameSceneEntity(const ROS::ROSString& kEntityToRenameR, const ROS::ROSString& kNewNameR);

        ROS::ASceneEntity* GetSelectedSceneEntity() const;
        void SetSelectedSceneEntity(ROS::ASceneEntity* sceneEntityP);
        ROS::ASceneEntity* GetSecondarySceneEntity() const;
        void LockSceneEntitySelection(bool lock);
        bool IsSceneEntitySelectionLocked() const;

		virtual void HandleEvent(const ROS::Event& kEventR);
		void SelectedSceneEntityUpdated();

		void UndoLastOperation();
		void RedoLastOperation();

		ROS::ROSString GetUndoOperationName() const;
		ROS::ROSString GetRedoOperationName() const;

		void AddSceneView(int screenX, int screenY, HWND parentWndH);

        virtual void Update(int updateID);

		void SceneUpdated();

        void ReplaceSceneModel(ROS::SceneModel* sceneModel);

		void ShutdownSystem();

    private :
//		void PerformUpdateCallback(ModelNS::UpdateID updateID);

        /**#: [Cardinalities = "1..1/"]*/
        AssPointer<ROS::SceneModel>	mSceneModelSP;
        UpdateCB		    		mUpdateCB;
};
// --------------------------------------------------------------------------
#endif
