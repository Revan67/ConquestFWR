//--------------------------------------------------------------------------//
//                                                                          //
//                               dbsymbols.cpp   							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/dbsymbols.cpp 7     3/31/03 1:26p Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------//

#include "pch.h"

#include "dbsymbols.h"

#include "ViewCnst.H"
#include <Document.h>
#include <malloc.h>
#include "TSmartPointer.h"
#include <assert.h>

#define MODIFIER_HAS_BITS (0x01000000)

//-------------------------------------------------------------------------------------
#pragma warning(disable : 4786)
#include <map>
#include <string>
#include <list>
using namespace std;


#include "..\..\Viewer\Docuview\SymTable.h"
#include "RawData.h"

//-------------------------------------------------------------------------------------

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-------------------------------------------------------------------------------------

// Symbol Helper functions
namespace SH
{
	bool GetBitCounts( SYMBOL s, BYTE& bitsOffset, BYTE& bitsCount )
	{
		bitsCount  = s->bit_count;
		bitsOffset = s->bit_offset;

		if( bitsCount )
			return true;

		SYMBOL enm = s->datatype;
		while( enm )
		{
			if( enm->bit_count ) 
			{
				bitsCount  = enm->bit_count;
				bitsOffset = enm->bit_offset;
				return true;
			}
			enm = enm->datatype;
		}
		return false;
	}

	bool IsEnum( SYMBOL s, BYTE& _bitCount )
	{
		_bitCount = s->bit_count;

		SYMBOL enm = s->datatype;
		while( enm )
		{
			if( !_bitCount )
			{
				_bitCount = enm->bit_count;
			}

			if( enm->kind == ENUMSYM )
			{
				return true;
			}
			enm = enm->datatype;
		}
		return false;
	}

	bool IsInteger( SYMBOL s, BYTE& _signed )
	{
		_signed = false;

		SYMBOL dtint = s->datatype;
		while( dtint )
		{
			if( dtint->kind == TYPESYM && !_signed )
			{
				_signed = dtint->modifiers & MODIFIER_UNSIGNED;
			}
			if( dtint->kind == BASICTYPE && dtint->name )
			{
				if( !strcmp(dtint->name,"long")  || 
					!strcmp(dtint->name,"int")   || 
					!strcmp(dtint->name,"short") || 
					!strcmp(dtint->name,"char") )
				{
					return true;
				}
			}
			dtint = dtint->datatype;
		}
		return false;
	}

	bool IsFloat( SYMBOL s, BYTE& _signed )
	{
		_signed = false;

		SYMBOL dtflt = s->datatype;
		while( dtflt )
		{
			if( dtflt->kind == TYPESYM && !_signed )
			{
				_signed = dtflt->modifiers & MODIFIER_UNSIGNED;
			}
			if( dtflt->kind == BASICTYPE && dtflt->name )
			{
				if( !strcmp(dtflt->name,"float") || !strcmp(dtflt->name,"double") )
				{
					return true;
				}
			}
			dtflt = dtflt->datatype;
		}
		return false;
	}

	bool IsString( SYMBOL s, BYTE& _numArray )
	{
		// get to its more basic type
		SYMBOL vs = s;
		while( vs->kind == VARSYM )
		{
			vs = vs->datatype;
		}
		assert(vs);
		s = vs;

		if( s->kind == ARRAYSYM            && 
			s->datatype->kind == BASICTYPE && 
			s->datatype->name              &&
			!strcmp(s->datatype->name,"char") )
		{
			_numArray = s->arrayval; 
			return true;
		}
		return false;
	}

	bool IsRecord( SYMBOL s )
	{
		if( !s ) return false;
		if( s->kind == RECORDSYM ) return true;
		return SH::IsRecord( s->datatype );
	}

