// char.cpp
//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <zmouse.h>

// Disable useless STL 'identifier truncated' warning.
#pragma warning ( disable : 4786 )

#include <vector>

//

#include "DACom.h"
#include "fdump.h"
#include "GameSys.h"
#include "BaseLight.h"
#include "timer.h"
#include "BaseCam.h"
#include "ChannelEventTypes.h"
#include "RPUL.h"
#include "material.h"
#include "FileSys.h"
#include "ITextureLibrary.h"
#include "IDeformable.h"
#include "IVertexBufferManager.h"
#include "IMaterialLibrary.h"
#include "IHardpoint.h"
#include "Physics.h"
#include "IProfileParser_Utility.h"

//

#include "EulerAngles.h"
#include "Tfuncs.h"

//

#include "resource.h"

//

HRESULT GetProfileInfo( char *out_app_name, U32 max_app_name_len, char *out_ini_pathname, U32 max_ini_pathname_len );

//

const int WIDTH			= 800;
const int HEIGHT		= 600;

const int MAX_PARTS		= 8;
const int MAX_SCRIPTS	= 128;

//

struct ScriptSet
{
	SCRIPT_SET_ARCH	arch;
	int				num_scripts;
	char *			scripts[MAX_SCRIPTS];

	ScriptSet(void)
	{
		arch = INVALID_SCRIPT_SET_ARCH;
		num_scripts = 0;
	}

	~ScriptSet(void)
	{
		for (int i = 0; i < num_scripts; i++)
		{
			if (scripts[i])
			{
				free(scripts[i]);
				scripts[i] = NULL;
			}
		}
	}
};

//

struct BoneDesc
{
	const char *	name;
	INSTANCE_INDEX	idx;
};

//

typedef std::vector<INSTANCE_INDEX>	InstanceList;

//

BOOL CALLBACK ScriptDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

//
// we have to use this instead of make_string() because make_string() uses
// the broken wsprintf() which does not handle floats
//
const char *make_string( char *fmt, ... )
{
	static char buffer[1024];

	va_list args;
	va_start (args, fmt);
	vsprintf (buffer, fmt, args);
	va_end (args);

	return buffer;
}

//

inline CHECK( GENRESULT gr )
{
	if( gr != GR_OK ) {
		GENERAL_NOTICE( "call failed CHECK" );
		ASSERT_FATAL( 0 );
	}
}

//

//
// Globals.
//


PrimitiveBuilder	pb;
RPFont3D			Font;

ICOManager			*DACOM			=NULL;
ISystemContainer	*System			=NULL;
IRenderPipeline		*PIPE			=NULL;
IRenderPrimitive	*BATCH			=NULL;
IWindowManager		*WIN			=NULL;
IEngine				*ENG			=NULL;
IAnimation			*ANIM			=NULL;
IChannel			*CHANNEL		=NULL;
ILightManager		*LIGHT			=NULL;
IHardpoint			*HARDPOINT		=NULL;
IPhysics			*PHY			=NULL;
ITextureLibrary		*TEXTURELIB		=NULL;
IMaterialLibrary	*MATLIB			=NULL;
IProfileParser		*PROF			=NULL;
IDeformable			*DEFORM			=NULL;
IVertexBufferManager*VBM			=NULL;
HINSTANCE			GlobalInstance	=0;
HWND				GlobalHWnd		=0;
int					CurrentDriver	=0;
HDC					hDC;
HGLRC				hRC;
GameSystem			GAME;
bool				FullScreen = false;
bool				Quit = false;
char				Work[256];
BaseCamera *		Camera = NULL;
float				object_yaw = 3.1415926536f/180.0f;
bool				draw_wireframe = false;
bool				cull_face = false;
bool				active_motion = false;
int					nR = 0;
Matrix				R[2];
bool				DrawMesh = true;
bool				DrawFrame = false;
int					DrawFloor = 2;
bool				DrawExtents = false;
float				update_rate		=1.0f/30.0f;
bool				AnimateTarget	=false;
bool				AttachCamera	=false;
Vector				AttachOffset;
Vector				AttachOffset2;
bool				AnimationStarted=false;
int					DrawJoints		=0;
int					DrawHardPoints	=0;
int					DrawBones		=0;
int					ScriptPhase		=100;
U32					wheelMsg;
char				app_name[MAX_PATH];
char				ini_file[MAX_PATH];

HWND				ScriptDlg = NULL;

HANDLE				TheCharacter;
INSTANCE_INDEX		TheRoot;

U32					NumScripts = 0;
const char **		TheScripts = NULL;

//float				fps;

bool				Wrong = false;

float				floor_height = 0;
ITL_TEXTURE_REF_ID	floor_texture = ITL_INVALID_REF_ID;

int					subdivision_cnt = 2;

int					NumBones = 0;
BoneDesc			Bones[256];

InstanceList		AttachedChildren;

int					NumScriptSets = 0;
ScriptSet			ScriptSets[MAX_PARTS];

SCRIPT_SET_ARCH		script_set_arch[MAX_PARTS];

IDeformable::Axis	up_axis;
IDeformable::Axis	heading_axis;

bool				yaw_changed = false;
Vector				UP(0, 1, 0);
JOINT_INDEX			IKJoint = -1;
bool				EnforceLimits = true;

bool				LoopFlag = false;
float				PrevHeading = 0;

bool				Aiming = false;
HANDLE				hAim = 0;
float				angle = 0;

float				GlobalTime = 0;
int					lod = 0;
int					max_lod;
bool				Gravity = false;
float				rx	=0.0f;
float				ry	=0.0f;

int					num_bones_hit;
INSTANCE_INDEX		bones_hit[128];

//

static	void	GetMouseInput(void)
{
	POINT	Point;
	float	dx, dy;
	int		x, y;
	Matrix	mrot, mrot2, mrot3, mrot4;

	GetCursorPos(&Point);
  
	x = Point.x;
	y = Point.y;

	dx = ((float) (((float)WIDTH/2.0f) - Point.x) / 5.0f);
	dy = ((float) (((float)HEIGHT/2.0f) - Point.y) / 5.0f);

	SetCursorPos(WIDTH/2, HEIGHT/2);

	rx += dx;
	ry += dy;

	mrot.compose_rotation(ROLL, 0.0f);
	mrot2.compose_rotation(YAW, rx);
	mrot3.compose_rotation(PITCH, -ry);

	mrot4 = mrot * mrot2;
	mrot4 = mrot4 * mrot3;

	AttachOffset2 = mrot4 * AttachOffset;
}

//

void AddScript(const char * script_name)
{
	HWND listbox = GetDlgItem(ScriptDlg, IDC_LIST1);
	SendMessage(listbox, LB_ADDSTRING, 0, (LONG) script_name);
}

//

void recursively_get_bone_names(BoneDesc bones[], int & n, INSTANCE_INDEX root)
{
	bones[n++].name = ENG->get_instance_part_name(root);

	INSTANCE_INDEX c = ENG->get_instance_child_next(root, EN_DONT_RECURSE, INVALID_INSTANCE_INDEX);
	while (c != INVALID_INSTANCE_INDEX)
	{
		recursively_get_bone_names(Bones, n, c);

		c = ENG->get_instance_child_next(root, EN_DONT_RECURSE, c);
	}
}

//

void AddBone(const char * bone_name)
{
	if( bone_name )
	{
		HWND combobox = GetDlgItem(ScriptDlg, IDC_BONE_LIST);
		SendMessage(combobox, CB_ADDSTRING, 0, (LONG) bone_name);
	}
}

//

bool load_texture_file( const char *texture_filename )
{
	char fname[_MAX_PATH], drive[_MAX_DRIVE], path[_MAX_PATH], ext[_MAX_EXT], type[_MAX_EXT];
	char filename[_MAX_PATH], cmd[_MAX_PATH], txm_filename[_MAX_PATH];
	COMPTR<IFileSystem> IFS;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset( &si, 0, sizeof(si) );
	memset( &pi, 0, sizeof(pi) );

	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWMINIMIZED;

	strcpy( filename, texture_filename );
	strlwr( filename );
	strcpy( txm_filename, filename );

	_splitpath( filename, drive, path, fname, ext );

	if( stricmp( ext, ".txm" ) != 0 ) {

		strcpy( type, strlwr( &ext[1] ) );
		sprintf( txm_filename, "%s%s%s_%s.txm", drive, path, fname, type );

		sprintf( cmd, "txmlib.exe -c %s", txm_filename );
		CreateProcess( NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi );
		WaitForSingleObject( pi.hProcess, 5000 );

		sprintf( cmd, "txmlib.exe -tex %s %s", filename, txm_filename );
		CreateProcess( NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ) ;
		WaitForSingleObject( pi.hProcess, 5000 );
	}

	if( SUCCEEDED( ENG->create_file_system( txm_filename, IFS ) ) ) {
			
		TEXTURELIB->load_library( IFS, NULL );
	}

	return true;
}

//

void DisplayStartup(int w, int h)
{
	PIPE->startup();

	if (PIPE->create_buffers(GlobalHWnd, w, h) != GR_OK)
	{
		MessageBox(GlobalHWnd, "Unable to create display buffers.", "Error", MB_OK);
	}

	if( VBM ) {
		VBM->initialize( NULL );
	}
}

//

void DisplayShutdown(void)
{
	if( VBM ) {
		VBM->cleanup();
	}

	PIPE->shutdown();
}

//

void UpdateWindowTitle( const char *filename )
{
	ASSERT( PIPE );
	char window_title[MAX_PATH];

	RPDEVICEINFO di;
	if( FAILED( PIPE->get_device_info( &di ) ) ) {
		strcpy( di.device_class, "Unknown" );
		strcpy( di.device_type, "Unknown" );
	}

	sprintf( window_title, "%s -- %s [%s,%s,%s]", filename, app_name, ini_file, di.device_class, di.device_type );

	SetWindowText( GlobalHWnd, window_title );
}

//

BaseCamera	*CreateCamera(void)
{
	ViewRect p;
	p.x0 = p.y0 = 0;
	p.x1 = WIDTH - 1;
	p.y1 = HEIGHT - 1;

	BaseCamera	*cam	=new BaseCamera(GAME.ENG, &p);

	cam->set_Horizontal_FOV(90);
	cam->set_Horizontal_to_vertical_aspect(4.0f/3);
	cam->set_near_plane_distance(0.01f);

	return	cam;
}

//

void SetLight( ICamera *camera, D3DLIGHT7 *light, ILight *light_data )
{
	Vector	v;
	LightRGB rgb;

	memset( light, 0, sizeof(D3DLIGHT7) );

	if( light_data->IsInfinite() ) {
		light->dltType = D3DLIGHT_DIRECTIONAL;
	}
	else { 
		
		light->dvTheta = light_data->GetCutoff() ;

		if( light->dvTheta <= 0.0f || light->dvTheta >= 180.0f ) {
			light->dltType = D3DLIGHT_POINT;
			light->dvTheta *= MUL_DEG_TO_RAD;
			light->dvPhi = light->dvTheta;
		}
		else {
			light->dltType = D3DLIGHT_SPOT;
			light->dvTheta *= MUL_DEG_TO_RAD;
			light->dvPhi = __min( 1.3f * light->dvTheta, 180.0f * MUL_DEG_TO_RAD );
		}
	}

	light_data->GetColor( rgb );
	light->dcvDiffuse.r = ((float)rgb.r) / 255.0f;
	light->dcvDiffuse.g = ((float)rgb.g) / 255.0f;
	light->dcvDiffuse.b = ((float)rgb.b) / 255.0f;
	light->dcvDiffuse.a = 1.0f;

	light->dvFalloff = 1.0f;

	float r = light_data->GetRange();

	if( r < 0.0001 ) {
		light->dvAttenuation1 = 0.0001f;
		light->dvRange = D3DLIGHT_RANGE_MAX;
	}
	else {
		light->dvAttenuation0 = 1.0f;
		light->dvAttenuation1 = 0.0f; //1.0f/(4.00f * r);
		light->dvAttenuation2 = 1.0f/(0.25f * r * r);
		light->dvRange = 2.0f * r;
	}
	
	Vector P, D;
	Transform T;
	T.d[2][2] = -1.0f;

	light_data->GetDirection( v );
	D = T * camera->get_inverse_transform() * v;
	light->dvDirection.x = D.x;
	light->dvDirection.y = D.y;
	light->dvDirection.z = D.z;

	light_data->GetPosition( v );
	P = T * camera->get_inverse_transform() * v;
	
	light->dvPosition.x = P.x;
	light->dvPosition.y = P.y;
	light->dvPosition.z = P.z;
}

//

