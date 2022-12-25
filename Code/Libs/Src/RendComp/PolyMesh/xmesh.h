//$Header: /Libs/dev/Src/RendComp/PolyMesh/xmesh.h 20    3/10/00 5:51p Mstembera $
// Copyright (C) 1997 Digital Anvil, Inc.

#ifndef __XMESH_H
#define __XMESH_H

#include "FileSys.h"
#include "Vector.h"
#include "FaceProp.h"
#include "material.h"
#include "facegroup.h"
#include "mesh.h"

#include "ITextureLibrary.h"

#include "IMaterialManager.h"

//

#define MAX_SPLIT_POLYS 50
struct Split
{
	unsigned int	f_cnt[2];
	int				f[2][MAX_SPLIT_POLYS*3*2];
	int				t_verts[2][MAX_SPLIT_POLYS*3*2];
	int				n[2][MAX_SPLIT_POLYS*2];
	int				orig[2][MAX_SPLIT_POLYS*2];

	unsigned int	v_cnt;
	Vector			v[MAX_SPLIT_POLYS*2];
	TexCoord		t[MAX_SPLIT_POLYS*2];
	TexCoord		t2[MAX_SPLIT_POLYS*2];
	int				vn[MAX_SPLIT_POLYS*2];
	float			vD[MAX_SPLIT_POLYS*2];
	

	Split::Split()
	{
		v_cnt=f_cnt[0]=f_cnt[1]=0;
	}

};

//

bool pm_simple_map_gen( TC_UVGENERATORCONTEXT *ctx, void *user_context );

//

struct XMesh : public Mesh
{
public:

//	ITextureLibrary *txmlib;

	void compute_bounds (void);
	void compute_centroid (void);
	void compute_face_areas (void);
	float XMesh::compute_face_area (const FaceGroup *fg, const int fid) const;
	void sort_faces_by_area (void);	// largest to smallest

public:

// Fills in array of 8 vertices of bounding box (in object space).
	void get_bounding_box(SINGLE box[6]) const;
	SINGLE compute_radius (void) const;
	void compute_sphere (void);
	bool expand_bounding_box (float* box) const;

	struct FaceIndex
	{
		int index;
		FaceIndex* next;
		FaceIndex* prev;
	};

	struct GroupIndex
	{
		int index;
		LList<FaceIndex> faces;
		
		GroupIndex* next;
		GroupIndex* prev;
	};

	XMesh* build_sub_mesh2 (const LList<GroupIndex>& groups, const Split *split_list, U8 which) const;
	void XMesh::AssertMesh(void) const;

	bool split_mesh (const Vector& v0, float d, XMesh*& r2, XMesh*& r1,U32 split_polys=0, BOOL32 keep_polys=0) const;

	enum PlaneStatus
	{
		PS_FRONT,
		PS_BACK,
		PS_ON,
		PS_SPAN
	};

	struct PointStatus
	{
		PlaneStatus stat;
		SINGLE r;
	};

	void split_face (unsigned int idx,Split *split,const PointStatus* ps, bool bExact,int group = -1) const;

	PlaneStatus check_face(const FaceGroup& fg, int f_id, const PointStatus* ps) const;

	XMesh(void)
	{
		memset(this, 0, sizeof(*this));

		tc_user_generator = pm_simple_map_gen;
		tc_user_generator_context = NULL;
	}

	~XMesh (void);

	bool duplicate_mesh( XMesh **out_mesh, ITextureLibrary *texturelibrary,IMaterialManager* matMan)
	{
		if( (*out_mesh = new XMesh) == NULL ) {
			return false;
		}
		
		(*out_mesh)->copy_full_mesh( *this, texturelibrary );
		
		return true;
	}


	// manipulate continuous lod meshes
	void copy_partial_mesh(const XMesh & source);
	void copy_full_mesh(const XMesh & source, ITextureLibrary *txmlib );
	void copy_lod_mesh(const XMesh & source);

	void release_partial_mesh(void);
	void release_full_mesh(void);
	//void release_lod_mesh(void);

	bool read (IFileSystem* fs, ITextureLibrary *txmlib,IMaterialManager * matman, U32 cont_lod_mode );
	HRESULT read_face_groups( IFileSystem *parent );
	HRESULT read_vertex_data( IFileSystem *parent );
	HRESULT read_lod_library( IFileSystem *parent );
	HRESULT read_edge_data( IFileSystem *parent );
	HRESULT read_uv_channels( IFileSystem *parent );
	HRESULT read_material_library( IFileSystem *parent, ITextureLibrary *texturelib, IMaterialManager* matMan );
	HRESULT read_material( IFileSystem *parent, ITextureLibrary *texturelib, int material_entry, const char *material_name, Material *material_list, int material_cnt );
	HRESULT read_material_new( IFileSystem *parent, ITextureLibrary *texturelib, Material *material );
	HRESULT read_material_old( IFileSystem *parent, ITextureLibrary *texturelib, Material *material );
	HRESULT read_material_property( IFileSystem *parent, ITextureLibrary *texturelib, const char *name, U32 num_constants, float *default_constants, float *out_constants, ITL_TEXTURE_REF_ID *out_map_ref_id, U32 *out_map_flags, float *out_map_blend );

};

#endif