	SYMBOL GetRecordContaining( SYMBOL s, SYMBOL check )
	{
		if( !s ) return NULL;

		if( s->kind == RECORDSYM )
		{
			SYMBOL internalSymbol = s->datatype;
			while( internalSymbol )
			{
				if( internalSymbol == check )
				{
					return s;
				}
				internalSymbol = internalSymbol->link;
			}
		}
		else if( s->kind == ARRAYSYM )
		{
			if( s->datatype->kind == RECORDSYM )
			{
				SYMBOL arraySymbol = s->datatype->datatype;
				while( arraySymbol )
				{
					if( arraySymbol == check )
					{
						return s->datatype;
					}
					arraySymbol = arraySymbol->link;
				}
			}
		}

		return SH::GetRecordContaining( s->datatype, check );
	}

	SYMBOL GetBaseRecordSize( SYMBOL s, int& size, struct IViewConstructor2* pParser )
	{
		if( !s ) return NULL;

		if( s->kind == RECORDSYM )
		{
			size += pParser->GetTypeSize(s);

			SYMBOL internalSymbol = s->datatype;
			while( internalSymbol )
			{
				if( internalSymbol->kind == RECORDSYM )
				{
					SH::GetBaseRecordSize( internalSymbol, size, pParser );
				}
				internalSymbol = internalSymbol->link;
			}
		}

		return SH::GetBaseRecordSize( s->datatype, size, pParser );
	}
};

//-------------------------------------------------------------------------------------

namespace SymbolsExportNamespace
{
	struct NameList  : list< string >{};
	struct HeaderMap : map < SYMBOL, string >{};
	struct BitsList  : list< DWORD >{};
	struct SymbolMap : std::map < std::string, SYMBOL >{};

	SymbolMap g_SymbolMap;
	NameList  g_NameList;
	string    g_Row;
	NameList  g_RecordNameList;
	string    g_CurrentHeader;
	BitsList  g_BitsList;
	string    g_Seperator = ",";
	RawData   g_RawData;

	void BuildBitsList( SYMBOL s, DWORD value )
	{
		int bits = 32;

		SYMBOL dt = s;
		while( dt )
		{
			// test for a sibling not a bit field
			if( !dt->bit_count )
			{
				return;
			}

			// test for over the 32 bit limit
			bits -= dt->bit_count;
			if( bits < 0 )
			{
				return;
			}

			// push around bits to get the contained value for this bit window
			DWORD cnt = dt->bit_count - 1;
			DWORD val = value >> dt->bit_offset;
			val <<= (31 - cnt);
			val >>= (31 - cnt);

			g_BitsList.push_back( val );

			dt = dt->link;
		}
	}
}
using namespace SymbolsExportNamespace;

//--------------------------------------------------------------------------//

LPSTR far FloatToString(LPSTR buff,double r,int prec) 
{ 
	char *buffer;
	LPSTR startbuffptr;
	static int dec,sign;  // must use static, don't assume SS==DS
	int i;

	startbuffptr = buff;

	buffer = fcvt(r,prec,&dec,&sign);

	// copy the negative sign if less than zero
	if (sign)
		*buff++ = '-';

	if (dec<=0)
		*buff++ = '0';

	if (dec < -prec)
	{
		strcpy( startbuffptr, "0.0000");
		return startbuffptr;
	}

	// copy the non-fractional part before the decimal place
	for(i=0; i<dec; i++)
		*buff++ = *buffer++;

	*buff++ = '.';  // copy the decimal point into the string
	*buff = '\0';   // don't assume NULL termination

	// copy the fractional part after the decimal place
	lstrcat(buff,buffer);

	return startbuffptr;
} 

//--------------------------------------------------------------------------//

const char* IntegerToString( DWORD _integer, bool _signed )
{
	static char szNumber[128];

	if( _signed )
	{
		return itoa( static_cast<int>( _integer ), szNumber, 10 );
	}

	return ultoa( _integer, szNumber, 10 );
}

//--------------------------------------------------------------------------//

