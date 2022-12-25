//--------------------------------------------------------------------------//
//                                                                          //
//                               Scripting.cpp                              //
//                                                                          //
//--------------------------------------------------------------------------//
/*
*/			    
//----------------------------------------------------------------------------------------------
/*
	The scripting engine
*/
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------

#include "pch.h"
#include <globals.h>

#include "Scripting.h"

#include "Startup.h"
#include <TSmartPointer.h>
#include <EventSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <FileSys.h>
#include <HKEvent.h>
#include <SaveLoad.h>
#include <DQuickSave.h>

#include "CQTrace.h"
#include "TerrainMap.h"
#include "IObject.h"
#include "MScript.h"
#include "MPartRef.h"
#include "ObjList.h"
#include "DMBaseData.h"
#include "Sector.h"

#include <map>
#include <string>
#include <malloc.h>

//----------------------------------------------------------------------------------------------
// ScriptParameterList

struct ScriptParameter : IScriptParameter
{
	enum
	{
		MAX_STRING = 64,
	};

	union Value
	{
		IBaseObject* Object;
		long         Long;
		float        Float;
		char         String[MAX_STRING];
		wchar_t      WideString[MAX_STRING];
		GRIDVECTOR   GridVector;
	};

	ScriptParameterType type;
	Value               value;
	std::string         name;

	virtual ScriptParameterType GetType()
	{
		return type;
	}

	virtual void* GetValue()
	{
		switch( type )
		{
			case SPT_OBJECT:	 return value.Object;
			case SPT_LONG:		 return &value.Long;
			case SPT_FLOAT:		 return &value.Float;
			case SPT_STRING:	 return value.String;
			case SPT_WIDESTRING: return value.WideString;
			case SPT_GRIDVECTOR: return &value.GridVector;
		}

		return NULL;
	}

	virtual const char* GetName()
	{
		return name.c_str();
	}

	ScriptParameter(ScriptParameterType _type) : type(_type)
	{
		value.Long = 0;
	}

	~ScriptParameter()
	{
		clear();
	}

	void clear()
	{
		switch( type )
		{
			case SPT_OBJECT:	 value.Object = 0; break;
			case SPT_LONG:		 value.Long   = 0;   break;
			case SPT_FLOAT:		 value.Float  = 0; break;
			case SPT_STRING:	 value.String[0] = 0; break;
			case SPT_WIDESTRING: value.WideString[0] = 0; break;
		}
	}

	void operator = (const ScriptParameter& _other)
	{
		if( type == _other.type )
		{
			clear();

			switch( _other.type )
			{
				case SPT_OBJECT:	 value.Object     = _other.value.Object; break;
				case SPT_LONG:		 value.Long       = _other.value.Long;   break;
				case SPT_FLOAT:		 value.Float      = _other.value.Float; break;
				case SPT_GRIDVECTOR: value.GridVector = _other.value.GridVector; break;

				case SPT_STRING:
				{
					strncpy( value.String, _other.value.String, MAX_STRING-1 );
					value.String[MAX_STRING-1] = 0;
					break;
				}

				case SPT_WIDESTRING:
				{
					wcsncpy( value.WideString, _other.value.WideString, ScriptParameter::MAX_STRING-1 );
					value.WideString[ScriptParameter::MAX_STRING-1] = 0;
					break;
				}
			}

			name = _other.name;
		}
	}

	// memory pool

	enum
	{
		PAGE_SIZE = 1024,
	};

	// pointer to "head object"; start of memory pool
	static ScriptParameter* s_pHead;
	ScriptParameter* m_Next;

	void* operator new( size_t size )
	{
		// take the current head (to be returned), allocate new memory (if so required) and relink the chain

		assert( sizeof(ScriptParameter) == size ); // protect against children (3)

		const int numObj = PAGE_SIZE / sizeof( ScriptParameter );
		const int lastObjInArray = numObj - 1;

		if ( 0 == s_pHead ) 
		{
			// use the global operator new to allocate a block for the pool
			ScriptParameter* pool = reinterpret_cast< ScriptParameter* >(::operator new( numObj * sizeof( ScriptParameter ) ) );

			// initialize the chain (one-directional linked list)
			s_pHead = &pool[0];
			for ( int i = 0; i < lastObjInArray; ++i ) 
			{
				pool[ i ].m_Next = &pool[ i + 1 ];
			}
			pool[ lastObjInArray ].m_Next = 0;
		}

		ScriptParameter* retMemory = s_pHead;
		s_pHead = retMemory->m_Next; // the start of the memory block is forgotten
		return retMemory;	
	}


