//***************************************************************************************
// Particle Editor
//  bramage november 97
//***************************************************************************************

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>			// sprintf()
#include <stdlib.h>			// exit()
#include <Zmouse.h>
#include <float.h>

#include "dacom.h"
#include "fdump.h"
#include "tempstr.h"
#include "TSmartPointer.h"
#include "PersistMisc.h"
#include "System.h"
#include "RendPipeline.h"
#include "IRenderPrimitive.h"
#include "FileSys_Utility.h"
#include "Engine.h"
#include "Renderer.h"
#include "IProfileParser_Utility.h"
#include "IParticleSystem.h"
#include "RPUL.h"

#include "ParticleEditor.h"
#include "Camera.h"
#include "resource.h"

#pragma warning(disable:4244)	// conversion double to float
#pragma warning(disable:4305)	// truncation double to float
#pragma warning(disable:4800)	// forcing value to bool

//

bool GetColorSelection( HWND hParent, LPCTSTR DlgTemplate, const char *Title, float *out_color );
bool GetTextureSelection( HWND hParent, LPCTSTR DlgTemplate, const char *Title, ITextureLibrary *texturelibrary, IRenderPipeline *renderpipe, char *out_texture, U32 max_texture_len );
HRESULT GetProfileInfo( char *out_app_name, U32 max_app_name_len, char *out_ini_pathname, U32 max_ini_pathname_len );

HRESULT CopyPath( const char *source_path, IFileSystem *dest_ifs );

//

//

HWND g_hTooltipControl = 0;
HWND g_hTooltipParent = 0;
HHOOK g_hTooltipHook = 0;

RPFont3D font;

//

template<typename T>
inline T Tblend( const T & c0, const T & c1, float alpha )
{
	return (1.0f-alpha) * c0 + (alpha) * c1;	
}

//

inline void TrackBar_SetRangeMin( HWND hwnd, U32 min, U32 redraw )
{
	SendMessage( hwnd, TBM_SETRANGEMIN, (WPARAM)redraw, (LPARAM)min );
}

//

inline void TrackBar_SetRangeMax( HWND hwnd, U32 max, U32 redraw )
{
	SendMessage( hwnd, TBM_SETRANGEMAX, (WPARAM)redraw, (LPARAM)max );
}

//

inline U32 TrackBar_GetPos( HWND hwnd )
{
	return (U32)SendMessage( hwnd, TBM_GETPOS, 0, 0 );
}

//

inline void TrackBar_SetPos( HWND hwnd, U32 pos, U32 redraw )
{
	SendMessage( hwnd, TBM_SETPOS, (WPARAM)redraw, (LPARAM)pos );
}

//

inline void TrackBar_SetTick( HWND hwnd, U32 pos )
{
	SendMessage( hwnd, TBM_SETTIC, (WPARAM)0, (LPARAM)pos );
}

//

inline void TrackBar_ClearTicks( HWND hwnd, U32 redraw  )
{
	SendMessage( hwnd, TBM_CLEARTICS, (WPARAM)redraw, (LPARAM)0 );
}

//

ParticleEditor theParticleEditor;

//

void exit_handler( void )
{
	theParticleEditor.cleanup();
}

//

#define	BOUNDS_EDITFIELD_INDEX	20	//should match IDC_BOUNDING_RADIUS below

//

EditField PSPFields[] = 
{
	{ IDC_F_INHERIT_TRANSFORM,			EF_T_BIT,	PSP(psp_f_flags), PSP_F_RELATIVE_TRANSFORM, 0 },
	{ IDC_F_INHERIT_VELOCITY,			EF_T_BIT,	PSP(psp_f_flags), PSP_F_RELATIVE_VELOCITY, 0 },
	{ IDC_F_IGNORE_ORIENTATION,			EF_T_BIT,	PSP(psp_f_flags), PSP_F_IGNORE_ORIENTATION, 0 },
	{ IDC_F_RENDER_PARTICLE_LIFETIME,	EF_T_BIT,	PSP(psp_f_flags), PSP_F_RENDER_PARTICLE_LIFE, 0 },
	{ IDC_F_RENDER_DITHER,				EF_T_BIT,	PSP(psp_f_flags), PSP_F_RENDER_DITHER, 0 },
	{ IDC_F_RENDER_FOG,					EF_T_BIT,	PSP(psp_f_flags), PSP_F_RENDER_FOG, 0 },
	{ IDC_INITIAL_PARTICLE_COUNT,		EF_T_INT,	PSP(initial_particle_count), 0, -1 },  
	{ IDC_MAX_PARTICLE_COUNT,			EF_T_INT,	PSP(max_particle_count), 0, -1 },  
	{ IDC_EMITTER_LIFETIME,				EF_T_REAL,  PSP(lifetime), 0, -1 },          
	{ IDC_EMITTER_FREQUENCY,			EF_T_REAL,  PSP(frequency), 0, -1 },
	{ IDC_EMITTER_DIRECTION_X,			EF_T_REAL,	PSP(emitter_direction.x), -1, -1 },
	{ IDC_EMITTER_DIRECTION_Y,			EF_T_REAL,	PSP(emitter_direction.y), -1, -1 },
	{ IDC_EMITTER_DIRECTION_Z,			EF_T_REAL,	PSP(emitter_direction.z), -1, -1 },
	{ IDC_EMITTER_SIZE,					EF_T_REAL,	PSP(emitter_nozzle_size), 0, -1 },              
	{ IDC_EMITTER_DAMP_X,				EF_T_REAL,  PSP(emitter_nozzle_damp.x), -1, -1 },
	{ IDC_EMITTER_DAMP_Y,				EF_T_REAL,	PSP(emitter_nozzle_damp.y), -1, -1 },
	{ IDC_EMITTER_DAMP_Z,				EF_T_REAL,	PSP(emitter_nozzle_damp.z), -1, -1 },
	{ IDC_GRAVITY_X,					EF_T_REAL,  PSP(gravity.x), -1, -1 },
	{ IDC_GRAVITY_Y,					EF_T_REAL,	PSP(gravity.y), -1, -1 },
	{ IDC_GRAVITY_Z,					EF_T_REAL,	PSP(gravity.z), -1, -1 },
	{ IDC_BOUNDING_RADIUS,				EF_T_REAL,	PSP(bounding_sphere_radius), 0, -1 },             
	{ IDC_PARTICLE_LIFETIME,			EF_T_REAL,	PSP(particle_lifetime), 0, -1 },                        
	{ IDC_PARTICLE_POSITION_RANDOM,		EF_T_REAL,	PSP(particle_position_randomizer), -1, -1 },
	{ IDC_PARTICLE_VELOCITY,			EF_T_REAL,	PSP(particle_velocity), -1, -1 },
	{ IDC_PARTICLE_VELOCITY_RANDOM,		EF_T_REAL,	PSP(particle_velocity_randomizer), -1, -1 },
	{ IDC_PARTICLE_TWIST_VELOCITY,		EF_T_REAL,	PSP(particle_twist_velocity), -1, -1 },
	{ IDC_PARTICLE_SIZE,				EF_T_REAL,	PSP(particle_size), 0, -1 },                                       
	{ IDC_PARTICLE_SIZE_VELOCITY,		EF_T_REAL,	PSP(particle_size_velocity), -1, -1 },                              
	{ IDC_PARTICLE_SRC_BLEND,			EF_T_INT,	PSP(src_blend), 1, 12 },
	{ IDC_PARTICLE_DST_BLEND,			EF_T_INT,	PSP(dst_blend), 1, 12 },
	{ IDC_PARTICLE_TEXTURE,				EF_T_CSTR,  PSP(texture_name), 0, PSP_TEXTURE_NAME_LEN },   
	{ IDC_PARTICLE_TEXTURE_FPS,			EF_T_REAL,	PSP(texture_fps), -1, -1 },    
	
	// leave me last
	{ 0, EF_T_NULL, 0, 0, 0 }
};

//

EditField PEFields[] = 
{
	{ IDC_EVENT_VELOCITY,				EF_T_REAL,	PE(current_system_speed), -1, -1 },
	{ IDC_GRID_SCALE,					EF_T_REAL,	PE(current_grid_scale), 0, 1000 },
	{ IDC_MIN_RENDER_DIST,				EF_T_REAL,	PE(current_render_min), -1, -1 },    
	{ IDC_MAX_RENDER_DIST,				EF_T_REAL,	PE(current_render_max), -1, -1 },    

		// leave me last
	{ 0, EF_T_NULL, 0, 0, 0 }
};

// ........................................................................
//
// ParticleSystem
//
// ........................................................................


ParticleSystem::ParticleSystem( void )
{
	system_name[0] = 0;

	ps_f_flags = 0;

	set_export_physics( true );
	set_render_radius( false );

	arch_index = INVALID_ARCHETYPE_INDEX;
	render_min = 0;
	render_max = 0;

	active_inst_index = INVALID_INSTANCE_INDEX;

	memset( &parameters, 0, sizeof(parameters) );

	system_speed = 0.0f;
}

//

bool ParticleSystem::initialize( IEngine *engine, const char *_name, const char *_filename, ARCHETYPE_INDEX _arch_index )
{
	ASSERT( _name );

	COMPTR<IFileSystem> ifs;

	ps_f_flags &= ~(PS_F_VALID);

	strcpy( system_name, _name );

	if( arch_index != INVALID_ARCHETYPE_INDEX ) {
		engine->release_archetype( arch_index );
	}

	if( _arch_index == INVALID_ARCHETYPE_INDEX ) {

		#pragma message( "TODO: load min and max render data" )
		#pragma message( "TODO: load scale and point extent data" )

		if( _filename ) {
			if( FAILED( engine->create_file_system( _filename, ifs, NULL ) ) ) {
				return false;
			}
		}

		if( (arch_index = engine->create_archetype( system_name, ifs )) == INVALID_ARCHETYPE_INDEX ) {
			return false;
		}
	}
	else {
		if( (arch_index = engine->duplicate_archetype( _arch_index, system_name )) == INVALID_ARCHETYPE_INDEX ) {
			return false;
		}
	}

	//will need to get the render min and max dist when
	//it's possible to get at em
	get_parameters( engine );

	ps_f_flags |= PS_F_VALID;

	return true;
}

//

void ParticleSystem::cleanup( IEngine *engine )
{
	deactivate( engine );

	if( arch_index != INVALID_ARCHETYPE_INDEX ) {
		engine->release_archetype( arch_index );
		arch_index = INVALID_ARCHETYPE_INDEX;
	}

	ps_f_flags &= ~(PS_F_VALID);
}

//

bool ParticleSystem::set_active_enable( bool on_off )
{
	if( on_off == true ) {
		ps_f_flags |= PS_F_ACTIVE_ENABLE;
	}
	else {
		ps_f_flags &= ~(PS_F_ACTIVE_ENABLE);
	}

	return true;
}

//

bool ParticleSystem::is_active_enabled( void )
{
	return (ps_f_flags & PS_F_ACTIVE_ENABLE);
}

//

bool ParticleSystem::activate( IEngine *engine )
{
	if( !(ps_f_flags & PS_F_VALID) ) {
		return false;
	}

	deactivate( engine );

	if( (active_inst_index = engine->create_instance2( arch_index, NULL )) == INVALID_INSTANCE_INDEX ) {
		return false;
	}

	return true;
}

//

bool ParticleSystem::deactivate( IEngine *engine )
{
	if( active_inst_index != INVALID_INSTANCE_INDEX ) {
		engine->destroy_instance( active_inst_index );
		active_inst_index = INVALID_INSTANCE_INDEX;
	}	

	return true;
}

//

