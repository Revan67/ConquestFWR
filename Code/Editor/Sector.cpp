//--------------------------------------------------------------------------//
//                                                                          //
//                                Sector.cpp                                //
//                                                                          //
//                                                                          //
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

 
#include "stdafx.h"
#include "globals.h"

#include "Startup.h"
#include "SaveLoad.h"
#include "CQTrace.h"
#include "SystemStructs.h"
#include "tinyxml\tinyxml.h"
#include "GridVector.h"
#include "ExportImport.h"
#include "DataList.h"
#include "Object.h"
#include "Camera.h"
#include "Editor.h"
#include "SpaceEnv.h"

#include <TComponent.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <Engine.h>
#include <EventSys.h>
#include <system.h>
#include <HKEvent.h>
#include <FileSys.h>
#include <IAnim.h>
#include <ICamera.h>

#include <list>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Editor;

struct Sector : public ISector, public ISaverLoader
{
	BEGIN_DACOM_MAP_INBOUND(Sector)
		DACOM_INTERFACE_ENTRY(ISector)
		DACOM_INTERFACE_ENTRY(ISaverLoader)
	END_DACOM_MAP()

	struct Background
	{
		TRANSFORM      xform;
		INSTANCE_INDEX meshInstIndex;
		CString        archname;
		U32            systemID;

		Background()
		{
			meshInstIndex = INVALID_INSTANCE_INDEX;
			systemID = 0;
		}
	};

	Settings   m_Settings;
	System     m_SystemList[MAX_SYSTEMS];
	U32        m_CurrentSystemID;
	S32        xExtent,yExtent,xOrg,yOrg;
	Background m_Background;

	// settings

	virtual bool SetSettings( Settings& _s )
	{
		memcpy( &m_Settings, &_s, sizeof(m_Settings) );
		return true;
	}

	virtual Settings& GetSettings( void )
	{
		return m_Settings;
	}

	// operations

	virtual System* GetActiveSystem()
	{
		updateSystemsInfo();
		return FindSystemByIdx(m_CurrentSystemID);
	}

	virtual System* NewSystem( const wchar_t* _name );

	virtual bool DeleteSystem( struct System* _system );

	virtual System* FindSystemByIdx( U32 _sysIdx )
	{
		for( int i = 0; i < MAX_SYSTEMS; i++ )
		{
			if( m_SystemList[i].id == _sysIdx && !m_SystemList[i].bEmpty )
			{
				return &m_SystemList[i];
			}
		}
		return NULL;
	}

	virtual const System* findSystemByIdx( U32 _sysIdx ) const
	{
		for( int i = 0; i < MAX_SYSTEMS; i++ )
		{
			if( m_SystemList[i].id == _sysIdx && !m_SystemList[i].bEmpty )
			{
				return &m_SystemList[i];
			}
		}
		return NULL;
	}

	virtual System* FindSystemByName( const wchar_t* _name )
	{
		CString search(_name);

		for( int i = 0; i < MAX_SYSTEMS; i++ )
		{
			if( !m_SystemList[i].bEmpty && m_SystemList[i].name == search )
			{
				return &m_SystemList[i];
			}
		}
		return NULL;
	}

	// operations

	virtual BOOL32 SetCurrentSystem (U32 SystemID)
	{ 
		if( SystemID < MAX_SYSTEMS+1 )
		{
			m_CurrentSystemID = SystemID;
			return true;
		}
		return false;
	}

	virtual U32 GetCurrentSystem (void) const 
	{ 
		return m_CurrentSystemID; 
	}

	virtual BOOL32 GetSystemRect (U32 SystemID, struct tagRECT * rect,bool bAbsolute) const;

	virtual BOOL32 GetCurrentRect (struct tagRECT * rect) const 
	{ 
		return GetSystemRect( GetCurrentSystem(), rect, false );
	}

	virtual void GetDefaultSystemSize (S32 &_sizeX,S32 &_sizeY) 
	{
		_sizeX = MAX_SYS_SIZE / 2;
		_sizeY = MAX_SYS_SIZE / 2;
	}

	virtual BOOL32 GetSectorCenter (S32 *x, S32 *y) const 
	{ 
		*x = (xExtent-xOrg) / 2 + xOrg;
		*y = (yExtent-yOrg) / 2 + yOrg;
		return true; 
	}

