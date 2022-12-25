//************************************************************************** 
//* Asciiexp.cpp	- Ascii File Exporter
//* 
//* By Christer Janson
//* Kinetix Development
//*
//* January 20, 1997 CCJ Initial coding
//*
//* This module contains the DLL startup functions
//*
//* Copyright (c) 1997, All Rights Reserved. 
//***************************************************************************

#include <crtdbg.h>
#include <process.h>
#include "asciiexp.h"
#include "mnbigmat.h"	// BigMatrix
//#include "mapping.h"	// UVW
#include "lod.h"
//#include "h:\mikes\common\timer.h"

#ifdef _DEBUG
#pragma warning( disable : 4189 )		// local variable is initialized but not referenced
#pragma warning( disable : 4100 )		// 
#endif
#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( error : 4700 ) // local variable used without having been initialized
#pragma warning( push, 4 )

Matrix3 world_adjust(TRUE);
Matrix3 root_adjust_m(TRUE);

HINSTANCE hInstance;
int controlsInit = FALSE;

static BOOL showPrompts;

 long large_size = 0;
 long small_size = 0;

 double large_time = 0.0;
 double small_time = 0.0;

// Class ID. These must be unique and randomly generated!!
// If you use this as a sample project, this is the first thing
// you should change!
#if MAX_RELEASE == 2500
//#define OFL_EXP_CLASS_ID	Class_ID(0x85548e0c, 0x4a26450d)
#define OFL_EXP_CLASS_ID	Class_ID(0x7d440328, 0x9274896)
#elif MAX_RELEASE >= 3000
//#define OFL_EXP_CLASS_ID	Class_ID(0x85548e0d, 0x4a26450e)
#if USE_DA_MESH
#define OFL_EXP_CLASS_ID	Class_ID(0x4cfb5db8, 0x71b8260a)
#else
#define OFL_EXP_CLASS_ID	Class_ID(0x4cfb5db7, 0x71b82609)
#endif
#endif

int verbose_level = 1;
int convex_hull_flag = 1;
float scale_factor = 1.0f;
float fps = 30.0f;
//int old_style = 0;
float face_normal_tolerance = FACE_NORMAL_TOLERANCE;
float vertex_normal_tolerance = VERTEX_NORMAL_TOLERANCE;

// set by the user
int user_head;
int export_vertex_colors;
int split_flag;
int mip_flag;
int dither_flag;
//int flag_565;
int txt_depth;
int default_mat_flag;
int export_mesh;
int export_skeleton;
int export_animation;
int export_lights;
int export_cameras;
int export_selected;
//int biped_y_up;
float lod_mtl_weight;
float lod_uv_weight;
int ignore_warnings;
int no_physics;
int center_mass;
int ik_extents;
int txt_flag;

bool remove_constant_channels = false;

// character options set by data
bool exporting_deformable = false; // physique, bones pro, or MESH_BONE
bool exporting_bones_pro = false;
bool exporting_physique = false;

// character options set by user
int selected_anims_only;
int use_loose_joints;				// default is spherical
int export_root_animation;
int allign_root_to_world;
int allign_heading;
int relative_deformable;
float adjust_rotate_x;				// world coord adjust (used for z up to y up)
float adjust_rotate_y;
float adjust_rotate_z;
int allow_loose_joints;
int allow_translational_joints;		// same as allow_loose_joints for now

FILE *std_err = NULL;
FILE *std_out = NULL;
char body_name[256] = {0};
char dest_path[256] = {0};
char *root_name = NULL;
char date_name[32] = {0};

LARGE_INTEGER timer_start;
LARGE_INTEGER timer_stop;

Matrix3 biped_adjust(TRUE);

/*
extern void  __cdecl sortQ(void *, size_t, size_t, 
					 int (__cdecl *)(const void *, const void *, const void *),
					 const void *comp_lib,
					 void (__cdecl *swap)(void *, const int id1, const int id2),
					 void *swap_lib);
*/

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG /*fdwReason*/, LPVOID /*lpvReserved*/) 
{
	hInstance = hinstDLL;

	// Initialize the custom controls. This should be done only once.
	if (!controlsInit) {
		controlsInit = TRUE;
		InitCustomControls(hInstance);
		InitCommonControls();
	}
	
	return (TRUE);
}


__declspec( dllexport ) const TCHAR* LibDescription() 
{
	return GetString(IDS_LIBDESCRIPTION);
}

/// MUST CHANGE THIS NUMBER WHEN ADD NEW CLASS 
__declspec( dllexport ) int LibNumberClasses()
{
	return 1;
}


ClassDesc* GetAsciiExpDesc(); // forward declare

__declspec( dllexport ) ClassDesc* LibClassDesc(int i) 
{
	switch(i) {
	case 0: return GetAsciiExpDesc();
	default: return 0;
	}
}

__declspec( dllexport ) ULONG LibVersion() 
{
	return VERSION_3DSMAX;
}

class AsciiExpClassDesc:public ClassDesc {
public:
	int				IsPublic() { return 1; }
	virtual void*	Create(BOOL loading = FALSE)
	{
		return new AsciiExp( options );
	} 
	const TCHAR*	ClassName() { return GetString(IDS_ASCIIEXP); }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; } 
	Class_ID		ClassID() { return OFL_EXP_CLASS_ID; }
	const TCHAR*	Category() { return GetString(IDS_CATEGORY); }
	BOOL			NeedsToSave() { return TRUE; }
	IOResult 		Save(ISave *isave);
	IOResult 		Load(ILoad *iload);

	AsciiExpClassDesc()
	{ 
		options.Init();
	}
	ExpOptions options;
};

static AsciiExpClassDesc AsciiExpDesc;

ClassDesc* GetAsciiExpDesc()
{
	return &AsciiExpDesc;
}

// writes exporter options to a max file
// also see virtual void SetUserPropInt(const TSTR &key,int val)=0;
// for other ways to save user data
IOResult AsciiExpClassDesc::Save(ISave *isave)
{
	ULONG bytes_written;

	//isave->BeginChunk(IDC_SAVE_MAX_OPT);
	//isave->Write(&options.nsave_options_to_max_file, sizeof(options.nsave_options_to_max_file), &bytes_written);
	//isave->EndChunk();

	if( options.nsave_options_to_max_file )
	{
		isave->BeginChunk(IDC_DENS);
		isave->Write(&options.nDensity, sizeof(options.nDensity), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_SCALE);
		isave->Write(&options.nScale, sizeof(options.nScale), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_MIP);
		isave->Write(&options.nmip_flag, sizeof(options.nmip_flag), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_565);
		isave->Write(&options.nflag_565, sizeof(options.nflag_565), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_888);
		isave->Write(&options.nflag_888, sizeof(options.nflag_888), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_565);
		isave->Write(&options.nflag_565, sizeof(options.nflag_565), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_DITHER);
		isave->Write(&options.ndither_flag, sizeof(options.ndither_flag), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_SPLIT);
		isave->Write(&options.nsplit_flag, sizeof(options.nsplit_flag), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_DEFAULT_MAT);
		isave->Write(&options.ndefault_mat_flag, sizeof(options.ndefault_mat_flag), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_MESH);
		isave->Write(&options.nexport_mesh, sizeof(options.nexport_mesh), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_ANIMATION);
		isave->Write(&options.nexport_animation, sizeof(options.nexport_animation), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_LIGHT);
		isave->Write(&options.nexport_lights, sizeof(options.nexport_lights), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_CAMERA);
		isave->Write(&options.nexport_cameras, sizeof(options.nexport_cameras), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_SELECTED);
		isave->Write(&options.nexport_selected, sizeof(options.nexport_selected), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_NO_PHYSICS);
		isave->Write(&options.nno_physics, sizeof(options.nno_physics), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_CENTER_MASS);
		isave->Write(&options.ncenter_mass, sizeof(options.ncenter_mass), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_VERTEX_COLOR);
		isave->Write(&options.nvertex_color, sizeof(options.nvertex_color), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_IK_EXT);
		isave->Write(&options.nik_extents, sizeof(options.nik_extents), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_SM_GRP);
		isave->Write(&options.nsm_groups, sizeof(options.nsm_groups), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_EX_AVI);
		isave->Write(&options.nex_avi, sizeof(options.nex_avi), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_EX_TXT);
		isave->Write(&options.nex_txt, sizeof(options.nex_txt), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_NOT_UNIQUE);
		isave->Write(&options.nnon_unique_name, sizeof(options.nnon_unique_name), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_ALLOW_LOSE_J);
		isave->Write(&options.nallow_loose_j, sizeof(options.nallow_loose_j), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_ALLIGN_HEADING);
		isave->Write(&options.nallign_heading, sizeof(options.nallign_heading), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_REL_ANIM);
		isave->Write(&options.nrelative_deformable, sizeof(options.nrelative_deformable), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_SEL_ANIM_ONLY);
		isave->Write(&options.nsel_anim_only, sizeof(options.nsel_anim_only), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_LOOSE_J);
		isave->Write(&options.nloose_j, sizeof(options.nloose_j), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_ROOT_ANIM);
		isave->Write(&options.nroot_anim, sizeof(options.nroot_anim), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_IN_WORLD);
		isave->Write(&options.nin_world, sizeof(options.nin_world), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_IGNORE_WARNINGS);
		isave->Write(&options.nignore_warnings, sizeof(options.nignore_warnings), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_LOD_PERCENT);
		isave->Write(&options.nLodPercent, sizeof(options.nLodPercent), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_LOD_CLOSEST);
		isave->Write(&options.nLodClosestDist, sizeof(options.nLodClosestDist), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_LOD_FURTHEST);
		isave->Write(&options.nLodFurthestDist, sizeof(options.nLodFurthestDist), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_LOD_DROPOUT);
		isave->Write(&options.nLodDropOut, sizeof(options.nLodDropOut), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_MTL_BOUNDARY);
		isave->Write(&options.nMtlBoundary, sizeof(options.nMtlBoundary), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_TXT_WEIGHT);
		isave->Write(&options.nTxtWeight, sizeof(options.nTxtWeight), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_ROTATE_X);
		isave->Write(&options.nadjust_rotate_x, sizeof(options.nadjust_rotate_x), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_ROTATE_Y);
		isave->Write(&options.nadjust_rotate_y, sizeof(options.nadjust_rotate_y), &bytes_written);
		isave->EndChunk();

		isave->BeginChunk(IDC_ROTATE_Z);
		isave->Write(&options.nadjust_rotate_z, sizeof(options.nadjust_rotate_z), &bytes_written);
		isave->EndChunk();
	}

	return IO_OK;
}

// loads exporter options from a max file
IOResult AsciiExpClassDesc::Load(ILoad *iload)
{
	ULONG bytes_read;
	IOResult res;

	while( IO_OK == (res = iload->OpenChunk()) )
	{
		const USHORT chunk_id = iload->CurChunkID();
		switch( chunk_id )
		{
			//case IDC_SAVE_MAX_OPT:
				//res = iload->Read(&options.nsave_options_to_max_file, sizeof(options.nsave_options_to_max_file), &bytes_read);
				//break;
			case IDC_DENS:
				res = iload->Read(&options.nDensity, sizeof(options.nDensity), &bytes_read);
				break;
			case IDC_SCALE:
				res = iload->Read(&options.nScale, sizeof(options.nScale), &bytes_read);
				break;
			case IDC_MIP:
				res = iload->Read(&options.nmip_flag, sizeof(options.nmip_flag), &bytes_read);
				break;
			case IDC_565:
				res = iload->Read(&options.nflag_565, sizeof(options.nflag_565), &bytes_read);
				break;
			case IDC_888:
				res = iload->Read(&options.nflag_888, sizeof(options.nflag_888), &bytes_read);
				break;
			case IDC_DITHER:
				res = iload->Read(&options.ndither_flag, sizeof(options.ndither_flag), &bytes_read);
				break;
			case IDC_SPLIT:
				res = iload->Read(&options.nsplit_flag, sizeof(options.nsplit_flag), &bytes_read);
				break;
			case IDC_DEFAULT_MAT:
				res = iload->Read(&options.ndefault_mat_flag, sizeof(options.ndefault_mat_flag), &bytes_read);
				break;
			case IDC_MESH:
				res = iload->Read(&options.nexport_mesh, sizeof(options.nexport_mesh), &bytes_read);
				break;
			case IDC_ANIMATION:
				res = iload->Read(&options.nexport_animation, sizeof(options.nexport_animation), &bytes_read);
				break;
			case IDC_LIGHT:
				res = iload->Read(&options.nexport_lights, sizeof(options.nexport_lights), &bytes_read);
				break;
			case IDC_CAMERA:
				res = iload->Read(&options.nexport_cameras, sizeof(options.nexport_cameras), &bytes_read);
				break;
			case IDC_SELECTED:
				res = iload->Read(&options.nexport_selected, sizeof(options.nexport_selected), &bytes_read);
				break;
			case IDC_NO_PHYSICS:
				res = iload->Read(&options.nno_physics, sizeof(options.nno_physics), &bytes_read);
				break;
			case IDC_CENTER_MASS:
				res = iload->Read(&options.ncenter_mass, sizeof(options.ncenter_mass), &bytes_read);
				break;
			case IDC_VERTEX_COLOR:
				res = iload->Read(&options.nvertex_color, sizeof(options.nvertex_color), &bytes_read);
				break;
			case IDC_IK_EXT:
				res = iload->Read(&options.nik_extents, sizeof(options.nik_extents), &bytes_read);
				break;
			case IDC_SM_GRP:
				res = iload->Read(&options.nsm_groups, sizeof(options.nsm_groups), &bytes_read);
				break;
			case IDC_EX_AVI:
				res = iload->Read(&options.nex_avi, sizeof(options.nex_avi), &bytes_read);
				break;
			case IDC_EX_TXT:
				res = iload->Read(&options.nex_txt, sizeof(options.nex_txt), &bytes_read);
				break;
			case IDC_NOT_UNIQUE:
				res = iload->Read(&options.nnon_unique_name, sizeof(options.nnon_unique_name), &bytes_read);
				break;
			case IDC_ALLOW_LOSE_J:
				res = iload->Read(&options.nallow_loose_j, sizeof(options.nallow_loose_j), &bytes_read);
				break;
			case IDC_ALLIGN_HEADING:
				res = iload->Read(&options.nallign_heading, sizeof(options.nallign_heading), &bytes_read);
				break;
			case IDC_REL_ANIM:
				res = iload->Read(&options.nrelative_deformable, sizeof(options.nrelative_deformable), &bytes_read);
				break;
			case IDC_SEL_ANIM_ONLY:
				res = iload->Read(&options.nsel_anim_only, sizeof(options.nsel_anim_only), &bytes_read);
				break;
			case IDC_LOOSE_J:
				res = iload->Read(&options.nloose_j, sizeof(options.nloose_j), &bytes_read);
				break;
			case IDC_ROOT_ANIM:
				res = iload->Read(&options.nroot_anim, sizeof(options.nroot_anim), &bytes_read);
				break;
			case IDC_IN_WORLD:
				res = iload->Read(&options.nin_world, sizeof(options.nin_world), &bytes_read);
				break;
			case IDC_IGNORE_WARNINGS:
				res = iload->Read(&options.nignore_warnings, sizeof(options.nignore_warnings), &bytes_read);
				break;
			case IDC_LOD_PERCENT:
				res = iload->Read(&options.nLodPercent, sizeof(options.nLodPercent), &bytes_read);
				break;
			case IDC_LOD_CLOSEST:
				res = iload->Read(&options.nLodClosestDist, sizeof(options.nLodClosestDist), &bytes_read);
				break;
			case IDC_LOD_FURTHEST:
				res = iload->Read(&options.nLodFurthestDist, sizeof(options.nLodFurthestDist), &bytes_read);
				break;
			case IDC_LOD_DROPOUT:
				res = iload->Read(&options.nLodDropOut, sizeof(options.nLodDropOut), &bytes_read);
				break;
			case IDC_MTL_BOUNDARY:
				res = iload->Read(&options.nMtlBoundary, sizeof(options.nMtlBoundary), &bytes_read);
				break;
			case IDC_TXT_WEIGHT:
				res = iload->Read(&options.nTxtWeight, sizeof(options.nTxtWeight), &bytes_read);
				break;
			case IDC_ROTATE_X:
				res = iload->Read(&options.nadjust_rotate_x, sizeof(options.nadjust_rotate_x), &bytes_read);
				break;
			case IDC_ROTATE_Y:
				res = iload->Read(&options.nadjust_rotate_y, sizeof(options.nadjust_rotate_y), &bytes_read);
				break;
			case IDC_ROTATE_Z:
				res = iload->Read(&options.nadjust_rotate_z, sizeof(options.nadjust_rotate_z), &bytes_read);
				break;
		}

		iload->CloseChunk();
		if(res != IO_OK)
		{
			return res;
		}
	}
	
	return IO_OK;
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256]; buf[0] = 0;

	if (hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;

	return NULL;
}

AsciiExp::AsciiExp( ExpOptions & _options ) : options( _options )
{
	nStaticFrame = 0;
	ip = NULL;
	batch_file_name[0] = 0;
}

AsciiExp::~AsciiExp()
{
}

int AsciiExp::ExtCount()
{
	return 6;
}

const TCHAR * AsciiExp::Ext(int n)
{
	switch(n)
	{
		case 0:
			return _T("3DB");
		case 1:
			return _T("CMP");
		case 2:
			return _T("ANM");
		case 3:
			return _T("DFM");
		case 4:
			return _T("NRB");
		case 5:
			return _T("BEZ");
	}

	return _T("");
}

const TCHAR * AsciiExp::LongDesc()
{
	return GetString(IDS_LONGDESC);
}

const TCHAR * AsciiExp::ShortDesc()
{
	return GetString(IDS_SHORTDESC);
}

const TCHAR * AsciiExp::AuthorName() 
{
	return _T("Christer Janson");
}

const TCHAR * AsciiExp::CopyrightMessage() 
{
	return GetString(IDS_COPYRIGHT);
}

const TCHAR * AsciiExp::OtherMessage1() 
{
	return _T("");
}

const TCHAR * AsciiExp::OtherMessage2() 
{
	return _T("");
}

unsigned int AsciiExp::Version()
{
	return 100;
}

static BOOL CALLBACK AboutBoxDlgProc(HWND hWnd, UINT msg, 
	WPARAM wParam, LPARAM /*lParam*/)
{
	switch (msg) {
	case WM_INITDIALOG:
		CenterWindow(hWnd, GetParent(hWnd)); 
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hWnd, 1);
			break;
		}
		break;
		default:
			return FALSE;
	}
	return TRUE;
}       

void AsciiExp::ShowAbout(HWND hWnd)
{
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutBoxDlgProc, 0);
}

void GetUI( HWND hWnd )
{
	Interval animRange;
	ISpinnerControl  *spin;

	AsciiExp *exp = (AsciiExp*)GetWindowLong(hWnd, GWL_USERDATA); 

	spin = GetISpinner(GetDlgItem(hWnd, IDC_DENS_SPIN)); 
	exp->SetDensity(spin->GetFVal());
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_SCALE_SPIN)); 
	exp->SetScale(spin->GetFVal());
	ReleaseISpinner(spin);

	exp->SetMipFlag(IsDlgButtonChecked(hWnd, IDC_MIP)); 
	exp->Set565Flag(IsDlgButtonChecked(hWnd, IDC_565)); 
	exp->Set888Flag(IsDlgButtonChecked(hWnd, IDC_888));
	exp->SetDitherFlag(IsDlgButtonChecked(hWnd, IDC_DITHER));
	exp->SetDefaultMatFlag(IsDlgButtonChecked(hWnd, IDC_DEFAULT_MAT)); 
	exp->SetSplitFlag(IsDlgButtonChecked(hWnd, IDC_SPLIT)); 
	exp->SetLightsFlag(IsDlgButtonChecked(hWnd, IDC_LIGHT));
	exp->SetCamerasFlag(IsDlgButtonChecked(hWnd, IDC_CAMERA)); 
	exp->SetSelectedFlag(IsDlgButtonChecked(hWnd, IDC_SELECTED));

	exp->SetMeshFlag(IsDlgButtonChecked(hWnd, IDC_MESH)); 
	//exp->SetSkeletonFlag(IsDlgButtonChecked(hWnd, IDC_SKELETON)); 
	exp->SetAnimationFlag(IsDlgButtonChecked(hWnd, IDC_ANIMATION)); 
	//exp->SetBipedYUpFlag(IsDlgButtonChecked(hWnd, IDC_BIPED_Y_UP));
	exp->SetIgnoreWarningsFlag(IsDlgButtonChecked(hWnd, IDC_IGNORE_WARNINGS));
	exp->SetNoPhysics(IsDlgButtonChecked(hWnd, IDC_NO_PHYSICS));
	exp->SetCenterMass(IsDlgButtonChecked(hWnd, IDC_CENTER_MASS));
	exp->SetVertexColor(IsDlgButtonChecked(hWnd, IDC_VERTEX_COLOR));
	exp->SetIkExt(IsDlgButtonChecked(hWnd, IDC_IK_EXT));
	exp->SetSmGrp(IsDlgButtonChecked(hWnd, IDC_SM_GRP));
	exp->SetExAvi(IsDlgButtonChecked(hWnd, IDC_EX_AVI));
	exp->SetExTxt(IsDlgButtonChecked(hWnd, IDC_EX_TXT));
	exp->SetNonUniqueName(IsDlgButtonChecked(hWnd, IDC_NOT_UNIQUE));
	exp->SetAllowLooseJ(IsDlgButtonChecked(hWnd, IDC_ALLOW_LOSE_J));

	exp->SetAllignHeading(IsDlgButtonChecked(hWnd, IDC_ALLIGN_HEADING));
	exp->SetRelAnim(IsDlgButtonChecked(hWnd, IDC_REL_ANIM));
	//exp->SetHead(IsDlgButtonChecked(hWnd, IDC_HEAD));
	exp->SetSelAnimOnly(IsDlgButtonChecked(hWnd, IDC_SEL_ANIM_ONLY));
	exp->SetLooseJ(IsDlgButtonChecked(hWnd, IDC_LOOSE_J));
	exp->SetRootAnim(IsDlgButtonChecked(hWnd, IDC_ROOT_ANIM));
	exp->SetInWorld(IsDlgButtonChecked(hWnd, IDC_IN_WORLD));
	
	spin = GetISpinner(GetDlgItem(hWnd, IDC_LOD_PERCENT_SPIN)); 
	exp->SetLodPercent(spin->GetFVal());
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_LOD_CLOSEST_SPIN)); 
	exp->SetLodClosest(spin->GetFVal());
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_LOD_FURTHEST_SPIN)); 
	exp->SetLodFurthest(spin->GetFVal());
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_LOD_DROPOUT_SPIN)); 
	exp->SetLodDropOut(spin->GetFVal());
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_MTL_BOUNDARY_SPIN)); 
	exp->SetMtlBoundary(spin->GetFVal());
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_TXT_WEIGHT_SPIN)); 
	exp->SetTxtWeight(spin->GetFVal());
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_ROTATE_X_SPIN)); 
	exp->SetAdjustRotateX(spin->GetFVal());
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_ROTATE_Y_SPIN)); 
	exp->SetAdjustRotateY(spin->GetFVal());
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_ROTATE_Z_SPIN)); 
	exp->SetAdjustRotateZ(spin->GetFVal());
	ReleaseISpinner(spin);

	/*
	spin = GetISpinner(GetDlgItem(hWnd, IDC_LOD_COUNT_SPIN)); 
	exp->SetLodCount(spin->GetIVal());
	ReleaseISpinner(spin);
	*/

	// debug options
	{
		char options[256]; options[0] = 0;
		ICustEdit *edit = GetICustEdit( GetDlgItem(hWnd, IDC_EDIT) );
		edit->GetText( options, 256 );
		ReleaseICustEdit( edit );

		// parse debug options
		const char * start = NULL;

		face_normal_tolerance = FACE_NORMAL_TOLERANCE;
		vertex_normal_tolerance = VERTEX_NORMAL_TOLERANCE;
		start = strstr( options, "-n_angle");
		if(start != NULL)
		{
			const char * value = strchr( start, ' ' ) + 1;
			if(value)
			{
				face_normal_tolerance = 
				vertex_normal_tolerance = (float)atof(value);
			}
		}
		else
		{
			start = strstr( options, "-vn_angle");
			if(start != NULL)
			{
				const char * value = strchr( start, ' ' ) + 1;
				if(value)
				{
					vertex_normal_tolerance = (float)atof(value);
				}
			}
			
			start = strstr( options, "-fn_angle");
			if(start != NULL)
			{
				const char * value = strchr( start, ' ' ) + 1;
				if(value)
				{
					face_normal_tolerance = (float)atof(value);
				}
			}
		}

		// don't export textures
		/*
		txt_flag = 1;
		start = strstr( options, "-no_txt");
		if(start != NULL)
		{
			txt_flag = 0;
		}
		*/

		/*
		selected_anims_only = 0;
		start = strstr( options, "-sa");
		if(start != NULL)
		{
			selected_anims_only = 1;
		}
		*/
	}
}

void SetUI( HWND hWnd )
{
	Interval animRange;
	ISpinnerControl  *spin;

	AsciiExp *exp = (AsciiExp*)GetWindowLong(hWnd, GWL_USERDATA); 

	// Setup the spinner controls for the floating point precision 
	spin = GetISpinner(GetDlgItem(hWnd, IDC_DENS_SPIN)); 
	spin->LinkToEdit(GetDlgItem(hWnd,IDC_DENS), EDITTYPE_FLOAT ); 
	spin->SetLimits(0.000001f, 10000.0f, TRUE);
	//spin->SetResetValue(float v);
	spin->SetScale(1.0f);
	spin->SetValue(exp->GetDensity(), FALSE);
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_SCALE_SPIN)); 
	spin->LinkToEdit(GetDlgItem(hWnd,IDC_SCALE), EDITTYPE_FLOAT ); 
	spin->SetLimits(0.0f, 1000000.0f, TRUE);
	spin->SetResetValue(1.0f);
	spin->SetScale(1.0f);
	spin->SetValue(exp->GetScale(), FALSE);
	ReleaseISpinner(spin);

	CheckDlgButton(hWnd, IDC_MIP, exp->GetMipFlag());
	CheckDlgButton(hWnd, IDC_565, exp->Get565Flag());
	CheckDlgButton(hWnd, IDC_888, exp->Get888Flag());
	CheckDlgButton(hWnd, IDC_DITHER, exp->GetDitherFlag());
	CheckDlgButton(hWnd, IDC_DEFAULT_MAT, exp->GetDefaultMatFlag());
	CheckDlgButton(hWnd, IDC_SPLIT, exp->GetSplitFlag());
	CheckDlgButton(hWnd, IDC_LIGHT, exp->GetLightsFlag());
	CheckDlgButton(hWnd, IDC_CAMERA, exp->GetCamerasFlag());
	CheckDlgButton(hWnd, IDC_SELECTED, exp->GetSelectedFlag());
	
	CheckDlgButton(hWnd, IDC_MESH, exp->GetMeshFlag());
	//CheckDlgButton(hWnd, IDC_SKELETON, exp->GetSkeletonFlag());
	CheckDlgButton(hWnd, IDC_ANIMATION, exp->GetAnimationFlag());
	//CheckDlgButton(hWnd, IDC_BIPED_Y_UP, exp->GetBipedYUpFlag());
	CheckDlgButton(hWnd, IDC_IGNORE_WARNINGS, exp->GetIgnoreWarningsFlag());
	CheckDlgButton(hWnd, IDC_NO_PHYSICS, exp->GetNoPhysics());
	CheckDlgButton(hWnd, IDC_CENTER_MASS, exp->GetCenterMass());
	CheckDlgButton(hWnd, IDC_VERTEX_COLOR, exp->GetVertexColor());
	CheckDlgButton(hWnd, IDC_IK_EXT, exp->GetIkExt());
	CheckDlgButton(hWnd, IDC_SM_GRP, exp->GetSmGrp());
	CheckDlgButton(hWnd, IDC_EX_AVI, exp->GetExAvi());
	CheckDlgButton(hWnd, IDC_EX_TXT, exp->GetExTxt());
	CheckDlgButton(hWnd, IDC_NOT_UNIQUE, exp->GetNonUniqueName());
	CheckDlgButton(hWnd, IDC_ALLOW_LOSE_J, exp->GetAllowLooseJ());

	CheckDlgButton(hWnd, IDC_ALLIGN_HEADING, exp->GetAllignHeading());
	CheckDlgButton(hWnd, IDC_REL_ANIM, exp->GetRelAnim());
	//CheckDlgButton(hWnd, IDC_HEAD, exp->GetHead());
	CheckDlgButton(hWnd, IDC_SEL_ANIM_ONLY, exp->GetSelAnimOnly());
	CheckDlgButton(hWnd, IDC_LOOSE_J, exp->GetLooseJ());
	CheckDlgButton(hWnd, IDC_ROOT_ANIM, exp->GetRootAnim());
	CheckDlgButton(hWnd, IDC_IN_WORLD, exp->GetInWorld());

	spin = GetISpinner(GetDlgItem(hWnd, IDC_LOD_PERCENT_SPIN)); 
	spin->LinkToEdit(GetDlgItem(hWnd,IDC_LOD_PERCENT), EDITTYPE_FLOAT ); 
	spin->SetLimits(0.0f, 100.0f, TRUE);
	spin->SetResetValue(100.0f);
	spin->SetScale(1.0f);
	spin->SetValue(exp->GetLodPercent(), FALSE);
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_LOD_CLOSEST_SPIN)); 
	spin->LinkToEdit(GetDlgItem(hWnd,IDC_LOD_CLOSEST), EDITTYPE_FLOAT ); 
	spin->SetLimits(0.0f, 10000000.0f, TRUE); 
	spin->SetScale(10.0f);
	spin->SetValue(exp->GetLodClosest(), FALSE);
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_LOD_FURTHEST_SPIN)); 
	spin->LinkToEdit(GetDlgItem(hWnd,IDC_LOD_FURTHEST), EDITTYPE_FLOAT ); 
	spin->SetLimits(0.0f, 10000000.0f, TRUE); 
	spin->SetScale(10.0f);
	spin->SetValue(exp->GetLodFurthest(), FALSE);
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_LOD_DROPOUT_SPIN)); 
	spin->LinkToEdit(GetDlgItem(hWnd,IDC_LOD_DROPOUT), EDITTYPE_FLOAT ); 
	spin->SetLimits(0.0f, 10000000.0f, TRUE); 
	spin->SetScale(10.0f);
	spin->SetValue(exp->GetLodDropOut(), FALSE);
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_MTL_BOUNDARY_SPIN)); 
	spin->LinkToEdit(GetDlgItem(hWnd,IDC_MTL_BOUNDARY), EDITTYPE_FLOAT ); 
	spin->SetLimits(0.0f, 1000.0f, TRUE); 
	spin->SetScale(1.0f);
	spin->SetValue(exp->GetMtlBoundary(), FALSE);
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_TXT_WEIGHT_SPIN)); 
	spin->LinkToEdit(GetDlgItem(hWnd,IDC_TXT_WEIGHT), EDITTYPE_FLOAT ); 
	spin->SetLimits(0.01f, 0.99f, TRUE); 
	spin->SetScale(0.1f);
	spin->SetValue(exp->GetTxtWeight(), FALSE);
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_ROTATE_X_SPIN)); 
	spin->LinkToEdit(GetDlgItem(hWnd,IDC_ROTATE_X), EDITTYPE_FLOAT ); 
	spin->SetLimits(-180.0f, 180.0f, TRUE); 
	spin->SetScale(5.0f);
	spin->SetValue(exp->GetAdjustRotateX(), FALSE);
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_ROTATE_Y_SPIN)); 
	spin->LinkToEdit(GetDlgItem(hWnd,IDC_ROTATE_Y), EDITTYPE_FLOAT ); 
	spin->SetLimits(-180.0f, 180.0f, TRUE); 
	spin->SetScale(5.0f);
	spin->SetValue(exp->GetAdjustRotateY(), FALSE);
	ReleaseISpinner(spin);

	spin = GetISpinner(GetDlgItem(hWnd, IDC_ROTATE_Z_SPIN)); 
	spin->LinkToEdit(GetDlgItem(hWnd,IDC_ROTATE_Z), EDITTYPE_FLOAT ); 
	spin->SetLimits(-180.0f, 180.0f, TRUE); 
	spin->SetScale(5.0f);
	spin->SetValue(exp->GetAdjustRotateZ(), FALSE);
	ReleaseISpinner(spin);

	/*
		spin = GetISpinner(GetDlgItem(hWnd, IDC_LOD_COUNT_SPIN)); 
		spin->LinkToEdit(GetDlgItem(hWnd,IDC_LOD_COUNT), EDITTYPE_INT ); 
		spin->SetLimits(1, 20, TRUE); 
		spin->SetScale(1);
		spin->SetValue(exp->GetLodCount(), FALSE);
		ReleaseISpinner(spin);
	*/
}

// Dialog proc
static BOOL CALLBACK ExportDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Interval animRange;
	ISpinnerControl  *spin;

	AsciiExp *exp = (AsciiExp*)GetWindowLong(hWnd,GWL_USERDATA); 
	switch (msg) {
	case WM_INITDIALOG:
		exp = (AsciiExp*)lParam;
		SetWindowLong(hWnd,GWL_USERDATA,lParam); 
		CenterWindow(hWnd, GetParent(hWnd)); 
		
		{
			ICustEdit *edit = GetICustEdit( GetDlgItem(hWnd,IDC_EDIT) );
			edit->SetText("");
			ReleaseICustEdit(edit);
		}

		SetUI( hWnd );

		break;

	case CC_SPINNER_CHANGE:
		spin = (ISpinnerControl*)lParam; 
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{

		case IDOK:
			
			GetUI( hWnd );
			
			exp->options.Write( NULL );

			EndDialog(hWnd, 1);
			break;

		case IDC_BATCH_EXP:
			
			GetUI( hWnd );
			
			if( exp->GetBatchFileName( hWnd ) )
			{
				exp->options.Write( NULL );
				EndDialog(hWnd, 1);
			}
			else
			{
				EndDialog(hWnd, 0);
			}
			break;

		case IDC_SAVE_OPT:

			GetUI( hWnd );

			exp->options.Write( hWnd );

			EndDialog(hWnd, 0);
			break;

		case IDC_LOAD_OPT:

			exp->options.Read( hWnd );

			SetUI( hWnd );

			//EndDialog(hWnd, 0);
			break;

		case IDCANCEL:
			EndDialog(hWnd, 0);
			break;
		}
		break;
		default:
			return FALSE;
	}
	return TRUE;
}       

