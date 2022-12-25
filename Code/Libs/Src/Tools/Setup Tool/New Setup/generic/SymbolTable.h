//
//
//
//



class CSymbolTable 
{
public:
	HRESULT AddSymbol( const char *symbol_name, U32 st_type, void *initial_value );
	HRESULT RemoveSymbol( const char *symbol_name );
	
	HRESULT SetSymbolValueFromU32( const char *symbol_name, U32 value );
	HRESULT SetSymbolValueFromString( const char *symbol_name, const char *value );
	HRESULT SetSymbolValueFromFloat( const char *symbol_name, float value );

	HRESULT GetSymbolValueAsU32( const char *symbol_name, U32 *value );
	HRESULT GetSymbolValueAsString( const char *symbol_name, char *value, U32 max_value_len );
	HRESULT GetSymbolValueAsFloat( const char *symbol_name, float *value );

	HRESULT RemoveAllSymbols();

	CSymbolTable( U32 symbol_table_size=23 );
	virtual ~CSymbolTable();

protected:
	U32 HashSymbolName( const char *symbol_name );
	U32 FindSymbolName( const char *symbol_name );

	struct STSYMBOL 
	{
		union {
			U32 u32_value;
			float float_value;
			char *string_value;
			void *user_value;
		};
		U32 st_type;
		char *symbol_name;
	};
#define ST_UNALLOC	0
#define ST_UNDEF	1
#define ST_FLOAT	2
#define ST_U32		3
#define ST_STRING	4	
//#define ST_USER		5

	STSYMBOL  *m_Symbols;
	U32        m_SymbolsTableSize;
};

