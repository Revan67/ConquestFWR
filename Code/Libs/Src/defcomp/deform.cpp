//
// Deformable object stuff.
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <limits.h>
#include <stdio.h>

#include "eng.h"
#include "IDeformable.h"
#include "deform.h"
#include "ICamera.h"
#include "system.h"

#include "heapobj.h"
#include "material.h"
#include "iprofileparser_utility.h"
#include "packed_argb.h"

#include "IChannel2.h"

//

using namespace Deform;

bool	use_constant_alpha = false;
U8		constant_alpha = 255;

void Deform::DebugPrint(char *fmt, ...)
{
	if (fmt)
	{
		char work[256];

		va_list va;
		va_start(va,fmt);
		vsprintf(work,fmt,va);
		va_end(va);

		int length = strlen(work);
		work[length] = '\n';
		work[length+1] = 0;

		GENERAL_TRACE_1( work );
	}
}

//
#ifdef _DEBUG
void Deform::TrapFpu(bool on)
{
	_clear87();

	if(on)
	{
		// prepare to trap floating point exceptions
		unsigned int control_word = _controlfp (0, 0);

		control_word &= ~(EM_INVALID | EM_OVERFLOW | EM_ZERODIVIDE);
		_control87 (control_word, MCW_EM);
	}
	else
	{
		_control87(_CW_DEFAULT, 0xfffff);
	}
}
#else
void Deform::TrapFpu(bool) {}
#endif

const char *CLSID_Deform = "Deform";
void Deform::get_ini_info( void )
{
	got_ini_info = false;

	ICOManager *DACOM = DACOM_Acquire();

	if(PIPE)
	{
		U32 value, modes[4];
		
		PIPE->query_device_ability( RP_A_DEVICE_SOFTWARE, &value );
		
		if( value == 0xFFFFFFFF ) {
			GENERAL_WARNING( "POLYMESH: Trying to create archetypes/istances before calling IRenderPipeline::startup, YMMV." );
		}
		
		if( value ) {
			value = MF_NO_DIFFUSE2_PASS | MF_NO_EMITTER_PASS | MF_NO_SPECULAR_PASS;
		}
		else {
			value = 0;
		}
		opt_get_u32( DACOM, NULL, CLSID_Deform, "DefaultMaterialFlags", value, &default_material_flags );

		PIPE->query_device_ability( RP_A_TEXTURE_STAGES, &value );
		opt_get_u32( DACOM, NULL, CLSID_Deform, "NumTSS", value, &device_num_tss );

		PIPE->query_device_ability( RP_A_TEXTURE_COORDINATES, &value );
		value = (value==1)? FALSE : TRUE;
		opt_get_u32( DACOM, NULL, CLSID_Deform, "Diffuse2AllowTC2", value, &device_supports_uvchannel1 );

		PIPE->query_device_ability( RP_A_BLEND_MUL_SRC, &value );
		opt_get_u32( DACOM, NULL, CLSID_Deform, "Diffuse2SrcBlend", value, &diffuse2_fallback_blend[0] );
		
		PIPE->query_device_ability( RP_A_BLEND_MUL_DST, &value );
		opt_get_u32( DACOM, NULL, CLSID_Deform, "Diffuse2DstBlend", value, &diffuse2_fallback_blend[1] );

		PIPE->query_device_ability( RP_A_BLEND_ADD_SRC, &value );
		opt_get_u32( DACOM, NULL, CLSID_Deform, "EmissiveSrcBlend", value, &emissive_fallback_blend[0] );
		
		PIPE->query_device_ability( RP_A_BLEND_ADD_DST, &value );
		opt_get_u32( DACOM, NULL, CLSID_Deform, "EmissiveDstBlend", value, &emissive_fallback_blend[1] );

		PIPE->query_device_ability( RP_A_BLEND_MATRIX, modes );

		opt_get_u32( DACOM, NULL, CLSID_Deform, "SpecularSrcBlend", D3DBLEND_SRCALPHA, &specular_fallback_blend[0] );
		opt_get_u32( DACOM, NULL, CLSID_Deform, "SpecularDstBlend", D3DBLEND_ONE, &specular_fallback_blend[1] );

		if( !rp_a_is_blend_supported( (D3DBLEND)specular_fallback_blend[0], (D3DBLEND)specular_fallback_blend[1], modes ) ) {
			specular_fallback_blend[0] = D3DBLEND_ONE;
			specular_fallback_blend[1] = D3DBLEND_ONE;
		}
	}

	opt_get_float( DACOM, NULL, CLSID_Deform, "PixelError", 1.0f, &pixel_error );

	got_ini_info = true;
}

void Deform::delete_pools( void )
{
	delete [] vertex_pool;
	vertex_pool = NULL;

	delete [] normal_pool;
	normal_pool = NULL;

	delete [] normal_index_pool;
	normal_index_pool = NULL;

	delete [] light_pool;
	light_pool = NULL;

	delete [] normal_pool_cnt;
	normal_pool_cnt = NULL;
}

void Deform::verify_pools(const int size)
{
	if (vertex_pool_len < size)
	{
		delete_pools();

		vertex_pool_len = size;
		vertex_pool = new MTVERTEX[vertex_pool_len];
		normal_pool = new Vector[vertex_pool_len];
		light_pool = new LightRGB[vertex_pool_len];
		normal_index_pool = new U32[vertex_pool_len];
		for(int i = 0; i < vertex_pool_len; i++)
		{
			normal_index_pool[i] = i;
		}
		normal_pool_cnt = new int[vertex_pool_len];
	}
}

void Deform::delete_lists( void )
{
	delete [] index_list;
	index_list = NULL;

	delete [] vertex_slot;
	vertex_slot = NULL;
}

void Deform::verify_lists(const int size)
{
	if (index_list_len < size)
	{
		delete_lists();

		index_list_len = size;
		index_list = new U16[index_list_len];
		vertex_slot = new U16[index_list_len];
	}
}

void Deform::delete_scratch_pools( void )
{
	delete [] scratch_xyz;
	scratch_xyz = NULL;
	delete[] scratch_uv;
	scratch_uv = NULL;
	delete [] scratch_xyz_rp_idx;
	scratch_xyz_rp_idx = NULL;
	delete [] scratch_xyz_uv_idx;
	scratch_xyz_uv_idx = NULL;

	scratch_pool_len = 0;
}

void Deform::verify_scratch_pools(const int size)
{
	if (scratch_pool_len < size)
	{
		delete_scratch_pools();

		scratch_pool_len = size;
		scratch_xyz = new Vector[scratch_pool_len];
		scratch_uv = new TexCoord[scratch_pool_len];
		scratch_xyz_rp_idx = new int[scratch_pool_len];
		scratch_xyz_uv_idx = new int[scratch_pool_len];
	}
}

void Deform::delete_patch_lists( void )
{
	delete [] patch_list;
	patch_list = NULL;
	delete [] patch_normals;
	patch_normals = NULL;
	delete [] patch_D;
	patch_D = NULL;
	delete [] patch_corners;
	patch_corners = NULL;
	delete [] patch_depth;
	patch_depth = NULL;
	delete [] patch_alt_tri_style;
	patch_alt_tri_style = NULL;
	patch_list_len = 0;
}

void Deform::verify_patch_lists(const int size)
{
	if(size > patch_list_len)
	{
		delete_patch_lists();
		patch_list_len = size;
		patch_list = new BezierPatch[patch_list_len];
		patch_normals = new Vector_4[patch_list_len];
		patch_D = new float_4[patch_list_len];
		patch_corners = new Corner[patch_list_len];
		patch_depth = new int[patch_list_len];
		patch_alt_tri_style = new bool[patch_list_len];
	}
}

void Deform::delete_edge_lists( void )
{
	delete [] edge_list;
	edge_list = NULL;
	edge_list_len = 0;
}

void Deform::verify_edge_lists(const int size)
{
	if(size > edge_list_len)
	{
		delete_edge_lists();
		edge_list_len = size;
		edge_list = new StitchEdge[edge_list_len];
	}
}

//

LList<DeformablePartArchetype>	DeformableObject::archetypes;

bool DeformOpen(IDAComponent * system, IEngine * engine)
{
	if( active ) {
		return true;
	}

	active = false;

	DACOM = DACOM_Acquire();

	ASSERT( DACOM );
	ASSERT( system );
	ASSERT( engine );

	// get system interfaces
	if( FAILED( system->QueryInterface( IID_IRenderPrimitive, (void**) &BATCH ) ) ) {
		GENERAL_ERROR( "Deform: Initialize: Unable to acquire IID_IRenderPrimitive, unable to continue." );
		return false;
	}

	if( FAILED( system->QueryInterface( IID_IRenderPipeline, (void**) &PIPE ) ) ) {
		GENERAL_ERROR( "Deform: Initialize: Unable to acquire IID_IRenderPrimitive, unable to continue." );
		return false;
	}

	if( FAILED( system->QueryInterface( IID_ITextureLibrary, (void**) &TXMLIB ) ) ) {
		GENERAL_ERROR( "Deform: Initialize: Unable to acquire IID_ITextureLibrary, unable to continue." );
		return false;
	}

	if( FAILED( system->QueryInterface( IID_ILightManager, (void**) &LIGHT) ) ) {
		GENERAL_WARNING("Deform: Initialize: Unable to acquire IID_ILightManager, continuing without lighting." );
	}

	// get engine interfaces
	if( FAILED( engine->QueryInterface( IID_IEngine, (void**) &ENG ) ) ) {
		GENERAL_ERROR( "Deform: Initialize: Unable to acquire IID_IEngine, unable to continue" );
		return false;
	}
	ENG->Release();

	if( FAILED( engine->QueryInterface( IID_IAnimation, (void**) &ANIM ) ) ) {
		GENERAL_ERROR( "Deform: Initialize: Unable to acquire IID_IAnimation, unable to continue" );
		return false;
	}
	ANIM->Release();	// release peer reference

	if( FAILED( engine->QueryInterface( IID_IChannel, (void**) &CHANNEL ) ) ) {
		GENERAL_ERROR( "Deform: Initialize: Unable to acquire IID_IChannel, unable to continue" );
		return false;
	}
	CHANNEL->Release();	// release peer reference

	if( FAILED( engine->QueryInterface( IID_IHardpoint, (void**) &HARDPOINT ) ) ) {
		GENERAL_ERROR( "Deform: Initialize: Unable to acquire IID_IHardpoint, unable to continue" );
		return false;
	}
	HARDPOINT->Release();	// release peer reference

	if( FAILED( engine->QueryInterface( IID_IPhysics, (void**) &PHYSICS ) ) ) {
		GENERAL_NOTICE( "Deform: Initialize: Unable to acquire IID_IPhysics" );
	}
	else
	{
		PHYSICS->Release();	// release peer reference
	}

	if( FAILED( engine->QueryInterface( IID_ICollision, (void**) &COLLIDE ) ) ) {
		GENERAL_NOTICE( "Deform: Initialize: Unable to acquire IID_ICollision" );
	}
	else
	{
		COLLIDE->Release();	// release peer reference
	}

	// get options from ini file
	//
	opt_get_u32( DACOM, NULL, CLSID_Deform, "SpecularMode", 0, &specular_mode );
	opt_get_string( DACOM, NULL, CLSID_Deform, "SpecularTextureName",	"SpecularHighlight", specular_texture_name, 64 );

	pb.SetIRenderPrimitive(BATCH);

	active = true;

	return true;
}

//

IKScriptLink::~IKScriptLink(void)
{
	for (U32 i = 0; i < num_joints; i++)
	{
		CHANNEL->destroy_channel_instance(channels[i]);
	}

	num_joints = 0;
	delete [] joints;
	joints = NULL;
	delete [] channels;
	channels = NULL;
	delete [] data;
	data = NULL;
	delete [] Rmid;
	Rmid = NULL;
	delete [] locked;
	locked = NULL;
}

//

void ScriptLink::release(void)
{
	if (ik_script)
	{
		delete ik;
		ik = NULL;
	}
	else
	{
		ANIM->release_script_inst(instance);
		instance = INVALID_SCRIPT_INST;
	}
}

//

void DeformClose(void)
{
	if( !active ) {
		return;
	}

	Deform::pb.SetIRenderPrimitive( NULL );
	Deform::pb.~MTPrimitiveBuilder();

	// release interfaces we got from the system
	//
	DACOM_RELEASE( BATCH );
	DACOM_RELEASE( PIPE );
	DACOM_RELEASE( TXMLIB );
	DACOM_RELEASE( LIGHT );

	DACOM_RELEASE( DACOM );

	got_ini_info = false;

	delete_pools();
	delete_lists();
	delete_scratch_pools();
	delete_patch_lists();
	delete_edge_lists();

	delete [] rgb;
	rgb = NULL;
	rgb_count = 0;

	delete [] DoubleSided;
	DoubleSided = NULL;
	double_count = 0;

	active = false;
}