// Dummy function for progress bar
DWORD WINAPI fn(LPVOID /*arg*/)
{
	return(0);
}

bool AsciiExp::GetBatchFileName( HWND hWnd )
{
	if( hWnd != NULL )
	{
		OPENFILENAME ofn;
		memset( &ofn, 0, sizeof(ofn) );
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hWnd;
		ofn.hInstance = GetModuleHandle(NULL);
		ofn.lpstrFilter = "Exporter Option Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
		ofn.lpstrFile = batch_file_name;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = NULL;
		ofn.lpstrTitle = "Open Batch Export File";
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_LONGNAMES | 0;

		if( !GetOpenFileName( &ofn ) )
			return false;
	}

	if( batch_file_name[0] == 0 )
	{
		return false;
	}
	else
	{
		return true;
	}
}

void SetGlobalOptions( ExpOptions & options )
{
	split_flag = options.nsplit_flag;
#ifndef _DEBUG // split is allowed in debug only
	split_flag = 0;
#endif
	mip_flag = options.nmip_flag;

	if(options.nflag_565 && options.nflag_888)
	{
		Winprint("Can't specify both 565 and 888 textures! Using 888.\n");
		options.nflag_565 = 0;
	}
	
	default_mat_flag = options.ndefault_mat_flag;
	export_mesh =
	export_skeleton = options.nexport_mesh; //nexport_skeleton;
	export_animation = options.nexport_animation;
	export_lights = options.nexport_lights;
	export_cameras = options.nexport_cameras;
	export_selected = options.nexport_selected;
	//biped_y_up = options.nbiped_y_up;
	ignore_warnings = options.nignore_warnings;
	lod_mtl_weight = options.nMtlBoundary;
	lod_uv_weight = options.nTxtWeight;
	adjust_rotate_x = options.nadjust_rotate_x;
	adjust_rotate_y = options.nadjust_rotate_y;
	adjust_rotate_z = options.nadjust_rotate_z;
	no_physics = options.nno_physics;
	center_mass = options.ncenter_mass;
	export_vertex_colors = options.nvertex_color;
	ik_extents = options.nik_extents;
	allign_heading = options.nallign_heading;
	relative_deformable = options.nrelative_deformable;
	//user_head = options.nhead;
	selected_anims_only = options.nsel_anim_only;
	use_loose_joints = options.nloose_j;
	export_root_animation = options.nroot_anim;
	allign_root_to_world = options.nin_world;
	scale_factor = options.nScale;
	allow_loose_joints =
	allow_translational_joints = options.nallow_loose_j;
	txt_flag = !options.nex_txt;
}

int GetLine( FILE *fp, char line[512] )
{
  int c = fgetc( fp );
  int result = c; //EOF

  for(int i = 0; i < 511 && c != EOF && c != '\n'; i++)
  {
    line[i] = (char)c;
    c = fgetc( fp );
  }
  line[i] = 0;

  return result;
}

// Start the exporter - main !
// This is the real entrypoint to the exporter. After the user has selected
// the filename (and he's prompted for overwrite etc.) this method is called.
#if MAX_RELEASE == 2500
int AsciiExp::DoExport(const TCHAR *full_name, ExpInterface * /*ei*/, Interface *inter, BOOL suppressPrompts)
#elif MAX_RELEASE >= 3000
int AsciiExp::DoExport(const TCHAR *full_name, ExpInterface * /*ei*/, Interface *inter, BOOL suppressPrompts, DWORD __options) 
#endif
{
	// Grab the interface pointer.
	ip = inter;
/*
	RemoveVertexColors( inter->GetRootNode() );
	return 1;
*/

	const HANDLE pid = GetCurrentProcess();
	const HANDLE tid = GetCurrentThread();
#ifdef _DEBUG
	SetPriorityClass(pid, IDLE_PRIORITY_CLASS);
	SetThreadPriority(tid, THREAD_PRIORITY_IDLE + 1);
#else
	SetThreadPriority(tid, THREAD_PRIORITY_LOWEST);
#endif
	//BOOL boost;
	//GetThreadPriorityBoost( tid, &boost );
	
	//TrapFpu(true);

	//int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
	//tmpFlag |= _CRTDBG_CHECK_ALWAYS_DF;
	//_CrtSetDbgFlag( tmpFlag );
	
	char *temp_path = NULL;
	char std_out_filename[256] = {0};
	char std_err_filename[256] = {0};

#ifdef _DEBUG
	temp_path="c:\\export";
#else
	temp_path=getenv("TEMP");
	if(temp_path==NULL){
		temp_path="c:";
	}
#endif

	_snprintf(std_out_filename, 255, "%s\\std_out.txt", temp_path);
	_snprintf(std_err_filename, 255, "%s\\std_err.txt", temp_path);

	std_out = freopen(std_out_filename, "wa", stdout);
	if(!std_out){
      Winprint("Error redirecting stdout.\n");
      exit(1);
	} 
    
	std_err = freopen(std_err_filename, "wa", stderr);
	if(!std_err){
      Winprint("Error redirecting stderr.\n");
      exit(1);
	} 

	atexit(ExitCleanup);

	// Set a global prompt display switch
	showPrompts = suppressPrompts ? FALSE : TRUE;

	// Get the options the user selected the last time
	if( options.file_name[0] == 0 )
	{
		strcpy(options.file_name, GetCfgFilename());
	}
	options.Read( NULL );

	INode *world_node = ip->GetRootNode();

	if(showPrompts)
	{
		// Prompt the user with our dialogbox, and get all the options.
		if ( !DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_ASCIIEXPORT_DLG),
				ip->GetMAXHWnd(), ExportDlgProc, (LPARAM)this) )
		{
			ExitCleanup();
			return 1; // canceled
		}
	}

	SetGlobalOptions( options );

	if( batch_file_name[0] == 0 )
	{
		ExportCurrentScene( full_name );
	}
	else // batch export
	{
		FILE *fp = fopen( batch_file_name, "r" );

		if( fp )
		{
			const char seps[5] = { ' ', '\t', '\n', 13, 0 };
			char line[512];
			while( GetLine( fp, line ) != EOF )
			{
				if( line[0] == ';' || line[0] == '#' || strlen(line) == 0 )
					continue;

				const char *tok1 = strtok(line, seps);
				const char *tok2 = (tok1) ? strtok(NULL, seps) : NULL;

				if( !tok1 || !tok2 || strlen(tok1) == 0 || strlen(tok2) == 0 )
					continue;

				if( !_stricmp( tok1, "options" ) )
				{
					FILE *test_fp = fopen(tok2, "r");
					if( test_fp )
					{
						fclose(test_fp); test_fp = NULL;
						strcpy(options.file_name, tok2);
						options.Read( NULL );
						SetGlobalOptions( options );
					}
					else
					{
						fprintf(stderr, "Bad options file %s\n", tok2);
					}
				}
				else
				{
					// load max file
					if( ip->LoadFromFile(tok1, TRUE) )
					{
						SetGlobalOptions( options ); // specific to this max file

						ExportCurrentScene( tok2 );

						options.Read( NULL ); // restore common options from .cfg file
					}
					else
					{
						fprintf(stderr, "Batch could not load %s\n", tok1);
					}
				}
			}
			fclose ( fp );
		}
		
		batch_file_name[0] = 0;
	}
	
	ExitCleanup();

	return 1;
}

int AsciiExp::ExportCurrentScene(const TCHAR *full_name)
{
	SetStaticFrame( _MAX<TimeValue>(0, ip->GetAnimRange().Start()) );

	// check for correct units
	if((GetUnitDisplayType() != UNITDISP_METRIC) && (GetUnitDisplayType() != UNITDISP_GENERIC))
	{
		Winprint("Warning: units should be set to METERS!\n");
	}

	// generate date/time extension to randomize mesh file names
	if(!options.nnon_unique_name)
	{
		GetStringTime(date_name);
	}
	else
	{
		date_name[0] = 0;
	}

	// process names
	strcpy(body_name, full_name);
	StripPath(body_name);
	StripExtension(body_name);
	
	// isolate dest directory name
	strcpy(dest_path, full_name);
	// strtok(dest_path, body_name);  for some reason this does not work
	int last=0;
	for(unsigned int ii=0; ii<strlen(dest_path); ii++)
	{
		if(dest_path[ii]=='\\')
		{
			last=ii;
		}
	}
	dest_path[last+1]=0;

#ifdef _DEBUG
	strcpy(dest_path, "c:\\export\\");
#endif

	root_adjust_m.IdentityMatrix();
	world_adjust.IdentityMatrix();

	inode_list.current = 0;
	inode_list.Reset(ip->GetRootNode());

	CheckTransforms(ip->GetRootNode());

	// Startup the progress bar.
	if( TRUE != ip->ProgressStart(GetString(IDS_PROGRESS_MSG), TRUE, fn, NULL) )
	{
		Winprint("Error: couldn't enter progress mode!\n");
	}

	ip->SetRealTimePlayback(FALSE);
	ip->SetPlayActiveOnly(TRUE);

	if(GetBonesProNodeVer1(ip->GetRootNode()))
	{
		Winprint("Error: Bones Pro Version 1 is no longet supported!\n");
		ExitCleanup();
		return 1;
	}

	bool export_rigid = false;
	if(export_mesh || export_skeleton || export_animation)
	{
		exporting_deformable = true;

		if(ExportDeformable() > 0) // 1 nothing deformable to export; 0 success; -1 failure
		{
			export_rigid = true;
		}
		
		exporting_deformable = false;

		assert( _CrtCheckMemory( ) );
	}
	else
	{
		if( FindPhysiqueModifier(ip->GetRootNode()) || FindBonesModifier(ip->GetRootNode()))
		{
			Winprint("Error: Can't export a deformable object w/ neither Mesh/Skeleton nor Animation being checked!\n");
			export_rigid = false;
		}
		else
		{
			export_rigid = true;
		}
	}

	if(export_rigid)
	{
		int export_count = 0;
		for (int idx = 0; idx < ip->GetRootNode()->NumberOfChildren(); idx++)
		{	
			if (ip->GetCancel())
			{
				break;
			}
			
			INode *cmp_node = ip->GetRootNode()->GetChildNode(idx);
			char cmp_name[256]; cmp_name[0] = 0;
			strncpy(cmp_name, cmp_node->GetName(), 255);

			CompoundObject c_obj;  
			InitCompoundObject(&c_obj);

			// _snprintf(c_obj.file_name, 255, "%s%s.cmp",dest_path,FixupName(cmp_node->GetName()));
			_snprintf(c_obj.file_name, 255, "%s%s.cmp", dest_path, body_name);

			StartMyTimer();
			nodeEnum(cmp_node, 0, &c_obj);
			StopMyTimer("nodeEnum %s", cmp_node->GetName());

			//RemoveIdentityChannels(&c_obj);
			//ShortenConstantChannels(&c_obj);
			ExportGlobalEvents(&c_obj);
			CombineChannels(&c_obj, 0);  
			// SynchronizeScriptTiming(&c_obj); // uses first & last frame
			ExtractKeyFrames(&c_obj);
			
			if(center_mass)
			{
				CenterMass(&c_obj, NULL);
			}

			if(	scale_factor != 1.0f )
			{
				ScaleCompoundObject(&c_obj, scale_factor);
			}

			WriteCmpd(&c_obj, split_flag, txt_flag);//TXT_ON);  // only writes if it has data

			// char exec_buf[256] = {0};
			// _snprintf(exec_buf, 255, "c:\\bin\\objview -ic:\\bin\\objview.ini %s",c_obj.file_name);
			// Win32Exec(exec_buf, TRUE);

			if(c_obj.part_count > 0)
			{
				export_count++;
			}
			FreeCmpObject(&c_obj);
			Free(root_name);
			
			if(export_count>1)
			{
				Winprint("Error: All objects are NOT linked properly to just ONE root object!\n");
			}
			assert( _CrtCheckMemory( ) );
		}
	}

	ip->ProgressEnd();

	if( options.nsave_options_to_max_file )
	{
		ip->FileSave();
	}

	return 1;
}

void CheckTransforms(INode *node)
{
	if(node)
	{
		if(!export_selected || node->Selected()) //ip->SelectNode(node, 0)
		{
			Matrix3 m ( node->GetNodeTM((TimeValue)0) );

			m.NoScale();
			Point3 k ( CrossProd(m.GetRow(0), m.GetRow(1)) );

			if( DotProd(k, m.GetRow(2)) <= 0.0 )
			{
#ifdef _DEBUG
				fprintf(stderr,"Warning: object %s is in a Left-Handed coordinate system!\n", node->GetName());
#else
				Winprint("Warning: object %s is in a Left-Handed coordinate system!\n", node->GetName());
#endif
			}

			const float xy = (float)fabs(DotProd(m.GetRow(0), m.GetRow(1)));
			const float xz = (float)fabs(DotProd(m.GetRow(0), m.GetRow(2)));
			const float yz = (float)fabs(DotProd(m.GetRow(1), m.GetRow(2)));
			if( xy > .1f || xz > .1f || yz > .1f)
			{
				Winprint("Warning: object %s is in a NON Orthogonal coordinate system!\n", node->GetName());
			}
		}

		for (int i = 0; i < node->NumberOfChildren(); i++)
		{
			CheckTransforms(node->GetChildNode(i));
		}
	}
}

void __cdecl ExitCleanup(void)
{
	if( std_out )
	{
		fflush(std_out);
		fclose(std_out);
		std_out = NULL;
	}

	if( std_err )
	{
		fflush(std_err);
		fclose(std_err);
		std_err = NULL;
	}

	_clear87();
	_control87(_CW_DEFAULT, 0xfffff);
	assert( _CrtCheckMemory( ) );

	const HANDLE pid = GetCurrentProcess();
	const HANDLE tid = GetCurrentThread();
	SetPriorityClass(pid, NORMAL_PRIORITY_CLASS);
	SetThreadPriority(tid, THREAD_PRIORITY_NORMAL);
}


// instance copy
INode* AsciiExp::GetFirstInstance(INode *node)
{
	Object *o_ref = node->GetObjectRef();
	Mtl *o_mat = node->GetMtl();
	
	for(int i = 0; i < inode_list.count; i++)
	{
		if( inode_list[i] == node )
			break;

		if( o_ref == inode_list[i]->GetObjectRef() )
		{
			Mtl *i_mat = inode_list[i]->GetMtl();

			if( o_mat != i_mat )
			{
				Winprint("Warning: can't export %s as an instance of %s because their materials\n"
						 "%s and %s are different!\n", node->GetName(), inode_list[i]->GetName(),
						 (o_mat) ? o_mat->GetName() : "NULL",
						 (i_mat) ? i_mat->GetName() : "NULL");
			}
			else
			{
				return inode_list[i];
			}
		}
	}

	return NULL;
}

// reference copy
INode* AsciiExp::GetFirstReference(INode *node)
{
	const ObjectState os ( node->EvalWorldState(TimeValue(0)) );
	Mtl *o_mat = node->GetMtl();

	for(int i = 0; i < inode_list.count; i++)
	{
		if( inode_list[i] == node )
			break;

		const ObjectState os2 ( inode_list[i]->EvalWorldState(TimeValue(0)) );
		if( os.obj == os2.obj &&
			node->GetObjectRef() != inode_list[i]->GetObjectRef() ) // filter out instances
		{
			Mtl *i_mat = inode_list[i]->GetMtl();

			if( o_mat != i_mat )
			{
				Winprint("Warning: can't export %s as a reference of %s because their materials\n"
						 "%s and %s are different!\n", node->GetName(), inode_list[i]->GetName(),
						 (o_mat) ? o_mat->GetName() : "NULL",
						 (i_mat) ? i_mat->GetName() : "NULL");
			}
			else
			{
				return inode_list[i];
			}
		}
	}

	return NULL;
}

// This method is the main object exporter.
// It is called once of every node in the scene. The objects are
// exported as they are encoutered.

// Before recursing into the children of a node, we will export it.
// The benefit of this is that a nodes parent is always before the
// children in the resulting file. This is desired since a child's
// transformation matrix is optionally relative to the parent.

BOOL AsciiExp::nodeEnum(INode* node, int indentLevel, CompoundObject *c_obj) 
{
	char node_name[256]; node_name[0] = 0;
	strncpy(node_name, node->GetName(), 255);

	char progress_name[256] = {0};
	sprintf(progress_name, "%d%% %s", (int)((float)inode_list.current/inode_list.count*100.0f), node->GetName());
	ip->ProgressUpdate((int)((float)inode_list.current/inode_list.count*100.0f), FALSE, progress_name); 

	// Stop recursing if the user pressed Cancel 
	if(ip->GetCancel())
		return FALSE;


	if(strlen(node->GetName()) > PARTNAME_MAX-1)
	{
		Winprint("Error: name %s is LONGER than %d characters!\n", node->GetName(), PARTNAME_MAX-1);
	}

	// The ObjectState is a 'thing' that flows down the pipeline containing
	// all information about the object. By calling EvalWorldState() we tell
	// max to eveluate the object at end of the pipeline.
	ObjectState os = node->EvalWorldState(GetStaticFrame()); 

	// see if this is an object we want to export
	if(Exportable(node))
	{	
		int placeholder_flag = IsPlaceholder(node);
		int lod_flag = HasLodChildren(node);
		char extension_name[5];

		INode *inst_node = GetFirstInstance(node);

		if(placeholder_flag)
		{
			extension_name[0] = 0;
		}else
		if(os.obj->SuperClassID() == GEOMOBJECT_CLASS_ID || lod_flag)
		{
			if(os.obj->ClassID() == EDITABLE_SURF_CLASS_ID || os.obj->ClassID() == FITPOINT_PLANE_CLASS_ID)
			{
				strcpy(extension_name, ".nrb");
			}
			else
			{
				strcpy(extension_name, ".3db");
			}
		}else
		if(os.obj->SuperClassID() == LIGHT_CLASS_ID)
		{
			strcpy(extension_name, ".lit");
		}else
		if(os.obj->SuperClassID() == CAMERA_CLASS_ID)
		{
			strcpy(extension_name, ".cam");
		}

		int root_flag;
		if(root_name==NULL)
		{
			root_flag=1;
			root_name=(char*)Malloc((strlen(node->GetName())+1)*sizeof(char));
			strcpy(root_name, node->GetName());
		}
		else
		{
			root_flag=0;
		}

		int export_result = FALSE;

		lod_object l_obj;
		InitLodObject(&l_obj, /*&(c_obj->tl), &(c_obj->atl),*/ &(c_obj->ml));

		object obj;
		InitObject(&obj, /*&(c_obj->tl), &(c_obj->atl),*/ &(c_obj->ml));

		if( !placeholder_flag && !inst_node )
		{
			if(lod_flag)
			{	
				if(options.nLodPercent < 100.0f)
				{
					Winprint("Error: %s already has artist generated LOD!",
						node->GetName());
				}
			
				ExportLod(&l_obj, node);
				if(l_obj.count > 0)
				{
					export_result = TRUE;
				}	
			}else
			if(os.obj->SuperClassID() == GEOMOBJECT_CLASS_ID)
			{
				//InitObject(&obj, &(c_obj->tl), &(c_obj->atl), &(c_obj->ml));

				export_result = Export3DB(node, &obj, FIXED_OBJ, node, GetStaticFrame());

				if(export_result)
				{
#if USE_DA_MESH
					obj.da_mesh.PostProcess(options.nLodPercent, options.nLodClosestDist, options.nLodFurthestDist);
					calcRigidBody(&obj, options.nDensity, -1, node->GetName(), true);
#else
					PostProcessMesh(&obj, options.nDensity, options.nLodPercent,
						options.nLodClosestDist, options.nLodFurthestDist, true,
						node->GetName(), scale_factor);
#endif
					l_obj.mim_max_dist[0] = 0.0f;
					l_obj.mim_max_dist[1] = options.nLodDropOut;
				}
			}else
			if(os.obj->SuperClassID() == LIGHT_CLASS_ID)
			{
				//InitObject(&obj, &(c_obj->tl), &(c_obj->atl), &(c_obj->ml));
				export_result = ExportLight(node, &obj);
			}else
			if(os.obj->SuperClassID() == CAMERA_CLASS_ID)
			{
				//InitObject(&obj, &(c_obj->tl), &(c_obj->atl), &(c_obj->ml));
				export_result = ExportCamera(node, &obj);
			}
		}

		if(export_result || placeholder_flag || inst_node)
		{
			char file_name[256]; file_name[0] = 0;
			
			if(placeholder_flag)
			{
				if(!StripPrefix(node_name, "Particle"))
				{
					if(!StripPrefix(node_name, "particle"))
					{
						StripPrefix(node_name, "PARTICLE");
					}
				}
				StripPrefix(node_name, "_");
				strcpy(file_name, node_name);
			}else
			if( inst_node )
			{
				strcpy(file_name, inst_node->GetName());
				strcat(file_name, date_name);
			}
			else
			{
				strcpy(file_name, node_name);
				strcat(file_name, date_name);
			}
			
			InsertCompoundName(c_obj, node_name, file_name, root_name, extension_name, 1, NULL, true); // increments count
						
			if(lod_flag)
			{
				c_obj->lod_object_list[c_obj->part_count-1] = l_obj;
			}
			else
			{
				c_obj->lod_object_list[c_obj->part_count-1].count = 1;
				c_obj->lod_object_list[c_obj->part_count-1].obj_list = (object*)Malloc(sizeof(object));
				c_obj->lod_object_list[c_obj->part_count-1].obj_list[0] = obj;
			}

			strcpy(c_obj->lod_object_list[c_obj->part_count-1].file_name, dest_path);
			strcat(c_obj->lod_object_list[c_obj->part_count-1].file_name, file_name);
			
			if(strlen(extension_name) > 0)
			{
				strcat(c_obj->lod_object_list[c_obj->part_count-1].file_name, extension_name);
			}

			if(strlen(file_name) + strlen(extension_name) + 1 > 64)
			{
				Winprint("Error: file name %s%s is too long!", file_name, extension_name);
			}
			
			ExportConnection(node, c_obj, NULL, NONE, -1, NULL);	

			if(IsPlaceholder(node)) // particle system will be attached instead
			{
				c_obj->lod_object_list[c_obj->part_count-1].export_flag = 0;
			}
		}
		else
		{
			if(root_flag)
			{
				Free(root_name);
			}
		}
	}

	inode_list.current++;

	assert( _CrtCheckMemory( ) );

	// For each child of this node, we recurse into ourselves
	// until no more children are found.
	for (int c = 0; c < node->NumberOfChildren(); c++) // children are counted only 1 level deep
	{
		if (!nodeEnum(node->GetChildNode(c), indentLevel+1, c_obj))
		{
			return FALSE;
		}
	}

	return TRUE;
}

struct mm{
	float min, max;
};

void AsciiExp::ExportLod(lod_object *l_obj, INode *group_node)
{	
	mm dist[32];
	
	for(int i=0; i<group_node->NumberOfChildren(); i++)
	{
		if( HasLod(group_node->GetChildNode(i)) )
		{
			object obj;
			InitObject(&obj, /*l_obj->tl, l_obj->atl,*/ l_obj->ml);

			INode *child_node = group_node->GetChildNode(i);

			int export_result = Export3DB(child_node, &obj,
								FIXED_OBJ, group_node, GetStaticFrame());
			if(export_result)
			{		
				PostProcessMesh(&obj, options.nDensity, options.nLodPercent, options.nLodClosestDist,
					options.nLodFurthestDist, true,
					child_node->GetName(), scale_factor);

				l_obj->count++;
				l_obj->obj_list = (object*)Realloc(l_obj->obj_list, l_obj->count*sizeof(object));
				l_obj->obj_list[l_obj->count-1] = obj;

				GetLodDist(group_node->GetChildNode(i),
					&(dist[l_obj->count-1].min),
					&(dist[l_obj->count-1].max));

				// should we be scaling these switching distances if scaling is on ?
				dist[l_obj->count-1].min *= scale_factor;
				dist[l_obj->count-1].max *= scale_factor;
			}
		}
	}

	// sort (bubble) by switch in distance
	for(int j=0; j<l_obj->count; j++)
	{
		for(int i=0; i<l_obj->count-(j+1); i++)
		{
			if(dist[i].max > dist[i+1].max)
			{
				MemSwap(dist + i, dist + i + 1, sizeof(dist[0]));
				MemSwap(l_obj->obj_list + i, l_obj->obj_list + i + 1, sizeof(*(l_obj->obj_list)));
			}
		}
	}
	
	ExportHP(&(l_obj->obj_list[0]), group_node, GetStaticFrame());

	l_obj->switch_list = (float*)Malloc( (l_obj->count-1) * sizeof(float) );
	for(i=0; i<l_obj->count-1; i++)
	{
		l_obj->switch_list[i] = dist[i].max; // LOD_0 dist is assumed 0
	}

	l_obj->mim_max_dist[0] = dist[0].min;
	l_obj->mim_max_dist[1] = dist[l_obj->count-1].max;
}

void GetLodDist(INode *node, float *min, float *max)
{
	assert(node);

	Control* visCont = node->GetVisController();

	if( visCont && (visCont->ClassID() == LOD_CONTROL_CLASS_ID) )
	{
		LODCtrl *lod_c = (LODCtrl*)visCont;
		*max = lod_c->max; 
		*min = lod_c->min; // used to be ignored
	}
	else
	{
		*min = -1.0f;
		*max = -1.0f;
	}
}

bool HasLodChildren(INode *node)
{
	for(int i=0; i<node->NumberOfChildren(); i++)
	{
		if( HasLod(node->GetChildNode(i)) )
		{
			return true;
		}
	}

	return false;
}

bool HasLod(INode *node)
{
	//TODO: also could check node's S_class_id == GEOMOBJECT_CLASS_ID

	Control* visCont = node->GetVisController();

	if( visCont && (visCont->ClassID() == LOD_CONTROL_CLASS_ID) )
	{
		return true;
	}

	return false;
}

bool AsciiExp::Exportable(INode *node)
{
#ifdef _DEBUG
	const char *name = node->GetName();
#endif
	ObjectState os ( node->EvalWorldState((TimeValue)0) ); 

	const SClass_ID sc_id ( os.obj->SuperClassID() );
	const Class_ID c_id ( os.obj->ClassID() );



	const char *an_name=NULL;
	INode *an = node->GetActualINode();
	if(an)
	{
		an_name = an->GetName();
	}


	BOOL dp = node->HasDependents();
	BOOL dpr = node->HasRealDependents();

	RefList & rl = node->GetRefList();

	//CoreExport virtual	int EnumDependents(DependentEnumProc* dep);

	BOOL rt = node->IsRefTarget();

	//EnumModifiers(node, DumpParam2Block, NULL);


	if ((os.obj) && 
		(MyGetGBufID(node) == 0 || exporting_deformable) && 
		(!export_selected || node->Selected()) ) 
	{
		// We look at the super/class ID's to determine the type of the object.
		if(	
			(	( sc_id == GEOMOBJECT_CLASS_ID )  
				&&
				( c_id != Class_ID(TARGET_CLASS_ID, 0)	)
				&&
				(!HasLod(node)) // already exported by group
			)	

			||

			(
				HasLodChildren(node)
			)
			
			||	
				
			(	( sc_id == HELPER_CLASS_ID )  // particles
				&& 
				( IsPlaceholder(node) )		
			)	// group == HELPER_CLASS_ID; DUMMY_CLASS_ID,0    
			
			||

			(	( sc_id == LIGHT_CLASS_ID	)  
				&&
				( export_lights	)		
			)	
			
			||

			(	( sc_id == CAMERA_CLASS_ID )  
				&&
				( export_cameras )		
			)

			||

			(	( sc_id == HELPER_CLASS_ID)
				&&
				( c_id == Class_ID( BONE_CLASS_ID , 0) )
				&&
				( exporting_deformable )
			)
		  )
		{
			return true;
		}
	}

	return false;
}

void AsciiExp::PreProcess(INode* node, int& nodeCount)
{
	nodeCount++;
	
	// For each child of this node, we recurse into ourselves 
	// and increment the counter until no more children are found.
	for (int c = 0; c < node->NumberOfChildren(); c++) 
	{
		PreProcess(node->GetChildNode(c), nodeCount);
	}
}

/****************************************************************************

 Configuration.
 To make all options "sticky" across sessions, the options are read and
 written to a configuration file every time the exporter is executed.

 ****************************************************************************/

TSTR AsciiExp::GetCfgFilename(void)
{
	TSTR filename;
	
	filename += ip->GetDir(APP_PLUGCFG_DIR);
	filename += "\\";
	filename += CFGFILENAME;

	return filename;
}

// NOTE: Update anytime the CFG file changes
/*
#define CFG_VERSION 27

void AsciiExp::ReadConfig()
{
#if 1
	options.Read( GetCfgFilename() );

#else
	TSTR filename = GetCfgFilename();
	FILE* cfgStream;

	cfgStream = fopen(filename, "rb");
	if (!cfgStream)
		return FALSE;

	// First item is a file version
	int fileVersion = _getw(cfgStream);

	if (fileVersion != CFG_VERSION)
	{
		// remove old cfg file (this will reinstate defaults)
		fclose(cfgStream);
		DeleteFile(filename);
		return FALSE;
	}

	int i;
	float f;
	i = _getw(cfgStream);
	f = *(float*)&i;
	SetDensity(f);

	i = _getw(cfgStream);
	f = *(float*)&i;
	SetScale(f);

	SetMipFlag(_getw(cfgStream));
	Set565Flag(_getw(cfgStream));
	Set888Flag(_getw(cfgStream));
	SetDitherFlag(_getw(cfgStream));
	SetSplitFlag(_getw(cfgStream));
	SetDefaultMatFlag(_getw(cfgStream));
	SetMeshFlag(_getw(cfgStream));
	//SetSkeletonFlag(_getw(cfgStream));
	SetAnimationFlag(_getw(cfgStream));
	SetLightsFlag(_getw(cfgStream));
	SetCamerasFlag(_getw(cfgStream));
	SetSelectedFlag(_getw(cfgStream));
	//SetBipedYUpFlag(_getw(cfgStream));
	SetNoPhysics(_getw(cfgStream));
	SetCenterMass(_getw(cfgStream));
	SetVertexColor(_getw(cfgStream));
	SetIkExt(_getw(cfgStream));
	SetSmGrp(_getw(cfgStream));
	SetExAvi(_getw(cfgStream));
	SetExTxt(_getw(cfgStream));
	SetNonUniqueName(_getw(cfgStream));
	SetAllowLooseJ(_getw(cfgStream));

	SetAllignHeading(_getw(cfgStream));
	SetRelAnim(_getw(cfgStream));
	//SetHead(_getw(cfgStream));
	SetSelAnimOnly(_getw(cfgStream));
	SetLooseJ(_getw(cfgStream));
	SetRootAnim(_getw(cfgStream));
	SetInWorld(_getw(cfgStream));

#ifdef _DEBUG
	SetIgnoreWarningsFlag(_getw(cfgStream));
#else
	_getw(cfgStream);
	SetIgnoreWarningsFlag(0);
#endif

	//SetLodCount(_getw(cfgStream));

	i = _getw(cfgStream);
	f = *(float*)&i;
	SetLodPercent(f);

	i = _getw(cfgStream);
	f = *(float*)&i;
	SetLodClosest(f);

	i = _getw(cfgStream);
	f = *(float*)&i;
	SetLodFurthest(f);

	i = _getw(cfgStream);
	f = *(float*)&i;
	SetLodDropOut(f);

	i = _getw(cfgStream);
	f = *(float*)&i;
	SetMtlBoundary(f);

	i = _getw(cfgStream);
	f = *(float*)&i;
	SetTxtWeight(f);

	i = _getw(cfgStream);
	f = *(float*)&i;
	SetAdjustRotateX(f);

	i = _getw(cfgStream);
	f = *(float*)&i;
	SetAdjustRotateY(f);

	i = _getw(cfgStream);
	f = *(float*)&i;
	SetAdjustRotateZ(f);

	fclose(cfgStream);

	return TRUE;
#endif
}
*/

/*
void AsciiExp::WriteConfig()
{
#if 1
	options.Write( GetCfgFilename() );
#else
	TSTR filename = GetCfgFilename();
	FILE* cfgStream;

	cfgStream = fopen(filename, "wb");
	if (!cfgStream)
		return;

	// Write CFG version
	_putw(CFG_VERSION,				cfgStream);

	float f;
	int *ipt;

	f = GetDensity();
	ipt=(int*)&f;
	_putw(*ipt,			cfgStream);

	f = GetScale();
	ipt=(int*)&f;
	_putw(*ipt,			cfgStream);

	_putw(GetMipFlag(),			cfgStream);
	_putw(Get565Flag(),			cfgStream);
	_putw(Get888Flag(),			cfgStream);
	_putw(GetDitherFlag(),		cfgStream);
	_putw(GetSplitFlag(),		cfgStream);
	_putw(GetDefaultMatFlag(),	cfgStream);
	_putw(GetMeshFlag(),		cfgStream);
	//_putw(GetSkeletonFlag(),	cfgStream);
	_putw(GetAnimationFlag(),	cfgStream);
	_putw(GetLightsFlag(),		cfgStream);
	_putw(GetCamerasFlag(),		cfgStream);
	_putw(GetSelectedFlag(),	cfgStream);
	//_putw(GetBipedYUpFlag(),	cfgStream);
	_putw(GetNoPhysics(),		cfgStream);
	_putw(GetCenterMass(),		cfgStream);
	_putw(GetVertexColor(),		cfgStream);
	_putw(GetIkExt(),			cfgStream);
	_putw(GetSmGrp(),			cfgStream);
	_putw(GetExAvi(),			cfgStream);
	_putw(GetExTxt(),			cfgStream);
	_putw(GetNonUniqueName(),	cfgStream);
	_putw(GetAllowLooseJ(),		cfgStream);
	_putw(GetAllignHeading(),	cfgStream);
	_putw(GetRelAnim(),			cfgStream);
	//_putw(GetHead(),			cfgStream);
	_putw(GetSelAnimOnly(),		cfgStream);
	_putw(GetLooseJ(),			cfgStream);
	_putw(GetRootAnim(),		cfgStream);
	_putw(GetInWorld(),			cfgStream);

	_putw(GetIgnoreWarningsFlag(),	cfgStream);

	// LOD stuff
	//_putw(GetLodCount(),	cfgStream);
	
	f = GetLodPercent();
	ipt=(int*)&f;
	_putw(*ipt,			cfgStream);

	f = GetLodClosest();
	ipt=(int*)&f;
	_putw(*ipt,			cfgStream);

	f = GetLodFurthest();
	ipt=(int*)&f;
	_putw(*ipt,			cfgStream);

	f = GetLodDropOut();
	ipt=(int*)&f;
	_putw(*ipt,			cfgStream);

	f = GetMtlBoundary();
	ipt=(int*)&f;
	_putw(*ipt,			cfgStream);

	f = GetTxtWeight();
	ipt=(int*)&f;
	_putw(*ipt,			cfgStream);

	f = GetAdjustRotateX();
	ipt=(int*)&f;
	_putw(*ipt,			cfgStream);

	f = GetAdjustRotateY();
	ipt=(int*)&f;
	_putw(*ipt,			cfgStream);

	f = GetAdjustRotateZ();
	ipt=(int*)&f;
	_putw(*ipt,			cfgStream);


	fclose(cfgStream);
#endif
}
*/

