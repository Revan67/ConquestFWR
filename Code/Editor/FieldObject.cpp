//
// FieldObject.cpp
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
#include "FieldManager.h"
#include "Clipboard.h"
#include "Scenario.h"
#include "ObjectFamily.h"
#include "Campaign.h"

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

struct FieldObject : public IObject, public ISaverLoader, public IClipboardObject
{
	BEGIN_DACOM_MAP_INBOUND(FieldObject)
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
	U32                   stringHandle;
	bool                  bUseDataOverride;
	MISSION_DATA_OVERRIDE dataOverride;
	void*                 customData;
	U32                   customDataSize;

	FieldObject() : 
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

	// IClipboardObject

	virtual const char* GetType();
	virtual bool Copy( CSharedFile& _memfile );
	virtual bool Paste( CSharedFile& _memfile );
	virtual bool Append( CSharedFile& _memfile ); 

	// local

	bool create( const char* _archname );
	void createName( CString& _name );
	void setMissionData( MISSION_DATA& );

	static INT_PTR CALLBACK tabProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

	MISSION_DATA* getMissionData()
	{
		if( !data ) return NULL;

		BASE_FIELD_DATA* fieldData = (BASE_FIELD_DATA*)data->basicData;

		if( fieldData->fieldClass == FC_ASTEROIDFIELD )
		{
			return &((BT_ASTEROIDFIELD_DATA*)data->basicData)->missionData;
		}
		else if( fieldData->fieldClass == FC_MINEFIELD )
		{
			return NULL;
		}
		else if( fieldData->fieldClass == FC_NEBULA )
		{
			return &((BT_NEBULA_DATA*)data->basicData)->missionData;
		}
		else if( fieldData->fieldClass == FC_ANTIMATTER )
		{
			return &((BT_ANTIMATTER_DATA*)data->basicData)->missionData;
		}

		return NULL;
	}
};

//-----------------------------------------------------------------------------------------------------

Transform& FieldObject::GetTransform()
{
	return xform;
}

//-----------------------------------------------------------------------------------------------------

bool FieldObject::SetTransform( Transform& _xform )
{
	xform = _xform;
	if( meshInstIndex != INVALID_INSTANCE_INDEX )
	{
		ENGINE->set_transform(meshInstIndex, xform);
	}
	return true;
}

//-----------------------------------------------------------------------------------------------------

UniqueID FieldObject::GetID()
{
	return uniqueID;
}

//-----------------------------------------------------------------------------------------------------