bool ParticleSystem::is_active( IEngine *engine )
{
	if( active_inst_index == INVALID_INSTANCE_INDEX ) {
		return false;
	}

	COMPTR<IParticleSystem> ips;

	if( SUCCEEDED( engine->query_instance_interface( active_inst_index, IID_IParticleSystem, (IDAComponent**)&ips ) ) ) {
		return ips->is_active();
	}

	return false;
}

//

const char *ParticleSystem::get_name( IEngine * )
{
	return system_name;
}

//

const Vector ParticleSystem::get_position( IEngine *engine )
{
	if( active_inst_index ) {
		return engine->get_position( active_inst_index );
	}

	return Vector(0,0,0);
}

//

void ParticleSystem::set_render_radius( bool on_or_off )
{
	if( on_or_off == true ) {
		ps_f_flags |= PS_F_RENDER_RADIUS;
	}
	else {
		ps_f_flags &= ~(PS_F_RENDER_RADIUS);
	}
}

//

bool ParticleSystem::get_render_radius( void )
{
	return ps_f_flags & PS_F_RENDER_RADIUS;
}

//

void ParticleSystem::set_export_physics( bool on_or_off )
{
	if( on_or_off == true ) {
		ps_f_flags |= PS_F_EXPORT_PHYSICS;
	}
	else {
		ps_f_flags &= ~(PS_F_EXPORT_PHYSICS);
	}
}

//

bool ParticleSystem::get_export_physics( void )
{
	return ps_f_flags & PS_F_EXPORT_PHYSICS;
}

//

void ParticleSystem::set_max_render( SINGLE max )
{
	render_max = max;
}

//

SINGLE ParticleSystem::get_max_render( )
{
	return render_max ;
}

//

void ParticleSystem::find_size_velocity( void )
{
	if( parameters.particle_lifetime > 0.0f ) {
		parameters.particle_size_velocity = - parameters.particle_size / parameters.particle_lifetime;
	}
	else if( parameters.lifetime > 0.0f ) {
		parameters.particle_size_velocity = - parameters.particle_size / parameters.lifetime;
	}
	else {
		parameters.particle_size_velocity = - parameters.particle_size / 100000.00f;	// arbitrarily large number
	}
}

//

void ParticleSystem::get_parameters( IEngine *engine )
{
	COMPTR<IParticleSystem> ips;

	if( arch_index != INVALID_ARCHETYPE_INDEX ) {
		if( SUCCEEDED( engine->query_archetype_interface( arch_index, IID_IParticleSystem, (IDAComponent**)&ips ) ) ) {
			ips->get_parameters( &parameters );
		}
	}
}

//

void ParticleSystem::set_parameters( IEngine *engine )
{
	COMPTR<IParticleSystem> ips;

	if( arch_index != INVALID_ARCHETYPE_INDEX ) {
		if( SUCCEEDED( engine->query_archetype_interface( arch_index, IID_IParticleSystem, (IDAComponent**)&ips ) ) ) {
			ips->set_parameters( &parameters );
		}
	}

	if( active_inst_index != INVALID_INSTANCE_INDEX ) {
		if( SUCCEEDED( engine->query_instance_interface( active_inst_index, IID_IParticleSystem, (IDAComponent**)&ips ) ) ) {
			ips->set_parameters( &parameters );
		}
	}
}

//

void ParticleSystem::render( IRenderPipeline *renderpipeline, IEngine *engine, ICamera *camera, U32 show_axes )
{
	if( active_inst_index == INVALID_INSTANCE_INDEX ) {
		return;
	}

	Vector	centerpos;

	engine->get_instance_bounding_sphere(active_inst_index, 0, &parameters.bounding_sphere_radius, &centerpos);

	PrimitiveBuilder pb( renderpipeline );

	engine->render_instance( camera, active_inst_index, EN_DONT_RECURSE, 1.0f, RF_RELATIVE_LOD, NULL ); 

	renderpipeline->set_render_state(D3DRS_ALPHABLENDENABLE, FALSE);
	renderpipeline->set_render_state( D3DRS_ZENABLE, FALSE );
	renderpipeline->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
	renderpipeline->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	renderpipeline->set_texture_stage_texture( 0, 0 );

	renderpipeline->set_modelview( Transform( camera->get_inverse_transform() * engine->get_transform( active_inst_index ) ) );

	if( show_axes ) {
		// emitter coordinate space
		//		

		pb.Begin( PB_LINES );
			pb.Color3f( 1,0,0 );	pb.Vertex3f( 0,0,0 );	pb.Vertex3f( grid_scale,0,0 );
			pb.Color3f( 0,1,0 );	pb.Vertex3f( 0,0,0 );	pb.Vertex3f( 0,grid_scale,0 );
			pb.Color3f( 0,0,1 );	pb.Vertex3f( 0,0,0 );	pb.Vertex3f( 0,0,grid_scale);
		pb.End();
	}

	if( ps_f_flags & PS_F_RENDER_RADIUS ) {
		render_radius_points();
	}
}

//

void ParticleSystem::render_radius_points( void )
{
}

//

void ParticleSystem::update( IEngine *engine, float dt_s )
{
	if( active_inst_index == INVALID_INSTANCE_INDEX ) {
		return ;
	}

	static float loop_t = 0;

	if( fabs(system_speed) > 0.001f  ) {
		Vector last_pos, pos, i, j, k;

		last_pos = engine->get_position( active_inst_index );
		pos.set( grid_scale * sin( system_speed*loop_t ), grid_scale * cos( system_speed*loop_t ), grid_scale );
		i = Vector( pos - last_pos ).normalize();
		j = Vector( pos ).normalize();
		k = cross_product( i, j ).normalize();
		j = cross_product( i, k ).normalize();

		engine->set_orientation( active_inst_index, Matrix( i, j, k ) );
		engine->set_position( active_inst_index, pos );
	}

	engine->update_instance( active_inst_index, 0, dt_s );

	loop_t += dt_s;
}

//


// ........................................................................
//
// ParticleEditor
//
// ........................................................................

bool ParticleEditor::initialize( HINSTANCE hInst )
{
	char app_name[MAX_PATH];
	char ini_file[MAX_PATH];
	char default_particle_file[MAX_PATH];
	char textures_to_load[MAX_PATH];
	char buffer[MAX_PATH];
	RECT r,d,e;

	camera_initialized = false;
	initialized = false;

	last_dt = 0.001f;
	locked_framerate = 1000.0f;

	set_render_enable( false );
	set_update_enable( false );

    atexit( exit_handler );

	// setup dacom and get interfaces
	//
	if( (comanager = DACOM_Acquire()) == NULL ) {
		return false;
	}	

	if( FAILED( GetProfileInfo( app_name, MAX_PATH, ini_file, MAX_PATH ) ) ) {
		MessageBox( NULL, "Unable to find appropriate .ini file, exiting", "Error", MB_OK );
		return false;
	}

	if( FAILED( comanager->SetINIConfig( ini_file ) ) ) {
		MessageBox( NULL, "Unable to initialize DACOM", "Error", MB_OK );
		return false;
	}

	AGGDESC adesc = "ISystemContainer";
	if( FAILED( comanager->CreateInstance( &adesc, (void**)&system ) ) ) {
		MessageBox( NULL, "Unable to create system container.", "Error", MB_OK );
		return false;
	}

	system->LoadSystemComponents();

	// initialize rendering stuff
	//
	if( FAILED( system->QueryInterface( IID_IRenderPipeline, (void**) &renderpipeline ) ) ) {
		return false;
	}
	if( FAILED( renderpipeline->startup() ) ) {
		GENERAL_WARNING( "ParticleEditor: initialize: unable to startup render pipe" );
		return false;
	}

	if( FAILED( system->QueryInterface( IID_ITextureLibrary, (void**) &texturelibrary ) ) ) {
		return false;
	}

	DACOMDESC desc = "IEngine";
	if( FAILED( comanager->CreateInstance( &desc, (void**)&engine ) ) ) {
		MessageBox( NULL, "Unable to create engine container.", "Error", MB_OK );
		return E_FAIL;
	}

	engine->load_engine_components( system );

	if( FAILED( engine->QueryInterface( IID_IRenderer, (void**) &renderer ) ) ) {
		return false;
	}

	engine->update( 0 );

	// setup options
	//
	GetCurrentDirectory( sizeof(particle_directory), particle_directory );

	char *tempname;
	if( (tempname = _tempnam( NULL, "dape_" )) == NULL ) {
		MessageBox( NULL, "Cannot create temporary filename", "Error", MB_OK );
		return false;
	}
	strcpy( tmp_particle_name, tempname );
	free( tempname );

	opt_get_string( comanager, NULL, app_name, "ParticlePath", particle_directory, particle_directory, MAX_PATH );
	opt_get_string( comanager, NULL, app_name, "TexturePath", particle_directory, texture_directory, MAX_PATH );
	opt_get_string( comanager, NULL, app_name, "TexturePalette", "", textures_to_load, MAX_PATH );
	opt_get_string( comanager, NULL, app_name, "DefaultParticleFile", tmp_particle_name, default_particle_file, MAX_PATH );
	opt_get_string( comanager, NULL, app_name, "txmlib.exe", "txmlib.exe", buffer, MAX_PATH );
	opt_get_vector( comanager, NULL, app_name, "BackgroundColor", Vector(0.0f,0.0f,0.4f), background_color );
	opt_get_float( comanager, NULL, app_name, "GridScale", PE_DEFAULT_GRID_SCALE, &default_grid_scale );
	opt_get_vector( comanager, NULL, app_name, "GridNormal", PE_DEFAULT_GRID_NORMAL, grid_normal );
	opt_get_float( comanager, NULL, app_name, "MaxFramerate", 1000.0f, &locked_framerate );
	opt_get_u32( comanager, NULL, app_name, "UsePointExtent", 0, &use_point_extent );
	opt_get_u32( comanager, NULL, app_name, "UseToolTips", 1, &use_tooltips );
	opt_get_u32( comanager, NULL, app_name, "UseAxes", 0, &use_axes );

	if( SearchPath( NULL, buffer, NULL, MAX_PATH, txmlib_exe, NULL ) == 0 ) {
		MessageBox( NULL, "The tool you've specified in the [ParticleEditor] profile to be used to \n"
						  "convert bitmaps to textures has not been found.\n"
						  "\n"
						  "If you click OK, you will not be able to import bitmaps into the particle editor.\n"
						  "If you click Cancel, the editor will exit so you may correct the problem.", 
						  
						  "Error", 
						  MB_OKCANCEL );
	}


	grid_normal.normalize();

	font.SetRenderPipeline( renderpipeline );

	// render window
	//
	if( (hRender = create_render_window()) == NULL ) {
		return false;
	}
	if( !setup_render_buffers() ) {
		return false;
	}
	
    // initialize particle systems
	//
	char name[MAX_PATH];

	selected_system = 0;
	current_grid_scale = default_grid_scale;

	current_system_speed = 0.0f;

	current_render_min	=0.0f;
	current_render_max	=32768.0f;

	if( (default_system = load_default_system( default_particle_file )) == INVALID_ARCHETYPE_INDEX ) {
		return false;
	}

	untitled_counter = 0;

    for( int i=0; i<PE_MAX_NUM_SYSTEMS; i++ ) {
		sprintf( name, "%s%d", PE_UNTITLED_PREFIX, untitled_counter++ );
		systems[i].initialize( engine, name, NULL, default_system );
		systems[i].grid_scale = default_grid_scale;
    }

	// initialize editor window
	//
	if( (hEditor = create_editor_window()) == NULL ) {
		return false;
	}

	// resize render window to be the same size as the editor window
	// and place the two windows centered side by side in the desktop
	//
	GetWindowRect( GetDesktopWindow(), &d );
	GetWindowRect( hEditor, &r );

	const long channel = 5;
	long w = r.right - r.left;
	long h = r.bottom - r.top;

	SetRect( &e, d.right/2 - channel/2 - w,	d.bottom/2 - h/2, w, h );
	SetWindowPos( hEditor, NULL, e.left, e.top, e.right, e.bottom, 0 );

	SetRect( &r, d.right/2 + channel/2,		d.bottom/2 - h/2, w, h );	
	SetWindowPos( hRender, NULL, r.left, r.top, r.right, r.bottom, 0 );

	if( !setup_render_buffers() ) {
		return false;
	}

	InvalidateRect( hEditor, NULL, FALSE );
	UpdateWindow( hEditor );

	// initialize available textures
	//
    if( !load_default_textures( texture_directory, textures_to_load ) ) {
		GENERAL_WARNING( "ParticleEditor: initialize: unable to find any default textures" );
	}

	initialized = true;

	set_render_enable( true );
	set_update_enable( true );

	return true;
}

