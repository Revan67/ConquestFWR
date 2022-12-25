// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef SceneView_h
#define SceneView_h

#include <afx.h>
#include "View.h"
#include "Links.h"
#include "Position.h"
// --------------------------------------------------------------------------
namespace ROS
{
class ADynamicCamera;
class SceneModel;
class SceneController;
}
class TGLSceneViewUIForm;
// --------------------------------------------------------------------------
//	SceneView
// --------------------------------------------------------------------------
class SceneView: public View
{
	public:
		SceneView(ROS::SceneModel& sceneModel, int screenX, int screenY, HWND parentWndH);
        /**# :[Description = "Calls SceneController::delete() and Model::Detach()"] */
        virtual ~SceneView();

        virtual void Update(int updateID);
		void UIClosed();

     private:
		typedef View BaseClass;
        typedef AggPointer<TGLSceneViewUIForm> TSceneViewUIFormPtr;

        TSceneViewUIFormPtr mSceneViewUI;
};
// --------------------------------------------------------------------------
#endif