struct CharEventHandler : public Animation::IEventHandler
{
	void COMAPI on_event(unsigned int channel_id, void* user_supplied, const EventIterator& event_iter)
	{
		int count = event_iter.get_event_count();
		for (int i = 0; i < count; i++)
		{
			float t				= event_iter.get_event_time(i);
			unsigned int type	= event_iter.get_event_type(i);
			void * data			= event_iter.get_event_data(i);
			if (type == NAMED_EVENT || type == SOUND_START)
			{
				GENERAL_NOTICE( make_string( " GT(%f) %3.2f, %u, %s\n", GlobalTime, t, type, (char *) data ) );

				char * str = (char *) data;

				if (GlobalTime > 3 && ScriptPhase == 0)
				{
					if (strcmp(str, "flagtransitionrightfoot") == 0)
					{
						ScriptPhase = 1;
					}
				}

				if (ScriptPhase == 2)
				{
					if (strcmp(str, "flagtransitionleftfoot") == 0)
					{
						ScriptPhase = 3;
					}
				}
			}
			else if (type == CHANNEL_END)
			{
				GENERAL_NOTICE( make_string( " GT(%f) %3.2f %s CHANNEL_END\n", GlobalTime, t, user_supplied ) );
			}
			else if (type == CHANNEL_BEGIN)
			{
				GENERAL_NOTICE( make_string( " GT(%f) %3.2f %s CHANNEL_BEGIN\n", GlobalTime, t, user_supplied ) );
			}
		}
	}

	void COMAPI on_finished(unsigned int channel_id, void * user)
	{
		GENERAL_NOTICE( make_string( "on_finished: %u, %X\n", channel_id, user ) );
	}

	void COMAPI on_loop(unsigned int channel_id, Transform & T, void * user) 
	{
		GENERAL_NOTICE( make_string( "LOOP\n" ) );

	// compute heading.
		const Matrix & R = T.get_orientation();//ENG->get_orientation(TheRoot);

		Vector heading = R.get_k();
/*
		float h = heading.z;
		float angle = acos(h);
		if (heading.x < 0)
		{
			angle = 2.0 * 3.14159 - angle;
		}
		float degrees = angle * 180 / 3.14159;
*/
		float angle = atan2(heading.x, heading.z);
		float degrees = angle * 180 / 3.14159;

		if (degrees > 180)
		{
			degrees = degrees - 360;
		}
		GENERAL_NOTICE( make_string( "pre-loop heading : %g\n", degrees ) );
		GENERAL_NOTICE( make_string( "delta            : %g\n", degrees - PrevHeading ) );

		PrevHeading = degrees;
		LoopFlag = true;
	}
};

//

INSTANCE_INDEX find_child(INSTANCE_INDEX root, const char * name)
{
	INSTANCE_INDEX result = INVALID_INSTANCE_INDEX;

	const char *part_name = ENG->get_instance_part_name(root);
	if (part_name && !strcmp(part_name, name))
	{
		result = root;
	}
	else
	{
		INSTANCE_INDEX child = ENG->get_instance_child_next(root, EN_DONT_RECURSE, INVALID_INSTANCE_INDEX);
		while ((child != INVALID_INSTANCE_INDEX) && (result == INVALID_INSTANCE_INDEX))
		{
			result = find_child(child, name);
			child = ENG->get_instance_child_next(root, EN_DONT_RECURSE, child);
		}
	}

	return result;
}

//

void DrawTransform(const Transform & T, float scale, const Vector *colors = NULL )
{
	static const Vector default_colors[3] = { Vector(1,0,0), Vector(0,1,0), Vector(0,0,1) };

	if( !colors ) colors = default_colors;

	Vector i ( scale * T.get_i() );
	Vector j ( scale * T.get_j() );
	Vector k ( scale * T.get_k() );

	const Vector & p = T.get_position();

	Vector v = p + i;
	pb.Color3f(colors[0].x, colors[0].y, colors[0].z);
	pb.Vertex3f(p.x, p.y, p.z);
	pb.Vertex3f(v.x, v.y, v.z);

	v = p + j;
	pb.Color3f(colors[1].x, colors[1].y, colors[1].z);
	pb.Vertex3f(p.x, p.y, p.z);
	pb.Vertex3f(v.x, v.y, v.z);

	v = p + k;
	pb.Color3f(colors[2].x, colors[2].y, colors[2].z);
	pb.Vertex3f(p.x, p.y, p.z);
	pb.Vertex3f(v.x, v.y, v.z);

#if 0
	if( colors[0].x != 1.0f )
	{
		v = p - 7 * j; // hp
		pb.Color3f(1.0, 1.0, 1.0);
		pb.Vertex3f(p.x, p.y, p.z);
		pb.Vertex3f(v.x, v.y, v.z);
	}
	else
	{
		v = p - 7 * i;
		pb.Color3f(1.0, 1.0, 1.0);
		pb.Vertex3f(p.x, p.y, p.z);
		pb.Vertex3f(v.x, v.y, v.z);
	}
#endif
}

//

void SetViewVolume(BaseCamera *Camera)
{
	BATCH->set_viewport( Camera->pane.x0, Camera->pane.y0,
							  WIDTH-Camera->pane.x0, HEIGHT-Camera->pane.y0 );

	BATCH->set_perspective( Camera->fovy, Camera->aspect, Camera->znear, Camera->zfar );

	BATCH->set_modelview( Camera->get_inverse_transform() );
}

//

GENRESULT SetRender2D( void )
{
	Transform model(false);
	int vp[4];

	BATCH->get_viewport( &vp[0], &vp[1], &vp[2], &vp[3] );
	BATCH->set_ortho( vp[0], vp[2], vp[3], vp[1], 0, 1000 );

	model.set_identity();
	BATCH->set_modelview( model );

	return GR_OK;
}

//

void RenderFrameInfo( float dt )
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
	Font.RenderFormattedString( 10, 20, "FPS % 5.1f", fps );
}

//

/*
void DrawFPS(void)
{
	int x = 10;
	int y = 20;

	float font_scale = .125f;
	Set2D();
	pb.Color4ub(255,255,255,255);
		
	StringDraw(x, y, font_scale, "FPS %.1f", fps);
	//y += 20;
}
*/

//

void RenderJoints(INSTANCE_INDEX idx)
{
	static const Vector colors[3] = { Vector(1,0,0), Vector(0,1,0), Vector(0,0,1) };
	//static const Vector colors[3] = { Vector(0,1,1), Vector(1,0,1), Vector(1,1,0) };
	float radius;
	Vector center;
	ENG->get_instance_bounding_sphere(idx, 0, &radius, &center);

	BATCH->set_texture_stage_texture( 0, 0 );

	JOINT_INDEX jnt_idx = -1;
	
	pb.Begin(PB_LINES);
	while (-1 != (jnt_idx = ENG->get_instance_child_next(idx, 0, jnt_idx)))
	{
		const JointInfo* jnt = ENG->get_joint_info (jnt_idx);
		assert (jnt);

		switch (jnt->type)
		{
			case JT_SPHERICAL:
			{			
				const Transform & tm = ENG->get_transform( ENG->get_instance_parent(jnt_idx) );
				Transform hp_t ( (const Matrix &)tm * jnt->rel_orientation,
								 tm * jnt->parent_point );

				DrawTransform( hp_t, .1f * radius, colors );
			}
			break;
			case JT_LOOSE:
			{
				const Transform & tm = ENG->get_transform( ENG->get_instance_parent(jnt_idx) );
				Transform hp_t ( (const Matrix &)tm * jnt->rel_orientation,
								 tm *  jnt->rel_position );

				DrawTransform( hp_t, .1f * radius, colors );
			}
			break;
			default:
			{
				assert( "Unsupported joint type\n" );
			}
		}
	}
	pb.End();
}

//

void RenderBones(INSTANCE_INDEX idx, const Vector *parent_pos)
{
	static int level = 0;
	static const Vector colors[3] = { Vector(1,0,0), Vector(0,1,0), Vector(0,0,1) };
	//static const Vector colors[3] = { Vector(0,1,1), Vector(1,0,1), Vector(1,1,0) };
	static float radius = 1.0f;

	if( !level )
	{
		Vector center;
		ENG->get_instance_bounding_sphere (idx, 0, &radius, &center);

		BATCH->set_texture_stage_texture( 0, 0 );
		pb.Begin(PB_LINES);

	}

	level++;

	ARCHETYPE_INDEX arch = ENG->get_instance_archetype (idx);
	
	const Transform & tm = ENG->get_transform( idx );

	if( DrawBones == 2 )
	{
		DrawTransform( tm, .1f * radius );
	}

	if( parent_pos )
	{
		pb.Color3f(0, 0, 0);
		pb.Vertex3f(parent_pos->x, parent_pos->y, parent_pos->z); 
		pb.Vertex3f(tm.translation.x, tm.translation.y, tm.translation.z);
	}
	
	INSTANCE_INDEX child = INVALID_INSTANCE_INDEX;
	while( INVALID_INSTANCE_INDEX != (child = ENG->get_instance_child_next(idx, EN_DONT_RECURSE, child)) )
	{
		RenderBones( child, &(tm.translation) );
	}

	level--;

	if ( !level )
	{
		pb.End();
	}
}

//

void RenderHP( const char *hp_name, void *misc )
{
	static const Vector colors[3] = { Vector(1,0,0), Vector(0,1,0), Vector(0,0,1) };

	INSTANCE_INDEX idx = *((INSTANCE_INDEX*)misc);
	static float radius = 0.0f;
	if( radius == 0.0f )
	{
		Vector center;
		ENG->get_instance_bounding_sphere (idx, 0, &radius, &center);
	}

	ARCHETYPE_INDEX arch = ENG->get_instance_archetype( idx );
	if( arch != INVALID_ARCHETYPE_INDEX )
	{
		HardpointInfo inf;
		if (HARDPOINT->retrieve_hardpoint_info(arch, hp_name, inf))
		{
			{
				const Transform & tm = ENG->get_transform( idx );
				Transform hp_t ( tm * Transform( inf.orientation, inf.point ) );

				pb.Begin(PB_LINES);
					DrawTransform( hp_t, .1f * radius, colors );
				pb.End();

				if( DrawHardPoints == 2 )
				{
					float x, y, depth;
					if (Camera->point_to_screen(x, y, depth, hp_t.translation))
					{
						SetRender2D();
						pb.Color4ub( 255,255,255,255 );
//						Font.SetSize( .125 );
						Font.RenderFormattedString( x, y+20, "%s", hp_name );
						SetViewVolume( Camera );
					}
				}
			}
		}
	}
	ENG->release_archetype(arch);
}

//

typedef int (__cdecl * BONE_ENUM_CALLBACK)(INSTANCE_INDEX idx, void *misc);

//

int EnumHardPoints( INSTANCE_INDEX idx, void *misc )
{
	ARCHETYPE_INDEX arch = ENG->get_instance_archetype( idx );
	
	if( arch != INVALID_ARCHETYPE_INDEX )
	{
		HARDPOINT->enumerate_hardpoints (RenderHP, arch, &idx);
		ENG->release_archetype(arch);
	}
								
	return 0;
}

//

int EnumerateBones( INSTANCE_INDEX parent, BONE_ENUM_CALLBACK cb, void *misc )
{
	if( parent != INVALID_INSTANCE_INDEX )
	{
		cb(parent, misc);

		INSTANCE_INDEX child = -1;
		while( (child = ENG->get_instance_child_next( parent, EN_DONT_RECURSE, child )) != INVALID_INSTANCE_INDEX )
		{
			EnumerateBones(child, cb, misc);
		}
	}

	return -1;
}

//

void RenderHardPoints(void)
{
	SetViewVolume( Camera );

	BATCH->set_texture_stage_texture( 0, 0 );
	BATCH->set_render_state( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );

	INSTANCE_INDEX root_idx;
	
	if( FAILED( DEFORM->get_root(TheCharacter, root_idx) ) ) {
		GENERAL_TRACE_1( "RenderHardpoints: unable to get root of the object" );
		return;
	}

#if 1 // includes anything attached to the character

	EnumerateBones( root_idx, EnumHardPoints, NULL);

#else // character hp's only

	static const Vector colors[3] = { Vector(1,0,0), Vector(0,1,0), Vector(0,0,1) };

	U32 num_hp;
	const HardpointDesc * hp;
	float radius;
	Vector center;
	
	if( FAILED( DEFORM->get_num_hardpoints( TheCharacter, num_hp ) ) ) {
		GENERAL_TRACE_1( "RenderHardpoints: unable to get hardpoint count" );
		return;
	}

	if( FAILED( DEFORM->get_hardpoints( TheCharacter, hp ) ) ) {
		GENERAL_TRACE_1( "RenderHardpoints: unable to get hardpoint count" );
		return;
	}

	ENG->get_compound_radius (root_idx, &radius, &center);
	
	for(U32 i = 0; i < num_hp; i++)
	{
		ARCHETYPE_INDEX arch = ENG->get_archetype(hp[i].object);

		HardpointInfo inf;
		if (HARDPOINT->retrieve_hardpoint_info(arch, hp[i].name, inf))
		{
			//if(!strcmp(hp[i].name, "hphandright") )
			{
				const Transform & tm = ENG->get_transform( hp[i].object );
				Transform hp_t ( tm * Transform( inf.orientation, inf.point ) );

				pb.Begin(PB_LINES);
					DrawTransform( hp_t, .1f * radius, colors );
				pb.End();

				if( DrawHardPoints == 2 )
				{
					float x, y, depth;
					if (Camera->point_to_screen(x, y, depth, hp_t.translation))
					{
						Set2D();
						pb.Color4ub(255,255,255,255);
						StringDraw(x, y+20, .125, "%s", hp[i].name);

						SetViewVolume( Camera );
					}
				}
			}
		}
		ENG->release_archetype(arch);
	}
#endif

	BATCH->set_render_state( D3DRS_ZFUNC, D3DCMP_LESS );
}

