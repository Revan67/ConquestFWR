/*
**
** Originally based on luac.c from the lua 3.2 distribution.
**
** $Id: luac.c,v 1.17 1999/07/02 19:34:26 lhf Exp $
** lua compiler (saves bytecodes to files; also list binary files)
** See Copyright Notice in lua.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//

#include "luac/luac.h"
#include "lparser.h"
#include "lstate.h"
#include "lzio.h"

//
//

int st_compile( char *filename, char **out_buffer )
{
	static int debugging=0;			/* emit debug information? */
	static int dumping=1;			/* dump bytecodes? */
	static int optimizing=1;		/* optimize? */
	static int native=1;			/* save numbers in native format? */

	char *out_filename;
	FILE *in, *out;
	ZIO z;
	TProtoFunc* Main;
	char source[255+2];			/* +2 for '@' and '\0' */

	if( (in = fopen( filename, "r" )) == NULL ) {
		// unable to open input file
		return -2;
	}

	out_filename = tmpnam( NULL );

	if( (out = fopen( out_filename, "wb" )) == NULL ) {
		// unable to open output file
		return -1;
	}

	luaL_filesource( source, filename, sizeof(source) );
	
	zFopen( &z, in, source );
	
	if( optimizing ) L->debug=0;
	if( debugging )  L->debug=1;
	
	Main = luaY_parser( &z );
	
	if (optimizing) luaU_optchunk( Main );
	if (dumping)	luaU_dumpchunk( Main, out, native );
	
	fclose( in );
	fclose( out );

	return 0;
}

// EOF
