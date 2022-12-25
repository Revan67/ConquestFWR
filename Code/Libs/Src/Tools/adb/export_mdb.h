//--------------------------------------------------------------------------//
//                                                                          //
//                               export_mdb.h    							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/export_mdb.h 4     3/28/03 10:24a Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------//

#ifndef __SQL_EXPORT_MDB_H
#define __SQL_EXPORT_MDB_H

//-------------------------------------------------------------------------------------
// STL stuff

#pragma warning(disable : 4786)
#include <string>
#include <list>

//-------------------------------------------------------------------------------------

#include <sql.h>

struct SqlDb
{
	SQLHANDLE handleEnv;
	SQLHANDLE handleDbc;
	SQLHANDLE handleStm;

	bool bInited;
	bool bNeedFetch;
	
	enum{ MAXBUFLEN = 1024, };

	SQLCHAR     szOutConn[MAXBUFLEN];
	SQLSMALLINT cbConnStrOut;
	SQLSMALLINT cbNumColumns;


	SqlDb();

	void ProcessLogMessages(SQLSMALLINT plm_handle_type, SQLHANDLE plm_handle, char *logstring, int ConnInd);

	void PrintError( SQLRETURN _ret, SQLSMALLINT _handle_type, SQLHANDLE _handle );

	bool Connect( const char* filename );

	bool Initalize();

	bool Execute_Select( const char* _table );

	const char* FetchRowField( int _index, bool& _more );

	bool Execute( const char* _sql );

	bool Disconnect();
};

struct CreateTableQuery
{
	enum Type
	{
		CHAR_VAR,
		CHAR_ARRAY,
		CHAR_SINGLE,
		FLOAT,
		INTEGER,
	};

	std::string foreignKeys;
	std::string sql;
	bool first;

	void Start( const char* _tableName );
	void Add( const char* _label, CreateTableQuery::Type _type, int _parm );
	void AddForeignKey( const char* _label, const char* _key );
	void End( void );
};

struct InsertDataRow
{
	struct StringList : std::list<std::string>{};

	std::string sql;
	bool first;

	StringList values;
	StringList labels;

	void Start( const char* _tableName );
	void AddInt( const char* _label, unsigned int _value );
	void AddFloat( const char* _label, double _value );
	void AddString( const char* _label, const char* _value );
	void End( void );

protected:

	void FindSafeLabel( const char* _label, std::string& _outLabel );
};

#endif