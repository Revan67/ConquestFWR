//--------------------------------------------------------------------------//
//                                                                          //
//                               dbsymbols.h    							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/dbsymbols.h 7     11/13/03 1:52p Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------//

#ifndef DBSYMBOLS_H
#define DBSYMBOLS_H

#include "ViewCnst.H"
#include <stdio.h>

class DbSymbols
{
	private:
		static BOOL32 __stdcall StructEnumerator(struct IViewConstructor2 * cnst, const C8 *typeName, SYMBOL symbol, void *context);

		void ExportSymbolTable( SYMBOL s );
		void ExportSymbolRow( SYMBOL s, DWORD sModifiers = 0);
		
		SYMBOL FindSymbolIndex( DWORD& idx, SYMBOL s );

		void PrintLevels( void );

		FILE*  file;
		DWORD  level;
		bool   isEnum;
		bool   isArray;
		DWORD  arraySize;

		SYMBOL recordType;
		struct IViewConstructor2 * pParser;
		//BYTE*  rowData;

	public:

		DbSymbols( void );

		bool Init( struct IViewConstructor2 * );

		virtual ~DbSymbols();

		bool ExportTableHeader( const char* _symbolName );

		bool ExportRow( const char* _symbolName, struct IFileSystem* _dataFile );

		SYMBOL GetSymbolByIndex( const char* _baseSymbolName, DWORD _idx );

		SYMBOL GetSymbolByName( const char* _symbolName );

		SYMBOL GetSymbolAndCursorByIndex( const char* _baseSymbolName, const DWORD _idx, DWORD& _outCur );

		long GetEnumSymbolValue( const char* _enumValueName );
};

// Symbol Helper functions
namespace SH
{
	bool   IsEnum( SYMBOL s, BYTE& _bitCount );
	bool   IsInteger( SYMBOL s, BYTE& _signed );
	bool   IsFloat( SYMBOL s, BYTE& _signed );
	bool   IsString( SYMBOL s, BYTE& _numArray );
	bool   GetBitCounts( SYMBOL s, BYTE& bitsOffset, BYTE& bitsCount );
	SYMBOL GetBaseRecordSize( SYMBOL s, int& size, struct IViewConstructor2* pParser );
};

LPSTR far FloatToString(LPSTR buff,double r,int prec);
const char* IntegerToString( DWORD _integer, bool _signed = false );

#endif
