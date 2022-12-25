//************************************************************************** 
//* Asciiexp.h	- Ascii File Exporter
//* 
//* By Christer Janson
//* Kinetix Development
//*
//* January 20, 1997 CCJ Initial coding
//*
//* Class definition 
//*
//* Copyright (c) 1997, All Rights Reserved. 
//***************************************************************************

#ifndef __ASCIIEXP__H
#define __ASCIIEXP__H

#include "Max.h"
#if MAX_RELEASE >= 3000
#include "iparamb2.h"
#endif
#include "resource.h"
#include "istdplug.h"
#include "stdmat.h"
#include "decomp.h"
#include "shape.h"
#include "interpik.h"
#include "asciitok.h"
#include "bmmlib.h"
#include "phyexp.h"
#include "bipexp.h"
//#include "BonesPro.H"
#include "notetrck.h"

#include "3db.h"
#include "cmp.h"
#include "channeleventtypes.h"

#include "surf_api.h"
#include "libver.h"

#define FixupName(X) ((X))

typedef enum {BAD_DEF, PHYSIQUE, BONES_PRO, SELF} def_mod_type;

extern char dest_path[256];
extern char win_message[256];
extern int default_mat_flag;

extern bool exporting_deformable;
extern int txt_flag;

//extern ClassDesc* GetAsciiExpDesc();
extern TCHAR *GetString(int id);
extern HINSTANCE hInstance;

#define VERSION			200					// Version number * 100
#define CFGFILENAME		_T("3DBEXP.CFG")	// Configuration file

typedef struct {
	char *name;
	TimeValue t; // frame id
} Note;

typedef struct {
	float magnitude;
	float relative;
	INode  *node;
}bone_contrib;

typedef struct {
	object obj;
	INode *skin_node;
	int vertex_count;
	int bone_count;
	bone_contrib **vertex_bone_matrix;
}max_skin;

// these match the bone ID's assigned by the artists
typedef enum {UNKNOWN_BONE=0, PRO_BONE=1, PHYS_BONE=2, MESH_BONE=3, ROOT_BONE=4, SKIRT_BONE=5,
				OPEN_BONE=6, UV_BONE=7, CHILD_BONE=8, PARENT_BONE=9, REYE_BONE=RIGHT_EYE, LEYE_BONE=LEFT_EYE,
				END7=INT_MAX} bone_type;

typedef struct {
	INode *node;
	INode *parent;
	int depth;
	int con_type;
	bone_type type;
	int frame_count;
	Frame *frame_list;
} max_bone;

typedef struct {
	int count;
	max_bone *bone_list;
}bone_lib;

struct INodeList {

	int count;
	int current;
	INode **list;

	INodeList()
	{
		count = 0;
		current = 0;
		list = NULL;		
	}

	~INodeList()
	{
		Free(list);
	}

	int AddINode(INode *node)
	{
		if(node)
		{
			count++;
			list = (INode**)Realloc(list, count * sizeof(INode*));
			list[count-1] = node;

			return count - 1;
		}

		return -1;
	}

	void AddINodeTree(INode *node)
	{
		if(node)
		{
			AddINode(node);

			for(int i = 0; i < node->NumberOfChildren(); i++)
			{
				AddINodeTree(node->GetChildNode(i));
			}
		}
	}

	inline INode* operator [] (const int i)
	{
		return list[i];
	}

	void Reset(INode *node)
	{
		count = 0;
		current = 0;
		Free(list);
		AddINodeTree(node);
	}
};

