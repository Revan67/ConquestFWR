// Author: Shaival Varma
// --------------------------------------------------------------------------
#if !defined(AFX_SELECTDBENTITYDIALOG_H__F5C5A5A1_6065_11D2_968C_CF5E24341049__INCLUDED_)
#define AFX_SELECTDBENTITYDIALOG_H__F5C5A5A1_6065_11D2_968C_CF5E24341049__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SelectDBEntityDialog.h : header file
//
#include "StringList.h"
#include "resource.h"
/////////////////////////////////////////////////////////////////////////////
// CSelectDBEntityDialog dialog

class CSelectDBEntityDialog : public CDialog
{
	// Construction
	public:
		enum EntityType
		{
			kUnknown,
			kDeformable,
			kCompound,
			kAudio,
			kEvent
		};

		typedef da_std::vector<EntityType>	EntityTypeList;

		CSelectDBEntityDialog(CWnd* pParent);   // standard constructor
		virtual ~CSelectDBEntityDialog();

		void SetEntityTypes(const EntityTypeList& entityTypes);

		ROS::ROSString GetEntityName() const;
		ROS::ROSString GetCategoryName() const;
		EntityType GetEntityType() const;
		ROS::StringList GetEntityDescriptionStrings() const;

	// Dialog Data
		//{{AFX_DATA(CSelectDBEntityDialog)
		enum { IDD = IDD_SELECT_DB_ENTITY_DIALOG };
		CButton	mEntitiesGroupBox;
		CListBox	mEntityListbox;
	//}}AFX_DATA


	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CSelectDBEntityDialog)
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

	// Implementation
	protected:

		// Generated message map functions
		//{{AFX_MSG(CSelectDBEntityDialog)
		virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDblclkEntityListbox();
	//}}AFX_MSG
		DECLARE_MESSAGE_MAP()

	private:
		typedef	std::list<CButton*> RadioCollection;

		void AddRadio(const ROS::ROSString& radioName, unsigned int categoryIdx);
		void RemoveAllRadios();
		void UpdateList(unsigned int categoryIndex);

		EntityTypeList	mEntityTypes;	// Entity types to show
		ROS::ROSString	mEntityName;
		EntityType		mEntityType;
		ROS::ROSString	mCategoryName;
		ROS::StringList	mEntityStrings;
		RadioCollection	mRadios;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTDBENTITYDIALOG_H__F5C5A5A1_6065_11D2_968C_CF5E24341049__INCLUDED_)
