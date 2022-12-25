// DialogProc_ObjectProperties.cpp
//

#include "stdafx.h"
#include "globals.h"

#include "Startup.h"
#include "CQTrace.h"
#include "SuperTrans.h"
#include "GridVector.h"
#include "Editor.h"
#include "StringEditor.h"
#include "StringTable.h"
#include "ObjectFamily.h"

#include "Mode.h"
#include "Object.h"
#include "DataList.h"
#include "Undo.h"
#include "SystemStructs.h"
#include "MainFrm.h"
#include "Campaign.h"
#include "Scenario.h"
#include "StringTable.h"
#include "Crc32Static.h"

#include <DBaseData.h> // MISSION_DATA & MISSION_DATA_OVERRIDE
#include <DPlanet.h>
#include <DSpaceShip.h>
#include <DPlatform.h>

#include <string>
#include <windowsx.h>

static CPoint s_Offset;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//----------------------------------------------------------------------------------------------
// string table enum

struct StringEnumList : public IStringEnum
{
	HWND listCtrl;

	virtual void EnumStringInfo( IStringEnum::StringInfo& _stringInfo )
	{
		// TODO: need to find a way insert Unicode into a list control box
		CString outText(_stringInfo.wideString);

		LVITEM item;
		memset( &item, 0, sizeof(item) );
		item.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
		item.lParam = _stringInfo.idString;
		item.pszText = (LPSTR)_stringInfo.tagString;
		item.state = 0;
		item.iItem = ListView_GetItemCount(listCtrl);
		item.cchTextMax = strlen(_stringInfo.tagString);

		int itemID = ListView_InsertItem( listCtrl, &item );
		ListView_SetItemText( listCtrl, itemID, 1, outText.GetBuffer(0) );
	}
};

struct ObjectFamilyComboInit : IObjectFamilyEnum
{
	IObject* object;
	CString currentFamily;

	ObjectFamilyComboInit() : object(NULL) {}

	virtual void EnumFamilyInfo( FamilyInfo& _info )
	{
		HWND combo = (HWND)_info.context;

		int cbItem = ComboBox_AddString( combo, _info.family );
		ComboBox_SetItemData( combo, cbItem, 0 );

		if( _info.family == currentFamily )
		{
			ComboBox_SetCurSel( combo, cbItem );
		}
	}

	virtual void EnumObjectInfo( ObjectInfo& _info )
	{
		if( _info.object == object )
		{
			currentFamily = _info.family;
		}
	}
};

//----------------------------------------------------------------------------------------------

struct ObjectProperties
{
	IObject*              object;
	CString               scriptHandle;
	U32                   stringTableEntry;
	MISSION_DATA_OVERRIDE data;
	U32                   dataCrcCheck;
	bool                  bDataUsed;

	ObjectProperties() : object(NULL), stringTableEntry(0), bDataUsed(false) {}

	void ApplyChanges( HWND _hwnd )
	{
		HWND editName = GetDlgItem( _hwnd, IDC_EDIT_SCRIPTHANDLE );
		if( editName )
		{
			char handle[64];
			::GetWindowText( editName, handle, 64 );
			object->SetScriptHandle( handle );
		}

		if( stringTableEntry )
		{
			object->SetStringHandle( stringTableEntry );
		}

		HWND combo = GetDlgItem(_hwnd, IDC_COMBO_GROUP);
		if( combo )
		{
			int cbItem = ComboBox_GetCurSel(combo);
			if( cbItem != CB_ERR )
			{
				TCHAR buffer[128];
				ComboBox_GetLBText( combo, cbItem, buffer );
				CAMPAIGN->GetCurrentScenario()->GetSettings().objectFamily->RemoveObjectFromFamily( NULL, object );
				CAMPAIGN->GetCurrentScenario()->GetSettings().objectFamily->AddObjectToFamily( buffer, object );
			}
		}


		U32 thisDataCheck = 0;
		CCrc32Static::BufferCrc32( &data, sizeof(data), thisDataCheck );
		if( thisDataCheck != dataCrcCheck )
		{
			bDataUsed = true;
		}

		if( bDataUsed )
		{
			object->SetDataOverride( data );
		}

		CMainFrame * pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
		pFrame->UpdateBars();
	}

	void UpdateObject( HWND _name, HWND _scriptHandle )
	{
		if( _name )
		{
			const wchar_t* objName = STRINGTABLE->GetStringByID( stringTableEntry );

			if( objName )
			{
				::SetWindowTextW(_name,objName);
			}
			else
			{
				CString notSet = "(not set) - ";
				notSet += scriptHandle;

				::SetWindowText(_name,notSet);
			}
		}

		if( _scriptHandle )
		{
			::SetWindowText(_scriptHandle,scriptHandle);
		}
	}
};

