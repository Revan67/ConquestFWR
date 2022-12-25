#ifndef  ASSET_BAR_H
#define  ASSET_BAR_H

#include "Sidebar.h"

class CAssetBar : public CSidebar
{
friend struct FindByObjectClass;

public:
	virtual ~CAssetBar();
	virtual int DoPaint( CPaintDC& );
	virtual bool Reset();

private:
	CTreeCtrl                   m_treeView;
	CImageList                  m_imageList;
	CArray<AssetData,AssetData> m_assetArray;

protected:

	void loadSystemKits();
	void loadObjects();
	void loadTriggers();
	void loadPlanets();
	void loadFields();
	void loadLights();
	void loadObstructions();
	void loadCameras();

	// Generated message map functions
protected:
	//{{AFX_MSG(CAssetBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif  // ASSET_BAR_H
