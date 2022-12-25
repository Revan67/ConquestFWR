//
// stfunctions.cpp
//
//

#pragma warning( disable : 4018 )	// signed/unsigned mismatch
#pragma warning( disable : 4100 )	// unreferenced formal parameter
#pragma warning( disable : 4239 )
#pragma warning( disable : 4245 )	// conversion from int to long
#pragma warning( disable : 4530 )   // exceptions disabled
#pragma warning( disable : 4702 )   // unreachable code
#pragma warning( disable : 4710 )   // function 'foo' not inlined
#pragma warning( disable : 4786 )   // identifier truncated

//

#define D3D_OVERLOADS
#include <windows.h>
#include <assert.h>
#include <ddraw.h>
#include <d3d9.h>
#include <map>
#include <string>

//

#include "D3DX.h"

//

#include "FVF.h"	// DALIBS HEADER!

//

#include "stfunctions.h"

//

struct ST_VERTEXBUFFER
{
	DWORD num_verts;
	DWORD vertex_format;
	LPDIRECT3DVERTEXBUFFER7 vb;
};

//

typedef std::map< std::string, ST_VERTEXBUFFER > st_vertex_buffer_map;

//

typedef std::map< std::string, LPDIRECTDRAWSURFACE7 > st_loaded_texture_map;

//

static LPDIRECT3DDEVICE7 g_D3DDevice = NULL;

st_loaded_texture_map loaded_textures;
st_vertex_buffer_map vertex_buffers;

//

void st_load_file( const char *filename )
{
	lua_dofile( const_cast<char*>( filename ) );
}

//

void st_cleanup( void )
{
	st_loaded_texture_map::iterator tbeg = loaded_textures.begin();
	st_loaded_texture_map::iterator tend = loaded_textures.end();
	st_loaded_texture_map::iterator t;

	for( t=tbeg; t!=tend; t++ ) {
		t->second->Release();
	}

	loaded_textures.clear();

	st_vertex_buffer_map::iterator vbeg = vertex_buffers.begin();
	st_vertex_buffer_map::iterator vend = vertex_buffers.end();
	st_vertex_buffer_map::iterator v;

	for( v=vbeg; v!=vend; v++ ) {
		v->second.vb->Release();
	}

	vertex_buffers.clear();
}

//

void st_init_scene( HWND hWnd, LPDIRECT3DDEVICE7 D3DDevice )
{
	g_D3DDevice = D3DDevice;
	lua_callfunction(lua_getglobal("initstate"));
	g_D3DDevice = NULL;
}

//

void st_render_scene( LPDIRECT3DDEVICE7 D3DDevice )
{
	g_D3DDevice = D3DDevice;
	lua_callfunction(lua_getglobal("scene"));
	g_D3DDevice = NULL;
}

//

template< typename T >
inline T st_lua_get_number( lua_Object table, char *key )
{
	lua_pushobject( table );
	lua_pushstring( key );
	return (T)lua_getnumber( lua_gettable() );
}

//

template< typename T >
inline void st_lua_get_number_vector( lua_Object table, char *key, int count, T *out_values )
{
	lua_Object v;

	if( key ) {
		lua_pushobject( table );
		lua_pushstring( key );
		v = lua_gettable();
	}
	else {
		v = table ;
	}


	if( count > 4 ) {

		for( int i=0; i<count; i++ ) {
			lua_pushobject( v );
			lua_pushnumber( i+1 );
			out_values[i] = (T)lua_getnumber( lua_gettable() );
		}
	}
	else {

		lua_pushobject( v );
		lua_pushnumber( 1 );
		out_values[0] = (T)lua_getnumber( lua_gettable() );

		lua_pushobject( v );
		lua_pushnumber( 2 );
		out_values[1] = (T)lua_getnumber( lua_gettable() );

		lua_pushobject( v );
		lua_pushnumber( 3 );
		out_values[2] = (T)lua_getnumber( lua_gettable() );

		lua_pushobject( v );
		lua_pushnumber( 4 );
		out_values[3] = (T)lua_getnumber( lua_gettable() );
	}
}

//

