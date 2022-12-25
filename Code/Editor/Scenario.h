//
// Scenario.h
//

#ifndef SCENARIO_INTERFACE_HEADER_H_
#define SCENARIO_INTERFACE_HEADER_H_

struct ISector;
struct IObjectFamily;
struct IObject;

struct IScenario : public IDAComponent
{
	// settings

	struct Settings
	{
		wchar_t        name[128];
		char           nameTag[64]; // over rides the "name" member, is a tag from the string table
		DWORD          maxResources;
		char           scriptBriefing[128];
		char           scriptDebriefing[128];
		FILETIME       lastSaved;
		IObjectFamily* objectFamily;
		bool           loaded;

		Settings()
		{
			name[0] = 0;
			nameTag[0] = 0;
			objectFamily = NULL;
			loaded = false;
		}

	};

	virtual bool SetSettings( Settings& ) = 0;

	virtual Settings& GetSettings( void ) = 0;

	// operations

	virtual ISector* GetActiveSector() = 0;

	virtual struct ISector* NewSector( const wchar_t* _name ) = 0;

	virtual bool DeleteSector( struct ISector* _sector ) = 0;

	virtual bool Prepare( U32 _sectorID ) = 0; // called when the sceneario want to load systems in a sector

	virtual bool Finish( U32 _sectorID ) = 0; // called when the sceneario wants to save systems in a sector

	// save/load

	virtual bool Save( const char* _filename ) = 0;

	virtual bool Load( const char* _filename ) = 0;

	virtual bool Save( struct IFileSystem& ) = 0;

	// helper methods

	virtual IObject* FindObjectByScriptHandle( const char* _scriptHandle ) = 0;
};

// factory
namespace Scenario
{
	IScenario* New();
	bool       Delete(IScenario*);
};

#endif