//
// TriggerObject.cpp
//

#include "stdafx.h"
#include "globals.h"

#include "Object.h"
#include "Camera.h"
#include "DataList.h"
#include "CQTrace.h"
#include "SystemStructs.h"
#include "tinyxml\tinyxml.h"
#include "ExportImport.h"
#include "TRect.h"
#include "Editor.h"
#include "ClipBoard.h"
#include "TriggerFlags.h"

#include <GameTypes.h>
#include <FileSys.h>
#include <TSmartPointer.h>
#include <ITextureLibrary.h>
#include <Renderer.h>
#include <IAnim.h>
#include <ICamera.h>
#include <IMesh.h>
#include <TComponent.h>

#include <IConnection.h>
#include <Engine.h>
#include <system.h>
#include <startup.h>

#include <map>
#include <string>
#include <windowsx.h>
#include <afxadv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Object;

//-----------------------------------------------------------------------------------------------------

struct TriggerObject : public IObject, public ISaverLoader, public IClipboardObject
{
	BEGIN_DACOM_MAP_INBOUND(TriggerObject)
		DACOM_INTERFACE_ENTRY(IObject)
		DACOM_INTERFACE_ENTRY(ISaverLoader)
		DACOM_INTERFACE_ENTRY(IClipboardObject)
	END_DACOM_MAP()

	ArchetypeData*        data;
	const UniqueID        uniqueID;
	TRANSFORM             xform;
	INSTANCE_INDEX        meshInstIndex;
	CString               archname;
	CPoint                gridSize;
	U32                   systemID;
	U32                   playerID;
	CString               scriptHandle;
	void*                 customData;
	U32                   customDataSize;
	MT_TRIGGER_QLOAD      triggerData;

	TriggerObject() : 
		uniqueID(Object::nextUniqueID++), 
		data(NULL), 
		meshInstIndex(INVALID_INSTANCE_INDEX), 
		gridSize(2,2), 
		systemID(0), 
		playerID(0), 
		customData(NULL),
		customDataSize(0)
		{}

	virtual Transform& GetTransform();
	virtual bool       SetTransform( Transform& _xform );
	virtual UniqueID   GetID();
	virtual void       Render();
	virtual void       Delete();
	virtual void       GetObjectData( struct ObjectData& _data );
	virtual bool       AddTab( HWND _tabCtrl );

	virtual void SetSystemID( U32 _systemID )
	{
		systemID = _systemID;
	}

	virtual void SetPlayerID( U8 _playerID )
	{
		playerID = _playerID;
	}

	virtual void SetStringHandle( U32 _stringHandle )
	{
	}

	virtual void SetDataOverride( struct MISSION_DATA_OVERRIDE& _missionDataOverride )
	{
	}

	virtual void SetScriptHandle( const char* _scriptHandle )
	{
		scriptHandle = _scriptHandle;
	}

	virtual bool SetCustomData( void* _data, int _dataSize )
	{
		if( customData )
		{
			::realloc( customData, _dataSize );
		}
		else
		{
			customData = new char[ _dataSize ];
		}
		customDataSize = _dataSize;
		memcpy( customData, _data, customDataSize );
		return true;
	}

	virtual void* GetCustomData( int* _dataSize )
	{
		if( _dataSize )
		{
			*_dataSize = customDataSize;
		}
		return customData;
	}

	virtual void ResetData( void )
	{
	}

	// ISaverLoader

	virtual bool Save( class TiXmlNode& );
	virtual bool Load( class TiXmlNode& );
	virtual bool Save( struct IFileSystem& );
	virtual bool Load( struct IFileSystem& );

	// IClipboardObject

	virtual const char* GetType();
	virtual bool Copy( CSharedFile& _memfile );
	virtual bool Paste( CSharedFile& _memfile );
	virtual bool Append( CSharedFile& _memfile ); 

	// local

	bool create( const char* _archname );
	bool createRegion( BT_TRIGGER* );
	bool createPlayerStart( BT_PLAYERBOMB_DATA* );
	bool createWaypoint( BT_WAYPOINT* );
	bool createScriptObject( BT_SCRIPTOBJECT* );

	void createName( CString& _name );
	void setMissionData( MISSION_DATA& );

