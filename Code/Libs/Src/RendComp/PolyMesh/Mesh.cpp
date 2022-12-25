// $Header: /Libs/dev/Src/RendComp/PolyMesh/Mesh.cpp 59    3/10/00 5:51p Mstembera $
// Copyright (C) 1997 Digital Anvil, Inc.

#define PCI

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

#define MATERIAL_H_INCLUDE_LOAD_CODE 1

#include "DACOM.h"
#include "tempstr.h"
#include "TSmartPointer.h"

#include "FileSys_Utility.h"
#include "ITextureLibrary.h"
#include "IRenderComponent.h"
#include "renderer.h"

#include "DAVariant.h"
#include "Mesh.h"


#include "XMesh.h"


#undef MATERIAL_H_INCLUDE_LOAD_CODE 



#include <stdio.h>
#include <float.h>

#include "RPUL.h"
#include "IMaterialManager.h"



#define S_WARN_OLD_FORMAT MAKE_HRESULT( SEVERITY_SUCCESS, 0, 1 )


#ifndef NDEBUG
	#define CHECK(x)		\
	{						\
		int result = (x);	\
		ASSERT (result);	\
	}
#else								
	 #define CHECK(x)	(void)(x);
#endif


void XMesh::compute_sphere (void)
{
	if(object_vertex_cnt < 1)
	{
		sphere_center.zero();
		radius = 0.0f;
		return;
	}

	int i;
	int min_xi = 0, max_xi = 0;
	int min_yi = 0, max_yi = 0;
	int min_zi = 0, max_zi = 0;
	// initial guess
	for(i = 1; i < object_vertex_cnt; i++)
	{
		if(object_vertex_list[i].x < object_vertex_list[min_xi].x)
		{
			min_xi = i;
		}
		else
		if(object_vertex_list[i].x > object_vertex_list[max_xi].x)
		{
			max_xi = i;
		}

		if(object_vertex_list[i].y < object_vertex_list[min_yi].y)
		{
			min_yi = i;
		}
		else
		if(object_vertex_list[i].y > object_vertex_list[max_yi].y)
		{
			max_yi = i;
		}

		if(object_vertex_list[i].z < object_vertex_list[min_zi].z)
		{
			min_zi = i;
		}
		else
		if(object_vertex_list[i].z > object_vertex_list[max_zi].z)
		{
			max_zi = i;
		}
	}
	
	float dx = object_vertex_list[max_xi].x - object_vertex_list[min_xi].x;
	float dy = object_vertex_list[max_yi].y - object_vertex_list[min_yi].y;
	float dz = object_vertex_list[max_zi].z - object_vertex_list[min_zi].z;
	int mini, maxi;
	if( dx >= dy && dx >= dz)
	{
		mini = min_xi;
		maxi = max_xi;
	}
	else
	if( dy >= dx && dy >= dz)
	{
		mini = min_yi;
		maxi = max_yi;
	}
	else
	{
		mini = min_zi;
		maxi = max_zi;
	}

	Vector delta (object_vertex_list[maxi] - object_vertex_list[mini]);
	sphere_center = object_vertex_list[mini] + .5f * delta;
	radius = 0.5f * delta.magnitude();
	float rad_sq = radius * radius;
	
	const Vector * vp = object_vertex_list;
	// refine guess
	for (i = 0; i < object_vertex_cnt; i++, vp++)
	{
		delta = *vp - sphere_center;

		float old_sq = dot_product(delta, delta);
		if (old_sq > rad_sq)
		{
		//
		// Point is outside current bounding sphere, need to update.
		//
			// save last sphere
			Vector min_center = sphere_center;
			float min_rad = radius;

			// update
			float old = (float) sqrt(old_sq);
			radius = 0.5f * (radius + old);
			rad_sq = radius * radius;

			float offset = old - radius;
			sphere_center = (radius * sphere_center + offset * *vp) / old;
		}
	}
}

SINGLE XMesh::compute_radius (void) const
{
	SINGLE big = 0;

	Vector *stop = object_vertex_list + object_vertex_cnt;
	for (Vector *v=object_vertex_list; v < stop; v++)
	{
		SINGLE r2 = v->magnitude_squared();
		if (r2 > big)
			big = r2;
	}

	return sqrt(big);
}

void XMesh::compute_centroid (void)
{
	centroid = Vector(0, 0, 0);

	for (Vector* v = object_vertex_list; 
			v < object_vertex_list + object_vertex_cnt; 
			v++)
	{
		centroid += *v;
	}

	centroid /= object_vertex_cnt;
}

inline void Swap32(void * const a, void * const b)
{	
	if(a != b)
	{
		*(unsigned long*)a ^= *(unsigned long*)b;
		*(unsigned long*)b ^= *(unsigned long*)a;
		*(unsigned long*)a ^= *(unsigned long*)b;
	}
}

struct IndexedFaceGroup : public FaceGroup
{
	int *indices;
};

void swap_faces(void *vfg, const int id1, const int id2)
{
	if(id1 != id2)
	{
		IndexedFaceGroup *fg = (IndexedFaceGroup*)vfg;

		Swap32(fg->face_vertex_chain + 3*id1, fg->face_vertex_chain + 3*id2);
		Swap32(fg->face_vertex_chain + 3*id1+1, fg->face_vertex_chain + 3*id2+1);
		Swap32(fg->face_vertex_chain + 3*id1+2, fg->face_vertex_chain + 3*id2+2);

		Swap32(fg->face_normal + id1, fg->face_normal + id2);
		Swap32(fg->face_D_coefficient + id1, fg->face_D_coefficient + id2);
		Swap32(fg->face_properties + id1, fg->face_properties + id2);
		Swap32(fg->face_area + id1, fg->face_area + id2);

		Swap32(fg->indices +  id1, fg->indices + id2);
	}
}

// largest to smallest
int compare_faces_by_area(const void *pt1, const void *pt2, const void *)
{
 
  if(*(float*)pt1 < *(float*)pt2)
  {
	  return 1;
  }
  else
  if(*(float*)pt1 > *(float*)pt2)
  {
	  return -1;
  }
  
  return 0;
}

extern void  __cdecl sortQ(void *, size_t, size_t, 
						 int (__cdecl *)(const void *, const void *, const void *),
						 const void *comp_lib,
						 void (__cdecl *swap)(void *, const int id1, const int id2),
						 void *swap_lib);

// largest to smallest
void XMesh::sort_faces_by_area (void)
{
	for(int gid = 0; gid < face_group_cnt; gid++)
	{
		FaceGroup *fg = face_groups + gid;

		ASSERT_FATAL(fg->face_area);

		if(fg->face_cnt > 1)
		{
			// NOTE: the exporter now sorts faces but some old objects are still unsorted
			// qsort has a O(n^2) when already sorted so we check first
			bool sorted = true;
			for(int fid = 0; fid < fg->face_cnt - 1; fid++)
			{
				// they don't have to be sorted exactly
				if(fg->face_area[fid] < 0.999f * fg->face_area[fid + 1])
				{
					sorted = false;
					break;
				}
			}

			if(!sorted)
			{
				IndexedFaceGroup ifg;
				memcpy(&ifg, fg, sizeof(*fg));
				ifg.indices = new int[ifg.face_cnt];

				int fid;
				for(fid = 0; fid < ifg.face_cnt; fid++)
				{
					ifg.indices[fid] = fid;
				}

				// randomize faces in case they are almost sorted to avoid O(N^2) qsort
				for(fid = 0; fid < ifg.face_cnt; fid++)
				{
					swap_faces(&ifg, fid, (rand() * (ifg.face_cnt - 1)) / RAND_MAX);
				}

				sortQ(ifg.face_area, ifg.face_cnt, sizeof(float), 
					compare_faces_by_area, NULL, 
					swap_faces, &ifg);

				// reindex lod information
				if(lod_step_list)
				{
					int *tmp = new int[ifg.face_cnt];
					for(int fid = 0; fid < ifg.face_cnt; fid++)
					{
						tmp[ifg.indices[fid]] = fid;
					}

					for(int m_fid = 0; m_fid < lod_morphed_face_cnt; m_fid++)
					{
						if(lod_group_face_list[m_fid] == gid &&
						   lod_morphed_face_list[m_fid] < ifg.face_cnt)
						{
							lod_morphed_face_list[m_fid] =
								tmp[lod_morphed_face_list[m_fid]];
						}
					}

					delete [] tmp;
				}

				delete [] ifg.indices;
				// prevent destructor of FaceGroup from freeing memory
				memset(&ifg, 0, sizeof(ifg));
			}
		}
	}
}

float XMesh::compute_face_area (const FaceGroup *fg, const int fid) const
{
	const int vid1 = vertex_batch_list[ fg->face_vertex_chain[3*fid] ];
	const int vid2 = vertex_batch_list[ fg->face_vertex_chain[3*fid+1] ];
	const int vid3 = vertex_batch_list[ fg->face_vertex_chain[3*fid+2] ];

	const Vector v1 ( object_vertex_list[vid2] - object_vertex_list[vid1] );
	const Vector v2 ( object_vertex_list[vid3] - object_vertex_list[vid1] );

	return ( .5f * cross_product(v1, v2).magnitude() );
}

void XMesh::compute_face_areas (void)
{
	for(int gid = 0; gid < face_group_cnt; gid++)
	{
		FaceGroup *fg = face_groups + gid;

		for(int fid = 0; fid < fg->face_cnt; fid++)
		{
			fg->face_area[fid] = compute_face_area(fg, fid);
		}
	}
}

