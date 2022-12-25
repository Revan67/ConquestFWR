//
// Campaign.cpp
//

#include "stdafx.h"
#include "globals.h"
#include "Campaign.h"

#include "Startup.h"
#include "SaveLoad.h"
#include "Scenario.h"
#include "SystemStructs.h"
#include "tinyxml\tinyxml.h"
#include "StringTable.h"

#include <TComponent.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <Engine.h>
#include <EventSys.h>
#include <system.h>
#include <HKEvent.h>

#include <list>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

struct Campaign : public ICampaign, public ISaverLoader
{
	BEGIN_DACOM_MAP_INBOUND(Campaign)
		DACOM_INTERFACE_ENTRY(ICampaign)
		DACOM_INTERFACE_ENTRY(ISaverLoader)
	END_DACOM_MAP()

	typedef std::list<IScenario*> SCENARIOLIST;

	SCENARIOLIST m_ScenarioList;
	IScenario*   m_CurrentScenario;
	Settings     m_Settings;

	// scenario list

	virtual bool New( const wchar_t* _name, Settings* _settings );

	virtual IScenario* GetCurrentScenario();

	virtual bool SetCurrentScenario( IScenario* );

	virtual bool GetScenarioList( IScenario** _list, int _listSize);

	virtual bool AddScenario( IScenario* );

	virtual bool RemoveScenario( IScenario* );

	virtual U32 GetNumScenarios();

	// visual editing of campaing screen

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
		return m_Settings;
	}

	virtual bool SetBackground( const char* _filename ) { return true; }

	// ISaverLoader

	virtual bool Save( class TiXmlNode& );
	virtual bool Load( class TiXmlNode& );

	virtual bool Save( struct IFileSystem& );
	virtual bool Load( struct IFileSystem& );

	// locals

	Campaign()
	{
		m_CurrentScenario = NULL;
		ZeroMemory( &m_Settings, sizeof(m_Settings) );
		New(L"NONAMECAMPAIGN",NULL);
	}

	virtual ~Campaign()
	{
		// get rid of the rest of the scenarios
		for( SCENARIOLIST::iterator it = m_ScenarioList.begin(); it != m_ScenarioList.end(); it++ )
		{
			Scenario::Delete( *it );
		}
	}
};

//-----------------------------------------------------------------------------------------------------

bool Campaign::New( const wchar_t* _name, Settings* _settings )
{
	// supply name
	wcscpy( m_Settings.name, _name);

	// get time for "new campaign"
	SYSTEMTIME systemTime;
	GetSystemTime( &systemTime );
	SystemTimeToFileTime( &systemTime, &m_Settings.lastModified );

	return true;
}

//-----------------------------------------------------------------------------------------------------

IScenario* Campaign::GetCurrentScenario() 
{ 
	return m_CurrentScenario;
}

//-----------------------------------------------------------------------------------------------------

bool Campaign::SetCurrentScenario( IScenario* _s ) 
{ 
	// make sure this scenario exists...
	for( SCENARIOLIST::iterator it = m_ScenarioList.begin(); it != m_ScenarioList.end(); it++ )
	{
		if( *it == _s )
		{
			m_CurrentScenario = _s;
			return true;
		}
	}
	return false; 
}

//-----------------------------------------------------------------------------------------------------

bool Campaign::GetScenarioList( IScenario** _list, int _listSize) 
{ 
	int count = 0;

	// counting all
	for( SCENARIOLIST::iterator it = m_ScenarioList.begin(); it != m_ScenarioList.end(); it++ )
	{
		if( count < _listSize )
		{
			_list[count] = *it;
			count++;
		}
	}
	return( count <= _listSize );
}

//-----------------------------------------------------------------------------------------------------

bool Campaign::AddScenario( IScenario* _s ) 
{ 
	// check for duplicate pointers
	for( SCENARIOLIST::iterator it = m_ScenarioList.begin(); it != m_ScenarioList.end(); it++ )
	{
		if( *it == _s )
		{
			return true;
		}
	}

	// check for duplicate name
	const wchar_t* newname = _s->GetSettings().name;
	for( it = m_ScenarioList.begin(); it != m_ScenarioList.end(); it++ )
	{
		IScenario* scenario = *it;
		if( !wcscmp(newname, scenario->GetSettings().name) )
		{
			// make sure to invalidate current scenario...
			if( scenario == m_CurrentScenario )
			{
				m_CurrentScenario = NULL;
			}

			it++;
			RemoveScenario( scenario );
		}
	}

	m_ScenarioList.push_back( _s );
	return true; 
}

//-----------------------------------------------------------------------------------------------------