//----------------------------------------------------------------------------------------------

INT_PTR CALLBACK DLGPROC_BasicProperties( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_INITDIALOG )
	{
		ObjectProperties* props = (ObjectProperties*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, (LONG)props );

		CString str;

		str.Format("%d", props->data.hullPointsMax );
		SetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_HULLMAX), str );

		str.Format("%d", props->data.supplyPointsMax );
		SetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_SUPPLYPNTS), str );

		str.Format("%d", props->data.scrapValue );
		SetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_SCRAPVAL), str );

		str.Format("%d", props->data.commandPoints );
		SetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_COMMANDPNTS), str );

		str.Format("%f", props->data.sensorRadius );
		SetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_SENSORRAD), str );

		str.Format("%f", props->data.cloakedSensorRadius );
		SetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_CLOAKEDRAD), str );

		str.Format("%f", props->data.maxVelocity );
		SetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_MAXVELOCITY), str );

		str.Format("%f", props->data.baseShieldLevel );
		SetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_BASESHIELD), str );
	}

	else if( uMsg == WM_SHOWWINDOW )
	{
		if( !wParam ) // being hidden
		{
			ObjectProperties* props = (ObjectProperties*)GetWindowLong( hwndDlg, GWL_USERDATA );
			
			char strNumber[64];

			GetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_HULLMAX), strNumber, 64 );
			props->data.hullPointsMax = atoi( strNumber );

			GetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_SUPPLYPNTS), strNumber, 64 );
			props->data.supplyPointsMax = atoi( strNumber );

			GetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_SCRAPVAL), strNumber, 64 );
			props->data.scrapValue = atoi( strNumber );

			GetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_COMMANDPNTS), strNumber, 64 );
			props->data.commandPoints = atoi( strNumber );

			GetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_SENSORRAD), strNumber, 64 );
			props->data.sensorRadius = atof( strNumber );

			GetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_CLOAKEDRAD), strNumber, 64 );
			props->data.cloakedSensorRadius = atof( strNumber );

			GetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_MAXVELOCITY), strNumber, 64 );
			props->data.maxVelocity = atof( strNumber );

			GetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_BASESHIELD), strNumber, 64 );
			props->data.baseShieldLevel = atof( strNumber );
		}
	}

	return 0;
}

//----------------------------------------------------------------------------------------------

INT_PTR CALLBACK DLGPROC_ArmorProperties( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_INITDIALOG )
	{
		ObjectProperties* props = (ObjectProperties*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, (LONG)props );

		//----------------------------------------------------------------------------------------------
		// armor defense value

		int cbItem = 0;
		cbItem = ComboBox_AddString( GetDlgItem(hwndDlg,IDC_COMBO_ARMOR), "None" );
		ComboBox_SetItemData( GetDlgItem(hwndDlg,IDC_COMBO_ARMOR), cbItem, NO_ARMOR );

		if( props->data.armorData.myArmor == NO_ARMOR )
		{
			ComboBox_SetCurSel(GetDlgItem(hwndDlg,IDC_COMBO_ARMOR), cbItem);
		}

		cbItem = ComboBox_AddString( GetDlgItem(hwndDlg,IDC_COMBO_ARMOR), "Light" );
		ComboBox_SetItemData( GetDlgItem(hwndDlg,IDC_COMBO_ARMOR), cbItem, LIGHT_ARMOR );

		if( props->data.armorData.myArmor == LIGHT_ARMOR )
		{
			ComboBox_SetCurSel(GetDlgItem(hwndDlg,IDC_COMBO_ARMOR), cbItem);
		}

		cbItem = ComboBox_AddString( GetDlgItem(hwndDlg,IDC_COMBO_ARMOR), "Medium" );
		ComboBox_SetItemData( GetDlgItem(hwndDlg,IDC_COMBO_ARMOR), cbItem, MEDIUM_ARMOR );

		if( props->data.armorData.myArmor == MEDIUM_ARMOR )
		{
			ComboBox_SetCurSel(GetDlgItem(hwndDlg,IDC_COMBO_ARMOR), cbItem);
		}

		cbItem = ComboBox_AddString( GetDlgItem(hwndDlg,IDC_COMBO_ARMOR), "Heavy" );
		ComboBox_SetItemData( GetDlgItem(hwndDlg,IDC_COMBO_ARMOR), cbItem, HEAVY_ARMOR );

		if( props->data.armorData.myArmor == HEAVY_ARMOR )
		{
			ComboBox_SetCurSel(GetDlgItem(hwndDlg,IDC_COMBO_ARMOR), cbItem);
		}

		//----------------------------------------------------------------------------------------------
		// armor attack values

		CString str;

		str.Format("%f", props->data.armorData._damageTable[NO_ARMOR] );
		SetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_DMGVSNONE), str );

		str.Format("%f", props->data.armorData._damageTable[LIGHT_ARMOR] );
		SetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_DMGVSLIGHT), str );

		str.Format("%f", props->data.armorData._damageTable[MEDIUM_ARMOR] );
		SetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_DMGVSMEDIUM), str );

		str.Format("%f", props->data.armorData._damageTable[HEAVY_ARMOR] );
		SetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_DMGVSHEAVY), str );
	}

	else if( uMsg == WM_SHOWWINDOW )
	{
		if( !wParam ) // being hidden
		{
			ObjectProperties* props = (ObjectProperties*)GetWindowLong( hwndDlg, GWL_USERDATA );
			
			char strNumber[64];

			GetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_DMGVSNONE), strNumber, 64 );
			props->data.armorData._damageTable[NO_ARMOR] = atof( strNumber );

			GetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_DMGVSLIGHT), strNumber, 64 );
			props->data.armorData._damageTable[LIGHT_ARMOR] = atof( strNumber );

			GetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_DMGVSMEDIUM), strNumber, 64 );
			props->data.armorData._damageTable[MEDIUM_ARMOR] = atof( strNumber );

			GetWindowText( GetDlgItem(hwndDlg,IDC_EDIT_DMGVSHEAVY), strNumber, 64 );
			props->data.armorData._damageTable[HEAVY_ARMOR] = atof( strNumber );

			int cbItem = ComboBox_GetCurSel(GetDlgItem(hwndDlg,IDC_COMBO_ARMOR));
			if( cbItem != CB_ERR )
			{
				props->data.armorData.myArmor = (ARMOR_TYPE)ComboBox_GetItemData( GetDlgItem(hwndDlg,IDC_COMBO_ARMOR), cbItem );
			}
		}
	}

	return 0;
}