Point3 GetPivot(INode *node, TimeValue t, INode *parent)
{
	return( GetMyLocalNodeTM(node, t, parent).GetTrans() );
}

Matrix3 GetMyLocalNodeTM(INode* node, TimeValue t, INode *parent)
{
	assert(node);
	Matrix3 tm;
	
    if(node && node != parent)
	{
		if(parent == NULL)
		{
			parent = GetMyObjParent(node);
		}
		
		if(parent)
		{
			tm = GetMyNodeTM(node, t);
			CleanMatrix3(tm); //tm.NoScale(); NoShear(tm);
			Matrix3 ptm( GetMyNodeTM(parent, t) );
			CleanMatrix3(ptm); //ptm.NoScale(); NoShear(ptm);

			tm = tm * Inverse(ptm);
		}
		else
		{
			tm.IdentityMatrix();
		}
    }
	else
	{
		tm.IdentityMatrix();
	}

    return tm;
}

Matrix3 GetMyNodeTM(INode *node, TimeValue t)
{
	Matrix3 tm ( node->GetNodeTM(t) ); // SLOOOOW!!!

	AdjustMyNodeTM(tm, node);
	
	return tm;
}

Matrix3 GetMyObjTMAfterWSM(INode *node, TimeValue t)
{
	return node->GetObjTMAfterWSM(t);
}

void AdjustMyNodeTM(Matrix3 & tm, INode *node)
{
	// adjust for root
	// correct for -Z
	assert(root_name);
	if(allign_root_to_world && !root_adjust_m.IsIdentity() && !strcmp(root_name, node->GetName()))
	{
		Point3 p ( tm.GetTrans() );
		tm = root_adjust_m * tm; 
		tm.SetTrans(p);
	}
	
	// adjust for world
	if( !world_adjust.IsIdentity() && !IsHP(node, (TimeValue)0)) //added after BF demo
	{
		tm = world_adjust * tm;
		// why not ?? tm.SetTrans(tm.GetTrans() * world_adjust);
	}
}

Matrix3 GetFullLocalNodeTM(INode* node, TimeValue t, INode *parent)
{
	Matrix3 tm(TRUE);
	Matrix3 ptm(TRUE);

    if(node)
	{
		if(parent == NULL)
		{
			parent = GetMyObjParent(node);
			/*										// root_node	
			if((parent!=node->GetParentNode()) && (parent!=node ))
			{
				fprintf(stderr,"Warning: %s does not have a DIRECT parent.\n",node->GetName());
				fprintf(stderr,"Parent is %s.  Direct parent is %s\n",
					parent->GetName(),node->GetParentNode()->GetName());
			}
			*/
		}
		
		if(parent)
		{
			tm = GetMyNodeTM(node, t);
			ptm = GetMyNodeTM(parent, t);
			tm = tm * Inverse(ptm);
		}
    }
    return tm;
}

void CleanMatrix3(Matrix3 & m)
{
	m.NoScale();

		  Point3 j ( m.GetRow(1) ); //y
	const Point3 k ( m.GetRow(2) ); //z

	// compute x direction (in case of a left handed coord system)
	const Point3 i ( Normalize( CrossProd(j, k) ) );

	// make sure y is 90 deg from z (in case of shear)
	j = Normalize( CrossProd(k, i) );

	m.SetRow(0, i);
	m.SetRow(1, j);
}

// keep only rotation and translation
void TransposeMatrix3(Matrix3 & m)
{
#if 1
	// transpose since DA vectors are column but MAX vectors are row
	Point3 r0 ( m.GetRow(0) );
	Point3 r1 ( m.GetRow(1) );
	Point3 r2 ( m.GetRow(2) );

	float tmpf;
	tmpf = r0.y;	r0.y = r1.x;	r1.x = tmpf;
	tmpf = r0.z;	r0.z = r2.x;	r2.x = tmpf;
	tmpf = r1.z;	r1.z = r2.y;	r2.y = tmpf;

	m.SetRow(0, r0);
	m.SetRow(1, r1);
	m.SetRow(2, r2);
#else
	// this in fact computtes the transpose (a MAX bug ??)
	// Decompose local matrix
	AffineParts ap;
	decomp_affine(m, &ap);
	//Quat qi(0.0, 0.0, 0.0, 1.0);
	//ap.q.MakeClosest(qi);

	float rotAngle;
	Point3 rotAxis;
	AngAxisFromQ(ap.q, &rotAngle, rotAxis);

	// construct new local matrix
	// ap.q.MakeMatrix(result);  // this does not work and is a max bug
	m = RotAngleAxisMatrix(rotAxis, rotAngle);
	m.SetTrans(ap.t);
#endif
}

// root bone is 1; world is 0
int GetDepth(INode *node)
{
	if(!node) return -1;

	int i = 0;
	while(node)
	{
		node = GetMyObjParent(node);
		i++;
	}

	return i;
}

// get parent node that is an object
INode* GetMyObjParent(INode *node)
{
	if(!node) return NULL;

	const char *name = node->GetName();

	if(root_name && !strcmp(name, root_name))
	{
		return NULL;
		//return node;
	}

	INode *parent = NULL;

	if ( !(node->IsRootNode()) )
	{
		parent = node->GetParentNode();

		// hack to fix Biped hierarchy
		if(!strcmp(name, "Bip01 R Thigh") || !strcmp(name, "Bip01 L Thigh"))
		{
			assert(!strcmp(parent->GetName(), "Bip01 Spine"));
			parent = parent->GetParentNode();
			assert(!strcmp(parent->GetName(), "Bip01 Pelvis"));
		}

		//if(!exporting_deformable) // slow
		{
			while(parent && !(parent->IsRootNode()) )
			{
				const char *parent_name = parent->GetName();

				/*
				ObjectState parent_os ( parent->EvalWorldState(t) );
					// scene root node		// geometry
				if( (parent_os.obj == NULL) || 
					(parent_os.obj->SuperClassID() == GEOMOBJECT_CLASS_ID  && !HasLod(parent)) ||
					(HasLodChildren(parent)) )
				{
					break;
				}
				*/

				if(AsciiExp::Exportable(parent))
				{
					break;
				}
				else
				{
					Winprint("Warning: %s has a parent %s that is NOT an exportable object type!\n"
							 "Please remove %s from the hierarchy! Trying next parent.\n",
						node->GetName(), parent->GetName(), parent_name);

					parent = parent->GetParentNode();

					if(parent->IsRootNode())
					{
						parent = NULL;
						break;
					}
				}
			}
		}
	}
    
	if(parent && parent->IsRootNode())
	{
		return NULL;
		//fprintf(stderr, "Warning: only parent found for %s was scene root \"Object\" !\n", node->GetName());
	}

	return parent;
}

int AsciiExp::MakeDofConnection(CompoundObject *c_obj, INode *node, INode *parent, int con_type)
{
	Fix fixed;
	GetFixed(node, &fixed, parent);

	// get joint info
	DofData dof_data;
	GetDofData(node, &dof_data, GetStaticFrame(), parent);
	CleanDofData(&dof_data);

	if(con_type != NONE)
	{
		dof_data.type = con_type;
	}
		
	if(dof_data.type==SPHERICAL)
	{
		PersistSphere sphere;
		LoadSphere(&sphere, &fixed, &dof_data, PIVOT);
		InsertSphere(c_obj, sphere);

		return SPHERICAL;
	}else
	if(dof_data.type==REVOLUTE)
	{	
		Rev revolute;
		LoadRevolute(&revolute, &fixed, &dof_data, PIVOT);
		InsertRev(c_obj, revolute);

		return REVOLUTE;
	}else
	if(dof_data.type==PRISMATIC)
	{
		Pris prismatic;
		LoadPrismatic(&prismatic, &fixed, &dof_data, PIVOT);
		InsertPris(c_obj, prismatic);
		
		return PRISMATIC;
	}else
	if(dof_data.type==TRANSLATIONAL)
	{
		InsertTrans(c_obj, (Trans)fixed);
		return TRANSLATIONAL;
	}else
	if(dof_data.type==FFIXED)
	{
		InsertFixed(c_obj, fixed);
		return FFIXED;
	}else
	if(dof_data.type==LOOSE)
	{
		InsertLoose(c_obj, (Loose)fixed);
		return LOOSE;
	}
	else
	{
		Winprint("Error: unknown connection type in %s",node->GetName());
	}
	return -1;
}

INode* GetClosestLODNode(INode *group_node)
{
	if(!group_node) return NULL;

	INode *result = NULL;
	float min_dist = FLT_MAX;
	for(int i=0; i<group_node->NumberOfChildren(); i++)
	{
		float min, max;
		GetLodDist(group_node->GetChildNode(i), &min, &max);
		
		if((max > 0.0f) && (max < min_dist))
		{
			min_dist = max;
			result = group_node->GetChildNode(i);
		}
	}
		
	return result;
}

void GetDofData(INode *in_node, DofData *data, TimeValue t, INode *parent)
{
	INode *node = in_node;

	float min_r[3] = {0.0f, 0.0f, 0.0f};
	float max_r[3] = {0.0f, 0.0f, 0.0f};
	
	JointParams* joint;
	
	InitDofData(data);
	
	strncpy(data->name, node->GetName(), 255);

	Point3 pivot = GetPivot(node, t, parent);
	data->pivot[0] = pivot.x;
	data->pivot[1] = pivot.y;
	data->pivot[2] = pivot.z;

	// DOF info for LOD nodes has to be set on the closest child
	if(GetClosestLODNode(node))
	{
		node = GetClosestLODNode(node);
	}
	
	// try to read revolute data
	Control *cont = node->GetTMController()->GetRotationController();
	if(cont)
	{
		joint = (JointParams*)cont->GetProperty(PROPID_JOINTPARAMS);
		
		if(joint && !joint->IsDefault())
		{
			assert(joint->Type() & JNT_ROT);

			//int num_of_dof = joint->dofs;

			if(joint->flags & JNT_XACTIVE)
			{
				if(joint->flags & JNT_XLIMITED)
				{
					data->min_r[0] = joint->min[0];
					data->max_r[0] = joint->max[0];

					while(data->min_r[0] >= 360.0f*D2R || data->max_r[0] >= 360.0f*D2R)
					{
						data->min_r[0] -= 360.0f*(float)D2R;
						data->max_r[0] -= 360.0f*(float)D2R;
					}
					data->min_r[0] = NormAngle(data->min_r[0]);
					data->max_r[0] = NormAngle(data->max_r[0]);
				}
				else
				{
					data->min_r[0] = min_r[0] = -360.0f * (float)D2R;
					data->max_r[0] = max_r[0] =  360.0f * (float)D2R;
				}
			}
			if(joint->flags & JNT_YACTIVE)
			{
				if(joint->flags & JNT_YLIMITED)
				{
					data->min_r[1] = joint->min[1]; 
					data->max_r[1] = joint->max[1];

					while(data->min_r[1] >= 360.0f*D2R || data->max_r[1] >= 360.0f*D2R)
					{
						data->min_r[1] -= 360.0f*(float)D2R;
						data->max_r[1] -= 360.0f*(float)D2R;
					}
					data->min_r[1] = NormAngle(data->min_r[1]);
					data->max_r[1] = NormAngle(data->max_r[1]);
				}
				else
				{
					data->min_r[1] = min_r[1] = -360.0f * (float)D2R;
					data->max_r[1] = max_r[1] =  360.0f * (float)D2R;
				}
			}
			if(joint->flags & JNT_ZACTIVE)
			{
				if(joint->flags & JNT_ZLIMITED)
				{
					data->min_r[2] = joint->min[2]; 
					data->max_r[2] = joint->max[2];

					while(data->min_r[2] >= 360.0f*D2R || data->max_r[2] >= 360.0f*D2R)
					{
						data->min_r[2] -= 360.0f*(float)D2R;
						data->max_r[2] -= 360.0f*(float)D2R;
					}
					data->min_r[2] = NormAngle(data->min_r[2]);
					data->max_r[2] = NormAngle(data->max_r[2]);
				}
				else
				{
					data->min_r[2] = min_r[2] = -360.0f * (float)D2R;
					data->max_r[2] = max_r[2] =  360.0f * (float)D2R;
				}
			}
		}
	}

	// try to read prismatic data
	cont = node->GetTMController()->GetPositionController();
	if(cont)
	{
		joint = (JointParams*)cont->GetProperty(PROPID_JOINTPARAMS);
		
		if(joint && !joint->IsDefault())
		{
			assert(joint->Type() & JNT_POS);

			//int num_of_dof = joint->dofs;

			if(joint->flags & JNT_XACTIVE)
			{
				if(joint->flags & JNT_XLIMITED)
				{
					data->min_t[0] = joint->min[0]; 
					data->max_t[0] = joint->max[0];
				}
				else
				{
					data->min_t[0] = -FLT_MAX; 
					data->max_t[0] = FLT_MAX;
				}
			}
			if(joint->flags & JNT_YACTIVE)
			{
				if(joint->flags & JNT_YLIMITED)
				{
					data->min_t[1] = joint->min[1]; 
					data->max_t[1] = joint->max[1];
				}
				else
				{
					data->min_t[1] = -FLT_MAX; 
					data->max_t[1] = FLT_MAX;
				}
			}
			if(joint->flags & JNT_ZACTIVE)
			{
				if(joint->flags & JNT_ZLIMITED)
				{
					data->min_t[2] = joint->min[2]; 
					data->max_t[2] = joint->max[2];
				}
				else
				{
					data->min_t[2] = -FLT_MAX; 
					data->max_t[2] = FLT_MAX;
				}
			}
		}
	}

	// axis based on limits
	//CleanDofData(data);	
}

void AsciiExp::MakeLooseAnim(CompoundObject *c_obj, INode *node, INode *parent, int in_n_frames,
							 Frame * in_frame_list)
{
	Frame *frame_list;
	int n_frames;

	Fix fixed;
	
	GetFixed(node, &fixed, parent);

	n_frames = 1 + (GetEndTime(node) - _MAX<TimeValue>(0, ip->GetAnimRange().Start())) / GetTicksPerFrame();
    
	if(in_n_frames > 0)
	{
		if(in_n_frames != n_frames)
		{
			Winprint("Error: inconsistent number of frames for %s!\n", node->GetName());
			return;
		}
		frame_list = in_frame_list;
	}
	else
	{
		frame_list=(Frame*)Malloc(n_frames*sizeof(Frame));
		if(n_frames!=ReadFrames(node, frame_list, (REVOLUTE | PRISMATIC), parent, NULL))
		{
			Winprint("Error: inconsistent frame count.\n");
			exit(1);
		}
	}
	
	RotateTransFrames(frame_list, n_frames, fixed);

	for(int i=0; i<n_frames; i++)
	{
		frame_list[i].vector_t.x *= frame_list[i].step;
		frame_list[i].vector_t.y *= frame_list[i].step;
		frame_list[i].vector_t.z *= frame_list[i].step;
	}
	
	InsertLoose(c_obj, (Loose)fixed);

	// Channel
	NamedChannel n_channel;
	InitNamedChannel(&n_channel);
	// NOTE: characters are forced into one script
	// objects will have a single scripts because we add the export name
	if(strlen(body_name) + strlen(node->GetName()) + 4 > PersistAnimCHANNEL_NAME_MAX-1)
	{
		Winprint("Error: Channel name Ch_%s_%s is longer than %d characters! Truncating.\n",
			body_name, node->GetName(), PersistAnimCHANNEL_NAME_MAX-1);
	}
	_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s_%s", body_name, node->GetName());
	// this means objects w/o note tracks will have multiple scripts
	//_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s", node->GetName());

	n_channel.first_frame=frame_list[0].index;
	n_channel.last_frame=frame_list[n_frames-1].index;

	n_channel.channel.header.frames=n_frames;
	n_channel.channel.header.capture_rate=(float)(1.0/fps);
	n_channel.channel.header.type=(PersistDT_VECTOR | PersistDT_QUATERNION);
	n_channel.channel.data=
	  (unsigned char*)Malloc(n_channel.channel.header.frames *
	                         (sizeof(PersistVector) + sizeof(PersistQuaternion)));
	for(i=0; i<n_frames; i++){
	  *(PersistVector*)(n_channel.channel.data + (sizeof(PersistQuaternion)+sizeof(PersistVector)) * i)=
		  frame_list[i].vector_t;
	  *(PersistQuaternion*)(n_channel.channel.data + 
		                   (sizeof(PersistQuaternion)+sizeof(PersistVector)) * i +
						    sizeof(PersistVector))=
		  frame_list[i].quat.v;
	}
	
	// Script
	NamedScript script;
	InitScript(&script);
	_snprintf(script.name, 255, "Sc_%s", body_name);
	script.channel_count=1;
	script.channel_list=(PersistAnimChannelMapping*)
						Malloc(sizeof(PersistAnimChannelMapping));
	strcpy(script.channel_list[0].parent, fixed.parent);
	strcpy(script.channel_list[0].child, fixed.child);
	strcpy(script.channel_list[0].channel, n_channel.name);

	// add data to cmp object
	InsertAnim(c_obj, node, script, n_channel, GetStaticFrame() / GetTicksPerFrame());

	// cleanup
	if(in_n_frames <= 0)
	{
		for(i=0; i<n_frames; i++)
		{
		  Free(frame_list[i].name);
		  Free(frame_list[i].parent_name);
		}

		Free(frame_list);
	}
}

void AsciiExp::MakeRootAnim(CompoundObject *c_obj, INode *node, int in_n_frames, Frame * in_frame_list)
{
	Frame *frame_list;
	int n_frames;
	int i;
	
	n_frames = 1 + (GetEndTime(node) - _MAX<TimeValue>(0, ip->GetAnimRange().Start())) / GetTicksPerFrame();
   
	if(in_n_frames > 0)
	{
		if(in_n_frames != n_frames)
		{
			Winprint("Error: inconsistent number of frames for %s!\n", node->GetName());
			return;
		}
		frame_list = in_frame_list;
	}
	else
	{
		frame_list=(Frame*)Malloc(n_frames*sizeof(Frame));
		if(n_frames!=ReadFrames(node, frame_list, (REVOLUTE | PRISMATIC), NULL, NULL))
		{
			Winprint("Error: inconsistent frame count.\n");
			exit(1);
		}
	}

	for(i=0;i<n_frames;i++)
	{
		frame_list[i].vector_t.x*=frame_list[i].step;
		frame_list[i].vector_t.y*=frame_list[i].step;
		frame_list[i].vector_t.z*=frame_list[i].step;
	}

	// Channel
	NamedChannel n_channel;
	InitNamedChannel(&n_channel);
	if(strlen(body_name) + strlen(node->GetName()) + 4 > PersistAnimCHANNEL_NAME_MAX-1)
	{
		Winprint("Error: Channel name Ch_%s_%s is longer than %d characters! Truncating.\n",
			body_name, node->GetName(), PersistAnimCHANNEL_NAME_MAX-1);
	}
	_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s_%s", body_name, node->GetName());
	//_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s", ROOT_OBJ_NAME);

	n_channel.first_frame=frame_list[0].index;
	n_channel.last_frame=frame_list[n_frames-1].index;

	n_channel.channel.header.frames=n_frames; 
	// printf("frame rate=%f\n",GetFrameRate());  // fix
	n_channel.channel.header.capture_rate=(float)(1.0/fps);
	n_channel.channel.header.type=(PersistDT_VECTOR | PersistDT_QUATERNION);
	n_channel.channel.data=
	  (unsigned char*)Malloc(n_channel.channel.header.frames*
	                         (sizeof(PersistQuaternion)+sizeof(PersistVector)));
	for(i=0;i<n_frames;i++){
	  *(PersistVector*)(n_channel.channel.data + (sizeof(PersistQuaternion)+sizeof(PersistVector)) * i)=
		  frame_list[i].vector_t;
	  *(PersistQuaternion*)(n_channel.channel.data + 
		                   (sizeof(PersistQuaternion)+sizeof(PersistVector)) * i +
						    sizeof(PersistVector))=
		  frame_list[i].quat.v;
	}	

	// Script
	NamedScript script;
	InitScript(&script);
	_snprintf(script.name, 255, "Sc_%s", body_name);
	script.channel_count=1;
	script.channel_list=(PersistAnimChannelMapping*)
						Malloc(sizeof(PersistAnimChannelMapping));
	strcpy(script.channel_list[0].parent, ROOT_OBJ_NAME);
	script.channel_list[0].child[0]=0;
	strcpy(script.channel_list[0].channel, n_channel.name);

	// add data to cmp object
	InsertAnim(c_obj, node, script, n_channel, GetStaticFrame() / GetTicksPerFrame());

	// cleanup
	if(in_n_frames <= 0)
	{
		for(i=0; i<n_frames; i++){
		  Free(frame_list[i].name);
		  Free(frame_list[i].parent_name);
		}
		Free(frame_list);
	}
}

void AsciiExp::MakePrisAnim(CompoundObject *c_obj, INode *node, INode *parent, int in_n_frames,
							Frame * in_frame_list)
{
	Frame *frame_list;
	int n_frames;
	float axis[3];
	SINGLE anim_min, anim_max;
	int i;
	Fix fixed;
	int type;
	NamedChannel n_channel;
	float tmp_v[3];

	GetFixed(node, &fixed, parent);

	DofData dof_data;
	GetDofData(node, &dof_data, GetStaticFrame(), parent);
	CleanDofData(&dof_data);

	if(exporting_deformable)
	{	
		Winprint("Error: Characer's node %s has a prismatic joint!\n",node->GetName());
	}

	dof_data.pivot[0] = fixed.pos.x;
	dof_data.pivot[1] = fixed.pos.y;
	dof_data.pivot[2] = fixed.pos.z;

	n_frames = 1 + (GetEndTime(node) - _MAX<TimeValue>(0, ip->GetAnimRange().Start())) / GetTicksPerFrame();
    
	if(in_n_frames > 0)
	{
		if(in_n_frames != n_frames)
		{
			Winprint("Error: inconsistent number of frames for %s!\n", node->GetName());
			return;
		}
		frame_list = in_frame_list;
	}
	else
	{
		frame_list=(Frame*)Malloc(n_frames*sizeof(Frame));
		if(n_frames!=ReadFrames(node, frame_list, PRISMATIC, parent, NULL))
		{
			Winprint("Error: inconsistent frame count.\n");
			exit(1);
		}
	}

	RotateTransFrames(frame_list, n_frames, fixed);
	//DumpFrames(frame_list, n_frames);

	float min_v[3]={0,0,0}, max_v[3]={0,0,0};
	type=AnimAxisMinMax(frame_list, n_frames, axis, &anim_min, &anim_max, PRISMATIC, min_v, max_v);

	if( (type==PRISMATIC) || (!allow_translational_joints && type==TRANSLATIONAL) )
	{
		Pris prismatic;
		LoadPrismatic(&prismatic, &fixed, &dof_data, MAX_ANIM);

		if(anim_min<prismatic.min)
			prismatic.min=anim_min;
		if(anim_max>prismatic.max)
			prismatic.max=anim_max;
#if 0
		prismatic.axis.x=(axis[0]*fixed.orient.e00 + axis[1]*fixed.orient.e10 + axis[2]*fixed.orient.e20);
		prismatic.axis.y=(axis[0]*fixed.orient.e01 + axis[1]*fixed.orient.e11 + axis[2]*fixed.orient.e21);
		prismatic.axis.z=(axis[0]*fixed.orient.e02 + axis[1]*fixed.orient.e12 + axis[2]*fixed.orient.e22);
		axis[0]=prismatic.axis.x;
		axis[1]=prismatic.axis.y;
		axis[2]=prismatic.axis.z;
#endif

		prismatic.axis.x=axis[0];
		prismatic.axis.y=axis[1];
		prismatic.axis.z=axis[2];

		InsertPris(c_obj, prismatic);

		// Channel
		InitNamedChannel(&n_channel);
		if(strlen(body_name) + strlen(node->GetName()) + 4 > PersistAnimCHANNEL_NAME_MAX-1)
		{
			Winprint("Error: Channel name Ch_%s_%s is longer than %d characters! Truncating.\n",
				body_name, node->GetName(), PersistAnimCHANNEL_NAME_MAX-1);
		}
		_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s_%s", body_name, node->GetName());
		//_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s", node->GetName());

		n_channel.first_frame=frame_list[0].index;
		n_channel.last_frame=frame_list[n_frames-1].index;

		n_channel.channel.header.frames=n_frames;
		n_channel.channel.header.capture_rate=(float)(1.0/fps);
		n_channel.channel.header.type=PersistDT_FLOAT;
		n_channel.channel.data=
			(unsigned char*)Malloc(n_channel.channel.header.frames*sizeof(float));
		for(i=0;i<n_frames;i++){
			// this code is just to project the rotation of the frame onto the rotation axis
			// since the axis is the average of all the frames this is overkill of correctness in most cases
			tmp_v[0]=frame_list[i].vector_t.x;
			tmp_v[1]=frame_list[i].vector_t.y;
			tmp_v[2]=frame_list[i].vector_t.z;
			*(float*)(n_channel.channel.data+4*i)=frame_list[i].step*
				Dot3(axis, tmp_v);
				//(float)cos(GetAngle(axis, tmp_v));
			// printf("angle %f\n",R2D*GetAngle(axis, tmp_v));
		}
	}else
	if( type==TRANSLATIONAL || type==PRISMATIC )
	{ // 3DOF
		
		PersistVector tmp;
		InsertTrans(c_obj, (Trans)fixed);
		
		// Channel
		InitNamedChannel(&n_channel);
		if(strlen(body_name) + strlen(node->GetName()) + 4 > PersistAnimCHANNEL_NAME_MAX-1)
		{
			Winprint("Error: Channel name Ch_%s_%s is longer than %d characters! Truncating.\n",
				body_name, node->GetName(), PersistAnimCHANNEL_NAME_MAX-1);
		}
		_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s_%s", body_name, node->GetName());
		//_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s", node->GetName());

		n_channel.first_frame=frame_list[0].index;
		n_channel.last_frame=frame_list[n_frames-1].index;

		n_channel.channel.header.frames=n_frames;
		n_channel.channel.header.capture_rate=(float)(1.0/fps);
		n_channel.channel.header.type=PersistDT_VECTOR;
		n_channel.channel.data=
		  (unsigned char*)Malloc(n_channel.channel.header.frames*sizeof(PersistVector));
		for(i=0;i<n_frames;i++){
          tmp.x = frame_list[i].vector_t.x * frame_list[i].step;
		  tmp.y = frame_list[i].vector_t.y * frame_list[i].step;
		  tmp.z = frame_list[i].vector_t.z * frame_list[i].step;
		  *(PersistVector*)(n_channel.channel.data+sizeof(PersistVector)*i) = tmp;
		}
	}
	else{
		Winprint("Error: confused about joint type of %s.\n",node->GetName());
		exit(1);
	}

    // Script
    NamedScript script;
    InitScript(&script);
	_snprintf(script.name, 255, "Sc_%s", body_name);
    script.channel_count=1;
    script.channel_list=(PersistAnimChannelMapping*)
                        Malloc(sizeof(PersistAnimChannelMapping));
    strcpy(script.channel_list[0].parent, fixed.parent);
    strcpy(script.channel_list[0].child, fixed.child);
    strcpy(script.channel_list[0].channel, n_channel.name);

    // add data to cmp object
	InsertAnim(c_obj, node, script, n_channel, GetStaticFrame() / GetTicksPerFrame());

    // cleanup
	if(in_n_frames <= 0)
	{
		for(i=0; i<n_frames; i++){
		  Free(frame_list[i].name);
  		Free(frame_list[i].parent_name);
		}
		Free(frame_list);
	}
}

void AsciiExp::MakeRevAnim(CompoundObject *c_obj, INode *node, INode *parent, int in_n_frames,
						   Frame * in_frame_list)
{
	Frame *frame_list;
	int n_frames;
	float axis[3];
	SINGLE anim_min, anim_max;
	int i;
	int type;
	NamedChannel n_channel;
	Fix fixed;
	float tmp_v[3];

	GetFixed(node, &fixed, parent);

	DofData dof_data;
	GetDofData(node, &dof_data, GetStaticFrame(), parent);  // for limits only
	CleanDofData(&dof_data);
	
	// disabled for now
	/*
	assert(!use_loose_joints);
	if(dof_data.type!=REVOLUTE && dof_data.type!=SPHERICAL  && exporting_deformable)
	{
		Winprint("Error: character joint %s does not have any limits specified.\n",node->GetName());
	}
	*/

	dof_data.pivot[0] = fixed.pos.x;
	dof_data.pivot[1] = fixed.pos.y;
	dof_data.pivot[2] = fixed.pos.z;

	n_frames = 1 + (GetEndTime(node) - _MAX<TimeValue>(0, ip->GetAnimRange().Start())) / GetTicksPerFrame();
    
	if(in_n_frames > 0)
	{
		if(in_n_frames != n_frames)
		{
			Winprint("Error: inconsistent number of frames for %s!\n", node->GetName());
			return;
		}
		frame_list = in_frame_list;
	}
	else
	{
		frame_list=(Frame*)Malloc(n_frames*sizeof(Frame));
		if(n_frames!=ReadFrames(node, frame_list, REVOLUTE, parent, NULL))
		{
			Winprint("Error: inconsistent frame count.\n"); 
			exit(1);
		}
	}

	float min_v[3]={0,0,0}, max_v[3]={0,0,0};
	type=AnimAxisMinMax(frame_list, n_frames, axis, &anim_min, &anim_max, REVOLUTE, min_v, max_v);

	if(type==REVOLUTE && !exporting_deformable)
	{
		Rev revolute;

		LoadRevolute(&revolute, &fixed, &dof_data, MAX_ANIM);

		char tmp_name[256] = {0};
		strncpy(tmp_name, node->GetName(), 255);

		// see of specified limits are looser than animation
		if(anim_min<revolute.min)
			revolute.min=anim_min;
		if(anim_max>revolute.max)
			revolute.max=anim_max;

		revolute.axis.x=axis[0];
		revolute.axis.y=axis[1];
		revolute.axis.z=axis[2];

		InsertRev(c_obj, revolute);

		// Add Channel
		InitNamedChannel(&n_channel);
		if(strlen(body_name) + strlen(node->GetName()) + 4 > PersistAnimCHANNEL_NAME_MAX-1)
		{
			Winprint("Error: Channel name Ch_%s_%s is longer than %d characters! Truncating.\n",
				body_name, node->GetName(), PersistAnimCHANNEL_NAME_MAX-1);
		}
		_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s_%s", body_name, node->GetName());
		//_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s", node->GetName());

		n_channel.first_frame=frame_list[0].index;  assert(frame_list[0].index == 0);
		n_channel.last_frame=frame_list[n_frames-1].index;

		n_channel.channel.header.frames=n_frames;
		n_channel.channel.header.capture_rate=(float)(1.0/fps);
		n_channel.channel.header.type=PersistDT_FLOAT;
		n_channel.channel.data=
		  (unsigned char*)Malloc(n_channel.channel.header.frames*sizeof(float));
		for(i=0; i<n_frames; i++)
		{
			// this code is just to project the rotation of the frame onto the rotation axis
			// since the axis is the average of all the frames this is overkill of correctness in most cases
			tmp_v[0]=frame_list[i].vector_r.x;
			tmp_v[1]=frame_list[i].vector_r.y;
			tmp_v[2]=frame_list[i].vector_r.z;

		  *(float*)(n_channel.channel.data+4*i)=frame_list[i].angle *
			  Dot3(axis, tmp_v);
		   // printf("angle %f\n",R2D*GetAngle(axis, tmp_v));
		}
	}else
	if(type==SPHERICAL || type==REVOLUTE) // SPHERICAL
	{
		type = SPHERICAL;
		PersistSphere sphere;

		LoadSphere(&sphere, &fixed, &dof_data, MAX_ANIM);

		// see if specified limits are looser than animation
//		if(0)//exporting_deformable)
exporting_deformable = false; if(exporting_deformable)
		{
			//int IsInRange(float angle, const Vector& axis, const Vector& min, const Vector& max);
			if(min_v[0] < sphere.min_about_i){
				Winprint("Warning: min DOF angle about X for %s is set to\n%.5f but animation goes up to %.5f !\n",
				node->GetName(), sphere.min_about_i*R2D, min_v[0]*R2D);
				sphere.min_about_i = min_v[0];
			}
			if(max_v[0] > sphere.max_about_i){
				Winprint("Warning: max DOF angle about X for %s is set to\n%.5f but animation goes up to %.5f !\n",
				node->GetName(), sphere.max_about_i*R2D, max_v[0]*R2D);
				sphere.max_about_i = max_v[0];
			}

			if(min_v[1] < sphere.min_about_j){
				Winprint("Warning: min DOF angle about Y for %s is set to\n%.5f but animation goes up to %.5f !\n",
				node->GetName(), sphere.min_about_j*R2D, min_v[1]*R2D);
				sphere.min_about_j = min_v[1];
			}
			if(max_v[1] > sphere.max_about_j){
				Winprint("Warning: max DOF angle about Y for %s is set to\n%.5f but animation goes up to %.5f !\n",
				node->GetName(), sphere.max_about_j*R2D, max_v[1]*R2D);
				sphere.max_about_j = max_v[1];
			}

			if(min_v[2] < sphere.min_about_k){
				Winprint("Warning: min DOF angle about Z for %s is set to\n%.5f but animation goes up to %.5f !\n",
				node->GetName(), sphere.min_about_k*R2D, min_v[2]*R2D);
				sphere.min_about_k = min_v[2];
			}
			if(max_v[2] > sphere.max_about_k){
				Winprint("Warning: max DOF angle about Z for %s is set to\n%.5f but animation goes up to %.5f !\n",
				node->GetName(), sphere.max_about_k*R2D, max_v[2]*R2D);
				sphere.max_about_k = max_v[2];
			}
		}
		else
		{
			// void QuatToEuler(Quat &q, float *ang, int type);
			sphere.min_about_i=sphere.min_about_j=sphere.min_about_k=(float)(-360.0*D2R);
			sphere.max_about_i=sphere.max_about_j=sphere.max_about_k=(float)( 360.0*D2R);
		}

		InsertSphere(c_obj, sphere);
		
		// Channel
		InitNamedChannel(&n_channel);
		if(strlen(body_name) + strlen(node->GetName()) + 4 > PersistAnimCHANNEL_NAME_MAX-1)
		{
			Winprint("Error: Channel name Ch_%s_%s is longer than %d characters! Truncating.\n",
				body_name, node->GetName(), PersistAnimCHANNEL_NAME_MAX-1);
		}
		_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s_%s", body_name, node->GetName());
		//_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s", node->GetName());

		n_channel.first_frame=frame_list[0].index;
		n_channel.last_frame=frame_list[n_frames-1].index;

		n_channel.channel.header.frames=n_frames;
		n_channel.channel.header.capture_rate=(float)(1.0/fps);
		n_channel.channel.header.type=PersistDT_QUATERNION;
		n_channel.channel.data=
		  (unsigned char*)Malloc(n_channel.channel.header.frames*sizeof(PersistQuaternion));
		for(i=0; i<n_frames; i++){
		  *(PersistQuaternion*)(n_channel.channel.data+sizeof(PersistQuaternion)*i)=frame_list[i].quat.v;
		}
	}
	else{
		Winprint("Error: confused about joint type of %s.\n",node->GetName());
		exit(1);
	}

	// Script
	NamedScript script;
	InitScript(&script);
	_snprintf(script.name, 255, "Sc_%s", body_name);
	script.channel_count=1;
	script.channel_list=(PersistAnimChannelMapping*)
						Malloc(sizeof(PersistAnimChannelMapping));
	strcpy(script.channel_list[0].parent, fixed.parent);
	strcpy(script.channel_list[0].child, fixed.child);
	strcpy(script.channel_list[0].channel, n_channel.name);


	InsertAnim(c_obj, node, script, n_channel, GetStaticFrame() / GetTicksPerFrame());

	// cleanup
	if(in_n_frames <= 0)
	{
		for(i=0; i<n_frames; i++)
		{
		  Free(frame_list[i].name);
		  Free(frame_list[i].parent_name);
		}
		Free(frame_list);
	}
}

