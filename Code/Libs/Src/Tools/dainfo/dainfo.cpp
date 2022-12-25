// dainfo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

//

const U32 dainfo_version = 1;	// increment this when you change the format of the
								// output (either adding new items to dump or just re-formatting
								// the printing).


//

char ini_file[MAX_PATH];
char app_path[MAX_PATH];
char cur_path[MAX_PATH];

bool show_base			= false;
bool show_components	= false;
bool show_display		= false;
bool show_help			= false;
bool show_version		= false;
bool show_verbose		= false;
bool show_engine		= false;
bool show_system		= false;


DA_ERROR_HANDLER old_handler = NULL;

//

int		__cdecl dainfo_load_engine_components_handler( ErrorCode code, const C8 *fmt, ... );
int		__cdecl dainfo_load_system_components_handler( ErrorCode code, const C8 *fmt, ... );
int		__cdecl dainfo_set_ini_config_handler( ErrorCode code, const C8 *fmt, ... );
BOOL32	__cdecl dainfo_enumerate_components_cb( struct IComponentFactory *factory, const C8 *iif_name, U32 priority, void * );
BOOL			dainfo_enumerate_renderpipeline_cb( IDAComponent *device, void *user );

void			dainfo_dump_interfaces( const char *comp_name, IDAComponent *comp );

//