//

char * BoneDescriptor::mesh_name = NULL;

//

void BoneInstance::InitBoneInstance(const BoneArchetype * _arch, const bool need_normals)
{
	arch = _arch;
	vertex_counter = 0;

	if (arch->num_vertices)
	{
		transformed_vertices = new Vector[arch->num_vertices];
		if(need_normals)
		{
			transformed_normals = new Vector[arch->num_vertices];
		}
		else	// patches don't need normals
		{
			transformed_normals = NULL;
		}
	}
	else
	{
		transformed_vertices = NULL;
		transformed_normals = NULL;
	}
}

//

void DeformablePart::build_tree(INSTANCE_INDEX * tree, INSTANCE_INDEX root, int & n)
{
	tree[n++] = root;

	INSTANCE_INDEX child = INVALID_INSTANCE_INDEX;
	while( (child = ENG->get_instance_child_next( root, EN_DONT_RECURSE, child )) != INVALID_INSTANCE_INDEX )
	{
		build_tree(tree, child, n);
	}
}

//

DeformablePart::DeformablePart(int num_archs, DeformablePartArchetype * _arch[], IEngineInstance * user_instance)
{
	num_meshes = num_archs;
	meshes = new DeformablePartMesh[num_meshes];

	need_normals = false;
	for (int i = 0; i < num_meshes; i++)
	{
		meshes[i].arch = _arch[i];
		if( !_arch[i]->is_patch() )
		{
			need_normals = true;
		}
	}

	last_face_cnt =
	last_vertex_cnt = 0;
	pixel_error = Deform::pixel_error;

// assumes all archetypes use the same engine archetype (skeleton).
	if (user_instance)
	{
		const DeformablePartArchetype * arch = meshes[0].arch;

	// create base tree.
		root = ENG->create_instance2(arch->arch_index, user_instance);
	}
	else
	{
		const DeformablePartArchetype * arch = meshes[0].arch;

	// create base tree.
		root = ENG->create_instance2(arch->arch_index, NULL);
	}

	if (root != INVALID_INSTANCE_INDEX)
	{
		DeformablePartArchetype * arch0 = meshes[0].arch;

	// create extra bones.
		int eidx = 0;
		for (i = 0; i < arch0->num_extras; i++)
		{
			if (arch0->extras[i] != INVALID_ARCHETYPE_INDEX)
			{
				extras[eidx++] = ENG->create_instance2(arch0->extras[i], NULL);
			}
		}

		INSTANCE_INDEX * tree = new INSTANCE_INDEX[meshes[0].arch->num_bones];
		int n = 0;
		build_tree(tree, root, n);

		for (int m = 0; m < num_meshes; m++)
		{
			const DeformablePartArchetype * arch = meshes[m].arch;

			int num_extras_attached = 0;

			meshes[m].bones = new BoneInstance [arch->num_bones];
			for (int i = 0; i < arch->num_bones; i++)
			{
				BoneArchetype * barch = arch->bones + i;

				meshes[m].bones[i].InitBoneInstance(barch, !arch->is_patch());

				BoneInstance * binst = &(meshes[m].bones[i]);

				if (barch->extra)
				{
					binst->instance = extras[num_extras_attached++];
				}
				else
				{
					char * name = barch->name;
					if (strlen(name))
					{
					// search tree for bone.
						for (int j = 0; j < arch->num_bones; j++)
						{
							if( !strcmp( ENG->get_instance_part_name( tree[j] ), name) )
							{
								binst->instance = tree[j];
								break;
							}
						}
					}
				}
			}
		}

		delete [] tree;
		tree = NULL;
	}

	int max_verts = -1;
	for (i = 0; i < num_meshes; i++)
	{
		if (meshes[i].arch->object_vertex_cnt > max_verts)
		{
			max_verts = meshes[i].arch->object_vertex_cnt;
		}
	}

	transformed_vertices = new Vector[max_verts];
	if( need_normals )
	{
		transformed_normals = new Vector[max_verts];
	}
	else
	{
		transformed_normals = NULL;
	}
}

//

DeformablePart::~DeformablePart(void)
{
	for (int m = 0; m < num_meshes; m++)
	{
		delete [] meshes[m].bones;
		meshes[m].bones = NULL;
	}

	delete [] meshes;
	meshes = NULL;

	delete [] transformed_vertices;
	transformed_vertices = NULL;
	delete [] transformed_normals;
	transformed_normals = NULL;
}

//

BOOL32 DeformablePart::intersect_ray(Vector & intersection, Vector & normal, const Vector & origin, const Vector & direction, int & num_bones_hit, INSTANCE_INDEX * bones_hit, int mesh_index) const
{
	BOOL32 result = FALSE;

	num_bones_hit = 0;

	DeformablePartArchetype * a = meshes[0].arch;

	{
		const FaceGroup *	gmin = NULL;
		int					fmin;
		float				tmin = FLT_MAX;
		Vector				pmin;
		Vector				emin[2];

		for (int i = 0; i < a->num_bones; i++)
		{
			BoneInstance * bone = &(meshes[mesh_index].bones[i]);

			const BaseExtent * extent;
			if (PHYSICS && PHYSICS->get_extent(&extent, bone->instance) && extent)
			{
				Transform T = ENG->get_transform(bone->instance);
				T.set_position(PHYSICS->get_center_of_mass(bone->instance));

				Vector N;
				if (COLLIDE && COLLIDE->intersect_ray_with_extent_hierarchy(intersection, N, origin, direction, *extent, T, true))
				{
					if (bones_hit)
					{
						bones_hit[num_bones_hit] = bone->instance;
					}
					num_bones_hit++;
				// Check ray against faces associated with bone.
					const int * face = bone->arch->faces;
					for (int j = 0; j < bone->arch->num_faces; j++, face++)
					{
					// Do ray-face intersection using transformed vertices.
						const FaceGroup * group = a->face_groups + a->face_group_lookup[*face];
						int group_index = a->face_group_index_lookup[*face];
						int chain_index = group_index * 3;

						int vert0 = a->vertex_batch_list[group->face_vertex_chain[chain_index+0]];
						int vert1 = a->vertex_batch_list[group->face_vertex_chain[chain_index+1]];
						int vert2 = a->vertex_batch_list[group->face_vertex_chain[chain_index+2]];

						const Vector * v0 = transformed_vertices + vert0;
						const Vector * v1 = transformed_vertices + vert1;
						const Vector * v2 = transformed_vertices + vert2;

						Vector edge1 = *v1 - *v0;
						Vector edge2 = *v2 - *v0;

						Vector pvec = cross_product(direction, edge2);
						float det = dot_product(edge1, pvec);

						if (group->face_properties[group_index] & TWO_SIDED)
						{
						// non-culling version.
							if (fabs(det) > FLT_EPSILON)
							{
								float inv_det = 1.0/det;

								Vector tvec = origin - *v0;
								float u = dot_product(tvec, pvec) * inv_det;
								if ((u >= 0.0f) && (u <= 1.0f))
								{
									Vector qvec = cross_product(tvec, edge1);
									float v = dot_product(direction, qvec) * inv_det;
									if ((v >= 0.0f) && ((u+v) <= 1.0f))
									{
										result = TRUE;

										float t = dot_product(edge2, qvec) / det;
										if (t < tmin)
										{
											tmin = t;
											pmin = origin + t * direction;
											emin[0] = edge1;
											emin[1] = edge2;

											gmin = group;
											fmin = group_index;
										}
									}
								}
							}
						}
						else
						{
						// culling version.
							if (det > FLT_EPSILON)
							{
								Vector tvec = origin - *v0;
								float u = dot_product(tvec, pvec);
								if ((u >= 0.0f) && (u <= det))
								{
									Vector qvec = cross_product(tvec, edge1);
									float v = dot_product(direction, qvec);
									if ((v >= 0.0f) && ((u+v) <= det))
									{
										result = TRUE;
										float t = dot_product(edge2, qvec) / det;

										if (t < tmin)
										{
											tmin = t;
											pmin = origin + t * direction;
											emin[0] = edge1;
											emin[1] = edge2;

											gmin = group;
											fmin = group_index;
										}
									}
								}
							}
						}
					}
				}
			}
		}

		if (result)
		{
			intersection = pmin;
			normal = cross_product(emin[0], emin[1]);

			if (gmin->face_properties[fmin] & TWO_SIDED)
			{
			// make sure we get the right normal on double-sided faces.
				if (dot_product(normal, direction) > 0)
				{
					normal = -normal;
				}
			}
		}
	}

	return result;
}

//

DeformableObject::DeformableObject(void)
{
	parts = NULL;
	root = INVALID_INSTANCE_INDEX;

// heading X side = up.

	heading_axis = POS_Z;
	up_axis = POS_Y;

	side_axis = POS_X;

	floor_height = 0;

	num_hardpoints = 0;
	hardpoints = NULL;

	last_height = 0;
}

//

DeformableObject::~DeformableObject(void)
{
	destroy();
}

//

namespace Deform
{
	struct EnumerateScriptInfo
	{
		DeformableObject *	obj;
		DeformablePart *	part;
	};
};

	
//

static IFileSystem * CreateFileSystem(IFileSystem * parent, const char * filename)
{
	IFileSystem * result;
	DAFILEDESC desc = filename;
	if (parent)
	{
		parent->CreateInstance(&desc, (void **) &result);
	}
	else
	{
		DACOM->CreateInstance(&desc, (void **) &result);
	}
	return result;
}

//
// Allocates and reads.
//
bool LoadChild(void ** buffer, IFileSystem * parent, const char * child_name)
{
	bool result = false;
	if (parent)
	{
		DAFILEDESC desc = child_name;
		HANDLE h = parent->OpenChild(&desc);
		if (h != INVALID_HANDLE_VALUE)
		{
			DWORD size = parent->GetFileSize(h, NULL);
			*buffer = malloc(size);
			DWORD bytes_read;
			parent->ReadFile(h, *buffer, size, &bytes_read);
			if (bytes_read == size)
			{
				result = true;
			}
			else
			{
				free(*buffer);
				*buffer = NULL;
			}
			parent->CloseHandle(h);
		}
	}
	return result;
}

//
// Reads into existing buffer.
//
bool ReadChild(void * buffer, IFileSystem * parent, const char * child_name)
{
	bool result = false;
	if (parent)
	{
		DAFILEDESC desc = child_name;
		HANDLE h = parent->OpenChild(&desc);
		if (h != INVALID_HANDLE_VALUE)
		{
			DWORD size = parent->GetFileSize(h, NULL);
			DWORD bytes_read;
			parent->ReadFile(h, buffer, size, &bytes_read);
			if (bytes_read == size)
			{
				result = true;
			}
			parent->CloseHandle(h);
		}

	}
	return result;
}

//