void AdjustHeading(Matrix3 & tm, INode *node, const TimeValue t1, const TimeValue t2);
int GetRGBID(VRGB **rgb, const int id, const int length);
PersistTransform Matrix3_to_PersistTransform(const Matrix3 & m3);
void TransformUV(float *u, float *v, const float mu, const float mv, const float mw, const float mh);
Modifier* GetUVWModifier(INode *node);
void ExportRootTransform(INode *node, PersistTransform *xform, TimeValue t);
//void CalcMeshBoneRigidbodies(bone_lib *bl, const object& obj, float density);
bool HaveNodeID(INode *node, unsigned int id);
INode* GetParentBone(const bone_lib *bl, INode *bone_node);
INode* GetNodeByName(INode *node, const char *name);
INode* GetClosestLODNode(INode *group_node);
void GetLodDist(INode *node, float *min, float *max);
bool HasLodChildren(INode *node);
bool HasLod(INode *node);
//void AddHeadNeck(bone_lib *bl, max_skin *ms);
int IsPlaceholder(INode *node);
bool IsHP(INode *node, TimeValue t);
//INode* GetBonesProNodeVer2(INode *node);
INode* GetBonesProNodeVer1(INode *node);
file_node *CreateBones(bone_lib *bl);
int GetBoneID(const bone_lib *bl, INode *bone_node);
int GetFirstBoneType(const bone_lib *bl, const bone_type type);
int CompareBonesByDepth(const void *pt1, const void *pt2);
int CompareBonesAlphabetically(const void *pt1, const void *pt2);
//int CompareElements(const void *pt1, const void *pt2);
//int AppendBone(bone_lib *bl, INode *bone_node, int depth, INode *parent, int con_type);
//void SortBones(bone_lib *bl, int verbose_level);
void CheckBoneIDs(bone_lib bl, unsigned int id);
//void TestBoneHierarchy(const bone_lib *bl);
//void TransformVertsToLocal(max_skin *ms, bone_lib *bl, INode *default_bone, TimeValue t);
void InitMaxBone(max_bone *mb);
void FreeMaxBone(max_bone *mb);
//void InitMaxSkin(max_skin *ms, txt_lib *tl, anim_txt_lib *atl, mtl_lib *ml);
void InitMaxSkin(max_skin *ms, mtl_lib *ml);
void FreeMaxSkin(max_skin *ms);
void LoadObject(object *obj, char *file_name);
int GetDepth(INode *node);
void InitBoneLib(bone_lib *b_lib);
void FreeBoneLib(bone_lib *blib);
Modifier* FindPhysiqueModifier (INode* nodePtr);
Modifier* FindBonesModifier (INode* nodePtr);

Point3 SwitchCoord(const Point3 & p, INode *from, INode *to, TimeValue t);
Point3 SwitchNormal(const Point3 & p, INode *from, INode *to, TimeValue t);

//void SortBoneWeightMatrix(max_skin *ms);
INode* GetMyObjParent(INode *node);
Matrix3 GetMyNodeTM(INode *node, TimeValue t);
Matrix3 GetMyObjTMAfterWSM(INode *node, TimeValue t);
void AdjustMyNodeTM(Matrix3 & tm, INode *node);
Matrix3 GetMyLocalNodeTM(INode* node, TimeValue t, INode *parent);
Matrix3 GetFullLocalNodeTM(INode* node, TimeValue t, INode *parent);
Point3 GetPivot(INode *node, TimeValue t, INode *parent);
BOOL Win32Exec(LPSTR lpCmdLine, BOOL bWait);
void GetDofData(INode *node, DofData *data, TimeValue t, INode *parent);
Object* FindNURBRefObject(INode *node);
void AddDebugMtls(mtl_lib *ml);
BitmapTex* GetMap(StdMat *std, const int map_type);
void GetMtlProperty(StdMat *std, FACE_PROPERTY *property);
void GetUVChannels(Mesh *mesh,
				   const TVFace **f_uv0, const TVFace **f_uv1,
				   const Point3 **v_uv0, const Point3 **v_uv1);
BOOL CheckIdentity(Matrix3 *m);
void DumpMatrix3(Matrix3* m);
void TransposeMatrix3(Matrix3 & m);
void CleanMatrix3(Matrix3 & m);
//void GetNoteTag(INode *node);
DefNoteTrack* GetNoteTrack(INode *node);
void GetNotes(INode *node, int *num_notes, Note **note_list, DefNoteTrack *nt);
int CompareNotes(const void *pt1, const void *pt2);
void InsertAnim(CompoundObject *c_obj, INode *node, NamedScript & script, NamedChannel & n_channel,
				const TimeValue frame_id);
