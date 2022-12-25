//
// WormholeObject.cpp
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


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Object;

//-----------------------------------------------------------------------------------------------------

struct WormholeObject : public IObject, public ISaverLoader
{
	BEGIN_DACOM_MAP_INBOUND(WormholeObject)
		DACOM_INTERFACE_ENTRY(IObject)
		DACOM_INTERFACE_ENTRY(ISaverLoader)
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
	U32                   stringHandle;
	bool                  bUseDataOverride;
	MISSION_DATA_OVERRIDE dataOverride;
	void*                 customData;
	U32                   customDataSize;

	WormholeObject() : 
		uniqueID(Object::nextUniqueID++), 
		data(NULL), 
		meshInstIndex(INVALID_INSTANCE_INDEX), 
		gridSize(1,1), 
		systemID(0), 
		playerID(0), 
		stringHandle(0),
		bUseDataOverride(false),
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
		stringHandle = _stringHandle;
	}

	virtual void SetDataOverride( struct MISSION_DATA_OVERRIDE& _missionDataOverride )
	{
		bUseDataOverride = true;
		memcpy( &dataOverride, &_missionDataOverride, sizeof(MISSION_DATA_OVERRIDE) );
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

	// local

	bool create( const char* _archname );
	void createName( CString& _name );
	void setMissionData( MISSION_DATA& );

	static INT_PTR CALLBACK tabProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
};

//-----------------------------------------------------------------------------------------------------

Transform& WormholeObject::GetTransform()
{
	return xform;
}

//-----------------------------------------------------------------------------------------------------