int main( int argc, char *argv[] )
{

	strcpy( ini_file, "dacom.ini" );

	// parse command line
	//
	for( int arg=1; arg<argc; arg++ ) {
		
		if( argv[arg][0] == '-' ) {

			char *option = &argv[arg][1];

			while( *option ) {
				
				switch( *option ) {

					// please place option characters in alphabetical order

				case 'b':	show_base = true;		break;
				case 'c':	show_components = true;	break;
				case 'd':	show_display = true;	break;
				case 'e':	show_engine = true;		break;
				case 'h':	show_help = true;		break;
				case 's':	show_system = true;		break;
				case 'v':	show_version = true;	break;
				case 'x':	show_verbose = true;	break;
				
				default:	printf( "dainfo: unrecognized option '%c'\n", *option );
				}

				option++;
			}
	
		}
		else {
			strcpy( ini_file, argv[arg] );
		}
	}

	// actually do the dumping here
	//
	
	if( show_help ) {
		printf( "dainfo [options] <ini_file_name>\n\n"
				" -b         Displays information about the base DACOM setup including \n"
				"            version, full path to DACOM.DLL, and DACOM environment \n"
				"            variable info.\n\n"
				" -c         Displays informations about components registered in the\n"
				"            [Libraries] section of the ini file in use.\n\n"
				" -d         Displays information about display device\n\n"
				" -e         Displays information about the engine container\n\n"
				" -h         Displays usage information\n\n"
				" -s         Displays information about the system container\n\n"
				" -v         Displays the version of the dainfo application. (NOT! DACOM)\n\n"
				" -x         Turn on verbose information display.\n\n" 
			   );

		exit( 0 );
	}

	if( show_version ) {
		printf( "dainfo: version: %d\n", dainfo_version );
		exit( 0 );
	}

	char *cc;

	GetModuleFileName( NULL, app_path, sizeof(app_path) );
	
	if( (cc = strrchr( app_path, '\\' )) != NULL ) {
		*cc = 0;
	}
	
	GetCurrentDirectory( sizeof(cur_path), cur_path );

	printf( "dainfo: using '%s' as app directory\n", app_path );
	printf( "dainfo: using '%s' as current directory\n", cur_path );
	printf( "dainfo: compile-time version: %d.%d.%d\n", LIB_MAJOR, LIB_MINOR, LIB_BUILD );

	ICOManager *DACOM = DACOM_Acquire();;

	if( show_base ) {

		U32 m=23, n=42, b=0;
		char dacom_env[10*MAX_PATH];
		char path[MAX_PATH];
		
		printf( "\n..................... DACOM Base Information .....................\n");

		if( FAILED( DACOM_GetVersion( &m, &n, &b ) ) ) {
			printf( "error: unable to get DACOM run-time version: %d.%d.%d\n", m, n, b );
		}
		else {
			printf( "DACOM run-time version: %d.%d.%d\n", m, n, b );
		}

		GetModuleFileName( GetModuleHandle( "DACOM.DLL" ), path, sizeof(path) );

		printf( "DACOM dll path: '%s'\n", path );

		if( GetEnvironmentVariable( "DACOM", dacom_env, sizeof(dacom_env) ) == 0 ) {
			dacom_env[0] = 0;
			printf( "DACOM environment variable is not set\n" );
		}
		else {
			printf( "DACOM env path: '%s'\n", dacom_env );

			if( show_verbose ) {
				// go through the directories in the DACOM path 
				// and make sure they are valid

				char search_path[10*MAX_PATH];
				char *endptr, *startptr;
				
				startptr = strcpy( search_path, dacom_env );

				do {
			
					if( (endptr = strchr( startptr, ';' )) != NULL )	{
						// get end of first path
						*endptr++ = 0;
					}
					
					if( !SetCurrentDirectory( startptr ) ) {
						printf( "      env path element '%s' does not exist\n", startptr );
					}
					else {
						SetCurrentDirectory( cur_path );
						printf( "      env path element '%s' exists\n", startptr );
					}

					startptr = endptr;
				} 
				while( startptr != NULL );
			}
		}

		dainfo_dump_interfaces( "DACOM", DACOM );
	}

	printf( "\n.................... SetINIConfig Information ....................\n");

	// set ini file 
	//
	printf( "using '%s' as ini file\n", ini_file );

	old_handler = FDUMP ;
	
	FDUMP = dainfo_set_ini_config_handler;

	if( DACOM->SetINIConfig( ini_file ) != GR_OK ) {
		printf( "error: unable to set ini file, exiting\n" );
		exit( 0 );
	}

	FDUMP = old_handler;


	if( show_components ) {

		printf( "\n...................... Component Information .....................\n");

		DACOM->EnumerateComponents( NULL, dainfo_enumerate_components_cb, NULL );
	}

	if( show_display ) {

		printf( "\n....................... Display Information ......................\n");

		U32 count;

		if( FAILED( EnumerateRenderPipelineDevices( RPUL_DIRECT3D, dainfo_enumerate_renderpipeline_cb, NULL, &count ) ) ) {
			printf( "error: renderpipeline device enumeration failed\n" );
		}

		printf( "%d renderpipeline device(s) found\n", count );
	}

	
	if( show_system ) {

		printf( "\n....................... System Information .......................\n");

		FDUMP = dainfo_load_system_components_handler;

		bool loaded = false;
		ISystemContainer	*System	= NULL;
		AGGDESC adesc = "ISystemContainer";

		if( FAILED( DACOM->CreateInstance( &adesc, (void **) &System ) ) ) {
			printf( "error: createinstance failed, no system container available?\n" );
		}
		else {
			
			if( FAILED( System->LoadSystemComponents() ) ) {
				printf( "error: loadsystemcomponents failed\n" );
			}
			else {
				loaded = true;
				dainfo_dump_interfaces( "System", System );
			}	
		}

		printf( "System container %s correctly\n", loaded? "loaded" : "did not load" );


		FDUMP = old_handler;
	}

	if( show_engine ) {

		printf( "\n....................... Engine Information .......................\n");

		FDUMP = dainfo_load_engine_components_handler;

		ISystemContainer	*System	= NULL;
		AGGDESC adesc = "ISystemContainer";

		if( FAILED( DACOM->CreateInstance( &adesc, (void **) &System ) ) ) {
			printf( "error: createinstance failed, no system container available?\n" );
		}
		else {
			if( FAILED( System->LoadSystemComponents() ) ) {
				printf( "error: loadsystemcomponents failed\n" );
			}
		}


		bool loaded = false;
		IEngine *Engine;
		DACOMDESC desc = "IEngine";
		
		if( FAILED( DACOM->CreateInstance( &desc, (void**)&Engine ) ) ) {
			printf( "error: createinstance failed, no engine container available?\n" );
			return 0;
		}
		else {
			
			if( FAILED( Engine->load_engine_components( System ) ) ) {
				printf( "error: load_engine_components failed\n" );
			}
			else {
				loaded = true;
				dainfo_dump_interfaces( "Engine", Engine );
			}
		}

		printf( "Engine container %s correctly\n", loaded? "loaded" : "did not load" );


		FDUMP = old_handler;
	}
	

	return 0;
}