void DeformablePartArchetype::setup(IFileSystem * sk_parent, const char * sk_name, BoneDescriptor * bDesc, int nbones)
{
	ASSERT(bDesc);

	num_bones = nbones;
	bones = new BoneArchetype[num_bones];

//
// remap bone descriptor indices in order of index, since in general bDesc[i].index != i
//
	BoneDescriptor * remap[256];
	for (int i = 0; i < num_bones; i++)
	{
		for (int j = 0; j < num_bones; j++)
		{
			if (bDesc[j].index == i)
			{
				remap[i] = &bDesc[j];
				break;
			}
		}
	}

// 1. Count up vertices for each bone.
	int * id = bone_id_list;
	for (i = 0; i < object_vertex_cnt; i++)
	{
		int n = vertex_bone_cnt[i];
		for (int j = 0; j < n; j++, id++)
		{
			if (*id >= 0 && *id < num_bones)
			{
				remap[*id]->num_vertices++;
			}
			else
			{
				GENERAL_ERROR("DEFOMABLE: Bone out of range in DeformablePartArchetype::setup().\n");
			}
		}
	}

// 2. Allocate vertex buffers.
	for (i = 0; i < num_bones; i++)
	{
		if (bDesc[i].num_vertices)
		{
			bDesc[i].vertices = new Vector[bDesc[i].num_vertices];
			if( !is_patch() )
			{
				bDesc[i].normals = new Vector[bDesc[i].num_vertices];
			}
		}
		bDesc[i].vertex_counter = 0;
	}

// 3. Copy vertices & normals to structs.
	id = bone_id_list;
	Vector * v = bone_vertex_list;
	Vector * N = bone_normal_list;
	for (i = 0; i < object_vertex_cnt; i++)
	{
		int n = vertex_bone_cnt[i];
		for (int j = 0; j < n; j++, id++, v++, N++)
		{
			BoneDescriptor * bd = remap[*id];
			bd->vertices[bd->vertex_counter] = *v;
			if(bd->normals) // no normals for patches
			{
				bd->normals[bd->vertex_counter] = *N;
			}
			bd->vertex_counter++;
		}
	}

// Initialize bone archetypes.
	BoneArchetype * bone = bones;
	for (i = 0; i < num_bones; i++, bone++)
	{
		BoneDescriptor * desc = remap[i];
		bone->init(*desc);
	}

#if 1
// Build bone archetype face lists - list of faces that may be affected
// by each bone. THIS INVOLVES A LOT OF VERY INEFFICIENT SEARCHING, AND
// SHOULD REALLY BE DONE OFFLINE.
	bone = bones;
	for (i = 0; i < num_bones; i++, bone++)
	{
		bone->num_faces = 0;
	}

	bool * already = new bool[num_bones];

	{
		FaceGroup * group = face_groups;
		for (int i = 0; i < face_group_cnt; i++, group++)
		{
			int * fvc_idx = group->face_vertex_chain;
			for (int f = 0; f < group->face_cnt; f++)
			{
				memset(already, 0, sizeof(bool) * num_bones);

				for (int v = 0; v < 3; v++)
				{
					int vtx = vertex_batch_list[*fvc_idx];

					int * id = bone_id_list + vertex_bone_index[vtx];
					for (int j = 0; j < vertex_bone_cnt[vtx]; j++, id++)
					{
					// Add this face to each bone that controls one of its vertices.
						if (!already[*id])
						{
							bones[*id].num_faces++;

							ASSERT((*id) < num_bones);
							already[*id] = true;
						}
					}

					fvc_idx++;
				}
			}
		}									   
	}
	

	bone = bones;
	for (i = 0; i < num_bones; i++, bone++)
	{
		if (bone->num_faces)
		{
			bone->faces = new int[bone->num_faces];
			bone->num_faces = 0;
		}
	}

// NOW actually go fill in the face indices.
	{
		int face_idx = 0;

		FaceGroup * group = face_groups;
		for (int i = 0; i < face_group_cnt; i++, group++)
		{
			int * fvc_idx = group->face_vertex_chain;
			for (int f = 0; f < group->face_cnt; f++, face_idx++)
			{
				memset(already, 0, sizeof(bool) * num_bones);

				for (int v = 0; v < 3; v++)
				{
					int vtx = vertex_batch_list[*fvc_idx];

					int * id = bone_id_list + vertex_bone_index[vtx];
					for (int j = 0; j < vertex_bone_cnt[vtx]; j++, id++)
					{
					// Add this face to each bone that controls one of its vertices.
						if (!already[*id])
						{
							bone = bones + *id;
							bone->faces[bone->num_faces++] = face_idx;

							ASSERT(*id < num_bones);

							already[*id] = true;
						}
					}

					fvc_idx++;
				}
			}
		}
	}

	delete [] already;
	already = NULL;
#endif
}

//

bool DeformablePartArchetype::load_skeleton(IFileSystem * file, BoneDescriptor * bdesc, int & num_bones)
{
	ASSERT(bdesc);

	bool result = false;
	if (file)
	{
		if (file->SetCurrentDirectory("Cmpnd"))
		{
		// Read scale if present.
			if (!ReadChild(&scale, file, "Scale"))
			{
				scale = 1.0;
			}

			num_bones = 1;	// root always exists.

			if (file->SetCurrentDirectory("Root"))
			{
				bdesc[0].read(file);
				file->SetCurrentDirectory("..");
				result = true;
			}

			WIN32_FIND_DATA fd;
			char part_name[32];
			strcpy(part_name, "Part*");


			HANDLE search = file->FindFirstFile(part_name, &fd);
			if (search != INVALID_HANDLE_VALUE)
			{
				bool done = false;
				while (!done)
				{
					if (file->SetCurrentDirectory(fd.cFileName))
					{
						bdesc[num_bones++].read(file);
						file->SetCurrentDirectory("..");
					}

					if (!file->FindNextFile(search, &fd))
					{
						done = true;
					}
				}

				file->FindClose(search);
			}

			file->SetCurrentDirectory("..");
		}

	// Now go through and see if there are any 3DBs in the DFM file that aren't in the CMPND
	// directory.

		int bone_index = 0;

		WIN32_FIND_DATA fd;
		char object_name[] = "*.3db";
		HANDLE search = file->FindFirstFile(object_name, &fd);
		if (search != INVALID_HANDLE_VALUE)
		{
			bool done = false;
			while (!done)
			{
			//
			// search existing bone list...
			//
				bool found = false;
				for (int i = 0; i < num_bones; i++)
				{
					if (strcmp(bdesc[i].file_name, fd.cFileName) == 0)
					{
						found = true;
						break;
					}
				}

				if (!found)
				{
				// we've come across a bone that wasn't loaded in the CMPND chunk.
					BoneDescriptor * bd = &bdesc[num_bones++];

					bd->file_name = new char[strlen(fd.cFileName) + 1];
					strcpy(bd->file_name, fd.cFileName);
					bd->index = bone_index;
					bd->extra = true;
//					bd->object_name = NULL;
				}

				if (!file->FindNextFile(search, &fd))
				{
					done = true;
				}
				bone_index++;
			}

			file->FindClose(search);
		}
	}

	return result;
}

//

#define MAX_HARDPOINTS	64

//

struct HardpointList
{
	int				count;
	HardpointDesc	info[MAX_HARDPOINTS];

	HardpointList(void)
	{
		count = 0;
	}
};

//
// THis is ridiculous.
//
struct HP_object
{
	HardpointList * list;
	INSTANCE_INDEX	object;
};


//

void __cdecl HPCallback(const char * script_name, void * misc)
{
	ASSERT(misc);

	HP_object * obj = (HP_object *) misc;
	HardpointList * list = obj->list;

	if (list->count < MAX_HARDPOINTS)
	{
		list->info[list->count].name = strdup(script_name);
		list->info[list->count++].object = obj->object;
	}
}

//

static void RecursivelyEnumerateHardpoints(INSTANCE_INDEX root, HardpointList & list)
{
	ARCHETYPE_INDEX arch = ENG->get_instance_archetype(root);

	HP_object obj;
	obj.list = &list;
	obj.object = root;

	HARDPOINT->enumerate_hardpoints(HPCallback, arch, &obj);
	ENG->release_archetype(arch);

	INSTANCE_INDEX child = INVALID_INSTANCE_INDEX;
	while( (child = ENG->get_instance_child_next( root, EN_DONT_RECURSE, child )) != INVALID_INSTANCE_INDEX )
	{
		RecursivelyEnumerateHardpoints(child, list);
	}
}

//

void BuildTree(INSTANCE_INDEX);

bool DeformableObject::create(const DeformDesc & desc, Animation::IEventHandler * _callback, void * _user_data, IEngineInstance * user_instance)
{
	bool result = true;

	if(!got_ini_info) Deform::get_ini_info();

	callback = _callback;
	user_data = _user_data;

	num_parts = desc.num_parts;

	parts = new DeformablePart *[num_parts];
	for (int i = 0; i < num_parts; i++)
	{
		DeformablePartArchetype ** part_archs = new DeformablePartArchetype *[desc.parts[i].num_meshes];
		for (int j = 0; j < desc.parts[i].num_meshes; j++)
		{

		// Search archetypes:
			DeformablePartArchetype * parch = archetypes.first();
			while (parch)
			{
				if (strcmp(desc.parts[i].meshes[j].mesh_name, parch->name) == 0)
				{
					break;
				}
				parch = archetypes.next(parch);
			}

			if (parch)
			{
				parch->add_ref();
				part_archs[j] = parch;
			}
			else
			{
				parch = new DeformablePartArchetype(desc.parts[i], j);
				if (parch->is_valid())
				{
					archetypes.link(parch);
					part_archs[j] = parch;
				}
				else
				{
					delete parch;
					parch = NULL;
					result = false;
				}
			}
		}

		if (result)
		{
			parts[i] = new DeformablePart(desc.parts[i].num_meshes, part_archs, user_instance);
		}
		else
		{
			parts[i] = NULL;
		}

		delete [] part_archs;
		part_archs = NULL;
	}

	if (!result)
	{
		destroy();
		return result;
	}

//
// Now hook up everyone via their hardpoints, get root.
//
// Enumerate all hardpoints:
//
	num_hardpoints = 0;
	HardpointList * lists = new HardpointList[num_parts];
	for (i = 0; i < num_parts; i++)
	{
		RecursivelyEnumerateHardpoints(parts[i]->root, lists[i]);

		for (int j = 0; j < parts[i]->meshes[0].arch->num_extras; j++)
		{
			if (parts[i]->extras[j] != INVALID_INSTANCE_INDEX)
			{
				RecursivelyEnumerateHardpoints(parts[i]->extras[j], lists[i]);
			}
		}

		num_hardpoints += lists[i].count;
	}

	if (num_parts > 1)
	{
		for (i = 0; i < num_parts; i++)
		{
			HardpointList * list1 = &lists[i];

			for (int j = i+1; j < num_parts; j++)
			{
				HardpointList * list2 = &lists[j];

				for (int k = 0; k < list1->count; k++)
				{
					HardpointDesc * info1 = list1->info + k;
					for (int l = 0; l < list2->count; l++)
					{
						HardpointDesc * info2 = list2->info + l;
						if (strcmp(info1->name, info2->name) == 0)
						{
						// connect.
							if (HARDPOINT->connect(info1->object, info1->name, info2->object, info2->name) != 0)
							{
								OutputDebugString("DEFOMABLE: Can't connect parts.\n");
							}
						}
					}
				}
			}
		}
	}

// Now create local lists to stay with DeformableObject.
	if (num_hardpoints)
	{
		hardpoints = new HardpointDesc[num_hardpoints];

		HardpointDesc * desc = hardpoints;

		for (i = 0; i < num_parts; i++)
		{
			HardpointList * list = lists + i;
			for (int j = 0; j < list->count; j++)
			{
				*(desc++) = list->info[j];
			}
		}

	}


	delete [] lists;
	lists = NULL;

// 
// Now everything's connected, find root.
//
	INSTANCE_INDEX prev = parts[0]->root;
	INSTANCE_INDEX curr;
	while ((curr = ENG->get_instance_parent(prev)) != INVALID_INSTANCE_INDEX)
	{
		prev = curr;
	}

	root = prev;

	return result;
}

//

void DeformableObject::destroy(void)
{
	if (active)
	{
		ScriptLink * script = active_scripts.first();
		while (script)						  
		{
			script->release();
			script = active_scripts.next(script);
		}

		active_scripts.free();

		if (num_parts)
		{
			for (int i = 0; i < num_parts; i++)
			{
				if (parts[i])
				{
					int num_meshes = parts[i]->num_meshes;
					DeformablePartArchetype * arch[16];
					for (int j = 0; j < num_meshes; j++)
					{
						arch[j] = parts[i]->meshes[j].arch;
					}

					delete parts[i];
					parts[i] = NULL;

					for (j = 0; j < num_meshes; j++)
					{
						if (arch[j]->release() == 0)
						{
							archetypes.free(arch[j]);
						}
					}
				}
			}
			delete [] parts;
			parts = NULL;

			num_parts = 0;
		}

		if (root != INVALID_INSTANCE_INDEX)
		{
			ENG->destroy_instance(root);
			root = INVALID_INSTANCE_INDEX;
		}

		if (hardpoints)
		{
			num_hardpoints = 0;
			delete [] hardpoints;
			hardpoints = NULL;
		}
	}
}

//

void DeformableObject::deform(const int * mesh_index)
{
	const int * idx = mesh_index;
	for (int i = 0; i < num_parts; i++, idx++)
	{
		if (parts[i])
		{
			parts[i]->deform(*idx);
		}
	}
}

//

void DeformableObject::render(struct ICamera * camera, const int * mesh_index)
{
	const int * idx = mesh_index;

	for (int i = 0; i < num_parts; i++, idx++)
	{
		if (parts[i])
		{
			parts[i]->render(camera, *idx);
		}
	}

// Render any child objects. This assumes bones have no appearance.
	ENG->render_instance(camera, root, 0, 1.0f, RF_RELATIVE_LOD, NULL);
}

//

void DeformableObject::get_face_vertex_counts(const int * mesh_index, int ** faces, int ** vertices) const
{
	const int * idx = mesh_index;

	for (int i = 0; i < num_parts; i++, idx++)
	{
		if (parts[i])
		{
			parts[i]->get_face_vertex_counts(*idx, faces[i], vertices[i]);
		}
	}
}

//

