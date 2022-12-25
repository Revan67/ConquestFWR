// MeshInstance.h
//
//
//

#ifndef __MeshInstance_h__
#define __MeshInstance_h__

#include "Mesh.h"
#include "XMesh.h"

// ................................................................................
//
// MeshInstance
//
//
//
struct MeshInstance
{
   const XMesh* mesh;         // Original mesh archetype from which object was
                             // instantiated

   XMesh * local_mesh;	// holds a unique copy of the mesh for this instance
						// note: some (nonmodifiable) arrays still just point to mesh

   //
   // Miscellaneous workspace
   // 

   // use optional Transform as a parameter to render_intance instead
   //Vector	scale_vector;
   //Vector   scale_origin;

   //
   // animated UV stuff
   //
   int				animated_txt_vertex_list_length;	// amount of memory mallocked to animated_txt_vertex_list
   int				animated_txt_vertex_cnt;			// current number of valid entries in animated_txt_vertex_list
   TexCoord *		animated_txt_vertex_list;			// list of uv's
   float			anim_head;							// elapsed time from frame 0
   float			total_time_length;					// the multiple of all channel time lengths to mod anim_head by

   int				smooth_light_pool_len;

   LightRGB *		smooth_light_pool;					// caches light values for smooth vertices
   LightRGB *		back_smooth_light_pool;				// caches light values for back side of double sided smooth vertices
   
// In some cases, Mesh pointer will be set as a property later. Need
// default constructor.

	void initialize (void);

	MeshInstance(void);
	MeshInstance(XMesh *_mesh, const U32 cont_lod_mode);

	void set_mesh (const XMesh* m, const U32 cont_lod_mode);

	const XMesh* get_mesh (void);
	const XMesh* get_unique_mesh (void);
	const GENRESULT release_unique_mesh (void);

	void free (void);

	~MeshInstance(void);

	void update_uv(float dt);
	void update_axm(float dt, ITextureLibrary *txm_lib);
	void set_lod(float fraction);
	void create_light_pools(void);
};

//

void MeshInstance::initialize (void)
{
	mesh = NULL;
	local_mesh = NULL;

	animated_txt_vertex_list = NULL;
	animated_txt_vertex_list_length = 0;
	animated_txt_vertex_cnt = 0;
	anim_head = 0.0f;

	smooth_light_pool_len = 0;
	smooth_light_pool = NULL;
	back_smooth_light_pool = NULL;
}

MeshInstance::MeshInstance(void)
{
	initialize ();
}

MeshInstance::MeshInstance(XMesh *_mesh, const U32 cont_lod_mode)
{                   
	smooth_light_pool_len = 0;
	smooth_light_pool = NULL;
	back_smooth_light_pool = NULL;

	set_mesh (_mesh, cont_lod_mode);
}

void MeshInstance::free (void)
{
	animated_txt_vertex_list_length = 0;
	delete [] animated_txt_vertex_list;
	animated_txt_vertex_list = NULL;

	if(local_mesh != NULL)
	{
		local_mesh->release_partial_mesh();
		delete local_mesh;
		local_mesh = NULL;
	}

	smooth_light_pool_len = 0;
	delete [] smooth_light_pool;
	smooth_light_pool = NULL;
	delete [] back_smooth_light_pool;
	back_smooth_light_pool = NULL;
}

MeshInstance::~MeshInstance(void)
{
	free ();
}

void MeshInstance::set_lod(float fraction)
{
	// don't update (non lod) meshes
	if(local_mesh)
	{
		local_mesh->set_lod(fraction);
	}
	else
	{
		((XMesh*)mesh)->set_lod(fraction); // used by per archetype lod
	}
}

void MeshInstance::update_axm(float dt, ITextureLibrary* txm_lib)
{
	const XMesh *themesh = mesh;

	if( local_mesh ) {
		themesh = local_mesh;
	}

	for( int mid=0; mid<themesh->material_cnt; mid++ ) {
		themesh->material_list[mid].update( dt );
	}
}