void WriteNumber( BYTE* _pData, int _size, bool _unsigned )
{
	char sNumber[16];

	if( _size == 4 || _size == 2 )
	{
		long signedLong = 0;

		if( _size == 4 )
		{
			long* longs = (long*)_pData;
			signedLong = *longs;
		}
		if( _size == 2 )
		{
			short* shorts = (short*)_pData;
			signedLong = static_cast<long>( *shorts );
		}

		if( _unsigned )
		{
			DWORD unsignedLong = static_cast<DWORD>( signedLong );
			g_Row += ultoa( unsignedLong, sNumber, 10 );
		}
		else
		{
			g_Row += itoa( signedLong, sNumber, 10 );
		}
	}
	else if( _size == 1 )
	{
		if( _unsigned )
		{
			itoa( _pData[0], sNumber, 10 );
			g_Row += sNumber;
		}
		else
		{
			char str[2] = {_pData[0], NULL};
			g_Row += str;
		}
	}
	else
		__asm int 3; // eek! not a number
}

//--------------------------------------------------------------------------//

BOOL32 __stdcall DbSymbols::StructEnumerator(struct IViewConstructor2 * cnst, const C8 *typeName, SYMBOL symbol, void *context)
{
	g_SymbolMap[ typeName ] = symbol;

	// if an ENUM, print out table here
	// if a Namespace ... ?

	if( symbol->kind == BASICTYPE )
	{
		_localprintf("SymbolBasic : %s, %d\n", symbol->name, cnst->GetTypeSize(symbol) );
	}

	return 1;
}

//--------------------------------------------------------------------------//

DbSymbols::DbSymbols()
{
}

//--------------------------------------------------------------------------//

bool DbSymbols::Init( struct IViewConstructor2 * PARSER )
{
	g_SymbolMap.clear();
	pParser = PARSER;
	pParser->EnumerateTypes( StructEnumerator, this );
	return true;
}

//--------------------------------------------------------------------------//

DbSymbols::~DbSymbols()
{
}

//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
// table exporting
//--------------------------------------------------------------------------//

void DbSymbols::ExportSymbolTable( SYMBOL s )
{
	if( s->kind == RECORDSYM )
	{
		g_NameList.push_back( s->name );
		PrintLevels();
		fprintf( file, "\n" );

		g_NameList.push_back( "{\n" );
		PrintLevels();
		level++;

		SYMBOL walker = s->datatype;
		while( walker )
		{
			ExportSymbolTable( walker );
			walker = walker->link;
		}

		level--;
		g_NameList.push_back( "}\n" );
		PrintLevels();
	}
	else
	{
		bool bComplexArray = false;

		// push this record name
		if( SH::IsRecord(s) && s->kind == VARSYM && s->name )
		{
			g_RecordNameList.push_front( s->name );
		}

		// look for arrays of Records
		if( s->kind == ARRAYSYM && s->datatype->kind == RECORDSYM )
		{
			bComplexArray = true;
			const int arrayNum = s->arrayval;
			int num = arrayNum;
			SYMBOL rec = s->datatype;

			while( num )
			{
				SYMBOL walker = rec->datatype;
				while( walker )
				{
					g_CurrentHeader += walker->name + string("_") + IntegerToString(arrayNum - num);
					g_CurrentHeader += g_Seperator;
					walker = walker->link;
				}
				num--;
			}
		}

		if( s->datatype )
		{
			if( s->kind == VARSYM && s->name && !bComplexArray )
			{
				if( !SH::IsRecord(s) )
				{
					// pre-pend the name of the record to the basic varible
					if( g_RecordNameList.size() )
					{
						NameList::reverse_iterator rnit = g_RecordNameList.rbegin();
						for( ; rnit != g_RecordNameList.rend(); rnit++ )
						{
							string& recname  = *rnit;
							g_CurrentHeader += recname.c_str() + string("_");
						}
					}

					g_CurrentHeader += s->name;
					g_CurrentHeader += g_Seperator;
				}
			}

			string name;
			if( s->arrayval )
			{
				isArray = true;
				arraySize = s->arrayval;
			}
			if( s->kind == ENUMSYM )
			{
				name   =  "e_";
				isEnum = true;
			}
			if( s->name )
			{
				name += s->name;
				g_NameList.push_back( name );
			}
			ExportSymbolTable( s->datatype );
		}
		else
		{
			if( isEnum == false )
			{
				g_NameList.push_back( s->name );
			}
			PrintLevels();
			fprintf( file, "\n" );
		}

		// pop the last record name
		if( SH::IsRecord(s) && s->kind == VARSYM && s->name )
		{
			g_RecordNameList.pop_front();
		}
	}
}

