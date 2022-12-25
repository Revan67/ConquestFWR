//$Header: /Libs/dev/Src/Tools/ObjView/cmpndview.cpp 100   3/20/00 11:18a Pbleisch $

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <io.h>
#include <commdlg.h>
#include <shellapi.h>

//

#include "DACOM.h"
#include "FDUMP.h"
#include "TempStr.h"
#include "TSmartPointer.h"
#include "3DMath.h"
#include "Timer.h"
#include "StdDAT.h"
#include "mesh.h"
#include "da_heap_utility.h"
#include "FileSys.h"
#include "Engine.h"
#include "Renderer.h"
#include "IProfileParser_Utility.h"
#include "ITextureLibrary.h"
#include "IMaterialLibrary.h"
#include "System.h"
#include "BaseCam.h"
#include "IHardPoint.h"
#include "IVertexBufferManager.h"
#include "physics.h"
#include "IMesh.h"
#include "IMaterialProperties.h"
#include "IRenderDebugger.h"

//

#include "Shapes.h"
#include "VirtualKeyStrings.h"

//

#include "CmpndView.h"
#include "Resource.h"

//

// Extent.cpp
// 
#ifdef DRAWEXTENTS
	#include "extent.h"
	#include "geom.h"
	extern void RenderExtents( unsigned long extent_depth );
#endif

// Hardpoint.cpp
// 
#ifdef HARDPOINTS
	int  HardpointList_GetCount();
	void HardpointList_Clear( void );
	void HardpointList_Append( INSTANCE_INDEX inst_index );
	void HardpointList_Render( unsigned int which_hp, unsigned int flags );
	void HardpointList_Connect( void );
#endif

// Animation.cpp
// 
#ifdef ANIMATION
	extern void CleanupAnimation (void);
	extern void LoadAnimation (const char* filename, INSTANCE_INDEX obj);
	extern void StartAnimation (int i, bool loop);
	extern void StopAnimation (int i);
	extern void ReleaseAnimationComponent (void);
	extern void StartupAnimation (IEngine* engine);
	extern void UpdateAnimation (float);
#endif

// Joint.cpp
//
extern void DoExerciseJoints (INSTANCE_INDEX, float dt);
extern void ResetJoints (INSTANCE_INDEX);
extern void SetJointsToExtreme (INSTANCE_INDEX idx, bool min);
extern void UpdateJointMotionChangeRate (bool inc, float dt);
extern float JointChangeRate;

// Err.cpp
//
extern void Err_Initialize( void );
extern void Err_SetLogFilename( const char *filename );
extern void Err_SetLogEnable( bool LogToFile );

// WinStuff.cpp
//
extern HWND   hWnd;
extern HACCEL hKeyBindings;
extern unsigned int AppActive;

extern void MessagePump (void);
extern void SetStyleInWindow (int display_size_X, int display_size_Y);
extern void SetStyleFullScreen (void);

// GetProfileInfo.cpp
//
HRESULT GetProfileInfo( char *out_app_name, U32 max_app_name_len, char *out_ini_pathname, U32 max_ini_pathname_len );


// States.cpp
//
HRESULT CreateDepthTestedMaterial( IDAComponent *System, IMaterial **out_material );

// CmpndView.cpp (This File)
//

#define CAP_TIME !0

const int   MAX_FTL = 16;
const float MAX_DT = (1.0 / 8.0);
const float DEFAULT_START_DISTANCE = 500.0f;
const U32   DEPTH_COOKIE = 0xabcdef01;

const unsigned int MAX_HELP_LINES_PER_PAGE = 16;

const char *FILTER_RENDERABLE_FILES = "Renderable Files (*.pte;*.3db;*.cmp;*.txm)\0*.pte;*.3db;*.cmp;*.txm;*.utf\0All Files (*.*)\0*.*\0\0";
const char *FILTER_SCENE_FILES		= "IRD Scene Trace Files (*.st)\0*.st\0All Files (*.*)\0*.*\0\0";
const char *FILTER_DDS_FILES		= "DirectDraw Surface Files (*.dds)\0*.dds\0All Files (*.*)\0*.*\0\0";
const char *FILTER_BMP_FILES		= "Bitmap Files (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0\0";

IEngine				* Engine		= NULL;
ISystemContainer	* System		= NULL;
ITextureLibrary		* TextureLib	= NULL;
IRenderer			* Renderer		= NULL;
ILightManager		* LightMan		= NULL;
IRenderPipeline		* RenderPipe	= NULL;
IRenderPrimitive	* RenderPrim	= NULL;
IMaterialLibrary	* MaterialLib	= NULL;

IRenderDebugger		* RenderDebugger= NULL;

PrimitiveBuilder pb;
RPFont3D	Font;

