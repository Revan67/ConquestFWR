//
// Scenario.cpp
//

#include "stdafx.h"
#include "globals.h"

#include "Scenario.h"
#include "SystemStructs.h"
#include "SaveLoad.h"
#include "tinyxml\tinyxml.h"
#include "CQTrace.h"
#include "GameTypes.h"
#include "ExportImport.h"
#include "StringTable.h"
#include "Editor.h"
#include "ObjectFamily.h"
#include "ClipBoard.h"

#include <TComponent.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <Engine.h>
#include <EventSys.h>
#include <system.h>
#include <FileSys.h>

#include <list>
#include <string>
#include <afxadv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Editor;

static int CScenario_UID = 1;

struct CScenario : public IScenario, public ISaverLoader, public IClipboardObject
{
	BEGIN_DACOM_MAP_INBOUND(CScenario)
		DACOM_INTERFACE_ENTRY(IScenario)
		DACOM_INTERFACE_ENTRY(ISaverLoader)
		DACOM_INTERFACE_ENTRY(IClipboardObject)
	END_DACOM_MAP()

	// IScenario

	virtual bool SetSettings( Settings& _Settings )
	{
		memcpy( &m_Settings, &_Settings, sizeof(m_Settings) );

		if( m_Settings.nameTag[0] )
		{
			const wchar_t* pName = STRINGTABLE->GetStringByTag( m_Settings.nameTag );
			if( pName )
			{
				wcsncpy( m_Settings.name, pName, countof(m_Settings.name) - 1 );
				m_Settings.name[ countof(m_Settings.name)-1 ] = 0;
			}
		}

		return true;
	}

	virtual Settings& GetSettings( void )
	{
		m_Settings.objectFamily = m_ObjectFamily;
		return m_Settings;
	}

	// IScenario operations

	virtual ISector* GetActiveSector();
	virtual struct ISector* NewSector( const wchar_t* _name );
	virtual bool DeleteSector( struct ISector* _sector );
	virtual bool Prepare( U32 _sectorID );
	virtual bool Finish( U32 _sectorID );

	// IScenario save/load

	virtual bool Save( const char* _filename );
	virtual bool Load( const char* _filename );

	// IScenario helper

	virtual IObject* FindObjectByScriptHandle( const char* _scriptHandle );

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

	// locals

	ISector*       m_Sector; // TODO(aaj-4/26/2004): at some point, this could become a list of sectors, but not right now
	Settings       m_Settings;
	DWORD          m_dwFileAttribs;
	MT_GlobalData  m_globalData;
	MT_MUSIC_DATA  m_musicData;
	IObjectFamily* m_ObjectFamily;

	CScenario()
	{
		m_ObjectFamily = NULL;
		m_dwFileAttribs = 0;
		m_Sector = NULL;
		ZeroMemory( &m_Settings, sizeof(m_Settings) );
		swprintf( m_Settings.name, L"NONAMESCENARIO_%04d", CScenario_UID++ );
		initGlobalData();
	}

	virtual ~CScenario()
	{
		if(m_Sector)
		{
			delete m_Sector;
			m_Sector = NULL;
		}
		if(m_ObjectFamily)
		{
			m_ObjectFamily->Release();
			m_ObjectFamily = NULL;
		}
	}

	void initGlobalData();

	BOOL32 saveParseData( struct IFileSystem& _file );
	BOOL32 saveMusicData( struct IFileSystem& _file );
	BOOL32 saveGlobalData( struct IFileSystem& _file );
	BOOL32 saveStringTable( struct IFileSystem& _file );
};

//-----------------------------------------------------------------------------------------------------

ISector* CScenario::GetActiveSector()
{
	return m_Sector;
}

//-----------------------------------------------------------------------------------------------------

struct ISector* CScenario::NewSector( const wchar_t* _name )
{
	if( m_ObjectFamily )
	{
		m_ObjectFamily->Release();
		m_ObjectFamily = NULL;
	}
	DACOMDESC desc("ObjectFamily");
	DACOM->CreateInstance(&desc,(void**)&m_ObjectFamily);

	if( m_Sector )
	{
		ISector::Delete( m_Sector );
		m_Sector = NULL;
	}

	m_Sector = ISector::New();

	ISector::Settings s;
	if( _name )
		wcscpy( s.name, _name );

	m_Sector->SetSettings( s );
	return m_Sector;
}

//-----------------------------------------------------------------------------------------------------

