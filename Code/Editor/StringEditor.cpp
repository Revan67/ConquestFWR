//
// StringEditor.cpp
//

#include "stdafx.h"
#include "globals.h"

#include "Editor.h"
#include "StringEditor.h"
#include "StringTable.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// string table enum

struct StringEnum : public IStringEnum
{
	CListCtrl* listCtrl;
	DWORD stringLowestID;
	DWORD selectedStringID;

	virtual void EnumStringInfo( IStringEnum::StringInfo& _stringInfo )
	{
		// TODO: need to find a way insert Unicode into a list control box
		CString outText(_stringInfo.wideString);

		int newItemID = listCtrl->GetItemCount();

		int itemID = listCtrl->InsertItem( LVIF_TEXT|LVIF_STATE, newItemID, _stringInfo.tagString, 0, 0, 0, 0);
		listCtrl->SetItemText(itemID, 1, outText);
		listCtrl->SetItemData(itemID, _stringInfo.idString);

		stringLowestID = __min( stringLowestID, _stringInfo.idString );

		if( selectedStringID == _stringInfo.idString )
		{
			listCtrl->SetHotItem( itemID );
		}
	}
};

/////////////////////////////////////////////////////////////////////////////
// MFC macro implemetations

IMPLEMENT_DYNAMIC(StringEditor, CDialog);

BEGIN_MESSAGE_MAP(StringEditor, CDialog)
	//{{AFX_MSG_MAP(StringEditor)
		ON_MESSAGE(WM_INITDIALOG, InitDialog)
		ON_BN_CLICKED(IDOK, OnBnClickedOk)
		ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
		ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedButtonDelete)
		ON_BN_CLICKED(IDC_BUTTON_EDIT, OnBnClickedButtonEdit)
		ON_BN_CLICKED(IDC_BUTTON_NEW, OnBnClickedButtonNew)
		ON_WM_SIZE()
		ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// implementation

StringEditor::StringEditor(CWnd* pParent) : CDialog(StringEditor::IDD), m_selectedStringID(INVALID_STRING)
{
}

//-----------------------------------------------------------------------------------------------------

StringEditor::~StringEditor()
{
}

//-----------------------------------------------------------------------------------------------------

void StringEditor::initStrings()
{
	if( STRINGTABLE == NULL || STRINGTABLE->GetLanguage() == 0 )
	{
		return;
	}

	CString stringID;
	stringID.LoadString( IDS_ED_STRINGID );

	CString stringValue;
	stringValue.LoadString( IDS_ED_STRINGVALUE );

	CListCtrl* listCtrl = (CListCtrl*)GetDlgItem(IDC_STRING_LIST);
	listCtrl->InsertColumn( 0, stringID, LVCFMT_LEFT, 200 );
	listCtrl->InsertColumn( 1, stringValue, LVCFMT_LEFT, 700 );

	ListView_SetExtendedListViewStyle(listCtrl->m_hWnd, LVS_EX_FULLROWSELECT );

	StringEnum strenum;
	strenum.listCtrl = listCtrl;
	strenum.stringLowestID = ms_stringLowestID;
	strenum.selectedStringID = m_selectedStringID;

	STRINGTABLE->EnumerateStringInfo( strenum );
}

//-----------------------------------------------------------------------------------------------------

void StringEditor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(StringEditor)
	//}}AFX_DATA_MAP
}

//-----------------------------------------------------------------------------------------------------

void StringEditor::OnBnClickedOk()
{
	// committing changes to string table

	LANGID langid = STRINGTABLE->GetLanguage();

	CListCtrl* listCtrl = (CListCtrl*)GetDlgItem(IDC_STRING_LIST);

	// update string table
	for( int item = 0; item < listCtrl->GetItemCount(); item++ )
	{
		CString tag = listCtrl->GetItemText( item, 0 );
		CString txt = listCtrl->GetItemText( item, 1 );
		DWORD   sid = listCtrl->GetItemData( item );

		if( sid )
		{
			// TODO: localize this!
			wchar_t* wideString = (wchar_t*)alloca( sizeof(wchar_t) * txt.GetLength() + 1 );
			::MultiByteToWideChar( CP_ACP, 0, txt, txt.GetLength(), wideString, txt.GetLength() );
			wideString[txt.GetLength()] = 0;

			STRINGTABLE->SetString( sid, wideString, tag, langid );
		}
		else
		{
			// TODO: localize this!
			wchar_t* wideString = (wchar_t*)alloca( sizeof(wchar_t) * txt.GetLength() + 1 );
			::MultiByteToWideChar( CP_ACP, 0, txt, txt.GetLength(), wideString, txt.GetLength() );
			wideString[txt.GetLength()] = 0;

			U32 newStringID = STRINGTABLE->NewString( wideString, tag, langid );
			listCtrl->SetItemData( item, newStringID );
		}
	}

	// record the selected string
	int selectedStringItem = listCtrl->GetSelectionMark();
	if( selectedStringItem != -1 )
	{
		m_selectedStringID = listCtrl->GetItemData( selectedStringItem );
	}

	OnOK();
}

//-----------------------------------------------------------------------------------------------------

void StringEditor::OnBnClickedCancel()
{
	OnCancel();
}

//-----------------------------------------------------------------------------------------------------