//

BOOL dainfo_enumerate_renderpipeline_cb( IDAComponent *device, void *user )
{
	RPDEVICEINFO rpdi;
	IRenderPipeline *IRP;

	if( SUCCEEDED( device->QueryInterface( IID_IRenderPipeline, (void**)&IRP ) ) ) {

		if( FAILED( IRP->get_device_info( &rpdi ) ) ) {
			printf( "error: renderpipeline device failed get_device_info() query\n" );
		}

		printf( "device: %s [%s]", rpdi.device_id_persist, rpdi.device_description );

		if( show_verbose ) {
			printf( " [%s] [%s]", rpdi.device_class, rpdi.device_type );
		}
		
		printf( "\n" );
	}

	return TRUE;
}

//

BOOL32 __cdecl dainfo_enumerate_components_cb( struct IComponentFactory *factory, const C8 *iif_name, U32 priority, void * )
{
	char prio[100+1];

	switch( priority ) {
	
	case DACOM_HIGH_PRIORITY:	strcpy( prio, "high" );	break;
	case DACOM_NORMAL_PRIORITY:	strcpy( prio, "normal" );	break;
	case DACOM_LOW_PRIORITY:	strcpy( prio, "low" );	break;
	
	default:	
		if( priority > DACOM_HIGH_PRIORITY ) {
			sprintf( prio, "high + %d", priority - DACOM_HIGH_PRIORITY );
		}
		else if( priority > DACOM_NORMAL_PRIORITY ) {
			sprintf( prio, "normal + %d", priority - DACOM_NORMAL_PRIORITY );
		}
		else if( priority > DACOM_LOW_PRIORITY ) {
			sprintf( prio, "low + %d", priority - DACOM_LOW_PRIORITY );
		}
		else {
			sprintf( prio, "%d", priority );
		}
		break;
	}

	printf( "component: % 30.30s (%s)\n", iif_name, prio );
	return TRUE;
}

//

int __cdecl dainfo_load_system_components_handler( ErrorCode code, const C8 *fmt, ... )
{
	// Report the error
	// WARNING: This uses a fixed size buffer.
	static bool bNoMoreWarnings;
	char buffer[4096];
	va_list args;
	va_start (args, fmt);
	wvsprintf (buffer, fmt, args);
	va_end (args);
	OutputDebugString (buffer);

	// NOTE: Newlines are already added to trace severity.
	if (code.severity < SEV_TRACE_1)
	{
		OutputDebugString ("\n");
	}

	// Kill the program in all SEV_FATAL or SEV_ERROR dumps.

	if (code.severity == SEV_FATAL || code.severity == SEV_ERROR || (bNoMoreWarnings==0 && code.severity == SEV_WARNING))
	{
		switch (MessageBox(0, buffer, "DACOM::STANDARD_DUMP", MB_ABORTRETRYIGNORE|MB_ICONSTOP|MB_TOPMOST))
		{
		case IDABORT:
			PostQuitMessage(-1);
			abort();
			break;

		case IDRETRY:
			if (code.kind == ERR_ASSERT)
				__asm int 3
			return 1;

		case IDIGNORE:
			if (code.severity == SEV_WARNING)
				bNoMoreWarnings = true;
			break;

		default:
			break;
		}

	}

	// This code tracks what dlls DACOM is trying to load and prints
	// whether the load succeeded.
	//
	char *c, *p;
	char comp_name[MAX_PATH], ret[64];

	if( (c = strstr( buffer, "LoadSystemComponents: Loading" )) != NULL ) {

		p = strchr( c, '\'' );
		ASSERT( p );
		strcpy( comp_name, &p[1] );
		p = strchr( comp_name, '\'' );
		ASSERT( p );
		*p = 0;

		p = strrchr( c, '[' );
		ASSERT( p );
		strcpy( ret, &p[1] );
		p = strrchr( ret, ']' );
		ASSERT( p );
		*p = 0;

		printf( "system: %s [%s]\n", comp_name, ret );
	}

	return 0;
}