bool CScenario::DeleteSector( struct ISector* _sector )
{
	if(_sector && m_Sector == _sector)
	{
		ISector::Delete( m_Sector );
		m_Sector = NULL;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

bool CScenario::Prepare( U32 _sectorID )
{
	if( m_ObjectFamily )
	{
		m_ObjectFamily->Prepare(this);
	}
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool CScenario::Finish( U32 _sectorID )
{
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool CScenario::Save( class TiXmlNode& _doc ) 
{ 
	TiXmlElement scenario ("SCENARIO");
	scenario.SetAttribute( "name", CString(m_Settings.name) );
	scenario.SetAttribute( "nameTag", m_Settings.nameTag );
	scenario.SetAttribute( "date_high", m_Settings.lastSaved.dwHighDateTime );
	scenario.SetAttribute( "date_low", m_Settings.lastSaved.dwLowDateTime );

	// save the SINGLE sector
	if( m_Sector )
	{
		COMPTR<ISaverLoader> stringTableSaver;
		STRINGTABLE->QueryInterface( "ISaverLoader", stringTableSaver );
		if( stringTableSaver )
		{
			stringTableSaver->Save(scenario);
		}

		COMPTR<ISaverLoader> saver;
		m_Sector->QueryInterface( "ISaverLoader", (void**)saver );
		if( saver )
		{
			saver->Save(scenario);
		}

		if( m_ObjectFamily )
		{
			COMPTR<ISaverLoader> saver;
			m_ObjectFamily->QueryInterface( "ISaverLoader", (void**)saver );
			if( saver )
			{
				saver->Save(scenario);
			}
		}
	}

	return( _doc.InsertEndChild(scenario) != NULL );
}

//-----------------------------------------------------------------------------------------------------

bool CScenario::Load( class TiXmlNode& _doc )
{ 
	m_Settings.loaded = false;

	TiXmlElement* scenario = _doc.FirstChildElement("SCENARIO");
	if( scenario )
	{
		if( scenario->Attribute("nameTag") )
		{
			strncpy( m_Settings.nameTag, scenario->Attribute("nameTag"), countof(m_Settings.nameTag)-1 );
		}

		Import::XML::widestring( m_Settings.name, 128, "name", scenario );
		scenario->QueryUnsignedIntValue("date_high",&m_Settings.lastSaved.dwHighDateTime);
		scenario->QueryUnsignedIntValue("date_low",&m_Settings.lastSaved.dwLowDateTime);

		// only support for one sector per scenario
		DeleteSector( m_Sector );
		m_Sector = NewSector(NULL);

		// load the scneario's string table
		COMPTR<ISaverLoader> stringTableLoader;
		STRINGTABLE->QueryInterface( "ISaverLoader", stringTableLoader );
		if( stringTableLoader )
		{
			// adds all the entries, but does not "delete" any previous string table entries (may replace entries!)
			stringTableLoader->Load(*scenario);
		}

		// load sector and objects
		COMPTR<ISaverLoader> sectorLoader;
		m_Sector->QueryInterface( "ISaverLoader", (void**)sectorLoader );
		if( sectorLoader )
		{
			if( !sectorLoader->Load(*scenario) )
			{
				return false;
			}
		}

		// load object family info
		COMPTR<ISaverLoader> objectFamilyloader;
		m_ObjectFamily->QueryInterface( "ISaverLoader", (void**)objectFamilyloader );
		if( objectFamilyloader )
		{
			if( !objectFamilyloader->Load(*scenario) )
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}

	m_Settings.loaded = true;
	return true; 
}

//-----------------------------------------------------------------------------------------------------

bool CScenario::Save( struct IFileSystem& _fs )
{ 
	// basic structure of a qmission is:
	//	ParseData
	//		Data.h (the data.i file renamed to data.h)
	//	MT_SECTOR_SAVELOAD
	//		Sector (data)
	//  ObjectList
	//		Count
	//		QuickSave
	//			Archname Entry (like GBOAT!!T_Corvette)
	//				MT_QSHIPLOAD
	//					object instance data
	//	MT_MUSIC_DATA
	//		Music
	//	MT_GlobalData
	//		Globals

	saveParseData( _fs );

	// save the SINGLE sector
	if( m_Sector )
	{
		COMPTR<ISaverLoader> saver;
		m_Sector->QueryInterface( "ISaverLoader", (void**)saver );
		if( saver )
		{
			saver->Save( _fs );
		}
	}

	saveMusicData(_fs);
	saveGlobalData(_fs);
	saveStringTable(_fs);

	if( m_ObjectFamily )
	{
		COMPTR<ISaverLoader> saver;
		m_ObjectFamily->QueryInterface( "ISaverLoader", (void**)saver );
		if( saver )
		{
			saver->Save( _fs );
		}
	}

	return false; 
}

//-----------------------------------------------------------------------------------------------------

bool CScenario::Load( struct IFileSystem& )
{ 
	return false; 
}

//-----------------------------------------------------------------------------------------------------

bool CScenario::Save( const char* _filename )
{
	// todo(aaj-5/5/2004): right now all saves/loads will be XML, at some time the user will be able to use UTF as well

	m_dwFileAttribs = ::GetFileAttributes(_filename);

	// make sure that this file is not READONLY
	if( m_dwFileAttribs != 0xFFFFFFFF && m_dwFileAttribs & FILE_ATTRIBUTE_READONLY )
	{
		return false;
	}

	// create a new doc
	TiXmlDocument doc( _filename );
	doc.InsertEndChild( TiXmlDeclaration("1.0","","yes") );

	if( Save(doc) )
	{
		return doc.SaveFile();
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

bool CScenario::Load( const char* _filename )
{
	// todo(aaj-5/5/2004): right now all saves/loads will be XML, at some time the user will be able to use UTF as well

	m_dwFileAttribs = ::GetFileAttributes(_filename);

	// make sure that this file exists
	if( m_dwFileAttribs != 0xFFFFFFFF )
	{
		TiXmlDocument doc( _filename );

		// is this a valid XML doc?
		if( doc.LoadFile() && doc.FirstChildElement() )
		{
			return Load(doc);
		}
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

BOOL32 CScenario::saveParseData( struct IFileSystem& _fileSystem )
{
	COMPTR<IFileSystem> file;

	_fileSystem.CreateDirectory("\\ParseData");
	if (_fileSystem.SetCurrentDirectory("\\ParseData") == 0)
	{
		return false;
	}

	DAFILEDESC fdesc = "Data.h";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (_fileSystem.CreateInstance(&fdesc, file) != GR_OK)
	{
		int t = _fileSystem.GetLastError();
		return false;
	}

	// use the same parser block from Globals.dll in the application

	int preprocessDataBlockSize;
	void* preprocessDataBlock = NULL;
	HRSRC hRes = ::FindResource(hStringTable, MAKEINTRESOURCE(IDR_PARSER1), "PARSER");

	if( hRes != NULL )
	{
		HGLOBAL hGlobal;

		if ((hGlobal = LoadResource(hStringTable, hRes)) != 0)
		{
			preprocessDataBlock		= LockResource(hGlobal);
			preprocessDataBlockSize = SizeofResource(hStringTable, hRes);
		}
	}

	if( preprocessDataBlock )
	{
		DWORD dwWritten;
		file->WriteFile(0, preprocessDataBlock, preprocessDataBlockSize, &dwWritten);
	}

	_fileSystem.SetCurrentDirectory("..");
	return true;
}

//-----------------------------------------------------------------------------------------------------

BOOL32 CScenario::saveMusicData( struct IFileSystem& _fileSystem )
{
	_fileSystem.CreateDirectory("\\MT_MUSIC_DATA");
	if (_fileSystem.SetCurrentDirectory("\\MT_MUSIC_DATA") == 0)
	{
		return false;
	}

	DAFILEDESC fdesc = "Music";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	COMPTR<IFileSystem> file;
	if (_fileSystem.CreateInstance(&fdesc, file) != GR_OK)
	{
		int t = _fileSystem.GetLastError();
		return false;
	}

	// fill out data

	ZeroMemory( &m_musicData, sizeof(SONG) * NUM_SONGS );
	m_musicData[0].looping = true;
	m_musicData[0].playing = true;
	m_musicData[0].volume  = 1.0f;
	strcpy( m_musicData[0].filename, "terrangame14.wav" );

	// write data

	DWORD dwWritten;
	file->WriteFile(0, &m_musicData, sizeof(m_musicData), &dwWritten);

	_fileSystem.SetCurrentDirectory("..");
	return true;
}

//-----------------------------------------------------------------------------------------------------

BOOL32 CScenario::saveGlobalData( struct IFileSystem& _fileSystem )
{
	_fileSystem.CreateDirectory("\\MT_GlobalData");
	if (_fileSystem.SetCurrentDirectory("\\MT_GlobalData") == 0)
	{
		return false;
	}

	DAFILEDESC fdesc = "Globals";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	COMPTR<IFileSystem> file;
	if (_fileSystem.CreateInstance(&fdesc, file) != GR_OK)
	{
		int t = _fileSystem.GetLastError();
		return false;
	}

	// fill out data

	DWORD dwWritten;
	file->WriteFile(0, &m_globalData, sizeof(m_globalData), &dwWritten);

	_fileSystem.SetCurrentDirectory("..");
	return true;
}

//-----------------------------------------------------------------------------------------------------

BOOL32 CScenario::saveStringTable( struct IFileSystem& _file )
{
	COMPTR<ISaverLoader> saver;
	STRINGTABLE->QueryInterface("ISaverLoader",saver);
	if( saver )
	{
		return saver->Save( _file );
	}
	return false;
}

//----------------------------------------------------------------------------
//
void CScenario::initGlobalData (void)
{
	// todo: should "global settings" be per Scenario?

	#define DEF_MAX_CP_PER_PLAYER  100
	#define BASE_MAX_CREW 250
	#define BASE_MAX_METAL 220
	#define BASE_MAX_GAS 180

	int i;

	memset(&m_globalData, 0, sizeof(m_globalData));

	for (i = 0; i <= MAX_PLAYERS; i++)
		m_globalData.colorAssignment[i] = i;

	for (i = 1; i <= MAX_PLAYERS; i++)
		m_globalData.gas[i] = BASE_MAX_GAS;

	for (i = 1; i <= MAX_PLAYERS; i++)
		m_globalData.metal[i] = BASE_MAX_METAL;

	for (i = 1; i <= MAX_PLAYERS; i++)
		m_globalData.crew[i] = BASE_MAX_CREW;

	for (i = 1; i <= MAX_PLAYERS; i++)
		m_globalData.gasMax[i] = 0;

	for (i = 1; i <= MAX_PLAYERS; i++)
		m_globalData.metalMax[i] = 0;

	for (i = 1; i <= MAX_PLAYERS; i++)
		m_globalData.crewMax[i] = 0;

	for (i = 0; i <= MAX_PLAYERS; i++)
		m_globalData.playerRace[i] = M_TERRAN;

	for (i = 0; i < MAX_PLAYERS; i++)
		m_globalData.allyMask[i] = (1 << i);

	for (i = 0; i < MAX_PLAYERS; i++)
		m_globalData.visibilityMask[i] = (1 << i);

	for (i = 0; i < MAX_PLAYERS; i++)
		m_globalData.playerAssignments[i] = i + 1;		// convert lobby slot into playerID

	m_globalData.currentPlayer = 1;
	m_globalData.bGlobalLighting = true;		// turn on the standard lights

	for(i = 1; i <= MAX_PLAYERS; i++)
		m_globalData.maxComPts[i] = DEF_MAX_CP_PER_PLAYER;

	m_globalData.availableTechNode.InitLevel(TECHTREE::FULL_TREE);

	// a new way of settings number of units for starting
//	m_globalData.gameSettings.units = CQGAMETYPES::UNITS_NONE;
}

//----------------------------------------------------------------------------------------------

IObject* CScenario::FindObjectByScriptHandle( const char* _scriptHandle )
{
	// note: if there is more than one object with the same script handle, 
	//       then this logic will return the first it finds and skip the rest

	if( m_Sector )
	{
		ObjectData data;

		for( int i = 0; i <= MAX_SYSTEMS; i++ )
		{
			System* sys = m_Sector->FindSystemByIdx(i);
			if( sys )
			{
				for( ObjectList::iterator it = sys->objectList.begin(); it != sys->objectList.end(); it++ )
				{
					(*it)->GetObjectData(data);

					if( data.scriptHandle == _scriptHandle )
					{
						return *it;
					}
				}
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------------------------------
// IClipboardObject

const char* CScenario::GetType()
{
	return "Scenario";
}

bool CScenario::Copy( CSharedFile& _memfile )
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

bool CScenario::Paste( CSharedFile& _memfile )
{
	m_Settings.loaded = false;

	int size = _memfile.GetLength();
	void* buffer = alloca(size + 2);
    memset( buffer, 0, size+2 );

	if( buffer )
	{
		_memfile.Read( buffer, size );

		TiXmlDocument doc;
		doc.Parse( (const char*)buffer );

		TiXmlElement* scenario = doc.FirstChildElement("SCENARIO");
		if( scenario )
		{
			if( Load(doc) )
			{
				return true;
			}
		}
	}

	return false;
}

bool CScenario::Append( CSharedFile& _memfile )
{
	return true;
}

//-----------------------------------------------------------------------------------------------------

namespace Scenario
{
	IScenario* New()
	{
		return new DAComponent<CScenario>;
	}

	bool Delete(IScenario* _s)
	{
		CScenario* s = (CScenario*)_s;

		// delete the sector...
		if( s->m_Sector )
			s->DeleteSector( s->m_Sector );

		// try to release it
		if( _s->Release() )
		{
			return false;
		}
		return true;
	}
};