void DeformableObject::get_surface_pixel_error(const int * mesh_index, float ** error) const
{
	const int * idx = mesh_index;

	for (int i = 0; i < num_parts; i++, idx++)
	{
		if (parts[i])
		{
			parts[i]->get_surface_pixel_error(*idx, error[i]);
		}
	}
}

//

void DeformableObject::set_surface_pixel_error(const int * mesh_index, const float * error)
{
	const int * idx = mesh_index;

	for (int i = 0; i < num_parts; i++, idx++)
	{
		if (parts[i])
		{
			parts[i]->set_surface_pixel_error(*idx, error[i]);
		}
	}
}

//

void DeformableObject::render_alpha(struct ICamera * camera, const int * mesh_index, U8 alpha)
{
	const int * idx = mesh_index;

	use_constant_alpha = true;
	constant_alpha = alpha;

	for (int i = 0; i < num_parts; i++, idx++)
	{
		if (parts[i])
		{
			parts[i]->render(camera, *idx);
		}
	}

// Render any child objects. This assumes bones have no appearance.
	ENG->render_instance(camera, root, 0, 1.0f, RF_RELATIVE_LOD, NULL);

	use_constant_alpha = false;
}

//

Quaternion DeformableObject::compute_heading_quaternion(float heading)
{
	Vector up;
	switch (up_axis)
	{
	case POS_X:
		up.set(1, 0, 0);
		break;
	case NEG_X:
		up.set(-1, 0, 0);
		break;
	case POS_Y:
		up.set(0, 1, 0);
		break;
	case NEG_Y:
		up.set(0, -1, 0);
		break;
	case POS_Z:
		up.set(0, 0, 1);
		break;
	case NEG_Z:
		up.set(0, 0, -1);
		break;
	}

	return Quaternion(up, heading);
}

//

Quaternion DeformableObject::get_heading_quaternion(const Matrix & R)
{
	Vector heading;
	float h, s;

	switch (heading_axis)
	{
	case POS_X:
		heading = R.get_i();
		h = heading.x;
		break;
	case NEG_X:
		heading = -R.get_i();
		h = heading.x;
		break;
	case POS_Y:
		heading = R.get_j();
		h = heading.y;
		break;
	case NEG_Y:
		heading = -R.get_j();
		h = heading.y;
		break;
	case POS_Z:
		heading = R.get_k();
		h = heading.z;
		break;
	case NEG_Z:
		heading = -R.get_k();
		h = heading.z;
		break;
	}
	
	switch (side_axis)
	{
	case POS_X:
		s = heading.x;
		break;
	case NEG_X:
		s = -heading.x;
		break;
	case POS_Y:
		s = heading.y;
		break;
	case NEG_Y:
		s = -heading.y;
		break;
	case POS_Z:
		s = heading.z;
		break;
	case NEG_Z:
		s = -heading.z;
		break;
	}

	float angle = atan2(s, h);

	Vector u;
	switch (up_axis)
	{
	    case POS_X:
			u.set( 1,  0,  0);
			break;		   
		case NEG_X:
			u.set(-1,  0,  0);
			break;
		case POS_Y:
			u.set( 0,  1,  0);
			break;
		case NEG_Y:
			u.set( 0, -1,  0);
			break;
		case POS_Z:
			u.set( 0,  0,  1);
			break;
		case NEG_Z:
			u.set( 0,  0, -1);
			break;
	}

	Quaternion result(u, angle);
	return result;
}

//

Vector DeformableObject::adjust_height(const Vector & pos, float height)
{
	Vector result = pos;
	switch (up_axis)
	{
	case POS_X:
		result.x = height;
		break;
	case NEG_X:
		result.x = -height;
		break;
	case POS_Y:
		result.y = height;
		break;
	case NEG_Y:
		result.y = -height;
		break;
	case POS_Z:
		result.z = height;
		break;
	case NEG_Z:
		result.z = -height;
		break;
	}

	return result;
}

//

SCRIPT_INST	DeformableObject::start_motion(SCRIPT_SET_ARCH set, const char * script_name, float start_time, float transition_duration, float time_scale, float weight, unsigned int flags, float heading, void *_user_data)
{
	SCRIPT_INST	inst, result	=INVALID_SCRIPT_INST;

	update();

// WILL THIS WORK? Or do we need to give it the correct root?
	if(_user_data)
	{
		inst	=ANIM->create_script_inst(set, root, script_name, this, _user_data);
	}
	else
	{
		inst	=ANIM->create_script_inst(set, root, script_name, this, user_data);
	}

	if(inst != INVALID_SCRIPT_INST)
	{
		Vector pos = ENG->get_position(root);
		Vector hpos;

	// use scale of 0th part. This will work in existing cases but is not sufficiently general.
		float scale = parts[0]->meshes[0].arch->scale;

		float anim_height, script_start_height;
		if (ANIM->get_script_start_height(set, script_name, script_start_height))
		{
			anim_height = scale * script_start_height;
		}
		else
		{
			anim_height = last_height;
		}

		float _start_height = floor_height + anim_height;
		hpos = adjust_height(pos, _start_height);

		Quaternion qh;
		if (heading < 0)
		{
			qh = get_heading_quaternion(ENG->get_orientation(root));
		}
		else
		{
			qh = compute_heading_quaternion(heading);
		}

		Transform T(false);
		T.set_position(hpos);
		T.set_orientation(qh);

		if (0 <= ANIM->script_start(inst, 
							flags,					// direction, loop, etc.
							start_time,				// where to start
							time_scale,				// time scalar.
							transition_duration,	// transition duration
							weight,
							scale,
							&T))
		{
			ScriptLink	*link	=active_scripts.alloc();

			ASSERT(link);

			//link->index = get_script_index(script_name);
			link->instance = inst;

			link->start_height = _start_height;

			result = inst;
		}
		else
		{
			ANIM->release_script_inst (inst);
		}
	}
	
	return result;
}

//

SCRIPT_INST DeformableObject::start_motion_locked(	SCRIPT_SET_ARCH set, const char * script_name, 
													float start_time,
													float transition_duration,
													float time_scale,
													float weight,
													unsigned int flags,
													float heading,
													INSTANCE_INDEX locked_bone)
{
	Vector pb0 = ENG->get_position(locked_bone);

// start new motion.

	SCRIPT_INST result = start_motion(set, script_name, start_time, transition_duration, time_scale, weight, flags, heading);

// build up list of channel data from root to locked bone.
//PLB	const Joint * joints[64];
	const JointInfo * joints[64];
	Quaternion jq[64];
	int i = 0;

	Animation::Target t;
	t.type = Animation::JOINT;

	// Walk the heirarchy from the bone to the root.
	INSTANCE_INDEX bone = locked_bone;
	while (bone != root)
	{
		INSTANCE_INDEX parent = ENG->get_instance_parent(bone);

		const JointInfo * jnt = ENG->get_joint_info(bone);
		assert(jnt->type == JT_SPHERICAL);
	
		// Save the joint info for later.
		joints[i] = jnt;

		t.joint = bone;

		// Get the channel information for this bone.
		// NOTE: All bones are asserted to be spherical with quaternion animation data.
		CHANNEL_INSTANCE_INDEX channel = ANIM->find_channel(result, t);
		IChannel2* channel_arch;
		GENRESULT gr = CHANNEL->get_channel_archetype(channel, channel_arch);
		ASSERT (GR_OK == gr);

		U32 cdt;
		gr = channel_arch->get_data_type(cdt);
		ASSERT (GR_OK == gr);
		assert(cdt == DT_QUATERNION);

		const void* base;
		const void* next;
		SINGLE ratio;

		// Get the two quaternions on either side of the given time, and the interpolation ratio between them.
		gr = channel_arch->get_data_pair (start_time, base, next, ratio);
		ASSERT (GR_OK == gr);

		channel_arch->Release ();

		// Calculate and store the interpolated channel value for the start time.
		jq[i] = Interpolate (*(Quaternion*)base, *(Quaternion*)next, ratio);

		bone = parent;
		i++;
	}

	int num_joints = i;

// get root transform.
	// root animation has its own channel, comprised of a vector and a quaternion
	t.type = Animation::OBJECT;
	t.object = root;

	CHANNEL_INSTANCE_INDEX channel = ANIM->find_channel(result, t);

	IChannel2* channel_arch;
	GENRESULT gr = CHANNEL->get_channel_archetype(channel, channel_arch);
	ASSERT (GR_OK == gr);

	U32 cdt;
	gr = channel_arch->get_data_type(cdt);
	ASSERT (GR_OK == gr);
	assert(cdt == (DT_VECTOR | DT_QUATERNION));

	const void* base;
	const void* next;
	SINGLE ratio;

	// Retrieve the pair for the starting time, then calculate its interpolation value.
	gr = channel_arch->get_data_pair (start_time, base, next, ratio);
	ASSERT (GR_OK == gr);

	Vector v (Interpolate (*(Vector*)base, *(Vector*)next, ratio));
	Quaternion q (Interpolate (*(Quaternion*)((Vector*)base + 1), *(Quaternion*)((Vector*)next + 1), ratio));

	channel_arch->Release ();

	// R == rotation of root in new animation
	// v == translation of root in new animation
	Matrix R(q);

	Vector p(0,0,0), pprev = v;
	Matrix Rprev = R;

// now go back and evaluate kinematic chain based on animation data in order to 
// determine locked bone position.
// the transformations are applied in reverse order, i.e. from root to bone 
	for (i = num_joints-1; i >= 0; i--)
	{
		Matrix Rj(jq[i]);
		const JointInfo * j = joints[i];

		R = Rprev * Rj * j->rel_orientation;
		p = pprev + Rprev * j->parent_point - R * j->child_point;

		pprev = p;
		Rprev = R;
	}

	//pb0 is the current worldspace location of the locked bone
	//p is the locked bone's location in the model space of the transition animation. The current orientation of the
	// deformable root has not been factored into it at this point. In other words, it is an offset from the transition
	// animation's root bone.
	//subtracting the current world root location from the current world bone location yields an offset in world space.
	//transforming the offset in model space into world space (via rotation) yields two offsets in world space.
	//the difference between these offsets is the required adjustment offset.
	Vector offset = (pb0 - ENG->get_position(root)) - ENG->get_orientation(root) * p;

	// Apply the world space offset.
	ANIM->adjust_start_position(result, offset);

	return result;
}

//

void DeformableObject::stop_motion(void)
{
	ScriptLink * link = active_scripts.first();
	while (link)
	{
		ScriptLink * next = active_scripts.next(link);
		link->release();
		active_scripts.free(link);

		link = next;
	}
}

//

void DeformableObject::stop_single_motion(const SCRIPT_INST inst)
{
	ScriptLink	*link	=active_scripts.first();

	while(link)
	{
		ScriptLink	*next	=active_scripts.next(link);

		if(inst == link->instance)
		{
			link->release();
			active_scripts.free(link);

			break;
		}
		link	=next;
	}
}

//

void DeformableObject::update(void)
{
	ScriptLink * link = active_scripts.first();
	while (link)
	{
		ScriptLink * next = active_scripts.next(link);

		if (link->ik_script)
		{
			if (link->ik->done)
			{
				if (!CHANNEL->channel_in_use(link->ik->channels[0]))
				{
					link->release();
					active_scripts.free(link);
				}
			}
		}
		else
		{
			if (ANIM->script_is_done(link->instance))
			{
				ANIM->release_script_inst(link->instance);
				active_scripts.free(link);
			}
		}

		link = next;
	}

	if (active_scripts.count() == 0)
	{
		Vector pos = ENG->get_position(root);
		
		switch (up_axis)
		{
			case POS_X:
				last_height = pos.x - floor_height;
				break;
			case NEG_X:
				last_height = floor_height - pos.x;
				break;
			case POS_Y:
				last_height = pos.y - floor_height;
				break;
			case NEG_Y:
				last_height = floor_height - pos.y;
				break;
			case POS_Z:
				last_height = pos.z - floor_height;
				break;
			case NEG_Z:
				last_height = floor_height - pos.z;
				break;
		}
	}
}

//

