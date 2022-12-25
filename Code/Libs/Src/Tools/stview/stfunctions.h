//
// stfunctions.h
//
//

#ifndef __stfunctions_h__
#define __stfunctions_h__

//

extern "C" {
#include "lua.h"
};


//

void st_init( void );
void st_cleanup( void );
void st_load_file( const char *filename );
void st_init_scene( HWND hWnd, LPDIRECT3DDEVICE7 D3DDevice );
void st_render_scene( LPDIRECT3DDEVICE7 d3d_device );

//

#endif // EOF
