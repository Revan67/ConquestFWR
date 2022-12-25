// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef AnimationToolsController_h
#define AnimationToolsController_h
// --------------------------------------------------------------------------
#include "AController.h"
#include "Links.h"
#include "Timer.h"
#include "ASceneEntity.h"
#include "Callback.hpp"
// --------------------------------------------------------------------------
namespace ROS
{
class SceneModel;
}
// --------------------------------------------------------------------------
//  AnimationToolsController
// --------------------------------------------------------------------------
class AnimationToolsController: public ROS::AController
{
    public:
    	typedef CBFunctor1<int> UpdateCB;

        AnimationToolsController(ROS::SceneModel& sceneModel, UpdateCB& updateCB);
        virtual ~AnimationToolsController();

        virtual void Update(int updateID);

        ROS::ROSString	GetSceneFileName() const;

        void GetSceneEntities(ROS::SceneEntityCollection& sceneEntityCollectionR) const;
        ROS::ASceneEntity* GetSelectedSceneEntity() const;

        void PlayScene();
        void PauseScene();
		bool IsScenePlaying() const;

		void UpdateScene();

		void SaveScene(const ROS::ROSString& fileName) const;

        ROS::Time GetSceneDuration() const;
        void SetSceneDuration(ROS::Time duration);

    private:
        /**#: [Cardinalities = "1..1/"]*/
        AssPointer<ROS::SceneModel>	mSceneModelSP;
		UpdateCB					mUpdateCB;
	    Timer						mTimer;
};
// --------------------------------------------------------------------------
#endif