void DeformableObject::set_position(const Vector & pos)
{
	update();

	Vector new_pos = pos;

	bool changed = false;

	ScriptLink * link = active_scripts.first();
	if (link)
	{
		while (link)
		{
			if (!link->ik_script)
			{
				float scale = parts[0]->meshes[0].arch->scale;

				Vector abs(0, 0, 0), offset(0, 0, 0);
				float dheight, current_height;
				float anim_height, script_start_height;

				if (ANIM->get_script_start_height(link->instance, script_start_height))
				{
					ANIM->get_root_data(link->instance, abs, offset);
					switch (up_axis)
					{
						case POS_X:
							current_height = abs.x;
							dheight = +offset.x;
							break;
						case NEG_X:
							current_height = -abs.x;
							dheight = -offset.x;
							break;
						case POS_Y:
							current_height = abs.y;
							dheight = +offset.y;
							break;
						case NEG_Y:
							current_height = -abs.y;
							dheight = -offset.y;
							break;
						case POS_Z:
							current_height = abs.z;
							dheight = +offset.z;
							break;
						case NEG_Z:
							current_height = -abs.z;
							dheight = -offset.z;
							break;
					}

					anim_height = scale * script_start_height;

				}
				else
				{
					anim_height = last_height;

					dheight = 0;

					Vector actual_pos = ENG->get_position(root);

					switch (up_axis)
					{
						case POS_X:
							current_height = actual_pos.x;
							break;
						case NEG_X:
							current_height = -actual_pos.x;
							break;
						case POS_Y:
							current_height = actual_pos.y;
							break;
						case NEG_Y:
							current_height = -actual_pos.y;
							break;
						case POS_Z:
							current_height = actual_pos.z;
							break;
						case NEG_Z:
							current_height = -actual_pos.z;
							break;
					}
				}

				float desired_rel_height = anim_height + dheight;
				float desired_height = floor_height + desired_rel_height;

				//float dc = desired_height - current_height;

				Vector adjusted = pos;
				switch (up_axis)
				{
					case POS_X:
						adjusted.x = desired_height;
						break;
					case NEG_X:
						adjusted.x = -desired_height;
						break;
					case POS_Y:
						adjusted.y = desired_height;
						break;
					case NEG_Y:
						adjusted.y = -desired_height;
						break;
					case POS_Z:
						adjusted.z = desired_height;
						break;
					case NEG_Z:
						adjusted.z = -desired_height;
						break;
				}

				new_pos = adjusted;
				ANIM->change_script_position(link->instance, adjusted);

				changed = true;
			}

			link = active_scripts.next(link);
		}
	}

	if (!changed)
	{
		switch (up_axis)
		{
			case POS_X:
				new_pos.x = floor_height + last_height;
				break;
			case NEG_X:
				new_pos.x = floor_height - last_height;
				break;
			case POS_Y:
				new_pos.y = floor_height + last_height;
				break;
			case NEG_Y:
				new_pos.y = floor_height - last_height;
				break;
			case POS_Z:
				new_pos.z = floor_height + last_height;
				break;
			case NEG_Z:
				new_pos.z = floor_height - last_height;
				break;
		}
	}

	ENG->set_position(root, new_pos);
}

//

void DeformableObject::set_orientation(const Matrix & R)
{
	ScriptLink * link = active_scripts.first();
	if (link)
	{
		while (link)
		{
			if (!link->ik_script)
			{
				ANIM->change_script_orientation(link->instance, R);
			}
			link = active_scripts.next(link);
		}
	}

	ENG->set_orientation(root, R);
}

//

void DeformableObject::compute_side_axis(void)
{
	Vector h, u, s;

// heading X side = up.
	switch (heading_axis)
	{
		case POS_X:
			h.set( 1,  0,  0);
			break;		   
		case NEG_X:
			h.set(-1,  0,  0);
			break;
		case POS_Y:
			h.set( 0,  1,  0);
			break;
		case NEG_Y:
			h.set( 0, -1,  0);
			break;
		case POS_Z:
			h.set( 0,  0,  1);
			break;
		case NEG_Z:
			h.set( 0,  0, -1);
			break;
	}

	switch (up_axis)
	{
	    case POS_X:
			u.set( 1,  0,  0);
			break;		   
		case NEG_X:
			u.set(-1,  0,  0);
			break;
		case POS_Y:
			u.set( 0,  1,  0);
			break;
		case NEG_Y:
			u.set( 0, -1,  0);
			break;
		case POS_Z:
			u.set( 0,  0,  1);
			break;
		case NEG_Z:
			u.set( 0,  0, -1);
			break;
	}

	s = cross_product(u, h);
	if (s.x != 0)
	{
		side_axis = (s.x < 0) ? NEG_X : POS_X;
	}
	else if (s.y != 0)
	{
		side_axis = (s.y < 0) ? NEG_Y : POS_Y;
	}
	else if (s.z != 0)
	{
		side_axis = (s.z < 0) ? NEG_Z : POS_Z;
	}
}

//

void DeformableObject::set_heading_axis(Axis ax)
{
	heading_axis = ax;
	compute_side_axis();
}

//

void DeformableObject::set_up_axis(Axis ax)
{
	up_axis = ax;

	compute_side_axis();
}

//

void DeformableObject::set_floor_height(float height)
{
	float prev_floor_height = floor_height;
	floor_height = height;

	update();

	ScriptLink * link = active_scripts.first();
	if (link)
	{
		while (link)
		{
			if (!link->ik_script)
			{
				float scale = parts[0]->meshes[0].arch->scale;

				Vector abs(0, 0, 0), offset(0, 0, 0);
				float dheight, current_height;
				float anim_height, script_start_height;

				if (ANIM->get_script_start_height(link->instance, script_start_height))
				{
					ANIM->get_root_data(link->instance, abs, offset);
					switch (up_axis)
					{
						case POS_X:
							current_height = abs.x;
							dheight = +offset.x;
							break;
						case NEG_X:
							current_height = -abs.x;
							dheight = -offset.x;
							break;
						case POS_Y:
							current_height = abs.y;
							dheight = +offset.y;
							break;
						case NEG_Y:
							current_height = -abs.y;
							dheight = -offset.y;
							break;
						case POS_Z:
							current_height = abs.z;
							dheight = +offset.z;
							break;
						case NEG_Z:
							current_height = -abs.z;
							dheight = -offset.z;
							break;
					}

					anim_height = scale * script_start_height;
				}
				else
				{
					anim_height = last_height;
					dheight = 0;

					dheight = 0;

					Vector pos = ENG->get_position(root);

					switch (up_axis)
					{
						case POS_X:
							current_height = pos.x;
							break;
						case NEG_X:
							current_height = -pos.x;
							break;
						case POS_Y:
							current_height = pos.y;
							break;
						case NEG_Y:
							current_height = -pos.y;
							break;
						case POS_Z:
							current_height = pos.z;
							break;
						case NEG_Z:
							current_height = -pos.z;
							break;
					}
				}

				float desired_rel_height = anim_height + dheight;
				float desired_height = floor_height + desired_rel_height;

				float dc = desired_height - current_height;

				Vector adjusted(0, 0, 0);
				switch (up_axis)
				{
					case POS_X:
						adjusted.x = dc;
						break;
					case NEG_X:
						adjusted.x = -dc;
						break;
					case POS_Y:
						adjusted.y = dc;
						break;
					case NEG_Y:
						adjusted.y = -dc;
						break;
					case POS_Z:
						adjusted.z = dc;
						break;
					case NEG_Z:
						adjusted.z = -dc;
						break;
				}

				if (fabs(dc) > FLT_EPSILON)
				{
					ANIM->adjust_start_position(link->instance, adjusted);
				}
			}

			link = active_scripts.next(link);
		}
	}
}

//

bool DeformableObject::visible_rect(RECT & rect, struct ICamera * camera)
{
	bool result = false;

	rect.left = rect.top = LONG_MAX;
	rect.right = rect.bottom = LONG_MIN;

	for (int i = 0; i < num_parts; i++)
	{
		if (parts[i])// && parts[i]->arch->face_cnt)    // WHAT'S THIS ABOUT?
		{
			RECT part_rect;
			if (parts[i]->visible_rect(part_rect, camera))
			{
				if (part_rect.left < rect.left)
				{
					rect.left = part_rect.left;
				}
				if (part_rect.top < rect.top)
				{
					rect.top = part_rect.top;
				}
				if (part_rect.right > rect.right)
				{
					rect.right = part_rect.right;
				}
				if (part_rect.bottom > rect.bottom)
				{
					rect.bottom = part_rect.bottom;
				}

				result = true;
			}
		}
	}

	return result;
}

//

DeformablePartArchetype::DeformablePartArchetype(const DeformPartDesc & pdesc, int mesh_number)
{
	memset(this, 0, sizeof(*this));
								
	arch_index = INVALID_ARCHETYPE_INDEX;
	for (int i = 0; i < DPA_MAX_EXTRAS; i++)
	{
		extras[i] = INVALID_ARCHETYPE_INDEX;
	}

	valid = false;
	ref_cnt = 1;

	name = strdup(pdesc.meshes[mesh_number].mesh_name);
	strlwr(name);

	IFileSystem * file = CreateFileSystem(pdesc.meshes[mesh_number].mesh_parent, pdesc.meshes[mesh_number].mesh_name);
	if (file)
	{
		if (file->SetCurrentDirectory("Skeleton"))
		{
			char skeleton_name[_MAX_PATH];
			if (ReadChild(skeleton_name, file, "name"))
			{
				IFileSystem * sk_file;
				
				sk_file = file;
				sk_file->SetCurrentDirectory("\\");

				if (sk_file)
				{
					static unsigned int skeleton_counter = 0;

				// MAX BONES = 256
					BoneDescriptor desc[256];
					if (load_skeleton(sk_file, desc, num_bones))
					{
						char number[8];
						itoa(num_bones, number, 10);
						char * dot = strchr(skeleton_name, '.');
						if (dot)
						{
							*dot = 0;
							strcat(skeleton_name, number);
						}

						itoa(skeleton_counter++, number, 10);
						strcat(skeleton_name, number);

						file->SetCurrentDirectory("..");
						IComponentFactory * saved_search_path;
						
						ENG->get_search_path2(&saved_search_path);
						ENG->set_search_path2(file);
						
						arch_index = ENG->create_archetype(skeleton_name, sk_file);

						for (int i = 0; i < num_bones; i++)
						{
							if (desc[i].extra)
							{
								if (sk_file->SetCurrentDirectory(desc[i].file_name))
								{
									extras[num_extras++] = ENG->create_archetype(desc[i].file_name, sk_file);
									sk_file->SetCurrentDirectory("..");
								}
							}
						}

						ENG->set_search_path2(saved_search_path);
						saved_search_path->Release();
						
						bool lm = load_mesh(file);
						if( !lm )
						{
							char temp[128];
							sprintf(temp, "DEFOMABLE: Could NOT load mesh %s\n", pdesc.meshes[mesh_number].mesh_name);
							GENERAL_WARNING(temp);
						}
    					
						setup(sk_file, skeleton_name, desc, num_bones);
						valid = true;
					}
					else
					{
						char temp[128];
						sprintf(temp, "DEFOMABLE: Error loading skeleton from file %s\n", skeleton_name);
						GENERAL_ERROR(temp);
					}
				}
				else
				{
					char temp[128];
					sprintf(temp, "DEFOMABLE: Unable to open skeleton file %s\n", skeleton_name);
					GENERAL_ERROR(temp);
				}
			}
			else
			{
				char temp[128];
				sprintf(temp, "DEFOMABLE: Error reading file %s\n", pdesc.meshes[mesh_number].mesh_name);
				GENERAL_ERROR(temp);
			}
		}
		else
		{
			char temp[128];
			sprintf(temp, "DEFOMABLE: Unable to find Skeleton chunk in %s\n", pdesc.meshes[mesh_number].mesh_name);
			GENERAL_ERROR(temp);
		}

		file->Release();
	}
	else
	{
		char temp[128];
		sprintf(temp, "DEFOMABLE: Unable to open file %s\n", pdesc.meshes[mesh_number].mesh_name);
		GENERAL_ERROR(temp);
	}
}

//

DeformablePartArchetype::~DeformablePartArchetype(void)
{
	free(name);
	name = NULL;

//	ANIM->release_script_set_arch(script_set);

	ENG->release_archetype(arch_index);
	arch_index = INVALID_ARCHETYPE_INDEX;

	for (int i = 0; i < num_extras; i++)
	{
		if (extras[i] != INVALID_ARCHETYPE_INDEX)
		{
			ENG->release_archetype(extras[i]);
			extras[i] = INVALID_ARCHETYPE_INDEX;
		}
	}

	delete [] bones;
	bones = NULL;


	delete [] face_groups;
	face_groups = NULL;

	delete [] face_group_lookup;
	face_group_lookup = NULL;

	delete [] face_group_index_lookup;
	face_group_index_lookup = NULL;

	delete [] vertex_batch_list;
	vertex_batch_list = NULL;

	delete [] texture_batch_list;
	texture_batch_list = NULL;

	delete [] texture_batch_list2;
	texture_batch_list2 = NULL;
	

	delete [] material_list; // material destructor releases textures
	material_list = NULL;

	
	delete [] vertex_bone_cnt;
	vertex_bone_cnt = NULL;

	delete [] vertex_bone_index;
	vertex_bone_index = NULL;
	
	delete [] bone_id_list;
	bone_id_list = NULL;

	delete [] bone_weight_list;
	bone_weight_list = NULL;

	delete [] bone_vertex_list;
	bone_vertex_list = NULL;

	delete [] texture_vertex_list;
	texture_vertex_list = NULL;
	
	delete [] bone_normal_list;
	bone_normal_list = NULL;

	
	delete [] uv_bone_id;
	delete [] uv_vertex_count;
	delete [] uv_plane_distance;
	delete [] x_to_u_scale;
	delete [] y_to_v_scale;
	delete [] min_du;
	delete [] max_du;
	delete [] min_dv;
	delete [] max_dv;
	delete [] uv_vertex_id;
	delete [] uv_default_list;	

	// patch stuff
	delete [] patch_groups;
	patch_groups = NULL;
	delete [] edges;
	edges = NULL;
}

