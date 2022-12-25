#ifndef ENG_H
#define ENG_H

#include "fdump.h"
#include "DACom.h"
#include "Engine.h"
#include "IAnim.h"
#include "LightMan.h"
#include "ITextureLibrary.h"
#include "IHardpoint.h"
#include "stddat.h"
#include "Physics.h"
#include "Collision.h"
#include "ICamera.h"
#include "rendpipeline.h"
#include "RPUL/MTPrimitiveBuilder.h"
#include "patchgroup.h"
#include "TextureCoord.h"
#include "Tfuncs.h"

#define RWM_DONT_TEXTURE	(1<<0)

#define MAX_SUB_DIV_CNT 3	// this makes 4^4 = 8x8 = 64 patches out of one patch
#define EDGE_LENGTH 9		// one side of 8 by 8

//#define MAX_SUB_DIV_CNT 2	// this makes 4^3 = 4x4 = 16 patches out of one patch
//#define EDGE_LENGTH 5		// one side of 4 by 4

typedef Vector Vector_4[4];
typedef float float_4[4];
typedef U8 Corner[6];		// 01 - horizontal \; 23 - vertical |; 34 - unused /

struct StitchEdge
{
	int pid1;				// source (uber) patch id 1
	int pid2;				// source (uber) patch id 2
	int el1[EDGE_LENGTH];
	int el2[EDGE_LENGTH];

	int depth1;
	int depth2;
};

namespace Deform
{
	extern ICOManager *			DACOM;
	extern IEngine *			ENG;
	extern ILightManager *		LIGHT;
	extern ITextureLibrary *	TXMLIB;
	//extern ITXMLib *			TXMLIB;
	extern IAnimation *			ANIM;
	extern IChannel *			CHANNEL;
	extern IHardpoint *			HARDPOINT;
	extern IPhysics *			PHYSICS;
	extern ICollision *			COLLIDE;
	extern IRenderPipeline *	PIPE;
	extern IRenderPrimitive *	BATCH;

	extern MTPrimitiveBuilder	pb;

// buffers shared by polygon meshes and patches
	extern int						vertex_pool_len;
	extern int						vertex_pool_index;
	extern MTVERTEX *				vertex_pool;
	extern Vector *					normal_pool;
	extern int *					normal_pool_cnt;	// number of contributing patches
	extern U32 *					normal_index_pool;
	extern LightRGB *				light_pool;
	
	extern int						index_list_len;
	extern int						index_list_index;
	extern U16 *					index_list;
	extern U16 *					vertex_slot;

// buffers specific to patches
	// pool of patches being rendered
	extern int						patch_list_len;
	extern int						patch_list_index;
	extern BezierPatch *			patch_list;
	extern Vector_4 *				patch_normals;
	extern float_4	*				patch_D;
	extern Corner *					patch_corners;
	extern int	*					patch_depth;
	extern bool *					patch_alt_tri_style;

	// patch edges used to stitch cracks
	extern int						edge_list_len;
	extern int						edge_list_index;
	extern StitchEdge *				edge_list;

	// scratch buffers
	extern int						scratch_pool_len;
	extern int						scratch_xyz_index;
	extern Vector *					scratch_xyz;
	extern int *					scratch_xyz_rp_idx;
	extern int *					scratch_xyz_uv_idx;
	extern int						scratch_uv_index;
	extern TexCoord *				scratch_uv;

	extern int						sub_div_cnt;
	extern int						back_face_cull;
	extern float					pixel_error;

// state flags
	extern bool						active;

	extern U32						default_material_flags;
	extern U32						device_supports_uvchannel1;
	extern U32						device_num_tss;

	extern U32						specular_mode;		// 0, 1, or 2
	extern char						specular_texture_name[64];

	extern U32						diffuse2_fallback_blend[2];	// src,dst framebuffer blend modes for Diffuse1*Diffuse2
	extern U32						emissive_fallback_blend[2];	// src,dst framebuffer blend modes for + Emissive
	extern U32						specular_fallback_blend[2];	// src,dst framebuffer blend modes for + Specular

	extern int						rgb_count;
	extern LightRGB *				rgb;

	extern int						double_count;
	extern int *					DoubleSided;

	extern bool						got_ini_info;

	void DebugPrint (char *fmt, ...);
	void TrapFpu(bool on);
	void get_ini_info( void );

	void delete_pools( void );
	void verify_pools(const int size);
	void delete_lists( void );
	void verify_lists(const int size);
	void delete_scratch_pools( void );
	void verify_scratch_pools(const int size);
	void delete_patch_lists( void );
	void verify_patch_lists(const int size);
	void delete_edge_lists( void );
	void verify_edge_lists(const int size);

	void EvalPatches( const PatchGroup & group );
	void SplitPatches( const PatchGroup & group, const float tolerance );
	void CompPatchAux(BezierPatch & patch);
	void GetPatchError( const int id, float err[3], Vector & normal, float & D );
	void CalcPatchPlane( const int pid, Vector & normal, float & D );
	int SplitPatch4Vertical( const int id1, const int id2, const int src_pid );
	int SplitPatch4Horizontal( const int id1, const int id2, const int src_pid );
	int SplitPatch3( const int id1, const int id2, const int direction, const int src_pid );
	void CalcInteriorPoints( const BezierPatch & patch );
	void CalcPatchNormals( const int pid );
	void CheckEdges( const PatchGroup & group );
};

//

#endif