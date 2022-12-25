//---------------------------------------------------------------------------
#include "PCH.h"
#include "SceneEntityControls.h"
//---------------------------------------------------------------------------
TSceneEntityForm::TSceneEntityForm(UINT templateID, CWnd* parent, const CallbackOnChange& callback, ROS::ASceneEntity* sceneEntity)
:CDialog(templateID, parent), mCallback(callback), mSceneEntity(sceneEntity)
{
}
//---------------------------------------------------------------------------
TSceneEntityForm::~TSceneEntityForm()
{
	DestroyWindow();
}
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(TSceneEntityForm, CDialog)
	//{{AFX_MSG_MAP(TSceneEntityForm)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
//---------------------------------------------------------------------------
void TSceneEntityForm::NotifyEntityStateChanged() const
{
	if(mCallback)
    {	mCallback();
    }
}
//---------------------------------------------------------------------------
ROS::ASceneEntity* TSceneEntityForm::GetSceneEntity()
{
	return mSceneEntity;
}
//---------------------------------------------------------------------------
void TSceneEntityForm::SetSceneEntity(ROS::ASceneEntity* sceneEntity)
{
	if(mSceneEntity != sceneEntity)
	{	mSceneEntity = sceneEntity;
		UpdateForm();
	}
}
//---------------------------------------------------------------------------