//

void DeformablePartArchetype::add_ref(void)
{
	ref_cnt++;
}

//

U32 DeformablePartArchetype::release(void)
{
	return (--ref_cnt);
}

//

void DeformablePart::mtl_render_indexed_primitive_list(Material *mat, int uv_ch_num,
												   U32 rwm_flags)
{
	// NOTES
	//
	// o Try to always put D3DTA_TEXTURE in ARG1 as some hardware
	//   cannot handle _TEXTURE in ARG2 for *some* operations.  (TNT)
	// 
	// o Try to put _CURRENT in ARG2 for similar reasons (ATI?)
	//

	U32 mat_uid = (((U32)mat)<<16) | (mat->unique<<8);


	// Set common state for all passes
	//
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP,		D3DTOP_SELECTARG2 );
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG1,	D3DTA_TEXTURE );
	BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAARG2,	D3DTA_DIFFUSE );
	BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP,		D3DTOP_SELECTARG2 );
	BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG1,	D3DTA_TEXTURE );
	BATCH->set_texture_stage_state( 0, D3DTSS_COLORARG2,	D3DTA_DIFFUSE );
	BATCH->set_sampler_state( 0, D3DSAMP_MINFILTER,	D3DTEXF_LINEAR );
	BATCH->set_sampler_state( 0, D3DSAMP_MAGFILTER,	D3DTEXF_LINEAR );
	BATCH->set_sampler_state( 0, D3DSAMP_MIPFILTER,	D3DTEXF_POINT );

	if(GET_TC_WRAP_MODE(mat->texture_flags) == TC_WRAP_UV_1 && uv_ch_num >= 2)
	{
		BATCH->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	1 );
	}
	else
	{
		BATCH->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	0 );
	}

	BATCH->set_render_state( D3DRS_ZENABLE,		TRUE );
	BATCH->set_render_state( D3DRS_ZWRITEENABLE,	TRUE );


	// Transparency
	//
	U32 blend_needed = 0;
	U32 stage_cnt = 0;
	U32 stage_zero_flags = 0;

	if( use_constant_alpha || mat->transparency < 255 ) {
		blend_needed++;
	}

#if 1
	// Diffuse1
	//
	if( !(mat->flags & MF_NO_DIFFUSE1_PASS) )
	{

		if( !(rwm_flags & RWM_DONT_TEXTURE) ) {
			BATCH->set_texture_stage_texture( stage_cnt, mat->texture_id );
		}
		else {
			BATCH->set_texture_stage_texture( stage_cnt, 0 );
		}

		if( mat->texture_id ) {
			
			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_COLOROP,	D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAOP,	D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_ADDRESSU,	MAT_GET_ADDR_MODE(mat->texture_flags,0) );
			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_ADDRESSV,	MAT_GET_ADDR_MODE(mat->texture_flags,1) );
				
			if( mat->texture_id && (mat->texture_flags & TF_F_HAS_ALPHA) ) {
				blend_needed++;
			}

			if( !stage_cnt ) {
				stage_zero_flags = MF_NO_DIFFUSE1_PASS;
			}
			stage_cnt++;
		}
	}
#endif

#if 1
	// Diffuse2
	//
	// Don't setup multitexture if we already know or are told to do
	// multpass.
	//
	// Don't setup multitexture if we are not to do any diffuse2 things.
	//
	if( !(stage_cnt && (mat->num_passes > 1)) && !(mat->flags & MF_NO_DIFFUSE2_PASS) ) {

		if( mat->second_diffuse_texture_id || (!mat->texture_id && !mat->emissive_texture_id) ) {
		
			if( !(rwm_flags & RWM_DONT_TEXTURE) ) {
				BATCH->set_texture_stage_texture( stage_cnt, mat->second_diffuse_texture_id );
			}
			else {
				BATCH->set_texture_stage_texture( stage_cnt, 0 );
			}
			
			if(GET_TC_WRAP_MODE(mat->second_diffuse_texture_flags) == TC_WRAP_UV_1 && uv_ch_num >= 2)
			{
				BATCH->set_texture_stage_state( stage_cnt, D3DTSS_TEXCOORDINDEX, 1 );
			}
			else
			{
				BATCH->set_texture_stage_state( stage_cnt, D3DTSS_TEXCOORDINDEX, 0 );
			}

			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_COLOROP,	D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAOP,	D3DTOP_MODULATE );
			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_ADDRESSU,	MAT_GET_ADDR_MODE(mat->second_diffuse_texture_flags,0) );
			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_ADDRESSV,	MAT_GET_ADDR_MODE(mat->second_diffuse_texture_flags,1) );

			if( stage_cnt ) {
				BATCH->set_texture_stage_state( stage_cnt, D3DTSS_COLORARG1,	D3DTA_TEXTURE );
				BATCH->set_texture_stage_state( stage_cnt, D3DTSS_COLORARG2,	D3DTA_CURRENT );
				BATCH->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAARG1,	D3DTA_TEXTURE );
				BATCH->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAARG2,	D3DTA_CURRENT );
				BATCH->set_texture_stage_state( stage_cnt, D3DSAMP_MINFILTER,	D3DTEXF_LINEAR );
				BATCH->set_texture_stage_state( stage_cnt, D3DSAMP_MAGFILTER,	D3DTEXF_LINEAR );
				BATCH->set_texture_stage_state( stage_cnt, D3DSAMP_MIPFILTER,	D3DTEXF_POINT );
			}

			if( mat->second_diffuse_texture_id && (mat->second_diffuse_texture_flags & TF_F_HAS_ALPHA) ) {
				blend_needed++;
			}
			
			if( !stage_cnt ) {
				stage_zero_flags = MF_NO_DIFFUSE2_PASS;
			}
			stage_cnt++;
		}
	}
#endif

	// Emissive
	//
	if( !(stage_cnt && (mat->num_passes > 1)) && !(mat->flags & MF_NO_EMITTER_PASS) && mat->emissive_texture_id ) {
		
		// emissive_blend

		if( !(rwm_flags & RWM_DONT_TEXTURE) ) {
			BATCH->set_texture_stage_texture( stage_cnt, mat->emissive_texture_id );
		}
		else {
			BATCH->set_texture_stage_texture( stage_cnt, 0 );
		}

		if(GET_TC_WRAP_MODE(mat->emissive_texture_flags) == TC_WRAP_UV_1 && uv_ch_num >= 2)
		{
			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_TEXCOORDINDEX,	1 );
		}
		else
		{
			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_TEXCOORDINDEX,	0 );
		}

		BATCH->set_texture_stage_state( stage_cnt, D3DTSS_COLOROP,	D3DTOP_ADD );
		BATCH->set_texture_stage_state( stage_cnt, D3DTSS_ADDRESSU,	MAT_GET_ADDR_MODE(mat->emissive_texture_flags,0) );
		BATCH->set_texture_stage_state( stage_cnt, D3DTSS_ADDRESSV,	MAT_GET_ADDR_MODE(mat->emissive_texture_flags,1) );
		
		if( stage_cnt ) {
			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_COLORARG1,	D3DTA_TEXTURE );
			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_COLORARG2,	D3DTA_CURRENT );
			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAARG1,	D3DTA_TEXTURE );
			BATCH->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAARG2,	D3DTA_CURRENT );
			BATCH->set_texture_stage_state( stage_cnt, D3DSAMP_MINFILTER,	D3DTEXF_LINEAR );
			BATCH->set_texture_stage_state( stage_cnt, D3DSAMP_MAGFILTER,	D3DTEXF_LINEAR );
			BATCH->set_texture_stage_state( stage_cnt, D3DSAMP_MIPFILTER,	D3DTEXF_POINT );
		}
		
		if( mat->emissive_texture_id && (mat->emissive_texture_flags & TF_F_HAS_ALPHA) ) {
			blend_needed++;
		}
		
		if( !stage_cnt ) {
			stage_zero_flags = MF_NO_EMITTER_PASS;
		}
		stage_cnt++;
	}

	// Set up blending
	//
	BATCH->set_render_state( D3DRS_ALPHABLENDENABLE, (blend_needed > 0) );
	if( blend_needed ) {
		BATCH->set_render_state( D3DRS_SRCBLEND, mat->src_blend );
		BATCH->set_render_state( D3DRS_DESTBLEND, mat->dst_blend );
	}

	BATCH->set_texture_stage_state( stage_cnt, D3DTSS_COLOROP, D3DTOP_DISABLE );
	BATCH->set_texture_stage_state( stage_cnt, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

	if( mat->num_passes < 0 ) {
		if( SUCCEEDED( PIPE->verify_state( ) ) ) {
			mat->num_passes = 1;
		}
		else {
			mat->num_passes = stage_cnt;
		}
	}
	
	U32 enable_specular = mat->shininess && (mat->shininess_width > 0.0) && !(mat->flags & MF_NO_SPECULAR_PASS);

	if( enable_specular && specular_mode == 1 ) {
		BATCH->set_render_state( D3DRS_SPECULARENABLE, TRUE );
	}
	else {
		BATCH->set_render_state( D3DRS_SPECULARENABLE, FALSE );
	}
	
	BATCH->set_state( RPR_STATE_ID, mat_uid++ );

	U32 clip = 0;

	if( mat->num_passes > 1 ) {
	
		// This texture count/configuration can't be done in one pass
		// do multiple passes.

		// Stage0 will already be setup above
		BATCH->draw_indexed_primitive( D3DPT_TRIANGLELIST, MTVERTEX_FVFFLAGS, vertex_pool,
			vertex_pool_index, index_list, index_list_index, clip ) ;

		if( enable_specular && specular_mode == 1 ) {
			BATCH->set_render_state( D3DRS_SPECULARENABLE, FALSE );
		}

		// set up framebuffer blending for next N passes
		//
		BATCH->set_render_state( D3DRS_ALPHABLENDENABLE, TRUE ) ;
		BATCH->set_render_state( D3DRS_ZENABLE, TRUE ) ;
		BATCH->set_render_state( D3DRS_ZWRITEENABLE, FALSE ) ;
		BATCH->set_render_state( D3DRS_ZFUNC, D3DCMP_LESSEQUAL ) ;


		// Diffuse2
		//
		if( (stage_zero_flags != MF_NO_DIFFUSE2_PASS) && !(mat->flags & MF_NO_DIFFUSE2_PASS) &&
			mat->second_diffuse_texture_id )
		{

			BATCH->set_render_state( D3DRS_SRCBLEND, diffuse2_fallback_blend[0] );
			BATCH->set_render_state( D3DRS_DESTBLEND, diffuse2_fallback_blend[1] );
			
			if(GET_TC_WRAP_MODE(mat->second_diffuse_texture_flags) == TC_WRAP_UV_1 && uv_ch_num >= 2)
			{
				BATCH->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 1 );
			}
			else
			{
				BATCH->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	0 );
			}

			if( !(rwm_flags & RWM_DONT_TEXTURE) ) {
				BATCH->set_texture_stage_texture( 0, mat->second_diffuse_texture_id );
			}
			else {
				BATCH->set_texture_stage_texture( 0, 0 );
			}

			BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

			BATCH->set_texture_stage_state( 0, D3DTSS_ADDRESSU,	MAT_GET_ADDR_MODE(mat->second_diffuse_texture_flags,0) );
			BATCH->set_texture_stage_state( 0, D3DTSS_ADDRESSV,	MAT_GET_ADDR_MODE(mat->second_diffuse_texture_flags,1) );
			
			BATCH->set_state( RPR_STATE_ID, mat_uid++ );
		
			BATCH->draw_indexed_primitive( D3DPT_TRIANGLELIST, MTVERTEX_FVFFLAGS, vertex_pool,
				vertex_pool_index, index_list, index_list_index, clip ) ;
		}

		// Emissive
		//
		if( (stage_zero_flags != MF_NO_EMITTER_PASS) && !(mat->flags & MF_NO_EMITTER_PASS) && mat->emissive_texture_id ) {
			
			BATCH->set_render_state( D3DRS_SRCBLEND, emissive_fallback_blend[0] );
			BATCH->set_render_state( D3DRS_DESTBLEND, emissive_fallback_blend[1] );
			
			if(GET_TC_WRAP_MODE(mat->emissive_texture_flags) == TC_WRAP_UV_1 && uv_ch_num >= 2)
			{
				BATCH->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 1 );
			}
			else
			{
				BATCH->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0 );
			}

			if( !(rwm_flags & RWM_DONT_TEXTURE) ) {
				BATCH->set_texture_stage_texture( 0, mat->emissive_texture_id );
			}
			else {
				BATCH->set_texture_stage_texture( 0, 0 );
			}
			
			BATCH->set_texture_stage_state( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
			BATCH->set_texture_stage_state( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );

			BATCH->set_texture_stage_state( 0, D3DTSS_ADDRESSU, MAT_GET_ADDR_MODE(mat->emissive_texture_flags,0) );
			BATCH->set_texture_stage_state( 0, D3DTSS_ADDRESSV, MAT_GET_ADDR_MODE(mat->emissive_texture_flags,1) );
			
			BATCH->set_state( RPR_STATE_ID, mat_uid++ );

			BATCH->draw_indexed_primitive( D3DPT_TRIANGLELIST, MTVERTEX_FVFFLAGS, vertex_pool,
				vertex_pool_index, index_list, index_list_index, clip ) ;
		}

		BATCH->set_render_state( D3DRS_ZFUNC, D3DCMP_LESS ) ;
	}
	else if( mat->num_passes >= 0 ) {

		BATCH->draw_indexed_primitive( D3DPT_TRIANGLELIST, MTVERTEX_FVFFLAGS, vertex_pool,
			vertex_pool_index, index_list, index_list_index, clip ) ;
	}

	BATCH->set_state( RPR_STATE_ID, 0 );
}

