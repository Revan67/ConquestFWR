//
// StringTable.cpp
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "StringTable.h"
#include "tcomponent.h"
#include "typedefs.h"
#include <malloc.h>
#include "persistchannel.h"
#include "filesys.h"
#include "da_heap_utility.h"
#include <TSmartPointer.h>
#include "SaveLoad.h"
#include "..\XML\tinyxml.h"

#include <IConnection.h>
#include <Engine.h>
#include <EventSys.h>
#include <system.h>
#include <FileSys.h>

#include <list>
#include <map>
#include <string>

#define CLSID_StringTable "IStringTable"

struct StringTable : public IStringTable, public ISaverLoader, public IAggregateComponent
{
	BEGIN_DACOM_MAP_INBOUND(StringTable)
		DACOM_INTERFACE_ENTRY(IStringTable)
		DACOM_INTERFACE_ENTRY2(IID_IStringTable,IStringTable)
		DACOM_INTERFACE_ENTRY(IAggregateComponent)
		DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
		DACOM_INTERFACE_ENTRY(ISaverLoader)
		DACOM_INTERFACE_ENTRY2(IID_ISaverLoader,ISaverLoader)
	END_DACOM_MAP()

	// IAggregateComponent 
	GENRESULT COMAPI Initialize(void) { return GR_OK; }
	GENRESULT init( AGGDESC *desc );

	// IStringTable

	virtual const wchar_t* GetStringByID( U32 _stringID );

	virtual const wchar_t* GetStringByTag( const char* _tag );

	virtual const char* GetStringTag( U32 _stringID );

	virtual U32 GetStringIdByTag( const char* _tag );

	virtual bool SetLanguage( LANGID _langid );

	virtual LANGID GetLanguage( void );

	virtual U32 NewString( wchar_t* _stringData, const char* _stringTag, LANGID _langID );

	virtual bool SetString( U32 _uid, wchar_t* _stringData, const char* _stringTag, LANGID _langID );

	virtual void Reset( void );

	virtual void EnumerateStringInfo( IStringEnum& _enumerator );

	// ISaverLoader

	virtual bool Save( class TiXmlNode& );
	virtual bool Load( class TiXmlNode& );

	virtual bool Save( struct IFileSystem& );
	virtual bool Load( struct IFileSystem& );

	// data

	IDAComponent * systemServices;

	typedef std::wstring WideString;
	typedef std::string  ByteString;

	struct StringInfo
	{
		std::wstring wideString;
		U32          stringID;
	};

	typedef std::map<ByteString,StringInfo> STRING_MAP;

	struct Dictionary : public STRING_MAP
	{
		LANGID     langID;
		U32        nextStringId;

		Dictionary(LANGID _langID)
		{
			langID	     = _langID;
			nextStringId = 0xFFFF + 1;
		}
	};

	typedef std::list<Dictionary> DICTIONARY_LIST;
	typedef DICTIONARY_LIST::iterator  DICTIONARY;

	DICTIONARY_LIST m_DictionaryList;
	DICTIONARY      m_Dictionary;

	// locals

	StringTable() : systemServices(NULL)
	{
		Reset();
	}

	virtual ~StringTable()
	{
		m_DictionaryList.clear();
	}

	DICTIONARY getDictionaryByLangID( LANGID _langid )
	{
		DICTIONARY dit;
		for( dit = m_DictionaryList.begin(); dit != m_DictionaryList.end(); dit++ )
		{
			if( (*dit).langID == _langid )
			{
				return dit;
			}
		}
		return dit;
	}

	char* getLanguage( LANGID _langid )
	{
		static char buffer[128];
		memset( buffer, 0, 128 );

		char* pBuffer = buffer;
		strcpy( pBuffer, "LANGUAGE\t" );
		pBuffer = strchr(pBuffer,0);

		wchar_t* wideLang = getLang(_langid);
		if( wideLang )
		{
			::WideCharToMultiByte( CP_ACP, 0, wideLang, (int)wcslen(wideLang), pBuffer, 96, NULL, NULL );

			strcat( pBuffer, "\t" );
			pBuffer = strchr( pBuffer, '\t' ) + 1;

			wideLang = getSubLang(_langid);
			if( wideLang )
			{
				::WideCharToMultiByte( CP_ACP, 0, wideLang, (int)wcslen(wideLang), pBuffer, 64, NULL, NULL );
			}
		}

		return buffer;
	}

	LANGID getLanguage( const char* _langname )
	{
		static wchar_t buffer[128];
		buffer[0] = 0;

		::MultiByteToWideChar( CP_ACP, 0, _langname, (int)strlen(_langname), buffer, 128 );
		return getLanguage( buffer );
	}

	STRING_MAP::iterator getStringByID( U32 _stringID, Dictionary& _dictionary )
	{
		// slower operation than find by tag, but quicker compare than string compare
		for( STRING_MAP::iterator it = _dictionary.begin(); it != _dictionary.end(); it++ )
		{
			if( it->second.stringID == _stringID )
			{
				return it;
			}
		}
		return _dictionary.end();
	}

	bool      replaceStringData( const char* _tag, wchar_t* _wideString, WORD _langID);
	wchar_t * getLang(WORD wLang);
	wchar_t * getSubLang(WORD wLang);
	DWORD     getPrimaryLangID(wchar_t * lang);
	DWORD     getSubLangID(wchar_t * lang);
	WORD      getLanguage(wchar_t * lang);
	void      DEBUG_print();
};


//-----------------------------------------------------------------------------------------------------

GENRESULT StringTable::init( AGGDESC *desc )
{ 
	// This is called during normal use.  We are a normal aggregate.
	// Specifically, this is a system aggregate.
	//
	
	systemServices = desc->outer;

	return GR_OK;
}

//-----------------------------------------------------------------------------------------------------

const wchar_t* StringTable::GetStringByID( U32 _stringID )
{
	STRING_MAP::iterator it = getStringByID( _stringID, *m_Dictionary );

	if( it != (*m_Dictionary).end() )
	{
		return it->second.wideString.c_str();
	}

	return NULL;
}

//-----------------------------------------------------------------------------------------------------

const wchar_t* StringTable::GetStringByTag( const char* _tag )
{
	STRING_MAP& stringMap = *m_Dictionary;
	STRING_MAP::iterator str = stringMap.find( _tag );

	if( str != stringMap.end() )
	{
		return str->second.wideString.c_str();
	}

	return NULL;
}

//-----------------------------------------------------------------------------------------------------

