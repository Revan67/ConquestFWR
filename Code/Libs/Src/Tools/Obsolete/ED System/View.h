// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef View_h
#define View_h

#include "AView.h"
#include "Links.h"
#include "TimeType.h"
#include "HardPoint.h"
// --------------------------------------------------------------------------
namespace ROS
{
class ASceneEntity;
class SceneModel;
class Position;
class AOperation;
}
// --------------------------------------------------------------------------
//	SceneView
// --------------------------------------------------------------------------
class View: public ROS::AView
{
	public:
		View(ROS::SceneModel& sceneModelR);
        virtual ~View();

        void GetSceneEntities(ROS::SceneEntityCollection& sceneEntityCollectionR) const;

		void SetSelectedSceneEntity(ROS::ASceneEntity* sceneEntityP);
		ROS::ASceneEntity* GetSelectedSceneEntity();
        void LockSceneEntitySelection(bool lock);
        bool IsSceneEntitySelectionLocked() const;
        void SelectedSceneEntityUpdated();

		void SetSecondarySceneEntity(ROS::ASceneEntity* sceneEntityP);
		void SecondarySceneEntityUpdated();

		void SceneUpdated();

		void RememberPosition(const ROS::Position& position);
		ROS::Position RecallPosition() const;

		void RememberHardPoint(const ROS::HardPoint& hardPoint);
		ROS::HardPoint RecallHardPoint() const;

		ROS::Time GetCurrentSceneTime() const;

        virtual void Update(int updateID);
		
		void AddUndoableOperation(ROS::AOperation* operation);
		void UndoLastOperation();
		void RedoLastOperation();

	private :
        /**#: [Cardinalities = "1..1/"]*/
        AssPointer<ROS::SceneModel>	mSceneModel;
};
// --------------------------------------------------------------------------
#endif