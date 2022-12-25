// Author: Shaival Varma
// --------------------------------------------------------------------------
// SelectDBEntityDialog.cpp : implementation file
//

#include "PCH.h"
#include "da_vector"
#include "stdafx.h"
#include "SelectDBEntityDialog.h"
#include "DBExtension.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// --------------------------------------------------------------------------
CSelectDBEntityDialog::EntityType GetEntityType(DBE::EntityType entityType)
{
	switch(entityType)
	{
		case DBE::kCompound:
			return CSelectDBEntityDialog::kCompound;
			break;
		case DBE::kDeformable:
			return CSelectDBEntityDialog::kDeformable;
			break;
		case DBE::kAudio:
			return CSelectDBEntityDialog::kAudio;
			break;
		case DBE::kEvent:
			return CSelectDBEntityDialog::kEvent;
			break;
		default:
			return CSelectDBEntityDialog::kUnknown;
	}
}
/////////////////////////////////////////////////////////////////////////////
// CSelectDBEntityDialog dialog

CSelectDBEntityDialog::CSelectDBEntityDialog(CWnd* pParent /*=NULL*/)
: CDialog(CSelectDBEntityDialog::IDD, pParent), mEntityType(kUnknown)
{
	//{{AFX_DATA_INIT(CSelectDBEntityDialog)
	//}}AFX_DATA_INIT

	EntityTypeList	entities;
	
	entities.push_back(kDeformable);
	entities.push_back(kCompound);
	entities.push_back(kAudio);
	entities.push_back(kEvent);

	SetEntityTypes(entities);
}

CSelectDBEntityDialog::~CSelectDBEntityDialog()
{
	RemoveAllRadios();
}

void CSelectDBEntityDialog::SetEntityTypes(const EntityTypeList& entityTypes)
{
	mEntityTypes = entityTypes;
}

ROS::ROSString CSelectDBEntityDialog::GetEntityName() const
{	
	return mEntityName;
}

ROS::ROSString CSelectDBEntityDialog::GetCategoryName() const
{	
	return mCategoryName;
}

CSelectDBEntityDialog::EntityType CSelectDBEntityDialog::GetEntityType() const
{
	return mEntityType;
}

ROS::StringList CSelectDBEntityDialog::GetEntityDescriptionStrings() const
{
	return mEntityStrings;
}

void CSelectDBEntityDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectDBEntityDialog)
	DDX_Control(pDX, IDC_ENTITIES_STATIC, mEntitiesGroupBox);
	DDX_Control(pDX, IDC_ENTITY_LISTBOX, mEntityListbox);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectDBEntityDialog, CDialog)
	//{{AFX_MSG_MAP(CSelectDBEntityDialog)
	ON_WM_CREATE()
	ON_LBN_DBLCLK(IDC_ENTITY_LISTBOX, OnDblclkEntityListbox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectDBEntityDialog message handlers

int CSelectDBEntityDialog::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}