void StringEditor::OnSize(UINT nType, int cx, int cy)
{
	CWnd* buttonOk	   = GetDlgItem(IDOK);
	CWnd* buttonCancel = GetDlgItem(IDCANCEL);
	CWnd* stringList   = GetDlgItem(IDC_STRING_LIST);
	CWnd* buttonDelete = GetDlgItem(IDC_BUTTON_DELETE);
	CWnd* buttonEdit   = GetDlgItem(IDC_BUTTON_EDIT);
	CWnd* buttonNew	   = GetDlgItem(IDC_BUTTON_NEW);

	double rMainX = (double)cx / (double)ms_mainRect.Width();
	double rMainY = (double)cy / (double)ms_mainRect.Height();

	if( buttonOk )
	{
		double x = ms_buttonOk.left * rMainX;
		double y = ms_buttonOk.top  * rMainY;
		buttonOk->SetWindowPos( NULL, (int)x, (int)y, ms_buttonOk.Width(), ms_buttonOk.Height(), 0 );
	}

	if( buttonCancel )
	{
		double x = ms_buttonCancel.left * rMainX;
		double y = ms_buttonCancel.top  * rMainY;
		buttonCancel->SetWindowPos( NULL, (int)x, (int)y, ms_buttonCancel.Width(), ms_buttonCancel.Height(), 0 );
	}

	if( buttonDelete )
	{
		double x = ms_buttonDelete.left * rMainX;
		double y = ms_buttonDelete.top  * rMainY;
		buttonDelete->SetWindowPos( NULL, (int)x, (int)y, ms_buttonDelete.Width(), ms_buttonDelete.Height(), 0 );
	}

	if( buttonEdit )
	{
		double x = ms_buttonEdit.left * rMainX;
		double y = ms_buttonEdit.top  * rMainY;
		buttonEdit->SetWindowPos( NULL, (int)x, (int)y, ms_buttonEdit.Width(), ms_buttonEdit.Height(), 0 );
	}

	if( buttonNew )
	{
		double x = ms_buttonNew.left * rMainX;
		double y = ms_buttonNew.top  * rMainY;
		buttonNew->SetWindowPos( NULL, (int)x, (int)y, ms_buttonNew.Width(), ms_buttonNew.Height(), 0 );
	}

	if( stringList )
	{
		double x = ms_stringTable.left * rMainX;
		double y = ms_stringTable.top  * rMainY;
		double w = ms_stringTable.Width() * rMainX;
		double h = ms_stringTable.Height() * rMainY;
		stringList->SetWindowPos( NULL, (int)x, (int)y, (int)w, (int)h, 0 );
	}
}

//-----------------------------------------------------------------------------------------------------

int StringEditor::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	return CDialog::OnCreate( lpCreateStruct );
}

//-----------------------------------------------------------------------------------------------------

LRESULT StringEditor::InitDialog(WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = CDialog::OnInitDialog();

	GetWindowRect( ms_mainRect );
	GetDlgItem(IDOK)->GetWindowRect( ms_buttonOk );
	GetDlgItem(IDCANCEL)->GetWindowRect( ms_buttonCancel );
	GetDlgItem(IDC_STRING_LIST)->GetWindowRect( ms_stringTable );
	GetDlgItem(IDC_BUTTON_DELETE)->GetWindowRect( ms_buttonDelete );
	GetDlgItem(IDC_BUTTON_EDIT)->GetWindowRect( ms_buttonEdit );
	GetDlgItem(IDC_BUTTON_NEW)->GetWindowRect( ms_buttonNew );

	// make relative to mainWindow's topLeft
	ms_buttonOk     -= ms_mainRect.TopLeft();
	ms_buttonCancel -= ms_mainRect.TopLeft();
	ms_stringTable  -= ms_mainRect.TopLeft();
	ms_buttonDelete -= ms_mainRect.TopLeft();
	ms_buttonEdit   -= ms_mainRect.TopLeft();
	ms_buttonNew    -= ms_mainRect.TopLeft();
	ms_mainRect     -= ms_mainRect.TopLeft();

	initStrings();

	ms_stringLowestID = 0xFFFF-1;

    return lResult;
}

//-----------------------------------------------------------------------------------------------------

void StringEditor::OnBnClickedButtonNew()
{
	CString numText;
	numText.Format("_%04x", ms_stringLowestID );
	ms_stringLowestID--;

	CString newText;
	newText.LoadString(IDS_STRING_NEW);
	newText += numText;

	CListCtrl* listCtrl = (CListCtrl*)GetDlgItem(IDC_STRING_LIST);

	int newItemID = listCtrl->GetItemCount();
	int itemID = listCtrl->InsertItem( LVIF_TEXT|LVIF_STATE, newItemID, newText, 0, 0, 0, 0);
	listCtrl->SetItemText(itemID, 1, newText);
}

//-----------------------------------------------------------------------------------------------------

void StringEditor::OnBnClickedButtonDelete()
{
}

//-----------------------------------------------------------------------------------------------------

void StringEditor::OnBnClickedButtonEdit()
{
	CListCtrl* listCtrl = (CListCtrl*)GetDlgItem(IDC_STRING_LIST);

	int item = listCtrl->GetSelectionMark();
	if( item == -1 )
		return;

	CString tag   = listCtrl->GetItemText(item, 0);
	CString value = listCtrl->GetItemText(item, 1);

	CDialog dialog;
	dialog.Create( IDD_EDIT_STRING_ENTRY, this );

	dialog.GetDlgItem(IDC_EDIT_TAG)->SetWindowText(tag);
	dialog.GetDlgItem(IDC_EDIT_VALUE)->SetWindowText(value);

	dialog.ShowWindow( SW_NORMAL );
	if( dialog.RunModalLoop() == IDOK )
	{
		dialog.GetDlgItem(IDC_EDIT_TAG)->GetWindowText( tag );
		dialog.GetDlgItem(IDC_EDIT_VALUE)->GetWindowText( value );

		listCtrl->SetItemText( item, 0, tag );
		listCtrl->SetItemText( item, 1, value );
	}
}