void DeformablePart::render_mesh(const struct ICamera * camera, DeformablePartMesh * mesh)
{
	U32 rwm_flags;
	PIPE->get_pipeline_state( RP_TEXTURE, &rwm_flags );
	if( !rwm_flags )
	{
		rwm_flags = RWM_DONT_TEXTURE;
	}
	else
	{
		rwm_flags = 0;
	}

	DeformablePartArchetype *arch = mesh->arch;

	if( arch->object_vertex_cnt > rgb_count )
	{
		delete [] rgb;
		rgb_count = arch->object_vertex_cnt;
		rgb = new LightRGB[rgb_count];
	}

	if( arch->face_cnt > double_count )
	{
		delete [] DoubleSided;
		double_count = arch->face_cnt;
		DoubleSided = new int[double_count];
	}

	if( LIGHT )
	{
		LIGHT->light_vertices(rgb, transformed_vertices, transformed_normals, arch->object_vertex_cnt);
	}
	else
	{
		for(int i = 0; i < arch->object_vertex_cnt; i++)
		{
			rgb[i].r =
			rgb[i].g =
			rgb[i].b = 255;
		}
	}

	verify_lists( Tmax(3 * arch->face_cnt, arch->vertex_batch_cnt));
	verify_pools(3 * arch->face_cnt);

	Transform world_to_view ( ((ICamera*)camera)->get_inverse_transform() );

	BoneInstance *bone = &(mesh->bones[0]);
	Vector pos_in_camera ( world_to_view * ENG->get_position( ENG->get_instance_root(bone->instance) ) );

	// note that object is in world space
	const Vector cam_pos_in_object( camera->get_position() );

	for(int i = 0; i < arch->face_group_cnt; i++)
	{
		FaceGroup *group = arch->face_groups + i;
		Material *mat = arch->material_list + group->material;

		memset(vertex_slot, 0xff, sizeof(short) * arch->vertex_batch_cnt);

		U8 transparency = (use_constant_alpha) ? constant_alpha : mat->transparency;
	//
	// AMBIENT is already accounted for in the light_pool values.
	// We're basically rolling ambient and diffuse together here.
	//
		unsigned int base_r = 0, base_g = 0, base_b = 0;

		if (mat->flags & MF_EMITTER)
		{
			base_r += mat->emission.r;
			base_g += mat->emission.g;
			base_b += mat->emission.b;
		}

		
		U16 *idx_ptr = index_list;
		unsigned int r, g, b;
		int max_c;

		int num_double_sided = 0;
		vertex_pool_index = 0;
	
		for (int f = 0; f < group->face_cnt; f++)
		{
			FACE_PROPERTY * fp = group->face_properties + f;

			if ( !(*fp & HIDDEN) )
			{
				if(*fp & TWO_SIDED)
				{
					DoubleSided[num_double_sided++] = f;
				}
				else
				{
					const int * chain = group->face_vertex_chain + f * 3;
					for (int v = 0; v < 3; v++, chain++)
					{
						ASSERT(*chain < index_list_len);
						if (vertex_slot[*chain] == 0xFFFF) // Must copy vertex to list.
						{
							MTVERTEX & current_mt = vertex_pool[vertex_pool_index];
							vertex_slot[*chain] = vertex_pool_index; //pb.current_vertex;

							const int vchain = arch->vertex_batch_list[*chain];
							const int tchain = arch->texture_batch_list[*chain];
							const int tchain2 = (arch->texture_batch_list2) ? arch->texture_batch_list2[*chain] : -1;

							r = base_r;
							g = base_g;
							b = base_b;

							{
							//
							// Smooth-shaded. Use vertex light values computed above.
							//
								LightRGB * l = rgb + vchain;
								
								if (mat->flags & MF_DIFFUSE)
								{
									r += (mat->diffuse.r * l->r) >> 8;
									g += (mat->diffuse.g * l->g) >> 8;
									b += (mat->diffuse.b * l->b) >> 8;
								}
							}

							if(255 < (max_c = Tmax(Tmax(r, g), b)))
							{
								r = (r * 255) / max_c;
								g = (g * 255) / max_c;
								b = (b * 255) / max_c;
							}

							current_mt.color =
							current_mt.scolor = ARGB_MAKE(r, g, b, 0);
							current_mt.a = transparency;

							current_mt.u = arch->texture_vertex_list[tchain].u;
							current_mt.v = arch->texture_vertex_list[tchain].v;

							if (tchain2 == -1)
							{
								current_mt.u2 = current_mt.u;
								current_mt.v2 = current_mt.v;
							}
							else
							{
								current_mt.u2 = arch->texture_vertex_list[tchain2].u;
								current_mt.v2 = arch->texture_vertex_list[tchain2].v;
							}

							current_mt.pos.x = transformed_vertices[vchain].x;
							current_mt.pos.y = transformed_vertices[vchain].y;
							current_mt.pos.z = transformed_vertices[vchain].z;
						
							vertex_pool_index++;
							ASSERT(vertex_pool_index <= vertex_pool_len);  
						}

						// Add index to list:
						*idx_ptr++ = vertex_slot[*chain];

						ASSERT_FATAL(idx_ptr <= (index_list + index_list_len) || !idx_ptr);
					}
				}
			}
		}

		BATCH->set_render_state( D3DRS_CULLMODE, D3DCULL_CW ) ;
		index_list_index = idx_ptr - index_list;
		mtl_render_indexed_primitive_list(mat, (arch->texture_batch_list2) ? 2 : 1,
			rwm_flags);


		if(num_double_sided > 0)
		{
			Vector cam_pos(camera->get_position());
			LightRGB vrgb[3];

			idx_ptr = index_list;
			memset(vertex_slot, 0xff, sizeof(short) * arch->vertex_batch_cnt);
			vertex_pool_index = 0;
			for (int fd = 0; fd < num_double_sided; fd++)
			{
				const int * chain = group->face_vertex_chain + DoubleSided[fd] * 3;

				const int v0 = arch->vertex_batch_list[*(chain+0)];
				const int v1 = arch->vertex_batch_list[*(chain+1)];
				const int v2 = arch->vertex_batch_list[*(chain+2)];

			//
			// see which side of face camera is on. assumes clockwise faces.
			//
				const Vector & vtx0 = transformed_vertices[v0];
				const Vector & vtx1 = transformed_vertices[v1];
				const Vector & vtx2 = transformed_vertices[v2];

				Vector e0 = vtx1 - vtx0;
				Vector e1 = vtx2 - vtx0;
				Vector Nf = cross_product(e0, e1);
				Nf.normalize();
				float df = -dot_product(vtx0, Nf);

				float dC = dot_product(cam_pos, Nf) + df;
				bool flipped;
				if (dC < 0.0f)	// wrong side
				{
					flipped = true;
					Vector vtx[3] = {vtx0, vtx1, vtx2};
					Vector Nv[3] = {-transformed_normals[v0],
									-transformed_normals[v1],
									-transformed_normals[v2]};

					if( LIGHT )
					{
						LIGHT->light_vertices(vrgb, vtx, Nv, 3);
					}
					else
					{
						vrgb[0].r = vrgb[0].g = vrgb[0].b =
						vrgb[1].r = vrgb[1].g = vrgb[1].b =
						vrgb[2].r = vrgb[2].g = vrgb[2].b = 255;
					}
				}
				else			// right side. use existing light values.
				{
					flipped = false;
					memcpy(vrgb + 0, rgb + v0, sizeof(LightRGB));
					memcpy(vrgb + 1, rgb + v1, sizeof(LightRGB));
					memcpy(vrgb + 2, rgb + v2, sizeof(LightRGB));
				}

				for (int v = 0; v < 3; v++, chain++)
				{
					if (vertex_slot[*chain] == 0xFFFF) // Must copy vertex to list.
					{
						MTVERTEX & current_mt = vertex_pool[vertex_pool_index];
						vertex_slot[*chain] = vertex_pool_index;

						int vchain = arch->vertex_batch_list[*chain];
						int tchain = arch->texture_batch_list[*chain];
						int tchain2 = (arch->texture_batch_list2) ? arch->texture_batch_list2[*chain] : -1;

						r = base_r;
						g = base_g;
						b = base_b;
					
						{
						//
						// Smooth-shaded. Use vertex light values computed above.
						//
							LightRGB * l = (flipped) ? vrgb + v : rgb + vchain;
							
							if (mat->flags & MF_DIFFUSE)
							{
								r += (mat->diffuse.r * l->r) >> 8;
								g += (mat->diffuse.g * l->g) >> 8;
								b += (mat->diffuse.b * l->b) >> 8;
							}
						}

						if(255 < (max_c = Tmax(Tmax(r, g), b)))
						{
							r = (r * 255) / max_c;
							g = (g * 255) / max_c;
							b = (b * 255) / max_c;
						}

						current_mt.color =
						current_mt.scolor = ARGB_MAKE(r, g, b, 0);
						current_mt.a = transparency;

						current_mt.u = arch->texture_vertex_list[tchain].u;
						current_mt.v = arch->texture_vertex_list[tchain].v;

						if (tchain2 == -1)
						{
							current_mt.u2 = current_mt.u;
							current_mt.v2 = current_mt.v;
						}
						else
						{
							current_mt.u2 = arch->texture_vertex_list[tchain2].u;
							current_mt.v2 = arch->texture_vertex_list[tchain2].v;
						}

						
						current_mt.pos.x = transformed_vertices[vchain].x;
						current_mt.pos.y = transformed_vertices[vchain].y;
						current_mt.pos.z = transformed_vertices[vchain].z;

						vertex_pool_index++;
					}

					// Add index to list:
					*idx_ptr++ = vertex_slot[*chain];

					ASSERT_FATAL(idx_ptr <= (index_list + index_list_len) || !idx_ptr);
				}
			}

			BATCH->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE ) ;
			index_list_index = idx_ptr - index_list;
			mtl_render_indexed_primitive_list(mat, (arch->texture_batch_list2) ? 2 : 1,
			rwm_flags);
		}
	}
}


//