const char* StringTable::GetStringTag( U32 _stringID )
{
	STRING_MAP& stringMap = *m_Dictionary;

	for( STRING_MAP::iterator it = stringMap.begin(); it != stringMap.end(); it++ )
	{
		if( it->second.stringID == _stringID )
		{
			return it->first.c_str();
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------------------------------

U32 StringTable::GetStringIdByTag( const char* _tag )
{
	STRING_MAP& stringMap = *m_Dictionary;
	STRING_MAP::iterator str = stringMap.find( _tag );

	if( str != stringMap.end() )
	{
		return str->second.stringID;
	}

	return 0;
}

//-----------------------------------------------------------------------------------------------------

bool StringTable::SetLanguage( LANGID _langid )
{
	for( DICTIONARY dit = m_DictionaryList.begin(); dit != m_DictionaryList.end(); dit++ )
	{
		if( (*dit).langID == _langid )
		{
			m_Dictionary = dit;
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

LANGID StringTable::GetLanguage( void )
{
	if( m_Dictionary != m_DictionaryList.end() )
	{
		return (*m_Dictionary).langID;
	}
	return 0;
}

//-----------------------------------------------------------------------------------------------------

U32 StringTable::NewString( wchar_t* _stringData, const char* _stringTag, LANGID _langID )
{
	DICTIONARY dict = getDictionaryByLangID(_langID);

	if( dict == m_DictionaryList.end() )
	{
		// note, this does NOT change the current dictionary; just adds a new one to the list
		Dictionary d(_langID);
		m_DictionaryList.push_front(d);
		dict = m_DictionaryList.begin();
	}

	STRING_MAP& stringMap = *dict;

	U32 stringID = (*dict).nextStringId;
	while( getStringByID(stringID,*dict) != stringMap.end() )
	{
		stringID++;
	}
	(*dict).nextStringId = stringID;

	std::string tag;
	if( !_stringTag )
	{
		// need to make a new one
		char newTag[32];
		sprintf(newTag,"wsid_%04x",stringID);
		tag = newTag;
	}
	else
	{
		tag = _stringTag;
	}

	WideString wstr(_stringData);

	StringInfo info;
	info.wideString = _stringData;
	info.stringID   = stringID;

	stringMap[tag] = info;
	return stringID;
}

//-----------------------------------------------------------------------------------------------------

bool StringTable::replaceStringData( const char* _tag, wchar_t* _wideString, WORD _langID)
{
	DICTIONARY dictionary = getDictionaryByLangID(_langID);
	if( dictionary == m_DictionaryList.end() )
	{
		return false;
	}

	STRING_MAP::iterator stringIt = (*dictionary).find( _tag );
	if( stringIt == (*dictionary).end() )
	{
		return false;
	}

	// replace string
	StringInfo info;
	info.wideString = _wideString;
	info.stringID   = stringIt->second.stringID;
	(*dictionary)[_tag] = info;

	// all done and good
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool StringTable::SetString( U32 _stringID, wchar_t* _stringData, const char* _stringTag, LANGID _langID )
{
	DICTIONARY dictionary = getDictionaryByLangID(_langID);
	if( dictionary == m_DictionaryList.end() )
	{
		return false;
	}

	STRING_MAP::iterator stringIt = getStringByID( _stringID, *dictionary );
	if( stringIt == (*dictionary).end() )
	{
		return false;
	}

	// remove old one
	(*dictionary).erase( stringIt );

	// insert new string one
	StringInfo info;
	info.wideString = _stringData;
	info.stringID   = _stringID;
	(*dictionary)[_stringTag] = info;

	// all done and good
	return true;
}

//-----------------------------------------------------------------------------------------------------

void StringTable::Reset( void )
{
	m_DictionaryList.clear();

	// set up default language dictionary
	Dictionary dict( GetSystemDefaultLCID() );
	m_DictionaryList.push_front( dict );

	// assign default dictionary
	m_Dictionary = m_DictionaryList.begin();
}

//-----------------------------------------------------------------------------------------------------

void StringTable::EnumerateStringInfo( IStringEnum& _enumerator )
{
	IStringEnum::StringInfo iseStringInfo;

	for( STRING_MAP::iterator it = (*m_Dictionary).begin(); it != (*m_Dictionary).end(); it++ )
	{
		StringInfo& info = it->second;

		iseStringInfo.idString	 = info.stringID;
		iseStringInfo.tagString	 = it->first.c_str();
		iseStringInfo.wideString = info.wideString.c_str();

		_enumerator.EnumStringInfo( iseStringInfo );
	}
}

//-----------------------------------------------------------------------------------------------------

bool StringTable::Save( class TiXmlNode& _node )
{
	TiXmlElement dictionaryList("DICTIONARY_LIST");

	for( DICTIONARY dit = m_DictionaryList.begin(); dit != m_DictionaryList.end(); dit++ )
	{
		Dictionary& dict = *dit;

		TiXmlElement dictionary( "DICTIONARY" );

		dictionary.SetAttribute( "language", getLanguage(dict.langID) );
		dictionary.SetAttribute( "nextStringId", dict.nextStringId );

		for( Dictionary::iterator it = dict.begin(); it != dict.end(); it++ )
		{
			StringInfo& info = it->second;

			TiXmlElement unicodeString ( it->first.c_str() );
			unicodeString.SetAttribute( "stringID", info.stringID );
			unicodeString.SetAttribute( "wideString", info.wideString.c_str() );

			dictionary.InsertEndChild( unicodeString );
		}

		dictionaryList.InsertEndChild( dictionary );
	}

	_node.InsertEndChild( dictionaryList );

	return true;
}

//-----------------------------------------------------------------------------------------------------

bool StringTable::Load( class TiXmlNode& _node )
{
	TiXmlElement* dictionaryList = _node.FirstChildElement("DICTIONARY_LIST");

	if( dictionaryList )
	{
		TiXmlElement* dictionary = dictionaryList->FirstChildElement("DICTIONARY");

		while( dictionary )
		{
			LANGID langid = getLanguage( dictionary->Attribute("language") );

			TiXmlNode* stringInfo = dictionary->FirstChild();

			while( stringInfo )
			{
				if( stringInfo->ToElement() )
				{
					wchar_t* wideString = 0;
					stringInfo->ToElement()->QueryUnicodeValue( "wideString", &wideString );

					if( wideString )
					{
						if( !replaceStringData(stringInfo->Value(),wideString,langid) )
						{
							NewString( wideString, stringInfo->Value(), langid );
						}
					}
				}

				stringInfo = stringInfo->NextSibling();
			}

			dictionary = dictionary->NextSiblingElement();
		}
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

bool StringTable::Save( struct IFileSystem& _fs )
{
	_fs.CreateDirectory(CLSID_StringTable);

	if( _fs.SetCurrentDirectory(CLSID_StringTable) )
	{
		for( DICTIONARY dit = m_DictionaryList.begin(); dit != m_DictionaryList.end(); dit++ )
		{
			Dictionary& dict = *dit;

			char* langDir = getLanguage(dict.langID);

			// turn tabs into dots
			char* tab = strchr(langDir,'\t');
			while(tab)
			{
				*tab = '.';
				tab++;
				tab = strchr(tab,'\t');
			}

			if( _fs.CreateDirectory(langDir) && _fs.SetCurrentDirectory(langDir) )
			{
				for( Dictionary::iterator it = dict.begin(); it != dict.end(); it++ )
				{
					StringInfo& info = it->second;

					DAFILEDESC fdesc = it->first.c_str();
					fdesc.lpImplementation = "UTF";
					fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
					fdesc.dwShareMode = 0;  // no sharing
					fdesc.dwCreationDistribution = CREATE_NEW; // fail if file already exists

					HANDLE hFile = _fs.OpenChild(&fdesc);
					if( hFile != INVALID_HANDLE_VALUE )
					{
						// prepare data
						const wchar_t* wideString = info.wideString.c_str();
						wchar_t nullChar = 0;
						DWORD dwWritten = 0;
						DWORD strlen = wcslen(wideString);

						// for some reason just writing to the file the number of bytes wanted writes trash after the 10th bytes or so...
						while( wideString[0] != 0 )
						{
							_fs.WriteFile( hFile, wideString, sizeof(wchar_t), &dwWritten, 0 );
							wideString++;
						}
						_fs.WriteFile( hFile, &nullChar, sizeof(nullChar), &dwWritten);
						_fs.CloseHandle( hFile );

//						// commit data
//						_fs.WriteFile( hFile, wideString, strlen, &dwWritten, 0 );
//						_fs.WriteFile( hFile, &nullChar, sizeof(nullChar), &dwWritten);
//						_fs.CloseHandle( hFile );
					}
				}

				// out of string table
				_fs.SetCurrentDirectory("..");
			}
		}
		// out of dictionary list
		_fs.SetCurrentDirectory("..");
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

bool StringTable::Load( struct IFileSystem& _fs )
{
	if( _fs.SetCurrentDirectory(CLSID_StringTable) )
	{
		WIN32_FIND_DATA data;
		DAFILEDESC fdesc;

		HANDLE handle = _fs.FindFirstFile("*.*", &data);

		if( handle == INVALID_HANDLE_VALUE )
		{
			return false;
		}
		fdesc.hFindFirst = handle;

		do
		{
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{

				COMPTR<IFileSystem> f;
				DAFILEDESC fdesc = data.cFileName;
				fdesc.lpImplementation = "UTF";
				fdesc.dwDesiredAccess = GENERIC_READ;
				fdesc.dwShareMode = 0;  // no sharing

				if( _fs.CreateInstance(&fdesc,f) == GR_OK )
				{
					char langname[128];
					strcpy( langname, data.cFileName );

					// turn dots to tabs
					char* dot = strchr(langname,'.');
					while(dot)
					{
						*dot = '\t';
						dot++;
						dot = strchr(dot,'.');
					}

					LANGID langid = getLanguage(langname);

					if( langid )
					{
						WIN32_FIND_DATA fileData;
						HANDLE fileHandle = f->FindFirstFile("*.*", &fileData);

						while( fileHandle != INVALID_HANDLE_VALUE )
						{
							DAFILEDESC fdesc = fileData.cFileName;
							fdesc.lpImplementation = "UTF";
							fdesc.dwDesiredAccess = GENERIC_READ;
							fdesc.dwShareMode = 0;  // no sharing

							HANDLE hFile = f->OpenChild(&fdesc);
							if( hFile != INVALID_HANDLE_VALUE )
							{
								int fileSize = f->GetFileSize(hFile);
								wchar_t* wideString = (wchar_t*)alloca( fileSize );
								memset( wideString, 0, fileSize );

								DWORD dwWritten = 0;
								f->ReadFile(hFile, wideString, fileSize, &dwWritten);
								f->CloseHandle(hFile);

								NewString( wideString, fileData.cFileName, langid );
							}

							if( f->FindNextFile(fileHandle, &fileData) == false )
							{
								fileHandle = INVALID_HANDLE_VALUE;
							}
						}
					}

				}

			}
		}
		while( _fs.FindNextFile(handle, &data) );

		_fs.FindClose(handle);
	}

	return 1;
}

//-----------------------------------------------------------------------------------------------------

void StringTable::DEBUG_print()
{
#ifdef CString
	OutputDebugString("\n++++++++++++++++ " CLSID_StringTable "++++++++++++++++\n");

	for( DICTIONARY dit = m_DictionaryList.begin(); dit != m_DictionaryList.end(); dit++ )
	{
		Dictionary& dict = *dit;

		CString out = CString(getLanguage(dict.langID)) + CString("\n");
		OutputDebugString( out );

		for( Dictionary::iterator it = dict.begin(); it != dict.end(); it++ )
		{
			StringInfo& info = it->second;

			out.Format("%s[%d] - ", it->first.c_str(), info.stringID );
			out += CString( info.wideString.c_str() );
			out += "\n";

			OutputDebugString( out );
		}
	}
#endif
}

//--------------------------------------------------------------------------//
//
wchar_t * StringTable::getLang(WORD wLang)
{
	wLang = PRIMARYLANGID(wLang);
	if(wLang == LANG_AFRIKAANS)
		return L"LANG_AFRIKAANS";
	else if(wLang == LANG_KONKANI)
		return L"LANG_KONKANI";
	else if(wLang == LANG_ALBANIAN)
		return L"LANG_ALBANIAN";
	else if(wLang == LANG_KOREAN)
		return L"LANG_KOREAN";
	else if(wLang == LANG_ARABIC)
		return L"LANG_ARABIC";
	else if(wLang == LANG_LATVIAN)
		return L"LANG_LATVIAN";
	else if(wLang == LANG_ARMENIAN)
		return L"LANG_ARMENIAN";
	else if(wLang == LANG_LITHUANIAN)
		return L"LANG_LITHUANIAN";
	else if(wLang == LANG_ASSAMESE)
		return L"LANG_ASSAMESE";
	else if(wLang == LANG_MACEDONIAN)
		return L"LANG_MACEDONIAN";
	else if(wLang == LANG_AZERI)
		return L"LANG_AZERI";
	else if(wLang == LANG_MALAY)
		return L"LANG_MALAY";
	else if(wLang == LANG_BASQUE)
		return L"LANG_BASQUE";
	else if(wLang == LANG_MALAYALAM)
		return L"LANG_MALAYALAM";
	else if(wLang == LANG_BELARUSIAN)
		return L"LANG_BELARUSIAN";
	else if(wLang == LANG_MANIPURI)
		return L"LANG_MANIPURI";
	else if(wLang == LANG_BENGALI)
		return L"LANG_BENGALI";
	else if(wLang == LANG_MARATHI)
		return L"LANG_MARATHI";
	else if(wLang == LANG_BULGARIAN)
		return L"LANG_BULGARIAN";
	else if(wLang == LANG_NEPALI)
		return L"LANG_NEPALI";
	else if(wLang == LANG_CATALAN)
		return L"LANG_CATALAN";
	else if(wLang == LANG_NEUTRAL)
		return L"LANG_NEUTRAL";
	else if(wLang == LANG_CHINESE)
		return L"LANG_CHINESE";
	else if(wLang == LANG_NORWEGIAN)
		return L"LANG_NORWEGIAN";
	else if(wLang == LANG_CROATIAN)
		return L"LANG_CROATIAN";
	else if(wLang == LANG_ORIYA)
		return L"LANG_ORIYA";
	else if(wLang == LANG_CZECH)
		return L"LANG_CZECH";
	else if(wLang == LANG_POLISH)
		return L"LANG_POLISH";
	else if(wLang == LANG_DANISH)
		return L"LANG_DANISH";
	else if(wLang == LANG_PORTUGUESE)
		return L"LANG_PORTUGUESE";
	else if(wLang == LANG_DUTCH)
		return L"LANG_DUTCH";
	else if(wLang == LANG_PUNJABI)
		return L"LANG_PUNJABI";
	else if(wLang == LANG_ENGLISH)
		return L"LANG_ENGLISH";
	else if(wLang == LANG_ROMANIAN)
		return L"LANG_ROMANIAN";
	else if(wLang == LANG_ESTONIAN)
		return L"LANG_ESTONIAN";
	else if(wLang == LANG_RUSSIAN)
		return L"LANG_RUSSIAN";
	else if(wLang == LANG_FAEROESE)
		return L"LANG_FAEROESE";
	else if(wLang == LANG_SANSKRIT)
		return L"LANG_SANSKRIT";
	else if(wLang == LANG_FARSI)
		return L"LANG_FARSI";
	else if(wLang == LANG_SERBIAN)
		return L"LANG_SERBIAN";
	else if(wLang == LANG_FINNISH)
		return L"LANG_FINNISH";
	else if(wLang == LANG_SINDHI)
		return L"LANG_SINDHI";
	else if(wLang == LANG_FRENCH)
		return L"LANG_FRENCH";
	else if(wLang == LANG_SLOVAK)
		return L"LANG_SLOVAK";
	else if(wLang == LANG_GEORGIAN)
		return L"LANG_GEORGIAN";
	else if(wLang == LANG_SLOVENIAN)
		return L"LANG_SLOVENIAN";
	else if(wLang == LANG_GERMAN)
		return L"LANG_GERMAN";
	else if(wLang == LANG_SPANISH)
		return L"LANG_SPANISH";
	else if(wLang == LANG_GREEK)
		return L"LANG_GREEK";
	else if(wLang == LANG_SWAHILI)
		return L"LANG_SWAHILI";
	else if(wLang == LANG_GUJARATI)
		return L"LANG_GUJARATI";
	else if(wLang == LANG_SWEDISH)
		return L"LANG_SWEDISH";
	else if(wLang == LANG_HEBREW)
		return L"LANG_HEBREW";
	else if(wLang == LANG_TAMIL)
		return L"LANG_TAMIL";
	else if(wLang == LANG_HINDI)
		return L"LANG_HINDI";
	else if(wLang == LANG_TATAR)
		return L"LANG_TATAR";
	else if(wLang == LANG_HUNGARIAN)
		return L"LANG_HUNGARIAN";
	else if(wLang == LANG_TELUGU)
		return L"LANG_TELUGU";
	else if(wLang == LANG_ICELANDIC)
		return L"LANG_ICELANDIC";
	else if(wLang == LANG_THAI)
		return L"LANG_THAI";
	else if(wLang == LANG_INDONESIAN)
		return L"LANG_INDONESIAN";
	else if(wLang == LANG_TURKISH)
		return L"LANG_TURKISH";
	else if(wLang == LANG_ITALIAN)
		return L"LANG_ITALIAN";
	else if(wLang == LANG_UKRAINIAN)
		return L"LANG_UKRAINIAN";
	else if(wLang == LANG_JAPANESE)
		return L"LANG_JAPANESE";
	else if(wLang == LANG_URDU)
		return L"LANG_URDU";
	else if(wLang == LANG_KANNADA)
		return L"LANG_KANNADA";
	else if(wLang == LANG_UZBEK)
		return L"LANG_UZBEK";
	else if(wLang == LANG_KASHMIRI)
		return L"LANG_KASHMIRI";
	else if(wLang == LANG_VIETNAMESE)
		return L"LANG_VIETNAMESE";
	else if(wLang == LANG_KAZAK)
		return L"LANG_KAZAK";
	return L"LANG_NEUTRAL"; 
}
//--------------------------------------------------------------------------//
//
wchar_t * StringTable::getSubLang(WORD wLang)
{
	WORD primary = PRIMARYLANGID(wLang);

	wLang = SUBLANGID(wLang);
	if(primary == LANG_ARABIC)
	{
		if(wLang == SUBLANG_ARABIC_SAUDI_ARABIA)
			return L"SUBLANG_ARABIC_SAUDI_ARABIA";
		else if(wLang == SUBLANG_ARABIC_IRAQ)
			return L"SUBLANG_ARABIC_IRAQ";
		else if(wLang == SUBLANG_ARABIC_EGYPT)
			return L"SUBLANG_ARABIC_EGYPT";
		else if(wLang == SUBLANG_ARABIC_LIBYA)
			return L"SUBLANG_ARABIC_LIBYA";
		else if(wLang == SUBLANG_ARABIC_ALGERIA)
			return L"SUBLANG_ARABIC_ALGERIA";
		else if(wLang == SUBLANG_ARABIC_MOROCCO)
			return L"SUBLANG_ARABIC_MOROCCO";
		else if(wLang == SUBLANG_ARABIC_TUNISIA)
			return L"SUBLANG_ARABIC_TUNISIA";
		else if(wLang == SUBLANG_ARABIC_OMAN)
			return L"SUBLANG_ARABIC_OMAN";
		else if(wLang == SUBLANG_ARABIC_YEMEN)
			return L"SUBLANG_ARABIC_YEMEN";
		else if(wLang == SUBLANG_ARABIC_SYRIA)
			return L"SUBLANG_ARABIC_SYRIA";
		else if(wLang == SUBLANG_ARABIC_JORDAN)
			return L"SUBLANG_ARABIC_JORDAN";
		else if(wLang == SUBLANG_ARABIC_LEBANON)
			return L"SUBLANG_ARABIC_LEBANON";
		else if(wLang == SUBLANG_ARABIC_KUWAIT)
			return L"SUBLANG_ARABIC_KUWAIT";
		else if(wLang == SUBLANG_ARABIC_UAE)
			return L"SUBLANG_ARABIC_UAE";
		else if(wLang == SUBLANG_ARABIC_BAHRAIN)
			return L"SUBLANG_ARABIC_BAHRAIN";
		else if(wLang == SUBLANG_ARABIC_QATAR)
			return L"SUBLANG_ARABIC_QATAR";
	}
	else if(primary == LANG_GERMAN)
	{
		if(wLang == SUBLANG_GERMAN_AUSTRIAN)
			return L"SUBLANG_GERMAN_AUSTRIAN";
		else if(wLang == SUBLANG_GERMAN_LUXEMBOURG)
			return L"SUBLANG_GERMAN_LUXEMBOURG";
		else if(wLang == SUBLANG_GERMAN_LIECHTENSTEIN)
			return L"SUBLANG_GERMAN_LIECHTENSTEIN";
		else if(wLang == SUBLANG_GERMAN)
			return L"SUBLANG_GERMAN";
		else if(wLang == SUBLANG_GERMAN_SWISS)
			return L"SUBLANG_GERMAN_SWISS";
	}
	else if(primary == LANG_ITALIAN)
	{
		if(wLang == SUBLANG_ITALIAN)
			return L"SUBLANG_ITALIAN";
		else if(wLang == SUBLANG_ITALIAN_SWISS)
			return L"SUBLANG_ITALIAN_SWISS";
	}
	else if(primary == LANG_KASHMIRI)
	{
		if(wLang == SUBLANG_KASHMIRI_INDIA)
			return L"SUBLANG_KASHMIRI_INDIA";
	}
	else if(primary == LANG_KOREAN)
	{
		if(wLang == SUBLANG_KOREAN)
			return L"SUBLANG_KOREAN";
	}
	else if(primary == LANG_LITHUANIAN)
	{
		if(wLang == SUBLANG_LITHUANIAN)
			return L"SUBLANG_LITHUANIAN";
//	#if !defined(_XBOX)
//		else if(wLang == SUBLANG_LITHUANIAN_CLASSIC)
//			return L"SUBLANG_LITHUANIAN_CLASSIC";
//	#endif
	}
	else if(primary == LANG_MALAY)
	{
		if(wLang == SUBLANG_MALAY_BRUNEI_DARUSSALAM)
			return L"SUBLANG_MALAY_BRUNEI_DARUSSALAM";
		else if(wLang == SUBLANG_MALAY_MALAYSIA)
			return L"SUBLANG_MALAY_MALAYSIA";
	}
	else if(primary == LANG_NEPALI)
	{
		if(wLang == SUBLANG_NEPALI_INDIA)
			return L"SUBLANG_NEPALI_INDIA";
	}
	else if(primary == LANG_NORWEGIAN)
	{
		if(wLang == SUBLANG_NORWEGIAN_BOKMAL)
			return L"SUBLANG_NORWEGIAN_BOKMAL";
		else if(wLang == SUBLANG_NORWEGIAN_NYNORSK)
			return L"SUBLANG_NORWEGIAN_NYNORSK";
	}
	else if(primary == LANG_PORTUGUESE)
	{
		if(wLang == SUBLANG_PORTUGUESE)
			return L"SUBLANG_PORTUGUESE";
		else if(wLang == SUBLANG_PORTUGUESE_BRAZILIAN)
			return L"SUBLANG_PORTUGUESE_BRAZILIAN";
	}
	else if(primary == LANG_AZERI)
	{
		if(wLang == SUBLANG_AZERI_CYRILLIC)
			return L"SUBLANG_AZERI_CYRILLIC";
		else if(wLang == SUBLANG_AZERI_LATIN)
			return L"SUBLANG_AZERI_LATIN";
	}
	else if(primary == LANG_SERBIAN)
	{
		if(wLang == SUBLANG_SERBIAN_LATIN)
			return L"SUBLANG_SERBIAN_LATIN";
		else if(wLang == SUBLANG_SERBIAN_CYRILLIC)
			return L"SUBLANG_SERBIAN_CYRILLIC";
	}
	else if(primary == LANG_CHINESE)
	{
		if(wLang == SUBLANG_CHINESE_TRADITIONAL)
			return L"SUBLANG_CHINESE_TRADITIONAL";
		else if(wLang == SUBLANG_CHINESE_SIMPLIFIED)
			return L"SUBLANG_CHINESE_SIMPLIFIED";
		else if(wLang == SUBLANG_CHINESE_HONGKONG)
			return L"SUBLANG_CHINESE_HONGKONG";
		else if(wLang == SUBLANG_CHINESE_SINGAPORE)
			return L"SUBLANG_CHINESE_SINGAPORE";
		else if(wLang == SUBLANG_CHINESE_MACAU)
			return L"SUBLANG_CHINESE_MACAU";
	}
	else if(primary == LANG_SPANISH)
	{
		if(wLang == SUBLANG_SPANISH)
			return L"SUBLANG_SPANISH";
		else if(wLang == SUBLANG_SPANISH_MEXICAN)
			return L"SUBLANG_SPANISH_MEXICAN";
		else if(wLang == SUBLANG_SPANISH_MODERN)
			return L"SUBLANG_SPANISH_MODERN";
		else if(wLang == SUBLANG_SPANISH_GUATEMALA)
			return L"SUBLANG_SPANISH_GUATEMALA";
		else if(wLang == SUBLANG_SPANISH_COSTA_RICA)
			return L"SUBLANG_SPANISH_COSTA_RICA";
		else if(wLang == SUBLANG_SPANISH_DOMINICAN_REPUBLIC)
			return L"SUBLANG_SPANISH_DOMINICAN_REPUBLIC";
		else if(wLang == SUBLANG_SPANISH_VENEZUELA)
			return L"SUBLANG_SPANISH_VENEZUELA";
		else if(wLang == SUBLANG_SPANISH_COLOMBIA)
			return L"SUBLANG_SPANISH_COLOMBIA";
		else if(wLang == SUBLANG_SPANISH_PERU)
			return L"SUBLANG_SPANISH_PERU";
		else if(wLang == SUBLANG_SPANISH_ARGENTINA)
			return L"SUBLANG_SPANISH_ARGENTINA";
		else if(wLang == SUBLANG_SPANISH_ECUADOR)
			return L"SUBLANG_SPANISH_ECUADOR";
		else if(wLang == SUBLANG_SPANISH_CHILE)
			return L"SUBLANG_SPANISH_CHILE";
		else if(wLang == SUBLANG_SPANISH_URUGUAY)
			return L"SUBLANG_SPANISH_URUGUAY";
		else if(wLang == SUBLANG_SPANISH_PARAGUAY)
			return L"SUBLANG_SPANISH_PARAGUAY";
		else if(wLang == SUBLANG_SPANISH_BOLIVIA)
			return L"SUBLANG_SPANISH_BOLIVIA";
		else if(wLang == SUBLANG_SPANISH_EL_SALVADOR)
			return L"SUBLANG_SPANISH_EL_SALVADOR";
		else if(wLang == SUBLANG_SPANISH_HONDURAS)
			return L"SUBLANG_SPANISH_HONDURAS";
		else if(wLang == SUBLANG_SPANISH_NICARAGUA)
			return L"SUBLANG_SPANISH_NICARAGUA";
		else if(wLang == SUBLANG_SPANISH_PUERTO_RICO)
			return L"SUBLANG_SPANISH_PUERTO_RICO";
	}
	else if(primary == LANG_DUTCH)
	{
		if(wLang == SUBLANG_DUTCH)
			return L"SUBLANG_DUTCH";
		else if(wLang == SUBLANG_DUTCH_BELGIAN)
			return L"SUBLANG_DUTCH_BELGIAN";
	}
	else if(primary == LANG_ENGLISH)
	{
		if(wLang == SUBLANG_ENGLISH_US)
			return L"SUBLANG_ENGLISH_US";
		else if(wLang == SUBLANG_ENGLISH_UK)
			return L"SUBLANG_ENGLISH_UK";
		else if(wLang == SUBLANG_ENGLISH_AUS)
			return L"SUBLANG_ENGLISH_AUS";
		else if(wLang == SUBLANG_ENGLISH_CAN)
			return L"SUBLANG_ENGLISH_CAN";
		else if(wLang == SUBLANG_ENGLISH_NZ)
			return L"SUBLANG_ENGLISH_NZ";
		else if(wLang == SUBLANG_ENGLISH_EIRE)
			return L"SUBLANG_ENGLISH_EIRE";
		else if(wLang == SUBLANG_ENGLISH_SOUTH_AFRICA)
			return L"SUBLANG_ENGLISH_SOUTH_AFRICA";
		else if(wLang == SUBLANG_ENGLISH_JAMAICA)
			return L"SUBLANG_ENGLISH_JAMAICA";
		else if(wLang == SUBLANG_ENGLISH_CARIBBEAN)
			return L"SUBLANG_ENGLISH_CARIBBEAN";
		else if(wLang == SUBLANG_ENGLISH_BELIZE)
			return L"SUBLANG_ENGLISH_BELIZE";
		else if(wLang == SUBLANG_ENGLISH_TRINIDAD)
			return L"SUBLANG_ENGLISH_TRINIDAD";
		else if(wLang == SUBLANG_ENGLISH_ZIMBABWE)
			return L"SUBLANG_ENGLISH_ZIMBABWE";
		else if(wLang == SUBLANG_ENGLISH_PHILIPPINES)
			return L"SUBLANG_ENGLISH_PHILIPPINES";
	}
	else if(primary == LANG_FRENCH)
	{
		if(wLang == SUBLANG_FRENCH)
			return L"SUBLANG_FRENCH";
		else if(wLang == SUBLANG_FRENCH_BELGIAN)
			return L"SUBLANG_FRENCH_BELGIAN";
		else if(wLang == SUBLANG_FRENCH_CANADIAN)
			return L"SUBLANG_FRENCH_CANADIAN";
		else if(wLang == SUBLANG_FRENCH_SWISS)
			return L"SUBLANG_FRENCH_SWISS";
		else if(wLang == SUBLANG_FRENCH_LUXEMBOURG)
			return L"SUBLANG_FRENCH_LUXEMBOURG";
		else if(wLang == SUBLANG_FRENCH_MONACO)
			return L"SUBLANG_FRENCH_MONACO";
	}
	else if(primary == LANG_SWEDISH)
	{
		if(wLang == SUBLANG_SWEDISH)
			return L"SUBLANG_SWEDISH";
		else if(wLang == SUBLANG_SWEDISH_FINLAND)
			return L"SUBLANG_SWEDISH_FINLAND";
	}
	else if(primary == LANG_URDU)
	{
		if(wLang == SUBLANG_URDU_INDIA)
			return L"SUBLANG_URDU_INDIA";
		else if(wLang == SUBLANG_URDU_PAKISTAN)
			return L"SUBLANG_URDU_PAKISTAN";
	}
	else if(primary == LANG_UZBEK)
	{
		if(wLang == SUBLANG_UZBEK_CYRILLIC)
			return L"SUBLANG_UZBEK_CYRILLIC";
		else if(wLang == SUBLANG_UZBEK_LATIN)
			return L"SUBLANG_UZBEK_LATIN";
	}
	else if(wLang == SUBLANG_NEUTRAL)
		return L"SUBLANG_NEUTRAL";
	else if(wLang == SUBLANG_DEFAULT)
		return L"SUBLANG_DEFAULT";
	else if(wLang == SUBLANG_SYS_DEFAULT)
		return L"SUBLANG_SYS_DEFAULT";
	return L"SUBLANG_NEUTRAL"; 
}
//--------------------------------------------------------------------------//
//
DWORD StringTable::getPrimaryLangID(wchar_t * lang)
{
	if(wcscmp(lang,L"LANG_AFRIKAANS") == 0)
		return LANG_AFRIKAANS;
	else if(wcscmp(lang,L"LANG_KONKANI") == 0)
		return LANG_KONKANI;
	else if(wcscmp(lang,L"LANG_ALBANIAN") == 0)
		return LANG_ALBANIAN;
	else if(wcscmp(lang,L"LANG_KOREAN") == 0)
		return LANG_KOREAN;
	else if(wcscmp(lang,L"LANG_ARABIC") == 0)
		return LANG_ARABIC;
	else if(wcscmp(lang,L"LANG_LATVIAN") == 0)
		return LANG_LATVIAN;
	else if(wcscmp(lang,L"LANG_ARMENIAN") == 0)
		return LANG_ARMENIAN;
	else if(wcscmp(lang,L"LANG_LITHUANIAN") == 0)
		return LANG_LITHUANIAN;
	else if(wcscmp(lang,L"LANG_ASSAMESE") == 0)
		return LANG_ASSAMESE;
	else if(wcscmp(lang,L"LANG_MACEDONIAN") == 0)
		return LANG_MACEDONIAN;
	else if(wcscmp(lang,L"LANG_AZERI") == 0)
		return LANG_AZERI;
	else if(wcscmp(lang,L"LANG_MALAY") == 0)
		return LANG_MALAY;
	else if(wcscmp(lang,L"LANG_BASQUE") == 0)
		return LANG_BASQUE;
	else if(wcscmp(lang,L"LANG_MALAYALAM") == 0)
		return LANG_MALAYALAM;
	else if(wcscmp(lang,L"LANG_BELARUSIAN") == 0)
		return LANG_BELARUSIAN;
	else if(wcscmp(lang,L"LANG_MANIPURI") == 0)
		return LANG_MANIPURI;
	else if(wcscmp(lang,L"LANG_BENGALI") == 0)
		return LANG_BENGALI;
	else if(wcscmp(lang,L"LANG_MARATHI") == 0)
		return LANG_MARATHI;
	else if(wcscmp(lang,L"LANG_BULGARIAN") == 0)
		return LANG_BULGARIAN;
	else if(wcscmp(lang,L"LANG_NEPALI") == 0)
		return LANG_NEPALI;
	else if(wcscmp(lang,L"LANG_CATALAN") == 0)
		return LANG_CATALAN;
	else if(wcscmp(lang,L"LANG_NEUTRAL") == 0)
		return LANG_NEUTRAL;
	else if(wcscmp(lang,L"LANG_CHINESE") == 0)
		return LANG_CHINESE;
	else if(wcscmp(lang,L"LANG_NORWEGIAN") == 0)
		return LANG_NORWEGIAN;
	else if(wcscmp(lang,L"LANG_CROATIAN") == 0)
		return LANG_CROATIAN;
	else if(wcscmp(lang,L"LANG_ORIYA") == 0)
		return LANG_ORIYA;
	else if(wcscmp(lang,L"LANG_CZECH") == 0)
		return LANG_CZECH;
	else if(wcscmp(lang,L"LANG_POLISH") == 0)
		return LANG_POLISH;
	else if(wcscmp(lang,L"LANG_DANISH") == 0)
		return LANG_DANISH;
	else if(wcscmp(lang,L"LANG_PORTUGUESE") == 0)
		return LANG_PORTUGUESE;
	else if(wcscmp(lang,L"LANG_DUTCH") == 0)
		return LANG_DUTCH;
	else if(wcscmp(lang,L"LANG_PUNJABI") == 0)
		return LANG_PUNJABI;
	else if(wcscmp(lang,L"LANG_ENGLISH") == 0)
		return LANG_ENGLISH;
	else if(wcscmp(lang,L"LANG_ROMANIAN") == 0)
		return LANG_ROMANIAN;
	else if(wcscmp(lang,L"LANG_ESTONIAN") == 0)
		return LANG_ESTONIAN;
	else if(wcscmp(lang,L"LANG_RUSSIAN") == 0)
		return LANG_RUSSIAN;
	else if(wcscmp(lang,L"LANG_FAEROESE") == 0)
		return LANG_FAEROESE;
	else if(wcscmp(lang,L"LANG_SANSKRIT") == 0)
		return LANG_SANSKRIT;
	else if(wcscmp(lang,L"LANG_FARSI") == 0)
		return LANG_FARSI;
	else if(wcscmp(lang,L"LANG_SERBIAN") == 0)
		return LANG_SERBIAN;
	else if(wcscmp(lang,L"LANG_FINNISH") == 0)
		return LANG_FINNISH;
	else if(wcscmp(lang,L"LANG_SINDHI") == 0)
		return LANG_SINDHI;
	else if(wcscmp(lang,L"LANG_FRENCH") == 0)
		return LANG_FRENCH;
	else if(wcscmp(lang,L"LANG_SLOVAK") == 0)
		return LANG_SLOVAK;
	else if(wcscmp(lang,L"LANG_GEORGIAN") == 0)
		return LANG_GEORGIAN;
	else if(wcscmp(lang,L"LANG_SLOVENIAN") == 0)
		return LANG_SLOVENIAN;
	else if(wcscmp(lang,L"LANG_GERMAN") == 0)
		return LANG_GERMAN;
	else if(wcscmp(lang,L"LANG_SPANISH") == 0)
		return LANG_SPANISH;
	else if(wcscmp(lang,L"LANG_GREEK") == 0)
		return LANG_GREEK;
	else if(wcscmp(lang,L"LANG_SWAHILI") == 0)
		return LANG_SWAHILI;
	else if(wcscmp(lang,L"LANG_GUJARATI") == 0)
		return LANG_GUJARATI;
	else if(wcscmp(lang,L"LANG_SWEDISH") == 0)
		return LANG_SWEDISH;
	else if(wcscmp(lang,L"LANG_HEBREW") == 0)
		return LANG_HEBREW;
	else if(wcscmp(lang,L"LANG_TAMIL") == 0)
		return LANG_TAMIL;
	else if(wcscmp(lang,L"LANG_HINDI") == 0)
		return LANG_HINDI;
	else if(wcscmp(lang,L"LANG_TATAR") == 0)
		return LANG_TATAR;
	else if(wcscmp(lang,L"LANG_HUNGARIAN") == 0)
		return LANG_HUNGARIAN;
	else if(wcscmp(lang,L"LANG_TELUGU") == 0)
		return LANG_TELUGU;
	else if(wcscmp(lang,L"LANG_ICELANDIC") == 0)
		return LANG_ICELANDIC;
	else if(wcscmp(lang,L"LANG_THAI") == 0)
		return LANG_THAI;
	else if(wcscmp(lang,L"LANG_INDONESIAN") == 0)
		return LANG_INDONESIAN;
	else if(wcscmp(lang,L"LANG_TURKISH") == 0)
		return LANG_TURKISH;
	else if(wcscmp(lang,L"LANG_ITALIAN") == 0)
		return LANG_ITALIAN;
	else if(wcscmp(lang,L"LANG_UKRAINIAN") == 0)
		return LANG_UKRAINIAN;
	else if(wcscmp(lang,L"LANG_JAPANESE") == 0)
		return LANG_JAPANESE;
	else if(wcscmp(lang,L"LANG_URDU") == 0)
		return LANG_URDU;
	else if(wcscmp(lang,L"LANG_KANNADA") == 0)
		return LANG_KANNADA;
	else if(wcscmp(lang,L"LANG_UZBEK") == 0)
		return LANG_UZBEK;
	else if(wcscmp(lang,L"LANG_KASHMIRI") == 0)
		return LANG_KASHMIRI;
	else if(wcscmp(lang,L"LANG_VIETNAMESE") == 0)
		return LANG_VIETNAMESE;
	else if(wcscmp(lang,L"LANG_KAZAK") == 0)
		return LANG_KAZAK;
	return LANG_NEUTRAL; 
}
//--------------------------------------------------------------------------//
//
DWORD StringTable::getSubLangID(wchar_t * lang)
{
	if(wcscmp(lang,L"SUBLANG_ARABIC_SAUDI_ARABIA") == 0)
		return SUBLANG_ARABIC_SAUDI_ARABIA;
	else if(wcscmp(lang,L"SUBLANG_GERMAN_AUSTRIAN") == 0)
		return SUBLANG_GERMAN_AUSTRIAN;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_IRAQ") == 0)
		return SUBLANG_ARABIC_IRAQ;
	else if(wcscmp(lang,L"SUBLANG_GERMAN_LUXEMBOURG") == 0)
		return SUBLANG_GERMAN_LUXEMBOURG;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_EGYPT") == 0)
		return SUBLANG_ARABIC_EGYPT;
	else if(wcscmp(lang,L"SUBLANG_GERMAN_LIECHTENSTEIN") == 0)
		return SUBLANG_GERMAN_LIECHTENSTEIN;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_LIBYA") == 0)
		return SUBLANG_ARABIC_LIBYA;
	else if(wcscmp(lang,L"SUBLANG_ITALIAN") == 0)
		return SUBLANG_ITALIAN;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_ALGERIA") == 0)
		return SUBLANG_ARABIC_ALGERIA;
	else if(wcscmp(lang,L"SUBLANG_ITALIAN_SWISS") == 0)
		return SUBLANG_ITALIAN_SWISS;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_MOROCCO") == 0)
		return SUBLANG_ARABIC_MOROCCO;
	else if(wcscmp(lang,L"SUBLANG_KASHMIRI_INDIA") == 0)
		return SUBLANG_KASHMIRI_INDIA;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_TUNISIA") == 0)
		return SUBLANG_ARABIC_TUNISIA;
	else if(wcscmp(lang,L"SUBLANG_KOREAN") == 0)
		return SUBLANG_KOREAN;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_OMAN") == 0)
		return SUBLANG_ARABIC_OMAN;
	else if(wcscmp(lang,L"SUBLANG_LITHUANIAN") == 0)
		return SUBLANG_LITHUANIAN;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_YEMEN") == 0)
		return SUBLANG_ARABIC_YEMEN;
//#if !defined(_XBOX)
//	else if(wcscmp(lang,L"SUBLANG_LITHUANIAN_CLASSIC") == 0)
//		return SUBLANG_LITHUANIAN_CLASSIC;
//#endif
	else if(wcscmp(lang,L"SUBLANG_ARABIC_SYRIA") == 0)
		return SUBLANG_ARABIC_SYRIA;
	else if(wcscmp(lang,L"SUBLANG_MALAY_BRUNEI_DARUSSALAM") == 0)
		return SUBLANG_MALAY_BRUNEI_DARUSSALAM;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_JORDAN") == 0)
		return SUBLANG_ARABIC_JORDAN;
	else if(wcscmp(lang,L"SUBLANG_MALAY_MALAYSIA") == 0)
		return SUBLANG_MALAY_MALAYSIA;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_LEBANON") == 0)
		return SUBLANG_ARABIC_LEBANON;
	else if(wcscmp(lang,L"SUBLANG_NEPALI_INDIA") == 0)
		return SUBLANG_NEPALI_INDIA;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_KUWAIT") == 0)
		return SUBLANG_ARABIC_KUWAIT;
	else if(wcscmp(lang,L"SUBLANG_NEUTRAL") == 0)
		return SUBLANG_NEUTRAL;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_UAE") == 0)
		return SUBLANG_ARABIC_UAE;
	else if(wcscmp(lang,L"SUBLANG_NORWEGIAN_BOKMAL") == 0)
		return SUBLANG_NORWEGIAN_BOKMAL;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_BAHRAIN") == 0)
		return SUBLANG_ARABIC_BAHRAIN;
	else if(wcscmp(lang,L"SUBLANG_NORWEGIAN_NYNORSK") == 0)
		return SUBLANG_NORWEGIAN_NYNORSK;
	else if(wcscmp(lang,L"SUBLANG_ARABIC_QATAR") == 0)
		return SUBLANG_ARABIC_QATAR;
	else if(wcscmp(lang,L"SUBLANG_PORTUGUESE") == 0)
		return SUBLANG_PORTUGUESE;
	else if(wcscmp(lang,L"SUBLANG_AZERI_CYRILLIC") == 0)
		return SUBLANG_AZERI_CYRILLIC;
	else if(wcscmp(lang,L"SUBLANG_PORTUGUESE_BRAZILIAN") == 0)
		return SUBLANG_PORTUGUESE_BRAZILIAN;
	else if(wcscmp(lang,L"SUBLANG_AZERI_LATIN") == 0)
		return SUBLANG_AZERI_LATIN;
	else if(wcscmp(lang,L"SUBLANG_SERBIAN_LATIN") == 0)
		return SUBLANG_SERBIAN_LATIN;
	else if(wcscmp(lang,L"SUBLANG_CHINESE_TRADITIONAL") == 0)
		return SUBLANG_CHINESE_TRADITIONAL;
	else if(wcscmp(lang,L"SUBLANG_SERBIAN_CYRILLIC") == 0)
		return SUBLANG_SERBIAN_CYRILLIC;
	else if(wcscmp(lang,L"SUBLANG_CHINESE_SIMPLIFIED") == 0)
		return SUBLANG_CHINESE_SIMPLIFIED;
	else if(wcscmp(lang,L"SUBLANG_SPANISH") == 0)
		return SUBLANG_SPANISH;
	else if(wcscmp(lang,L"SUBLANG_CHINESE_HONGKONG") == 0)
		return SUBLANG_CHINESE_HONGKONG;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_MEXICAN") == 0)
		return SUBLANG_SPANISH_MEXICAN;
	else if(wcscmp(lang,L"SUBLANG_CHINESE_SINGAPORE") == 0)
		return SUBLANG_CHINESE_SINGAPORE;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_MODERN") == 0)
		return SUBLANG_SPANISH_MODERN;
	else if(wcscmp(lang,L"SUBLANG_CHINESE_MACAU") == 0)
		return SUBLANG_CHINESE_MACAU;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_GUATEMALA") == 0)
		return SUBLANG_SPANISH_GUATEMALA;
	else if(wcscmp(lang,L"SUBLANG_DEFAULT") == 0)
		return SUBLANG_DEFAULT;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_COSTA_RICA") == 0)
		return SUBLANG_SPANISH_COSTA_RICA;
	else if(wcscmp(lang,L"SUBLANG_DUTCH") == 0)
		return SUBLANG_DUTCH;
	else if(wcscmp(lang,L"SUBLANG_DUTCH_BELGIAN") == 0)
		return SUBLANG_DUTCH_BELGIAN;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_DOMINICAN_REPUBLIC") == 0)
		return SUBLANG_SPANISH_DOMINICAN_REPUBLIC;
	else if(wcscmp(lang,L"SUBLANG_ENGLISH_US") == 0)
		return SUBLANG_ENGLISH_US;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_VENEZUELA") == 0)
		return SUBLANG_SPANISH_VENEZUELA;
	else if(wcscmp(lang,L"SUBLANG_ENGLISH_UK") == 0)
		return SUBLANG_ENGLISH_UK;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_COLOMBIA") == 0)
		return SUBLANG_SPANISH_COLOMBIA;
	else if(wcscmp(lang,L"SUBLANG_ENGLISH_AUS") == 0)
		return SUBLANG_ENGLISH_AUS;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_PERU") == 0)
		return SUBLANG_SPANISH_PERU;
	else if(wcscmp(lang,L"SUBLANG_ENGLISH_CAN") == 0)
		return SUBLANG_ENGLISH_CAN;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_ARGENTINA") == 0)
		return SUBLANG_SPANISH_ARGENTINA;
	else if(wcscmp(lang,L"SUBLANG_ENGLISH_NZ") == 0)
		return SUBLANG_ENGLISH_NZ;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_ECUADOR") == 0)
		return SUBLANG_SPANISH_ECUADOR;
	else if(wcscmp(lang,L"SUBLANG_ENGLISH_EIRE") == 0)
		return SUBLANG_ENGLISH_EIRE;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_CHILE") == 0)
		return SUBLANG_SPANISH_CHILE;
	else if(wcscmp(lang,L"SUBLANG_ENGLISH_SOUTH_AFRICA") == 0)
		return SUBLANG_ENGLISH_SOUTH_AFRICA;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_URUGUAY") == 0)
		return SUBLANG_SPANISH_URUGUAY;
	else if(wcscmp(lang,L"SUBLANG_ENGLISH_JAMAICA") == 0)
		return SUBLANG_ENGLISH_JAMAICA;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_PARAGUAY") == 0)
		return SUBLANG_SPANISH_PARAGUAY;
	else if(wcscmp(lang,L"SUBLANG_ENGLISH_CARIBBEAN") == 0)
		return SUBLANG_ENGLISH_CARIBBEAN;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_BOLIVIA") == 0)
		return SUBLANG_SPANISH_BOLIVIA;
	else if(wcscmp(lang,L"SUBLANG_ENGLISH_BELIZE") == 0)
		return SUBLANG_ENGLISH_BELIZE;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_EL_SALVADOR") == 0)
		return SUBLANG_SPANISH_EL_SALVADOR;
	else if(wcscmp(lang,L"SUBLANG_ENGLISH_TRINIDAD") == 0)
		return SUBLANG_ENGLISH_TRINIDAD;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_HONDURAS") == 0)
		return SUBLANG_SPANISH_HONDURAS;
	else if(wcscmp(lang,L"SUBLANG_ENGLISH_PHILIPPINES") == 0)
		return SUBLANG_ENGLISH_PHILIPPINES;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_NICARAGUA") == 0)
		return SUBLANG_SPANISH_NICARAGUA;
	else if(wcscmp(lang,L"SUBLANG_ENGLISH_ZIMBABWE") == 0)
		return SUBLANG_ENGLISH_ZIMBABWE;
	else if(wcscmp(lang,L"SUBLANG_SPANISH_PUERTO_RICO") == 0)
		return SUBLANG_SPANISH_PUERTO_RICO;
	else if(wcscmp(lang,L"SUBLANG_FRENCH") == 0)
		return SUBLANG_FRENCH;
	else if(wcscmp(lang,L"SUBLANG_SWEDISH") == 0)
		return SUBLANG_SWEDISH;
	else if(wcscmp(lang,L"SUBLANG_FRENCH_BELGIAN") == 0)
		return SUBLANG_FRENCH_BELGIAN;
	else if(wcscmp(lang,L"SUBLANG_SWEDISH_FINLAND") == 0)
		return SUBLANG_SWEDISH_FINLAND;
	else if(wcscmp(lang,L"SUBLANG_FRENCH_CANADIAN") == 0)
		return SUBLANG_FRENCH_CANADIAN;
	else if(wcscmp(lang,L"SUBLANG_SYS_DEFAULT") == 0)
		return SUBLANG_SYS_DEFAULT;
	else if(wcscmp(lang,L"SUBLANG_FRENCH_SWISS") == 0)
		return SUBLANG_FRENCH_SWISS;
	else if(wcscmp(lang,L"SUBLANG_URDU_INDIA") == 0)
		return SUBLANG_URDU_INDIA;
	else if(wcscmp(lang,L"SUBLANG_FRENCH_LUXEMBOURG") == 0)
		return SUBLANG_FRENCH_LUXEMBOURG;
	else if(wcscmp(lang,L"SUBLANG_URDU_PAKISTAN") == 0)
		return SUBLANG_URDU_PAKISTAN;
	else if(wcscmp(lang,L"SUBLANG_FRENCH_MONACO") == 0)
		return SUBLANG_FRENCH_MONACO;
	else if(wcscmp(lang,L"SUBLANG_UZBEK_CYRILLIC") == 0)
		return SUBLANG_UZBEK_CYRILLIC;
	else if(wcscmp(lang,L"SUBLANG_GERMAN") == 0)
		return SUBLANG_GERMAN;
	else if(wcscmp(lang,L"SUBLANG_UZBEK_LATIN") == 0)
		return SUBLANG_UZBEK_LATIN;
	else if(wcscmp(lang,L"SUBLANG_GERMAN_SWISS") == 0)
		return SUBLANG_GERMAN_SWISS;
	return SUBLANG_NEUTRAL; 
}
//--------------------------------------------------------------------------//
//
WORD StringTable::getLanguage(wchar_t * lang)
{
	wchar_t string2[256];
	wchar_t string3[256];

	wchar_t * str = wcschr(lang,L'L');
	while(!iswspace(str[0]))//LANGUAGE   
		++str;
	while(iswspace(str[0]))
		++str;
	U32 len = wcschr(str,L'\t') - str;
	wcsncpy(string2,str,len);
	string2[len] = 0;
	str = wcschr(str,L'\t');
	str++;
	while(iswspace(str[0]))
		++str;
	swscanf(str,L"%s",string3);
	return MAKELANGID(getPrimaryLangID(string2),getSubLangID(string3));
}

//-----------------------------------------------------------------------------------------------------
// DllMain

//--------------------------------------------------------------------------
//  
BOOL COMAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it with DACOM manager
	//
		case DLL_PROCESS_ATTACH:
		{
			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE( hinstDLL );

			ICOManager * DACOM = DACOM_Acquire();
			IComponentFactory * server;

			// Register System aggragate factory
			if( DACOM && (server = new DAComponentFactory2<DAComponentAggregate<StringTable>, AGGDESC>(CLSID_StringTable)) != NULL ) 
			{
				DACOM->RegisterComponent( server, CLSID_StringTable, DACOM_NORMAL_PRIORITY );
				server->Release();
			}
			
			break;
		}

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------

/*

void test_string_code()
{
	STRINGTABLE->NewString( L"hello", "hello", 1033 );
	STRINGTABLE->NewString( L"world", "world", 1033 );
	STRINGTABLE->NewString( L"cat",   NULL, 1033 );
	STRINGTABLE->NewString( L"dog",  "word_dog", 1033 );

	STRINGTABLE->NewString( L"hello", "hello", 1035 );
	STRINGTABLE->NewString( L"world", "world", 1035 );
	STRINGTABLE->NewString( L"cat",   NULL, 1035 );
	STRINGTABLE->NewString( L"dog",  "word_dog", 1035 );

	STRINGTABLE->SetLanguage( 1033 );

	STRINGTABLE->GetStringByTag("hello");

	// --- start xml test ---

	COMPTR<ISaverLoader> saver;
	STRINGTABLE->QueryInterface("ISaverLoader",saver);
	if( saver )
	{
		TiXmlDocument doc( "test.lang.xml" );
		doc.InsertEndChild( TiXmlDeclaration("1.0","","yes") );
		saver->Save(doc);
		doc.SaveFile();
	}

	STRINGTABLE->Reset();

	TiXmlDocument doc( "test.lang.xml" );
	if( doc.LoadFile() )
	{
		saver->Load(doc);
	}

	// --- start utf test ---

	((StringTable*)STRINGTABLE)->DEBUG_print();

	COMPTR<IFileSystem> f;
	DAFILEDESC fdesc = "test.lang.utf";
	fdesc.lpImplementation = "UTF";
	fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	if( DACOM->CreateInstance(&fdesc,f) == GR_OK )
	{
		IFileSystem* fs = f;
		saver->Save( *fs );
		f->CloseHandle(0);
	}

	STRINGTABLE->Reset();

	fdesc.dwCreationDistribution = OPEN_EXISTING;
	if( DACOM->CreateInstance(&fdesc,f) == GR_OK )
	{
		IFileSystem* fs = f;
		saver->Load( *fs );
		f->CloseHandle(0);
	}

	((StringTable*)STRINGTABLE)->DEBUG_print();
}

*/