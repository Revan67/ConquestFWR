#ifndef SceneEntityControls_h
#define SceneEntityControls_h
//---------------------------------------------------------------------------
#include <afxwin.h>

#include "Callback.hpp"
//---------------------------------------------------------------------------
namespace ROS
{
class ASceneEntity;
}
//---------------------------------------------------------------------------
class TSceneEntityForm : public CDialog
{
    public:		// User declarations
    	typedef CBFunctor0	CallbackOnChange;

	    TSceneEntityForm(UINT templateID, CWnd* parent, const CallbackOnChange& callback, ROS::ASceneEntity* sceneEntity);
		virtual ~TSceneEntityForm();

        void SetSceneEntity(ROS::ASceneEntity* sceneEntity);

		virtual void UpdateForm() = 0;

    protected:
		// Generated message map functions
		//{{AFX_MSG(TSceneEntityForm)
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()

	protected:

    	void NotifyEntityStateChanged() const;
        ROS::ASceneEntity*	GetSceneEntity();
    private:
    	CallbackOnChange	mCallback;
        ROS::ASceneEntity*	mSceneEntity;
};
//---------------------------------------------------------------------------
#endif

