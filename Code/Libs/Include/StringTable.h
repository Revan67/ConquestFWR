//
// StringTable.h
//

#ifndef STRING_TABLE_HEADER_H
#define STRING_TABLE_HEADER_H

#include "dacom.h"

struct IStringEnum
{
	struct StringInfo
	{
		const wchar_t* wideString;
		const char*    tagString;
		unsigned long  idString;
	};

	virtual void EnumStringInfo( IStringEnum::StringInfo& _stringInfo ) = 0;
};

struct DACOM_NO_VTABLE IStringTable : IDAComponent
{
	virtual const wchar_t* GetStringByID( U32 _stringID ) = 0;

	virtual const wchar_t* GetStringByTag( const char* _tag ) = 0;

	virtual const char* GetStringTag( U32 _stringID ) = 0;

	virtual U32 GetStringIdByTag( const char* _tag ) = 0;

	virtual bool SetLanguage( LANGID _langid ) = 0;

	virtual LANGID GetLanguage( void ) = 0;

	virtual U32 NewString( wchar_t* _stringData, const char* _stringTag = NULL, LANGID _langID = 1033 ) = 0;

	virtual bool SetString( U32 _uid, wchar_t* _stringData, const char* _stringTag, LANGID _langID ) = 0;

	virtual void Reset( void ) = 0;

	virtual void EnumerateStringInfo( IStringEnum& _enumerator ) = 0;
};

#define IID_IStringTable MAKE_IID("StringTable",1)

#endif