//

//

int __cdecl dainfo_load_engine_components_handler( ErrorCode code, const C8 *fmt, ... )
{
	// Report the error
	// WARNING: This uses a fixed size buffer.
	static bool bNoMoreWarnings;
	char buffer[4096];
	va_list args;
	va_start (args, fmt);
	wvsprintf (buffer, fmt, args);
	va_end (args);
	OutputDebugString (buffer);

	// NOTE: Newlines are already added to trace severity.
	if (code.severity < SEV_TRACE_1)
	{
		OutputDebugString ("\n");
	}

	// Kill the program in all SEV_FATAL or SEV_ERROR dumps.

	if (code.severity == SEV_FATAL || code.severity == SEV_ERROR || (bNoMoreWarnings==0 && code.severity == SEV_WARNING))
	{
		switch (MessageBox(0, buffer, "DACOM::STANDARD_DUMP", MB_ABORTRETRYIGNORE|MB_ICONSTOP|MB_TOPMOST))
		{
		case IDABORT:
			PostQuitMessage(-1);
			abort();
			break;

		case IDRETRY:
			if (code.kind == ERR_ASSERT)
				__asm int 3
			return 1;

		case IDIGNORE:
			if (code.severity == SEV_WARNING)
				bNoMoreWarnings = true;
			break;

		default:
			break;
		}

	}

	// This code tracks what dlls DACOM is trying to load and prints
	// whether the load succeeded.
	//
	char *c, *p;
	char comp_name[MAX_PATH], ret[64];

	if( (c = strstr( buffer, "load_engine_components: aggregation of" )) != NULL ) {

		p = strchr( c, '\'' );
		ASSERT( p );
		strcpy( comp_name, &p[1] );
		p = strchr( comp_name, '\'' );
		ASSERT( p );
		*p = 0;

		p = strrchr( c, '[' );
		ASSERT( p );
		strcpy( ret, &p[1] );
		p = strrchr( ret, ']' );
		ASSERT( p );
		*p = 0;

		printf( "engine: %s [%s]\n", comp_name, ret );
	}

	return 0;
}

//