//--------------------------------------------------------------------------//

void DbSymbols::PrintLevels()
{
	for( unsigned int i = 0; i < level; ++i )
	{
		fprintf( file, "\t" );
	}

	string type = g_NameList.back();
	string name = g_NameList.front();

	if( isArray )
	{
		type += "[";
		type += IntegerToString( arraySize );
		type += "]";
	}

	if( g_NameList.size() > 1 )
		fprintf( file, "%s %s", type.c_str(), name.c_str() );
	else
		fprintf( file, "%s", type.c_str() );

	g_NameList.clear();
	isEnum = false;
	isArray = false;
}

//--------------------------------------------------------------------------//

bool DbSymbols::ExportTableHeader( const char* _symbolName )
{
	string fn = string(_symbolName) + string(".txt");

	file = fopen( fn.c_str(), "wt" );
	if( !file ) return false;

	SymbolMap::iterator it = g_SymbolMap.find( _symbolName );
	if( it == g_SymbolMap.end() )
	{
		fclose( file );
		return false;
	}

	g_CurrentHeader = string(_symbolName) + g_Seperator;
	recordType = it->second;
	level = 0;

	ExportSymbolTable( recordType );

	fflush( file );
	fclose( file );

//	::DeleteFile( fn.c_str() );

	// export the header for this name

	string filename = string(_symbolName) + string("_xls.csv");
	file = fopen( filename.c_str(), "wt" );
	if( file )
	{
		g_CurrentHeader += "\n";
		fwrite( g_CurrentHeader.c_str(), 1, g_CurrentHeader.size(), file );
		fflush( file );
		fclose( file );
	}
	return true;
}

//--------------------------------------------------------------------------//
// row exporting
//--------------------------------------------------------------------------//