void st_fbconfig( void )
{
	DWORD width = lua_getnumber( lua_getparam( 1 ) ) ;
	DWORD height = lua_getnumber( lua_getparam( 2 ) ) ;
	DWORD color = lua_getnumber( lua_getparam( 3 ) ) ;
	DWORD depth = lua_getnumber( lua_getparam( 4 ) ) ;
	DWORD stencil = lua_getnumber( lua_getparam( 5 ) ) ;
	DWORD fullscreen = lua_getnumber( lua_getparam( 6 ) ) ;

	// TODO: do something with this information
}

//

void st_clear( void )
{
	D3DRECT rect;
	
	st_lua_get_number_vector( lua_getparam( 1 ), NULL, 4, (long*)&rect );
	
	DWORD flags   = lua_getnumber( lua_getparam( 2 ) );
	DWORD color   = lua_getnumber( lua_getparam( 3 ) );
	float depth   = lua_getnumber( lua_getparam( 4 ) );
	DWORD stencil = lua_getnumber( lua_getparam( 5 ) );


	if( FAILED( g_D3DDevice->Clear( 1, &rect, flags, color, depth, stencil ) ) ) {
		// trace
	}
}

//

void st_begin_scene( void )
{
	long scene_number = lua_getnumber( lua_getparam( 1 ) ) ;

	if( FAILED( g_D3DDevice->BeginScene() ) ) {
		// trace a message
	}
}

//

void st_end_scene( void )
{
	long scene_number = lua_getnumber( lua_getparam( 1 ) ) ;

	if( FAILED( g_D3DDevice->EndScene() ) ) {
		// trace a message
	}
}

//

void st_viewport( void ) 
{
	D3DVIEWPORT9 vp;

	vp.dwX		= lua_getnumber( lua_getparam( 1 ) ) ;
	vp.dwY		= lua_getnumber( lua_getparam( 2 ) ) ;
	vp.dwWidth	= lua_getnumber( lua_getparam( 3 ) ) ;
	vp.dwHeight = lua_getnumber( lua_getparam( 4 ) ) ;
	vp.dvMinZ	= lua_getnumber( lua_getparam( 5 ) ) ;
	vp.dvMaxZ	= lua_getnumber( lua_getparam( 6 ) ) ;

	if( FAILED( g_D3DDevice->SetViewport( &vp ) ) ) {
		// trace a message
	}
}

//

void st_ortho( void )
{
	// TODO: setup our ortho mode
}

//

void st_perspective( void )	
{
	// TODO: setup our persp mode
}

//

void st_set_transform( D3DTRANSFORMSTATETYPE which, long offset )		
{
	D3DMATRIX M;

	M.m[0][0] = lua_getnumber( lua_getparam( offset +  1 ) ) ;
	M.m[0][1] = lua_getnumber( lua_getparam( offset +  2 ) ) ;
	M.m[0][2] = lua_getnumber( lua_getparam( offset +  3 ) ) ;
	M.m[0][3] = lua_getnumber( lua_getparam( offset +  4 ) ) ;

	M.m[1][0] = lua_getnumber( lua_getparam( offset +  5 ) ) ;
	M.m[1][1] = lua_getnumber( lua_getparam( offset +  6 ) ) ;
	M.m[1][2] = lua_getnumber( lua_getparam( offset +  7 ) ) ;
	M.m[1][3] = lua_getnumber( lua_getparam( offset +  8 ) ) ;

	M.m[2][0] = lua_getnumber( lua_getparam( offset +  9 ) ) ;
	M.m[2][1] = lua_getnumber( lua_getparam( offset + 10 ) ) ;
	M.m[2][2] = lua_getnumber( lua_getparam( offset + 11 ) ) ;
	M.m[2][3] = lua_getnumber( lua_getparam( offset + 12 ) ) ;

	M.m[3][0] = lua_getnumber( lua_getparam( offset + 13 ) ) ;
	M.m[3][1] = lua_getnumber( lua_getparam( offset + 14 ) ) ;
	M.m[3][2] = lua_getnumber( lua_getparam( offset + 15 ) ) ;
	M.m[3][3] = lua_getnumber( lua_getparam( offset + 16 ) ) ;

	if( FAILED( g_D3DDevice->SetTransform( which, &M ) ) ) {
		// trace
	}
}

//

void st_world_transform( void )		
{
	st_set_transform( D3DTS_WORLD, 0 );
}

//

void st_view_transform( void )		
{
	st_set_transform( D3DTS_VIEW, 0 );
}