//

void ParticleEditor::cleanup( void )
{
	initialized = false;

	DeleteFile( tmp_particle_name );

	set_update_enable( false );
	set_render_enable( false );

	font.SetRenderPipeline( NULL );

    for( int i=0; i<PE_MAX_NUM_SYSTEMS; i++ ) {
		systems[i].cleanup( engine );
    }

	RELEASE( texturelibrary );

	if( hEditor ) {
		DestroyWindow( hEditor );
		hEditor = 0;
	}

	if( hRender ) {
		DestroyWindow( hRender );
		hRender = 0;
	}

	if( engine ) {
		engine->release_archetype( default_system );
	}

	RELEASE( renderpipeline );
	RELEASE( renderer );
	RELEASE( system );
	RELEASE( engine );

	if( comanager ) {
		comanager->ShutDown();
		comanager = NULL;
	}

	return ;
}

//

bool ParticleEditor::message_pump( void )
{
	MSG msg;

	if( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
		if( !GetMessage( &msg, NULL, 0, 0 ) ) {
			return true; // exit app
		}
		
		if( !IsDialogMessage( hEditor, &msg ) ) {
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}
	else {

		LARGE_INTEGER freq, tick;
		static LARGE_INTEGER last_tick = { 0,0 };
		float ticks_per_second;

		// get update time delta
		//
		QueryPerformanceFrequency( &freq );
		ticks_per_second = float(freq.u.LowPart);

		last_dt = 0.0f;
		do {
			QueryPerformanceCounter( &tick );
			if( last_tick.u.LowPart == 0 ) {
				last_tick = tick;
			}
			last_dt += (tick.u.LowPart-last_tick.u.LowPart)/ticks_per_second ;
		}
		while( last_dt < 0.9f/locked_framerate );

		update();
		render();

		EditField_UpdateControlFromStruct(hEditor, &PSPFields[BOUNDS_EDITFIELD_INDEX], (void *)&systems[selected_system].parameters);

		last_tick = tick;
	}
	
	return false;
}

//

void ParticleEditor::addref_texture_palette( void )
{
	// This is a hack to make sure no textures in the palette ever
	// disappear...
	//

	ITL_TEXTURE_ID tid;
	U32 tc, tn;
	texturelibrary->get_texture_count( &tc );
	for( tn=0; tn<tc; tn++ ) {
		texturelibrary->get_texture( tn, &tid );
		texturelibrary->add_ref_texture_id( tid, NULL );
	}
}

//

BOOL CALLBACK ParticleEditor::loading_dialog_proc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message ) {

	case WM_INITDIALOG:
		SetDlgItemText( hDlg, IDC_LOADING_TEXT, (const char*)lParam );
		break;

	}
	return FALSE;
}

//

bool ParticleEditor::load_texture_file( const char *texture_filename )
{
	char fname[_MAX_PATH], drive[_MAX_DRIVE], path[_MAX_PATH], ext[_MAX_EXT], type[_MAX_EXT];
	char filename[_MAX_PATH], cmd[_MAX_PATH], txm_filename[_MAX_PATH];
	COMPTR<IFileSystem> IFS;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	HWND hDlg;

	memset( &si, 0, sizeof(si) );
	memset( &pi, 0, sizeof(pi) );

	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWMINIMIZED;

	strcpy( filename, texture_filename );
	strlwr( filename );
	strcpy( txm_filename, filename );

	_splitpath( filename, drive, path, fname, ext );

	sprintf( cmd, "Loading %s...", txm_filename );
	hDlg = CreateDialogParam( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_TEXTURE_LOADING), hEditor, loading_dialog_proc, (LPARAM)txm_filename );

	if( stricmp( ext, ".txm" ) != 0 ) {
		strcpy( type, strlwr( &ext[1] ) );
		sprintf( txm_filename, "%s%s%s_%s.txm", drive, path, fname, type );

		sprintf( cmd, "%s -c %s", txmlib_exe, txm_filename );
		CreateProcess( NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi );
		WaitForSingleObject( pi.hProcess, 5000 );

		sprintf( cmd, "%s -tex %s %s", txmlib_exe, filename, txm_filename );
		CreateProcess( NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ) ;
		WaitForSingleObject( pi.hProcess, 5000 );
	}

	if( SUCCEEDED( engine->create_file_system( txm_filename, IFS ) ) ) {
			
		texture_sources.register_texture_source( IFS );

		texturelibrary->load_library( IFS, NULL );
			
		addref_texture_palette();
	}

	DestroyWindow( hDlg );

	return true;
}

//

bool ParticleEditor::load_default_textures( const char *path, const char *preload )
{
	bool more_specs;
	bool more_dirs;
	char p_buffer[MAX_PATH], d_buffer[MAX_PATH];
	char filename[MAX_PATH], current_dir[MAX_PATH], drive[_MAX_DRIVE];
	char *filespec, *p;
	char *dirname, *d;
	WIN32_FIND_DATA fd;
	HANDLE hfind;
	char tmp_dir_name[MAX_PATH];


	strcpy( p_buffer, preload );
	p = p_buffer; 
	more_specs = true;

	while( more_specs ) {

		// remove leading whitespace
		for( ; *p && strchr( " \t;", *p ); p++ );
		
		if( *p == 0 ) {
			break;
		}
		
		filespec = p;
		for( ; *p && !strchr( " \t;", *p ); p++ );
		if( *p == 0 ) {
			more_specs = false;
		}

		*p = 0;
		p++;

		if( !strchr( filespec, '*' ) ) {
			if( SearchPath( path, filespec, NULL, MAX_PATH, filename, NULL ) ) {
				load_texture_file( filename );
			}
		}
		else {

			if( filespec[0] == '\\' || filespec[1] == ':' ) {
				_splitpath( filespec, drive, tmp_dir_name, NULL, NULL );
				if( (hfind = FindFirstFile( filespec, &fd )) != INVALID_HANDLE_VALUE ) {
					do {
						sprintf( filename, "%s%s%s", drive, tmp_dir_name, fd.cFileName );
						load_texture_file( filename );
					} while( FindNextFile( hfind, &fd ) );
				}
			}
			else {
				GetCurrentDirectory( MAX_PATH, current_dir );

				strcpy( d_buffer, path );
				d = d_buffer;
				more_dirs = true;

				while( more_dirs ) {

					// remove leading whitespace
					for( ; *d && strchr( " \t;", *d ); d++ );
					if( *d == 0 ) {
						break;
					}
					
					dirname = d;
					for( ; *d && !strchr( "\t;", *d ); d++ );
					if( *d == 0 ) {
						more_dirs = false;
					}

					*d = 0;
					d++;


					SetCurrentDirectory( dirname );

					if( (hfind = FindFirstFile( filespec, &fd )) != INVALID_HANDLE_VALUE ) {
						do {
							sprintf( filename, "%s%s", dirname, fd.cFileName );
							load_texture_file( filename );
						} while( FindNextFile( hfind, &fd ) );
					}
				}

				SetCurrentDirectory( current_dir );
			}
		}
	}

	return true;
}

//

ARCHETYPE_INDEX ParticleEditor::load_default_system( const char *filename )
{
	ARCHETYPE_INDEX arch_index;
    IFileSystem *ifs;

	// try to load default particle file
	//
	if( SUCCEEDED( engine->create_file_system( filename, &ifs, NULL ) ) ) {
		
		texture_sources.register_texture_source( ifs );

		if( (arch_index = engine->create_archetype( filename, ifs )) != INVALID_ARCHETYPE_INDEX ) {

			addref_texture_palette();

			ifs->Release();
			return arch_index;
		}

		ifs->Release();
		ifs = NULL;
	}

	// define default particle system archetype
	//
	ParticleSystem *ps = get_selected_system();

	ps->parameters.psp_f_flags = 0;
	ps->parameters.color_frames[0].x = 1.0f;
	ps->parameters.color_frames[0].y = 1.0f;
	ps->parameters.color_frames[0].z = 1.0f;
	ps->parameters.color_frames[0].w = 1.0f;
	ps->parameters.color_key_frame_bits = 0x80000001;

	ps->parameters.texture_fps = 0.0f;
	ps->parameters.texture_name[0];

	ps->parameters.src_blend = D3DBLEND_ONE;
	ps->parameters.dst_blend = D3DBLEND_ONE;

	ps->parameters.gravity.zero();

	ps->parameters.emitter_direction.set( 0.0f, 0.0f, 1.0f );
	ps->parameters.emitter_nozzle_size = 1.75f;
	ps->parameters.emitter_nozzle_damp.set( 1.0f, 1.0f, 1.0f );

	ps->parameters.initial_particle_count = 0;
	ps->parameters.max_particle_count = 0;
	ps->parameters.lifetime = 0.0f;
	ps->parameters.frequency = 1.0f;
	
	ps->parameters.particle_lifetime = 1000.0f;
	ps->parameters.particle_position_randomizer = 0.0f;
	ps->parameters.particle_velocity = 0.0065f;
	ps->parameters.particle_velocity_randomizer = 0.90f;
	ps->parameters.particle_twist_velocity = 0.0f;
	ps->parameters.particle_size = 4.0f;
	ps->parameters.particle_size_velocity = -0.004f;

	ps->parameters.bounding_sphere_radius = 100.0f;



	// save and reload default file
	//
	DAFILEDESC desc;
	
	desc.lpFileName = filename;
	desc.dwDesiredAccess = GENERIC_WRITE;
    desc.lpImplementation = "UTF";
    desc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
    desc.dwCreationDistribution = CREATE_ALWAYS;    

	if( SUCCEEDED( comanager->CreateInstance( &desc, (void**) &ifs ) ) ) {

		// save to temporary file
		//
		if( !save_to_filesystem( ifs ) ) {
			MessageBox( NULL, "Unable to save default particle file!", "Error", MB_OK );
			ifs->Release();
			return INVALID_ARCHETYPE_INDEX;
		}

		// close file to write any buffered contents
		//
		ifs->Release();	
		ifs = NULL;

		// load archetype
		//
		if( FAILED( engine->create_file_system( filename, &ifs, NULL ) ) ) {
			MessageBox( NULL, "Unable to reload default particle file!", "Error", MB_OK );
			ifs->Release();
			return INVALID_ARCHETYPE_INDEX;
		}
			
		if( (arch_index = engine->create_archetype( filename, ifs )) == INVALID_ARCHETYPE_INDEX ) {
			MessageBox( NULL, "Unable to create default archetype!", "Error", MB_OK );
			ifs->Release();
			return INVALID_ARCHETYPE_INDEX;
		}

		// register textures in the file system so we can get at them later
		//
		texture_sources.register_texture_source( ifs );

		addref_texture_palette();


		ifs->Release();
		ifs = NULL;
	}

	return arch_index;
}