int __cdecl dainfo_set_ini_config_handler( ErrorCode code, const C8 *fmt, ... )
{
	// Report the error
	// WARNING: This uses a fixed size buffer.
	static bool bNoMoreWarnings;
	char buffer[4096];
	va_list args;
	va_start (args, fmt);
	wvsprintf (buffer, fmt, args);
	va_end (args);
	OutputDebugString (buffer);

	// NOTE: Newlines are already added to trace severity.
	if (code.severity < SEV_TRACE_1)
	{
		OutputDebugString ("\n");
	}

	// Kill the program in all SEV_FATAL or SEV_ERROR dumps.

	if (code.severity == SEV_FATAL || code.severity == SEV_ERROR || (bNoMoreWarnings==0 && code.severity == SEV_WARNING))
	{
		switch (MessageBox(0, buffer, "DACOM::STANDARD_DUMP", MB_ABORTRETRYIGNORE|MB_ICONSTOP|MB_TOPMOST))
		{
		case IDABORT:
			PostQuitMessage(-1);
			abort();
			break;

		case IDRETRY:
			if (code.kind == ERR_ASSERT)
				__asm int 3
			return 1;

		case IDIGNORE:
			if (code.severity == SEV_WARNING)
				bNoMoreWarnings = true;
			break;

		default:
			break;
		}

	}

	// This code tracks what dlls DACOM is trying to load and prints
	// whether the load succeeded.
	//
	char *c, *p;
	static char adding_dll_name[MAX_PATH];
	char dll_name[MAX_PATH], version[100];
	static bool adding_dll = false;

	strlwr( buffer );

	if( (c = strstr( buffer, "addlibrary:" )) != NULL ) {

		p = strchr( c, '\'' );
		ASSERT( p );
		strcpy( dll_name, &p[1] );
		p = strchr( dll_name, '\'' );
		ASSERT( p );
		*p = 0;

		p = strchr( c, '[' );
		ASSERT( p );
		strcpy( version, &p[1] );
		p = strchr( version, ']' );
		ASSERT( p );
		*p = 0;

		if( (p = strrchr( dll_name, '\\' )) == NULL ) {
			p = dll_name;
		}
		else {
			p++;
		}

		if( strstr( p, adding_dll_name ) ) {
			printf( "library: %s [%s]", dll_name, version );

			if( strcmp( version, "0.0.0" ) == 0 ) {
				printf( " [unofficial?]" );
			}

			printf( "\n", dll_name, version );
		}

		adding_dll = false;
	}
	else if( (c = strstr( buffer, "initialize: adding " )) != NULL ) {
		
		if( adding_dll ) {
			printf( "library: '%s' was not found\n", adding_dll_name );
			adding_dll = false;
		}

		p = strchr( c, '\'' );
		ASSERT( p );
		strcpy( adding_dll_name, &p[1] );
		p = strchr( adding_dll_name, '\'' );
		ASSERT( p );
		*p = 0;

		adding_dll = true;
	}

	return 0;
}

//

inline dainfo_dump_interface( IDAComponent *comp, const char *space, const char *filename, const char *iid )
{
	IDAComponent *iif;

	if( SUCCEEDED( comp->QueryInterface( iid, (void**)&iif ) ) ) {
		printf( "%s +--o %s (%s)\n", space, iid, filename );
		DACOM_RELEASE( iif );
	}
}

//