//

void st_projection_transform( void )		
{
	st_set_transform( D3DTS_PROJECTION, 0 );
}

//

void st_texture_transform( void )		
{
	long stage = lua_getnumber( lua_getparam( 1 ) );

	st_set_transform( (D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + stage), 1 );
}

//

void st_render_state( void )
{
	long state = lua_getnumber( lua_getparam( 1 ) ) ;
	long value = lua_getnumber( lua_getparam( 2 ) ) ;

	if( FAILED( g_D3DDevice->SetRenderState( (D3DRENDERSTATETYPE)state, value ) ) ) {
		// trace
	}
}

//

void st_null_texture( void )		
{
	long stage = lua_getnumber( lua_getparam( 1 ) ) ;

	if( FAILED( g_D3DDevice->SetTexture( stage, NULL ) ) ) {
		// trace
	}
}

//

void st_find_texture( const char *filename, LPDIRECTDRAWSURFACE7 *out_surface )
{
	st_loaded_texture_map::iterator texture;

	if( (texture = loaded_textures.find( std::string( filename ) )) != loaded_textures.end() ) {
		
		*out_surface = texture->second;

	}
	else {
		
		D3DX_SURFACEFORMAT format = D3DX_SF_UNKNOWN;
		DWORD mipcount;
		HRESULT hr;

		hr = D3DXCreateTextureFromFile(	g_D3DDevice,
									    0, 
									    0,  
									    0,  
									    &format,
									    NULL,
									    out_surface,
									    &mipcount,
									    const_cast<char*>( filename ),
									    D3DX_FT_DEFAULT );

		if( SUCCEEDED( hr ) ) {
			loaded_textures[ std::string( filename ) ] = *out_surface;
		}
	}
}

//

void st_texture( void )		
{
	long stage = lua_getnumber( lua_getparam( 1 ) ) ;
	char *texture = lua_getstring( lua_getparam( 2 ) ) ;

	LPDIRECTDRAWSURFACE7 surface = NULL;

	st_find_texture( texture, &surface );

	if( FAILED( g_D3DDevice->SetTexture( stage, surface ) ) ) {
		// trace
	}
}

//

void st_texture_state( void )
{
	long stage = lua_getnumber( lua_getparam( 1 ) ) ;
	long state = lua_getnumber( lua_getparam( 2 ) ) ;
	long value = lua_getnumber( lua_getparam( 3 ) ) ;

	if( FAILED( g_D3DDevice->SetTextureStageState( stage, (D3DTEXTURESTAGESTATETYPE)state, value ) ) ) {
		// trace
	}
}

//

void st_light( void )
{
	long index = lua_getnumber( lua_getparam( 1 ) ) ;
	lua_Object light = lua_getparam( 2 ) ;

	D3DLIGHT9 value;

	value.Type	= (D3DLIGHTTYPE)st_lua_get_number<long>( light, "Type" );
	value.dvRange	= st_lua_get_number<float>( light, "Range" );
	value.dvFalloff = st_lua_get_number<float>( light, "Falloff" );
	value.dvTheta	= st_lua_get_number<float>( light, "Theta" );
	value.dvPhi		= st_lua_get_number<float>( light, "Phi" );

	st_lua_get_number_vector( light, "Position", 3, (float*)&value.dvPosition );
	st_lua_get_number_vector( light, "Direction", 3, (float*)&value.dvDirection );
	st_lua_get_number_vector( light, "Attenuation", 3, (float*)&value.dvAttenuation0 );
	st_lua_get_number_vector( light, "Ambient", 4, (float*)&value.dcvAmbient );
	st_lua_get_number_vector( light, "Diffuse", 4, (float*)&value.dcvDiffuse );
	st_lua_get_number_vector( light, "Specular", 4, (float*)&value.dcvSpecular );

	if( FAILED( g_D3DDevice->SetLight( index, &value ) ) ) {
		// trace
	}
}

//

void st_light_enable( void )
{
	long index = lua_getnumber( lua_getparam( 1 ) ) ;
	long value = lua_getnumber( lua_getparam( 2 ) ) ;

	if( FAILED( g_D3DDevice->LightEnable( index, value ) ) ) {
		// trace
	}
}

//