bool XMesh::expand_bounding_box (float* box) const
{
	bool result = false;

	if (object_vertex_cnt)
	{
		if (bounds[0] > box[0])
			box[0] = bounds[0];

		if (bounds[1] < box[1])
			box[1] = bounds[1];

		if (bounds[2] > box[2])
			box[2] = bounds[2];

		if (bounds[3] < box[3])
			box[3] = bounds[3];

		if (bounds[4] > box[4])
			box[4] = bounds[4];

		if (bounds[5] < box[5])
			box[5] = bounds[5];

		result = true;
	}

	return result;
}

void XMesh::compute_bounds (void)
{

	bounds[0] = bounds[1] = 0.0f;
	bounds[2] = bounds[3] = 0.0f;
	bounds[4] = bounds[5] = 0.0f;

	if (object_vertex_cnt)
	{
		bounds[0] = bounds[1] = object_vertex_list[0].x;
		bounds[2] = bounds[3] = object_vertex_list[0].y;
		bounds[4] = bounds[5] = object_vertex_list[0].z;

		for (Vector* v = object_vertex_list + 1; 
				v < object_vertex_list + object_vertex_cnt; 
				v++)
		{
			float* b = bounds;

			if (v->x > *b)
				*b = v->x;
			else
			if (v->x < *(b+1))
				*(b+1) = v->x;

			if (v->y > *(b+2))
				*(b+2) = v->y;
			else
			if (v->y < *(b+3))
				*(b+3) = v->y;

			if (v->z > *(b+4))
				*(b+4) = v->z;
			else
			if (v->z < *(b+5))
				*(b+5) = v->z;
		}
	}
}

//

void XMesh::get_bounding_box(SINGLE box[6]) const
{
	box[0] = bounds[0];
	box[1] = bounds[1];
	box[2] = bounds[2];
	box[3] = bounds[3];
	box[4] = bounds[4];
	box[5] = bounds[5];
}

//

void XMesh::release_full_mesh(void)
{
	if (face_groups != NULL)
	{
		delete [] face_groups;
		face_groups = NULL;
	}

	//
	// Free normal list
	//

	delete [] normal_ABC;
	normal_ABC = NULL;
	

	//
	// Free vertices and vertex reference chains
	//

	delete [] vertex_batch_list;
	vertex_batch_list = NULL;

	delete [] texture_batch_list;
	texture_batch_list = NULL;

	delete [] texture_batch_list2;
	texture_batch_list2 = NULL;

	delete [] texture_vertex_list;
	texture_vertex_list = NULL;

	delete [] object_vertex_list;
	object_vertex_list = NULL;

	delete [] vertex_color_list;
	vertex_color_list = NULL;
	
	delete [] vertex_normal;
	vertex_normal = NULL;

	delete [] vertex_D_coefficient;
	vertex_D_coefficient = NULL;


//
// Release material textures so TXMLib can get rid of them if nobody else is
// using them.
//
#if 0
	Material * mat = material_list;
	for (int i = 0; i < material_cnt; i++, mat++)
	{
		txmlib->release_texture_ref( mat->diffuse_texture_ref );
		txmlib->release_texture_ref( mat->second_diffuse_texture_ref );
		txmlib->release_texture_ref( mat->emissive_texture_ref );
	}
#else
	for( int i=0; i<material_cnt; i++ ) {
		material_list[i].cleanup();
	}
#endif

	delete [] material_list;
	material_list = NULL;

	delete [] uv_channel_list;
	uv_channel_list = NULL;
	
	delete [] edge_vertex_chain;
	edge_vertex_chain = NULL;

	delete [] edge_angle_list;
	edge_angle_list = NULL;

	
// continuous lod stuff
	delete [] lod_step_list;
	lod_step_list = NULL;

	delete [] lod_uv_step_list;
	lod_uv_step_list = NULL;

	delete [] lod_removed_face_list;
	lod_removed_face_list = NULL;
	
	delete [] lod_morphed_face_list;
	lod_morphed_face_list = NULL;

	delete [] lod_group_face_list;
	lod_group_face_list = NULL;

	delete [] lod_high_face_normal_list;
	lod_high_face_normal_list = NULL;
	
	delete [] lod_high_face_D_coefficient;
	lod_high_face_D_coefficient = NULL;
	
	delete [] lod_low_face_normal_list;
	lod_low_face_normal_list = NULL;
	
	delete [] lod_low_face_D_coefficient;
	lod_low_face_D_coefficient = NULL;
	
	delete [] lod_vertex_list;
	lod_vertex_list = NULL;
	
	delete [] lod_edge_list;
	lod_edge_list = NULL;

	delete [] lod_uv_batch_chain;
	lod_uv_batch_chain = NULL;

	delete [] lod_uv_high_id1;
	lod_uv_high_id1 = NULL;

	delete [] lod_uv_batch_cnt1;
	lod_uv_batch_cnt1 = NULL;

	delete [] lod_uv_batch_first1;
	lod_uv_batch_first1 = NULL;

	delete [] lod_uv_high_id2;
	lod_uv_high_id2 = NULL;

	delete [] lod_uv_batch_cnt2;
	lod_uv_batch_cnt2 = NULL;

	delete [] lod_uv_batch_first2;
	lod_uv_batch_first2 = NULL;

	delete [] lod_uv_low_id;
	lod_uv_low_id = NULL;
}

//

XMesh::~XMesh(void)
{
	release_full_mesh();
}

void GetFullName(IFileSystem * fs, char * full_name)
{
	if(fs != NULL && full_name != NULL)
	{
		full_name[0] = 0;
		
		COMPTR<IFileSystem> parent;
		if(GR_OK == fs->GetParentSystem( parent ) && parent != NULL)
		{
			GetFullName(parent, full_name);
		
			char tmp_name[256];
			fs->GetFileName(tmp_name, 256);
			strcat(full_name, tmp_name);
		}
	}
}

//

HRESULT XMesh::read_face_groups( IFileSystem *parent )
{

#define MESH_FACEGROUP_MAX_NAME_LEN 32

	// Read face groups
	//
	if( parent->SetCurrentDirectory( "Face groups" ) ) {
		read_type( parent, "Count", &face_group_cnt );
		face_groups = new FaceGroup[face_group_cnt];

		face_cnt=0;
		have_smooth_shaded_faces = 0;
		have_double_sided_faces = 0;
		
		FaceGroup *group = face_groups;
		for( int i=0; i<face_group_cnt; i++, group++ ) {
			
			char group_name[MESH_FACEGROUP_MAX_NAME_LEN];
			sprintf( group_name, "Group%u", i );

			if( parent->SetCurrentDirectory( group_name ) ) {

				read_type(  parent, "Material",					&group->material );
				read_type(  parent, "Face count",				&group->face_cnt );
				read_alloc_chunk( parent, "Face vertex chain",	0,	&group->face_vertex_chain );
				read_alloc_chunk( parent, "Face normal",		0,	&group->face_normal );
				read_alloc_chunk( parent, "Face D-coefficient",	0,	&group->face_D_coefficient );
				read_alloc_chunk( parent, "Face property",		0,	&group->face_properties );

				group->face_area = new float[group->face_cnt];	// computed at the end

				if( !have_smooth_shaded_faces || !have_double_sided_faces ) {
					FACE_PROPERTY *fp = group->face_properties;
					for( int f=0; f<group->face_cnt; f++, fp++ ) {
						if( *fp & SMOOTH_SHADED ) {
							have_smooth_shaded_faces++;
						}
						if( *fp & TWO_SIDED ) {
							have_double_sided_faces++;
						}
						if( have_smooth_shaded_faces && have_double_sided_faces ) { 
							break;
						}
					}
				}

				parent->SetCurrentDirectory( ".." );
			}

			face_cnt += group->face_cnt;
		}

		parent->SetCurrentDirectory( ".." );

		return S_OK;
	}

	return E_FAIL;
}


//

HRESULT XMesh::read_vertex_data( IFileSystem *parent )
{
	//
	// Read vertex- and vertex-reference lists
	//
	if( parent->SetCurrentDirectory( "Vertices" ) ) {
		
		read_type( parent, "Vertex batch count", &vertex_batch_cnt );
		read_alloc_chunk( parent, "Vertex batch list", 0, &vertex_batch_list );
		
		read_type( parent, "Object vertex count", &object_vertex_cnt );
		read_alloc_chunk( parent, "Object vertex list", 0, &object_vertex_list );

		read_alloc_chunk( parent, "Vertex normal", 0, &vertex_normal );
		read_alloc_chunk( parent, "Vertex D-coefficient", 0, &vertex_D_coefficient );

		read_alloc_chunk( parent, "Color", 0, &vertex_color_list );

		read_type( parent, "Texture vertex count", &texture_vertex_cnt );
		read_alloc_chunk( parent, "Texture vertex list", 0, &texture_vertex_list );
		read_alloc_chunk( parent, "Texture batch list", 0, &texture_batch_list );
		read_alloc_chunk( parent, "Texture batch list2", 0, &texture_batch_list2 );

		parent->SetCurrentDirectory( ".." );

		return S_OK;
	}

	return E_FAIL;
}

//

