// Author: Shaival Varma
// --------------------------------------------------------------------------
#include "PCH.h"
#include "SceneView.h"
#include "GLSceneViewUI.h"
#include "SceneModel.h"
#include "Utils.h"
#include "ASceneEntity.h"
#include "ADynamicCamera.h"
#include "GLSceneViewUI.h"
// --------------------------------------------------------------------------
/**# implementation SceneView:: id(C_0888350800)
*/
// --------------------------------------------------------------------------
SceneView::SceneView(ROS::SceneModel& sceneModel, int screenX, int screenY, HWND parentWndH)
:BaseClass(sceneModel)
{
	int	top = screenY;
	int left = screenX;

    if(top < 0)
    {	top = 0;
    }

    if(left < 0)
    {	left = 0;
    }

#if 0
    mSceneViewUI = TSceneViewUIFormPtr(new TSceneViewUIForm(NULL, this, top, left, 480, 640));
    mSceneViewUI->Show();
#else
    mSceneViewUI = TSceneViewUIFormPtr(new TGLSceneViewUIForm(*this, top, left, 480, 640, parentWndH));
#endif
}
// --------------------------------------------------------------------------
SceneView::~SceneView()
{
	if(IsNotNull(mSceneViewUI))
    {
		delete mSceneViewUI.reset(NULL);	// Important to set mSceneViewUI to NULL before deleting it. When ~TGLSceneViewUIForm
    }										// calls SceneView::UIClosed(), it won't invoke ~SceneView again.
}
// --------------------------------------------------------------------------
void SceneView::Update(int updateID)
{
	BaseClass::Update(updateID);
	
	if(updateID == ModelNS::kSceneModelReplaced)
	{
		mSceneViewUI->AssociateCamera(NULL);
	}
	else
	{
		if(updateID == ModelNS::kSelectedEntityRemoved)
		{
			const ROS::ASceneEntity* deletedEntity = GetSelectedSceneEntity();
			
			if(deletedEntity == mSceneViewUI->GetAssociatedCamera())
			{
				mSceneViewUI->AssociateCamera(NULL);
			}
		}

		if(updateID != ModelNS::kSceneSystemShutdown)
        {
			mSceneViewUI->AssociatedCameraUpdated();
            mSceneViewUI->Repaint();
        }
		else
		{
			delete this;
		}
    }
}
// --------------------------------------------------------------------------
void SceneView::UIClosed()
{
	if(IsNotNull(mSceneViewUI))
	{
		mSceneViewUI = TSceneViewUIFormPtr(NULL);

		delete this;
	}
}
// --------------------------------------------------------------------------