// move UV animation along
void MeshInstance::update_uv(float dt)
{
	// see if we have any UV vertices to animate
	if(animated_txt_vertex_cnt > 0)
	{
		ASSERT_FATAL(animated_txt_vertex_list);

		const XMesh *xmesh = mesh;
		
		ASSERT_FATAL(xmesh->uv_channel_list);

		anim_head += dt;
		
		// TODO: keep a separate playback head for each channel -ms
		if(anim_head >= total_time_length)
			anim_head -= total_time_length;

		for(int j=0; j<xmesh->uv_channel_cnt; j++)
		{
			UVChannel *uvc = &(xmesh->uv_channel_list[j]);
		
			// figure out which frames to interpolate between
			float fraction = anim_head * uvc->fps;
			int current_frame = (int)fraction;
			fraction -= current_frame;
			
			ASSERT_FATAL(current_frame < uvc->frame_count);

			if(!uvc->interpolate)  // movies
			{
				int last_frame = (int)((anim_head - dt) * uvc->fps);
				if(current_frame != last_frame)
				{
					int frame_offset1 = current_frame * uvc->vertex_count;
					int *lookup = uvc->vertex_lookup;
					int *uv_list_id1 = uvc->uv_chain + frame_offset1;
					for(int k=0; k<uvc->vertex_count; k++, lookup++, uv_list_id1++)
					{
						animated_txt_vertex_list[*lookup].u =
							xmesh->texture_vertex_list[*uv_list_id1].u;
						animated_txt_vertex_list[*lookup].v =
							xmesh->texture_vertex_list[*uv_list_id1].v;
					}
				}
			}
			else  // sliding textures
			{
				float one_minus_fraction = 1.0f - fraction;
				int frame_offset1 = current_frame * uvc->vertex_count;
				int frame_offset2 = ((current_frame + 1)%uvc->frame_count) * uvc->vertex_count;

				// compute interpolated value
				int *lookup = uvc->vertex_lookup;
				int *uv_list_id1 = uvc->uv_chain + frame_offset1;
				int *uv_list_id2 = uvc->uv_chain + frame_offset2;
				for(int k=0; k<uvc->vertex_count; k++, lookup++, uv_list_id1++, uv_list_id2++)
				{
					float v1 = xmesh->texture_vertex_list[*uv_list_id1].u;
					float v2 = xmesh->texture_vertex_list[*uv_list_id2].u;
					// unfortunately uv space loops back
					if(fabs(v1 - v2) > .5f)
					{
						if(v1 < v2) 
						{
							v1 += 1.0f;
						}
						else
						{
							v2 += 1.0f;
						}
					}
					animated_txt_vertex_list[*lookup].u =
						one_minus_fraction * v1 + fraction * v2;
					
					v1 = xmesh->texture_vertex_list[*uv_list_id1].v;
					v2 = xmesh->texture_vertex_list[*uv_list_id2].v;
					if(fabs(v1 - v2) > .5f)
					{
						if(v1 < v2) 
						{
							v1 += 1.0f;
						}
						else
						{
							v2 += 1.0f;
						}
					}
					animated_txt_vertex_list[*lookup].v =
						one_minus_fraction * v1 + fraction * v2;
				}
			}
		}
	}
}

//

void MeshInstance::create_light_pools(void)
{
	if( mesh )
	{
		delete [] smooth_light_pool;
		delete [] back_smooth_light_pool;
	
		smooth_light_pool_len = mesh->object_vertex_cnt;
		smooth_light_pool = new LightRGB[smooth_light_pool_len];
		back_smooth_light_pool = new LightRGB[smooth_light_pool_len];
		memset(smooth_light_pool, 0, smooth_light_pool_len * sizeof(LightRGB));
		memset(back_smooth_light_pool, 0, smooth_light_pool_len * sizeof(LightRGB));
	}
}

//