void AsciiExp::InsertAnim(CompoundObject *c_obj, INode *node, NamedScript & script, NamedChannel & n_channel,
				const TimeValue frame_id)
{
	// defaults to parents notetracks
	DefNoteTrack* pNoteTrack = GetNoteTrack(node);

	if(!pNoteTrack)
	{
		//Winprint("Error: Please, name animation of object %s w/ a NoteTrack!\n", node->GetName());

		// attempt to use old naming convention
		assert(!strcmp(script.channel_list[0].channel, n_channel.name));
		InsertNamedChannel(c_obj, n_channel);
		InsertScript(c_obj, script);
	}
	else
	{
		int num_notes;
		Note *note_list;
		GetNotes(node, &num_notes, &note_list, pNoteTrack);
		if(num_notes > 0)
		{
			if(exporting_deformable)
			{
				for(int iNote = 0; iNote < num_notes; iNote++)
				{
					if(note_list[iNote].t == frame_id)
					{
						break;
					}
				}

				if(iNote < num_notes)
				{
					if(strlen(note_list[iNote].name) + strlen(node->GetName()) + 4 > PersistAnimCHANNEL_NAME_MAX-1)
					{
						Winprint("Error: Channel name Ch_%s_%s is longer than %d characters! Truncating.\n",
							note_list[iNote].name, node->GetName(), PersistAnimCHANNEL_NAME_MAX-1);
					}
					_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s_%s",
						note_list[iNote].name, node->GetName());
					strcpy(script.channel_list[0].channel, n_channel.name);
				}
				else // no note at the starting/static time
				{
					if(frame_id != 0)
					{
						Winprint("Warning: There should be a NoteTrack w/ a note at time %d to name the animation!\n",
							frame_id);
					}
				}

				assert(!strcmp(script.channel_list[0].channel, n_channel.name));
				InsertNamedChannel(c_obj, n_channel);
				InsertScript(c_obj, script);
			}
			else
			{
				if(note_list[0].t != 0)
				{
					Winprint("Error: first note track %s of object %s must be at time 0 NOT %d!\n",
						note_list[0].name, node->GetName(), note_list[0].t);
				}

				// split up by note tracks
				for (int iNote = 0; iNote < num_notes; iNote++)
				{
					NamedChannel sub_channel;
					InitNamedChannel(&sub_channel);

					int first = note_list[iNote].t;
					int last;
					if(iNote < num_notes - 1)
					{
						last = note_list[iNote+1].t;
						//assert((unsigned int)last < n_channel.channel.header.frames);
						// this happens if a node has the last key frame before the last note (the notes can come from the parent)
						if((unsigned int)last >= n_channel.channel.header.frames)
						{
							assert(!exporting_deformable);
							break;
						}
					}
					else
					{
						last = n_channel.channel.header.frames - 1;
					}

					bool root_flag = !strcmp(root_name, node->GetName());
					SubNamedChannel(n_channel, &sub_channel, first, last, root_flag);
					// override channel name
					if(strlen(note_list[iNote].name) + strlen(node->GetName()) + 5 > PersistAnimCHANNEL_NAME_MAX-1)
					{
						Winprint("Error: Channel name Ch_%s_%s is longer than %d characters! Truncating.\n",
							note_list[iNote].name, node->GetName(), PersistAnimCHANNEL_NAME_MAX-1);
					}
					_snprintf(sub_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s_%s",
					note_list[iNote].name, node->GetName());
					
					//create a 2nd channel with the notes from the root node "Objects" as events

					NamedChannel n_channelEvents;
					InitNamedChannel(&n_channelEvents);
					if(strlen(sub_channel.name) + 3 > PersistAnimCHANNEL_NAME_MAX-1)
					{
						Winprint("Error: Channel name %s_EV is longer than %d characters! Truncating.\n",
							sub_channel.name, PersistAnimCHANNEL_NAME_MAX-1);
					}
					_snprintf(n_channelEvents.name, PersistAnimCHANNEL_NAME_MAX-1, "%s_EV", sub_channel.name);

					n_channelEvents.channel.header.frames = 0;
					n_channelEvents.channel.header.capture_rate = -1.0f;
					n_channelEvents.channel.header.type = PersistDT_EVENT;
					InsertChannelEvent(n_channelEvents, 0.0f, NAMED_EVENT, (char*)"Begin Animation");
//					InsertChannelEvent(n_channelEvents, 0.0f, CHANNEL_BEGIN, NULL);
					if(ip->GetRootNode()->HasNoteTracks())
					{
						DefNoteTrack * ntrack = (DefNoteTrack*)(ip->GetRootNode()->GetNoteTrack(0));
						int keys = ntrack->NumKeys();
						for(int i = 0; i < keys; i++)
						{
							int evTime = (ntrack->GetKeyTime(i)/GetTicksPerFrame());
							if(evTime >= first && evTime < last && ntrack->keys[i]->note)
							{
								float time = ((float)(evTime-first))/fps;
								InsertChannelEvent(n_channelEvents, time, NAMED_EVENT, (char*)ntrack->keys[i]->note);
							}
						}
					}

					float end_channel = ((float)last-1)/fps;
					InsertChannelEvent(n_channelEvents, end_channel, NAMED_EVENT, (char*)"End Animation");
//					InsertChannelEvent(n_channelEvents, end_channel, CHANNEL_END, NULL);

					// Script
					NamedScript sub_script;
					InitScript(&sub_script);
					_snprintf(sub_script.name, 255, "Sc_%s", note_list[iNote].name);
					sub_script.channel_count = 2;
					sub_script.channel_list = (PersistAnimChannelMapping*)
										Malloc(sizeof(PersistAnimChannelMapping)*2);
					strcpy(sub_script.channel_list[0].parent, script.channel_list[0].parent);
					strcpy(sub_script.channel_list[0].child, script.channel_list[0].child);
					strcpy(sub_script.channel_list[0].channel, sub_channel.name);

					sub_script.channel_list[1].parent[0] = 0;
					sub_script.channel_list[1].child[0] = 0;
					strcpy(sub_script.channel_list[1].channel, n_channelEvents.name);

					assert(!strcmp(script.channel_list[0].channel, n_channel.name));
					InsertNamedChannel(c_obj, sub_channel);
					InsertNamedChannel(c_obj, n_channelEvents);
					InsertScript(c_obj, sub_script);
				}

				// since we don't insert the originals, we have to free them here
				Free(n_channel.channel.data);
				Free(script.channel_list);
			}
		}
		else // no notes found on track
		{
			assert(!strcmp(script.channel_list[0].channel, n_channel.name));
			InsertNamedChannel(c_obj, n_channel);
			InsertScript(c_obj, script);
		}

		Free(note_list);
	}
}

TimeValue AsciiExp::GetEndTime(INode *node)
{
	TimeValue end = ip->GetAnimRange().End();

	if(node && (end > 0) && !exporting_deformable)
	{
		Control *cont = node->GetTMController();
		if(cont)
		{
			TimeValue lask_k_time = 0;

			Control *r_cont = cont->GetRotationController();
			if(r_cont)
			{
				IKeyControl *ikeys = GetKeyControlInterface(r_cont);
				if(ikeys)
				{
					const int n_keys = ikeys->GetNumKeys();
					if(n_keys > 0)
					{
						IKey l_key[10]; // hack to prevent GetKey from corrupting memory
						ikeys->GetKey(n_keys - 1, l_key);
						lask_k_time = _MAX(lask_k_time, l_key[0].time);
					}
				}
			}
		
			Control *t_cont = cont->GetPositionController();	
			if(t_cont)
			{
				IKeyControl *ikeys = GetKeyControlInterface(t_cont);
				if(ikeys)
				{
					const int n_keys = ikeys->GetNumKeys();
					if(n_keys > 0)
					{
						IKey l_key[10];
						ikeys->GetKey(n_keys - 1, l_key);
						lask_k_time = _MAX(lask_k_time, l_key[0].time);
					}
				}
			}

			end = _MIN(lask_k_time, end);
		}
	}

	return end;
}

int AsciiExp::GetFrame(INode *node, INode *parent, Frame & frame, const TimeValue t,
					   const Matrix3 & inv_frame0_tm, int type, bool root_flag)
{
	int result = 0;
	InitFrame(&frame);

	Matrix3 tm;
	// root animation
	if(root_flag)
	{
		tm = GetMyNodeTM(node, t);
		CleanMatrix3(tm); //tm.NoScale(); NoShear(tm);

		tm = tm * inv_frame0_tm; // good

		CleanMatrix3(tm);
		TransposeMatrix3(tm);
	}
	else // child animation
	{
		// skeletons have to pass in their parent
		assert(parent || !exporting_deformable);

		tm = GetMyLocalNodeTM(node, t, parent);
		CleanMatrix3(tm);
		TransposeMatrix3(tm);

		if(relative_deformable || !exporting_deformable)
		{
			tm = tm * inv_frame0_tm; // good
		}
	}

	AffineParts ap;
	decomp_affine(tm, &ap);
	//Quat qi(0.0, 0.0, 0.0, 1.0);
	//ap.q.MakeClosest(qi);
	if(type & (REVOLUTE | SPHERICAL | LOOSE))
	{		
		Point3 axis;
		float angle;
		AngAxisFromQ(ap.q, &angle, axis);
		if(fabs(angle) > 0.0f)
		{
			axis = Normalize(axis);

			frame.vector_r.x = axis.x;
			frame.vector_r.y = axis.y;
			frame.vector_r.z = axis.z;
			frame.angle = angle; //don't normalize from -180 to 180 YET
		}

		if( (type & (REVOLUTE | SPHERICAL)) &&
			( fabs(ap.t.x) + fabs(ap.t.y) + fabs(ap.t.z) > .001f ) )
		{
			result = -1;
		}
	} 

	if(type & (PRISMATIC | TRANSLATIONAL | LOOSE))
	{
		float magnitude = (float)sqrt(ap.t.x*ap.t.x + ap.t.y*ap.t.y + ap.t.z*ap.t.z);
									  
		if(magnitude >= 0.00001f)
		{
			frame.step = magnitude;
			magnitude = 1.0f / magnitude;

			frame.vector_t.x = ap.t.x * magnitude; 
			frame.vector_t.y = ap.t.y * magnitude; 
			frame.vector_t.z = ap.t.z * magnitude;
		}
	}

	return result;
}

void AdjustHeading(Matrix3 & tm, INode *node, const TimeValue t1, const TimeValue t2)
{		
	Matrix3 skel_tm ( node->GetNodeTM(t1) );
	CleanMatrix3(skel_tm);
	Matrix3 start_tm ( node->GetNodeTM(t2) );
	CleanMatrix3(start_tm);

	// in MAX heading is orientation about Z
	// pick i or j to compute angle about Z (whichever is more perpendicular)
	const Point3 start_i (start_tm.GetRow(0));
	const Point3 start_j (start_tm.GetRow(1));

	float start_z_rot;
	float skel_z_rot;
	if( fabs(DotProd(start_i, Point3(0,0,1))) < fabs(DotProd(start_j, Point3(0,0,1))) )// use i
	{
		start_z_rot = (float)atan2(start_i.y, start_i.x);

		const Point3 skel_i (skel_tm.GetRow(0));
		skel_z_rot = (float)atan2(skel_i.y, skel_i.x);
	}
	else// use j
	{
		start_z_rot = (float)atan2(-start_j.x, start_j.y);

		const Point3 skel_j (skel_tm.GetRow(1));
		skel_z_rot = (float)atan2(-skel_j.x, skel_j.y);
	}

	/*
	while(start_z_rot < 0.0f)
	{
		start_z_rot += (float)M_PI;
	}

	while(skel_z_rot < 0.0f)
	{
		skel_z_rot += (float)M_PI;
	}
	*/

	/*
	float start_ang[3];
	float skel_ang[3];
	MatrixToEuler(start_tm, start_ang, EULERTYPE_XYZ);
	MatrixToEuler(skel_tm, skel_ang, EULERTYPE_XYZ);

	start_ang[0] *= R2D;
	start_ang[1] *= R2D;
	start_ang[2] *= R2D;

	skel_ang[0] *= R2D;
	skel_ang[1] *= R2D;
	skel_ang[2] *= R2D;
	*/


	// default .989723 -.136345 .005883 -.042707

	/*
	Quat q;
	bool up = true;
	float ang = 6.0f;
	float delta = 0.1f;
	do
	{
		Matrix3 tmp_m = tm;

		tmp_m.PreRotateY( ang );



	}while( fabs(q.z) > .000001 )

	EulerToQuat(float *ang, Quat &q, int type);

	EulerToQuat(ang, q, EULERTYPE_XYZ);
	*/


	if(adjust_rotate_x > 45.0f && adjust_rotate_x < 135.0f)
	{
		//float tmp1 = start_z_rot * R2D;
		//float tmp2 = skel_z_rot * R2D;
		tm.PreRotateY(start_z_rot - skel_z_rot);
		//tm.PreRotateY( 7.184375 * D2R); // 7.175 - 7.19375
		//tm.PreRotateY(start_ang[2]*D2R - skel_ang[2]*D2R);
		//tm.RotateY(-start_z_rot + skel_z_rot);
	}
	else
	{
		tm.PreRotateZ(start_z_rot - skel_z_rot);
	}
}

// relative to rest position at frame 0
int AsciiExp::ReadFrames(INode *node, Frame* frame_list, int type, INode *parent, bone_lib *bl)
{
#ifdef _DEBUG
	const char *name = (node) ? node->GetName() : NULL;
	const char *pname = (parent) ? parent->GetName() : NULL;
#endif
	const TimeValue start = _MAX<TimeValue>(0, ip->GetAnimRange().Start());
	const TimeValue end = GetEndTime(node);
	const int delta = GetTicksPerFrame();
	const int n_frames = 1 + (end - start) / delta;

	int node_count;
	Frame ** node_frame_list;
	Matrix3 * inv_frame0_tm_list;
	INode ** node_list;
	INode ** parent_list;
	bool *root_flag_list;
	int *type_list;
	bool *bad_data_list;
    if(bl == NULL)
	{
		node_count = 1;
		node_frame_list = (Frame**)Malloc(sizeof(Frame*));
		node_frame_list[0] = frame_list;
		inv_frame0_tm_list = (Matrix3*)Malloc(sizeof(Matrix3));
		node_list = (INode**)Malloc(sizeof(INode*));
		node_list[0] = node;
		parent_list = (INode**)Malloc(sizeof(INode*));
		parent_list[0] = parent;
		root_flag_list = (bool*)Malloc(sizeof(bool));
		root_flag_list[0] = !strcmp(root_name, node->GetName());
		type_list = (int*)Malloc(sizeof(int));
		type_list[0] = type;
		bad_data_list = (bool*)Malloc(sizeof(bool));
		bad_data_list[0] = false;
	}
	else
	{
		node_count = bl->count;
		node_frame_list = (Frame**)Malloc(node_count * sizeof(Frame*));
		inv_frame0_tm_list = (Matrix3*)Malloc(node_count * sizeof(Matrix3));
		node_list = (INode**)Malloc(node_count * sizeof(INode*));
		parent_list = (INode**)Malloc(node_count * sizeof(INode*));
		root_flag_list = (bool*)Malloc(node_count * sizeof(bool));
		type_list = (int*)Malloc(node_count * sizeof(int));
		bad_data_list = (bool*)Malloc(node_count * sizeof(bool));

		for(int i = 0; i < node_count; i++)
		{
			node_frame_list[i] = bl->bone_list[i].frame_list;
			node_list[i] = bl->bone_list[i].node;
			parent_list[i] = bl->bone_list[i].parent;
			bl->bone_list[i].frame_count = n_frames;
			root_flag_list[i] = !strcmp(root_name, bl->bone_list[i].node->GetName());
			type_list[i] = bl->bone_list[i].con_type;
			bad_data_list[i] = false;
		}
	}

	// init first frame inverse transforms
	for(int id = 0; id < node_count; id++)
	{
		INode * current_node = node_list[id];
		INode * current_parent = parent_list[id];
		Matrix3 & inv_frame0_tm = inv_frame0_tm_list[id];
	
		if(root_flag_list[id]) // root node is special
		{
			inv_frame0_tm = GetMyNodeTM(current_node, start);

			// characters have rotation relative to skeleton
			// which is either frame 0 or a negative frame
			if(exporting_deformable)
			{
				const TimeValue adjust_time = _MIN<TimeValue>(0, ip->GetAnimRange().Start());
				const Matrix3 skel_tm ( GetMyNodeTM(current_node, adjust_time) );

				inv_frame0_tm.SetRow(0, skel_tm.GetRow(0));
				inv_frame0_tm.SetRow(1, skel_tm.GetRow(1));
				inv_frame0_tm.SetRow(2, skel_tm.GetRow(2));
			}
			
			CleanMatrix3(inv_frame0_tm); //inv_frame0_tm.NoScale(); NoShear(inv_frame0_tm);

			if(allign_heading)
			{
				AdjustHeading(inv_frame0_tm, current_node,
					_MIN<TimeValue>(0, ip->GetAnimRange().Start()), start);
			}

			inv_frame0_tm = Inverse(inv_frame0_tm);
		}
		else // regular node in the tree
		{
			if(exporting_deformable && relative_deformable)
			{
				inv_frame0_tm = GetMyLocalNodeTM(current_node,
					_MIN<TimeValue>(0, ip->GetAnimRange().Start()),
					current_parent);
				//inv_frame0_tm = GetMyLocalNodeTM(current_node, ip->GetAnimRange().Start()); 
				// might (probably not)  be needed if characters animations were exported
				// relative instead of absolute
			}
			else
			{
				inv_frame0_tm = GetMyLocalNodeTM(current_node, start, current_parent);
			}

			CleanMatrix3(inv_frame0_tm);
			TransposeMatrix3(inv_frame0_tm);
			inv_frame0_tm = Inverse(inv_frame0_tm);
		}
	}


	char prog_buf[256] = {0};
	TimeValue t = start;
	for (int i = 0; i < n_frames; i++, t+=delta)
	{
		for(int id = 0; id < node_count; id++)
		{
			if(node_frame_list[id]) // skip nodes for which we don't want to exort animation
			{
				int result = GetFrame(node_list[id], parent_list[id], node_frame_list[id][i], t,
					inv_frame0_tm_list[id], type_list[id], root_flag_list[id]);

				if(bad_data_list[id] == false && result < 0)
				{
					bad_data_list[id] = true;
					fprintf(stderr,
						"Warning: Rotational/Spherical node %s has some translational data on frame %d\n!\n",
						node_list[id]->GetName(), t/delta);
				}

				node_frame_list[id][i].index = t;
			}
		}


		if(bl != NULL)
		{
			sprintf(prog_buf, "Frame %d", t / delta);
			ip->ProgressUpdate((int)((float)(i)/n_frames*100.0f), FALSE, prog_buf);
			if( ip->GetCancel() ) break;
		}
	}


	for(id = 0; id < node_count; id++)
	{
		if(node_frame_list[id])
		{
			CleanFrames(node_frame_list[id], n_frames);
		}
	}


	Free(node_frame_list);
	Free(inv_frame0_tm_list);
	Free(node_list);
	Free(parent_list);
	Free(root_flag_list);
	Free(type_list);
	Free(bad_data_list);

	return(n_frames);
}

//                                                                  
// Win32Exec: Create process and return IMMEDIATELY, without doing a
//            WaitForInputIdle() call like WinExec()                
//                                                                  
// Optionally, wait for process to terminate                        
//                                                                  
//

BOOL Win32Exec(LPSTR lpCmdLine, BOOL bWait)
{
   STARTUPINFO         StartInfo;
   PROCESS_INFORMATION ProcessInfo;
   BOOL                result;
   
   memset(&StartInfo,   0, sizeof(StartInfo));
   memset(&ProcessInfo, 0, sizeof(ProcessInfo));

   StartInfo.cb = sizeof(StartInfo);

   result = CreateProcess(NULL,         // Image name
                          lpCmdLine,    // Command line
                          NULL,         // Process security
                          NULL,         // Thread security
                          FALSE,        // Do not inherit handles
                          0,            // Creation flags
                          NULL,         // Inherit parent environment
                          NULL,         // Keep current working directory
                         &StartInfo,    // Startup info structure
                         &ProcessInfo); // Process info structure

   if (bWait)
      {
      WaitForSingleObject(ProcessInfo.hProcess,
                          INFINITE);
      }

   return result;
}

// Not truly the correct way to compare floats of arbitary magnitude...
BOOL EqualPoint3(Point3 p1, Point3 p2)
{
	#define ALMOST_ZERO 1.0e-3f

	if (fabs(p1.x - p2.x) > ALMOST_ZERO)
		return FALSE;
	if (fabs(p1.y - p2.y) > ALMOST_ZERO)
		return FALSE;
	if (fabs(p1.z - p2.z) > ALMOST_ZERO)
		return FALSE;

	return TRUE;
}

BOOL AsciiExp::CheckForAnimation(INode* node, BOOL& bPos, BOOL& bRot, BOOL& bScale, INode *parent)
{
	bPos = bRot = bScale = FALSE;

	if( !(node->IsAnimated()) )
		return FALSE;

	TimeValue start = _MAX<TimeValue>(0, ip->GetAnimRange().Start());
	TimeValue end = GetEndTime(node);
	TimeValue t;
	int delta = GetTicksPerFrame();
	Matrix3 tm;
	AffineParts ap;
	Point3 firstPos(0,0,0);
	float rotAngle, firstRotAngle=0;
	Point3 rotAxis;
	Point3 firstScaleFactor(0,0,0);
	Matrix3 inv_frame0_tm;
	int root_flag;

	if(!strcmp(root_name, node->GetName())){
		root_flag=1;
	}
	else{
		root_flag=0;
	}

	if(root_flag)
	{
		inv_frame0_tm=Inverse(GetMyNodeTM(node, start));
	}
	else
	{
		inv_frame0_tm=Inverse(GetFullLocalNodeTM(node, start, parent));
	}

	for (t=start; t<=end; t+=delta)
	{	
		if(root_flag)
		{
			tm = GetMyNodeTM(node, t) * inv_frame0_tm;
		}
		else
		{
			tm = GetFullLocalNodeTM(node, t, parent) * inv_frame0_tm;
		}

		decomp_affine(tm, &ap);
		AngAxisFromQ(ap.q, &rotAngle, rotAxis);

		if (t != start) {
			if (!EqualPoint3(ap.t, firstPos)) {
				bPos = TRUE;
			}
			// We examine the rotation angle to see if the rotation component
			// has changed.
			// Although not entierly true, it should work.
			// It is rare that the rotation axis is animated without
			// the rotation angle being somewhat affected.
			if (fabs(rotAngle - firstRotAngle) > (float)(1.0*D2R)) {  // use 1 degree
				bRot = TRUE;
			}
			if (!EqualPoint3(ap.k, firstScaleFactor)) {
				bScale = TRUE;
			}			
		}
		else {
			firstPos = ap.t;
			firstRotAngle = rotAngle;
			firstScaleFactor = ap.k;
		}

		// No need to continue looping if all components are animated
		if (bPos && bRot && bScale)
			break;
	}

	return bPos || bRot || bScale;
}

void AsciiExp::GetFixed(INode *node, Fix *fixed, INode *parent)
{
	//const char *node_name = node->GetName();

	TimeValue t;
	if(exporting_deformable && relative_deformable)
	{
		t = _MIN<TimeValue>(0, ip->GetAnimRange().Start());
	}
	else
	{
		t = GetStaticFrame();
	}

		Matrix3 tm;

		InitFixed(fixed);

//#pragma message("TODO: reconsile bone types w/ use_loose_joints ~3281 " __FILE__ )
		// identity position for loose joints
		if(!relative_deformable && exporting_deformable && use_loose_joints && (MyGetGBufID(node) != UV_BONE)) 
		{
			fixed->pos.x =
			fixed->pos.y =
			fixed->pos.z = 0.0f;
		}
		else
		{
			// get local transform matrix
			tm = GetMyLocalNodeTM(node, t, parent);

			CleanMatrix3(tm);
			TransposeMatrix3(tm);

			// Get translation
			Point3 pos = tm.GetTrans();

			fixed->pos.x = pos.x;
			fixed->pos.y = pos.y;
			fixed->pos.z = pos.z;
		}

		if(relative_deformable || !exporting_deformable)
		{
			// Get orientation
			Point3 point;

			// NOTE: DA matrices are transposed from MAX matrices but we already
			// took care of this w/ CleanMatrix3
			point=tm.GetRow(0);
			fixed->orient.e00=point.x;
			fixed->orient.e01=point.y;
			fixed->orient.e02=point.z;

			point=tm.GetRow(1);
			fixed->orient.e10=point.x;
			fixed->orient.e11=point.y;
			fixed->orient.e12=point.z;

			point=tm.GetRow(2);
			fixed->orient.e20=point.x;
			fixed->orient.e21=point.y;
			fixed->orient.e22=point.z;
		}
		// else identity orientation for spherical joints

		char name[256]; name[0] = 0;
		strncpy(name, node->GetName(), 255);
		if(IsPlaceholder(node))
		{
			if(!StripPrefix(name, "Particle"))
			{
				if(!StripPrefix(name, "particle"))
				{
					StripPrefix(name, "PARTICLE");
				}
			}
			StripPrefix(name, "_");
		}
		
		// parent wasn't explicitly supplied, find it in the hierarchy
		if(parent == NULL)
		{
			parent = GetMyObjParent(node);
			assert(parent);
		}

		if(!strcmp(parent->GetName(), root_name)) // parent is ROOT
		{
			strcpy(fixed->parent, ROOT_OBJ_NAME);
		}
		else
		{
			strncpy(fixed->parent, parent->GetName(), PARTNAME_MAX-1);	 
		}
		strcpy(fixed->child, name);
}

void AsciiExp::GetFixedHP(INode *node, Fix *fixed, INode *parent)
{
		InitFixed(fixed);

		// get local transform matrix
		Matrix3 tm ( GetMyLocalNodeTM(node, GetStaticFrame(), parent) );
		CleanMatrix3(tm);
		TransposeMatrix3(tm);

		// Get translation
		Point3 pos ( tm.GetTrans() );

		fixed->pos.x = pos.x;
		fixed->pos.y = pos.y;
		fixed->pos.z = pos.z;

		// Get orientation
		// NOTE: DA matrices are transposed from MAX matrices but we already
		// took care of this w/ CleanMatrix3
		Point3 point;
		point=tm.GetRow(0);
		fixed->orient.e00=point.x;
		fixed->orient.e01=point.y;
		fixed->orient.e02=point.z;

		point=tm.GetRow(1);
		fixed->orient.e10=point.x;
		fixed->orient.e11=point.y;
		fixed->orient.e12=point.z;

		point=tm.GetRow(2);
		fixed->orient.e20=point.x;
		fixed->orient.e21=point.y;
		fixed->orient.e22=point.z;
	
		char name[256]; name[0] = 0;
		strncpy(name, node->GetName(), 255);
		if(IsPlaceholder(node))
		{
			if(!StripPrefix(name, "Particle"))
			{
				if(!StripPrefix(name, "particle"))
				{
					StripPrefix(name, "PARTICLE");
				}
			}
			StripPrefix(name, "_");
		}
		
		// parent wasn't explicitly supplied, find it in the hierarchy
		if(parent==NULL)
		{
			parent=GetMyObjParent(node);
			assert(parent);
		}

		if(!strcmp(parent->GetName(), root_name)) // parent is ROOT
		{
			strcpy(fixed->parent, ROOT_OBJ_NAME);
		}
		else
		{
			strncpy(fixed->parent, parent->GetName(), PARTNAME_MAX-1);	 
		}
		strcpy(fixed->child, name);
}

// Export hard points
void AsciiExp::ExportHP(object *obj, INode* node, TimeValue t) // t of static frame
{
	INode *child;
	Fix fixed;
	DofData dof_data;

	for (int i=0; i < node->NumberOfChildren(); i++) // children are counted only 1 level deep
	{
		child = node->GetChildNode(i);

		if( IsHP(child, t) )
		{										
			GetFixedHP(child, &fixed, NULL);

			GetDofData(child, &dof_data, t, NULL);
			CleanDofData(&dof_data);

			PersistHPFixed hp_fixed;
			InitHPFixed(&hp_fixed);

			strcpy(hp_fixed.name, dof_data.name);
			hp_fixed.point.x = dof_data.pivot[0];
			hp_fixed.point.y = dof_data.pivot[1];
			hp_fixed.point.z = dof_data.pivot[2];

			hp_fixed.orientation = fixed.orient;

			if(dof_data.type==FFIXED){ // parent fixed hard point
				InsertHPFixed(obj, hp_fixed);
			}else
			if(dof_data.type==PRISMATIC){ // child pris hard point
				PersistHPPrismatic hp_prismatic;

				hp_prismatic.spot=hp_fixed;
				hp_prismatic.axis.x=dof_data.axis[0];
				hp_prismatic.axis.y=dof_data.axis[1];
				hp_prismatic.axis.z=dof_data.axis[2];
				hp_prismatic.min=dof_data.min_step;
				hp_prismatic.max=dof_data.max_step;
				InsertHPPrismatic(obj, hp_prismatic);
			}else
			if(dof_data.type==REVOLUTE){ // child rev hard point
				PersistHPRevolute hp_revolute;

				hp_revolute.spot=hp_fixed;
				hp_revolute.axis.x=dof_data.axis[0];
				hp_revolute.axis.y=dof_data.axis[1];
				hp_revolute.axis.z=dof_data.axis[2];
				hp_revolute.min=dof_data.min_angle;
				hp_revolute.max=dof_data.max_angle;
				InsertHPRevolute(obj, hp_revolute);
			}
			else
			{
				Winprint("Error: %s is a bad hard point type!\n", child->GetName());
			}
		}	
	}
}

Object* FindNURBRefObject(INode *node)
{
	// Get object from node. Abort if no object.
	Object* ObjectPtr = node->GetObjectRef();
	
	while(ObjectPtr)
	{
		Class_ID cid ( ObjectPtr->ClassID() );
		if(cid == EDITABLE_SURF_CLASS_ID || cid == FITPOINT_PLANE_CLASS_ID)
		{
			return ObjectPtr;
		}

		if(cid == EDITABLE_CVCURVE_CLASS_ID || cid == EDITABLE_FPCURVE_CLASS_ID)
		{
			Winprint("Error: please collapse stack for %s to turn the curve into a surface.\n",
				node->GetName());
			return NULL;
		}

		if(ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID)
		{
			ObjectPtr = ((IDerivedObject*)ObjectPtr)->GetObjRef();
		}
		else
		{
			ObjectPtr = NULL;
		}
	}

	// Not found.
	return NULL;
}

