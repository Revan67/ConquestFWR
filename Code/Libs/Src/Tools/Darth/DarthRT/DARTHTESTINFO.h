// DARTHTESTINFO.h
//
//
//


#ifndef DARTHTESTINFO_H
#define DARTHTESTINFO_H

//

#define DARTH_SHORT_TEXT 128
#define DARTH_LONG_TEXT  2048

//

struct IDarthRuntime;

//

typedef HRESULT (*RunTestType)( IDarthRuntime *RunTime );
typedef HRESULT (*GetTestNameType)( LPTSTR out_Name, int MaxLen );
typedef HRESULT (*GetTestContactType)( LPTSTR out_Contact, int MaxLen );
typedef HRESULT (*GetTestDescriptionType)( LPTSTR out_Description, int MaxLen );
typedef HRESULT (*GetTestCategoryType)( LPTSTR out_Cat, int MaxLen );


//

struct DARTHTESTINFO
{
	HMODULE hDllModule;

	TCHAR Name[DARTH_SHORT_TEXT];
	TCHAR Contact[DARTH_SHORT_TEXT];
	TCHAR Category[DARTH_SHORT_TEXT];
	TCHAR Description[DARTH_LONG_TEXT];

	RunTestType				Run;

	//

	DARTHTESTINFO( )
	{
		hDllModule = 0;
		Name[0] = 0;
		Contact[0] = 0;
		Category[0] = 0;
		Description[0] = 0;
		Run = 0;
	}

	//

	DARTHTESTINFO( const DARTHTESTINFO &dti )
	{
		this->operator=( dti );
	}

	//

	DARTHTESTINFO &operator=( const DARTHTESTINFO &dti )
	{
		hDllModule = dti.hDllModule ;
		_tcscpy( Name, dti.Name ) ;
		_tcscpy( Contact, dti.Contact ) ;
		_tcscpy( Category, dti.Category ) ;
		_tcscpy( Description, dti.Description ) ;
		Run = dti.Run ;
		
		return *this;
	}

	//

	~DARTHTESTINFO( )
	{
	}

	//

	HRESULT Link( HMODULE hMod )
	{
		hDllModule = hMod;

		GetTestNameType GetName			= (GetTestNameType)GetProcAddress( hDllModule, "GetTestName" );
		GetTestContactType GetContact	= (GetTestContactType)GetProcAddress( hDllModule, "GetTestContact" );
		GetTestDescriptionType GetDesc	= (GetTestDescriptionType)GetProcAddress( hDllModule, "GetTestDescription" );
		GetTestCategoryType GetCat		= (GetTestCategoryType)GetProcAddress( hDllModule, "GetTestCategory" );

		if( !GetName || !GetContact || !GetDesc || !GetCat ) {
			return E_FAIL;
		}

		GetName( Name, MAX_PATH );
		GetContact( Contact, MAX_PATH );
		GetDesc( Description, MAX_PATH );
		GetCat( Category, MAX_PATH );

		if( (Run = (RunTestType)GetProcAddress( hDllModule, "RunTest" )) == NULL ) {
			return E_FAIL;
		}

		return S_OK;
	}

};


#endif // eof