void MeshInstance::set_mesh (const XMesh* m, const U32 cont_lod_mode)
{
	if (m != mesh)
	{
		mesh = m;

		if(mesh)
		{
			// light caches
			if(smooth_light_pool_len && smooth_light_pool_len < mesh->object_vertex_cnt)
			{
				delete [] smooth_light_pool;
				delete [] back_smooth_light_pool;
			
				smooth_light_pool_len = mesh->object_vertex_cnt;
				smooth_light_pool = new LightRGB[smooth_light_pool_len];
				back_smooth_light_pool = new LightRGB[smooth_light_pool_len];
				memset(smooth_light_pool, 0, smooth_light_pool_len * sizeof(LightRGB));
				memset(back_smooth_light_pool, 0, smooth_light_pool_len * sizeof(LightRGB));
			}

			// if mesh has animated uv's we make a copy for the instance
			if(mesh->uv_channel_list)
			{
				// increase uv list size if necessary
				if(mesh->vertex_batch_cnt > animated_txt_vertex_list_length)
				{
					animated_txt_vertex_list_length = mesh->vertex_batch_cnt;
					delete [] animated_txt_vertex_list;
					animated_txt_vertex_list = new TexCoord[animated_txt_vertex_list_length];
				}

				// set current uv count
				animated_txt_vertex_cnt = mesh->vertex_batch_cnt;

				// copy all uv's (update only touches those uv's that are animated)
				int i;
				for(i=0; i < animated_txt_vertex_cnt; i++)
				{
					int uv_id = mesh->texture_batch_list[i];
					animated_txt_vertex_list[i].u = mesh->texture_vertex_list[uv_id].u;
					animated_txt_vertex_list[i].v = mesh->texture_vertex_list[uv_id].v;
				}

				// compute common denominator time for all channels
				total_time_length = 1.0f;
				for(i=0; i < mesh->uv_channel_cnt; i++)
				{
					total_time_length *= mesh->uv_channel_list[i].time_length;
				}

				anim_head = 0.0f;
			}	
			
			// if this is a continuous lod mesh we make a local copy of it's geometry
			if(mesh->lod_step_list && (cont_lod_mode == 1))
			{
				if(!local_mesh)
				{
					local_mesh = new XMesh;
				}
				else
				{	// TODO: keep array sizes and only allocate when increasing
					// fortunately continuous lod meshes should have no reason to be switched
					local_mesh->release_partial_mesh();
				}

				local_mesh->copy_lod_mesh(*mesh);
			}
			else
			{
				if(local_mesh)
				{
					local_mesh->release_partial_mesh();
					delete local_mesh;
					local_mesh = NULL;
				}
			}
		}
		else // mesh == NULL
		{
			smooth_light_pool_len = 0;
			delete [] smooth_light_pool;
			smooth_light_pool = NULL;
			delete [] back_smooth_light_pool;
			back_smooth_light_pool = NULL;

			animated_txt_vertex_list_length = 0;
			delete [] animated_txt_vertex_list;
			animated_txt_vertex_list = NULL;

			if(local_mesh)
			{
				local_mesh->release_partial_mesh();
				delete local_mesh;
				local_mesh = NULL;
			}
		}
	}
}

const XMesh* MeshInstance::get_mesh (void)
{
	if(local_mesh)
	{
		return local_mesh;
	}
	else
	{
		return mesh;
	}
}

//

const GENRESULT MeshInstance::release_unique_mesh (void)
{
	GENRESULT result = GR_GENERIC;
	if(local_mesh && (local_mesh->allow_lod_change == false)) // this prevents the app from releasing local data
	{												  // needed for lod; assuming the app does NOT know
													  // what it's doing						
		local_mesh->release_partial_mesh();
		delete local_mesh;
		local_mesh = NULL;
		
		result = GR_OK;
	}

	return result;
}

//

const XMesh* MeshInstance::get_unique_mesh (void)
{
	if(local_mesh)
	{
		return local_mesh;
	}
	else if(mesh)
	{
		local_mesh = new XMesh;
		local_mesh->copy_partial_mesh(*mesh);
		return local_mesh;
	}
	else
	{
		return NULL;
	}
}

//

#endif // EOF