// code to find if a given node contains a Physique Modifier
// requires "modstack.h"
Modifier* FindPhysiqueModifier (INode *node)
{
	// Get object from node. Abort if no object.
	Object* ObjectPtr = node->GetObjectRef();
	

	if (!ObjectPtr) return NULL;

	// Is derived object ?
	if (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject* DerivedObjectPtr = static_cast<IDerivedObject*>(ObjectPtr);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < DerivedObjectPtr->NumModifiers())
		{
			// Get current modifier.
			Modifier* ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);
	
			// Is this Physique ?
			if (ModifierPtr->ClassID() == Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B))
			{
				// Yes -> Exit.
				return ModifierPtr;
			}

			// Next modifier stack entry.
			ModStackIndex++;
		}
	}

	// Not found.
	return NULL;
}

bool HaveNodeID(INode *node, unsigned int id)
{
	if(node == NULL) return false;
	
	if(MyGetGBufID(node) == id){
		return true;
	}

	for (int i=0; i < node->NumberOfChildren(); i++) // children are counted only 1 level deep
	{
		if(bool result = HaveNodeID(node->GetChildNode(i), id) ){
			return result;
		}
	}

	return false;
}

Modifier* GetUVWModifier(INode *node)
{
	Object* ObjectPtr = node->GetObjectRef();
	
	if (!ObjectPtr) return NULL;

	// Is derived object ?
	if (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject* DerivedObjectPtr = static_cast<IDerivedObject*>(ObjectPtr);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < DerivedObjectPtr->NumModifiers())
		{
			// Get current modifier.
			Modifier* ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);
	
			//if(!strcmp(ModifierPtr->GetName(), "UVW Mapping"))
			if(ModifierPtr->ClassID() == Class_ID(UVWMAPOSM_CLASS_ID, 0) &&
				ModifierPtr->SuperClassID() == OSM_CLASS_ID )
			{
				return ModifierPtr;
			}
				
			// Next modifier stack entry.
			ModStackIndex++;
		}
	}
	return NULL;
}

/*
Modifier* GetUVWModifier(INode *node)
{

	Object* ObjectPtr = node->GetObjectRef();
	
	if (!ObjectPtr) return NULL;

	// Is derived object ?
	if (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject* DerivedObjectPtr = static_cast<IDerivedObject*>(ObjectPtr);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < DerivedObjectPtr->NumModifiers())
		{
			// Get current modifier.
			Modifier* ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);
	
			//if(!strcmp(ModifierPtr->GetName(), "UVW Mapping"))
			if(ModifierPtr->ClassID() == Class_ID(UVWMAPOSM_CLASS_ID, 0) &&
				ModifierPtr->SuperClassID() == OSM_CLASS_ID )
			{


                                                                    // 1
				Control *c = ((Control*)ModifierPtr->SubAnim(TM_REF ))->GetPositionController();
				IParamBlock *pb = (IParamBlock*)ModifierPtr->SubAnim(PBLOCK_REF); // 2
			

#if 1
				//c = c->GetPositionController();
				IKeyControl *ikeys = GetKeyControlInterface(c);


				if (c->ClassID() == Class_ID(HYBRIDINTERP_POSITION_CLASS_ID, 0))
				{
					Winprint("Error: unsupported Bezier animation controller!");
					
					
				}else
				if (c->ClassID() == Class_ID(TCBINTERP_POSITION_CLASS_ID, 0))
				{
					//ITCBPoint3Key tcbPosKey;
					Winprint("Error: unsupported TCB animation controller!");
					
				}else
				if (c->ClassID() == Class_ID(LININTERP_POSITION_CLASS_ID, 0))
				{
					//ILinPoint3Key linPosKey;
					Winprint("Error: unsupported LINEAR animation controller!");
					
				}
				else{
					Winprint("Error: UNKNOWN animation controller!");
				
			}
#endif

				return ModifierPtr;
			}
				
		
			// Next modifier stack entry.
			ModStackIndex++;
		}
	}

	return NULL;
}
*/

// bones version 1
INode* GetBonesProNodeVer1(INode *node)
{
	if(!node) return NULL;

	int i;
	for ( i=0; i < node->NumRefs(); i++ )
	{
		ReferenceTarget *refTarg = node->GetReference(i);

		if( refTarg != NULL && refTarg->ClassID() == Class_ID(WSM_DERIVOB_CLASS_ID,0))
		{
			IDerivedObject *WSMDerObj = (IDerivedObject *) refTarg;
			// MessageBox(NULL, "WSM found", _T("WSM"), MB_OK);

			for(int ModStackIndex=0;  ModStackIndex < WSMDerObj->NumModifiers(); ModStackIndex++)
			{
				// Get current modifier.
				Modifier* ModifierPtr = WSMDerObj->GetModifier(ModStackIndex);

				const char *name = ModifierPtr->GetName();
                if( !strcmp(name, "Bones Pro Binding") )
				{
					return node;
				}
			}
		}
	}
	
	for ( i=0; i < node->NumberOfChildren(); i++)  // children are counted only 1 level deep
	{
		if( INode *result = GetBonesProNodeVer1(node->GetChildNode(i)) )
		{
			return result;
		}
	}
	
	return NULL;
}

/*
INode* GetBonesProNodeVer2(INode *node)
{
	if(!node) return NULL;

	if(FindBonesModifier(node) != NULL)
	{
		return node;
	}

	for (int c = 0; c < node->NumberOfChildren(); c++)  // children are counted only 1 level deep
	{
		if( INode *result=GetBonesProNodeVer2(node->GetChildNode(c)) )
		{
			return result;
		}
	}
	
	return NULL;
}
*/

/*
INode* GetPhysiqueNode(INode *node)
{
	if(!node) return NULL;

	if(FindPhysiqueModifier(node) != NULL)
	{
		return node;
	}

	for (int c = 0; c < node->NumberOfChildren(); c++)  // children are counted only 1 level deep
	{
		if( INode *result=GetPhysiqueNode(node->GetChildNode(c)) )
		{
			return result;
		}
	}
	
	return NULL;
}
*/

// code to find if a given node contains a Physique Modifier
// requires "modstack.h"
Modifier* FindBonesModifier (INode *node)
{
	// Get object from node. Abort if no object.
	Object* ObjectPtr = node->GetObjectRef();
	

	if (!ObjectPtr) return NULL;

	// Is derived object ?
	if (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject* DerivedObjectPtr = static_cast<IDerivedObject*>(ObjectPtr);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < DerivedObjectPtr->NumModifiers())
		{
			// Get current modifier.
			Modifier* ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);
	
			//char m_name[256] = {0};
			//_snprintf(m_name, 255, ModifierPtr->GetName());// "Bones Pro 2"
			//Class_ID tmp_id (ModifierPtr->ClassID());
			// Is this Bones ?
//			if (ModifierPtr->ClassID() == BP_CLASS_ID_OSM || ModifierPtr->ClassID() == BP_CLASS_ID_WSM)
//			{
//				// Yes -> Exit.
//				return ModifierPtr;
//			}

			// Next modifier stack entry.
			ModStackIndex++;
		}
	}

	// Not found.
	return NULL;
}

BOOL CheckIdentity(Matrix3 *m)
{
#define TOL 0.0001f  // tolerance

	Point3 row;

	row = m->GetRow(0);
	row.x=(float)fabs(row.x - 1.0);
	row.y=(float)fabs(row.y);
	row.z=(float)fabs(row.z);

	if(row.x>TOL)
		return FALSE;
	if(row.y>TOL)
		return FALSE;
	if(row.z>TOL)
		return FALSE;

	row = m->GetRow(1);
	row.x=(float)fabs(row.x);
	row.y=(float)fabs(row.y - 1.0);
	row.z=(float)fabs(row.z);

	if(row.x>TOL)
		return FALSE;
	if(row.y>TOL)
		return FALSE;
	if(row.z>TOL)
		return FALSE;

	row = m->GetRow(2);
	row.x=(float)fabs(row.x);
	row.y=(float)fabs(row.y);
	row.z=(float)fabs(row.z - 1.0);

	if(row.x>TOL)
		return FALSE;
	if(row.y>TOL)
		return FALSE;
	if(row.z>TOL)
		return FALSE;

	return TRUE;
#undef TOL
}

void LoadObject(object *obj, char *file_name)
{
  Winprint("Error: LoadObject() is broken!\n"); return;

  InitObject(obj, NULL);
  obj->type = FIXED_MESH;
  
  file_node *root;

  root=ReadUTF(file_name);

  Load3DBObject( obj, root); 
  FreeTreeData(root);
}

void InitMaxSkin(max_skin *ms, mtl_lib *ml)
{
	InitObject(&(ms->obj), ml);
	ms->obj.type = DEF_MESH;
	ms->skin_node=NULL;
	ms->bone_count=0;
	ms->vertex_count=0;
	//ms->bone_node_list=NULL;
	ms->vertex_bone_matrix=NULL;
}

void FreeMaxSkin(max_skin *ms)
{
	FreeObject(&(ms->obj));
	//Free(ms->bone_node_list);
	if(ms->vertex_bone_matrix){
		Free(ms->vertex_bone_matrix[0]);
	}
	Free(ms->vertex_bone_matrix);
}

void FreeBoneLib(bone_lib *bl)
{
	int i;
	for(i=0; i<bl->count; i++)
	{
		FreeMaxBone(&(bl->bone_list[i]));
	}
	Free(bl->bone_list);
}

void FreeMaxBone(max_bone* mb)
{
	Free(mb->frame_list);
}

void AsciiExp::AssignFaceVertices(max_skin *ms, bone_lib *bl)
{
	int i, j, k;
	INode *face_node;
	float radius;

	ms->bone_count = bl->count;

	face_node = ms->skin_node;
	radius = ms->obj.extents.sphere.radius;

	face_node->EvalWorldState(1 * GetTicksPerFrame());

	Matrix3 tm ( GetMyObjTMAfterWSM(face_node, 1 * GetTicksPerFrame()) );

	// get mesh
	BOOL needDel;
	TriObject* tri = GetTriObjectFromNode(face_node, 1, &needDel);
	if (!tri) {
		return;
	}
	
	Mesh *mesh = &(tri->mesh);
	// mesh->DeleteIsoVerts();  // causes indexing problem ??
	Mesh start_mesh(*mesh);

#if 0 // causes indexing problem
	start_mesh.buildNormals();
	if(start_mesh.RemoveDegenerateFaces()){ // two or more equal indecies
		fprintf(stderr,"Warning: removed faces w/ 2 or more equal indicesfrom %s.\n",FixupName(face_node->GetName()));
	}
	if(start_mesh.RemoveIllegalFaces()){ // indices out of range
		fprintf(stderr,"Warning: removed faces w/ indices out of range from %s\n",FixupName(face_node->GetName()));
	}
#endif
	
	ms->vertex_count = start_mesh.getNumVerts();

	ms->vertex_bone_matrix = (bone_contrib**)Malloc(ms->vertex_count*sizeof(bone_contrib*));
	ms->vertex_bone_matrix[0] = (bone_contrib*)Malloc(ms->vertex_count * ms->bone_count * sizeof(bone_contrib));
	for(i=1; i<ms->vertex_count; i++){
		ms->vertex_bone_matrix[i] = ms->vertex_bone_matrix[0] + i*ms->bone_count;
	}

	// init
	for(i=0; i<ms->vertex_count; i++)
	{
		for(j=0; j<ms->bone_count; j++)
		{
			ms->vertex_bone_matrix[i][j].magnitude =
			ms->vertex_bone_matrix[i][j].relative = 0.0f;
			ms->vertex_bone_matrix[i][j].node = bl->bone_list[j].node;
		}
	}
	
	BOOL tmp_del;
	TriObject *tmp_tri;
	Mesh *tmp_mesh;
	Matrix3 b_tm, start_b_tm;
	Point3 p1, p2, delta, trans;
	Point3 trans_delta;
	float magnitude;

	ip->SetTime(1*GetTicksPerFrame(), TRUE);
	ip->SetAnimateButtonState(1);

	for(j=0; j<ms->bone_count; j++)
	{
		// disconnect any children
		int num_children = bl->bone_list[j].node->NumberOfChildren();
		INode **child_list = NULL;

		if(num_children > 0)
		{
			child_list = (INode**)Malloc(num_children * sizeof(INode*));
			for(int c_id=0; c_id<num_children; c_id++)
			{
				child_list[c_id] = (bl->bone_list[j].node)->GetChildNode(0);
				child_list[c_id]->Detach(1, 1);
			}
		}

		// save node position
		start_b_tm = GetMyNodeTM(bl->bone_list[j].node, 1);

		for(k=0; k<3; k++){
			switch(k)
			{
			case 0:
				trans_delta.x = .1f * radius;
				trans_delta.y = trans_delta.z = 0.0f;
				break;
			case 1:
				trans_delta.y = .1f * radius;
				trans_delta.x = trans_delta.z = 0.0f;
				break;
			case 2:
				trans_delta.z = .1f * radius;
				trans_delta.x = trans_delta.y = 0.0f;
				break;
			}

			// save state for undo
			theHold.Begin();

			// move bone
			// should move x,y,z individually and add up effects
			b_tm = start_b_tm;
			b_tm.Translate(trans_delta);
			bl->bone_list[j].node->SetNodeTM(1, b_tm);

//ip->RedrawViews(1, REDRAW_INTERACTIVE, NULL); 

			// get new mesh
			tmp_tri = GetTriObjectFromNode(face_node, 1, &tmp_del);
			tmp_mesh = &(tmp_tri->mesh);
			// tmp_mesh->DeleteIsoVerts();

			for(i=0; i<ms->vertex_count; i++){
				// get starting vert position
				p1 = tm * start_mesh.verts[i];

				// get new vert position
				p2 = tm * tmp_mesh->verts[i];

				delta = p2 - p1;
#if 0
				switch(k)
				{
				case 0:
					magnitude = (float)(sqrt(delta.x*delta.x) / 
							(.1f * radius)) / (1.0f/3.0f);
					break;
				case 1:
					magnitude = (float)(sqrt(delta.y*delta.y) / 
							(.1f * radius)) / (1.0f/3.0f);
					break;
				case 2:
					magnitude = (float)(sqrt(delta.z*delta.z) / 
							(.1f * radius)) / (1.0f/3.0f);
					break;
				}
#else
				magnitude = (float)(sqrt(delta.x*delta.x + delta.y*delta.y + delta.z*delta.z) / 
							(.1f * radius)) * (1.0f/3.0f); // divide by 3 for x,y,z
#endif

				ms->vertex_bone_matrix[i][j].magnitude += magnitude;
			}
			if(tmp_del) delete tmp_tri;

			// move bone back
			theHold.Cancel();
			//bl->bone_list[j].node->SetNodeTM(1, start_b_tm);
		}

		// reconnect any children
		if(num_children > 0)
		{
			for(int c_id=0; c_id<num_children; c_id++)
			{
				bl->bone_list[j].node->AttachChild(child_list[c_id], 1);
			}
	
			Free(child_list);
		}

	}

	//ip->RedrawViews(1, REDRAW_INTERACTIVE, NULL); 

	if (needDel) 
	{
		tri->DeleteMe();
		tri = NULL;
	}

	ip->SetAnimateButtonState(0);
	ip->SetTime(ip->GetAnimRange().Start(), TRUE);
    
//SortBoneWeightMatrix(ms);

	// mark unassigned vertices to be later assigned to root (neck, head, or nose)
	// assign non affected vertices to root/nose
	int unaffected_count = 0;
	for(i=0; i<ms->vertex_count; i++)
	{
		if(ms->vertex_bone_matrix[i][0].magnitude == 0.0f)
		{
			unaffected_count++;
			ms->vertex_bone_matrix[i][0].magnitude =
			ms->vertex_bone_matrix[i][0].relative = 1.0f; 
			ms->vertex_bone_matrix[i][0].node = bl->bone_list[0].node; 

			//ms->obj.v.assignment_vertex_list[i] = default_node;
		}
	}
}

void FixBiped(Interface *ip)
{
	INode *node = ip->GetINodeByName("Bip01");

	if (node)
	{
		// Get the node's transform control
		Control *c = node->GetTMController();

		// You can test whether or not this is a biped controller with the following pseudo code:
		if ((c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) ||
			(c->ClassID() == BIPBODY_CONTROL_CLASS_ID) || 
			(c->ClassID() == FOOTPRINT_CLASS_ID))
		{
			// Get the Biped Export Interface from the controller 
			IBipedExport *BipIface = (IBipedExport *) c->GetInterface(I_BIPINTERFACE);

			// Remove the non uniform scale
			BipIface->RemoveNonUniformScale(1);
		}
	}
}

int AsciiExp::ExportDeformable(void)
{
	bone_lib bl;
	InitBoneLib(&bl);
	
	GetBones(&bl);

	if(bl.count == 0)
	{
		//world_adjust.IdentityMatrix();
		return 1;
	}

	// sort & resolve bone types
	if(OrganizeBones(&bl) == false)
	{
		return -1;
	}

	// hack to force HTR files to load
	ip->SetTime(ip->GetAnimRange().End(), TRUE);
	ip->SetTime(_MAX(0, ip->GetAnimRange().Start()), TRUE);

	// adjust head as well ??
	if( (fabs(adjust_rotate_x) || fabs(adjust_rotate_y) || fabs(adjust_rotate_z)) )
	{
		world_adjust =	RotateXMatrix(adjust_rotate_x * (float)D2R) * // 90
						RotateYMatrix(adjust_rotate_y * (float)D2R) *
						RotateZMatrix(adjust_rotate_z * (float)D2R) ; // 180 for FL
	}
	else
	{
		world_adjust.IdentityMatrix();
	}

	root_name = _strdup(bl.bone_list[0].node->GetName());

	if(allign_root_to_world)
	{
		// 0 is in case animations start past 0 frame
		TimeValue adjust_time = _MIN<TimeValue>(0, ip->GetAnimRange().Start());
		root_adjust_m = bl.bone_list[0].node->GetNodeTM(adjust_time);
		CleanMatrix3(root_adjust_m); //root_adjust_m.NoScale(); NoShear(root_adjust_m);
		root_adjust_m.NoTrans();
		root_adjust_m = Inverse(root_adjust_m);
	}
	else
	{
		root_adjust_m.IdentityMatrix();
	}

	CompoundObject c_obj;
	InitCompoundObject(&c_obj);

	INode *first_mesh_node = NULL;
	for(int i = 0; i < inode_list.count; i++)
	{
		INode *node = inode_list[i];

		Modifier *mod;
		if(NULL != (mod = FindPhysiqueModifier(node)))
		{
			first_mesh_node = node;
			break;
		}
		else
		if(NULL != (mod = FindBonesModifier(node)))
		{
			first_mesh_node = node;
			break;
		}
	}

	if(!first_mesh_node)
	{
		for(int i = 0; i < inode_list.count; i++)
		{
			INode *node = inode_list[i];

			if(MyGetGBufID(node) == MESH_BONE || MyGetGBufID(node) == UV_BONE)
			{
				first_mesh_node = node;
			}
		}
	}

	const char *first_mesh_name = (first_mesh_node) ? first_mesh_node->GetName() : NULL;

	max_skin ms;
	InitMaxSkin(&ms, /*&(c_obj.tl), &(c_obj.atl),*/ &(c_obj.ml));
	if(export_mesh && first_mesh_node)
	{
		const TimeValue mesh_time = _MIN<TimeValue>(0, ip->GetAnimRange().Start());

		// for all nodes w/ physique, bones pro, ... or id3 export into one mesh
		for(int i = 0; i < inode_list.count; i++)
		{
			INode *node = inode_list[i];

			bool export_this = false;
			Modifier *mod;
			if(NULL != (mod = FindPhysiqueModifier(node)))
			{
				export_this = true;
			}else
			if(NULL != (mod = FindBonesModifier(node)))
			{
				export_this = true;
			}else
			if(MyGetGBufID(node) == MESH_BONE || MyGetGBufID(node) == UV_BONE)
			{
				export_this = true;
			}

			if(export_this)
			{
				const char *mesh_name = node->GetName();

				// rigid_body assign_bone
				if(FALSE == Export3DB(node, &(ms.obj), DEF_OBJ, bl.bone_list[0].node, mesh_time))
				{
					Winprint("Error: %s failed to exort!\n", mesh_name);
					return -1;
				}
			}
		}


		if(ms.obj.type == DEF_MESH)
		{
#if USE_DA_MESH
			ms.obj.da_mesh.PostProcess(options.nLodPercent, options.nLodClosestDist, options.nLodFurthestDist);
			calcRigidBody(&(ms.obj), options.nDensity, -1, first_mesh_name, true);
#else
			PostProcessMesh(&(ms.obj), options.nDensity, options.nLodPercent, options.nLodClosestDist,
				options.nLodFurthestDist, true, first_mesh_name, 1.0f);
#endif
		}else
		if( ms.obj.type == DEF_NURB || ms.obj.type == DEF_PATCH)// nurb or bezier
		{
			calcRigidBody(&(ms.obj), options.nDensity, -1, first_mesh_name, true);
		}
		

#if 0 // this bakes vertices and screws up patch indexing for later //def _DEBUG
		char out_file_name[256] = {0};
		if(ms.obj.type == DEF_MESH)
		{
			_snprintf(out_file_name, 255, "%s%s.3db", dest_path, first_mesh_name);
		}else
		if(ms.obj.type == DEF_NURB)
		{
			_snprintf(out_file_name, 255, "%s%s.nrb", dest_path, first_mesh_name);
		}else
		if(ms.obj.type == DEF_PATCH)
		{
			_snprintf(out_file_name, 255, "%s%s.bez", dest_path, first_mesh_name);
		}

		WriteObject(&(ms.obj), out_file_name, txt_flag);//TXT_ON);
#endif
		// export all vertices in their bone space
		if(ms.obj.type == DEF_MESH)
		{
#if USE_DA_MESH
			ExportBoneVertices( ms.obj.da_mesh, &bl, mesh_time );
			SynchFreeBoneMeshVertices( ms.obj.da_mesh );
#else
			ExportBoneVertices(&(ms.obj), &bl, mesh_time);
			SynchFreeBoneMeshVertices(&(ms.obj.v));

			//ExportBoneUVVertices(&(ms.obj), &bl, mesh_time);
			ExportBoneUVVertices(&(ms.obj), &bl, mesh_time, REYE_BONE, RIGHT_EYE);
			ExportBoneUVVertices(&(ms.obj), &bl, mesh_time, LEYE_BONE, LEFT_EYE);
#endif
		}else
		if(ms.obj.type == DEF_NURB) // nurb
		{
			ExportBoneNurbCV(&(ms.obj), &bl, mesh_time);
			SynchFreeBoneNurbCV(&(ms.obj));
		}else
		if(ms.obj.type == DEF_PATCH)
		{
			ExportBoneBezCV(&(ms.obj.b_mesh), &bl, mesh_time);
			SynchFreeBoneBezCV(&(ms.obj.b_mesh));
		}
	}
	
	// export compound object & animations
	ExportBoneConnections(&c_obj, &bl);

	// make sure objects don't end up in .cmp
	for(int oid=0; oid < c_obj.part_count; oid++)
	{
		c_obj.lod_object_list[oid].export_flag = 0;
	}
	
	if(export_animation)
	{
		//RemoveIdentityChannels(&c_obj);
		//ShortenConstantChannels(&c_obj);
		ExportGlobalEvents(&c_obj);
		CombineChannels(&c_obj, 1);
		ExtractKeyFrames(&c_obj);
	}

	if( export_mesh )
	{
		ExportMeshHP(&c_obj, &bl);
	}

	if(first_mesh_node && export_mesh)
	{
		assert(c_obj.part_count == bl.count);

		if(ms.obj.type == DEF_MESH)
		{
			// bounding boxes (before any LOD)
#if USE_DA_MESH
			Winprint("Warning: ComputeBoneExtents() not yet supported.\n");
#else
			ComputeBoneExtents(&(ms.obj), &c_obj, options.nDensity);
#endif

			if(options.nLodPercent < 100.0f)
			{
#if USE_DA_MESH
				Winprint("Warning: LOD not yet supported.\n");
#else
				ms.obj.lol.closest = options.nLodClosestDist;
				ms.obj.lol.furthest = options.nLodFurthestDist;

				CollapseEdges(&(ms.obj), (int)((.01f * options.nLodPercent) * ms.obj.face_count + .5f));

				ConsolidateDuplicateAssignments(&(ms.obj));
				FreeLodLib(&(ms.obj.lol));
				RemoveUnusedNormals(&(ms.obj));

				// compute new lod positions in bone space
				TransformBoneVertices(&(ms.obj), &c_obj);

				// if any groups now have 0 faces remove them
				RemoveUnusedFaceGroups(&(ms.obj));

				// keep rigid body based on original character
				// calcRigidBody(&(ms.obj), nDensity, -1, first_mesh_name, true);
#endif
			}
		}else
		if(ms.obj.type == DEF_NURB)
		{
			ComputeBoneExtentsNurbCV(&(ms.obj), &c_obj, options.nDensity);
		}else
		if(ms.obj.type == DEF_PATCH)
		{
			ComputeBoneExtentsBezCV(&(ms.obj.b_mesh), &c_obj, options.nDensity);
		}
	}

	if(scale_factor != 1.0f)
	{
#if USE_DA_MESH
		if(ms.obj.type == DEF_MESH)
			Winprint("Warning: Scaling not yet supported.\n");
		else
#endif
		{
			ScaleCompoundObject(&c_obj, scale_factor);

			for(int i=0; i < bl.count; i++) 
			{
				ScaleObjectHP(&(c_obj.lod_object_list[i].obj_list[0]), scale_factor);
			}

			ScaleObject(&(ms.obj), scale_factor);
			ScaleObjectDeformable(&(ms.obj), scale_factor);

			calcVertexNormals(&(ms.obj), 0/*SMOOTH_SHADED*/);
			calcEdges(&(ms.obj)); 
			calcRigidBody(&(ms.obj), options.nDensity, -1, first_mesh_name, true);
		}
	}
	
	// do after scaling
	if(center_mass)
	{
#if USE_DA_MESH
		if(ms.obj.type == DEF_MESH)
			Winprint("Warning: CenterMass() not yet supported.\n");
		else
#endif
			CenterMass(&c_obj, &(ms.obj));
	}

	// write out
	char out_file_name[256] = {0};
	char skeleton_name[256] = {0};
	_snprintf(out_file_name, 255, "%s%s.cmp", dest_path, body_name);
	_snprintf(skeleton_name, 255, "%s.cmp", bl.bone_list[0].node->GetName());
	if(export_mesh)
	{
		file_node **next = NULL;
		file_node *root = CreateNode("\\",D);
		root->child = CreateExporterVersion();

		if( first_mesh_node )
		{
			root->child->sibling = CreateObject(&(ms.obj), txt_flag);//TXT_ON);
			root->child->sibling->sibling = CreateRigidBody(ms.obj.extents);
			next = &(root->child->sibling->sibling->sibling);
		}
		else
		{
			next = &(root->child->sibling);
		}

		*next = CreateSkeletonName(skeleton_name);
		next = &((*next)->sibling);

		//*next = CreateCmpnd(&c_obj, 0, 0, TXT_OFF); // split_flag, anim_flag
		*next = CreateCmpnd(&c_obj, 0, 0, 0); // split_flag, anim_flag
		next = &((*next)->sibling);

		*next = CreateBoneExtents(&c_obj);
		next = &((*next)->sibling);

		_snprintf(out_file_name, 255, "%s%s.dfm",dest_path, body_name);

		WriteUTF(root, out_file_name);
		FreeTree(root);
	}

	if(export_animation)
	{
		file_node *root = CreateNode("\\",D);
		root->child = CreateExporterVersion();
		root->child->sibling = CreateAnimation(&c_obj);
		root->child->sibling->sibling = CreateSkeletonName(skeleton_name);
		
		_snprintf(out_file_name, 255, "%s%s.anm", dest_path, body_name);
		WriteUTF(root, out_file_name);
		FreeTree(root);
	}

	FreeMaxSkin(&ms);
	FreeBoneLib(&bl);
	FreeCmpObject(&c_obj);
	Free(root_name);

	return 0;
}

// NOTE: eye bones have to be alligned w/ their parent for now
void AsciiExp::ExportBoneUVVertices(object *obj, const bone_lib *bl, const TimeValue mesh_time,
									const bone_type type, const int flags)
{
	vertices * v = &(obj->v);

	int bone_id = GetFirstBoneType(bl, type);
	if(bone_id < 0)
	{
		return;
	}

	INode *node = bl->bone_list[bone_id].node;
	const char *node_name = node->GetName();

	float min_x =  FLT_MAX;
	float max_x = -FLT_MAX;
	float min_y =  FLT_MAX;
	float max_y = -FLT_MAX;
	float avg_u =  0.0f;
	float avg_v =  0.0f;
	
	Point3 *offset_list = NULL;
	int bone_index = -1; // nth uv bone

	bool hit = false;
	for(int i = 0; i < v->texture_count; i++)
	{
		if(v->uv_flags[i] & flags)
		{
			if(!hit)
			{
				bone_index = v->uv_bone_count;
				v->uv_bone_count++;
				v->uv_bone_id = (int*)Realloc(v->uv_bone_id, v->uv_bone_count * sizeof(int));
				v->uv_bone_id[bone_index] = bone_id;

				v->uv_vertex_count = (int*)Realloc(v->uv_vertex_count, v->uv_bone_count * sizeof(int));
				v->uv_vertex_count[bone_index] = 0;

				v->uv_plane_distance = (float*)Realloc(v->uv_plane_distance, v->uv_bone_count * sizeof(float));
				v->uv_plane_distance[bone_index] = 0.0f;

				v->x_to_u_scale = (float*)Realloc(v->x_to_u_scale, v->uv_bone_count * sizeof(float));
				v->x_to_u_scale[bone_index] = 0.0f;
				v->y_to_v_scale = (float*)Realloc(v->y_to_v_scale, v->uv_bone_count * sizeof(float));
				v->y_to_v_scale[bone_index] = 0.0f;

				v->min_du = (float*)Realloc(v->min_du, v->uv_bone_count * sizeof(float));
				v->min_du[bone_index] = FLT_MAX;
				v->max_du = (float*)Realloc(v->max_du, v->uv_bone_count * sizeof(float));
				v->max_du[bone_index] = -FLT_MAX;

				v->min_dv = (float*)Realloc(v->min_dv, v->uv_bone_count * sizeof(float));
				v->min_dv[bone_index] = FLT_MAX;
				v->max_dv = (float*)Realloc(v->max_dv, v->uv_bone_count * sizeof(float));
				v->max_dv[bone_index] = -FLT_MAX;

				hit = true;
			}


			v->uv_vertex_count[v->uv_bone_count - 1]++;

			const int list_index = v->uv_list_length;
			v->uv_list_length++;
			v->uv_vertex_id_list = (int*)Realloc(v->uv_vertex_id_list, v->uv_list_length * sizeof(int));
			v->uv_vertex_id_list[list_index] = i;

			v->uv_list = (float*)Realloc(v->uv_list, 2 * v->uv_list_length * sizeof(float));
			const float uu = v->texture_list[2*i];
			const float vv = v->texture_list[2*i+1];

			v->uv_list[2*list_index] = uu;
			v->uv_list[2*list_index+1] = vv;

			avg_u += uu;
			avg_v += vv;

			if(uu < v->min_du[bone_index])
			{
				v->min_du[bone_index] = uu;
			}
			if(uu > v->max_du[bone_index])
			{
				v->max_du[bone_index] = uu;
			}

			if(vv < v->min_dv[bone_index])
			{
				v->min_dv[bone_index] = vv;
			}
			if(vv > v->max_dv[bone_index])
			{
				v->max_dv[bone_index] = vv;
			}

			const int v_id = GetFirstUVtoXYZ_ID(v, i);
			assert(v_id >= 0);

			Point3 offset;
			offset.x = v->object_list[3*v_id];
			offset.y = v->object_list[3*v_id+1];
			offset.z = v->object_list[3*v_id+2];
			offset = SwitchCoord(offset, bl->bone_list[0].node, node, mesh_time);
			
			v->uv_plane_distance[bone_index] += Length(offset); //-offset.z;

			if(offset.x < min_x)
			{
				min_x = offset.x;
			}else
			if(offset.x > max_x)
			{
				max_x = offset.x;
			}

			if(offset.y < min_y)
			{
				min_y = offset.y;
			}else
			if(offset.y > max_y)
			{
				max_y = offset.y;
			}
		}
	}

	if(hit)
	{
		assert(bone_index >= 0);

		// average distance from plane
		v->uv_plane_distance[bone_index] /= (float)v->uv_vertex_count[bone_index];

		// compute scale factors
		if(max_x - min_x)
		{
			v->x_to_u_scale[bone_index] = -(v->max_du[bone_index] - v->min_du[bone_index]) / (max_x - min_x);
		}
		else
		{
			v->x_to_u_scale[bone_index] = 0.0f;
		}

		if(max_y - min_y)
		{
			// the distance should probably be negative
			// and then we need to figure out why y would have to be flipped (mapped upside down; use a test texture)
			// TODO: make the eyes independent of initial orientation if possible
			// can't: we have to be able to assume they look straight ahead in -z
			v->y_to_v_scale[bone_index] = (v->max_dv[bone_index] - v->min_dv[bone_index]) / (max_y - min_y);
		}
		else
		{
			v->y_to_v_scale[bone_index] = 0.0f;
		}
		

		const float scale_fudge_factor = 1.2f;
		v->x_to_u_scale[bone_index] *= scale_fudge_factor;
		v->y_to_v_scale[bone_index] *= scale_fudge_factor;

		// compute limits
#if 0   // based on mapping
		avg_u /= (float)v->uv_vertex_count[bone_index];
		avg_v /= (float)v->uv_vertex_count[bone_index];

		v->min_du[bone_index] = scale_fudge_factor * (v->min_du[bone_index] - avg_u);
		v->max_du[bone_index] = scale_fudge_factor * (v->max_du[bone_index] - avg_u);

		v->min_dv[bone_index] = scale_fudge_factor * (v->min_dv[bone_index] - avg_v);
		v->max_dv[bone_index] = scale_fudge_factor * (v->max_dv[bone_index] - avg_v);	

		// todo: assert that we are sorted and don't repeat
#else   // based on IK

		DofData dof_data;
		GetDofData(node, &dof_data, GetStaticFrame(), bl->bone_list[ v->uv_bone_id[bone_index] ].parent);

		// if limited (-10 to 15 about x works good  -10 is up 15 is down)
		if(dof_data.min_r[1] >= -180.0f * D2R && dof_data.max_r[1] < 180.0f * D2R)
		{
			v->min_du[bone_index] =
				(float)sin( dof_data.min_r[1] ) *
				v->uv_plane_distance[bone_index] *
				-v->x_to_u_scale[bone_index];
			v->max_du[bone_index] =
				(float)sin( dof_data.max_r[1] ) *
				v->uv_plane_distance[bone_index] *
				-v->x_to_u_scale[bone_index];

			assert(v->min_du[bone_index] <= v->max_du[bone_index]);
		}
		else // not limited
		{
			v->min_du[bone_index] = -100.0f;
			v->max_du[bone_index] =  100.0f;
		}

		if(dof_data.min_r[0] >= -180.0f * D2R && dof_data.max_r[0] < 180.0f * D2R)
		{
			v->min_dv[bone_index] =
				(float)sin( dof_data.min_r[0] ) *
				v->uv_plane_distance[bone_index] *
				v->y_to_v_scale[bone_index];
			v->max_dv[bone_index] =
				(float)sin( dof_data.max_r[0] ) *
				v->uv_plane_distance[bone_index] *
				v->y_to_v_scale[bone_index];
			assert(v->min_dv[bone_index] <= v->max_dv[bone_index]);
		}
		else
		{
			v->min_dv[bone_index] = -100.0f;
			v->max_dv[bone_index] =  100.0f;
		}
#endif
	}
}