void CheckTransforms(INode *node);
ULONG MyGetGBufID(INode *node);
void __cdecl DumpParam2Block( Animatable *an_obj, void *user_data );
void EnumModifiers( INode *node, void (__cdecl *fp)(Animatable *mod, void *user_data), void *user_data);

// character studio export
//INode* GetPhysiqueNode(INode *node);

void StopMyTimer(const char *format, ...);
void StartMyTimer();
void FixBiped(Interface *ip);

//

class ExpOptions
{
public:
	char file_name[256];
	float	nDensity;
	float	nScale;
	int		nmip_flag;
	int		nflag_565;
	int		nflag_888;
	int		ndither_flag;
	int		nsplit_flag;
	int		ndefault_mat_flag;
	int		nexport_mesh;
	//int		nexport_skeleton;
	int		nexport_animation;
	int		nexport_lights;
	int		nexport_cameras;
	int		nexport_selected;
	//int		nbiped_y_up;
	int		nignore_warnings;

	float	nLodPercent;
	float	nLodClosestDist;
	float	nLodFurthestDist;
	float	nLodDropOut;
	//int		nLodCount;
	float	nMtlBoundary;
	float	nTxtWeight;
	
	float	nadjust_rotate_x;
	float	nadjust_rotate_y;
	float	nadjust_rotate_z;
	int		nno_physics;
	int		ncenter_mass;
	int		nvertex_color;
	int		nik_extents;
	int		nsm_groups;
	int		nex_avi;
	int		nex_txt;
	int		nnon_unique_name;
	int		nallow_loose_j;
	int		nallign_heading;
	int		nrelative_deformable;
	//int		nhead;
	int		nsel_anim_only;
	int		nloose_j;
	int		nroot_anim;
	int		nin_world;

	int		nsave_options_to_max_file;

	ExpOptions( void )
	{
		file_name[0] = 0;
		Init( );
	}

	void Init( void );
	void Read(HWND hWnd);
	void Write(HWND hWnd);
};

// This is the main class for the exporter.

void __cdecl ExitCleanup(void);
class AsciiExp : public SceneExport {
public:
	AsciiExp( ExpOptions & _options );
	~AsciiExp();

	void RemoveVertexColors(INode *node);
	bool GetBatchFileName( HWND hWnd );
	void DebugFn(INode* node);
	int ExportDeformable(void);
	void GetBones(bone_lib *bl);
	bool OrganizeBones(bone_lib *bl);
	void SwapBones(bone_lib *bl, const int id1, const int id2);
	void ExportBoneVertices(object * obj, bone_lib *bl, const TimeValue mesh_time);
	void ExportBoneVertices(DAMesh & da_mesh, bone_lib *bl, const TimeValue mesh_time);
	void ExportBoneNurbCV(object * obj, const bone_lib *bl, const TimeValue mesh_time);
	void ExportBoneBezCV(Bezier_mesh * b_mesh, const bone_lib *bl, const TimeValue mesh_time);
	void ExportBoneUVVertices(object *obj, const bone_lib *bl, const TimeValue mesh_time,
							  const bone_type type, const int flags);

