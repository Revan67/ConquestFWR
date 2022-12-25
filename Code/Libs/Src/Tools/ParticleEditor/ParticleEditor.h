//***************************************************************************************
// Particle Editor
//  bramage november 97
//***************************************************************************************

#ifndef PARTICLEEDITOR_H
#define PARTICLEEDITOR_H

#include <windows.h>

#include "vector.h"
#include "matrix.h"
#include "ParticleSystemParameters.h"
#include "ITextureLibrary.h"

#include "EditField.h"
#include "TextureSourceMap.h"

//

#define PE_MAX_NUM_SYSTEMS					5
#define PE_MAX_NUM_RADIUS_POINTS			500
#define PE_DEFAULT_TEXTURE_NAME				"particle"
#define PE_DEFAULT_GRID_SCALE				(5.0f)
#define PE_WINDOW_WIDTH						500		// render window width
#define PE_WINDOW_HEIGHT					500		// render window height
#define PE_FLOAT_FORMAT						"%7.4f"
#define PE_UNTITLED_PREFIX					"Untitled-"	
#define PE_DEFAULT_GRID_NORMAL				Vector(0,0,1)
#define PE_TEXTURE_BORDER_WIDTH				8

//

#define RELEASE( iff ) if( iff ) { iff->Release(); iff = NULL; }

//

#define PSP(field)	offsetof( ParticleSystemParameters, field )
#define PS(field)	offsetof( ParticleSystem, field )
#define PE(field)	offsetof( ParticleEditor, field )

//

#pragma warning( disable : 4244 )

//

#define PS_F_VALID			(1<<0)
#define PS_F_EXPORT_PHYSICS	(1<<1)
#define PS_F_RENDER_RADIUS	(1<<2)
#define PS_F_ACTIVE_ENABLE	(1<<3)

//

class ParticleSystem
{

public: // Interface

	ParticleSystem( void );
	
	bool initialize( IEngine *engine, const char *_name, const char *_filesys, ARCHETYPE_INDEX _arch_index );
	void cleanup( IEngine *engine );

	bool set_active_enable( bool on_off );
	bool is_active_enabled( void );

	bool activate( IEngine *engine );
	bool deactivate( IEngine *engine );
	bool is_active( IEngine *engine );

	const char *get_name( IEngine *engine );
	const Vector get_position( IEngine *engine );
	void get_parameters( IEngine *engine );
	void set_parameters( IEngine *engine );

	void set_render_radius( bool on_or_off );
	bool get_render_radius( void );

	void set_export_physics( bool on_or_off );
	bool get_export_physics( void );
	
	void set_max_render( SINGLE max );
	SINGLE get_max_render( );

	void find_size_velocity( void );

	void update( IEngine *engine, float dt_s );
	void render( IRenderPipeline *renderpipeline, IEngine *engine, ICamera *camera, U32 show_axes );
	void render_radius_points( void );

public: // Data

	float render_min, render_max;
	float grid_scale;
	float system_speed;

	ParticleSystemParameters parameters;


protected:	// Data

	char system_name[MAX_PATH];

	U32 ps_f_flags;		// PS_F_ flags

	Vector radius_points[PE_MAX_NUM_RADIUS_POINTS];
	
	INSTANCE_INDEX active_inst_index;
	ARCHETYPE_INDEX arch_index;
};

//

#define PEID_F_USE_POINT_EXTENT		(1<<0)

// This is persisted to disk with the particle system 
// It is data that the editor will use (if available)
// to setup the ui while editing the particle system.
//
#define PE_IFS_PARTICLE_EDITOR_DATA	"ParticleEditorData"
struct ParticleEditorData
{
	U32   peid_f_flags;
	float grid_scale;
};



//

class ParticleEditor
{

public: // Data

	float			current_system_speed;
	float			current_grid_scale;
	float			current_render_min;
	float			current_render_max;

protected: // Data

	bool			initialized;
	bool			camera_initialized;
	bool			render_enabled;
	bool			update_enabled;
	
    C8              particle_directory[MAX_PATH];
    C8              tmp_particle_name[MAX_PATH];

    Vector          background_color;
    float			default_grid_scale;
	Vector			grid_normal;
	U32 			use_point_extent;
	U32				use_tooltips;
	U32				use_axes;

	ARCHETYPE_INDEX default_system;
	ParticleSystem	systems[PE_MAX_NUM_SYSTEMS];
	U32				selected_system;

	TextureSourceMap texture_sources;
    C8               texture_directory[MAX_PATH];
	C8				 txmlib_exe[MAX_PATH];

	U32				untitled_counter;
	float			last_dt;	// used for the framerate counter
	float			locked_framerate;
	float			fixed_framerate;

	ICOManager		 *comanager;
	ISystemContainer *system;
	IEngine			 *engine;
	IRenderer		 *renderer;

public: // Interface
    HWND				hEditor;        // handle to editor dialog
	HWND				hRender;		// handle to the render window
	ITextureLibrary		*texturelibrary;
	IRenderPipeline		*renderpipeline;
    
	bool initialize( HINSTANCE hInst );
	void cleanup( void );

	HWND create_render_window( );
	HWND create_editor_window( );
	bool message_pump( void );

	void update( void );
	void render( void );

	void set_selected_system( int new_index );
	ParticleSystem *get_selected_system();

	void activate_selected_system( void );
	void deactivate_selected_system( void );

	void set_render_enable( bool enabled );

	void set_update_enable( bool enabled );
	bool get_update_enable( void );
	
	void set_grid_scale( float new_grid_scale );
	float get_grid_scale( void );

	void set_system_speed( float new_system_speed );
	float get_system_speed( void );

	float get_min_render_dist( void );
	void set_min_render_dist( float new_dist );

	float get_max_render_dist( void );
	void set_max_render_dist( float new_dist );

	void on_file_new( void );
	void on_file_open( void );
	void on_file_save( bool save_as );
	void on_file_exit( void );
	
	static long CALLBACK render_window_proc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK editor_window_proc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK loading_dialog_proc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );

protected: // Interface

	bool setup_render_buffers( void );

	void addref_texture_palette( void );

	bool load_default_textures( const char *path, const char *preload );
	bool load_texture_file( const char *texture_filename );

	ARCHETYPE_INDEX load_default_system( const char *filename );

	bool save_to_filesystem( IFileSystem *IFS );
	bool save_texture_to_filesystem( IFileSystem *IFS, const char *texture_name );

	void update_data( bool from_psp_to_dialog );
	void update_color_key_control( void );

	void render_grid_scale( void );
	void render_grid_plane( void );
};

//

#endif