//

void ParticleEditor::update_color_key_control( void )
{
	char buffer[MAX_PATH];
	U32 CurrentCK = 0, ckf_t, ckf_0, ckf_1;
	HWND hCK_Slider, hCK_Color;
	Vector4 *color_frames;
	U32 color_keys;
	
	hCK_Slider = GetDlgItem( hEditor, IDC_PARTICLE_COLOR_KEY_FRAME_SLIDER );
	hCK_Color = GetDlgItem( hEditor, IDC_PARTICLE_COLOR_KEY_COLOR );

	color_frames = systems[selected_system].parameters.color_frames;
	color_keys = systems[selected_system].parameters.color_key_frame_bits;

	TrackBar_SetRangeMin( hCK_Slider, 0, FALSE );
	TrackBar_SetRangeMax( hCK_Slider, PSP_NUM_COLOR_KEYS-1, TRUE );

	CurrentCK = TrackBar_GetPos( hCK_Slider );

	EnableWindow( GetDlgItem(hEditor, IDC_PARTICLE_COLOR_CLEAR_KEY), FALSE );
	TrackBar_ClearTicks( hCK_Slider, FALSE );
	
	for( ckf_0=0; ckf_0<PSP_NUM_COLOR_KEYS; ckf_0++ ) {
		
		if( color_keys & (1<<ckf_0) ) {

			// This color frame (number 'ckf') is a key frame.  Find the next
			// key frame after this one and lerp the intervening keyframes.
			// Also show a tick mark at this slider position and enable the
			// clear key frame button.
			//
			TrackBar_SetTick( hCK_Slider, ckf_0 );
			if( CurrentCK == ckf_0 ) {
				EnableWindow( GetDlgItem(hEditor, IDC_PARTICLE_COLOR_CLEAR_KEY), TRUE );
			}

			for( ckf_1=ckf_0+1; ckf_1<PSP_NUM_COLOR_KEYS; ckf_1++ ) {
				if( color_keys & (1<<ckf_1) ) {
					break;
				}
			}

			if( ckf_1 != (ckf_0+1) ) {

				ASSERT( ckf_1 != PSP_NUM_COLOR_KEYS );

				// lerp colors between ckf_0 and ckf_1
				//
				for( ckf_t=ckf_0+1; ckf_t<ckf_1; ckf_t++ ) {

					float b = (float)(ckf_t-ckf_0) / (float)(ckf_1-ckf_0);

					color_frames[ckf_t].x = Tblend<float>( color_frames[ckf_0].x, color_frames[ckf_1].x, b );
					color_frames[ckf_t].y = Tblend<float>( color_frames[ckf_0].y, color_frames[ckf_1].y, b );
					color_frames[ckf_t].z = Tblend<float>( color_frames[ckf_0].z, color_frames[ckf_1].z, b );
					color_frames[ckf_t].w = Tblend<float>( color_frames[ckf_0].w, color_frames[ckf_1].w, b );
				}
			}
		}
	}

	sprintf( buffer, "%d", CurrentCK+1 );
	SetDlgItemText( hEditor, IDC_PARTICLE_COLOR_KEY_FRAME, buffer );

	sprintf( buffer, "%d", PSP_NUM_COLOR_KEYS );
	SetDlgItemText( hEditor, IDC_PARTICLE_COLOR_KEY_COUNT, buffer );

	InvalidateRect( hEditor, NULL, FALSE );
	UpdateWindow( hEditor );

	systems[selected_system].set_parameters( engine );
}

//

void ParticleEditor::update_data( bool from_psp_to_dialog )
{
	ASSERT( selected_system >= 0 ) ;
	ASSERT( selected_system < PE_MAX_NUM_SYSTEMS ) ;

	char buffer[MAX_PATH];
	ParticleSystem *sps;


	sps = get_selected_system();

	// update particle system specific stuff
	//
	if( from_psp_to_dialog ) {

		int field;

		for( field=0; PSPFields[field].type != EF_T_NULL; field++ ) {
			EditField_UpdateControlFromStruct( hEditor, &PSPFields[field], &sps->parameters );
		}

		CheckDlgButton( hEditor, IDC_CHECK_ACTIVE, sps->is_active_enabled() );

		sprintf( buffer, PE_FLOAT_FORMAT, sps->render_min );
		SetDlgItemText( hEditor, IDC_MIN_RENDER_DIST, buffer );

		sprintf( buffer, PE_FLOAT_FORMAT, sps->render_max );
		SetDlgItemText( hEditor, IDC_MAX_RENDER_DIST, buffer );

		CheckDlgButton( hEditor, IDC_EXPORT_PHYSICS, sps->get_export_physics() );
		CheckDlgButton( hEditor, IDC_RENDER_RADIUS, sps->get_render_radius() );

		// update general editor state stuff
		//
		CheckDlgButton( hEditor, IDC_EVENT1 + selected_system, BST_CHECKED );

		for( field=0; PSPFields[field].type != EF_T_NULL; field++ ) {
			EditField_UpdateControlFromStruct( hEditor, &PEFields[field], this );
		}

		sprintf( buffer, PE_FLOAT_FORMAT, get_system_speed() );
		SetDlgItemText( hEditor, IDC_EVENT_VELOCITY, buffer );

		sprintf( buffer, PE_FLOAT_FORMAT, get_grid_scale() );
		SetDlgItemText( hEditor, IDC_GRID_SCALE, buffer );

		sprintf( buffer, "%s - Particle Editor", systems[selected_system].get_name( engine ) );
		SetWindowText( hRender, buffer );
		SetWindowText( hEditor, buffer );
	}
	else {
		int field;

		for( field=0; PSPFields[field].type != EF_T_NULL; field++ ) {
			EditField_UpdateStructFromControl( hEditor, &PSPFields[field], &sps->parameters );
		}

		sps->set_parameters( engine );

		for( field=0; PEFields[field].type != EF_T_NULL; field++ ) {
			EditField_UpdateStructFromControl( hEditor, &PEFields[field], this );
		}

	}

	InvalidateRect( hEditor, NULL, FALSE );
	UpdateWindow( hEditor );
}

//

void ParticleEditor::set_selected_system( int new_index )
{
	if( new_index > PE_MAX_NUM_SYSTEMS || new_index < 0 ) {
		return;
	}

	selected_system = new_index;
	update_data( true );
	update_color_key_control();
}

//

ParticleSystem *ParticleEditor::get_selected_system()
{
	ASSERT( selected_system >= 0 );
	ASSERT( selected_system < PE_MAX_NUM_SYSTEMS );

	return &systems[selected_system];
}

//

void ParticleEditor::activate_selected_system( void )
{
	systems[selected_system].activate( engine );
}

//

void ParticleEditor::deactivate_selected_system( void )
{
	systems[selected_system].deactivate( engine );
}

//

void ParticleEditor::set_render_enable( bool enabled )
{
	render_enabled = enabled;
}

//

void ParticleEditor::set_update_enable( bool enabled )
{
	update_enabled = enabled;
}

//

bool ParticleEditor::get_update_enable( void )
{
	return update_enabled;
}


//turn this back on when able to read minmax from file
#define PE_USE_LOD 0

bool ParticleEditor::save_to_filesystem( IFileSystem *IFS )
{
	ASSERT( IFS );

	ParticleSystem *ps = get_selected_system();


#if PE_USE_LOD
	SWITCH_DISTANCE_TYPE min_max[2];
	min_max[0] = ps->render_min;
	min_max[1] = ps->render_max;

	if( !IFS->CreateDirectory( MULTI_APPEARANCE_DIR ) ) {
		return false;
	}
	IFS->SetCurrentDirectory( MULTI_APPEARANCE_DIR );

	write_type_array( IFS, MIN_MAX_DIST_FILE, 2, min_max );

	if( IFS->CreateDirectory( "Level0" ) ) {
		IFS->SetCurrentDirectory( "Level0" );
#endif

		if( FAILED( write_type( IFS, PSP_IFS_KEY, ps->parameters ) ) ) {
			return false;		
		}

		if( ps->parameters.texture_name[0] ) {
			if( !save_texture_to_filesystem( IFS, ps->parameters.texture_name ) ) {
				return false;
			}
		}

		ParticleEditorData peid;

		if( use_point_extent ) {
			peid.peid_f_flags |= PEID_F_USE_POINT_EXTENT;
		}
		else {
			peid.peid_f_flags &= ~(PEID_F_USE_POINT_EXTENT);
		}

		peid.grid_scale = get_grid_scale();

		write_type( IFS, PE_IFS_PARTICLE_EDITOR_DATA, peid );


#if PE_USE_LOD
		IFS->SetCurrentDirectory( ".." );
	}
	
	IFS->SetCurrentDirectory( ".." );
#endif

	return true;
}

//

bool ParticleEditor::save_texture_to_filesystem( IFileSystem *IFS, const char *texture_name )
{
	ASSERT( IFS );

	char texture_source_name[MAX_PATH];
	char texture_source_buffer[MAX_PATH];
	COMPTR<IFileSystem> Parent;
	ITL_TEXTURE_ID tid, ftid;
	U32 frame_count = 0;


	IFS->CreateDirectory( "Texture library" );
	IFS->SetCurrentDirectory( "Texture library" );

	if( SUCCEEDED( texturelibrary->get_texture_id( texture_name, &tid ) ) ) {
		
		texturelibrary->get_texture_frame_count( tid, &frame_count );

		if( frame_count > 1 ) {
			for( U32 f=0; f<frame_count; f++ ) {

				sprintf( texture_source_name, "%s_%d", texture_name, f );
				
				if( FAILED( texturelibrary->has_texture_id( texture_source_name ) ) ) {
					break;
				}

				if( SUCCEEDED( texturelibrary->get_texture_id( texture_source_name, &ftid ) ) ) {

					texture_sources.find_texture_source( texture_source_name, texture_source_buffer );

					IFS->CreateDirectory( texture_source_name );
					IFS->SetCurrentDirectory( texture_source_name );
					
					CopyPath( texture_source_buffer, IFS );

					IFS->SetCurrentDirectory( ".." );
					
					texturelibrary->release_texture_id( ftid );
				}
			}
		}

		texture_sources.find_texture_source( texture_name, texture_source_buffer );

		IFS->CreateDirectory( texture_name );
		IFS->SetCurrentDirectory( texture_name );
			
		CopyPath( texture_source_buffer, IFS );

		IFS->SetCurrentDirectory( ".." );
				
		texturelibrary->release_texture_id( tid );
	}

	IFS->SetCurrentDirectory( ".." );

	return true;
}

//

void ParticleEditor::on_file_new( void )
{
	char buffer[MAX_PATH];

	sprintf( buffer, "%s%d", PE_UNTITLED_PREFIX, untitled_counter++ );
	systems[selected_system].initialize( engine, buffer, NULL, default_system );
	update_data( true );
	update_color_key_control();
}

//

