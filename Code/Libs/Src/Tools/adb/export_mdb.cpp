//--------------------------------------------------------------------------//
//                                                                          //
//                               export_mdb.cpp    							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/export_mdb.cpp 5     3/28/03 10:24a Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------//
#include "pch.h"
#include "export_mdb.h"

#include <assert.h>
#include <sql.h>
#include <sqlext.h>
#include <odbcss.h>

#define printf _localprintf

//-------------------------------------------------------------------------------------

#ifdef _DEBUG
#ifndef DEBUG_NEW
	void* __cdecl operator new(size_t nSize, LPCSTR lpszFileName, int nLine);
	#define DEBUG_NEW new(THIS_FILE, __LINE__)
	#if _MSC_VER >= 1200
		void __cdecl operator delete(void* p, LPCSTR lpszFileName, int nLine);
	#endif
#endif
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define PRINT_MDB_ERRORS

//-------------------------------------------------------------------------------------
// CreateTableQuery
//-------------------------------------------------------------------------------------

void CreateTableQuery::Start( const char* _tableName )
{
	first = true;
	sql  = "CREATE TABLE ";
	sql += _tableName;
	sql += "(";

	foreignKeys = "";
}

void CreateTableQuery::End( void )
{
	// add the foreign keys to the end of the statement
	if( foreignKeys.size() )
	{
		sql += foreignKeys;
	}
	sql += ")";
}

void CreateTableQuery::Add( const char* _label, CreateTableQuery::Type _type, int _parm )
{
	if( !first )
	{
		sql += ",";
	}

	sql += _label;

	switch( _type )
	{
		case CHAR_ARRAY:
//		{
//			char buf[16];
//			
//			sql += " char(";
//			sql += itoa( _parm, buf, 10 );
//			sql += ")";
//
//			break;
//		}
		case CHAR_VAR:
		{
			sql += " varchar";
			break;
		}
		case CHAR_SINGLE:
		{
			sql += " int";
			break;
		}
		case FLOAT:
		{
			sql += " float";
			break;
		}
		case INTEGER:
		{
			sql += " int";
			break;
		}
	}
	if( first )
	{
		first = false;
		sql += " PRIMARY KEY";
	}
}

void CreateTableQuery::AddForeignKey( const char* _label, const char* _key )
{
	assert( !first );

// testing to see if only one foriegn key will work...
if( foreignKeys.size() ) return;

	foreignKeys += ",";

	// FOREIGN KEY(scale_id) REFERENCES colscales(id));
	// FOREIGN KEY( _label ) REFERENCES _key(enumid));

	foreignKeys += std::string(" FOREIGN KEY(") + std::string(_label) + std::string(")") +
				   std::string(" REFERENCES ")  + std::string(_key)   + "(enumid)";

	// FOREIGN KEY( _label ) REFERENCES _key );

//	foreignKeys += std::string(" FOREIGN KEY(") + std::string(_label) + std::string(")") +
//				   std::string(" REFERENCES ")  + std::string(_key)   + " ";
}

//-------------------------------------------------------------------------------------
// InsertDataRow
//-------------------------------------------------------------------------------------

// in dbsybols.cpp
LPSTR far FloatToString(LPSTR buff,double r,int prec);

void InsertDataRow::Start( const char* _tableName )
{
	first = true;
	sql  = "INSERT INTO ";
	sql += _tableName;
	sql += "(";

	if( labels.size() )
		labels.clear();

	if( values.size() )
		values.clear();
}

void InsertDataRow::End( void )
{
	std::string vString = "VALUES (";

	for( StringList::iterator vit = values.begin(); vit != values.end(); vit++ )
	{
		if( vit != values.begin() )
		{
			vString += ",";
		}

		vString += *vit;
	}
	vString += ")";

	for( StringList::iterator lit = labels.begin(); lit != labels.end(); lit++ )
	{
		if( lit != labels.begin() )
		{
			sql += ",";
		}

		sql += *lit;
	}
	sql += ")";

	sql += vString;
}