void AsciiExp::ExportBoneNurbCV(object *obj, const bone_lib *bl, const TimeValue mesh_time)
{
	for(int nid = 0; nid < obj->nurb_count; nid++)
	{
		nurb & nrb = obj->nurb_list[nid];
	
		nrb.b_v_list = (bone_vertex*)Malloc(nrb.s_point_count * nrb.t_point_count * sizeof(bone_vertex));

		INode *node = (INode*)nrb.api_node_id;
		Modifier *mod = NULL;

		if(NULL != (mod = FindPhysiqueModifier(node)))
		{
			AppendPhysiqueNurbCV(&nrb, bl, mod, mesh_time);
		}else
		if(NULL != (mod = FindBonesModifier(node)))
		{
			Winprint("Error: AppendBonesProNurbCV() not yet implemented!\n");
			//AppendBonesProCV(obj, vid, bl, mod, &v_step, mesh_time);
		}
		else
		{
			AppendSelfCV(&nrb, bl, mesh_time);	
		}

		// TODO
		// drop CV's less than 10% of highest weight
		//CleanCV(nrb);
	}
}

void AsciiExp::ExportBoneBezCV(Bezier_mesh *b_mesh, const bone_lib *bl, const TimeValue mesh_time)
{
	b_mesh->b_v_list_ver = (bone_vertex*)Malloc(b_mesh->vertex_cnt * sizeof(bone_vertex));
	b_mesh->b_v_list_vec = (bone_vertex*)Malloc(b_mesh->vector_cnt * sizeof(bone_vertex));
	
	for(int api_node_id = 0; api_node_id < b_mesh->api_node_count; api_node_id++)
	{
		INode *node = (INode*)b_mesh->api_node_list[api_node_id];
		assert(node);

		Modifier *mod = NULL;

		if(NULL != (mod = FindPhysiqueModifier(node)))
		{
			AppendPhysiqueBezCV(b_mesh, api_node_id, bl, mod, mesh_time);
		}else
		if(NULL != (mod = FindBonesModifier(node)))
		{
			Winprint("Error: AppendBonesProBezCV() not yet implemented!\n");
		}
		else
		{
			Winprint("Error: AppendSelfBezCV() not yet implemented!\n");
		}
	}

	//AssignAuxCV(b_mesh, bl, mesh_time);

	// remove any assignments less than 10% of strongest assignment
	//TODO CleanBezCV(v);
}

void AsciiExp::ExportBoneVertices(DAMesh & da_mesh, bone_lib *bl, const TimeValue mesh_time)
{
	da_mesh.b_v_list = (bone_vertex*)Malloc(da_mesh.v_cnt * sizeof(bone_vertex));

	int vid = 0;
	while(vid < da_mesh.v_cnt)
	{
		INode *node = (INode*)da_mesh.v_list_api_node_id[vid];
		assert(node);
#ifdef _DEBUG
		const char *node_name = node->GetName();
#endif

		int v_step;
		Modifier *mod = NULL;

		if(NULL != (mod = FindPhysiqueModifier(node)))
		{
			AppendPhysiqueVertex(da_mesh, vid, bl, mod, &v_step, mesh_time);
		}else
		if(NULL != (mod = FindBonesModifier(node)))
		{
			AppendBonesProVertex(da_mesh, vid, bl, mod, &v_step, mesh_time);
		}
		else
		{
			AppendSelfVertex(da_mesh, vid, bl, &v_step, mesh_time);
		}

		vid += v_step;
	}

	assert(vid == da_mesh.v_cnt);

	// remove any assignments less than 10% of strongest assignment
	//TODO:
	//CleanBoneVertices(da_mesh);
}

void AsciiExp::ExportBoneVertices(object *obj, bone_lib *bl, const TimeValue mesh_time)
{
	vertices * v = &(obj->v);
	v->b_v_list = (bone_vertex*)Malloc(v->object_count * sizeof(bone_vertex));

	int vid = 0;
	while(vid < v->object_count)
	{
		INode *node = (INode*)v->api_node_xyz_id[vid];
		assert(node);

		int v_step;
		Modifier *mod = NULL;

		if(NULL != (mod = FindPhysiqueModifier(node)))
		{
			AppendPhysiqueVertex(obj, vid, bl, mod, &v_step, mesh_time);
		}else
		if(NULL != (mod = FindBonesModifier(node)))
		{
			AppendBonesProVertex(obj, vid, bl, mod, &v_step, mesh_time);
		}
		else
		{
			AppendSelfVertex(obj, vid, bl, &v_step, mesh_time);
		}

		vid += v_step;
	}

	assert(vid == v->object_count);

	// remove any assignments less than 10% of strongest assignment
	CleanVertices(v);
}

void AsciiExp::CleanVertices(vertices * v)
{
	for(int i = 0; i < v->object_count; i++)
	{
		bone_vertex *bv = v->b_v_list + i;

		float max_w = 0.0f;
		for(int j = 0; j < bv->bone_count; j++)
		{
			if(bv->weight_list[j] > max_w)
			{
				max_w = bv->weight_list[j];
			}
		}
		assert(max_w > 0.0f);

		bool renormalize = false;
		float sum_w = 0.0f;
		for(j = 0; j < bv->bone_count; j++)
		{
redo_label:
			if(bv->weight_list[j] < .10f * max_w)
			{
				renormalize = true;

				if(j != bv->bone_count - 1)
				{
					Swap32(bv->weight_list + j, bv->weight_list + bv->bone_count - 1);
					Swap32(bv->bone_id_list + j, bv->bone_id_list + bv->bone_count - 1);

					Swap32(bv->xyz_list + 3*j,     bv->xyz_list + 3*(bv->bone_count - 1));
					Swap32(bv->xyz_list + 3*j + 1, bv->xyz_list + 3*(bv->bone_count - 1)+1);
					Swap32(bv->xyz_list + 3*j + 2, bv->xyz_list + 3*(bv->bone_count - 1)+2);

					Swap32(bv->normal_list + 3*j,     bv->normal_list + 3*(bv->bone_count - 1));
					Swap32(bv->normal_list + 3*j + 1, bv->normal_list + 3*(bv->bone_count - 1)+1);
					Swap32(bv->normal_list + 3*j + 2, bv->normal_list + 3*(bv->bone_count - 1)+2);
				}

				bv->bone_count--;
				bv->weight_list = (float*)Realloc(bv->weight_list, bv->bone_count * sizeof(float));
				bv->bone_id_list = (int*)Realloc(bv->bone_id_list, bv->bone_count * sizeof(int));
				bv->xyz_list = (float*)Realloc(bv->xyz_list, 3 * bv->bone_count * sizeof(float));
				bv->normal_list = (float*)Realloc(bv->normal_list, 3 * bv->bone_count * sizeof(float));

				if( j < bv->bone_count) // make sure we don't skip the element just swapped in
				{
					goto redo_label;
				}
			}
			else
			{
				sum_w += bv->weight_list[j];
			}
		}
		assert(sum_w > 0.0f && sum_w < 1.00001f);

		if(renormalize)
		{
			sum_w = (float)(1.0 / sum_w);
			// renormalize
			for(j = 0; j < bv->bone_count; j++)
			{
				bv->weight_list[j] *= sum_w;
			}
		}
	}
}

void AsciiExp::AppendSelfCV(nurb * nrb, const bone_lib *bl, const TimeValue mesh_time)
{
	INode *node = (INode*)nrb->api_node_id;
	const char *node_name = node->GetName();
	const int bone_id = GetBoneID(bl, node);
	assert(bone_id >= 0);

	char progress_name[256] = {0};

	// process a series if vertices belonging to the same node
	for(int s = 0; s < nrb->s_point_count; s++)
	{
		for(int t = 0; t < nrb->t_point_count; t++)
		{
			const int i = s * nrb->t_point_count + t;

			if( (i & 0x7) == 0 )
			{
				const float percent = 100.0f * (float)(i) / (float)(nrb->s_point_count * nrb->t_point_count);
				sprintf(progress_name, "%d%% CV %d %s", (int)percent, i, node_name);
				ip->ProgressUpdate((int)percent, FALSE, progress_name); 
			}

			bone_vertex *bv = nrb->b_v_list + i;
			bv->Init();

			bv->bone_count = 1;
			bv->bone_id_list = (int*)Malloc(bv->bone_count * sizeof(int));
			bv->xyz_list = (float*)Malloc(bv->bone_count * 3 * sizeof(float));
			if(nrb->weight_list)
			{
				bv->rational_list = (float*)Malloc(bv->bone_count * sizeof(float));
			}
			bv->weight_list = (float*)Malloc(bv->bone_count * sizeof(float));

			bv->bone_id_list[0] = bone_id;
			bv->weight_list[0] = 1.0f;

			Point3 offset;
			offset.x = nrb->point_list[3*i];
			offset.y = nrb->point_list[3*i+1];
			offset.z = nrb->point_list[3*i+2];
			offset = SwitchCoord(offset, bl->bone_list[0].node, node, mesh_time);
			bv->xyz_list[0] = offset.x;
			bv->xyz_list[1] = offset.y;
			bv->xyz_list[2] = offset.z;

			if(bv->rational_list)
			{
				bv->rational_list[0] = nrb->weight_list[i];
			}
		}
	}
}

void AsciiExp::AppendSelfVertex(DAMesh & da_mesh, const int v_id, const bone_lib *bl, int *v_step,
								const TimeValue mesh_time)
{
	INode *node = (INode*)da_mesh.v_list_api_node_id;
	const char *node_name = node->GetName();
	const int bone_id = GetBoneID(bl, node);
	assert(bone_id >= 0);

	INode *to_node;
	if(bl->bone_list[bone_id].type != UV_BONE)
	{
		to_node = node;
	}
	else
	{
		to_node = bl->bone_list[bone_id].parent;
	}

	const char *to_node_name = to_node->GetName();
	const int to_bone_id = GetBoneID(bl, to_node);
	assert(to_bone_id >= 0);

	char progress_name[256] = {0};
	*v_step = 0;
	// process a series if vertices belonging to the same node
	for(int i = v_id; i < da_mesh.v_cnt; i++)	
	{
		// new series of vertices
		if(node != (INode*)da_mesh.v_list_api_id[i])
		{
			assert(i > v_id);
			return;
		}

		if( (i & 0x7) == 0 ) // don't update all the time
		{
			const float percent = 100.0f * (float)(i) / (float)da_mesh.v_cnt;
			sprintf(progress_name, "%d%% vertex %d %s", (int)percent, i, node_name);
			ip->ProgressUpdate((int)percent, FALSE, progress_name); 
		}

		bone_vertex *bv = da_mesh.b_v_list + i;
		bv->Init();

		bv->bone_count = 1;
		bv->bone_id_list = (int*)Malloc(bv->bone_count * sizeof(int));
		bv->weight_list = (float*)Malloc(bv->bone_count * sizeof(float));

		bv->bone_id_list[0] = to_bone_id;
		bv->weight_list[0] = 1.0f;

		(*v_step)++;
	}
}

void AsciiExp::AppendSelfVertex(object * obj, const int v_id, const bone_lib *bl, int *v_step,
								const TimeValue mesh_time)
{
	vertices * v = &(obj->v);
	INode *node = (INode*)v->api_node_xyz_id[v_id];
	const char *node_name = node->GetName();
	const int bone_id = GetBoneID(bl, node);
	assert(bone_id >= 0);

	INode *to_node;
	if(bl->bone_list[bone_id].type != UV_BONE)
	{
		to_node = node;
	}
	else
	{
		to_node = bl->bone_list[bone_id].parent;
	}

	const char *to_node_name = to_node->GetName();
	const int to_bone_id = GetBoneID(bl, to_node);
	assert(to_bone_id >= 0);

	char progress_name[256] = {0};
	*v_step = 0;
	// process a series if vertices belonging to the same node
	for(int i = v_id; i < v->object_count; i++)	
	{
		// new series of vertices
		if(node != (INode*)v->api_node_xyz_id[i])
		{
			assert(i > v_id);
			return;
		}

		if( (i & 0x7) == 0 ) // don't update all the time
		{
			const float percent = 100.0f * (float)(i) / (float)v->object_count;
			sprintf(progress_name, "%d%% vertex %d %s", (int)percent, i, node_name);
			ip->ProgressUpdate((int)percent, FALSE, progress_name); 
		}

		bone_vertex *bv = v->b_v_list + i;
		bv->Init();

		bv->bone_count = 1;
		bv->bone_id_list = (int*)Malloc(bv->bone_count * sizeof(int));
		bv->xyz_list = (float*)Malloc(bv->bone_count * 3 * sizeof(float));
		bv->normal_list = (float*)Malloc(bv->bone_count * 3 * sizeof(float));
		bv->weight_list = (float*)Malloc(bv->bone_count * sizeof(float));

		bv->bone_id_list[0] = to_bone_id;
		bv->weight_list[0] = 1.0f;

		Point3 offset;
		offset.x = v->object_list[3*i];
		offset.y = v->object_list[3*i+1];
		offset.z = v->object_list[3*i+2];
		offset = SwitchCoord(offset, bl->bone_list[0].node, to_node, mesh_time);
		bv->xyz_list[0] = offset.x;
		bv->xyz_list[1] = offset.y;
		bv->xyz_list[2] = offset.z;

		const int nid = v->normal[i];
		Point3 normal(obj->n.list[3*nid], obj->n.list[3*nid+1], obj->n.list[3*nid+2]); 
		Point3 new_normal ( SwitchNormal(normal, bl->bone_list[0].node, to_node, mesh_time));
		bv->normal_list[0] = new_normal.x;
		bv->normal_list[1] = new_normal.y;
		bv->normal_list[2] = new_normal.z; 

		(*v_step)++;
	}
}

void AsciiExp::AppendBonesProVertex(DAMesh & da_mesh, const int v_id, bone_lib *bl, Modifier *mod, int *v_step, const TimeValue mesh_time)
{
//	INode *node = (INode*)da_mesh.v_list_api_node_id[v_id];
//	const char *node_name = node->GetName();
//
//	const int bone_count = mod->SetProperty ( BP_PROPID_GET_N_BONES, NULL );
//	const int vertex_count = mod->SetProperty ( BP_PROPID_GET_N_VERTS, NULL );
//
//	// read weights
//	BonesPro_WeightArray* wa = NULL;
//    mod->SetProperty ( BP_PROPID_GET_WEIGHTS, &wa );
//	assert(wa);
//
//	// read bone properties
//	BonesPro_Bone  *bone_list = (BonesPro_Bone*)Malloc(bone_count * sizeof(BonesPro_Bone));
//	int *my_bone_id = (int*)Malloc(bone_count * sizeof(int));
//	for(int i = 0; i < bone_count; i++)
//	{
//		bone_list[i].t = BP_TIME_ATTACHED;
//		bone_list[i].index = i;
//		mod->SetProperty ( BP_PROPID_GET_BONE, &(bone_list[i]) );
//
//		my_bone_id[i] = GetBoneID(bl, bone_list[i].node);
//	}
//
//	char progress_name[256] = {0};
//
//	*v_step = 0;
//	// process a series if vertices belonging to the same node
//	for(i = v_id; i < da_mesh.v_cnt; i++)	
//	{
//		// new series of vertices
//		if(node != (INode*)da_mesh.v_list_api_node_id[i])
//		{
//			Free(bone_list);
//			Free(my_bone_id);
//			assert(i > v_id);
//			return;
//		}
//
//		if( (i & 0x7) == 0 )
//		{
//			const float percent = 100.0f * (float)(i) / (float)da_mesh.v_cnt;
//			sprintf(progress_name, "%d%% vertex %d %s", (int)percent, i, node_name);
//			ip->ProgressUpdate((int)percent, FALSE, progress_name); 
//		}
//
//		bone_vertex *bv = da_mesh.b_v_list + i;
//		bv->Init();
//
//		const int api_vid = da_mesh.v_list_api_id[i];
//		assert(api_vid < vertex_count);
//
//		float weight_sum = 0.0f;
//		for(int bid=0; bid < bone_count; bid++)
//		{
//			if(wa->w[api_vid * bone_count + bid] > 0.0f)
//			{
//				assert(my_bone_id[bid] >= 0);
//				INode *bone_node = bone_list[bid].node;
//				const char *bone_name = bone_node->GetName();
//
//				bv->bone_count++;
//				bv->bone_id_list = (int*)Realloc(bv->bone_id_list, bv->bone_count * sizeof(int));
//				bv->weight_list = (float*)Realloc(bv->weight_list, bv->bone_count * sizeof(float));
//
//				bv->bone_id_list[bv->bone_count - 1] = my_bone_id[bid];
//				bv->weight_list[bv->bone_count - 1] = wa->w[api_vid * bone_count + bid];
//
//				weight_sum += wa->w[api_vid * bone_count + bid];
//			}
//			else if(wa->w[api_vid * bone_count + bid] < 0.0f)
//			{
//				Winprint("Error: bone weight of bone %s on vertex %d of %s is %.4f!\n",
//				bone_list[bid].node->GetName(), i, node->GetName(), wa->w[api_vid * bone_count + bid]);
//			}
//		}
//
//		if( fabs(1.0f - weight_sum) > .0001f )
//		{
//			Winprint("Error: bone weights for vertex %d of %s sum up to %.4f!\n",
//				i, node_name, weight_sum);
//		}
//
//		(*v_step)++;
//	}
//
//	Free(bone_list);
//	Free(my_bone_id);
}

void AsciiExp::AppendBonesProVertex(object * obj, const int v_id, bone_lib *bl, Modifier *mod, int *v_step, const TimeValue mesh_time)
{
//	vertices * v = &(obj->v);
//	INode *node = (INode*)v->api_node_xyz_id[v_id];
//	const char *node_name = node->GetName();
//
//	const int bone_count = mod->SetProperty ( BP_PROPID_GET_N_BONES, NULL );
//	const int vertex_count = mod->SetProperty ( BP_PROPID_GET_N_VERTS, NULL );
//
//	// read weights
//	BonesPro_WeightArray* wa = NULL;
//    mod->SetProperty ( BP_PROPID_GET_WEIGHTS, &wa );
//	assert(wa);
//
//	// read bone properties
//	BonesPro_Bone  *bone_list = (BonesPro_Bone*)Malloc(bone_count * sizeof(BonesPro_Bone));
//	int *my_bone_id = (int*)Malloc(bone_count * sizeof(int));
//	for(int i = 0; i < bone_count; i++)
//	{
//		bone_list[i].t = BP_TIME_ATTACHED;
//		bone_list[i].index = i;
//		mod->SetProperty ( BP_PROPID_GET_BONE, &(bone_list[i]) );
//
//		my_bone_id[i] = GetBoneID(bl, bone_list[i].node);
//	}
//
//	char progress_name[256] = {0};
//
//	*v_step = 0;
//	// process a series if vertices belonging to the same node
//	for(i = v_id; i < v->object_count; i++)	
//	{
//		// new series of vertices
//		if(node != (INode*)v->api_node_xyz_id[i])
//		{
//			Free(bone_list);
//			Free(my_bone_id);
//			assert(i > v_id);
//			return;
//		}
//
//		if( (i & 0x7) == 0 )
//		{
//			const float percent = 100.0f * (float)(i) / (float)v->object_count;
//			sprintf(progress_name, "%d%% vertex %d %s", (int)percent, i, node_name);
//			ip->ProgressUpdate((int)percent, FALSE, progress_name); 
//		}
//
//		bone_vertex *bv = v->b_v_list + i;
//		bv->Init();
//
//		const int api_vid = v->api_xyz_id[i];
//		assert(api_vid < vertex_count);
//
//		const int nid = v->normal[i];
//		Point3 normal(obj->n.list[3*nid], obj->n.list[3*nid+1], obj->n.list[3*nid+2]); 
//
//		float weight_sum = 0.0f;
//		for(int bid=0; bid < bone_count; bid++)
//		{
//			if(wa->w[api_vid * bone_count + bid] > 0.0f)
//			{
//				assert(my_bone_id[bid] >= 0);
//				INode *bone_node = bone_list[bid].node;
//				const char *bone_name = bone_node->GetName();
//
//				bv->bone_count++;
//				bv->bone_id_list = (int*)Realloc(bv->bone_id_list, bv->bone_count * sizeof(int));
//				bv->xyz_list = (float*)Realloc(bv->xyz_list, bv->bone_count * 3 * sizeof(float));
//				bv->normal_list = (float*)Realloc(bv->normal_list, bv->bone_count * 3 * sizeof(float));
//				bv->weight_list = (float*)Realloc(bv->weight_list, bv->bone_count * sizeof(float));
//
//
//				bv->bone_id_list[bv->bone_count - 1] = my_bone_id[bid];
//				bv->weight_list[bv->bone_count - 1] = wa->w[api_vid * bone_count + bid];
//
//				Point3 offset;
//				offset.x = v->object_list[3*i];
//				offset.y = v->object_list[3*i+1];
//				offset.z = v->object_list[3*i+2];
//				offset = SwitchCoord(offset, bl->bone_list[0].node, bone_node, mesh_time);
//
//				bv->xyz_list[3*(bv->bone_count - 1)  ] = offset.x;
//				bv->xyz_list[3*(bv->bone_count - 1)+1] = offset.y;
//				bv->xyz_list[3*(bv->bone_count - 1)+2] = offset.z;
//
//				Point3 new_normal ( SwitchNormal(normal, bl->bone_list[0].node, bone_node, mesh_time));
//
//				bv->normal_list[3*(bv->bone_count - 1)  ] = new_normal.x;
//				bv->normal_list[3*(bv->bone_count - 1)+1] = new_normal.y;
//				bv->normal_list[3*(bv->bone_count - 1)+2] = new_normal.z;
//
//				weight_sum += wa->w[api_vid * bone_count + bid];
//			}
//			else if(wa->w[api_vid * bone_count + bid] < 0.0f)
//			{
//				Winprint("Error: bone weight of bone %s on vertex %d of %s is %.4f!\n",
//				bone_list[bid].node->GetName(), i, node->GetName(), wa->w[api_vid * bone_count + bid]);
//			}
//		
//		}
//
//		if( fabs(1.0f - weight_sum) > .0001f )
//		{
//			Winprint("Error: bone weights for vertex %d of %s sum up to %.4f!\n",
//				i, node_name, weight_sum);
//		}
//
//		(*v_step)++;
//	}
//
//	Free(bone_list);
//	Free(my_bone_id);
}

// assumes the order of assignment is vertices first then vectors
//NOTE: aux vertices of tri patches are not assigned by physique
void AsciiExp::AppendPhysiqueBezCV(Bezier_mesh * b_mesh, const int api_node_id, const bone_lib *bl,
								   Modifier *mod, const TimeValue mesh_time)
{
	INode *node = (INode*)(b_mesh->api_node_list[api_node_id]);
	const char *node_name = node->GetName();

	// create a Physique Export Interface for the given Physique Modifier
	IPhysiqueExport *phyExport = (IPhysiqueExport*)mod->GetInterface(I_PHYINTERFACE);
	assert(phyExport);
	
	// create a ModContext Export Interface for the specific node of the Physique Modifier
	IPhyContextExport *mcExport = (IPhyContextExport*)phyExport->GetContextInterface(node);
	assert(mcExport);

	// default 
	mcExport->AllowBlending(TRUE);

	// convert all vertices to Rigid (or blended ver 2.0)
	mcExport->ConvertToRigid(TRUE);

	const int def_v_cnt = mcExport->GetNumberVertices();

	int ver_cnt;
	int vec_cnt;

	if(api_node_id < b_mesh->api_node_count - 1)
	{
		ver_cnt = b_mesh->api_node_ver_offset[api_node_id+1] - b_mesh->api_node_ver_offset[api_node_id];
		vec_cnt = b_mesh->api_node_vec_offset[api_node_id+1] - b_mesh->api_node_vec_offset[api_node_id];
	}
	else // last node
	{
		ver_cnt = b_mesh->vertex_cnt - b_mesh->api_node_ver_offset[api_node_id];
		vec_cnt = b_mesh->vector_cnt - b_mesh->api_node_vec_offset[api_node_id];
	}

	assert(def_v_cnt == ver_cnt + vec_cnt);

	char progress_name[256] = {0};

	const int ver_offset = b_mesh->api_node_ver_offset[api_node_id];
	const int vec_offset = b_mesh->api_node_vec_offset[api_node_id];

	// process a series if vertices belonging to the same node
	for(int i = 0; i < def_v_cnt; i++)	
	{
		if( (i & 0x7) == 0 )
		{
			const float percent = 100.0f * (float)(i) / (float)def_v_cnt;
			sprintf(progress_name, "%d%% CV %d %s", (int)percent, i, node_name);
			ip->ProgressUpdate((int)percent, FALSE, progress_name); 
		}

		const Vector *point;
		bone_vertex *bv;
		
		if(i < ver_cnt)
		{
			bv = b_mesh->b_v_list_ver + ver_offset + i;
			point = b_mesh->vertices + ver_offset + i;
		}
		else
		{
			assert( i < ver_cnt + vec_cnt );
			bv = b_mesh->b_v_list_vec + vec_offset + i - ver_cnt;
			point = b_mesh->vectors + vec_offset + i - ver_cnt;
		}
	
		bv->Init();

		const int api_vid = i; // no need to remap

		IPhyVertexExport *vtxExport = mcExport->GetVertexInterface(api_vid);
		if(!vtxExport)
		{	
			Winprint("Error: unassigned vertex %d of %s!\n", api_vid, node->GetName());
		}
		else
		{
			const int type = vtxExport->GetVertexType();

			switch(type)
			{
				//case RIGID_NON_BLENDED_TYPE:
				case RIGID_TYPE:
				{
					IPhyRigidVertex *r_vtxExport = (IPhyRigidVertex*)vtxExport;
				
					INode *bone_node = r_vtxExport->GetNode();
					assert(bone_node);
					const char *bone_name = bone_node->GetName();

					int bone_id = GetBoneID(bl, bone_node);
					if(bone_id < 0)
					{
						Winprint("Error: %s is not a bone ??!\n", bone_name);
						return;
					}

					bv->bone_count = 1;
					bv->bone_id_list = (int*)Malloc(bv->bone_count * sizeof(int));
					bv->xyz_list = (float*)Malloc(bv->bone_count * 3 * sizeof(float));
					bv->weight_list = (float*)Malloc(bv->bone_count * sizeof(float));

					bv->bone_id_list[0] = bone_id;
					bv->weight_list[0] = 1.0f;
#if 1
					// get vertex in bone's coord
					Matrix3 m ( bone_node->GetNodeTM(mesh_time) );
					Matrix3 mns ( GetMyNodeTM(bone_node, mesh_time) );
					CleanMatrix3(mns);
					m *= Inverse(mns);

					Point3 offset ( m * r_vtxExport->GetOffsetVector() );
#else
					Point3 offset ( point.x,
									point.y,
									point.z);
					offset = SwitchCoord(offset, bl->bone_list[0].node, bone_node, mesh_time);
#endif
					bv->xyz_list[0] = offset.x;
					bv->xyz_list[1] = offset.y;
					bv->xyz_list[2] = offset.z;
				}
				break;

				case RIGID_BLENDED_TYPE: // (RIGID_TYPE | BLENDED_TYPE) // what we want!!
				{
					IPhyBlendedRigidVertex *b_vtxExport = (IPhyBlendedRigidVertex*)vtxExport;
				
					bv->bone_count = b_vtxExport->GetNumberNodes();
					bv->bone_id_list = (int*)Malloc(bv->bone_count * sizeof(int));
					bv->xyz_list = (float*)Malloc(bv->bone_count * 3 * sizeof(float));
					bv->weight_list = (float*)Malloc(bv->bone_count * sizeof(float));

					float sum_weight = 0.0f;
					for(int j = 0; j < bv->bone_count; j++)
					{
						INode *bone_node = b_vtxExport->GetNode(j);
						assert(bone_node);
						const char *bone_name = bone_node->GetName();

						int bone_id = GetBoneID(bl, bone_node);
						if(bone_id < 0)
						{
							Winprint("Error: %s is not a bone ??!\n", bone_name);
							return;
						}

						bv->bone_id_list[j] = bone_id;
#if 1
						// get vertex in bone's coord
						Matrix3 m ( bone_node->GetNodeTM(mesh_time) );
						Matrix3 mns ( GetMyNodeTM(bone_node, mesh_time) );
						CleanMatrix3(mns);
						m *= Inverse(mns);

						Point3 offset ( m * b_vtxExport->GetOffsetVector(j) );
#else
						Point3 offset ( point.x,
										point.y,
										point.z);
						offset = SwitchCoord(offset, bl->bone_list[0].node, bone_node, mesh_time);
#endif
						bv->xyz_list[3*j+0] = offset.x;
						bv->xyz_list[3*j+1] = offset.y;
						bv->xyz_list[3*j+2] = offset.z;

						bv->weight_list[j] = b_vtxExport->GetWeight(j);

						sum_weight += bv->weight_list[j];
					}

					if( fabs(1.0f - sum_weight) > .0001f )
					{
						Winprint("Error: bone weights for vertex %d of %s sum up to %.4f!\n",
							i, node->GetName(), sum_weight);
					}
				}
				break;

				//case DEFORMABLE_NON_BLENDED_TYPE:
				case BLENDED_TYPE:
					Winprint("Error: Got a Blended vertex on %s!\n", node->GetName());
					break;

				case DEFORMABLE_BLENDED_TYPE: // (DEFORMABLE_TYPE | BLENDED_TYPE)
				case DEFORMABLE_TYPE:
					Winprint("Error: Got a Deformable vertex on %s!\n", node->GetName());
					break;

				default:
					Winprint("Error: Unknown Physique vertex type %d on %s!n", type, node->GetName());
					exit(1);
			}

			mcExport->ReleaseVertexInterface(vtxExport);
			vtxExport = NULL;
		}
	}

	phyExport->ReleaseContextInterface(mcExport);
	mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
}

/*
void AsciiExp::AssignAuxCV(Bezier_mesh *b_mesh, const bone_lib *bl, const TimeValue mesh_time)
{
	if(b_mesh->aux_cnt > 0)
	{
		b_mesh->b_v_list_aux = (bone_vertex*)Malloc(b_mesh->aux_cnt * sizeof(bone_vertex));

		int aux_cnt = 0;
		for(int gid = 0; gid < b_mesh->group_cnt; gid++)
		{
			const Bezier_patch_group *group = b_mesh->groups + gid;
			for(int pid = 0; pid < group->patch_cnt; pid++)
			{
				const Bezier_patch *patch = group->patch_list + pid;

				if(patch->type == 3)
				{
					int aux_id;
					const bone_vertex *bv1;
					const bone_vertex *bv2;
					bone_vertex * bv_dst;

					for(int i = 0; i < 3; i++)
					{
						// 1
						aux_id = patch->aux_index + 3 * i;
						assert(aux_id == aux_cnt);
						assert(aux_id < b_mesh->aux_cnt);

						bv1 = &(b_mesh->b_v_list_ver[ patch->v[i] ]);
						bv2 = &(b_mesh->b_v_list_vec[ patch->vec[2 * i + 0] ]);
						bv_dst = b_mesh->b_v_list_aux + aux_id;

						RecomputeBVPos(bl, b_mesh, bv_dst, bv1, bv2, .25, .75, aux_id, mesh_time);
						aux_cnt++;

						// 2
						aux_id = patch->aux_index + 3 * i + 1;
						assert(aux_id == aux_cnt);
						assert(aux_id < b_mesh->aux_cnt);

						bv1 = &(b_mesh->b_v_list_vec[ patch->vec[2 * i + 0] ]);
						bv2 = &(b_mesh->b_v_list_vec[ patch->vec[2 * i + 1] ]);
						bv_dst = b_mesh->b_v_list_aux + aux_id;

						RecomputeBVPos(bl, b_mesh, bv_dst, bv1, bv2, .5, .5, aux_id, mesh_time);
						aux_cnt++;	

						// 3
						aux_id = patch->aux_index + 3 * i + 2;
						assert(aux_id == aux_cnt);
						assert(aux_id < b_mesh->aux_cnt);

						bv1 = &(b_mesh->b_v_list_ver[ patch->v[(i + 1) % 3] ]);
						bv2 = &(b_mesh->b_v_list_vec[ patch->vec[2 * i + 1] ]);
						bv_dst = b_mesh->b_v_list_aux + aux_id;

						RecomputeBVPos(bl, b_mesh, bv_dst, bv1, bv2, .25, .75, aux_id, mesh_time);
						aux_cnt++;	
					}
				}
			}
		}
		assert(aux_cnt == b_mesh->aux_cnt);
	}
}

void AsciiExp::RecomputeBVPos(const bone_lib *bl, Bezier_mesh *b_mesh,
							  bone_vertex *bv_dst, const bone_vertex *bv1, const bone_vertex *bv2,
							  const float w1, const float w2, int aux_id, const TimeValue mesh_time)
{
	InitBoneVertex( bv_dst );

	MergeBoneVertex(*bv_dst, *bv1, *bv2, w1, w2); // normalizes weights of dst

	const Point3 pos ( b_mesh->aux[aux_id].x,
					   b_mesh->aux[aux_id].y,
					   b_mesh->aux[aux_id].z);

	for(int j = 0; j < bv_dst->bone_count; j++)
	{
		INode *bone_node = (INode*)bl->bone_list[ bv_dst->bone_id_list[j] ].node;

		Point3 new_pos ( SwitchCoord(pos, bl->bone_list[0].node, bone_node, mesh_time) );

		bv_dst->xyz_list[3*j  ] = new_pos.x;
		bv_dst->xyz_list[3*j+1] = new_pos.y;
		bv_dst->xyz_list[3*j+2] = new_pos.z;
	}
}
*/