	void operator delete( void* p, size_t size )
	{
		if ( 0 == p )
		{
			// standard requirement on operator delete
			return;
		}

		// protect against children
		assert( sizeof( ScriptParameter ) == size ); 

		// recycle the object memory, by putting it back into the chain
		reinterpret_cast< ScriptParameter* >( p )->m_Next = s_pHead;
		s_pHead = reinterpret_cast< ScriptParameter* >( p );	
	}

};

// the static memory pool head
ScriptParameter* ScriptParameter::s_pHead = 0;

MEXTERN bool ScriptParameterList::Push( IBaseObject* _object, const char* _paramname )
{
	ScriptParameter* p = new ScriptParameter(SPT_OBJECT);
	p->name = _paramname;
	p->value.Object = _object;
	push_back( p );
	return true;
}

MEXTERN bool ScriptParameterList::Push( U32 _long, const char* _paramname )
{
	ScriptParameter* p = new ScriptParameter(SPT_LONG);
	p->name = _paramname;
	p->value.Long = _long;
	push_back( p );
	return true;
}

MEXTERN bool ScriptParameterList::Push( float _float, const char* _paramname )
{
	ScriptParameter* p = new ScriptParameter(SPT_FLOAT);
	p->name = _paramname;
	p->value.Float = _float;
	push_back( p );
	return true;
}

MEXTERN bool ScriptParameterList::Push( const char* _string, const char* _paramname )
{
	ScriptParameter* p = new ScriptParameter(SPT_STRING);
	p->name = _paramname;
	strncpy( p->value.String, _string, ScriptParameter::MAX_STRING-1 );
	p->value.String[ScriptParameter::MAX_STRING-1] = 0;
	push_back( p );
	return true;
}

MEXTERN bool ScriptParameterList::Push( const wchar_t* _wideString, const char* _paramname )
{
	ScriptParameter* p = new ScriptParameter(SPT_WIDESTRING);
	p->name = _paramname;
	wcsncpy( p->value.WideString, _wideString, ScriptParameter::MAX_STRING-1 );
	p->value.WideString[ScriptParameter::MAX_STRING-1] = 0;
	push_back( p );
	return true;
}

MEXTERN bool ScriptParameterList::Push( GRIDVECTOR* _gridVector, const char* _paramname )
{
	ScriptParameter* p = new ScriptParameter(SPT_GRIDVECTOR);
	p->name = _paramname;
	p->value.GridVector = *_gridVector;
	push_back( p );
	return true;
}

MEXTERN ScriptParameterList::~ScriptParameterList()
{
	for( iterator it = begin(); it != end(); it++ )
	{
		ScriptParameter* p = (ScriptParameter*) *it;
		delete p;
	}
	clear();
}


//----------------------------------------------------------------------------------------------

struct Scripting : public IScripting, public IEventCallback, public ISaverLoader
{
	BEGIN_DACOM_MAP_INBOUND(Scripting)
		DACOM_INTERFACE_ENTRY(IScripting)
		DACOM_INTERFACE_ENTRY(IEventCallback)
		DACOM_INTERFACE_ENTRY(ISaverLoader)
	END_DACOM_MAP()

	// IScripting startup/shutdown

	virtual bool Startup();

	virtual bool Shutdown();

	// IScripting Script Handling

	virtual U32 RunScript (const char * _module, const char * _function, ScriptParameterList* _parameters );

	virtual bool ScriptExists(const char * _module, const char * _function);

	// IScripting Event 

	virtual void CallScriptEvent( ScriptEvent _eventID, ScriptParameterList* _parameters );

	virtual U32 AddEventListener(const char * _module, const char * _function, ScriptEvent _eventID, char* _scriptContext );

	virtual void RemoveEventListener(U32 _listenerID, ScriptEvent _eventID);

	virtual void ScheduleEvent(SINGLE _deltaTime, const char * _module, const char * _function);

	// IScripting Debugging

	virtual const char* GetTraceback( void );

	virtual const char* BuildCallStack( void );

	virtual void AppendScriptPath( const char* _scriptDir );

	virtual bool PushCall( const char* _name );

	virtual bool PopCall( const char* _name );

	virtual bool StartDebugger( const char* _scriptDirectory );

	virtual bool StopDebugger();

	virtual bool StopDebuggerRunning();

	virtual char* GetErrorString();

	// IScripting Compiling

	virtual bool CompileStringBuffer( const char* _buffer, DWORD _bufferSize, const char* _moduleName, bool _printError );