HRESULT XMesh::read_lod_library( IFileSystem *parent )
{
	if( parent->SetCurrentDirectory( "Lod library" ) ) {

		lod_fraction = 1.0f;
		lod_fraction_previous = 1.0f;
		lod_threshold = 0.0001f;

		read_type( parent, "Step count", &lod_step_cnt );
		read_alloc_chunk( parent, "Step list", 0, &lod_step_list );

		read_type( parent, "Removed face count", &lod_removed_face_cnt );
		read_alloc_chunk( parent, "Removed face list", 0, &lod_removed_face_list );

		read_type( parent, "Morphed face count", &lod_morphed_face_cnt );
		read_alloc_chunk( parent, "Morphed face list", 0, &lod_morphed_face_list );
		read_alloc_chunk( parent, "Group list", 0, &lod_group_face_list );
		read_alloc_chunk( parent, "High normal list", 0, &lod_high_face_normal_list );
		read_alloc_chunk( parent, "High D coefficient", 0, &lod_high_face_D_coefficient );
		read_alloc_chunk( parent, "Low normal list", 0,	&lod_low_face_normal_list );
		read_alloc_chunk( parent, "Low D coefficient", 0, &lod_low_face_D_coefficient );

		read_type( parent, "Vertex count", &lod_vertex_cnt );
		if(lod_vertex_cnt > 0) // this can happen if only one edge is getting collapsed
		{
			read_alloc_chunk( parent, "Vertex list", 0, &lod_vertex_list );
		}

		// lod UV stuff
		if( FAILED( read_type( parent,  "UV count", &lod_uv_count ) ) ) {
			parent->SetCurrentDirectory( ".." );
			return S_WARN_OLD_FORMAT;
		}

		if(lod_uv_count > 0)
		{
			read_type(		  parent, "UV chain count", &uv_chain_count);
			read_alloc_chunk( parent, "UV step list", 0, &lod_uv_step_list );
			read_alloc_chunk( parent, "UV batch chain", 0, &lod_uv_batch_chain );
			read_alloc_chunk( parent, "High UV id 1", 0, &lod_uv_high_id1 );
			read_alloc_chunk( parent, "High batch count 1", 0, &lod_uv_batch_cnt1 );
			read_alloc_chunk( parent, "High batch first 1", 0, &lod_uv_batch_first1 );
			read_alloc_chunk( parent, "High UV id 2", 0, &lod_uv_high_id2 );
			read_alloc_chunk( parent, "High batch count 2", 0, &lod_uv_batch_cnt2 );
			read_alloc_chunk( parent, "High batch first 2", 0, &lod_uv_batch_first2 );
			read_alloc_chunk( parent, "Low UV id", 0, &lod_uv_low_id );
		}

		parent->SetCurrentDirectory( ".." );

		return S_OK;
	}

	return E_FAIL;
}

//

HRESULT XMesh::read_edge_data( IFileSystem *parent )
{
	HRESULT hr_ret = E_FAIL;

	if( parent->SetCurrentDirectory( "Edges" ) ) {

		read_type( parent, "Edge count", &edge_cnt );
		read_alloc_chunk( parent, "Vertex list", 0, &edge_vertex_chain );
			
		if( FAILED( read_alloc_chunk( parent, "Edge angle", 0, &edge_angle_list ) ) ) {
			
			EDGE_PROPERTY *edge_properties = NULL;
			
			if( !FAILED( read_alloc_chunk( parent, "Edge property", 0, &edge_properties ) ) ) {

				// old style edges here, so we convert to new format and sort
				int *tmp_edge_vertex_chain = edge_vertex_chain;
				edge_vertex_chain = new int[2 * edge_cnt];
				edge_angle_list = new float[edge_cnt];
				
				int i;
				int tmp_count = 0;
				for( i=0; i<edge_cnt; i++ ) {
					if( !(edge_properties[i] & TRIANGULATING) )	{
						edge_angle_list[tmp_count] = 3.14159265358979323846f;
						edge_vertex_chain[2 * tmp_count] = tmp_edge_vertex_chain[2 * i];
						edge_vertex_chain[2 * tmp_count + 1] = tmp_edge_vertex_chain[2 * i + 1];
						tmp_count++;
					}
				}

				for( i=0; i<edge_cnt; i++ )	{
					if( edge_properties[i] & TRIANGULATING ) {
						edge_angle_list[tmp_count] = 0.0f;
						edge_vertex_chain[2 * tmp_count] = tmp_edge_vertex_chain[2 * i];
						edge_vertex_chain[2 * tmp_count + 1] = tmp_edge_vertex_chain[2 * i + 1];
						tmp_count++;
					}
				}

				delete [] edge_properties;
				delete [] tmp_edge_vertex_chain;

				ASSERT_FATAL( tmp_count == edge_cnt );
			
				hr_ret = S_WARN_OLD_FORMAT;
			}
		}

		parent->SetCurrentDirectory( ".." );

		hr_ret = S_OK;
	}

	return hr_ret;
}

//

HRESULT XMesh::read_uv_channels( IFileSystem *parent )
{
#define MESH_UVCHANNEL_MAX_NAME_LEN 64

	if( parent->SetCurrentDirectory( "UVChannel library" ) ) {

		read_type( parent, "Channel count", &uv_channel_cnt );

		if( uv_channel_cnt > 0 ) {

			uv_channel_list = new UVChannel[uv_channel_cnt];

			char uv_name[MESH_UVCHANNEL_MAX_NAME_LEN];

			for( int uv_ch_id=0; uv_ch_id<uv_channel_cnt; uv_ch_id++ ) {

				sprintf( uv_name, "Channel %2d", uv_ch_id );

				if( parent->SetCurrentDirectory( uv_name ) ) {
					
					read_alloc_chunk( parent, "Name", 0, &(uv_channel_list[uv_ch_id].name) );

					read_type( parent, "Vertex count", &(uv_channel_list[uv_ch_id].vertex_count) );
					read_alloc_chunk( parent, "Vertex lookup", 0, &(uv_channel_list[uv_ch_id].vertex_lookup) );
					
					read_type( parent, "Frame rate",  &(uv_channel_list[uv_ch_id].fps) );
					read_type( parent, "Frame count", &(uv_channel_list[uv_ch_id].frame_count) );
					
					read_alloc_chunk( parent, "UV chain", 0, &(uv_channel_list[uv_ch_id].uv_chain) );
					
					read_type( parent, "Interpolate", &(uv_channel_list[uv_ch_id].interpolate) );

					parent->SetCurrentDirectory( ".." );
				}

				uv_channel_list[uv_ch_id].time_length =	uv_channel_list[uv_ch_id].frame_count / uv_channel_list[uv_ch_id].fps;
			}
		}

		parent->SetCurrentDirectory( ".." );

		return S_OK;
	}

	return E_FAIL;
}

//