void dainfo_dump_interfaces( const char *comp_name, IDAComponent *comp )
{
	printf( "%s +--o IDAComponent\n", comp_name );

	char spaces[MAX_PATH];
	U32 s, num_spaces = strlen( comp_name );

	for( s=0; s<num_spaces; s++ ) {
		spaces[s] = ' ';
	}
	spaces[s] = 0;
	
	dainfo_dump_interface( comp, spaces, "3dmath.h",				IID_I3DMathEngine );
	dainfo_dump_interface( comp, spaces, "DACOM.H",					IID_IDAComponent ) ;
	dainfo_dump_interface( comp, spaces, "DACOM.H",					IID_IComponentFactory ) ;
	dainfo_dump_interface( comp, spaces, "DACOM.H",					IID_IAggregateComponent) ;
	dainfo_dump_interface( comp, spaces, "ENGINE.H",				IID_IEngine ) ;
	dainfo_dump_interface( comp, spaces, "FILESYS.H",				IID_IFileSystem ) ;
	dainfo_dump_interface( comp, spaces, "FontImage.h",				IID_IFontFactory ) ;
	dainfo_dump_interface( comp, spaces, "FontImage.h",				IID_IFontImage ) ;
	dainfo_dump_interface( comp, spaces, "IAnim.h",					IID_IAnimation ) ;
	dainfo_dump_interface( comp, spaces, "ICamera.h",				IID_ICamera ) ;
	dainfo_dump_interface( comp, spaces, "IChannel2.h",				IID_IChannel2 ) ;
	dainfo_dump_interface( comp, spaces, "IDDBackDoor.h",			IID_IDDBackDoor) ;
	dainfo_dump_interface( comp, spaces, "IDeformable.h",			IID_IDeformable ) ;
	dainfo_dump_interface( comp, spaces, "IDocClient.h",			IID_IDocumentClient) ;
	dainfo_dump_interface( comp, spaces, "IExtentContainer.h",		IID_IExtentContainer  ) ;
	dainfo_dump_interface( comp, spaces, "IGammaControl.h",			IID_IGammaControl) ;
	dainfo_dump_interface( comp, spaces, "IHardPoint.h",			IID_IHardpoint ) ;
	dainfo_dump_interface( comp, spaces, "ILight.h",				IID_ILight ) ;
	dainfo_dump_interface( comp, spaces, "IMaterial.h",				IID_IMaterial ) ;
	dainfo_dump_interface( comp, spaces, "IMaterialLibrary.h",		IID_IMaterialLibrary ) ;
	dainfo_dump_interface( comp, spaces, "IMaterialProperties.h",	IID_IMaterialProperties ) ;
	dainfo_dump_interface( comp, spaces, "IMesh.h",					IID_IMesh ) ;
	dainfo_dump_interface( comp, spaces, "IParticleSystem.h",		IID_IParticleSystem  ) ;
	dainfo_dump_interface( comp, spaces, "IProfileParser.h",		IID_IProfileParser ) ;
	dainfo_dump_interface( comp, spaces, "IProperties.h",			IID_IProperty ) ;
	dainfo_dump_interface( comp, spaces, "IProperties.h",			IID_ISetProperty ) ;
	dainfo_dump_interface( comp, spaces, "IProperties.h",			IID_IProperties ) ;
	dainfo_dump_interface( comp, spaces, "IRenderComponent.h",		IID_IRenderComponent ) ;
	dainfo_dump_interface( comp, spaces, "IRenderPrimitive.h",		IID_IRenderPrimitive ) ;
	dainfo_dump_interface( comp, spaces, "IRigidBody.h",			IID_IRigidBody  ) ;
	dainfo_dump_interface( comp, spaces, "IRigidBodyState.h",		IID_IRigidBodyState  ) ;
	dainfo_dump_interface( comp, spaces, "ISound.h",				IID_ISoundSource ) ;
	dainfo_dump_interface( comp, spaces, "ISoundListener.h",		IID_ISoundListener ) ;
	dainfo_dump_interface( comp, spaces, "ISoundManager.h",			IID_ISoundArchetype ) ;
	dainfo_dump_interface( comp, spaces, "ISoundManager.h",			IID_ISoundManager ) ;
	dainfo_dump_interface( comp, spaces, "IStateMaterial.h",		IID_IStateMaterial ) ;
	dainfo_dump_interface( comp, spaces, "ITextureLibrary.h",		IID_ITextureLibrary ) ;
	dainfo_dump_interface( comp, spaces, "IUTFWriter.h",			IID_IUTFWriter ) ;
	dainfo_dump_interface( comp, spaces, "IVertexBufferManager.h",	IID_IVertexBufferManager ) ;
	dainfo_dump_interface( comp, spaces, "IVideoStreamControl.h",	IID_IVideoStreamControl) ;
	dainfo_dump_interface( comp, spaces, "SearchPath.h",			IID_ISearchPath ) ;
	dainfo_dump_interface( comp, spaces, "Streamer.h",				IID_IStreamer ) ;
	dainfo_dump_interface( comp, spaces, "WindowManager.h",			IID_IWindowManager ) ;
	dainfo_dump_interface( comp, spaces, "bigimage.h",				IID_IBigImage ) ;
	dainfo_dump_interface( comp, spaces, "collision.h",				IID_ICollision) ;
	dainfo_dump_interface( comp, spaces, "engcomp.h",				IID_IEngineComponent ) ;
	dainfo_dump_interface( comp, spaces, "ichannel.h",				IID_IChannel ) ;
	dainfo_dump_interface( comp, spaces, "iimagesource.h",			IID_IImageSource ) ;
	dainfo_dump_interface( comp, spaces, "lightman.h",				IID_ILightManager ) ;
	dainfo_dump_interface( comp, spaces, "physics.h",				IID_IPhysics ) ;
	dainfo_dump_interface( comp, spaces, "renderer.h",				IID_IRenderer ) ;
	dainfo_dump_interface( comp, spaces, "rendpipeline.h",			IID_IRenderPipeline );
}

//