	void AppendPhysiqueVertex(object * obj, const int v_id, const bone_lib *bl, Modifier *mod, int *v_step,
								const TimeValue mesh_time);
	void AppendPhysiqueVertex(DAMesh & da_mesh, const int v_id, const bone_lib *bl, Modifier *mod, int *v_step,
								const TimeValue mesh_time);
	void AppendPhysiqueNurbCV(nurb * nrb, const bone_lib *bl, Modifier *mod, const TimeValue mesh_time);
	void AppendPhysiqueBezCV(Bezier_mesh * b_mesh, const int api_node_id, const bone_lib *bl, Modifier *mod, const TimeValue mesh_time);
	/*
	void AssignAuxCV(Bezier_mesh *b_mesh, const bone_lib *bl, const TimeValue mesh_time);
	void RecomputeBVPos(const bone_lib *bl, Bezier_mesh *b_mesh,
					  bone_vertex *bv_dst, const bone_vertex *bv1, const bone_vertex *bv2,
					  const float w1, const float w2, int aux_id, const TimeValue mesh_time);
	*/
	void AppendBonesProVertex(object * obj, const int v_id, bone_lib *bl, Modifier *mod, int *v_step,
								const TimeValue mesh_time);
	void AppendBonesProVertex(DAMesh & da_mesh, const int v_id, bone_lib *bl, Modifier *mod, int *v_step,
								const TimeValue mesh_time);

	void AppendSelfVertex(object * obj, const int v_id, const bone_lib *bl, int *v_step,
							const TimeValue mesh_time);
	void AppendSelfVertex(DAMesh & da_mesh, const int v_id, const bone_lib *bl, int *v_step,
							const TimeValue mesh_time);
	void AppendSelfCV(nurb * nrb, const bone_lib *bl, const TimeValue mesh_time);
	void CleanVertices(vertices * v);

	//static void __cdecl ExitCleanup(void);
	void ExportLod(lod_object *l_obj, INode *node);
	void ExportGlobalEvents(CompoundObject *c_obj);
	//void ExportBoneRigidBodies(bone_lib *bl);
	void ExportMeshHP(CompoundObject *c_obj, const bone_lib *bl);
	
	// void ConnectBones(CompoundObject *cmp_obj, max_skin *ms, bone_lib *bl);
	//void AugmentBones(max_skin *ms, bone_lib *bl);
	void ExportBoneConnections(CompoundObject *cmp_obj, bone_lib *bl);
	void ExportConnection(INode *node, CompoundObject *c_obj, INode *parent, int con_type,
		 int n_frames, Frame * frame_list);
	int MakeDofConnection(CompoundObject *c_obj, INode *node, INode *parent, int con_type);
	void MakeRevAnim(CompoundObject *c_obj, INode *node, INode *parent, int in_n_frames, Frame * in_frame_list);
	void MakePrisAnim(CompoundObject *c_obj, INode *node, INode *parent, int in_n_frames, Frame * in_frame_list);
	void MakeLooseAnim(CompoundObject *c_obj, INode *node, INode *parent, int in_n_frames, Frame * in_frame_list);
	void MakeRootAnim(CompoundObject *c_obj, INode *node, int in_n_frames, Frame * in_frame_list);

	void InsertAnim(CompoundObject *c_obj, INode *node, NamedScript & script, NamedChannel & n_channel,	const TimeValue frame_id);

	int ReadFrames(INode *node, Frame* frame_list, int type, INode *parent, bone_lib *bl);
	int GetFrame(INode *node, INode *parent, Frame & frame, const TimeValue t,
					   const Matrix3 & inv_frame0_tm, int type, bool root_flag);
	TimeValue GetEndTime(INode *node);
	
	//void FindPhysiqueBones(max_skin *ms, const bone_lib *bl);
	//void FindBonesByID(bone_lib *bl, INode *node, unsigned int id, int con_type);
	static bool Exportable(INode *node);
	