bool Campaign::RemoveScenario( IScenario* _s ) 
{ 
	if( _s == m_CurrentScenario )
	{
		m_CurrentScenario = NULL;
	}

	// make sure this scenario exists...
	for( SCENARIOLIST::iterator it = m_ScenarioList.begin(); it != m_ScenarioList.end(); it++ )
	{
		if( *it == _s )
		{
			Scenario::Delete( _s );
			m_ScenarioList.erase( it );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

U32 Campaign::GetNumScenarios()
{
	return m_ScenarioList.size();
}

//-----------------------------------------------------------------------------------------------------

bool Campaign::Save( class TiXmlNode& _node ) 
{ 
	TiXmlElement campaign ("CAMPAIGN");
	campaign.SetAttribute( "name", m_Settings.name );
	campaign.SetAttribute( "date_high", m_Settings.lastModified.dwHighDateTime );
	campaign.SetAttribute( "date_low", m_Settings.lastModified.dwLowDateTime );

	// save off the "global string table" for this entire campaign
	COMPTR<ISaverLoader> stringTableSaver;
	STRINGTABLE->QueryInterface( "ISaverLoader", stringTableSaver );
	if( stringTableSaver )
	{
		stringTableSaver->Save(campaign);
	}

	// save the list of scenarios
	for( SCENARIOLIST::iterator it = m_ScenarioList.begin(); it != m_ScenarioList.end(); it++ )
	{
		IScenario* s = *it;

		// a scenario is loaded and saved in different files so that they can be source controlled
		CArchive* pArch = (CArchive*)_node.GetUserData();

		CString filepath = pArch->GetFile()->GetFilePath();

		if( filepath.ReverseFind('\\') != -1 )
		{
			filepath = filepath.GetBufferSetLength( filepath.ReverseFind('\\') );
		}

		// make filename
		CString filename( s->GetSettings().name );
		filename += ".scenario";

		// make an entry for this file
		TiXmlElement scenario ("SCENARIO");
		scenario.SetAttribute("file", filename );
		campaign.InsertEndChild( scenario );

		// make full filename
		CString fn = filepath;
		fn += CString("\\");
		fn += filename;

		// save sceneario
		s->Save(fn);
	}

	return( _node.InsertEndChild(campaign) != NULL );
}

//-----------------------------------------------------------------------------------------------------

bool Campaign::Load( TiXmlNode& _doc )
{
	TiXmlElement* campaign = _doc.FirstChildElement("CAMPAIGN");
	if( campaign )
	{
		// note that even trying to load a campaign will ERASE the current string table
		STRINGTABLE->Reset();

		SCENARIOLIST list;

		// try to load all the scenario files
		TiXmlElement* scenario = campaign->FirstChildElement("SCENARIO");
		while( scenario )
		{
			IScenario* s = Scenario::New();
			if( s )
			{
				const char* fn = scenario->Attribute("file");
				if( ::GetFileAttributes(fn) != 0xFFFFFFFF )
				{
					TiXmlDocument doc;
					if( doc.LoadFile(fn) )
					{
						COMPTR<ISaverLoader> loader;
						if( s->QueryInterface("ISaverLoader",loader) == GR_OK )
						{
							if( loader->Load(doc) )
							{
								list.push_back(s);
							}
						}
					}
				}

				// verify that the scenario loaded
				if( !s->GetActiveSector() )
				{
					Scenario::Delete(s);
				}
			}

			scenario = scenario->NextSiblingElement("SCENARIO");
		}

		// must have some scenarios to override current CAMPAIGN
		if( list.size() )
		{
			// nuke all current scenarios
			for( SCENARIOLIST::iterator it = m_ScenarioList.begin(); it != m_ScenarioList.end(); )
			{
				Scenario::Delete( *it );
				it = m_ScenarioList.erase( it );
			}

			// add all new scenarios
			for( it = list.begin(); it != list.end(); it++ )
			{
				AddScenario( *it );
			}

			// get name of campaign
			wchar_t* wideStringValue;
			if( campaign->QueryUnicodeValue( "name", &wideStringValue ) == TIXML_SUCCESS )
			{
				wcsncpy( m_Settings.name, wideStringValue, countof(m_Settings.name) );
				m_Settings.name[countof(m_Settings.name)-1] = 0;
			}

			// get last mod time
			m_Settings.lastModified.dwHighDateTime = campaign->GetAttributeUnsignedLong("date_high");
			m_Settings.lastModified.dwLowDateTime  = campaign->GetAttributeUnsignedLong("date_low");

			// did not fail anywhere...
			return true;
		}
	}

	return false; 
}

//-----------------------------------------------------------------------------------------------------

bool Campaign::Save( struct IFileSystem& )
{ 
	return false; 
}

//-----------------------------------------------------------------------------------------------------

bool Campaign::Load( struct IFileSystem& )
{ 
	return false; 
}

//-----------------------------------------------------------------------------------------------------
// startup

struct _Campaign : GlobalComponent
{
	Campaign * campaign;

	virtual void Startup (void)
	{
		CAMPAIGN = campaign = new DAComponent<Campaign>;
		AddToGlobalCleanupList((IDAComponent **) &CAMPAIGN);
	}

	virtual void Initialize (void)
	{
		IScenario* s    = Scenario::New();
		ISector* sector = s->NewSector(L"NewSector");
		System* system  = sector->NewSystem(L"NewSystem");

		system->fRect.UpperLeftCorner.X  = 0;
		system->fRect.UpperLeftCorner.Y  = 0;
		system->fRect.LowerRightCorner.X = 0.2;
		system->fRect.LowerRightCorner.Y = 0.2; 

		sector->GetDefaultSystemSize( system->sizeX, system->sizeY );
		sector->SetCurrentSystem( system->id );

		CAMPAIGN->AddScenario(s);
		CAMPAIGN->SetCurrentScenario(s);
	}
};
static _Campaign __Campaign;