	virtual struct GT_SYSTEM_KIT GetSystemLightKit(U32 systemID)
	{ 
		// eek, who wrote this API?
		if( systemID < MAX_SYSTEMS )
		{
			return m_SystemList[systemID].systemKit;
		}
		return m_SystemList[0].systemKit;
	}

	virtual int GetNumSystems()
	{
		int numSystems = 0;
		for( int i = 0; i < MAX_SYSTEMS; i++ )
		{
			if( !m_SystemList[i].bEmpty )
			{
				numSystems++;
			}
		}
		return numSystems;
	}

	virtual void GetSystemName(wchar_t * nameBuffer, U32 nameBufferrSize, U32 systemID);

	virtual void GetSystemNameChar (U32 systemID, char * nameBuffer, U32 bufferSize);

	virtual void SetSystemName(U32 systemID, U32 stringID);

	virtual void SetLightingKit(U32 systemID, char * lightingKit);

	virtual void Render( void );

	// for saving and loading using XML

	virtual bool Save( class TiXmlNode& );
	virtual bool Load( class TiXmlNode& );

	// for saving and loading using IFileSystem

	virtual bool Save( struct IFileSystem& );
	virtual bool Load( struct IFileSystem& );

	bool saveSystemDataBlock(IFileSystem&);
	bool saveJumpGateInfo(IFileSystem&);
	bool saveObjectList(IFileSystem&);

	// locals

	Sector()
	{
		m_CurrentSystemID = 1;
		xExtent = yExtent = 20;
		xOrg = 0;
		yOrg = 0;

		for( int i = 0 ; i < MAX_SYSTEMS; i++ )
		{
			// very careful NOT to change this again
			m_SystemList[i].id = i + 1;
		}
	}

	System * getSystemHandle(U32 SystemID)
	{
		if( SystemID < MAX_SYSTEMS )
		{
			return &m_SystemList[SystemID];
		}
		return NULL;
	}

	void loadKit(U32 systemID);
	void updateSystemsInfo(void);
};

//-----------------------------------------------------------------------------------------------------
// factory

ISector* ISector::New()
{
	return new DAComponent<Sector>;
}

bool ISector::Delete( ISector* _sector )
{
	_sector->Release();
	return true;
}

//-----------------------------------------------------------------------------------------------------

