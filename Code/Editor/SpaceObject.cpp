//
// SpaceObject.cpp
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
#include "Clipboard.h"

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
#include <afxpriv.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Object;

//-----------------------------------------------------------------------------------------------------

struct SpaceObject : public IObject, public ISaverLoader, public IClipboardObject
{
	BEGIN_DACOM_MAP_INBOUND(SpaceObject)
		DACOM_INTERFACE_ENTRY(IObject)
		DACOM_INTERFACE_ENTRY(ISaverLoader)
		DACOM_INTERFACE_ENTRY(IClipboardObject)
	END_DACOM_MAP()

	union Block
	{
		MT_QSHIPLOAD       shipSave;
		MT_PLATFORM_QLOAD  platSave;
		MT_PLANET_QLOAD    planetSave;
		MT_BLACKHOLE_QLOAD blackHoleSave;
	};

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
	Block                 dataBlock;

	SpaceObject() : 
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
	{
		memset( &dataBlock, 0, sizeof(dataBlock) );
	}

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
		// user definition of script handle means the MISSION_DATA_OVERRIDE chunk will be written to UTF file
		bUseDataOverride = true;
		scriptHandle = _scriptHandle;

		// for exporting latter
		strncpy( dataOverride.scriptHandle, _scriptHandle, countof(dataOverride.scriptHandle)-1 );
		dataOverride.scriptHandle[countof(dataOverride.scriptHandle)-1] = 0;
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
		resetMissionData();
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
	bool createPlanet( BT_PLANET_DATA*, const char* _archname );
	bool createPlatform( BASE_PLATFORM_DATA*, const char* _archname );
	bool createShip( BASE_SPACESHIP_DATA*, const char* _archname );
	bool createBlackHole( BT_BLACKHOLE_DATA*, const char* _archname ); 

	void createName( CString& _name );
	void setMissionData( MISSION_DATA& );
	void resetMissionData( void );
	bool putIntoValidSpot();