void FieldObject::Render()
{
	// set up default system rendering states

	CAMERA->SetModelView();

	PIPE->set_texture_stage_texture(0,0);
	PIPE->set_texture_stage_state( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
	PIPE->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
	PIPE->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );

	PIPE->set_texture_stage_texture(1,0);
	PIPE->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	PIPE->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	PIPE->set_render_state( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	PIPE->set_render_state( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	PIPE->set_render_state( D3DRS_ALPHATESTENABLE, TRUE );  
	PIPE->set_render_state( D3DRS_ALPHABLENDENABLE,TRUE);
	PIPE->set_render_state( D3DRS_ALPHAREF, 0x02 );  
	PIPE->set_render_state( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
	PIPE->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE );

	SINGLE cellSize = (GRIDSIZE / 4) * gridSize.x;

	NETGRIDVECTOR grid;
	grid.init( xform.translation, 0 );
	grid.centerpos();

	FPoint point;
	point.X = grid.getX() * GRIDSIZE;
	point.Y = grid.getY() * GRIDSIZE;

	PB.Begin(PB_QUADS);

		switch( ((BASE_FIELD_DATA*)data->basicData)->fieldClass )
		{
			case FC_ASTEROIDFIELD: PB.Color4ub( 255, 255,  64, 64 ); break;
			case FC_MINEFIELD:     PB.Color4ub(  64, 255,  64, 64 ); break;
			case FC_NEBULA:        PB.Color4ub( 255,  64,  64, 64 ); break;
			case FC_ANTIMATTER:    PB.Color4ub( 255,  64, 255, 64 ); break;
		}

		PB.Vertex3f(point.X - cellSize, point.Y - cellSize, 0);
		PB.Vertex3f(point.X - cellSize, point.Y + cellSize, 0);
		PB.Vertex3f(point.X + cellSize, point.Y + cellSize, 0);
		PB.Vertex3f(point.X + cellSize, point.Y - cellSize, 0);

	PB.End();
}

//-----------------------------------------------------------------------------------------------------

void FieldObject::GetObjectData( struct ObjectData& _data )
{
	_data.archetype	   = archname;
	_data.xform		   = xform;
	_data.id		   = uniqueID;
	_data.gridSize	   = gridSize;
	_data.slotsNeeded  = 0;
	_data.stringHandle = stringHandle;
	_data.scriptHandle = scriptHandle;
	_data.objectClass  = OC_FIELD;
	_data.bJumpGate    = false;

	_data.bUseDataOverride = bUseDataOverride;
	memcpy( &_data.dataOverride, &dataOverride, sizeof(MISSION_DATA_OVERRIDE) );
}

//-----------------------------------------------------------------------------------------------------

bool FieldObject::Save( class TiXmlNode& _node )
{
	TiXmlElement fieldObject("OBJECT");

	Export::XML::transform( xform, fieldObject );

	fieldObject.SetAttribute("FieldObject", archname );
	fieldObject.SetAttribute("systemID", systemID );
	fieldObject.SetAttribute("playerID", playerID );
	fieldObject.SetAttribute("stringHandle", stringHandle );
	fieldObject.SetAttribute("scriptHandle", scriptHandle );

	_node.InsertEndChild(fieldObject);
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool FieldObject::Load( class TiXmlNode& _node )
{
	TiXmlElement* fieldObject = _node.ToElement();

	if( fieldObject && fieldObject->Attribute("FieldObject") )
	{
		// for pasted objects
		if( archname == "" )
		{
			create( fieldObject->Attribute("FieldObject") );
		}

		Import::XML::transform( xform, fieldObject->FirstChild("TRANSFORM") );
		SetTransform( xform );

		if( fieldObject->Attribute("scriptHandle") )
		{
			scriptHandle = fieldObject->Attribute("scriptHandle");
		}

		systemID	 = fieldObject->GetAttributeLong("systemID");
		playerID	 = fieldObject->GetAttributeLong("playerID");
		stringHandle = fieldObject->GetAttributeLong("stringHandle");

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

bool FieldObject::Save( struct IFileSystem& _filesystem )
{
	// delay saving out this info until all fields are reported
	return FIELDMANAGER->Insert( this );
}

//----------------------------------------------------------------------------------------------

bool FieldObject::Load( struct IFileSystem& )
{
	return 0;
}

//-----------------------------------------------------------------------------------------------------

void FieldObject::Delete()
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

bool FieldObject::AddTab( HWND _tabCtrl )
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

bool FieldObject::create( const char* _archname )
{
	// creating a dummy object?
	if( _archname == "" )
	{
		archname = "";
		return true;
	}

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
	gridSize.SetPoint(2,2);

	MISSION_DATA* missionData = getMissionData();
	if( missionData )
	{
		setMissionData( *missionData );
		scriptHandle.LoadString( hStringTable, missionData->displayName, 1033 );
		createName( scriptHandle );
	}

//	meshInstIndex = ENGINE->create_instance2( data->meshIndex, NULL );
//	if( meshInstIndex == INVALID_INSTANCE_INDEX )
//	{
//		return false;
//	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

void FieldObject::createName( CString& _name )
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

void FieldObject::setMissionData( MISSION_DATA& _missionData )
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

INT_PTR CALLBACK FieldObject::tabProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
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
// IClipboardObject

const char* FieldObject::GetType()
{
	return "FieldObject";
}

bool FieldObject::Copy( CSharedFile& _memfile )
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

bool FieldObject::Paste( CSharedFile& _memfile )
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

bool FieldObject::Append( CSharedFile& _memfile )
{
	return true;
}

//-----------------------------------------------------------------------------------------------------
// startup

struct FieldObjectFactory : public IComponentFactory
{
	BEGIN_DACOM_MAP_INBOUND(FieldObjectFactory)
		DACOM_INTERFACE_ENTRY(IComponentFactory)
	END_DACOM_MAP()

	// IComponentFactory

	virtual GENRESULT COMAPI CreateInstance (DACOMDESC *descriptor, void **instance)
	{
		*instance = NULL;

		if( descriptor->size == sizeof(OBJECT_DACOMDESC) )
		{
			OBJECT_DACOMDESC* objectDacomdesc = (OBJECT_DACOMDESC*)descriptor;

			FieldObject* object = new DAComponent<FieldObject>;
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

struct _FieldObjectfactory : GlobalComponent
{
	FieldObjectFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<FieldObjectFactory>;
		AddToGlobalCleanupList((IDAComponent **) &factory);
	}

	virtual void Initialize (void)
	{
		DACOM->RegisterComponent( factory, "FieldObject", 0);
	}
};

static _FieldObjectfactory __FieldObjectfactory;

//-----------------------------------------------------------------------------------------------------
// The FieldManager
//-----------------------------------------------------------------------------------------------------

struct FieldManager : public IFieldManager
{
	BEGIN_DACOM_MAP_INBOUND(FieldManager)
		DACOM_INTERFACE_ENTRY(IFieldManager)
	END_DACOM_MAP()

	// IFieldManager

	virtual bool Reset( System* _system );
	virtual bool Insert( IObject* _field );
	virtual bool Save( IFileSystem& _fileSystem );
	virtual bool Load( IFileSystem& _fileSystem );

	// locals

	#define FIELDNAME "field_%d"

	struct FieldMap : public std::map<std::string,ObjectQuickList>
	{
	};

	enum FieldType
	{
		FT_ANTIMATTER,
		FT_NORMAL,
	};

	union FieldBlock
	{
		MT_QANTIMATTERLOAD antimatter;
		MT_QFIELDLOAD      field;
	};

	struct Field
	{
		Field( U32 _uniqueID ) : uniqueID(_uniqueID) {}

		FieldType  type;
		FieldBlock block;
		const U32  uniqueID;
	};

	struct FieldList : public std::list<Field>
	{
	};

	struct FieldLink
	{
		IObject* fieldObject;
		CString  groupName;
		CString  scriptHandle;
		CString  fieldName;
		CString  scenario;
	};

	struct FieldLinkList : public std::list<FieldLink>, IObjectFamilyEnum
	{
		virtual void EnumFamilyInfo( FamilyInfo& _info ){}

		virtual void EnumObjectInfo( ObjectInfo& _info )
		{
			FieldLink* link = (FieldLink*)_info.context;
			link->groupName = _info.family;
		}

		void insert( IObject* _object, IScenario* _scenario, U32 _fieldNumber )
		{
			// start a new link
			FieldLink link;
			link.fieldObject = _object;
			link.scenario.Format("%S", _scenario->GetSettings().name);

			// get object family/group
			_scenario->GetSettings().objectFamily->EnumObjectFamilyInfo( *this, _object, (DWORD)&link );

			// field name
			link.fieldName.Format(FIELDNAME,_fieldNumber);

			// script handle
			ObjectData data;
			_object->GetObjectData(data);
			link.scriptHandle = data.scriptHandle;

			push_back( link );
		}

		void save( IFileSystem& _fileSystem )
		{
			const char* linkDir = "\\ObjectLinks";
			U32 linkid = 0;

			if( _fileSystem.SetCurrentDirectory(linkDir) == false)
			{
				_fileSystem.CreateDirectory(linkDir);
				if(_fileSystem.SetCurrentDirectory(linkDir) == false)
				{
					CQERROR0("Failed on Create/Set Directory");
					return;
				}
			}

			for( iterator it = begin(); it != end(); it++ )
			{
				FieldLink& link = *it;

				CString savename;
				savename.Format("link_%d.", linkid++);
				savename += link.scenario;

				DAFILEDESC fdesc = savename;
				fdesc.lpImplementation = "UTF";
				fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
				fdesc.dwShareMode = 0;  // no sharing
				fdesc.dwCreationDistribution = CREATE_NEW; // fail if file already exists

				DWORD dwWritten = 0;
				HANDLE hFile = _fileSystem.OpenChild(&fdesc);
				if( hFile != INVALID_HANDLE_VALUE )
				{
					MT_FIELDLINK mtFieldLink;
					memset( &mtFieldLink, 0, sizeof(mtFieldLink) );
					strncpy( mtFieldLink.field, link.fieldName, sizeof(mtFieldLink.field) );
					strncpy( mtFieldLink.groupname, link.groupName, sizeof(mtFieldLink.groupname) );
					strncpy( mtFieldLink.scriptHandle, link.scriptHandle, sizeof(mtFieldLink.scriptHandle) );

					_fileSystem.WriteFile( hFile, &mtFieldLink, sizeof(mtFieldLink), &dwWritten, 0 );
					_fileSystem.CloseHandle( hFile );
				}
			}

			_fileSystem.SetCurrentDirectory(".."); // out of ObjectLinks dir
		}
	};

	System*       system;
    FieldMap      fieldMap;
	U32           fieldNumber;
	FieldLinkList fieldLinkList;

	FieldManager() : system(NULL), fieldNumber(0) {}

	bool prepareFieldList( ObjectList& _objectList, FieldList& _fieldList );
	bool createNewField( ObjectList& _objectList, FieldList& _fieldList, FieldType _fieldType );
	void applyObjectListToField( Field& _field, ObjectList& _objectList, float _range );
	bool addObjectToField( IObject* _object, Field& _field, float _range );
};

//----------------------------------------------------------------------------------------------

bool FieldManager::Reset( System* _system )
{
	system = _system;
	fieldMap.clear();
	return true;
}

//----------------------------------------------------------------------------------------------

bool FieldManager::Insert( IObject* _field )
{
	FieldObject* fieldObj = (FieldObject*)_field;

	const char* type = fieldObj->archname.GetBuffer(0);
	FieldMap::iterator it = fieldMap.find( type );

	// sort by archetype name
	if( it == fieldMap.end() )
	{
		fieldMap[type].push_back( _field );
	}
	else
	{
		it->second.push_back( _field );
	}
	return true;
}

//----------------------------------------------------------------------------------------------

bool FieldManager::Save( IFileSystem& _fileSystem )
{
	if( fieldMap.size() == 0 || system == NULL )
	{
		return false;
	}

	for( FieldMap::iterator it = fieldMap.begin(); it != fieldMap.end(); it++ )
	{
		// prepare the export list
		FieldList exportList;
		if( prepareFieldList(it->second,exportList) )
		{
			const char* archname = it->first.c_str();
			_fileSystem.CreateDirectory(archname);
			if (_fileSystem.SetCurrentDirectory(archname) == false)
			{
				CQERROR0("QuickSave failed on Directory");
				return false;
			}

			// create correct kind of data struct for field
			if( exportList.front().type == FT_NORMAL )
			{
				_fileSystem.CreateDirectory("MT_QFIELDLOAD");
				if (_fileSystem.SetCurrentDirectory("MT_QFIELDLOAD") == 0)
				{
					_fileSystem.SetCurrentDirectory("..");
					CQERROR0("QuickSave failed on Directory 'MT_QFIELDLOAD'");
					return false;
				}
			}
			else if( exportList.front().type == FT_ANTIMATTER )
			{
				_fileSystem.CreateDirectory("MT_QANTIMATTERLOAD");
				if (_fileSystem.SetCurrentDirectory("MT_QANTIMATTERLOAD") == 0)
				{
					_fileSystem.SetCurrentDirectory("..");
					CQERROR0("QuickSave failed on Directory 'MT_QANTIMATTERLOAD'");
					return false;
				}
			}

			for( FieldList::iterator exportIt = exportList.begin(); exportIt != exportList.end(); exportIt++ )
			{
				Field& field = *exportIt;

				// write out save block

				CString saveName;
				saveName.Format(FIELDNAME, field.uniqueID);

				DAFILEDESC fdesc = saveName;
				fdesc.lpImplementation = "UTF";
				fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
				fdesc.dwShareMode = 0;  // no sharing
				fdesc.dwCreationDistribution = CREATE_NEW; // fail if file already exists

				DWORD dwWritten = 0;
				HANDLE hFile = _fileSystem.OpenChild(&fdesc);
				if( hFile != INVALID_HANDLE_VALUE )
				{
					if( field.type == FT_NORMAL )
					{
						MT_QFIELDLOAD save = field.block.field;
						_fileSystem.WriteFile( hFile, &save, sizeof(save), &dwWritten, 0 );
					}
					else if( field.type == FT_ANTIMATTER )
					{
						MT_QANTIMATTERLOAD save = field.block.antimatter;
						_fileSystem.WriteFile( hFile, &save, sizeof(save), &dwWritten, 0 );
					}

					_fileSystem.CloseHandle( hFile );
				}
			}

			_fileSystem.SetCurrentDirectory(".."); // out of MT_Q?? dir
			_fileSystem.SetCurrentDirectory(".."); // out of archname dir
		}
	}

	fieldLinkList.save( _fileSystem );

	return true;
}

//----------------------------------------------------------------------------------------------

bool FieldManager::Load( IFileSystem& _fileSystem )
{
	return false;
}

//----------------------------------------------------------------------------------------------

bool FieldManager::prepareFieldList( ObjectList& _objectList, FieldList& _fieldList )
{
	// all done?
	if( _objectList.size() == 0 )
	{
		return( _fieldList.size() != 0 );
	}

	// set up the anchor
	IObject* anchor = _objectList.front();

	// is this antimatter?
	FieldType fieldType = FT_NORMAL;
	if( ((BASE_FIELD_DATA*)((FieldObject*)anchor)->data->basicData)->fieldClass == FC_ANTIMATTER )
	{
		fieldType = FT_ANTIMATTER;
	}

	const float inCloseDistance = GRIDSIZE * 2;

	// try to find a field this object is "near to"
	for( FieldList::iterator fieldIt = _fieldList.begin(); fieldIt != _fieldList.end(); fieldIt++ )
	{
		if( addObjectToField(anchor,*fieldIt,inCloseDistance) )
		{
			// record the association between the field block entry and this object
			fieldLinkList.insert( anchor, CAMPAIGN->GetCurrentScenario(), (*fieldIt).uniqueID );

			_objectList.pop_front();
			return prepareFieldList( _objectList, _fieldList );
		}
	}

	// starting new field
	return createNewField( _objectList, _fieldList, fieldType );
}

//----------------------------------------------------------------------------------------------

bool FieldManager::createNewField( ObjectList& _objectList, FieldList& _fieldList, FieldType _fieldType )
{
	// start a new field
	Field fieldEntry(fieldNumber++);
	fieldEntry.type = _fieldType;

	// remember center point for new field
	IObject* anchor = _objectList.front();
	_objectList.pop_front();

	// record the association between the field block entry and this object
	fieldLinkList.insert( anchor, CAMPAIGN->GetCurrentScenario(), fieldEntry.uniqueID );

	const float anchorDistance = GRIDSIZE * 3;

	NETGRIDVECTOR grid;
	grid.init( anchor->GetTransform().translation, system->id );

	if( _fieldType == FT_NORMAL )
	{
		MT_QFIELDLOAD& field = fieldEntry.block.field;
		memset( &field, 0, sizeof(field) );
		field.systemID = system->id;
		field.numSquares = 1;
		field.pos[0] = grid;
	}
	else if( _fieldType == FT_ANTIMATTER )
	{
		MT_QANTIMATTERLOAD& field = fieldEntry.block.antimatter;
		memset( &field, 0, sizeof(field) );

		field.systemID = system->id;
		field.numSegments = 1;
		field.pts[0] = grid;
	}

	// add chain of objects to this field
	applyObjectListToField( fieldEntry, _objectList, GRIDSIZE * 3 );

	// record new field
	_fieldList.push_back( fieldEntry );

	// continue the maddness
	return prepareFieldList( _objectList, _fieldList );
}

//-----------------------------------------------------------------------------------------------------

void FieldManager::applyObjectListToField( Field& _field, ObjectList& _objectList, float _range )
{
	bool bFieldChanged = false;

	for( ObjectList::iterator objectIt = _objectList.begin(); objectIt != _objectList.end(); )
	{
		if( addObjectToField(*objectIt,_field,_range) )
		{
			// record the association between the field block entry and this object
			fieldLinkList.insert( *objectIt, CAMPAIGN->GetCurrentScenario(), _field.uniqueID );

			objectIt = _objectList.erase(objectIt);
			bFieldChanged = true;
		}
		else
		{
			objectIt++;
		}
	}

	if( bFieldChanged )
	{
		applyObjectListToField( _field, _objectList, _range );
	}
}

//----------------------------------------------------------------------------------------------

bool FieldManager::addObjectToField( IObject* _object, Field& _field, float _range )
{
	if( _field.type == FT_NORMAL )
	{
		MT_QFIELDLOAD& field = _field.block.field;

		for( int i = 0; i < field.numSquares; i++ )
		{
			Vector fieldPos( field.pos[i].getX() * GRIDSIZE, field.pos[i].getY() * GRIDSIZE, 0 );

			if( (_object->GetTransform().translation - fieldPos).magnitude() < _range )
			{
				NETGRIDVECTOR grid;
				grid.init( _object->GetTransform().translation, system->id );

				// save off this object's grid
				field.pos[ field.numSquares ] = grid;
				field.numSquares++;

				// object recorded in field
				return true;
			}
		}
	}
	else if( _field.type == FT_ANTIMATTER )
	{
		MT_QANTIMATTERLOAD& field = _field.block.antimatter;

		for( int i = 0; i < field.numSegments; i++ )
		{
			Vector fieldPos( field.pts[i].getX() * GRIDSIZE, field.pts[i].getY() * GRIDSIZE, 0 );

			if( (_object->GetTransform().translation - fieldPos).magnitude() < _range )
			{
				NETGRIDVECTOR grid;
				grid.init( _object->GetTransform().translation, system->id );

				// save off this object's grid
				field.pts[ field.numSegments ] = grid;
				field.numSegments++;

				// object recorded in field
				return true;
			}
		}
	}

	return false;
}


//-----------------------------------------------------------------------------------------------------
// startup

struct _FieldManager : GlobalComponent
{
	FieldManager * fieldmanager;

	virtual void Startup (void)
	{
		fieldmanager = new DAComponent<FieldManager>;
		AddToGlobalCleanupList((IDAComponent **) &fieldmanager);
		FIELDMANAGER = fieldmanager;
	}

	virtual void Initialize (void)
	{
	}
};
static _FieldManager __FieldManager;