void ParticleEditor::on_file_open( void )
{
	char filename[MAX_PATH];
	char system_name[MAX_PATH];
	char base_name[MAX_PATH];
	OPENFILENAME ofn;
	U32 name_ctr;

	filename[0] = 0;

	memset( &ofn, 0, sizeof(ofn) );
	ofn.lStructSize = sizeof(ofn);

	ofn.hwndOwner = hEditor;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpstrFilter = "Particle System Files (*.pte)\0*.pte;*.utf\0All Files (*.*)\0*.*\0\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrInitialDir = particle_directory;
	ofn.lpstrTitle = "Open Particle System";
	ofn.Flags = OFN_FILEMUSTEXIST | 
				OFN_LONGNAMES | 
				OFN_HIDEREADONLY |
				0;

	if( !GetOpenFileName( &ofn ) ) {
		return;
    }

	// see if this particle system is already loaded, if so, rename the archetype
	// so that the user can use the same .pte file as a starting point for creating/editing
	// multiple particle systems.
	//
	_splitpath( filename, NULL, NULL, base_name, NULL ) ;
	strcpy( system_name, base_name );
	name_ctr = 1;

	while( 1 ) {

		if( engine->get_archetype_by_name( system_name ) == INVALID_ARCHETYPE_INDEX ) {
			break;
		}
		
		sprintf( system_name, "%s-%d", base_name, name_ctr );
		name_ctr++;
	}

	if( !systems[selected_system].initialize( engine, system_name, filename, INVALID_ARCHETYPE_INDEX ) ) {
		MessageBox( hEditor, "Unable to create particle system. Possible reasons for this include:\n\n"
							 "The file cannot be found.\n"
							 "The file is read-only.\n"
							 "The file does not contain a particle system definition.",
							 "Error", MB_OK );
		return ;
	}

	COMPTR<IFileSystem> ifs;
	if( SUCCEEDED( engine->create_file_system( filename, ifs, NULL ) ) ) {
		texture_sources.register_texture_source( ifs );
	}

	addref_texture_palette();

	update_data( true );
	update_color_key_control();
}

//

void ParticleEditor::on_file_save( bool save_as )
{
	char filename[MAX_PATH];
	OPENFILENAME ofn;

	strcpy( filename, systems[selected_system].get_name( engine ) );

	save_as = !strncmp( filename, PE_UNTITLED_PREFIX, strlen(PE_UNTITLED_PREFIX) ) ? true : save_as;

	if( save_as ) {
		memset( &ofn, 0, sizeof(ofn) );
		ofn.lStructSize = sizeof(ofn);

		ofn.hwndOwner = hEditor;
		ofn.hInstance = GetModuleHandle(NULL);
		ofn.lpstrFilter = "Particle System Files (*.pte)\0*.pte;*.utf\0All Files (*.*)\0*.*\0\0";
		ofn.lpstrFile = filename;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = particle_directory;
		ofn.lpstrTitle = "Save As";
		ofn.Flags = OFN_FILEMUSTEXIST | 
					OFN_LONGNAMES | 
					OFN_HIDEREADONLY |
					0;

		if( !GetSaveFileName( &ofn ) ) {
			return;
		}
	}

	int	fnlen	=strlen(filename);

	// ensure .pte is appended to the filename
	if(stricmp(filename + fnlen - 4, ".pte"))
	{
		// watch for filename. nothing
		if(*(filename + fnlen - 1) == '.')
		{
			*(filename + fnlen -1)	=0;
		}
		strcat(filename, ".pte");
	}

	// save to filename
	IFileSystem *ifs;
	DAFILEDESC desc;
	char *tempname;

	if( (tempname = _tempnam( NULL, "dape_" )) == NULL ) {
		MessageBox( NULL, "Cannot create temporary filename", "Error", MB_OK );
		return ;
	}
	
	desc.lpFileName = tempname;
	desc.dwDesiredAccess = GENERIC_WRITE;
    desc.lpImplementation = "UTF";
    desc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
    desc.dwCreationDistribution = CREATE_ALWAYS;    

	if( SUCCEEDED( comanager->CreateInstance( &desc, (void**) &ifs ) ) ) {

		// save to temporary file
		//
		if( !save_to_filesystem( ifs ) ) {
			MessageBox( hEditor, "Unable to save file!", "Error", MB_OK );
			ifs->Release();
			free( tempname );
			return ;
		}
		
		// close temp file 
		//
		ifs->Release();	
		ifs = NULL;

		// copy to original filename
		//
		CopyFile( tempname, filename, FALSE );
		DeleteFile( tempname );

		if( save_as ) {
			if( !systems[selected_system].initialize( engine, filename, filename, INVALID_ARCHETYPE_INDEX ) ) {
				MessageBox( hEditor, "Unable to initialize system after Save As...", "Error", MB_OK );
			}

			COMPTR<IFileSystem> ifs;
			if( SUCCEEDED( engine->create_file_system( filename, ifs, NULL ) ) ) {
				texture_sources.register_texture_source( ifs );
			}

			update_data( true );
			update_color_key_control();
		}
	}

	free( tempname );
}

//

void ParticleEditor::on_file_exit( void )
{
	set_render_enable( false );
	set_update_enable( false );
	PostQuitMessage(0);
}


//

void ParticleEditor::update( void )
{
	if( initialized == false || update_enabled == false ) {
		return ;
	}

	U32 ss, num_active_systems = 0;
	
    for( ss=0; ss<PE_MAX_NUM_SYSTEMS; ss++ ) {

		systems[ss].update( engine, last_dt );

		if( systems[ss].is_active( engine ) ) {
			num_active_systems++;
		}
    }

	if( num_active_systems == 0 ) {
		for( ss=0; ss<PE_MAX_NUM_SYSTEMS; ss++ ) {
			if( systems[ss].is_active_enabled() ) {
				systems[ss].activate( engine );
			}
			else {
				systems[ss].deactivate( engine );
			}
		}
	}

//	position[0] += eventVel*(tick.u.LowPart-last_tick.u.LowPart)/ticks_per_second;
//	GAME.ENG->set_position(eventArray[0],position[0]);
//	GAME.ENG->set_velocity(eventArray[0],eventVel);

}

//

void ParticleEditor::render( void )
{
	if( initialized == false || render_enabled == false ) {
		return ;
	}

	RECT cr;
	GetClientRect( hRender, &cr );

	renderpipeline->set_window( hRender, 0, 0, cr.right, cr.bottom );
	renderpipeline->set_viewport( 0, 0, cr.right, cr.bottom );
	renderpipeline->set_pipeline_state( RP_CLEAR_COLOR, D3DRGBA( background_color.x, background_color.y, background_color.z, 1.0f ) );
	renderpipeline->clear_buffers( RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL );

	if( SUCCEEDED( renderpipeline->begin_scene() ) ) {
		
		renderpipeline->set_modelview( Transform() );
		font.SetOrtho();
		font.RenderFormattedString( 5, 20, PE_FLOAT_FORMAT, last_dt );

		if( last_dt != 0.00f ) {
			font.RenderFormattedString( 5, 34, PE_FLOAT_FORMAT, 1/last_dt );
		}
		else {
			font.RenderFormattedString( 5, 48, "zero update!" );
		}

		if( update_enabled == false ) {
			font.RenderFormattedString( 5, 62, "PAUSE" );
		}

		render_grid_scale();
		render_grid_plane();

		for( U32 i=0; i<PE_MAX_NUM_SYSTEMS; i++ ) {
			systems[i].render( renderpipeline, engine, Camera::getCamera(), use_axes );
		}

		renderpipeline->end_scene();
    }

	renderpipeline->swap_buffers();
}

//

void ParticleEditor::set_system_speed( float new_system_speed ) {
	
	current_system_speed = new_system_speed;

    for( U32 ss=0; ss<PE_MAX_NUM_SYSTEMS; ss++ ) {
		systems[ss].system_speed = new_system_speed;
	}
}

//

float ParticleEditor::get_system_speed( void ) {
	
	return current_system_speed;
}

//

void ParticleEditor::set_min_render_dist( float new_dist ) {
	
	current_render_min = new_dist;

    for( U32 ss=0; ss<PE_MAX_NUM_SYSTEMS; ss++ ) {
		systems[ss].render_min = new_dist;
	}
}

//

float ParticleEditor::get_min_render_dist( void ) {
	
	return current_render_min;
}

//

void ParticleEditor::set_max_render_dist( float new_dist ) {
	
	current_render_max = new_dist;

    for( U32 ss=0; ss<PE_MAX_NUM_SYSTEMS; ss++ ) {
		systems[ss].render_max = new_dist;
	}
}

//

float ParticleEditor::get_max_render_dist( void ) {
	
	return current_render_max;
}

//

void ParticleEditor::set_grid_scale( float new_grid_scale )
{
	if( new_grid_scale < 0.0001f || new_grid_scale > 10000.00f ) {
		return;
	}

	Vector camPos = Camera::getPosition();
	camPos.scale( 1/current_grid_scale * new_grid_scale );
	Camera::setPosition( camPos.x, camPos.y, camPos.z );
	Camera::lookAt( 0, 0, 0 );
	Camera::initializeMatrix();

    for( U32 ss=0; ss<PE_MAX_NUM_SYSTEMS; ss++ ) {
		systems[ss].grid_scale = new_grid_scale ;
	}

	current_grid_scale = new_grid_scale;
}

//

float ParticleEditor::get_grid_scale( void ) 
{
	return current_grid_scale;
}

//

void ParticleEditor::render_grid_scale( void )
{
	Vector pnt;
	float x,y,z;
	float gs;

	gs = get_grid_scale();

	pnt.x = - gs * 2.0f;
	pnt.y = - gs * 2.0f;
	pnt.z = 0.0f;

	ICamera *cam = Camera::getCamera();
	Transform xf = cam->get_transform();

	renderpipeline->set_modelview( Transform() );
	font.SetOrtho();

	for( int i=0; i<5; i++ ) {
		if( cam->point_to_screen( x, y, z, pnt ) ){
			font.RenderFormattedString( x, y, "%.1f", pnt.x );
		}			
	 
		pnt.x += gs;
	}
}

//

void ParticleEditor::render_grid_plane( void )
{
	Vector O, P, Pi, Pj, i, j;
	float gs;
	PrimitiveBuilder pb( renderpipeline );

	gs = get_grid_scale();

	Camera::initializeMatrix();
	renderpipeline->set_texture_stage_texture( 0, 0 );
	renderpipeline->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
	renderpipeline->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	renderpipeline->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );

	if( grid_normal.y == 1.0f ) {
		O.set( -gs * 2.0f, 0.0f, -gs * 2.0f );
		i.set( gs, 0.0f, 0.0f );
		j.set( 0.0f, 0.0f, gs );
	}
	else if( grid_normal.x == 1.0f ) {
		O.set( 0.0f, -gs * 2.0f, -gs * 2.0f );
		i.set( 0.0f, gs, 0.0f );
		j.set( 0.0f, 0.0f, gs );
	}
	else {// if( grid_normal.z == 1.0f ) {
		O.set( -gs * 2.0f, -gs * 2.0f, 0.0f );
		i.set( gs, 0.0f, 0.0f );
		j.set( 0.0f, gs , 0.0f );
	}
    

	U32 l;
	Vector N;


	pb.Begin(PB_LINES);
	pb.Color3f( 0.8, 0.8, 0.0 ); 

	if( use_axes ) {
		N = 1.25f * gs * grid_normal;
		pb.Vertex3f( 0, 0, 0 ); 
		pb.Vertex3f( N.x, N.y, N.z );
	}

	P = O;
	Pi = P + 4 * i;
	for( l=0; l<5; l++ ) {
        pb.Vertex3f( P.x,	P.y,	P.z		);	
		pb.Vertex3f( Pi.x,	Pi.y,	Pi.z	);
		Pi += j;
		P += j;
	}

	P = O;
	Pj = P + 4 * j;
	for( l=0; l<5; l++ ) {
        pb.Vertex3f( P.x,	P.y,	P.z		);	
		pb.Vertex3f( Pj.x,	Pj.y,	Pj.z	);
		Pj += i;
		P += i;
	}

    pb.End();														


}

//

