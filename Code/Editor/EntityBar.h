#ifndef  ENTITY_BAR_H
#define  ENTITY_BAR_H

#include "Sidebar.h"
#include "EventSys.h"
#include "TComponent.h"
#include "treelist\MltiTree.h"

struct IObject;

class CEntityBar : public CSidebar
{
public:
	virtual ~CEntityBar();
	virtual int DoPaint( CPaintDC& );

	virtual bool Update();

private:
	CMultiTree     m_treeView;
	struct System* m_currentSystem;
	CImageList     m_imageList;
	bool           m_bNeedUpdate;

	friend struct CEntityBar_Notifier;

	DEFMETHOD(Notify) (U32 message, void * param);
	void onNotify( MSG* pMsg );

protected:
	void insertObject( HTREEITEM item, IObject* obj );
	void removeObject( HTREEITEM item );
	bool selectObject( HTREEITEM item, IObject* _object );
	void addImage( const char* _imagename, U32 _rgb );
	void updateSelectedList( HTREEITEM item );

	HTREEITEM findFolder( IObject* obj, U32& _iconImage );

	// Generated message map functions
protected:
	//{{AFX_MSG(CEntityBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};

#endif  // ENTITY_BAR_H

