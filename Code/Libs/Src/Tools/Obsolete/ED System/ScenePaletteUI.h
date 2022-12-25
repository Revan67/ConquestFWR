// ScenePaletteUI.h : header file
//
//---------------------------------------------------------------------------
#include "Links.h"
#include "ScenePaletteController.h"
#include "SceneEntityControls.h"
#include "SceneEntityFormFactory.h"
#include "Type.h"
#include "AnimationToolsUI.h"
#include "UserPreferences.h"
#include "DlgBars.h"
#include "SceneEvent.h"
//---------------------------------------------------------------------------
#if !defined(AFX_EDDLG_H__2CB106AB_4341_11D2_823D_0000F4A24556__INCLUDED_)
#define AFX_EDDLG_H__2CB106AB_4341_11D2_823D_0000F4A24556__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
//---------------------------------------------------------------------------
namespace ROS
{
class DACompoundObject;
class DADeformableObject;
class StringList;
}

struct ISystemContainer;
struct IEngine;
//---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////
// TScenePaletteUIForm dialog

class TScenePaletteUIForm : public CDialog
{
	// Construction
	public:
		TScenePaletteUIForm(CWnd* pParent, ISystemContainer* system, IEngine* engine);	// standard constructor
		~TScenePaletteUIForm();

        void SceneEntityStateChanged();

	// Dialog Data
		//{{AFX_DATA(TScenePaletteUIForm)
	enum { IDD = IDD_SCENE_PALETTE_UI_DIALOG };
	CButton	mEntitiesGroupBoxP;
		CButton	mRenderSelectionCheckBox;
		CButton	mPropertiesGroupBoxP;
		CButton	mPropertiesButtonP;
		CEdit	mCurrentSceneEditP;
		CListBox	mEntityListP;
		CButton	mLockSelectionCheckBox;
		CButton	mActorsRadioP;
		CButton	mDeformablesRadioP;
		CButton	mCompoundsRadioP;
		CButton	mArticlesRadioP;
		CButton	mLightsRadioP;
		CButton	mCamerasRadioP;
		CButton	mAllRadioP;
	//}}AFX_DATA

		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(TScenePaletteUIForm)
		protected:
		virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
		//}}AFX_VIRTUAL

	// Implementation
	protected:
		virtual void OnCancel( );
		virtual void OnOK( );

		HICON		mIcon;
		CDlgToolBar	mToolBar;

		// Generated message map functions
		//{{AFX_MSG(TScenePaletteUIForm)
		virtual BOOL OnInitDialog();
		afx_msg BOOL OnToolTipTextNotify(UINT id, NMHDR * pTTTStruct, LRESULT * pResult);
		afx_msg void OnPaint();
		afx_msg HCURSOR OnQueryDragIcon();
		afx_msg void OnNewSceneButton();
		afx_msg void SceneBrowseBitBtnClick();
		afx_msg void OnSaveSceneButton();
		afx_msg void OnSaveAndConvertSceneButton();
		afx_msg void OnClickAllRadio();
		afx_msg void OnClickActorsRadio();
		afx_msg void OnClickArticlesRadio();
		afx_msg void OnClickCamerasRadio();
		afx_msg void OnClickCompoundsRadio();
		afx_msg void OnClickDeformablesRadio();
		afx_msg void OnClickLightsRadio();
		afx_msg void AddSceneObjectBitBtnClick();
		afx_msg void OnAddPositionMarkerButton();
		afx_msg void OnAddCameraButton();
		afx_msg void OnAddLiveCameraButton();
		afx_msg void OnAddAmbientLightButton();
		afx_msg void OnAddDirectionalLightButton();
		afx_msg void OnAddPointLightButton();
		afx_msg void OnAddSpotLightButton();
		afx_msg void OnUndo();
		afx_msg void OnRedo();
		afx_msg void OnWriteEntityPositionsFile();
		afx_msg void OnClickEntityList();
		afx_msg void OnClickPropertiesButton();
		afx_msg void OnLockSelectionCheckBoxClick();
		afx_msg void OnCreateViewButton();
		afx_msg void OnTrackViewButton();
		afx_msg void OnEditEntitiesButton();
		afx_msg void OnEditViewsButton();
		afx_msg void OnExitButton();
		afx_msg void OnRenderSelectionCheck();
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
		afx_msg void OnDeleteClick();
		afx_msg void OnReplaceClick();
	afx_msg void OnMove(int x, int y);
	//}}AFX_MSG
		afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);
		DECLARE_MESSAGE_MAP()
	private:
        typedef AggAPointer<TSceneEntityForm>   SceneEntitySPtr;

		virtual void UpdateGUI(int updateID);

        void __fastcall LoadScene(const CString& kFileNameR);

        void __fastcall UpdateEntityList(const AType<ROS::ASceneEntity>& kTypeR);
        void __fastcall UpdatePropertyEditor();
        void DisplayCustomControls(bool display);
        
		ROS::ASceneEntity* __fastcall GetSelectedSceneEntity() const;
        void __fastcall SetSelectedSceneEntity(ROS::ASceneEntity* sceneEntityP);
        
		ROS::ASceneEntity* __fastcall GetSecondarySceneEntity() const;

		void CreateView();

		ASceneEntityFormFactory<TSceneEntityForm>* GetCompatibleFormType(const ROS::ASceneEntity* kSceneEntityP);
#ifdef PORTED
        TDragObject* GetCompatibleDragObject(ROS::ASceneEntity* sceneEntityP);
        TDragObject* GetCompatibleCreationalDragObject(ROS::ASceneEntity* sceneEntityP);
#endif

        void SetSelectionInEntityList(const ROS::ASceneEntity* sceneEntity);
		void AttemptEntityListSelection(int itemIdx);

		const ROS::DACompoundObject* CreateCompoundObject(const ROS::ROSString& sceneEntityName, const ROS::StringList& descriptionStrings) const;
		const ROS::DADeformableObject* CreateDeformableObject(const ROS::ROSString& sceneEntityName, const ROS::StringList& descriptionStrings) const;
		void DestroyDeformableObject(const ROS::DADeformableObject* entity, const ROS::StringList& descriptionStrings) const;
		const ROS::DAAudioObject* CreateAudioObject(const ROS::ROSString& soundName, const ROS::AStaticSceneEntity* sourceEntity) const;
		void PlayAudioObject(const ROS::DAAudioObject* audioObj, float startTimePoint) const;

		static void SceneCallback(ROS::SceneModel& sceneModel, ROS::SceneEvent sceneEvent, const ROS::ROSString& sceneEntityName, const ROS::ROSString& entityCategory, const ROS::StringList& descriptionStrings, const void** entity, void** entityUserData, ROS::SceneEventFlag* flags);

		ISystemContainer*					mSystem;
		IEngine*							mEngine;
		UserPreferences						mUserPreferences;
        bool								mPropertiesAreExpanded;
		AggAPointer<ScenePaletteController> mScenePaletteControllerSP;
        AggPointer<TAnimationToolsUIForm>	mAnimToolsUIFormSP;	// No need for an automatic destruction (auto_ptr) since child windows are automatically destructed by MFC
        SceneEntitySPtr						mPropertyControlsFormSP;
        CMenu								mPopupMenus;
	    AssPointer<TTrackViewUIForm>		mTrackView;			// No need for an automatic destruction (auto_ptr) since child windows are automatically destructed by MFC
		bool                                mDialogInitialized;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDDLG_H__2CB106AB_4341_11D2_823D_0000F4A24556__INCLUDED_)
