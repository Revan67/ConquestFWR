//--------------------------------------------------------------------------//
//                                                                          //
//                               RawData.h      							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/RawData.h 2     3/31/03 1:26p Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------//
#ifndef RAWDATA_HEADER
#define RAWDATA_HEADER

struct RawData
{
	struct Record
	{
		string name;
		int    size;
		int    used;
	};

	struct Records : list<Record>{};
	struct PadList : list<int>{};

	BYTE* data;
	DWORD dataSize;
	DWORD dataCursor;

	Records recordList;
	PadList padList;
	string  padRecordName;
	string  nextRecordName;

	void SetData( void* _data, int _dataSize );

	void StartRecord( const char* _record, int _recordSize );
	void EndRecord( void );

	void ResolveChainPadding( void );
	void ResolveDwordAlign( void );

	// peek data method(s)
	bool IsNullString( int _size );

	// get data method(s)
	long   GetLong( int _size );
	short  GetShort( int _size );
	INT64  GetInt64( int _size );
	BYTE   GetByte( int _size );
	char*  GetString( int _size );
	float  GetFloat( int _size );
	double GetDouble( int _size );
};


#endif
