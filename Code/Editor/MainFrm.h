// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__8B5F5321_8EB6_4BC4_B6F7_B46EF47088C2__INCLUDED_)
#define AFX_MAINFRM_H__8B5F5321_8EB6_4BC4_B6F7_B46EF47088C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SectorBar.h"
#include "SystemBar.h"
#include "AssetBar.h"
#include "EntityBar.h"
#include "OutputBar.h"
#include "GroupBar.h"

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
	void ResetBars();
	void UpdateBars();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL DestroyWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// control bar embedded members
protected:  
	CStatusBar m_wndStatusBar;
	CToolBar   m_wndToolBar;
	CImageList m_modeImageList;
	CToolBar   m_editorToolBar;
	CSectorBar m_sectorBar;
	CSystemBar m_systemBar;
	CAssetBar  m_assetBar;
	CEntityBar m_entityBar;
	COutputBar m_outputBar;
	CGroupBar  m_groupBar;
	U8         m_currentPlayerID;

	CArray<CSidebar*,CSidebar*> m_sidebars;

protected:
	void SetupBar( CSidebar& _bar, U32 _idsString, U32 _cbrsAlignFlags, U32 _afxIdwDockbarAlign, CRect& _rect, CSidebar* _nextTo );
	void UpdateStatusBar();
	void DockControlBarNextTo(CControlBar* pBar, CControlBar* pTargetBar);

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedo();
	afx_msg void OnEditCopy();
	afx_msg void OnModeCampaign();
	afx_msg void OnModeScenario();
	afx_msg void OnModeSector();
	afx_msg void OnModeStart();
	afx_msg void OnModeSystem();
	afx_msg void OnFileNew();
	afx_msg void OnNewScenario();
	afx_msg void OnScenarioOpen();
	afx_msg void OnFileSave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	virtual void ActivateFrame(int nCmdShow = -1);
	afx_msg void OnFileSavesenario();
	afx_msg void OnFileSaveasmission();
	afx_msg void OnPlayerPlayerone();
	afx_msg void OnPlayerPlayertwo();
	afx_msg void OnPlayerPlayerthree();
	afx_msg void OnPlayerPlayerfour();
	afx_msg void OnPlayerPlayerfive();
	afx_msg void OnPlayerPlayersix();
	afx_msg void OnPlayerPlayerseven();
	afx_msg void OnPlayerPlayereight();
	afx_msg void OnMenu();
	afx_msg void OnStringTable();
	afx_msg void OnObjectcommandDelete();
	afx_msg void OnObjectcommandRename();
	afx_msg void OnObjectcommandClone();
	afx_msg void OnObjectcommandProperties();
	afx_msg void OnObjectcommandWarp();
	afx_msg void OnEditPaste();
	afx_msg void OnEditCut();
	afx_msg void OnObjectcommandGoto();
	afx_msg void OnFileOpen();
	afx_msg void OnModeCampaignView();
	afx_msg void OnModeScenarioView();
	afx_msg void OnModeSectorView();
	afx_msg void OnModeSystemView();
protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
public:
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnFileExportstringtable();
	afx_msg void OnFileImportstringtable();
	afx_msg void OnGroupcommandAddto();
	afx_msg void OnObjectcommandSelect();
	afx_msg void OnMapLaunch();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__8B5F5321_8EB6_4BC4_B6F7_B46EF47088C2__INCLUDED_)