//

void RenderFloor( void )
{
	ITL_TEXTUREFRAME_IRP frame;


	BATCH->set_render_state( D3DRS_CULLMODE,			D3DCULL_NONE );
	BATCH->set_render_state( D3DRS_ALPHABLENDENABLE,	FALSE );
	BATCH->set_render_state( D3DRS_LIGHTING,			FALSE );

	if( SUCCEEDED( TEXTURELIB->get_texture_ref_frame( floor_texture, ITL_FRAME_CURRENT, &frame ) ) ) {
		BATCH->set_texture_stage_texture( 0, frame.rp_texture_id );
	}
	else {
		BATCH->set_texture_stage_texture( 0, 0 );
	}

	BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP,	D3DTOP_SELECTARG1 );
	BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1,D3DTA_TEXTURE );
	BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2,D3DTA_CURRENT ); 
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP,	D3DTOP_SELECTARG1 );
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1,D3DTA_TEXTURE ); 
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG2,D3DTA_CURRENT );

	BATCH->set_texture_stage_texture( 1, 0 );
	BATCH->set_texture_stage_state( 1, D3DTSS_COLOROP,	D3DTOP_DISABLE );
	BATCH->set_texture_stage_state( 1, D3DTSS_ALPHAOP,	D3DTOP_DISABLE );

	if( FAILED( BATCH->verify_state() ) ) {
		static bool foo = false;
	}

	pb.Color4f(1, 1, 1, 1);
	pb.Begin(PB_QUADS);

		static float corner = 100.0f;
		static float scale  = 100.0f;
		if (Wrong)
		{
			pb.TexCoord2f(0, 0);
			pb.Vertex3f(-corner, +corner, floor_height);
			pb.TexCoord2f(scale, 0);
			pb.Vertex3f(+corner, +corner, floor_height);

			pb.TexCoord2f(scale, scale);
			pb.Vertex3f(+corner, -corner, floor_height);
			pb.TexCoord2f(0, scale);
			pb.Vertex3f(-corner, -corner, floor_height);
		}
		else
		{
			pb.TexCoord2f(0, 0);
			pb.Vertex3f(-corner, floor_height, +corner);
			pb.TexCoord2f(scale, 0);
			pb.Vertex3f(+corner, floor_height, +corner);

			pb.TexCoord2f(scale, scale);
			pb.Vertex3f(+corner, floor_height, -corner);
			pb.TexCoord2f(0, scale);
			pb.Vertex3f(-corner, floor_height, -corner);
		}

	pb.End();

	if( DrawFloor == 2)
	{
		int tiles = 300;
		BATCH->set_texture_stage_texture( 0, 0 );
		BATCH->set_render_state( D3DRS_ZFUNC, D3DCMP_ALWAYS );
		pb.Color4f(.5, .5, .5, 1);
		pb.Begin(PB_LINES);

			float pos = -corner;
			float inc = 2 * corner / tiles;
			for( int i = 0; i < tiles; i++, pos += inc )
			{
				if (Wrong)
				{
					pb.Vertex3f(pos, -corner, floor_height);
					pb.Vertex3f(pos, +corner, floor_height);

					pb.Vertex3f(-corner, pos, floor_height);
					pb.Vertex3f(+corner, pos, floor_height);
				}
				else
				{
					pb.Vertex3f(pos, floor_height, -corner);
					pb.Vertex3f(pos, floor_height, +corner);

					pb.Vertex3f(-corner, floor_height, pos);
					pb.Vertex3f(+corner, floor_height, pos);
				}
			}

		pb.End();
		BATCH->set_render_state( D3DRS_ZFUNC, D3DCMP_LESS );
	}
}

//

int check_limits(const JointInfo * joint, float x, float y, float z)
{
	int result = 0;
	if (x < joint->min0 || x > joint->max0)
	{
		result++;
	}
	if (y < joint->min1 || y > joint->max1) 
	{
		result++;
	}
	if (z < joint->min2 || z > joint->max2) 
	{
		result++;
	}
	return result;
}

//

struct TestCtrl : public Animation::IVirtualChannel
{
	const float quarter_pi;
	const float half_pi;
	const Vector & point;

	TestCtrl(const Vector & _target) : half_pi(3.14159/2), quarter_pi(3.14159/4), point(_target)
	{
	}

	virtual int update(void * dst, U32 id, const Animation::Target & target, float time)
	{
		if (target.type == Animation::JOINT)
		{
			Quaternion src;
			ENG->get_joint_state(target.joint, IE_JST_BASIC, &src.w);

			Matrix R(src);
			Vector look = R.get_k();

			const JointInfo * joint = ENG->get_joint_info(target.joint);

			INSTANCE_INDEX neck = ENG->get_instance_parent(target.joint);
			INSTANCE_INDEX head = target.joint;
			Vector head_pos = ENG->get_position(head);
			Vector aim = point - head_pos;
			aim.normalize();

			Matrix Rneck = ENG->get_orientation(neck);
			Quaternion qneck(Rneck);
			Vector k = aim;
			Vector j = Rneck.get_j();
			Vector i = cross_product(j, k);
			i.normalize();
			j = cross_product(k, i);

			Matrix Rgoal(i, j, k);
			Matrix Rrel = Rneck.get_transpose() * Rgoal;

			Matrix Rhead = ENG->get_orientation(head);
			Matrix Rcurr = Rneck.get_transpose() * Rhead;

			float x, y, z;
			EulerAnglesFromMatrix(x, y, z, Rrel, false);

			int v0 = check_limits(joint, x, y, z);
			if (v0)
			{
				float x0, y0, z0;
				EulerAnglesFromMatrix(x0, y0, z0, Rrel, true);
				int v1 = check_limits(joint, x0, y0, z0);

				if (v1 < v0)
				{
					x = x0;
					y = y0;
					z = z0;
				}
			}

		Matrix Rtest;
		MatrixFromEulerAngles(Rtest, x, y, z);

		GENERAL_NOTICE( make_string( "VM update (pre) : %f, %f, %f\n", Trad2deg(x), Trad2deg(y), Trad2deg(z) ) );

			if (EnforceLimits)
			{
			// Compute euler angles.

				bool limit = false;

				if (x < joint->min0)
				{
				GENERAL_NOTICE("x lower limit\n");
					x = joint->min0;
					limit = true;
				}
				if (x > joint->max0) 
				{
				GENERAL_NOTICE("x upper limit\n");
					x = joint->max0;
					limit = true;
				}
				if (y < joint->min1) 
				{
				GENERAL_NOTICE("y lower limit\n");
					y = joint->min1;
					limit = true;
				}
				if (y > joint->max1) 
				{
				GENERAL_NOTICE("y upper limit\n");
					y = joint->max1;
					limit = true;
				}
				if (z < joint->min2) 
				{
				GENERAL_NOTICE("z lower limit\n");
					z = joint->min2;
					limit = true;
				}
				if (z > joint->max2) 
				{
				GENERAL_NOTICE("z upper limit\n");
					z = joint->max2;
					limit = true;
				}

				if (limit)
				{
					MatrixFromEulerAngles(Rrel, x, y, z);
					GENERAL_NOTICE( make_string( "VM update (post): %f, %f, %f\n", Trad2deg(x), Trad2deg(y), Trad2deg(z) ) );
				}
			}

			Quaternion * q = (Quaternion *) dst;
			*q = Quaternion(Rrel);
		}

		return 0;
	}
};

Vector target(0, -50, 0);
Vector ptarget;
Matrix Rtarget;
TestCtrl test_ctrl(target);

struct ArmCtrl : public Animation::IVirtualChannel
{
	virtual int update(void * dst, U32 id, const Animation::Target & target, float time)
	{
		return 0;
	}
};

void DrawPlus(const Vector & p, float len, U8 r, U8 g, U8 b)
{
	BATCH->set_render_state(D3DRS_ZFUNC, D3DCMP_ALWAYS);
	BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	BATCH->set_texture_stage_texture( 0, 0 );
	
	pb.Color3ub(r, g, b);
	pb.Begin(PB_LINES);

 		const Matrix & R = ENG->get_orientation(Camera->index);
		float half_len = 0.5 * len;
		Vector i = R.get_i() * half_len;
		Vector j = R.get_j() * half_len;

		Vector v0 = p - i;
		Vector v1 = p + i;
		pb.Vertex3f(v0.x, v0.y, v0.z);
		pb.Vertex3f(v1.x, v1.y, v1.z);

		v0 = p - j;
		v1 = p + j;
		pb.Vertex3f(v0.x, v0.y, v0.z);
		pb.Vertex3f(v1.x, v1.y, v1.z);
	pb.End();

	BATCH->set_render_state(D3DRS_ZFUNC, D3DCMP_LESS);

}

void DrawExtentHierarchy(const BaseExtent * extent, const Transform & T, int r, int g, int b)
{
// Only draw leaves.
/*
	if (extent->child)
	{
		DrawExtentHierarchy(extent->child, T, r, g, b);
	}
	else
*/
	{
		Transform X = T.multiply(extent->xform);

		BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
		BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		BATCH->set_texture_stage_texture( 0, 0 );
		
		pb.Color3ub(r, g, b);

		switch (extent->type)
		{
			case ET_SPHERE:
			{
				pb.Begin(PB_POINTS);
					
					const Vector & p = X.get_position();
					pb.Vertex3f(p.x, p.y, p.z);

				pb.End();

				break;
			}
			case ET_BOX:
			{
				const Box * box = (const Box *) extent->get_primitive();

				pb.Begin(PB_LINES);
					
					Vector v[8];
					v[0].set( box->half_x,  box->half_y,  box->half_z);
					v[1].set( box->half_x, -box->half_y,  box->half_z);
					v[2].set(-box->half_x, -box->half_y,  box->half_z);
					v[3].set(-box->half_x,  box->half_y,  box->half_z);
					v[4].set( box->half_x,  box->half_y, -box->half_z);
					v[5].set( box->half_x, -box->half_y, -box->half_z);
					v[6].set(-box->half_x, -box->half_y, -box->half_z);
					v[7].set(-box->half_x,  box->half_y, -box->half_z);

					for (int i = 0; i < 8; i++)
					{
						v[i] = X.rotate_translate(v[i]);
					}

					pb.Vertex3f(v[0].x, v[0].y, v[0].z);
					pb.Vertex3f(v[1].x, v[1].y, v[1].z);

					pb.Vertex3f(v[1].x, v[1].y, v[1].z);
					pb.Vertex3f(v[2].x, v[2].y, v[2].z);

					pb.Vertex3f(v[2].x, v[2].y, v[2].z);
					pb.Vertex3f(v[3].x, v[3].y, v[3].z);

					pb.Vertex3f(v[3].x, v[3].y, v[3].z);
					pb.Vertex3f(v[0].x, v[0].y, v[0].z);

					pb.Vertex3f(v[4].x, v[4].y, v[4].z);
					pb.Vertex3f(v[5].x, v[5].y, v[5].z);

					pb.Vertex3f(v[5].x, v[5].y, v[5].z);
					pb.Vertex3f(v[6].x, v[6].y, v[6].z);

					pb.Vertex3f(v[6].x, v[6].y, v[6].z);
					pb.Vertex3f(v[7].x, v[7].y, v[7].z);

					pb.Vertex3f(v[7].x, v[7].y, v[7].z);
					pb.Vertex3f(v[4].x, v[4].y, v[4].z);

					pb.Vertex3f(v[0].x, v[0].y, v[0].z);
					pb.Vertex3f(v[4].x, v[4].y, v[4].z);

					pb.Vertex3f(v[1].x, v[1].y, v[1].z);
					pb.Vertex3f(v[5].x, v[5].y, v[5].z);

					pb.Vertex3f(v[2].x, v[2].y, v[2].z);
					pb.Vertex3f(v[6].x, v[6].y, v[6].z);

					pb.Vertex3f(v[3].x, v[3].y, v[3].z);
					pb.Vertex3f(v[7].x, v[7].y, v[7].z);

				pb.End();
				break;
			}
			case ET_TUBE:
			{
				const Tube * tube = (const Tube *) extent->get_primitive();

				Vector half_axis = tube->length * 0.5 * X.get_k();

				Vector t0 = X.get_position() - half_axis;
				Vector t1 = X.get_position() + half_axis;

				pb.Begin(PB_LINES);
					pb.Vertex3f(t0.x, t0.y, t0.z);
					pb.Vertex3f(t1.x, t1.y, t1.z);
				pb.End();

				break;
			}

			case ET_CYLINDER:
			{
				const Cylinder * cyl = (const Cylinder *) extent->get_primitive();

				Vector half_axis = cyl->length * 0.5 * X.get_k();

				Vector t0 = X.get_position() - half_axis;
				Vector t1 = X.get_position() + half_axis;

				pb.Begin(PB_LINES);
					pb.Vertex3f(t0.x, t0.y, t0.z);
					pb.Vertex3f(t1.x, t1.y, t1.z);
				pb.End();

				break;
			}
			default:
				//__asm int 3;
				break;
		}

		if (extent->child)
		{
			DrawExtentHierarchy(extent->child, T, r, g, b);
		}

	}

	const BaseExtent * x = extent->next;
	while (x)
	{
		DrawExtentHierarchy(x, T, r, g, b);
		x = x->next;
	}
}