	virtual bool SetConstant( int _constant, char* _constantName, const char* _module );

	// IEventCallback

	DEFMETHOD(Notify) (U32 message, void *param);

	// ISaverLoader

	virtual bool Save( class TiXmlNode& );
	virtual bool Load( class TiXmlNode& );
	virtual bool Save( struct IFileSystem& );
	virtual bool Load( struct IFileSystem& );

	// local data structs

	struct Grid : std::list<GRIDVECTOR>
	{
		void insertObject( IBaseObject* _object )
		{
			push_back( _object->GetGridPosition() );
		}

		bool getStaticList( GRIDVECTOR* _gridList )
		{
			if( size() )
			{
				int i = 0;
				for( iterator it = begin(); it != end(); it++, i++ )
				{
					_gridList[i] = *it;
				}
				return true;
			}
			return false;
		}
	};

	struct Region : FootprintInfo
	{
		std::string m_name;
		Grid        m_grid;
		U32         m_systemID;

		Region() : m_systemID(0) {}
	};

	struct RegionList : std::list<Region>
	{
	};

	struct FieldLinks : std::map< std::string /* script handle */, std::string /* field name */ >
	{
	};

	struct GroupList : std::list<U32 /* uniqueID */ >
	{
	};

	struct GroupMap : std::map< std::string /* group name */, GroupList >
	{
	};

	// local data

	U32        eventHandle;
	FieldLinks m_FieldLinks;
	RegionList m_RegionList;
	GroupMap   m_GroupMap;

	// local methods

	bool loadObjecLinks( struct IFileSystem& _fs );
	bool loadObjectGroups( struct IFileSystem& _fs );
	bool loadScripts( struct IFileSystem& _fs );

	bool applyRegionToTerrainMap( Region& _region );
	bool removeRegionToTerrainMap( Region& _region );

	void update( MSG* _msg );

	IObject* findObjectByName( const char* _scriptHandle )
	{
		FieldLinks::iterator fieldIt = m_FieldLinks.find( _scriptHandle );
		if( fieldIt != m_FieldLinks.end() )
		{
			_scriptHandle = fieldIt->second.c_str();
		}

		IBaseObject::MDATA mdata;
		IBaseObject * obj = OBJLIST->GetObjectList();

		while (obj)
		{
			if (obj->GetMissionData(mdata))
			{
				if (strcmp(_scriptHandle, mdata.pSaveData->partName) == 0)
				{	
					return obj;
				}
			}

			obj = obj->next;
		}

		return obj;
	}
};

//----------------------------------------------------------------------------------------------
// IScripting startup/shutdown

bool Scripting::Startup() 
{ 
	return 0; 
}

bool Scripting::Shutdown() 
{ 
	return 0; 
}

//----------------------------------------------------------------------------------------------
// IScripting Script Handling

U32 Scripting::RunScript (const char * _module, const char * _function, ScriptParameterList* _parameters ) 
{ 
	return 0; 
}

bool Scripting::ScriptExists(const char * _module, const char * _function) 
{ 
	return 0; 
}

//----------------------------------------------------------------------------------------------
// IScripting Event 

void Scripting::CallScriptEvent( ScriptEvent _eventID, ScriptParameterList* _parameters )
{ 
	if( _eventID == SE_SHIP_ENTER )
	{
		int t = 5;
	}
	else if( _eventID == SE_SHIP_EXIT )
	{
		int k = 5;
	}
}

U32 Scripting::AddEventListener(const char * _module, const char * _function, ScriptEvent _eventID, char* _scriptContext ) 
{ 
	return 0; 
}

void Scripting::RemoveEventListener(U32 _listenerID, ScriptEvent _eventID) 
{ 
	return; 
}

void Scripting::ScheduleEvent(SINGLE _deltaTime, const char * _module, const char * _function) 
{ 
	return; 
}

//----------------------------------------------------------------------------------------------
// IScripting Debugging

const char* Scripting::GetTraceback( void ) 
{ 
	return 0; 
}

const char* Scripting::BuildCallStack( void ) 
{ 
	return 0; 
}

void Scripting::AppendScriptPath( const char* _scriptDir ) 
{ 
	return; 
}

bool Scripting::PushCall( const char* _name ) 
{ 
	return 0; 
}

bool Scripting::PopCall( const char* _name ) 
{ 
	return 0; 
}

bool Scripting::StartDebugger( const char* _scriptDirectory ) 
{ 
	return 0; 
}

bool Scripting::StopDebugger() 
{ 
	return 0; 
}