BOOL32 Sector::GetSystemRect (U32 SystemID, struct tagRECT * rect,bool bAbsolute) const
{
	// todo(aaj-4/23/2004): I am assuming this wants the system rect in System coord space

	if( SystemID < MAX_SYSTEMS )
	{
		const System* sys = findSystemByIdx( SystemID );

		if( !sys )
			return false;

		if (bAbsolute)
		{
			rect->left   = sys->x;
			rect->bottom = sys->y;
			rect->right  = sys->x+(sys->sizeX)-1;
			rect->top    = sys->y+(sys->sizeY)-1;
		}
		else
		{
			rect->left   = 0;
			rect->bottom = 0;
			rect->right  = (sys->sizeX)-1;
			rect->top    = (sys->sizeY)-1;
		}

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

void Sector::SetLightingKit(U32 systemID, char * lightingKit)
{
	System* sys = FindSystemByIdx(systemID);

	if( sys )
	{
		sys->systemKitName = lightingKit;

		if(sys->systemKitName[0])
		{
			loadKit(systemID);
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void Sector::Render( void )
{
	BACKGROUND->RenderNeb();

//	if( m_Background.meshInstIndex != INVALID_INSTANCE_INDEX )
//	{
//		CAMERA->SetModelView();
//
//	PIPE->set_render_state( D3DRENDERSTATE_ALPHATESTENABLE, TRUE );  
//	PIPE->set_render_state( D3DRENDERSTATE_ALPHABLENDENABLE,TRUE);
//	PIPE->set_render_state( D3DRENDERSTATE_ALPHAREF, 0x02 );  
//	PIPE->set_render_state( D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATEREQUAL );
//
//	PB.Begin(PB_QUADS);
//		PB.Color4ub( 255, 0, 255, 255 );
//		PB.Vertex3f(m_Background.xform.translation.x - GRIDSIZE, m_Background.xform.translation.y - GRIDSIZE, 0);
//		PB.Vertex3f(m_Background.xform.translation.x - GRIDSIZE, m_Background.xform.translation.y + GRIDSIZE, 0);
//		PB.Vertex3f(m_Background.xform.translation.x + GRIDSIZE, m_Background.xform.translation.y + GRIDSIZE, 0);
//		PB.Vertex3f(m_Background.xform.translation.x + GRIDSIZE, m_Background.xform.translation.y - GRIDSIZE, 0);
//	PB.End();
//
//		ENGINE->set_transform( m_Background.meshInstIndex, m_Background.xform );
//
//		SINGLE dt = Editor::deltaTime;
//		ENGINE->update_instance(m_Background.meshInstIndex, 0, dt);
//		ANIM->update_instance(m_Background.meshInstIndex,dt);
//
//		ARCHETYPE_INDEX	archIdx = ENGINE->get_instance_archetype( m_Background.meshInstIndex );
//
//		const float LODPERCENT = 1.0f;
//		ENGINE->render_instance(CAMERALIB, m_Background.meshInstIndex, 0, LODPERCENT, 0, NULL);
//	}
}

//-----------------------------------------------------------------------------------------------------

void Sector::GetSystemName(wchar_t * nameBuffer, U32 nameBufferSize, U32 systemID)
{
	System * system = getSystemHandle(systemID);
	CQASSERT(system);

	LPCSTR name = (LPCSTR)system->name;
	if( name[0] )
		::MultiByteToWideChar( CP_ACP, MB_USEGLYPHCHARS, name, strlen(name), nameBuffer, nameBufferSize);
	else
		wcsncpy(nameBuffer, L"BUGBUG::Uninitialized System Name", nameBufferSize/sizeof(wchar_t));
}

//-----------------------------------------------------------------------------------------------------

void Sector::GetSystemNameChar (U32 systemID, char * nameBuffer, U32 bufferSize)
{
	System * system = getSystemHandle(systemID);
	CQASSERT(system);
	LPCSTR name = (LPCSTR)system->name;
	strncpy(nameBuffer,name,bufferSize);
}

//-----------------------------------------------------------------------------------------------------

void Sector::SetSystemName(U32 systemID, U32 stringID)
{
	char name[256];
	::LoadString( hStringTable, stringID, name, sizeof(name)-1 );

	System * system = getSystemHandle(systemID);
	CQASSERT(system);
	system->name = name;
}

//-----------------------------------------------------------------------------------------------------
	
System* Sector::NewSystem( const wchar_t* _name )
{
	for( int i = 0 ; i < MAX_SYSTEMS; i++ )
	{
		if( m_SystemList[i].bEmpty )
		{
			m_SystemList[i].bEmpty = false;
			m_SystemList[i].jList.RemoveAll();

			if( _name )
			{
				CString szName(_name);
				m_SystemList[i].name = szName;
			}
			else
			{
				m_SystemList[i].name.Format(_T("System_%d"),i);
			}
			
			return &m_SystemList[i];
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------------------------------

bool Sector::DeleteSystem( struct System* _system )
{
	if( _system )
	{
		for( int i = 0 ; i < MAX_SYSTEMS; i++ )
		{
			if( &m_SystemList[i] == _system )
			{
				// "delete" system
				m_SystemList[i].bEmpty = true;
				m_SystemList[i].jList.RemoveAll();

				// remove all references to this system
				for( int i = 0; i < MAX_SYSTEMS; i++ )
				{
					if( m_SystemList[i].id != _system->id )
					{
						// remove points that refer to this deleted system
						for( int j = 0; j < m_SystemList[i].jList.GetSize(); j++ )
						{
							if( m_SystemList[i].jList[j].destSystemID == _system->id )
							{
								m_SystemList[i].jList.RemoveAt(j);
							}
						}
					}
				}
			}
		}
	}



	return false;
}

//-----------------------------------------------------------------------------------------------------

void Sector::updateSystemsInfo(void)
{
	const int maxSystemSize = 64;
	const int maxSectorSize = maxSystemSize * 4;
	const int maxSectorUnitSize = GRIDSIZE * maxSectorSize;
	const int maxSystemUnitSize = GRIDSIZE * 64;

	for( int i = 0; i < MAX_SYSTEMS; i++ )
	{
		if( m_SystemList[i].bEmpty == false )
		{
			m_SystemList[i].prepareForSaving( maxSectorUnitSize, maxSystemUnitSize );

			for( int j = 0; j < m_SystemList[i].jList.GetSize(); j++ )
			{
				JumpPoint& jumpPoint = m_SystemList[i].jList[j];

				jumpPoint.x = jumpPoint.fPoint.X * m_SystemList[i].sizeX;
				jumpPoint.y = jumpPoint.fPoint.Y * m_SystemList[i].sizeY;
			}

		}
	}

}

//-----------------------------------------------------------------------------------------------------

void Sector::loadKit(U32 systemID)
{
	// TODO: system kits need some work on rendering (going to unify this from Conquest 2)
	System* system = FindSystemByIdx(systemID);

	if( system && system->systemKitName.IsEmpty() == false )
	{
		U32 archid = GENDATA->GetArchetypeDataID(system->systemKitName);
		if( !archid )
		{
			return;
		}

		U32 dataSize = GENDATA->GetArchetypeDataSize(archid);
		if( dataSize != sizeof(GT_SYSTEM_KIT) )
		{
			return;
		}

		const GT_SYSTEM_KIT* kit = (GT_SYSTEM_KIT*)GENDATA->GetArchetypeData( archid );
		if( kit )
		{
			BACKGROUND->LoadBackground( (char*)kit->fileName, systemID );
		}
	}
}	


//-----------------------------------------------------------------------------------------------------

bool Sector::Save( class TiXmlNode& _node ) 
{ 
	TiXmlElement sector ("SECTOR");
	sector.SetAttribute( "name", CString(m_Settings.name) );
	sector.SetAttribute( "date_high", m_Settings.lastSaved.dwHighDateTime );
	sector.SetAttribute( "date_low", m_Settings.lastSaved.dwLowDateTime );
	sector.SetAttribute( "currentSystem", m_CurrentSystemID );

	// save all valid systems
	for( int i = 0; i < MAX_SYSTEMS; i++ )
	{
		if( m_SystemList[i].bEmpty == false )
		{
			m_SystemList[i].Save(sector);
		}
	}

	return( _node.InsertEndChild(sector) != NULL );
}

//-----------------------------------------------------------------------------------------------------

bool Sector::Load( class TiXmlNode& _node )
{
	TiXmlElement* sector = _node.FirstChildElement("SECTOR");
	if( sector )
	{
		Import::XML::widestring(m_Settings.name,128,"name",sector);
		sector->QueryUnsignedIntValue("date_high",&m_Settings.lastSaved.dwHighDateTime);
		sector->QueryUnsignedIntValue("date_low",&m_Settings.lastSaved.dwLowDateTime);
		m_CurrentSystemID = atoi( sector->Attribute("currentSystem") );

		int systemIdx = 0;

		TiXmlElement* child = sector->FirstChildElement();
		while( child )
		{
			if( m_SystemList[systemIdx].bEmpty == true )
			{
				if( m_SystemList[systemIdx].Load(*child) )
				{
					systemIdx++;
				}
			}
			child = child->NextSiblingElement();
		}

		return true;
	}
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool Sector::Save( struct IFileSystem& _fileSystem )
{ 
	//	MT_SECTOR_SAVELOAD
	//		Sector (data)
	//  JumpList <- New
	//		JumpGates (data)
	//  ObjectList
	//		Count
	//		QuickSave
	//			Archname Entry (like GBOAT!!T_Corvette)
	//				MT_QSHIPLOAD
	//					object instance data

	if( saveSystemDataBlock(_fileSystem) )
	{
		if( saveJumpGateInfo(_fileSystem) )
		{
			return saveObjectList(_fileSystem);
		}
	}

	return false; 
}

//-----------------------------------------------------------------------------------------------------

bool Sector::saveSystemDataBlock(IFileSystem& _fileSystem)
{
	updateSystemsInfo();

	MT_SECTOR_SAVELOAD saveLoad;
	memset( &saveLoad, 0, sizeof(saveLoad) );

	saveLoad.currentSystem = m_CurrentSystemID;

	for( int i = 0; i < MAX_SYSTEMS; i++ )
	{
		memset( &saveLoad.sysData[i], 0, sizeof(saveLoad.sysData[i]) );

		if( m_SystemList[i].bEmpty == false )
		{
			saveLoad.sysData[i].id    = m_SystemList[i].id;
			saveLoad.sysData[i].x	  = m_SystemList[i].x;
			saveLoad.sysData[i].y	  = m_SystemList[i].y;
			saveLoad.sysData[i].sizeX = m_SystemList[i].sizeX;
			saveLoad.sysData[i].sizeY = m_SystemList[i].sizeY;

			strncpy( saveLoad.sysData[i].name, m_SystemList[i].name.GetBuffer(0), 16 );
			strncpy( saveLoad.sysData[i].systemKitName, m_SystemList[i].systemKitName, GT_PATH);
			strncpy( saveLoad.sysData[i].backgroundName, m_SystemList[i].backgroundName, GT_PATH);

			// save off that camera buffer
			memcpy( &saveLoad.sysData[i].cameraBuffer, &m_SystemList[i].cameraData, sizeof(saveLoad.sysData[i].cameraBuffer) );
		}
	}

	COMPTR<IFileSystem> file;

	_fileSystem.CreateDirectory("MT_SECTOR_SAVELOAD");
	if (_fileSystem.SetCurrentDirectory("MT_SECTOR_SAVELOAD") == 0)
	{
		return false;
	}

	DAFILEDESC fdesc = "Sector";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	if (_fileSystem.CreateInstance(&fdesc, file) != GR_OK)
	{
		_fileSystem.GetLastError();
		return false;
	}

	DWORD dwWritten;
	file->WriteFile(0, &saveLoad, sizeof(saveLoad), &dwWritten);
	_fileSystem.SetCurrentDirectory("..");
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool Sector::saveJumpGateInfo(IFileSystem& _fileSystem)
{
	DWORD numGates = 0;
	for( int i = 0; i < MAX_SYSTEMS; i++ )
	{
		if( !m_SystemList[i].bEmpty )
		{
			numGates += m_SystemList[i].jList.GetSize();
		}
	}

	if( !numGates )
	{
		// even if no gates, the system could still be valid
		return true;
	}

	_fileSystem.CreateDirectory("JumpList");
	if (_fileSystem.SetCurrentDirectory("JumpList") == 0)
	{
		return false;
	}

	DWORD dwWritten;

	DAFILEDESC fdesc = "JumpGates";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	COMPTR<IFileSystem> file;
	if (_fileSystem.CreateInstance(&fdesc, file) != GR_OK)
	{
		_fileSystem.GetLastError();
		return false;
	}

	// record the number of gates
	file->WriteFile(0, &numGates, sizeof(numGates), &dwWritten);

	// record the gate archname
	char jumpArchName[GT_PATH] = "JGATE!!Jumpgate";
	file->WriteFile(0, jumpArchName, GT_PATH, &dwWritten);

	// record each gate
	for( i = 0; i < MAX_SYSTEMS; i++ )
	{
		if( m_SystemList[i].bEmpty )
		{
			continue;
		}

		for( int nGate = 0; nGate < m_SystemList[i].jList.GetSize(); nGate++ )
		{
			JumpPoint& jumpPoint = m_SystemList[i].jList[nGate];

			MT_QJGATELOAD gate;
			gate.gate_id	  = jumpPoint.id;             // where it is
			gate.exit_gate_id = jumpPoint.destWormholeID; // where it goes to

			Vector pos( jumpPoint.x, jumpPoint.y, 0 );
			gate.pos.init( pos, m_SystemList[i].id );
			
			gate.bJumpAllowed = (jumpPoint.bJumpAllowed != false);

			file->WriteFile(0, &gate, sizeof(gate), &dwWritten);
		}
	}

	_fileSystem.SetCurrentDirectory("..");
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool Sector::saveObjectList(IFileSystem& _fileSystem)
{
	_fileSystem.CreateDirectory("\\ObjectList");
	if (_fileSystem.SetCurrentDirectory("\\ObjectList") == 0)
	{
		return false;
	}

	_fileSystem.CreateDirectory("QuickSave");
	if (_fileSystem.SetCurrentDirectory("QuickSave") == 0)
	{
		return false;
	}

	// record each system
	DWORD numObjects = 0;
	for( int i = 0; i < MAX_SYSTEMS; i++ )
	{
		if( !m_SystemList[i].bEmpty )
		{
			numObjects += m_SystemList[i].getNumObjects();
			m_SystemList[i].Save(_fileSystem);
		}
	}

	// out of QuickSave
	_fileSystem.SetCurrentDirectory("..");

	// write the total count of objects

	DWORD dwWritten;
	DAFILEDESC fdesc = "Count";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	COMPTR<IFileSystem> file;
	if (_fileSystem.CreateInstance(&fdesc, file) != GR_OK)
	{
		_fileSystem.GetLastError();
		return false;
	}

	file->WriteFile(0, &numObjects, sizeof(numObjects), &dwWritten);

	// all done

	_fileSystem.SetCurrentDirectory("..");
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool Sector::Load( struct IFileSystem& )
{ 
	return false; 
}