void DbSymbols::ExportSymbolRow( SYMBOL s, DWORD sModifiers )
{
	if( !s ) return;

	// build a list of the upcomming "bits" data
	if( s->bit_count && g_BitsList.empty() )
	{
		BuildBitsList( s, g_RawData.GetLong(sizeof(DWORD)) );
	}

	if( s->kind == RECORDSYM )
	{
		const char* name = s->name;
		int size = pParser->GetTypeSize(s);

		g_RawData.StartRecord( name, size );

		SYMBOL internalSymbol = s->datatype;
		while( internalSymbol )
		{
			ExportSymbolRow( internalSymbol, internalSymbol->modifiers );
			internalSymbol = internalSymbol->link;
		}

		g_RawData.EndRecord();
	}
	else if( s->kind == TYPESYM )
	{
		const char* name = s->name;
		ExportSymbolRow( s->datatype, s->modifiers );
	}
	else if( s->kind == ENUMSYM )
	{
		const char* name = s->name;
		ExportSymbolRow( s->datatype, s->modifiers );
	}
	else if( s->kind == VARSYM )
	{
		bool isRecord = (s->datatype->kind == RECORDSYM);

		const char* name = s->name;
		ExportSymbolRow( s->datatype, s->modifiers );

		if( !isRecord )
			g_Row += g_Seperator;
	}
	else if( s->kind == ARRAYSYM )
	{
		const char* name = s->name;
		int size = s->arrayval;
		int esiz = pParser->GetTypeSize(s->datatype);

		// special casing for a NULL string
		if( esiz == 1 && !strcmp(s->datatype->name,"char") )
		{
			if( g_RawData.IsNullString(size) )
			{
				g_Row += "(null string)";
				g_RawData.GetString(size);
				return;
			}
		}
		while( size )
		{
			ExportSymbolRow( s->datatype, s->modifiers );
			size--;
		}
	}
	else if( s->kind == BASICTYPE )
	{
		const char* name = s->name;
		int size = pParser->GetTypeSize(s);

		if( !strcmp(name,"double") )
		{
			assert(g_BitsList.empty());

			char buff[64];

			double dbl = g_RawData.GetDouble( size );
			FloatToString( buff, dbl, 5 );

			g_Row += string(buff);
		}
		else if( !strcmp(name,"float") )
		{
			assert(g_BitsList.empty());

			char buff[64];

			float flt = g_RawData.GetFloat( size );
			FloatToString( buff, flt, 5 );

			g_Row += string(buff);
		}
		else if( !strcmp(name,"__int64") )
		{
			assert(g_BitsList.empty());
			__asm int 3; // is this ever used?
		}
		// note, could be unsigned
		else if( !strcmp(name,"int") || !strcmp(name,"long") || !strcmp(name,"short") || !strcmp(name,"char") )
		{
			if( !g_BitsList.empty() )
			{
				DWORD nNumber = g_BitsList.front();
				g_BitsList.pop_front();
				WriteNumber( (BYTE*)&nNumber, size, sModifiers & MODIFIER_UNSIGNED );
			}
			else
			{
				DWORD nNumber = 0;
				switch( size )
				{
					case 1: nNumber = g_RawData.GetByte(1);  break;
					case 2: nNumber = g_RawData.GetShort(2); break;
					case 4: nNumber = g_RawData.GetLong(4);  break;
				}
				WriteNumber( (BYTE*)&nNumber, size, sModifiers & MODIFIER_UNSIGNED );
			}
		}
		else if( !strcmp(name,"void") )
		{
			assert(g_BitsList.empty());
			__asm int 3; // exporting a void!?!
		}
	}
	else if( s->kind == CONSTSYM )
	{
		const char* name = s->name;
		int size = pParser->GetTypeSize(s);

		// an assumption "all const numbers will be unsigned"
		DWORD nNumber = 0;
		if( g_BitsList.empty() )
		{
			switch( size )
			{
				case 1: nNumber = g_RawData.GetByte(1);  break;
				case 2: nNumber = g_RawData.GetShort(2); break;
				case 4: nNumber = g_RawData.GetLong(4);  break;
			}
		}
		else
		{
			nNumber = g_BitsList.front();
			g_BitsList.pop_front();
		}

		g_Row += IntegerToString( nNumber );
	}
	else
	{
		__asm int 3; // need to include this type to be exported
		const char* name = s->name;
		ExportSymbolRow( s->datatype );
	}
}

//--------------------------------------------------------------------------//

bool DbSymbols::ExportRow( const char* _symbolName, struct IFileSystem* _dataFile )
{
	if( g_Row.size() )
	{
		g_Row.erase();
	}

	DAFILEDESC fdesc = _symbolName;
	COMPTR<IFileSystem> f;
	if( _dataFile->CreateInstance(&fdesc,f) != GR_OK )
	{
		return false;
	}

	DWORD size = pParser->GetTypeSize( recordType );
	DWORD dwWritten = 0;

	void* structure = ::malloc( size );

	if( f->ReadFile(0, structure, size, &dwWritten ) )
	{
		g_Row = _symbolName;
		g_Row += g_Seperator;

		g_RawData.SetData( structure, size );
		ExportSymbolRow( recordType );
	}

	::free( structure );

	g_Row += "\n";

	string filename = string(recordType->name) + string("_xls.csv");
	if( file )
	{
		FILE* f = fopen(filename.c_str(), "at" );
		fwrite( g_Row.c_str(), 1, g_Row.size(), f );
		fflush( f );
		fclose( f );
	}

	return true;
}

//--------------------------------------------------------------------------//
// Symbol look up
//--------------------------------------------------------------------------//