void InsertDataRow::AddInt( const char* _label, unsigned int _value )
{
	std::string outLabel;
	FindSafeLabel( _label, outLabel );

	char buf[64];
	std::string v = ultoa( _value, buf, 10 );

	values.push_back( v );
	labels.push_back( outLabel );
}

void InsertDataRow::AddFloat( const char* _label, double _value )
{
	std::string outLabel;
	FindSafeLabel( _label, outLabel );

	char v[64];
	FloatToString( v, _value, 10 );

	values.push_back( v );
	labels.push_back( outLabel );
}

void InsertDataRow::AddString( const char* _label, const char* _value )
{
	std::string outLabel;
	FindSafeLabel( _label, outLabel );

	std::string v = std::string("\'") + std::string(_value) + std::string("\'");
	values.push_back( v );
	labels.push_back( outLabel );
}

void InsertDataRow::FindSafeLabel( const char* _label, std::string& _outLabel )
{
	// assume nothing is wrong...
	_outLabel = _label;

	const char* keywords[] = {
	"level", "add","except","percent","all","exec","plan","alter","execute","precision","and","exists",
	"print","as","fetch","proc","asc","file","procedure","authorization","fillfactor","public","backup",
	"for","raiserror","begin","foreign","read","between","freetext","any","exit","primary","by",
	"readtext","break","freetexttable","reconfigure","browse","from","references","bulk","full","replication",
	"function","restore","cascade","goto","restrict","case","grant","return","check","group","revoke","checkpoint",
	"having","right","close","holdlock","rollback","clustered","identity","rowcount","coalesce","identity_insert",
	"constraint","inner","session_user","contains","insert","set","containstable","intersect","setuser","continue",
	"into","shutdown","convert","is","some","create","join","statistics","cross","key","system_user","current","kill",
	"table","current_date","left","textsize","current_time","like","then","current_timestamp","lineno","to",
	"current_user","load","top","cursor","national","tran","database","nocheck","transaction","dbcc","nonclustered",
	"trigger","deallocate","not","truncate","declare","null","tsequal","default","nullif","union","delete","of",
	"unique","deny","off","update","desc","offsets","updatetext","disk","on","use","distinct","open","user",
	"distributed","opendatasource","values","double","openquery","varying","drop","openrowset","view","dummy",
	"openxml","waitfor","dump","option","when","else","or","where","end","order","while","errlvl","outer","with",
	"escape","over","writetext", 0 };

	const char* pKey = keywords[0];

	for( int i = 0; pKey; i++ )
	{
		pKey = keywords[i];

		if( !pKey ) break;

		if( stricmp(pKey,_label)==0 )
		{
			_outLabel = std::string("\"") + std::string(_label) + std::string("\"");
			return;
		}	
	}
}

//-------------------------------------------------------------------------------------
// SqlDb
//-------------------------------------------------------------------------------------

SqlDb::SqlDb()
{
	handleEnv = 0;
	handleDbc = 0;
	handleStm = 0;
	
	szOutConn[0] = 0;
	cbConnStrOut = 0;

	bInited = false;
	bNeedFetch = false;
}

//-------------------------------------------------------------------------------------

void SqlDb::ProcessLogMessages(SQLSMALLINT plm_handle_type, SQLHANDLE plm_handle, char *logstring, int ConnInd)
{
#ifdef PRINT_MDB_ERRORS

	const int MaxBufferSize = 1024*8;

	UCHAR* plm_szSqlState = new UCHAR[MaxBufferSize];
	UCHAR* plm_szErrorMsg = new UCHAR[MaxBufferSize];

	RETCODE     plm_retcode = SQL_SUCCESS;
	SDWORD      plm_pfNativeError = 0L;
	SWORD       plm_pcbErrorMsg = 0;
	SQLSMALLINT plm_cRecNmbr = 1;
	SDWORD      plm_SS_MsgState = 0, plm_SS_Severity = 0;
	SQLINTEGER  plm_Rownumber = 0;

	printf("\n%s\n",logstring);

	while (plm_retcode != SQL_NO_DATA_FOUND) 
	{
		plm_retcode = SQLGetDiagRec(plm_handle_type, plm_handle,
			plm_cRecNmbr, plm_szSqlState, &plm_pfNativeError,
			plm_szErrorMsg, MaxBufferSize - 1, &plm_pcbErrorMsg);

		// Note that if the application has not yet made a
		// successful connection, the SQLGetDiagField
		// information has not yet been cached by ODBC
		// Driver Manager and these calls to SQLGetDiagField
		// will fail.
		if (plm_retcode != SQL_NO_DATA_FOUND) 
		{
			printf("szSqlState = %s\n",plm_szSqlState);
			printf("pfNativeError = %d\n",plm_pfNativeError);
			printf("szErrorMsg = %s\n",plm_szErrorMsg);
			printf("pcbErrorMsg = %d\n\n",plm_pcbErrorMsg);
		}
		plm_cRecNmbr++; //Increment to next diagnostic record.
	} // End while.

	delete [] plm_szSqlState;
	delete [] plm_szErrorMsg;

#endif
}