void st_material( void )
{
	lua_Object mat = lua_getparam( 1 ) ;

	D3DMATERIAL9 value;

	value.dvPower = st_lua_get_number<float>( mat, "Power" );

	st_lua_get_number_vector( mat, "Ambient", 4, (float*)&value.dcvAmbient );
	st_lua_get_number_vector( mat, "Emissive", 4, (float*)&value.dcvEmissive );
	st_lua_get_number_vector( mat, "Diffuse", 4, (float*)&value.dcvDiffuse );
	st_lua_get_number_vector( mat, "Specular", 4, (float*)&value.dcvSpecular );

	if( FAILED( g_D3DDevice->SetMaterial( &value ) ) ) {
		// trace
	}
}

//

void st_swap( void )
{
}


//

void st_fill_vertex_buffer( const char *filename, LPDIRECT3DVERTEXBUFFER7 *out_vb )
{
	st_vertex_buffer_map::iterator vb;

	if( (vb = vertex_buffers.find( std::string( filename ) )) != vertex_buffers.end() ) {

		*out_vb = vb->second.vb;

	}
	else {

		DWORD magic;
		DWORD vertex_format;
		DWORD num_verts;
		void *dst_verts;
		FILE *file;
		IDirect3D7 *D3D;
		ST_VERTEXBUFFER stvb;

		if( (file = fopen( filename, "rb" )) != NULL ) {

			fread( &magic, sizeof(DWORD), 1, file );
			fread( &vertex_format, sizeof(DWORD), 1, file );
			fread( &num_verts, sizeof(DWORD), 1, file );

			if( SUCCEEDED( g_D3DDevice->GetDirect3D( &D3D ) ) ) {

				stvb.num_verts = __max( num_verts, 64 );
				stvb.vertex_format = vertex_format;
			
				// Create the vertex buffer.
				D3DVERTEXBUFFERDESC vbdesc;
				ZeroMemory( &vbdesc, sizeof(D3DVERTEXBUFFERDESC) );
				vbdesc.dwSize        = sizeof(D3DVERTEXBUFFERDESC);
				vbdesc.dwCaps        = 0L;
				vbdesc.dwFVF         = stvb.vertex_format;
				vbdesc.dwNumVertices = stvb.num_verts ;


				if( SUCCEEDED( D3D->CreateVertexBuffer( &vbdesc, &stvb.vb, 0L ) ) ) {

					vertex_buffers[ std::string( filename ) ] = stvb;

					if( SUCCEEDED( stvb.vb->Lock( DDLOCK_WAIT, (VOID**)&dst_verts, NULL ) ) ) {

						fread( dst_verts, 1, FVF_SIZEOF_VERT(vertex_format) * num_verts, file );
						
						stvb.vb->Unlock();
					}
				}

				D3D->Release();
			}

			*out_vb = stvb.vb;

			fclose( file );
		}
		else {

			*out_vb = NULL;

		}
	}
}

//

void st_fill_index_buffer( const char *filename, WORD *out_ib )
{
	DWORD num_indices;
	DWORD magic;
	FILE *file;

	if( (file = fopen( filename, "rb" )) == NULL ) {
		return;
	}

	fread( &magic, sizeof(DWORD), 1, file );
	fread( &num_indices, sizeof(DWORD), 1, file );
	fread( out_ib, sizeof(WORD), num_indices, file );
	
	fclose( file );
}

//

void st_indexed_primitive_vb( D3DPRIMITIVETYPE type )
{
	static WORD indices[32*1024];

	LPDIRECT3DVERTEXBUFFER7 vb = NULL ; 
	char *vbfilename = lua_getstring( lua_getparam( 1 ) );
	DWORD start_vert = lua_getnumber( lua_getparam( 2 ) ) ;
	DWORD num_verts = lua_getnumber( lua_getparam( 3 ) ) ;
	char *ibfilename = lua_getstring( lua_getparam( 4 ) );
	DWORD num_indices = lua_getnumber( lua_getparam( 5 ) ) ;
	DWORD flags = lua_getnumber( lua_getparam( 6 ) ) ;

	st_fill_vertex_buffer( vbfilename, &vb );
	st_fill_index_buffer( ibfilename, indices );

	if( FAILED( g_D3DDevice->DrawIndexedPrimitiveVB( type, vb, start_vert, num_verts, indices, num_indices, flags ) ) ) {
		// trace
	}

}

