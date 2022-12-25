// SymbolTable.cpp
//
//
//

#include <windows.h>
#include <stdio.h>

#include "typedefs.h"


#include "SymbolTable.h"

U32 CSymbolTable::HashSymbolName( const char *symbol_name )
{
	U32 hash = 0, mul=1;
	const char *p = symbol_name;

	while( p && *p ) {
		hash += *p * mul;
		mul <<= 1;
		p++;
	}

	hash = hash % m_SymbolsTableSize;
	return hash;
}

//

U32 CSymbolTable::FindSymbolName( const char *symbol_name )
{
	U32 hash;
	if( (hash = HashSymbolName( symbol_name )) == 0xFFFFFFFF ) {
		return 0xFFFFFFFF;
	}

	U32 start = hash;
	while( m_Symbols[hash].st_type!=ST_UNALLOC && strcmp( m_Symbols[hash].symbol_name, symbol_name ) != 0 ) { 
		hash = (hash+1) % m_SymbolsTableSize;
		if( hash == start ) {
			return 0xFFFFFFFF;
		}
	}

	return hash;
}

//

HRESULT CSymbolTable::AddSymbol( const char *symbol_name, U32 st_type, void *initial_value )
{
	U32 hash;
	if( (hash = HashSymbolName( symbol_name )) == 0xFFFFFFFF ) {
		return E_FAIL;
	}

	U32 start = hash;
	while( m_Symbols[hash].st_type != ST_UNALLOC ) {
		hash = (hash+1) % m_SymbolsTableSize;
		if( hash == start ) {
			return E_FAIL;
		}
	}

	m_Symbols[hash].st_type = st_type;

	m_Symbols[hash].symbol_name = new char[strlen(symbol_name)+1];
	strcpy( m_Symbols[hash].symbol_name, symbol_name );

	switch( m_Symbols[hash].st_type ) {
	case ST_STRING:
		delete[] m_Symbols[hash].string_value;
		m_Symbols[hash].string_value = new char[strlen((const char*)initial_value)+1];
		strcpy( m_Symbols[hash].string_value, (const char*)initial_value );
		break;
	case ST_U32:
		m_Symbols[hash].u32_value = ((U32)initial_value);
		break;
	case ST_FLOAT:
		m_Symbols[hash].float_value = *((float*)&initial_value);
		break;
	}
	return S_OK;
}

//

HRESULT CSymbolTable::RemoveSymbol( const char *symbol_name )
{
	U32 hash;
	if( (hash = FindSymbolName( symbol_name )) == 0xFFFFFFFF ) {
		return E_FAIL;
	}

	m_Symbols[hash].st_type = ST_UNALLOC;
	
	delete[] m_Symbols[hash].symbol_name;
	m_Symbols[hash].symbol_name = NULL;

	return S_OK;
}

//

HRESULT CSymbolTable::SetSymbolValueFromU32( const char *symbol_name, U32 value )
{
	U32 hash;
	if( (hash = FindSymbolName( symbol_name )) == 0xFFFFFFFF ) {
		return E_FAIL;
	}

	char szVal[20+1];
	switch( m_Symbols[hash].st_type ) {
	
	case ST_STRING:
		delete[] m_Symbols[hash].string_value;
		itoa( value, szVal, 10 );
		m_Symbols[hash].string_value = new char[strlen(szVal)+1];
		strcpy( m_Symbols[hash].string_value, szVal );
		break;
	
	case ST_U32:
		m_Symbols[hash].u32_value = value;
		break;

	case ST_FLOAT:
		m_Symbols[hash].float_value = ((float)value);
		break;
	}
	return S_OK;
}

//

HRESULT CSymbolTable::SetSymbolValueFromString( const char *symbol_name, const char *value )
{
	U32 hash;
	if( (hash = FindSymbolName( symbol_name )) == 0xFFFFFFFF ) {
		return E_FAIL;
	}

	switch( m_Symbols[hash].st_type ) {
	case ST_STRING:
		delete[] m_Symbols[hash].string_value;
		m_Symbols[hash].string_value = new char[strlen(value)+1];
		strcpy( m_Symbols[hash].string_value, value );
		break;
	case ST_U32:	m_Symbols[hash].u32_value = atoi(value);				break;
	case ST_FLOAT:	m_Symbols[hash].float_value = ((float)atof(value));	break;
	}
	return S_OK;
}