//

void RecursivelyDrawExtents(INSTANCE_INDEX root)
{
	const BaseExtent * extent;
	if (PHY && PHY->get_extent(&extent, root))
	{
		Transform T = ENG->get_transform(root);
		T.set_position(PHY->get_center_of_mass(root));

		int g = 0;
		for (int i = 0; i < num_bones_hit; i++)
		{
			if (bones_hit[i] == root)
			{
				g = 255;
				break;
			}
		}

		DrawExtentHierarchy(extent, T, 0, g, 0);
	}

	INSTANCE_INDEX child = ENG->get_instance_child_next(root, EN_DONT_RECURSE, INVALID_INSTANCE_INDEX);
	while (child != INVALID_INSTANCE_INDEX)
	{
		RecursivelyDrawExtents(child);
		child = ENG->get_instance_child_next(root, EN_DONT_RECURSE, child);
	}
}

//

Matrix LookAt(const Vector & looker, const Vector & target)
{
	Vector look = looker - target;	
	look.normalize();

	Vector j;
	if (Wrong)
	{
		j.set(0, 0, 1);
	}
	else
	{
		j.set(0, 1, 0);
	}

	Vector i = cross_product(j, look);
	i.normalize();

	j = cross_product(look, i);
	j.normalize();

	i = cross_product(j, look);

	Matrix R;
	R.set_i(i);
	R.set_j(j);
	R.set_k(look);
	return R;
}

//

void check_names(INSTANCE_INDEX root)
{
	char name[128];

	const char * instance_name = ENG->get_instance_part_name(root);

	ARCHETYPE_INDEX arch = ENG->get_instance_archetype(root);
	strcpy( name, ENG->get_archetype_name(arch) );
	ENG->release_archetype(arch);

	GENERAL_NOTICE( make_string( "inst: %d %s, arch: %d %s %s\n", root, instance_name, arch, name, (PHY && PHY->is_valid(root)) ? "" : "INVALID" ) );

	INSTANCE_INDEX child = ENG->get_instance_child_next(root, EN_DONT_RECURSE, INVALID_INSTANCE_INDEX);
	while (child != INVALID_INSTANCE_INDEX)
	{
		check_names(child);

		child = ENG->get_instance_child_next(root, EN_DONT_RECURSE, child);
	}
}

//

void script_callback(const char * script_name, void * misc)
{
	SCRIPT_SET_ARCH arch = (SCRIPT_SET_ARCH) misc;

	for (int i = 0; i < NumScriptSets; i++)
	{
		if (ScriptSets[i].arch == arch)
		{
			break;
		}
	}

	if (i == NumScriptSets)
	{
		NumScriptSets++;
	}

	ScriptSet * set = ScriptSets + i;
	set->arch = arch;
	set->scripts[set->num_scripts++] = strdup(script_name);

	NumScripts++;
}

//

static inline bool intervals_overlap(int a0, int a1, int b0, int b1)
{
	return ((a0 <= b1) && (b0 <= a1));
}


int ConnectHP(INSTANCE_INDEX parent, const char *name1, INSTANCE_INDEX hpo_idx, const char *name2)
{
	if( parent != INVALID_INSTANCE_INDEX && hpo_idx != INVALID_INSTANCE_INDEX )
	{
		if( 0 == HARDPOINT->connect( parent, name1, hpo_idx, name2 ) )
		{
			return 0;
		}

		INSTANCE_INDEX child = -1;
		while( (child = ENG->get_instance_child_next( parent, EN_DONT_RECURSE, child )) != INVALID_INSTANCE_INDEX )
		{
			if( 0 == ConnectHP( child, name1, hpo_idx, name2 ) )
			{
				return 0;
			}
		}
	}

	return -1;
}