bool ParticleEditor::setup_render_buffers( void )
{
	RECT r;
	Transform T(0);
	ViewRect p;

	if( hRender == NULL ) {
		return false;
	}

	GetClientRect( hRender, &r );

	if( FAILED( renderpipeline->create_buffers( hRender, r.right, r.bottom ) ) ) {
		MessageBox( NULL, "The size of the render window is too large.", "Error", MB_OK );
		return false;
	}

    // setup camera
	//
	p.x0 = 0;
	p.y0 = 0;
	p.x1 = r.right - 1;
	p.y1 = r.bottom - 1;

	if( camera_initialized ) {
		T = Camera::getTransform();
		Camera::uninit();
	}

    Camera::init( renderpipeline, engine, &p );

	if( camera_initialized ) {
		Camera::setTransform( &T );
	}
	else {
		Vector P;
		P = 10 * grid_normal + Vector(8,8,8);
		Camera::setPosition( P.x, P.y, P.z );
		Camera::lookAt( 0, 0, 0 );
	}

    Camera::initializeMatrix();

	camera_initialized = true;
	
	return true;
}

//

HWND ParticleEditor::create_render_window( )
{
	WNDCLASSEX  wc;

    memset( &wc, 0, sizeof(wc) );

    wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = render_window_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hIcon = 0; 
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = "ParticleEditor";

	RegisterClassEx( &wc );

	HWND hwnd;

	hwnd = CreateWindowEx( 0, "ParticleEditor",
						    "Render Window - Particle Editor",
							WS_OVERLAPPEDWINDOW,
							0, 0,
							PE_WINDOW_WIDTH, PE_WINDOW_HEIGHT,
							NULL,
							NULL,
							GetModuleHandle(NULL),
							this );

	ShowWindow( hwnd, SW_SHOW );
    SetFocus( hwnd );
	UpdateWindow( hwnd );

	return hwnd;
}

//

HWND ParticleEditor::create_editor_window( )
{
	HWND hdlg;

    hdlg = CreateDialogParam( GetModuleHandle(NULL), 
							  MAKEINTRESOURCE(IDD_PARTICLE_PARAMETERS), 
							  hRender,
							  editor_window_proc,
							  (LPARAM)this );

	if( hdlg == NULL ) {
		DWORD lerr = GetLastError();
		return NULL;
	}

	ShowWindow( hdlg, SW_SHOW );
    SetFocus( hdlg );
	UpdateWindow( hdlg );

	return hdlg;
}

//

long CALLBACK ParticleEditor::render_window_proc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	static	ParticleEditor *pe = NULL;
    static	Vector lb_last_pos, rb_last_pos;
	static	RECT start_rect;

	static bool in_resize = false;
	static bool in_camera_move = false;
	static bool in_camera_zoom = false;

	Vector delta_pos, mouse_pos, camera_pos;
	RECT end_rect;

    switch( message ) {

    case WM_CREATE:
		if( (pe = (ParticleEditor*)((CREATESTRUCT*)lParam)->lpCreateParams) == NULL ) {
			return FALSE;
		}
		break;


	case WM_PAINT:
		{
			PAINTSTRUCT	ps;
			BeginPaint(hWnd, &ps);
			EndPaint( hWnd, &ps ) ;

			if( pe ) {
				pe->render();
			}
		}
		break;

	case WM_ACTIVATEAPP:
		pe->set_render_enable( !(wParam == 0) );
		pe->set_update_enable( !(wParam == 0) );
		break;

	case WM_ENTERSIZEMOVE:
		GetClientRect( hWnd, &start_rect );
		in_resize = true;
		break;

	case WM_SIZE:
		if( !in_resize ) {
			SetRect( &start_rect, 0, 0, 0, 0 );
		}	SendMessage( hWnd, WM_EXITSIZEMOVE, 0, 0 );
		break;

	case WM_EXITSIZEMOVE:
		if( pe->hRender ) {
			GetClientRect( hWnd, &end_rect );
			if( (start_rect.bottom != end_rect.bottom) || (start_rect.right != end_rect.right) ) {
				if( !pe->setup_render_buffers() ) {
					if( MessageBox( NULL, "The size of the render window is too large.\n\n"
										  "Click Ok and then make the render window smaller.\n"
										  "Click Cancel to exit the app.", "Error", MB_OKCANCEL ) == IDCANCEL ) {
						PostQuitMessage( 0 );
					}
				}
			}
		}
		in_resize = false;
		break;

    case WM_LBUTTONDOWN:
		in_camera_move = true;
		lb_last_pos.x = LOWORD(lParam);
        lb_last_pos.y = HIWORD(lParam);
        lb_last_pos.z = 0.0f;
        break;

    case WM_LBUTTONUP:
		in_camera_move = false;
        break;

    case WM_RBUTTONDOWN:
		in_camera_zoom = true;
		rb_last_pos.x = LOWORD(lParam);
        rb_last_pos.y = HIWORD(lParam);
        rb_last_pos.z = 0.0f;
        break;

    case WM_RBUTTONUP:
        in_camera_zoom = false;
        break;

    case WM_MOUSEMOVE:

		mouse_pos.x = LOWORD(lParam);
		mouse_pos.y = HIWORD(lParam);
		mouse_pos.z = 0.0f;

        if( in_camera_move ) {
			delta_pos = mouse_pos - lb_last_pos;
            lb_last_pos = mouse_pos;

			Vector cam_pos = Camera::getPosition();
			delta_pos.z = delta_pos.y;
			delta_pos.y = 0.0;
			Vector new_cam_pos = cam_pos - delta_pos.scale( pe->get_grid_scale() / 100.0f );
			Camera::setPosition( &new_cam_pos );
			Camera::lookAt( 0.0, 0.0, 0.0 );
			Camera::initializeMatrix();
        }            
        if( in_camera_zoom ) {
			delta_pos = mouse_pos - rb_last_pos;
			rb_last_pos = mouse_pos;

			delta_pos.x = 0.0f;
			delta_pos.z = 0.0f;
			delta_pos.scale( pe->get_grid_scale() / 100.00f );
			Camera::moveRelative( 0, 0, delta_pos.y );
			Camera::initializeMatrix();
        }            
        break;

    case WM_KEYDOWN:
        if( wParam == VK_ESCAPE ) {
			pe->on_file_exit();
        }            
		else if( wParam == VK_PAUSE ) {
			pe->set_update_enable( !pe->get_update_enable() );
		}
        break;

    case WM_CLOSE:
		pe->on_file_exit();
	    return 0;
    }

    return DefWindowProc( hWnd, message, wParam, lParam );
}

//

BOOL CALLBACK enum_dialog_windows( HWND hwndCtrl, LPARAM lParam ) 
{ 
	TOOLINFO ti; 
	char *c, szClass[64], buffer[1024+1]; 
 
    // Skip static controls. 
	//
    GetClassName( hwndCtrl, szClass, sizeof(szClass) ); 

	if( lstrcmpi( szClass, "STATIC" ) != 0 ) { 

		int id = GetDlgCtrlID(hwndCtrl);

		if( LoadString( GetModuleHandle(NULL), id, buffer, 1024 ) ) {

			for( c = buffer; *c; c++ ) {
				if( *c == '\n' ) {
					*c = ' ';
				}
			}

			ti.cbSize = sizeof(TOOLINFO); 
			ti.uFlags = TTF_CENTERTIP | TTF_IDISHWND;
			ti.hwnd = g_hTooltipParent; 
			ti.uId = (UINT)hwndCtrl; 
			ti.hinst = GetModuleHandle(NULL); 
			ti.lpszText = buffer;
			SetRect( &ti.rect, 0, 0, 100, 100 );
			SendMessage( g_hTooltipControl, TTM_ADDTOOL, 0, (LPARAM)&ti ); 
		}
	} 

	return TRUE; 
}

//

LRESULT CALLBACK dialog_msg_hook( int nCode, WPARAM wParam, LPARAM lParam ) 
{ 
    MSG		*lpmsg, msg; 
	
    lpmsg = (MSG *)lParam; 

    if( nCode < 0 || !IsChild( g_hTooltipParent, lpmsg->hwnd ) ) {
        return CallNextHookEx( g_hTooltipHook, nCode, wParam, lParam ); 
	}
	
    switch( lpmsg->message ) { 
    case WM_MOUSEMOVE: 
    case WM_LBUTTONDOWN: 
    case WM_LBUTTONUP: 
    case WM_RBUTTONDOWN: 
    case WM_RBUTTONUP: 
		if( g_hTooltipControl != NULL ) { 
			msg = *lpmsg;
			SendMessage( g_hTooltipControl, TTM_RELAYEVENT, 0, (LPARAM) (LPMSG) &msg ); 
		} 
		break; 

	default: 
		break; 
	} 

    return CallNextHookEx( g_hTooltipHook, nCode, wParam, lParam ); 
}
	
//

void destroy_tooltip_control( void )
{
	UnhookWindowsHookEx( g_hTooltipHook );
	DestroyWindow( g_hTooltipControl );
}

//

bool create_tooltip_control( HWND hDlg )
{ 

    g_hTooltipControl = CreateWindowEx( 0, 
									    TOOLTIPS_CLASS, 
									    (LPSTR)NULL, 
									    TTS_ALWAYSTIP, 
									    CW_USEDEFAULT, CW_USEDEFAULT, 
									    CW_USEDEFAULT, CW_USEDEFAULT, 
									    hDlg, 
									    (HMENU)NULL, 
									    GetModuleHandle(NULL), 
									    NULL ); 
 
	if( g_hTooltipControl == NULL ) {
		return false;
	}

	g_hTooltipParent = hDlg;

    if( !EnumChildWindows( g_hTooltipParent, (WNDENUMPROC)enum_dialog_windows, NULL ) )  {
		return false;
	}

	SendMessage( g_hTooltipControl, TTM_SETMAXTIPWIDTH, 0, 200 );

    // Install a hook procedure to monitor the message stream for mouse 
    // messages intended for the controls in the dialog box. 
	//
    if( (g_hTooltipHook = SetWindowsHookEx( WH_GETMESSAGE, dialog_msg_hook, (HINSTANCE)NULL, GetCurrentThreadId() )) == NULL ) {
		return false;
	}
	return true;
}

//

//