bool Scripting::StopDebuggerRunning() 
{ 
	return 0; 
}

char* Scripting::GetErrorString() 
{ 
	return 0; 
}

//----------------------------------------------------------------------------------------------
// IScripting Compiling

bool Scripting::CompileStringBuffer( const char* _buffer, DWORD _bufferSize, const char* _moduleName, bool _printError ) 
{ 
	return 0; 
}

bool Scripting::SetConstant( int _constant, char* _constantName, const char* _module )  
{ 
	return 0; 
}


//-------------------------------------------------------------------
// receive notifications from event system
//
GENRESULT Scripting::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
		case CQE_GAME_ACTIVE:
			break;

		case CQE_KILL_FOCUS:
			break;

		case CQE_SET_FOCUS:
			break;

		case CQE_UPDATE:
			update(msg);
			break;

		case CQE_HOTKEY:
			break;

		case WM_CLOSE:
			break;

		case CQE_DELETEPLAYER:
			break;

		case CQE_MISSION_PROG_ENDING:
			break;

		case CQE_MISSION_ENDING_SPLASH:
			break;

		case CQE_PLAYER_RESIGN:
			break;

		case CQE_PLAYER_QUIT:
			break;

		case WM_CHAR:
			break;

		case WM_INITMENUPOPUP:
			break;

		case WM_COMMAND:
			break;
	}

	return GR_OK;
}

//----------------------------------------------------------------------------------------------

void Scripting::update( MSG* _msg )
{
	_msg = NULL;
}

//----------------------------------------------------------------------------------------------
// ISaverLoader

bool Scripting::Save( class TiXmlNode& ) 
{ 
	// TODO: import the XML parser code
	return 0; 
}

//----------------------------------------------------------------------------------------------

bool Scripting::Load( class TiXmlNode& ) 
{ 
	// TODO: import the XML parser code
	return 0; 
}

//----------------------------------------------------------------------------------------------

bool Scripting::Save( struct IFileSystem& _fs ) 
{ 
	// what is this going to do?
	return 0; 
}

//----------------------------------------------------------------------------------------------

bool Scripting::Load( struct IFileSystem& _fs )
{ 
	bool ret = 0;

	ret |= loadObjecLinks(_fs);
	ret |= loadObjectGroups(_fs);
	ret |= loadScripts(_fs);

	return ret; 
}

//----------------------------------------------------------------------------------------------

bool Scripting::loadObjecLinks( struct IFileSystem& _fs )
{
	if( _fs.SetCurrentDirectory("\\ObjectLinks") == 0 )
	{
		// no object links...
		return false;
	}

	WIN32_FIND_DATA filedata;
	
	HANDLE hFile = _fs.FindFirstFile("*.*", &filedata);
	if( hFile != INVALID_HANDLE_VALUE )
	{
		do 
		{
			DAFILEDESC desc = filedata.cFileName;
			desc.lpImplementation = "UTF";
			desc.dwDesiredAccess = GENERIC_READ;
			desc.dwShareMode = 0;  // no sharing

			DWORD dwWritten;
			HANDLE hChild = _fs.OpenChild(&desc);
			if( hChild != INVALID_HANDLE_VALUE )
			{
				MT_FIELDLINK link;
				_fs.ReadFile(hChild, &link, sizeof(link), &dwWritten, NULL);
				m_FieldLinks[ link.scriptHandle ] = link.field;
			}
		} 
		while (_fs.FindNextFile(hFile, &filedata));

		_fs.FindClose(hFile);
	}

	return true;
}

//----------------------------------------------------------------------------------------------