	static INT_PTR CALLBACK tabProc_planet( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static INT_PTR CALLBACK tabProc_ship( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static INT_PTR CALLBACK tabProc_platform( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
	static INT_PTR CALLBACK tabProc_blackhole( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
};

//-----------------------------------------------------------------------------------------------------

Transform& SpaceObject::GetTransform()
{
	return xform;
}

//-----------------------------------------------------------------------------------------------------

bool SpaceObject::SetTransform( Transform& _xform )
{
	xform = _xform;
	if( meshInstIndex != INVALID_INSTANCE_INDEX )
	{
		ENGINE->set_transform(meshInstIndex, xform);
	}
	return true;
}

//-----------------------------------------------------------------------------------------------------

UniqueID SpaceObject::GetID()
{
	return uniqueID;
}

//-----------------------------------------------------------------------------------------------------

void SpaceObject::Render()
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

void SpaceObject::GetObjectData( struct ObjectData& _data )
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
	_data.playerID     = (U8)playerID;

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

bool SpaceObject::Save( class TiXmlNode& _node )
{
	TiXmlElement spaceObject("OBJECT");

	Export::XML::transform( xform, spaceObject );

	spaceObject.SetAttribute("SpaceObject", archname );
	spaceObject.SetAttribute("systemID", systemID );
	spaceObject.SetAttribute("playerID", playerID );
	spaceObject.SetAttribute("stringHandle", stringHandle );
	spaceObject.SetAttribute("scriptHandle", scriptHandle );

	if( bUseDataOverride )
	{
		TiXmlElement missionDataOverride("MISSION_DATA_OVERRIDE");

		missionDataOverride.SetAttribute("myArmor", (U32)dataOverride.armorData.myArmor );
		missionDataOverride.SetAttribute("NO_ARMOR", (SINGLE)dataOverride.armorData._damageTable[NO_ARMOR] );
		missionDataOverride.SetAttribute("LIGHT_ARMOR", (SINGLE)dataOverride.armorData._damageTable[LIGHT_ARMOR] );
		missionDataOverride.SetAttribute("MEDIUM_ARMOR", (SINGLE)dataOverride.armorData._damageTable[MEDIUM_ARMOR] );
		missionDataOverride.SetAttribute("HEAVY_ARMOR", (SINGLE)dataOverride.armorData._damageTable[HEAVY_ARMOR] );
		missionDataOverride.SetAttribute("hullPointsMax", (U32)dataOverride.hullPointsMax );
		missionDataOverride.SetAttribute("supplyPointsMax", (U32)dataOverride.supplyPointsMax );
		missionDataOverride.SetAttribute("scrapValue", (U32)dataOverride.scrapValue );
		missionDataOverride.SetAttribute("commandPoints", (U32)dataOverride.commandPoints );
		missionDataOverride.SetAttribute("sensorRadius", (SINGLE)dataOverride.sensorRadius );
		missionDataOverride.SetAttribute("cloakedSensorRadius", (SINGLE)dataOverride.cloakedSensorRadius );
		missionDataOverride.SetAttribute("maxVelocity", (SINGLE)dataOverride.maxVelocity );
		missionDataOverride.SetAttribute("baseShieldLevel", (SINGLE)dataOverride.baseShieldLevel );

		spaceObject.InsertEndChild( missionDataOverride );
	}

	_node.InsertEndChild(spaceObject);
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool SpaceObject::Load( class TiXmlNode& _node )
{
	TiXmlElement* spaceObject = _node.ToElement();

	if( spaceObject && spaceObject->Attribute("SpaceObject") )
	{
		// for pasted objects
		if( archname == "" )
		{
			create( spaceObject->Attribute("SpaceObject") );
		}

		Import::XML::transform( xform, spaceObject->FirstChild("TRANSFORM") );
		SetTransform( xform );

		if( spaceObject->Attribute("scriptHandle") )
		{
			scriptHandle = spaceObject->Attribute("scriptHandle");
		}

		spaceObject->QueryUnsignedIntValue("systemID", &systemID );
		spaceObject->QueryUnsignedIntValue("playerID", &playerID );
		spaceObject->QueryUnsignedIntValue("stringHandle", &stringHandle );

		TiXmlElement* override = spaceObject->FirstChildElement("MISSION_DATA_OVERRIDE");
		if( override )
		{
			bUseDataOverride = true;

			dataOverride.armorData.myArmor					  = (ARMOR_TYPE)override->GetAttributeUnsignedLong("myArmor");
			dataOverride.armorData._damageTable[NO_ARMOR]	  = override->GetAttributeFloat("NO_ARMOR");
			dataOverride.armorData._damageTable[LIGHT_ARMOR]  = override->GetAttributeFloat("LIGHT_ARMOR");
			dataOverride.armorData._damageTable[MEDIUM_ARMOR] = override->GetAttributeFloat("MEDIUM_ARMOR");
			dataOverride.armorData._damageTable[HEAVY_ARMOR]  = override->GetAttributeFloat("HEAVY_ARMOR");
			dataOverride.hullPointsMax						  = override->GetAttributeUnsignedLong("hullPointsMax");
			dataOverride.supplyPointsMax					  = override->GetAttributeUnsignedLong("supplyPointsMax");
			dataOverride.scrapValue							  = override->GetAttributeUnsignedLong("scrapValue");
			dataOverride.commandPoints						  = override->GetAttributeUnsignedLong("commandPoints");
			dataOverride.sensorRadius						  = override->GetAttributeFloat("sensorRadius");
			dataOverride.cloakedSensorRadius				  = override->GetAttributeFloat("cloakedSensorRadius");
			dataOverride.maxVelocity						  = override->GetAttributeFloat("maxVelocity");
			dataOverride.baseShieldLevel					  = override->GetAttributeFloat("baseShieldLevel");
		}

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

bool SpaceObject::Save( struct IFileSystem& _filesystem )
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
			MT_QSHIPLOAD       shipSave;
			MT_PLATFORM_QLOAD  platSave;
			MT_PLANET_QLOAD    planetSave;
			MT_BLACKHOLE_QLOAD blackHoleSave;
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
		case OC_PLANETOID:
		{
			BT_PLANET_DATA* pData = (BT_PLANET_DATA*)data->basicData;

			// prepare data
			dataBlock.block.planetSave.pos.init( xform.translation, systemID );

			// log size
			dataBlock.size = sizeof(MT_PLANET_QLOAD);

			_filesystem.CreateDirectory("MT_PLANET_QLOAD");
			if (_filesystem.SetCurrentDirectory("MT_PLANET_QLOAD") == 0)
			{
				CQERROR0("QuickSave failed on Directory 'MT_PLANET_QLOAD'");
				return false;
			}
			break;
		}

		case OC_PLATFORM:
		{
			BASE_PLATFORM_DATA* pData = (BASE_PLATFORM_DATA*)data->basicData;

			// prepare data
			dataBlock.block.platSave.dwMissionID = playerID;
			dataBlock.block.platSave.position.init( xform.translation, systemID );

			// log size
			dataBlock.size = sizeof(MT_PLATFORM_QLOAD);

			_filesystem.CreateDirectory("MT_PLATFORM_QLOAD");
			if (_filesystem.SetCurrentDirectory("MT_PLATFORM_QLOAD") == 0)
			{
				CQERROR0("QuickSave failed on Directory 'MT_PLATFORM_QLOAD'");
				return false;
			}
			break;
		}

		case OC_SPACESHIP:
		{
			BASE_SPACESHIP_DATA* pData = (BASE_SPACESHIP_DATA*)data->basicData;

			// log size
			dataBlock.size = sizeof(MT_QSHIPLOAD);

			// prepare data
			dataBlock.block.shipSave.pos.init( xform.translation, systemID );
			dataBlock.block.shipSave.yaw = xform.get_yaw();
			dataBlock.block.shipSave.dwMissionID = playerID;

			_filesystem.CreateDirectory("MT_QSHIPLOAD");
			if (_filesystem.SetCurrentDirectory("MT_QSHIPLOAD") == 0)
			{
				CQERROR0("QuickSave failed on Directory 'MT_QSHIPLOAD'");
				return false;
			}
			break;
		}

		case OC_BLACKHOLE:
		{
			BT_BLACKHOLE_DATA* pData = (BT_BLACKHOLE_DATA*)data->basicData;

			// log size
			dataBlock.size = sizeof(MT_BLACKHOLE_QLOAD);

			// prepare data
			dataBlock.block.blackHoleSave.pos.init( xform.translation, systemID );
			dataBlock.block.blackHoleSave.numTargetSys = this->dataBlock.blackHoleSave.numTargetSys;
			memcpy( &dataBlock.block.blackHoleSave.targetSys, &this->dataBlock.blackHoleSave.targetSys, sizeof(dataBlock.block.blackHoleSave.targetSys) );

			_filesystem.CreateDirectory("MT_BLACKHOLE_QLOAD");
			if (_filesystem.SetCurrentDirectory("MT_BLACKHOLE_QLOAD") == 0)
			{
				CQERROR0("QuickSave failed on Directory 'MT_BLACKHOLE_QLOAD'");
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

	COMPTR<IFileSystem> f;
	if( _filesystem.CreateInstance(&fdesc,f) != GR_OK )
	{
		return false;
	}

	DWORD dwWritten;
	f->WriteFile(0, &dataBlock.block, dataBlock.size, &dwWritten, 0);

	// write out any overridden info
	if( bUseDataOverride )
	{
		DAFILEDESC desc = "MISSION_DATA_OVERRIDE";

		desc.lpImplementation = "UTF";
		desc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
		desc.dwShareMode = 0;  // no sharing
		desc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

		HANDLE hChild = f->OpenChild(&desc);
		if( hChild != INVALID_HANDLE_VALUE )
		{
			f->WriteFile( hChild, &dataOverride, sizeof(dataOverride), &dwWritten );
			f->CloseHandle( hChild );
		}
	}

	_filesystem.SetCurrentDirectory(".."); // out of MT_?? dir
	_filesystem.SetCurrentDirectory(".."); // out of archname dir
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool SpaceObject::Load( struct IFileSystem& )
{
	return 0;
}

//-----------------------------------------------------------------------------------------------------

void SpaceObject::Delete()
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

bool SpaceObject::AddTab( HWND _tabCtrl )
{
	HWND hTab = 0;
	HINSTANCE hInstance = ::AfxGetApp()->m_hInstance;
	LPSTR label = 0;

	if( data->basicData->objClass == OC_PLANETOID )
	{
		hTab  = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_OP_BASIC), _tabCtrl, (DLGPROC)tabProc_planet, (DWORD)this );
		label = "Planet";
	}
	else if( data->basicData->objClass == OC_SPACESHIP )
	{
		hTab  = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_OP_BASIC), _tabCtrl, (DLGPROC)tabProc_ship, (DWORD)this );
		label = "Ship";
	}
	else if( data->basicData->objClass == OC_PLATFORM )
	{
		hTab  = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_OP_BASIC), _tabCtrl, (DLGPROC)tabProc_platform, (DWORD)this );
		label = "Platform";
	}
	else if( data->basicData->objClass == OC_BLACKHOLE )
	{
		hTab  = ::CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_OP_BLACKHOLE), _tabCtrl, (DLGPROC)tabProc_blackhole, (DWORD)this );
		label = "Blackhole";
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

bool SpaceObject::create( const char* _archname )
{
	// creating a dummy object?
	if( _archname == "" )
	{
		archname = "";
		return true;
	}

	BASIC_DATA* pData = (BASIC_DATA*)GAMETYPES->GetArchetypeData( _archname );
	if( !pData )
	{
		return false;
	}

	archname = _archname;

	if( pData->objClass == OC_PLANETOID )
	{
		return createPlanet( (BT_PLANET_DATA*)pData, _archname );
	}
	else if( pData->objClass == OC_SPACESHIP )
	{
		return createShip( (BASE_SPACESHIP_DATA*)pData, _archname );
	}
	else if( pData->objClass == OC_PLATFORM )
	{
		return createPlatform( (BASE_PLATFORM_DATA*)pData, _archname );
	}
	else if( pData->objClass == OC_BLACKHOLE )
	{
		return createBlackHole( (BT_BLACKHOLE_DATA*)pData, _archname );
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

void SpaceObject::createName( CString& _name )
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

bool SpaceObject::createPlanet( BT_PLANET_DATA* _planetData, const char* _archname )
{
	// loading up the archetype data
	data = Object::getArchetypeData( _archname, _planetData->fileName, _planetData );

	// creating an instance of this model
	if( data )
	{
		resetMissionData();

		// load name
		scriptHandle.LoadString( hStringTable, _planetData->missionData.displayName, 1033 );
		createName( scriptHandle );

		gridSize.SetPoint(4,4);
		meshInstIndex = ENGINE->create_instance2( data->meshIndex, NULL );
		return( meshInstIndex != INVALID_INSTANCE_INDEX );
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

bool SpaceObject::createPlatform( BASE_PLATFORM_DATA* _platformData, const char* _archname )
{
	// loading up the archetype data
	data = Object::getArchetypeData( _archname, _platformData->fileName, _platformData );

	// creating an instance of this model
	if( data )
	{
		resetMissionData();

		// load name
		scriptHandle.LoadString( hStringTable, _platformData->missionData.displayName, 1033 );
		createName( scriptHandle );

		if( _platformData->slotsNeeded )
		{
			// free floating, until near planet
			gridSize.SetPoint(0,0);
		}
		else if( _platformData->missionData.mObjClass == M_JUMPPLAT )
		{
			// free floating, until near wormhole
			gridSize.SetPoint(0,0);
		}
		else if( _platformData->size )
		{
			gridSize.SetPoint(_platformData->size,_platformData->size);
		}

		meshInstIndex = ENGINE->create_instance2( data->meshIndex, NULL );
		return( meshInstIndex != INVALID_INSTANCE_INDEX );
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

bool SpaceObject::createShip( BASE_SPACESHIP_DATA* _spaceShipData, const char* _archname )
{
	// loading up the archetype data
	data = Object::getArchetypeData( _archname, _spaceShipData->fileName, _spaceShipData );

	// creating an instance of this model
	if( data )
	{
		resetMissionData();

		// load name
		scriptHandle.LoadString( hStringTable, _spaceShipData->missionData.displayName, 1033 );
		createName( scriptHandle );

		if( _spaceShipData->bLargeShip )
		{
			gridSize.SetPoint(2,2);
		}
		meshInstIndex = ENGINE->create_instance2( data->meshIndex, NULL );
		return( meshInstIndex != INVALID_INSTANCE_INDEX );
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

bool SpaceObject::createBlackHole( BT_BLACKHOLE_DATA* _blackHoleData, const char* _archname )
{
	// loading up the archetype data
	data = Object::getArchetypeData( _archname, _blackHoleData->ringObjectName, _blackHoleData );

	// creating an instance of this model
	if( data )
	{
		resetMissionData();

		// load name
		scriptHandle.LoadString( hStringTable, _blackHoleData->missionData.displayName, 1033 );
		createName( scriptHandle );

		gridSize.SetPoint(4,4);
		meshInstIndex = ENGINE->create_instance2( data->meshIndex, NULL );
		return( meshInstIndex != INVALID_INSTANCE_INDEX );
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

void SpaceObject::resetMissionData( void )
{
	if( !data )
		return;

	// record object's default mission data (and reset the "bUseDataOverride" flag back to false)
	MISSION_DATA* missionData = NULL;

	if( data->basicData->objClass == OC_PLANETOID )
	{
		missionData = &((BT_PLANET_DATA*)data->basicData)->missionData;
	}
	else if( data->basicData->objClass == OC_SPACESHIP )
	{
		missionData = &((BASE_SPACESHIP_DATA*)data->basicData)->missionData;
	}
	else if( data->basicData->objClass == OC_PLATFORM )
	{
		missionData = &((BASE_PLATFORM_DATA*)data->basicData)->missionData;
	}
	else if( data->basicData->objClass == OC_BLACKHOLE )
	{
		missionData = &((BT_BLACKHOLE_DATA*)data->basicData)->missionData;
	}

	if( missionData )
	{
		bUseDataOverride = false;
		setMissionData( *missionData );
	}
}

//-----------------------------------------------------------------------------------------------------

void SpaceObject::setMissionData( MISSION_DATA& _missionData )
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

INT_PTR CALLBACK SpaceObject::tabProc_blackhole( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	DWORD ids[] = 
	{ IDC_CHECK_SYS1, IDC_CHECK_SYS2, IDC_CHECK_SYS3, IDC_CHECK_SYS4, IDC_CHECK_SYS5, IDC_CHECK_SYS6, IDC_CHECK_SYS7, IDC_CHECK_SYS8,
      IDC_CHECK_SYS9, IDC_CHECK_SYS10, IDC_CHECK_SYS11, IDC_CHECK_SYS12, IDC_CHECK_SYS13, IDC_CHECK_SYS14, IDC_CHECK_SYS15, IDC_CHECK_SYS16,
	};

	if( uMsg == WM_INITDIALOG )
	{
		SpaceObject* obj = (SpaceObject*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, (LONG)obj );

		for( int i = 0; i < sizeof(ids); i++ )
		{
			Button_SetCheck( GetDlgItem(hwndDlg,ids[i]),  obj->dataBlock.blackHoleSave.targetSys[i] ? BST_CHECKED : BST_UNCHECKED);
		}
	}
	else if( uMsg == WM_SHOWWINDOW )
	{
		if( !wParam ) // being hidden
		{
			SpaceObject* obj = (SpaceObject*)GetWindowLong( hwndDlg, GWL_USERDATA );

			for( int i = 0; i < sizeof(ids); i++ )
			{
				obj->dataBlock.blackHoleSave.targetSys[i] = Button_GetCheck( GetDlgItem(hwndDlg,ids[i]) ) == BST_CHECKED;
			}
		}
	}

	return 0;
}

//----------------------------------------------------------------------------------------------

INT_PTR CALLBACK SpaceObject::tabProc_planet( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_INITDIALOG )
	{
		SpaceObject* obj = (SpaceObject*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, (LONG)obj );
	}
	else if( uMsg == WM_SHOWWINDOW )
	{
	}

	return 0;
}

//----------------------------------------------------------------------------------------------

INT_PTR CALLBACK SpaceObject::tabProc_ship( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_INITDIALOG )
	{
		SpaceObject* obj = (SpaceObject*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, (LONG)obj );
	}
	else if( uMsg == WM_SHOWWINDOW )
	{
	}

	return 0;
}

//----------------------------------------------------------------------------------------------

INT_PTR CALLBACK SpaceObject::tabProc_platform( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_INITDIALOG )
	{
		SpaceObject* obj = (SpaceObject*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, (LONG)obj );
	}
	else if( uMsg == WM_SHOWWINDOW )
	{
	}

	return 0;
}

//-----------------------------------------------------------------------------------------------------
// IClipboardObject

const char* SpaceObject::GetType()
{
	return "SpaceObject";
}

bool SpaceObject::Copy( CSharedFile& _memfile )
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

bool SpaceObject::Paste( CSharedFile& _memfile )
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

bool SpaceObject::Append( CSharedFile& _memfile )
{
	return true;
}

//-----------------------------------------------------------------------------------------------------
// startup

struct SpaceObjectFactory : public IComponentFactory
{
	BEGIN_DACOM_MAP_INBOUND(SpaceObjectFactory)
		DACOM_INTERFACE_ENTRY(IComponentFactory)
	END_DACOM_MAP()

	// IComponentFactory

	virtual GENRESULT COMAPI CreateInstance (DACOMDESC *descriptor, void **instance)
	{
		*instance = NULL;

		if( descriptor->size == sizeof(OBJECT_DACOMDESC) )
		{
			OBJECT_DACOMDESC* objectDacomdesc = (OBJECT_DACOMDESC*)descriptor;

			SpaceObject* object = new DAComponent<SpaceObject>;
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

struct _spaceobjectfactory : GlobalComponent
{
	SpaceObjectFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<SpaceObjectFactory>;
		AddToGlobalCleanupList((IDAComponent **) &factory);
	}

	virtual void Initialize (void)
	{
		DACOM->RegisterComponent( factory, "SpaceObject", 0);
	}
};

static _spaceobjectfactory __spaceobjectfactory;