void DeformablePart::deform(int mesh_index)
{
	if (mesh_index < 0 || mesh_index >= num_meshes)
	{
		return;
	}

	DeformablePartMesh * mesh = meshes + mesh_index;
	DeformablePartArchetype * archetype = mesh->arch;
	const bool arch_is_patch = archetype->is_patch();

// transform all vertices from local (bone) frames to world frame.
	for (int i = 0; i < archetype->num_bones; i++)
	{
		BoneInstance * bone = mesh->bones + i;

		if (bone->arch->num_vertices)
		{
			Transform xform ( ENG->get_transform(bone->instance) );

			bone->vertex_counter = 0;

			MATH_ENGINE()->transform_list(bone->transformed_vertices, xform, bone->arch->vertices, bone->arch->num_vertices);

			if( !arch_is_patch )
			{
				MATH_ENGINE()->transform_list(bone->transformed_normals, (Matrix)xform, bone->arch->normals, bone->arch->num_vertices);
			}
		}
	}

	int * cnt	= archetype->vertex_bone_cnt;
	int * id	= archetype->bone_id_list;
	float * wt	= archetype->bone_weight_list;
	Vector * vdst = transformed_vertices;
	Vector * ndst = transformed_normals;
	for (i = 0; i < archetype->object_vertex_cnt; i++, cnt++, vdst++, ndst++)
	{
		BoneInstance * b = mesh->bones + *id;
		id++;

		const int n = *cnt;
		if(n == 1) // don't have to multiply by weight
		{
			*vdst = b->transformed_vertices[b->vertex_counter];

			if( !arch_is_patch )	// mesh
			{
				*ndst = b->transformed_normals[b->vertex_counter];
			}
			
			b->vertex_counter++;
			wt++;
		}
		else
		{
			if( arch_is_patch )
			{
				*vdst = b->transformed_vertices[b->vertex_counter] * (*wt);
				b->vertex_counter++;
				wt++;

				for (int j = 1; j < n; j++, id++, wt++)
				{
					b = mesh->bones + *id;
					*vdst += b->transformed_vertices[b->vertex_counter] * (*wt);

					b->vertex_counter++;
				}
			}
			else // normal poly mesh
			{
				// assigning the first value avoids the memsets above
				*vdst = b->transformed_vertices[b->vertex_counter] * (*wt);
				*ndst = b->transformed_normals[b->vertex_counter] * (*wt);
				b->vertex_counter++;
				wt++;

				for (int j = 1; j < n; j++, id++, wt++)
				{
					b = mesh->bones + *id;
					*vdst += b->transformed_vertices[b->vertex_counter] * (*wt);
					*ndst += b->transformed_normals[b->vertex_counter] * (*wt);

					b->vertex_counter++;
				}
			}
		}
	}

	if( !arch_is_patch )
	{
		ndst = transformed_normals;
		for (i = 0; i < archetype->object_vertex_cnt; i++, ndst++)
		{
		// Many normals will have only one contributing bone, in which case there's
		// no need to normalize.
			if (archetype->vertex_bone_cnt[i] > 1)
			{
				ndst->normalize();
			}
		}

		// UV bone stuff
		{
			int index = 0;
			for(int i = 0; i < archetype->uv_bone_count; i++)
			{
				const int bone_id = archetype->uv_bone_id[i];

				BoneInstance *bone = mesh->bones + bone_id;

				Matrix orientation ( ENG->get_orientation(bone->instance) );

				INSTANCE_INDEX parent = ENG->get_instance_parent(bone->instance);
				if(parent != INVALID_INSTANCE_INDEX)
				{
					// transpose multiply
					orientation = ENG->get_orientation(parent).get_transpose() * orientation;
				}

				const Vector k ( orientation.get_k() );

				float du = archetype->x_to_u_scale[i] * k.x * archetype->uv_plane_distance[i];
				float dv = archetype->y_to_v_scale[i] * k.y * archetype->uv_plane_distance[i];

				// clamp against limits
				if(du < archetype->min_du[i])
				{
					du = archetype->min_du[i];
				}
				else if(du > archetype->max_du[i])
				{
					du = archetype->max_du[i];
				}

				if(dv < archetype->min_dv[i])
				{
					dv = archetype->min_dv[i];
				}
				else if(dv > archetype->max_dv[i])
				{
					dv = archetype->max_dv[i];
				}
				
				// update all uv's belonging to this bone
				for(int j = 0; j < archetype->uv_vertex_count[i]; j++)
				{
					TexCoord * tc = archetype->texture_vertex_list + archetype->uv_vertex_id[index];
					TexCoord * df = archetype->uv_default_list + index;

					tc->u = df->u + du;
					tc->v = df->v + dv;

					//archetype->texture_vertex_list[ archetype->uv_vertex_id[index] ] = TexCoord(archetype->uv_default_list[index].u + du, archetype->uv_default_list[index].v + dv);

					index++;
				}
			}
		}
	}
}

//

void DeformablePart::render(struct ICamera * camera, int mesh_index)
{
	if (mesh_index < 0 || mesh_index >= num_meshes)
	{
		return;
	}

	DeformablePartMesh * mesh = meshes + mesh_index;
	DeformablePartArchetype * archetype = mesh->arch;

	if (archetype->is_patch())
	{
		render_patches(camera, mesh);
	}
	else
	{
		render_mesh(camera, mesh);
	}
}

//

void DeformablePart::get_face_vertex_counts(int mesh_index, int * faces, int * vertices) const
{
	if (mesh_index < 0 || mesh_index >= num_meshes)
	{
		*faces =
		*vertices = 0;
		return;
	}

	DeformablePartMesh * mesh = meshes + mesh_index;
	DeformablePartArchetype * archetype = mesh->arch;

	if (archetype->is_patch())
	{
		*faces = last_face_cnt;
		*vertices = last_vertex_cnt;
	}
	else
	{
		*faces = archetype->face_cnt;
		*vertices = archetype->object_vertex_cnt;
	}
}

//

void DeformablePart::get_surface_pixel_error(int mesh_index, float * error) const
{
	if (mesh_index < 0 || mesh_index >= num_meshes)
	{
		*error = -2.0f;
		return;
	}

	DeformablePartMesh * mesh = meshes + mesh_index;
	DeformablePartArchetype * archetype = mesh->arch;

	if (archetype->is_patch())
	{
		*error = pixel_error;
	}
	else
	{
		*error = -1.0f;
	}
}

//

void DeformablePart::set_surface_pixel_error(int mesh_index, float error)
{
	pixel_error = error;
}

//

bool DeformablePart::visible_rect(RECT & rect, const struct ICamera * camera) const
{
	bool result = false;

	rect.left = rect.top = LONG_MAX;
	rect.right = rect.bottom = LONG_MIN;

	Vector v[8];
	Vector p;
	Matrix R;

	DeformablePartMesh * mesh = &meshes[0];

	for (int i = 0; i < mesh->arch->num_bones; i++)
	{
		BoneInstance * bone = mesh->bones + i;

		const BaseExtent * extent;
		if (PHYSICS && PHYSICS->get_extent(&extent, bone->instance))
		{
			if (extent->type == ET_SPHERE)
			{
				BaseExtent * box_extent = extent->child;
				bool got_box = false;
				while( box_extent )
				{
					if( box_extent->type == ET_BOX )
					{
						const BoxExtent * b = (const BoxExtent *) box_extent;
						v[0].set(+b->box.half_x, +b->box.half_y, +b->box.half_z);
						v[1].set(+b->box.half_x, +b->box.half_y, -b->box.half_z);
						v[2].set(+b->box.half_x, -b->box.half_y, -b->box.half_z);
						v[3].set(+b->box.half_x, -b->box.half_y, +b->box.half_z);
						v[4].set(-b->box.half_x, +b->box.half_y, +b->box.half_z);
						v[5].set(-b->box.half_x, +b->box.half_y, -b->box.half_z);
						v[6].set(-b->box.half_x, -b->box.half_y, -b->box.half_z);
						v[7].set(-b->box.half_x, -b->box.half_y, +b->box.half_z);

						got_box = true;
						break;
					}

					box_extent = box_extent->child;
				}

				if( !got_box ) // fall back to sphere
				{
					const SphereExtent * b = (const SphereExtent *) extent;
					v[0].set(+b->sphere.radius, +b->sphere.radius, +b->sphere.radius);
					v[1].set(+b->sphere.radius, +b->sphere.radius, -b->sphere.radius);
					v[2].set(+b->sphere.radius, -b->sphere.radius, -b->sphere.radius);
					v[3].set(+b->sphere.radius, -b->sphere.radius, +b->sphere.radius);
					v[4].set(-b->sphere.radius, +b->sphere.radius, +b->sphere.radius);
					v[5].set(-b->sphere.radius, +b->sphere.radius, -b->sphere.radius);
					v[6].set(-b->sphere.radius, -b->sphere.radius, -b->sphere.radius);
					v[7].set(-b->sphere.radius, -b->sphere.radius, +b->sphere.radius);
				}

				p = PHYSICS->get_center_of_mass(bone->instance);
				R = ENG->get_orientation(bone->instance);

				for (int j = 0; j < 8; j++)
				{
					float x, y, z;
					Vector vx = p + R * (v[j]);

					if (camera->point_to_screen(x, y, z, vx))
					{
						if (x < rect.left)		rect.left = x;
						if (x > rect.right)		rect.right = x;
						if (y < rect.top)		rect.top = y;
						if (y > rect.bottom)	rect.bottom = y;
						result = true;
					}
				}
			}
		}
	}

	return result;
}

//

U32 DeformableObject::get_num_active_scripts(void)
{
	update();
	return active_scripts.count();
}

//

void DeformableObject::describe_active_scripts(ActiveScriptDesc * desc)
{
	ActiveScriptDesc * dst = desc;
	ScriptLink * link = active_scripts.first();
	while (link)
	{
		dst->current_time = ANIM->get_current_time(link->instance);
		dst->instance = link->instance;

		dst++;
		link = active_scripts.next(link);
	}
}

//

void DeformableObject::pause(void)
{
	ScriptLink * link = active_scripts.first();
	while (link)
	{
		ANIM->script_stop(link->instance);
		link = active_scripts.next(link);
	}
}

//

void DeformableObject::resume(void)
{
	ScriptLink * link = active_scripts.first();
	while (link)
	{
		ANIM->script_start(link->instance);
		link = active_scripts.next(link);
	}
}

//

U32 DeformableObject::get_num_hardpoints(void) const
{
	return num_hardpoints;
}

//

const HardpointDesc * DeformableObject::get_hardpoints(void) const
{
	return hardpoints;
}

//

void COMAPI DeformableObject::on_event(unsigned int channel_id, void * user_supplied, const EventIterator & event_iterator)
{
	if (callback)
	{
		const char * script_name = NULL;
		ScriptLink * link = active_scripts.first();
		while (link)
		{
			if (!link->ik_script)
			{
				if (ANIM->script_contains_channel(link->instance, channel_id))
				{
					ANIM->get_instance_name(link->instance, script_name);
					break;
				}
			}
			link = active_scripts.next(link);
		}

		callback->on_event((unsigned int) script_name, user_supplied, event_iterator);
	}
}

//

void COMAPI DeformableObject::on_finished(unsigned int channel_id, void * user_supplied)
{
	if (callback)
	{
		callback->on_finished(channel_id, user_supplied);
	}
}

//

void COMAPI DeformableObject::on_loop(unsigned int channel_id, Transform & T, void * user_supplied)
{
// UGLY. Need more data to come back from the callback to easily identify source
// instead of searching script by script.

	ScriptLink * link = active_scripts.first();
	while (link)
	{
		if (!link->ik_script)
		{
			if (ANIM->script_contains_channel(link->instance, channel_id))
			{
				break;
			}
		}
		link = active_scripts.next(link);
	}

	if (link)
	{
		Vector pos = T.get_position();

		Vector hpos;
		float scale = parts[0]->meshes[0].arch->scale;

		float anim_height, script_start_height;
		if (ANIM->get_script_start_height(link->instance, script_start_height))
		{
			anim_height = scale * script_start_height;
		}
		else
		{
			anim_height = last_height;
		}

		float height = floor_height + anim_height;
		hpos = adjust_height(pos, height);
		T.set_position(hpos);

		Quaternion qh = get_heading_quaternion(T.get_orientation());
		T.set_orientation(qh);

		if (callback)
		{
			callback->on_loop(channel_id, T, user_supplied);
		}
	}
}
					   
//

BOOL32 COMAPI DeformableObject::intersect_ray(Vector & intersection, Vector & normal, const Vector & origin, const Vector & direction, int & num_bones_hit, INSTANCE_INDEX * bones_hit, const int * mesh_index) const
{
	BOOL32 result = FALSE;

	float min = FLT_MAX;
	Vector minx, minN;

	const int * idx = mesh_index;

	for (int i = 0; i < num_parts; i++, idx++)
	{
		result = parts[i]->intersect_ray(intersection, normal, origin, direction, num_bones_hit, bones_hit, *idx);
		if (result)
		{
			Vector dp = intersection - origin;
			float dsquared = dot_product(dp, dp);
			if (dsquared < min)
			{
				min = dsquared;
				minx = intersection;
				minN = normal;
			}
		}
	}

	if (result)
	{
		intersection = minx;
		normal = minN;
		normal.normalize();
	}

	return result;
}