//
// AppMain.
//
void AppMain(LPSTR lpCmdLine)
{
#define MAX_NAME_LEN	128
	char	mesh_name[MAX_PARTS][MAX_NAME_LEN];
	char	anim_name[MAX_PARTS][MAX_NAME_LEN];
	int		i, num_parts	=0;

	ASSERT(lpCmdLine);

	char path[MAX_PATH];

	GetCurrentDirectory( MAX_PATH, path );

	if(*lpCmdLine)
	{
		char	*c		=lpCmdLine;
		bool	done	=false;

		do
		{
			while (*c == ' ') c++;

			if (*c == '-')
			{
				switch (c[1])
				{
					case 'w':
					case 'W':
						Wrong = true;
						break;
				}

				c += 2;
			}
			else
			{
				char * start = c;
				while (*c != ' ') c++;

				int cnt = c - start;

				strncpy(mesh_name[num_parts], start, cnt);
				mesh_name[num_parts][cnt] = 0;

				while (*c == ' ') c++;
				start = c;
				while (*c != ' ' && *c != 0) c++;

				cnt = c - start;

				strncpy(anim_name[num_parts], start, cnt);
				anim_name[num_parts][cnt] = 0;

				num_parts++;
			}

			if (*c == 0)
			{
				done = true;
			}
		} while (!done);
	}
	else
	{
		OPENFILENAME	ofn;
		char			file_names[_MAX_PATH * MAX_PARTS];	//seem reasonable?

		memset(mesh_name, 0, sizeof(char) * MAX_PARTS * MAX_NAME_LEN);
		memset(anim_name, 0, sizeof(char) * MAX_PARTS * MAX_NAME_LEN);
		memset(file_names, 0, sizeof(char) * _MAX_PATH * MAX_PARTS);

		char			*fn, *c			=lpCmdLine;
		static char		dfm_filter[]="Deform files (*.DFM) \0*.dfm\0\0";
		static char		anm_filter[]="Anim files (*.ANM) \0*.anm\0\0";

		ofn.lStructSize			=sizeof(ofn);
		ofn.hwndOwner			=GlobalHWnd;
		ofn.hInstance			=NULL;
		ofn.lpstrFilter			=dfm_filter;
		ofn.lpstrCustomFilter	=NULL;
		ofn.nMaxCustFilter		=0;
		ofn.nFilterIndex		=0;
		ofn.lpstrFile			=file_names;
		ofn.nMaxFile			=_MAX_PATH * MAX_PARTS;
		ofn.lpstrFileTitle		=NULL;
		ofn.nMaxFileTitle		=0;
		ofn.lpstrInitialDir		=NULL;
		ofn.lpstrTitle			=NULL;
		ofn.nFileOffset			=0;
		ofn.nFileExtension		=0;
		ofn.lpstrDefExt			="dfm";
		ofn.lCustData			=0L;
		ofn.lpfnHook			=NULL;
		ofn.lpTemplateName		=NULL;
		ofn.Flags				=OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

		if(!GetOpenFileName(&ofn))
		{
			return;
		}

		for(i=0, fn=file_names+strlen(file_names)+1;*fn;i++)
		{
			strcpy(mesh_name[i], file_names);
			strcat(mesh_name[i], "\\");
			strcat(mesh_name[i], fn);
			fn	+=strlen(fn)+1;
		}

		if(!i)	//single selection?
		{
			strcpy(mesh_name[i], file_names);
		}

		memset(file_names, 0, sizeof(char) * _MAX_PATH * MAX_PARTS);

		ofn.lpstrFilter			=anm_filter;
		ofn.lpstrFile			=file_names;
		ofn.lpstrDefExt			="anm";

		if(!GetOpenFileName(&ofn))
		{
			return;
		}

		for(i=0, fn=file_names+strlen(file_names)+1;*fn;i++)
		{
			strcpy(anim_name[i], file_names);
			strcat(anim_name[i], "\\");
			strcat(anim_name[i], fn);
			fn	+=strlen(fn)+1;
		}

		if(!i)	//single selection?
		{
			strcpy(anim_name[i++], file_names);
		}

		num_parts	=i;
	}

	SetCurrentDirectory( path );


	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	float ticks_per_second = float(freq.u.LowPart);

	LARGE_INTEGER last_tick;
	QueryPerformanceCounter(&last_tick);

// Set up camera, lights.
	Camera = CreateCamera();
	ENG->set_position(Camera->index, Vector(0, 0.6, 4));

	BaseLight light(ENG, GAME.GS);
	light.color.r = 255;
	light.color.g = 255;
	light.color.b = 255;
	light.range = 1000;
	light.infinite = false;
	light.cutoff = 180.0;
	light.map = 0;
	light.set_On(true);

	light.set_position(ENG->get_position(Camera->index));
	ILight * light_list = &light;
	if ( LIGHT )
	{
		LIGHT->activate_lights(&light_list, 1);

		LIGHT->set_ambient_light(32, 32, 32);
		//LIGHT->set_material_ambient(255, 255, 255);
		//LIGHT->set_material_diffuse(255, 255, 255);
		//LIGHT->set_material_emission(0, 0, 0);
	}


	D3DLIGHT7 d3dl;

	SetLight( Camera, &d3dl, &light );
	PIPE->set_light( 0, &d3dl );
	PIPE->set_light_enable( 0, TRUE );
	PIPE->set_render_state( D3DRS_AMBIENT, 0xFFCCCCCC );

// Create character hierarchy and deformable mesh.

	DAFILEDESC fdesc = ".";

	DeformPartMeshDesc meshes[MAX_PARTS];
	DeformPartDesc parts[MAX_PARTS];

	for (i = 0; i < num_parts; i++)
	{
		meshes[i].mesh_parent = NULL;
		meshes[i].mesh_name = mesh_name[i];

		parts[i].num_meshes = 1;
		parts[i].meshes = meshes + i;
		parts[i].skeleton_parent = NULL;

		IFileSystem * anim_file;
		DAFILEDESC anim_desc = anim_name[i];
		if (DACOM->CreateInstance(&anim_desc, (void **) &anim_file) == GR_OK)
		{
			script_set_arch[i] = ANIM->create_script_set_arch(anim_file);
			anim_file->Release();

			ANIM->enumerate_scripts(script_callback, script_set_arch[i], (void *) script_set_arch[i]);
		}
		else
		{
			char temp[128];
			sprintf(temp, "Unable to open %s.", anim_name[i]);
			MessageBox(GlobalHWnd, temp, "Error", MB_OK);
			return;
		}

		COMPTR<IFileSystem> ifs;

		if( SUCCEEDED( ENG->create_file_system( mesh_name[i], ifs ) ) ) {
			if( MATLIB ) MATLIB->load_library( ifs ); // 1.6, NULL );	// 1.7
			TEXTURELIB->load_library( ifs, NULL );
		}
	}

	max_lod = 0;

	CharEventHandler event_handler;

	DeformDesc desc;
	desc.num_parts = num_parts;
	desc.parts = parts;

	char user[] = "Bill";

	if (DEFORM->create(TheCharacter, desc, &event_handler, user) != GR_OK)
	{
		MessageBox(GlobalHWnd, "Unable to create character.", "Error", MB_OK);
		delete Camera;
		return;
	}

	CHECK(DEFORM->get_root(TheCharacter, TheRoot));
	check_names(TheRoot);

	recursively_get_bone_names(Bones, NumBones, TheRoot);

	for (i = 0; i < NumBones; i++)
	{
		AddBone(Bones[i].name);
	}

	DeformableObject * object;
	CHECK(DEFORM->get_DeformableObject(TheCharacter, object));

	if (Wrong)
	{
		Matrix Rcam;
		Rcam.set_i(Vector( 1,  0,  0));
		Rcam.set_j(Vector( 0,  0,  1));
		Rcam.set_k(Vector( 0, -1,  0));
		ENG->set_orientation(Camera->index, Rcam);

		heading_axis = IDeformable::POS_Y;
		up_axis = IDeformable::POS_Z;

		UP.set(0, 0, 1);

		AttachOffset.set(0, 0, 5);

	}
	else
	{
		heading_axis = IDeformable::POS_Z;
		up_axis = IDeformable::POS_Y;
		UP.set(0, 1, 0);

		AttachOffset.set(0, 0, 5);
	}

	CHECK(DEFORM->set_heading_axis(TheCharacter, heading_axis));
	CHECK(DEFORM->set_up_axis(TheCharacter, up_axis));

	for (i = 0; i < NumScriptSets; i++)
	{
		ScriptSet * set = ScriptSets + i;
		for (int j = 0; j < set->num_scripts; j++)
		{
			AddScript(set->scripts[j]);
		}
	}

	CHECK(DEFORM->get_root(TheCharacter, TheRoot));

#if 0
// virtual channel
	CHANNEL_INSTANCE_INDEX chan = CHANNEL->create_channel_instance(&test_ctrl, DT_VECTOR | DT_QUATERNION);
	if (chan != INVALID_CHANNEL_INSTANCE_INDEX)
	{
		Animation::StartParms parms;
		parms.target.type = Animation::OBJECT;
		parms.target.object = body;

		CHANNEL->start(chan, &parms);
	}
#endif
	GAME.ENG->set_position(TheRoot, Vector(0, 0, 0));
	Matrix I;
	I.set_identity();

	R[0].set_identity();
	R[1].set_i(I.get_i());
	R[1].set_j(-I.get_k());
	R[1].set_k(I.get_j());

	GAME.ENG->set_orientation(TheRoot, R[0]);
	//TheCharacter.set_orientation(Quaternion(UP, 3.14159));
	nR = 1;

	CHECK(DEFORM->set_floor_height(TheCharacter, floor_height));

//	INSTANCE_INDEX neck = find_child(TheCharacter.get_root(), "head");
//	INSTANCE_INDEX parent = ENG->get_parent(neck);

	INSTANCE_INDEX paim = find_child(TheRoot, "rshoulderjoint");
	INSTANCE_INDEX caim = find_child(TheRoot, "rhand");
//	TheCharacter.end_aim(haim);
								
	char texture_file[MAX_PATH];
	char texture_name[MAX_PATH];
	ITL_TEXTURE_ID tid;

	opt_get_string( DACOM, NULL, app_name, "FloorTextureFile", "", texture_file, MAX_PATH );
	opt_get_string( DACOM, NULL, app_name, "FloorTextureName", "floor", texture_name, MAX_PATH );

	if( texture_file[0] ) {
		if( load_texture_file( texture_file ) ) {

			if( SUCCEEDED( TEXTURELIB->has_texture_id( texture_name ) ) ) {

				if( SUCCEEDED( TEXTURELIB->get_texture_id( texture_name, &tid ) ) ) {

					if( SUCCEEDED( TEXTURELIB->add_ref_texture_id( tid, &floor_texture ) ) ) {
						
					}

					TEXTURELIB->release_texture_id( tid );
				}
			}
		}
	}



 	float dt = 1.0 / 30;

	Timer frame_timer, util_timer;

	frame_timer.begin();

	float restart_time = 0;

	int frame_count = 0;

	int phase = 0;

//
// ENABLE BATCHING OF TRANSLUCENT POLYS.
//
	BATCH->set_state(RPR_BATCH,						true);
	BATCH->set_state(RPR_BATCH_POOLS,				RPR_TRANSLUCENT_DEPTH_SORTED);
	BATCH->set_state(RPR_BATCH_TRANSLUCENT_MODE,	RPR_TRANSLUCENT_DEPTH_SORTED);

	angle = 0;

//TODO: make this a command line parameter
#if 0 // connect a 3db using hard points
	IFileSystem * hpo_file;
	DAFILEDESC hpo_desc = "gun3.3db";
	INSTANCE_INDEX hpo_idx;
	if (DACOM->CreateInstance(&hpo_desc, (void **) &hpo_file) == GR_OK)
	{
		hpo_idx = ENG->create_instance("gun", hpo_file, NULL);
		hpo_file->Release();
	}

	INSTANCE_INDEX bone_idx;
	CHECK(DEFORM->get_root(TheCharacter, bone_idx));

	ConnectHP(bone_idx, "hphandright", hpo_idx, "hphand");
#endif

	UpdateWindowTitle( mesh_name[i] );

	if( MATLIB ) MATLIB->verify_library( 0, 1.0f ) ; // 1.6 // IML_ALL, 0, 1.0f );

	while (!Quit)
	{
		WIN->ServeMessageQueue();

		if(GetAsyncKeyState(VK_RBUTTON) & 0x8000)
		{
			RECT	ClipRect;
			RECT	ClientRect;
			POINT	RPoint;

			GetClientRect(GlobalHWnd, &ClientRect);
			RPoint.x		=ClientRect.left;
			RPoint.y		=ClientRect.top;
			ClientToScreen(GlobalHWnd, &RPoint);
			ClipRect.left	=RPoint.x;
			ClipRect.top	=RPoint.y;

			RPoint.x		=ClientRect.right;
			RPoint.y		=ClientRect.bottom;
			ClientToScreen(GlobalHWnd, &RPoint);
			ClipRect.right	=RPoint.x;
			ClipRect.bottom	=RPoint.y;
			ClipCursor(&ClipRect);

			GetMouseInput();
		}
		else
		{
			ClipCursor(NULL);
		}


		light.set_position(ENG->get_position(Camera->index));

		static unsigned int color = 0x808080;

	// Render something
		BATCH->begin_scene();

		PIPE->set_pipeline_state(RP_CLEAR_COLOR, color);
		PIPE->clear_buffers(RP_CLEAR_COLOR_BIT | RP_CLEAR_DEPTH_BIT, NULL);

		if (AnimateTarget)
		{
			target.x = 50 * cos(angle);
			target.z = 50 * sin(angle);
//			target.z = -0.9 + 15 * fabs(cos(1.5 * angle));

		// UPDATE Rtarget.
			Vector look = target - ENG->get_position(caim);
			look.normalize();
#if 0
		// LOOSE CANNON
			Vector i = -look;
			Vector k(0, 0, 1);
			Vector j = cross_product(k, i);
			j.normalize();
			k = cross_product(i, j);
			k.normalize();
#else
		// FREELANCER
			Vector j = look;
			j.normalize();
			Vector k(0, 1, 0);
			Vector i = cross_product(j, k);
			i.normalize();
			k = cross_product(i, j);
			k.normalize();
#endif
			Rtarget.set_i(i);
			Rtarget.set_j(j);
			Rtarget.set_k(k);

			Vector head_pos = ENG->get_position(find_child(TheRoot, "Head01"));
			look = target - head_pos;
			look.normalize();
			float arm_len = 2.5;
			ptarget = head_pos + arm_len * look;
		}

	 	U32 num_scripts = 0;
		if (AnimationStarted)
		{
			CHECK(DEFORM->get_num_active_scripts(TheCharacter, num_scripts));
		}


		SetViewVolume(Camera);

		if (yaw_changed)
		{
			Matrix rot(Quaternion(UP, object_yaw));
			Matrix R = rot * ENG->get_orientation(TheRoot);
			CHECK(DEFORM->set_orientation(TheCharacter, R));
			yaw_changed = false;
		}

		dt = __min(dt, 1.0/10);
		util_timer.begin();

		ANIM->update (dt);
		TEXTURELIB->update (dt);
		GAME.ENG->update(dt);
		
		float update_time = util_timer.elapsedSecs();

		if (AnimationStarted)
		{
			CHECK(DEFORM->set_floor_height(TheCharacter, floor_height));
		}

		PIPE->set_render_state( D3DRS_LIGHTING,	FALSE );
		PIPE->set_render_state( D3DRS_ZENABLE,		TRUE );
		PIPE->set_render_state( D3DRS_ZFUNC,		D3DCMP_LESS );
		PIPE->set_render_state( D3DRS_ZWRITEENABLE,TRUE );

	// DRAW STUFF.
		util_timer.begin();

		if ( LIGHT )
			LIGHT->update_lighting(Camera);

		if (DrawFloor)
		{
			RenderFloor();
		}

		if (DrawMesh)
		{
			RECT rect;
			BOOL32 vis;
			CHECK(DEFORM->visible_rect(TheCharacter, vis, rect, Camera));
			if (vis)
			{
				int w = WIDTH;
				int h = HEIGHT;

				if (intervals_overlap(rect.top, rect.bottom, 0, h) && intervals_overlap(rect.left, rect.right, 0, w))
				{
					int mesh_indices[16];
					for (int i = 0; i < 16; i++) mesh_indices[i] = lod;
					CHECK(DEFORM->update(TheCharacter, dt, mesh_indices));
					CHECK(DEFORM->render(TheCharacter, Camera, mesh_indices)); //, subdivision_cnt));
				}
			}
		}

		if ( 0 ) //draw_wireframe)
		{
			SetViewVolume(Camera);

			MaterialRGB rgb;

			if (DrawMesh)
			{
				PIPE->set_render_state(D3DRS_ZFUNC,		D3DCMP_LESSEQUAL);
				rgb.r = rgb.g = rgb.b = 0xff;
			}
			else
			{
				rgb.r = rgb.g = rgb.b = 0;
			}

			PIPE->set_render_state(D3DRS_FILLMODE,			D3DFILL_WIREFRAME);

			int mat_idx = 0;
			U32 mat_txm[256];
			MaterialRGB mat_rgb[256];

			for (int i = 0; i < object->num_parts; i++)
			{
				for (int j = 0; j < object->parts[i]->num_meshes; j++)
				{
					DeformablePartMesh * mesh = &(object->parts[i]->meshes[j]);
					Material * mat = mesh->arch->material_list;

					for (int k = 0; k < mesh->arch->material_cnt; k++, mat++)
					{
						mat_txm[mat_idx] = mat->texture_id;
						mat->texture_id = 0;

						mat_rgb[mat_idx] = mat->diffuse;
						mat_rgb[mat_idx] = rgb;

						mat_idx++;
					}
				}
			}

			if ( LIGHT )
				LIGHT->set_ambient_light(255, 255, 255);

			int mesh_indices[16];
			for (i = 0; i < 16; i++) mesh_indices[i] = lod;

			CHECK(DEFORM->update(TheCharacter, dt, mesh_indices));
			CHECK(DEFORM->render(TheCharacter, Camera, mesh_indices)); //, subdivision_cnt));

			if ( LIGHT )
				LIGHT->set_ambient_light(32, 32, 32);

		// restore materials.
			mat_idx = 0;
			for (i = 0; i < object->num_parts; i++)
			{
				for (int j = 0; j < object->parts[i]->num_meshes; j++)
				{
					DeformablePartMesh * mesh = &(object->parts[i]->meshes[j]);
					Material * mat = mesh->arch->material_list;

					for (int k = 0; k < mesh->arch->material_cnt; k++, mat++)
					{
						mat->texture_id = mat_txm[mat_idx];
						mat->diffuse = mat_rgb[mat_idx];
						mat_idx++;
					}
				}
			}


			PIPE->set_render_state(D3DRS_FILLMODE,			D3DFILL_SOLID);
			if (DrawMesh)
			{
				PIPE->set_render_state(D3DRS_ZFUNC,		D3DCMP_LESS);
			}
		}

		PIPE->set_render_state( D3DRS_LIGHTING, FALSE );

		INSTANCE_INDEX root_idx;
		CHECK(DEFORM->get_root(TheCharacter, root_idx));

		if( DrawBones )
		{
			RenderBones( root_idx, NULL );
		}

		if( DrawJoints )
		{
			RenderJoints( root_idx );
		}

		if ( DrawHardPoints )
		{
			RenderHardPoints();
		}

		if (DrawExtents)
		{
			RecursivelyDrawExtents(TheRoot);
		}
		
		RenderFrameInfo( dt );

		Vector pt, normal;
		Vector ray_origin = ENG->get_position(Camera->index);
		Vector ray_direction = -ENG->get_orientation(Camera->index).get_k();

#if 0
		BOOL32 hit;
		int lod = 0;
		CHECK(DEFORM->intersect_ray(TheCharacter, hit, pt, normal, ray_origin, ray_direction, num_bones_hit, bones_hit, &lod));
		if (hit)
		{
			DrawPlus(pt, 1, 255, 255, 0);	

			pb.Begin(PB_LINES);

				pb.Color3ub(255, 255, 0);
				pb.Vertex3fv(&pt.x);

				Vector end = pt + normal;
				pb.Vertex3fv(&end.x);

			pb.End();
		}
		else
		{
		/*
			pb.Begin(PB_LINES);

				BATCH->set_render_state(D3DRS_TEXTUREHANDLE, 0);

				pb.Color3ub(255, 255, 0);
				pb.Vertex3fv(&ray_origin.x);
				Vector end = ray_origin + ray_direction * 100;
				pb.Color3ub(255, 0, 0);
				pb.Vertex3fv(&end.x);

			pb.End();
		*/
			DrawPlus(ray_origin + ray_direction, 0.1, 255, 0, 0);
		}
#endif

		RECT rect;
		if (0)//TheCharacter.visible_rect(rect, Camera))
		{
			Transform I; I.set_identity();
			BATCH->set_modelview(I);
			BATCH->set_ortho(0, 639, 479, 0);
			BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
			BATCH->set_texture_stage_texture( 0, 0 );

			pb.Begin(PB_LINES);

				pb.Color3ub(255, 255, 255);
				pb.Vertex3f(rect.left, rect.top, 0);
				pb.Vertex3f(rect.right, rect.top, 0);
				  
				pb.Vertex3f(rect.right, rect.top, 0);
				pb.Vertex3f(rect.right, rect.bottom, 0);

				pb.Vertex3f(rect.right, rect.bottom, 0);
				pb.Vertex3f(rect.left, rect.bottom, 0);

				pb.Vertex3f(rect.left, rect.bottom, 0);
				pb.Vertex3f(rect.left, rect.top, 0);

			pb.End();

			SetViewVolume(Camera);
		}

		if (Aiming)
		{
			BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
			BATCH->set_texture_stage_texture( 0, 0 );

			pb.Begin(PB_LINES);

				Vector i = Camera->get_orientation().get_i();
				Vector j = Camera->get_orientation().get_j();

				pb.Color3ub(255, 0, 255);
				Vector v0 = target - i;
				Vector v1 = target + i;
				pb.Vertex3f(v0.x, v0.y, v0.z);
				pb.Vertex3f(v1.x, v1.y, v1.z);

				v0 = target - j;
				v1 = target + j;
				pb.Vertex3f(v0.x, v0.y, v0.z);
				pb.Vertex3f(v1.x, v1.y, v1.z);

			pb.End();
		}

		if (DrawFrame)
		{
			BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
			BATCH->set_texture_stage_texture( 0, 0 );

			pb.Begin(PB_LINES);

				Vector p = ENG->get_position(caim);
				Matrix R = ENG->get_orientation(caim);
				Vector i = R.get_i();
				Vector j = R.get_j();
				Vector k = R.get_k();

				Vector v = p + i;
				pb.Color3ub(255, 0, 0);
				pb.Vertex3f(p.x, p.y, p.z);
				pb.Vertex3f(v.x, v.y, v.z);
				v = p + j;
				pb.Color3ub(0, 255, 0);
				pb.Vertex3f(p.x, p.y, p.z);
				pb.Vertex3f(v.x, v.y, v.z);
				v = p + k;
				pb.Color3ub(0, 0, 255);
				pb.Vertex3f(p.x, p.y, p.z);
				pb.Vertex3f(v.x, v.y, v.z);

			pb.End();
		}

		if (AttachCamera)
		{
			const Vector & char_pos = GAME.ENG->get_position(TheRoot);
			Vector cam_pos = char_pos + AttachOffset2;
			Matrix cam_R = LookAt(cam_pos, char_pos);

			Camera->set_position(cam_pos);
			Camera->set_orientation(cam_R);
		}

		float render_time = util_timer.elapsedSecs();
		dt = frame_timer.elapsedSecs();
		
		frame_timer.begin();

		GlobalTime += dt;

		BATCH->end_scene();

		BATCH->flush(RPR_TRANSLUCENT_DEPTH_SORTED);

		PIPE->swap_buffers();

		restart_time += update_rate;
		frame_count++;
	}

	for (i = 0; i < NumScriptSets; i++)
	{
		ScriptSet * set = ScriptSets + i;
		for (int j = 0; j < set->num_scripts; j++)
		{
			if (set->scripts[j])
			{
				free(set->scripts[j]);
				set->scripts[j] = NULL;
			}
		}
	}

	CHECK(DEFORM->destroy(TheCharacter));
	delete [] TheScripts;

	for (i = 0; i < num_parts; i++)
	{
		ANIM->release_script_set_arch(script_set_arch[i]);
	}

	delete Camera;
	ClipCursor(NULL);
}

