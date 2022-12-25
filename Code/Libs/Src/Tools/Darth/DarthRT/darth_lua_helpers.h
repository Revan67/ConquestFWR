// darth_lua_helpers.h
//
//
//

#ifndef darth_lua_helpers_h
#define darth_lua_helpers_h


#define TBUFFER_MAX_LEN 4096
static char s_tbuffer[2][TBUFFER_MAX_LEN];

#ifdef UNICODE
inline char *tchar_convert( TCHAR *in, int buf=0 )	
{
	tcscpy_check( in, TBUFFER_MAX_LEN );	
	tcscpy_os2ansi( s_tbuffer[buf], in, TBUFFER_MAX_LEN );
	return s_tbuffer[buf];
}
#else
inline char *tchar_convert( TCHAR *in, int buf=0 )	
{
	return in;
}
#endif

//

inline void lua_set_string( lua_Object tbl, TCHAR *Member, TCHAR *Value )
{
	if( !tbl ) {
		ASSERT( Member );		// Member required if setting global
		lua_pushstring( tchar_convert( Value ) );
		lua_setglobal( tchar_convert( Member ) );
	}
	else if( Member == NULL ) {
		ASSERT( 0 );			// TODO: lua_setstring();
	}
	else {
		lua_pushobject( tbl );
		lua_pushstring( tchar_convert( Member ) );
		lua_pushstring( tchar_convert( Value ) );
		lua_settable();
	}
}

//

inline void lua_get_string( lua_Object tbl, TCHAR *Member, TCHAR *out_Value, int MaxLen )
{
	char *as;
	lua_Object s = 0;

	if( !tbl ) {
		ASSERT( Member );
		s = lua_getglobal( tchar_convert( Member ) );
	}
	else if( Member == NULL ) {
		s = tbl;
	}
	else {
		lua_pushobject( tbl );
		lua_pushstring( tchar_convert( Member ) );
		s = lua_gettable();
	}

	if( as = lua_getstring(s) ) {
		tcscpy_ansi2os( out_Value, as, MaxLen );
	}
	else {
		_tcscpy( out_Value, _T("") );
	}
}

//

inline void lua_set_object( lua_Object tbl, TCHAR *Member, lua_Object Value )
{
	if( !tbl ) {
		ASSERT( Member );
		lua_pushobject( Value );
		lua_setglobal( tchar_convert( Member ) );
	}
	else if( Member == NULL ) {
		ASSERT( 0 );	// TODO: lua_setobject()
	}
	else {
		lua_pushobject( tbl );
		lua_pushstring( tchar_convert( Member ) );
		lua_pushobject( Value );
		lua_settable();
	}
}

//

inline void lua_get_object( lua_Object tbl, TCHAR *Member, lua_Object *out_Value )
{
	if( !tbl ) {
		ASSERT( Member );
		*out_Value = lua_getglobal( tchar_convert( Member ) );
	}
	else if( Member == NULL ) {
		*out_Value = tbl;	// Lame but consistent.
	}	
	else {
		lua_pushobject( tbl );
		lua_pushstring( tchar_convert( Member ) );
		*out_Value = lua_gettable();
	}
}

//

inline void lua_set_userdata( lua_Object tbl, TCHAR *Member, void *Value )
{
	if( !tbl ) {
		ASSERT( Member );
		lua_pushuserdata( Value );
		lua_setglobal( tchar_convert( Member ) );
	}
	else if( Member == NULL ) {
		ASSERT( 0 ); //lua_setuserdata()
	}	
	else {
		lua_pushobject( tbl );
		lua_pushstring( tchar_convert( Member ) );
		lua_pushuserdata( Value );
		lua_settable();
	}
}

//

inline void lua_get_userdata( lua_Object tbl, TCHAR *Member, void **out_Value )
{
	lua_Object o;

	if( !tbl ) {
		ASSERT( Member );
		o = lua_getglobal( tchar_convert( Member ) );
	}
	else if( Member == NULL ) {
		o = tbl;
	}
	else {
		lua_pushobject( tbl );
		lua_pushstring( tchar_convert( Member ) );
		o = lua_gettable();
	}

	*out_Value = lua_getuserdata(o);
}

//

inline void lua_set_float( lua_Object tbl, TCHAR *Member, float Value )
{
	if( !tbl ) {
		ASSERT( Member );
		lua_pushnumber( Value );
		lua_setglobal( tchar_convert( Member ) );
	}
	else if( Member == NULL ) {
		ASSERT( 0 );		// TODO: lua_setnumber
	}
	else {
		lua_pushobject( tbl );
		lua_pushstring( tchar_convert( Member ) );
		lua_pushnumber( Value );
		lua_settable();
	}
}

//

inline void lua_get_float( lua_Object tbl, TCHAR *Member, float *out_Value )
{
	lua_Object s;

	if( !tbl ) {
		ASSERT( Member );
		s = lua_getglobal( tchar_convert( Member ) );
	}
	else if( Member == NULL ) {
		s = tbl;
	}
	else {
		lua_pushobject( tbl );
		lua_pushstring( tchar_convert( Member ) );
		s = lua_gettable();
	}

	*out_Value = lua_getnumber( s );
}

//

inline void lua_get_int( lua_Object tbl, TCHAR *Member, int *out_Value )
{
	lua_Object s;

	if( !tbl ) {
		ASSERT( Member );
		s = lua_getglobal( tchar_convert( Member ) );
	}
	else if( Member == NULL ) {
		s = tbl;
	}
	else {
		lua_pushobject( tbl );
		lua_pushstring( tchar_convert( Member ) );
		s = lua_gettable();
	}

	*out_Value = lua_getnumber( s );
}

//

inline void lua_set_int( lua_Object tbl, TCHAR *Member, int Value )
{
	if( !tbl ) {
		ASSERT( Member );
		lua_pushnumber( Value );
		lua_setglobal( tchar_convert( Member ) );
	}
	else if( Member == NULL ) {
		ASSERT( 0 );		// TODO: lua_setnumber()
	}
	else {
		lua_pushobject( tbl );
		lua_pushstring( tchar_convert( Member ) );
		lua_pushnumber( Value );
		lua_settable();
	}
}

//

inline void lua_get_vector_float( lua_Object tbl, TCHAR *Member, int NumItems, float *out_Value )
{
	lua_Object t;

	if( !tbl ) {
		t = lua_getglobal( tchar_convert( Member ) );
	}
	else if( Member == NULL ) {
		t = tbl;
	}
	else {
		lua_pushobject( tbl );
		lua_pushstring( tchar_convert( Member ) );
		t = lua_gettable();
	}

	if( !lua_istable( t ) ) {
		for( int i=0; i<NumItems; i++ ) {
			out_Value[i] = 0.00F;
		}
	}
	else {
		for( int i=0; i<NumItems; i++ ) {
			lua_pushobject( t );
			lua_pushnumber( i+1 );
			out_Value[i] = lua_getnumber( lua_gettable() );
		}
	}

}
//

inline void lua_set_vector_float( lua_Object tbl, TCHAR *Member, int NumItems, float *in_Value )
{
	ASSERT( 0 );
}



#endif // EOF
