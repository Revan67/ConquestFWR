//--------------------------------------------------------------------------//
//                                                                          //
//                               RawData.cpp      							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/RawData.cpp 2     3/31/03 1:26p Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------//

#include "pch.h"

//-------------------------------------------------------------------------------------
#pragma warning(disable : 4786)
#include <map>
#include <string>
#include <list>
using namespace std;

#include "rawData.h"

//------------------------------------------------------------------------------
// Raw Data structure implementation
//------------------------------------------------------------------------------

void RawData::SetData( void* _data, int _dataSize )
{
	data = (BYTE*)_data;
	dataSize = _dataSize;
	dataCursor = 0;
	
	if( recordList.size() )
	{
		recordList.clear();
	}
}

//------------------------------------------------------------------------------

void RawData::StartRecord( const char* _record, int _recordSize )
{
	// if record size is not DWORD aligned, it must be auto mushed with other records
	if( _recordSize % sizeof(DWORD) )
	{
		if( padList.size() )
		{
			if( padList.front() == _recordSize )
			{
				// record more padding
				padList.push_back( _recordSize );
			}
			else
			{
				// resolve this type of record's padding ...
				ResolveChainPadding();

				//... and start a new list of padding
				padRecordName = _record;
				padList.push_back( _recordSize );
			}
		}
		else
		{
			// start recording pad
			padRecordName = _record;
			padList.push_back( _recordSize );
		}
	}
	else if( padList.size() )
	{
		// resolve this record's padding, before starting this record
		ResolveChainPadding();
	}

	ResolveDwordAlign();

	recordList.push_front( Record() );

	Record& r = recordList.front();
	r.name  = _record;
	r.size  = _recordSize;
	r.used  = 0;
}

//------------------------------------------------------------------------------

void RawData::EndRecord( void )
{
	Record& currentRecord = recordList.front();

	if( currentRecord.used > currentRecord.size )
	{
		__asm int 3;
	}

	// pad for rest of "size"
	if( currentRecord.used < currentRecord.size )
	{
		dataCursor += currentRecord.size - currentRecord.used;
	}

	int currentRecordSize = currentRecord.size;
	recordList.pop_front();

	// update last record
	if( recordList.size() )
	{
		int padSize = 0;
		if( currentRecordSize % sizeof(DWORD) )
		{
			if( currentRecordSize < sizeof(DWORD) )
			{
				padSize = sizeof(DWORD) - currentRecordSize;
			}
			else
				padSize = currentRecordSize % sizeof(DWORD);
		}

		Record& prevRecord = recordList.front();
		prevRecord.used   += currentRecordSize + padSize;
	}
}

//------------------------------------------------------------------------------

bool RawData::IsNullString( int _size )
{
	BYTE* pData = data + dataCursor;

	for( int i = 0; i < _size; i++ )
	{
		if( pData[i] != 0 )
		{
			return false;
		}
	}
	return true;
}

//------------------------------------------------------------------------------

long RawData::GetLong( int _size )
{
	Record& currentRecord = recordList.front();
	currentRecord.used += _size;

	if( padRecordName != currentRecord.name && padList.size() )
	{
		ResolveChainPadding();
	}

	long* lng = reinterpret_cast<long*>( data + dataCursor );

	dataCursor += _size;

	return *lng;
}

//------------------------------------------------------------------------------

short RawData::GetShort( int _size )
{
	Record& currentRecord = recordList.front();
	currentRecord.used += _size;

	if( padRecordName != currentRecord.name && padList.size() )
	{
		ResolveChainPadding();
	}

	short* shrt = reinterpret_cast<short*>( data + dataCursor );

	dataCursor += _size;

	return *shrt;
}

//------------------------------------------------------------------------------

INT64 RawData::GetInt64( int _size )
{
	Record& currentRecord = recordList.front();
	currentRecord.used += _size;

	if( padRecordName != currentRecord.name && padList.size() )
	{
		ResolveChainPadding();
	}

	__int64* int64 = reinterpret_cast<__int64*>( data + dataCursor );

	dataCursor += _size;

	return *int64;
}

//------------------------------------------------------------------------------

BYTE RawData::GetByte( int _size )
{
	Record& currentRecord = recordList.front();
	currentRecord.used += _size;

	if( padRecordName != currentRecord.name && padList.size() )
	{
		ResolveChainPadding();
	}

	BYTE* byte = reinterpret_cast<BYTE*>( data + dataCursor );

	dataCursor += _size;

	return *byte;
}

//------------------------------------------------------------------------------

char* RawData::GetString( int _size )
{
	Record& currentRecord = recordList.front();
	currentRecord.used += _size;

	if( padRecordName != currentRecord.name && padList.size() )
	{
		ResolveChainPadding();
	}

	char* byte = reinterpret_cast<char*>( data + dataCursor );

	dataCursor += _size;

	return byte;
}

//------------------------------------------------------------------------------

float RawData::GetFloat( int _size )
{
	Record& currentRecord = recordList.front();
	currentRecord.used += _size;

	if( padRecordName != currentRecord.name && padList.size() )
	{
		ResolveChainPadding();
	}

	float* flt = reinterpret_cast<float*>( data + dataCursor );

	dataCursor += _size;

	return *flt;
}

//------------------------------------------------------------------------------

double RawData::GetDouble( int _size )
{
	Record& currentRecord = recordList.front();
	currentRecord.used += _size;

	if( padRecordName != currentRecord.name && padList.size() )
	{
		ResolveChainPadding();
	}

	double* dbl = reinterpret_cast<double*>( data + dataCursor );

	dataCursor += _size;

	return *dbl;
}

//------------------------------------------------------------------------------

void RawData::ResolveChainPadding( void )
{
	int totalSize = 0;
	for( PadList::iterator pit = padList.begin(); pit != padList.end(); pit++ )
	{
		totalSize += *pit;
	}
	padList.clear();
	padRecordName = "NoPadRecordName";

	// find the padding size
	int padSize = totalSize % sizeof(DWORD); 

	// update the data cursor
	dataCursor += padSize;
}

//------------------------------------------------------------------------------

void RawData::ResolveDwordAlign( void )
{
	if( !padList.size() && (dataCursor % sizeof(DWORD)) )
	{
		// all records need to start DWORD aligned
		while( dataCursor % sizeof(DWORD) )
		{
			dataCursor++;
		}
	}
}