int NumSelectedScripts = 0;
static char SelectedScript[1][128];

int MouseX = 0, MouseY = 0;

//
// WndProc.
//
LONG _stdcall WndProc(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == wheelMsg)
	{
		AttachOffset	*=(0.05f * wParam);
		return	0;
	}

	switch (message)
	{
		case WM_SYSCOMMAND:
		{
			unsigned int cmd = wParam;
			switch (cmd)
			{
				case SC_CLOSE:
					Quit = true;
					return 0;
			}
			break;
		}

		case WM_MOUSEMOVE:
			if (1)//MouseRotation)
			{
				int x = LOWORD(lParam);
				int y = HIWORD(lParam);

				int dx = x - MouseX;
				int dy = y - MouseY;

				float yaw = dx / 5.0 * 3.14159 / 80.0;
				float pitch = dy / 5.0 * 3.14159 / 80.0;

				Quaternion q1(Vector(0, 1, 0), yaw);
				Quaternion q2(Vector(1, 0, 0), pitch);
				Matrix R1(q1);
				Matrix R2(q2);
				Rtarget = R1 * R2 * Rtarget;

				MouseX = x;
				MouseY = y;
			}
			return 0;


		case WM_SYSKEYDOWN:
		{
			if (wParam == VK_RETURN)
			{
//				HDC dc = GetDC(hWindow);

				if (FullScreen)
				{
					WIN->SetWindowPos( WIDTH, HEIGHT, WMF_SHOW );
					FullScreen = false;
				}
				else
				{
					WIN->SetWindowPos( WIDTH, HEIGHT, WMF_FULL_SCREEN | WMF_SHOW);
					FullScreen = true;
				}

//				ReleaseDC(hWindow, dc);

				return 0;
			}
			break;
		}

		case WM_KEYDOWN: 
		{
			switch (wParam)
			{
				case VK_SPACE:
				{
					INSTANCE_INDEX root;
					CHECK(DEFORM->get_root(TheCharacter, root));
				  	INSTANCE_INDEX gun = find_child(root, "rfingers");

					break;
				}
				case VK_PRIOR:
				{
					target += 0.1 * Camera->get_orientation().get_j();
					break;
				}
				case VK_NEXT:
				{
					target -= 0.1 * Camera->get_orientation().get_j();
					break;
				}
				case VK_INSERT:
				{
					target += 0.1 * Camera->get_orientation().get_i();
					break;
				}
				case VK_DELETE:
				{
					target -= 0.1 * Camera->get_orientation().get_i();
					break;
				}
				case VK_END:
				{
					target += 0.1 * Camera->get_orientation().get_k();
					break;
				}
				case VK_HOME:
				{
					target -= 0.1 * Camera->get_orientation().get_k();
					break;
				}

				case 'C':
					AttachCamera = !AttachCamera;
					break;

				case 'X':
					DrawExtents = !DrawExtents;
					break;

				case 'O':
				{
					INSTANCE_INDEX root;
					if (DEFORM->get_root(TheCharacter, root) == GR_OK)
					{
						Vector pos = ENG->get_position(root);
						if (Wrong)
						{
							pos.z += 15;
						}
						else
						{
							pos.y += 2;
						}
						DEFORM->set_position(TheCharacter, pos);
					}
					break;
				}

				case 'P':
				{
					INSTANCE_INDEX root;
					if (DEFORM->get_root(TheCharacter, root) == GR_OK)
					{
						Vector pos = ENG->get_position(root);
						if (Wrong)
						{
							pos.z -= 15;
						}
						else
						{
							pos.y -= 2;
						}
						DEFORM->set_position(TheCharacter, pos);
					}
					break;
				}

				case 'E':
					EnforceLimits = !EnforceLimits;
					break;

				case '1':
				{
					CHECK(DEFORM->pause(TheCharacter));
					break;
				}

				case '2':
				{
					CHECK(DEFORM->resume(TheCharacter));
					break;
				}

				case 'D':
					CHECK(DEFORM->destroy(TheCharacter));
					break;

				case 'T':
				{
					U32 num_scripts;
					CHECK(DEFORM->get_num_active_scripts(TheCharacter, num_scripts));
					if (num_scripts)
					{
						ActiveScriptDesc desc[64];
						CHECK(DEFORM->describe_active_scripts(TheCharacter, desc));
						ANIM->set_current_time(desc[0].instance, desc[0].duration * 0.5);
					}
					break;
				}

				case 'Y':
					update_rate -= 0.091;
					if (update_rate < 0)
					{
						update_rate = 0;
					}
					break;

				case 'L':
					DrawFloor = ( DrawFloor + 1 ) % 3;
					break;

				case 'S':
					DrawBones = ( DrawBones + 1 ) % 3;
					break;

				case 'J':
					DrawJoints = ( DrawJoints + 1 ) % 2;
					break;

				case 'H':
					DrawHardPoints = ( DrawHardPoints + 1 ) % 3;
					break;

				case 'F':
					DrawFrame = !DrawFrame;
					break;

				case 'M':
					DrawMesh = !DrawMesh;
					break;

				case 'R':
					CHECK(DEFORM->set_orientation(TheCharacter, R[nR++]));
					if (nR == 2)
					{
						nR = 0;
					}
					break;

				case 'N':
					AnimateTarget = !AnimateTarget;
					break;

				case 'A':
				{
					U32 active;
					CHECK(DEFORM->get_num_active_scripts(TheCharacter, active));
					if (active)
					{
						ActiveScriptDesc * desc = new ActiveScriptDesc[active];
						CHECK(DEFORM->describe_active_scripts(TheCharacter, desc));

						for (U32 i = 0; i < active; i++)
						{
							GENERAL_NOTICE( make_string( "active: %s, %f, %f\n", desc[i].name, desc[i].duration, desc[i].current_time ) );
						}

						delete [] desc;
					}

					break;
				}

				case VK_F1:
					if (lod < max_lod)
					{
						lod++;
					}
					break;
				case VK_F2:
					if (lod > 0)
					{
						lod--;
					}
					break;

				case 'Z':
					CHECK(DEFORM->stop_motion(TheCharacter));
					break;

				case 'W':
					draw_wireframe = !draw_wireframe;
					break;

				case VK_ADD:
				{
					angle += 0.009 * 3.14159;
					break;
				}

				case VK_SUBTRACT:
				{
				 	angle -= 0.010 * 3.14159;
					break;
				}

				case VK_ESCAPE:
					Quit = true;
					return 0;

				case VK_NUMPAD4:
				{
					Matrix rot(Quaternion(UP, 5 * 3.14159 / 180));
					Matrix R = rot * ENG->get_orientation(Camera->index);
					ENG->set_orientation(Camera->index, R);
					return 0;
				}

				case VK_NUMPAD6:
				{
					Matrix rot(Quaternion(UP, -5 * 3.14159 / 180));
					Matrix R = rot * ENG->get_orientation(Camera->index);
					ENG->set_orientation(Camera->index, R);
					return 0;
				}

				case VK_NUMPAD8:
				{
					Transform xform = ENG->get_transform(Camera->index);
					xform.move_position(0, 0, -0.3);
					ENG->set_transform(Camera->index, xform);
					return 0;
				}

				case VK_NUMPAD2:
				{
					Transform xform = ENG->get_transform(Camera->index);
					xform.move_position(0, 0, 0.4);
					ENG->set_transform(Camera->index, xform);
					return 0;
				}

				case VK_NUMPAD9:
				{
					Transform xform = ENG->get_transform(Camera->index);
					xform.move_position(0, 0.10, 0);
					ENG->set_transform(Camera->index, xform);
					return 0;
				}

				case VK_NUMPAD3:
				{
					Transform xform = ENG->get_transform(Camera->index);
					xform.move_position(0, -0.09, 0);
					ENG->set_transform(Camera->index, xform);
					return 0;
				}

				case VK_UP:
					if (AttachCamera)
					{
						AttachOffset *= 0.9;
					}
					return 0;

				case VK_DOWN:
					if (AttachCamera)
					{
						AttachOffset *= 1.1;
					}
					return 0;

				case VK_LEFT:
					yaw_changed = true;
					object_yaw = 2.4 * 3.14159 / 180;
					return 0;

				case VK_RIGHT:
					yaw_changed = true;
					object_yaw = -2.6 * 3.14159 / 180;
					return 0;

				case VK_NUMPAD1:
					subdivision_cnt = Tmax( 1, subdivision_cnt - 1);
					return 0;

				case VK_NUMPAD7:
					subdivision_cnt++;
					return 0;
			}
		}
	}

	return DefWindowProc(hWindow, message, wParam, lParam);
}

//
// Exit handlers.
//
static int exit_handler_active = 0;

void WINAPI WinClean(void)
{
	GENERAL_NOTICE( "WinClean called\n" );

	if (exit_handler_active)
	{
		GENERAL_NOTICE( "Re-entered exit handler!\n" );
		return;
	}

	exit_handler_active = 1;

	Font.SetRenderPipeline( NULL );

	pb.SetPipeline( NULL );
	pb.SetIRenderPrimitive( NULL );
	
	DACOM_RELEASE( System );
	DACOM_RELEASE( PIPE );
	DACOM_RELEASE( BATCH );
	DACOM_RELEASE( WIN );
	DACOM_RELEASE( ENG );
	DACOM_RELEASE( ANIM );
	DACOM_RELEASE( CHANNEL );
	DACOM_RELEASE( LIGHT );
	DACOM_RELEASE( HARDPOINT );
	DACOM_RELEASE( PHY );
	DACOM_RELEASE( TEXTURELIB );
	DACOM_RELEASE( MATLIB );
	DACOM_RELEASE( PROF );
	DACOM_RELEASE( DEFORM );
	DACOM_RELEASE( VBM );

	GAME.shut_down();

	DACOM_RELEASE( DACOM );

	GENERAL_NOTICE( "Final exit OK\n" );
}

void _cdecl WinExit(int code)
{
	GENERAL_NOTICE( "WinExit called\n" );

	if (!exit_handler_active)
	{
		WinClean();
	}

	exit(code);
}

void AppExit(void)
{
	GENERAL_NOTICE( "AppExit() called via atexit()\n" );

	if (!exit_handler_active)
	{
		WinClean();
	}

	return;
}