BOOL CALLBACK ParticleEditor::editor_window_proc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	static ParticleEditor *pe = NULL;
	static bool in_grid_scale_update = false;
	static bool in_size_velocity_update = false;
	static HBRUSH hCK_Brush = 0;

	HWND hwnd;
	RECT rect;
	Vector4 color;
	char buffer[MAX_PATH];
	int i, num;
	ParticleSystem *sps = NULL;
	COLORREF current_color;
	U32 rp_texture_id;
	PAINTSTRUCT	ps;
	ITL_TEXTURE_ID tlid;
	ITL_TEXTUREFRAME_IRP tlframe;
	char filename[MAX_PATH];
	OPENFILENAME ofn;


	switch( message ) {

	case WM_INITDIALOG:
		if( (pe = (ParticleEditor*)lParam) == NULL ) {
			return FALSE;
		}

		pe->hEditor = hDlg;

		if( pe->use_tooltips ) {
			create_tooltip_control( hDlg );
		}

		hwnd = GetDlgItem( hDlg, IDC_PARTICLE_SRC_BLEND );
		ComboBox_ResetContent( hwnd );
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "ZERO" ),				D3DBLEND_ZERO );
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "ONE" ),				D3DBLEND_ONE );           
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "SRCCOLOR" ),			D3DBLEND_SRCCOLOR );
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "INVSRCCOLOR" ),		D3DBLEND_INVSRCCOLOR );
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "SRCALPHA" ),			D3DBLEND_SRCALPHA );        
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "INVSRCALPHA" ),		D3DBLEND_INVSRCALPHA );     
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "DESTALPHA" ),		D3DBLEND_DESTALPHA );       
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "INVDESTALPHA" ),		D3DBLEND_INVDESTALPHA );    
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "DESTCOLOR" ),		D3DBLEND_DESTCOLOR );       
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "INVDESTCOLOR" ),		D3DBLEND_INVDESTCOLOR );    
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "SRCALPHASAT" ),		D3DBLEND_SRCALPHASAT );     
		ComboBox_SetCurSel( hwnd, 1 );
	
		hwnd = GetDlgItem( hDlg, IDC_PARTICLE_DST_BLEND );
		ComboBox_ResetContent( hwnd);
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "ZERO" ),				D3DBLEND_ZERO );
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "ONE" ),				D3DBLEND_ONE );           
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "SRCCOLOR" ),			D3DBLEND_SRCCOLOR );
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "INVSRCCOLOR" ),		D3DBLEND_INVSRCCOLOR );
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "SRCALPHA" ),			D3DBLEND_SRCALPHA );        
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "INVSRCALPHA" ),		D3DBLEND_INVSRCALPHA );     
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "DESTALPHA" ),		D3DBLEND_DESTALPHA );       
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "INVDESTALPHA" ),		D3DBLEND_INVDESTALPHA );    
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "DESTCOLOR" ),		D3DBLEND_DESTCOLOR );       
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "INVDESTCOLOR" ),		D3DBLEND_INVDESTCOLOR );    
		ComboBox_SetItemData( hwnd, ComboBox_AddString( hwnd, "SRCALPHASAT" ),		D3DBLEND_SRCALPHASAT );     
		ComboBox_SetCurSel( hwnd, 1 );
	
		pe->update_color_key_control();
		pe->update_data( true );

		for( i=0; PSPFields[i].type != EF_T_NULL; i++ ) {
			EditField_Attach( hDlg, &PSPFields[i], 1.0f );
		}
		
		for( i=0; PEFields[i].type != EF_T_NULL; i++ ) {
			EditField_Attach( hDlg, &PEFields[i], 1.0f );
		}
		
		return TRUE;


    case WM_CLOSE:
		pe->on_file_exit();
        return FALSE;

	case WM_DESTROY:
		DeleteObject( hCK_Brush );

		for( i=0; PSPFields[i].type != EF_T_NULL; i++ ) {
			EditField_Detach( hDlg, &PSPFields[i] );
		}
		
		for( i=0; PEFields[i].type != EF_T_NULL; i++ ) {
			EditField_Detach( hDlg, &PEFields[i] );
		}
		
		if( g_hTooltipControl ) {
			destroy_tooltip_control();
		}
		break;

	case WM_PAINT:
		
		hwnd = GetDlgItem( hDlg, IDC_TEXTURE_WINDOW );
		ASSERT( hwnd != 0 );

		BeginPaint( hDlg, &ps );
		EndPaint( hDlg, &ps );

		sps = pe->get_selected_system();

		UpdateWindow( hwnd );

		rp_texture_id = 0;

		if( SUCCEEDED( pe->texturelibrary->has_texture_id( sps->parameters.texture_name ) ) ) {

			if( SUCCEEDED( pe->texturelibrary->get_texture_id( sps->parameters.texture_name, &tlid ) ) ) {

				if( SUCCEEDED( pe->texturelibrary->get_texture_frame( tlid, ITL_FRAME_FIRST, &tlframe ) ) ) {

					rp_texture_id = tlframe.rp_texture_id;
				}

				pe->texturelibrary->release_texture_id( tlid );
			}
		}

		{
			GetClientRect( hwnd, &rect );
			
			float x0 = PE_TEXTURE_BORDER_WIDTH;
			float y0 = PE_TEXTURE_BORDER_WIDTH;
			float x1 = rect.right - PE_TEXTURE_BORDER_WIDTH;
			float y1 = rect.bottom - PE_TEXTURE_BORDER_WIDTH;
			
			pe->renderpipeline->set_window( hwnd, 0, 0, rect.right, rect.bottom );
			pe->renderpipeline->set_viewport( 0, 0, rect.right, rect.bottom );

			pe->renderpipeline->clear_buffers( RP_CLEAR_COLOR_BIT, NULL );

			pe->renderpipeline->begin_scene();

			pe->renderpipeline->set_ortho( 0, rect.right, rect.bottom, 0 );
			pe->renderpipeline->set_modelview( Transform() );

			pe->renderpipeline->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE );
			pe->renderpipeline->set_render_state( D3DRS_ZENABLE, FALSE );

			pe->renderpipeline->set_texture_stage_texture( 0, rp_texture_id );
			pe->renderpipeline->set_texture_stage_state( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
			pe->renderpipeline->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			pe->renderpipeline->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
			pe->renderpipeline->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
			pe->renderpipeline->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
			pe->renderpipeline->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
			
			pe->renderpipeline->set_texture_stage_texture( 1, 0 );
			pe->renderpipeline->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
			pe->renderpipeline->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

			PrimitiveBuilder pb( pe->renderpipeline, 6 );
			pb.Begin( PB_QUADS );
				pb.Color3f( 1, 1, 1 );	pb.TexCoord2f( tlframe.u0, tlframe.v0 );	pb.Vertex3f( x0, y0, 0.0f );
				pb.Color3f( 1, 1, 1 );	pb.TexCoord2f( tlframe.u0, tlframe.v1 );	pb.Vertex3f( x0, y1, 0.0f );
				pb.Color3f( 1, 1, 1 );	pb.TexCoord2f( tlframe.u1, tlframe.v1 );	pb.Vertex3f( x1, y1, 0.0f );
				pb.Color3f( 1, 1, 1 );	pb.TexCoord2f( tlframe.u1, tlframe.v0 );	pb.Vertex3f( x1, y0, 0.0f );
			pb.End();
					
			pe->renderpipeline->end_scene();
			pe->renderpipeline->swap_buffers();

		}
		break;

	case WM_CTLCOLORSTATIC:
		if( ((HWND)lParam) == GetDlgItem( hDlg, IDC_PARTICLE_COLOR_KEY_COLOR ) ) {
			
			sps = pe->get_selected_system();
			
			num = TrackBar_GetPos( GetDlgItem( hDlg, IDC_PARTICLE_COLOR_KEY_FRAME_SLIDER) );
			
			current_color = RGB_MAKE( int(sps->parameters.color_frames[num].z * 255.0),
									  int(sps->parameters.color_frames[num].y * 255.0),
									  int(sps->parameters.color_frames[num].x * 255.0) );
			if( hCK_Brush != 0 ) {
				DeleteObject( hCK_Brush );
				hCK_Brush = 0;
			}
			
			return (BOOL)(hCK_Brush = CreateSolidBrush( current_color ));
		}
		break;

	case WM_HSCROLL:
		pe->update_color_key_control();
		break;

	case WM_COMMAND:
		// handle non-edit fields here
		//
		sps = pe->get_selected_system();

		// handle ui non-edit fields attached to PSP fields
		//
		for( i=0; PSPFields[i].type != EF_T_NULL; i++ ) {
//			if( PSPFields[i].control_id == LOWORD(wParam) && !(PSPFields[i].type == EF_T_REAL || PSPFields[i].type == EF_T_INT) ) {
			if( PSPFields[i].control_id == LOWORD(wParam) ) {
				EditField_UpdateStructFromControl( hDlg, &PSPFields[i], &sps->parameters );
				sps->set_parameters( pe->engine );
				break;
			}
		}

		// handle non-psp non-edit fields 
		//
		switch( LOWORD(wParam) ) {

		case IDC_PE_PARTICLE_CALC_SIZE_VELOCITY:
			EnableWindow( GetDlgItem( hDlg, IDC_PARTICLE_SIZE_VELOCITY ), !IsDlgButtonChecked( hDlg,  LOWORD(wParam) ) );
			if( IsDlgButtonChecked( hDlg,  LOWORD(wParam) ) ) {
				sps->find_size_velocity();
				sprintf( buffer, PE_FLOAT_FORMAT, sps->parameters.particle_size_velocity );
				SetDlgItemText( hDlg, IDC_PARTICLE_SIZE_VELOCITY, buffer );

			}
			return TRUE;

		case IDC_PARTICLE_COLOR_KEY_COLOR:
			num = TrackBar_GetPos( GetDlgItem( hDlg, IDC_PARTICLE_COLOR_KEY_FRAME_SLIDER) );
			sprintf( buffer, "Set Color Key Frame #%d", num+1 );
			GetColorSelection( hDlg, MAKEINTRESOURCE(IDD_COLOR_SELECT), buffer, &sps->parameters.color_frames[num].x );
			sps->parameters.color_key_frame_bits |= (1<<num);
			pe->update_color_key_control();
			return TRUE;

		case IDC_PARTICLE_COLOR_CLEAR_KEY:
			num = TrackBar_GetPos( GetDlgItem( hDlg, IDC_PARTICLE_COLOR_KEY_FRAME_SLIDER) );
			sps->parameters.color_key_frame_bits &= ~(1<<num);
			pe->update_color_key_control();
			return TRUE;

		case IDC_PARTICLE_COLOR_CLEAR_ALL:
			
			sprintf( buffer, "Set Constant Color" );
			GetColorSelection( hDlg, MAKEINTRESOURCE(IDD_COLOR_SELECT), buffer, &color.x );
			
			for( num=0; num<PSP_NUM_COLOR_KEYS; num++ ) {
				sps->parameters.color_frames[num].x = color.x;
				sps->parameters.color_frames[num].y = color.y;
				sps->parameters.color_frames[num].z = color.z;
				sps->parameters.color_frames[num].w = color.w;
			}

			sps->parameters.color_key_frame_bits = 0x80000001;

			pe->update_color_key_control();
			return TRUE;

		case IDC_EXPORT_PHYSICS:
            sps->set_export_physics( IsDlgButtonChecked( hDlg, IDC_EXPORT_PHYSICS ) );
            return TRUE;

        case IDC_DRAW_RADIUS:
            sps->set_render_radius( IsDlgButtonChecked( hDlg, IDC_EXPORT_PHYSICS ) );
            return TRUE;

		case IDC_CHECK_ACTIVE:
			if( IsDlgButtonChecked( hDlg, IDC_CHECK_ACTIVE ) ) {
				sps->set_active_enable( true );
				pe->activate_selected_system();
			}
			else {
				sps->set_active_enable( false );
				pe->deactivate_selected_system();
			}
			return TRUE;

		case IDC_TEXTURE_WINDOW:
			GetTextureSelection( hDlg, MAKEINTRESOURCE(IDD_TEXTURE_SELECT), "Select Texture", pe->texturelibrary, pe->renderpipeline, sps->parameters.texture_name, 64 );
			SetDlgItemText( pe->hEditor, IDC_PARTICLE_TEXTURE, sps->parameters.texture_name );
			sps->set_parameters( pe->engine );
			InvalidateRect( pe->hEditor, NULL, FALSE );
			UpdateWindow( pe->hEditor );
			break;

		case IDC_PARTICLE_TEXTURE_PREV:
		case IDC_PARTICLE_TEXTURE_NEXT:
		{
			char			buf[PSP_TEXTURE_NAME_LEN];
			U32				tn, tcnt;
			ITL_TEXTURE_ID	tlid;

			// find the current texture
			//
			pe->texturelibrary->get_texture_count( &tcnt );
			for( tn = 0; tn < tcnt; tn++ ) {
				
				pe->texturelibrary->get_texture( tn, &tlid );
				pe->texturelibrary->get_texture_name( tlid, buf, PSP_TEXTURE_NAME_LEN );
				pe->texturelibrary->release_texture_id( tlid );
				
				if( strcmp( buf, sps->parameters.texture_name ) == 0 ) {
					break;
				}				
			}

			// if it was found, advance it by one
			//
			if( tn < tcnt ) {
				
				if( LOWORD(wParam) == IDC_PARTICLE_TEXTURE_NEXT ) {
					tn = (tn + 1) % tcnt;
				}
				else {
					if( tn == 0 ) {
						tn = tcnt-1;
					}
					else {
						tn = tn - 1;
					}
				}

				pe->texturelibrary->get_texture( tn, &tlid );
				pe->texturelibrary->get_texture_name( tlid, sps->parameters.texture_name, PSP_TEXTURE_NAME_LEN );
				pe->texturelibrary->release_texture_id( tlid );

				SetDlgItemText( pe->hEditor, IDC_PARTICLE_TEXTURE, sps->parameters.texture_name );
				sps->set_parameters( pe->engine );

				InvalidateRect( hDlg, NULL, FALSE );
				UpdateWindow( hDlg );
			}
			return	TRUE;
		}

		case ID_PTE_NEW:
			pe->on_file_new();
			return TRUE;

        case ID_PTE_LOAD:
            pe->on_file_open();
            return TRUE;

        case ID_PTE_SAVE:
            pe->on_file_save( false );
            return TRUE;

		case ID_PTE_SAVEAS:
            pe->on_file_save( true );
			return TRUE;

        case ID_FILE_EXIT:
			pe->on_file_exit();
            return TRUE;

        case IDC_EVENT1:
        case IDC_EVENT2:
        case IDC_EVENT3:
        case IDC_EVENT4:
        case IDC_EVENT5:
			CheckDlgButton( hDlg, IDC_EVENT1, BST_UNCHECKED );
			CheckDlgButton( hDlg, IDC_EVENT2, BST_UNCHECKED );
			CheckDlgButton( hDlg, IDC_EVENT3, BST_UNCHECKED );
			CheckDlgButton( hDlg, IDC_EVENT4, BST_UNCHECKED );
			CheckDlgButton( hDlg, IDC_EVENT5, BST_UNCHECKED );
			pe->set_selected_system( LOWORD(wParam) - IDC_EVENT1 );
            return TRUE;
		
		case IDC_IMPORT:
			filename[0] = 0;

			memset( &ofn, 0, sizeof(ofn) );
			ofn.lStructSize = sizeof(ofn);

			ofn.hwndOwner = hDlg;
			ofn.hInstance = GetModuleHandle(NULL);
			ofn.lpstrFilter = "Texture Library Files (*.txm)\0*.txm;*.utf\0"
							  "Bitmap Files (*.bmp)\0*.bmp\0"
							  "Targa Files (*.tga)\0*.tga\0"
							  "All Files (*.*)\0*.*\0\0";
			ofn.lpstrFile = filename;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrInitialDir = pe->texture_directory;
			ofn.lpstrTitle = "Import Textures";
			ofn.Flags = OFN_FILEMUSTEXIST | 
						OFN_LONGNAMES | 
						OFN_HIDEREADONLY |
						0;

			if( GetOpenFileName( &ofn ) ) {
				pe->load_texture_file( filename );
			}
		}
		break;


	case WM_EF_FIELD_CHANGED:
		// handle all edit-fields here
		//
		sps = pe->get_selected_system();

		// handle ui edit fields attached to PSP fields
		//
		for( i=0; PSPFields[i].type != EF_T_NULL; i++ ) {
			if( PSPFields[i].control_id == LOWORD(wParam) ) {
				EditField_UpdateStructFromControl( hDlg, &PSPFields[i], &sps->parameters );
				sps->set_parameters( pe->engine );
				break;
			}
		}

		// handle all other edit fields here else
		//
		switch( LOWORD(wParam) ) {

		case IDC_PARTICLE_SIZE:
		case IDC_PARTICLE_LIFETIME:
		case IDC_EMITTER_LIFETIME:
			if( !in_size_velocity_update ) {
				
				in_size_velocity_update = true;

				if( IsDlgButtonChecked( hDlg, IDC_PE_PARTICLE_CALC_SIZE_VELOCITY ) ) {
					sps->find_size_velocity();
					sprintf( buffer, PE_FLOAT_FORMAT, sps->parameters.particle_size_velocity );
					SetDlgItemText( hDlg, IDC_PARTICLE_SIZE_VELOCITY, buffer );
				}

				in_size_velocity_update = false;
			}
			return TRUE;

		case IDC_EVENT_VELOCITY:
			GetDlgItemText( hDlg, IDC_EVENT_VELOCITY, buffer, MAX_PATH );
			pe->set_system_speed( atof( buffer ) ) ;
			sprintf( buffer, PE_FLOAT_FORMAT, pe->get_system_speed() );
			SetDlgItemText( hDlg, IDC_EVENT_VELOCITY, buffer );
			return TRUE;

		case IDC_GRID_SCALE:
			GetDlgItemText( hDlg, IDC_GRID_SCALE, buffer, MAX_PATH );
			pe->set_grid_scale( (float)atof(buffer) );
			sprintf( buffer, PE_FLOAT_FORMAT, pe->get_grid_scale() );
			SetDlgItemText( hDlg, IDC_GRID_SCALE, buffer );
			return TRUE;

		case IDC_MIN_RENDER_DIST:
			GetDlgItemText( hDlg, IDC_MIN_RENDER_DIST, buffer, MAX_PATH );
			pe->set_min_render_dist( (float)atof(buffer) );
			sprintf( buffer, PE_FLOAT_FORMAT, pe->get_min_render_dist() );
			SetDlgItemText( hDlg, IDC_MIN_RENDER_DIST, buffer );
			return TRUE;

		case IDC_MAX_RENDER_DIST:
			GetDlgItemText( hDlg, IDC_MAX_RENDER_DIST, buffer, MAX_PATH );
			pe->set_max_render_dist( (float)atof(buffer) );
			sprintf( buffer, PE_FLOAT_FORMAT, pe->get_max_render_dist() );
			SetDlgItemText( hDlg, IDC_MAX_RENDER_DIST, buffer );
			return TRUE;

		}
		break;
    }

    return FALSE;
}