//----------------------------------------------------------------------------------------------

INT_PTR CALLBACK DialogProc_ObjectProperties( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_INITDIALOG )
	{
		// window placement
		CRect rect;
		::GetWindowRect( hwndDlg, rect );

		::SetWindowPos( hwndDlg, NULL, s_Offset.x + rect.left, s_Offset.y + rect.top, 0, 0, SWP_NOSIZE );
		s_Offset += CPoint(10,10);

		ShowWindow( hwndDlg, SW_NORMAL );

		// get object data
		ObjectData data;
		IObject* object = (IObject*)lParam;
		object->GetObjectData( data );

		// set up ObjectProperties
		ObjectProperties* props = new ObjectProperties;
		props->object			= object;
		props->stringTableEntry = data.stringHandle;
		props->scriptHandle		= data.scriptHandle;
		props->bDataUsed        = data.bUseDataOverride;
		memcpy( &props->data, &data.dataOverride, sizeof(props->data) );
		CCrc32Static::BufferCrc32(&props->data, sizeof(props->data), props->dataCrcCheck);

		props->UpdateObject( GetDlgItem(hwndDlg,IDC_RICHEDIT_NAME), GetDlgItem(hwndDlg,IDC_EDIT_SCRIPTHANDLE) );

		// add family groups (if any)
		HWND combo = GetDlgItem( hwndDlg, IDC_COMBO_GROUP );
		if( combo )
		{
			ObjectFamilyComboInit ofci;
			ofci.object = object;
			CAMPAIGN->GetCurrentScenario()->GetSettings().objectFamily->EnumObjectFamilyInfo( ofci, object );
			CAMPAIGN->GetCurrentScenario()->GetSettings().objectFamily->EnumFamilyList( ofci, (DWORD)combo );
		}

		HWND tabs = GetDlgItem(hwndDlg,IDC_TABS);
		if( tabs )
		{
			HWND hBasic = ::CreateDialogParam( ::AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDD_OP_BASIC), tabs, (DLGPROC)DLGPROC_BasicProperties, (DWORD)props );
			HWND hArmor = ::CreateDialogParam( ::AfxGetApp()->m_hInstance, MAKEINTRESOURCE(IDD_OP_ARMOR), tabs, (DLGPROC)DLGPROC_ArmorProperties, (DWORD)props );

			TCITEM itemBasic;
			itemBasic.mask = TCIF_PARAM | TCIF_TEXT;
			itemBasic.pszText = "Basic";
			itemBasic.cchTextMax = strlen(itemBasic.pszText);
			itemBasic.iImage = -1;
			itemBasic.lParam = (DWORD)hBasic;
			TabCtrl_InsertItem( tabs,  TabCtrl_GetItemCount(tabs),  &itemBasic );

			TCITEM itemArmor;
			itemArmor.mask = TCIF_PARAM | TCIF_TEXT;
			itemArmor.pszText = "Armor";
			itemArmor.cchTextMax = strlen(itemBasic.pszText);
			itemArmor.iImage = -1;
			itemArmor.lParam = (DWORD)hArmor;
			TabCtrl_InsertItem( tabs,  TabCtrl_GetItemCount(tabs),  &itemArmor );

			props->object->AddTab( tabs );

			CRect tabsRect;
			::GetClientRect( tabs, tabsRect );
			TabCtrl_AdjustRect(tabs, true, tabsRect );
			tabsRect.SetRect( 0, 0, 16, 24 );

			CRect rect;
			::GetClientRect( hArmor, rect );
			rect.OffsetRect( tabsRect.Width(), tabsRect.Height() );
			::SetWindowPos( hArmor, NULL, rect.left, rect.top, rect.Width(), rect.Height(), 0 );

			::GetClientRect( hBasic, rect );
			rect.OffsetRect( tabsRect.Width(), tabsRect.Height() );
			::SetWindowPos( hBasic, NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_SHOWWINDOW );

			TabCtrl_SetCurSel( tabs, 0 );
		}

		if( props->bDataUsed )
		{
			Button_SetCheck( GetDlgItem(hwndDlg,IDC_CHECK_MODIFIED),  BST_CHECKED);
		}

		SetWindowLong( hwndDlg, GWL_USERDATA, (LONG)props );
	}

	else if( uMsg == WM_COMMAND )
	{
		if( LOWORD(wParam) == IDOK )
		{
			// close all tabs
			HWND hwndTab = GetDlgItem(hwndDlg,IDC_TABS);
			if( hwndTab )
			{
				for( int nTab = 0; nTab < TabCtrl_GetItemCount(hwndTab); nTab++ )
				{
					TCITEM item;
					item.mask = TCIF_PARAM;

					if( TabCtrl_GetItem(hwndTab,nTab,&item) )
					{
						ShowWindow( (HWND)item.lParam, SW_HIDE );
					}
				}
			}

			ObjectProperties* props = (ObjectProperties*)::GetWindowLong( hwndDlg, GWL_USERDATA);
			props->ApplyChanges( hwndDlg );
			delete props;

			EndDialog( hwndDlg, IDOK );
		}
		else if( LOWORD(wParam) == IDCANCEL )
		{
			ObjectProperties* props = (ObjectProperties*)::GetWindowLong( hwndDlg, GWL_USERDATA);
			delete props;

			EndDialog( hwndDlg, IDCANCEL );
		}
		else if( LOWORD(wParam) == IDC_BTN_NEWSTRING )
		{
			ObjectProperties* props = (ObjectProperties*)::GetWindowLong( hwndDlg, GWL_USERDATA);

			StringEditor stringEditor;
			stringEditor.SetSelectedString( props->stringTableEntry );

			if( stringEditor.DoModal() == IDOK )
			{
				if( stringEditor.GetSelectedString() != StringEditor::INVALID_STRING )
				{
					props->stringTableEntry = stringEditor.GetSelectedString();
					props->UpdateObject( GetDlgItem(hwndDlg,IDC_RICHEDIT_NAME), GetDlgItem(hwndDlg,IDC_EDIT_SCRIPTHANDLE) );
				}
			}
		}
		else if( LOWORD(wParam) == IDC_RESET )
		{
			ObjectProperties* props = (ObjectProperties*)::GetWindowLong( hwndDlg, GWL_USERDATA);
			props->object->ResetData();
			delete props;

			EndDialog( hwndDlg, IDC_RESET );
		}
	}

	// tab(s) control
	else if( uMsg == WM_NOTIFY )
	{
		int idCtrl = (int) wParam; 
		LPNMHDR pnmh = (LPNMHDR) lParam; 

		HWND hwndTab = idCtrl == IDC_TABS ? GetDlgItem(hwndDlg,IDC_TABS) : NULL;
		if( hwndTab == pnmh->hwndFrom )
		{
			if( pnmh->code == TCN_SELCHANGE )
			{
				int iPage = TabCtrl_GetCurSel(hwndTab);
				if( iPage != -1 )
				{
					TCITEM item;
					item.mask = TCIF_PARAM;

					if( TabCtrl_GetItem(hwndTab,iPage,&item) )
					{
						ShowWindow( (HWND)item.lParam, SW_NORMAL );
					}
				}
			}
			else if( pnmh->code == TCN_SELCHANGING )
			{
				int iPage = TabCtrl_GetCurSel(hwndTab);
				if( iPage != -1 )
				{
					TCITEM item;
					item.mask = TCIF_PARAM;

					if( TabCtrl_GetItem(hwndTab,iPage,&item) )
					{
						ShowWindow( (HWND)item.lParam, SW_HIDE );
					}
				}
			}
		}
	}

	return 0;
}