//
// WinMain
//
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	GlobalInstance = hInstance;

	GENERAL_NOTICE( "Initializing DACOM\n" );

	DACOM = DACOM_Acquire();

	if (DACOM == NULL)
	{
		return 0;
	}

	if( FAILED( GetProfileInfo( app_name, MAX_PATH, ini_file, MAX_PATH ) ) ) {
		MessageBox( NULL, "Unable to find appropriate .INI file, exiting...", "Error", MB_OK );
		exit(0);
	}

	if( FAILED( DACOM->SetINIConfig(ini_file) ) ) {
		MessageBox( NULL, "Unable to initialize DACOM (bad ini file), exiting...", "Error", MB_OK );
		exit(0);
	}

//
// Initialize game system shell
//
	GENERAL_NOTICE( "Initializing game system interface\n" );

	bool allow_multiple_instances = true;
	bool ok = false;

	if (GAME.startup(hInstance, "Testbed", WinExit) == 0)
		return 0;

/*
	DADEFORMDESC temp_desc(GAME.GS, GAME.ENG);
	CHECK(DACOM->CreateInstance(&temp_desc, (void **) &DEFORM));
*/

	GAME.GS->QueryInterface( IID_IWindowManager,	(void **) &WIN);
	GAME.GS->QueryInterface( IID_IRenderPipeline,	(void **) &PIPE);
	GAME.GS->QueryInterface( IID_IRenderPrimitive, (void **) &BATCH);
	GAME.GS->QueryInterface( IID_ILightManager,	(void **) &LIGHT);
	GAME.GS->QueryInterface( IID_ITextureLibrary,	(void **) &TEXTURELIB);
	GAME.GS->QueryInterface( IID_IMaterialLibrary,	(void **) &MATLIB);
	GAME.GS->QueryInterface( IID_IVertexBufferManager, (void**) &VBM );

	GAME.ENG->QueryInterface( IID_IEngine,			(void **) &ENG);
	GAME.ENG->QueryInterface( IID_IAnimation,		(void **) &ANIM);
	GAME.ENG->QueryInterface( IID_IChannel,		(void **) &CHANNEL);
	GAME.ENG->QueryInterface( IID_IHardpoint,		(void **) &HARDPOINT);
	GAME.ENG->QueryInterface( IID_IPhysics,		(void **) &PHY);
	GAME.ENG->QueryInterface( IID_IDeformable,	(void **) &DEFORM);

	DACOM->QueryInterface( IID_IProfileParser,		(void **) &PROF);

	ASSERT( DEFORM );
	ASSERT( WIN );
	ASSERT( PIPE );
	ASSERT( BATCH );
	ASSERT( ENG );
	ASSERT( ANIM );
	ASSERT( CHANNEL );
	ASSERT( LIGHT );
	ASSERT( TEXTURELIB );
	ASSERT( HARDPOINT );
	ASSERT( PROF );
	
	if( !MATLIB ) {
		GENERAL_NOTICE( "Unable to locate IID_IMaterialLibrary" );
	}

	HWND hWnd = GlobalHWnd = WIN->GetWindowHandle();
//	HDC dc = GetDC(hWnd);
//	hDC = dc;

	pb.SetPipeline( PIPE );
	pb.SetIRenderPrimitive( BATCH );
	Font.SetRenderPipeline( PIPE );

	if (FullScreen)
	{
		WIN->SetWindowPos( WIDTH, HEIGHT, WMF_FULL_SCREEN | WMF_SHOW);
	}
	else
	{
		WIN->SetWindowPos( WIDTH, HEIGHT, WMF_SHOW);
	}

	DisplayStartup( WIDTH, HEIGHT );

	WIN->SetCallback(WndProc);

	//
	// Register exit handler 
	//
	atexit(AppExit);

	//
	// Set up script selection window.
	//
	ScriptDlg = CreateDialog(GlobalInstance, MAKEINTRESOURCE(IDD_FORMVIEW), GlobalHWnd, (DLGPROC) ScriptDlgProc);
	if (!ScriptDlg)
	{
		unsigned long err = GetLastError();

		MessageBox(GlobalHWnd, "Unable to create script selection dialog box.", "Error", MB_OK);
	}

	ShowWindow(ScriptDlg, SW_SHOW);

	wheelMsg = RegisterWindowMessage(MSH_MOUSEWHEEL);
	AppMain( lpCmdLine );

	if (FullScreen)
	{
		WIN->SetWindowPos( WIDTH, HEIGHT, WMF_SHOW);
		InvalidateRect(0, 0, 1);
	}

	if( floor_texture != ITL_INVALID_REF_ID ) {
		TEXTURELIB->release_texture_ref( floor_texture );
		floor_texture = ITL_INVALID_REF_ID ;
	}

	DisplayShutdown();

//	ReleaseDC(hWnd, dc);

	DestroyWindow( ScriptDlg );

	return 0;
}

//

BOOL CALLBACK PropertiesDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL result = 0;

	int lo = LOWORD(wParam);
	int hi = HIWORD(wParam);

	switch (msg)
	{
		case WM_INITDIALOG:
			break;
		case WM_COMMAND:
			switch(lo)
			{
				case IDOK:
					EndDialog(hwndDlg, lo);
					break;
			}
	}

	return result;
}

//

int CurrentBone;
Matrix CurrentJ;
float CurrentMass, CurrentStiffness;

void GetBoneProperties(int bone, Matrix & J, float & mass, float & stiffness);
void SetBoneProperties(int bone, const Matrix & J, float mass, float stiffness);

//

BOOL CALLBACK BonePropertiesDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL result = 0;
/*
	int lo = LOWORD(wParam);
	int hi = HIWORD(wParam);

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			GetBoneProperties(CurrentBone, CurrentJ, CurrentMass, CurrentStiffness);
			char temp[80];
			sprintf(temp, "%f", CurrentMass);
			SetDlgItemText(hwndDlg, IDC_MASS, temp);

			sprintf(temp, "%f", CurrentJ.d[0][0]);
			SetDlgItemText(hwndDlg, IDC_IXX, temp);

			sprintf(temp, "%f", CurrentJ.d[0][1]);
			SetDlgItemText(hwndDlg, IDC_IXY, temp);

			sprintf(temp, "%f", CurrentJ.d[0][2]);
			SetDlgItemText(hwndDlg, IDC_IXZ, temp);

			sprintf(temp, "%f", CurrentJ.d[1][0]);
			SetDlgItemText(hwndDlg, IDC_IYX, temp);

			sprintf(temp, "%f", CurrentJ.d[1][1]);
			SetDlgItemText(hwndDlg, IDC_IYY, temp);

			sprintf(temp, "%f", CurrentJ.d[1][2]);
			SetDlgItemText(hwndDlg, IDC_IYZ, temp);

			sprintf(temp, "%f", CurrentJ.d[2][0]);
			SetDlgItemText(hwndDlg, IDC_IZX, temp);

			sprintf(temp, "%f", CurrentJ.d[2][1]);
			SetDlgItemText(hwndDlg, IDC_IZY, temp);

			sprintf(temp, "%f", CurrentJ.d[2][2]);
			SetDlgItemText(hwndDlg, IDC_IZZ, temp);

			sprintf(temp, "%f", CurrentStiffness);
			SetDlgItemText(hwndDlg, IDC_STIFFNESS, temp);

			break;
		}
		case WM_COMMAND:
			switch(lo)
			{
				case IDOK:
				{
					char temp[80];
					GetDlgItemText(hwndDlg, IDC_MASS, temp, 80);
					CurrentMass = atof(temp);

					GetDlgItemText(hwndDlg, IDC_IXX, temp, 80);
					CurrentJ.d[0][0] = atof(temp);

					GetDlgItemText(hwndDlg, IDC_IXY, temp, 80);
					CurrentJ.d[0][1] = atof(temp);

					GetDlgItemText(hwndDlg, IDC_IXZ, temp, 80);
					CurrentJ.d[0][2] = atof(temp);

					GetDlgItemText(hwndDlg, IDC_IYX, temp, 80);
					CurrentJ.d[1][0] = atof(temp);
					
					GetDlgItemText(hwndDlg, IDC_IYY, temp, 80);
					CurrentJ.d[1][1] = atof(temp);

					GetDlgItemText(hwndDlg, IDC_IYZ, temp, 80);
					CurrentJ.d[1][2] = atof(temp);

					GetDlgItemText(hwndDlg, IDC_IZX, temp, 80);
					CurrentJ.d[2][0] = atof(temp);

					GetDlgItemText(hwndDlg, IDC_IZY, temp, 80);
					CurrentJ.d[2][1] = atof(temp);

					GetDlgItemText(hwndDlg, IDC_IZZ, temp, 80);
					CurrentJ.d[2][2] = atof(temp);

					GetDlgItemText(hwndDlg, IDC_STIFFNESS, temp, 80);
					CurrentStiffness = atof(temp);

					SetBoneProperties(CurrentBone, CurrentJ, CurrentMass, CurrentStiffness);
					EndDialog(hwndDlg, lo);
					break;
				}
				case IDCANCEL:
					EndDialog(hwndDlg, lo);
					break;
			}
	}
*/
	return result;
}

//

struct HPInstanceInfo
{
	std::string		hp;
	INSTANCE_INDEX	instance;

	HPInstanceInfo(void)
	{
		instance = INVALID_INSTANCE_INDEX;
	}
};

typedef std::vector<HPInstanceInfo>	HPInstanceList;

//

struct AttachHPInfo
{
// input parameters.
	HANDLE					hdeform;
	const HPInstanceList &	child_hp_list;

// output parameters.
	int					parent_hp_index;
	int					child_hp_index;

	AttachHPInfo(HANDLE character, const HPInstanceList & chplist) : hdeform(character), child_hp_list(chplist)
	{
		parent_hp_index = child_hp_index = -1;
	}
};

//

BOOL CALLBACK ChooseHPDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL result = 0;

	static AttachHPInfo * info = NULL;

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			info = reinterpret_cast<AttachHPInfo *>(lParam);

		// populate comboboxes with HP names.

			U32 num_hp;
			DEFORM->get_num_hardpoints(info->hdeform, num_hp);

			const HardpointDesc * char_hp;
			DEFORM->get_hardpoints(info->hdeform, char_hp);

			HWND parent_hp_cb = GetDlgItem(hwndDlg, IDC_CHAR_HP);
			for (U32 i = 0; i < num_hp; i++)
			{
				SendMessage(parent_hp_cb, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(char_hp[i].name));
			}

			HWND child_hp_cb = GetDlgItem(hwndDlg, IDC_CHILD_HP);

			HPInstanceList::const_iterator s = info->child_hp_list.begin();
			while (s != info->child_hp_list.end())
			{
				const HPInstanceList::value_type & v = *s;

				SendMessage(child_hp_cb, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(v.hp.c_str()));

				s++;
			}

		// default selection to first item in each list.

			SendMessage(parent_hp_cb, CB_SETCURSEL, 0, 0);
			SendMessage(child_hp_cb, CB_SETCURSEL, 0, 0);

			break;
		}

		case WM_COMMAND:
		{
			int lo = LOWORD(wParam);
			int hi = HIWORD(wParam);

			switch (lo)
			{
				case IDOK:
				{
					HWND parent_hp_cb = GetDlgItem(hwndDlg, IDC_CHAR_HP);
					HWND child_hp_cb = GetDlgItem(hwndDlg, IDC_CHILD_HP);

					info->parent_hp_index	= SendMessage(parent_hp_cb, CB_GETCURSEL, 0, 0);
					info->child_hp_index	= SendMessage(child_hp_cb, CB_GETCURSEL, 0, 0);

					EndDialog(hwndDlg, 1);
					break;
				}
				case IDCANCEL:
					EndDialog(hwndDlg, 0);
					break;
			}

			break;
		}
	}



	return result;
}

//

void __cdecl AttachHPCallback(const char * hp_name, void* misc)
{
	HPInstanceList * list = reinterpret_cast<HPInstanceList *>(misc);

	HPInstanceInfo info;
	info.hp = hp_name;

	list->push_back(info);
}

//