	// SceneExport methods
	int    ExtCount();     // Number of extensions supported 
	const TCHAR * Ext(int n);     // Extension #n (i.e. "ASC")
	const TCHAR * LongDesc();     // Long ASCII description (i.e. "Ascii Export") 
	const TCHAR * ShortDesc();    // Short ASCII description (i.e. "Ascii")
	const TCHAR * AuthorName();    // ASCII Author name
	const TCHAR * CopyrightMessage();   // ASCII Copyright message 
	const TCHAR * OtherMessage1();   // Other message #1
	const TCHAR * OtherMessage2();   // Other message #2
	unsigned int Version();     // Version number * 100 (i.e. v3.01 = 301) 
	void	ShowAbout(HWND hWnd);  // Show DLL's "About..." box
#if	MAX_RELEASE == 2500
	int		DoExport(const TCHAR *name,ExpInterface *ei,Interface *inter, BOOL suppressPrompts=FALSE);
#elif MAX_RELEASE >= 3000
	int		DoExport(const TCHAR *name,ExpInterface *ei,Interface *inter, BOOL suppressPrompts=FALSE, DWORD __options=0);
#endif
	int		ExportCurrentScene(const TCHAR *full_name);

	// Other methods

	// Node enumeration
	BOOL	nodeEnum(INode* node, int indentLevel, CompoundObject *c_obj);
	void	PreProcess(INode* node, int& nodeCount);
	INode*	GetFirstInstance(INode *node);
	INode*	GetFirstReference(INode *node);

	// High level export
	//void	ExportPhysique(INode *skin_node);
	//void  ExportBonesPro(INode *face_node);
	//void	AddKeyframes0(const bone_lib& bl);
	void	AssignFaceVertices(max_skin *ms, bone_lib *bl); //  keep for undo
	BOOL	Export3DB(INode* node, object *obj, const object_type mesh_type, INode *to_node,
						const TimeValue t); 
	void	GetFixed(INode *node, Fix *fixed, INode *parent);
	void	GetFixedHP(INode *node, Fix *fixed, INode *parent);

	// Mid level export
	bool	CheckForUVAnim(INode* node, bool **uv_used_list);
	void	ExportUVAnim(object *obj, INode* node, INode *to_node, const bool *uv_used_list);
						
	void	ExportMesh(object *obj, INode* node, const TimeValue t, INode *to_node);
	void	GetPolyUV(const TVFace *tv_face, const Point3 *tv_vert, const int f_id,
				const int vx1, const int vx2, const int vx3,
				const bool *uv_used_list, const object_type type, float *uv, int *api_uv_id,
				const int channel_id);
	void	ExportNURBS(object *obj, INode* node, TimeValue t, INode *to_node);
	void	ExportPatch(object *obj, INode* node, TimeValue t, INode *to_node);
	void	ExportHP(object *obj, INode* node, TimeValue t);
	void	ExportUserProperties(object *obj, INode *node);
	int		ExportCamera(INode* node, object *obj);
	int		ExportLight(INode *node, object *obj);
	//int		ExportMAXMaterial(Mtl *mat, object *obj, const TimeValue t, INode *node);
	int		ExportCQ2Material(Mtl *mat, object *obj, const TimeValue t, INode *node);
	void	ExportIKJoints(INode* node, int indentLevel);

	// Low level export
	void	DumpJointParams(JointParams* joint, int indentLevel);
	void	DumpMatrix3(Matrix3* m, int indentLevel);

//	void	defineTexture(BitmapTex *tex, BitmapTex *o_tex, mtl_property *mp);
//	int		CheckTxtAlphaConsistency(BitmapTex *tex, BitmapTex *alpha_tex, mtl_property *mp);
//	bool	IsAnimatedTxt(BitmapTex *tex);
//	void	ExportTxtProperties(BitmapTex *tex, mtl_property *mp);
//	bool	UseTexAlpha(BitmapTex *tex);

//	int		ExportAnimatedTexture(BitmapTex *tex, BitmapTex *alpha_tex, mtl_property *mp,
//									const char *name);
//	int		ExportTextureFrame(mtl_property *mp, BitmapTex *tex, BitmapTex *alpha_tex,
//						const char *txt_name, TimeValue frame_t, VRGB **in_rgb, VRGB **alpha_rgb,
//						int drop_mip);
//	void	ReadPixels(Bitmap *bmap, VRGB *rgb);
	//bool AsciiExp::IsDifferentBitmap(BitmapTex *tex, TimeValue t1, TimeValue t2);