//-------------------------------------------------------------------------------------

void SqlDb::PrintError( SQLRETURN _ret, SQLSMALLINT _handle_type, SQLHANDLE _handle )
{
	#define PRINT_ERROR(x) if( x == _ret ) { ProcessLogMessages( _handle_type, _handle, #x, 1 ); }


	PRINT_ERROR( SQL_SUCCESS );
	PRINT_ERROR( SQL_SUCCESS_WITH_INFO );
	PRINT_ERROR( SQL_NEED_DATA );
	PRINT_ERROR( SQL_STILL_EXECUTING );
	PRINT_ERROR( SQL_ERROR );
	PRINT_ERROR( SQL_NO_DATA );
	PRINT_ERROR( SQL_INVALID_HANDLE );
}

//-------------------------------------------------------------------------------------

bool SqlDb::Connect( const char* filename )
{
	assert( !bInited );

	SQLRETURN ret = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &handleEnv );
	assert( SQL_SUCCEEDED(ret) );

	ret = SQLSetEnvAttr(handleEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3,0);
	assert( SQL_SUCCEEDED(ret) );

	ret = SQLAllocHandle(SQL_HANDLE_DBC, handleEnv, &handleDbc);
	assert( SQL_SUCCEEDED(ret) );

	std::string szConnection = "Driver={Microsoft Access Driver (*.mdb)}; DBQ=" + std::string(filename);
	SQLCHAR* connString = (SQLCHAR*)szConnection.c_str();

	ret = SQLDriverConnect( handleDbc, NULL, connString, SQL_NTS, szOutConn, MAXBUFLEN, &cbConnStrOut, SQL_DRIVER_NOPROMPT);

	// auto Initalize
	if( SQL_SUCCEEDED(ret) )
	{
		Initalize();
	}

	// need to set more connection stuff here

	return SQL_SUCCEEDED(ret);
}

//-------------------------------------------------------------------------------------

bool SqlDb::Initalize()
{
	if( !bInited )
	{
		bInited = true;

		SQLUINTEGER fFuncs;
		SQLRETURN ret = SQLGetInfo(handleDbc, SQL_STRING_FUNCTIONS, (SQLPOINTER)&fFuncs, sizeof(fFuncs), NULL);

		ret = SQLAllocHandle(SQL_HANDLE_STMT, handleDbc, &handleStm);
		assert( SQL_SUCCEEDED(ret) );

		return SQL_SUCCEEDED(ret);
	}
	return true;
}

//-------------------------------------------------------------------------------------

bool SqlDb::Execute_Select(const char* _table)
{
	std::string selectStatement = std::string("SELECT * FROM ") + std::string(_table);

	SQLRETURN  ret;

	ret = SQLSetStmtAttr(handleStm, SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_FORWARD_ONLY, 0 ); 

	ret = SQLExecDirect(handleStm, (SQLCHAR*)selectStatement.c_str(), SQL_NTS);

	if( SQL_SUCCEEDED(ret) )
	{
		ret = SQLNumResultCols( handleStm,	&cbNumColumns);
		bNeedFetch = true;
	}
	else
	{
		PrintError( ret, SQL_HANDLE_STMT, handleStm );
	}

	return false;
}