void AsciiExp::AppendPhysiqueNurbCV(nurb * nrb, const bone_lib *bl, Modifier *mod, const TimeValue mesh_time)
{
	//FILE *fp = fopen("c:\\export\\n.txt", "a");

	INode *node = (INode*)(nrb->api_node_id);
	const char *node_name = node->GetName();

	// create a Physique Export Interface for the given Physique Modifier
	IPhysiqueExport *phyExport = (IPhysiqueExport*)mod->GetInterface(I_PHYINTERFACE);
	assert(phyExport);
	
	// create a ModContext Export Interface for the specific node of the Physique Modifier
	IPhyContextExport *mcExport = (IPhyContextExport*)phyExport->GetContextInterface(node);
	assert(mcExport);

	// default 
	mcExport->AllowBlending(TRUE);

	// convert all vertices to Rigid ( or blended ver 2.0)
	mcExport->ConvertToRigid(TRUE);

	const int def_v_cnt = mcExport->GetNumberVertices();
//assert(def_v_cnt == nrb->api_node_cv_total);
/*
assert(def_v_cnt >= nrb->api_node_cv_total);
if(def_v_cnt != nrb->api_node_cv_total)
{
	fprintf(fp, "%s %s n=%d p=%d\n", node_name, nrb->name, nrb->api_node_cv_total, def_v_cnt);
}
*/
// NOTE: the assert above could fail because of refinement:
// or other CV's (non surface ones like curves) being assigned
// or if the conversion to CV happended after the assignment to bones
/*
The only way that I could force this to happen was to create a nurbs
object, apply physique, link to bones and then refine the Nurbs object
by adding more rows or columns and *not* reinitialize the vertex to
bone assignments. 

Are you able to create this situation without the nurbs object
refinement step?
*/ 
	
	char progress_name[256] = {0};
	
	// process a series if vertices belonging to the same node	
	for(int s = 0; s < nrb->s_point_count; s++)
	{
		//fprintf(fp, "s%1d ", s);
		//int ss = (nrb->s_point_count - 1) - s;
		for(int t = 0; t < nrb->t_point_count; t++)
		{
			const int i = s * nrb->t_point_count + t;
			//const int ii = (i % nrb->t_point_count) * nrb->s_point_count + i/nrb->t_point_count;
		
			assert(i + nrb->api_st_offset < def_v_cnt);

			if( (i & 0x7) == 0 )
			{
				const float percent = 100.0f * (float)(i) / (float)(nrb->s_point_count * nrb->t_point_count);
				sprintf(progress_name, "%d%% CV %d %s", (int)percent, i, node_name);
				ip->ProgressUpdate((int)percent, FALSE, progress_name); 
			}

			bone_vertex *bv = nrb->b_v_list + i;
			bv->Init();

			// this lookup is only in case normals were flipped
			// otherwise api_vid would just be i
			int api_vid = nrb->api_s_id[i] * nrb->t_point_count + nrb->api_t_id[i];

			if(nrb->api_st_offset)
			{
				api_vid += nrb->api_st_offset;
				api_vid = (api_vid % (nrb->t_point_count + 4)) * (nrb->s_point_count+4) +
					api_vid/(nrb->t_point_count+4);
			}
			else
			{
				api_vid = nrb->api_st_offset + (api_vid % nrb->t_point_count) * nrb->s_point_count + api_vid/nrb->t_point_count;
			}
			assert(api_vid < def_v_cnt);

 
			IPhyVertexExport *vtxExport = mcExport->GetVertexInterface( api_vid );
			if(!vtxExport)
			{	
				Winprint("Error: unassigned vertex %d of %s!\n", api_vid, node_name);
			}
			else
			{
				const int type = vtxExport->GetVertexType();

				switch(type)
				{
					//case RIGID_NON_BLENDED_TYPE:
					case RIGID_TYPE:
					{
						IPhyRigidVertex *r_vtxExport = (IPhyRigidVertex*)vtxExport;
					
						INode *bone_node = r_vtxExport->GetNode();
						assert(bone_node);
						const char *bone_name = bone_node->GetName();

						int bone_id = GetBoneID(bl, bone_node);
						if(bone_id < 0)
						{
							Winprint("Error: %s is not a bone ??!\n", bone_name);
							return;
						}

						bv->bone_count = 1;
						bv->bone_id_list = (int*)Malloc(bv->bone_count * sizeof(int));
						bv->xyz_list = (float*)Malloc(bv->bone_count * 3 * sizeof(float));
						//bv->normal_list = (float*)Malloc(bv->bone_count * 3 * sizeof(float));
						bv->weight_list = (float*)Malloc(bv->bone_count * sizeof(float));
						if(nrb->weight_list)
						{
							bv->rational_list = (float*)Malloc(bv->bone_count * sizeof(float));
						}

						bv->bone_id_list[0] = bone_id;
						bv->weight_list[0] = 1.0f;

		//fprintf(fp, "t%1di%1d ",t, i);

#if 0
						Point3 offset;
						offset.x = nrb->point_list[3*i];
						offset.y =	nrb->point_list[3*i+1];
						offset.z =	nrb->point_list[3*i+2];

						offset = SwitchCoord(offset, bl->bone_list[0].node, bone_node, mesh_time);
		//fprintf(fp, "%.2f %.2f %.2f; ", offset.x, offset.y, offset.z);
		//fprintf(fp, "%.0f|", Length(offset));
#else
						// get vertex in bone's coord
						//Matrix3 m ( bone_node->GetObjTMAfterWSM(mesh_time) );
						Matrix3 m ( bone_node->GetNodeTM(mesh_time) );
						Matrix3 mns ( GetMyNodeTM(bone_node, mesh_time) );
						CleanMatrix3(mns); //mns.NoScale(); NoShear(mns);
						m *= Inverse(mns);

						//Point3 offset ( m * r_vtxExport->GetDeformOffsetVector(mesh_time) );
						Point3 offset ( m * r_vtxExport->GetOffsetVector() );
						//offset = m * r_vtxExport->GetOffsetVector();

		//fprintf(fp, "%.2f %.2f %.2f/", offset.x, offset.y, offset.z);
		//fprintf(fp, "%.0f ", Length(offset));
#endif

						bv->xyz_list[0] = offset.x;
						bv->xyz_list[1] = offset.y;
						bv->xyz_list[2] = offset.z;

						if(bv->rational_list)
						{
							bv->rational_list[0] = nrb->weight_list[i];
						}
						
						/*
						const int nid = v->normal[i];
						Point3 normal(obj->n.list[3*nid], obj->n.list[3*nid+1], obj->n.list[3*nid+2]); 
						Point3 new_normal ( SwitchNormal(normal, bl->bone_list[0].node, bone_node, mesh_time));

						bv->normal_list[0] = new_normal.x;
						bv->normal_list[1] = new_normal.y;
						bv->normal_list[2] = new_normal.z;
						*/
					}
					break;

					case RIGID_BLENDED_TYPE: // (RIGID_TYPE | BLENDED_TYPE) // what we want!!
					{
						IPhyBlendedRigidVertex *b_vtxExport = (IPhyBlendedRigidVertex*)vtxExport;
					
						bv->bone_count = b_vtxExport->GetNumberNodes();
						bv->bone_id_list = (int*)Malloc(bv->bone_count * sizeof(int));
						bv->xyz_list = (float*)Malloc(bv->bone_count * 3 * sizeof(float));
						//bv->normal_list = (float*)Malloc(bv->bone_count * 3 * sizeof(float));
						bv->weight_list = (float*)Malloc(bv->bone_count * sizeof(float));
						if(nrb->weight_list)
						{
							bv->rational_list = (float*)Malloc(bv->bone_count * sizeof(float));
						}

						//const int nid = v->normal[i];
						//Point3 normal(obj->n.list[3*nid], obj->n.list[3*nid+1], obj->n.list[3*nid+2]); 

						float sum_weight = 0.0f;
						for(int j = 0; j < bv->bone_count; j++)
						{
							INode *bone_node = b_vtxExport->GetNode(j);

							assert(bone_node);
							const char *bone_name = bone_node->GetName();

							int bone_id = GetBoneID(bl, bone_node);
							if(bone_id < 0)
							{
								Winprint("Error: %s is not a bone ??!\n", bone_name);
								return;
							}

							bv->bone_id_list[j] = bone_id;
#if 1
							// get vertex in bone's coord
							//Matrix3 m ( bone_node->GetObjTMAfterWSM(mesh_time) );
							Matrix3 m ( bone_node->GetNodeTM(mesh_time) );
							Matrix3 mns ( GetMyNodeTM(bone_node, mesh_time) );
							CleanMatrix3(mns); //mns.NoScale(); NoShear(mns);
							m *= Inverse(mns);

							//Point3 offset ( m * b_vtxExport->GetDeformOffsetVector(mesh_time, j) );
							Point3 offset ( m * b_vtxExport->GetOffsetVector(j) );
#else
							Point3 offset ( nrb->point_list[3*i],
											nrb->point_list[3*i+1],
											nrb->point_list[3*i+2]);

							offset = SwitchCoord(offset, bl->bone_list[0].node, bone_node, mesh_time);
#endif
							bv->xyz_list[3*j+0] = offset.x;
							bv->xyz_list[3*j+1] = offset.y;
							bv->xyz_list[3*j+2] = offset.z;

							if(bv->rational_list)
							{
								bv->rational_list[j] = nrb->weight_list[i];
							}

							/*
							Point3 new_normal ( SwitchNormal(normal, bl->bone_list[0].node,
												bone_node, mesh_time));

							bv->normal_list[3*j+0] = new_normal.x;
							bv->normal_list[3*j+1] = new_normal.y;
							bv->normal_list[3*j+2] = new_normal.z;
							*/

							bv->weight_list[j] = b_vtxExport->GetWeight(j);

							sum_weight += bv->weight_list[j];
						}

						if( fabs(1.0f - sum_weight) > .0001f )
						{
							Winprint("Error: bone weights for vertex %d of %s sum up to %.4f!\n",
								i, node->GetName(), sum_weight);
						}
					}
					break;

					//case DEFORMABLE_NON_BLENDED_TYPE:
					case BLENDED_TYPE:
						Winprint("Error: Got a Blended vertex on %s!\n", node->GetName());
						break;

					case DEFORMABLE_BLENDED_TYPE: // (DEFORMABLE_TYPE | BLENDED_TYPE)
					case DEFORMABLE_TYPE:
						Winprint("Error: Got a Deformable vertex on %s!\n", node->GetName());
						break;

					default:
						Winprint("Error: Unknown Physique vertex type %d on %s!n", type, node->GetName());
						exit(1);
				}

				mcExport->ReleaseVertexInterface(vtxExport);
				vtxExport = NULL;
			}
		}
		//fprintf(fp, "\n");
	}

	phyExport->ReleaseContextInterface(mcExport);
	mod->ReleaseInterface(I_PHYINTERFACE, phyExport);

	//fflush(fp);
	//fclose(fp);
}

void AsciiExp::AppendPhysiqueVertex(DAMesh & da_mesh, const int v_id, const bone_lib *bl, Modifier *mod, int *v_step,
									const TimeValue mesh_time)
{
	INode *node = (INode*)da_mesh.v_list_api_node_id[v_id];
	const char *node_name = node->GetName();

	// create a Physique Export Interface for the given Physique Modifier
	IPhysiqueExport *phyExport = (IPhysiqueExport*)mod->GetInterface(I_PHYINTERFACE);
	assert(phyExport);
	
	// create a ModContext Export Interface for the specific node of the Physique Modifier
	IPhyContextExport *mcExport = (IPhyContextExport*)phyExport->GetContextInterface(node);
	assert(mcExport);

	// default 
	mcExport->AllowBlending(TRUE);

	// convert all vertices to Rigid ( or blended ver 2.0)
	mcExport->ConvertToRigid(TRUE);

	const int def_v_cnt = mcExport->GetNumberVertices();
	
	char progress_name[256] = {0};
	
	*v_step = 0;
	// process a series if vertices belonging to the same node
	for(int i = v_id; i < da_mesh.v_cnt; i++)	
	{
		// new series of vertices
		if(node != (INode*)da_mesh.v_list_api_node_id[i])
		{
			phyExport->ReleaseContextInterface(mcExport);
			mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
			assert(i > v_id);
			return;
		}

		if( (i & 0x7) == 0 )
		{
			const float percent = 100.0f * (float)(i) / (float)da_mesh.v_cnt;
			sprintf(progress_name, "%d%% vertex %d %s", (int)percent, i, node_name);
			ip->ProgressUpdate((int)percent, FALSE, progress_name); 
		}

		bone_vertex *bv = da_mesh.b_v_list + i;
		bv->Init();

		const int api_vid = da_mesh.v_list_api_id[i];
		assert(api_vid < def_v_cnt);

		IPhyVertexExport *vtxExport = mcExport->GetVertexInterface(api_vid);
		if(!vtxExport)
		{	
			Winprint("Error: unassigned vertex %d of %s!\n", api_vid, node->GetName());
		}
		else
		{
			const int type = vtxExport->GetVertexType();

			switch(type)
			{
				//case RIGID_NON_BLENDED_TYPE:
				case RIGID_TYPE:
				{
					IPhyRigidVertex *r_vtxExport = (IPhyRigidVertex*)vtxExport;
				
					INode *bone_node = r_vtxExport->GetNode();
					assert(bone_node);
					const char *bone_name = bone_node->GetName();

					int bone_id = GetBoneID(bl, bone_node);
					if(bone_id < 0)
					{
						Winprint("Error: %s is not a bone ??!\n", bone_name);
						return;
					}

					bv->bone_count = 1;
					bv->bone_id_list = (int*)Malloc(bv->bone_count * sizeof(int));
					bv->weight_list = (float*)Malloc(bv->bone_count * sizeof(float));

					bv->bone_id_list[0] = bone_id;
					bv->weight_list[0] = 1.0f;
				}
				break;

				case RIGID_BLENDED_TYPE: // (RIGID_TYPE | BLENDED_TYPE) // what we want!!
				{
					IPhyBlendedRigidVertex *b_vtxExport = (IPhyBlendedRigidVertex*)vtxExport;
				
					bv->bone_count = b_vtxExport->GetNumberNodes();
					bv->bone_id_list = (int*)Malloc(bv->bone_count * sizeof(int));
					bv->weight_list = (float*)Malloc(bv->bone_count * sizeof(float));

					
					

					float sum_weight = 0.0f;
					for(int j = 0; j < bv->bone_count; j++)
					{
						INode *bone_node = b_vtxExport->GetNode(j);
						assert(bone_node);
						const char *bone_name = bone_node->GetName();

						int bone_id = GetBoneID(bl, bone_node);
						if(bone_id < 0)
						{
							Winprint("Error: %s is not a bone ??!\n", bone_name);
							return;
						}

						bv->bone_id_list[j] = bone_id;

						bv->weight_list[j] = b_vtxExport->GetWeight(j);

						sum_weight += bv->weight_list[j];
					}

					if( fabs(1.0f - sum_weight) > .0001f )
					{
						Winprint("Error: bone weights for vertex %d of %s sum up to %.4f!\n",
							i, node->GetName(), sum_weight);
					}
				}
				break;

				//case DEFORMABLE_NON_BLENDED_TYPE:
				case BLENDED_TYPE:
					Winprint("Error: Got a Blended vertex on %s!\n", node->GetName());
					break;

				case DEFORMABLE_BLENDED_TYPE: // (DEFORMABLE_TYPE | BLENDED_TYPE)
				case DEFORMABLE_TYPE:
					Winprint("Error: Got a Deformable vertex on %s!\n", node->GetName());
					break;

				default:
					Winprint("Error: Unknown Physique vertex type %d on %s!n", type, node->GetName());
					exit(1);
			}

			mcExport->ReleaseVertexInterface(vtxExport);
			vtxExport = NULL;
		}
	
		(*v_step)++;
	}

	phyExport->ReleaseContextInterface(mcExport);
	mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
}

void AsciiExp::AppendPhysiqueVertex(object * obj, const int v_id, const bone_lib *bl, Modifier *mod, int *v_step,
									const TimeValue mesh_time)
{
	vertices * v = &(obj->v);
	INode *node = (INode*)v->api_node_xyz_id[v_id];
	const char *node_name = node->GetName();

	// create a Physique Export Interface for the given Physique Modifier
	IPhysiqueExport *phyExport = (IPhysiqueExport*)mod->GetInterface(I_PHYINTERFACE);
	assert(phyExport);
	
	// create a ModContext Export Interface for the specific node of the Physique Modifier
	IPhyContextExport *mcExport = (IPhyContextExport*)phyExport->GetContextInterface(node);
	assert(mcExport);

	// default 
	mcExport->AllowBlending(TRUE);

	// convert all vertices to Rigid ( or blended ver 2.0)
	mcExport->ConvertToRigid(TRUE);

	const int def_v_cnt = mcExport->GetNumberVertices();
	
	char progress_name[256] = {0};
	
	*v_step = 0;
	// process a series if vertices belonging to the same node
	for(int i = v_id; i < v->object_count; i++)	
	{
		// new series of vertices
		if(node != (INode*)v->api_node_xyz_id[i])
		{
			phyExport->ReleaseContextInterface(mcExport);
			mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
			assert(i > v_id);
			return;
		}

		if( (i & 0x7) == 0 )
		{
			const float percent = 100.0f * (float)(i) / (float)v->object_count;
			sprintf(progress_name, "%d%% vertex %d %s", (int)percent, i, node_name);
			ip->ProgressUpdate((int)percent, FALSE, progress_name); 
		}

		bone_vertex *bv = v->b_v_list + i;
		bv->Init();

		const int api_vid = v->api_xyz_id[i];
		assert(api_vid < def_v_cnt);

		IPhyVertexExport *vtxExport = mcExport->GetVertexInterface(api_vid);
		if(!vtxExport)
		{	
			Winprint("Error: unassigned vertex %d of %s!\n", api_vid, node->GetName());
		}
		else
		{
			const int type = vtxExport->GetVertexType();

			switch(type)
			{
				//case RIGID_NON_BLENDED_TYPE:
				case RIGID_TYPE:
				{
					IPhyRigidVertex *r_vtxExport = (IPhyRigidVertex*)vtxExport;
				
					INode *bone_node = r_vtxExport->GetNode();
					assert(bone_node);
					const char *bone_name = bone_node->GetName();

					int bone_id = GetBoneID(bl, bone_node);
					if(bone_id < 0)
					{
						Winprint("Error: %s is not a bone ??!\n", bone_name);
						return;
					}

					bv->bone_count = 1;
					bv->bone_id_list = (int*)Malloc(bv->bone_count * sizeof(int));
					bv->xyz_list = (float*)Malloc(bv->bone_count * 3 * sizeof(float));
					bv->normal_list = (float*)Malloc(bv->bone_count * 3 * sizeof(float));
					bv->weight_list = (float*)Malloc(bv->bone_count * sizeof(float));

					bv->bone_id_list[0] = bone_id;
					bv->weight_list[0] = 1.0f;
#if 1
					// get vertex in bone's coord
					//Matrix3 m ( bone_node->GetObjTMAfterWSM(mesh_time) );
					Matrix3 m ( bone_node->GetNodeTM(mesh_time) );
					Matrix3 mns ( GetMyNodeTM(bone_node, mesh_time) );
					CleanMatrix3(mns); //mns.NoScale(); NoShear(mns);
					m *= Inverse(mns);

					//Point3 offset ( m * r_vtxExport->GetDeformOffsetVector(mesh_time) );
					Point3 offset ( m * r_vtxExport->GetOffsetVector() );
#else
					Point3 offset ( obj->v.object_list[3*i],
									obj->v.object_list[3*i+1],
									obj->v.object_list[3*i+2]);
					offset = SwitchCoord(offset, bl->bone_list[0].node, bone_node, mesh_time);
#endif
					bv->xyz_list[0] = offset.x;
					bv->xyz_list[1] = offset.y;
					bv->xyz_list[2] = offset.z;
					
					const int nid = v->normal[i];
					Point3 normal(obj->n.list[3*nid], obj->n.list[3*nid+1], obj->n.list[3*nid+2]); 
					Point3 new_normal ( SwitchNormal(normal, bl->bone_list[0].node, bone_node, mesh_time));

					bv->normal_list[0] = new_normal.x;
					bv->normal_list[1] = new_normal.y;
					bv->normal_list[2] = new_normal.z;
				}
				break;

				case RIGID_BLENDED_TYPE: // (RIGID_TYPE | BLENDED_TYPE) // what we want!!
				{
					IPhyBlendedRigidVertex *b_vtxExport = (IPhyBlendedRigidVertex*)vtxExport;
				
					bv->bone_count = b_vtxExport->GetNumberNodes();
					bv->bone_id_list = (int*)Malloc(bv->bone_count * sizeof(int));
					bv->xyz_list = (float*)Malloc(bv->bone_count * 3 * sizeof(float));
					bv->normal_list = (float*)Malloc(bv->bone_count * 3 * sizeof(float));
					bv->weight_list = (float*)Malloc(bv->bone_count * sizeof(float));

					const int nid = v->normal[i];
					Point3 normal(obj->n.list[3*nid], obj->n.list[3*nid+1], obj->n.list[3*nid+2]); 

					float sum_weight = 0.0f;
					for(int j = 0; j < bv->bone_count; j++)
					{
						INode *bone_node = b_vtxExport->GetNode(j);
						assert(bone_node);
						const char *bone_name = bone_node->GetName();

						int bone_id = GetBoneID(bl, bone_node);
						if(bone_id < 0)
						{
							Winprint("Error: %s is not a bone ??!\n", bone_name);
							return;
						}

						bv->bone_id_list[j] = bone_id;
#if 1
						// get vertex in bone's coord
						//Matrix3 m ( bone_node->GetObjTMAfterWSM(mesh_time) );
						Matrix3 m ( bone_node->GetNodeTM(mesh_time) );
						Matrix3 mns ( GetMyNodeTM(bone_node, mesh_time) );
						CleanMatrix3(mns); //mns.NoScale(); NoShear(mns);
						m *= Inverse(mns);

						//Point3 offset ( m * b_vtxExport->GetDeformOffsetVector(mesh_time, j) );
						Point3 offset ( m * b_vtxExport->GetOffsetVector(j) );
#else
						Point3 offset ( obj->v.object_list[3*i],
										obj->v.object_list[3*i+1],
										obj->v.object_list[3*i+2]);
						offset = SwitchCoord(offset, bl->bone_list[0].node, bone_node, mesh_time);
#endif
						bv->xyz_list[3*j+0] = offset.x;
						bv->xyz_list[3*j+1] = offset.y;
						bv->xyz_list[3*j+2] = offset.z;

						Point3 new_normal ( SwitchNormal(normal, bl->bone_list[0].node,
											bone_node, mesh_time));

						bv->normal_list[3*j+0] = new_normal.x;
						bv->normal_list[3*j+1] = new_normal.y;
						bv->normal_list[3*j+2] = new_normal.z;

						bv->weight_list[j] = b_vtxExport->GetWeight(j);

						sum_weight += bv->weight_list[j];
					}

					if( fabs(1.0f - sum_weight) > .0001f )
					{
						Winprint("Error: bone weights for vertex %d of %s sum up to %.4f!\n",
							i, node->GetName(), sum_weight);
					}
				}
				break;

				//case DEFORMABLE_NON_BLENDED_TYPE:
				case BLENDED_TYPE:
					Winprint("Error: Got a Blended vertex on %s!\n", node->GetName());
					break;

				case DEFORMABLE_BLENDED_TYPE: // (DEFORMABLE_TYPE | BLENDED_TYPE)
				case DEFORMABLE_TYPE:
					Winprint("Error: Got a Deformable vertex on %s!\n", node->GetName());
					break;

				default:
					Winprint("Error: Unknown Physique vertex type %d on %s!n", type, node->GetName());
					exit(1);
			}

			mcExport->ReleaseVertexInterface(vtxExport);
			vtxExport = NULL;
		}
	
		(*v_step)++;
	}

	phyExport->ReleaseContextInterface(mcExport);
	mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
}

ULONG MyGetGBufID(INode *node)
{
	//const char *name = node->GetName();
	ObjectState os ( node->EvalWorldState((TimeValue)0) );

	if( os.obj )
	{
		const SClass_ID sc_id ( os.obj->SuperClassID() );
		const Class_ID c_id ( os.obj->ClassID() );

		if( sc_id == HELPER_CLASS_ID && c_id == Class_ID( BONE_CLASS_ID, 0 ) )
		{
			return 2; // PHYS_BONE
		}
	}

	return node->GetGBufID();
}

void AsciiExp::GetBones(bone_lib *bl)
{
	for(int i = 0; i < inode_list.count; i++)
	{
		INode *node = inode_list[i];

		switch(MyGetGBufID(node))
		{
			case ROOT_BONE:
			case PRO_BONE:
			case PHYS_BONE:
			case MESH_BONE:
			case SKIRT_BONE:
			case OPEN_BONE:
			case CHILD_BONE:
			case PARENT_BONE:
			case UV_BONE:
			case REYE_BONE:
			case LEYE_BONE:
			{
				bl->count++;
				bl->bone_list = (max_bone*)Realloc(bl->bone_list, bl->count * sizeof(max_bone));
				InitMaxBone(&(bl->bone_list[bl->count - 1]));

				max_bone & bone = bl->bone_list[bl->count - 1];
				bone.node = node;
				bone.type = (bone_type)MyGetGBufID(node);
				bone.depth = GetDepth(node);
				bone.parent = GetMyObjParent(node);
			}
			break;

			default:
				;
		}
	}
}

bool AsciiExp::OrganizeBones(bone_lib *bl)
{
	bool result = false;

	qsort(bl->bone_list, bl->count, sizeof(max_bone), CompareBonesAlphabetically);
	qsort(bl->bone_list, bl->count, sizeof(max_bone), CompareBonesByDepth);

	// if a root was explicitly specified put it first
	int root_count = 0;
	int r_eye_count = 0;
	int l_eye_count = 0;
	for(int i = 0; i < bl->count; i++)
	{
		if(bl->bone_list[i].type == ROOT_BONE)
		{
			if(root_count == 0)
			{
				SwapBones(bl, 0, i);
			}
			root_count++;
		}else
		if(bl->bone_list[i].type == REYE_BONE)
		{
			r_eye_count++;
		}else
		if(bl->bone_list[i].type == LEYE_BONE)
		{
			l_eye_count++;
		}
	}

	if(root_count > 1)
	{
		Winprint("Error: %d root (ID %d) bones found!\n", root_count, ROOT_BONE);
		return false;
	}

	if(r_eye_count > 1)
	{
		Winprint("Error: %d RightEye (ID %d) bones found!\n", r_eye_count, REYE_BONE);
		return false;
	}

	if(l_eye_count > 1)
	{
		Winprint("Error: %d LeftEye (ID %d) bones found!\n", l_eye_count, LEYE_BONE);
		return false;
	}

	const int default_con_type = (use_loose_joints) ? LOOSE : SPHERICAL;

	// special case root bone
	bl->bone_list[0].type = ROOT_BONE;
	bl->bone_list[0].parent = NULL;

	// if loose connections assign all bones directly to root
	/* not after E3 demo
	if(default_con_type == LOOSE)
	{
		for(int i = 1; i < bl->count; i++)
		{
			// orientation is relative to root bone
			if(bl->bone_list[i].type != UV_BONE)
			{
				bl->bone_list[i].parent = bl->bone_list[0].node;
			}
		}
	}
	*/


	// fix up child/parent bones (workaround for bones that MAX won't allow to be linked correctly)
	int parent_bid = -1;
	for(i = 0; i < bl->count; i++)
	{
		if(bl->bone_list[i].type == PARENT_BONE)
		{
			if(parent_bid == -1)
			{
				parent_bid = i;
			}
			else
			{
				Winprint("Error: only one bone can be a parent bone ID %s!\n", PARENT_BONE);
				break;
			}
		}
	}

	for(i = 1; i < bl->count; i++)
	{
		
		if(bl->bone_list[i].type == CHILD_BONE)
		{
			if(parent_bid >= 0)
			{
				bl->bone_list[i].parent = bl->bone_list[parent_bid].node;
			}
			else
			{
				Winprint("Error: found a child bone %s w/ ID %d but no parent bone ID %d !\n",
					bl->bone_list[i].node->GetName(), CHILD_BONE, PARENT_BONE);
					break;
			}
		}
	}



	for(i = 0; i < bl->count; i++)
	{
		max_bone & mb = bl->bone_list[i];
		switch(mb.type)
		{
			case ROOT_BONE:
				mb.con_type = LOOSE;
				break;
			case PRO_BONE:
				mb.con_type = default_con_type;
				break;

			case PHYS_BONE:
				mb.con_type = default_con_type;
				break;

			case MESH_BONE:
				mb.con_type = default_con_type;
				break;

			case SKIRT_BONE:
				mb.con_type = LOOSE;
				break;

			case OPEN_BONE:
				mb.con_type = OPEN;
				break;

			case UV_BONE:
			case REYE_BONE:
			case LEYE_BONE:
				mb.con_type = SPHERICAL;
				break;

			case CHILD_BONE:
				mb.con_type = default_con_type;
				break;

			case PARENT_BONE:
				mb.con_type = default_con_type;
				break;

			default:
				;
		}
	}

	result = true;

	// check all parents are valid bones
	for(i = 1; i < bl->count; i++)
	{
		if( bl->bone_list[i].parent == NULL)
		{
			const char *name = bl->bone_list[i].node->GetName();
			Winprint("Error: NO parent for bone %s!\n", name);
			result = false;
		}
		else
		if( GetBoneID(bl, bl->bone_list[i].parent) < 0 )
		{
			const char *pname = bl->bone_list[i].parent->GetName();
			const char *name = bl->bone_list[i].node->GetName();
			Winprint("Error: parent %s of %s is not a bone!\n",
				pname, name);
			result = false;
		}
	}

	return result;
}

void AsciiExp::SwapBones(bone_lib *bl, const int id1, const int id2)
{
	if(id1 == id2) return;

	MemSwap(bl->bone_list + id1, bl->bone_list + id2, sizeof(max_bone));
}

void InitMaxBone(max_bone *mb)
{
	mb->node=NULL;
	mb->parent=NULL;
	mb->depth=-1;
	mb->con_type = NONE;
	mb->type = UNKNOWN_BONE;
	mb->frame_count = 0;
	mb->frame_list = NULL;
}


void AsciiExp::ExportMeshHP(CompoundObject *c_obj, const bone_lib *bl)
{
	assert(c_obj->part_count == bl->count);
	for(int i=0; i < bl->count; i++) 
	{
		InitObject(&(c_obj->lod_object_list[i].obj_list[0]),/* &(c_obj->tl), &(c_obj->atl),*/ &(c_obj->ml));
		ExportHP(&(c_obj->lod_object_list[i].obj_list[0]), bl->bone_list[i].node, GetStaticFrame());
		ip->ProgressUpdate((int)((float)(i)/bl->count*100.0f), TRUE, NULL);
		if( ip->GetCancel() ) break;
	}
}

void InitBoneLib(bone_lib *bl)
{
	bl->count=0;
	bl->bone_list=NULL;
}

void AsciiExp::ExportBoneConnections(CompoundObject *c_obj, bone_lib *bl)
{
	// insert root
	if(root_name == NULL)
	{
		root_name = (char*)Malloc((strlen(bl->bone_list[0].node->GetName())+1)*sizeof(char));
		strcpy(root_name, bl->bone_list[0].node->GetName());
	}
	else
	{
		assert(!strcmp(root_name, bl->bone_list[0].node->GetName()));
	}
	

	int n_frames = 1 +
		(ip->GetAnimRange().End() - _MAX<TimeValue>(0, ip->GetAnimRange().Start())) / GetTicksPerFrame();
	if(export_animation)
	{
		for(int i=0; i < bl->count; i++)
		{
			// if selected animations only checked
			if(!selected_anims_only || bl->bone_list[i].node->Selected())
			{
				bl->bone_list[i].frame_list = (Frame*)Malloc(n_frames * sizeof(Frame));
				bl->bone_list[i].frame_count = n_frames;
			}
		}

		ReadFrames(NULL, NULL, -1, NULL, bl);
	}
	
	char file_name[256] = {0};

	PersistTransform o_to_w;
	o_to_w.identity();

	int i;
	if(export_root_animation)
	{
		i = 0;	
	}
	else
	{
		strncpy(file_name, bl->bone_list[0].node->GetName(), 255);
		strcat(file_name, date_name);
		InsertCompoundName(c_obj, bl->bone_list[0].node->GetName(), file_name, root_name,
			".3db", 2, &o_to_w, true);
		i = 1;
	}

	const TimeValue trans_time = _MIN<TimeValue>(0, ip->GetAnimRange().Start());

	for(; i < bl->count; i++)
	{
		Matrix3 m_o_to_w = GetMyLocalNodeTM(bl->bone_list[0].node, trans_time,
			bl->bone_list[i].node);
		CleanMatrix3(m_o_to_w);
		TransposeMatrix3(m_o_to_w);

		o_to_w = Matrix3_to_PersistTransform(m_o_to_w);
		
		strncpy(file_name, bl->bone_list[i].node->GetName(), 255);
		strcat(file_name, date_name);

		
		if(bl->bone_list[i].con_type != OPEN)
		{
			InsertCompoundName(c_obj, bl->bone_list[i].node->GetName(), file_name, root_name,
			".3db", 2, &o_to_w, true);
			ExportConnection(bl->bone_list[i].node, c_obj, bl->bone_list[i].parent,
							 bl->bone_list[i].con_type,
							 bl->bone_list[i].frame_count, bl->bone_list[i].frame_list);
		}
		else
		{
			InsertCompoundName(c_obj, bl->bone_list[i].node->GetName(), file_name, root_name,
			".3db", 2, &o_to_w, false);
		}
	}

	for(i=0; i < bl->count; i++)
	{
		Free(bl->bone_list[i].frame_list);
		bl->bone_list[i].frame_count = 0;
	}
}