	// Misc methods
	TCHAR*	GetMapID(Class_ID cid, int subNo);
	void	CommaScan(TCHAR* buf);
	BOOL	CheckForAnimation(INode* node, BOOL& pos, BOOL& rot, BOOL& scale, INode *parent);
	TriObject*	GetTriObjectFromNode(INode *node, TimeValue t, BOOL *deleteIt);
	PatchObject* AsciiExp::GetPatchObjectFromNode(INode *node, TimeValue t, int *deleteIt);
	// BOOL	IsKnownController(Control* cont);

	// Configuration methods
	TSTR	GetCfgFilename(void);
	//void	ReadConfig();
	//void	WriteConfig();
	
	// Interface to member variables
	inline TimeValue GetStaticFrame()		{ return nStaticFrame; }
	//inline TimeValue GetZeroFrame()			{ return TimeValue(0); }
	inline float	GetDensity()			{ return options.nDensity; }
	inline float	GetScale()				{ return options.nScale; }
	inline int		GetMipFlag()			{ return options.nmip_flag; }
	inline int		Get565Flag()			{ return options.nflag_565; }
	inline int		Get888Flag()			{ return options.nflag_888; }
	inline int		GetDitherFlag()			{ return options.ndither_flag; }
	inline int		GetSplitFlag()			{ return options.nsplit_flag; }
	inline int		GetDefaultMatFlag()		{ return options.ndefault_mat_flag; }
	inline int		GetMeshFlag()			{ return options.nexport_mesh; }
	//inline int		GetSkeletonFlag()		{ return options.nexport_skeleton; }
	inline int		GetAnimationFlag()		{ return options.nexport_animation; }
	inline int		GetLightsFlag()			{ return options.nexport_lights; }
	inline int		GetCamerasFlag()		{ return options.nexport_cameras; }
	inline int		GetSelectedFlag()		{ return options.nexport_selected; }
	//inline int		GetBipedYUpFlag()		{ return options.nbiped_y_up; }
	inline int		GetIgnoreWarningsFlag()	{ return options.nignore_warnings; }
	inline float	GetLodPercent()			{ return options.nLodPercent; }
	inline float	GetLodClosest()			{ return options.nLodClosestDist; }
	inline float	GetLodFurthest()		{ return options.nLodFurthestDist; }
	inline float	GetLodDropOut()			{ return options.nLodDropOut; }
	inline float	GetMtlBoundary()		{ return options.nMtlBoundary; }
	inline float	GetTxtWeight()			{ return options.nTxtWeight; }
	//inline int		GetLodCount()			{ return options.nLodCount; }
	inline float	GetAdjustRotateX()		{ return options.nadjust_rotate_x; }
	inline float	GetAdjustRotateY()		{ return options.nadjust_rotate_y; }
	inline float	GetAdjustRotateZ()		{ return options.nadjust_rotate_z; }
	inline int		GetNoPhysics()			{ return options.nno_physics; }
	inline int		GetCenterMass()			{ return options.ncenter_mass; }
	inline int		GetVertexColor()		{ return options.nvertex_color; }
	inline int		GetIkExt()				{ return options.nik_extents; }
	inline int		GetSmGrp()				{ return options.nsm_groups; }
	inline int		GetExAvi()				{ return options.nex_avi; }
	inline int		GetExTxt()				{ return options.nex_txt; }
	inline int		GetNonUniqueName()		{ return options.nnon_unique_name; }
	inline int		GetAllowLooseJ()		{ return options.nallow_loose_j; }

	inline int		GetAllignHeading()		{ return options.nallign_heading; }
	inline int		GetRelAnim()			{ return options.nrelative_deformable; }
	//inline int		GetHead()				{ return options.nhead; }
	inline int		GetSelAnimOnly()		{ return options.nsel_anim_only; }
	inline int		GetLooseJ()				{ return options.nloose_j; }
	inline int		GetRootAnim()			{ return options.nroot_anim; }
	inline int		GetInWorld()			{ return options.nin_world; }
	