	static INT_PTR CALLBACK tabProc_trigger( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static INT_PTR CALLBACK tabProc_playerStart( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static INT_PTR CALLBACK tabProc_waypoint( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static INT_PTR CALLBACK tabProc_scriptObject( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
};

//-----------------------------------------------------------------------------------------------------

Transform& TriggerObject::GetTransform()
{
	return xform;
}

//-----------------------------------------------------------------------------------------------------

bool TriggerObject::SetTransform( Transform& _xform )
{
	xform = _xform;
	if( meshInstIndex != INVALID_INSTANCE_INDEX )
	{
		ENGINE->set_transform(meshInstIndex, xform);
	}
	return true;
}

//-----------------------------------------------------------------------------------------------------

UniqueID TriggerObject::GetID()
{
	return uniqueID;
}

//-----------------------------------------------------------------------------------------------------

void TriggerObject::Render()
{
	if( meshInstIndex != INVALID_INSTANCE_INDEX )
	{
		SINGLE dt = Editor::deltaTime;
		ENGINE->update_instance(meshInstIndex, 0, dt);
		ANIM->update_instance(meshInstIndex,dt);

		ARCHETYPE_INDEX	archIdx = ENGINE->get_instance_archetype( meshInstIndex );

		COMPTR<IMesh> mesh;
		ENGINE->query_archetype_interface( archIdx, IID_IMesh, (IDAComponent**)&mesh );

		const float LODPERCENT = 1.0f;
		ENGINE->render_instance(CAMERALIB, meshInstIndex, 0, LODPERCENT, 0, NULL);
	}
}

//-----------------------------------------------------------------------------------------------------

void TriggerObject::GetObjectData( ObjectData& _data )
{
	_data.archetype	   = archname;
	_data.xform		   = xform;
	_data.id		   = uniqueID;
	_data.gridSize	   = gridSize;
	_data.slotsNeeded  = 0;
	_data.stringHandle = 0;
	_data.scriptHandle = scriptHandle;
	_data.objectClass  = data->basicData->objClass;
	_data.bJumpGate    = false;
}

//-----------------------------------------------------------------------------------------------------

bool TriggerObject::Save( TiXmlNode& _node )
{
	TiXmlElement trigger("OBJECT");

	Export::XML::transform( xform, trigger );

	trigger.SetAttribute("TriggerObject", archname );
	trigger.SetAttribute("systemID", systemID );
	trigger.SetAttribute("playerID", playerID );
	trigger.SetAttribute("scriptHandle", scriptHandle );

	if( data->basicData->objClass == OC_TRIGGER )
	{
		TiXmlElement trig("TRIGGER_DATA");
			trig.SetAttribute( "triggerFlags", triggerData.triggerFlags );
			trig.SetAttribute( "triggerShipID", triggerData.triggerShipID );
			trig.SetAttribute( "triggerObjClassID", triggerData.triggerObjClassID );
			trig.SetAttribute( "triggerMObjClassID", triggerData.triggerMObjClassID );
			trig.SetAttribute( "triggerPlayerID", triggerData.triggerPlayerID );
			trig.SetAttribute( "triggerRange", triggerData.triggerRange );
			trig.SetAttribute( "progName", triggerData.progName );
			trig.SetAttribute( "bEnabled", triggerData.bEnabled );
			trig.SetAttribute( "bDetectOnlyReady", triggerData.bDetectOnlyReady );
		trigger.InsertEndChild( trig );
	}

	_node.InsertEndChild(trigger);
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool TriggerObject::Load( TiXmlNode& _node )
{
	TiXmlElement* trigger = _node.ToElement();

	if( trigger && trigger->Attribute("TriggerObject") )
	{
		// for pasted objects
		if( archname == "" )
		{
			create( trigger->Attribute("TriggerObject") );
		}

		const char* loadHandle = trigger->Attribute("scriptHandle");
		if( loadHandle )
		{
			scriptHandle = loadHandle;
		}

		Import::XML::transform( xform, trigger->FirstChild("TRANSFORM") );
		SetTransform( xform );

		systemID = trigger->GetAttributeLong("systemID");
		playerID = trigger->GetAttributeLong("playerID");

		if( data && data->basicData->objClass == OC_TRIGGER )
		{
			TiXmlElement* t = trigger->FirstChildElement("TRIGGER_DATA");
			if( t )
			{
				triggerData.triggerFlags	   = t->GetAttributeUnsignedLong("triggerFlags");
				triggerData.triggerShipID	   = t->GetAttributeUnsignedLong("triggerShipID");
				triggerData.triggerObjClassID  = t->GetAttributeUnsignedLong("triggerObjClassID");
				triggerData.triggerMObjClassID = t->GetAttributeUnsignedLong("triggerMObjClassID");
				triggerData.triggerPlayerID	   = t->GetAttributeUnsignedLong("triggerPlayerID");
				triggerData.triggerRange	   = t->GetAttributeUnsignedLong("triggerRange");
				triggerData.bEnabled		   = t->GetAttributeUnsignedLong("bEnabled") != false;
				triggerData.bDetectOnlyReady   = t->GetAttributeUnsignedLong("bDetectOnlyReady") != false;

				if( t->Attribute("progName") )
				{
					strncpy( triggerData.progName, t->Attribute("progName"), countof(triggerData.progName) );
				}
			}
		}

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

bool TriggerObject::Save( struct IFileSystem& _filesystem )
{
	// must belong to a system
	if( !systemID ) 
		return false;

	// spell out archname dir

	_filesystem.CreateDirectory(data->archname);
	if (_filesystem.SetCurrentDirectory(data->archname) == 0)
	{
		return false;
	}

	// define block

	struct Data
	{
		union Block
		{
			MT_TRIGGER_QLOAD      trigger;
			MT_PLAYERBOMB_QLOAD   playerStart;
			MT_SCRIPTOBJECT_QLOAD scripObject;
			MT_WAYPOINT_QLOAD     waypoint;
		};

		CString name;
		int size;
		Block block;
	};

	// prepare data block

	Data dataBlock;

	// get name
	dataBlock.name = scriptHandle;

	switch( data->basicData->objClass )
	{
		case OC_TRIGGER:
		{
			BT_TRIGGER* pData = (BT_TRIGGER*)data->basicData;

			// prepare data
			dataBlock.block.trigger.position.init( xform.translation, systemID );
			dataBlock.block.trigger.bDetectOnlyReady   = triggerData.bDetectOnlyReady;
			dataBlock.block.trigger.bEnabled		   = triggerData.bEnabled;
			dataBlock.block.trigger.triggerFlags	   = triggerData.triggerFlags;
			dataBlock.block.trigger.triggerMObjClassID = triggerData.triggerMObjClassID;
			dataBlock.block.trigger.triggerObjClassID  = triggerData.triggerObjClassID;
			dataBlock.block.trigger.triggerPlayerID	   = triggerData.triggerPlayerID;
			dataBlock.block.trigger.triggerRange	   = triggerData.triggerRange;
			dataBlock.block.trigger.triggerShipID	   = triggerData.triggerShipID;
			dataBlock.block.trigger.progName[0]		   = 0; // TODO: figure out how to use this...

			// log size
			dataBlock.size = sizeof(MT_TRIGGER_QLOAD);

			_filesystem.CreateDirectory("MT_TRIGGER_QLOAD");
			if (_filesystem.SetCurrentDirectory("MT_TRIGGER_QLOAD") == 0)
			{
				CQERROR0("QuickSave failed on Directory 'MT_TRIGGER_QLOAD'");
				return false;
			}
			break;
		}

		case OC_PLAYERBOMB:
		{
			BT_PLAYERBOMB_DATA* pData = (BT_PLAYERBOMB_DATA*)data->basicData;

			// prepare data
			dataBlock.block.playerStart.position.init( xform.translation, systemID );
			dataBlock.block.playerStart.dwMissionID = playerID;
			dataBlock.block.playerStart.bNoExplode = true;

			// log size
			dataBlock.size = sizeof(MT_PLAYERBOMB_QLOAD);

			_filesystem.CreateDirectory("MT_PLAYERBOMB_QLOAD");
			if (_filesystem.SetCurrentDirectory("MT_PLAYERBOMB_QLOAD") == 0)
			{
				CQERROR0("QuickSave failed on Directory 'MT_PLAYERBOMB_QLOAD'");
				return false;
			}
			break;
		}

		case OC_WAYPOINT:
		{
			BT_WAYPOINT* pData = (BT_WAYPOINT*)data->basicData;

			// log size
			dataBlock.size = sizeof(MT_WAYPOINT_QLOAD);

			// prepare data
			dataBlock.block.waypoint.position.init( xform.translation, systemID );
			dataBlock.block.waypoint.dwMissionID = playerID;

			_filesystem.CreateDirectory("MT_WAYPOINT_QLOAD");
			if (_filesystem.SetCurrentDirectory("MT_WAYPOINT_QLOAD") == 0)
			{
				CQERROR0("QuickSave failed on Directory 'MT_WAYPOINT_QLOAD'");
				return false;
			}
			break;
		}

		case OC_SCRIPTOBJECT:
		{
			BT_SCRIPTOBJECT* pData = (BT_SCRIPTOBJECT*)data->basicData;

			// log size
			dataBlock.size = sizeof(MT_SCRIPTOBJECT_QLOAD);

			// prepare data
			dataBlock.block.scripObject.position.init( xform.translation, systemID );
			dataBlock.block.scripObject.dwMissionID = playerID;

			_filesystem.CreateDirectory("MT_SCRIPTOBJECT_QLOAD");
			if (_filesystem.SetCurrentDirectory("MT_SCRIPTOBJECT_QLOAD") == 0)
			{
				CQERROR0("QuickSave failed on Directory 'MT_SCRIPTOBJECT_QLOAD'");
				return false;
			}
			break;
		}
	}

	// write out block

	DAFILEDESC fdesc = dataBlock.name;

	fdesc.lpImplementation = "UTF";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

	HANDLE hDataBlock = _filesystem.OpenChild(&fdesc);
	if( hDataBlock != INVALID_HANDLE_VALUE )
	{
		DWORD dwWritten;
		_filesystem.WriteFile( hDataBlock, &dataBlock.block, dataBlock.size, &dwWritten, 0);
		_filesystem.CloseHandle( hDataBlock );
	}

//	// write out any overridden info, if there is a script handle
//	if( !scriptHandle.IsEmpty() )
//	{
//		MISSION_DATA_OVERRIDE missionDataOverride;
//		ZeroMemory( &missionDataOverride, sizeof(missionDataOverride) );
//		strncpy( missionDataOverride.scriptHandle, scriptHandle, countof(missionDataOverride.scriptHandle) );
//
//		DAFILEDESC desc = "MISSION_DATA_OVERRIDE";
//
//		desc.lpImplementation = "UTF";
//		desc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
//		desc.dwShareMode = 0;  // no sharing
//		desc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists
//
//		HANDLE hChild = f->OpenChild(&desc);
//		if( hChild != INVALID_HANDLE_VALUE )
//		{
//			f->WriteFile( hChild, &missionDataOverride, sizeof(missionDataOverride), &dwWritten );
//			f->CloseHandle( hChild );
//		}
//	}

	_filesystem.SetCurrentDirectory(".."); // out of MT_?? dir
	_filesystem.SetCurrentDirectory(".."); // out of archname dir
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool TriggerObject::Load( struct IFileSystem& )
{
	return 0;
}

//-----------------------------------------------------------------------------------------------------

void TriggerObject::Delete()
{
	delete customData;
	customDataSize = NULL;

	if( meshInstIndex != INVALID_INSTANCE_INDEX )
	{
		ARCHETYPE_INDEX	archIdx = ENGINE->get_instance_archetype( meshInstIndex );
		ENGINE->release_archetype( archIdx );

		// todo(aaj-4/16/2004): how does the engine release instances?
		meshInstIndex = INVALID_INSTANCE_INDEX;
	}
}

//-----------------------------------------------------------------------------------------------------

bool TriggerObject::AddTab( HWND _tabCtrl )
{
	HWND hTab = 0;
	HINSTANCE hInstance = ::AfxGetApp()->m_hInstance;
	LPSTR label = 0;

	if( data->basicData->objClass == OC_TRIGGER )
	{
		hTab  = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_OP_TRIGGER), _tabCtrl, (DLGPROC)tabProc_trigger, (DWORD)this );
		label = "Trigger Object";
	}
	else if( data->basicData->objClass == OC_PLAYERBOMB )
	{
		hTab  = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_OP_BASIC), _tabCtrl, (DLGPROC)tabProc_playerStart, (DWORD)this );
		label = "Player Start";
	}
	else if( data->basicData->objClass == OC_WAYPOINT )
	{
		hTab  = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_OP_BASIC), _tabCtrl, (DLGPROC)tabProc_waypoint, (DWORD)this );
		label = "Waypoint";
	}
	else if( data->basicData->objClass == OC_SCRIPTOBJECT )
	{
		hTab  = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_OP_BLACKHOLE), _tabCtrl, (DLGPROC)tabProc_scriptObject, (DWORD)this );
		label = "Script Object";
	}