//

void st_indexed_pointlist_vb( void )
{
	st_indexed_primitive_vb( D3DPT_POINTLIST );
}

//

void st_indexed_linelist_vb( void )
{
	st_indexed_primitive_vb( D3DPT_LINELIST );
}

//

void st_indexed_linestrip_vb( void ) 
{
	st_indexed_primitive_vb( D3DPT_LINESTRIP );
}

//

void st_indexed_trilist_vb( void ) 
{
	st_indexed_primitive_vb( D3DPT_TRIANGLELIST );
}

//

void st_indexed_tristrip_vb( void ) 
{
	st_indexed_primitive_vb( D3DPT_TRIANGLESTRIP );
}

//

void st_indexed_trifan_vb( void ) 
{
	st_indexed_primitive_vb( D3DPT_TRIANGLEFAN );
}

//

void st_primitive_vb( D3DPRIMITIVETYPE type )
{
	LPDIRECT3DVERTEXBUFFER7 vb = NULL ;
	char *vbfilename = lua_getstring( lua_getparam( 1 ) ) ;
	DWORD start_vert = lua_getnumber( lua_getparam( 2 ) ) ;
	DWORD num_verts = lua_getnumber( lua_getparam( 3 ) ) ;
	DWORD flags = lua_getnumber( lua_getparam( 4 ) ) ;

	st_fill_vertex_buffer( vbfilename, &vb );

	if( FAILED( g_D3DDevice->DrawPrimitiveVB( type, vb, start_vert, num_verts, flags ) ) ) {
		// trace
	}
}

//

void st_pointlist_vb( void )
{
	st_primitive_vb( D3DPT_POINTLIST );
}

//

void st_linelist_vb( void )
{
	st_primitive_vb( D3DPT_LINELIST );
}

//

void st_linestrip_vb( void ) 
{
	st_primitive_vb( D3DPT_LINESTRIP );
}

//

void st_trilist_vb( void ) 
{
	st_primitive_vb( D3DPT_TRIANGLELIST );
}

//

void st_tristrip_vb( void ) 
{
	st_primitive_vb( D3DPT_TRIANGLESTRIP );
}

//

void st_trifan_vb( void ) 
{
	st_primitive_vb( D3DPT_TRIANGLEFAN );
}

//



//

void st_init( void )
{
	lua_open();

	lua_register( "fbconfig",			st_fbconfig );	 
	lua_register( "clear",				st_clear );	 
	lua_register( "beginscene",			st_begin_scene );	 
	lua_register( "endscene",			st_end_scene );		 
	lua_register( "viewport",			st_viewport );		 
	lua_register( "ortho",				st_ortho );			 
	lua_register( "perspective",		st_perspective );	 
	lua_register( "world",				st_world_transform );		 
	lua_register( "view",				st_view_transform );		 
	lua_register( "projection",			st_projection_transform );		 
	lua_register( "texturetransform",	st_texture_transform );		 
	lua_register( "renderstate",		st_render_state );	 
	lua_register( "nulltexture",		st_null_texture );		 
	lua_register( "texture",			st_texture );		 
	lua_register( "texturestate",		st_texture_state );	 
	lua_register( "light",				st_light );			 
	lua_register( "lightenable",		st_light_enable );	 
	lua_register( "material",			st_material );		 
	lua_register( "swap",				st_swap );			 

	lua_register( "indexedpointlistvb", st_indexed_pointlist_vb );
	lua_register( "indexedlinelistvb",	st_indexed_linelist_vb );
	lua_register( "indexedlinestripvb", st_indexed_linestrip_vb );
	lua_register( "indexedtrilistvb",	st_indexed_trilist_vb );
	lua_register( "indexedtristripvb",	st_indexed_tristrip_vb );
	lua_register( "indexedtrifanvb",	st_indexed_trifan_vb );
	
	lua_register( "pointlistvb",		st_pointlist_vb );
	lua_register( "linelistvb",			st_linelist_vb );
	lua_register( "linestripvb",		st_linestrip_vb );
	lua_register( "trilistvb",			st_trilist_vb );
	lua_register( "tristripvb",			st_tristrip_vb );
	lua_register( "trifanvb",			st_trifan_vb );
}

//


// EOF