SYMBOL DbSymbols::FindSymbolIndex( DWORD& idx, SYMBOL s )
{
	if( !s ) return NULL;

	if( s->kind == RECORDSYM )
	{
		SYMBOL internalSymbol = s->datatype;
		while( internalSymbol )
		{
			return FindSymbolIndex(idx,internalSymbol);
			internalSymbol = internalSymbol->link;
		}
	}
	else if( s->kind == ARRAYSYM )
	{
		if( s->datatype->kind == RECORDSYM )
		{
			for( int i = 0; i < s->arrayval; i++ )
			{
				SYMBOL arraySymbol = s->datatype->datatype;
				while( arraySymbol )
				{
					if( idx == 0 )
					{
						return arraySymbol;
					}
					arraySymbol = arraySymbol->link;
					idx--;
				}
			}
		}
		else
		{
			if( idx == 0 )
			{
				return s;
			}
			idx--;
			return NULL;
		}
	}
	else if( s->kind == VARSYM )
	{
		if( s->datatype && s->datatype->kind == RECORDSYM )
		{
			SYMBOL s1 = FindSymbolIndex(idx,s->datatype);
			if( s1 ) return s1;
			idx++;
		}
		else if( s->datatype && s->datatype->kind == ARRAYSYM )
		{
			SYMBOL s1 = FindSymbolIndex(idx,s->datatype);
			if( s1 ) return s1;
			idx++;
		}

		if( idx == 0 )
		{
			return s;
		}
		idx--;
		return FindSymbolIndex(idx,s->link);
	}
	else if( s->datatype )
	{
		return FindSymbolIndex(idx,s->datatype);
	}

	return NULL;
}

//--------------------------------------------------------------------------//

SYMBOL DbSymbols::GetSymbolByIndex( const char* _baseSymbolName, DWORD _idx )
{
	SymbolMap::iterator it = g_SymbolMap.find( _baseSymbolName );
	if( it != g_SymbolMap.end() )
	{
		SYMBOL s = it->second;
		return FindSymbolIndex( _idx, s );
	}
	return NULL;
}

//--------------------------------------------------------------------------//

SYMBOL DbSymbols::GetSymbolByName( const char* _symbolName )
{
	SymbolMap::iterator it = g_SymbolMap.find( _symbolName );
	if( it != g_SymbolMap.end() )
	{
		SYMBOL s = it->second;
		return s;
	}
	return NULL;
}

//--------------------------------------------------------------------------//

long DbSymbols::GetEnumSymbolValue( const char* _enumValueName )
{
	// quick hack to make sure "true" & "false" strings faster
	if( !strcmp(_enumValueName,"true") || !strcmp(_enumValueName,"false") )
	{
		return( strcmp(_enumValueName,"true") == 0 );
	}

	for(SymbolMap::iterator sit = g_SymbolMap.begin(); sit != g_SymbolMap.end(); sit++ )
	{
		SYMBOL s = sit->second;

		if( s->kind == ENUMSYM )
		{
			SYMBOL enm = s->datatype;
			while( enm )
			{
				if( !strcmp(_enumValueName,enm->name) )
				{
					return enm->constval.intnum;
				}
				enm = enm->link;
			}
		}
	}
	return -1;
}

//------------------------------------------------------------------------------