//

HRESULT CSymbolTable::SetSymbolValueFromFloat( const char *symbol_name, float value )
{
	U32 hash;
	if( (hash = FindSymbolName( symbol_name )) == 0xFFFFFFFF ) {
		return E_FAIL;
	}

	char szVal[20+1];
	switch( m_Symbols[hash].st_type ) {
	case ST_STRING:
		delete[] m_Symbols[hash].string_value;
		sprintf( szVal, "%f", value );
		m_Symbols[hash].string_value = new char[strlen(szVal)+1];
		strcpy( m_Symbols[hash].string_value, szVal );
		break;
	case ST_U32:	m_Symbols[hash].u32_value = ((U32)value);		break;
	case ST_FLOAT:	m_Symbols[hash].float_value = ((float)value);	break;
	}
	return S_OK;
}

//

HRESULT CSymbolTable::GetSymbolValueAsU32( const char *symbol_name, U32 *value )
{
	U32 hash;
	if( (hash = FindSymbolName( symbol_name )) == 0xFFFFFFFF ) {
		return E_FAIL;
	}

	switch( m_Symbols[hash].st_type ) {
	case ST_STRING:	*value = atoi( m_Symbols[hash].string_value );	break;
	case ST_U32:	*value = m_Symbols[hash].u32_value;				break;
	case ST_FLOAT:	*value = (U32)m_Symbols[hash].float_value;		break;
	}
	return S_OK;
}

//

HRESULT CSymbolTable::GetSymbolValueAsString( const char *symbol_name, char *value, U32 max_value_len )
{
	U32 hash;
	if( (hash = FindSymbolName( symbol_name )) == 0xFFFFFFFF ) {
		return E_FAIL;
	}

	switch( m_Symbols[hash].st_type ) {
	case ST_STRING:	strncpy( value, m_Symbols[hash].string_value, max_value_len );	break;
	case ST_U32:	sprintf( value, "%d", m_Symbols[hash].u32_value );				break;
	case ST_FLOAT:	sprintf( value, "%f", m_Symbols[hash].float_value );			break;
	}
	return S_OK;
}

//

HRESULT CSymbolTable::GetSymbolValueAsFloat( const char *symbol_name, float *value )
{
	U32 hash;
	if( (hash = FindSymbolName( symbol_name )) == 0xFFFFFFFF ) {
		return E_FAIL;
	}

	switch( m_Symbols[hash].st_type ) {
	case ST_STRING:	*value = (float)atof( m_Symbols[hash].string_value );	break;
	case ST_U32:	*value = (float)m_Symbols[hash].u32_value;				break;
	case ST_FLOAT:	*value = m_Symbols[hash].float_value;					break;
	}
	return S_OK;
}

//

HRESULT CSymbolTable::RemoveAllSymbols()
{
	for( U32 i=0; i<m_SymbolsTableSize; i++ ) {
		m_Symbols[i].u32_value = 0;
		m_Symbols[i].st_type = ST_UNALLOC;
		
		delete[] m_Symbols[i].symbol_name;
		m_Symbols[i].symbol_name = NULL;
	}
	return S_OK;
}

//

CSymbolTable::CSymbolTable( U32 symbol_table_size )
{
	m_SymbolsTableSize = symbol_table_size;
	m_Symbols = new STSYMBOL[symbol_table_size];

	for( U32 s=0; s<m_SymbolsTableSize; s++ ) {
		m_Symbols[s].st_type = ST_UNALLOC;
		m_Symbols[s].symbol_name = NULL;
		m_Symbols[s].u32_value = 0;
	}
}

//

CSymbolTable::~CSymbolTable()
{
	RemoveAllSymbols();
	delete[] m_Symbols;
	m_Symbols = NULL;
	m_SymbolsTableSize = 0;
}

//