char	AppName[MAX_PATH]			={0};
char	IniFile[MAX_PATH]			={0};
char	PathFromINI[MAX_PATH * 3]	={0};
char	FilesToLoad[MAX_FTL][MAX_PATH]	={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
char	PreloadTextureFile[MAX_PATH]={0};

INSTANCE_INDEX LoadedInstIndices[MAX_OBJS];
int			   LoadedInstCount = 0;

float	lod_fraction = -1.0f;
float	lod_delta = .01f;
float	nurb_error = 1.0f;
float	nurb_delta = 0.05f;
int		bezier_sub = 4;
float	edge_angle = 20.0f * MUL_DEG_TO_RAD;
U32		texture_count = 0;
U32		material_count = 0;
float	pixel_threshold = .5f;

U32		ird_capture_scene = 0;
U32		ird_capture_startup = 0;
U32		ird_capture_screen_shot = 0;
char	ird_capture_output_dir[MAX_PATH];

bool			draw_object = false;
bool			draw_fc_vc = false;
bool			draw_stats = false;
int				draw_axis = 0;
int				draw_texture = 0;
int				draw_material = 0;
unsigned long	draw_wire = 0;
unsigned long	draw_extent = 0;
unsigned long	draw_hp = 0;
unsigned long	draw_help = 0;
unsigned long   num_help_keys = 0;

#if defined LIGHT
U32				draw_builtin_light = 0;
U32				draw_light_list = 0;
bool			builtin_light_enabled = true;
bool			builtin_light_orbit = false;
float			LightRangeScale = 10.0f; 
#endif

bool			use_irp_lighting = true;

RenderFlags		render_mode = RF_RELATIVE_LOD;

U32				SkipBatch = 0;

U32				Fullscreen = 0; 
U32				DisplayWidth = 640;
U32				DisplayHeight = 480;
U32				DisplayDepth = DEPTH_COOKIE;

ICamera	   *TheCamera = NULL;
BaseCamera *Camera = NULL;
bool circle_camera = true;

float CameraNearClippingPlane = 1.0f;
float CameraFarClippingPlane = 1e4;
float CameraRotationRate = MUL_DEG_TO_RAD * 60.0f;
float CameraZoomRate = 64.0f;
float CameraStartDistance = DEFAULT_START_DISTANCE;
float CameraHFov = 90.0f;
float CameraHVAspect = 4.0f/3.0f;

float global_scale = DEFAULT_START_DISTANCE;

float initial_radius = DEFAULT_START_DISTANCE;

const float STATUS_DEFAULT_FULL		= 4.0f;
const float STATUS_DEFAULT_DECAY	= 3.0f;
char	StatusMessage[MAX_PATH]		= {0};
float	StatusTimeout				= 0.0f;
int		StatusMode					= 0;


//camera motion
bool ShiftDown = false;
bool CtrlDown = false;
bool Up = false;
bool Down = false;
bool LeftDown = false;
bool RightDown = false;
bool In = false;
bool Out = false;

bool JointMotionChange = false;
bool ExerciseJoints = false;
bool ChangeAmbient = false;
U32  PromptIfNotFound = false;

float ClearRed = 0.0;
float ClearGreen = 0.0;
float ClearBlue = 0.3;

float SpinRate = 35 * MUL_DEG_TO_RAD;
float TransRate = CameraZoomRate;

int SpinI = 0;
int SpinJ = 0;
int SpinK = 0;

int TransI = 0;
int TransJ = 0;
int TransK = 0;

//

HWND CreateAppWindow( HINSTANCE hAppInstance );

void UpdateWindowTitle( void );

void AttachCamera(bool);
void SetCameraStartDistance (void);
void SetInitialCameraPosition (void);

void StatusUpdate( const char *fmt, ... );
void StatusRender( float dt );

void SetDisplayMode( U32 width, U32 height, U32 depth, U32 fullscreen );
void TrapFpu(bool on);
void StripExtension( char *name );
void StripPath( char *name );
bool GetFileNameFromUser( HWND hParent, bool save_dialog, const char *filter, char *out_buffer, U32 max_buffer_len );

void RenderAxis(void);
void RenderTexture( U32 texture_num, bool show_mappings );
void RenderMaterial(void);

void HandleCommand( U32 command );

//

float ObjectList_ComputeRadius( void );

//

void ObjectList_Initialize( void )
{
	for (int i=0; i<MAX_OBJS; i++){
		LoadedInstIndices[i] = INVALID_INSTANCE_INDEX;
	}
}

//

int ObjectList_FindIndex( int begin_lii_idx, int end_lii_idx, INSTANCE_INDEX srch )
{
	for( int i = begin_lii_idx; i < end_lii_idx; i++ ) {
		if( LoadedInstIndices[i] == srch ) {
			return i;
		}
	}

	return -1;
}

//

int ObjectList_AppendChildren( int begin_lii_idx )
{
	if( LoadedInstIndices[begin_lii_idx] == INVALID_INSTANCE_INDEX ) {
		return begin_lii_idx;
	}

	ASSERT( begin_lii_idx+1 < MAX_OBJS );

	int last_lii_idx = begin_lii_idx + 1;

	INSTANCE_INDEX child = INVALID_INSTANCE_INDEX;

	while( INVALID_INSTANCE_INDEX != (child = Engine->get_instance_child_next( LoadedInstIndices[begin_lii_idx], EN_DONT_RECURSE, child ) ) ) {
		LoadedInstIndices[last_lii_idx] = child;
		last_lii_idx = _MAX( ObjectList_AppendChildren( last_lii_idx ), last_lii_idx );
	}

	return last_lii_idx;
}

//

void ObjectList_Load( void )
{
	int num_loaded = 0;

	if( PromptIfNotFound && !FilesToLoad[0][0] ) {
		if( !GetFileNameFromUser( hWnd, false, FILTER_RENDERABLE_FILES, FilesToLoad[0], MAX_PATH ) ) {
			return;
		}
	}

	for( U32 ftl=0; FilesToLoad[ftl][0]; ftl++ ) {

		COMPTR<IFileSystem> IFS;
		int					last_lii_idx = 0;

		if( FAILED( Engine->create_file_system( FilesToLoad[ftl], IFS ) ) ) {
			GENERAL_WARNING( TEMPSTR( "Unable to cfs for '%s'", FilesToLoad[ftl] ) ) ;
			continue;
		}

		StatusUpdate( "Loading '%s'...", FilesToLoad[ftl] );

		TextureLib->load_library( IFS, NULL );
		if( MaterialLib ) MaterialLib->load_library( IFS );

		LoadedInstIndices[LoadedInstCount] = Engine->create_instance( FilesToLoad[ftl], IFS, NULL );

		if( LoadedInstIndices[LoadedInstCount] != INVALID_INSTANCE_INDEX ) {
			
			Engine->set_transform( LoadedInstIndices[LoadedInstCount], Transform() );

			LoadAnimation( FilesToLoad[ftl], LoadedInstIndices[LoadedInstCount] );

			ResetJoints( LoadedInstIndices[LoadedInstCount] );

			HardpointList_Append( LoadedInstIndices[LoadedInstCount] );

			LoadedInstCount = ObjectList_AppendChildren( LoadedInstCount );

			draw_object = true;
		}

		TextureLib->get_texture_count( &texture_count );
		if( MaterialLib ) MaterialLib->get_material_count( &material_count );

		num_loaded++;
	}

	if( num_loaded ) {

		if( MaterialLib ) MaterialLib->verify_library( 0, 1.0f );

		global_scale =
		initial_radius = ObjectList_ComputeRadius();
		
		SetCameraStartDistance ();

		SetInitialCameraPosition ();
		
		AmbientLight_Reset();
	}
	else if( PromptIfNotFound ) {
		
		if( !GetFileNameFromUser( hWnd, false, FILTER_RENDERABLE_FILES, FilesToLoad[0], MAX_PATH ) ) {
			return;
		}

		ObjectList_Load();
	}
}

//

void ObjectList_Unload( void )
{
	int i;

	// First go through and remove the children
	//
	for( i=0; i<LoadedInstCount; i++ ) {
		if( LoadedInstIndices[i] != INVALID_INSTANCE_INDEX ) {
			if( Engine->get_instance_parent( LoadedInstIndices[i] ) != INVALID_INSTANCE_INDEX ) {
				LoadedInstIndices[i] = INVALID_INSTANCE_INDEX;
			}
		}
	}

#ifdef ANIMATION
	CleanupAnimation ();
#endif

	HardpointList_Clear();

	// Next go through destroy the parents
	//
	for( i=0; i<LoadedInstCount; i++ ) {
		if( LoadedInstIndices[i] != INVALID_INSTANCE_INDEX ) {
			Engine->destroy_instance( LoadedInstIndices[i] );
		}
	}

	LoadedInstCount = 0;

	draw_object = false;
	draw_axis = 0;
	draw_wire = 0;
	draw_texture = 0;
	draw_material = 0;

	texture_count = 0;
	material_count = 0;
}

//

float ObjectList_ComputeRadius( void )
{
	Vector center;
	float radius;

	Engine->get_instance_bounding_sphere( LoadedInstIndices[0], 0, &radius, &center);

	return radius + center.magnitude();
}

//

void UpdateWindowTitle( void )
{
	ASSERT( RenderPipe );
	char window_title[MAX_PATH], buffer[MAX_PATH];

	RPDEVICEINFO di;
	if( FAILED( RenderPipe->get_device_info( &di ) ) ) {
		strcpy( di.device_class, "Unknown" );
		strcpy( di.device_type, "Unknown" );
	}

	window_title[0] = 0;

	for( U32 ftl=0; FilesToLoad[ftl][0]; ftl++ ) {
		strcat( window_title, FilesToLoad[ftl] );
		strcat( window_title, ", " );
	}

	U32 m, n, b;
	DACOM_GetVersion( &m, &n, &b );

	sprintf( buffer, "%s -- %s [%s,%s,%s] [%d.%d.%d]", window_title, AppName, IniFile, di.device_class, di.device_type, m, n, b );

	SetWindowText( hWnd, buffer );
}

//

GENRESULT SetRender2D( void )
{
	Transform model(false);
	int vp[4];

	RenderPrim->get_viewport( &vp[0], &vp[1], &vp[2], &vp[3] );
	RenderPrim->set_ortho( vp[0], vp[2], vp[3], vp[1], 0, 1000 );

	model.set_identity();
	RenderPrim->set_modelview( model );

	return GR_OK;
}

//

GENRESULT SetRenderVolume (const class Transform *object_to_world = NULL)
{
	RenderPrim->set_viewport( Camera->pane.x0, Camera->pane.y0,
							  DisplayWidth-Camera->pane.x0, DisplayHeight-Camera->pane.y0 );

	RenderPrim->set_perspective( Camera->fovy, Camera->aspect, Camera->znear, Camera->zfar );

	Transform to_view ( Camera->transform.get_inverse() );
	// If object-to-world transform supplied, concatenate world-to-view 
	// and object-to-world transforms to get object-to-view transform
	//
	if (object_to_world != NULL)
	{
		to_view = to_view.multiply(*object_to_world);
	}

	RenderPrim->set_modelview( to_view );

	return GR_OK;
}

//

void AttachCamera( bool set_nearfar )
{
	COMPTR<ICamera> cam;
	
	for( int i=0; i < LoadedInstCount; i++ ) {

		Camera->set_orientation( Engine->get_orientation( LoadedInstIndices[i] ) );
		Camera->set_position( Engine->get_position( LoadedInstIndices[i] ) );

		if(GR_OK == Engine->query_instance_interface( LoadedInstIndices[i], IID_ICamera, (IDAComponent**)&cam))
		{
			circle_camera = false;

			Camera->set_Horizontal_FOV( 2.0f * cam->get_fovx() );
			Camera->set_Horizontal_to_vertical_aspect( tan(cam->get_fovx()*MUL_DEG_TO_RAD) / 
													   tan(cam->get_fovy()*MUL_DEG_TO_RAD) );

			if( set_nearfar )
			{
				Camera->set_near_plane_distance( cam->get_znear() );
				Camera->set_far_plane_distance( cam->get_zfar() );
			}
			break;
		}
	}
}

//

void RenderUVMapping( IMesh *IM, ITL_TEXTURE_ID tid, TC_WRAPMODE mode, U32 element_bit, int x0, int y0, int dx, int dy ) 
{
	COMPTR<IMaterial> mat;
	COMPTR<IMaterialProperties> mat_prop;
	U32 uv_stride, uv_size, uv_count, face_count;
	const U16 *uv_indices;
	TexCoord *uvs;
	ITL_TEXTURE_REF_ID	trid[3];
	TC_WRAPMODE			twm[3];
	ITL_TEXTURE_ID		tids[3]  = { -1, -1, -1 };

	if( FAILED( IM->get_facegroup_material( mat ) ) ) {
		return;	
	}
			
	if( !mat || FAILED( mat->QueryInterface( IID_IMaterialProperties, mat_prop ) ) ) {
		return;
	}

	if( SUCCEEDED( mat_prop->get_texture( 0, &trid[0] ) ) ) {
		TextureLib->get_texture_ref_texture_id( trid[0], &tids[0] );
		mat_prop->get_texture_wrap_mode( 0, &twm[0] );
	}
	if( SUCCEEDED( mat_prop->get_texture( 1, &trid[1] ) ) ) {
		TextureLib->get_texture_ref_texture_id( trid[1], &tids[1] );
		mat_prop->get_texture_wrap_mode( 0, &twm[1] );
	}
	if( SUCCEEDED( mat_prop->get_texture( 2, &trid[2] ) ) ) {
		TextureLib->get_texture_ref_texture_id( trid[2], &tids[2] );
		mat_prop->get_texture_wrap_mode( 0, &twm[2] );
	}

	if( ((tids[0] == tid) && (twm[0] == mode)) || ((tids[1] == tid) && (twm[1] == mode)) || ((tids[2] == tid) && (twm[2] == mode)) ) {
		
		if( FAILED( IM->lock_facegroup_data( element_bit ) ) )
			goto release_tex_ref;

		if( FAILED( IM->lock_facegroup_indices( element_bit ) ) )
			goto release_tex_ref;

		if( FAILED( IM->get_facegroup_data( element_bit, (const void**)&uvs, &uv_stride, &uv_size, &uv_count ) ) )
			goto release_tex_ref;

		if( FAILED( IM->get_facegroup_indices( element_bit, &uv_indices, &face_count ) ) )
			goto release_tex_ref;

		face_count /= 3;	// get number of faces from number of indices, assumes trilists

		pb.Begin( PB_LINES );
		pb.Color4ub(0, 224, 0, 0);

		for( U32 f=0; f<face_count; f++ ) {
	
			int uv_id1 = uv_indices[f*3+0];
			int uv_id2 = uv_indices[f*3+1];
			int uv_id3 = uv_indices[f*3+2];
			
			pb.Vertex2f( x0 + dx * uvs[uv_id1].u, y0 + dy * (1.0f - uvs[uv_id1].v) );
			pb.Vertex2f( x0 + dx * uvs[uv_id2].u, y0 + dy * (1.0f - uvs[uv_id2].v) );

			pb.Vertex2f( x0 + dx * uvs[uv_id2].u, y0 + dy * (1.0f - uvs[uv_id2].v) );
			pb.Vertex2f( x0 + dx * uvs[uv_id3].u, y0 + dy * (1.0f - uvs[uv_id3].v) );

			pb.Vertex2f( x0 + dx * uvs[uv_id3].u, y0 + dy * (1.0f - uvs[uv_id3].v) );
			pb.Vertex2f( x0 + dx * uvs[uv_id1].u, y0 + dy * (1.0f - uvs[uv_id1].v) );
		}

		pb.End();
	}

release_tex_ref:
	TextureLib->release_texture_ref( trid[0] );
	TextureLib->release_texture_ref( trid[1] );
	TextureLib->release_texture_ref( trid[2] );
}

//

void RenderTexture( U32 texture_num, bool show_mappings )
{
	ITL_TEXTURE_ID			index;
	ITL_TEXTUREFRAME_IRP	frame;
	U32	w, h, numlod;
	PixelFormat pf;
	char name[256];
	char *fmt_name, fmt[256];
	GENRESULT gr;
	
	if( SUCCEEDED( TextureLib->get_texture( texture_num, &index ) ) ) {

		gr = TextureLib->get_texture_frame( index, 0, &frame );

		if( FAILED( TextureLib->get_texture_name( index, name, 256 ) ) ) {
			strcpy( name, "(Unknown)" );
		}

		RenderPipe->get_texture_dim( frame.rp_texture_id, &w, &h, &numlod );
		RenderPipe->get_texture_format( frame.rp_texture_id, &pf );

		pf.persist( fmt );
		if( (fmt_name = strchr( fmt, '_' )) == NULL ) {
			fmt_name = fmt;
		}


		// Setup state
		//
		RenderPrim->set_texture_stage_texture( 0, frame.rp_texture_id );

		RenderPrim->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	0 ); 

		RenderPrim->set_texture_stage_state( 0, D3DTSS_COLOROP,	  D3DTOP_MODULATE ); 
		RenderPrim->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE ); 
		RenderPrim->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_CURRENT ); 

		RenderPrim->set_texture_stage_state( 0, D3DTSS_ALPHAOP,	  D3DTOP_MODULATE ); 
		RenderPrim->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE ); 
		RenderPrim->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_CURRENT ); 

		RenderPrim->set_texture_stage_state( 1, D3DTSS_COLOROP,	  D3DTOP_DISABLE ); 
		RenderPrim->set_texture_stage_state( 1, D3DTSS_ALPHAOP,	  D3DTOP_DISABLE ); 
		RenderPrim->set_texture_stage_texture( 1, 0 );

		if( pf.has_alpha_channel() && !show_mappings ) {

			RenderPrim->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE );
			RenderPrim->set_render_state( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			RenderPrim->set_render_state( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		}
		else {
			RenderPrim->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );
		}

		RenderPrim->set_render_state( D3DRS_ZENABLE, FALSE );
		RenderPrim->set_render_state( D3DRS_ZWRITEENABLE, FALSE );
		RenderPrim->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE );

		SetRender2D();
		
		if( show_mappings ) {

			while( DisplayWidth >= 2 * w && DisplayHeight >= 2 * h ) {
				w *= 2;
				h *= 2;
			}

			RenderPrim->set_render_state(D3DRS_DITHERENABLE, FALSE);
			RenderPrim->set_sampler_state(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			RenderPrim->set_sampler_state(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			RenderPrim->set_sampler_state(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
		}
		else {
			RenderPrim->set_render_state(D3DRS_DITHERENABLE, TRUE);
			RenderPrim->set_sampler_state(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			RenderPrim->set_sampler_state(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			RenderPrim->set_sampler_state(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		}
					
		int x0 = (DisplayWidth - w)/2;
		int y0 = (DisplayHeight - h)/2;
		int x1 = x0 + w - 1;
		int y1 = y0 + h - 1;

		pb.Color4ub( 255, 255, 255, 255 );

		// draw texture
		//
		pb.Begin(PB_QUADS);
			pb.TexCoord2f(0,1); pb.Vertex2f(x0,y0);
			pb.TexCoord2f(1,1); pb.Vertex2f(x1,y0);
			pb.TexCoord2f(1,0); pb.Vertex2f(x1,y1);
			pb.TexCoord2f(0,0); pb.Vertex2f(x0,y1);
		pb.End();

		RenderPrim->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );

		pb.Color4ub( 255, 255, 255, 255 );

		// draw white frame around texture
		//
		if( show_mappings || pf.has_alpha_channel() ) {

			RenderPrim->set_texture_stage_texture( 0, 0 );
			
			pb.Begin(PB_LINES);
				pb.Vertex2f(x0,	y0);	pb.Vertex2f(x1,	y0);
				pb.Vertex2f(x1,	y0);	pb.Vertex2f(x1,	y1);
				pb.Vertex2f(x1,	y1);	pb.Vertex2f(x0,	y1);
				pb.Vertex2f(x0,	y1);	pb.Vertex2f(x0,	y0);
			pb.End();
		}

		// draw uvs over texturemap
		if( show_mappings ) {
			
			for( int j=0; j<LoadedInstCount; j++ ) {

				Mesh *mesh;
				IMesh *IM;

				int dx = x1 - x0;
				int dy = y1 - y0;

				if( (mesh = Renderer->get_instance_mesh( LoadedInstIndices[j], Camera )) != NULL ) {

					TexCoord *uv0s = mesh->texture_vertex_list;

					pb.Begin( PB_LINES );
					pb.Color4ub(0, 224, 0, 0);

					for( int g_id=0; g_id < mesh->face_group_cnt; g_id++ ) {

						FaceGroup *fg = mesh->face_groups + g_id;
						Material *m = &(mesh->material_list[fg->material]);

						if( frame.rp_texture_id == (int)m->texture_id ||
							frame.rp_texture_id == (int)m->second_diffuse_texture_id ||
							frame.rp_texture_id == (int)m->emissive_texture_id )
						{
							
							for( int f_id=0; f_id < fg->face_cnt; f_id++ ) {
							
								int b_id1 = fg->face_vertex_chain[3*f_id];
								int b_id2 = fg->face_vertex_chain[3*f_id+1];
								int b_id3 = fg->face_vertex_chain[3*f_id+2];

								int uv_id1 = mesh->texture_batch_list[b_id1];
								int uv_id2 = mesh->texture_batch_list[b_id2];
								int uv_id3 = mesh->texture_batch_list[b_id3];

								pb.Vertex2f( x0 + dx * uv0s[uv_id1].u, y0 + dy * (1.0f - uv0s[uv_id1].v) );
								pb.Vertex2f( x0 + dx * uv0s[uv_id2].u, y0 + dy * (1.0f - uv0s[uv_id2].v) );

								pb.Vertex2f( x0 + dx * uv0s[uv_id2].u, y0 + dy * (1.0f - uv0s[uv_id2].v) );
								pb.Vertex2f( x0 + dx * uv0s[uv_id3].u, y0 + dy * (1.0f - uv0s[uv_id3].v) );

								pb.Vertex2f( x0 + dx * uv0s[uv_id3].u, y0 + dy * (1.0f - uv0s[uv_id3].v) );
								pb.Vertex2f( x0 + dx * uv0s[uv_id1].u, y0 + dy * (1.0f - uv0s[uv_id1].v) );
							}
						}
					}

					pb.End();
				}
				else if( SUCCEEDED( Engine->query_instance_interface( LoadedInstIndices[j], IID_IMesh, (IDAComponent**)&IM ) ) ) {

					U32 fgc;

					IM->get_num_facegroups( &fgc );

					for( U32 fg = 0; fg < fgc; fg++ ) {
						
						IM->lock_facegroup( fg, true );

						RenderUVMapping( IM, index, TC_WRAP_UV_0, IM_E_UV0S, x0, y0, dx, dy );
						RenderUVMapping( IM, index, TC_WRAP_UV_1, IM_E_UV1S, x0, y0, dx, dy  );

						IM->unlock_facegroup();
					}
				}
			}
		}

		// show name
		//
		Font.RenderString( x0, y0-48, name );
		Font.RenderFormattedString( x0, y0-24, "%dx%d %d %s", w, h, numlod, fmt_name );

		TextureLib->release_texture_id( index );
	}
}

//

void RenderMaterials( void )
{
	IMaterial *material;

	if( MaterialLib && SUCCEEDED( MaterialLib->get_material( (draw_material & ~HIGH_BIT)-1, &material ) ) ) {

		Transform object_to_world( false );
		Transform world_to_view( false );
		Transform object_to_view( false );
		char name[MAX_PATH], type[MAX_PATH];
		Box b;
		U32 np;
		Shape box;
		float size;
		MaterialContext mc;

		size = 2.1f * initial_radius;

		CreateBox( ST_SOLID, 0xFFFFFFFF, size, size, size, &box );

		material->get_type( type, MAX_PATH );
		material->get_name( name, MAX_PATH );
		material->get_num_passes( &np );

		world_to_view = TheCamera->get_inverse_transform();

		object_to_world.translation.zero();
		object_to_world.set_x_rotation( 45.0f );
		object_to_world.set_y_rotation( 45.0f );
		
		object_to_view = world_to_view.multiply( object_to_world );

		RenderPrim->set_modelview( object_to_view );

		mc.inst_index = INVALID_INSTANCE_INDEX;
		mc.object_to_world = &object_to_world;
		mc.world_to_view = &world_to_view;
		mc.object_to_view = &object_to_view;
		mc.vertex_buffer = 0;
		mc.extended_context = NULL;

		material->apply();
		material->render( &mc, box.primitive_type, &box.vbd, 0, box.vbd.num_vertices, box.i_buffer, box.num_indices, 0 );

		SetRender2D();

		StatusUpdate( "" );
		Font.RenderFormattedString( 12.0f, DisplayHeight-20.0f, "%s (%s %d)", name, type, np );

		material->Release();
	}
}

//

void RenderAxis(void)
{
	SetRenderVolume();

	RenderPrim->set_texture_stage_texture( 0, 0 );
	if(draw_axis & HIGH_BIT){
		RenderPrim->set_render_state( D3DRS_ZENABLE, FALSE );
	}
	else{
		RenderPrim->set_render_state( D3DRS_ZENABLE, TRUE );
	}
	RenderPrim->set_render_state( D3DRS_LIGHTING, FALSE );

	int start, end;
	if((draw_axis & ~HIGH_BIT) == 1)
	{
		start = 0;
		end = LoadedInstCount;
	}
	else
	{
		start = (draw_axis & ~HIGH_BIT) - 2;
		end = start + 1;
	}

	float scale = 0.2f * global_scale;
	Vector p1, p2;

	pb.Begin(PB_POINTS);
	pb.Color4ub(255,255,255,255);
	for(int j = start; j < end; j++)
	{
		pb.Vertex3fv((float*)&(Engine->get_position (LoadedInstIndices[j])));
	}
	pb.End();

	pb.Begin(PB_LINES);
	for(j = start; j < end; j++)
	{
		p1 = Engine->get_position (LoadedInstIndices[j]);

		p2 = p1 + scale * Engine->get_orientation(LoadedInstIndices[j]).get_i();
		pb.Color4ub(255,0,0,255);
		pb.Vertex3f(p1.x, p1.y, p1.z);
		pb.Vertex3f(p2.x, p2.y, p2.z);

		p2 = p1 + scale * Engine->get_orientation(LoadedInstIndices[j]).get_j();
		pb.Color4ub(0,255,0,255);
		pb.Vertex3f(p1.x, p1.y, p1.z);
		pb.Vertex3f(p2.x, p2.y, p2.z);

		p2 = p1 + scale * Engine->get_orientation(LoadedInstIndices[j]).get_k();
		pb.Color4ub(0,0,255,255);
		pb.Vertex3f(p1.x, p1.y, p1.z);
		pb.Vertex3f(p2.x, p2.y, p2.z);
#if 0 // line in -z
		p2 = p1 - 10.0f * scale * Engine->get_orientation(LoadedInstIndices[j]).get_k();
		pb.Color4ub(255,255,255,255);
		pb.Vertex3f(p1.x, p1.y, p1.z);
		pb.Vertex3f(p2.x, p2.y, p2.z);
#endif
	}
	pb.End();

	// print object names
	Font.SetColor( 1,1,1,1 );
	Font.SetSize( .0005f * global_scale );
	Transform world_to_cam ( Camera->get_inverse_transform() );
	for(j = start; j < end; j++)
	{
		ARCHETYPE_INDEX arch = Engine->get_instance_archetype(LoadedInstIndices[j]);
		ASSERT( arch != INVALID_ARCHETYPE_INDEX );
		Engine->release_archetype(arch);

		char name[256];

		strcpy( name, Engine->get_archetype_name( arch ) );
		StripPath( name );
		StripExtension( name );

		int length = strlen(name);
		if( length > 10 )
		{
			name[length - 10] = 0; // strip date and extension
		}

		Transform model_m(false);
		model_m.set_x_rotation( M_PI ); // make it face +Z
		model_m.translation = world_to_cam * Engine->get_position (LoadedInstIndices[j]);

		RenderPipe->set_modelview( model_m );
		
		Font.RenderString( .0f, .0f, name );
	}

	RenderPrim->set_render_state( D3DRS_ZENABLE, TRUE );
}

//

void RenderStats( void )
{
	if( RenderPipe ) {
		RPDEVICESTATS stats;

		if( SUCCEEDED( RenderPipe->get_device_stats( &stats ) ) ) {

			SetRender2D();
	
			int x = DisplayWidth - 350;
			int y = DisplayHeight - 80;

			Font.RenderFormattedString( x, y, "Tmgr: vid: %d of %d act:%d of %d (%d:%d:%d)", 
						stats.num_texture_vidmem,
						stats.num_texture_managed, 
						stats.num_texture_vidmem_activated,
						stats.num_texture_activated, 
						stats.num_texture_vidmem_created,
						stats.num_texture_vidmem_evicted,
						stats.is_thrashing );

			y += 20;
			Font.RenderFormattedString( x, y, "Tmem: v:%d s:%d", 
						stats.sizeof_texture_vidmem, 
						stats.sizeof_texture_sysmem );

			y += 20;
			Font.RenderFormattedString( x, y, "Prim: DP: %d in %d DIP:%d in %d", 
						stats.num_dp_primitives, 
						stats.num_dp_calls,
						stats.num_dip_primitives, 
						stats.num_dip_calls );
		}
	}
}

//

void RenderFcVc(void)
{
	int total_fc = 0;
	int total_vc = 0;

	if ( Renderer )
	{
		for(int j=0; j < LoadedInstCount; j++)
		{
			Mesh *mesh = Renderer->get_instance_mesh(LoadedInstIndices[j], Camera);
			if(mesh)
			{
				total_fc += mesh->face_cnt;
				total_vc += mesh->object_vertex_cnt;
			}
			else
			{
				U32 fc, vc;

				ARCHETYPE_INDEX arch = Engine->get_instance_archetype(LoadedInstIndices[j]);
				Renderer->get_archetype_statistics( arch, 1.0f, ST_NUM_FACES, &fc );
				Renderer->get_archetype_statistics( arch, 1.0f, ST_NUM_VERTICES, &vc );
				Engine->release_archetype(arch);

				total_fc += fc;
				total_vc += vc;
			}
		}
	}
	
	int vp[4];

	RenderPrim->get_viewport( &vp[0], &vp[1], &vp[2], &vp[3] );

	SetRender2D();
	
	Font.SetSize( .125 );
	Font.RenderFormattedString( vp[2] - 100, 20, "FC %d", total_fc);
	Font.RenderFormattedString( vp[2] - 100, 40, "VC %d", total_vc);
}

//

void RenderWireframe( U32 mode, bool depth_tested, float scale )
{
	if( Renderer == NULL ) {
		return;	// required
	}

	COMPTR<IMesh> IM;
	Mesh *mesh;
	unsigned char r, g, b;


	RenderPrim->set_texture_stage_texture( 0, 0 );

	RenderPrim->set_render_state( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	RenderPrim->set_render_state( D3DRS_ZENABLE, depth_tested );
	RenderPrim->set_render_state( D3DRS_LIGHTING, FALSE );

	//

	for( int j=0; j<LoadedInstCount; j++ ) {

		SetRenderVolume( &Engine->get_transform( LoadedInstIndices[j] ) );

		mesh = Renderer->get_instance_mesh( LoadedInstIndices[j], Camera );
		Engine->query_instance_interface( LoadedInstIndices[j], IID_IMesh, (IDAComponent**)&IM );
				
		//

		pb.Begin( PB_LINES );

		pb.Color4ub(255, 255, 255, 255);

		if( mesh != NULL ) {

			// in case we did not render normally first
			if( !(lod_fraction < 0.0f) ) {
				mesh->set_lod( lod_fraction );
			}
				
			FaceGroup *fg = NULL;
			Vector P, N, N1, N2, N3;

			switch( mode ) {

			case 2:	// Edges
				if( (mesh->lod_fraction || mesh->lod_fraction == -1.0f) ) {

					for( int e_id=0; e_id < mesh->edge_cnt; e_id++ ) {
						
						if( mesh->edge_angle_list[e_id] < edge_angle ) {
							break;
						}

						pb.Vertex3fv( (float*)&mesh->object_vertex_list[ mesh->edge_vertex_chain[ 2*e_id + 0 ] ] );
						pb.Vertex3fv( (float*)&mesh->object_vertex_list[ mesh->edge_vertex_chain[ 2*e_id + 1 ] ] );
					}
				}
				break;

			case 1: // Faces
			case 3: // Normals

				for( fg = mesh->face_groups; fg < mesh->face_groups + mesh->face_group_cnt; fg++ ) {

					int *Fis = fg->face_vertex_chain;

					for( int f_id = 0; f_id < fg->face_cnt; f_id++ ) {
						
						// Red		--> double sided
						// Green	--> flat shaded
						// Blue		--> hidden
						// White	--> smooth
						//
						r = (fg->face_properties[f_id] & TWO_SIDED)?	255 : 0;
						g = (fg->face_properties[f_id] & FLAT_SHADED)?	255 : 0;
						b = (fg->face_properties[f_id] & HIDDEN)?		255 : 0;

						if( r + g + b == 0 ) {
							r = g = b = 255;
						}

						const int v_id1 = mesh->vertex_batch_list[ Fis[ 3*f_id + 0 ] ];
						const int v_id2 = mesh->vertex_batch_list[ Fis[ 3*f_id + 1 ] ];
						const int v_id3 = mesh->vertex_batch_list[ Fis[ 3*f_id + 2 ] ];

						const Vector &P1 = mesh->object_vertex_list[v_id1];
						const Vector &P2 = mesh->object_vertex_list[v_id2];
						const Vector &P3 = mesh->object_vertex_list[v_id3];

						// Face
						pb.Color4ub( r, g, b, 255 );

						pb.Vertex3fv( (float*)&P1 );	pb.Vertex3fv( (float*)&P2 );
						pb.Vertex3fv( (float*)&P2 );	pb.Vertex3fv( (float*)&P3 );
						pb.Vertex3fv( (float*)&P3 );	pb.Vertex3fv( (float*)&P1 );

						if( mode == 3 ) {
							// Face and vertex normals
							r = mesh->material_list[fg->material].diffuse.r;
							g = mesh->material_list[fg->material].diffuse.g;
							b = mesh->material_list[fg->material].diffuse.b;
							
							P = (1.0f/3.0f) * (P1 + P2 + P3);
						
							N  = P + mesh->normal_ABC[ fg->face_normal[f_id] ];
							N1 = P1 + mesh->normal_ABC[ mesh->vertex_normal[ v_id1 ] ];
							N2 = P2 + mesh->normal_ABC[ mesh->vertex_normal[ v_id2 ] ];
							N3 = P3 + mesh->normal_ABC[ mesh->vertex_normal[ v_id3 ] ];

							pb.Vertex3fv( (float*)&P );		pb.Vertex3fv( (float*)&N );

							pb.Color4ub( 255, 0, 255, 255 );
	
							pb.Vertex3fv( (float*)&P1 );	pb.Vertex3fv( (float*)&N1 );
							pb.Vertex3fv( (float*)&P2 );	pb.Vertex3fv( (float*)&N2 );
							pb.Vertex3fv( (float*)&P3 );	pb.Vertex3fv( (float*)&N3 );
						}
					}
				}
				break;
			
			} // end of switch
		}
		else if( IM != NULL ) {

			U32 FGcount, face_count;
			
			Vector *Ps;
			U32 Pstride, Psize, Pcount;
			const U16 *Pis;

			const Vector *Ns;
			U32 Nstride, Nsize, Ncount;
			const U16 *Nis;

			const Vector * Es;
			const float *Eangles;
			U32 Estride, Esize, Ecount, num_edges;
			const U16 *Eis;

			Vector P, N, Ne1, Ne2, Ne3;

			//

			IM->get_num_facegroups( &FGcount );

			for( U32 fg = 0; fg < FGcount; fg++ ) {
				
				IM->lock_facegroup( fg, true );

				IM->lock_facegroup_data( IM_E_POINTS | IM_E_NORMALS  );
				IM->lock_facegroup_indices( IM_E_POINTS | IM_E_NORMALS );

				IM->get_facegroup_data( IM_E_POINTS, (const void**)&Ps, &Pstride, &Psize, &Pcount );
				IM->get_facegroup_indices( IM_E_POINTS, &Pis, &face_count );

				IM->get_facegroup_data( IM_E_NORMALS, (const void**)&Ns, &Nstride, &Nsize, &Ncount );
				IM->get_facegroup_indices( IM_E_NORMALS, &Nis, &face_count );

				face_count /= 3;	// get number of faces from number of indices, assumes trilists

#pragma message( "TODO: add material and edge support in RenderWireframe" )

				switch( mode ) {

				case 2:
					if( SUCCEEDED( IM->lock_facegroup_data( IM_E_EDGES ) ) &&
						SUCCEEDED( IM->lock_facegroup_indices( IM_E_EDGES ) ) )
					{
						IM->get_facegroup_data( IM_E_EDGE_ANGLES, (const void**)&Eangles, &Estride, &Esize, &Ecount );
						IM->get_facegroup_data( IM_E_EDGES, (const void**)&Es, &Estride, &Esize, &Ecount );
						IM->get_facegroup_indices( IM_E_EDGES, &Eis, &num_edges );

						num_edges /= 2;
						
						pb.Color4ub( 255, 255, 255, 255 );
						for( U32 e_id = 0; e_id < num_edges; e_id++ )
						{
							if( Eangles[e_id] > edge_angle )
							{
								const Vector &P1 = Es[ Eis[ e_id*2 + 0 ] ];
								const Vector &P2 = Es[ Eis[ e_id*2 + 1 ] ];

								pb.Vertex3fv( (float*)&P1 );	
								pb.Vertex3fv( (float*)&P2 );
							}
							else // assumes sorted edges
							{
								break;
							}
						}
					}
					else
					{
						StatusUpdate( "No edge information in mesh..." );
					}
					break;

				case 1: // Faces
				case 3: // Normals

					for( U32 f_id = 0; f_id < face_count; f_id++ ) {
							
						const Vector &P1 = Ps[ Pis[ f_id*3 + 0 ] ];
						const Vector &P2 = Ps[ Pis[ f_id*3 + 1 ] ];
						const Vector &P3 = Ps[ Pis[ f_id*3 + 2 ] ];

						const Vector &N1 = Ns[ Nis[ f_id*3 + 0 ] ];
						const Vector &N2 = Ns[ Nis[ f_id*3 + 1 ] ];
						const Vector &N3 = Ns[ Nis[ f_id*3 + 2 ] ];

						N = cross_product( P2 - P1, P3 - P1 );
						N.normalize();

						// if the face normal matches all 3 vertex normals 
						// we consider the face "flat"
						bool flat_v1 = (dot_product( N1, N ) > .98f );
						bool flat_v2 = (dot_product( N2, N ) > .98f );
						bool flat_v3 = (dot_product( N3, N ) > .98f );

						if( flat_v1 && flat_v2 && flat_v3 )
						{
							pb.Color4ub( 0, 255, 0, 255 );
						}
						else
						{
							pb.Color4ub( 255, 255, 255, 255 );
						}
						
						pb.Vertex3fv( (float*)&P1 );	pb.Vertex3fv( (float*)&P2 );
						pb.Vertex3fv( (float*)&P2 );	pb.Vertex3fv( (float*)&P3 );
						pb.Vertex3fv( (float*)&P3 );	pb.Vertex3fv( (float*)&P1 );

						if( mode == 3 ) {
							// Face and vertex normals
						
							Ne1 = P1 + N1;
							Ne2 = P2 + N2;
							Ne3 = P3 + N3;

							pb.Color4ub( 255, 0, 255, 255 );
							pb.Vertex3fv( (float*)&P1 );	pb.Vertex3fv( (float*)&Ne1 );
							pb.Vertex3fv( (float*)&P2 );	pb.Vertex3fv( (float*)&Ne2 );
							pb.Vertex3fv( (float*)&P3 );	pb.Vertex3fv( (float*)&Ne3 );
						}
					}
					break;
				
				} // end of switch

				IM->unlock_facegroup();
			}
		}

		pb.End();
	}	

	RenderPrim->set_render_state( D3DRS_ZFUNC, D3DCMP_LESS );
}

//

void RenderObjects (void)
{
	vis_state visible;

	SetRenderVolume();

	RenderPrim->set_render_state( D3DRS_ZENABLE, false );
	RenderPrim->set_render_state( D3DRS_ZFUNC, D3DCMP_LESS );
	RenderPrim->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE );

	SkipBatch = true;
	if(!SkipBatch)
	{
		RenderPrim->set_state (RPR_BATCH, true);
	}


	for( U32 i=0; i<MAX_OBJS; i++ ) {

		if( LoadedInstIndices[i] == INVALID_INSTANCE_INDEX ) {
			break;
		}

		if( Engine->get_instance_parent( LoadedInstIndices[i] ) != INVALID_INSTANCE_INDEX ) {
			continue;
		}	
		
		if( lod_fraction < 0.0f ) {
			visible = Engine->render_instance( Camera, LoadedInstIndices[i], 0, 1.0f, render_mode, NULL );
		}
		else {
			Renderer->set_render_property( NURB_PIXEL_ERROR, nurb_error );
			Renderer->set_render_property( BEZIER_SUBDIV_CNT, bezier_sub );
			visible = Engine->render_instance( Camera, LoadedInstIndices[i], 0, lod_fraction, render_mode, NULL );
		}
	}

	if(!SkipBatch)
	{
		RenderPrim->set_state (RPR_BATCH, false);
		RenderPrim->flush( RPR_TRANSLUCENT_DEPTH_SORTED_ONLY|RPR_TRANSLUCENT_UNSORTED_ONLY );
		RenderPrim->set_render_state( D3DRS_ZENABLE, TRUE );
		RenderPrim->set_render_state( D3DRS_ZWRITEENABLE, TRUE );
	}
}

//

void DrawHelp( void )
{
	static ACCEL keys[256];

	if( !num_help_keys ) {
		ASSERT( CopyAcceleratorTable( hKeyBindings, NULL, 0 ) < 256 );
		num_help_keys = CopyAcceleratorTable( hKeyBindings, keys, 256 );
	}

	SetRender2D();
	Font.SetSize( .125 );

	float x = 10.0f;
	float y = 70.0f;
	char  buffer[MAX_PATH], keystring[MAX_PATH];

	int stop = _MIN<int>( draw_help+MAX_HELP_LINES_PER_PAGE, num_help_keys );

	for( int key=draw_help; key<stop; key++ ) {

		if( !LoadString( GetModuleHandle(NULL), keys[key].cmd, buffer, MAX_PATH ) ) {
			strcpy( buffer, "Does something! (no help?)" );
		}

		if( keys[key].fVirt & FVIRTKEY ) {
			strcpy( keystring, VirtualKeyStrings[keys[key].key&0x7F] );
		}
		else {
			keystring[0] = keys[key].key;
			keystring[1] = 0;
		}

		Font.RenderFormattedString( x, y, "%s%s%s%s%s", 
										  (keys[key].fVirt & FALT)? "[Alt]" : "",
										  (keys[key].fVirt & FCONTROL)? "[Ctrl]" : "",
										  (keys[key].fVirt & FSHIFT)? "[Shift]" : "",
										  (keys[key].fVirt & (FSHIFT|FALT|FCONTROL))? "-" : "",
										   keystring
										  );

		Font.RenderString( x+140, y, buffer );

		y += 24.0f;
	}
}

//

void RenderFrameInfo( SINGLE dt )
{
#define FPS_NUM_FRAMES_AVG 32

	static int fid = 0;
	static SINGLE dt_32 = FPS_NUM_FRAMES_AVG * dt;
	static SINGLE dt_buf[FPS_NUM_FRAMES_AVG] = {dt,dt,dt,dt, dt,dt,dt,dt,
												dt,dt,dt,dt, dt,dt,dt,dt,
												dt,dt,dt,dt, dt,dt,dt,dt,
												dt,dt,dt,dt, dt,dt,dt,dt};

	dt_32 += dt;
	dt_buf[fid] = dt;
	fid = (fid + 1) % FPS_NUM_FRAMES_AVG;
	dt_32 -= dt_buf[fid];

	SINGLE fps = (dt_32 > 0.0f) ? FPS_NUM_FRAMES_AVG / dt_32 : -1.0f;

	SetRender2D();
	Font.SetSize( .125 );
	Font.RenderFormattedString( 0, 20, "FPS % 5.1f", fps );

	if( LoadedInstCount ) {
		Font.RenderFormattedString( 0, 40, "Dist %.2f",
			(Camera->get_position() - Engine->get_position(LoadedInstIndices[0])).magnitude());
	}
}

//

void SetInitialCameraPosition (void)
{
	ASSERT (Engine);

	Matrix o;
	o.set_identity ();

	Camera->set_orientation( o );
	Camera->set_position( o.get_k () * CameraStartDistance );
}

//

void SetPixelThreshold (void)
{
	if (Renderer)
	{
		Renderer->set_render_property( SUB_PIXEL_THRESHOLD, pixel_threshold);
	}
}

//

void SetCameraStartDistance (void)
{
	float max_radius;
	Vector center;

	Engine->get_instance_bounding_sphere( LoadedInstIndices[0], 0, &max_radius, &center );
	max_radius += center.magnitude();

	if (max_radius > 0.0f)
	{
		CameraStartDistance = __max(2.0f * max_radius, 2.5f * CameraNearClippingPlane); 
	}
}

//

void SetCameraZoomRate (void)
{
	CameraZoomRate = CameraStartDistance / 10.0f;
}

//

void SetTranslationRate (void)
{
	TransRate = CameraZoomRate;
}

//

void MoveCamera (const Quaternion& q)
{
	Matrix cam_ornt (Camera->get_orientation());

	cam_ornt = q * cam_ornt;

	Camera->set_orientation(cam_ornt);

	if(circle_camera)
	{
		const float dist = (Camera->get_position()).magnitude();
		Camera->set_position(cam_ornt.get_k() * dist);
	}
}

//

void UpdateCamera (float dt)
{

	if (In)
	{
		//don't let the camera go past the center of the object

		Vector dist (Camera->get_position());

		Matrix cam_ornt (Camera->get_orientation());

		Vector translation (-(Camera->get_orientation().get_k ()) * dt * .9 * CameraZoomRate);

		if (dist.magnitude () > translation.magnitude ())
		{
			Vector cam_pos (Camera->get_position());
			cam_pos += translation;

			Camera->set_position(cam_pos);
		}
	}
	else if (Out)
	{
		Vector cam_pos (Camera->get_position());

		Vector translation (Camera->get_orientation().get_k () * dt * 1.1 * CameraZoomRate);

		cam_pos += translation;

		Camera->set_position(cam_pos);
	}

	// rotate
	if ((Up || Down) && !ShiftDown)
	{
		Quaternion q (Camera->get_orientation().get_i (), ((Up) ? -CameraRotationRate : CameraRotationRate) * dt);
		MoveCamera (q);
	}

	if (RightDown  && !ShiftDown)
	{
		if (CtrlDown)
		{
			Quaternion q (-(Camera->get_orientation().get_k ()), -CameraRotationRate * dt);
			MoveCamera (q);
		}
		else
		{
			Quaternion q (Camera->get_orientation().get_j (), -CameraRotationRate * dt);
			MoveCamera (q);
		}
	}

	if (LeftDown && !ShiftDown)
	{
		if (CtrlDown)
		{
			Quaternion q (-(Camera->get_orientation().get_k ()), CameraRotationRate * dt);
			MoveCamera (q);
		}
		else
		{
			Quaternion q (Camera->get_orientation().get_j (), CameraRotationRate * dt);
			MoveCamera (q);
		}
	}

	// pan
	if (Up && ShiftDown)
	{
		Camera->set_position(-TransRate * dt * Camera->get_orientation().get_j() + 
			                 Camera->get_position());
	}

	if(Down && ShiftDown)
	{
		Camera->set_position(TransRate * dt * Camera->get_orientation().get_j() + 
			                 Camera->get_position());
	}

	if(RightDown && ShiftDown)
	{
		Camera->set_position(-TransRate * dt * Camera->get_orientation().get_i() + 
			                 Camera->get_position());
	}

	if(LeftDown && ShiftDown)
	{
		Camera->set_position(TransRate * dt * Camera->get_orientation().get_i() + 
			                 Camera->get_position());
	}

}

//

void ProcessRequests (float dt)
{
	UpdateCamera (dt);

	if (JointMotionChange)
		UpdateJointMotionChangeRate (ShiftDown, dt);

	if (ExerciseJoints)
	{
		for (INSTANCE_INDEX* i = LoadedInstIndices; i < LoadedInstIndices + MAX_OBJS; i++)
			if (*i != INVALID_INSTANCE_INDEX)
				DoExerciseJoints (*i, dt);
	}

	if (SpinJ)
	{
		Quaternion q (Vector (0, SpinJ, 0).normalize (), SpinRate * dt);
		Engine->set_orientation (LoadedInstIndices[0], Matrix (q) * Engine->get_orientation (LoadedInstIndices[0]));
	}

	if (SpinI)
	{
		Quaternion q (Vector (SpinI, 0, 0).normalize (), SpinRate * dt);
		Engine->set_orientation (LoadedInstIndices[0], Matrix (q) * Engine->get_orientation (LoadedInstIndices[0]));
	}

	if (SpinK)
	{
		Quaternion q (Vector (0, 0, SpinK).normalize (), SpinRate * dt);
		Engine->set_orientation (LoadedInstIndices[0], Matrix (q) * Engine->get_orientation (LoadedInstIndices[0]));
	}

	if(TransI){
		Engine->set_position(LoadedInstIndices[0],
			Vector(TransI * TransRate * dt, 0, 0) + Engine->get_position(LoadedInstIndices[0]));
	}

	if(TransJ){
		Engine->set_position(LoadedInstIndices[0],
			Vector(0, TransJ * TransRate * dt, 0) + Engine->get_position(LoadedInstIndices[0]));
	}

	if(TransK){
		Engine->set_position(LoadedInstIndices[0],
			Vector(0, 0, TransK * TransRate * dt) + Engine->get_position(LoadedInstIndices[0]));
	}
}

//

void ParseCommandLine ( const char *_cmd_line )
{
	U32 ftl = 0;
	bool done;
	char *cmd_line, *arg;

	if( strlen( _cmd_line ) == 0 ) {
		return;
	}

	cmd_line = strdup( _cmd_line );
	ASSERT( cmd_line );

	if( (arg = strtok( cmd_line, " \t" )) == NULL ) {
		return;
	}

	done = false;
	do {
		if( strnicmp( arg, "-i", 2 ) == 0 ) {
		}
		else if( strnicmp( arg, "-e", 2 ) == 0 ) {

			Err_SetLogEnable( true );

			if( strlen( arg ) > 2 ) {
				Err_SetLogFilename( &arg[2] );
			}
			else if( (arg = strtok( NULL, " \t" )) != NULL ) {
				Err_SetLogFilename( arg );
			}
			else {
				done = true;
				continue;
			}
		}else if( strnicmp( arg, "-i", 2 ) == 0 ) {
			continue;
		}
		else if( strnicmp( arg, "-t", 2 ) == 0 ) {
			if( strlen( arg ) > 2 ) {
				strcpy( PreloadTextureFile, &arg[2] );
			}
			else if( (arg = strtok( NULL, " \t" )) != NULL ) {
				strcpy( PreloadTextureFile, arg );
			}
			else {
				done = true;
				continue;
			}
		}
		else {
			char	*opq, *clq;

			opq	=strchr(arg, '\"');
			if(opq)	//check for quotes
			{
				//quotes found, aim at original cmdline
				opq	=strchr(_cmd_line, '\"');

				ASSERT(opq);	//shouldn't happen
				
				clq	=strchr(opq + 1, '\"');
				if(clq)	//check for trailing quotes
				{
					//grab out the string
					memcpy(FilesToLoad[ftl++], opq + 1, clq - opq - 1);

					//advance strtok to the next arg
					arg	=strtok(NULL, FilesToLoad[ftl - 1]) + 1;
				}
				else	//no matching quotes
				{
					GENERAL_TRACE_1("Filename without matching quotes!\n");
					free(cmd_line);
					return;
				}
			}
			else
			{
				strcpy( FilesToLoad[ftl++], arg );
			}
		}
	}
	while( !done && (arg = strtok( NULL, " \t" )) != NULL );

	free( cmd_line );

}

//

void SetSearchPath (const char* name)
{
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char path[MAX_PATH * 4] = {0};

	if (name)
	{
		_splitpath (name, drive, dir, NULL, NULL);
		_makepath (path, drive, dir, NULL, NULL);
	}

	if (PathFromINI[0])
	{
		if (name)
			strcat (path, ";");

		strcat (path, PathFromINI);
	}

	//this call trashes current search path

	if (path[0])
		Engine->set_search_path (path);
}

//

//

void HandleCommand( U32 command )
{
	static bool help_describe_next = false;

	char buffer[MAX_PATH];
	bool loop_anim = false;

	// This handles the "describe next command" command 
	//
	if( help_describe_next ) {
	
		if( LoadString( GetModuleHandle(NULL), command, buffer, MAX_PATH ) ) {
			StatusUpdate( "Command: %s", buffer );
		}
		else {
			StatusUpdate( "Command: Sorry. No help available" );
		}

		help_describe_next = false;
		
		return;
	}

	// Actually process commands
	//

	switch( command ) {

	case IDC_HELP_DESCRIBE:
		StatusUpdate( "Waiting for keystroke to describe..." );
		help_describe_next = true;
		break;

	case IDC_HELP_NEXT:
		if( draw_help == 0 ) {
			draw_help = 1;
		}
		else {
			draw_help += MAX_HELP_LINES_PER_PAGE;
			if( draw_help > num_help_keys ) {
				draw_help = 1;
			}
		}
		break;

	case IDC_HELP_PREV:
		if( draw_help > MAX_HELP_LINES_PER_PAGE ) {
			draw_help -= MAX_HELP_LINES_PER_PAGE;
		}
		else {
			draw_help = 0;
		}
		break;

	case IDC_HELP_DISABLE:
		draw_help = 0;
		break;

	case IDC_SCENE_CAPTURE_TRACE_TO_FILE:
		ird_capture_scene = 1;
		break;

	case IDC_SCENE_CAPTURE_TO_FILE:
		ird_capture_screen_shot = true;
		break;

		// Stat display controls
		//
	case IDC_DISPLAY_GEOMETRY_STATS:
		draw_fc_vc = !draw_fc_vc;
		break;

	case IDC_DISPLAY_RENDER_STATS:
		draw_stats = !draw_stats;
		break;

		// Display mode control fullscreen
		//
	case IDC_DISPLAYMODE_FULLSCREEN_TOGGLE:
		Fullscreen = !Fullscreen;
		SetDisplayMode( DisplayWidth, DisplayHeight, DisplayDepth, Fullscreen );
		StatusUpdate( "Mode %dx%d %s", DisplayWidth, DisplayHeight, Fullscreen?"Fullscreen":"Windowed" );
		break;

	
		// Reset camera or object to default position, orientation, etc
		//
	case IDC_CAMERA_POSE_DEFAULT:
	case IDC_OBJECT_POSE_DEFAULT:

		if( command == IDC_OBJECT_POSE_DEFAULT ) {

			for( INSTANCE_INDEX* i = LoadedInstIndices; i < LoadedInstIndices + MAX_OBJS; i++ ) {
				if( *i != INVALID_INSTANCE_INDEX ) {
					ResetJoints ( *i );
				}
			}

			Matrix o;
			o.set_identity ();
			
			Engine->set_orientation (LoadedInstIndices[0], o);
			Engine->set_position (LoadedInstIndices[0], Vector (0, 0, 0));
		}
		else {
			SetInitialCameraPosition ();
		}

		SpinI = 0;
		SpinJ = 0;
		SpinK = 0;

		TransI = 0;
		TransJ = 0;
		TransK = 0;

		ExerciseJoints = false;
		
		lod_fraction = -1.0f;
		lod_delta = .01f;

		nurb_error = 1.0f;
		nurb_delta = 0.05f;

		Camera->set_Horizontal_FOV( CameraHFov );
		Camera->set_Horizontal_to_vertical_aspect( CameraHVAspect );

		SetCameraZoomRate ();

		StatusUpdate( "Reset camera to default pose" );
		break;

	case IDC_OBJECT_ROTATE_STOP:
		SpinI = 0;
		SpinJ = 0;
		SpinK = 0;
		break;

		// Object unload/reload/load
		//
	case IDC_OBJECT_FLUSH:
	case IDC_OBJECT_LOAD_SAME:
	case IDC_OBJECT_LOAD_NEW:

		ObjectList_Unload();

		if( command == IDC_OBJECT_LOAD_NEW ) {
			for( U32 ftl=0; ftl<MAX_FTL; ftl++ ) {
				FilesToLoad[ftl][0] = 0;
			}
		}
		
		if( command != IDC_OBJECT_FLUSH ) {
			ObjectList_Load();
			lod_fraction = -1.0f;
		}
		break;

	case IDC_OBJECT_CONNECT:
		HardpointList_Connect();
		break;

		// Object rendering control
		//
	case IDC_OBJECT_DISABLE:
		draw_object = !draw_object;
		StatusUpdate( "Disable object rendering" );
		break;

	case IDC_OBJECT_TRANSLATION_RATE_NEXT:
		TransRate *= 1.1f;
		StatusUpdate( "Object Translation Rate: %5.3f", TransRate );
		break;

	case IDC_OBJECT_TRANSLATION_RATE_PREV:
		TransRate *= 0.9f;
		StatusUpdate( "Object Translation Rate: %5.3f", TransRate );
		break;

	case IDC_OBJECT_LOD_STEP_NEXT:
		lod_delta *= .9f;
		if(lod_delta > 1.0f)
		{
			lod_delta = 1.0f;
		}

		nurb_delta *= .9f;
		break;

	case IDC_OBJECT_LOD_NEXT: // PgDown
		render_mode &= ~RF_RELATIVE_LOD;
		if( lod_fraction < 0.0f ) {
			lod_fraction = 1.0f;
		}
		else {
			lod_fraction = (lod_fraction - lod_delta) > 0.0f ? (lod_fraction - lod_delta) : 0.0f;
		}

		nurb_error *= 1.1f;
		bezier_sub = _MAX(bezier_sub - 1, 1);
		break;

	case IDC_OBJECT_LOD_STEP_PREV:
		lod_delta *= 1.1f;
		if( lod_delta > 1.0f )
		{
			lod_delta = 1.0f;
		}

		nurb_delta *= 1.1f;
		
		break;

	case IDC_OBJECT_LOD_PREV: // PgUp
		render_mode &= ~RF_RELATIVE_LOD;
		if( lod_fraction < 0.0f ) {
			lod_fraction = 1.0f;
		}
		else {
			lod_fraction = (lod_fraction + lod_delta) < 1.0f ? (lod_fraction + lod_delta) : 1.0f;
		}

		nurb_error *= .9f;
		bezier_sub++;
		break;

	#ifdef LIGHT
	case ID_LIGHTS_AMBIENT_RESETTODEFAULT:
		AmbientLight_Reset();
		break;

	case ID_LIGHTS_AMBIENT_NEXT:
		{
			float r,g,b;
			AmbientLight_Get( &r, &g, &b );
			AmbientLight_Set( r+0.10, g+0.10, b+0.10 );
		}
		break;

	case ID_LIGHTS_AMBIENT_PREV:
		{
			float r,g,b;
			AmbientLight_Get( &r, &g, &b );
			AmbientLight_Set( r-0.10, g-0.10, b-0.10 );
		}
		break;

	case ID_LIGHTS_AMBIENT_SELECTCOLOR:
//		AmbientLight_SelectColor();
		break;

	case ID_LIGHTS_AMBIENT_FULLINTENSITY:
		AmbientLight_Set( 1.0f, 1.0f, 1.0f );
		break;

	case ID_LIGHTS_IRP_LIGHTING_ENABLED:
		use_irp_lighting = !use_irp_lighting;
		break;

	case ID_LIGHTS_BUILTIN_ENABLED:

		builtin_light_enabled = !builtin_light_enabled;

		if( builtin_light_enabled && !BuiltinLight_IsValid() ) {
			BuiltinLight_Create( 1.0f, 1.0f, 1.0f, LightRangeScale * CameraStartDistance );	
		}

		BuiltinLight_Enable( builtin_light_enabled );
		break;

	case ID_LIGHTS_BUILTIN_ORBIT:
		builtin_light_orbit = !builtin_light_orbit;
		BuiltinLight_EnableOrbit( builtin_light_orbit );
		break;

	case ID_LIGHTS_BUILTIN_SELECTCOLOR:
		BuiltinLight_SelectColor();		
		break;

	case ID_LIGHTS_BUILTIN_ATTACHEDTOCAMERA:
		BuiltinLight_Detach();		
		BuiltinLight_Attach( Camera->index );		
		break;

	case ID_LIGHTS_BUILTIN_RESET:
		BuiltinLight_Destroy();
		BuiltinLight_Create( 1.0f, 1.0f, 1.0f, LightRangeScale * CameraStartDistance );	
		break;

	case ID_LIGHTS_BUILTIN_RENDER:
		draw_builtin_light = !draw_builtin_light;
		break;

	case ID_LIGHTS_EXPORTED_RENDER:
		draw_light_list = !draw_light_list;
		break;
	#endif

		// Hardpoint rendering control
		//
	#ifdef HARDPOINTS

	case IDC_HARDPOINT_NEXT:
		draw_hp = (draw_hp & HIGH_BIT) | ( ((draw_hp & ~HIGH_BIT) + 1) % (HardpointList_GetCount() + 2) );
		break;

	case IDC_HARDPOINT_PREV:
		draw_hp = (draw_hp & HIGH_BIT) | ( ((draw_hp & ~HIGH_BIT) + (HardpointList_GetCount()+1)) % (HardpointList_GetCount() + 2) );
		break;

	case IDC_HARDPOINT_DETAILED:
		draw_hp ^= HIGH_BIT;
		break;

	#endif

		// Extents rendering control
		//
	#ifdef DRAWEXTENTS
				
	case IDC_EXTENT_NEXT:
		draw_extent++;
		break;

	case IDC_EXTENT_PREV:
		if( draw_extent > 0 ) {
			draw_extent--;
		}
		break;

	case IDC_EXTENT_DETAILED:
		draw_extent = draw_extent ^ HIGH_BIT;
		break;

	#endif

		// Joint related commands
		//
	case IDC_JOINT_RATE_NEXT:
	case IDC_JOINT_RATE_PREV:
		JointMotionChange = !JointMotionChange;
		break;

	case IDC_JOINT_TOGGLE:
		ExerciseJoints = !ExerciseJoints;
		StatusUpdate( "%sxercising joints", ExerciseJoints? "E" : "Not e" );
		break;

	case IDC_JOINT_MINIMA:
		{
			for( INSTANCE_INDEX* i = LoadedInstIndices; i < LoadedInstIndices + MAX_OBJS; i++ ) {
				if( *i != INVALID_INSTANCE_INDEX ) {
					SetJointsToExtreme (*i, true);
				}
			}

			StatusUpdate( "Set joints to minima" );
		}
		break;

	case IDC_JOINT_MAXIMA:
		{
			for( INSTANCE_INDEX* i = LoadedInstIndices; i < LoadedInstIndices + MAX_OBJS; i++ ) {
				if( *i != INVALID_INSTANCE_INDEX ) {
					SetJointsToExtreme (*i, false);
				}
			}

			StatusUpdate( "Set joints to maxima" );
		}
		break;
		
		// Coordinate frame axes rendering
		//
	case IDC_AXIS_NEXT:
		draw_axis = (draw_axis & HIGH_BIT) | ( ((draw_axis & ~HIGH_BIT) + 1) % (LoadedInstCount + 2) );
		break;

	case IDC_AXIS_PREV:
		draw_axis = (draw_axis & HIGH_BIT) | ( ((draw_axis & ~HIGH_BIT) + (LoadedInstCount+1)) % (LoadedInstCount + 2) );
		break;

	case IDC_AXIS_DETAILED:
		draw_axis = draw_axis ^ HIGH_BIT;
		break;

		// Wireframe rendering
		//
	case IDC_WIREFRAME_NEXT:
		draw_wire = (draw_wire & HIGH_BIT) | ( ((draw_wire & ~HIGH_BIT) + 1) % 4 );
		break;
	
	case IDC_WIREFRAME_PREV:
		draw_wire = (draw_wire & HIGH_BIT) | ( ((draw_wire & ~HIGH_BIT) + 3) % 4 );
		break;
	
	case IDC_WIREFRAME_DETAILED:
		draw_wire = draw_wire ^ HIGH_BIT;
		break;

		// Edge angle control for wireframe rendering
		//
	case IDC_EDGE_ANGLE_NEXT:
		edge_angle += 3.0f * MUL_DEG_TO_RAD;

		if( edge_angle > 180.0f * MUL_DEG_TO_RAD ) {
			edge_angle = 180.0f * MUL_DEG_TO_RAD;
		}
		break;

	case IDC_EDGE_ANGLE_PREV:
		edge_angle -= 3.0f * MUL_DEG_TO_RAD;

		if( edge_angle < 0.0f ) {
			edge_angle = 0.0f;
		}
		break;
		
		// Texture rendering
		//
	case IDC_TEXTURE_NEXT:
		draw_texture = (draw_texture & HIGH_BIT) | ( ((draw_texture & ~HIGH_BIT) + 1) % (texture_count + 1) );
		break;
	
	case IDC_TEXTURE_PREV:
		draw_texture = (draw_texture & HIGH_BIT) | ( ((draw_texture & ~HIGH_BIT) + (texture_count)) % (texture_count + 1) );
		break;
	
	case IDC_TEXTURE_DETAILED:
		draw_texture = draw_texture ^ HIGH_BIT;
		break;
	
	case IDC_TEXTURE_DISABLE:
		draw_texture = 0;
		break;

		// Material rendering
		//
	case IDC_MATERIAL_NEXT:
		draw_material = (draw_material & HIGH_BIT) | ( ((draw_material & ~HIGH_BIT) + 1) % (material_count + 1) );
		break;
	
	case IDC_MATERIAL_PREV:
		draw_material = (draw_material & HIGH_BIT) | ( ((draw_material & ~HIGH_BIT) + (material_count)) % (material_count + 1) );
		break;
	
	case IDC_MATERIAL_DETAILED:
		draw_material ^= HIGH_BIT;
		break;
	
	case IDC_MATERIAL_DISABLE:
		draw_material = 0;
		break;

		// Camera control
		//
	case IDC_CAMERA_SELECT_NEXT:
		AttachCamera( true );
		break;

	case IDC_CAMERA_SELECT_PREV:
		AttachCamera( true );
		break;

	case IDC_CAMERA_SELECT_TOGGLE:
		AttachCamera( false );
		break;

	case IDC_CAMERA_TRANSLATION_RATE_NEXT:
		CameraZoomRate *= 1.1f;
		break;

	case IDC_CAMERA_TRANSLATION_RATE_PREV:
		CameraZoomRate *= 0.9f;
		break;

	case IDC_CAMERA_FOV_DEC:
		{
			float fovx = .9f * 2.0f * Camera->get_fovx();
			if(fovx < 1.0f) // Deg
			{
				fovx = 1.0f;
			}
			Camera->set_Horizontal_FOV( fovx );
			Camera->set_Horizontal_to_vertical_aspect( CameraHVAspect );

			StatusUpdate( "Camera HFOV: %5.3f", fovx );
		}
		break;

	case IDC_CAMERA_FOV_INC:
		{
			float fovx = 1.1f * 2.0f * Camera->get_fovx();
			if(fovx > 179.9f)
			{
				fovx = 179.9f;
			}
			Camera->set_Horizontal_FOV( fovx );
			Camera->set_Horizontal_to_vertical_aspect( CameraHVAspect );

			StatusUpdate( "Camera HFOV: %5.3f", fovx );
		}
		break;

		// Light controls
		//
		// Animation related commands
		//
	#ifdef ANIMATION

	case IDC_ANIM_0_START_LOOP:	
	case IDC_ANIM_1_START_LOOP:	
		
		loop_anim = true;		

		// NOTE lack of break;

	case IDC_ANIM_0_START:
	case IDC_ANIM_1_START:
	case IDC_ANIM_2_START:
	case IDC_ANIM_3_START:
	case IDC_ANIM_4_START:
	case IDC_ANIM_5_START:
	case IDC_ANIM_6_START:
	case IDC_ANIM_7_START:
	case IDC_ANIM_8_START:
	case IDC_ANIM_9_START:
		
		if( loop_anim )
		{
			StartAnimation( command-IDC_ANIM_0_START_LOOP, loop_anim );
		}
		else
		{
			StartAnimation( command-IDC_ANIM_0_START, loop_anim );
		}
		
		break;

	case IDC_ANIM_0_STOP:
	case IDC_ANIM_1_STOP:
	case IDC_ANIM_2_STOP:
	case IDC_ANIM_3_STOP:
	case IDC_ANIM_4_STOP:
	case IDC_ANIM_5_STOP:
	case IDC_ANIM_6_STOP:
	case IDC_ANIM_7_STOP:
	case IDC_ANIM_8_STOP:
	case IDC_ANIM_9_STOP:

		StopAnimation( command-IDC_ANIM_0_START-1 );
		
		break;
	#endif

	case IDC_SCALE_DOWN:
		global_scale *= .9f;
		break;
	case IDC_SCALE_UP:
		global_scale *= 1.1f;
		break;
	}
}

//

void KeyUp (HWND window, WPARAM wparam, LPARAM lparam)
{
	switch( wparam ) {
		case VK_SHIFT:		ShiftDown = false;	break;
		case VK_CONTROL:	CtrlDown = false;	break;
		case VK_ADD:		In = false;			break;
		case VK_SUBTRACT:	Out = false;		break;
		case VK_LEFT:		LeftDown = false;	break;
		case VK_RIGHT:		RightDown = false;	break;
		case VK_UP:			Up = false;			break;
		case VK_DOWN:		Down = false;		break;
		case 'I':			SpinI = 0;			break;
		case 'J':			SpinJ = 0;			break;
		case 'K':			SpinK = 0;			break;
		case 'X':			TransI = 0;			break;
		case 'Y':			TransJ = 0;			break;
		case 'Z':			TransK = 0;			break;
	}
}

//

void KeyDown( HWND window, WPARAM wparam, LPARAM lparam )
{
	switch( wparam ) {

		case VK_SHIFT:		ShiftDown = true;		break;
		case VK_CONTROL:	CtrlDown = true;		break;
		case VK_ADD:		In = true;				break;
		case VK_SUBTRACT:	Out = true;				break;
		case VK_LEFT:		LeftDown = true;		break;
		case VK_RIGHT:		RightDown = true;		break;
		case VK_UP:			Up = true;				break;
		case VK_DOWN:		Down = true;			break;
		case 'I':
			if( CtrlDown ) 	SpinI = -1;
			else			SpinI = 1;
			break;
		case 'J':
			if( CtrlDown ) 	SpinJ = -1;
			else			SpinJ = 1;
			break;
		case 'K':
			if( CtrlDown ) 	SpinK = -1;
			else			SpinK = 1;
			break;
		case 'X':
			if (CtrlDown)	TransI = -1;
			else			TransI = 1;
			break;
		case 'Y':
			if (CtrlDown)	TransJ = -1;
			else			TransJ = 1;
			break;
		case 'Z':
			if (CtrlDown)	TransK = -1;
			else			TransK = 1;
			break;
	}
}

//

long WINAPI WndProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{

	case WM_CREATE:
		DragAcceptFiles( hWindow, TRUE );
		break;

	case WM_DROPFILES:
		{
			HDROP hdrop = (HDROP)wParam;
			U32 file_count = DragQueryFile( hdrop, 0xFFFFFFFF, NULL, 0 ); 

			if( file_count ) {

				ObjectList_Unload();

				for( U32 ftl=0; ftl<MAX_FTL; ftl++ ) {
					if( ftl < file_count ) {
						DragQueryFile( hdrop, ftl, FilesToLoad[ftl], MAX_PATH );
					}
					else {
						FilesToLoad[ftl][0] = 0;
					}
				}
				
				ObjectList_Load();
				lod_fraction = -1.0f;
			}
		}
		break;

	case WM_COMMAND:
		HandleCommand( LOWORD(wParam) );
		break;

	case WM_KEYUP:
		KeyUp (hWindow, wParam, lParam);
		break;

	case WM_KEYDOWN:
		KeyDown (hWindow, wParam, lParam);
		break;

	case WM_RBUTTONDOWN:
		{
			HMENU pm = CreatePopupMenu();
			HMENU hm = LoadMenu( GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MENU) );
			AppendMenu( pm, MF_POPUP, (int)hm, 0 );

			POINT p;
			p.x = LOWORD(lParam);
			p.y = HIWORD(lParam);
			ClientToScreen( hWindow, &p );

			TrackPopupMenu( hm,
							0,
							p.x,
							p.y,
							0,
							hWindow,
							NULL );

			DestroyMenu( hm );
			DestroyMenu( pm );
		}
		break;

	case WM_CLOSE:
		ObjectList_Unload();
		PostQuitMessage (0);
		return 0;
	
	case WM_ACTIVATEAPP:
		AppActive = wParam;
		break;

	}  // end switch (message)

	return DefWindowProc(hWindow, message, wParam, lParam);
}

//

void ParseINIFile (void)
{
	COMPTR<IProfileParser> parser;
	ICOManager *DACOM;
	float ambient;
	Vector clear;

	DACOM = DACOM_Acquire();

	if( SUCCEEDED( DACOM->QueryInterface( IID_IProfileParser, parser ) ) ) {

		opt_get_string( DACOM, parser, AppName, "PATH", "", PathFromINI, MAX_PATH*3 );
		opt_get_string( DACOM, parser, AppName, "TextureFile", "", PreloadTextureFile, MAX_PATH );
		
		opt_get_u32( DACOM, parser, AppName, "Window", TRUE, &Fullscreen );
		opt_get_u32( DACOM, parser, AppName, "DisplayWidth", 640, &DisplayWidth );
		opt_get_u32( DACOM, parser, AppName, "DisplayHeight", 480, &DisplayHeight );
		opt_get_u32( DACOM, parser, AppName, "DisplayDepth", DEPTH_COOKIE, &DisplayDepth );

		opt_get_vector( DACOM, parser, AppName, "ClearColor", Vector(0.0f,0.0f,0.3f), clear );
		opt_get_float( DACOM, parser, AppName, "ClearRed", clear.x, &ClearRed );
		opt_get_float( DACOM, parser, AppName, "ClearGreen", clear.y, &ClearGreen );
		opt_get_float( DACOM, parser, AppName, "ClearBlue", clear.z, &ClearBlue );

		opt_get_float( DACOM, parser, AppName, "JointChangeRate", 20.0f, &JointChangeRate );
		opt_get_float( DACOM, parser, AppName, "SpinRate", 35.0f, &SpinRate );
		opt_get_float( DACOM, parser, AppName, "PixelArea", 0.5f, &pixel_threshold );
		opt_get_u32( DACOM, parser, AppName, "SkipBatch", false, &SkipBatch );

		opt_get_float( DACOM, parser, AppName, "DefaultAmbientLight", 0.5f, &ambient );

		opt_get_float( DACOM, parser, AppName, "FarClip", 1e4, &CameraFarClippingPlane );
		opt_get_float( DACOM, parser, AppName, "NearClip", 1.0f, &CameraNearClippingPlane );
		opt_get_float( DACOM, parser, AppName, "HorizFov", 90.0f, &CameraHFov );
		opt_get_float( DACOM, parser, AppName, "HVAspect", 4.0f/3.0f, &CameraHVAspect );

		opt_get_u32( DACOM, parser, AppName, "PromptIfNotFound", false, &PromptIfNotFound );

		opt_get_u32( DACOM, parser, AppName, "RDTraceStartup", 0, &ird_capture_startup );
		opt_get_string( DACOM, parser, AppName, "RDTraceOutputDir", "c:\\ird_capture", ird_capture_output_dir, MAX_PATH );

		Fullscreen = !Fullscreen;

		JointChangeRate *= MUL_DEG_TO_RAD;
		SpinRate *= MUL_DEG_TO_RAD;

		AmbientLight_SetDefault( ambient, ambient, ambient );
	}
}
//

void CleanUp (void)
{
	HardpointList_Clear();

#ifdef ANIMATION
	ReleaseAnimationComponent ();
#endif

	Font.SetRenderPipeline( NULL );

	pb.SetIRenderPrimitive( NULL );
	pb.SetPipeline( NULL );

	BuiltinLight_Destroy();
	LightList_Clear();

	ObjectList_Unload();

	IVertexBufferManager *IVBM = NULL;
	if( System && SUCCEEDED( System->QueryInterface( IID_IVertexBufferManager, (void**)&IVBM ) ) ) {
		IVBM->cleanup();
	}
	DACOM_RELEASE( IVBM );


	if( MaterialLib ) {
		MaterialLib->free_library();
	}

	if( TextureLib ) {
		TextureLib->free_library( FALSE );
	}

	DACOM_RELEASE( Camera );
	DACOM_RELEASE( RenderPrim );
	DACOM_RELEASE( RenderPipe );
	DACOM_RELEASE( TextureLib );
	DACOM_RELEASE( MaterialLib );
	DACOM_RELEASE( LightMan );
	DACOM_RELEASE( Renderer );
	DACOM_RELEASE( Engine );
	DACOM_RELEASE( System );

	DA_HEAP_PRINT( HEAP );

 	DACOM_Acquire()->ShutDown();

	CoUninitialize();
}

//

int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{

	ICOManager *DACOM = NULL;

	atexit(CleanUp);

#ifdef DA_HEAP_ENABLED
	InitializeDAHeap( 0x1000000, 0x800000, DAHEAPFLAG_DEBUGFILL_SNAN|DAHEAPFLAG_GROWHEAP );
#endif

	DACOM = DACOM_Acquire();
	ASSERT( DACOM );

	Err_Initialize();

	if( FAILED( GetProfileInfo( AppName, MAX_PATH, IniFile, MAX_PATH ) ) ) {
		GENERAL_TRACE_1( "No .INI profile found, exiting...\n" );
		return 0;
	}

	ParseCommandLine( lpCmdLine );

	if( FAILED( DACOM->SetINIConfig( IniFile ) ) ) {
		GENERAL_TRACE_1( "Unable to open .INI profile, exiting...\n" );
		return 0;
	}

	ParseINIFile();

	// Create System Container for system components
	// and load the system components out of the [System] section
	// of the profile.
	//
	AGGDESC adesc = "ISystemContainer";
	if( FAILED( DACOM->CreateInstance( &adesc, (void **) &System ) ) ) {
		GENERAL_TRACE_1 ("Failed to create System provider\n");
		return 0;
	}

	System->LoadSystemComponents();

	// init MS COM for iigloo
	//
	CoInitialize(NULL);

	// Get necessary System components
	//
	if( FAILED( System->QueryInterface ( IID_IRenderPipeline, (void**)&RenderPipe ) ) ) {
		GENERAL_TRACE_1 ("Failed to find IID_IRenderPipeline\n");
		return 0;
	}

	if( FAILED( System->QueryInterface ( IID_IRenderPrimitive, (void**)&RenderPrim ) ) ) {
		GENERAL_TRACE_1 ("Failed to find IID_IRenderPrimitive\n");
		return 0;
	}

	if( FAILED( System->QueryInterface ( IID_ITextureLibrary, (void**) &TextureLib ) ) ) {
		GENERAL_TRACE_1 ("Failed to find IID_ITextureLibrary\n");
		return 0;
	}
		
	if( FAILED( System->QueryInterface ( IID_ILightManager, (void**) &LightMan ) ) ) {
		GENERAL_TRACE_1 ("Failed to find IID_ILightManager\n");
		return 0;
	}

	if( FAILED( System->QueryInterface ( IID_IMaterialLibrary, (void**) &MaterialLib ) ) ) {
		GENERAL_TRACE_1 ("Failed to find IID_IMaterialLibrary\n");
	}

#if _DEBUG
	if( FAILED( System->QueryInterface ( IID_IRenderDebugger, (void**) &RenderDebugger ) ) ) {
		GENERAL_TRACE_1 ("Failed to find IID_IRenderDebugger\n");
	}
#endif
	
	// Initialize renderpipe
	//
	if( FAILED( RenderPipe->startup() ) ) {
		GENERAL_TRACE_1 ("Failed to start renderpipe\n");
		return 0;
	}

	pb.SetPipeline(RenderPipe);
	pb.SetIRenderPrimitive(RenderPrim);
	Font.SetRenderPipeline(RenderPipe);

	if( RenderDebugger ) {
		RenderDebugger->set_trace_output_dir( ird_capture_output_dir );

		RenderDebugger->set_trace_enable( ird_capture_startup? true : false );
		RenderDebugger->begin_trace_section( "startup" );
	}

	// Create main application window and render buffers.
	// If we can't get buffers in the current fullscreen (windowed)
	// configuration, we try windowed (fullscreen).
	//
	if( CreateAppWindow( hInstance ) == 0 ) {
		GENERAL_TRACE_1( "Failed to create main application window, exiting...\n" );
		return 0;
	}

	SetDisplayMode( DisplayWidth, DisplayHeight, DisplayDepth, Fullscreen );

	// Create Engine Container and load the necessary engine components
	// out of the [Engine] section.
	//
	DACOMDESC desc = "IEngine";
	if( FAILED( DACOM->CreateInstance( &desc, (void**)&Engine ) ) ) {
		GENERAL_TRACE_1( "Failed to create Engine Container, exiting...\n" );
		return 0;
	}
				
	Engine->load_engine_components (System);

	if( FAILED( Engine->QueryInterface( IID_IRenderer, (void**) &Renderer ) ) ) {
		GENERAL_TRACE_1( "Failed to acquire IID_IRenderer" );
		return 0;
	}
	
	UpdateWindowTitle();
	
	ShowWindow (hWnd, SW_SHOWNORMAL);

	#ifdef ANIMATION
	StartupAnimation (Engine);
	#endif

	DA_HEAP_MARK_ALLOCATED(HEAP);

	TrapFpu( false );

	ObjectList_Initialize();

	SetSearchPath( FilesToLoad[0] );

	COMPTR<IFileSystem> IFS;
	if( SUCCEEDED( Engine->create_file_system( PreloadTextureFile, IFS ) ) ) {
		TextureLib->load_library( IFS, NULL );
		TextureLib->get_texture_count( &texture_count );
	}

	ViewRect p;

	p.x0 = p.y0 = 0;
	p.x1 = DisplayWidth - 1;
	p.y1 = DisplayHeight - 1;

	Camera = new BaseCamera(Engine, &p);
	
	ASSERT(Camera);
	
	TheCamera = Camera;

	Camera->set_near_plane_distance (CameraNearClippingPlane);
	Camera->set_far_plane_distance (CameraFarClippingPlane);
	Camera->set_Horizontal_FOV (CameraHFov);
	Camera->set_Horizontal_to_vertical_aspect (CameraHVAspect);

	SetCameraZoomRate ();

	SetPixelThreshold();

	SetTranslationRate();

	ObjectList_Load();

	Engine->update (0);

	RenderPipe->set_pipeline_state( RP_CLEAR_COLOR, ARGB_MAKE( ((U32)(ClearRed * 255.0)), ((U32)(ClearGreen* 255.0)), ((U32)(ClearBlue* 255.0)), 255 ) );
	RenderPrim->set_render_state( D3DRS_ZENABLE, TRUE );

#ifdef LIGHT
	BuiltinLight_Create( 1.0f, 1.0f, 1.0f, LightRangeScale * CameraStartDistance );
	BuiltinLight_Attach( Camera->index );
#endif

	if(SkipBatch)
	{
		RenderPrim->set_state (RPR_BATCH, false);
	}
	else
	{
		RenderPrim->set_state (RPR_BATCH, true);
		RenderPrim->set_state (RPR_BATCH_TRANSLUCENT_POOL, 256*1024);
		RenderPrim->set_state (RPR_BATCH_POOLS,            RPR_TRANSLUCENT_DEPTH_SORTED | RPR_TRANSLUCENT_UNSORTED );
		RenderPrim->set_state (RPR_BATCH_TRANSLUCENT_MODE, RPR_TRANSLUCENT_DEPTH_SORTED );
	}

	if( RenderDebugger ) {
		RenderDebugger->end_trace_section();
		RenderDebugger->set_trace_enable( false );
	}


	Timer timer;
	timer.begin();

	while (true)
	{
		timer.end();
		float dt = timer.deltaSecs();
		timer.begin();
		float non_cap_dt = dt;

	#if CAP_TIME
		if (dt > MAX_DT)
			dt = MAX_DT;
	#endif

		if( ird_capture_scene ) {

			if( RenderDebugger ) {
				RenderDebugger->set_trace_enable( IRD_ST_MOST );

				RenderDebugger->begin_trace_section( "initstate" );
				RenderDebugger->dump_current_state( IRD_ST_ALL, ird_capture_output_dir );
				RenderDebugger->end_trace_section();

				RenderDebugger->begin_trace_section( "scene" );
			}
			else {
				StatusUpdate( "No IRenderDebugger found, are you using a debug build of RP?" );
			}
		}

		if( ird_capture_screen_shot ) {

			if( RenderDebugger ) {
				
				char filename[MAX_PATH];

				filename[0] = 0;

				if( GetFileNameFromUser( hWnd, true, FILTER_BMP_FILES, filename, MAX_PATH ) ) {
					RenderDebugger->save_screen_capture( filename, true );
				}
			}
		}

		RenderPipe->clear_buffers( RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL );
		RenderPipe->begin_scene();

	#if defined ANIMATION
		UpdateAnimation (dt);
	#endif

		TextureLib->update( dt );
			
		Engine->update (dt);

	#if defined LIGHT
		BuiltinLight_Update( TheCamera, dt );
		LightList_Update( TheCamera, dt );	
		AmbientLight_Update( dt );
	#endif

		if(draw_object)
			RenderObjects();

		if(draw_material & ~HIGH_BIT)
			RenderMaterials();

#if 0
		RenderPrim->set_modelview( Camera->get_inverse_transform() );
		pb.Begin( PB_LINES );
		pb.Color3f( 1,0,0 );	pb.Vertex3f( 0,0,0 );	pb.Vertex3f( 100, 0, 0 );
		pb.Color3f( 0,1,0 );	pb.Vertex3f( 0,0,0 );	pb.Vertex3f( 0, 100, 0 );
		pb.Color3f( 0,0,1 );	pb.Vertex3f( 0,0,0 );	pb.Vertex3f( 0, 0, 100 );
		pb.End();
#endif

		if(draw_wire & ~HIGH_BIT)
			RenderWireframe( draw_wire & ~HIGH_BIT, (draw_wire & HIGH_BIT)? true : false, 0.15f * global_scale );

		if(draw_axis & ~HIGH_BIT)
			RenderAxis();

	#if defined LIGHT
		if( draw_light_list ) 
			LightList_Render( Camera, 0.03f * global_scale );
		
		if( draw_builtin_light ) 
			BuiltinLight_Render( Camera, 0.02f * global_scale );
	#endif

	#if defined HARDPOINTS
		if( draw_hp & ~HIGH_BIT )
			HardpointList_Render( draw_hp & ~HIGH_BIT, draw_hp & HIGH_BIT );
	#endif
	
	#if defined DRAWEXTENTS
		if( draw_extent & ~HIGH_BIT )
			RenderExtents( draw_extent );
	#endif

		if(draw_texture & ~HIGH_BIT)
			RenderTexture( (draw_texture & ~HIGH_BIT)-1, (draw_texture & HIGH_BIT)? true : false );

		if(draw_stats) 
			RenderStats();

		if(draw_fc_vc)
			RenderFcVc();

		if(draw_help)
			DrawHelp();

		StatusRender( non_cap_dt );
		RenderFrameInfo( non_cap_dt );

		RenderPipe->end_scene();
		RenderPipe->swap_buffers();


		if( ird_capture_scene && RenderDebugger ) {

			RenderDebugger->end_trace_section();
			RenderDebugger->set_trace_enable( false );

			char filename[MAX_PATH];

			filename[0] = 0;

			if( GetFileNameFromUser( hWnd, true, FILTER_SCENE_FILES, filename, MAX_PATH ) ) {
				RenderDebugger->save_trace_capture( filename );
			}

			ird_capture_scene = 0;
		}

		ird_capture_screen_shot = 0;

		MessagePump ();
		ProcessRequests (dt);
	}
	
	return 0;
}

//

void StatusUpdate( const char *fmt, ... )
{
	va_list args;
	va_start( args, fmt );
	vsprintf( StatusMessage, fmt, args );
	va_end( args );

	StatusTimeout = STATUS_DEFAULT_FULL;
	StatusMode = 1;
}

//

void StatusRender( float dt )
{
	if( StatusMessage[0] == 0 ) {
		return;
	}

	switch( StatusMode ) {
	
	case 0:	return;

	case 1:
		StatusTimeout -= dt;

		if( StatusTimeout < 0.0f ) {
			StatusTimeout = STATUS_DEFAULT_DECAY;
			StatusMode = 2;
		}

		Font.SetColor( 1.0f, 1.0f, 1.0f );
		break;

	case 2:
		StatusTimeout -= dt;

		if( StatusTimeout < 0.0f ) {
			StatusTimeout = 0.0f;
			StatusMessage[0] = 0;
			StatusMode = 0;
			return;
		}

		Font.SetColor( StatusTimeout, StatusTimeout, StatusTimeout );
		break;
	}

	RenderPipe->set_modelview( Transform() );
	Font.SetSize( .125 );
	Font.SetOrtho();
	Font.RenderString( 10, DisplayHeight-20, StatusMessage );
	
	Font.SetColor( 1.0, 1.0, 1.0 );
}

//

void SetDisplayMode( U32 width, U32 height, U32 depth, U32 fullscreen )
{
	IVertexBufferManager *IVBM;
	if( SUCCEEDED( System->QueryInterface( IID_IVertexBufferManager, (void**)&IVBM ) ) ) {
		if( FAILED( IVBM->cleanup() ) ) {
			GENERAL_TRACE_1( "Failed to cleanup vertex buffer manager" );
		}
	}
	DACOM_RELEASE( IVBM );


	if( depth != DEPTH_COOKIE ) {
		RenderPipe->set_pipeline_state( RP_BUFFERS_COLOR_BPP, depth );
	}

	if( Fullscreen ) {
		SetStyleFullScreen();
		RenderPipe->set_pipeline_state( RP_BUFFERS_FULLSCREEN, TRUE );
	}
	else {
		RenderPipe->set_pipeline_state( RP_BUFFERS_FULLSCREEN, FALSE );
		SetStyleInWindow( width, height );
	}
	
	if( FAILED( RenderPipe->create_buffers( hWnd, width, height ) ) ) {
	
		Fullscreen = !Fullscreen;
		
		if( Fullscreen ) {
			SetStyleFullScreen();
			RenderPipe->set_pipeline_state( RP_BUFFERS_FULLSCREEN, TRUE );
		}
		else {
			RenderPipe->set_pipeline_state( RP_BUFFERS_FULLSCREEN, FALSE );
			SetStyleInWindow( width, height );
		}
		
		if( FAILED( RenderPipe->create_buffers( hWnd, width, height ) ) ) {
			GENERAL_TRACE_1( "We're in a bad way here, Roger.  Cannot modeswitch." );
		}
	}

	if( depth != DEPTH_COOKIE ) {
		DisplayDepth = depth;
	}
	DisplayWidth = width;
	DisplayHeight = height;
	Fullscreen = fullscreen;

	if( SUCCEEDED( System->QueryInterface( IID_IVertexBufferManager, (void**)&IVBM ) ) ) {
		if( FAILED( IVBM->initialize( NULL ) ) ) {
			GENERAL_TRACE_1( "Failed to cleanup vertex buffer manager" );
		}
	}
	DACOM_RELEASE( IVBM );
}

//

void StripPath( char *name )
{
	char *previous;
	char *current = strtok(name, "\\");

	while(current)
	{
		previous = current;
		current = strtok(NULL, "\\");
	}

	memmove(name, previous, strlen(previous) + 1);
}

//

void StripExtension( char *name )
{
	int i = strlen(name) - 1;
	while(i--)
	{
		if(name[i] == '.')
		{
			name[i] = 0;
			return;
		}
	}
}

//

bool GetFileNameFromUser( HWND hParent, bool save_dialog, const char *filter, char *out_buffer, U32 max_buffer_len )
{
	OPENFILENAME ofn;

	memset( &ofn, 0, sizeof(ofn) );
	ofn.lStructSize = sizeof(ofn);

	ofn.hwndOwner = hParent;
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpstrFilter = filter; 
	ofn.lpstrFile = out_buffer;
	ofn.nMaxFile = max_buffer_len;
	ofn.lpstrInitialDir = NULL;
//	ofn.lpstrTitle = "Open File To View";
	ofn.Flags = OFN_LONGNAMES | 
				0;

	if( save_dialog == false ) {
		ofn.Flags |= OFN_FILEMUSTEXIST;
	}

	if( !GetOpenFileName( &ofn ) ) {
		return false ;
	}

	return true;
}

//

void TrapFpu(bool on)
{
#ifdef _DEBUG

	_clear87();

	if(on)
	{
		// prepare to trap floating point exceptions
		unsigned int control_word = _controlfp (0, 0);

		control_word &= ~(EM_INVALID | EM_UNDERFLOW | EM_OVERFLOW | EM_ZERODIVIDE | EM_DENORMAL);
		//control_word &= ~(EM_INVALID | EM_OVERFLOW | EM_ZERODIVIDE);
		//control_word &= ~(EM_OVERFLOW | EM_ZERODIVIDE);
		_control87 (control_word, MCW_EM);
	}
	else
	{
		_control87(_CW_DEFAULT, 0xfffff);
	}
	
	// set precision
	//_controlfp( _PC_24, MCW_PC );
	//_controlfp( _PC_64, MCW_PC );
#endif
}