//-------------------------------------------------------------------------------------

const char* SqlDb::FetchRowField( int _index, bool& _more )
{
	SQLRETURN ret;
	_more = true;

	if( bNeedFetch )
	{
		bNeedFetch = false;
		ret = SQLFetch(handleStm);
		if( !SQL_SUCCEEDED(ret) )
		{
			SQLCloseCursor(handleStm);
			_more = false;
			return NULL;
		}
	}

	if( _index > cbNumColumns )
	{
		bNeedFetch = true;
		return NULL;
	}

	SQLINTEGER cbLabel;
	ret = SQLGetData(handleStm, _index, SQL_C_CHAR, szOutConn, MAXBUFLEN, &cbLabel);
	if( !SQL_SUCCEEDED(ret) )
	{
		PrintError( ret, SQL_HANDLE_STMT, handleStm );
	}
	return (const char*)szOutConn;
}

//-------------------------------------------------------------------------------------

bool SqlDb::Execute( const char* _sql )
{
	SQLINTEGER sqlQueryLen = strlen(_sql);
	SQLTCHAR*  sqlQuery = (SQLTCHAR*)_sql;

	SQLRETURN ret = SQLExecDirect( handleStm, sqlQuery, sqlQueryLen );

	if( ! SQL_SUCCEEDED(ret) )
	{
		char sqlline[1024];
		_snprintf( sqlline, 1024, "%s\n", _sql );
		OutputDebugString( sqlline );

		PrintError( ret, SQL_HANDLE_STMT, handleStm );
	}

	return SQL_SUCCEEDED(ret);
}

//-------------------------------------------------------------------------------------

bool SqlDb::Disconnect()
{
	SQLRETURN ret;

	ret = SQLFreeHandle( SQL_HANDLE_STMT, handleStm );
	if( !SQL_SUCCEEDED(ret) )
		PrintError( ret, SQL_HANDLE_STMT, handleStm );

	ret = SQLDisconnect( handleDbc );
	if( !SQL_SUCCEEDED(ret) )
		PrintError( ret, SQL_HANDLE_DBC, handleDbc );

	ret = SQLFreeHandle( SQL_HANDLE_DBC, handleDbc );
	if( !SQL_SUCCEEDED(ret) )
		PrintError( ret, SQL_HANDLE_DBC, handleDbc );

	ret = SQLFreeHandle( SQL_HANDLE_ENV, handleEnv );
	if( !SQL_SUCCEEDED(ret) )
		PrintError( ret, SQL_HANDLE_ENV, handleEnv );

	bInited = false;

	return 1;
}

//-------------------------------------------------------------------------------------
// Test code
//-------------------------------------------------------------------------------------

int test_main(int argc, char* argv[])
{
	SqlDb sqldb;

	sqldb.Connect( "C:\\projects\\test.mdb" );

	sqldb.Initalize();

	sqldb.Execute( "DROP TABLE Person" );

	//" CREATE TABLE Person ( Name char(64), Sanity float, Address varchar, Age int ) "
	CreateTableQuery ctq;
	ctq.Start( "Person" );
		ctq.Add( "Name",    CreateTableQuery::CHAR_ARRAY, 64 );
		ctq.Add( "Sanity",  CreateTableQuery::FLOAT, 8 );
		ctq.Add( "Address", CreateTableQuery::CHAR_VAR, 0 );
		ctq.Add( "Age",     CreateTableQuery::INTEGER, 0 );
	ctq.End();
	sqldb.Execute( ctq.sql.c_str() );

	//" INSERT INTO Persons (LastName, Address) VALUES ('Rasmussen', 'Storgt 67') "
	InsertDataRow idr;
	idr.Start("Person");
		idr.AddString( "Name",    "Rasmussen" );
		idr.AddFloat ( "Sanity",   1.01 );
		idr.AddString( "Address", "Storgt 67" );
		idr.AddInt   ( "Age",      101 );
	idr.End();
	sqldb.Execute( idr.sql.c_str() );

	sqldb.Disconnect();

	return 0;
}