void DbSymbols_FindDataCursor( DWORD& _thisIdx, RawData& _tempRawData, SYMBOL s )
{
	if( !s ) return;

	if( s->kind == RECORDSYM )
	{
		_tempRawData.StartRecord( s->name, s->size );

		SYMBOL internalSymbol = s->datatype;
		while( internalSymbol )
		{
			DbSymbols_FindDataCursor(_thisIdx,_tempRawData,internalSymbol);
			if( _thisIdx == 0 )
			{
				if( !internalSymbol->link )
				{
					_tempRawData.EndRecord();
				}
				else if( internalSymbol->link->kind == VARSYM &&  internalSymbol->link->datatype->kind == RECORDSYM  )
				{
					_tempRawData.nextRecordName = internalSymbol->link->datatype->name;
				}
				return;
			}

			internalSymbol = internalSymbol->link;
		}

		_tempRawData.EndRecord();
	}
	else if( s->kind == ARRAYSYM )
	{
		if( s->datatype->kind == RECORDSYM )
		{
			_tempRawData.StartRecord( s->name, s->size );

			for( int i = 0; i < s->arrayval; i++ )
			{
				SYMBOL arrayRecSymbol = s->datatype->datatype;
				while( arrayRecSymbol )
				{
					DbSymbols_FindDataCursor( _thisIdx, _tempRawData, arrayRecSymbol );
					if( _thisIdx == 0 )
					{
						if( !arrayRecSymbol->link )
						{
							_tempRawData.EndRecord();
						}
						else if( arrayRecSymbol->link->kind == RECORDSYM )
						{
							_tempRawData.nextRecordName = arrayRecSymbol->link->name;
						}
						return;
					}
					arrayRecSymbol = arrayRecSymbol->link;
				}
			}

			_tempRawData.EndRecord();
		}
		else
		{
			if( _thisIdx == 0 ) return;
			_tempRawData.GetString( s->size );
			_thisIdx--;
			return;
		}
	}
	else if( s->kind == VARSYM )
	{
		assert( s->datatype );

		if( (s->datatype->kind == RECORDSYM) || (s->datatype->kind == ARRAYSYM) )
		{
			DbSymbols_FindDataCursor(_thisIdx,_tempRawData,s->datatype);
			return;
		}

		if( _thisIdx == 0 ) return;

		int size = PARSER->GetTypeSize(s);
		switch( size )
		{
			case 1: _tempRawData.GetByte(1); break;
			case 2: _tempRawData.GetShort(2); break;
			case 4: _tempRawData.GetLong(4); break;
			case 8: _tempRawData.GetDouble(8); break;
		}
		_thisIdx--;
	}
	else if( s->datatype )
	{
		DbSymbols_FindDataCursor(_thisIdx,_tempRawData,s->datatype);
	}
}

//------------------------------------------------------------------------------

SYMBOL DbSymbols::GetSymbolAndCursorByIndex( const char* _baseSymbolName, const DWORD _idx, DWORD& _outCur )
{
if( !strcmp(_baseSymbolName,"BT_GRANNY") )
{
	static bool first = true;
	if( first )
	{
		first = false;
		__asm int 3;
	}
}
	// first find the symbol by the index

	DWORD thisIdx = _idx;

	SYMBOL baseSymbol = GetSymbolByName(_baseSymbolName);
	SYMBOL thisSymbol = FindSymbolIndex(thisIdx,baseSymbol);
	assert( thisSymbol );

	long  bogusSize   = PARSER->GetTypeSize(baseSymbol);
	void* bogusStruct = ::malloc( bogusSize );

	thisIdx = _idx;

	string szNoRecord = "_no_record_";

	RawData tempRawData;
	tempRawData.SetData( bogusStruct, bogusSize );
	tempRawData.nextRecordName = szNoRecord;

	DbSymbols_FindDataCursor( thisIdx, tempRawData, baseSymbol );

	if( tempRawData.padList.size() && tempRawData.nextRecordName != tempRawData.padRecordName )
	{
		// pad for elements right after a padding chain
		tempRawData.GetByte(0);
	}

	if( tempRawData.nextRecordName != szNoRecord )
	{
		RawData::Record& r = tempRawData.recordList.front();

		if( r.name != tempRawData.nextRecordName )
		{
			tempRawData.ResolveDwordAlign();
		}
	}

	::free( bogusStruct );

	_outCur = tempRawData.dataCursor;

// big old hack, because of BT_TRIGGEROBJECT
int basicDataSize = pParser->GetTypeSize( GetSymbolByName("BASIC_DATA") );
if( basicDataSize == bogusSize )
{
	switch( _idx )
	{
		case 0: _outCur = 0; break;
		case 1: _outCur = 4; break;
		case 2: _outCur = 5; break;
	}
}

	return thisSymbol;
}