// SystemProps.cpp : implementation file
//

#include "stdafx.h"
#include "Editor.h"
#include "globals.h"

#include "SystemProps.h"
#include "DataList.h"
#include "StringEditor.h"
#include "StringTable.h"

#include <windowsx.h>

#include <EventSys.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//----------------------------------------------------------------------------------------------
// EnumSystemKits

struct EnumSystemKits : public IArchetypeEnum
{
	EnumSystemKits( SystemProps* _props ) : props(*_props) {}

	SystemProps& props;

	virtual	BOOL32 ArchetypeEnum (const char * name, void *data, U32 size, DWORD context)
	{
		// if the same size & it has no !! in the name, it should be a system kit

		if( sizeof(GT_SYSTEM_KIT) == size && !strstr(name,"!!") )
		{
			GT_SYSTEM_KIT* kit = (GT_SYSTEM_KIT*)data;

			CWnd* combo = props.GetDlgItem(IDC_COMBO_SYSKIT);

			int cbItem = ComboBox_AddString( combo->m_hWnd, name );
			ComboBox_SetItemData( combo->m_hWnd, cbItem, GENDATA->GetArchetypeDataID(name) );

			if( props.m_data.systemKitName == name )
			{
				ComboBox_SetCurSel( combo->m_hWnd, cbItem);
			}
		}

		return 1;
	}
};

//----------------------------------------------------------------------------------------------
// SystemProps dialog

IMPLEMENT_DYNAMIC(SystemProps, CDialog)
SystemProps::SystemProps(CWnd* pParent /*=NULL*/)
	: CDialog(SystemProps::IDD, pParent)
{
}

SystemProps::~SystemProps()
{
}

void SystemProps::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

void SystemProps::SetSystemData( System* _system )
{
	m_data.backgroundName = _system->backgroundName;
	m_data.bEmpty         = _system->bEmpty;
	m_data.systemKitName  = _system->systemKitName;
	m_data.name           = _system->name;
	m_data.nameID         = _system->nameID;
	m_data.id             = _system->id;
}

//----------------------------------------------------------------------------------------------
// SystemProps message handlers

BEGIN_MESSAGE_MAP(SystemProps, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_NEWSTRING, OnBnClickedButtonNewstring)
	ON_CBN_SELCHANGE(IDC_COMBO_SYSKIT, OnCbnSelchangeComboSyskit)
END_MESSAGE_MAP()

void SystemProps::OnBnClickedOk()
{
	// apply changes
	System* system = Editor::GetSystem( 0, m_data.id );
	if( system )
	{
		system->backgroundName	= m_data.backgroundName;
		system->systemKitName	= m_data.systemKitName;
		system->nameID			= m_data.nameID;

		// load localized name
		system->updateSystemName();
	}

	MSG msg;
	msg.hwnd	= m_hWnd;
	msg.lParam	= IDOK;
	msg.wParam	= m_data.id;
	msg.message = WM_DESTROY;

	if( EVENTSYS )
		EVENTSYS->Send( WM_DESTROY, &msg );
}

void SystemProps::OnBnClickedCancel()
{
	MSG msg;
	msg.hwnd	= m_hWnd;
	msg.lParam	= IDCANCEL;
	msg.wParam	= 0;
	msg.message = WM_DESTROY;

	if( EVENTSYS )
		EVENTSYS->Send( WM_DESTROY, &msg );
}

void SystemProps::OnBnClickedButtonNewstring()
{
	StringEditor stringChooser;
	stringChooser.SetSelectedString( m_data.nameID );

	if( stringChooser.DoModal() == IDOK )
	{
		if( stringChooser.GetSelectedString() != StringEditor::INVALID_STRING )
		{
			m_data.nameID = stringChooser.GetSelectedString();
			const wchar_t* name = STRINGTABLE->GetStringByID( m_data.nameID );

			CWnd* editName = GetDlgItem(IDC_NAMESTRING);
			if( editName && name )
			{
				CString widename(name);
				editName->SetWindowText( widename );
				m_data.name = widename;
			}
		}
	}
}

void SystemProps::OnCbnSelchangeComboSyskit()
{
	CComboBox* box = (CComboBox*)GetDlgItem(IDC_COMBO_SYSKIT);

	int sel = box->GetCurSel();

	if( sel != CB_ERR )
	{
		CString str;

		int n = box->GetLBTextLen( sel );
		box->GetLBText( sel, str.GetBuffer(n) );
		str.ReleaseBuffer();

		GT_SYSTEM_KIT* kit = (GT_SYSTEM_KIT*)GENDATA->GetArchetypeData(str);
		if( kit )
		{
			m_data.backgroundName = kit->fileName;
			m_data.systemKitName = str.GetBuffer(0);
		}
	}
}

BOOL SystemProps::OnInitDialog()
{
	CDialog::OnInitDialog();

	EnumSystemKits enumerateKits(this);
	GENDATA->EnumerateArchetypeData ( &enumerateKits );

	CWnd* editName = GetDlgItem(IDC_NAMESTRING);
	if( editName )
	{
		editName->SetWindowText( m_data.name );
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