	CRect tabsRect;
	TabCtrl_AdjustRect(_tabCtrl, true, tabsRect );
	tabsRect.SetRect( 0, 0, 16, 24 );

	CRect rect;
	::GetClientRect( hTab, rect );
	rect.OffsetRect( tabsRect.Width(), tabsRect.Height() );
	::SetWindowPos( hTab, NULL, rect.left, rect.top, rect.Width(), rect.Height(), 0 );

	TCITEM itemTab;
	itemTab.mask = TCIF_PARAM | TCIF_TEXT;
	itemTab.pszText = label;
	itemTab.cchTextMax = strlen(itemTab.pszText);
	itemTab.iImage = -1;
	itemTab.lParam = (DWORD)hTab;
	TabCtrl_InsertItem( _tabCtrl,  TabCtrl_GetItemCount(_tabCtrl),  &itemTab );

	return true;
}

//-----------------------------------------------------------------------------------------------------

bool TriggerObject::createRegion( BT_TRIGGER* _trigger )
{
	// loading up the archetype data
	data = Object::getArchetypeData( archname, _trigger->fileName, _trigger );

	// creating an instance of this model
	if( !data )
	{
		return false;
	}

	// init trigger data
	ZeroMemory( &triggerData, sizeof(triggerData) );

	// load name
	scriptHandle.LoadString( hStringTable, _trigger->missionData.displayName, 1033 );
	createName( scriptHandle );

	meshInstIndex = ENGINE->create_instance2( data->meshIndex, NULL );
	if( meshInstIndex == INVALID_INSTANCE_INDEX )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

bool TriggerObject::createPlayerStart( BT_PLAYERBOMB_DATA* _playerBombData )
{
	// loading up the archetype data
	data = Object::getArchetypeData( archname, _playerBombData->filename, _playerBombData );

	// creating an instance of this model
	if( !data )
	{
		return false;
	}

	// load name
	scriptHandle.LoadString( hStringTable, _playerBombData->missionData.displayName, 1033 );
	createName( scriptHandle );

	meshInstIndex = ENGINE->create_instance2( data->meshIndex, NULL );
	if( meshInstIndex == INVALID_INSTANCE_INDEX )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

bool TriggerObject::createWaypoint( BT_WAYPOINT* _waypoint )
{
	// loading up the archetype data
	data = Object::getArchetypeData( archname, _waypoint->fileName, _waypoint );

	// creating an instance of this model
	if( !data )
	{
		return false;
	}

	// load name
	scriptHandle.LoadString( hStringTable, _waypoint->missionData.displayName, 1033 );
	createName( scriptHandle );

	meshInstIndex = ENGINE->create_instance2( data->meshIndex, NULL );
	if( meshInstIndex == INVALID_INSTANCE_INDEX )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

bool TriggerObject::createScriptObject( BT_SCRIPTOBJECT* _scriptObject )
{
	// loading up the archetype data
	data = Object::getArchetypeData( archname, _scriptObject->fileName, _scriptObject );

	// creating an instance of this model
	if( !data )
	{
		return false;
	}

	// load name
	scriptHandle.LoadString( hStringTable, _scriptObject->missionData.displayName, 1033 );
	createName( scriptHandle );

	meshInstIndex = ENGINE->create_instance2( data->meshIndex, NULL );
	if( meshInstIndex == INVALID_INSTANCE_INDEX )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

bool TriggerObject::create( const char* _archname )
{
	// creating a dummy object?
	if( _archname == "" )
	{
		archname = "";
		return true;
	}

	BASIC_DATA* basicData = (BASIC_DATA*)GAMETYPES->GetArchetypeData(_archname);
	if( !basicData )
	{
		return false;
	}

	archname = _archname;

	if( basicData->objClass == OC_TRIGGER )
	{
		return createRegion( (BT_TRIGGER*)basicData );
	}
	else if( basicData->objClass == OC_PLAYERBOMB )
	{
		return createPlayerStart( (BT_PLAYERBOMB_DATA*)basicData );
	}
	else if( basicData->objClass == OC_WAYPOINT )
	{
		return createWaypoint( (BT_WAYPOINT*)basicData );
	}
	else if( basicData->objClass == OC_SCRIPTOBJECT )
	{
		return createScriptObject( (BT_SCRIPTOBJECT*)basicData );
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

void TriggerObject::createName( CString& _name )
{
	// TODO: move this to the base IObject class...

	CQASSERT( uniqueID!=0 );

	if( _name == "" )
	{
		_name.Format("%0d8NoName",uniqueID);
	}

	// put uniqueID in hex view
	char uniqueIdName[16] = {"#"};
	_ltoa(uniqueID, uniqueIdName+1, 16);
	strupr(uniqueIdName);
	int len = strlen(uniqueIdName);
	uniqueIdName[len++] = '#';
	uniqueIdName[len] = 0;

	// get rid of the trailing ( ! )
	int trail = _name.Find(" ("); 
	if( trail != -1 )
	{
		_name.SetAt(trail+1, _T('\0') );
	}

	CString newName = CString(uniqueIdName) + CString(" ") + CString(_name);
	_name = newName;
}

//-----------------------------------------------------------------------------------------------------

void TriggerObject::setMissionData( MISSION_DATA& _missionData )
{
}

//-----------------------------------------------------------------------------------------------------
// tab controls

INT_PTR CALLBACK TriggerObject::tabProc_trigger( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_INITDIALOG )
	{
		TriggerObject* obj = (TriggerObject*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, (LONG)obj );

		Button_SetCheck( GetDlgItem(hwndDlg,IDC_FLAG_ENABLED),   obj->triggerData.bEnabled ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck( GetDlgItem(hwndDlg,IDC_FLAG_SHIPID),    obj->triggerData.triggerFlags & TRIGGER_SHIP_ID ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck( GetDlgItem(hwndDlg,IDC_FLAG_OBJCLASS),  obj->triggerData.triggerFlags & TRIGGER_OBJCLASS ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck( GetDlgItem(hwndDlg,IDC_FLAG_SHIPTYPE),  obj->triggerData.triggerFlags & TRIGGER_MOBJCLASS ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck( GetDlgItem(hwndDlg,IDC_FLAG_PLAYERID),  obj->triggerData.triggerFlags & TRIGGER_PLAYER ? BST_CHECKED : BST_UNCHECKED);
		Button_SetCheck( GetDlgItem(hwndDlg,IDC_FLAG_READYWAIT), obj->triggerData.triggerFlags & TRIGGER_FORCEREADY ? BST_CHECKED : BST_UNCHECKED);
	}
	else if( uMsg == WM_SHOWWINDOW )
	{
		if( !wParam ) // being hidden, apply changes
		{
			TriggerObject* obj = (TriggerObject*)GetWindowLong( hwndDlg, GWL_USERDATA );

			obj->triggerData.bEnabled = Button_GetCheck( GetDlgItem(hwndDlg,IDC_FLAG_ENABLED) ) == BST_CHECKED;

			if( Button_GetCheck( GetDlgItem(hwndDlg,IDC_FLAG_SHIPID) ) == BST_CHECKED )
			{
				obj->triggerData.triggerFlags |= TRIGGER_SHIP_ID;
			}
			if( Button_GetCheck( GetDlgItem(hwndDlg,IDC_FLAG_OBJCLASS) ) == BST_CHECKED )
			{
				obj->triggerData.triggerFlags |= TRIGGER_OBJCLASS;
			}
			if( Button_GetCheck( GetDlgItem(hwndDlg,IDC_FLAG_SHIPTYPE) ) == BST_CHECKED )
			{
				obj->triggerData.triggerFlags |= TRIGGER_MOBJCLASS;
			}
			if( Button_GetCheck( GetDlgItem(hwndDlg,IDC_FLAG_PLAYERID) ) == BST_CHECKED )
			{
				obj->triggerData.triggerFlags |= TRIGGER_PLAYER;
			}
			if( Button_GetCheck( GetDlgItem(hwndDlg,IDC_FLAG_READYWAIT) ) != BST_CHECKED )
			{
				obj->triggerData.triggerFlags &= ~TRIGGER_NOFORCEREADY;
				obj->triggerData.triggerFlags |= TRIGGER_FORCEREADY;
				obj->triggerData.bDetectOnlyReady = true;
			}
			else
			{
				obj->triggerData.triggerFlags |= TRIGGER_NOFORCEREADY;
				obj->triggerData.triggerFlags &= ~TRIGGER_FORCEREADY;
				obj->triggerData.bDetectOnlyReady = false;
			}
		}
	}

	return 0;
}

//----------------------------------------------------------------------------------------------

INT_PTR CALLBACK TriggerObject::tabProc_playerStart( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_INITDIALOG )
	{
		TriggerObject* obj = (TriggerObject*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, (LONG)obj );
	}
	else if( uMsg == WM_SHOWWINDOW )
	{
		if( !wParam ) // being hidden, apply changes
		{
			TriggerObject* obj = (TriggerObject*)GetWindowLong( hwndDlg, GWL_USERDATA );
		}
	}

	return 0;
}

//----------------------------------------------------------------------------------------------

INT_PTR CALLBACK TriggerObject::tabProc_waypoint( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_INITDIALOG )
	{
		TriggerObject* obj = (TriggerObject*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, (LONG)obj );
	}
	else if( uMsg == WM_SHOWWINDOW )
	{
		if( !wParam ) // being hidden, apply changes
		{
			TriggerObject* obj = (TriggerObject*)GetWindowLong( hwndDlg, GWL_USERDATA );
		}
	}

	return 0;
}

//----------------------------------------------------------------------------------------------

INT_PTR CALLBACK TriggerObject::tabProc_scriptObject( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_INITDIALOG )
	{
		TriggerObject* obj = (TriggerObject*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, (LONG)obj );
	}
	else if( uMsg == WM_SHOWWINDOW )
	{
		if( !wParam ) // being hidden, apply changes
		{
			TriggerObject* obj = (TriggerObject*)GetWindowLong( hwndDlg, GWL_USERDATA );
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------------------------------
// IClipboardObject

const char* TriggerObject::GetType()
{
	return "TriggerObject";
}

bool TriggerObject::Copy( CSharedFile& _memfile )
{
	FILE * tempfile = ::tmpfile();

	TiXmlDocument clip("clip");
	Save(clip);
	clip.Print( tempfile, 0 );

	int size = ftell(tempfile);
	void* buffer = alloca(size);

	fseek(tempfile, 0L, SEEK_SET );
	fread(buffer, size, 1, tempfile);

	_memfile.Write( buffer, size );

	return true;
}

bool TriggerObject::Paste( CSharedFile& _memfile )
{
	int size = _memfile.GetLength();
	void* buffer = alloca(size + 2);
    memset( buffer, 0, size+2 );

	if( buffer )
	{
		_memfile.Read( buffer, size );

		TiXmlDocument doc;
		doc.Parse( (const char*)buffer );

		TiXmlElement* objectElement = doc.FirstChildElement("OBJECT");
		if( objectElement )
		{
			if( Load(*objectElement) )
			{
				return true;
			}
		}
	}

	return false;
}

bool TriggerObject::Append( CSharedFile& _memfile )
{
	return true;
}

//-----------------------------------------------------------------------------------------------------
// startup

struct TriggerObjectFactory : public IComponentFactory
{
	BEGIN_DACOM_MAP_INBOUND(TriggerObjectFactory)
		DACOM_INTERFACE_ENTRY(IComponentFactory)
	END_DACOM_MAP()

	// IComponentFactory

	virtual GENRESULT COMAPI CreateInstance (DACOMDESC *descriptor, void **instance)
	{
		*instance = NULL;

		if( descriptor->size == sizeof(OBJECT_DACOMDESC) )
		{
			OBJECT_DACOMDESC* objectDacomdesc = (OBJECT_DACOMDESC*)descriptor;

			TriggerObject* object = new DAComponent<TriggerObject>;
			if( object->create(objectDacomdesc->archname) )
			{
				*instance = object;
				return GR_OK;
			}

			delete object;
		}

		// something bad happended
		return GR_INTERFACE_UNSUPPORTED;
	}
};

struct _TriggerObjectfactory : GlobalComponent
{
	TriggerObjectFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<TriggerObjectFactory>;
		AddToGlobalCleanupList((IDAComponent **) &factory);
	}

	virtual void Initialize (void)
	{
		DACOM->RegisterComponent( factory, "TriggerObject", 0);
	}
};

static _TriggerObjectfactory __TriggerObjectfactory;
