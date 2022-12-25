// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef Controller_h
#define Controller_h
// --------------------------------------------------------------------------
#include "AController.h"
#include "ASceneEntity.h"
// --------------------------------------------------------------------------
namespace ROS
{
class SceneModel;
class AOperation;
}
// --------------------------------------------------------------------------
//	Controller
// --------------------------------------------------------------------------
class Controller: public ROS::AController 
{
	public:
        Controller(ROS::SceneModel& sceneModel);
		virtual ~Controller();

        void GetSceneEntities(ROS::SceneEntityCollection& sceneEntityCollectionR) const;
        ROS::ASceneEntity* GetSelectedSceneEntity() const;
        void SetSelectedSceneEntity(ROS::ASceneEntity* sceneEntity);
		void LockSceneEntitySelection(bool lock);

		void SetSecondarySceneEntity(ROS::ASceneEntity* sceneEntityP);
		ROS::ASceneEntity* GetSecondarySceneEntity() const;
		void SecondarySceneEntityUpdated();

		void SceneUpdated();

        bool IsScenePresent() const;

        ROS::Time GetSceneDuration() const;
		void SetCurrentSceneTime(ROS::Time time);
        ROS::Time GetCurrentSceneTime() const;

		void AddUndoableOperation(ROS::AOperation* operation);
		void UndoLastOperation();
		void RedoLastOperation();

		virtual void Update(int updateID);

	private :
		typedef ROS::AController BaseClass;
			/**#: [Cardinalities = "1..1/"]*/
		AssPointer<ROS::SceneModel>	mSceneModelP;
};
// --------------------------------------------------------------------------
#endif