BOOL CSelectDBEntityDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Create radios for all the categories

	// Get the number of categories
	const U32	catCount = DBE::get_category_count();

	// Create radios
	for(U32 catIdx = 0; catIdx < catCount; ++catIdx)
	{
		// Add a radio only if the type of the category is desired
		const EntityType	entityType = ::GetEntityType(DBE::get_category_type(catIdx));
		bool				addRadio = false;

		for(unsigned int typeIdx = 0; typeIdx < mEntityTypes.size(); ++typeIdx)
		{
			if(entityType == mEntityTypes[typeIdx])
			{
				addRadio = true;
				break;
			}
		}
		
		if(addRadio)
		{
			AddRadio(DBE::get_category_name(catIdx), catIdx);
		}
	}

	// Adjust the dialog, group box and list according to the radios.
	CRect	lastRadioRect, rect;
	
	const CButton*	lastRadio = mRadios.back();

	lastRadio->GetWindowRect(&lastRadioRect);
	mEntitiesGroupBox.GetWindowRect(&rect);

	const int	heightAdjustment = lastRadioRect.bottom + 10 - rect.bottom;

	if(heightAdjustment > 0)
	{
		// Increase height of the dialog, group box and list.
		CRect	dlgRect;

		// dialog
		GetWindowRect(&dlgRect);
		dlgRect.bottom += heightAdjustment;
		MoveWindow(&dlgRect);

		// group box and list
		CPoint	clientOrigin(0, 0);

		ClientToScreen(&clientOrigin);

		// group box
		rect.OffsetRect(-clientOrigin);
		rect.bottom += heightAdjustment;
		mEntitiesGroupBox.MoveWindow(&rect);

		// list
		mEntityListbox.GetWindowRect(&rect);
		rect.OffsetRect(-clientOrigin);
		rect.bottom += heightAdjustment;
		mEntityListbox.MoveWindow(&rect);
	}

	// Fill in the entity list
	if(!mRadios.empty())
	{
		CButton*	firstRadio = mRadios.front();

		firstRadio->SetCheck(1);

		const unsigned int	categoryIdx = GetWindowLong(firstRadio->m_hWnd, GWL_USERDATA);

		UpdateList(categoryIdx);
	}
	else
	{
		mEntityListbox.EnableWindow(FALSE);
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectDBEntityDialog::OnOK() 
{
	const int	selectionIdx = mEntityListbox.GetCurSel();
	
	if(selectionIdx >= 0)
	{
		// Get information about selection
		DWORD	idx = mEntityListbox.GetItemData(selectionIdx);

		ASSERT(idx >= 0);

		// Get the entity's type
		const U32				category = DBE::get_entity_category(idx);
		const DBE::EntityType	entityType = DBE::get_category_type(category);

		mEntityType = ::GetEntityType(entityType);
		
		// Get the entity's name
		CString	entityName;
		
		mEntityListbox.GetText(selectionIdx, entityName);

		const char*	charEntityName = entityName;

		mEntityName = charEntityName;

		// Get the entity's name description strings
		const U32	stringCount = DBE::get_entity_string_count(idx);

		if(stringCount != 0)
		{
			mEntityStrings.Resize(stringCount);
			
			DBE::get_entity_strings(idx, mEntityStrings);
		}
	}

	CDialog::OnOK();
}

void CSelectDBEntityDialog::OnCancel() 
{
	CDialog::OnCancel();
}


void CSelectDBEntityDialog::OnDblclkEntityListbox() 
{
	OnOK();	
}

void CSelectDBEntityDialog::AddRadio(const ROS::ROSString& radioName, unsigned int categoryIndex)
{
	// set up style
	DWORD	style = WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON;

	if(mRadios.empty())
	{
		style |= WS_GROUP;
	}

	// compute position
	CPoint	clientOrigin(0, 0);

	ClientToScreen(&clientOrigin);

	CRect dlgRect, groupBoxRect, listBoxRect;

	GetWindowRect(&dlgRect);
	mEntitiesGroupBox.GetWindowRect(&groupBoxRect);
	mEntityListbox.GetWindowRect(&listBoxRect);

	const unsigned int	x = groupBoxRect.left - dlgRect.left + 10;
	const unsigned int	y = listBoxRect.top - clientOrigin.y + (mRadios.size() * 20);

	CRect rect(x, y, x + 100, y + 10);
	
	CButton*	radio = new CButton;

	radio->Create(radioName.c_str(), style, rect, this, 5);	// The control id is irrelevant since we will be matching by handle
	radio->SetFont(GetFont());

	SetWindowLong(radio->m_hWnd, GWL_USERDATA, categoryIndex);

	mRadios.push_back(radio);
}

void CSelectDBEntityDialog::RemoveAllRadios()
{
	RadioCollection::iterator		begin = mRadios.begin();
	const RadioCollection::iterator	end = mRadios.end();
	
	while(begin != end)
	{
		delete *begin;

		++begin;
	}
}

BOOL CSelectDBEntityDialog::OnCommand(WPARAM wParam, LPARAM lParam) 
{	
	RadioCollection::iterator		begin = mRadios.begin();
	const RadioCollection::iterator	end = mRadios.end();
	
	while(begin != end)
	{
		if((*begin)->m_hWnd == reinterpret_cast<HWND>(lParam))
		{
			const unsigned int	categoryIdx = GetWindowLong((*begin)->m_hWnd, GWL_USERDATA);

			UpdateList(categoryIdx);

			return 1;
		}

		++begin;
	}

	return CDialog::OnCommand(wParam, lParam);
}

void CSelectDBEntityDialog::UpdateList(unsigned int categoryIndex)
{
	mEntityListbox.ResetContent();

	// Get the number of entities
	const U32	entityCount = DBE::get_entity_count();

	// Get all the entity names for the specified category
	for(U32	entityIdx = 0; entityIdx < entityCount; ++entityIdx)
	{
		if(DBE::get_entity_category(entityIdx) == categoryIndex)
		{
			const int	insertionIdx = mEntityListbox.AddString(DBE::get_entity_name(entityIdx).c_str());
			mEntityListbox.SetItemData(insertionIdx, entityIdx);
		}
	}

	if(mEntityListbox.GetCount() > 0)
	{
		mEntityListbox.EnableWindow(TRUE);
		mEntityListbox.SetCurSel(0);
	}
	else
	{
		mEntityListbox.EnableWindow(FALSE);
	}

	mCategoryName = DBE::get_category_name(categoryIndex);
}