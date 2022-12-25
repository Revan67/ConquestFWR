//
// Campaign.h
//

#ifndef CAMPAIGN_INTERFACE_HEADER_H_
#define CAMPAIGN_INTERFACE_HEADER_H_

struct IScenario;

struct ICampaign : public IDAComponent
{
	struct Settings
	{
		FILETIME lastModified;
		wchar_t  name[128];
		char     nameTag[64];   // over rides the "name" member, is a tag from the string table

		Settings()
		{
			name[0] = 0;
			nameTag[0] = 0;
		}
	};

	// scenario list

	virtual bool New( const wchar_t* _name, Settings* _settings = NULL ) = 0;

	virtual IScenario* GetCurrentScenario() = 0;

	virtual bool SetCurrentScenario( IScenario* ) = 0;

	virtual bool GetScenarioList( IScenario** _list, int _listSize) = 0;

	virtual bool AddScenario( IScenario* ) = 0;

	virtual bool RemoveScenario( IScenario* ) = 0;

	virtual U32  GetNumScenarios() = 0;

	// editing of campaign settings

	virtual bool SetSettings( Settings& ) = 0;

	virtual Settings& GetSettings( void ) = 0;

	virtual bool SetBackground( const char* _filename ) = 0;
};

#endif