//

int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int )
{
	InitCommonControls();

    // init editor & set up windows
    if( !theParticleEditor.initialize( hInstance ) ) {
		return FALSE;
	}

	bool done = false;
    while( !done ) {
		done = theParticleEditor.message_pump();
	}

    theParticleEditor.cleanup() ; 

	return FALSE;
}

//

HRESULT OpenBasePath( char *path, IFileSystem *parent_fs, U32 access_flags, IFileSystem **out_fs )
{
	char buffer[MAX_PATH];
	char *p;
	COMPTR<IComponentFactory> FS;
	DAFILEDESC desc;

	strcpy( buffer, path );

	// c:foo.pte
	// c:\foo.pte
	// c:\bar\foo.pte
	// c:\bar\foo.pte\path\file.ext

	for( p=buffer; *p && (*p != '\\' && *p != ':'); p++ ); // find next '\' or ':'

	if( *p == ':' && p[1] == '\\' ) {
		p++;
	}

	if( *p != 0 ) {
		p++;
		strcpy( path, p );	// return shortened path	
		*p = 0;
	}
	else {
		path[0] = 0;
	}

	if( !strchr( buffer, ':' ) && (strlen(buffer)>1) && buffer[strlen(buffer)-1] == '\\' ) {
		buffer[strlen(buffer)-1] = 0 ;
	}
	
	desc.lpFileName = buffer;
//	desc.dwCreationDistribution = OPEN_ALWAYS;
//	desc.dwDesiredAccess = access_flags;
//	desc.dwShareMode = 0;		

	if( parent_fs == NULL ) {
		FS = DACOM_Acquire();
	}
	else {
		FS = parent_fs;
	}

	if( FAILED( FS->CreateInstance( &desc, (void**) out_fs ) ) ) {
		return E_FAIL;
	}
	
	return S_OK;
}

//

#define COPY_BUFFER_SIZE	65536 * 4
static U8 copy_buffer[COPY_BUFFER_SIZE];

//

void CopyTree( IFileSystem *src, IFileSystem *dest )
{
	WIN32_FIND_DATA wfd;
	HANDLE			handle;

	if( (handle = src->FindFirstFile( "*.*", &wfd )) != INVALID_HANDLE_VALUE ) {

		do {
			if( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

				dest->CreateDirectory( wfd.cFileName );
				
				src->SetCurrentDirectory( wfd.cFileName );
				dest->SetCurrentDirectory( wfd.cFileName );

				CopyTree( src, dest );

				src->SetCurrentDirectory( ".." );
				dest->SetCurrentDirectory( ".." );
			}
			else {
				
				DWORD bytes_read;
				DAFILEDESC desc, desc2;
				HANDLE h, h2;
				
				desc.lpFileName = wfd.cFileName;

				if( (h = src->OpenChild( &desc )) != INVALID_HANDLE_VALUE ) {
					src->ReadFile( h, copy_buffer, COPY_BUFFER_SIZE, &bytes_read, 0 );
					src->CloseHandle( h );
				}

				desc2.lpFileName = wfd.cFileName;
			    desc2.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
				desc2.dwShareMode = 0;
				desc2.dwCreationDistribution = CREATE_ALWAYS;    

				if( (h2 = dest->OpenChild( &desc2 )) != INVALID_HANDLE_VALUE ) {
					dest->WriteFile( h2, copy_buffer, bytes_read, &bytes_read, 0 );
					dest->CloseHandle( h2 );
				}
			}
		}
		while( src->FindNextFile( handle, &wfd ));

		FindClose( handle );
	}
}

//

HRESULT CopyPath( const char *source_path, IFileSystem *dest_ifs )
{
	COMPTR<IFileSystem> SrcFS;
	COMPTR<IFileSystem> ParentFS;
	char src_path[MAX_PATH];

	// Get a filesystem pointer to the source and destination
	//
	strcpy( src_path, source_path );
	ParentFS.free();
	while( src_path[0] != 0 ) {
		if( FAILED( OpenBasePath( src_path, ParentFS, GENERIC_READ, SrcFS ) ) ) {
			break;
		}
		ParentFS = SrcFS;
	}

	CopyTree( SrcFS, dest_ifs );

	return S_OK;
}


//



#if OLD_CODE_THAT_IS_FUNKY

	case IDC_BOUND:
	{
		LRESULT result = SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 );
		curEventDef->bBound = (result == BST_CHECKED);
		if (curEventDef->bBound)
		{
			EnableWindow(GetDlgItem(hDlg,IDC_INHERIT),FALSE);
			curEventDef->bInherit = FALSE;
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg,IDC_INHERIT),TRUE);
			result = SendMessage( GetDlgItem(hDlg,IDC_INHERIT), BM_GETCHECK, 0, 0 );
			curEventDef->bInherit = (result == BST_CHECKED);
		}
		particleEditor.updateCurrentEventParams();
		return FALSE;
	}

	case IDC_EDIT_DIRX:
		GetDlgItemText( hDlg, IDC_EDIT_DIRX, temp, 40 ); 
		if( atof(temp) == 0.0 ){
			if( curEventDef->direction.y == 0.0 &&
				curEventDef->direction.z == 0.0 ){
					sprintf( temp, PE_FLOAT_FORMAT,curEventDef->direction.x );
					SetDlgItemText( hDlg, LOWORD(wParam), temp );
					return FALSE;
				}
		}
		curEventDef->direction.x = atof(temp);                      
		particleEditor.updateCurrentEventParams();
		return FALSE;
#endif