PersistTransform Matrix3_to_PersistTransform(const Matrix3 & m3)
{
	PersistTransform result;
	result.identity();

	Point3 point;
	point = m3.GetRow(0);
	result.m.e00 = point.x;
	result.m.e01 = point.y;
	result.m.e02 = point.z;

	point = m3.GetRow(1);
	result.m.e10 = point.x;
	result.m.e11 = point.y;
	result.m.e12 = point.z;
	
	point = m3.GetRow(2);
	result.m.e20 = point.x;
	result.m.e21 = point.y;
	result.m.e22 = point.z;
	
	point = m3.GetRow(3);
	result.v.x = point.x;
	result.v.y = point.y;
	result.v.z = point.z;

	return result;
}

// lowest to highest by tree depth
int CompareBonesByDepth(const void *pt1, const void *pt2)
{
  if( ((max_bone*)pt1)->depth < ((max_bone*)pt2)->depth ){
    return -1;
  }else
  if( ((max_bone*)pt1)->depth > ((max_bone*)pt2)->depth ){
    return 1;
  }
  return 0;
}  

// lowest to highest by alphabet
int CompareBonesAlphabetically(const void *pt1, const void *pt2)
{
	return strcmp(((max_bone*)pt1)->node->GetName(), ((max_bone*)pt2)->node->GetName());
} 


INode* GetParentBone(const bone_lib *bl, INode *bone_node)
{
	if( (bone_node == NULL) || (bl->count < 1) ) return NULL;
#ifdef _DEBUG
	const char *name = bone_node->GetName();
#endif

	INode *parent = GetMyObjParent(bone_node);
	while(parent)
	{
#ifdef _DEBUG
		const char *pname = parent->GetName();
#endif
		if(GetBoneID(bl, parent) >= 0 )
		{
			return parent;
		}

		parent = GetMyObjParent(parent);
	}

	return NULL;
}


int GetBoneID(const bone_lib *bl, INode *bone_node)
{
	assert(bone_node);
#ifdef _DEBUG
	const char *name = bone_node->GetName();
#endif

	for(int i=0; i<bl->count; i++)
	{
		if(bl->bone_list[i].node == bone_node)
		{
			return i;
		}
	}

	return -1;
}

int GetFirstBoneType(const bone_lib *bl, const bone_type type)
{

	for(int i = 0; i < bl->count; i++)
	{
		if(bl->bone_list[i].type == type)
		{
			return i;
		}
	}

	return -1;
}

void ExportRootTransform(INode *node, PersistTransform *xform, TimeValue t)
{
	xform->identity();
	
	Matrix3 tm;
	Point3 pos;

	if(exporting_deformable) // characters are adjusted to be alligned to world
	{

// clean this up
		tm = node->GetNodeTM(t);
		if(!world_adjust.IsIdentity())
		{
			tm = world_adjust * tm;
		}
		CleanMatrix3(tm);
		TransposeMatrix3(tm);

		// Get translation
		pos = tm.GetTrans();

		// convert to this nodes local coord system
		tm.NoTrans();

		if(!world_adjust.IsIdentity())
		{
			pos = world_adjust * pos;
		}
	}
	else
	{
		// get local transform matrix
		tm = GetMyNodeTM(node, t);
		CleanMatrix3(tm);
		TransposeMatrix3(tm);

		// Get translation
		pos = tm.GetTrans();

		// convert to this nodes local coord system
		tm.NoTrans();
		pos = pos * tm;
	}

	xform->v.x = pos.x;
	xform->v.y = pos.y;
	xform->v.z = pos.z;

	// Get orientation
	Point3 point;
	point=tm.GetRow(0);

	xform->m.e00=point.x;
	xform->m.e01=point.y;
	xform->m.e02=point.z;

	point=tm.GetRow(1);
	xform->m.e10=point.x;
	xform->m.e11=point.y;
	xform->m.e12=point.z;

	point=tm.GetRow(2);
	xform->m.e20=point.x;
	xform->m.e21=point.y;
	xform->m.e22=point.z;
}

void AsciiExp::ExportConnection(INode *node, CompoundObject *c_obj, INode *parent, int con_type,
								int n_frames, Frame * frame_list)
{	
	const char *name = node->GetName();
#ifdef _DEBUG
	const char *pname = (parent) ? parent->GetName() : NULL;
#endif

	BOOL bPos = false, bRot = false, bScale = false;

	fps = (float)GetFrameRate();

	if(!strcmp(root_name, name))
	{
		ExportRootTransform(node, &(c_obj->root_transform), GetStaticFrame());
	}
	
	if(exporting_deformable)
	{
		if(!strcmp(root_name, node->GetName()))
		{
			if(!export_root_animation || frame_list == NULL) // skip root animation
			{
				return;
			}
			else
			{
				MakeRootAnim(c_obj, node, n_frames, frame_list);
			}
		}
		else
		{
			if(frame_list == NULL)
			{
				MakeDofConnection(c_obj, node, parent, con_type);
			}
			else
			{
				if(con_type == SPHERICAL)
				{
					MakeRevAnim(c_obj, node, parent, n_frames, frame_list);
				}else
				if(con_type == LOOSE)
				{
					MakeLooseAnim(c_obj, node, parent, n_frames, frame_list);
				}
				else
				{
					Winprint("Bad joint/animation type for %s !\n", node->GetName());
				}
			}
		}
	}
	else
	if(	n_frames > 0 || CheckForAnimation(node, bPos, bRot, bScale, parent) )
	{
		if(bScale)
		{
			Winprint("Warning: %s has animated SCALE", node->GetName());
		}

		if(bPos || bRot)
		{
			// printf("fps=%f\n",fps);
			// printf(" %d %d %d\n",ip->GetAnimRange().Start(), ip->GetAnimRange().End(), GetTicksPerFrame());
			  
			if(!strcmp(root_name, node->GetName()) && (bPos || bRot))
			{
				MakeRootAnim(c_obj, node, n_frames, frame_list);
			}else
			if(bPos && bRot)
			{			 
				if(allow_loose_joints)
				{
					MakeLooseAnim(c_obj, node, parent, n_frames, frame_list);
				}
				else
				{
					Winprint("Warning: %s has both prismatic and rotational animation\n"
							 "Delete any Position key frames OR\n"
							 "Try splitting joint into two joints.\n"
							 "I will export it as a Rotational ONLY joint for now.", name);

					MakeRevAnim(c_obj, node, parent, n_frames, frame_list);
				}
				//InsertFixed(c_obj, fixed);
			}else
			if(bRot){ // REVOLUTE
				MakeRevAnim(c_obj, node, parent, n_frames, frame_list);
				//InsertFixed(c_obj, fixed);
			}else
			if(bPos){ // PRISMATIC
				MakePrisAnim(c_obj, node, parent, n_frames, frame_list);
				//InsertFixed(c_obj, fixed);
			}
		}
	}
	else // add DOF/IK connections later 
	{
		if( strcmp(root_name, node->GetName()) )
		{
			MakeDofConnection(c_obj, node, parent, NONE);
		}
	}	
}

/*
_sqrt fsqrt
int power2( int num, int power )
{
   __asm
   {
      mov eax, num    ; Get first argument
      mov ecx, power  ; Get second argument
      shl eax, cl     ; EAX = EAX * ( 2 to the power of CL )
   }
   // Return with result in EAX
}
*/

void AsciiExp::ExportGlobalEvents(CompoundObject *c_obj)
{
	return;// no longer using this for a globals event list, now each animation cuts the note list up for its' own purposes
	/*
	// Add Channel
	NamedChannel n_channel;
	InitNamedChannel(&n_channel);
	if(strlen(body_name) + 3 > PersistAnimCHANNEL_NAME_MAX-1)
	{
		Winprint("Error: Channel name Ch_%s is longer than %d characters! Truncating.\n",
			body_name, PersistAnimCHANNEL_NAME_MAX-1);
	}
	_snprintf(n_channel.name, PersistAnimCHANNEL_NAME_MAX-1, "Ch_%s", body_name);

	n_channel.channel.header.frames = 0;
	n_channel.channel.header.capture_rate = -1.0f;
	n_channel.channel.header.type = PersistDT_EVENT;

	// only animated objects get sound for now
	if(c_obj->n_channel_count > 0 )
	{
		// Retrieve the current sound object
		SoundObj *sound = ip->GetSoundObject();

		// Get a wave interface
		IWaveSound *iWave = GetWaveSoundInterface(sound);
		if (iWave && (strlen(iWave->GetSoundFileName()) > 0) )
		{
			char wav_file_name[256]; wav_file_name[0] = 0;
			strncpy(wav_file_name, iWave->GetSoundFileName(), 255);
			StripPath(wav_file_name);

			float start_time = (float)iWave->GetStartTime();
			float end_time = (float)iWave->GetEndTime();
			float tpf = (float)GetTicksPerFrame();

			start_time = start_time / fps / tpf;
			end_time = end_time / fps / tpf; 
			assert(start_time < end_time);
			
			// make sure we don't play past end of channel
			float end_channel = (float)((ip->GetAnimRange().End() - _MAX<TimeValue>(0, ip->GetAnimRange().Start()))
										 / GetTicksPerFrame() ) / fps;
			end_time = _MIN(end_time, end_channel);
			
			InsertChannelEvent(n_channel, start_time, SOUND_START, wav_file_name);
			InsertChannelEvent(n_channel, end_time, SOUND_STOP, NULL);
		}
	}

	if(exporting_deformable || c_obj->n_channel_count > 0)
	{
		bool inserted_begin = false;
		if( exporting_deformable )
		{
			InsertChannelEvent(n_channel, 0.0f, CHANNEL_BEGIN, NULL);
			inserted_begin = true;
		}

		// export global note tracks as named events
		if(ip->GetRootNode()->HasNoteTracks())
		{
			TimeValue start = _MAX<TimeValue>(0, ip->GetAnimRange().Start()) / GetTicksPerFrame();
			TimeValue end = ip->GetAnimRange().End() / GetTicksPerFrame();
			DefNoteTrack * ntrack = (DefNoteTrack*)(ip->GetRootNode()->GetNoteTrack(0));
			int keys = ntrack->NumKeys();
			for(int i = 0; i < keys; i++)
			{
				Note note;
				note.t = ntrack->GetKeyTime(i) / GetTicksPerFrame();
				note.name = (char*)ntrack->keys[i]->note;

				if(note.t >= start &&
				   note.t <= end )
				{
					float time = ( note.t - start ) / fps;

					if( !inserted_begin ) // for regular objects we only want this if there is other stuff but it should be first
					{
						InsertChannelEvent(n_channel, 0.0f, CHANNEL_BEGIN, NULL);
						inserted_begin = true;
					}

					InsertChannelEvent(n_channel, time, NAMED_EVENT, note.name);
				}
			}
		}

		if( inserted_begin )
		{
			float end_channel = (float)((ip->GetAnimRange().End() - _MAX<TimeValue>(0, ip->GetAnimRange().Start()))
			/ GetTicksPerFrame() ) / fps;
			InsertChannelEvent(n_channel, end_channel, CHANNEL_END, NULL);
		}
	}


	if(n_channel.channel.header.frames > 0)
	{
		// Script
		NamedScript script;
		InitScript(&script);
		_snprintf(script.name, 255, "Sc_%s", body_name);
		script.channel_count=1;
		script.channel_list=(PersistAnimChannelMapping*)
							Malloc(sizeof(PersistAnimChannelMapping));
		script.channel_list[0].parent[0]=0;
		script.channel_list[0].child[0]=0;
		strcpy(script.channel_list[0].channel, n_channel.name);

		// add data to cmp object
		assert(!strcmp(script.channel_list[0].channel, n_channel.name));
		InsertNamedChannel(c_obj, n_channel);
		InsertScript(c_obj, script);
	}*/
}

int AsciiExp::ExportCamera(INode *node, object *obj)
{
	const char *name = node->GetName();

	assert(obj->type == INVALID || obj->type == CAMERA);
	obj->type = CAMERA;

	int success = FALSE;
	TimeValue t = GetStaticFrame();

	ObjectState os = node->EvalWorldState(t); 
	if (os.obj){									

		if( os.obj->SuperClassID() == CAMERA_CLASS_ID ){
			
			CameraObject *cam = (CameraObject *)os.obj;

			CameraState cs;
			Interval valid = FOREVER; // Interval(0,0)

			cam->EvalCameraState(t, valid, &cs);

			if(cam->IsOrtho())
			{
				Winprint("Error: ORTHO camera %s not yet supported!\n", name);
			}

			obj->c.fovx = cs.fov / 2.0f;	// 1/2 of the horizontal field of view in Radians
			obj->c.fovy = (float)atan((3.0/4.0) * tan(obj->c.fovx));
			
			if(cam->GetManualClip())
			{
				obj->c.znear = cam->GetClipDist((TimeValue)0, CAM_HITHER_CLIP);
				if(obj->c.znear == 0.0f)
				{
					obj->c.znear = 1.0f;
					Winprint("Error: camera %s can't have near range 0.0!  Using 1.0\n",
						node->GetName());
				}

				obj->c.zfar = cam->GetClipDist((TimeValue)0, CAM_YON_CLIP);
			}
			else
			{
				obj->c.znear = cam->GetEnvRange((TimeValue)0, ENV_NEAR_RANGE);
				if(obj->c.znear == 0.0f)
				{
					obj->c.znear = 1.0f;
					Winprint("Error: camera %s can't have near range 0.0! Using 1.0\n",
						node->GetName());
				}

				obj->c.zfar = cam->GetEnvRange((TimeValue)0, ENV_FAR_RANGE);
			}
			
			success = TRUE;
		}
	}

	return success;
}

int AsciiExp::ExportLight(INode *node, object *obj)
{
	const char *name = node->GetName();

	assert(obj->type == INVALID || obj->type == LIGHT);
	obj->type = LIGHT;

	int success = FALSE;
	TimeValue t = GetStaticFrame();

	ObjectState os = node->EvalWorldState(t); 
	if (os.obj)
	{									
		if( os.obj->SuperClassID() == LIGHT_CLASS_ID  )
		{
			GenLight* light = (GenLight*)os.obj;

			if(light->GetUseLight())
			{
				
				Texmap *tex = light->GetProjMap();
				if(tex)
				{
					Winprint("Light %s has texture %s assigned! Supported SOON!\n",
						node->GetName(), tex->GetName());
				}

				// set up transform to bone's local coord system
				Matrix3 tm ( GetMyObjTMAfterWSM(node, t) );
				Matrix3 ntm = node->GetNodeTM(t);
				CleanMatrix3(ntm); //ntm.NoScale();  NoShear(ntm); // this will make sure scale is applied to exported geometry
				tm *= Inverse(ntm);

				Point3 ps;
				ps = tm.GetRow(0);
				float sx = ps.x;
				ps = tm.GetRow(1);
				float sy = ps.y;
				ps = tm.GetRow(2);
				float sz = ps.z;

				float scale = (sx + sy + sz) / 3.0f;

				struct LightState ls;

				Interval valid = FOREVER;
				light->EvalLightState(t, valid, &ls);

				if(ls.on)
				{
					if(!ls.affectDiffuse)
					{
						Winprint("Warning: Light %s does not affect Diffuse. NOT exporting!\n", name);
					}
					else
					{
						obj->l.color[0] = ls.intens * ls.color.r;
						obj->l.color[1] = ls.intens * ls.color.g;
						obj->l.color[2] = ls.intens * ls.color.b;

						if(ls.nearAttenStart > 0.0 || ls.nearAttenEnd > 0.0 || ls.attenStart > 0.0)
						{
							Winprint("Warning: Light %s has non zero attenuation parameters for "
									 "either Near-Start/End or Far-Start!\n", name);
						}
			

						switch(ls.type)
						{
						case DIRECT_LGT:
							obj->l.parallel = 1;
							obj->l.cutoff =
							obj->l.hotspot = (float)M_PI;
							obj->l.range = -1.0f;
							break;

						case OMNI_LGT:
							obj->l.parallel = 0;
							obj->l.cutoff =
							obj->l.hotspot = (float)M_PI;

							if(ls.useAtten)
							{
								obj->l.range = ls.attenEnd * scale;
							}
							else
							{
								obj->l.range = -1;
							}
							break;

						case SPOT_LGT:
							obj->l.parallel = 0;
							obj->l.cutoff = (float)D2R * ls.fallsize/2.0f;
							obj->l.hotspot = (float)D2R * ls.hotsize/2.0f;

							if(ls.useAtten)
							{
								obj->l.range = ls.attenEnd * scale;
							}
							else
							{
								obj->l.range = -1;
							}
							break;

						case AMBIENT_LGT:
							Winprint("Error: Export of ambient light %s NOT supported!\n",
								node->GetName());
							return FALSE;

						default:
							Winprint("Error: unknown Light type for %s\n!", node->GetName());
							return FALSE;
						}

					
						// If we have a target object get direction in local coord
						INode* target = node->GetTarget();
						if (target) 
						{
							Point3 pivot = GetPivot(target, GetStaticFrame(), node);
							pivot=Normalize(pivot);
							obj->l.direction[0] = pivot.x;
							obj->l.direction[1] = pivot.y;
							obj->l.direction[2] = pivot.z;
						}
						else
						{
							obj->l.direction[0] =  0.0f;
							obj->l.direction[1] =  0.0f;
							obj->l.direction[2] = -1.0f;
							// should we set cutoff to 180 here ??
						}

						success = TRUE;
					}
				}
			}
		}
	}

	return success;
}

INode* GetNodeByName(INode *node, const char *name)
{
	if(node == NULL) return NULL;
	
	if( !strcmp(node->GetName(), name) ) return node;
		

	for (int c=0; c<node->NumberOfChildren(); c++) // children are counted only 1 level deep
	{
		if( INode *result = GetNodeByName(node->GetChildNode(c), name) )
		{
			return result;
		}
	}
	
	return NULL;
}

/*
inline _putf( float f, FILE *fp )
{
	_putw( *(int*)&f, fp );
}
*/

void ExpOptions::Init(void)
{
	// file_name[0] = 0;
	nsave_options_to_max_file = 1;
	
	nDensity = 10.0f;
	nScale = 1.0f;
	nmip_flag = 1;
	nflag_565 = 0;
	nflag_888 = 0;
	ndither_flag = 0;
	nsplit_flag = 0;
	ndefault_mat_flag = 0;
	nexport_mesh = 1;
	//nexport_skeleton = 1;
	nexport_animation = 1;
	nexport_lights = 0;
	nexport_cameras = 0;
	nexport_selected = 0;
	//nbiped_y_up = 0;
	nignore_warnings = 0;

	nLodPercent = 100.0f;
	nLodClosestDist = 100.0f;
	nLodFurthestDist = 1000.0f;
	nLodDropOut = 0.0f;
	//1500.0f; not written out for now, because even single level objects would have to be multilevel
	//nLodCount = 1;
	nMtlBoundary = 5.0f;
	nTxtWeight = 0.85f;

	nadjust_rotate_x = 90.0f;
	nadjust_rotate_y = 0.0f;
	nadjust_rotate_z = 180.0f;
	nno_physics = 0;
	ncenter_mass = 0;
	nvertex_color = 0;
	nik_extents = 0;
	nsm_groups = 0;
	nex_avi = 0;
	nex_txt = 0;
	nnon_unique_name = 0;
	nallow_loose_j = 0;
	nallign_heading = 1;
	nrelative_deformable = 0;
	//nhead = 0;
	nsel_anim_only = 0;
	nloose_j = 0;
	nroot_anim = 1;
	nin_world = 1;
}

void StripWhiteEnd(char *buf)
{
	if( buf )
	{
		const int len = strlen(buf);
		for(int i = len - 1; i >= 0; i--)
		{
			if(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\n' || buf[i] == 13)
			{
				buf[i] = 0;
			}
			else
			{
				break;
			}
		}
	}
}

void ExpOptions::Read( HWND hWnd )
{
	// open file selection window
	if( hWnd != NULL )
	{
		OPENFILENAME ofn;
		memset( &ofn, 0, sizeof(ofn) );
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hWnd;
		ofn.hInstance = GetModuleHandle(NULL);
		ofn.lpstrFilter = "Exporter Option Files (*.cfg)\0*.cfg\0All Files (*.*)\0*.*\0\0";
		ofn.lpstrFile = file_name;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = NULL;
		ofn.lpstrTitle = "Open Options File";
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_LONGNAMES | 0;

		if( !GetOpenFileName( &ofn ) )
			return;
	}

	if( file_name[0] == 0 )
		return;
	
	FILE* fp = fopen(file_name, "r");
	if( !fp )
		return;

	const char seps[5] = { ' ', '\t', '\n', 13, 0 };
	char line[512];

	while( GetLine(fp, line) != EOF )
	{
		char *key = strtok(line, "=");
		const char *val = strtok(NULL, seps);
		StripWhiteEnd( key ); // strip any white space off the end
		if( !key || !val )
			continue;

		if( !strcmp( key, "IDC_SAVE_MAX_OPT" ) )
		{
			nsave_options_to_max_file = (int)atoi( val );
		}else
		if( !strcmp( key, "IDC_DENS" ) )
		{
			nDensity = (float)atof( val );
		}else
		if( !strcmp( key, "IDC_SCALE" ) )
		{
			nScale = (float)atof( val );
		}else
		if( !strcmp( key, "IDC_MIP" ) )
		{
			nmip_flag = atoi( val );
		}else
		if( !strcmp( key, "IDC_565" ) )
		{
			nflag_565 = atoi( val );
		}else
		if( !strcmp( key, "IDC_888" ) )
		{
			nflag_888 = atoi( val );
		}else
		if( !strcmp( key, "IDC_DITHER" ) )
		{
			ndither_flag = atoi( val );
		}else
		if( !strcmp( key, "IDC_SPLIT" ) )
		{
			nsplit_flag = atoi( val );
		}else
		if( !strcmp( key, "IDC_DEFAULT_MAT" ) )
		{
			ndefault_mat_flag = atoi( val );
		}else
		if( !strcmp( key, "IDC_MESH" ) )
		{
			nexport_mesh = atoi( val );
		}else
		if( !strcmp( key, "IDC_ANIMATION" ) )
		{
			nexport_animation = atoi( val );
		}else
		if( !strcmp( key, "IDC_LIGHT" ) )
		{
			nexport_lights = atoi( val );
		}else
		if( !strcmp( key, "IDC_CAMERA" ) )
		{
			nexport_cameras = atoi( val );
		}else
		if( !strcmp( key, "IDC_SELECTED" ) )
		{
			nexport_selected = atoi( val );
		}else
		if( !strcmp( key, "IDC_NO_PHYSICS" ) )
		{
			nno_physics = atoi( val );
		}else
		if( !strcmp( key, "IDC_CENTER_MASS" ) )
		{
			ncenter_mass = atoi( val );
		}else
		if( !strcmp( key, "IDC_VERTEX_COLOR" ) )
		{
			nvertex_color = atoi( val );
		}else
		if( !strcmp( key, "IDC_IK_EXT" ) )
		{
			nik_extents = atoi( val );
		}else
		if( !strcmp( key, "IDC_SM_GRP" ) )
		{
			nsm_groups = atoi( val );
		}else
		if( !strcmp( key, "IDC_EX_AVI" ) )
		{
			nex_avi = atoi( val );
		}else
		if( !strcmp( key, "IDC_EX_TXT" ) )
		{
			nex_txt = atoi( val );
		}else
		if( !strcmp( key, "IDC_NOT_UNIQUE" ) )
		{
			nnon_unique_name = atoi( val );
		}else
		if( !strcmp( key, "IDC_ALLOW_LOSE_J" ) )
		{
			nallow_loose_j = atoi( val );
		}else
		if( !strcmp( key, "IDC_ALLIGN_HEADING" ) )
		{
			nallign_heading = atoi( val );
		}else
		if( !strcmp( key, "IDC_REL_ANIM" ) )
		{
			nrelative_deformable = atoi( val );
		}else
		if( !strcmp( key, "IDC_SEL_ANIM_ONLY" ) )
		{
			nsel_anim_only = atoi( val );
		}else
		if( !strcmp( key, "IDC_LOOSE_J" ) )
		{
			nloose_j = atoi( val );
		}else
		if( !strcmp( key, "IDC_ROOT_ANIM" ) )
		{
			nroot_anim = atoi( val );
		}else
		if( !strcmp( key, "IDC_IN_WORLD" ) )
		{
			nin_world = atoi( val );
		}else
		if( !strcmp( key, "IDC_IGNORE_WARNINGS" ) )
		{
			nignore_warnings = atoi( val );
		}else
		if( !strcmp( key, "IDC_LOD_PERCENT" ) )
		{
			nLodPercent = (float)atof( val );
		}else
		if( !strcmp( key, "IDC_LOD_CLOSEST" ) )
		{
			nLodClosestDist = (float)atof( val );
		}else
		if( !strcmp( key, "IDC_LOD_FURTHEST" ) )
		{
			nLodFurthestDist = (float)atof( val );
		}else
		if( !strcmp( key, "IDC_LOD_DROPOUT" ) )
		{
			nLodDropOut = (float)atof( val );
		}else
		if( !strcmp( key, "IDC_MTL_BOUNDARY" ) )
		{
			nMtlBoundary = (float)atof( val );
		}else
		if( !strcmp( key, "IDC_TXT_WEIGHT" ) )
		{
			nTxtWeight = (float)atof( val );
		}else
		if( !strcmp( key, "IDC_ROTATE_X" ) )
		{
			nadjust_rotate_x = (float)atof( val );
		}else
		if( !strcmp( key, "IDC_ROTATE_Y" ) )
		{
			nadjust_rotate_y = (float)atof( val );
		}else
		if( !strcmp( key, "IDC_ROTATE_Z" ) )
		{
			nadjust_rotate_z = (float)atof( val );
		}
	}

	fclose(fp);
}

void ExpOptions::Write( HWND hWnd )
{
	// pup up file selection box
	if( hWnd != NULL )
	{
		OPENFILENAME ofn;
		memset( &ofn, 0, sizeof(ofn) );
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hWnd;
		ofn.hInstance = GetModuleHandle(NULL);
		ofn.lpstrFilter = "Exporter Option Files (*.cfg)\0*.cfg\0All Files (*.*)\0*.*\0\0";
		ofn.lpstrFile = file_name;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = NULL;
		ofn.lpstrTitle = "Save Options File";
		ofn.Flags = OFN_LONGNAMES | 0;

		if( !GetSaveFileName( &ofn ) )
			return;
	}

	if( file_name[0] == 0 )
		return;

	FILE* fp = fopen(file_name, "w");
	if( !fp )
		return;

	// Write CFG version
	fprintf(fp, "ExporterBuildDate %s %s\n", __DATE__, __TIME__); // (version)for info only
	fprintf(fp, "Lib Version %s\n", LIB_VER_STRING);

	fprintf(fp, "IDC_SAVE_MAX_OPT = %d\n", nsave_options_to_max_file);
	
	fprintf(fp, "IDC_DENS = %f\n", nDensity);
	fprintf(fp, "IDC_SCALE = %f\n", nScale);
	fprintf(fp, "IDC_MIP = %d\n", nmip_flag);
	fprintf(fp, "IDC_565 = %d\n", nflag_565);
	fprintf(fp, "IDC_888 = %d\n", nflag_888);
	fprintf(fp, "IDC_DITHER = %d\n", ndither_flag);
	fprintf(fp, "IDC_SPLIT = %d\n", nsplit_flag);
	fprintf(fp, "IDC_DEFAULT_MAT = %d\n", ndefault_mat_flag);
	fprintf(fp, "IDC_MESH = %d\n", nexport_mesh);
	fprintf(fp, "IDC_ANIMATION = %d\n", nexport_animation);
	fprintf(fp, "IDC_LIGHT = %d\n", nexport_lights);
	fprintf(fp, "IDC_CAMERA = %d\n", nexport_cameras);
	fprintf(fp, "IDC_SELECTED = %d\n", nexport_selected);
	fprintf(fp, "IDC_NO_PHYSICS = %d\n", nno_physics);
	fprintf(fp, "IDC_CENTER_MASS = %d\n", ncenter_mass);
	fprintf(fp, "IDC_VERTEX_COLOR = %d\n", nvertex_color);
	fprintf(fp, "IDC_IK_EXT = %d\n", nik_extents);
	fprintf(fp, "IDC_SM_GRP = %d\n", nsm_groups);
	fprintf(fp, "IDC_EX_AVI = %d\n", nex_avi);
	fprintf(fp, "IDC_EX_TXT = %d\n", nex_txt);
	fprintf(fp, "IDC_NOT_UNIQUE = %d\n", nnon_unique_name);
	fprintf(fp, "IDC_ALLOW_LOSE_J = %d\n", nallow_loose_j);
	fprintf(fp, "IDC_ALLIGN_HEADING = %d\n", nallign_heading);
	fprintf(fp, "IDC_REL_ANIM = %d\n", nrelative_deformable);
	fprintf(fp, "IDC_SEL_ANIM_ONLY = %d\n", nsel_anim_only);
	fprintf(fp, "IDC_LOOSE_J = %d\n", nloose_j);
	fprintf(fp, "IDC_ROOT_ANIM = %d\n", nroot_anim);
	fprintf(fp, "IDC_IN_WORLD = %d\n", nin_world);
	fprintf(fp, "IDC_IGNORE_WARNINGS = %d\n", nignore_warnings);

	// LOD stuff
	fprintf(fp, "IDC_LOD_PERCENT = %f\n", nLodPercent);
	fprintf(fp, "IDC_LOD_CLOSEST = %f\n", nLodClosestDist);
	fprintf(fp, "IDC_LOD_FURTHEST = %f\n", nLodFurthestDist);
	fprintf(fp, "IDC_LOD_DROPOUT = %f\n", nLodDropOut);
	fprintf(fp, "IDC_MTL_BOUNDARY = %f\n", nMtlBoundary);
	fprintf(fp, "IDC_TXT_WEIGHT = %f\n", nTxtWeight);

	fprintf(fp, "IDC_ROTATE_X = %f\n", nadjust_rotate_x);
	fprintf(fp, "IDC_ROTATE_Y = %f\n", nadjust_rotate_y);
	fprintf(fp, "IDC_ROTATE_Z = %f\n", nadjust_rotate_z);

	fflush(fp);
	fclose(fp);
}

void StartMyTimer(void)
{
	QueryPerformanceCounter(&timer_start);
}

void StopMyTimer(const char *format, ...)
{
	va_list args;
	char buffer[256] = {0};

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	QueryPerformanceCounter(&timer_stop);

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	
	if(buffer[0])
	{
		printf("%s Time: %f seconds\n", buffer, 
			((double)timer_stop.QuadPart - (double)timer_start.QuadPart)/
			 (double)frequency.QuadPart);
	}
	else
	{
		printf("Time: %f\n", ((double)timer_stop.QuadPart - (double)timer_start.QuadPart)/
						(double)frequency.QuadPart);
	}
}


DWORD WINAPI ThreadProc(  LPVOID lpParameter )
{
	return MessageBox(NULL, (char*)lpParameter, "Message", MB_OK | MB_ICONSTOP);
	//ExitThread(0);
};

int Winprint(const char *format, ...)
{
  va_list args;
  static char buffer[256] = {0};

  va_start(args, format);
  //vsprintf(buffer, format, args); // ANSI
  _vsntprintf(buffer, 255, format, args);
  va_end(args);

  fprintf(stderr,"%s\n", buffer);
  fflush(stderr);

  int result = 0;
#ifdef _DEBUG
  if(!ignore_warnings)
  {
	  DWORD lpThreadId;
	  CreateThread(NULL, 1, ThreadProc, (LPVOID)buffer, 0, &lpThreadId);
	  Sleep(100);
  }

#else

  if(ignore_warnings && !strstr(buffer, "Error") && !strstr(buffer, "error"))
  {
	DWORD lpThreadId;
	CreateThread(NULL, 1, ThreadProc, (LPVOID)buffer, 0, &lpThreadId);
	Sleep(100);
  }
  else
  {
	result = MessageBox(NULL, buffer, "Message", MB_OK | MB_ICONSTOP);
  }
#endif

  return result;
}


void AsciiExp::RemoveVertexColors(INode *node)
{
	if(node)
	{
		BOOL needDel;
		TriObject* tri = NULL;
	
		ObjectState os ( node->EvalWorldState(0) );
		if (!os.obj)
		{
			goto remove_next_color;
		}
	
		{
			const SClass_ID sc_id = os.obj->SuperClassID();
			if (sc_id != GEOMOBJECT_CLASS_ID)
			{
				goto remove_next_color;
			}
		}

		// Targets are actually geomobjects, but we don't want them
		{
			const Class_ID c_id( os.obj->ClassID() );
			if (c_id == Class_ID(TARGET_CLASS_ID, 0))
			{
				goto remove_next_color;
			}
		}

		tri = GetTriObjectFromNode(node, 0, &needDel);
		if (!tri)
		{
			goto remove_next_color;
		}

		if (needDel) 
		{
			tri->DeleteMe();
			goto remove_next_color;
		}

		{
			Mesh* mesh = &(tri->mesh);

			// remove vertex colors
			mesh->setNumVertCol(0, FALSE);
			mesh->setNumVCFaces(0, FALSE, mesh->getNumFaces());

			mesh->vcFace = NULL;
			mesh->vertCol = NULL;
			mesh->numCVerts = 0;

			node->InvalidateWS();
			ip->RedrawViews(ip->GetTime());
		}
		

remove_next_color:


		for(int i = 0; i < node->NumberOfChildren(); i++)
		{
			RemoveVertexColors(node->GetChildNode(i));
		}
	}
}

#pragma warning( pop )