	inline Interface*	GetInterface()		{ return ip; }

	inline void SetStaticFrame(TimeValue val)		{ nStaticFrame = val; }

	inline void SetDensity(float val)				{ options.nDensity = val; }
	inline void SetScale(float val)					{ options.nScale = val; }
	inline void SetMipFlag(int val)					{ options.nmip_flag = val; }
	inline void Set565Flag(int val)					{ options.nflag_565 = val; }
	inline void Set888Flag(int val)					{ options.nflag_888 = val; }
	inline void SetDitherFlag(int val)				{ options.ndither_flag = val; }
	inline void SetSplitFlag(int val)				{ options.nsplit_flag = val; }
	inline void SetDefaultMatFlag(int val)			{ options.ndefault_mat_flag = val; }
	inline void SetMeshFlag(int val)				{ options.nexport_mesh = val; }
	//inline void SetSkeletonFlag(int val)			{ options.nexport_skeleton = val; }
	inline void SetAnimationFlag(int val)			{ options.nexport_animation = val; }
	inline void SetLightsFlag(int val)				{ options.nexport_lights = val; }
	inline void SetCamerasFlag(int val)				{ options.nexport_cameras = val; }
	inline void SetSelectedFlag(int val)			{ options.nexport_selected = val; }
	//inline void SetBipedYUpFlag(int val)			{ options.nbiped_y_up = val; }
	inline void SetIgnoreWarningsFlag(int val)		{ options.nignore_warnings = val; }
	inline void SetLodPercent(float val)			{ options.nLodPercent = val; }
	inline void SetLodClosest(float val)			{ options.nLodClosestDist = val; }
	inline void SetLodFurthest(float val)			{ options.nLodFurthestDist = val; }
	inline void SetLodDropOut(float val)			{ options.nLodDropOut = val; }
	inline void SetMtlBoundary(float val)			{ options.nMtlBoundary = val; }
	inline void SetTxtWeight(float val)				{ options.nTxtWeight = val; }
	//inline void SetLodCount(int val)				{ options.nLodCount = val; }
	inline void SetAdjustRotateX(float val)			{ options.nadjust_rotate_x = val; }
	inline void SetAdjustRotateY(float val)			{ options.nadjust_rotate_y = val; }
	inline void SetAdjustRotateZ(float val)			{ options.nadjust_rotate_z = val; }
	inline void SetNoPhysics(int val)				{ options.nno_physics = val; }
	inline void SetCenterMass(int val)				{ options.ncenter_mass = val; }
	inline void SetVertexColor(int val)				{ options.nvertex_color = val; }
	inline void SetIkExt(int val)					{ options.nik_extents = val; }
	inline void SetSmGrp(int val)					{ options.nsm_groups = val; }
	inline void SetExAvi(int val)					{ options.nex_avi = val; }
	inline void SetExTxt(int val)					{ options.nex_txt = val; }
	inline void SetNonUniqueName(int val)			{ options.nnon_unique_name = val; }
	inline void SetAllowLooseJ(int val)				{ options.nallow_loose_j = val; }

	inline void SetAllignHeading(int val)			{ options.nallign_heading = val; }
	inline void SetRelAnim(int val)					{ options.nrelative_deformable = val; }
	//inline void SetHead(int val)					{ options.nhead = val; }
	inline void SetSelAnimOnly(int val)				{ options.nsel_anim_only = val; }
	inline void SetLooseJ(int val)					{ options.nloose_j = val; }
	inline void SetRootAnim(int val)				{ options.nroot_anim = val; }
	inline void SetInWorld(int val)					{ options.nin_world = val; }
		
	ExpOptions & options; // references AsciiExpDesc.options
private:

	TimeValue	nStaticFrame; // max(0, start)

	Interface*	ip;
	INodeList	inode_list;

	char batch_file_name[256];
};

#endif // __ASCIIEXP__H