HRESULT XMesh::read_material_library( IFileSystem *parent, ITextureLibrary *texturelib, IMaterialManager* matMan)
{

#if 0
	txmlib = texturelib;
#endif

	texturelib->load_library( parent, NULL );

	if(parent->SetCurrentDirectory( "CQ2_Material_Library" ) ) 
	{
		if( SUCCEEDED( read_type( parent, "Material count", &imaterial_cnt ) ) || (imaterial_cnt <= 0) ) 
		{	
			if( (imaterial_list = new IMaterial*[imaterial_cnt]) != NULL ) 
			{

				HANDLE          hMatSearch;
				WIN32_FIND_DATA MatSearchData;
				int material_entry = 0;

				if( (hMatSearch = parent->FindFirstFile( "*.*", &MatSearchData )) != INVALID_HANDLE_VALUE ) 
				{
					do 
					{
						if( !(MatSearchData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
							 ( strcmp( MatSearchData.cFileName, "." ) == 0 ) ||
							 ( strcmp( MatSearchData.cFileName, ".." ) == 0 ) ) 
						{

							continue;
						}
						imaterial_list[material_entry] = matMan->FindMaterial(MatSearchData.cFileName);
						if(imaterial_list[material_entry])  
						{
							imaterial_list[material_entry]->Realize();
						}
						else
						{
							GENERAL_WARNING( TEMPSTR( "Unable to find CQ2 material '%s'", MatSearchData.cFileName ) );
						}
						material_entry++; 
					} while( parent->FindNextFile( hMatSearch, &MatSearchData ) );
					parent->FindClose(hMatSearch);
				}
			}
		}
		return S_OK;
	}
	else if(parent->SetCurrentDirectory( "Material library" ) ) 
	{

		if( SUCCEEDED( read_type( parent, "Material count", &material_cnt ) ) || (material_cnt <= 0) ) {
			
			if( (material_list = new Material[material_cnt]) != NULL ) 
			{
				HANDLE          hMatSearch;
				WIN32_FIND_DATA MatSearchData;
				int material_entry = 0;

				if( (hMatSearch = parent->FindFirstFile( "*.*", &MatSearchData )) != INVALID_HANDLE_VALUE ) {

					do {

						if( !(MatSearchData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
							 ( strcmp( MatSearchData.cFileName, "." ) == 0 ) ||
							 ( strcmp( MatSearchData.cFileName, ".." ) == 0 ) ) {

							continue;
						}

						if( SUCCEEDED( read_material( parent, texturelib, material_entry, MatSearchData.cFileName, material_list, material_cnt ) ) ) {
							material_entry++; 
						}
						else {
							GENERAL_WARNING( TEMPSTR( "Unable to read material '%s'", MatSearchData.cFileName ) );
						}
					}
					while( parent->FindNextFile( hMatSearch, &MatSearchData ) );
					parent->FindClose(hMatSearch);
				}
			}
		}

		parent->SetCurrentDirectory( ".." );

		return S_OK;
	}

	return E_FAIL;
}

//

HRESULT XMesh::read_material( IFileSystem *parent, ITextureLibrary *texturelib, int material_entry, const char *material_name, Material *material_list, int material_cnt )
{
	HRESULT hr;

	if( parent->SetCurrentDirectory( material_name ) ) {

		int material_index;

		if( FAILED( hr = read_type( parent, "Material identifier", &material_index ) ) ) {
			ASSERT(0);
		}

		ASSERT( material_index < material_cnt );

		Material *material = &material_list[material_entry];
		material->initialize();
		material->set_name( material_name );
		material->set_texture_library( texturelib );
		material->load_from_filesystem( parent );

		parent->SetCurrentDirectory( ".." );
		return S_OK;
	}

	return E_FAIL;

}


//

bool XMesh::read (IFileSystem* parent, ITextureLibrary* txm_lib,IMaterialManager * matMan,  U32 cont_lod_mode)
{
	char debug_name[MAX_PATH];
	GetFullName( parent, debug_name );
	TempStr old_format_warning( "%s is in an unsupported mesh format, re-export.", debug_name );
	HRESULT hr;

	// Check for old format
	//
	if( parent->SetCurrentDirectory( "Faces" ) ) {
		GENERAL_WARNING( (char*)old_format_warning );
		parent->SetCurrentDirectory( ".." );
		return false;
	}


	if( FAILED( hr = read_face_groups( parent ) ) ) {
		ASSERT(0);
	}

	if( parent->SetCurrentDirectory( "Normals" ) ) {
		read_type( parent, "Surface normal count", &normal_cnt );
		read_alloc_chunk( parent, "Surface normal list", 0, &normal_ABC );
		parent->SetCurrentDirectory( ".." );
	}

	if( FAILED( hr = read_vertex_data( parent ) ) ) {
		ASSERT(0);
	}


	lod_fraction = -1.0f;  // invalid
	lod_fraction_previous = -1.0f;
	lod_threshold = -1.0f;
	allow_lod_change = false;

	switch( cont_lod_mode )
	{
		case 0:	// ignore lod
			break;

		case 2: // do lod on a per archetype basis
			if( SUCCEEDED( hr = read_lod_library( parent ) ) ) {
				allow_lod_change = true;
			}
			break;

		default: // do lod on a per instance basis
			read_lod_library( parent );
	}
	

	if( FAILED( hr = read_edge_data( parent ) ) ) {
	}
	else if( hr == S_WARN_OLD_FORMAT ) {
		GENERAL_WARNING( (char*)old_format_warning );
	}


	hr = read_uv_channels( parent );
	
	if( FAILED( read_material_library( parent, txm_lib,matMan ) ) ) {
		// cleanup ?
		return false;
	}


	// Compute (or read) bounding sphere.
	//
	if( parent->SetCurrentDirectory( "Sphere" ) ) {
		read_type( parent, "Center", &sphere_center );
		read_type( parent, "Radius", &radius );
		parent->SetCurrentDirectory( ".." );
	}
	else {
		compute_sphere();
	}

	ASSERT_FATAL(radius > 0.0f);

	compute_bounds ();
	compute_centroid ();
		

	// Sort faces based on the area
	//
	if(lod_step_list == NULL)
	{
		compute_face_areas ();
		sort_faces_by_area ();
		sorted_faces = 1;
	}
	else 
	{
		// continuous lod meshes can only sort faces that never go away
		const bool save = allow_lod_change;
		allow_lod_change = true;
		set_lod(0.0f);
		lod_fraction_previous = 0.0f;	// supress thrashing warning
		compute_face_areas (); 
		sort_faces_by_area ();
		set_lod(1.0f);
		lod_fraction_previous = 1.0f;	// supress thrashing warning
		compute_face_areas ();	// different than at 0.0 lod than at 1.0 lod
		allow_lod_change = save;

		sorted_faces = 0;		// only sorted at lod 0.0
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////



struct Triangle
{
	int verts[3];
	int uvs[3];
	int material;
	int texture_ID;
	int normal;
	SINGLE d_coefficient;
	FACE_PROPERTY properties;

	Triangle (void)
	{
		memset (this, -1, sizeof (*this));
	}
};

void XMesh::AssertMesh(void) const
{
	for(int g_id=0; g_id < face_group_cnt; g_id++)
	{
		const FaceGroup *fg = face_groups + g_id;

		ASSERT_FATAL(fg->material < material_cnt);

		for(int f_id = 0; f_id < fg->face_cnt; f_id++)
		{
			ASSERT_FATAL(fg->face_vertex_chain[3*f_id] < vertex_batch_cnt);
			ASSERT_FATAL(fg->face_vertex_chain[3*f_id+1] < vertex_batch_cnt);
			ASSERT_FATAL(fg->face_vertex_chain[3*f_id+2] < vertex_batch_cnt);

			ASSERT_FATAL(fg->face_normal[f_id] < normal_cnt);
		}
	}

	for(int vb_id = 0; vb_id < vertex_batch_cnt; vb_id++)
	{
		ASSERT_FATAL(vertex_batch_list[vb_id] < object_vertex_cnt);
		ASSERT_FATAL(texture_batch_list[vb_id] < texture_vertex_cnt);
		if(texture_batch_list2)
		{
			ASSERT_FATAL(texture_batch_list2[vb_id] < texture_vertex_cnt);
		}
	}

			
	for(int v_id = 0; v_id < object_vertex_cnt; v_id++)
	{
		ASSERT_FATAL(vertex_normal[v_id] < normal_cnt);
	}
}

#define EPSILON 1e-4f
bool XMesh::split_mesh (const Vector& v0,
						float d,
						XMesh*& r0,
						XMesh*& r1,
						U32 split_polys,
						BOOL32 keep_polys) const
{
	ASSERT_FATAL (object_vertex_cnt);

	r0 = NULL;
	r1 = NULL;

	PointStatus* ps = new PointStatus[object_vertex_cnt];

// check all vertices against plane.

	{
		const Vector* v;

		PointStatus* p;

		for (v = object_vertex_list, p = ps; 
			v < object_vertex_list + object_vertex_cnt; 
			v++, p++)
		{
			p->r = dot_product(*v, v0) + d;

			if (p->r > EPSILON)
				p->stat = PS_FRONT;
			else if (p->r < -EPSILON)
				p->stat = PS_BACK;
			else
				p->stat = PS_ON;
		}
	}

	bool result = false;

	LList<GroupIndex> m0;
	LList<GroupIndex> m1;

	Split *split_list;
	if(split_polys)
	{
		split_list = new Split[face_group_cnt];
	}
	else
	{
		split_list = NULL;
	}

	U32 num_split_polys =0;

	FaceGroup *fg = face_groups;
	for (int g_id = 0; g_id < face_group_cnt; g_id++, fg++)
	{
		bool used_m0 = false;
		bool used_m1 = false;
		for (int f_id = 0; f_id < fg->face_cnt; f_id++)
		{
			
			PlaneStatus fs = check_face(*fg, f_id, ps);

			if (fs == PS_FRONT)
			{
				GroupIndex *g;
				if(!used_m0)
				{
					g = m0.alloc();
					g->index = g_id;
					used_m0 = true;
				}
				else
				{
					g = m0.last();
				}
				FaceIndex *f = g->faces.alloc();
				f->index = f_id;
				continue;
			}

			if (fs == PS_BACK)
			{
				GroupIndex *g;
				if(!used_m1)
				{
					g = m1.alloc();
					g->index = g_id;
					used_m1 = true;
				}
				else
				{
					g = m1.last();
				}
				FaceIndex *f = g->faces.alloc();
				f->index = f_id;
				continue;
			}

			if (fs == PS_SPAN && split_polys)
			{
				if (num_split_polys < MAX_SPLIT_POLYS)
				{
					split_face(f_id, split_list + g_id, ps,(split_polys == SA_SPLIT_EXACT), g_id);
					num_split_polys++; // total (not per group)
					continue;
				}
			}

			// (tlp) now only reaches here if 
			// not yet added to a split side
			if (keep_polys)
			{
				// divide evenly; 
				// make it look the same each time
				if (f_id & 1)
				{
					GroupIndex *g;
					if(!used_m0)
					{
						g = m0.alloc();
						g->index = g_id;
						used_m0 = true;
					}
					else
					{
						g = m0.last();
					}
					FaceIndex *f = g->faces.alloc();
					f->index = f_id;
				}
				else
				{
					GroupIndex *g;
					if(!used_m1)
					{
						g = m1.alloc();
						g->index = g_id;
						used_m1 = true;
					}
					else
					{
						g = m1.last();
					}
					FaceIndex *f = g->faces.alloc();
					f->index = f_id; 
				}
			}
		}
	}

	delete [] ps;

	result = false;

	if ( (m0.count () && m1.count ()) || (num_split_polys > 0) )
	{
		if (split_polys)
		{
			r0 = build_sub_mesh2 (m0, split_list, 0);
			//r0->AssertMesh();

			r1 = build_sub_mesh2 (m1, split_list, 1);
			//r1->AssertMesh();
		}
		else
		{
			r0 = build_sub_mesh2 (m0, NULL, 0);
			r1 = build_sub_mesh2 (m1, NULL, 0);
		}

		result = true;
	}

	if(split_list) delete [] split_list;

	GroupIndex *tmp_g0 = m0.first();
	while(tmp_g0)
	{
		tmp_g0->faces.free();
		tmp_g0 = tmp_g0->next;
	}
	m0.free();

	GroupIndex *tmp_g1 = m1.first();
	while(tmp_g1)
	{
		tmp_g1->faces.free();
		tmp_g1 = tmp_g1->next;
	}
	m1.free();		
	
	return result;
}


XMesh::PlaneStatus XMesh::check_face (const FaceGroup& fg, int face_id, 
								const XMesh::PointStatus* ps) const
{
	PlaneStatus result;

	int front = 0, back = 0, on = 0;
	for (int i = 0; i < 3; i++)
	{
		int v = vertex_batch_list[ fg.face_vertex_chain[3*face_id+i] ];
		//int v = vertex_batch_list[face_vert + i];
		switch (ps[v].stat)
		{
		case PS_FRONT:
			front++;
			break;

		case PS_BACK:
			back++;
			break;

		case PS_ON:
			on++;
			break;
		}
	}

	if (front)
	{
		if (back)
		{
			result = PS_SPAN;
		}
		else
		{
			result = PS_FRONT;
		}
	}
	else if (back)
	{
		result = PS_BACK;
	}
	else
	{
		result = PS_ON;
	}

	return result;
}

//int Mesh::Intersect(int s,int p,Split *split,unsigned int idx)
//{
/*	int s_tex = texture_vertex_chain[face_vertices[idx+face_num_vertices[idx]-1]];\
	int p_tex = texture_vertex_chain[face_vertices[idx] + i];\
	split->t[split->v_cnt].u = (texture_vertex_list[s_tex].u+texture_vertex_list[p_tex].u)*0.5;\
	split->t[split->v_cnt].v = (texture_vertex_list[s_tex].v+texture_vertex_list[p_tex].v)*0.5;\
\*/

/*#define INTERSECT() {\
if (split->v_cnt < MAX_SPLIT_POLYS*2)\
{\
	split->v[split->v_cnt] = (object_vertex_list[s]+object_vertex_list[p])*0.5;\
	split->t[split->v_cnt].u = (texture_vertex_list[s_tex].u+texture_vertex_list[p_tex].u)*0.5;\
	split->t[split->v_cnt].v = (texture_vertex_list[s_tex].v+texture_vertex_list[p_tex].v)*0.5;\
	split->vn[split->v_cnt] = f_n;\
	split->vD[split->v_cnt] = f_D;\
	split->v_cnt++;\
	i=split->v_cnt-1;\
}\
}*/

void Output(const int &idx, const int &t_idx, U8 *cnt, int *out, int *t_out)
{
	out[(*cnt)] = idx;
	t_out[(*cnt)++] = t_idx;
}

#define HIGH_BIT 0xF000000
void XMesh::split_face (unsigned int idx, Split *split, const XMesh::PointStatus* ps,bool bExact,int group) const
{
//	PlaneStatus result;
		//Sutherland Hodgman Polygon Clip
		int out0[4], out1[4], t_out0[4], t_out1[4];
		U8 cnt0 = 0, cnt1 = 0;
		
		int batch_id = face_groups[group].face_vertex_chain[3*idx + 2];
		int s = vertex_batch_list[ batch_id ];
		int s_tex = texture_batch_list[ batch_id ];
		
		int vertex_count = 3;

		int f_n = face_groups[group].face_normal[idx];
		float f_D = face_groups[group].face_D_coefficient[idx];
		
		for (int i = 0; i < vertex_count; i++)
		{
			batch_id = face_groups[group].face_vertex_chain[3*idx + i];
			int p = vertex_batch_list[ batch_id ];
			int p_tex = texture_batch_list[ batch_id ];
			
			if (ps[p].stat == PS_FRONT) // cases 1 and 4
			{
				if (ps[s].stat == PS_FRONT) // case 1
				{
					Output(batch_id, batch_id, &cnt0, out0, t_out0);
				}
				else					//case 4
				{
					int i;
					//	INTERSECT();
					if (bExact)
					{
						if (split->v_cnt < MAX_SPLIT_POLYS*2)
						{
							SINGLE inv_len = 1/(ps[s].r-ps[p].r);
							split->v[split->v_cnt] = (-ps[p].r*object_vertex_list[s]+ps[s].r*object_vertex_list[p])*inv_len;\
							split->t[split->v_cnt].u = (-ps[p].r*texture_vertex_list[s_tex].u+ps[s].r*texture_vertex_list[p_tex].u)*inv_len;\
							split->t[split->v_cnt].v = (-ps[p].r*texture_vertex_list[s_tex].v+ps[s].r*texture_vertex_list[p_tex].v)*inv_len;\
							
							split->vn[split->v_cnt] = f_n;
							split->vD[split->v_cnt] = f_D;
							split->v_cnt++;
							i=split->v_cnt-1;
						}
					}
					else
					{
						if (split->v_cnt < MAX_SPLIT_POLYS*2)
						{
							split->v[split->v_cnt] = (object_vertex_list[s]+object_vertex_list[p])*0.5;
							split->t[split->v_cnt].u = (texture_vertex_list[s_tex].u+texture_vertex_list[p_tex].u)*0.5;
							split->t[split->v_cnt].v = (texture_vertex_list[s_tex].v+texture_vertex_list[p_tex].v)*0.5;
							
							split->vn[split->v_cnt] = f_n;
							split->vD[split->v_cnt] = f_D;
							split->v_cnt++;
							i=split->v_cnt-1;
						}
					}
					
					// HIGH_BIT is checked at reconstruct time so we know it's an index into the new or old vertices
					Output(i | HIGH_BIT, i | HIGH_BIT, &cnt0, out0, t_out0);
					Output(batch_id, batch_id, &cnt0, out0, t_out0);
					Output(i | HIGH_BIT, i | HIGH_BIT, &cnt1, out1, t_out1);	
				}
			}
			else					// case 2 and 3
			{
				if (ps[s].stat == PS_FRONT)  // case 2
				{
					int i;
					//	INTERSECT();
					if (bExact)
					{
						if (split->v_cnt < MAX_SPLIT_POLYS*2)
						{
							SINGLE inv_len = 1/(ps[s].r-ps[p].r);
							split->v[split->v_cnt] = (-ps[p].r*object_vertex_list[s]+ps[s].r*object_vertex_list[p])*inv_len;\
							split->t[split->v_cnt].u = (-ps[p].r*texture_vertex_list[s_tex].u+ps[s].r*texture_vertex_list[p_tex].u)*inv_len;\
							split->t[split->v_cnt].v = (-ps[p].r*texture_vertex_list[s_tex].v+ps[s].r*texture_vertex_list[p_tex].v)*inv_len;\
							
							split->vn[split->v_cnt] = f_n;
							split->vD[split->v_cnt] = f_D;
							split->v_cnt++;
							i=split->v_cnt-1;
						}
					}
					else
					{
						if (split->v_cnt < MAX_SPLIT_POLYS*2)
						{
							split->v[split->v_cnt] = (object_vertex_list[s]+object_vertex_list[p])*0.5;
							split->t[split->v_cnt].u = (texture_vertex_list[s_tex].u+texture_vertex_list[p_tex].u)*0.5;
							split->t[split->v_cnt].v = (texture_vertex_list[s_tex].v+texture_vertex_list[p_tex].v)*0.5;
							
							split->vn[split->v_cnt] = f_n;
							split->vD[split->v_cnt] = f_D;
							split->v_cnt++;
							i=split->v_cnt-1;
						}
					}
					
					Output(i | HIGH_BIT, i | HIGH_BIT, &cnt0, out0, t_out0);
					Output(i | HIGH_BIT, i | HIGH_BIT, &cnt1, out1, t_out1);
					Output(batch_id, batch_id, &cnt1, out1, t_out1);	
				}
				else					// case 3
				{
					Output(batch_id, batch_id, &cnt1, out1, t_out1);
				}
			}
			s = p;
			s_tex = p_tex;
		}

		ASSERT_FATAL(cnt0 > 2 && cnt1 > 2);


		if (cnt0 == 3)
		{	
			split->n[0][split->f_cnt[0]] = face_groups[group].face_normal[idx];
			split->orig[0][split->f_cnt[0]] = idx;
			memcpy(&split->f[0][split->f_cnt[0]*3], out0, 3*sizeof(int));
			memcpy(&split->t_verts[0][split->f_cnt[0]*3], t_out0, 3*sizeof(int));
			split->f_cnt[0]++;
		}
		else
		{	
			split->n[0][split->f_cnt[0]] = face_groups[group].face_normal[idx];
			split->orig[0][split->f_cnt[0]] = idx;
			memcpy(&split->f[0][split->f_cnt[0]*3], out0, 3*sizeof(int));
			memcpy(&split->t_verts[0][split->f_cnt[0]*3], t_out0, 3*sizeof(int));
			split->f_cnt[0]++;
			
			split->n[0][split->f_cnt[0]] = face_groups[group].face_normal[idx];
			split->orig[0][split->f_cnt[0]] = idx;
			split->f[0][split->f_cnt[0]*3] = out0[2];
			split->f[0][split->f_cnt[0]*3+1] = out0[3];
			split->f[0][split->f_cnt[0]*3+2] = out0[0];
			split->t_verts[0][split->f_cnt[0]*3] = t_out0[2];
			split->t_verts[0][split->f_cnt[0]*3+1] = t_out0[3];
			split->t_verts[0][split->f_cnt[0]*3+2] = t_out0[0];
			split->f_cnt[0]++;
		}
		
		if (cnt1 == 3)
		{
			split->n[1][split->f_cnt[1]] = face_groups[group].face_normal[idx];
			split->orig[1][split->f_cnt[1]] = idx;
			memcpy(&split->f[1][split->f_cnt[1]*3], out1, 3*sizeof(int));
			memcpy(&split->t_verts[1][split->f_cnt[1]*3], t_out1, 3*sizeof(int));
			split->f_cnt[1]++;
		}
		else
		{
			split->n[1][split->f_cnt[1]] = face_groups[group].face_normal[idx];
			split->orig[1][split->f_cnt[1]] = idx;
			memcpy(&split->f[1][split->f_cnt[1]*3], out1, 3*sizeof(int));
			memcpy(&split->t_verts[1][split->f_cnt[1]*3], t_out1, 3*sizeof(int));
			split->f_cnt[1]++;
			
			split->n[1][split->f_cnt[1]] = face_groups[group].face_normal[idx];
			split->orig[1][split->f_cnt[1]] = idx;
			split->f[1][split->f_cnt[1]*3] = out1[2];
			split->f[1][split->f_cnt[1]*3+1] = out1[3];
			split->f[1][split->f_cnt[1]*3+2] = out1[0];
			split->t_verts[1][split->f_cnt[1]*3] = t_out1[2];
			split->t_verts[1][split->f_cnt[1]*3+1] = t_out1[3];
			split->t_verts[1][split->f_cnt[1]*3+2] = t_out1[0];
			split->f_cnt[1]++;
		}			
}

XMesh* XMesh::build_sub_mesh2 (const LList<GroupIndex>& groups, const Split *split_list,
							   U8 which) const
{
	XMesh* result = NULL;

	bool *used_split_group = NULL;
	int split_group_cnt = 0;

	// mark used split groups & count split faces
	int split_face_cnt = 0;
	int split_vertex_cnt = 0;
	
	if (split_list)
	{
		used_split_group = new bool[face_group_cnt];
		memset(used_split_group, 0, face_group_cnt * sizeof(bool)); 

		for(int s_id = 0; s_id < face_group_cnt; s_id++)
		{
			if(split_list[s_id].f_cnt[which] > 0)
			{
				used_split_group[s_id] = true;

				split_group_cnt++;
				split_vertex_cnt += split_list[s_id].v_cnt;

				split_face_cnt += split_list[s_id].f_cnt[which];
			}
		}
	}

	if (groups.count() != 0 || split_face_cnt != 0)
	{
		bool *used_link_group = new bool[face_group_cnt];
		memset(used_link_group, 0, face_group_cnt * sizeof(bool)); 

		bool *used_group = new bool[face_group_cnt];
	
		int used_group_cnt;

		{
			// mark used groups
			GroupIndex *gi = groups.first();
			while(gi)
			{
				used_link_group[gi->index] = true;
				gi = gi->next;
			}

			used_group_cnt = 0;
			for(int i = 0; i < face_group_cnt; i++)
			{
				used_group[i] = used_link_group[i];

				if (used_split_group && used_split_group[i])
					used_group[i] = true;

				if(used_group[i])
				{
					used_group_cnt++;
				}
			}
			ASSERT_FATAL(used_group_cnt >= groups.count());
		}

		result = new XMesh;
		result->have_smooth_shaded_faces = 0;
		result->have_double_sided_faces = 0;

		bool *used_batch = new bool[vertex_batch_cnt];
		memset(used_batch, 0, vertex_batch_cnt*sizeof(bool));

		bool *used_vertex = new bool[object_vertex_cnt];
		memset(used_vertex, 0, object_vertex_cnt*sizeof(bool));

		bool *used_texture = new bool[texture_vertex_cnt];
		memset(used_texture, 0, texture_vertex_cnt*sizeof(bool));

		bool *used_texture2=0;

		bool *used_normal = new bool[normal_cnt];
		memset(used_normal, 0, normal_cnt*sizeof(bool));

		bool *used_material = new bool[material_cnt];
		memset(used_material, 0, material_cnt*sizeof(bool));

		int *batch_remap = new int[vertex_batch_cnt];
		int *vertex_remap = new int[object_vertex_cnt];
		int *texture_remap = new int[texture_vertex_cnt];
		int *texture_remap2 = 0;
		int *normal_remap = new int[normal_cnt];
		int *material_remap = new int[material_cnt];

		result->face_cnt = split_face_cnt;

		{
			// mark used indices based on split faces
			if (split_list)
			{
				
				for(int s_id = 0; s_id < face_group_cnt; s_id++)
				{
					if(split_list[s_id].f_cnt[which] > 0)
					{
						// mtl
						used_material[ face_groups[s_id].material ] = true;
					}

					for(unsigned int f_id = 0; f_id < split_list[s_id].f_cnt[which]; f_id++)
					{
						// face normal
						used_normal[ split_list[s_id].n[which][f_id] ] = true;

						// batch
						int batch_id;

						batch_id = split_list[s_id].f[which][3*f_id];
						if( !(batch_id & HIGH_BIT) )
						{
							used_batch[batch_id] = true;
						}

						batch_id = split_list[s_id].f[which][3*f_id + 1];
						if( !(batch_id & HIGH_BIT) )
						{
							used_batch[batch_id] = true;
						}

						batch_id = split_list[s_id].f[which][3*f_id + 2];
						if( !(batch_id & HIGH_BIT) )
						{
							used_batch[batch_id] = true;
						}
					}

					for(unsigned int v_id = 0; v_id < split_list[s_id].v_cnt; v_id++)
					{
						// vertex normal
						used_normal[ split_list[s_id].vn[v_id] ] = true;
					}
				}
			}

			// mark used indices gased on list of groups
			GroupIndex *gi = groups.first();
			while(gi)
			{
				result->face_cnt += gi->faces.count();

				// materials
				FaceGroup *fg = face_groups + gi->index;
				used_material[fg->material] = true;

				FaceIndex *fi = gi->faces.first();
				while(fi)
				{
					int f_id = fi->index;

					// face normals
					used_normal[ fg->face_normal[f_id] ] = true;

					// batches
					used_batch[ fg->face_vertex_chain[3 * f_id] ] = true;
					used_batch[ fg->face_vertex_chain[3 * f_id + 1] ] = true;
					used_batch[ fg->face_vertex_chain[3 * f_id + 2] ] = true;
					
					fi = fi->next;
				}

				gi = gi->next;
			}

			// vertices & textures
			for(int batch_id=0; batch_id < vertex_batch_cnt; batch_id++)
			{
				if(used_batch[batch_id])
				{
					used_vertex[ vertex_batch_list[batch_id] ] = true;
					used_texture[ texture_batch_list[batch_id] ] = true;
				}
			}

			// vertex normals
			for(int v_id = 0; v_id < object_vertex_cnt; v_id++)
			{
				if(used_vertex[v_id])
				{
					used_normal[ vertex_normal[v_id] ] = true;
				}
			}


			// create remap arrays & set new counts
			{
				int index = 0;
				for(int i = 0; i < vertex_batch_cnt; i++)
				{
					if(used_batch[i])
					{
						batch_remap[i] = index;
						index++;
					}
				}
				result->vertex_batch_cnt = index;
			}


			{
				int index = 0;
				for(int i = 0; i < object_vertex_cnt; i++)
				{
					if(used_vertex[i])
					{
						vertex_remap[i] = index;
						index++;
					}
				}
				result->object_vertex_cnt = index;
			}

			{
				int index = 0;
				for(int i = 0; i < texture_vertex_cnt; i++)
				{
					if(used_texture[i])
					{
						texture_remap[i] = index;
						index++;
					}
				}
				result->texture_vertex_cnt = index;
			}

			{
				int index = 0;
				for(int i = 0; i < normal_cnt; i++)
				{
					if(used_normal[i])
					{
						normal_remap[i] = index;
						index++;
					}
				}
				result->normal_cnt = index;
			}

			{
				int index = 0;
				for(int i = 0; i < material_cnt; i++)
				{
					if(used_material[i])
					{
						material_remap[i] = index;
						index++;
					}
				}
				result->material_cnt = index;
			}

		}

		result->sorted_faces = 0;

		// populate new face groups result
		{
			result->face_group_cnt = used_group_cnt;
			result->face_groups = new FaceGroup[result->face_group_cnt];
			result->have_smooth_shaded_faces = 0;
			result->have_double_sided_faces = 0;

			GroupIndex *gi = groups.first();

			int index = 0;
			int current_vertex_offset = 0;
			for(int g_id = 0; g_id < face_group_cnt; g_id++)
			{
				if(used_group[g_id])
				{
				
					FaceGroup *old_fg = face_groups + g_id;
					FaceGroup *new_fg = result->face_groups + index;

					new_fg->material = material_remap[old_fg->material];

					FaceIndex *fi;

					if(used_link_group[g_id])
					{
						fi = gi->faces.first();
						new_fg->face_cnt = gi->faces.count();
					}
					else
					{
						fi = NULL;
						new_fg->face_cnt = 0;
					}
					
					if(used_split_group && used_split_group[g_id])
					{
						new_fg->face_cnt += split_list[g_id].f_cnt[which];
					}
					
					new_fg->face_vertex_chain = new int[3 * new_fg->face_cnt];
					new_fg->face_normal = new int[new_fg->face_cnt];
					new_fg->face_D_coefficient = new float[new_fg->face_cnt];
					new_fg->face_properties = new FACE_PROPERTY[new_fg->face_cnt];
					new_fg->face_area = new float[new_fg->face_cnt];

					int *new_face_vertex_chain = new_fg->face_vertex_chain;
					int *old_face_vertex_chain = old_fg->face_vertex_chain;

					int *new_face_normal = new_fg->face_normal;
					int *old_face_normal = old_fg->face_normal;

					float *new_face_D_coefficient = new_fg->face_D_coefficient;
					float *old_face_D_coefficient = old_fg->face_D_coefficient;

					FACE_PROPERTY* new_face_properties = new_fg->face_properties;
					FACE_PROPERTY* old_face_properties = old_fg->face_properties;

					float *new_face_area = new_fg->face_area;
					float *old_face_area = old_fg->face_area;

					int f_id = 0;
					if(used_link_group[g_id])
					{
						for(; f_id < gi->faces.count(); f_id++)
						{
							new_face_vertex_chain[3 * f_id] =
								batch_remap[ old_face_vertex_chain[3 * fi->index] ];
							new_face_vertex_chain[3 * f_id + 1] =
								batch_remap[ old_face_vertex_chain[3 * fi->index + 1] ];
							new_face_vertex_chain[3 * f_id + 2] =
								batch_remap[ old_face_vertex_chain[3 * fi->index + 2] ];

							new_face_normal[f_id] = normal_remap[ old_face_normal[fi->index] ];
							new_face_D_coefficient[f_id] = old_face_D_coefficient[fi->index];
							new_face_properties[f_id] = old_face_properties[fi->index];
							new_face_area[f_id] = old_face_area[fi->index];

							if( new_face_properties[f_id] & SMOOTH_SHADED )
							{
								result->have_smooth_shaded_faces++;
							}

							if( new_face_properties[f_id] & TWO_SIDED )
							{
								result->have_double_sided_faces++;
							}

							fi = fi->next;
						}
						ASSERT_FATAL(fi == NULL);
					}

					if(used_split_group && used_split_group[g_id])
					{
						const Split *sp = split_list + g_id;
						int sp_fid = 0;
						for(; f_id < new_fg->face_cnt; f_id++, sp_fid++)
						{
							for(int i = 0; i < 3; i++)
							{
								// old vertex
								if( !(sp->f[which][3*sp_fid + i] &  HIGH_BIT) )
								{
									new_face_vertex_chain[3 * f_id + i] =
										batch_remap[ sp->f[which][3*sp_fid + i] ];
								}
								else // vertex created by split_face
								{
									new_face_vertex_chain[3 * f_id + i] =
										(sp->f[which][3*sp_fid + i] & ~HIGH_BIT) 
										+ result->vertex_batch_cnt + current_vertex_offset;
								}
							}

							new_face_normal[f_id] = normal_remap[ sp->n[which][sp_fid] ];
							new_face_D_coefficient[f_id] = old_face_D_coefficient[ sp->orig[which][sp_fid] ];
							new_face_properties[f_id] = old_face_properties[ sp->orig[which][sp_fid] ];
							new_face_area[f_id] = -1.0f;

							if( new_face_properties[f_id] & SMOOTH_SHADED )
							{
								result->have_smooth_shaded_faces++;
							}

							if( new_face_properties[f_id] & TWO_SIDED )
							{
								result->have_double_sided_faces++;
							}	
						}

						current_vertex_offset += sp->v_cnt;
					}
					
					if(used_link_group[g_id])
					{
						gi = gi->next;
					}

					index++;
				}
			}
			ASSERT_FATAL(current_vertex_offset == split_vertex_cnt);
			ASSERT_FATAL(gi == NULL);
			ASSERT_FATAL(index == used_group_cnt);
		}

		int list_object_vertex_cnt = result->object_vertex_cnt;
		{
			result->object_vertex_cnt += split_vertex_cnt;

			result->object_vertex_list = new Vector[result->object_vertex_cnt];
			result->vertex_normal = new int[result->object_vertex_cnt];
			result->vertex_D_coefficient = new float[result->object_vertex_cnt];

			int index = 0;
			for(int ov_id = 0; ov_id < object_vertex_cnt; ov_id++)
			{
				if(used_vertex[ov_id])
				{
					result->object_vertex_list[index] = object_vertex_list[ov_id];
					result->vertex_normal[index] = normal_remap[ vertex_normal[ov_id] ];
					result->vertex_D_coefficient[index] = vertex_D_coefficient[ov_id];
					index++;
				}
			}
			ASSERT_FATAL(index == list_object_vertex_cnt);

			if (used_split_group)
			{
				for(int g_id = 0; g_id < face_group_cnt; g_id++)
				{
					if(used_split_group[g_id])
					{
						const Split *sp = split_list + g_id;

						for(unsigned int v_id = 0; v_id < sp->v_cnt; v_id++)
						{
							result->object_vertex_list[index] = sp->v[v_id];
							result->vertex_normal[index] = normal_remap[ sp->vn[v_id] ];
							result->vertex_D_coefficient[index] = sp->vD[v_id];
							index++;
						}	
					}
				}
			}
			
			// sanity check
			ASSERT_FATAL(index == result->object_vertex_cnt);
		}

		//1st texture channel
		int list_texture_vertex_cnt = result->texture_vertex_cnt;
		{
			result->texture_vertex_cnt += split_vertex_cnt;

			result->texture_vertex_list = new TexCoord[result->texture_vertex_cnt];

			int index = 0;
			for(int t_id = 0; t_id < texture_vertex_cnt; t_id++)
			{
				if(used_texture[t_id])
				{
					result->texture_vertex_list[index] = texture_vertex_list[t_id];
					index++;
				}
			}
			ASSERT_FATAL(index == list_texture_vertex_cnt);

			if (used_split_group)
			{
				for(int g_id = 0; g_id < face_group_cnt; g_id++)
				{
					if(used_split_group[g_id])
					{
						const Split *sp = split_list + g_id;

						for(unsigned int v_id = 0; v_id < sp->v_cnt; v_id++)
						{
							result->texture_vertex_list[index] = sp->t[v_id];
							index++;
						}	
					}
				}
			}
			ASSERT_FATAL(index == result->texture_vertex_cnt);
		}

		{
			int list_vertex_batch_cnt = result->vertex_batch_cnt;
			result->vertex_batch_cnt += split_vertex_cnt;

			result->vertex_batch_list = new int[result->vertex_batch_cnt];
			result->texture_batch_list = new int[result->vertex_batch_cnt];
			
			int index = 0;
			for(int i = 0; i < vertex_batch_cnt; i++)
			{
				if(used_batch[i])
				{
					result->vertex_batch_list[index] = vertex_remap[ vertex_batch_list[i] ];
					result->texture_batch_list[index] = texture_remap[ texture_batch_list[i] ];
					index++;
				}
			}

			ASSERT_FATAL(index == list_vertex_batch_cnt);

			if (used_split_group)
			{
				int vertex_offset = 0;
				for(int g_id = 0; g_id < face_group_cnt; g_id++)
				{
					if(used_split_group[g_id])
					{
						const Split *sp = split_list + g_id;

						for(unsigned int v_id = 0; v_id < sp->v_cnt; v_id++)
						{
							result->vertex_batch_list[index] = list_object_vertex_cnt + vertex_offset + v_id;
							result->texture_batch_list[index] = list_texture_vertex_cnt + vertex_offset + v_id;
							
							index++;
						}	
						vertex_offset += sp->v_cnt;
					}
				}
			}
			ASSERT_FATAL(index == result->vertex_batch_cnt);
		}

		{ 
			result->normal_ABC = new Vector[result->normal_cnt];

			int index = 0;
			for(int n_id = 0; n_id < normal_cnt; n_id++)
			{
				if(used_normal[n_id])
				{
					result->normal_ABC[index] = normal_ABC[n_id];
					index++;
				}
			}
			ASSERT_FATAL(index == result->normal_cnt);
		}

		{
			int new_material_entry=0, material_entry=0;

			result->material_list = new Material[result->material_cnt];

			while( material_entry < material_cnt ) {
				
				if( used_material[material_entry] ) {
					result->material_list[new_material_entry].copy_from( &material_list[material_entry] );
					new_material_entry++;
				}
				
				material_entry++;
			}

			ASSERT_FATAL( new_material_entry == result->material_cnt );
		}

		for(FaceGroup *fg = result->face_groups; fg < result->face_groups + result->face_group_cnt; fg++)
		{
			for(int fid = 0; fid < fg->face_cnt; fid++)
			{
				if(fg->face_area[fid] <= 0.0f)
				{
					fg->face_area[fid] = result->compute_face_area(fg, fid);
				}
			}
		}

		delete [] used_link_group;
		delete [] used_group;
		//delete [] group_remap;

		delete [] used_batch;
		delete [] used_vertex;
		delete [] used_texture;
		delete [] used_texture2;
		delete [] used_normal;
		delete [] used_material;

		delete [] batch_remap;
		delete [] vertex_remap;
		delete [] texture_remap;
		delete [] texture_remap2;
		delete [] normal_remap;
		delete [] material_remap;

		result->compute_sphere ();
		result->compute_bounds ();  
		result->compute_centroid ();
	}

	delete [] used_split_group;
	return result;
}


// point to static data of mesh & make local copies for changing data
void XMesh::copy_lod_mesh(const XMesh & source)
{
	// now actually copy some data locally
	copy_partial_mesh(source);

	// this has to be at the highest detail level
	lod_fraction = 1.0f;
	lod_fraction_previous = 1.0f;
	lod_threshold = 0.0001f;
	allow_lod_change = true;

	// set up uv indexing for lod interpolation
	if(uv_chain_count > 0)
	{
		set_uv_indices(lod_step_cnt - 1);
		interpolate_lod_step(lod_step_cnt - 1, 1.0f);
	}
}

void XMesh::copy_partial_mesh(const XMesh & source)
{
	const float save_fraction = source.lod_fraction;
	((XMesh&)source).set_lod( 1.0f );

	// point to original data
	*this = source;

	// now make some local copies
	// copy face groups
	face_groups = new FaceGroup[face_group_cnt];
	for(int g_id = 0; g_id < face_group_cnt; g_id++)
	{
		face_groups[g_id] = source.face_groups[g_id]; // = operator mallocs arrays
	}

	// copy vertices
	object_vertex_list = new Vector[object_vertex_cnt];
	memcpy(object_vertex_list, source.object_vertex_list, object_vertex_cnt * sizeof(Vector));
	vertex_normal = new int[object_vertex_cnt];
	memcpy(vertex_normal, source.vertex_normal, object_vertex_cnt * sizeof(int));
	vertex_D_coefficient = new float[object_vertex_cnt];
	memcpy(vertex_D_coefficient, source.vertex_D_coefficient, object_vertex_cnt * sizeof(float));
	if(source.vertex_color_list)
	{
		vertex_color_list = new unsigned char[3 * object_vertex_cnt];
		memcpy(vertex_color_list, source.vertex_color_list, 3 * object_vertex_cnt);
	}
	vertex_batch_list = new int[vertex_batch_cnt];
	memcpy(vertex_batch_list, source.vertex_batch_list, vertex_batch_cnt * sizeof(int));

	// UV lod stuff
	texture_vertex_list = new TexCoord[texture_vertex_cnt];
	memcpy(texture_vertex_list, source.texture_vertex_list, texture_vertex_cnt * sizeof(TexCoord));
	texture_batch_list = new int[vertex_batch_cnt];
	memcpy(texture_batch_list, source.texture_batch_list, vertex_batch_cnt * sizeof(int));

	if(source.texture_batch_list2)
	{
		texture_batch_list2 = new int[vertex_batch_cnt];
		memcpy(texture_batch_list2, source.texture_batch_list2, vertex_batch_cnt * sizeof(int));
	}


	// Material stuff
	material_list = new Material[material_cnt];
	for(int mid = 0; mid < material_cnt; mid++)
	{
		material_list[mid].copy_from(source.material_list + mid);
	}

	((XMesh&)source).set_lod( save_fraction );
}

// only free the XMesh components used by local_mesh
void XMesh::release_partial_mesh(void)
{
	delete [] face_groups; // face group has a destructor
	
	delete [] object_vertex_list;
	delete [] vertex_normal;
	delete [] vertex_D_coefficient;
	delete [] vertex_batch_list;
	delete [] vertex_color_list;

	delete [] texture_vertex_list;
	delete [] texture_batch_list;
	delete [] texture_batch_list2;

	delete [] material_list;	// destructor releases txmlib references

	// prevent ~XMesh from freeing arrays that are pointed to by the static archetype mesh
	memset(this, 0, sizeof(*this));
}

void XMesh::copy_full_mesh(const XMesh & source, ITextureLibrary *_txm_lib )
{
	
	face_cnt = source.face_cnt;

	face_group_cnt = source.face_group_cnt;
	face_groups = new FaceGroup[face_group_cnt];
	
	int i;
	for (i = 0; i < face_group_cnt; i++)
	{
		face_groups[i] = source.face_groups[i]; // = operator mallocs new arrays
	}
	
	object_vertex_cnt = source.object_vertex_cnt;
	object_vertex_list = new Vector[object_vertex_cnt];
	memcpy(object_vertex_list, source.object_vertex_list, sizeof(Vector) * object_vertex_cnt);

	vertex_batch_cnt = source.vertex_batch_cnt;
	vertex_batch_list = new int[vertex_batch_cnt];
	memcpy(vertex_batch_list, source.vertex_batch_list, sizeof(int) * vertex_batch_cnt);

	texture_vertex_cnt = source.texture_vertex_cnt;
	texture_vertex_list = new TC_UVCOORD[texture_vertex_cnt];
	memcpy(texture_vertex_list, source.texture_vertex_list, sizeof(TC_UVCOORD) * texture_vertex_cnt);

	texture_batch_list = new int[vertex_batch_cnt];
	memcpy(texture_batch_list, source.texture_batch_list, sizeof(int) * vertex_batch_cnt);

	if(source.texture_batch_list2)
	{
		texture_batch_list2 = new int[vertex_batch_cnt];
		memcpy(texture_batch_list2, source.texture_batch_list2, sizeof(int) * vertex_batch_cnt);
	}

	tc_user_generator = source.tc_user_generator;
	tc_user_generator_context = source.tc_user_generator_context;

	normal_cnt = source.normal_cnt;
	normal_ABC = new Vector[normal_cnt];
	memcpy(normal_ABC, source.normal_ABC, sizeof(Vector) * normal_cnt);

	vertex_normal = new int[object_vertex_cnt];
	memcpy(vertex_normal, source.vertex_normal, sizeof(int) * object_vertex_cnt);

	vertex_D_coefficient = new float[object_vertex_cnt];
	memcpy(vertex_D_coefficient, source.vertex_D_coefficient, sizeof(float) * object_vertex_cnt);

	if(source.vertex_color_list)
	{
		vertex_color_list = new unsigned char[3 * object_vertex_cnt];
		memcpy(vertex_color_list, source.vertex_color_list, 3 * sizeof(unsigned char) * object_vertex_cnt);
	}
	else
	{
		vertex_color_list = NULL;
	}

	if(source.edge_cnt > 0)
	{
		edge_cnt = source.edge_cnt;
		edge_vertex_chain = new int[2 * edge_cnt];
		memcpy(edge_vertex_chain, source.edge_vertex_chain, sizeof(int) * 2 * edge_cnt);

		edge_angle_list = new float[edge_cnt];
		memcpy(edge_angle_list, source.edge_angle_list, sizeof(float) * edge_cnt);
	}

#if 0
	material_cnt = source.material_cnt;
	material_list = new Material[material_cnt];
	memcpy(material_list, source.material_list, sizeof(Material) * material_cnt);

	Material * mat = material_list;
	for (i = 0; i < material_cnt; i++, mat++)
	{
		txmlib->add_ref_texture_ref( mat->diffuse_texture_ref);
		txmlib->add_ref_texture_ref( mat->second_diffuse_texture_ref);
		txmlib->add_ref_texture_ref( mat->emissive_texture_ref );
	}
#else
	material_cnt = source.material_cnt;
	material_list = new Material[material_cnt];
	for( i=0; i<material_cnt; i++ ) {
		material_list[i].copy_from( &source.material_list[i] );
	}
#endif


	if(source.uv_channel_cnt > 0)
	{
		uv_channel_cnt = source.uv_channel_cnt;
		uv_channel_list = new UVChannel[uv_channel_cnt];

		for(i = 0; i < uv_channel_cnt; i++)
		{
			uv_channel_list[i] = source.uv_channel_list[i]; // = operator mallocs new arrays
		}
	}


	lod_fraction = source.lod_fraction;
	lod_fraction_previous = source.lod_fraction_previous;
	lod_threshold = source.lod_threshold;
	if(source.lod_step_cnt > 0)
	{
		lod_step_cnt = source.lod_step_cnt;
		lod_step_list = new lod_step[lod_step_cnt];
		memcpy(lod_step_list, source.lod_step_list, sizeof(lod_step) * lod_step_cnt);

		lod_removed_face_cnt = source.lod_removed_face_cnt;
		lod_removed_face_list = new int[lod_removed_face_cnt];
		memcpy(lod_removed_face_list, source.lod_removed_face_list, sizeof(int) * lod_removed_face_cnt);

		lod_morphed_face_cnt = source.lod_morphed_face_cnt;
		lod_morphed_face_list = new int[lod_morphed_face_cnt];
		memcpy(lod_morphed_face_list, source.lod_morphed_face_list, sizeof(int) * lod_morphed_face_cnt);

		lod_group_face_list = new int[lod_morphed_face_cnt];
		memcpy(lod_group_face_list, source.lod_group_face_list, sizeof(int) * lod_morphed_face_cnt);

		lod_high_face_normal_list = new int[lod_morphed_face_cnt];
		memcpy(lod_high_face_normal_list, source.lod_high_face_normal_list, sizeof(int) * lod_morphed_face_cnt);

		lod_high_face_D_coefficient = new float[lod_morphed_face_cnt];
		memcpy(lod_high_face_D_coefficient, source.lod_high_face_D_coefficient, sizeof(int) * lod_morphed_face_cnt);

		lod_low_face_normal_list = new int[lod_morphed_face_cnt];
		memcpy(lod_low_face_normal_list, source.lod_low_face_normal_list, sizeof(int) * lod_morphed_face_cnt);

		lod_low_face_D_coefficient = new float[lod_morphed_face_cnt];
		memcpy(lod_low_face_D_coefficient, source.lod_low_face_D_coefficient, sizeof(int) * lod_morphed_face_cnt);

		lod_vertex_cnt = source.lod_vertex_cnt;
		lod_vertex_list = new int[lod_vertex_cnt];
		memcpy(lod_vertex_list, source.lod_vertex_list, sizeof(int) * lod_vertex_cnt);


		if(source.lod_uv_count > 0)
		{
			lod_uv_step_list = new lod_uv_step[lod_step_cnt];
			memcpy(lod_uv_step_list, source.lod_uv_step_list, sizeof(lod_uv_step) * lod_step_cnt);

			uv_chain_count = source.uv_chain_count;
			lod_uv_batch_chain = new int[uv_chain_count];
			memcpy(lod_uv_batch_chain, source.lod_uv_batch_chain, sizeof(int) * uv_chain_count);

			lod_uv_count = source.lod_uv_count;
			lod_uv_high_id1 = new int[lod_uv_count];
			memcpy(lod_uv_high_id1, source.lod_uv_high_id1, sizeof(int) * lod_uv_count);

			lod_uv_batch_cnt1 = new int[lod_uv_count];
			memcpy(lod_uv_batch_cnt1, source.lod_uv_batch_cnt1, sizeof(int) * lod_uv_count);

			lod_uv_batch_first1 = new int[lod_uv_count];
			memcpy(lod_uv_batch_first1, source.lod_uv_batch_first1, sizeof(int) * lod_uv_count);

			lod_uv_high_id2 = new int[lod_uv_count];
			memcpy(lod_uv_high_id2, source.lod_uv_high_id2, sizeof(int) * lod_uv_count);

			lod_uv_batch_cnt2 = new int[lod_uv_count];
			memcpy(lod_uv_batch_cnt2, source.lod_uv_batch_cnt2, sizeof(int) * lod_uv_count);

			lod_uv_batch_first2 = new int[lod_uv_count];
			memcpy(lod_uv_batch_first2, source.lod_uv_batch_first2, sizeof(int) * lod_uv_count);

			lod_uv_low_id = new int[lod_uv_count];
			memcpy(lod_uv_low_id, source.lod_uv_low_id, sizeof(int) * lod_uv_count);	
		}


		if(source.lod_edge_cnt > 0)
		{
			lod_edge_cnt = source.lod_edge_cnt;
			lod_edge_list = new int[lod_edge_cnt];
			memcpy(lod_edge_list, source.lod_edge_list, sizeof(int) * lod_edge_cnt);
		}
	}

//	txmlib = source.txmlib;

	radius = source.radius;
	sphere_center = source.sphere_center;

	memcpy(bounds, source.bounds, sizeof(float) * 6);

	centroid = source.centroid;
}