BOOL CALLBACK ScriptDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL result = 0;

	int lo = LOWORD(wParam);
	int hi = HIWORD(wParam);

	switch (msg)
	{
		case WM_INITDIALOG:
		{
		// fill in script listbox.
			HWND list = GetDlgItem(hwndDlg, IDC_LIST1);
			SendMessage(list, LB_RESETCONTENT, 0, 0);

			for (U32 i = 0; i < NumScripts; i++)
			{
				SendMessage(list, LB_INSERTSTRING, i, (LONG) &(TheScripts[i][0]));
			}

		// set default parameters.
			char trans[] = "0.0";
			SetDlgItemText(hwndDlg, IDC_EDIT1, trans);

			char time_scale[] = "1.0";
			SetDlgItemText(hwndDlg, IDC_EDIT2, time_scale);

			char weight[] = "1.0";
			SetDlgItemText(hwndDlg, IDC_EDIT3, weight);

			SendMessage(list, LB_SETCURSEL, 0, 0);
			SetFocus(list);

			char start_time[] = "0.0";
			SetDlgItemText(hwndDlg, IDC_EDIT4, start_time);

		// fill in bone combobox.
			HWND bone_list = GetDlgItem(hwndDlg, IDC_BONE_LIST);
			SendMessage(bone_list, CB_RESETCONTENT, 0, 0);

			SendMessage(bone_list, CB_INSERTSTRING, 0, (LONG) "none");

			INSTANCE_INDEX root;
			if (DEFORM->get_root(TheCharacter, root) == GR_OK)
			{
				recursively_get_bone_names(Bones, NumBones, root);

				for (int i = 0; i < NumBones; i++)
				{
					SendMessage(bone_list, CB_INSERTSTRING, i+1, (LONG) Bones[i].name);
				}
			}

			SendMessage(bone_list, CB_SETCURSEL, 0, 0);

			EnableWindow(bone_list, 0);

			HWND remove_child_button = GetDlgItem(hwndDlg, IDC_REMOVE_CHILD);
			EnableWindow(remove_child_button, 0);

			HWND child_combo = GetDlgItem(hwndDlg, IDC_CHILD_COMBO);
			EnableWindow(child_combo, 0);

			break;
		}

		case WM_NOTIFY:	
		{
			NM_UPDOWN * pnmud = (NM_UPDOWN *) lParam;
			result = 1;
			break;
		}

		case WM_COMMAND:
			switch(lo)
			{
				case IDC_LIST1:
					switch (hi)
					{
						case LBN_DBLCLK:
							SendMessage(hwndDlg, WM_COMMAND, IDOK, 0);
							result = 1;
							break;
					}
					break;

				case IDC_START:
				{
					HWND list = GetDlgItem(hwndDlg, IDC_LIST1);
					bool loop = (IsDlgButtonChecked(hwndDlg, IDC_CHECK1) != 0);
					bool backward = (IsDlgButtonChecked(hwndDlg, IDC_BACKWARD) != 0);
					bool xlat_blend = (IsDlgButtonChecked(hwndDlg, IDC_XLAT_BLEND) != 0);
					bool no_xlat_offset = (IsDlgButtonChecked(hwndDlg, IDC_NO_XLAT_OFFSET) != 0);
					bool overlay = (IsDlgButtonChecked(hwndDlg, IDC_OVERLAY) != 0);

					unsigned int flags = (backward) ? Animation::BACKWARDS : Animation::FORWARD;
					if (loop)
					{
						flags |= Animation::LOOP;
					}
					if (xlat_blend)
					{
						switch (up_axis)
						{
						case DeformableObject::POS_X:
						case DeformableObject::NEG_X:
							flags |= Animation::XLAT_BLEND_X;
							break;
						case DeformableObject::POS_Y:
						case DeformableObject::NEG_Y:
							flags |= Animation::XLAT_BLEND_Y;
							break;
						case DeformableObject::POS_Z:
						case DeformableObject::NEG_Z:
							flags |= Animation::XLAT_BLEND_Z;
							break;
						}
					}
					if (no_xlat_offset)
					{
						flags |= Animation::NO_XLAT_OFFSET;
					}
					if (overlay)
					{
						flags |= Animation::OVERLAY;
					}

					int sel = SendMessage(list, LB_GETCURSEL, 0, 0);
					if (sel == -1)
					{
						Beep(0, 0);
					}
					else
					{
						SendMessage(list, LB_GETTEXT, sel, (long) SelectedScript[0]);

						char buffer[32];
						GetDlgItemText(hwndDlg, IDC_EDIT1, buffer, 32);
						float trans = atof(buffer);

						GetDlgItemText(hwndDlg, IDC_EDIT2, buffer, 32);
						float time_scale = atof(buffer);

						GetDlgItemText(hwndDlg, IDC_EDIT3, buffer, 32);
						float weight = atof(buffer);

						GetDlgItemText(hwndDlg, IDC_EDIT4, buffer, 32);
						float start_time = atof(buffer);

						//float duration = TheCharacter.get_script_duration(SelectedScript[0]);

						float duration = ANIM->get_duration(script_set_arch[0], SelectedScript[0]);

						GENERAL_NOTICE( make_string( "script duration: %f\n", duration ) );
						//TheCharacter.start_motion(SelectedScript[0], (backward) ? Animation::END : Animation::BEGIN, trans, time_scale, weight, flags);

						if (backward && start_time == 0.0f)
						{
							start_time = Animation::END;
						}

						ScriptSet * set = ScriptSets;
						for (int i = 0; i < NumScriptSets; i++, set++)
						{
							for (int j = 0; j < set->num_scripts; j++)
							{
								if (strcmp(set->scripts[j], SelectedScript[0]) == 0)
								{
									break;
								}
							}

							if (j < set->num_scripts)
							{
								break;
							}
						}


						if (IsDlgButtonChecked(hwndDlg, IDC_LOCKBONE))
						{
							SCRIPT_INST inst;
							INSTANCE_INDEX root;
							CHECK(DEFORM->get_root(TheCharacter, root));

							HWND bone_list = GetDlgItem(hwndDlg, IDC_BONE_LIST);
							int sel = SendMessage(bone_list, CB_GETCURSEL, 0, 0);

							char buffer[80];
							SendMessage(bone_list, CB_GETLBTEXT, sel, (LONG) buffer);

							INSTANCE_INDEX lock = find_child(root, buffer);
							CHECK(DEFORM->start_motion_locked(TheCharacter, inst, set->arch, SelectedScript[0], start_time, trans, time_scale, weight, flags, -1, lock));
						}
						else
						{
							SCRIPT_INST anim_inst;
							if (GR_OK != DEFORM->start_motion(TheCharacter, anim_inst, set->arch, SelectedScript[0], start_time, trans, time_scale, weight, flags, -1.0)) {
								GENERAL_NOTICE( make_string( "failed to start motion: %s\n", SelectedScript[0] ) );
							}
						}

						AnimationStarted = true;
						GENERAL_NOTICE( make_string( "starting motion: %s\n", SelectedScript[0] ) );
					}
					break;
				}

				case IDC_PROPERTIES:
					{
						DialogBox(GlobalInstance, MAKEINTRESOURCE(IDD_SCRIPT_PROPERTIES), hwndDlg, PropertiesDlgProc); 
					}
					break;

				case IDC_LOCKBONE:
				{
					BOOL enable_bone_list = (IsDlgButtonChecked(hwndDlg, IDC_LOCKBONE) != 0);
					HWND bone_list = GetDlgItem(hwndDlg, IDC_BONE_LIST);
					EnableWindow(bone_list, enable_bone_list);

					break;
				}

				case IDCANCEL:
					NumSelectedScripts = 0;
					EndDialog(hwndDlg, lo);
					break;

				case IDC_ATTACH_CHILD:
				{
					U32 num_character_hardpoints;
					if (DEFORM->get_num_hardpoints(TheCharacter, num_character_hardpoints) == GR_OK)
					{
						if (num_character_hardpoints == 0)
						{
							MessageBox(hwndDlg, "Can't attach children - character has no hardpoints.", "Error", MB_OK);
						}
						else
						{
						// browse for child object.

							char filename[_MAX_PATH];
							filename[0] = 0;

							OPENFILENAME ofn;
							ofn.lStructSize			= sizeof(ofn);
							ofn.hwndOwner			= hwndDlg;
							ofn.hInstance			= 0;
							ofn.lpstrFilter			= "3DB files\0*.3db\0CMP files\0*.cmp\0\0";
							ofn.lpstrCustomFilter	= NULL;
							ofn.nMaxCustFilter		= 0;
							ofn.nFilterIndex		= 0;
							ofn.lpstrFile			= filename;
							ofn.nMaxFile			= _MAX_PATH;
							ofn.lpstrFileTitle		= NULL;
							ofn.nMaxFileTitle		= 0;
							ofn.lpstrInitialDir		= NULL;
							ofn.lpstrTitle			= "Open child object";
							ofn.Flags				= OFN_FILEMUSTEXIST;
							ofn.nFileOffset			= 0;
							ofn.nFileExtension		= 0;
							ofn.lpstrDefExt			= NULL;
							ofn.lCustData			= NULL;
							ofn.lpfnHook			= NULL;
							ofn.lpTemplateName		= NULL;

							if (GetOpenFileName(&ofn))
							{
								COMPTR<IFileSystem> file;
								DAFILEDESC fdesc = ofn.lpstrFile;

								INSTANCE_INDEX child		= INVALID_INSTANCE_INDEX;
								ARCHETYPE_INDEX child_arch	= INVALID_ARCHETYPE_INDEX;

								if (DACOM->CreateInstance(&fdesc, file) == GR_OK)
								{
								// create archetype.
									child_arch = ENG->create_archetype(ofn.lpstrFile, file);

									if (child_arch != INVALID_ARCHETYPE_INDEX)
									{
									// create instance of child.
										child = ENG->create_instance2(child_arch, 0);

									// don't need a ref to the compound arch.
										ENG->release_archetype(child_arch);
									}
								}

								bool attached = false;

								if (child != INVALID_INSTANCE_INDEX)
								{
								// select hardpoints on parent & child.


								// traverse child hierarchy (if compound), enumerating hardpoints on each piece.

									HPInstanceList child_hardpoints;	

									INSTANCE_INDEX obj = child;

									do
									{
										ARCHETYPE_INDEX a = ENG->get_instance_archetype(obj);

									// enumerate the hardpoints on this part.
										HARDPOINT->enumerate_hardpoints(AttachHPCallback, a, &child_hardpoints);

									// now go set the correct instance index for all the hardpoints just added.
										HPInstanceList::iterator l = child_hardpoints.begin();
										while (l != child_hardpoints.end())
										{
											HPInstanceList::value_type & v = *l;

											if (v.instance == INVALID_INSTANCE_INDEX)
											{
												v.instance = obj;
											}

											l++;
										}

										ENG->release_archetype(a);

										obj = ENG->get_instance_child_next(child, 0, (obj == child) ? INVALID_INSTANCE_INDEX : obj);

									} while (obj != INVALID_INSTANCE_INDEX);


								// NOW we've got a list of all the hardpoints on the children & which instance each is on.
									if (child_hardpoints.size() != 0)
									{
										AttachHPInfo info(TheCharacter, child_hardpoints);
										if (DialogBoxParam(GlobalInstance, MAKEINTRESOURCE(IDD_CHOOSE_HP), hwndDlg, ChooseHPDlgProc, reinterpret_cast<LPARAM>(&info)) == IDOK)
										{
											const HardpointDesc * char_hp;
											DEFORM->get_hardpoints(TheCharacter, char_hp);

											const HardpointDesc * parent_hardpoint = char_hp + info.parent_hp_index;

											HPInstanceInfo & child_hardpoint = child_hardpoints[info.child_hp_index];

											int result = HARDPOINT->connect(parent_hardpoint->object, parent_hardpoint->name, child_hardpoint.instance, child_hardpoint.hp.c_str());

											if (result != 0)
											{
												MessageBox(hwndDlg, "Connection failed.", "Error", MB_OK);
											}
											else
											{
												attached = true;
											// Add child to IDC_CHILD_COMBO.

												AttachedChildren.push_back(child_hardpoint.instance);
							
												if (AttachedChildren.size() == 1)
												{
													HWND remove_child_button = GetDlgItem(hwndDlg, IDC_REMOVE_CHILD);
													EnableWindow(remove_child_button, 1);

													HWND child_combo = GetDlgItem(hwndDlg, IDC_CHILD_COMBO);
													EnableWindow(child_combo, 1);
												}
											}
										}
									}
									else
									{
									// child has no hardpoints.
										MessageBox(hwndDlg, "Can't attach child - child has no hardpoints.", "Error", MB_OK);
									}
								}

								if (attached)
								{
								// add to list of attached children.
									HWND child_list = GetDlgItem(hwndDlg, IDC_CHILD_COMBO);
									int index = SendMessage(child_list, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ofn.lpstrFile + ofn.nFileOffset));

									SendMessage(child_list, CB_SETCURSEL, index, 0);
								}
								else
								{
									ENG->destroy_instance(child);
								}
							}
						}
					}

					break;
				}

				case IDC_REMOVE_CHILD:
				{
				// Get selected in IDC_CHILD_COMBO.
					HWND child_combo = GetDlgItem(hwndDlg, IDC_CHILD_COMBO);
					int sel = SendMessage(child_combo, CB_GETCURSEL, 0, 0);

					INSTANCE_INDEX child_to_remove = AttachedChildren[sel];

				// disconnect & destroy child object.

					INSTANCE_INDEX parent = ENG->get_instance_parent(child_to_remove);

					assert(parent != INVALID_INSTANCE_INDEX);

					ENG->destroy_joint(parent, child_to_remove);

					ENG->destroy_instance(child_to_remove);

					AttachedChildren.erase(AttachedChildren.begin() + sel);

				// Remove string from IDC_CHILD_COMBO.
					SendMessage(child_combo, CB_DELETESTRING, sel, 0);
					SendMessage(child_combo, CB_SETCURSEL, 0, 0);

					if (AttachedChildren.size() == 0)
					{
						HWND remove_child_button = GetDlgItem(hwndDlg, IDC_REMOVE_CHILD);
						EnableWindow(remove_child_button, 0);

						HWND child_combo = GetDlgItem(hwndDlg, IDC_CHILD_COMBO);
						EnableWindow(child_combo, 0);

					}
					break;
				}
			}
			break;
	}

	return result;
}

//