bool Scripting::loadObjectGroups( struct IFileSystem& _fs )
{
	if( _fs.SetCurrentDirectory("\\ObjectGroups") == 0 )
	{
		// no object groups...
		return false;
	}

	WIN32_FIND_DATA dirdata;
	dirdata.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	
	HANDLE hDirectory = _fs.FindFirstFile("*.*", &dirdata);
	if( hDirectory != INVALID_HANDLE_VALUE )
	{
		do 
		{
			if( _fs.SetCurrentDirectory(dirdata.cFileName) != 0 )
			{
				bool familyBlock = true;
				WIN32_FIND_DATA filedata;

				// data containers
				Region region;
				GroupList group;

				HANDLE hFile = _fs.FindFirstFile("*.*",&filedata);
				if( hFile != INVALID_HANDLE_VALUE )
				{
					do
					{
						DAFILEDESC desc = filedata.cFileName;
						desc.lpImplementation = "UTF";
						desc.dwDesiredAccess = GENERIC_READ;
						desc.dwShareMode = 0;  // no sharing

						DWORD dwWritten;
						HANDLE hChild = _fs.OpenChild(&desc);
						if( hChild != INVALID_HANDLE_VALUE )
						{
							if( familyBlock )
							{
								MT_OBJECTFAMILY_QLOAD objectFamily;
								_fs.ReadFile(hChild, &objectFamily, sizeof(objectFamily), &dwWritten, NULL);
								region.m_name = objectFamily.name;
								familyBlock = false;
							}
							else
							{
								MT_OBJECTFAMILYENTRY_QLOAD objectEntry;
								_fs.ReadFile(hChild, &objectEntry, sizeof(objectEntry), &dwWritten, NULL);

								// all trigger objects in group make the Region's footprint
								IBaseObject* obj = (IBaseObject*)findObjectByName( objectEntry.scriptHandle );

								// record for "general group" data
								if( obj )
								{
									group.push_back( obj->GetPartID() );
								}

								// record for Trigger Region
								if( obj && obj->objClass == OC_TRIGGER )
								{
									// note, all of these object should be within the same SystemID
									// if not this should be detected and reported

									region.m_systemID = obj->GetSystemID(); 
									region.m_grid.insertObject( obj );
								}
							}

							_fs.CloseHandle( hChild );
						}

					}
					while( _fs.FindNextFile(hFile, &filedata) );

					_fs.SetCurrentDirectory("..");
				}

				// record group
				if( group.size() )
				{
					m_GroupMap[ region.m_name ] = group;
				}

				// record region (if any)
				if( region.m_grid.size() )
				{
					applyRegionToTerrainMap( region );
				}

			}
		} 
		while (_fs.FindNextFile(hDirectory, &dirdata));

		_fs.FindClose(hDirectory);
	}

	// TODO: make group Python Variables for each group
	// TODO: make region Python Variables for each region

	return true;
}

//----------------------------------------------------------------------------------------------

bool Scripting::loadScripts( struct IFileSystem& _fs )
{
	return 0;
}

//----------------------------------------------------------------------------------------------

bool Scripting::applyRegionToTerrainMap( Region& _region )
{
//	#define TERRAIN_IMPASSIBLE	 0x00000001
//	#define TERRAIN_BLOCKLOS     0x00000002
//	#define TERRAIN_FULLSQUARE   0x00000040
//	#define TERRAIN_HALFSQUARE   0x00000080
//	#define TERRAIN_PARKED		 0x00000100
//	#define TERRAIN_MOVING		 0x00000200
//	#define TERRAIN_DESTINATION  0x00000400
//	#define TERRAIN_FIELD		 0x00000800		// nebula, asteroid field, minefield
//	#define TERRAIN_UNITROTATING 0x00001000		// unit is rotating, so ignore out-of-system placements
//	#define TERRAIN_OUTOFSYSTEM  0x00004000
//	#define TERRAIN_WILLBEPLAT	 0x00020000      //footprint for where a platform will soon be
//	#define TERRAIN_REGION	     0x00040000      //footprint also represents a trigger region

	GRIDVECTOR* grids = (GRIDVECTOR*) _alloca( _region.m_grid.size() * sizeof(GRIDVECTOR) );

	if( _region.m_grid.getStaticList(grids) )
	{
		// regions are non-player specific (right now)
		_region.missionID = MGlobals::CreateNewPartID(0);
		_region.height    = 0;
		_region.flags     = TERRAIN_FULLSQUARE | TERRAIN_REGION;

		// the object has to initialize its footprint
		COMPTR<ITerrainMap> map;
		SECTOR->GetTerrainMap(_region.m_systemID, map);
		if( map )
		{
			map->SetFootprint( grids, _region.m_grid.size(), _region );
		}
	}

	m_RegionList.push_back( _region );

	return 0;
}

//----------------------------------------------------------------------------------------------

bool Scripting::removeRegionToTerrainMap( Region& _region )
{
	return 0;
}

//----------------------------------------------------------------------------------------------
//
struct _scripting : GlobalComponent
{
	Scripting* scripting;

	virtual void Startup (void)
	{
		SCRIPTING = scripting = new DAComponent<Scripting>;
		AddToGlobalCleanupList((IDAComponent **) &SCRIPTING);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		if( EVENTSYS && EVENTSYS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK )
		{
			connection->Advise(SCRIPTING, &scripting->eventHandle);
		}
	}
};

static _scripting __scripting;