bool WormholeObject::SetTransform( Transform& _xform )
{
	xform = _xform;
	if( meshInstIndex != INVALID_INSTANCE_INDEX )
	{
		ENGINE->set_transform(meshInstIndex, xform);
	}

	System* system = Editor::GetSystem(0,systemID);
	if( system )
	{
		for( int i = 0; i < system->jList.GetCount(); i++ )
		{
			JumpPoint& point = system->jList.ElementAt(i);

			if( point.wormholeObject == this )
			{
				point.fPoint.X = xform.translation.x / system->sizeX;
				point.fPoint.Y = xform.translation.y / system->sizeY;
				break;
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

UniqueID WormholeObject::GetID()
{
	return uniqueID;
}

//-----------------------------------------------------------------------------------------------------

void WormholeObject::Render()
{
	if( meshInstIndex != INVALID_INSTANCE_INDEX )
	{
// TODO: figure out why animation Pixlates planets
		SINGLE dt = Editor::deltaTime;
		ENGINE->update_instance(meshInstIndex, 0, dt);
		ANIM->update_instance(meshInstIndex,dt);

		ARCHETYPE_INDEX	archIdx = ENGINE->get_instance_archetype( meshInstIndex );

		COMPTR<IMesh> mesh;
		ENGINE->query_archetype_interface( archIdx, IID_IMesh, (IDAComponent**)&mesh );

		const float LODPERCENT = 1.0f;
		ENGINE->render_instance(CAMERALIB, meshInstIndex, 0, LODPERCENT, 0, NULL);
	}

//				const SINGLE dt = OBJLIST->GetRealRenderTime();
//				INSTANCE_INDEX id = ENGINE->get_instance_root(instanceIndex);
//				const Transform &trans = ENGINE->get_transform(id);
//				ANIM->update_instance(renderArch1->planetMeshObj->id,dt);
//				ENGINE->set_transform(renderArch1->planetMeshObj->id,trans);
//				ENGINE->update_instance(renderArch1->planetMeshObj->id,0,dt);
//				if (bMouseOver)
//				{
//					alphaMod = alphaMod * .9 + .35 * .1;
//				}
//				else
//				{
//					alphaMod = alphaMod * .9 + 1.0 * .1;
//				}
//				if (alphaMod > 1.0) alphaMod = 1.0;
//				DWORD alph = 255 * alphaMod;
//				TreeRender(renderArch1->planetMeshObj->mc, false, alph);
}

//-----------------------------------------------------------------------------------------------------

void WormholeObject::GetObjectData( struct ObjectData& _data )
{
	_data.archetype	   = archname;
	_data.xform		   = xform;
	_data.id		   = uniqueID;
	_data.gridSize	   = gridSize;
	_data.slotsNeeded  = 0;
	_data.stringHandle = stringHandle;
	_data.scriptHandle = scriptHandle;
	_data.objectClass  = data->basicData->objClass;
	_data.bJumpGate    = false;

	if( data->basicData->objClass == OC_PLATFORM )
	{
		_data.slotsNeeded = ((BASE_PLATFORM_DATA*)data->basicData)->slotsNeeded;

		if( ((BASE_PLATFORM_DATA*)data->basicData)->type == PC_JUMPPLAT )
		{
			_data.bJumpGate = true;
		}
	}

	_data.bUseDataOverride = bUseDataOverride;
	memcpy( &_data.dataOverride, &dataOverride, sizeof(MISSION_DATA_OVERRIDE) );
}

//-----------------------------------------------------------------------------------------------------

bool WormholeObject::Save( class TiXmlNode& _node )
{
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool WormholeObject::Load( class TiXmlNode& _node )
{
	return false;
}

//-----------------------------------------------------------------------------------------------------

bool WormholeObject::Save( struct IFileSystem& _filesystem )
{

	return true;
}

//-----------------------------------------------------------------------------------------------------

bool WormholeObject::Load( struct IFileSystem& )
{
	return 0;
}

//-----------------------------------------------------------------------------------------------------

void WormholeObject::Delete()
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

bool WormholeObject::AddTab( HWND _tabCtrl )
{
//	HWND hTab = 0;
//	HINSTANCE hInstance = ::AfxGetApp()->m_hInstance;
//	LPSTR label = 0;
//
//	if( data->basicData->objClass == OC_PLANETOID )
//	{
//		hTab  = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_OP_BASIC), _tabCtrl, (DLGPROC)tabProc_planet, (DWORD)this );
//		label = "Planet";
//	}
//	else if( data->basicData->objClass == OC_SPACESHIP )
//	{
//		hTab  = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_OP_BASIC), _tabCtrl, (DLGPROC)tabProc_ship, (DWORD)this );
//		label = "Ship";
//	}
//	else if( data->basicData->objClass == OC_PLATFORM )
//	{
//		hTab  = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_OP_BASIC), _tabCtrl, (DLGPROC)tabProc_platform, (DWORD)this );
//		label = "Platform";
//	}
//	else if( data->basicData->objClass == OC_BLACKHOLE )
//	{
//		hTab  = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_OP_BLACKHOLE), _tabCtrl, (DLGPROC)tabProc_blackhole, (DWORD)this );
//		label = "Blackhole";
//	}
//
//	CRect tabsRect;
//	TabCtrl_AdjustRect(_tabCtrl, true, tabsRect );
//	tabsRect.SetRect( 0, 0, 16, 24 );
//
//	CRect rect;
//	::GetClientRect( hTab, rect );
//	rect.OffsetRect( tabsRect.Width(), tabsRect.Height() );
//	::SetWindowPos( hTab, NULL, rect.left, rect.top, rect.Width(), rect.Height(), 0 );
//
//	TCITEM itemTab;
//	itemTab.mask = TCIF_PARAM | TCIF_TEXT;
//	itemTab.pszText = label;
//	itemTab.cchTextMax = strlen(itemTab.pszText);
//	itemTab.iImage = -1;
//	itemTab.lParam = (DWORD)hTab;
//	TabCtrl_InsertItem( _tabCtrl,  TabCtrl_GetItemCount(_tabCtrl),  &itemTab );
//
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool WormholeObject::create( const char* _archname )
{
	BASIC_DATA* basicData = (BASIC_DATA*)GAMETYPES->GetArchetypeData( _archname );
	if( !basicData )
	{
		return false;
	}

	// loading up the archetype data
	data = Object::getArchetypeData( _archname, "jgate_sphere.3db", basicData );

	// creating an instance of this model
	if( !data )
	{
		return false;
	}

	archname = _archname;

	BT_JUMPGATE_DATA* wormholeData = (BT_JUMPGATE_DATA*)basicData;
	setMissionData( wormholeData->missionData );

	// load name
	scriptHandle.LoadString( hStringTable, wormholeData->missionData.displayName, 1033 );
	createName( scriptHandle );

	gridSize.SetPoint(2,2);

	meshInstIndex = ENGINE->create_instance2( data->meshIndex, NULL );
	if( meshInstIndex == INVALID_INSTANCE_INDEX )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

void WormholeObject::createName( CString& _name )
{
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

void WormholeObject::setMissionData( MISSION_DATA& _missionData )
{
	memcpy( &dataOverride.armorData, &_missionData.armorData, sizeof(dataOverride.armorData) );
	memcpy( &dataOverride.baseShieldLevel, &_missionData.baseShieldLevel, sizeof(dataOverride.baseShieldLevel) );
	memcpy( &dataOverride.buildTime, &_missionData.buildTime, sizeof(dataOverride.buildTime) );
	memcpy( &dataOverride.cloakedSensorRadius, &_missionData.cloakedSensorRadius, sizeof(dataOverride.cloakedSensorRadius) );
	memcpy( &dataOverride.hullPointsMax, &_missionData.hullPointsMax, sizeof(dataOverride.hullPointsMax) );
	memcpy( &dataOverride.maxVelocity, &_missionData.maxVelocity, sizeof(dataOverride.maxVelocity) );
	memcpy( &dataOverride.scrapValue, &_missionData.scrapValue, sizeof(dataOverride.scrapValue) );
	memcpy( &dataOverride.sensorRadius, &_missionData.sensorRadius, sizeof(dataOverride.sensorRadius) );
	memcpy( &dataOverride.supplyPointsMax, &_missionData.supplyPointsMax, sizeof(dataOverride.supplyPointsMax) );

	dataOverride.commandPoints = _missionData.resourceCost.commandPt;
}

//-----------------------------------------------------------------------------------------------------
// tab controls

INT_PTR CALLBACK WormholeObject::tabProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
//	DWORD ids[] = 
//	{ IDC_CHECK_SYS1, IDC_CHECK_SYS2, IDC_CHECK_SYS3, IDC_CHECK_SYS4, IDC_CHECK_SYS5, IDC_CHECK_SYS6, IDC_CHECK_SYS7, IDC_CHECK_SYS8,
//      IDC_CHECK_SYS9, IDC_CHECK_SYS10, IDC_CHECK_SYS11, IDC_CHECK_SYS12, IDC_CHECK_SYS13, IDC_CHECK_SYS14, IDC_CHECK_SYS15, IDC_CHECK_SYS16,
//	};
//
//	if( uMsg == WM_INITDIALOG )
//	{
//		SpaceObject* obj = (SpaceObject*)lParam;
//		SetWindowLong( hwndDlg, GWL_USERDATA, (LONG)obj );
//
//		for( int i = 0; i < sizeof(ids); i++ )
//		{
//			Button_SetCheck( GetDlgItem(hwndDlg,ids[i]),  obj->dataBlock.blackHoleSave.targetSys[i] ? BST_CHECKED : BST_UNCHECKED);
//		}
//	}
//	else if( uMsg == WM_SHOWWINDOW )
//	{
//		if( !wParam ) // being hidden
//		{
//			SpaceObject* obj = (SpaceObject*)GetWindowLong( hwndDlg, GWL_USERDATA );
//
//			for( int i = 0; i < sizeof(ids); i++ )
//			{
//				obj->dataBlock.blackHoleSave.targetSys[i] = Button_GetCheck( GetDlgItem(hwndDlg,ids[i]) ) == BST_CHECKED;
//			}
//		}
//	}
//
	return 0;
}

//-----------------------------------------------------------------------------------------------------
// startup

struct WormholeObjectFactory : public IComponentFactory
{
	BEGIN_DACOM_MAP_INBOUND(WormholeObjectFactory)
		DACOM_INTERFACE_ENTRY(IComponentFactory)
	END_DACOM_MAP()

	// IComponentFactory

	virtual GENRESULT COMAPI CreateInstance (DACOMDESC *descriptor, void **instance)
	{
		*instance = NULL;

		if( descriptor->size == sizeof(OBJECT_DACOMDESC) )
		{
			OBJECT_DACOMDESC* objectDacomdesc = (OBJECT_DACOMDESC*)descriptor;

			WormholeObject* object = new DAComponent<WormholeObject>;
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

struct _wormholeobjectfactory : GlobalComponent
{
	WormholeObjectFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<WormholeObjectFactory>;
		AddToGlobalCleanupList((IDAComponent **) &factory);
	}

	virtual void Initialize (void)
	{
		DACOM->RegisterComponent( factory, "WormholeObject", 0);
	}
};

static _wormholeobjectfactory __wormholeobjectfactory;
