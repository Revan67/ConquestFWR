//
// Deformable object stuff.
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <limits.h>
#include <stdio.h>

#include "eng.h"
#include "deform.h"
#include "ICamera.h"
#include "system.h"

#include "heapobj.h"
#include "material.h"
#include "iprofileparser_utility.h"
#include "packed_argb.h"

//

#define IRP_CALL( call )	\
	if( BATCH ) {			\
		BATCH->call ;		\
	}						\
	else if( PIPE ) {		\
		PIPE->call ;		\
	}

//

using namespace Deform;

bool	use_constant_alpha = false;
U8		constant_alpha = 255;

//

void Deform::DebugPrint(char *fmt, ...)
{
	if (fmt)
	{
		char work[256];

		va_list va;
		va_start(va,fmt);
		vsprintf(work,fmt,va);
		va_end(va);

		if (DUMP)
		{
			DUMP->debug_printf(work);
		}
		else
		{
			OutputDebugString(work);
		}
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

		//control_word &= ~(EM_INVALID | EM_UNDERFLOW | EM_OVERFLOW | EM_ZERODIVIDE | EM_DENORMAL);
		control_word &= ~(EM_INVALID | EM_OVERFLOW | EM_ZERODIVIDE);
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
		
		PIPE->query_device_ability( RP_A_DEVICE_SOFTWARE, &value, NULL );
		
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

		PIPE->query_device_ability( RP_A_TEXTURE_STAGES, &value, NULL );
		opt_get_u32( DACOM, NULL, CLSID_Deform, "NumTSS", value, &device_num_tss );

		PIPE->query_device_ability( RP_A_TEXTURE_COORDINATES, &value, NULL );
		value = (value==1)? FALSE : TRUE;
		opt_get_u32( DACOM, NULL, CLSID_Deform, "Diffuse2AllowTC2", value, &device_supports_uvchannel1 );

		PIPE->query_device_ability( RP_A_BLEND_MUL_SRC, &value, NULL );
		opt_get_u32( DACOM, NULL, CLSID_Deform, "Diffuse2SrcBlend", value, &diffuse2_fallback_blend[0] );
		
		PIPE->query_device_ability( RP_A_BLEND_MUL_DST, &value, NULL );
		opt_get_u32( DACOM, NULL, CLSID_Deform, "Diffuse2DstBlend", value, &diffuse2_fallback_blend[1] );

		PIPE->query_device_ability( RP_A_BLEND_ADD_SRC, &value, NULL );
		opt_get_u32( DACOM, NULL, CLSID_Deform, "EmissiveSrcBlend", value, &emissive_fallback_blend[0] );
		
		PIPE->query_device_ability( RP_A_BLEND_ADD_DST, &value, NULL );
		opt_get_u32( DACOM, NULL, CLSID_Deform, "EmissiveDstBlend", value, &emissive_fallback_blend[1] );

		PIPE->query_device_ability( RP_A_BLEND_MATRIX, modes, NULL );

		opt_get_u32( DACOM, NULL, CLSID_Deform, "SpecularSrcBlend", D3DBLEND_SRCALPHA, &specular_fallback_blend[0] );
		opt_get_u32( DACOM, NULL, CLSID_Deform, "SpecularDstBlend", D3DBLEND_ONE, &specular_fallback_blend[1] );

		if( !rp_a_is_blend_supported( (D3DBLEND)specular_fallback_blend[0], (D3DBLEND)specular_fallback_blend[1], modes ) ) {
			specular_fallback_blend[0] = D3DBLEND_ONE;
			specular_fallback_blend[1] = D3DBLEND_ONE;
		}
	}

	opt_get_float( DACOM, NULL, CLSID_Deform, "MinPolySize", 0.15f, &min_poly_size );

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

void Deform::SetDivWeights(int div_cnt)
{
	int clean_cnt = _MAX(1, div_cnt);

	if(clean_cnt != sub_div_cnt)
	{
		sub_div_cnt = clean_cnt;
		delete [] div_weights;
		delete [] div_n_weights;
		div_weights = new float[4 * (sub_div_cnt + 1)];
		div_n_weights = new float[4 * (sub_div_cnt + 1)];

		for(int i = 0; i <= sub_div_cnt; i++)
		{
			float pu = (float)i / (float)sub_div_cnt;
			float pu2 = pu * pu;
			float pu1 = 1.0f - pu;
			float pu12 = pu1 * pu1;

			div_weights[4 * i    ] = pu12 * pu1;
			div_weights[4 * i + 1] = 3.0f * pu * pu12;
			div_weights[4 * i + 2] = 3.0f * pu2 * pu1;
			div_weights[4 * i + 3] = pu2 * pu;

			div_n_weights[4 * i    ] = -3.0f * pu12;
			div_n_weights[4 * i + 1] =  3.0f * (pu12 - 2.0f * pu * pu1);
			div_n_weights[4 * i + 2] =  3.0f * (2.0f * pu * pu1 - pu2);
			div_n_weights[4 * i + 3] =  3.0f * pu2;
		}
	}
}

//

LList<DeformablePartArchetype>	DeformableObject::archetypes;


bool DeformOpen(IDAComponent * system, IEngine * engine, IDumpText * dump_text)
{
	if (!active)
	{
		if (system) 
		{
			if (system->QueryInterface("IRenderPrimitive", (void**) &BATCH) == GR_OK)
			{
				GENERAL_TRACE_1("DEFORM.LIB: Using IRenderPrimitive.\n");
			}
			
			if (system->QueryInterface("IRenderPipeline", (void**) &PIPE) == GR_OK)
			{
				GENERAL_TRACE_1("DEFORM.LIB: Got IRenderPipeline.\n");
			}

			if(!BATCH && !PIPE)
			{
				GENERAL_ERROR("DEFORM.LIB can't get IRenderPipeline or IRenderPrimitive pointer.\n");
				return active;
			}
		}

		DACOM = DACOM_Acquire();
		ENG = engine;
		if (ENG)
		{
			ENG->AddRef();
			ENG->QueryInterface("IModel",			(void **) &MODEL);
			ENG->QueryInterface(IID_ILightManager,	(void **) &LIGHT);
			ENG->QueryInterface("ITXMLib",			(void **) &TXMLIB);
			ENG->QueryInterface("IAnimation",		(void **) &ANIM);
			ENG->QueryInterface("IChannel",			(void **) &CHANNEL);
			ENG->QueryInterface("IHardpoint",		(void **) &HARDPOINT);
			ENG->QueryInterface("IPhysics",			(void **) &PHYSICS);
			ENG->QueryInterface("ICollision",		(void **) &COLLIDE);
		}

		if (!DACOM)		GENERAL_ERROR("DEFORM.LIB can't get DACOM pointer.\n");
		if (!ENG)		GENERAL_ERROR("DEFORM.LIB can't get IEngine pointer.\n");
		if (!MODEL)		GENERAL_ERROR("DEFORM.LIB can't get IModel pointer.\n");
		if (!LIGHT)		GENERAL_ERROR("DEFORM.LIB can't get ILightManager pointer.\n");
		if (!TXMLIB)	GENERAL_ERROR("DEFORM.LIB can't get ITXMLib pointer.\n");
		if (!ANIM)		GENERAL_ERROR("DEFORM.LIB can't get IAnimation pointer.\n");
		if (!CHANNEL)	GENERAL_ERROR("DEFORM.LIB can't get IChannel pointer.\n");
		if (!HARDPOINT)	GENERAL_ERROR("DEFORM.LIB can't get IHardpoint pointer.\n");
		if (!PHYSICS)	GENERAL_ERROR("DEFORM.LIB can't get IPhysics pointer.\n");
		if (!COLLIDE)	GENERAL_ERROR("DEFORM.LIB can't get ICollision pointer.\n");

		active = ((BATCH || PIPE) && DACOM && ENG && MODEL && LIGHT && TXMLIB && ANIM && CHANNEL && HARDPOINT && PHYSICS && COLLIDE);

		DUMP = dump_text;

		opt_get_u32( DACOM, NULL, CLSID_Deform, "SpecularMode", 0, &specular_mode );
		opt_get_string( DACOM, NULL, CLSID_Deform, "SpecularTextureName",	"SpecularHighlight", specular_texture_name, 64 );

		if(BATCH)
			pb.SetIRenderPrimitive(BATCH);
		else
			pb.SetPipeline(PIPE);
	}

	return active;
}

//
// same as DeformOpen(), but releases references to engine components.
//
bool DeformOpen2(IDAComponent * system, IEngine * engine, IDumpText * dump_text)
{
	bool result;

	if (DeformOpen(system, engine, dump_text))
	{
		ENG->Release();
		MODEL->Release();
		LIGHT->Release();
		TXMLIB->Release();
		ANIM->Release();
		CHANNEL->Release();
		HARDPOINT->Release();
		PHYSICS->Release();
		COLLIDE->Release();

		result = true;
	}
	else
	{
		result = false;
	}

	return result;
}



//

SCRIPT_SET_ARCH DefScriptInfo::get_script_set(void) const
{
	return part->meshes[0].arch->script_set;
}

//

float DefScriptInfo::get_scale(void) const
{
	return part->meshes[0].arch->scale;
}

//

void DefScriptNode::shutdown(void)
{
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

static inline void Release(IDAComponent * comp)
{
	if (comp)
	{
		comp->Release();
	}
}

//

void DeformClose(void)
{
	if (active)
	{
		Deform::pb.~MTPrimitiveBuilder();

		Release(BATCH);		BATCH = NULL;
		Release(PIPE);		PIPE = NULL;

		Release(COLLIDE);	COLLIDE = NULL;
		Release(PHYSICS);	PHYSICS = NULL;
		Release(HARDPOINT);	HARDPOINT = NULL;
		Release(MODEL);		MODEL = NULL;
		Release(LIGHT);		LIGHT = NULL;
		Release(TXMLIB);	TXMLIB = NULL;
		Release(ANIM);		ANIM = NULL;
		Release(CHANNEL);	CHANNEL = NULL;
 		Release(ENG);		ENG = NULL;
		Release(DUMP);		DUMP = NULL;
		Release(DACOM);		DACOM = NULL;
		active = false;
		got_ini_info = false;

		delete_lists();
		delete_pools();
	}
}

//
// Same as DeformClose(), but doesn't release engine component references.
//
void DeformClose2(void)
{
	if (active)
	{
		Deform::pb.~MTPrimitiveBuilder();

		Release(BATCH);		BATCH = NULL;
		Release(PIPE);		PIPE = NULL;
		Release(DUMP);		DUMP = NULL;
		Release(DACOM);		DACOM = NULL;

		active = false;
		got_ini_info = false;

		delete_pools();
		delete_lists();
	}
}

//

char * BoneDescriptor::mesh_name = NULL;

//

BoneInstance::BoneInstance(const BoneArchetype * _arch, const bool need_normals)
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

	INSTANCE_INDEX child = MODEL->get_child(root);
	while (child != INVALID_INSTANCE_INDEX)
	{
		build_tree(tree, child, n);
		child = MODEL->get_child(root, child);
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
		root = ENG->create_instance(arch->arch_index);
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
				extras[eidx++] = ENG->create_instance(arch0->extras[i]);
			}
		}

		INSTANCE_INDEX * tree = new INSTANCE_INDEX[meshes[0].arch->num_bones];
		int n = 0;
		build_tree(tree, root, n);

		for (int m = 0; m < num_meshes; m++)
		{
			const DeformablePartArchetype * arch = meshes[m].arch;

			int num_extras_attached = 0;

			meshes[m].bones = new BoneInstance *[arch->num_bones];
			for (int i = 0; i < arch->num_bones; i++)
			{
				BoneArchetype * barch = arch->bones + i;

				meshes[m].bones[i] = new BoneInstance(barch, !arch->is_patch());

				BoneInstance * binst = meshes[m].bones[i];

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
							if( !strcmp( MODEL->get_name( tree[j] ), name) )
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
		for (int i = 0; i < meshes[m].arch->num_bones; i++)
		{
			delete meshes[m].bones[i];
			meshes[m].bones[i] = NULL;
		}

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
			BoneInstance * bone = meshes[mesh_index].bones[i];

			const BaseExtent * extent;
			if (PHYSICS->get_extent(&extent, bone->instance) && extent)
			{
				Transform T = ENG->get_transform(bone->instance);
				T.set_position(PHYSICS->get_center_of_mass(bone->instance));

				Vector N;
				if (COLLIDE->intersect_ray_with_extent_hierarchy(intersection, N, origin, direction, *extent, T, true))
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
	script_count = 0;

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

BOOL32 DeformableObject::get_scripts(const char * script_names[]) const
{
	BOOL32 result = 0;

	int count = 0;
	DefScriptNode * node = scripts.list;
	for (int i = 0; i < scripts.list_size; i++, node++)
	{
		if (node->index != -1)
		{
			script_names[count++] = node->info.name;
			result |= 1;
		}
	}

	return result;
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
// Don't need a separate count phase, just add each script as it comes in.
//
static void ScriptCallback(const char * name, void * misc)
{
	EnumerateScriptInfo * info = (EnumerateScriptInfo *) misc;
	DeformableObject * obj = info->obj;
	DefScriptInfo sinfo;
	sinfo.name = strdup(name);
	sinfo.part = info->part;
	obj->scripts.allocate(&sinfo);
	obj->script_count++;
}
	
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
				GENERAL_ERROR("DEFORM.LIB: Bone out of range in DeformablePartArchetype::setup().\n");
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

//BoneDescriptor bDesc[128];

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
	ARCHETYPE_INDEX arch = ENG->get_archetype(root);

	HP_object obj;
	obj.list = &list;
	obj.object = root;

	HARDPOINT->enumerate_hardpoints(HPCallback, arch, &obj);
	ENG->release_archetype(arch);

	INSTANCE_INDEX child = MODEL->get_child(root);
	while (child != INVALID_INSTANCE_INDEX)
	{
		RecursivelyEnumerateHardpoints(child, list);
		child = MODEL->get_child(root, child);
	}
}

//

void BuildTree(INSTANCE_INDEX);

bool DeformableObject::create(const DeformDesc & desc, Channel::IEventHandler * _callback, void * _user_data, IEngineInstance * user_instance)
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

		// Enumerate part's scripts.
			EnumerateScriptInfo info;
			info.obj = this;
			info.part = parts[i];

			if (part_archs[0]->script_set != INVALID_SCRIPT_SET_ARCH)
			{
				ANIM->enumerate_scripts(ScriptCallback, part_archs[0]->script_set, &info);
			}
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
								OutputDebugString("DEFORM: Can't connect parts.\n");
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
	while ((curr = MODEL->get_parent(prev)) != INVALID_INSTANCE_INDEX)
	{
		prev = curr;
	}

	root = prev;

//
// Go through scripts, guessing as to whether each is an overlay or not.
//
	DefScriptNode * node = scripts.list;
	for (i = 0; i < scripts.list_size; i++, node++)
	{
		if (node->index != -1)
		{
			DefScriptInfo * info = &node->info;

			if (info->part->root == root)
			{
				info->overlay = true;

				SCRIPT_INST inst = ANIM->create_script_inst(info->get_script_set(), info->part->root, info->name);

				Channel::Target target;

				U32 num_channels = ANIM->get_script_channel_count(info->get_script_set(), info->name);
				for (U32 c = 0; c < num_channels; c++)
				{
					ANIM->get_script_channel_target(target, inst, c);
					if (target.type == Channel::OBJECT)
					{
						info->overlay = false;
						break;
					}
				}

				ANIM->release_script_inst(inst);
			}
			else
			{
				info->overlay = false;
			}
		}
	}

//#define DYNAMICS_TEST
#ifdef DYNAMICS_TEST
INSTANCE_INDEX find_child(INSTANCE_INDEX root, const char * name);

//INSTANCE_INDEX shld = find_child(root, "Root");
INSTANCE_INDEX shld = find_child(root, "T0_RCollarBone");
//INSTANCE_INDEX shld = find_child(root, "rshoulderjoint");
BuildTree(shld);
#endif

	return result;
}

//

void DeformableObject::destroy(void)
{
	if (active)
	{
		scripts.free();

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

S32 DeformableObject::get_script_index(const char * script_name) const
{
	DefScriptInfo src;
	src.name = const_cast<char *>(script_name);
	S32 idx = scripts.search(&src);
	src.name = NULL;
	return idx;
}

//

DefScriptInfo * DeformableObject::get_script_info(const char * script_name) const
{
	DefScriptInfo * result;

	S32 idx = get_script_index(script_name);
	if (idx != -1)
	{
		result = &(scripts.list[idx].info);
	}
	else
	{
		result = NULL;
	}

	return result;
}
 
DefScriptInfo * DeformableObject::get_script_info(S32 idx) const
{
	DefScriptInfo * result;

	if (idx < 0)
	{
		result = NULL;
	}
	else
	{
		result = &(scripts.list[idx].info);
	}

	return result;
}

//

CHANNEL_ARCHETYPE_INDEX DeformableObject::add_channel(const char * script_name, const char * channel_name)
{
	BOOL32 result = 0;
	DefScriptInfo * info = get_script_info(script_name);
	if (info)
	{
		result = ANIM->add_channel(info->get_script_set(), script_name, channel_name);
	}
	return result;
}

//

void DeformableObject::replace_channel_data(const char * script_name, CHANNEL_ARCHETYPE_INDEX channel_arch, const IChannel::Header& header_replacement, const void* data_replacement, size_t frame_size_replacement, float duration_replacement)
{
	DefScriptInfo * info = get_script_info(script_name);
	if (info)
	{
		ANIM->replace_channel_data(info->get_script_set(), channel_arch, header_replacement, data_replacement, frame_size_replacement, duration_replacement);
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

void DeformableObject::render(struct ICamera * camera, const int * mesh_index, int tessellation_cnt)
{
	const int * idx = mesh_index;

	for (int i = 0; i < num_parts; i++, idx++)
	{
		if (parts[i])
		{
			parts[i]->render(camera, *idx, tessellation_cnt);
		}
	}

// Render any child objects. This assumes bones have no appearance.
	ENG->render_instance(camera, root, RF_FILL, NULL);
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
	ENG->render_instance(camera, root, RF_FILL, NULL);

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

bool DeformableObject::start_motion(const char * script_name, float start_time, float transition_duration, float time_scale, float weight, unsigned int flags, float heading)
{
	SCRIPT_INST inst = start_motion2(script_name, start_time, transition_duration, time_scale, weight, flags, heading);

	return (inst != INVALID_SCRIPT_INST);
}

//

SCRIPT_INST DeformableObject::start_motion2(const char * script_name, float start_time, float transition_duration, float time_scale, float weight, unsigned int flags, float heading)
{
	SCRIPT_INST result = INVALID_SCRIPT_INST;

	update();

	S32 idx = get_script_index(script_name);
	if (idx != -1)
	{
		DefScriptInfo * info = &scripts.list[idx].info;
		if (info)
		{
			SCRIPT_INST inst = ANIM->create_script_inst(info->get_script_set(), info->part->root, info->name, this, user_data);
			if (inst != INVALID_SCRIPT_INST)
			{
				ScriptLink * link = active_scripts.alloc();
				ASSERT(link);
				link->index = get_script_index(script_name);
				link->instance = inst;

				Vector pos = ENG->get_position(root);
				Vector hpos;

				float anim_height, script_start_height;
				if (ANIM->get_script_start_height(info->get_script_set(), info->name, script_start_height))
				{
					anim_height = info->get_scale() * script_start_height;
				}
				else
				{
					anim_height = last_height;
				}

				link->start_height = floor_height + anim_height;
				hpos = adjust_height(pos, link->start_height);

				Quaternion qh;
				if (heading < 0)
				{
					qh = get_heading_quaternion(ENG->get_orientation(root));
				}
				else
				{
					qh = compute_heading_quaternion(heading);
				}

				Transform T;
				T.set_position(hpos);
				T.set_orientation(qh);

				ANIM->script_start(	inst, 
									flags,					// direction, loop, etc.
									start_time,				// where to start
									time_scale,				// time scalar.
									transition_duration,	// transition duration
									weight,
									info->overlay,			// overlay yes/no.
									info->get_scale(),
									&T);

				result = inst;
			}
			else
			{
			// try again for debug purposes.
				inst = ANIM->create_script_inst(info->get_script_set(), info->part->root, info->name, this, user_data);
			}
		}
	}
	
	return result;
}

//

SCRIPT_INST DeformableObject::start_motion3(SCRIPT_SET_ARCH set, const char * script_name, float start_time, float transition_duration, float time_scale, float weight, unsigned int flags, float heading)
{
	SCRIPT_INST result = INVALID_SCRIPT_INST;

	update();

// WILL THIS WORK? Or do we need to give it the correct root?
	SCRIPT_INST inst = ANIM->create_script_inst(set, root, script_name, this, user_data);
	if (inst != INVALID_SCRIPT_INST)
	{
		ScriptLink * link = active_scripts.alloc();
		ASSERT(link);
		link->index = get_script_index(script_name);
		link->instance = inst;

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

		link->start_height = floor_height + anim_height;
		hpos = adjust_height(pos, link->start_height);

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

		ANIM->script_start(	inst, 
							flags,					// direction, loop, etc.
							start_time,				// where to start
							time_scale,				// time scalar.
							transition_duration,	// transition duration
							weight,
						// TODO: ADDRESS OVERLAY ISSUE>
							false,					// overlay yes/no.
							scale,
							&T);

		result = inst;
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

	SCRIPT_INST result = start_motion3(set, script_name, start_time, transition_duration, time_scale, weight, flags, heading);

// build up list of channel data from root to locked bone.
	const Joint * joints[64];
	Quaternion jq[64];
	int i = 0;

	Channel::Target t;
	t.type = Channel::JOINT;

	INSTANCE_INDEX bone = locked_bone;
	while (bone != root)
	{
		INSTANCE_INDEX parent = MODEL->get_parent(bone);

		JOINT_INDEX jidx = MODEL->find_joint(bone, parent);
		assert(jidx != INVALID_JOINT_INDEX);

		const Joint * jnt = MODEL->get_joint(jidx);
		assert(jnt->type == JT_SPHERICAL);
		joints[i] = jnt;

		t.joint = jidx;

		CHANNEL_INSTANCE_INDEX channel = ANIM->find_channel(result, t);
		CHANNEL_ARCHETYPE_INDEX channel_arch = CHANNEL->get_channel_archetype(channel);

		unsigned int cdt = CHANNEL->get_data_type(channel_arch);
		assert(cdt == Channel::DT_QUATERNION);

		CHANNEL->get_channel_data_at_time(channel, start_time, &jq[i].w);

		bone = parent;
		i++;
	}

	int num_joints = i;

// get root transform.
	t.type = Channel::OBJECT;
	t.object = root;
	float f[7];
	CHANNEL_INSTANCE_INDEX channel = ANIM->find_channel(result, t);
	CHANNEL_ARCHETYPE_INDEX channel_arch = CHANNEL->get_channel_archetype(channel);
	unsigned int cdt = CHANNEL->get_data_type(channel_arch);
	assert(cdt == (Channel::DT_VECTOR | Channel::DT_QUATERNION));
	CHANNEL->get_channel_data_at_time(channel, start_time, f);
	Vector v(f[0], f[1], f[2]);
	Quaternion q(f[3], f[4], f[5], f[6]);
	Matrix R(q);

	Vector p, pprev = v;
	Matrix Rprev = R;
// now go back and evaluate kinematic chain based on animation data in order to 
// determine locked bone position.
	for (i = num_joints-1; i >= 0; i--)
	{
		Matrix Rj(jq[i]);
		const Joint * j = joints[i];

		R = Rprev * Rj * j->rel_orientation;
		p = pprev + Rprev * j->parent_point - R * j->child_point;

		pprev = p;
		Rprev = R;
	}

	Vector offset = p - pb0;

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

BOOL32 DeformableObject::get_script_events(const char * script_name, EventIterator & events) const
{
	BOOL32 result = 0;
	DefScriptInfo * info = get_script_info(script_name);
	if (info)
	{
		result = ANIM->get_script_events(info->get_script_set(), script_name, events);
	}

	return result;
}

//

float DeformableObject::get_script_duration(const char * script_name) const
{
	float result = 0;
	DefScriptInfo * info = get_script_info(script_name);
	if (info)
	{
		result = ANIM->get_duration(info->get_script_set(), script_name);
	}

	return result;
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
				float scale;
				DefScriptInfo * info = get_script_info(link->index);
				if (info)
				{
					scale = info->get_scale();
				}
				else
				{
				// use 0th part's scale?
					scale = parts[0]->meshes[0].arch->scale;
				}

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
	MODEL->update_tree(root);
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
	MODEL->update_tree(root);
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
				float scale;
				
				DefScriptInfo * info = get_script_info(link->index);
				if (info)
				{
					scale = info->get_scale();
				}
				else
				{
				// use 0th part's scale?
					scale = parts[0]->meshes[0].arch->scale;
				}


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
	script_set = INVALID_SCRIPT_SET_ARCH;

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
    					if (lm)
						{
							setup(sk_file, skeleton_name, desc, num_bones);
							valid = true;
						}
						else
						{
							char temp[128];
							sprintf(temp, "DEFORM.LIB: Error loading mesh %s\n", pdesc.meshes[mesh_number].mesh_name);
							GENERAL_ERROR(temp);
						}
					}
					else
					{
						char temp[128];
						sprintf(temp, "DEFORM: Error loading skeleton from file %s\n", skeleton_name);
						GENERAL_ERROR(temp);
					}
				}
				else
				{
					char temp[128];
					sprintf(temp, "DEFORM: Unable to open skeleton file %s\n", skeleton_name);
					GENERAL_ERROR(temp);
				}
			}
			else
			{
				char temp[128];
				sprintf(temp, "DEFORM: Error reading file %s\n", pdesc.meshes[mesh_number].mesh_name);
				GENERAL_ERROR(temp);
			}
		}
		else
		{
			char temp[128];
			sprintf(temp, "DEFORM: Unable to find Skeleton chunk in %s\n", pdesc.meshes[mesh_number].mesh_name);
			GENERAL_ERROR(temp);
		}

		file->Release();

		script_set = pdesc.anim_script_set;
	}
	else
	{
		char temp[128];
		sprintf(temp, "DEFORM: Unable to open file %s\n", pdesc.meshes[mesh_number].mesh_name);
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

static LightRGB rgb[1024];
static int DoubleSided[1024];

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
	IRP_CALL( set_texture_stage_state( 0, D3DTSS_ALPHAOP,		D3DTOP_SELECTARG2 ))
	IRP_CALL( set_texture_stage_state( 0, D3DTSS_ALPHAARG1,		D3DTA_TEXTURE ))
	IRP_CALL( set_texture_stage_state( 0, D3DTSS_ALPHAARG2,		D3DTA_DIFFUSE ))
	IRP_CALL( set_texture_stage_state( 0, D3DTSS_COLOROP,		D3DTOP_SELECTARG2 ))
	IRP_CALL( set_texture_stage_state( 0, D3DTSS_COLORARG1,		D3DTA_TEXTURE ))
	IRP_CALL( set_texture_stage_state( 0, D3DTSS_COLORARG2,		D3DTA_DIFFUSE ))
	IRP_CALL( set_sampler_state( 0, D3DSAMP_MINFILTER,		D3DTEXF_LINEAR ))
	IRP_CALL( set_sampler_state( 0, D3DSAMP_MAGFILTER,		D3DTEXF_LINEAR ))
	IRP_CALL( set_sampler_state( 0, D3DSAMP_MIPFILTER,		D3DTEXF_POINT ))

	if(GET_TC_WRAP_MODE(mat->texture_flags) == TC_WRAP_UV_1 && uv_ch_num >= 2)
	{
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	1 ))
	}
	else
	{
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	0 ))
	}

	IRP_CALL( set_render_state( D3DRS_ZENABLE,			TRUE ))
	IRP_CALL( set_render_state( D3DRS_ZWRITEENABLE,	TRUE ))


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
			IRP_CALL( set_texture_stage_texture( stage_cnt, mat->texture_id ))
		}
		else {
			IRP_CALL( set_texture_stage_texture( stage_cnt, 0 ))
		}

		if( mat->texture_id ) {
			
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_COLOROP,	D3DTOP_MODULATE ))
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_ALPHAOP,	D3DTOP_MODULATE ))
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_ADDRESSU,	MAT_GET_ADDR_MODE(mat->texture_flags,0) ))
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_ADDRESSV,	MAT_GET_ADDR_MODE(mat->texture_flags,1) ))
				
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
				IRP_CALL( set_texture_stage_texture( stage_cnt, mat->second_diffuse_texture_id ))
			}
			else {
				IRP_CALL( set_texture_stage_texture( stage_cnt, 0 ))
			}
			
			if(GET_TC_WRAP_MODE(mat->second_diffuse_texture_flags) == TC_WRAP_UV_1 && uv_ch_num >= 2)
			{
				IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_TEXCOORDINDEX,	1 ))
			}
			else
			{
				IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_TEXCOORDINDEX,	0 ))
			}

			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_COLOROP,	D3DTOP_MODULATE ))
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_ALPHAOP,	D3DTOP_MODULATE ))
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_ADDRESSU,	MAT_GET_ADDR_MODE(mat->second_diffuse_texture_flags,0) ))
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_ADDRESSV,	MAT_GET_ADDR_MODE(mat->second_diffuse_texture_flags,1) ))

			if( stage_cnt ) {
				IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_COLORARG1,	D3DTA_TEXTURE ))
				IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_COLORARG2,	D3DTA_CURRENT ))
				IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_ALPHAARG1,	D3DTA_TEXTURE ))
				IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_ALPHAARG2,	D3DTA_CURRENT ))
				IRP_CALL( set_texture_stage_state( stage_cnt, D3DSAMP_MINFILTER,	D3DTEXF_LINEAR ))
				IRP_CALL( set_texture_stage_state( stage_cnt, D3DSAMP_MAGFILTER,	D3DTEXF_LINEAR ))
				IRP_CALL( set_texture_stage_state( stage_cnt, D3DSAMP_MIPFILTER,	D3DTEXF_POINT ))
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
			IRP_CALL( set_texture_stage_texture( stage_cnt, mat->emissive_texture_id ))
		}
		else {
			IRP_CALL( set_texture_stage_texture( stage_cnt, 0 ))
		}

		if(GET_TC_WRAP_MODE(mat->emissive_texture_flags) == TC_WRAP_UV_1 && uv_ch_num >= 2)
		{
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_TEXCOORDINDEX,	1 ))
		}
		else
		{
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_TEXCOORDINDEX,	0 ))
		}

		IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_COLOROP,	D3DTOP_ADD ))
		IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_ADDRESSU,	MAT_GET_ADDR_MODE(mat->emissive_texture_flags,0) ))
		IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_ADDRESSV,	MAT_GET_ADDR_MODE(mat->emissive_texture_flags,1) ))
		
		if( stage_cnt ) {
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_COLORARG1,	D3DTA_TEXTURE ))
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_COLORARG2,	D3DTA_CURRENT ))
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_ALPHAARG1,	D3DTA_TEXTURE ))
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_ALPHAARG2,	D3DTA_CURRENT ))
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DSAMP_MINFILTER,	D3DTEXF_LINEAR ))
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DSAMP_MAGFILTER,	D3DTEXF_LINEAR ))
			IRP_CALL( set_texture_stage_state( stage_cnt, D3DSAMP_MIPFILTER,	D3DTEXF_POINT ))
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
	IRP_CALL( set_render_state( D3DRS_ALPHABLENDENABLE, (blend_needed > 0) ));
	if( blend_needed ) {
		IRP_CALL( set_render_state( D3DRS_SRCBLEND, mat->src_blend ));
		IRP_CALL( set_render_state( D3DRS_DESTBLEND, mat->dst_blend ));
	}

	IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_COLOROP, D3DTOP_DISABLE ))
	IRP_CALL( set_texture_stage_state( stage_cnt, D3DTSS_ALPHAOP, D3DTOP_DISABLE ))

	if( mat->num_passes < 0 ) {
		if( SUCCEEDED( PIPE->verify_state( (U32*)&mat->num_passes ) ) ) {
			mat->num_passes = 1;
		}
		else {
			mat->num_passes = stage_cnt;
		}
	}
	
	U32 enable_specular = mat->shininess && (mat->shininess_width > 0.0) && !(mat->flags & MF_NO_SPECULAR_PASS);

	if( enable_specular && specular_mode == 1 ) {
		IRP_CALL( set_render_state( D3DRS_SPECULARENABLE, TRUE ));
	}
	else {
		IRP_CALL( set_render_state( D3DRS_SPECULARENABLE, FALSE ));
	}
	
	BATCH->set_state( RPR_STATE_ID, mat_uid++ );

	U32 clip = 0;

	if( mat->num_passes > 1 ) {
	
		// This texture count/configuration can't be done in one pass
		// do multiple passes.

		// Stage0 will already be setup above
		IRP_CALL( draw_indexed_primitive( D3DPT_TRIANGLELIST, MTVERTEX_FVFFLAGS, vertex_pool,
			vertex_pool_index, index_list, index_list_index, clip ) )

		if( enable_specular && specular_mode == 1 ) {
			IRP_CALL( set_render_state( D3DRS_SPECULARENABLE, FALSE ));
		}

		// set up framebuffer blending for next N passes
		//
		IRP_CALL( set_render_state( D3DRS_ALPHABLENDENABLE, TRUE ) )
		IRP_CALL( set_render_state( D3DRS_ZENABLE, TRUE ) )
		IRP_CALL( set_render_state( D3DRS_ZWRITEENABLE, FALSE ) )
		IRP_CALL( set_render_state( D3DRS_ZFUNC, D3DCMP_LESSEQUAL ) )


		// Diffuse2
		//
		if( (stage_zero_flags != MF_NO_DIFFUSE2_PASS) && !(mat->flags & MF_NO_DIFFUSE2_PASS) &&
			mat->second_diffuse_texture_id )
		{

			IRP_CALL( set_render_state( D3DRS_SRCBLEND, diffuse2_fallback_blend[0] ))
			IRP_CALL( set_render_state( D3DRS_DESTBLEND, diffuse2_fallback_blend[1] ))
			
			if(GET_TC_WRAP_MODE(mat->second_diffuse_texture_flags) == TC_WRAP_UV_1 && uv_ch_num >= 2)
			{
				IRP_CALL( set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	1 ))
			}
			else
			{
				IRP_CALL( set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	0 ))
			}

			if( !(rwm_flags & RWM_DONT_TEXTURE) ) {
				IRP_CALL( set_texture_stage_texture( 0, mat->second_diffuse_texture_id ))
			}
			else {
				IRP_CALL( set_texture_stage_texture( 0, 0 ))
			}

			IRP_CALL( set_texture_stage_state( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 ))
			IRP_CALL( set_texture_stage_state( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE ))

			IRP_CALL( set_texture_stage_state( 0, D3DTSS_ADDRESSU,	MAT_GET_ADDR_MODE(mat->second_diffuse_texture_flags,0) ))
			IRP_CALL( set_texture_stage_state( 0, D3DTSS_ADDRESSV,	MAT_GET_ADDR_MODE(mat->second_diffuse_texture_flags,1) ))
			
			BATCH->set_state( RPR_STATE_ID, mat_uid++ );
		
			IRP_CALL( draw_indexed_primitive( D3DPT_TRIANGLELIST, MTVERTEX_FVFFLAGS, vertex_pool,
				vertex_pool_index, index_list, index_list_index, clip ) )
		}

		// Emissive
		//
		if( (stage_zero_flags != MF_NO_EMITTER_PASS) && !(mat->flags & MF_NO_EMITTER_PASS) && mat->emissive_texture_id ) {
			
			IRP_CALL( set_render_state( D3DRS_SRCBLEND, emissive_fallback_blend[0] ))
			IRP_CALL( set_render_state( D3DRS_DESTBLEND, emissive_fallback_blend[1] ))
			
			if(GET_TC_WRAP_MODE(mat->emissive_texture_flags) == TC_WRAP_UV_1 && uv_ch_num >= 2)
			{
				IRP_CALL( set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	1 ))
			}
			else
			{
				IRP_CALL( set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	0 ))
			}

			if( !(rwm_flags & RWM_DONT_TEXTURE) ) {
				IRP_CALL( set_texture_stage_texture( 0, mat->emissive_texture_id ))
			}
			else {
				IRP_CALL( set_texture_stage_texture( 0, 0 ))
			}
			
			IRP_CALL( set_texture_stage_state( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 ))
			IRP_CALL( set_texture_stage_state( 0, D3DTSS_ALPHAOP,   D3DTOP_DISABLE ))

			IRP_CALL( set_texture_stage_state( 0, D3DTSS_ADDRESSU, MAT_GET_ADDR_MODE(mat->emissive_texture_flags,0) ))
			IRP_CALL( set_texture_stage_state( 0, D3DTSS_ADDRESSV, MAT_GET_ADDR_MODE(mat->emissive_texture_flags,1) ))
			
			BATCH->set_state( RPR_STATE_ID, mat_uid++ );

			IRP_CALL( draw_indexed_primitive( D3DPT_TRIANGLELIST, MTVERTEX_FVFFLAGS, vertex_pool,
				vertex_pool_index, index_list, index_list_index, clip ) )
		}

		IRP_CALL( set_render_state( D3DRS_ZFUNC, D3DCMP_LESS ) )
	}
	else if( mat->num_passes >= 0 ) {

		IRP_CALL( draw_indexed_primitive( D3DPT_TRIANGLELIST, MTVERTEX_FVFFLAGS, vertex_pool,
			vertex_pool_index, index_list, index_list_index, clip ) )
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
	LIGHT->light_vertices(rgb, transformed_vertices, transformed_normals, arch->object_vertex_cnt);

	verify_lists( _MAX(3 * arch->face_cnt, arch->vertex_batch_cnt));
	verify_pools(3 * arch->face_cnt);

	Transform world_to_view ( ((ICamera*)camera)->get_inverse_transform() );

	BoneInstance *bone = mesh->bones[0];
	Vector pos_in_camera ( world_to_view * ENG->get_position( MODEL->get_root(bone->instance) ) );

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

							if(255 < (max_c = _MAX(_MAX(r, g), b)))
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

		IRP_CALL ( set_render_state( D3DRS_CULLMODE, D3DCULL_CW ) )
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
			for (int df = 0; df < num_double_sided; df++)
			{
				const int * chain = group->face_vertex_chain + DoubleSided[df] * 3;

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

					LIGHT->light_vertices(vrgb, vtx, Nv, 3);
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

						if(255 < (max_c = _MAX(_MAX(r, g), b)))
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

			IRP_CALL ( set_render_state( D3DRS_CULLMODE, D3DCULL_NONE ) )
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
	BoneInstance ** bone_ptr = mesh->bones;
	for (int i = 0; i < archetype->num_bones; i++)
	{
		BoneInstance * bone = *(bone_ptr++);

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
		BoneInstance * b = mesh->bones[*id];
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
					b = mesh->bones[*id];
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
					b = mesh->bones[*id];
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

				BoneInstance *bone = mesh->bones[bone_id];

				Matrix orientation ( ENG->get_orientation(bone->instance) );

				INSTANCE_INDEX parent = MODEL->get_parent(bone->instance);
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

void DeformablePart::render(struct ICamera * camera, int mesh_index, int tessellation_cnt)
{
	if (mesh_index < 0 || mesh_index >= num_meshes)
	{
		return;
	}

	DeformablePartMesh * mesh = meshes + mesh_index;
	DeformablePartArchetype * archetype = mesh->arch;

	if (archetype->is_patch())
	{
		render_patches(camera, mesh, tessellation_cnt);
	}
	else
	{
		render_mesh(camera, mesh);
	}
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

	Transform cam2world = camera->get_transform();

	DeformablePartMesh * mesh = &meshes[0];

	BoneInstance ** bone_ptr = mesh->bones;
	for (int i = 0; i < mesh->arch->num_bones; i++)
	{
		BoneInstance * bone = *(bone_ptr++);

		const BaseExtent * extent;
		if (PHYSICS->get_extent(&extent, bone->instance))
		{
			if (extent->type == ET_SPHERE)
			{
				extent = extent->child;
				if (extent && extent->type == ET_BOX)
				{
					p = PHYSICS->get_center_of_mass(bone->instance);
					R = ENG->get_orientation(bone->instance);

					const BoxExtent * b = (const BoxExtent *) extent;
					v[0].set(+b->box.half_x, +b->box.half_y, +b->box.half_z);
					v[1].set(+b->box.half_x, +b->box.half_y, -b->box.half_z);
					v[2].set(+b->box.half_x, -b->box.half_y, -b->box.half_z);
					v[3].set(+b->box.half_x, -b->box.half_y, +b->box.half_z);
					v[4].set(-b->box.half_x, +b->box.half_y, +b->box.half_z);
					v[5].set(-b->box.half_x, +b->box.half_y, -b->box.half_z);
					v[6].set(-b->box.half_x, -b->box.half_y, -b->box.half_z);
					v[7].set(-b->box.half_x, -b->box.half_y, +b->box.half_z);

					for (int j = 0; j < 8; j++)
					{
						float x, y, z;
						Vector vx = p + R * (v[j]);

						if (camera->point_to_screen(x, y, z, cam2world, vx))
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
		DefScriptInfo * info = get_script_info(link->index);//&(scripts.list[link->index].info);
		dst->name = info->name;
		dst->duration = ANIM->get_duration(info->get_script_set(), info->name);
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
					//DefScriptInfo * info = get_script_info(link->index);//&scripts.list[link->index].info;
					//script_name = info->name;
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
	int script_index = -1;

	ScriptLink * link = active_scripts.first();
	while (link)
	{
		if (!link->ik_script)
		{
			if (ANIM->script_contains_channel(link->instance, channel_id))
			{
				script_index = link->index;
				break;
			}
		}
		link = active_scripts.next(link);
	}

	if (link)
	{
		DefScriptInfo * info = get_script_info(script_index);//&scripts.list[script_index].info;

		Vector pos = T.get_position();

		Vector hpos;
		float scale;

		if (info)
		{
			scale = info->get_scale();
		}
		else
		{
			scale = parts[0]->meshes[0].arch->scale;
		}

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
