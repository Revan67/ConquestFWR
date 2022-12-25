#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#ifdef SGI
#ifndef __cdecl
#define __cdecl
#endif
#endif

extern void  __cdecl sortQ(void *, size_t, size_t, 
					 int (__cdecl *comp)(const void *, const void *, const void *),
					 const void *comp_lib,
					 void (__cdecl *swap)(void *, const int id1, const int id2),
					 void *swap_lib);

#include "3db.h"

#ifndef SGI
#pragma warning( 3 : 4100 ) // unreferenced formal parameter
#pragma warning( 3 : 4189 ) // local variable is initialized but not referenced
#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( error : 4700 )
#pragma warning( 3 : 4706 ) // assignment within conditional expression
//#pragma warning( 3 : 4705 ) // statement has no effect
//#pragma warning( 3 : 4710 ) // 'function' : function not inlined

#pragma warning( disable : 4244 ) // conversion from 'double' to 'float', possible loss of data
#pragma warning( disable : 4514 ) // unreferenced inline function has been removed
#endif

//#pragma optimize( "p", on ) // float consistency

PersistVector Vector_to_PersistVector(const Vector & v)
{
	return PersistVector(v.x, v.y, v.z);
}

PersistMatrix Matrix_to_PersistMatrix(const Matrix & m)
{
	return PersistMatrix(m.d[0][0], m.d[0][1], m.d[0][2],
						 m.d[1][0], m.d[1][1], m.d[1][2],
						 m.d[2][0], m.d[2][1], m.d[2][2]);
}

PersistTransform Transform_to_PersistTransform(const Transform & t)
{
	return PersistTransform( Matrix_to_PersistMatrix( (Matrix)t ),
							 Vector_to_PersistVector( t.translation ) );
}

Vector Transform_vector(const Transform & t, const Vector & v)
{
	return Vector(
			(t.d[0][0] * v.x) +
			(t.d[0][1] * v.y) +
			(t.d[0][2] * v.z) +
			(t.translation.x),

			(t.d[1][0] * v.x) +
			(t.d[1][1] * v.y) +
			(t.d[1][2] * v.z) +
			(t.translation.y),

			(t.d[2][0] * v.x) +
			(t.d[2][1] * v.y) +
			(t.d[2][2] * v.z) +
			(t.translation.z)
			);
}

/*
\ [0]
|---DA mesh [0]
|---|---Face groups [0]
|---|---|---Count [4]
|---|---|---Group0 [0]
|---|---|---|---Material id[4]
|---|---|---|---Face vertex chain [960] // 3 per face
|---|---Batch indices
|---|---|---XYZ indices
|---|---|---Normal indices
|---|---|---UV0 indices
|---|---|---Color indices
|---|---Geometry
|---|---|---Vertices
|---|---|---Normals
|---|---|---UV
|---|---|---Colors
|---|---Material library [0]
|---|---|---Material count [4]
|---|---|---GeoSphere01 [0]
|---|---|---|---Material identifier [4]
|---|---|---|---Diffuse [0]
|---|---|---|---|---Constant [12]
|---|---|---|---Ambient [0]
|---|---|---|---|---Constant [12]
*/

file_node* CreateDAFaceGroup(DAMeshGroup & group, int id)
{
	if(group.f_cnt <= 0)
	{
		Winprint("Error: face group w/ 0 faces! Possible cause: Too much LOD.\n");
	}

    char name[32];
    sprintf(name, "Group%d", id);

	file_node *tmp;
    file_node *node = CreateNode(name, D);

    node->child = tmp = CreateNode("Material_name", F);
	tmp->data_size = strlen(group.mtl_name) + 1;
    tmp->data = (unsigned char*)group.mtl_name;

    tmp->sibling=CreateNode("Face_indices", F);
    tmp=tmp->sibling;
    tmp->data_size=3*group.f_cnt*sizeof(*(group.f_list));
    tmp->data=(unsigned char*)group.f_list;

	tmp->sibling=CreateNode("Edge_indices", F);
    tmp=tmp->sibling;
    tmp->data_size=2*group.e_cnt*sizeof(*(group.e_list));
    tmp->data=(unsigned char*)group.e_list;

	tmp->sibling=CreateNode("Edge_angles", F);
    tmp=tmp->sibling;
    tmp->data_size=group.e_cnt*sizeof(*(group.e_angles));
    tmp->data=(unsigned char*)group.e_angles;

    return node;
}

file_node*  CreateDAFaceGroups( DAMesh & mesh )
{
	assert(mesh.grp_cnt > 0 && mesh.grp_list != NULL);

	file_node *next;
	file_node *node = CreateNode("Face_groups", D);
    node->child = next = CreateNode("Count", F);
    next->data_size = sizeof(mesh.grp_cnt);
    next->data = (unsigned char*)&(mesh.grp_cnt);

    for(int i = 0; i < mesh.grp_cnt; i++)
    {
		next->sibling = CreateDAFaceGroup(mesh.grp_list[i], i);
		next = next->sibling;	
    }

	return node;
}

file_node*  CreateDAGeometry( DAMesh & da_mesh )
{
	file_node *tmp;
	file_node *node = CreateNode("Geometry",D);

// batch indices
	tmp=node->child=CreateNode("Point_indices",F);
	tmp->data_size=da_mesh.batch_index_cnt*sizeof(*(da_mesh.vb_idx));
	tmp->data=(unsigned char*)da_mesh.vb_idx;

	tmp->sibling=CreateNode("Vertex_normal_indices",F);
	tmp=tmp->sibling;
	tmp->data_size=da_mesh.batch_index_cnt*sizeof(*(da_mesh.nb_idx));
	tmp->data=(unsigned char*)da_mesh.nb_idx;

	if( da_mesh.uvb0_idx )
	{
		tmp->sibling=CreateNode("UV0_indices",F);
		tmp=tmp->sibling;
		tmp->data_size=da_mesh.batch_index_cnt*sizeof(*(da_mesh.uvb0_idx));
		tmp->data=(unsigned char*)da_mesh.uvb0_idx;
	}

	if( da_mesh.uvb1_idx )
	{
		tmp->sibling=CreateNode("UV1_indices",F);
		tmp=tmp->sibling;
		tmp->data_size=da_mesh.batch_index_cnt*sizeof(*(da_mesh.uvb1_idx));
		tmp->data=(unsigned char*)da_mesh.uvb1_idx;
	}

	if( da_mesh.colb_idx )
	{
		tmp->sibling=CreateNode("Color0_indices",F);
		tmp=tmp->sibling;
		tmp->data_size=da_mesh.batch_index_cnt*sizeof(*(da_mesh.colb_idx));
		tmp->data=(unsigned char*)da_mesh.colb_idx;
	}

// geometry
	tmp->sibling=CreateNode("Points",F);
	tmp=tmp->sibling;
	tmp->data_size=da_mesh.v_cnt*sizeof(*(da_mesh.v_list));
	tmp->data=(unsigned char*)da_mesh.v_list;


	// deformable stuff
	if( da_mesh.bone_first_list )
	{
		tmp->sibling=CreateNode("Point_bone_first",F);
		tmp=tmp->sibling;
		tmp->data_size=da_mesh.v_cnt*sizeof(*(da_mesh.bone_first_list));
		tmp->data=(unsigned char*)da_mesh.bone_first_list;

		tmp->sibling=CreateNode("Point_bone_count",F);
		tmp=tmp->sibling;
		tmp->data_size=da_mesh.v_cnt*sizeof(*(da_mesh.bone_count_list));
		tmp->data=(unsigned char*)da_mesh.bone_count_list;

		int length = 0;
		for(int i = 0; i < da_mesh.v_cnt; i++)
		{
			length += da_mesh.bone_count_list[i];
		}

		tmp->sibling=CreateNode("Bone_id_chain",F);
		tmp=tmp->sibling;
		tmp->data_size=length*sizeof(*(da_mesh.bone_id_chain));
		tmp->data=(unsigned char*)da_mesh.bone_id_chain;

		tmp->sibling=CreateNode("Bone_weight_chain",F);
		tmp=tmp->sibling;
		tmp->data_size=length*sizeof(*(da_mesh.bone_weight_chain));
		tmp->data=(unsigned char*)da_mesh.bone_weight_chain;
	}

	tmp->sibling=CreateNode("Vertex_normals",F);
	tmp=tmp->sibling;
	tmp->data_size=da_mesh.vn_cnt*sizeof(*(da_mesh.vn_list));
	tmp->data=(unsigned char*)da_mesh.vn_list;

	if( da_mesh.uv0_list )
	{
		tmp->sibling=CreateNode("UV0",F);
		tmp=tmp->sibling;
		tmp->data_size=da_mesh.uv0_cnt*sizeof(*(da_mesh.uv0_list));
		tmp->data=(unsigned char*)da_mesh.uv0_list;
	}

	if( da_mesh.uv1_list )
	{
		tmp->sibling=CreateNode("UV1",F);
		tmp=tmp->sibling;
		tmp->data_size=da_mesh.uv1_cnt*sizeof(*(da_mesh.uv1_list));
		tmp->data=(unsigned char*)da_mesh.uv1_list;
	}

	if( da_mesh.color_list )
	{
		tmp->sibling=CreateNode("Color0",F);
		tmp=tmp->sibling;
		tmp->data_size=da_mesh.color_cnt*sizeof(*(da_mesh.color_list));
		tmp->data=(unsigned char*)da_mesh.color_list;
	}

	return node;
}

/* set up a test dir structure in memory to write out */
file_node* CreateObject(object *obj, int txt_flag)
{
    assert(obj->type != INVALID);

#if USE_DA_MESH
	if(obj->type == FIXED_MESH || obj->type == DEF_MESH)
	{
		DAMesh & da_mesh = obj->da_mesh;

		da_mesh.SyncEdges();

		if(da_mesh.grp_cnt <= 0 || da_mesh.v_cnt <= 0)
		{
			Winprint("Error: CreateObject() called on an empty object!\n");
			return NULL;
		}

		file_node *da_m_node = CreateNode("Mesh", D);
		file_node **next = &(da_m_node->child);

		*next = CreateDAFaceGroups( da_mesh );
		next = &((*next)->sibling);

		*next = CreateDAGeometry( da_mesh );
		next = &((*next)->sibling);

		if( obj->type == FIXED_MESH )
		{
			if(da_mesh.lol.count > 3  || da_mesh.face_cnt < 10) // only export if it's worth it
			{
				*next = CreateDALodLib(&(da_mesh.lol));
				next = &((*next)->sibling);
			}
			else
			if( da_mesh.lol.count > 0 )
			{
				fprintf(stderr, "%d is not enough LOD steps to bother.\n", da_mesh.lol.count);
			}
		}

		return da_m_node;
	}
	else
#endif
	if(obj->type == FIXED_NURB || obj->type == DEF_NURB)
	{
		file_node *node = CreateNode("NURB object", D);
		file_node **tmp = &(node->child);

		if(obj->nurb_count > 0)
		{
			*tmp = CreateNurbSet(obj);
			tmp = &((*tmp)->sibling);
		}
		else
		{
			Winprint("Error: object w/ %d nurbs!\n", obj->nurb_count);
		}

#if 0//!USE_DA_MESH not yet
		if(obj->ml->cq2Count > 0)
		{
			*tmp=CreateMtlLib(obj->ml);
			tmp=&((*tmp)->sibling);

			if(txt_flag && (obj->tl->count > 0))
			{
			  *tmp=CreateTxtLib(obj->tl, NULL, NULL);
			  tmp=&((*tmp)->sibling);
			}

			if(txt_flag && obj->atl->count > 0)
			{
			  *tmp=CreateAnimTxtLib(obj->atl);
			  tmp=&((*tmp)->sibling);
			}
		}
#endif

		if(obj->extents.sphere.radius > 0.0f)
		{
		  *tmp=CreateRenderSphere(obj->extents);
		  tmp=&((*tmp)->sibling);
		}

		return node;
	}else
	if(obj->type == FIXED_PATCH || obj->type == DEF_PATCH)
	{
		SynchPatchMesh( &(obj->b_mesh) );
		obj->b_mesh.CalcEdges();

		file_node *node = CreateNode("Bezier Patch object", D);
		file_node **tmp = &(node->child);

		if(obj->b_mesh.patch_cnt > 0)
		{
			*tmp = CreateNode("Patch count", F);
			(*tmp)->data_size = sizeof(obj->b_mesh.patch_cnt);
			(*tmp)->data = (unsigned char*)&(obj->b_mesh.patch_cnt);
			tmp = &((*tmp)->sibling);

			*tmp = CreatePatchGroups(&(obj->b_mesh));
			tmp = &((*tmp)->sibling);

			*tmp = CreatePatchVertices(&(obj->b_mesh)); // includes vectors
			tmp = &((*tmp)->sibling);

			*tmp = CreatePatchEdges(&(obj->b_mesh));
			tmp = &((*tmp)->sibling); 
		}
		else
		{
			Winprint("Error: object w/ %d patches!\n", obj->b_mesh.patch_cnt);
		}

		if(obj->ml->cq2Count >0)
		{
			*tmp=CreateMtlLib(obj->ml);
			tmp=&((*tmp)->sibling);

			/*if(txt_flag && (obj->tl->count > 0))
			{
			  *tmp=CreateTxtLib(obj->tl, NULL, NULL);
			  tmp=&((*tmp)->sibling);
			}*/

			/*if(txt_flag && obj->atl->count > 0)
			{
			  *tmp=CreateAnimTxtLib(obj->atl);
			  tmp=&((*tmp)->sibling);
			}*/
		}

		if(obj->extents.sphere.radius > 0.0f)
		{
		  *tmp=CreateRenderSphere(obj->extents);
		  tmp=&((*tmp)->sibling);
		}

		return node;
	}
	else
    if(obj->type == FIXED_MESH || obj->type == DEF_MESH)
    {
      if(obj->face_count <= 0 || obj->v.object_count <= 0)
	  {
		  Winprint("Error: CreateObject() called on an empty object!\n");
		  return NULL;
	  }

      SynchEdges(obj);

      /*
      |---openFLAME 3D N-mesh [0]
      */
      file_node *of_3d_node;
      file_node **tmp;

      of_3d_node=CreateNode(openFLAME_3D_N_mesh,D);
      tmp=&(of_3d_node->child);

	  if(obj->face_group_count > 0)
	  {
		*tmp=CreateFaceGroups(obj);
		tmp=&((*tmp)->sibling);
	  }
	  else
	  {
		  Winprint("Error: mesh w/ %d face groups!\n", obj->face_group_count);
	  }

      if(obj->n.count>0){
        *tmp=CreateNormals(&(obj->n));
        tmp=&((*tmp)->sibling);
      }
      if(obj->v.object_count>0){
        *tmp=CreateVertices(&(obj->v));
        tmp=&((*tmp)->sibling);
      }
      if(obj->e.count>0){
        *tmp=CreateEdges(&(obj->e));
        tmp=&((*tmp)->sibling);
      }

      if(obj->uvcl.count > 0){
        *tmp=CreateUVChannelLib(&(obj->uvcl));
        tmp=&((*tmp)->sibling);
      }

	  if(obj->lol.count > 3){  // only export if it's worth it
        *tmp=CreateLodLib(&(obj->lol));
        tmp=&((*tmp)->sibling);
      }

      if(obj->ml->cq2Count >0)
	  {
        *tmp=CreateMtlLib(obj->ml);
        tmp=&((*tmp)->sibling);

        /*if(txt_flag && (obj->tl->count > 0))
		{
          *tmp=CreateTxtLib(obj->tl, NULL, NULL);
          tmp=&((*tmp)->sibling);
        }*/

		/*if(txt_flag && (obj->atl->count > 0))
		{
		  *tmp=CreateAnimTxtLib(obj->atl);
		  tmp=&((*tmp)->sibling);
		}*/
      }

	  if(obj->extents.sphere.radius > 0.0f)
	  {
		*tmp=CreateRenderSphere(obj->extents);
		tmp=&((*tmp)->sibling);
	  }

      // has to be done explicitly because of LOD objects
      // of_3d_node->sibling=CreateRigidBody(&(obj->extents));
      if(verbose_level>=3){
        printf("done w/ CreateObject\n");
      }

      return of_3d_node;
    }
    else
    if(obj->type == LIGHT)
	{
        file_node *node;
        file_node *tmp;

        node = CreateNode("Light", D);

        node->child = CreateNode("Color", F);
        tmp = node->child;
        tmp->data_size = 3 * sizeof(float);
        tmp->data = (unsigned char*)(obj->l.color);

        tmp->sibling = CreateNode("Direction", F);
        tmp = tmp->sibling;
        tmp->data_size = 3 * sizeof(float);
        tmp->data = (unsigned char*)(obj->l.direction);

        tmp->sibling = CreateNode("Range", F);
        tmp = tmp->sibling;
        tmp->data_size = sizeof(float);
        tmp->data = (unsigned char*)&(obj->l.range);

        tmp->sibling = CreateNode("Cutoff", F);
        tmp = tmp->sibling;
        tmp->data_size = sizeof(float);
        tmp->data = (unsigned char*)&(obj->l.cutoff);

		tmp->sibling = CreateNode("Hotspot", F);
        tmp = tmp->sibling;
        tmp->data_size = sizeof(float);
        tmp->data = (unsigned char*)&(obj->l.hotspot);

		tmp->sibling = CreateNode("Parallel", F);
        tmp = tmp->sibling;
        tmp->data_size = sizeof(float);
        tmp->data = (unsigned char*)&(obj->l.parallel);

		if(obj->l.texture_name)
		{
			tmp->sibling = CreateNode("Texture Name", F);
			tmp = tmp->sibling;
			tmp->data_size = strlen(obj->l.texture_name) + 1;
			tmp->data = (unsigned char*)(obj->l.texture_name);
		}

        return node;
    }
    else
    if(obj->type == CAMERA)
	{
        file_node *node;
        file_node *tmp;

        node = CreateNode("Camera", D);

        node->child = CreateNode("Fovx", F);
        tmp = node->child;
        tmp->data_size = sizeof(float);
        tmp->data = (unsigned char*)&(obj->c.fovx);

        tmp->sibling = CreateNode("Fovy", F);
        tmp = tmp->sibling;
        tmp->data_size = sizeof(float);
        tmp->data = (unsigned char*)&(obj->c.fovy);

        tmp->sibling = CreateNode("Znear", F);
        tmp = tmp->sibling;
        tmp->data_size = sizeof(float);
        tmp->data = (unsigned char*)&(obj->c.znear);

        tmp->sibling = CreateNode("Zfar", F);
        tmp = tmp->sibling;
        tmp->data_size = sizeof(float);
        tmp->data = (unsigned char*)&(obj->c.zfar);

        return node;
    }
    
    return NULL;
}

void SynchPatchMesh(Bezier_mesh *bm)
{
	const int vector_offset = bm->vertex_cnt;
	const int aux_offset = bm->vertex_cnt + bm->vector_cnt;

	if(bm->vertex_bone_count_ver == NULL) // fixed
	{
		bm->final_vertex_count = bm->vertex_cnt + bm->vector_cnt + bm->aux_cnt;

		bm->final_vertices = (Vector*)Malloc(bm->final_vertex_count * sizeof(Vector));
		memcpy(bm->final_vertices, bm->vertices, bm->vertex_cnt * sizeof(Vector));
		memcpy(bm->final_vertices + vector_offset, bm->vectors, bm->vector_cnt * sizeof(Vector));
		if(bm->aux_cnt > 0)
		{
			memcpy(bm->final_vertices + aux_offset, bm->aux, bm->aux_cnt * sizeof(Vector));
		}
	}
	else // deformable
	{
		bm->final_vertex_count = bm->vertex_cnt + bm->vector_cnt;// + bm->aux_cnt;

		bm->final_vertices = (Vector*)Malloc(bm->final_vertex_count * sizeof(Vector));
		memcpy(bm->final_vertices, bm->vertices, bm->vertex_cnt * sizeof(Vector));
		memcpy(bm->final_vertices + vector_offset, bm->vectors, bm->vector_cnt * sizeof(Vector));
		/*
		if(bm->aux_cnt > 0)
		{
			memcpy(bm->final_vertices + aux_offset, bm->aux, bm->aux_cnt * sizeof(Vector));
		}
		*/

		// count
		bm->final_vertex_bone_count = (int*)Malloc(bm->final_vertex_count * sizeof(int));
		memcpy(bm->final_vertex_bone_count, bm->vertex_bone_count_ver, bm->vertex_cnt * sizeof(int));
		memcpy(bm->final_vertex_bone_count + vector_offset, bm->vertex_bone_count_vec, bm->vector_cnt * sizeof(int));
		
		/*
		if(bm->aux_cnt > 0)
		{
			memcpy(bm->final_vertex_bone_count + aux_offset, bm->vertex_bone_count_aux,
				bm->aux_cnt * sizeof(int));
		}
		*/

		// first
		bm->final_first_vertex = (int*)Malloc(bm->final_vertex_count * sizeof(int));
		memcpy(bm->final_first_vertex, bm->first_vertex_ver, bm->vertex_cnt * sizeof(int));
		for(int i = 0; i < bm->vector_cnt; i++)
		{
			bm->final_first_vertex[vector_offset + i] =
				bm->first_vertex_vec[i] + vector_offset;
		}
		/*
		for( i = 0; i < bm->aux_cnt; i++)
		{
			bm->final_first_vertex[aux_offset + i] =
				bm->first_vertex_aux[i] + aux_offset;
		}
		*/

		// figure out lengths
		bm->final_vertex_length = 0;
		for( i = 0; i < bm->final_vertex_count; i++)
		{
			bm->final_vertex_length += bm->final_vertex_bone_count[i];
		}

		int vertex_length = 0;
		for( i = 0; i < bm->vertex_cnt; i++)
		{
			vertex_length += bm->vertex_bone_count_ver[i];
		}
		int vector_length_offset = vertex_length;

		int vector_length = 0;
		for( i = 0; i < bm->vector_cnt; i++)
		{
			vector_length += bm->vertex_bone_count_vec[i];
		}
		/*
		int aux_length_offset = vertex_length + vector_length;

		int aux_length = 0;
		for( i = 0; i < bm->aux_cnt; i++)
		{
			aux_length += bm->vertex_bone_count_aux[i];
		}
		*/

		// bone id's
		bm->final_bone_id_list = (int*)Malloc(bm->final_vertex_length * sizeof(int));
		memcpy(bm->final_bone_id_list, bm->bone_id_list_ver, vertex_length * sizeof(int));
		memcpy(bm->final_bone_id_list + vector_length_offset, bm->bone_id_list_vec, vector_length * sizeof(int));
		/*
		if(bm->aux_cnt > 0)
		{
			memcpy(bm->final_bone_id_list + aux_length_offset, bm->bone_id_list_aux, aux_length * sizeof(int));
		}
		*/

		// weights
		bm->final_weight_list = (float*)Malloc(bm->final_vertex_length * sizeof(float));
		memcpy(bm->final_weight_list, bm->bone_weight_list_ver, vertex_length * sizeof(float));
		memcpy(bm->final_weight_list + vector_length_offset, bm->bone_weight_list_vec, vector_length * sizeof(float));
		/*
		if(bm->aux_cnt > 0)
		{
			memcpy(bm->final_weight_list + aux_length_offset, bm->bone_weight_list_aux, aux_length * sizeof(float));
		}
		*/

		// points
		bm->final_vertex_list = (Vector*)Malloc(bm->final_vertex_length * sizeof(Vector));
		memcpy(bm->final_vertex_list, bm->bone_vertex_list_ver, vertex_length * sizeof(Vector));
		memcpy(bm->final_vertex_list + vector_length_offset, bm->bone_vertex_list_vec, vector_length * sizeof(Vector));
		/*
		if(bm->aux_cnt > 0)
		{
			memcpy(bm->final_vertex_list + aux_length_offset, bm->bone_vertex_list_aux, aux_length * sizeof(Vector));
		}
		*/
	}

	for(int gid = 0; gid < bm->group_cnt; gid++)
	{
		Bezier_patch_group *pg = bm->groups + gid;

		for(int pid = 0; pid < pg->patch_cnt; pid++)
		{
			Bezier_patch *bp = pg->patch_list + pid;

			if(bp->type == 4)
			{
				for(int i = 0; i < 8; i++)
				{
					bp->vec[i] += vector_offset;
				}

				for(i = 0; i < 4; i++)
				{
					bp->interior[i] += vector_offset;
				}
			}
			else
			{
				for(int i = 0; i < 6; i++)
				{
					bp->vec[i] += vector_offset;
				}

				for(i = 0; i < 3; i++)
				{
					bp->interior[i] += vector_offset;
				}

				bp->aux_index += aux_offset;
			}
		}
	}
}

file_node* CreateRenderSphere(const Extents & ext)
{
  file_node *node;
  file_node *tmp;

  node = CreateNode("Sphere", D);
  
  node->child = tmp = CreateNode("Center", F);
  tmp->data_size = sizeof(ext.sphere.render_center);
  tmp->data = (unsigned char*)&(ext.sphere.render_center);

  tmp->sibling = CreateNode("Radius", F);
  tmp = tmp->sibling;
  tmp->data_size = sizeof(ext.sphere.render_radius);
  tmp->data = (unsigned char*)&(ext.sphere.render_radius);

  return node;
}

file_node* CreateNormals(normals *n)
{
/*
|---|---Normals [0]
|---|---|---Surface normal count [4]
|---|---|---Surface normal list [552]
*/
  file_node *node, *tmp;

  node=CreateNode(Normals,D);

  tmp=node->child=CreateNode(Surface_normal_count,F);
  tmp->data_size=sizeof(n->count);
  tmp->data=(unsigned char*)&(n->count);

  tmp->sibling=CreateNode(Surface_normal_list,F);
  tmp=tmp->sibling;
  tmp->data_size=n->count*3*sizeof(*(n->list));
  tmp->data=(unsigned char*)n->list;

  if(verbose_level>=3){
    printf("done w/ CreateNormals\n");
  }
  return node;
}

void SynchFreeBoneVertices(const int v_count, bone_vertex **bv_list, int **first_list, int **count_list,
							int **id_list, float **xyz_list, float **normal_list, float **weight_list,
							float **rational_list)
{
	if(v_count > 0)
	{
		assert( *bv_list );
		assert( !*count_list );
		*count_list = (int*)Malloc(v_count * sizeof(int));
		*first_list = (int*)Malloc(v_count * sizeof(int));

		int count = 0;
		for(int vid = 0; vid < v_count; vid++)
		{
			(*first_list)[vid] = count;
			(*count_list)[vid] = (*bv_list)[vid].bone_count;
			count += (*bv_list)[vid].bone_count;
		}

		*id_list = (int*)Malloc(count * sizeof(int));
		*weight_list = (float*)Malloc(count * sizeof(float));

		if(xyz_list && (*bv_list)[0].xyz_list)
		{
			*xyz_list = (float*)Malloc(count * 3 * sizeof(float));
		}		
		if(normal_list && (*bv_list)[0].normal_list)
		{
			*normal_list = (float*)Malloc(count * 3 * sizeof(float));
		}
		if(rational_list && (*bv_list)[0].rational_list) // for nurbs
		{
			*rational_list = (float*)Malloc(count * sizeof(float));
		}

		for(vid = 0; vid < v_count; vid++)
		{
			bone_vertex *bv = (*bv_list) + vid;

			const int fv = (*first_list)[vid];
			for(int bid = 0; bid < (*count_list)[vid]; bid++)
			{
				(*id_list)[fv + bid] = bv->bone_id_list[bid];

				(*weight_list)[fv + bid] = bv->weight_list[bid];

				if(xyz_list)
				{
					(*xyz_list)[3*(fv + bid)]   = bv->xyz_list[3*bid];
					(*xyz_list)[3*(fv + bid)+1] = bv->xyz_list[3*bid+1];
					(*xyz_list)[3*(fv + bid)+2] = bv->xyz_list[3*bid+2];
				}

				if(normal_list)
				{
					(*normal_list)[3*(fv + bid)]   = bv->normal_list[3*bid];
					(*normal_list)[3*(fv + bid)+1] = bv->normal_list[3*bid+1];
					(*normal_list)[3*(fv + bid)+2] = bv->normal_list[3*bid+2];
				}

				if(rational_list)
				{
					(*rational_list)[fv + bid] = bv->rational_list[bid];
				}
			}

			bv->Release();
		}

		Free(*bv_list);
	}
}


void MergeBoneVertex(bone_vertex & dst, const bone_vertex & src1, const bone_vertex & src2,
					 const float w1, const float w2)
{
	// copy first one
	dst.bone_count = src1.bone_count;

	dst.bone_id_list = (int*)Malloc(dst.bone_count * sizeof(int));
	memcpy(dst.bone_id_list, src1.bone_id_list, dst.bone_count * sizeof(int));

	dst.xyz_list = (float*)Malloc(dst.bone_count * 3 * sizeof(float));
	memcpy(dst.xyz_list, src1.xyz_list, dst.bone_count * 3 * sizeof(float));

	dst.weight_list = (float*)Malloc(dst.bone_count * sizeof(float));
	for(int i = 0; i < dst.bone_count; i++)
	{
		dst.weight_list[i] = w1 * src1.weight_list[i];
	}

	if(src1.normal_list)
	{
		dst.normal_list = (float*)Malloc(dst.bone_count * 3 * sizeof(float));
		memcpy(dst.normal_list, src1.normal_list, dst.bone_count * 3 * sizeof(float));
	}

	if(src1.rational_list)
	{
		dst.rational_list = (float*)Malloc(dst.bone_count * sizeof(float));
		memcpy(dst.rational_list, src1.rational_list, dst.bone_count * sizeof(float));
	}

	// add unique bones from second
	for( i = 0; i < src2.bone_count; i++)
	{
		bool duplicate = false;
		for(int j = 0; j < dst.bone_count; j++)
		{
			if( dst.bone_id_list[j] == src2.bone_id_list[i]) // merge
			{
				duplicate = true;

				dst.xyz_list[3*j  ] = .5f * (dst.xyz_list[3*j] + src2.xyz_list[3*i]);
				dst.xyz_list[3*j+1] = .5f * (dst.xyz_list[3*j+1] + src2.xyz_list[3*i+1]);
				dst.xyz_list[3*j+2] = .5f * (dst.xyz_list[3*j+2] + src2.xyz_list[3*i+2]);

				dst.weight_list[j] += w2 * src2.weight_list[i];

				if(dst.normal_list)
				{
					Winprint("Error: MergeBoneVertex() not supported yet!\n");
				}
				if(dst.rational_list)
				{
					Winprint("Error: MergeBoneVertex() not supported yet!\n");
				}

				break;
			}
		}

		// append
		if(!duplicate)
		{
			int old_count = dst.bone_count;
			dst.bone_count++;

			dst.bone_id_list = (int*)Realloc(dst.bone_id_list, dst.bone_count * sizeof(int));
			dst.bone_id_list[old_count] = src2.bone_id_list[i];

			dst.xyz_list = (float*)Realloc(dst.xyz_list, dst.bone_count * 3 * sizeof(float));
			dst.xyz_list[3*old_count  ] = src2.xyz_list[3 * i];
			dst.xyz_list[3*old_count+1] = src2.xyz_list[3 * i+1];
			dst.xyz_list[3*old_count+2] = src2.xyz_list[3 * i+2];

			dst.weight_list = (float*)Realloc(dst.weight_list, dst.bone_count * sizeof(float));
			dst.weight_list[old_count] = w2 * src2.weight_list[i];


			if(dst.normal_list)
			{
				Winprint("Error: MergeBoneVertex() not supported yet!\n");
			}
			if(dst.rational_list)
			{
				Winprint("Error: MergeBoneVertex() not supported yet!\n");
			}
		}
	}

	// normalize weights
	float sum = 0.0f;
	for( i = 0; i < dst.bone_count; i++)
	{
		sum += dst.weight_list[i];
	}
	assert(sum > 0.0f);

	sum = 1.0f / sum;
	for( i = 0; i < dst.bone_count; i++)
	{
		dst.weight_list[i] *= sum;
	}
}


void SynchFreeBoneBezCV(Bezier_mesh *b_mesh)
{
	SynchFreeBoneVertices(
		b_mesh->vertex_cnt,
		&(b_mesh->b_v_list_ver),
		&(b_mesh->first_vertex_ver),
		&(b_mesh->vertex_bone_count_ver),
		&(b_mesh->bone_id_list_ver),
		(float**)&(b_mesh->bone_vertex_list_ver),
		NULL, // normals
		&(b_mesh->bone_weight_list_ver),
		NULL); // rationals

	SynchFreeBoneVertices(
		b_mesh->vector_cnt,
		&(b_mesh->b_v_list_vec),
		&(b_mesh->first_vertex_vec),
		&(b_mesh->vertex_bone_count_vec),
		&(b_mesh->bone_id_list_vec),
		(float**)&(b_mesh->bone_vertex_list_vec),
		NULL, // normals
		&(b_mesh->bone_weight_list_vec),
		NULL); // rationals

	/*
	SynchFreeBoneVertices(
		b_mesh->aux_cnt,
		&(b_mesh->b_v_list_aux),
		&(b_mesh->first_vertex_aux),
		&(b_mesh->vertex_bone_count_aux),
		&(b_mesh->bone_id_list_aux),
		(float**)&(b_mesh->bone_vertex_list_aux),
		NULL, // normals
		&(b_mesh->bone_weight_list_aux),
		NULL); // rationals
	*/
}

void SynchFreeBoneNurbCV(object *obj)
{
	assert(obj->type == DEF_NURB);

	for(int nid = 0; nid < obj->nurb_count; nid++)
	{
		nurb * nrb = obj->nurb_list + nid;

		SynchFreeBoneVertices(
			nrb->s_point_count * nrb->t_point_count,
			&(nrb->b_v_list),
			&(nrb->first_vertex),
			&(nrb->vertex_bone_count),
			&(nrb->bone_id_list),
			&(nrb->bone_vertex_list),
			NULL, // normals
			&(nrb->bone_weight_list),
			&(nrb->bone_rational_list));
	}
}

void SynchFreeBoneMeshVertices(vertices *v)
{
	assert(v->b_v_list);

	SynchFreeBoneVertices(
		v->object_count,
		&(v->b_v_list),
		&(v->first_vertex),
		&(v->vertex_bone_count),
		&(v->bone_id_list),
		&(v->bone_vertex_list),
		&(v->bone_normal_list),
		&(v->bone_weight_list),
		NULL); // rationals
}

void SynchFreeBoneMeshVertices(DAMesh & da_mesh)
{
	assert(da_mesh.b_v_list);

	SynchFreeBoneVertices(
		da_mesh.v_cnt,
		&(da_mesh.b_v_list),
		&(da_mesh.bone_first_list),
		&(da_mesh.bone_count_list),
		&(da_mesh.bone_id_chain),
		NULL,//xyz
		NULL,//normal
		&(da_mesh.bone_weight_chain),
		NULL); // rationals
}

file_node* CreatePatchVertices(Bezier_mesh *bm)
{
	file_node *node;
	file_node *tmp;
	
	node = CreateNode("Geometry", D);

	tmp = node->child = CreateNode("Vertex count", F);
	tmp->data_size = sizeof(bm->final_vertex_count);
	tmp->data = (unsigned char*)&(bm->final_vertex_count);

	if(bm->final_vertex_bone_count != NULL) // deformable
	{
		tmp->sibling = CreateNode("Vertex bone count", F);
		tmp = tmp->sibling;
		tmp->data_size = bm->final_vertex_count * sizeof(*(bm->final_vertex_bone_count));
		tmp->data = (unsigned char*)bm->final_vertex_bone_count;

		tmp->sibling = CreateNode("Vertex bone first", F);
		tmp = tmp->sibling;
		tmp->data_size = bm->final_vertex_count * sizeof(*(bm->final_first_vertex));
		tmp->data = (unsigned char*)bm->final_first_vertex;

	
		tmp->sibling = CreateNode("Vertex bone id", F);
		tmp = tmp->sibling;
		tmp->data_size = bm->final_vertex_length * sizeof(*(bm->final_bone_id_list));
		tmp->data = (unsigned char*)bm->final_bone_id_list;

		tmp->sibling = CreateNode("Vertex bone weight", F);
		tmp = tmp->sibling;
		tmp->data_size = bm->final_vertex_length * sizeof(*(bm->final_weight_list));
		tmp->data = (unsigned char*)bm->final_weight_list;

		tmp->sibling = CreateNode("Vertex bone point", F);
		tmp = tmp->sibling;
		tmp->data_size = bm->final_vertex_length * sizeof(*(bm->final_vertex_list));
		tmp->data = (unsigned char*)bm->final_vertex_list;
	}
#ifndef _DEBUG
	else // fixed
#endif
	{
		tmp->sibling = CreateNode("Vertex list", F);
		tmp = tmp->sibling;
		tmp->data_size = bm->final_vertex_count * sizeof(*(bm->final_vertices));
		tmp->data = (unsigned char*)bm->final_vertices;
	}

	tmp->sibling = CreateNode("UV count", F);
	tmp = tmp->sibling;
	tmp->data_size = sizeof(bm->uv_cnt);
	tmp->data = (unsigned char*)&(bm->uv_cnt);

	tmp->sibling = CreateNode("UV list", F);
	tmp = tmp->sibling;
	tmp->data_size = bm->uv_cnt * sizeof(*(bm->uvs));
	tmp->data = (unsigned char*)bm->uvs;

	return node;
}

file_node* CreatePatchEdges(Bezier_mesh *bm)
{
	if( bm->edge_cnt <= 0 )
		return NULL;

	file_node *node;
	file_node *tmp;
	
	node = CreateNode("Edges", D);

	tmp = node->child = CreateNode("Edge count", F);
	tmp->data_size = sizeof(bm->edge_cnt);
	tmp->data = (unsigned char*)&(bm->edge_cnt);

	tmp->sibling = CreateNode("Edge list", F);
	tmp = tmp->sibling;
	tmp->data_size = bm->edge_cnt * sizeof(*(bm->edges));
	tmp->data = (unsigned char*)bm->edges;
	
	return node;
}

file_node* CreateVertices(vertices *v)
{
/*
|---|---Vertices [0]
|---|---|---Vertex reference count [4]
|---|---|---Object vertex chain [588]
|---|---|---Texture vertex chain [588]
|---|---|---Object vertex count [4]
|---|---|---Object vertex list [336]
|---|---|---Texture vertex count [4]
|---|---|---Texture vertex list [320]
*/

  file_node *node, *tmp;

  node=CreateNode(Vertices,D);

	  // new batching stuff
	  tmp=node->child=CreateNode("Vertex batch count",F);
	  tmp->data_size=sizeof(v->batch_count);
	  tmp->data=(unsigned char*)&(v->batch_count);

	  tmp->sibling=CreateNode("Vertex batch list",F);
	  tmp=tmp->sibling;
	  tmp->data_size=v->batch_count*sizeof(*(v->object_batch_list));
	  tmp->data=(unsigned char*)v->object_batch_list;

	  tmp->sibling=CreateNode("Texture batch list",F);
	  tmp=tmp->sibling;
	  tmp->data_size=v->batch_count*sizeof(*(v->texture_batch_list));
	  tmp->data=(unsigned char*)v->texture_batch_list;

	  if(v->texture_batch_list2)
	  {
		  tmp->sibling=CreateNode("Texture batch list2",F);
		  tmp=tmp->sibling;
		  tmp->data_size=v->batch_count*sizeof(*(v->texture_batch_list2));
		  tmp->data=(unsigned char*)v->texture_batch_list2;
	  }

	  /*
	  tmp->sibling=CreateNode("Material batch list",F);
	  tmp=tmp->sibling;
	  tmp->data_size=v->batch_count*sizeof(*(v->mtl_batch_list));
	  tmp->data=(unsigned char*)v->mtl_batch_list;
	  */

  tmp->sibling=CreateNode(Object_vertex_count,F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(v->object_count);
  tmp->data=(unsigned char*)&(v->object_count);

#ifndef _DEBUG
  if(v->vertex_bone_count == NULL)
#endif
  {
	  tmp->sibling=CreateNode(Object_vertex_list,F);
	  tmp=tmp->sibling;
	  tmp->data_size=3*v->object_count*sizeof(*(v->object_list));
	  tmp->data=(unsigned char*)v->object_list;

	  tmp->sibling=CreateNode(Vertex_normal,F);
	  tmp=tmp->sibling;
	  tmp->data_size=v->object_count*sizeof(*(v->normal));
	  tmp->data=(unsigned char*)v->normal; 
 
	  tmp->sibling=CreateNode(Vertex_D_coefficient,F);
	  tmp=tmp->sibling;
	  tmp->data_size=v->object_count*sizeof(*(v->D_coefficient));
	  tmp->data=(unsigned char*)v->D_coefficient;
  }

  if(v->color)
  {
    tmp->sibling=CreateNode("Color",F);
    tmp=tmp->sibling;
    tmp->data_size=3*v->object_count*sizeof(*(v->color));
    tmp->data=(unsigned char*)v->color;
  }
  
  tmp->sibling=CreateNode(Texture_vertex_count,F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(v->texture_count);
  tmp->data=(unsigned char*)&(v->texture_count);

  tmp->sibling=CreateNode(Texture_vertex_list,F);
  tmp=tmp->sibling;
  tmp->data_size=2*v->texture_count*sizeof(*(v->texture_list));
  tmp->data=(unsigned char*)v->texture_list;

  if(v->vertex_bone_count != NULL)
  {
      tmp->sibling=CreateNode(Vertex_bone_count, F);
      tmp=tmp->sibling;
      tmp->data_size=v->object_count*sizeof(*(v->vertex_bone_count));
      tmp->data=(unsigned char*)v->vertex_bone_count;

	  tmp->sibling=CreateNode("First vertex", F);
      tmp=tmp->sibling;
      tmp->data_size=v->object_count*sizeof(*(v->first_vertex));
      tmp->data=(unsigned char*)v->first_vertex;

    int count=0;
    for(int i=0; i<v->object_count; i++)
	{
		assert(count == v->first_vertex[i]);
        count += v->vertex_bone_count[i];
    }

    tmp->sibling=CreateNode(Bone_id_list, F);
    tmp=tmp->sibling;
    tmp->data_size=count*sizeof(*(v->bone_id_list));
    tmp->data=(unsigned char*)v->bone_id_list;

    tmp->sibling=CreateNode(Bone_weight_list, F);
    tmp=tmp->sibling;
    tmp->data_size=count*sizeof(*(v->bone_weight_list));
    tmp->data=(unsigned char*)v->bone_weight_list;

    tmp->sibling=CreateNode(Bone_vertex_list, F);
    tmp=tmp->sibling;
    tmp->data_size=3*count*sizeof(*(v->bone_vertex_list));
    tmp->data=(unsigned char*)v->bone_vertex_list;

    tmp->sibling=CreateNode(Bone_normal_list, F);
    tmp=tmp->sibling;
    tmp->data_size=3*count*sizeof(*(v->bone_normal_list));
    tmp->data=(unsigned char*)v->bone_normal_list;
  }

  if(v->uv_bone_count > 0)
  {
    tmp->sibling=CreateNode("UV bone count", F);
    tmp=tmp->sibling;
    tmp->data_size=sizeof(v->uv_bone_count);
    tmp->data=(unsigned char*)&(v->uv_bone_count);

	tmp->sibling=CreateNode("UV bone id", F);
    tmp=tmp->sibling;
    tmp->data_size=v->uv_bone_count*sizeof(*(v->uv_bone_id));
    tmp->data=(unsigned char*)v->uv_bone_id;

	tmp->sibling=CreateNode("UV vertex count", F);
    tmp=tmp->sibling;
    tmp->data_size=v->uv_bone_count*sizeof(*(v->uv_vertex_count));
    tmp->data=(unsigned char*)v->uv_vertex_count;

	tmp->sibling=CreateNode("UV plane distance", F);
    tmp=tmp->sibling;
    tmp->data_size=v->uv_bone_count*sizeof(*(v->uv_plane_distance));
    tmp->data=(unsigned char*)v->uv_plane_distance;

	tmp->sibling=CreateNode("Bone X to U scale", F);
    tmp=tmp->sibling;
    tmp->data_size=v->uv_bone_count*sizeof(*(v->x_to_u_scale));
    tmp->data=(unsigned char*)v->x_to_u_scale;

	tmp->sibling=CreateNode("Bone Y to V scale", F);
    tmp=tmp->sibling;
    tmp->data_size=v->uv_bone_count*sizeof(*(v->y_to_v_scale));
    tmp->data=(unsigned char*)v->y_to_v_scale;

	tmp->sibling=CreateNode("Min du", F);
    tmp=tmp->sibling;
    tmp->data_size=v->uv_bone_count*sizeof(*(v->min_du));
    tmp->data=(unsigned char*)v->min_du;

	tmp->sibling=CreateNode("Max du", F);
    tmp=tmp->sibling;
    tmp->data_size=v->uv_bone_count*sizeof(*(v->max_du));
    tmp->data=(unsigned char*)v->max_du;

	tmp->sibling=CreateNode("Min dv", F);
    tmp=tmp->sibling;
    tmp->data_size=v->uv_bone_count*sizeof(*(v->min_dv));
    tmp->data=(unsigned char*)v->min_dv;

	tmp->sibling=CreateNode("Max dv", F);
    tmp=tmp->sibling;
    tmp->data_size=v->uv_bone_count*sizeof(*(v->max_dv));
    tmp->data=(unsigned char*)v->max_dv;

	tmp->sibling=CreateNode("UV list length", F);
    tmp=tmp->sibling;
    tmp->data_size=sizeof(v->uv_list_length);
    tmp->data=(unsigned char*)&(v->uv_list_length);

	tmp->sibling=CreateNode("UV vertex id", F);
    tmp=tmp->sibling;
    tmp->data_size=v->uv_list_length*sizeof(*(v->uv_vertex_id_list));
    tmp->data=(unsigned char*)v->uv_vertex_id_list;

	tmp->sibling=CreateNode("UV default list", F);
    tmp=tmp->sibling;
    tmp->data_size=2*v->uv_list_length*sizeof(*(v->uv_list));
    tmp->data=(unsigned char*)v->uv_list;
  }

  if(verbose_level>=3){
    printf("done w/ CreateVertices\n");
  }
  
  return node;
}

file_node* CreateEdges(const edges *e)
{

  file_node *node, *tmp;

  node=CreateNode("Edges",D);

  tmp=node->child=CreateNode("Edge count",F);
  tmp->data_size=sizeof(e->count);
  tmp->data=(unsigned char*)&(e->count);

  tmp->sibling=CreateNode("Vertex list",F);
  tmp=tmp->sibling;
  tmp->data_size=2*e->count*sizeof(*(e->vertex_list));
  tmp->data=(unsigned char*)e->vertex_list;

#if 0 // old
  tmp->sibling=CreateNode("Edge property",F);
  tmp=tmp->sibling;
  tmp->data_size=e->count*sizeof(*(e->type));
  tmp->data=(unsigned char*)e->type;
#else
  tmp->sibling=CreateNode("Edge angle",F);
  tmp=tmp->sibling;
  tmp->data_size=e->count*sizeof(*(e->angle));
  tmp->data=(unsigned char*)e->angle;
#endif

  return node;
}

file_node* CreateDALodLib(const DAlod_lib *lol)
{
  file_node *node, *tmp;

  node=CreateNode("Lod_library", D);

  tmp=node->child = CreateNode("Closest_distance",F);
  tmp->data_size = sizeof(lol->closest);
  tmp->data = (unsigned char*)&(lol->closest);

  tmp->sibling = CreateNode("Furthest_distance",F);
  tmp = tmp->sibling;
  tmp->data_size = sizeof(lol->furthest);
  tmp->data = (unsigned char*)&(lol->furthest);

  tmp->sibling = CreateNode("Step_count",F);
  tmp=tmp->sibling;
  tmp->data_size = sizeof(lol->count);
  tmp->data = (unsigned char*)&(lol->count);

  tmp->sibling = CreateNode("Step_list", F);
  tmp=tmp->sibling;
  tmp->data_size = lol->count*sizeof(*(lol->step_list));
  tmp->data=(unsigned char*)lol->step_list;

  tmp->sibling = CreateNode("Removed_face_count",F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(lol->removed_face_count);
  tmp->data=(unsigned char*)&(lol->removed_face_count);

  tmp->sibling = CreateNode("Removed_face_list",F);
  tmp=tmp->sibling;
  tmp->data_size=lol->removed_face_count * sizeof(*(lol->removed_face_list));
  tmp->data=(unsigned char*)lol->removed_face_list;

  tmp->sibling = CreateNode("Vertex_count",F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(lol->vertex_count);
  tmp->data=(unsigned char*)&(lol->vertex_count);

  tmp->sibling = CreateNode("Vertex_list",F);
  tmp=tmp->sibling;
  tmp->data_size=lol->vertex_count * sizeof(*(lol->vertex_list));
  tmp->data=(unsigned char*)lol->vertex_list;

#if 1 // UV stuff
/*
	tmp->sibling = CreateNode("UV step list",F);
	tmp=tmp->sibling;
	tmp->data_size = lol->count*sizeof(lod_uv_step);
	tmp->data=(unsigned char*)lol->uv_step_list;
*/

	tmp->sibling = CreateNode("UV_chain_count",F);
	tmp=tmp->sibling;
	tmp->data_size = sizeof(lol->uv_chain_count);
	tmp->data = (unsigned char*)&(lol->uv_chain_count);

	tmp->sibling = CreateNode("UV_batch_chain",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_chain_count * sizeof(*(lol->batch_uv_id_chain));
	tmp->data=(unsigned char*)lol->batch_uv_id_chain;


	tmp->sibling = CreateNode("UV_count",F);
	tmp=tmp->sibling;
	tmp->data_size = sizeof(lol->uv_count);
	tmp->data = (unsigned char*)&(lol->uv_count);

	tmp->sibling = CreateNode("High_UV_id_1",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->high_uv_id1));
	tmp->data=(unsigned char*)lol->high_uv_id1;

	tmp->sibling = CreateNode("High_batch_count_1",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->high_batch_count1));
	tmp->data=(unsigned char*)lol->high_batch_count1;

	tmp->sibling = CreateNode("High_batch_first_1",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->high_batch_first1));
	tmp->data=(unsigned char*)lol->high_batch_first1;


	tmp->sibling = CreateNode("High_UV_id_2",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->high_uv_id2));
	tmp->data=(unsigned char*)lol->high_uv_id2;

	tmp->sibling = CreateNode("High_batch_count_2",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->high_batch_count2));
	tmp->data=(unsigned char*)lol->high_batch_count2;

	tmp->sibling = CreateNode("High_batch_first_2",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->high_batch_first2));
	tmp->data=(unsigned char*)lol->high_batch_first2;

	tmp->sibling = CreateNode("Low_UV_id",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->low_uv_id));
	tmp->data=(unsigned char*)lol->low_uv_id;
#endif

  return node;
}

file_node* CreateLodLib(const lod_lib *lol)
{

  file_node *node, *tmp;

  node=CreateNode("Lod library", D);

  tmp=node->child=CreateNode("Closest distance",F);
  tmp->data_size = sizeof(lol->closest);
  tmp->data = (unsigned char*)&(lol->closest);

  tmp->sibling = CreateNode("Furthest distance",F);
  tmp = tmp->sibling;
  tmp->data_size = sizeof(lol->furthest);
  tmp->data = (unsigned char*)&(lol->furthest);

  tmp->sibling = CreateNode("Step count",F);
  tmp=tmp->sibling;
  tmp->data_size = sizeof(lol->count);
  tmp->data = (unsigned char*)&(lol->count);

  tmp->sibling = CreateNode("Step list", F);
  tmp=tmp->sibling;
  tmp->data_size = lol->count*sizeof(lod_step);
  tmp->data=(unsigned char*)lol->step_list;

  tmp->sibling = CreateNode("Removed face count",F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(lol->removed_face_count);
  tmp->data=(unsigned char*)&(lol->removed_face_count);

  tmp->sibling = CreateNode("Removed face list",F);
  tmp=tmp->sibling;
  tmp->data_size=lol->removed_face_count * sizeof(*(lol->removed_face_list));
  tmp->data=(unsigned char*)lol->removed_face_list;


  tmp->sibling = CreateNode("Morphed face count",F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(lol->face_count);
  tmp->data=(unsigned char*)&(lol->face_count);

  tmp->sibling = CreateNode("Morphed face list",F);
  tmp=tmp->sibling;
  tmp->data_size=lol->face_count * sizeof(*(lol->face_list));
  tmp->data=(unsigned char*)lol->face_list;

  tmp->sibling = CreateNode("Group list",F);
  tmp=tmp->sibling;
  tmp->data_size=lol->face_count * sizeof(*(lol->group_list));
  tmp->data=(unsigned char*)lol->group_list;

  tmp->sibling = CreateNode("High normal list",F);
  tmp=tmp->sibling;
  tmp->data_size=lol->face_count * sizeof(*(lol->high_normal_list));
  tmp->data=(unsigned char*)lol->high_normal_list;

  tmp->sibling = CreateNode("Low normal list",F);
  tmp=tmp->sibling;
  tmp->data_size=lol->face_count * sizeof(*(lol->low_normal_list));
  tmp->data=(unsigned char*)lol->low_normal_list;

  tmp->sibling = CreateNode("High D coefficient",F);
  tmp=tmp->sibling;
  tmp->data_size=lol->face_count * sizeof(*(lol->high_D_coefficient_list));
  tmp->data=(unsigned char*)lol->high_D_coefficient_list;

  tmp->sibling = CreateNode("Low D coefficient",F);
  tmp=tmp->sibling;
  tmp->data_size=lol->face_count * sizeof(*(lol->low_D_coefficient_list));
  tmp->data=(unsigned char*)lol->low_D_coefficient_list;

  tmp->sibling = CreateNode("Vertex count",F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(lol->vertex_count);
  tmp->data=(unsigned char*)&(lol->vertex_count);

  tmp->sibling = CreateNode("Vertex list",F);
  tmp=tmp->sibling;
  tmp->data_size=lol->vertex_count * sizeof(*(lol->vertex_list));
  tmp->data=(unsigned char*)lol->vertex_list;

#if 1
	tmp->sibling = CreateNode("UV step list",F);
	tmp=tmp->sibling;
	tmp->data_size = lol->count*sizeof(lod_uv_step);
	tmp->data=(unsigned char*)lol->uv_step_list;


	tmp->sibling = CreateNode("UV chain count",F);
	tmp=tmp->sibling;
	tmp->data_size = sizeof(lol->uv_chain_count);
	tmp->data = (unsigned char*)&(lol->uv_chain_count);

	tmp->sibling = CreateNode("UV batch chain",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_chain_count * sizeof(*(lol->batch_uv_id_chain));
	tmp->data=(unsigned char*)lol->batch_uv_id_chain;


	tmp->sibling = CreateNode("UV count",F);
	tmp=tmp->sibling;
	tmp->data_size = sizeof(lol->uv_count);
	tmp->data = (unsigned char*)&(lol->uv_count);

	tmp->sibling = CreateNode("High UV id 1",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->high_uv_id1));
	tmp->data=(unsigned char*)lol->high_uv_id1;

	tmp->sibling = CreateNode("High batch count 1",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->high_batch_count1));
	tmp->data=(unsigned char*)lol->high_batch_count1;

	tmp->sibling = CreateNode("High batch first 1",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->high_batch_first1));
	tmp->data=(unsigned char*)lol->high_batch_first1;


	tmp->sibling = CreateNode("High UV id 2",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->high_uv_id2));
	tmp->data=(unsigned char*)lol->high_uv_id2;

	tmp->sibling = CreateNode("High batch count 2",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->high_batch_count2));
	tmp->data=(unsigned char*)lol->high_batch_count2;

	tmp->sibling = CreateNode("High batch first 2",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->high_batch_first2));
	tmp->data=(unsigned char*)lol->high_batch_first2;


	tmp->sibling = CreateNode("Low UV id",F);
	tmp=tmp->sibling;
	tmp->data_size=lol->uv_count * sizeof(*(lol->low_uv_id));
	tmp->data=(unsigned char*)lol->low_uv_id;
#endif

#if 0
  if(lol->edge_count > 0)
  {
    tmp->sibling = CreateNode("Edge count",F);
    tmp=tmp->sibling;
    tmp->data_size=sizeof(lol->edge_count);
    tmp->data=(unsigned char*)&(lol->edge_count);

    tmp->sibling = CreateNode("Edge list",F);
    tmp=tmp->sibling;
    tmp->data_size = lol->edge_count * sizeof(*(lol->edge_list));
    tmp->data=(unsigned char*)lol->edge_list;
  }
#endif

  if(verbose_level>=3){
    printf("done w/ CreateLodLib\n");
  }

  return node;
}

file_node* CreateUVChannelLib(const UVChannel_lib *uvcl)
{
  file_node *node, *tmp;

  node=CreateNode(UVChannel_library, D);

  tmp=node->child=CreateNode("Channel count",F);
  tmp->data_size=sizeof(uvcl->count);
  tmp->data=(unsigned char*)&(uvcl->count);

  int i;
  for(i=0; i<uvcl->count; i++)
  {
      tmp->sibling = CreateUVChannel(&(uvcl->list[i]), i);
      tmp = tmp->sibling;
  }

  if(verbose_level>=3){
    printf("done w/ CreateUVChannelLib\n");
  }

  return node;
}

file_node* CreateUVChannel(const UVChannel *uvc, int id)
{
    file_node *node, *tmp;

    char name[256]={0};
    sprintf(name, "Channel %2d",id);
    node = CreateNode(name, D);

    tmp=node->child=CreateNode("Name", F);
    tmp->data_size = (strlen(uvc->name) + 1)*sizeof(char);
    tmp->data=(unsigned char*)(uvc->name);

    tmp->sibling=CreateNode("Vertex count", F);
    tmp->sibling->data_size=sizeof(uvc->vertex_count);
    tmp->sibling->data=(unsigned char*)&(uvc->vertex_count);
    tmp=tmp->sibling;

    tmp->sibling=CreateNode("Vertex lookup", F);
    tmp->sibling->data_size=uvc->vertex_count * sizeof(*(uvc->vertex_lookup));
    tmp->sibling->data=(unsigned char*)(uvc->vertex_lookup);
    tmp=tmp->sibling;

    tmp->sibling=CreateNode("Frame rate", F);
    tmp->sibling->data_size=sizeof(uvc->fps);
    tmp->sibling->data=(unsigned char*)&(uvc->fps);
    tmp=tmp->sibling;

    tmp->sibling=CreateNode("Frame count", F);
    tmp->sibling->data_size=sizeof(uvc->frame_count);
    tmp->sibling->data=(unsigned char*)&(uvc->frame_count);
    tmp=tmp->sibling;

    tmp->sibling=CreateNode("UV chain", F);
    tmp->sibling->data_size = uvc->vertex_count * uvc->frame_count * sizeof(*(uvc->uv_chain));
    tmp->sibling->data=(unsigned char*)(uvc->uv_chain);
    tmp=tmp->sibling;

    tmp->sibling=CreateNode("Interpolate", F);
    tmp->sibling->data_size = sizeof(uvc->interpolate);
    tmp->sibling->data=(unsigned char*)&(uvc->interpolate);
    tmp=tmp->sibling;

    return node;
}

file_node* CreatePatchGroups(Bezier_mesh *bm)
{
    file_node *node;
    file_node *tmp;

    node = CreateNode("Patch groups", D);

    node->child = tmp = CreateNode("Count", F);
    tmp->data_size = sizeof(bm->group_cnt);
    tmp->data = (unsigned char*)&(bm->group_cnt);

    for(int i = 0; i < bm->group_cnt; i++)
    {
		tmp->sibling = CreatePatchGroup(bm->groups + i, i);
		tmp = tmp->sibling;	
    }

    return node;
}

file_node* CreatePatchGroup(Bezier_patch_group *pg, int id)
{
	file_node *node;
    file_node *tmp;

	char name[32];
    sprintf(name, "Group%d", id);
	
	if(pg->patch_cnt <= 0)
	{
		Winprint("Error: patch group w/ %d patches!\n", pg->patch_cnt);
	}

    node = CreateNode(name, D);

    node->child = tmp = CreateNode("Material", F);
    tmp->data_size = sizeof(pg->mtl_id);
    tmp->data = (unsigned char*)&(pg->mtl_id);

    tmp->sibling = CreateNode("Patch count", F);
    tmp = tmp->sibling;
    tmp->data_size = sizeof(pg->patch_cnt);
    tmp->data = (unsigned char*)&(pg->patch_cnt);

	tmp->sibling = CreateNode("Patch list", F);
    tmp = tmp->sibling;
    tmp->data_size = pg->patch_cnt * sizeof(*(pg->patch_list));
    tmp->data = (unsigned char*)(pg->patch_list);
  
	return node;
}

file_node* CreateFaceGroups(object *obj)
{
    assert(obj->face_group_count > 0 && obj->face_group_list != NULL);

    file_node *node;
    file_node *tmp;

    node = CreateNode("Face groups", D);

    node->child = tmp = CreateNode("Count", F);
    tmp->data_size = sizeof(obj->face_group_count);
    tmp->data = (unsigned char*)&(obj->face_group_count);

    for(int i=0; i<obj->face_group_count; i++)
    {
		tmp->sibling = CreateFaceGroup(&(obj->face_group_list[i]), i);
		tmp = tmp->sibling;	
    }

    return node;
}

file_node* CreateFaceGroup(face_group *fg, int id)
{
    file_node *node;
    file_node *tmp;

    char name[32];
    sprintf(name, "Group%d", id);
	
	if(fg->count <= 0)
	{
		Winprint("Error: face group w/ 0 faces! Possible cause: Too much LOD.\n");
	}

    node = CreateNode(name, D);

    node->child = tmp = CreateNode("Material", F);
    tmp->data_size = sizeof(fg->material_id);
    tmp->data = (unsigned char*)&(fg->material_id);

    tmp->sibling = CreateNode("Face count", F);
    tmp=tmp->sibling;
    tmp->data_size=sizeof(fg->count);
    tmp->data=(unsigned char*)&(fg->count);

    tmp->sibling=CreateNode("Face vertex chain", F);
    tmp=tmp->sibling;
    tmp->data_size=3*fg->count*sizeof(*(fg->batch_chain));
    tmp->data=(unsigned char*)fg->batch_chain;

    if(fg->adjacent_face_list)
	{
      tmp->sibling=CreateNode("Adjacent face list",F);
      tmp=tmp->sibling;
      tmp->data_size = 3 * fg->count * sizeof(*(fg->adjacent_face_list));
      tmp->data=(unsigned char*)fg->adjacent_face_list;
	}

    tmp->sibling=CreateNode(Face_normal,F);
    tmp=tmp->sibling;
    tmp->data_size=fg->count*sizeof(*(fg->normal));
    tmp->data=(unsigned char*)fg->normal;

    tmp->sibling=CreateNode(Face_D_coefficient,F);
    tmp=tmp->sibling;
    tmp->data_size=fg->count*sizeof(*(fg->D_coefficient));
    tmp->data=(unsigned char*)fg->D_coefficient;
  
    tmp->sibling=CreateNode(Face_property,F);
    tmp=tmp->sibling;
    tmp->data_size=fg->count*sizeof(*(fg->property));
    tmp->data=(unsigned char*)fg->property;
  
    if(verbose_level>=3)
	{
      printf("done w/ CreateFaceGroup %d\n", id);
	}

    return node;
}

file_node* CreateNurbSet(object *obj)
{
    assert(obj->nurb_count > 0 && obj->nurb_list != NULL);

    file_node *node;
    file_node *tmp;

    node = CreateNode("NURB set", D);

    node->child = tmp = CreateNode("Count", F);
    tmp->data_size = sizeof(obj->nurb_count);
    tmp->data = (unsigned char*)&(obj->nurb_count);

    for(int i=0; i < obj->nurb_count; i++)
    {
		tmp->sibling = CreateNurb(&(obj->nurb_list[i]), i);
		tmp = tmp->sibling;	
    }

    return node;
}

file_node* CreateNurb(nurb *nr, int id)
{
    file_node *node;
    file_node *tmp;

    char name[32];
    sprintf(name, "NURB%d", id);
	
    node = CreateNode(name, D);

    node->child = tmp = CreateNode("S order", F);
    tmp->data_size = sizeof(nr->s_order);
    tmp->data = (unsigned char*)&(nr->s_order);

	tmp->sibling = CreateNode("T order", F);
    tmp = tmp->sibling;
    tmp->data_size=sizeof(nr->t_order);
    tmp->data=(unsigned char*)&(nr->t_order);

	tmp->sibling = CreateNode("S knot count", F);
    tmp = tmp->sibling;
    tmp->data_size=sizeof(nr->s_knot_count);
    tmp->data=(unsigned char*)&(nr->s_knot_count);

	tmp->sibling = CreateNode("S knot list", F);
    tmp = tmp->sibling;
    tmp->data_size=nr->s_knot_count * sizeof(*(nr->s_knot_list));
    tmp->data=(unsigned char*)(nr->s_knot_list);

	tmp->sibling = CreateNode("T knot count", F);
    tmp = tmp->sibling;
    tmp->data_size=sizeof(nr->t_knot_count);
    tmp->data=(unsigned char*)&(nr->t_knot_count);

	tmp->sibling = CreateNode("T knot list", F);
    tmp = tmp->sibling;
    tmp->data_size=nr->t_knot_count * sizeof(*(nr->t_knot_list));
    tmp->data=(unsigned char*)(nr->t_knot_list);

	tmp->sibling = CreateNode("S point count", F);
    tmp = tmp->sibling;
    tmp->data_size=sizeof(nr->s_point_count);
    tmp->data=(unsigned char*)&(nr->s_point_count);

	tmp->sibling = CreateNode("T point count", F);
    tmp = tmp->sibling;
    tmp->data_size=sizeof(nr->t_point_count);
    tmp->data=(unsigned char*)&(nr->t_point_count);

#ifndef _DEBUG
  if(nr->vertex_bone_count == NULL)
#endif
  {
	tmp->sibling = CreateNode("Point list", F);
    tmp = tmp->sibling;
    tmp->data_size = 3 * nr->s_point_count * nr->t_point_count * sizeof(*(nr->point_list));
    tmp->data=(unsigned char*)(nr->point_list);

	if(nr->weight_list)
	{
		tmp->sibling = CreateNode("Weight list", F);
		tmp = tmp->sibling;
		tmp->data_size=nr->s_point_count * nr->t_point_count * sizeof(*(nr->weight_list));
		tmp->data=(unsigned char*)(nr->weight_list);
	}
  }

	tmp->sibling = CreateNode("Material", F);
    tmp = tmp->sibling;
    tmp->data_size=sizeof(nr->mtl_id);
    tmp->data=(unsigned char*)&(nr->mtl_id);
	
	if(nr->s_uv_count > 0 && nr->t_uv_count > 0)
	{
		tmp->sibling = CreateNode("S UV count", F);
		tmp = tmp->sibling;
		tmp->data_size = sizeof(nr->s_uv_count);
		tmp->data=(unsigned char*)&(nr->s_uv_count);

		tmp->sibling = CreateNode("T UV count", F);
		tmp = tmp->sibling;
		tmp->data_size = sizeof(nr->t_uv_count);
		tmp->data=(unsigned char*)&(nr->t_uv_count);

		tmp->sibling = CreateNode("UV list", F);
		tmp = tmp->sibling;
		tmp->data_size = 2 * nr->s_uv_count * nr->t_uv_count * sizeof(nr->uv_list[0]);
		tmp->data=(unsigned char*)(nr->uv_list);
	}

	if(nr->vertex_bone_count != NULL)
	{
		tmp->sibling = CreateNode("Point bone count", F);
		tmp = tmp->sibling;
		tmp->data_size = nr->s_point_count * nr->t_point_count * sizeof(*(nr->vertex_bone_count));
		tmp->data = (unsigned char*)(nr->vertex_bone_count);

		int count = 0;
		for(int i = 0; i < nr->s_point_count * nr->t_point_count; i++)
		{
			assert(count == nr->first_vertex[i]);
			count += nr->vertex_bone_count[i];
		}

		tmp->sibling = CreateNode("First point", F);
		tmp = tmp->sibling;
		tmp->data_size = count * sizeof(*(nr->first_vertex));
		tmp->data = (unsigned char*)(nr->first_vertex);

		tmp->sibling = CreateNode("Bone id list", F);
		tmp = tmp->sibling;
		tmp->data_size = count * sizeof(*(nr->bone_id_list));
		tmp->data = (unsigned char*)(nr->bone_id_list);

		tmp->sibling = CreateNode("Bone point list", F);
		tmp = tmp->sibling;
		tmp->data_size = 3 * count * sizeof(*(nr->bone_vertex_list));
		tmp->data = (unsigned char*)(nr->bone_vertex_list);

		if(nr->bone_rational_list)
		{
			tmp->sibling = CreateNode("Bone rational list", F);
			tmp = tmp->sibling;
			tmp->data_size = count * sizeof(*(nr->bone_rational_list));
			tmp->data = (unsigned char*)(nr->bone_rational_list);
		}

		tmp->sibling = CreateNode("Bone weight list", F);
		tmp = tmp->sibling;
		tmp->data_size = count * sizeof(*(nr->bone_weight_list));
		tmp->data = (unsigned char*)(nr->bone_weight_list);
	}

	return node;
}

/* v & n are copied into db */
void InsertPolyTriangles(object *obj, poly *p)
{
  if(p->vertex_count > MAX_POLY_VERTEX_COUNT)
  {
	  Winprint("Error: polygon w/ too many (%d) vertices!\n", p->vertex_count);
	  exit(1);
  }

  if(p->vertex_count > 3)
  {
	  poly pt;
	  pt.vertex_count = 3;
	  pt.property = p->property;
	  pt.material_id = p->material_id;
	  pt.api_face_id = p->api_face_id;
	  pt.api_node_id = p->api_node_id;
	  pt.api_mtl_id = p->api_mtl_id;
	  pt.api_smg_id = p->api_smg_id;

      int n_triangles = p->vertex_count - 2;
      for(int i=0; i < n_triangles; i++)
	  {
		pt.vertices[0] = p->vertices[0];
		pt.vertices[1] = p->vertices[1];
		pt.vertices[2] = p->vertices[2];

		pt.vertices[3] = p->vertices[3*(i+1)];
		pt.vertices[4] = p->vertices[3*(i+1)+1];
		pt.vertices[5] = p->vertices[3*(i+1)+2];

		pt.vertices[6] = p->vertices[3*(i+2)];
		pt.vertices[7] = p->vertices[3*(i+2)+1];
		pt.vertices[8] = p->vertices[3*(i+2)+2];

		pt.frozen[0] = p->frozen[0];
		pt.frozen[1] = p->frozen[1];
		pt.frozen[2] = p->frozen[2];

		pt.frozen[3] = p->frozen[3*(i+1)];
		pt.frozen[4] = p->frozen[3*(i+1)+1];
		pt.frozen[5] = p->frozen[3*(i+1)+2];

		pt.frozen[6] = p->frozen[3*(i+2)];
		pt.frozen[7] = p->frozen[3*(i+2)+1];
		pt.frozen[8] = p->frozen[3*(i+2)+2];

		pt.uv[0] = p->uv[0];
		pt.uv[1] = p->uv[1];

		pt.uv[2] = p->uv[2*(i+1)];
		pt.uv[3] = p->uv[2*(i+1)+1];

		pt.uv[4] = p->uv[2*(i+2)];
		pt.uv[5] = p->uv[2*(i+2)+1];

		pt.api_uv_id[0] = p->api_uv_id[0];
		pt.api_uv_id[1] = p->api_uv_id[i+1];
		pt.api_uv_id[2] = p->api_uv_id[i+2];

		pt.api_uv_id2[0] = p->api_uv_id2[0];
		pt.api_uv_id2[1] = p->api_uv_id2[i+1];
		pt.api_uv_id2[2] = p->api_uv_id2[i+2];

		pt.api_xyz_id[0] = p->api_xyz_id[0];
		pt.api_xyz_id[1] = p->api_xyz_id[i+1];
		pt.api_xyz_id[2] = p->api_xyz_id[i+2];

		InsertPoly(obj, &pt);
		p->group_id = pt.group_id;
		p->face_id = pt.face_id;
	  }
  }
  else
  {
	  InsertPoly(obj, p);
  }
}


/* v is copied into db */
// triangles ONLY
int InsertPoly(object *obj, poly *p)
{
  assert(p->vertex_count == 3);

  vertices *vl = &(obj->v);
  normals *nl = &(obj->n);
 
  // check for polys w/o area 
  // also rotate indices so that first index is the corner w/ the largest angle
  if( -1 == CheckPoly(p) )
  {
    fprintf(stderr,"Warning: CheckPoly() failed.\n");
    return -1;
  }

  /* adjust u,v coordinates to 1st quadrant */
  // TODO: run some tests to see if this really helps uv sharing or not
  AdjustUV(&(p->uv[0]), p->vertex_count);
  if(p->uv2[0] != FLT_MAX)
  {
	  AdjustUV(&(p->uv2[0]), p->vertex_count);
  }

  // calc poly normal (vertices must be counter clock wise)
  float normal[3];
  if(-1==calcNormal(&(p->vertices[0]), 
                    &(p->vertices[3]),
                    &(p->vertices[6]), normal))
  {
    fprintf(stderr,"Warning: poly not inserted due to invalid normal.\n");
    return -1; // no normal for this poly
  }
  
  // add xyz vertices
  const int xyz_id1 = InsertXYZVertex(vl, &(p->vertices[0]), p->api_node_id, p->api_xyz_id[0],
	  p->api_smg_id, &(p->frozen[0]), &(p->color[0]), p->flags[0]);
  const int xyz_id2 = InsertXYZVertex(vl, &(p->vertices[3]), p->api_node_id, p->api_xyz_id[1],
	  p->api_smg_id, &(p->frozen[3]), &(p->color[3]), p->flags[1]);
  const int xyz_id3 = InsertXYZVertex(vl, &(p->vertices[6]), p->api_node_id, p->api_xyz_id[2],
	  p->api_smg_id, &(p->frozen[6]), &(p->color[6]), p->flags[2]);

  // add uv's
  const int uv_id1 = InsertUVVertex(vl, &(p->uv[0]), p->api_node_id, p->api_uv_id[0], p->flags[0]);
  const int uv_id2 = InsertUVVertex(vl, &(p->uv[2]), p->api_node_id, p->api_uv_id[1], p->flags[1]);
  const int uv_id3 = InsertUVVertex(vl, &(p->uv[4]), p->api_node_id, p->api_uv_id[2], p->flags[2]);

  int uv2_id1 = -1;
  int uv2_id2 = -1;
  int uv2_id3 = -1;

  if(p->uv2[0] != FLT_MAX)
  {
	uv2_id1 = InsertUVVertex(vl, &(p->uv2[0]), p->api_node_id, p->api_uv_id2[0], 0);//p->flags[0]);
	uv2_id2 = InsertUVVertex(vl, &(p->uv2[2]), p->api_node_id, p->api_uv_id2[1], 0);//p->flags[1]);
	uv2_id3 = InsertUVVertex(vl, &(p->uv2[4]), p->api_node_id, p->api_uv_id2[2], 0);//p->flags[2]);
  }

   // update batch list & face group
  const int group_id = InsertFaceGroup(obj, p->material_id);
  face_group *fg = obj->face_group_list + group_id;

  p->group_id = group_id;
  p->face_id = fg->count;

  fg->count++;
  fg->max_count++;
  obj->face_count++;
  obj->max_face_count++;

  // add face & batch vertices
  fg->batch_chain = (int*)Realloc(fg->batch_chain, 3*fg->count*sizeof(int));
  fg->batch_chain[3*(fg->count-1)    ] = InsertBatchVertex(vl, xyz_id1, uv_id1, uv2_id1, p->material_id);
  fg->batch_chain[3*(fg->count-1) + 1] = InsertBatchVertex(vl, xyz_id2, uv_id2, uv2_id2, p->material_id);
  fg->batch_chain[3*(fg->count-1) + 2] = InsertBatchVertex(vl, xyz_id3, uv_id3, uv2_id3, p->material_id);

  fg->api_node_id = (int*)Realloc(fg->api_node_id, fg->count * sizeof(int));
  fg->api_node_id[fg->count-1] = p->api_node_id;

  fg->api_face_id = (int*)Realloc(fg->api_face_id, fg->count * sizeof(int));
  fg->api_face_id[fg->count-1] = p->api_face_id;

  fg->api_smg_id = (int*)Realloc(fg->api_smg_id, fg->count * sizeof(int));
  fg->api_smg_id[fg->count-1] = p->api_smg_id;

  if(fg->count == 1)
  {
	  fg->api_mtl_id = p->api_mtl_id;
  }
  else
  {
	  if(fg->api_mtl_id != p->api_mtl_id)
	  {
		  Winprint("Error: materials out of synch!\n");
	  }
  }

  fg->normal = (int*)Realloc(fg->normal, fg->count * sizeof(int));
  fg->normal[fg->count-1] = InsertNormal(nl, normal, face_normal_tolerance);

  fg->D_coefficient = (float*)Realloc(fg->D_coefficient, fg->count * sizeof(float));
  fg->D_coefficient[fg->count-1] = (-Dot3(normal, &(p->vertices[0])) - 
									 Dot3(normal, &(p->vertices[3])) -
									 Dot3(normal, &(p->vertices[6]))) * (1.0 / 3.0);

  fg->property = (FACE_PROPERTY*)Realloc(fg->property, fg->count * sizeof(FACE_PROPERTY));
  fg->property[fg->count-1] = p->property; 

  //fg->adjacent_face_list; // synch edges

  return 0;
}

int InsertFaceGroup(object *obj, int m_id)
{
    for(int i=0; i<obj->face_group_count; i++)
    {
        if(obj->face_group_list[i].material_id == m_id)
        {
            return i;
        }
    }

    obj->face_group_count++;
    obj->face_group_list = (face_group*)Realloc(obj->face_group_list, obj->face_group_count
                            * sizeof(face_group));
    InitFaceGroup(&(obj->face_group_list[obj->face_group_count-1]));
    obj->face_group_list[obj->face_group_count-1].material_id = m_id;

    return (obj->face_group_count - 1);
}

int GetBatchID(const vertices *vl, const int xyz_id, const int uv_id, const int uv_id2, const int m_id)
{
  for(int i = 0; i < vl->max_batch_count; i++)
  {
    if(vl->object_batch_list[i] == xyz_id &&
       vl->texture_batch_list[i] == uv_id &&
	   vl->mtl_batch_list[i] == m_id &&
	   ((vl->texture_batch_list2 && vl->texture_batch_list2[i] == uv_id2) ||
	    (vl->texture_batch_list2 == NULL && uv_id2 == -1))
	   //vl->api_uv_id_list[i] == api_uv_id &&
	   //(api_uv_id == -1 || vl->mtl_batch_list[i] == m_id)
	   // if this is an animated uv vertex mtl has to be unique too
	  )
    {
        return i;
    }
  }

  return -1;
}


int InsertBatchVertex(vertices *vl, const int xyz_id, const int uv_id, const int uv_id2, const int m_id)
{
  int batch_id = GetBatchID(vl, xyz_id, uv_id, uv_id2, m_id);

  if(batch_id >= 0)
  {
	  return batch_id;
  }

  batch_id = vl->batch_count;
  
  // new unique batch vertex
  vl->batch_count++;
  vl->max_batch_count++;

  vl->object_batch_list = (int*)Realloc(vl->object_batch_list, vl->batch_count*sizeof(int));
  vl->object_batch_list[batch_id] = xyz_id;

  vl->texture_batch_list = (int*)Realloc(vl->texture_batch_list, vl->batch_count*sizeof(int));
  vl->texture_batch_list[batch_id] = uv_id;


  if(uv_id2 >= 0 || vl->texture_batch_list2)
  {
	// first vertex w/ 2nd uv
	if(vl->texture_batch_list2 == NULL)
	{
		vl->texture_batch_list2 = (int*)Malloc(vl->batch_count*sizeof(int));
		for(int i=0; i < vl->batch_count; i++)
		{
			vl->texture_batch_list2[i] = -1;
		}
	}
	else
	{
		vl->texture_batch_list2 = (int*)Realloc(vl->texture_batch_list2, vl->batch_count*sizeof(int));
	}
	
	vl->texture_batch_list2[batch_id] = uv_id2;
  }


  vl->mtl_batch_list = (int*)Realloc(vl->mtl_batch_list, vl->batch_count*sizeof(int));
  vl->mtl_batch_list[batch_id] = m_id;

  //vl->api_uv_id_list = (int*)Realloc(vl->api_uv_id_list, vl->batch_count*sizeof(int));
  //vl->api_uv_id_list[vl->batch_count-1] = api_uv_id;

  //vl->api_face_list = (int*)Realloc(vl->api_face_list, vl->batch_count*sizeof(int));
  //vl->api_face_list[vl->batch_count-1] = api_face_id;

  return batch_id;
}


int InsertXYZVertex(vertices *vl, const float v[3], const int api_node_id, const int api_v_id,
					const int api_smg_id, const float frozen[3], const int color[3], const int flags)
{
    // look for duplicate vertex
    int v_id = GetVertexID(vl, v, api_node_id, api_v_id, api_smg_id);
    if(v_id >= 0)
    {
		vl->flags[v_id] |= flags;
        return v_id;
    }

    // new vertex
	v_id = vl->object_count;
    vl->object_count++;
    vl->max_object_count++;

	// add x,y,z into list
    vl->object_list=(float*)Realloc(vl->object_list, 3*vl->object_count*sizeof(float));
    vl->object_list[3*v_id  ] = v[0];
    vl->object_list[3*v_id+1] = v[1];
    vl->object_list[3*v_id+2] = v[2];

	// add frozen weights
    vl->frozen_list=(float*)Realloc(vl->frozen_list, 3*vl->object_count*sizeof(float));
    vl->frozen_list[3*v_id  ] = frozen[0];
    vl->frozen_list[3*v_id+1] = frozen[1];
    vl->frozen_list[3*v_id+2] = frozen[2];

	vl->api_node_xyz_id = (int*)Realloc(vl->api_node_xyz_id, vl->object_count*sizeof(int));
    vl->api_node_xyz_id[v_id] = api_node_id;
   
	vl->api_xyz_id = (int*)Realloc(vl->api_xyz_id, vl->object_count*sizeof(int));
    vl->api_xyz_id[v_id] = api_v_id;

	vl->api_smg_id = (int*)Realloc(vl->api_smg_id, vl->object_count*sizeof(int));
    vl->api_smg_id[v_id] = api_smg_id;

	vl->flags = (int*)Realloc(vl->flags, vl->object_count*sizeof(int));
    vl->flags[v_id] = flags;


	if(export_vertex_colors)
	{
		if(vl->object_count == 1 && (color[0] >= 0 && color[1] >= 0 && color[2] >= 0))
		{
			assert(vl->color == NULL);
			vl->color=(unsigned char*)Realloc(vl->color, 3*vl->object_count*sizeof(unsigned char));

			vl->color[3*v_id  ] = (unsigned char)color[0];
			vl->color[3*v_id+1] = (unsigned char)color[1];
			vl->color[3*v_id+2] = (unsigned char)color[2];
		}
		else
		{
			if(vl->color) // first vertex had a color
			{
				vl->color=(unsigned char*)Realloc(vl->color, 3*vl->object_count*sizeof(unsigned char));

				if(color[0] >= 0 && color[1] >= 0 && color[2] >= 0)
				{
					vl->color[3*v_id  ] = (unsigned char)color[0];
					vl->color[3*v_id+1] = (unsigned char)color[1];
					vl->color[3*v_id+2] = (unsigned char)color[2];
				}
				else // bad color for this vertex
				{
					Winprint("Error: vertex color is %d %d %d!\n", color[0], color[1], color[2]);
					vl->color[3*v_id  ] =
					vl->color[3*v_id+1] =
					vl->color[3*v_id+2] = 0;
				}
			}
			else
			if(color[0] >= 0 || color[1] >= 0 || color[2] >= 0)
			{
				// there should not be any valid colors since there were none for
				// the first vertex
				Winprint("Error: vertex %d is colored but first vertex was not!\n",
					vl->object_count - 1);
			}
		}
	}

    return v_id;
}

int InsertUVVertex(vertices *vl, const float uv[2], const int api_node_id, const int api_uv_id,
				   const int flags)
{
	// this has failed before when 3DS MAX had a few -1.#QNAN as uv's
	assert(uv[0] != FLT_MAX && uv[1] != FLT_MAX);

    int uv_id = GetUVID(vl, uv, api_node_id, api_uv_id);

	if(uv_id >= 0)
	{
		vl->uv_flags[uv_id] |= flags;
		return uv_id;
	}

	// new uv
	uv_id = vl->texture_count;

    vl->texture_count++;

	/* add u,v into list */
    vl->texture_list=(float*)Realloc(vl->texture_list, 2*vl->texture_count*sizeof(float));
    vl->texture_list[2*uv_id] = uv[0];
    vl->texture_list[2*uv_id+1] = uv[1];

	vl->api_node_uv_id=(int*)Realloc(vl->api_node_uv_id, vl->texture_count*sizeof(int));
	vl->api_node_uv_id[uv_id] = api_node_id;

	vl->api_uv_id=(int*)Realloc(vl->api_uv_id, vl->texture_count*sizeof(int));
	vl->api_uv_id[uv_id] = api_uv_id;

	vl->uv_flags=(int*)Realloc(vl->uv_flags, vl->texture_count*sizeof(int));
	vl->uv_flags[uv_id] = flags;
       
    return uv_id;
}

int GetUVID(const vertices *vl, const float uv[2], const int api_node_id, const int api_uv_id)
{
	if(api_node_id != 0 && api_uv_id >= 0) // based on api's id only
	{
		assert(api_node_id != 0 && api_uv_id >= 0);
		for(int i=0; i < vl->texture_count; i++)
		{
			if(vl->api_uv_id[i] == api_uv_id && vl->api_node_uv_id[i] == api_node_id)
			{
				assert(SameUV(vl->texture_list + 2*i, uv, UV_TOLERANCE));
				return i;
			}
		}
	}
	else  // based on uv
	//if(api_node_id == 0 && api_uv_id == -1)
	{
		for(int i=0; i < vl->texture_count; i++)
		{
			if( SameUV(vl->texture_list + 2*i, uv, UV_TOLERANCE) &&
				vl->api_uv_id[i] == api_uv_id &&
				vl->api_node_uv_id[i] == api_node_id)
			{
				return i;
			}
		}	
	}

	return -1;
}

int GetVertexID(const vertices * const vl, const float * const v, const int api_node_id,
				const int api_v_id, const int api_smg_id)
{           
/*
	for(int i = 0; i < vl->object_count; i++)
	{
		if( vl->api_smg_id[i] == api_smg_id &&
			SameXYZ(vl->object_list + 3*i, v, XYZ_TOLERANCE) )
		{
			printf("%d %d\n", i, api_smg_id);
			return i;
		}
	}

	printf("%d %d\n", -1, api_smg_id);

	return -1;
*/
	if(api_node_id != 0 && api_v_id >= 0) // based on api's id only
	{
		for(int i = 0; i < vl->object_count; i++)
		{
			if(vl->api_xyz_id[i] == api_v_id && vl->api_node_xyz_id[i] == api_node_id) 
			{
				assert(api_smg_id == 0 || vl->api_smg_id[i] == api_smg_id);
				assert(SameXYZ(vl->object_list + 3*i, v, XYZ_TOLERANCE));
				return i;
			}
		}
	}
	else // based on position
	// if(api_node_id == 0 && api_v_id == -1)
	{
		for(int i = 0; i < vl->object_count; i++)
		{
			if( (api_smg_id == 0 || vl->api_smg_id[i] == api_smg_id) && 
				SameXYZ(vl->object_list + 3*i, v, XYZ_TOLERANCE) && 
				vl->api_xyz_id[i] == api_v_id &&
				vl->api_node_xyz_id[i] == api_node_id)
			{
				return i;
			}
		}
	}

	return -1;
}

void InitNormal(normals *n)
{
  n->count=0;
  n->list=NULL;
}

void InitVertex(vertices *v)
{
  v->object_count=0;
  v->max_object_count=0;
  v->object_list=NULL;
  v->flags=NULL;
  v->normal=NULL;
  v->D_coefficient=NULL;
  v->frozen_list=NULL;
  v->err = NULL;
  v->color = NULL;
  v->api_node_xyz_id=NULL;
  v->api_xyz_id=NULL;
  v->api_smg_id=NULL;
  v->b_v_list=NULL;

  v->vertex_bone_count=NULL;  
  v->first_vertex=NULL;
  v->bone_id_list=NULL;       
  v->bone_weight_list=NULL; 
  v->bone_vertex_list=NULL; 
  v->bone_normal_list=NULL; 

  v->batch_count=0;
  v->max_batch_count=0;
  v->object_batch_list=NULL;
  v->texture_batch_list=NULL;
  v->texture_batch_list2=NULL;
  v->mtl_batch_list=NULL;

  v->texture_count=0;
  v->texture_list=NULL;
  v->uv_flags=NULL;
  v->api_node_uv_id=NULL;
  v->api_uv_id=NULL;

  v->uv_bone_count = 0;
  v->uv_bone_id = NULL;
  v->uv_vertex_count = NULL;	
  v->uv_plane_distance = NULL;	
  v->x_to_u_scale = NULL;			
  v->y_to_v_scale = NULL;
  v->min_du = NULL;			
  v->max_du = NULL;
  v->min_dv = NULL;			
  v->max_dv = NULL;
  v->uv_list_length = 0;
  v->uv_vertex_id_list = NULL;
  v->uv_list = NULL;	
}

void InitEdges(edges *e)
{
    e->count = 0;
    e->vertex_list = NULL;
	e->angle = NULL;
}

void InitEedgeLib(Eedge_lib *el)
{
    el->count = 0;
    el->max_count = 0;
    el->list = NULL;
}

void InitLodObject(lod_object *lobj, mtl_lib *ml)
{
  lobj->count=0;
  lobj->switch_list=NULL;
  lobj->mim_max_dist[0] = -1.0f;
  lobj->mim_max_dist[1] = -1.0f;
  lobj->obj_list=NULL;
  lobj->file_name[0]=0;
  lobj->export_flag=1;
//  lobj->tl = tl;
//  lobj->atl = atl;
  lobj->ml = ml;
}

void InitObject(object *obj, /*txt_lib *tl, anim_txt_lib *atl,*/ mtl_lib *ml)
{
  obj->type = INVALID;

  InitGeometry(obj);

//  obj->tl = tl;
//  obj->atl = atl;
  obj->ml = (mtl_lib*)Malloc(sizeof(mtl_lib));
  InitMtlLib(obj->ml);
  InitLodLib(&(obj->lol));
  InitUVChannelLib(&(obj->uvcl));
  InitHardPoints(obj);
  InitExtents(&(obj->extents));
  InitLight(&(obj->l));
  InitCamera(&(obj->c));

  obj->nurb_count = 0;
  obj->nurb_list = NULL;
  
  obj->b_mesh.Init();

  InitProps(obj);

  obj->da_mesh.Init( ml );
}

void InitProps(object *obj)
{
	obj->prop_hdr.Init();
	obj->prop_list = NULL;
	obj->bin_prop_size = 0;
	obj->bin_prop = NULL;
}

void InitCamera(camera *c)
{
	c->fovx = 0.0f;
	c->fovy = 0.0f;
	c->znear = 0.0f;
	c->zfar = 0.0f;
}

void InitLight(light *l)
{
    l->color[0] = l->color[1] = l->color[2] = 1.0f;
    l->direction[0] = l->direction[1] = 0.0f;
    l->direction[2] = -1.0f;
    l->cutoff = 180.0f * D2R;
	l->hotspot = 180.0f * D2R;
	l->parallel = 0;
    l->range = -1.0f;
	l->texture_name = NULL;
}

void FreeLight(light *l)
{
	Free(l->texture_name);
}

void InitNurb(nurb *nr)
{
	nr->name = NULL;

	nr->s_order = -1;
	nr->t_order = -1;

	nr->s_closed = false;
	nr->t_closed = false;

	nr->s_knot_count = 0;
	nr->t_knot_count = 0;
	nr->s_knot_list = NULL;
	nr->t_knot_list = NULL;

	nr->s_basis_cnt = 0;
	nr->t_basis_cnt = 0;

	nr->s_point_count = 0;
	nr->t_point_count = 0;
	nr->point_list = NULL;
	nr->weight_list = NULL;
	nr->api_s_id = NULL;
	nr->api_t_id = NULL;
	nr->api_node_id = 0;
	nr->api_st_offset = 0;
	nr->api_node_cv_total = 0;

	nr->mtl_id = -1;

	nr->s_uv_count = 0;
	nr->t_uv_count = 0;
	nr->uv_list = NULL;

	nr->b_v_list = NULL;
	nr->vertex_bone_count = NULL;
	nr->first_vertex = NULL;
	nr->bone_id_list = NULL;
	nr->bone_weight_list = NULL;
	nr->bone_vertex_list = NULL;
	nr->bone_rational_list = NULL;
}

void InitExtents(Extents *extents)
{
  extents->mass.mass = 0.01f;
  extents->mass.center.x=
  extents->mass.center.y=
  extents->mass.center.z= 0.0f;
  InitMatrix(&(extents->mass.inertia));
  // box
  extents->mass.inertia.d[0][0] /= 12.0f;
  extents->mass.inertia.d[1][1] /= 12.0f;
  extents->mass.inertia.d[2][2] /= 12.0f;


  extents->sphere.name[0] = 0;
  extents->sphere.center.x=
  extents->sphere.center.y=
  extents->sphere.center.z= 0.0f;
  extents->sphere.render_center.x=
  extents->sphere.render_center.y=
  extents->sphere.render_center.z= 0.0f;
  extents->sphere.radius = 0.0f;
  extents->sphere.render_radius = 0.0f;
  extents->sphere.volume = (4.0f/3.0f) * M_PI * extents->sphere.radius *
                           extents->sphere.radius * extents->sphere.radius;

  extents->box.name[0] = 0;
  extents->box.center.x=
  extents->box.center.y=
  extents->box.center.z= 0.0f;
  extents->box.size.x=
  extents->box.size.y=
  extents->box.size.z= 0.0f; // half of dimension
  extents->box.volume = 8.0f * extents->box.size.x *
                        extents->box.size.y * extents->box.size.z;
  extents->box.adjusted = false;

  extents->cylinder.Init();
  extents->convex_hull.Init();
}

/*
void ZeroExtents(Extents *extents)
{
  extents->mass.mass= 0.0f;
  extents->mass.center.x=
  extents->mass.center.y=
  extents->mass.center.z= 0.0f;
  InitMatrix(&(extents->mass.inertia));

  extents->sphere.center.x=
  extents->sphere.center.y=
  extents->sphere.center.z= 0.0f;
  extents->sphere.radius= 0.0f;
  extents->sphere.volume= 0.0f;

  extents->box.center.x=
  extents->box.center.y=
  extents->box.center.z= 0.0f;
  extents->box.size.x=
  extents->box.size.y=
  extents->box.size.z= 0.0f; // half of dimension
  extents->box.volume= 0.0f;

  extents->cylinder.Init(); // == Zero
  extents->convex_hull.Init();
}
*/

void InitMatrix(Matrix *m)
{
  m->d[0][0]=
  m->d[1][1]=
  m->d[2][2]=1.0f;
 
  m->d[0][1]= 
  m->d[0][2]= 
  m->d[1][0]= 
  m->d[1][2]= 
  m->d[2][0]= 
  m->d[2][1]=0.0;
}

void InitHardPoints(object *obj)
{
  obj->hp_fix_count=0;
  obj->hp_fix_list=NULL;
  obj->hp_pris_count=0;
  obj->hp_pris_list=NULL;
  obj->hp_rev_count=0;
  obj->hp_rev_list=NULL;
}

void InitGeometry(object *obj)
{
  obj->face_count = 0;
  obj->max_face_count = 0;
  obj->face_group_count = 0;
  obj->face_group_list = NULL;

  //InitFace(&(obj->f));
  InitVertex(&(obj->v));
  InitNormal(&(obj->n));
  InitEdges(&(obj->e));
  InitEedgeLib(&(obj->el));

  obj->nurb_count = 0;
  obj->nurb_list = NULL;
}

void InitLodLib(lod_lib *lol)
{
	lol->closest = -1.0f;
	lol->furthest = -1.0f;

    lol->count = 0;
    lol->step_list = NULL;
	lol->uv_step_list = NULL;

	lol->removed_face_count = 0;
	lol->removed_face_list = NULL;

    lol->face_count = 0;
    lol->face_list = NULL;
	lol->group_list = NULL;
    lol->high_normal_list = NULL;
    lol->high_D_coefficient_list = NULL;
    lol->low_normal_list = NULL;
    lol->low_D_coefficient_list = NULL;

    lol->vertex_count = 0;
    lol->vertex_list = NULL;

    lol->tmp_edge_count = 0;
    lol->tmp_edge_list = NULL;
    lol->edge_list = NULL;
    lol->edge_count = 0;

	lol->max_uv_per_step = 0;
	lol->uv_chain_count = 0;
	lol->batch_uv_id_chain = NULL;
	//lol->batch_uv_id_chain2 = NULL;
	lol->uv_count = 0;
	lol->high_uv_id1 = NULL;
	lol->high_batch_count1 = NULL;
	lol->high_batch_first1 = NULL;
	lol->high_uv_id2 = NULL;
	lol->high_batch_count2 = NULL;
	lol->high_batch_first2 = NULL;
	lol->low_uv_id = NULL;
}

void InitUVChannelLib(UVChannel_lib *uvlib)
{
    uvlib->count = 0;
    uvlib->list = NULL;
}

void InitUVChannel(UVChannel *uvc)
{
    uvc->name=NULL;
    uvc->frame_count = 0;
    uvc->fps = -1;
    uvc->uv_chain = NULL;
    uvc->vertex_count = 0;
    uvc->vertex_lookup = NULL;
    uvc->interpolate = 1;
}

void FreeLodObject(lod_object *lod_obj)
{
  for(int i=0;i<lod_obj->count;i++)
  {
    FreeObject(&(lod_obj->obj_list[i]));
  }
  Free(lod_obj->obj_list);
  Free(lod_obj->switch_list);
//  lod_obj->tl = NULL;
  //lod_obj->atl = NULL;
  lod_obj->ml = NULL;

  if(verbose_level>=2)
  {
    printf("done w/ FreeLodObject\n");
  }
}

void FreeVertices(void *v)
{
  FreeVertices((vertices*)v);
}

void FreeVertices(vertices *v)
{
  //Free(v->object_chain);
  //Free(v->texture_chain);

  Free(v->object_list);
  Free(v->flags);
  Free(v->normal);
  Free(v->D_coefficient);
  Free(v->frozen_list);
  Free(v->color);
  Free(v->api_node_xyz_id);
  Free(v->api_xyz_id);
  Free(v->api_smg_id);
  
  if(v->b_v_list)
  {
	  for(int i=0; i<v->object_count; i++)
	  {
		  v->b_v_list[i].Release();
	  }
	  Free(v->b_v_list);
  }

  Free(v->vertex_bone_count);
  Free(v->first_vertex);
  Free(v->bone_id_list);
  Free(v->bone_weight_list);
  Free(v->bone_vertex_list);
  Free(v->bone_normal_list);
  if(v->err)
  {
    for(int i=0; i<v->object_count; i++)
    {
	  v->err[i].Release();
    }
    Free(v->err);
  }
  
  Free(v->object_batch_list);
  Free(v->texture_batch_list);
  Free(v->texture_batch_list2);
  Free(v->mtl_batch_list);
 
  Free(v->texture_list);
  Free(v->uv_flags);
  Free(v->api_node_uv_id);
  Free(v->api_uv_id);
  
  Free(v->uv_bone_id);
  Free(v->uv_vertex_count);
  Free(v->uv_plane_distance);
  Free(v->x_to_u_scale);
  Free(v->y_to_v_scale);
  Free(v->min_du);
  Free(v->max_du);
  Free(v->min_dv);
  Free(v->max_dv);
  Free(v->uv_vertex_id_list);
  Free(v->uv_list);
}

void FreeEdges(edges *e)
{
    Free(e->vertex_list);
	Free(e->angle);
}

void FreeEedgeLib(Eedge_lib *el)
{
    for(int i=0; i<el->max_count; i++)
    {
        FreeEedge(&(el->list[i]));
    }
    Free(el->list);
}

void FreeEedge(Eedge *ee)
{
    Free(ee->face_list);
	Free(ee->group_list);
	Free(ee->uv_pairs);
	Free(ee->uv_dest);
}

void FreeObject(object *obj)
{
  FreeFaceGroups(obj);

  FreeVertices(&(obj->v));
  FreeEdges(&(obj->e));
  FreeEedgeLib(&(obj->el));
  Free(obj->n.list);
  FreeNurbs(obj);
  FreeLight(&(obj->l));
  obj->b_mesh.Release();

#if USE_DA_MESH
  obj->ml = NULL;
  obj->tl = NULL; // owned by compound
  obj->atl = NULL;
#else
//  FreeMtlLib(obj->ml);
  Free(obj->ml);
#endif
  
  FreeLodLib(&(obj->lol));
  FreeUVChannelLib(&(obj->uvcl));
  
  Free(obj->hp_fix_list);
  Free(obj->hp_pris_list);
  Free(obj->hp_rev_list);

  obj->extents.convex_hull.free();

  FreeProps(obj);

  obj->da_mesh.Release();

  if(verbose_level >= 3){
    printf("done w/ FreeObject\n");
  }
}

void FreeProps(object *obj)
{
  for(unsigned int i = 0; i < obj->prop_hdr.propCount; i++)
  {
	  obj->prop_list[i].Release();
  }
  Free(obj->prop_list);

  Free(obj->bin_prop);
}

void FreeNurbs(object *obj)
{
	for(int i=0; i<obj->nurb_count; i++)
	{
		FreeNurb(&(obj->nurb_list[i]));
	}

	Free(obj->nurb_list);
}

void FreeNurb(nurb *nr)
{
	Free(nr->name);
	Free(nr->s_knot_list);
	Free(nr->t_knot_list);
	Free(nr->point_list);
	Free(nr->weight_list);
	Free(nr->api_s_id);
	Free(nr->api_t_id);
	Free(nr->uv_list);

	if(nr->b_v_list)
	{
		for(int i = 0; i < nr->s_point_count * nr->t_point_count; i++)
		{
			nr->b_v_list[i].Release();
		}
		Free(nr->b_v_list);
	}
	Free(nr->vertex_bone_count);
	Free(nr->first_vertex);
	Free(nr->bone_id_list);
	Free(nr->bone_weight_list);
	Free(nr->bone_vertex_list);
	Free(nr->bone_rational_list);
}

void FreeFaceGroups(object *obj)
{
    for(int i=0; i<obj->face_group_count; i++)
    {
        FreeFaceGroup(&(obj->face_group_list[i]));
    }

    Free(obj->face_group_list);
}

void FreeFaceGroup(face_group *fg)
{
    Free(fg->batch_chain);
    Free(fg->normal);
    Free(fg->D_coefficient);
    Free(fg->property);
    Free(fg->adjacent_face_list);
	Free(fg->quadric_list);
	Free(fg->api_node_id);
	Free(fg->api_face_id);
	Free(fg->api_smg_id);
}

void InitFaceGroup(face_group *fg)
{
    fg->material_id = -1;
    fg->count = 0;
	fg->max_count = 0;
    fg->batch_chain = NULL;
    fg->normal = NULL;
    fg->D_coefficient = NULL;
    fg->property = NULL;
    fg->adjacent_face_list = NULL;
	fg->quadric_list = NULL;

	fg->api_mtl_id = -1;
	fg->api_node_id = NULL;
	fg->api_face_id = NULL;
	fg->api_smg_id = NULL;
}

void FreeLodLib(lod_lib *lol)
{
    for(int i=0; i<lol->count; i++)
    {
        FreeLodStep(&(lol->step_list[i]));
    }

    Free(lol->step_list);
	Free(lol->uv_step_list);
	Free(lol->removed_face_list);
    Free(lol->face_list);
	Free(lol->group_list);
    Free(lol->high_normal_list);
    Free(lol->high_D_coefficient_list);
    Free(lol->low_normal_list);
    Free(lol->low_D_coefficient_list);
    Free(lol->vertex_list);
    Free(lol->tmp_edge_list);
    Free(lol->edge_list);

	Free(lol->batch_uv_id_chain);
	//Free(lol->batch_uv_id_chain2);
	Free(lol->high_uv_id1);
	Free(lol->high_batch_count1);
	Free(lol->high_batch_first1);
	Free(lol->high_uv_id2);
	Free(lol->high_batch_count2);
	Free(lol->high_batch_first2);
	Free(lol->low_uv_id);

	lol->count = 0;
}

void FreeLodStep(lod_step * /*los*/)
{
    return;
}

void FreeUVChannelLib(UVChannel_lib *uvlib)
{
    int i;
    for(i=0; i<uvlib->count; i++)
    {
        FreeUVChannel(&(uvlib->list[i]));
    }
    Free(uvlib->list);
}

void FreeUVChannel(UVChannel *uvc)
{
    Free(uvc->name);
    Free(uvc->uv_chain);
    Free(uvc->vertex_lookup);
}

float GetAngle(const float * const vec1, const float * const vec2)
{
  const double mag_sq = Magnitude3_sq(vec1) * Magnitude3_sq(vec2);
						
  if(mag_sq)
  {
	  const double cos_angle = Dot3(vec1, vec2) / sqrt(mag_sq);

	  if(cos_angle >= 1.0)
	  {
		  return 0.0f;
	  }else
	  if(cos_angle <= -1.0)
	  {
		  return (float)M_PI;
	  }
	 
	  return (float)acos(cos_angle); // range 0 to Pi
  }
  else
  {
	  return 0.0f;
  }
}

int InsertNormal(normals *nl, float n[3], const float tolerance /* in degrees*/)
{
  int id = -1;
  
  Normalize3(n);

  float max_cos_angle = -1.0f;
  const float cos_tolerance = (float)cos(tolerance * D2R);

  // find closest duplicate
  for(int i = 0; i < nl->count; i++)
  {
    const float cos_angle = Dot3(n, nl->list + 3 * i);

	if(cos_angle > max_cos_angle)
	{
		max_cos_angle = cos_angle;
		id = i;
	}
  }

  // duplicate
  if(max_cos_angle > cos_tolerance)
  {
       return id;
  }
 
  // new normal
  nl->count++;
  nl->list=(float*)Realloc(nl->list, nl->count*3*sizeof(float));
  nl->list[3*(nl->count-1)]=n[0];
  nl->list[3*(nl->count-1)+1]=n[1];
  nl->list[3*(nl->count-1)+2]=n[2];
  return (nl->count-1);
}

void SwapLodFaces(void *swap_lib, const int id1, const int id2)
{
	object *obj = (object*)swap_lib;
	const int g_id = obj->active_group;
	lod_lib *lol = &(obj->lol);

	face_group *fg = obj->face_group_list + g_id;
	Eedge_lib *el = &(obj->el);

	assert(id1 < fg->max_count && id2 < fg->max_count);
	assert(g_id < obj->face_group_count);

    if(id1 == id2) return;

	Swap32(fg->normal + id1, fg->normal + id2);
	Swap32(fg->D_coefficient + id1, fg->D_coefficient + id2);
	Swap32(fg->property + id1, fg->property + id2);

	Swap32(fg->batch_chain + 3*id1,     fg->batch_chain + 3*id2);
	Swap32(fg->batch_chain + 3*id1 + 1, fg->batch_chain + 3*id2 + 1);
	Swap32(fg->batch_chain + 3*id1 + 2, fg->batch_chain + 3*id2 + 2);

	Swap32(fg->api_node_id + id1, fg->api_node_id + id2);
	Swap32(fg->api_face_id + id1, fg->api_face_id + id2);
	Swap32(fg->api_smg_id + id1, fg->api_smg_id + id2);

	for(int e_id=0; e_id < el->max_count; e_id++)
	{
		Eedge *e = el->list + e_id;
		
		for(int fr_id=0; fr_id < e->face_count; fr_id++)
		{
			if(e->group_list[fr_id] == g_id)
			{
				if(e->face_list[fr_id] == id1)
				{
					e->face_list[fr_id] = id2;
				}
				else
				if(e->face_list[fr_id] == id2)
				{
					e->face_list[fr_id] = id1;
				}
			}
		}
	}

    if(lol->count)
    {
        for(int fid=0; fid < lol->face_count; fid++)
        {
			if(lol->group_list[fid] == g_id)
			{
				if(lol->face_list[fid] == id1)
				{
					lol->face_list[fid] = id2;
				}
				else
				if(lol->face_list[fid] == id2)
				{
					lol->face_list[fid] = id1;
				}
			}
        }
    }


	/*  never reindexed
	unsigned long *q1 = (unsigned long*)(fg->quadric_list + id1);
	unsigned long *q2 = (unsigned long*)(fg->quadric_list + id2);

	for(int i=0; i < sizeof(Matrix4)/sizeof(unsigned long); i++)
	{
		Swap32(q1, q2);
		q1++;
		q2++;
	}

	{
		for(int vid=0; vid < obj->v.object_count; vid++)
		{
			vertex_error *ve = obj->v.err + vid;

			for(int pi=0; pi < ve->plane_count; pi++)
			{
				if(ve->plane_list[pi] == f_id1)
				{
					ve->plane_list[pi] = f_id2;
				}
				else
				if(ve->plane_list[pi] == f_id2)
				{
					ve->plane_list[pi] = f_id1;
				}
			}
		}
	}
	*/
}

/*
float GetTriangleArea(const faces *f, const vertices *v, int id)
{
    int v_o_id, v_id1, v_id2, v_id3;
    float p1[3], p2[3], p3[3];
    
    v_o_id = f->initial_vertex[id];

    v_id1 = v->object_chain[v_o_id];
    v_id2 = v->object_chain[v_o_id+1];
    v_id3 = v->object_chain[v_o_id+2];

    p1[0] = v->object_list[3*v_id1];
    p1[1] = v->object_list[3*v_id1+1];
    p1[2] = v->object_list[3*v_id1+2];

    p2[0] = v->object_list[3*v_id2];
    p2[1] = v->object_list[3*v_id2+1];
    p2[2] = v->object_list[3*v_id2+2];

    p3[0] = v->object_list[3*v_id3];
    p3[1] = v->object_list[3*v_id3+1];
    p3[2] = v->object_list[3*v_id3+2];

    float ve1[3], ve2[3];//, ve3[3];

    ve1[0] = p1[0] - p2[0];
    ve1[1] = p1[1] - p2[1];
    ve1[2] = p1[2] - p2[2];

    ve2[0] = p2[0] - p3[0];
    ve2[1] = p2[1] - p3[1];
    ve2[2] = p2[2] - p3[2];

    //ve3[0] = p3[0] - p1[0];
    //ve3[1] = p3[1] - p1[1];
    //ve3[2] = p3[2] - p1[2];

    float cr_v[3];
    float area1; //, area2, area3;

    Cross(ve1, ve2, cr_v);
    area1 = .5f * Magnitude3(cr_v);

    //Cross(ve2, ve3, cr_v);
    //area2 = .5f * Magnitude3(cr_v);

    //Cross(ve3, ve1, cr_v);
    //area3 = .5f * Magnitude3(cr_v);

    return area1;
}
*/

/*
void AutoSmooth(object *obj, float thresh_angle)
// 54 Deg is smallest angle for flat box
{
	faces *f = &(obj->f);
	vertices *v = &(obj->v);
	normals *n = &(obj->n);

    int f_id, v_id, ref_v_id;

    // set all faces to smooth
    for(f_id=0; f_id<f->count; f_id++)
    {
        // smooth
        f->property[f_id] &= ~FLAT_SHADED;
        f->property[f_id] |= SMOOTH_SHADED;

        // flat
        //f->property[f_id] &= ~SMOOTH_SHADED;
        //f->property[f_id] |= FLAT_SHADED;
    }
//return;


    int changed = 1;
    while(changed)
    {
        changed = 0;
        calcVertexNormals(obj, 0); //SMOOTH_SHADED

        for(f_id=0; f_id<f->count; f_id++) // for each face
        {
            if( (f->property[f_id] & SMOOTH_SHADED) )
            {
                int v_start = f->initial_vertex[f_id];
                int v_count = f->vertex_count[f_id];

                for(ref_v_id = v_start; ref_v_id < v_start+v_count; ref_v_id++)
                {
                    v_id = v->object_chain[ref_v_id];
                    float angle = R2D * GetAngle(&(n->list[3*f->normal[f_id]]), 
                                                 &(n->list[3*v->normal[v_id]]));    
                    if(angle > thresh_angle)
                    {
                        f->property[f_id] &= ~SMOOTH_SHADED;
                        f->property[f_id] |= FLAT_SHADED;
                        changed = 1;
                        break;
                    }        
                }
            }
        }
    }
}
*/

int calcVertexNormal(object *obj, int type, int v_id, float vertex_normal[3])
{
	if(obj->type != FIXED_MESH && obj->type != DEF_MESH)
	{
		return -1;
	}

	int result = -1;

	assert(obj->v.normal);
	assert(obj->v.D_coefficient);

	vertex_normal[0] = 0.0f;
	vertex_normal[1] = 0.0f;
	vertex_normal[2] = 0.0f;
	
	float min_angle = FLT_MAX;
	float min_normal[3];
	int used_face_count = 0;

	
label_smooth:

	for(int g_id=0; g_id < obj->face_group_count; g_id++)
	{
		face_group *fg = obj->face_group_list + g_id;
		for(int f_id=0; f_id < fg->count; f_id++)
		{
			if((type != SMOOTH_SHADED) || (fg->property[f_id] & SMOOTH_SHADED))
			{
				int b_id1 = fg->batch_chain[3*f_id];
				int b_id2 = fg->batch_chain[3*f_id+1];
				int b_id3 = fg->batch_chain[3*f_id+2];

				int v_id1 = obj->v.object_batch_list[b_id1];
				int v_id2 = obj->v.object_batch_list[b_id2];
				int v_id3 = obj->v.object_batch_list[b_id3];

				if(v_id1 == v_id || v_id2 == v_id || v_id3 == v_id)
				{
					used_face_count++;
					float angle;
					if(v_id == v_id1)
					{
						angle = GetNewVertexAngle(obj, g_id, f_id, 0);
					}
					else
					if(v_id == v_id2)
					{
						angle = GetNewVertexAngle(obj, g_id, f_id, 1);
					}
					else
					{
						angle = GetNewVertexAngle(obj, g_id, f_id, 2);
					}

					if((angle > 0.0f) && (angle < 180.0f * D2R))
					{
						float normal[3];
						int n_succes = calcFaceGroupNormal(fg, &(obj->v), f_id, normal);
						if(n_succes != -1)
						{
							vertex_normal[0] += angle * normal[0];
							vertex_normal[1] += angle * normal[1];
							vertex_normal[2] += angle * normal[2];

							if(angle < min_angle)
							{
								min_angle = angle;
								min_normal[0] = angle * normal[0];
								min_normal[1] = angle * normal[1];
								min_normal[2] = angle * normal[2];
							}
						}
						else
						{
							fprintf(stderr, "Warning: bad face %d/%d normal!\n", g_id, f_id);
						}
					}
					else
					{
						fprintf(stderr, "Warning: triangle face w/ internal angle of %f\n", angle * R2D);
								//GetTriangleArea(f, v, f_id));
					}
				}
			}
		}
	}

	if(used_face_count == 0)
	{
		if(type == SMOOTH_SHADED)
		{
			type = 0;
			goto label_smooth; // try again
		}
		else
		{
#ifdef _DEBUG
			//Winprint("Error: vertex %d is NOT used by any faces!\n", v_id);
#endif
		}
	}
	else
	{
		float alt_normal[3] = {
			vertex_normal[0] - min_normal[0],
			vertex_normal[1] - min_normal[1],
			vertex_normal[2] - min_normal[2]};

		int n_success = Normalize3(vertex_normal);
		if(n_success != -1)
		{
			result = 0;
		}
		else
		{
			fprintf(stderr, "Warning: degenerate vertex normal! fixing...\n");

			vertex_normal[0] = alt_normal[0];
			vertex_normal[1] = alt_normal[1];
			vertex_normal[2] = alt_normal[2];

			int n_success2 = Normalize3(vertex_normal);
			if(n_success2 != -1)
			{
				result = 0;	
			}
			else
			{
				fprintf(stderr, "Warning: could NOT fix!\n");
			}
		}
	}
	
	return result;
}

void calcVertexNormals(object *obj, int type)
{
	if(obj->v.object_count < 1) return;

	vertices *v = &(obj->v);
	normals *n = &(obj->n);

	Free(v->normal);
	Free(v->D_coefficient);

	v->normal=(int*)Malloc(v->object_count*sizeof(int)); 
	v->D_coefficient=(float*)Malloc(v->object_count*sizeof(float)); 

	float normal[3];
	for(int v_id=0; v_id<v->object_count; v_id++)
	{
		calcVertexNormal(obj, type, v_id, normal);

		v->normal[v_id] = InsertNormal(n, normal, vertex_normal_tolerance);
  
		v->D_coefficient[v_id] = -Dot3(normal, &(v->object_list[3*v_id]));
	}
}

// smallest to largest
/*
int CompareUVBonesByID(const void *b1, const void *b2)
{
  if(*(int*)b1 > *(int*)b2)
  {
	  return 1;
  }
  else
  if(*(int*)b1 < *(int*)b2)
  {
	  return -1;
  }
  
  return 0;
}
*/
/*
void SwapUVBones(void *v, const int id1, const int id2)
{
	SwapUVBones((vertices*)v, id1, id2);
}
*/

/*
void SwapUVBones(vertices *v, const int id1, const int id2)
{
    if(id1 == id2) return;

	Swap32(v->uv_vertex_id_list + id1, v->uv_vertex_id_list + id2);
	Swap32(v->uv_bone_id_list + id1, v->uv_bone_id_list + id2);
	Swap32(v->uv_list + 2*id1, v->uv_list + 2*id2);
	Swap32(v->uv_list + 2*id1+1, v->uv_list + 2*id2+1);
	Swap32(v->uv_plane_distance + id1, v->uv_plane_distance + id2);
}
*/

void DumpLodStep(const lod_lib& lol, int step_id)
{
    assert(step_id < lol.count);

    const lod_step& los = lol.step_list[step_id];
    printf("Step: %d\n",step_id);
    printf("removed face count = %d\n", los.removed_face_count);
	printf("first removed face = %d\n", los.first_removed_face);

    printf("morphed face count = %d\n",los.morphed_face_count);
    printf("first morphed face = %d\n", los.first_morphed_face);

    printf("morphed vertex chain count = %d\n",los.morphed_vertex_chain_count);
    printf("first vertex = %d\n",los.first_vertex);
    printf("vid1 = %d vid2 = %d\n",los.vid1, los.vid2);
    printf("v1 = %.2f %.2f %.2f\n",los.v1.x, los.v1.y, los.v1.z);
    printf("v2 = %.2f %.2f %.2f\n",los.v2.x, los.v2.y, los.v2.z);
    printf("v3 = %.2f %.2f %.2f\n",los.v3.x, los.v3.y, los.v3.z);
    printf("v1_nid = %d v1_D = %.2f\n",los.v1_nid, los.v1_D);
    printf("removed edge count = %d\n", los.removed_edge_count);
    printf("morphed edge count = %d\n", los.morphed_edge_count);
    printf("first edge = %d\n", los.first_edge);
    printf("\n");
    
}

void DumpLodLib(const lod_lib& lol)
{
    
    int i;
    printf("LIB:\n");

	printf("removed face count = %d\n",lol.removed_face_count);
    printf("removed_face_list\n");
    for(i=0; i<lol.removed_face_count; i++)
    {
        printf("%d ",lol.removed_face_list[i]);
    }
    printf("\n");

    printf("face count = %d\n",lol.face_count);
    printf("face_list\n");
    for(i=0; i<lol.face_count; i++)
    {
        printf("%d ",lol.face_list[i]);
    }
    printf("\n");

	printf("group_list\n");
    for(i=0; i<lol.face_count; i++)
    {
        printf("%d ",lol.group_list[i]);
    }
    printf("\n");

    printf("normal_list\n");
    for(i=0; i<lol.face_count; i++)
    {
        printf("%d ",lol.high_normal_list[i]);
    }
    printf("\n");

    printf("D_coefficient_list\n");
    for(i=0; i<lol.face_count; i++)
    {
        printf("%.2f ",lol.high_D_coefficient_list[i]);
    }
    printf("\n");

    printf("vertex count = %d\n",lol.vertex_count);
    printf("vertex_list\n");
    for(i=0; i<lol.vertex_count; i++)
    {
        printf("%d ",lol.vertex_list[i]);
    }
    printf("\n");

    printf("edge count = %d\n",lol.tmp_edge_count);
    printf("edge_list\n");
    for(i=0; i<lol.tmp_edge_count; i++)
    {
        printf("v1=%d v2=%d, ",lol.tmp_edge_list[2*i], lol.tmp_edge_list[2*i+1]);    
    }
    printf("\n");

    printf("tmp edge count = %d\n",lol.edge_count);
    printf("edge_list\n");
    for(i=0; i<lol.edge_count; i++)
    {
        printf("e=%d, ",lol.edge_list[i]);
    }    
    printf("\n\n");
}

void PutEdgeLast(Eedge_lib *el, const int eid)
{
	if(eid < el->count - 1)
	{
		Eedge tmp ( el->list[eid] );
		memmove(el->list + eid, el->list + eid + 1, ((el->count - 1) - eid) * sizeof(*(el->list)));
		el->list[el->count - 1] = tmp;
	}
}

static void SwapEdges(void *el, const int id1, const int id2)
{
	SwapEdges((Eedge_lib*)el, id1, id2);
}

static void SwapEdges(Eedge_lib *el, const int id1, const int id2)
{
    if(id1 != id2)
	{
		MemSwap(el->list + id1, el->list + id2, sizeof(Eedge));
	}
}

// resolve edge indices based on vertex id's
void SynchLodLib(lod_lib *lol, const Eedge_lib& el)
{
    lol->edge_count = lol->tmp_edge_count;

    lol->edge_list = (int*)Realloc(lol->edge_list, lol->edge_count * sizeof(int));
    for(int e_id=0; e_id < lol->edge_count; e_id++)
    {
        lol->edge_list[e_id] = GetEdgeID(el, lol->tmp_edge_list[2*e_id], lol->tmp_edge_list[2*e_id+1], el.max_count, NULL);
        if(lol->edge_list[e_id] == -1)
        {
            assert(lol->edge_list[e_id] != -1);
        }
    }
}


void CollapseEdges(object *obj, int target_face_count)
{
	if(obj->face_count == target_face_count) return;

	TrapFpu(true);
	assert(obj->type == FIXED_MESH || obj->type == DEF_MESH);
	assert(obj->face_count > target_face_count);

#ifdef _DEBUG
	assert(obj->face_group_list);
	int fg_id;
	for(fg_id=0; fg_id<obj->face_group_count; fg_id++)
	{
		assert(obj->face_group_list[fg_id].max_count == obj->face_group_list[fg_id].count);
	}
	assert(obj->v.max_batch_count == obj->v.batch_count);
    assert(obj->v.max_object_count == obj->v.object_count);
    assert(obj->el.max_count == obj->el.count);
#endif

    calcEdgeErrors(obj); // sorts

	//checksorting(&(obj->el));

    //while((obj->face_count > target_face_count) && (obj->el.count >= 7) && (obj->face_count >= 6) &&
          //(obj->v.object_count >= 5) && (obj->el.list[obj->el.count - 1].error < FLT_MAX))
	while(obj->face_count > target_face_count)
    {
		RemoveLastEdge(obj);
	}

	SortFacesByArea(obj);
	//StreamlineVertices(obj);

	//checksorting(&(obj->el));

	// used for edges
    //SynchLodLib(&(obj->lol), obj->el);

								// def meshes don't get cont. LOD
	if((obj->lol.count > 0) && (obj->v.vertex_bone_count == NULL))
	{
		// necessary to create edge_chain and get counts back up
		SplitVertices(obj);

		AddUVSpaces(obj);

#ifdef _DEBUG
		for(fg_id=0; fg_id<obj->face_group_count; fg_id++)
		{
			assert(obj->face_group_list[fg_id].max_count == obj->face_group_list[fg_id].count);
		}
		assert(obj->v.max_batch_count == obj->v.batch_count);
		assert(obj->v.max_object_count == obj->v.object_count);
		assert(obj->el.max_count == obj->el.count);

		// sanity check
		for(int i=0; i < obj->lol.count; i++)
		{
			assert(obj->lol.step_list[i].vid1 < obj->lol.step_list[i].vid2);
		}
#endif

		ReverseStepList(&(obj->lol));
	}

	calcEdges(obj);
	calcEdgeAngles(obj);

	TrapFpu(false);
}

void ReverseStepList(lod_lib *lol)
{
	for(int i=0; i < lol->count / 2; i++)
	{
		MemSwap(lol->step_list + i, lol->step_list + (lol->count - 1) - i, sizeof(lod_step));
		MemSwap(lol->uv_step_list + i, lol->uv_step_list + (lol->count - 1) - i, sizeof(lod_uv_step));
	}
}

void AddUVSpaces(object *obj)
{
	if(obj->lol.uv_step_list && obj->lol.max_uv_per_step > 0)
	{
		const int shift = 2 * obj->lol.max_uv_per_step;

		obj->v.texture_count += shift;
		obj->v.texture_list = (float*)Realloc(obj->v.texture_list, 2 * obj->v.texture_count * sizeof(float));
	
		memmove(obj->v.texture_list + 2 * shift, obj->v.texture_list,
			2 * (obj->v.texture_count - shift) * sizeof(float));	
		memset(obj->v.texture_list, 0, 2 * shift * sizeof(float));

		for(int i = 0; i < obj->v.max_batch_count; i++)
		{
			obj->v.texture_batch_list[i] += shift;
		}		

		for(i = 0; i < obj->lol.uv_count; i++)
		{
			obj->lol.high_uv_id1[i] += shift;
			obj->lol.high_uv_id2[i] += shift;
			obj->lol.low_uv_id[i] += shift;
		}
	}
}

void SplitVertices(object *obj)
{
    for(int step=obj->lol.count-1; step >= 0; step--)
    {
        AddLastEdge(obj, step);
    }

    assert(obj->lol.edge_count == obj->lol.tmp_edge_count);
}

void AddLastEdge(object *obj, int step_id)
{
    vertices *v = &(obj->v);
    //faces *f = &(obj->f);
	face_group *group_list = obj->face_group_list;
    //normals *n = &(obj->n);
    lod_lib& lol = obj->lol;
    const lod_step & lstep = obj->lol.step_list[step_id];
	const lod_uv_step & uv_step = obj->lol.uv_step_list[step_id];
    int index;

    Eedge_lib *el = &(obj->el);

    // restore removed edges
    el->count += lstep.removed_edge_count;
    int me_id;
    for(me_id=0, index=lstep.first_edge; me_id<lstep.morphed_edge_count; me_id++, index++)
    {
        if((lol.tmp_edge_list[2*index]   == lstep.vid1) ||
           (lol.tmp_edge_list[2*index+1] == lstep.vid1))
        {
            int e_id = GetEdgeID(*el, lol.tmp_edge_list[2*index], lol.tmp_edge_list[2*index+1],
				el->max_count, NULL);
            assert(e_id != -1);
            lol.edge_count++;
            lol.edge_list = (int*)Realloc(lol.edge_list, lol.edge_count * sizeof(int));
            lol.edge_list[lol.edge_count-1] = e_id;

            if(el->list[e_id].v1 == lstep.vid1)
            {
                el->list[e_id].v1 = lstep.vid2;
            }
            else
            if(el->list[e_id].v2 == lstep.vid1)
            {
                el->list[e_id].v2 = lstep.vid2;
            }
            else    
            {
                Winprint("Error: edges out of synch!\n");
            }
        }
    }
    
#if 0
    // restore morphed edges
    int me_id;
    for(me_id=0, index=lstep.first_edge; me_id<lstep.morphed_edge_count; me_id++, index++)
    {
        if(el->list[ lol.edge_chain[index] ].vertices[0] == lstep.vid1)
        {
            el->list[ lol.edge_chain[index] ].vertices[0] = lstep.vid2;
        }
        else
        if(el->list[ lol.edge_chain[index] ].vertices[1] == lstep.vid1)
        {
            el->list[ lol.edge_chain[index] ].vertices[1] = lstep.vid2;
        }
        else
        {
            Winprint("Error: edges out of synch!\n");
        }
    }
#endif

    // restore morphed faces
    int mf_id;
    for(mf_id=0, index=lstep.first_morphed_face; mf_id < lstep.morphed_face_count; mf_id++, index++)
    {
        int face_id = lol.face_list[index];
		int g_id = lol.group_list[index];

		face_group *fg = group_list + g_id;

		assert(fg->normal[ face_id ] == lol.low_normal_list[index]);
        fg->normal[ face_id ] = lol.high_normal_list[index];

		assert(fg->D_coefficient[ face_id ] == lol.low_D_coefficient_list[index]);
        fg->D_coefficient[ face_id ] = lol.high_D_coefficient_list[index];
    }
    
	// restore removed batch vertices
	v->batch_count += lstep.removed_batch_vertex_count;

    // restore affected batch list indices
	int v_ref;
    for(v_ref=0, index=lstep.first_vertex; v_ref < lstep.morphed_vertex_chain_count; v_ref++, index++)
    {
		assert(lol.vertex_list[index] < v->batch_count);
        v->object_batch_list[ lol.vertex_list[index] ] = lstep.vid2;
    }

	// restore UV's
	for(int i = 0; i < uv_step.morphed_uv_count; i++)
	{
		for(int j = 0; j < lol.high_batch_count1[uv_step.first_morphed_uv + i]; j++)
		{
			const int bid = lol.batch_uv_id_chain[ lol.high_batch_first1[uv_step.first_morphed_uv + i] + j ];
			assert(bid < v->batch_count);
			v->texture_batch_list[ bid ] = lol.high_uv_id1[ uv_step.first_morphed_uv + i ];
		}

		for(j = 0; j < lol.high_batch_count2[uv_step.first_morphed_uv + i]; j++)
		{
			const int bid = lol.batch_uv_id_chain[ lol.high_batch_first2[uv_step.first_morphed_uv + i] + j ];
			assert(bid < v->batch_count);
			v->texture_batch_list[ bid ] = lol.high_uv_id2[ uv_step.first_morphed_uv + i ];
		}

		/*
		const int bid2 = lol.batch_uv_id_chain2[ uv_step.first_morphed_uv + i];
		assert(bid2 < v->batch_count);
		v->texture_batch_list[ bid2 ] = lol.high_uv_id2[ uv_step.first_morphed_uv + i ];
		
		const int bid1 = lol.batch_uv_id_chain1[ uv_step.first_morphed_uv + i];
		assert(bid1 < v->batch_count);
		v->texture_batch_list[ bid1 ] = lol.high_uv_id1[ uv_step.first_morphed_uv + i ];
		*/
	}

    // restore vertex 1
    v->object_list[3*lstep.vid1] = lstep.v1.x;
    v->object_list[3*lstep.vid1+1] = lstep.v1.y;
    v->object_list[3*lstep.vid1+2] = lstep.v1.z;
    v->normal[lstep.vid1] = lstep.v1_nid;
    v->D_coefficient[lstep.vid1] = lstep.v1_D;
    v->object_count++;

	// restore face count
	int f_ref;
	for(f_ref=0, index=lstep.first_removed_face; f_ref < lstep.removed_face_count; f_ref++, index++)
    {
		int g_id = lol.removed_face_list[index];
		face_group *fg = group_list + g_id;

		fg->count++;
		obj->face_count++;
    }
}

/*
struct holder
{
	int count1;	// batch id's
	int *list1;
	int count2;
	int *list2;

	int count3;	// faces
	int *list3;

	inline void Init(void)
	{
		count1 = 0;
		list1 = NULL;

		count2 = 0;
		list2 = NULL;

		count3 = 0;
		list3 = NULL;
	}

	inline void Release(void)
	{
		Free(list1);
		count1 = 0;

		Free(list2);
		count2 = 0;

		Free(list3);
		count3 = 0;
	}

	void AddUnique(const int v1, const int v2, const int fid)
	{
		if(v1 >= 0)
		{
			bool duplicate = false;
			for(int i = 0; i < count1; i++)
			{
				if(list1[i] == v1)
				{
					duplicate = true;
					break;
				}
			}

			if(!duplicate)
			{
				count1++;
				list1 = (int*)Realloc(list1, count1 * sizeof(int));
				list1[count1 - 1] = v1;
			}
		}

		if(v2 >= 0)
		{
			bool duplicate = false;
			for(int i = 0; i < count2; i++)
			{
				if(list2[i] == v2)
				{
					duplicate = true;
					break;
				}
			}

			if(!duplicate)
			{
				count2++;
				list2 = (int*)Realloc(list2, count2 * sizeof(int));
				list2[count2 - 1] = v2;
			}
		}

		if(fid >= 0)
		{
			bool duplicate = false;
			for(int i = 0; i < count3; i++)
			{
				if(list3[i] == fid)
				{
					duplicate = true;
					break;
				}
			}

			if(!duplicate)
			{
				count3++;
				list3 = (int*)Realloc(list3, count3 * sizeof(int));
				list3[count3 - 1] = fid;
			}
		}
	}

};

struct holder_list
{
	int count;
	holder *list;
	int *key1;
	int *key2;

	holder_list(void)
	{
		memset(this, 0, sizeof(*this));
	}

	~holder_list()
	{
		for(int i = 0; i < count; i++)
		{
			list[i].Release();
		}

		Free(list);
		Free(key1);
		Free(key2);

		count = 0;
	}

	void AddUnique(const int k1, const int k2, const int v1, const int v2, const int fid)
	{
		for(int i = 0; i < count; i++)
		{
			if(key1[i] == k1 && key2[i] == k2)
			{
				list[i].AddUnique(v1, v2, fid);
				return;
			}
			else
			if(key1[i] == k2 && key2[i] == k1)
			{
				list[i].AddUnique(v2, v1, fid);
				return;
			}
		}

		assert(v1 >= 0 && v2 >= 0);

		count++;
		list = (holder*)Realloc(list, count * sizeof(holder));
		list[count - 1].Init();
		list[count - 1].AddUnique(v1, v2, fid);

		key1 = (int*)Realloc(key1, count * sizeof(int));
		key1[count - 1] = k1;
		key2 = (int*)Realloc(key2, count * sizeof(int));
		key2[count - 1] = k2;
	}

};
*/

void RemoveLastEdge(object *obj)
{
    vertices * const v = &(obj->v);
	const int group_count = obj->face_group_count;
	face_group * const group_list = obj->face_group_list;
	
    normals *n = &(obj->n);
    Eedge_lib *el = &(obj->el);
	const Eedge & e = el->list[el->count - 1];

	// save lod data in these
    lod_lib *lol = &(obj->lol);
    lod_step lstep;
	lstep.Init();

    const int v_id1 = (e.v1 < e.v2)
                       ? e.v1 : e.v2;
          int v_id2 = (e.v1 < e.v2)
                       ? e.v2 : e.v1;

    assert(v_id1 != v_id2);

    // swap second vertex for last vertex & reindex object
    SwapVertices(obj, v_id2, v->object_count - 1); 

    v_id2 = v->object_count - 1;
    
    assert(v_id1 != v_id2);

    // store v_id1 & v_id2 data in lod_step
    lstep.vid1 = v_id1;
    lstep.v1.x = v->object_list[3*v_id1];
    lstep.v1.y = v->object_list[3*v_id1+1];
    lstep.v1.z = v->object_list[3*v_id1+2];

    lstep.vid2 = v_id2;
    lstep.v2.x = v->object_list[3*v_id2];
    lstep.v2.y = v->object_list[3*v_id2+1];
    lstep.v2.z = v->object_list[3*v_id2+2];
	
	// assign new vertex position
	v->object_list[3*v_id1  ] = lstep.v3.x = e.v_dest.x;
	v->object_list[3*v_id1+1] = lstep.v3.y = e.v_dest.y;
	v->object_list[3*v_id1+2] = lstep.v3.z = e.v_dest.z;

	// sum up freeze weights
	v->frozen_list[3*v_id1] = Clamp01( new_lock( v->frozen_list[3*v_id1], v->frozen_list[3*v_id2] ) );
	v->frozen_list[3*v_id1+1] = Clamp01( new_lock( v->frozen_list[3*v_id1+1], v->frozen_list[3*v_id2+1] ) );
	v->frozen_list[3*v_id1+2] = Clamp01( new_lock( v->frozen_list[3*v_id1+2], v->frozen_list[3*v_id2+2] ) ); 
	//v->frozen_list[3*v_id1] += (1.0f - v->frozen_list[3*v_id1]) * v->frozen_list[3*v_id2];
	//v->frozen_list[3*v_id1+1] += (1.0f - v->frozen_list[3*v_id1+1]) * v->frozen_list[3*v_id2+1];
	//v->frozen_list[3*v_id1+2] += (1.0f - v->frozen_list[3*v_id1+2]) * v->frozen_list[3*v_id2+2];
	
    // store normal index & D of v_id1 in step
    lstep.v1_nid = v->normal[v_id1];
    lstep.v1_D = v->D_coefficient[v_id1];

    lstep.v2_nid = v->normal[v_id2];
    lstep.v2_D = v->D_coefficient[v_id2];
    
	v->err[v_id1].quadric = MergeQuadrics(obj, v_id1, v_id2, true, NULL);
	
	// deformable stuff
	if(v->vertex_bone_count != NULL)
	{
		CombineDefVertices(obj, v_id1, v_id2);
	}

	v->object_count--;

	bool *potential_batch_list = (bool*)Malloc(v->batch_count * sizeof(bool));
	memset(potential_batch_list, 0, v->batch_count * sizeof(bool));

	//int *uv_batch1 = (int*)Malloc(v->batch_count * sizeof(int));
	//memset(uv_batch1, 0xff, v->batch_count * sizeof(int)); // set to -1

	//holder_list uv_holder;

	// move deleted faces to the end
	for(int g_id=0; g_id < group_count; g_id++)
	{
		face_group *fg = group_list + g_id;
		for(int f_id = 0; f_id < fg->count; f_id++)
		{
f_label:
			const int bid1 = fg->batch_chain[3*f_id];
			const int bid2 = fg->batch_chain[3*f_id + 1];
			const int bid3 = fg->batch_chain[3*f_id + 2];

			const int f_vid1 = v->object_batch_list[ bid1 ];
			const int f_vid2 = v->object_batch_list[ bid2 ];
			const int f_vid3 = v->object_batch_list[ bid3 ];

	
			// batch indices that point to interpolated uv's
			int uv_b_id1 = -1;
			int uv_b_id2 = -1;

			if( f_vid1 == v_id1 && f_vid2 == v_id2 )
			{
				uv_b_id1 = bid1;
				uv_b_id2 = bid2;
			}else
			if( f_vid1 == v_id2 && f_vid2 == v_id1 )
			{
				uv_b_id1 = bid2;
				uv_b_id2 = bid1;
			}
			else
			if( f_vid1 == v_id1 && f_vid3 == v_id2 )
			{
				uv_b_id1 = bid1;
				uv_b_id2 = bid3;
			}else
			if( f_vid1 == v_id2 && f_vid3 == v_id1 )
			{
				uv_b_id1 = bid3;
				uv_b_id2 = bid1;
			}else
			if( f_vid2 == v_id1 && f_vid3 == v_id2 )
			{
				uv_b_id1 = bid2;
				uv_b_id2 = bid3;
			}else
			if( f_vid2 == v_id2 && f_vid3 == v_id1 )
			{
				uv_b_id1 = bid3;
				uv_b_id2 = bid2;
			}

			if(uv_b_id1 != -1)
			{
				assert(uv_b_id2 != -1);
				assert(uv_b_id1 != uv_b_id2);

				/*
				const mtl *m1 = obj->ml->cq2Materials[ v->mtl_batch_list[uv_b_id1] ];
				const mtl *m2 = obj->ml->cq2Materials[ v->mtl_batch_list[uv_b_id2] ];

				if(m1 == m2)
				{
					uv_holder.AddUnique(
						v->texture_batch_list[uv_b_id1],
						v->texture_batch_list[uv_b_id2],
						uv_b_id1, 
						uv_b_id2,
						fg->count - 1); // assumes swap below
				}
				*/

				obj->active_group = g_id;
				SwapLodFaces(obj, f_id, fg->count - 1);

				potential_batch_list[ fg->batch_chain[ 3 * (fg->count - 1) ] ] = true;
				potential_batch_list[ fg->batch_chain[ 3 * (fg->count - 1) + 1] ] = true;
				potential_batch_list[ fg->batch_chain[ 3 * (fg->count - 1) + 2] ] = true;

				fg->count--;
				obj->face_count--;

				// store the group id of the removed face
				if(lstep.first_removed_face == -1)
				{
					lstep.first_removed_face = lol->removed_face_count;
				}

				lstep.removed_face_count++;
				lol->removed_face_count++;
				lol->removed_face_list = (int*)Realloc(lol->removed_face_list,
					lol->removed_face_count * sizeof(int));
				lol->removed_face_list[lol->removed_face_count - 1] = g_id;

                if(f_id != fg->count)
                {
                    goto f_label;
                }
                else // face is last already so we are done
                {
                    break;
                }
			}
		}
	}


	// UV stuff
	lod_uv_step uv_step;
	uv_step.Init();

#if 1
	int uv_b_count1;
	int uv_b_count2;
	int *uv_bid1 = NULL;
	int *uv_bid2 = NULL;
	for(int pid = 0; pid < e.uv_pair_count; pid++)
	{
		uv_b_count1 = 0;
		uv_b_count2 = 0;

		const int uvid1 = e.uv_pairs[pid].uvid1;
		const int uvid2 = e.uv_pairs[pid].uvid2;
		//assert(uvid1 != uvid2);
		const cq2Mtl *m = e.uv_pairs[pid].m;

		// get batch indices for this uv pair
		for(int bid = 0; bid < v->batch_count; bid++)
		{	
			if(v->object_batch_list[bid] == v_id1 && obj->ml->cq2Materials[v->mtl_batch_list[bid]] == m)
			{
				if(v->texture_batch_list[bid] == uvid1)
				{
					uv_b_count1++;
					uv_bid1 = (int*)Realloc(uv_bid1, uv_b_count1 * sizeof(int));
					uv_bid1[uv_b_count1 - 1] = bid;
				}
			}
			else
			if(v->object_batch_list[bid] == v_id2 && obj->ml->cq2Materials[v->mtl_batch_list[bid]] == m)
			{
				if(v->texture_batch_list[bid] == uvid2)// || v->texture_batch_list[bid] == uvid1)
				{
					uv_b_count2++;
					uv_bid2 = (int*)Realloc(uv_bid2, uv_b_count2 * sizeof(int));
					uv_bid2[uv_b_count2 - 1] = bid;
				}
			}
		}

		
		// insert lod uv pair
		if(uv_step.first_morphed_uv == -1)
		{
			assert(uv_step.morphed_uv_count == 0);
			uv_step.first_morphed_uv = lol->uv_count;
		}
		uv_step.morphed_uv_count++;

		lol->uv_count++;
		

		// insert new uv
		lol->low_uv_id = (int*)Realloc(lol->low_uv_id, lol->uv_count * sizeof(int));
		lol->low_uv_id[lol->uv_count - 1] = InsertUVVertex(v, &(e.uv_dest[2*pid]), 0, -1, 0);

	
		lol->high_uv_id1 = (int*)Realloc(lol->high_uv_id1 , lol->uv_count * sizeof(int));
		lol->high_uv_id1[lol->uv_count - 1] = uvid1;

		lol->high_batch_count1 = (int*)Realloc(lol->high_batch_count1 , lol->uv_count * sizeof(int));
		lol->high_batch_count1[lol->uv_count - 1] = 0;
		lol->high_batch_first1 = (int*)Realloc(lol->high_batch_first1 , lol->uv_count * sizeof(int));
		lol->high_batch_first1[lol->uv_count - 1] = -1;

		for(int j = 0; j < uv_b_count1; j++)
		{
			// assign new uv
			v->texture_batch_list[ uv_bid1[j] ] = lol->low_uv_id[lol->uv_count - 1];

			// store changed batch locations
			if(lol->high_batch_first1[lol->uv_count - 1] == -1)
			{
				lol->high_batch_first1[lol->uv_count - 1] = lol->uv_chain_count;
			}
			lol->high_batch_count1[lol->uv_count - 1]++;

			lol->uv_chain_count++;
			lol->batch_uv_id_chain = (int*)Realloc(lol->batch_uv_id_chain, lol->uv_chain_count * sizeof(int));
			lol->batch_uv_id_chain[lol->uv_chain_count - 1] = uv_bid1[j];
		}
		assert(lol->high_batch_count1[lol->uv_count - 1] == uv_b_count1);


		lol->high_uv_id2 = (int*)Realloc(lol->high_uv_id2 , lol->uv_count * sizeof(int));
		lol->high_uv_id2[lol->uv_count - 1] = uvid2;

		lol->high_batch_count2 = (int*)Realloc(lol->high_batch_count2 , lol->uv_count * sizeof(int));
		lol->high_batch_count2[lol->uv_count - 1] = 0;
		lol->high_batch_first2 = (int*)Realloc(lol->high_batch_first2 , lol->uv_count * sizeof(int));
		lol->high_batch_first2[lol->uv_count - 1] = -1;

		for(j = 0; j < uv_b_count2; j++)
		{
			// assign new uv
			v->texture_batch_list[ uv_bid2[j] ] = lol->low_uv_id[lol->uv_count - 1];

			// store changed batch locations
			if(lol->high_batch_first2[lol->uv_count - 1] == -1)
			{
				lol->high_batch_first2[lol->uv_count - 1] = lol->uv_chain_count;
			}
			lol->high_batch_count2[lol->uv_count - 1]++;

			lol->uv_chain_count++;
			lol->batch_uv_id_chain = (int*)Realloc(lol->batch_uv_id_chain, lol->uv_chain_count * sizeof(int));
			lol->batch_uv_id_chain[lol->uv_chain_count - 1] = uv_bid2[j];
		}
		assert(lol->high_batch_count2[lol->uv_count - 1] == uv_b_count2);
	}

	// max two per face belonging to the collapsed edge
	if(e.uv_pair_count > lol->max_uv_per_step)
	{
		lol->max_uv_per_step = e.uv_pair_count;
	}
	assert(uv_step.morphed_uv_count == e.uv_pair_count);

	Free(uv_bid1);
	Free(uv_bid2);

#else

// using holder
// TODO: use when computing error  (v3 outside of v1 or v2 should be penalized)
	// only use motion along edge
	const float dist = (lstep.v2 - lstep.v1).magnitude();
	const float d1 = dot_product( (lstep.v3 - lstep.v1), (lstep.v2 - lstep.v1) / dist );
	const float d2 = dot_product( (lstep.v3 - lstep.v2), (lstep.v1 - lstep.v2) / dist );

	float fraction2 = d1 / dist;
	float fraction1 = d2 / dist;

	float inv_mag = 1.0f / (fraction1 + fraction2);
	fraction1 *= inv_mag;
	fraction2 *= inv_mag;

	// clamp
	if(fraction1 > 1.0f)
	{
		fraction1 = 1.0f;
	}
	else
	if(fraction1 < 0.0f)
	{
		fraction1 = 0.0f;
	}

	if(fraction2 > 1.0f)
	{
		fraction2 = 1.0f;
	}
	else
	if(fraction2 < 0.0f)
	{
		fraction2 = 0.0f;
	}

	inv_mag = 1.0f / (fraction1 + fraction2);
	fraction1 *= inv_mag;
	fraction2 *= inv_mag;

	// sweep for additional batch indices since batch list may not be unique once some LOD has been done
	for(int i = 0; i < uv_holder.count; i++)
	{
		holder & h = uv_holder.list[i];
		const int uvid1 = uv_holder.key1[i];
		const int uvid2 = uv_holder.key2[i];
		//assert(uvid1 != uvid2);

		const cq2Mtl *m1 = obj->ml->cq2Materials[ v->mtl_batch_list[h.list1[0]] ];
		const cq2Mtl *m2 = obj->ml->cq2Materials[ v->mtl_batch_list[h.list2[0]] ];

		for(int j = 0; j < v->batch_count; j++)
		{
			if( v->texture_batch_list[j] == uvid1 &&
				v->object_batch_list[j] == v_id1 &&
				obj->ml->cq2Materials[ v->mtl_batch_list[j] ] == m1)
			{
				h.AddUnique(j, -1, -1);
				/*
				const mtl *tmp_m = obj->ml->materials[ v->mtl_batch_list[j] ];
				if( (m1->diffuse.texture_name && tmp_m->diffuse.texture_name &&
					 !strcmp(m1->diffuse.texture_name, tmp_m->diffuse.texture_name)) ||
					(m1->diffuse.anim_texture_name && tmp_m->diffuse.anim_texture_name &&
					 !strcmp(m1->diffuse.anim_texture_name, tmp_m->diffuse.anim_texture_name)) )
				{
					h.AddUnique(j, -1, -1);
				}
				*/
			}
			else
			if( v->texture_batch_list[j] == uvid2 &&
				v->object_batch_list[j] == v_id2 &&
				obj->ml->cq2Materials[ v->mtl_batch_list[j] ] == m2)
			{
				h.AddUnique(-1, j, -1);
				/*
				const mtl *tmp_m = obj->ml->materials[ v->mtl_batch_list[j] ];
				if( (m2->diffuse.texture_name && tmp_m->diffuse.texture_name &&
					 !strcmp(m2->diffuse.texture_name, tmp_m->diffuse.texture_name)) ||
					(m2->diffuse.anim_texture_name && tmp_m->diffuse.anim_texture_name &&
					 !strcmp(m2->diffuse.anim_texture_name, tmp_m->diffuse.anim_texture_name)) )
				{
					h.AddUnique(-1, j, -1);
				}
				*/
			}
		}
	}
	
	for(i = 0; i < uv_holder.count; i++)
	{
		if(uv_step.first_morphed_uv == -1)
		{
			assert(uv_step.morphed_uv_count == 0);
			uv_step.first_morphed_uv = lol->uv_count;
		}
		uv_step.morphed_uv_count++;

		lol->uv_count++;
		
		const int uvid1 = uv_holder.key1[i];
		const int uvid2 = uv_holder.key2[i];

		// compute new uv
		const float uv[2] = {
				fraction1 * v->texture_list[2*uvid1    ] + fraction2 * v->texture_list[2*uvid2    ],
				fraction1 * v->texture_list[2*uvid1 + 1] + fraction2 * v->texture_list[2*uvid2 + 1] };

		lol->low_uv_id = (int*)Realloc(lol->low_uv_id, lol->uv_count * sizeof(int));
		lol->low_uv_id[lol->uv_count - 1] = InsertUVVertex(v, uv, 0, -1, 0);

	
		const holder & h = uv_holder.list[i];

		lol->high_uv_id1 = (int*)Realloc(lol->high_uv_id1 , lol->uv_count * sizeof(int));
		lol->high_uv_id1[lol->uv_count - 1] = uvid1;

		lol->high_batch_count1 = (int*)Realloc(lol->high_batch_count1 , lol->uv_count * sizeof(int));
		lol->high_batch_count1[lol->uv_count - 1] = 0;
		lol->high_batch_first1 = (int*)Realloc(lol->high_batch_first1 , lol->uv_count * sizeof(int));
		lol->high_batch_first1[lol->uv_count - 1] = -1;
		for(int j = 0; j < h.count1; j++)
		{
			// assign new uv
			v->texture_batch_list[ h.list1[j] ] = lol->low_uv_id[lol->uv_count - 1];

			// store changed batch locations
			if(lol->high_batch_first1[lol->uv_count - 1] == -1)
			{
				lol->high_batch_first1[lol->uv_count - 1] = lol->uv_chain_count;
			}
			lol->high_batch_count1[lol->uv_count - 1]++;

			lol->uv_chain_count++;
			lol->batch_uv_id_chain = (int*)Realloc(lol->batch_uv_id_chain, lol->uv_chain_count * sizeof(int));
			lol->batch_uv_id_chain[lol->uv_chain_count - 1] = h.list1[j];
		}
		assert(lol->high_batch_count1[lol->uv_count - 1] == h.count1);


		lol->high_uv_id2 = (int*)Realloc(lol->high_uv_id2 , lol->uv_count * sizeof(int));
		lol->high_uv_id2[lol->uv_count - 1] = uvid2;

		lol->high_batch_count2 = (int*)Realloc(lol->high_batch_count2 , lol->uv_count * sizeof(int));
		lol->high_batch_count2[lol->uv_count - 1] = 0;
		lol->high_batch_first2 = (int*)Realloc(lol->high_batch_first2 , lol->uv_count * sizeof(int));
		lol->high_batch_first2[lol->uv_count - 1] = -1;
		for(j = 0; j < h.count2; j++)
		{
			// assign new uv
			v->texture_batch_list[ h.list2[j] ] = lol->low_uv_id[lol->uv_count - 1];

			// store changed batch locations
			if(lol->high_batch_first2[lol->uv_count - 1] == -1)
			{
				lol->high_batch_first2[lol->uv_count - 1] = lol->uv_chain_count;
			}
			lol->high_batch_count2[lol->uv_count - 1]++;

			lol->uv_chain_count++;
			lol->batch_uv_id_chain = (int*)Realloc(lol->batch_uv_id_chain, lol->uv_chain_count * sizeof(int));
			lol->batch_uv_id_chain[lol->uv_chain_count - 1] = h.list2[j];
		}
		assert(lol->high_batch_count2[lol->uv_count - 1] == h.count2);
	}
	assert(uv_step.morphed_uv_count == uv_holder.count);

	// max two per face belonging to the collapsed edge
	if(uv_holder.count > lol->max_uv_per_step)
	{
		lol->max_uv_per_step = uv_holder.count;
	}
#endif

	// remove batch indices
	for(int pbid = v->batch_count - 1; pbid >= 0; pbid--)
	{
		if(potential_batch_list[pbid])
		{
			if( IsBatchUsed_N_or_Less_Times(obj, pbid, 0) )
			{
				SwapBatchIndecies(obj, pbid, v->batch_count - 1);

				/*
				for(int i = 0; i < uv_holder.count; i++)
				{
					holder & h = uv_holder.list[i];

					for(int j = 0; j < h.count1; j++)
					{
						if(h.list1[j] == pbid)
						{
							h.list1[j] = v->batch_count - 1;
						}else
						if(h.list1[j] == v->batch_count - 1)
						{
							h.list1[j] = pbid;
						}
					}

					for(j = 0; j < h.count2; j++)
					{
						if(h.list2[j] == pbid)
						{
							h.list2[j] = v->batch_count - 1;
						}else
						if(h.list2[j] == v->batch_count - 1)
						{
							h.list2[j] = pbid;
						}
					}
				}
				*/
				
				v->batch_count--;
				lstep.removed_batch_vertex_count++;
			}
		}
	}

	Free(potential_batch_list);

	// make every reference to v_id2 point to v_id1 & store those changed in lod_step
	for(int vb_id=0; vb_id<v->batch_count; vb_id++)
	{
		if(v->object_batch_list[vb_id] == v_id2)
		{
			// save
            if(lstep.first_vertex == -1)
            {
                lstep.first_vertex = lol->vertex_count;
            }
            lol->vertex_count++;
            lol->vertex_list = (int*)Realloc(lol->vertex_list, lol->vertex_count * sizeof(int));
            lol->vertex_list[lol->vertex_count-1] = vb_id;

            lstep.morphed_vertex_chain_count++;
			
			// update
			v->object_batch_list[vb_id] = v_id1;
		}
	}


	//bool removed_batch_flag = false;
	//memset(potential_batch_list, 0, v->batch_count * sizeof(bool));

	// handle morphed faces (a few may degenerate but we need to keep them in case they stretch back out)
	for(int fg_id=0; fg_id < group_count; fg_id++)
	{
		face_group *fg = group_list + fg_id;

		for(int f_id=0; f_id < fg->count; f_id++)
		{
//f2_label:
			int sanity_flag = 0;
			for(int i = 0; i < 3; i++)
			{
				int b_id = fg->batch_chain[3*f_id + i];
				if( v->object_batch_list[b_id] == v_id1 )
				{
					sanity_flag++; // count only one vertex per triangle got affected

					// calc new normal
					float normal[3];
					int n_succes = calcFaceGroupNormal(fg, v, f_id, normal);

					if(n_succes == -1) // degenerate normal so we just keep it the same
					{
						// I would like to get a test object that does this
						fprintf(stderr, "Warning: bad face normal due to lod morphing!\n");

						int tmp_nid = fg->normal[f_id];
						normal[0] = n->list[3*tmp_nid];
						normal[1] = n->list[3*tmp_nid + 1];
						normal[2] = n->list[3*tmp_nid + 2];
					}
/*
					// check for 0 area
					bool zero_area = false;
					if(n_succes != -1)
					{
						// handle degenerate faces (lines)
						int vb_id1 = fg->batch_chain[3*f_id];
						int vb_id2 = fg->batch_chain[3*f_id+1];
						int vb_id3 = fg->batch_chain[3*f_id+2];

						vb_id1 = v->object_batch_list[vb_id1];
						vb_id2 = v->object_batch_list[vb_id2];
						vb_id3 = v->object_batch_list[vb_id3];

						float tmp_vl[9];
						tmp_vl[0] = v->object_list[3*vb_id1];
						tmp_vl[1] = v->object_list[3*vb_id1+1];
						tmp_vl[2] = v->object_list[3*vb_id1+2];
						tmp_vl[3] = v->object_list[3*vb_id2];
						tmp_vl[4] = v->object_list[3*vb_id2+1];
						tmp_vl[5] = v->object_list[3*vb_id2+2];
						tmp_vl[6] = v->object_list[3*vb_id3];
						tmp_vl[7] = v->object_list[3*vb_id3+1];
						tmp_vl[8] = v->object_list[3*vb_id3+2];
					
						for(int vi=0; vi < 3; vi++)
						{
							float angle = GetVertexAngle(tmp_vl, vi);
							if(angle <= 0.0f || angle >= 180.0f*D2R)
							{
								zero_area = true;
								break;
							}
						}
					}

					if(n_succes == -1 || zero_area) // degenerate face (say a line) so remove
					{
						obj->active_group = fg_id;
						SwapLodFaces(obj, f_id, fg->count - 1);

						removed_batch_flag = true;
						potential_batch_list[ fg->batch_chain[ 3 * (fg->count - 1) ] ] = true;
						potential_batch_list[ fg->batch_chain[ 3 * (fg->count - 1) + 1] ] = true;
						potential_batch_list[ fg->batch_chain[ 3 * (fg->count - 1) + 2] ] = true;

						fg->count--;
						obj->f.count--;

						// store the group id of the removed face
						if(lstep.first_removed_face == -1)
						{
							lstep.first_removed_face = lol->removed_face_count;
						}

						lstep.removed_face_count++;
						lol->removed_face_count++;
						lol->removed_face_list = (int*)Realloc(lol->removed_face_list,
							lol->removed_face_count * sizeof(int));
						lol->removed_face_list[lol->removed_face_count - 1] = fg_id;

						if(f_id != fg->count)
						{
							goto f2_label;
						}
						else // face is last already so we are done
						{
							break;
						}
					}
					else // morphed
*/
					{
						// save morphed
						if(lstep.first_morphed_face == -1)
						{
							lstep.first_morphed_face = lol->face_count;
						}
		
						lstep.morphed_face_count++;

						lol->face_count++;

						lol->face_list = (int*)Realloc(lol->face_list, lol->face_count * sizeof(int));
						lol->face_list[lol->face_count-1] = f_id;

						lol->group_list = (int*)Realloc(lol->group_list, lol->face_count * sizeof(int));
						lol->group_list[lol->face_count-1] = fg_id;

						lol->high_normal_list = (int*)Realloc(lol->high_normal_list, lol->face_count*sizeof(int));
						lol->high_normal_list[lol->face_count-1] = fg->normal[f_id];

						lol->high_D_coefficient_list = (float*)Realloc(lol->high_D_coefficient_list, 
							lol->face_count * sizeof(float));
						lol->high_D_coefficient_list[lol->face_count-1] = fg->D_coefficient[f_id];


						// update
						fg->normal[f_id] = InsertNormal(n, normal, 1.5f * face_normal_tolerance);

						fg->D_coefficient[f_id] = 
							(-Dot3(normal, 
								   &(v->object_list[ 3 * v->object_batch_list[ fg->batch_chain[3*f_id  ] ] ])) -
							  Dot3(normal, 
								   &(v->object_list[ 3 * v->object_batch_list[ fg->batch_chain[3*f_id+1] ] ])) -
							  Dot3(normal, 
								   &(v->object_list[ 3 * v->object_batch_list[ fg->batch_chain[3*f_id+2] ] ])) ) *
								   (1.0 / 3.0);

						// store new normal index
						lol->low_normal_list = (int*)Realloc(lol->low_normal_list, lol->face_count*sizeof(int));
						lol->low_normal_list[lol->face_count-1] = fg->normal[f_id];

						lol->low_D_coefficient_list = (float*)Realloc(lol->low_D_coefficient_list, 
							lol->face_count * sizeof(float));
						lol->low_D_coefficient_list[lol->face_count-1] = fg->D_coefficient[f_id];

					}

					break;
				}
			}
			assert(sanity_flag <= 1);
		}
	}

	/*
	// remove batch indices from degenerate faces if any
	if(removed_batch_flag)
	{
		for(int pbid = v->batch_count - 1; pbid >= 0; pbid--)
		{
			if(potential_batch_list[pbid])
			{
				if( IsBatchUsed_N_or_Less_Times(obj, pbid, 0) )
				{
					SwapBatchIndecies(obj, pbid, v->batch_count - 1, lol);
					v->batch_count--;
					lstep.removed_batch_vertex_count++;
				}
			}
		}
	}
	*/
	//Free(potential_batch_list);

    // drop edge count (the edge we collapsed)
    el->count--;
	
    lstep.removed_edge_count++;


	int affected_count = 0;
	int *affected_ev_list = NULL;
	for(int e_id=0; e_id < el->count; e_id++)
	{
		if(el->list[e_id].v1 == v_id1)
        {
            affected_count++;
			affected_ev_list = (int*)Realloc(affected_ev_list, affected_count * sizeof(int));
            affected_ev_list[affected_count-1] = el->list[e_id].v2;
        }
        else
        if(el->list[e_id].v2 == v_id1)
        {
            affected_count++;
			affected_ev_list = (int*)Realloc(affected_ev_list, affected_count * sizeof(int));
            affected_ev_list[affected_count-1] = el->list[e_id].v1;
        }
	}

	
	for(int ii=0; ii<affected_count; ii++)
	{
		for(int e_id=0; e_id < el->count; e_id++)
		{
e_label:
			if( ((el->list[e_id].v1 == v_id2) && 
				 (el->list[e_id].v2 == affected_ev_list[ii]) ) ||
				((el->list[e_id].v2 == v_id2) && 
				 (el->list[e_id].v1 == affected_ev_list[ii]) ) )
			{
				
				PutEdgeLast(el, e_id);
				el->count--;
                lstep.removed_edge_count++;
				if(e_id < el->count)
				{
					goto e_label;
				}
			}
		}
	}

	Free(affected_ev_list);

	/*
    // change index v_id2 to v_id1 in edge_list
    int e_id;
    int *affected_list = NULL;
    int affected_count = 0;
	int *affected_vertex_list = NULL;
    for(e_id=0; e_id<el->count; e_id++)
    {
        if(el->list[e_id].vertices[0] == v_id2)
        {
            el->list[e_id].vertices[0] = v_id1;
            affected_count++;
            affected_list = (int*)Realloc(affected_list, affected_count * sizeof(int));
            affected_list[affected_count-1] = e_id;
			affected_vertex_list = (int*)Realloc(affected_vertex_list, affected_count * sizeof(int));
            affected_vertex_list[affected_count-1] = el->list[e_id].vertices[1];
        }
        else
        if(el->list[e_id].vertices[1] == v_id2)
        {
            el->list[e_id].vertices[1] = v_id1;
            affected_count++;
            affected_list = (int*)Realloc(affected_list, affected_count * sizeof(int));
            affected_list[affected_count-1] = e_id;
			affected_vertex_list = (int*)Realloc(affected_vertex_list, affected_count * sizeof(int));
            affected_vertex_list[affected_count-1] = el->list[e_id].vertices[0];
        }

        if(el->list[e_id].vertices[0] == el->list[e_id].vertices[1])
        {
            Winprint("Error: degenerate edge indices!\n");
        }
    }

    // remove duplicates (edges that collapsed onto each other)
    // return the duplicates (except for one) to the precollapse indexing and move them to the end
	for(int a_id=affected_count-1; a_id >= 0; a_id--)
    {
        int ae_id = affected_list[a_id];
        for(e_id=0; e_id<el->count; e_id++)
        {
            if(e_id == ae_id) continue;

            // collapsed (id3 - id1 & id3 - id2 are now both id3 - id1)
            if((el->list[ae_id].vertices[0] == el->list[e_id].vertices[1] &&
                el->list[ae_id].vertices[1] == el->list[e_id].vertices[0])    ||
               (el->list[ae_id].vertices[0] == el->list[e_id].vertices[0] &&
                el->list[ae_id].vertices[1] == el->list[e_id].vertices[1])    
            )
            {
                if(el->list[ae_id].vertices[0] == v_id1)
                {
                    el->list[ae_id].vertices[0] = v_id2;
                }
                else
                if(el->list[ae_id].vertices[1] == v_id1)
                {
                    el->list[ae_id].vertices[1] = v_id2;
                }
                else
                {
                    printf("Error: inconsistent collapsed edges.\n");
                }

				// note: this gets sorting out of whack so we have to resort later
                SwapEdges(el, ae_id, el->count - 1);

                // update affected list to match swap
                for(int aindex=0; aindex<affected_count; aindex++)
                {
                    if(affected_list[aindex] == ae_id)
                    {
                        affected_list[aindex] = el->count - 1;
                    }
                    else
                    if(affected_list[aindex] == el->count - 1)
                    {
                        affected_list[aindex] = ae_id;
                    }
                }

                el->count--;
                lstep.removed_edge_count++;

                affected_list[a_id] = -1;
                break;
            }
        }
    }
	
    // reindexed (morphed edges)
	// used for edges
    for(a_id=0; a_id < affected_count; a_id++)
    {
        int ae_id = affected_list[a_id];
        if(ae_id != -1) // reindexed
        {
            assert(ae_id < el->count);
            if(lstep.first_edge == -1)
            {
                lstep.first_edge = lol->tmp_edge_count;
            }
            lstep.morphed_edge_count++;
            lol->tmp_edge_count++;
            lol->tmp_edge_list = (int*)Realloc(lol->tmp_edge_list, 2*lol->tmp_edge_count * sizeof(int));
            lol->tmp_edge_list[2*(lol->tmp_edge_count-1)] =  el->list[ae_id].vertices[0];
            lol->tmp_edge_list[2*(lol->tmp_edge_count-1)+1] = el->list[ae_id].vertices[1];
        }
    }
	
    Free(affected_list); 
	*/
	
            
    // update affected edge errors & resort
    for(e_id=0; e_id<el->count; e_id++)
    {
redo_e_label:
		if(el->list[e_id].v1 == v_id2)
		{
			el->list[e_id].v1 = v_id1;
			assert(el->list[e_id].v2 != v_id1);
		}
		else
		if(el->list[e_id].v2 == v_id2)
		{
			el->list[e_id].v2 = v_id1;
			assert(el->list[e_id].v1 != v_id1);
		}
		
        if((el->list[e_id].v1 == v_id1) ||
           (el->list[e_id].v2 == v_id1))
        {
			SetEdge_Face_UV(obj, e_id);
			el->list[e_id].error = QuadricEdgeError(obj, e_id) * UVEdgeError(obj, e_id);

			if(AllignEdgeByError(el, e_id) == 1)
			{
				goto redo_e_label;
			}
        }
    }

	//BubbleSortEdgesByError(el);

	// calcVertexNormal & D based on new faces instead of averaging
	float normal[3];
	int n_success = calcVertexNormal(obj, 0, v_id1, normal);
	if(n_success == -1 ) // new normal is degenerate so just average (can happen for things like
						 // the spikes of dreadnought happen)
	{
		int n_id1 = v->normal[v_id1];
		int n_id2 = v->normal[v_id2];
		normal[0] = n->list[3 * n_id1] + n->list[3 * n_id2];
		normal[1] = n->list[3 * n_id1 + 1] + n->list[3 * n_id2 + 1];
		normal[2] = n->list[3 * n_id1 + 2] + n->list[3 * n_id2 + 2];
		Normalize3(normal);
	}

	v->normal[v_id1] = InsertNormal(n, normal, 1.5f * vertex_normal_tolerance);
	v->D_coefficient[v_id1] = -Dot3(normal, &(v->object_list[3*v_id1]));

	// store new normal & D
	lstep.v3_nid = v->normal[v_id1];
	lstep.v3_D = v->D_coefficient[v_id1];

	// save step in library
    lol->count++;
    lol->step_list = (lod_step*)Realloc(lol->step_list, lol->count * sizeof(lod_step));
    lol->step_list[lol->count-1] = lstep;

	lol->uv_step_list = (lod_uv_step*)Realloc(lol->uv_step_list, lol->count * sizeof(lod_uv_step));
	lol->uv_step_list[lol->count-1] = uv_step;
}

void SwapBatchIndecies(object *obj, const int id1, const int id2)
{
	if(id1 == id2) return;

	lod_lib *lol = &(obj->lol);

	Swap32(obj->v.object_batch_list + id1, obj->v.object_batch_list + id2);
	Swap32(obj->v.texture_batch_list + id1, obj->v.texture_batch_list + id2);
	if( obj->v.texture_batch_list2 )
	{
		Swap32(obj->v.texture_batch_list2 + id1, obj->v.texture_batch_list2 + id2);
	}
	Swap32(obj->v.mtl_batch_list + id1, obj->v.mtl_batch_list + id2);

	for(int fg_id=0; fg_id < obj->face_group_count; fg_id++)
	{
		face_group *fg = obj->face_group_list + fg_id;

		for(int f_id=0; f_id < fg->max_count; f_id++)
		{
			for(int i=0; i<3; i++)
			{
				if( fg->batch_chain[3*f_id + i] == id1 )
				{
					fg->batch_chain[3*f_id + i] = id2;
				}
				else
				if( fg->batch_chain[3*f_id + i] == id2 )
				{
					fg->batch_chain[3*f_id + i] = id1;
				}
			}
		}
	}

	for(int uvc_id=0; uvc_id < obj->uvcl.count; uvc_id++)
	{
		UVChannel *uvc = obj->uvcl.list + uvc_id;

		for(int vt_id=0; vt_id < uvc->vertex_count; vt_id++)
		{
			if(uvc->vertex_lookup[vt_id] == id1)
			{
				uvc->vertex_lookup[vt_id] = id2;
			}
			else
			if(uvc->vertex_lookup[vt_id] == id2)
			{
				uvc->vertex_lookup[vt_id] = id1;
			}
		}
	}

	for(int v_id=0; v_id < lol->vertex_count; v_id++)
	{
		if(lol->vertex_list[v_id] == id1)
		{
			lol->vertex_list[v_id] = id2;		

		}
		else
		if(lol->vertex_list[v_id] == id2)
		{
			lol->vertex_list[v_id] = id1;
		}
	}
	
	for(int i=0; i < lol->uv_chain_count; i++)
	{
		if(lol->batch_uv_id_chain[i] == id1)
		{
			lol->batch_uv_id_chain[i] = id2;

		}else
		if(lol->batch_uv_id_chain[i] == id2)
		{
			lol->batch_uv_id_chain[i] = id1;
		}
	}
}

bool IsVertexUsed_N_or_Less_Times(const object *obj, const int vid, const int t_count)
{
	int count = 0;

	for(int fg_id=0; fg_id < obj->face_group_count; fg_id++)
	{
		const face_group *fg = obj->face_group_list + fg_id;

		const int *bc_pt = fg->batch_chain;
		const int *end_pt = fg->batch_chain + 3 * fg->count;

		for( ; bc_pt < end_pt; bc_pt++)
		{
			if(obj->v.object_batch_list[*bc_pt] == vid)
			{
				count++;
				if(count > t_count)
					return false;
			}
		}
	}

	return true;
}

bool IsBatchUsed_N_or_Less_Times(const object *obj, const int vbid, const int t_count)
{
	int count = 0;

	for(int fg_id=0; fg_id < obj->face_group_count; fg_id++)
	{
		const face_group *fg = obj->face_group_list + fg_id;

		for(int f_id=0; f_id < 3 * fg->count; f_id++)
		{
			if( fg->batch_chain[f_id] == vbid )
			{
				count++;
				if(count > t_count)
					return false;
			}
		}
	}

	return true;
}

float AverageUV(float v1, float v2)
{
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

    return .5f * (v1 + v2);
}

void ConsolidateDuplicateAssignments(object *obj)
{
	assert(obj->v.vertex_bone_count);
	
	vertices & v = obj->v;

	// sort assignments by bone id
	int index=0;
	for(int v_id=0; v_id < v.object_count; v_id++)
	{
		assert(v.first_vertex[v_id] == index);

		int count = v.vertex_bone_count[v_id];
		for(int j=0; j < count - 1; j++)
		{
			for(int i=0; i < count - (j+1); i++)
			{
				if(v.bone_id_list[index + i] > v.bone_id_list[index + i+1])
				{
					Swap32(v.bone_id_list + index + i, v.bone_id_list + index + i+1);
					Swap32(v.bone_weight_list + index + i, v.bone_weight_list + index + i+1);

					Swap32(v.bone_vertex_list + 3*(index + i)  , v.bone_vertex_list + 3*(index + i+1));
					Swap32(v.bone_vertex_list + 3*(index + i)+1, v.bone_vertex_list + 3*(index + i+1)+1);
					Swap32(v.bone_vertex_list + 3*(index + i)+2, v.bone_vertex_list + 3*(index + i+1)+2);

					Swap32(v.bone_normal_list + 3*(index + i)  , v.bone_normal_list + 3*(index + i+1));
					Swap32(v.bone_normal_list + 3*(index + i)+1, v.bone_normal_list + 3*(index + i+1)+1);
					Swap32(v.bone_normal_list + 3*(index + i)+2, v.bone_normal_list + 3*(index + i+1)+2);
				}
			}
		}

		index += count;

		if(v_id < v.object_count - 1)
			assert(v.first_vertex[v_id] + count == v.first_vertex[v_id+1]);	
	}


	// count number of assignments w/o duplicates
	index = 0;
	int new_total_count = 0;
	for(v_id=0; v_id < v.object_count; v_id++)
	{
		int old_count = v.vertex_bone_count[v_id];
		int new_count = 0;

		int last_id = -1;
		for(int i=0; i < old_count; i++)
		{
			if(v.bone_id_list[index+i] != last_id)
			{
				new_count++;
			}
			last_id = v.bone_id_list[index+i];
		}
		assert(new_count <= old_count);

		new_total_count += new_count;

		index += old_count;		
	}


	// combine multiple assignments of the same bone to the same vertex into one
	int * bone_id_list = (int*)Malloc(new_total_count * sizeof(int));
	float * bone_weight_list = (float*)Malloc(new_total_count * sizeof(float));
	float * bone_vertex_list = (float*)Malloc(3 * new_total_count * sizeof(float));
	float * bone_normal_list = (float*)Malloc(3 * new_total_count * sizeof(float));


	int old_index = 0;
	int new_index = 0;
	for(v_id=0; v_id < v.object_count; v_id++)
	{
		int old_count = v.vertex_bone_count[v_id];
		int new_count = 0;

		int last_id = -1;
		float total_w_sum = 0.0f;
		for(int i=0; i < old_count; i++)
		{
			if(v.bone_id_list[old_index+i] != last_id)
			{
				bone_id_list[new_index + new_count] = v.bone_id_list[old_index+i];
				bone_weight_list[new_index + new_count] = v.bone_weight_list[old_index+i];

				bone_vertex_list[3*(new_index + new_count)  ] = v.bone_vertex_list[3*(old_index+i)];
				bone_vertex_list[3*(new_index + new_count)+1] = v.bone_vertex_list[3*(old_index+i)+1];
				bone_vertex_list[3*(new_index + new_count)+2] = v.bone_vertex_list[3*(old_index+i)+2];

				bone_normal_list[3*(new_index + new_count)  ] = v.bone_normal_list[3*(old_index+i)];
				bone_normal_list[3*(new_index + new_count)+1] = v.bone_normal_list[3*(old_index+i)+1];
				bone_normal_list[3*(new_index + new_count)+2] = v.bone_normal_list[3*(old_index+i)+2];

				total_w_sum += bone_weight_list[new_index + new_count];
				new_count++;
			}
			else
			{
				bone_weight_list[new_index + new_count-1] += v.bone_weight_list[old_index+i];
				total_w_sum += v.bone_weight_list[old_index+i];
			}

			last_id = v.bone_id_list[old_index+i];
		}

		v.vertex_bone_count[v_id] = new_count;
		v.first_vertex[v_id] = new_index;
		
		old_index += old_count;	
		new_index += new_count;

		assert(fabs(total_w_sum - 1.0f) < .0001f);
	}

	Free(v.bone_id_list);
	Free(v.bone_weight_list);
	Free(v.bone_vertex_list);
	Free(v.bone_normal_list);

	v.bone_id_list = bone_id_list;
	v.bone_weight_list = bone_weight_list;
	v.bone_vertex_list = bone_vertex_list;
	v.bone_normal_list = bone_normal_list;
}

void CombineDefVertices(object *obj, const int id1, const int id2)
{
	assert(id1 != id2);
	if(obj->v.vertex_bone_count == NULL)
		return;

	vertices *v = &(obj->v);

	int i;
	const int first_id = (id1 < id2) ? id1 : id2;
	const int second_id = (id1 < id2) ? id2 : id1;

	const int offset1 = v->first_vertex[first_id];
	const int offset2 = v->first_vertex[second_id];
	assert(offset1 < offset2);

	const int size1 = v->vertex_bone_count[first_id];
	const int size2 = v->vertex_bone_count[second_id];


	// TODO: reduce # of assignments by dropping low weighted bones
	v->vertex_bone_count[first_id] += v->vertex_bone_count[second_id];
	for(i = first_id + 1; i < v->object_count; i++)
	{
		v->first_vertex[i] = v->first_vertex[i-1] + v->vertex_bone_count[i-1];
	}


	bool move;
	if(offset1 + size1 == offset2) // no moving necessary
	{
		move = false;
	}
	else
	{
		move = true;
	}
		
	unsigned char *tmp2 = (unsigned char*)Malloc(3 * size2 * sizeof(int));

	if(move)
	{
		memcpy(tmp2, v->bone_id_list + offset2, size2*sizeof(int));
		memmove(v->bone_id_list + offset1 + size1 + size2, v->bone_id_list + offset1 + size1,
			(offset2 - (offset1 + size1))*sizeof(int));
		memcpy(v->bone_id_list + offset1 + size1, tmp2, size2*sizeof(int));

		memcpy(tmp2, v->bone_weight_list + offset2, size2*sizeof(float));
		memmove(v->bone_weight_list + offset1 + size1 + size2, v->bone_weight_list + offset1 + size1,
			(offset2 - (offset1 + size1))*sizeof(float));
		memcpy(v->bone_weight_list + offset1 + size1, tmp2, size2*sizeof(float));
	}

	// renormalize
	float sum = 0.0f;
	for(i = offset1; i < offset1 + v->vertex_bone_count[first_id]; i++)
	{
		sum += v->bone_weight_list[i];
	}
	sum = 1.0f / sum;
	for(i = offset1; i < offset1 + v->vertex_bone_count[first_id]; i++)
	{
		v->bone_weight_list[i] *= sum;
	}

	if(move)
	{
		memcpy(tmp2, v->bone_vertex_list + 3*offset2, 3*size2*sizeof(float));
		memmove(v->bone_vertex_list + 3*offset1 + 3*size1 + 3*size2,
			v->bone_vertex_list + 3*offset1 + 3*size1,
			(3*offset2 - (3*offset1 + 3*size1))*sizeof(float));
		memcpy(v->bone_vertex_list + 3*offset1 + 3*size1, tmp2, 3*size2*sizeof(float));

		memcpy(tmp2, v->bone_normal_list + 3*offset2, 3*size2*sizeof(float));
		memmove(v->bone_normal_list + 3*offset1 + 3*size1 + 3*size2,
			v->bone_normal_list + 3*offset1 + 3*size1,
			(3*offset2 - (3*offset1 + 3*size1))*sizeof(float));
		memcpy(v->bone_normal_list + 3*offset1 + 3*size1, tmp2, 3*size2*sizeof(float));
	}

	Free(tmp2);

	// retransform to new position
}

void SwapUVVertices(object *obj, const int id1, const int id2)
{
	if(id1 == id2) return;

	lod_lib *lol = &(obj->lol);
    vertices *v = &(obj->v);

	Swap32(v->texture_list + 2*id1,     v->texture_list + 2*id2);
	Swap32(v->texture_list + 2*id1 + 1, v->texture_list + 2*id2 + 1);
	

	// reindex object
    for(int v_id=0; v_id<v->max_batch_count; v_id++)
	{
		if(v->texture_batch_list[v_id] == id1)
		{
			v->texture_batch_list[v_id] = id2;
		}
		else
		if(v->texture_batch_list[v_id] == id2)
		{
			v->texture_batch_list[v_id] = id1;
		}

		if( v->texture_batch_list2 )
		{
			if(v->texture_batch_list2[v_id] == id1)
			{
				v->texture_batch_list2[v_id] = id2;
			}
			else
			if(v->texture_batch_list2[v_id] == id2)
			{
				v->texture_batch_list2[v_id] = id1;
			}
		}
	}

	if(v->uv_vertex_id_list)
	{
		for(int i = 0; i < v->uv_list_length; i++)
		{
			if(v->uv_vertex_id_list[i] == id1)
			{
				v->uv_vertex_id_list[i] = id2;
			}
			else
			if(v->uv_vertex_id_list[i] == id2)
			{
				v->uv_vertex_id_list[i] = id1;
			}
		}
	}
    
	if(lol->count)
	{
		for(int i = 0; i < lol->uv_count; i++)
		{
			if(lol->high_uv_id1[i] == id1)
			{
				lol->high_uv_id1[i] = id2;
			}else
			if(lol->high_uv_id1[i] == id2)
			{
				lol->high_uv_id1[i] = id1;
			}

			if(lol->high_uv_id2[i] == id1)
			{
				lol->high_uv_id2[i] = id2;
			}else
			if(lol->high_uv_id2[i] == id2)
			{
				lol->high_uv_id2[i] = id1;
			}

			if(lol->low_uv_id[i] == id1)
			{
				lol->low_uv_id[i] = id2;
			}else
			if(lol->low_uv_id[i] == id2)
			{
				lol->low_uv_id[i] = id1;
			}
		}
	}
}

void SwapVertices(object *obj, const int id1, const int id2)
{
    if(id1 == id2) return;

	lod_lib *lol = &(obj->lol);
    vertices *v = &(obj->v);
    Eedge_lib *el = &(obj->el);

	Swap32(v->object_list + 3*id1,     v->object_list + 3*id2);
	Swap32(v->object_list + 3*id1 + 1, v->object_list + 3*id2 + 1);
	Swap32(v->object_list + 3*id1 + 2, v->object_list + 3*id2 + 2);

	Swap32(v->frozen_list + 3*id1,     v->frozen_list + 3*id2);
	Swap32(v->frozen_list + 3*id1 + 1, v->frozen_list + 3*id2 + 1);
	Swap32(v->frozen_list + 3*id1 + 2, v->frozen_list + 3*id2 + 2);

	Swap32(v->normal + id1, v->normal + id2);
	Swap32(v->D_coefficient + id1, v->D_coefficient + id2);

	if(export_vertex_colors)
	{
		assert(v->color);
		MemSwap(v->color + 3*id1, v->color + 3*id2, 3*sizeof(unsigned char));
	}

	// reindex object
    for(int v_id=0; v_id<v->max_batch_count; v_id++)
	{
		if(v->object_batch_list[v_id] == id1)
		{
			v->object_batch_list[v_id] = id2;
		}
		else
		if(v->object_batch_list[v_id] == id2)
		{
			v->object_batch_list[v_id] = id1;
		}
	}

    for(int e_id=0; e_id < el->max_count; e_id++)
    {
        if(el->list[e_id].v1 == id1)
        {
            el->list[e_id].v1 = id2;
        }
        else
        if(el->list[e_id].v1 == id2)
        {
            el->list[e_id].v1 = id1;
        }

        if(el->list[e_id].v2 == id1)
        {
            el->list[e_id].v2 = id2;
        }
        else
        if(el->list[e_id].v2 == id2)
        {
            el->list[e_id].v2 = id1;
        }

        assert(el->list[e_id].v1 != el->list[e_id].v2);
    }

	if(lol->count)
	{
		for(int step_id=0; step_id < lol->count; step_id++)
		{
			lod_step& lstep = lol->step_list[step_id];

			if(lstep.vid1 == id1)
			{
				lstep.vid1 = id2;
			}
			else
			if(lstep.vid1 == id2)
			{
				lstep.vid1 = id1;    
			}

			if(lstep.vid2 == id1)
			{
				lstep.vid2 = id2;
			}
			else
			if(lstep.vid2 == id2)
			{
				lstep.vid2 = id1;    
			}
		}

		for(e_id=0; e_id<lol->tmp_edge_count; e_id++)
		{
			if(lol->tmp_edge_list[2*e_id] == id1)
			{
				lol->tmp_edge_list[2*e_id] = id2;
			}
			else
			if(lol->tmp_edge_list[2*e_id] == id2)
			{
				lol->tmp_edge_list[2*e_id] = id1;
			}

			if(lol->tmp_edge_list[2*e_id+1] == id1)
			{
				lol->tmp_edge_list[2*e_id+1] = id2;
			}
			else
			if(lol->tmp_edge_list[2*e_id+1] == id2)
			{
				lol->tmp_edge_list[2*e_id+1] = id1;
			}

			assert(lol->tmp_edge_list[2*e_id] != lol->tmp_edge_list[2*e_id+1]);
		}
	}

	// swap err
	if(v->err)
	{
		MemSwap(v->err + id1, v->err + id2, sizeof(vertex_error));
	}
	

	// deformable stuff
	if(v->vertex_bone_count != NULL)
	{
		const int first_id = (id1 < id2) ? id1 : id2;
		const int second_id = (id1 < id2) ? id2 : id1;

		const int offset1 = v->first_vertex[first_id];
		const int offset2 = v->first_vertex[second_id];
		assert(offset1 < offset2);

		const int count1 = v->vertex_bone_count[first_id];
		const int count2 = v->vertex_bone_count[second_id];

		if(count1 == count2)
		{
			MemSwap(v->bone_id_list + offset1, v->bone_id_list + offset2, count1*sizeof(int));
			MemSwap(v->bone_weight_list + offset1, v->bone_weight_list + offset2, count1*sizeof(float));
			MemSwap(v->bone_vertex_list + 3 * offset1, v->bone_vertex_list + 3 * offset2,
				3 * count1*sizeof(float));
			MemSwap(v->bone_normal_list + 3 * offset1, v->bone_normal_list + 3 * offset2,
				3 * count1*sizeof(float));
		}
		else
		{
			Swap32(v->vertex_bone_count + first_id, v->vertex_bone_count + second_id);
			for(int i = first_id + 1; i <= second_id; i++)
			{
				v->first_vertex[i] = v->first_vertex[i-1] + v->vertex_bone_count[i-1];
			}

			if(i < v->object_count) // can't check last index
			{
				assert(v->first_vertex[i] == v->first_vertex[i-1] + v->vertex_bone_count[i-1]);
			}

			unsigned char *tmp1 = (unsigned char*)Malloc(3 * count1*sizeof(int));
			unsigned char *tmp2 = (unsigned char*)Malloc(3 * count2*sizeof(int));

			memcpy(tmp1, v->bone_id_list + offset1, count1*sizeof(int));
			memcpy(tmp2, v->bone_id_list + offset2, count2*sizeof(int));

			memmove(v->bone_id_list + offset1 + count2, v->bone_id_list + offset1 + count1,
				(offset2 - (offset1 + count1))*sizeof(int));
			memcpy(v->bone_id_list + offset1, tmp2, count2*sizeof(int));
			memcpy(v->bone_id_list + (offset2 + count2) - count1, tmp1, count1*sizeof(int));

			memcpy(tmp1, v->bone_weight_list + offset1, count1*sizeof(float));
			memcpy(tmp2, v->bone_weight_list + offset2, count2*sizeof(float));
			memmove(v->bone_weight_list + offset1 + count2, v->bone_weight_list + offset1 + count1,
				(offset2 - (offset1 + count1))*sizeof(float));
			memcpy(v->bone_weight_list + offset1, tmp2, count2*sizeof(float));
			memcpy(v->bone_weight_list + (offset2 + count2) - count1, tmp1, count1*sizeof(float));

			memcpy(tmp1, v->bone_vertex_list + 3*offset1, 3*count1*sizeof(float));
			memcpy(tmp2, v->bone_vertex_list + 3*offset2, 3*count2*sizeof(float));
			memmove(v->bone_vertex_list + 3*offset1 + 3*count2, v->bone_vertex_list + 3*offset1 + 3*count1,
				(3*offset2 - (3*offset1 + 3*count1))*sizeof(float));
			memcpy(v->bone_vertex_list + 3*offset1, tmp2, 3*count2*sizeof(float));
			memcpy(v->bone_vertex_list + (3*offset2 + 3*count2) - 3*count1, tmp1, 3*count1*sizeof(float));

			memcpy(tmp1, v->bone_normal_list + 3*offset1, 3*count1*sizeof(float));
			memcpy(tmp2, v->bone_normal_list + 3*offset2, 3*count2*sizeof(float));
			memmove(v->bone_normal_list + 3*offset1 + 3*count2, v->bone_normal_list + 3*offset1 + 3*count1,
				(3*offset2 - (3*offset1 + 3*count1))*sizeof(float));
			memcpy(v->bone_normal_list + 3*offset1, tmp2, 3*count2*sizeof(float));
			memcpy(v->bone_normal_list + (3*offset2 + 3*count2) - 3*count1, tmp1, 3*count1*sizeof(float));

			Free(tmp1);
			Free(tmp2);
		}
	}
}

int GetFirstUVtoXYZ_ID(const vertices *v, const int uv_id)
{
	const int api_node = v->api_node_uv_id[uv_id];
	const int flags = v->uv_flags[uv_id];

	for(int i=0; i < v->batch_count; i++)
	{
		if( (v->texture_batch_list[i] == uv_id) &&
			(v->api_node_xyz_id[ v->object_batch_list[i] ] == api_node) &&
			(v->flags[ v->object_batch_list[i] ] == flags) )
		{
			return v->object_batch_list[i];
		}
	}

	return -1;
}

void GetVID3(object *obj, int g_id, int f_id, int *v_id1, int *v_id2, int *v_id3)
{
	const face_group *fg = obj->face_group_list + g_id;

	*v_id1 = obj->v.object_batch_list[ fg->batch_chain[3*f_id] ];
	*v_id2 = obj->v.object_batch_list[ fg->batch_chain[3*f_id+1] ];
	*v_id3 = obj->v.object_batch_list[ fg->batch_chain[3*f_id+2] ];
}

void calcEdgeErrors(object *obj)
{
	calcQuadrics(obj);

	Eedge_lib & el = obj->el;

	// holds new vertex to collapse edge to
	// filled by QuadricEdgeError
	//el.v_list = (Vector4*)Malloc(el.count * sizeof(Vector4));

	// compute errors
	for(int e_id=0; e_id < el.count; e_id++)
	{
		// assumes SetEdge_Face_UV(object *obj, const int e_id) was already called
		el.list[e_id].error = QuadricEdgeError(obj, e_id) * UVEdgeError(obj, e_id);
	}

	sortQ(obj->el.list, obj->el.count, sizeof(Eedge), CompareEdgesByError, NULL, SwapEdges, &(obj->el));
}


// add face and uv indices that belong to this edge
void SetEdge_Face_UV(object *obj, const int e_id)
{
	Eedge & e = obj->el.list[e_id];

	const int vid1 = e.v1;
	const int vid2 = e.v2;

	e.face_count = 0;
	e.uv_pair_count = 0;
	Free(e.group_list);
	Free(e.face_list);

	for(int gid = 0; gid < obj->face_group_count; gid++)
	{
		const face_group *fg = obj->face_group_list + gid;
		cq2Mtl * m;
		if (fg->material_id > 0)
		{
			m = obj->ml->cq2Materials[fg->material_id];
		}
		else
		{
			m=0;
		}

		for(int fid = 0; fid < fg->count; fid++)
		{
			const int bid1 = fg->batch_chain[3*fid];
			const int bid2 = fg->batch_chain[3*fid+1];
			const int bid3 = fg->batch_chain[3*fid+2];

			const int v1 = obj->v.object_batch_list[bid1];
			const int v2 = obj->v.object_batch_list[bid2];
			const int v3 = obj->v.object_batch_list[bid3];

			int uv1 = -1;
			int uv2 = -1;
			int e_o_f = 0;
			if(vid1 == v1 && vid2 == v2)
			{
				uv1 = obj->v.texture_batch_list[bid1];
				uv2 = obj->v.texture_batch_list[bid2];
				e_o_f = 1;
			}
			else
			if(vid1 == v2 && vid2 == v1)
			{
				uv1 = obj->v.texture_batch_list[bid2];
				uv2 = obj->v.texture_batch_list[bid1];
				e_o_f = -1;
			}
			else
			if(vid1 == v2 && vid2 == v3)
			{
				uv1 = obj->v.texture_batch_list[bid2];
				uv2 = obj->v.texture_batch_list[bid3];
				e_o_f = 2;
			}
			else
			if(vid1 == v3 && vid2 == v2)
			{
				uv1 = obj->v.texture_batch_list[bid3];
				uv2 = obj->v.texture_batch_list[bid2];
				e_o_f = -2;
			}
			else
			if(vid1 == v3 && vid2 == v1)
			{
				uv1 = obj->v.texture_batch_list[bid3];
				uv2 = obj->v.texture_batch_list[bid1];
				e_o_f = 3;
			}
			else
			if(vid1 == v1 && vid2 == v3)
			{
				uv1 = obj->v.texture_batch_list[bid1];
				uv2 = obj->v.texture_batch_list[bid3];
				e_o_f = -3;
			}

			// add indices
			if(uv1 >= 0 && uv2 >= 0)
			{
				//assert(uv1 != uv2);

				// add face & group id's
				bool duplicate = false;
				for(int i = 0; i < e.face_count; i++)
				{
					if(e.group_list[i] == gid && e.face_list[i] == fid)
					{
						duplicate = true;
						break;
					}
				}
				if(!duplicate)
				{
					e.face_count++;
					e.group_list = (int*)Realloc(e.group_list, e.face_count * sizeof(int));
					e.group_list[e.face_count-1] = gid;
					e.face_list = (int*)Realloc(e.face_list, e.face_count * sizeof(int));
					e.face_list[e.face_count-1] = fid;
				}

				//add UV id's
				duplicate = false;
				for(i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid1 == uv1 && e.uv_pairs[i].uvid2 == uv2 &&
					   e.uv_pairs[i].m == m)
					{
						duplicate = true;
						break;
					}
				}
				if(!duplicate)
				{
					e.uv_pair_count++;
					e.uv_pairs = (uv_mtl*)Realloc(e.uv_pairs, e.uv_pair_count * sizeof(uv_mtl));
					e.uv_pairs[e.uv_pair_count-1].uvid1 = uv1;
					e.uv_pairs[e.uv_pair_count-1].uvid2 = uv2;
					e.uv_pairs[e.uv_pair_count-1].m = m;

					e.uv_dest = (float*)Realloc(e.uv_dest, 2 * e.uv_pair_count * sizeof(float));
				}
			}
		}
	}
}

extern int RayPolygon( const Vector & ray_origin, const Vector & ray_dir,
			    const Vector tri[3], 
				Vector * const intersection,
				float * const alpha, float * const beta);

float UVEdgeError(object *obj, const int e_id)
{
//return 1.0f;

	assert(e_id < obj->el.count);

	const int vid1 = obj->el.list[e_id].v1;
	const int vid2 = obj->el.list[e_id].v2;
	assert(vid1 != vid2);

	const vertices & v = obj->v;
	const Eedge & e = obj->el.list[e_id];
	
	// calculate resulting uv's
	const Vector & dest_v = *(Vector*)&(e.v_dest);

	/*
	Vector normal(0.0f, 0.0f, 0.0f);
	for(int fid = 0; fid < e.face_count; fid++)
	{
		// TODO: fix: only use a subset of faces for each uv pair
		const int nid = obj->face_group_list[ e.group_list[fid] ].normal[ e.face_list[fid] ];

		normal.x += obj->n.list[3*nid];
		normal.y += obj->n.list[3*nid+1];
		normal.z += obj->n.list[3*nid+2];
	}

	float magnitude = normal.magnitude();
	if(magnitude >= .00001f)
	{
		normal.normalize();
	}

	Vector face_v[3];
	Vector intersection;
	float _beta;
	float _alpha;
	int hit_face = -1;
	float uv1[2], uv2[2], uv3[2]; 
	for(int ff = 0; ff < e.face_count && magnitude >= .00001f; ff++)
	{
		int gid = e.group_list[ff];
		int fid = e.face_list[ff];

		int bid1 = obj->face_group_list[gid].batch_chain[3*fid];
		int vvid1 = obj->v.object_batch_list[bid1];
		face_v[0].x = obj->v.object_list[3*vvid1];
		face_v[0].y = obj->v.object_list[3*vvid1+1];
		face_v[0].z = obj->v.object_list[3*vvid1+2];

		int bid2 = obj->face_group_list[gid].batch_chain[3*fid+1];
		int vvid2 = obj->v.object_batch_list[bid2];
		face_v[1].x = obj->v.object_list[3*vvid2];
		face_v[1].y = obj->v.object_list[3*vvid2+1];
		face_v[1].z = obj->v.object_list[3*vvid2+2];

		int bid3 = obj->face_group_list[gid].batch_chain[3*fid+2];
		int vvid3 = obj->v.object_batch_list[bid3];
		face_v[2].x = obj->v.object_list[3*vvid3];
		face_v[2].y = obj->v.object_list[3*vvid3+1];
		face_v[2].z = obj->v.object_list[3*vvid3+2];

		float tmp_beta;
		float tmp_alpha;
		Vector tmp_intersection;

		if(RayPolygon(dest_v, normal, face_v, &tmp_intersection, &tmp_alpha, &tmp_beta) == 1)
		{			
			if(hit_face >= 0) // we got two hits so we don't use this method (can happen for 3 or more faces)
			{
				hit_face = -1;
				break;
			}
			else
			{
				hit_face = ff;

				intersection = tmp_intersection;
				_alpha = tmp_alpha;
				_beta = tmp_beta;

				int uvid1 = obj->v.texture_batch_list[bid1];
				uv1[0] = obj->v.texture_list[2*uvid1];
				uv1[1] = obj->v.texture_list[2*uvid1+1];

				int uvid2 = obj->v.texture_batch_list[bid2];
				uv2[0] = obj->v.texture_list[2*uvid2];
				uv2[1] = obj->v.texture_list[2*uvid2+1];

				int uvid3 = obj->v.texture_batch_list[bid3];
				uv3[0] = obj->v.texture_list[2*uvid3];
				uv3[1] = obj->v.texture_list[2*uvid3+1];
			}
		}
	}

	if(hit_face >= 0)
	{
		assert(_beta <= 1.0f);

		for(int i = 0; i < e.uv_pair_count; i++)
		{
			assert(_beta <= 1.0f);

			if(e.uv_pairs[i].uvid1 == e.uv_pairs[i].uvid2)
			{
				e.uv_dest[2*i] =   v.texture_list[2 * e.uv_pairs[i].uvid1];
				e.uv_dest[2*i+1] = v.texture_list[2 * e.uv_pairs[i].uvid1 + 1];
			}
			else
			{
				e.uv_dest[2*i] =   (1.0f - (_alpha + _beta)) * uv1[0] + _alpha * uv2[0] + _beta * uv3[0];
				e.uv_dest[2*i+1] = (1.0f - (_alpha + _beta)) * uv1[1] + _alpha * uv2[1] + _beta * uv3[1];
			}
		}

		// V = (1 - (alpha + beta))*V0 + alpha*V1 + beta*V2
	}
	else // use straight interpolation
	*/
	{
		const Vector & v1 = *(Vector*)(v.object_list + 3 * vid1);
		const Vector & v2 = *(Vector*)(v.object_list + 3 * vid2);
			
		const float dist = (v2 - v1).magnitude();
		const float d1 = dot_product( (dest_v - v1), (v2 - v1) / dist );
		const float d2 = dot_product( (dest_v - v2), (v1 - v2) / dist );

		float fraction2 = d1 / dist;
		float fraction1 = d2 / dist;

		float inv_mag = 1.0f / (fraction1 + fraction2);
		fraction1 *= inv_mag;
		fraction2 *= inv_mag;

		// clamp
		if(fraction1 > 1.0f)
		{
			fraction1 = 1.0f;
		}
		else
		if(fraction1 < 0.0f)
		{
			fraction1 = 0.0f;
		}

		if(fraction2 > 1.0f)
		{
			fraction2 = 1.0f;
		}
		else
		if(fraction2 < 0.0f)
		{
			fraction2 = 0.0f;
		}

		inv_mag = 1.0f / (fraction1 + fraction2);
		fraction1 *= inv_mag;
		fraction2 *= inv_mag;

		for(int i = 0; i < e.uv_pair_count; i++)
		{
			if(e.uv_pairs[i].uvid1 == e.uv_pairs[i].uvid2)
			{
				e.uv_dest[2*i] =   v.texture_list[2 * e.uv_pairs[i].uvid1];
				e.uv_dest[2*i+1] = v.texture_list[2 * e.uv_pairs[i].uvid1 + 1];
			}
			else
			{
				e.uv_dest[2*i] = fraction1 * v.texture_list[2 * e.uv_pairs[i].uvid1] +
								 fraction2 * v.texture_list[2 * e.uv_pairs[i].uvid2];
				e.uv_dest[2*i+1] = fraction1 * v.texture_list[2 * e.uv_pairs[i].uvid1 + 1] +
								   fraction2 * v.texture_list[2 * e.uv_pairs[i].uvid2 + 1];
			}
		}
	}

	float error = 1.0f;
	float penalty = 0.1f;
	float decrement = 0.5f;

	// see if there are any faces whose uv's can't be interpolated and penalize them
	for(int gid = 0; gid < obj->face_group_count; gid++)
	{
		const face_group *fg = obj->face_group_list + gid;
		const cq2Mtl *m = obj->ml->cq2Materials[fg->material_id];

		for(int fid = 0; fid < fg->count; fid++)
		{
			const int bid1 = fg->batch_chain[3*fid];
			const int bid2 = fg->batch_chain[3*fid+1];
			const int bid3 = fg->batch_chain[3*fid+2];

			const int v_id1 = v.object_batch_list[bid1];
			const int v_id2 = v.object_batch_list[bid2];
			const int v_id3 = v.object_batch_list[bid3];

			const int uv_id1 = v.texture_batch_list[bid1];
			const int uv_id2 = v.texture_batch_list[bid2];
			const int uv_id3 = v.texture_batch_list[bid3];

			bool used = false;
			if(v_id1 == vid1)
			{
				for(int i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid1 == uv_id1 && e.uv_pairs[i].m == m)
					{
						used = true;
						break;
					}
				}

				if(!used)
				{
					error += penalty;
					penalty *= decrement;
				}
			}
			else
			if(v_id1 == vid2)
			{
				for(int i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid2 == uv_id1 && e.uv_pairs[i].m == m)
					{
						used = true;
						break;
					}
				}

				if(!used)
				{
					error += penalty;
					penalty *= decrement;
				}
			}

			used = false;
			if(v_id2 == vid1)
			{
				for(int i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid1 == uv_id2 && e.uv_pairs[i].m == m)
					{
						used = true;
						break;
					}
				}

				if(!used)
				{
					error += penalty;
					penalty *= decrement;
				}
			}
			else
			if(v_id2 == vid2)
			{
				for(int i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid2 == uv_id2 && e.uv_pairs[i].m == m)
					{
						used = true;
						break;
					}
				}

				if(!used)
				{
					error += penalty;
					penalty *= decrement;
				}
			}

			used = false;
			if(v_id3 == vid1)
			{
				for(int i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid1 == uv_id3 && e.uv_pairs[i].m == m)
					{
						used = true;
						break;
					}
				}

				if(!used)
				{
					error += penalty;
					penalty *= decrement;
				}
			}
			else
			if(v_id3 == vid2)
			{
				for(int i = 0; i < e.uv_pair_count; i++)
				{
					if(e.uv_pairs[i].uvid2 == uv_id3 && e.uv_pairs[i].m == m)
					{
						used = true;
						break;
					}
				}

				if(!used)
				{
					error += penalty;
					penalty *= decrement;
				}
			}
		}
	}

	return error;
}

inline float new_lock_v(const float x1, const float x2, const float x3,
						const float l1, const float l2)
{
	// NOTE: could use higher power than 2
	if(l1 + l2 > 0.0f)
	{
		const float l1_sq = l1 * l1;
		const float l2_sq = l2 * l2;

		return (l1_sq * (l1 * x1 + (1.0f - l1) * x3) + l2_sq * (l2 * x2 + (1.0f - l2) * x3) ) / (l1_sq + l2_sq);
	}
	else
	{
		return x3;
	}
}

inline float calc_penalty(const float l1, const float l2)
{
	if( (l1 >= 1.0f && l2 > 0.0f) || (l2 >= 1.0f && l1 > 0.0f) )
	{
		// note l1 or l2 will be 1.0 so it's just the min(l1, l2) * 1000000
		return 1000000.0f * l1 * l2;
		//return 1000000.0f * l1 * l1 * l2 * l2;
	}
	else
	{
		return 0.0f;
	}
}

float QuadricEdgeError(object *obj, const int e_id)
{
	assert(e_id < obj->el.count);

	vertices & v = obj->v;

	const int vid1 = obj->el.list[e_id].v1;
	const int vid2 = obj->el.list[e_id].v2;
	assert(vid1 != vid2);

	int total_plane_count;
    Matrix4 q ( MergeQuadrics(obj, vid1, vid2, false, &total_plane_count) );

	float current_uv_weight = lod_uv_weight;

	Vector4 *new_v = &(obj->el.list[e_id].v_dest); //obj->el.v_list + e_id;

	const float *v1 = v.object_list + 3*vid1;
	const float *v2 = v.object_list + 3*vid2;
	const float *fr1 = v.frozen_list + 3*vid1;
	const float *fr2 = v.frozen_list + 3*vid2;

	float path_length = FLT_MAX;
	float quad_error = FLT_MAX;
	float min_quad_error = FLT_MAX;
	float error = FLT_MAX;
	float step;

	Vector4 midpoint_v(.5f * (v1[0] + v2[0]), .5f * (v1[1] + v2[1]), .5f * (v1[2] + v2[2]), 1.0f);
	
	Vector4 quad_v;
	if( GetNewV(q, &quad_v) ) // inversion worked
	{
		step = 0.0f;
		//assert(fabs(1.0f - new_v->w) < .001f);
		//new_v->x /= new_v->w;
		//new_v->y /= new_v->w;
		//new_v->z /= new_v->w;
		//new_v->w = 1.0f;
	}
	else
	{
		quad_v = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		step = 1.0f;
	}


	float frozen_error_multiplier = 1.0f;
	// interpolate from calculated point to midpoint (if inversion failed just use midpoint)
	for(; step < 1.00001f; step += 0.1f)
	{
		Vector4 tmp_v ( (1.0f - step) * quad_v + step * midpoint_v);		

		tmp_v.x = new_lock_v( v1[0], v2[0], tmp_v.x, fr1[0], fr2[0] );
		tmp_v.y = new_lock_v( v1[1], v2[1], tmp_v.y, fr1[1], fr2[1] );
		tmp_v.z = new_lock_v( v1[2], v2[2], tmp_v.z, fr1[2], fr2[2] );

		frozen_error_multiplier +=
			calc_penalty( fr1[0], fr2[0] ) +
			calc_penalty( fr1[1], fr2[1] ) +
			calc_penalty( fr1[2], fr2[2] );

		// original guess gets a slight preference
		quad_error = GetQuadricVectorErr(tmp_v, q);

		if(quad_error == 0.0f || !is_float( quad_error ) )
		{
			//quad_error = FLT_MAX / 1e5f;
			Winprint("Warning: Bad quadric error %f!\n", quad_error);
		}	

		float vec[3];
		vec[0] = tmp_v.x - v1[0];
		vec[1] = tmp_v.y - v1[1];
		vec[2] = tmp_v.z - v1[2];
		const float dist1_sq = Magnitude3_sq(vec);
		vec[0] = tmp_v.x - v2[0];
		vec[1] = tmp_v.y - v2[1];
		vec[2] = tmp_v.z - v2[2];
		const float dist2_sq = Magnitude3_sq(vec);	
		path_length = (float)sqrt(dist1_sq + dist2_sq);

		const float tmp_error = pow(quad_error, (1.0 - current_uv_weight)) * pow(path_length, current_uv_weight);
		

		if(quad_error < min_quad_error)
		{
			min_quad_error = quad_error;
			error = tmp_error;
			*new_v = tmp_v;
		}
	}

	error *= frozen_error_multiplier;
		
	assert(error < FLT_MAX);


	// 1) check for flipped normals and penalize * 50
	// 2) check for orphan vertices and penalize * 8
	// TODO: keep correct track of faces in each edge as we progress
	// and use it here instead of searching each time
	{
		for(int g_id=0; g_id < obj->face_group_count; g_id++)
		{
			const face_group *fg = obj->face_group_list + g_id;
			vertices *v = &(obj->v);

			for(int f_id=0; f_id < fg->count; f_id++)
			{
				int vb1 = fg->batch_chain[3*f_id];
				int vb2 = fg->batch_chain[3*f_id+1];
				int vb3 = fg->batch_chain[3*f_id+2];

				vb1 = v->object_batch_list[vb1];
				vb2 = v->object_batch_list[vb2];
				vb3 = v->object_batch_list[vb3];

				int swap_id = -1;

				if( (vb1 == vid1 || vb2 == vid1 || vb3 == vid1) &&
					(vb1 == vid2 || vb2 == vid2 || vb3 == vid2) )
				{
					// check for orphan vertices (can still be used by two opposite faces)
					if( (vb1 != vid1) && (vb1 != vid2) )
					{
						if( IsVertexUsed_N_or_Less_Times(obj, vb1, 2) )
						{
							error *= 8;
							//goto label1;
						}
					}
					else
					if( (vb2 != vid1) && (vb2 != vid2) )
					{
						if( IsVertexUsed_N_or_Less_Times(obj, vb2, 2) )
						{
							error *= 8;
							//goto label1;
						}
					}
					else
					if( (vb3 != vid1) && (vb3 != vid2) )
					{
						if( IsVertexUsed_N_or_Less_Times(obj, vb3, 2) )
						{
							error *= 8;
							//goto label1;
						}
					}
					else
					{
						Winprint("Error: IsUniqueVertex out of synch!");
					}
				}
				else
				if( vb1 == vid1 || vb2 == vid1 || vb3 == vid1 )
				{
					swap_id = vid1;
				}
				else
				if( vb1 == vid2 || vb2 == vid2 || vb3 == vid2 )
				{
					swap_id = vid2;
				}

				
				if(swap_id != -1)
				{
					float old_normal[3];
					float new_normal[3];		
					
					float tmp_v[3] = {new_v->x, new_v->y, new_v->z}; 

					int succes1 = calcFaceGroupNormal(fg, v, f_id, old_normal);
					if(succes1 != -1)
					{
						MemSwap(tmp_v, v->object_list + 3*swap_id, 3 * sizeof(float));
						int success2 = calcFaceGroupNormal(fg, v, f_id, new_normal);
						MemSwap(tmp_v, v->object_list + 3*swap_id, 3 * sizeof(float));

						if(success2 != -1 && succes1 != -1)
						{					
							float dot = Dot3(new_normal, old_normal);

							if(dot < -0.98f) // 180
							{
								error *= 50.0f;  // penalty fro flipping normals
								goto label1;
							}
						}
					}	
				}
		
			}
		}
	}

label1:;

	return error;
}

void calcQuadrics(object *obj)
{

#ifdef _DEBUG
	int tmp_count = 0;
	for(int i=0; i<obj->face_group_count; i++)
	{
		tmp_count += obj->face_group_list[i].count;
	}
	assert(tmp_count == obj->face_count);
#endif

	vertices& v = obj->v;

	// compute initial quadric for each face/plane
	{
		for(int g_id = 0; g_id < obj->face_group_count; g_id++)
		{
			face_group *fg = obj->face_group_list + g_id;

			fg->quadric_list = (Matrix4*)Malloc(fg->count * sizeof(Matrix4));

			for(int f_id=0; f_id < fg->count; f_id++)
			{
				Matrix4 *m = fg->quadric_list + f_id;

				float normal[3];

				int vb1 = fg->batch_chain[3*f_id];
				int vb2 = fg->batch_chain[3*f_id+1];
				int vb3 = fg->batch_chain[3*f_id+2];

				vb1 = v.object_batch_list[vb1];
				vb2 = v.object_batch_list[vb2];
				vb3 = v.object_batch_list[vb3];

				// we recompute normals since we want them to be accurate (0 tolerance)
				int n_success = calcNormal(v.object_list + 3*vb1,
										   v.object_list + 3*vb2,
										   v.object_list + 3*vb3, 
										   normal);
				if(n_success == -1)
				{
					Winprint("Error: bad normal!\n");
				}

				float D = (-Dot3(v.object_list + 3*vb1, normal) -
					        Dot3(v.object_list + 3*vb2, normal) -
						    Dot3(v.object_list + 3*vb3, normal) ) * (1.0 / 3.0);
				m->quadric(normal[0], normal[1], normal[2], D);
			}
		}
	}


	{
		// initialize vertex error struct
		v.err = (vertex_error*)Malloc(v.object_count * sizeof(vertex_error));

		for(int vi = 0; vi < v.object_count; vi++)
		{
			v.err[vi].Init();
		}
	}

	// add face contribution to each vertex
	{
		for(int g_id = 0; g_id < obj->face_group_count; g_id++)
		{
			face_group *fg = obj->face_group_list + g_id;

			for(int f_id=0; f_id < fg->count; f_id++)
			{
				int vb1 = fg->batch_chain[3*f_id];
				int vb2 = fg->batch_chain[3*f_id+1];
				int vb3 = fg->batch_chain[3*f_id+2];

				vb1 = v.object_batch_list[vb1];
				vb2 = v.object_batch_list[vb2];
				vb3 = v.object_batch_list[vb3];

				assert(vb1 < v.object_count);
				assert(vb2 < v.object_count);
				assert(vb3 < v.object_count);

#if 0
				float w1 = GetNewVertexAngle(obj, g_id, f_id, 0);
				float w2 = GetNewVertexAngle(obj, g_id, f_id, 1);
				float w3 = GetNewVertexAngle(obj, g_id, f_id, 2);

				assert( fabs(R2D*(w1 + w2 + w3) - 180.0f) < 1.0f );

				obj->v.err[vb1].quadric += (w1 * fg->quadric_list[f_id]);
				obj->v.err[vb2].quadric += (w2 * fg->quadric_list[f_id]);
				obj->v.err[vb3].quadric += (w3 * fg->quadric_list[f_id]);
#else

				// TODO: get rid of plane_list and just sum quadrics
				AddVertexQuadric(obj, vb1, g_id, f_id);
				AddVertexQuadric(obj, vb2, g_id, f_id);
				AddVertexQuadric(obj, vb3, g_id, f_id);
				//obj->v.err[vb1].quadric += fg->quadric_list[f_id];
				//obj->v.err[vb2].quadric += fg->quadric_list[f_id];
				//obj->v.err[vb3].quadric += fg->quadric_list[f_id];
#endif
				
			}
		}
	}


	{	// add perpendicular planes to open boundary edges; or material boundaries

		for(int e_id=0; e_id < obj->el.count; e_id++)
		{
			const Eedge& e = obj->el.list[e_id];

			assert (e.face_count > 0);

			bool mat_boundary = false;
			int first_g = e.group_list[0];
			for(int f_id=1; f_id < e.face_count; f_id++)
			{
				if(e.group_list[f_id] != first_g)
				{
					mat_boundary = true;
					break;
				}
			}

			if(e.face_count == 1 || mat_boundary)
			{
				for(int ef_id=0; ef_id < e.face_count; ef_id++)
				{
					// compute perpendicular plane
					int g_id = e.group_list[ef_id];
					int f_id = e.face_list[ef_id];

					face_group *fg = obj->face_group_list + g_id;
					
					int vb1 = fg->batch_chain[3*f_id];
					int vb2 = fg->batch_chain[3*f_id+1];
					int vb3 = fg->batch_chain[3*f_id+2];

					vb1 = v.object_batch_list[vb1];
					vb2 = v.object_batch_list[vb2];
					vb3 = v.object_batch_list[vb3];
					
					assert(e.v1 == vb1 || e.v1 == vb2 || e.v1 == vb3);
					assert(e.v2 == vb1 || e.v2 == vb2 || e.v2 == vb3);

					float old_normal[3];
					// we recompute normals since we want them to be accurate (0 tolerance)
					calcNormal(v.object_list + 3*vb1,
							   v.object_list + 3*vb2,
							   v.object_list + 3*vb3, 
							   old_normal);

					float ve[3];
					ve[0] = v.object_list[3*e.v2  ] - v.object_list[3*e.v1  ];
					ve[1] = v.object_list[3*e.v2+1] - v.object_list[3*e.v1+1];
					ve[2] = v.object_list[3*e.v2+2] - v.object_list[3*e.v1+2];
					//Normalize3(ve);

					float new_normal[3];
					Cross(old_normal, ve, new_normal);
					Normalize3(new_normal);

					float D = (-Dot3(v.object_list + 3*e.v1, new_normal) -
						        Dot3(v.object_list + 3*e.v2, new_normal) ) * .5;

					Matrix4 q;
					q.quadric(new_normal[0], new_normal[1], new_normal[2], D);

					if(mat_boundary)
					{
						q *= lod_mtl_weight;
					}
					else
					{
						q *= 400.0f;
					}

					v.err[e.v1].quadric += q;
					v.err[e.v2].quadric += q;
				}
			}
		}
	}
}


// used to build initial vertex quadrics
void AddVertexQuadric(object *obj, int vid, int gid, int fid)
{
	vertex_error *err = obj->v.err + vid;

	for(int i=0; i < err->plane_count; i++)
	{
		if( (err->plane_face_list[i] == fid) && (err->plane_group_list[i] == gid) )
		{
			Winprint("Error: duplicate face/vertex quadric!\n");
			exit(1);
		}
	}

	err->plane_count++;
	err->plane_face_list = (int*)Realloc(err->plane_face_list, err->plane_count * sizeof(int));
	err->plane_face_list[err->plane_count - 1] = fid;

	err->plane_group_list = (int*)Realloc(err->plane_group_list, err->plane_count * sizeof(int));
	err->plane_group_list[err->plane_count - 1] = gid;

	const face_group *fg = obj->face_group_list + gid;
	err->quadric += fg->quadric_list[fid];
}

Matrix4 MergeQuadrics(object *obj, int vid1, int vid2, bool add_lists, int *sum_count)
{
	Matrix4 q (obj->v.err[vid1].quadric + obj->v.err[vid2].quadric);

	      vertex_error *err1 = obj->v.err + vid1;
	const vertex_error *err2 = obj->v.err + vid2;

	int original_count1 = err1->plane_count;
	if(sum_count)
		*sum_count = err1->plane_count + err2->plane_count;

	for(int i=0; i < err2->plane_count; i++)
	{
		bool hit = false;
		for(int j=0; j < original_count1; j++)
		{
			if((err2->plane_group_list[i] == err1->plane_group_list[j]) &&
			   (err2->plane_face_list[i]  == err1->plane_face_list[j]) )
			{
				const face_group *fg = obj->face_group_list + err2->plane_group_list[i];
				q -= fg->quadric_list[ err2->plane_face_list[i] ];

				if(sum_count)
					(*sum_count)--;

				hit = true;
				break;
			}	
		}

		if(!hit && add_lists)
		{	
			err1->plane_count++;

			err1->plane_group_list = (int*)Realloc(err1->plane_group_list,
				err1->plane_count * sizeof(int));
			err1->plane_group_list[err1->plane_count - 1] = err2->plane_group_list[i];

			err1->plane_face_list = (int*)Realloc(err1->plane_face_list,
				err1->plane_count * sizeof(int));
			err1->plane_face_list[err1->plane_count - 1] = err2->plane_face_list[i];
		}
	}

	if(sum_count && add_lists)
	{
		assert(*sum_count == err1->plane_count);
	}

	return q;
}

bool checksorting(Eedge_lib *el)
{
	for(int i=0; i < el->count - 1; i++)
	{
		if(el->list[i].error < el->list[i+1].error)
		{
			return false;
		}
	}

	return true;
}

// longest to shortest
// used only when very few edges are unsorted
void BubbleSortEdgesByError(Eedge_lib *el)
{
    //int debug_count=0;
    for(int j = 0; j < el->count - 1; j++)
    {
        bool flag = false;
		const Eedge *pt = el->list;
		const int end = el->count - (j+1);
        for(int i = 0; i < end; i++, pt++)
        {
            //debug_count++;
            if( pt->error < pt[1].error)
            {
                flag = true;
                SwapEdges(el, i, i+1);
            }
        }
        if(!flag) break;
    }

    //printf("sorted %d edges in %d itterations\n", el->count, debug_count);
}

// longest to shortest (list must be sorted except for e_id)
int AllignEdgeByError(Eedge_lib *el, const int eid)
{
	int i = eid;

	while((i < el->count - 1) &&
		  (el->list[eid].error < el->list[i+1].error))
	{
		i++;
	}
	if(i > eid)
	{
		Eedge tmp ( el->list[eid] );
		memmove(el->list + eid, el->list + eid + 1, (i - eid) * sizeof(*(el->list)));
		el->list[i] = tmp;

		return 1;
	}

	while((i > 0) && (el->list[eid].error > el->list[i-1].error))
	{
		i--;
	}
	if(i < eid)
	{
		Eedge tmp ( el->list[eid] );
		memmove(el->list + i + 1, el->list + i, (eid - i) * sizeof(*(el->list)));
		el->list[i] = tmp;

		return -1;
	}
	
	return 0;
}

void calcEdgeAngles(object *obj)
{
	Eedge_lib *el = &(obj->el);
	const normals & n = obj->n;

    // if all face normals are ~ the same then the edge is considered TRIANGULATING
    for(int e_id=0; e_id < el->count; e_id++)
    {
        // if we only have one face we can't be triangulating
        if(el->list[e_id].face_count == 1)
        {
			el->list[e_id].angle = (float)M_PI;
        }
        else
        {
			el->list[e_id].angle = 0.0f;

            for(int rf_id1=0; rf_id1 < el->list[e_id].face_count-1; rf_id1++)
            {
				int g_id1 = el->list[e_id].group_list[rf_id1];
				int f_id1 = el->list[e_id].face_list[rf_id1];
				int n_id1 = obj->face_group_list[g_id1].normal[f_id1];

              for(int rf_id2=rf_id1+1; rf_id2 < el->list[e_id].face_count; rf_id2++)
              {
				int g_id2 = el->list[e_id].group_list[rf_id2];
				int f_id2 = el->list[e_id].face_list[rf_id2];
				int n_id2 = obj->face_group_list[g_id2].normal[f_id2];
			
				el->list[e_id].angle =
					_MAX(el->list[e_id].angle, GetAngle(&(n.list[3*n_id1]), &(n.list[3*n_id2])));
              }
            }
        }
    }

	sortQ(el->list, el->count, sizeof(Eedge), CompareEdgesByAngle,
		NULL, SwapEdges, el);
	//SortEdgesByAngle(el);
}

void calcEdges(object *obj)
{
	if(obj->type != FIXED_MESH && obj->type != DEF_MESH)
	{
		return;
	}

	const vertices & v = obj->v;
	Eedge_lib *el = &(obj->el);

	FreeEedgeLib(el);
	InitEedgeLib(el);
    
	assert(obj->face_group_list);
	
	for(int g_id=0; g_id < obj->face_group_count; g_id++)
	{
		face_group *fg = obj->face_group_list + g_id;
	
		for(int f_id = 0; f_id < fg->count; f_id++)
		{
			const int bid1 = fg->batch_chain[3*f_id];
			const int bid2 = fg->batch_chain[3*f_id + 1];
			const int bid3 = fg->batch_chain[3*f_id + 2];

			const int v_id1 = v.object_batch_list[bid1];
			const int v_id2 = v.object_batch_list[bid2];
			const int v_id3 = v.object_batch_list[bid3];

			InsertEdge(el, v_id1, v_id2);
			InsertEdge(el, v_id2, v_id3);
			InsertEdge(el, v_id3, v_id1);
		}
	}

	for(int eid = 0; eid < el->count; eid++)
	{
		SetEdge_Face_UV(obj, eid);
	}
}

int InsertEdge(Eedge_lib *el, int v_id1, int v_id2)//, int g_id, int f_id, int uv_id1, int uv_id2)
{
    assert(v_id1 != v_id2);

	int swapped;
    int e_id = GetEdgeID(*el, v_id1, v_id2, el->count, &swapped);

    // duplicate edge
    if(e_id >= 0)
    {
		return e_id;
    }

    // add new edge
    el->count++;
    el->max_count++;

    el->list = (Eedge*)Realloc(el->list, el->count * sizeof(Eedge));
	el->list[el->count - 1].Init();

    Eedge & e = el->list[el->count - 1];

	swapped = (v_id1 > v_id2) ? 1 : 0;
	e.v1 = (swapped) ? v_id2 : v_id1;
    e.v2 = (swapped) ? v_id1 : v_id2;

	return (el->count - 1);
}

int GetEdgeID(const Eedge_lib& el, int v_id1, int v_id2, int count, int *swapped)
{
    assert(count <= el.max_count);
    assert(v_id1 != v_id2);
                    
    for(int e_id=0; e_id < count; e_id++)
    {
        Eedge & e = el.list[e_id];
        if(e.v1 == v_id1 && e.v2 == v_id2)
		{
			if(swapped)
				*swapped = 0;
			return e_id;
		}
		else
        if(e.v1 == v_id2 && e.v2 == v_id1)
        {
			if(swapped)
				*swapped = 1;
            return e_id;
        }
    }

    return -1;
}

float EdgeLength(const object *obj, int eid)
{
	int vid1 = obj->el.list[eid].v1;
	int vid2 = obj->el.list[eid].v2;

	assert(vid1 != vid2);

	const vertices & v = obj->v;

    float vector[3];
    vector[0] = v.object_list[3*vid1] - v.object_list[3*vid2];
    vector[1] = v.object_list[3*vid1 + 1] - v.object_list[3*vid2 + 1];
    vector[2] = v.object_list[3*vid1 + 2] - v.object_list[3*vid2 + 2];

    return Magnitude3(vector);
}

// TODO: rewrite for new style
void SynchEdges(object *obj)
{
  const Eedge_lib& el = obj->el;
  edges *e = &(obj->e);
	
  if(el.count > 0)
  {
    e->count = el.count;
    e->vertex_list = (int*)Realloc(e->vertex_list, 2*e->count * sizeof(int));
	e->angle = (float*)Realloc(e->angle, e->count * sizeof(float));
    for(int e_id=0; e_id < el.count; e_id++)
    {
        Eedge *ee = &(el.list[e_id]);
        
        e->vertex_list[2*e_id] = ee->v1;
        e->vertex_list[2*e_id+1] = ee->v2;
		e->angle[e_id] = ee->angle;
    }


	/*
	// new format
	for(int fg_id = 0; fg_id < obj->face_group_count; fg_id++)
	{
		face_group *fg = obj->face_group_list + fg_id;

		fg->adjacent_face_list = (int*)Realloc(fg->adjacent_face_list, 3 * fg->count * sizeof(int));

	}
	*/

	/*
	// old format
	if(!(obj->face_group_list))
	{
		// populate face chunk
		int adjacent_length=0;
		int f_id;
		for(f_id=0; f_id<f->count; f_id++)
		{
			adjacent_length += f->vertex_count[f_id];        
		}
		f->adjacent_face_list = (int*)Malloc(adjacent_length * sizeof(int));

		int index=0;
		for(f_id=0; f_id<f->count; f_id++)
		{
			int first = f->initial_vertex[f_id];
			int vr_count = f->vertex_count[f_id];
			for(int vr_id=0; vr_id<vr_count; vr_id++)
			{
				int vid1 = v.object_chain[first + vr_id];
				int vid2 = v.object_chain[first + (vr_id+1)%vr_count];

				int e_id = GetEdgeID(el, vid1, vid2, el.count);
				assert(e_id != -1);

				f->adjacent_face_list[index] = GetExceptFaceID(el, e_id, f_id); //not f_id
				index++;
			}
		}
		assert(index == adjacent_length);  
	}
	*/
  }
}

/*
int GetExceptFaceID(const Eedge_lib& el, int e_id, int not_fid)
{
    // safety
    if(e_id < 0) return -1;

    for(int i=0; i<el.list[e_id].face_count; i++)
    {
		// TODO use group id as well
        if(el.list[e_id].face_list[i] != not_fid)
        {
            return el.list[e_id].face_list[i];
        }
    }

    return -1;
}
*/

void SwapNormal(object *obj, const int id1, const int id2)
{
  if(id1 == id2) return;

  Swap32(obj->n.list + 3*id1, obj->n.list + 3*id2);
  Swap32(obj->n.list + 3*id1+1, obj->n.list + 3*id2+1);
  Swap32(obj->n.list + 3*id1+2, obj->n.list + 3*id2+2);

  for(int gid=0; gid < obj->face_group_count; gid++)
  {
    face_group *fg = obj->face_group_list + gid;

    for(int fid=0; fid < fg->max_count; fid++)
    {
      if(fg->normal[fid] == id1)
      {
        fg->normal[fid] = id2;
      } else
      if(fg->normal[fid] == id2)
      {
        fg->normal[fid] = id1;
      }
    }
  }

  for(int vid=0; vid < obj->v.max_object_count; vid++)
  {
    if(obj->v.normal[vid] == id1)
    {
      obj->v.normal[vid] = id2;
    } else
    if(obj->v.normal[vid] == id2)
    {
      obj->v.normal[vid] = id1;
    }
  }

  if(obj->lol.count)
  {		
	  lod_lib *lol = &(obj->lol);

	  for(int fid = 0; fid < lol->face_count; fid++)
	  {
		  if(lol->high_normal_list[fid] == id1)
		  {
			  lol->high_normal_list[fid] = id2;
		  } else
		  if(lol->high_normal_list[fid] == id2)
		  {
			  lol->high_normal_list[fid] = id1;
		  }

		  if(lol->low_normal_list[fid] == id1)
		  {
			  lol->low_normal_list[fid] = id2;
		  }else
		  if(lol->low_normal_list[fid] == id2)
		  {
			  lol->low_normal_list[fid] = id1;
		  }
	  }
  }
}

// used by deformable LOD
void RemoveUnusedNormals(object *obj)
{
  bool *used = (bool*)Malloc(obj->n.count * sizeof(bool));
  memset(used, 0, obj->n.count * sizeof(bool));

  // mark used normals
  for(int gid=0; gid < obj->face_group_count; gid++)
  {
    face_group *fg = obj->face_group_list + gid;

    for(int fid=0; fid < fg->count; fid++)
    {
      used[ fg->normal[fid] ] = true;

      int bid1 = fg->batch_chain[3*fid];
      int bid2 = fg->batch_chain[3*fid+1];
      int bid3 = fg->batch_chain[3*fid+2];

      int vid1 = obj->v.object_batch_list[bid1];
      int vid2 = obj->v.object_batch_list[bid2];
      int vid3 = obj->v.object_batch_list[bid3];

      used[ obj->v.normal[vid1] ] = true;
      used[ obj->v.normal[vid2] ] = true;
      used[ obj->v.normal[vid3] ] = true;
    }
  }

  for(int nid = obj->n.count - 1; nid >= 0; nid--)
  {
    if(used[nid] == false)
    {
      SwapNormal(obj, nid, obj->n.count - 1);

      obj->n.count--;
    }
  }

  obj->n.list = (float*)Realloc(obj->n.list, 3 * obj->n.count * sizeof(float));

  Free(used);
}

void RemoveUnusedFaceGroups(object *obj)
{
	if(obj->lol.count > 0 && !exporting_deformable)
	{
		Winprint("Error: can't remove face groups from a continuous lod object!\n");
		return;
	}

	for(int i=0; i < obj->face_group_count; i++)
	{
		if(obj->face_group_list[i].count == 0)
		{
			MemSwap(obj->face_group_list + i, obj->face_group_list + obj->face_group_count - 1,
				sizeof(*(obj->face_group_list)));

			obj->face_group_count--;
			obj->face_group_list = (face_group*)Realloc(obj->face_group_list,
				obj->face_group_count * sizeof(face_group));
			i--;
		}
	}
}

/*void RemoveUnusedMaterials(object *obj)
{
//  if(obj->ml->count < 1) return;
	bool *used_list = (bool*)Malloc(obj->ml->count * sizeof(bool));
	memset(used_list, 0, obj->ml->count * sizeof(bool));

	int i;
	for(i=0; i<obj->face_group_count; i++)
    {
		used_list[ obj->face_group_list[i].material_id ] = true;
	}

	for(i=0; i < obj->ml->count; i++)
	{
		if(!used_list[i])
		{
			fprintf(stderr, "Warning: material %s is assigned to object but not used by any faces!\n",
				obj->ml->materials[i]->name);

			SwapMaterial(obj, i, obj->ml->count - 1);
			used_list[i] = used_list[obj->ml->count - 1];

			FreeMtl(obj->ml->materials[obj->ml->count - 1]);
			Free(obj->ml->materials[obj->ml->count - 1]);

			obj->ml->count--;
			obj->ml->materials=(mtl**)Realloc(obj->ml->materials, obj->ml->count*sizeof(mtl*));
			if(i < obj->ml->count)
			{
				i--;
			}
		}
	}

	Free(used_list);
}*/

void SwapMaterial(object *obj, int id1, int id2)
{
	if(id1 == id2) return;

	cq2Mtl *tmpm = obj->ml->cq2Materials[id1];
	obj->ml->cq2Materials[id1] = obj->ml->cq2Materials[id2];
	obj->ml->cq2Materials[id2] = tmpm;

	int tmpi = obj->ml->cq2Materials[id1]->identifier;
	obj->ml->cq2Materials[id1]->identifier = obj->ml->cq2Materials[id2]->identifier;
	obj->ml->cq2Materials[id2]->identifier = tmpi;

	int api_id = obj->ml->cq2Materials[id1]->api_id;
	obj->ml->cq2Materials[id1]->api_id = obj->ml->cq2Materials[id2]->api_id;
	obj->ml->cq2Materials[id2]->api_id = api_id;

	for(int i=0; i<obj->face_group_count; i++)
	{
		face_group *fg = obj->face_group_list + i;

		if(fg->material_id == id1)
		{
			fg->material_id = id2;
		}
		else
		if(fg->material_id == id2)
		{
			fg->material_id = id1;
		}
	}

	for(i=0; i<obj->v.batch_count; i++)
	{
		if(obj->v.mtl_batch_list[i] == id1)
		{
			obj->v.mtl_batch_list[i] = id2;
		}
		else
		if(obj->v.mtl_batch_list[i] == id2)
		{
			obj->v.mtl_batch_list[i] = id1;
		}
	}
}

/*
// alphabetically & NULL textures first
// TODO: put alpha groups (based on texture and material) last
void SortGroupsByTexture(object *obj)
{
    for(int j=0; j<obj->face_group_count - 1; j++)
    {
        bool flag = false;
        for(int i=0; i<obj->face_group_count - (j+1); i++)
        {
            const int m_id1 = obj->face_group_list[i].material_id;
			const int m_id2 = obj->face_group_list[i+1].material_id;

			const cq2Mtl *m1 = obj->ml->cq2Materials[m_id1];
			const cq2Mtl *m2 = obj->ml->cq2Materials[m_id2];
	
			if(
				// no texture first
				(( m1->diffuse.texture_name ||  m1->diffuse.anim_texture_name) && 
				 (!m2->diffuse.texture_name && !m2->diffuse.anim_texture_name) )
				||
				// static texture before animated texture
				(m1->diffuse.anim_texture_name && !m2->diffuse.anim_texture_name) 
				||
				(m1->diffuse.anim_texture_name && m2->diffuse.anim_texture_name &&
				 strcmp(m1->diffuse.anim_texture_name, m2->diffuse.anim_texture_name) > 0 )
				||
				(m1->diffuse.texture_name && m2->diffuse.texture_name &&
				 strcmp(m1->diffuse.texture_name, m2->diffuse.texture_name) > 0)
			)
			{
				flag = true;
				MemSwap(obj->face_group_list + i, obj->face_group_list + i + 1, sizeof(face_group));
			}
        }
        if(!flag)
			break;
    }
}*/

void PostProcessMesh(object *obj, const float nDensity, const float nLodPercent, const float nLodClosestDist,
					 const float nLodFurthestDist, const bool rigid_flag, const char *name,
					 const float scale)
{
	if(obj->type == FIXED_MESH || obj->type == DEF_MESH)
	{
#ifdef SGI
		RemoveUnusedMaterials(obj);
#endif
		MergeGroupsByMaterial(obj);
		//SortGroupsByTexture(obj);

#ifdef WIN32
		if(scale != 1.0f)
		{
			ScaleObject(obj, scale);
			if(obj->v.vertex_bone_count != NULL)
			{
				ScaleObjectDeformable(obj, scale);
			}
		}
#endif

		calcVertexNormals(obj, 0/*SMOOTH_SHADED*/);

		// lod objects get sorted at their minimum LOD
		if(nLodPercent == 100.0f)
		{
			SortFacesByArea(obj);
			//StreamlineVertices(obj);
		}

		calcEdges(obj);
		calcEdgeAngles(obj);
	
		// deformable meshes have to do LOD later, after bones are assigned
		if((nLodPercent < 100.0f) && obj->type == FIXED_MESH)
		{
			assert(!exporting_deformable);
			CollapseEdges(obj, (int)((.01f * nLodPercent) * obj->face_count + .5f) );
			obj->lol.closest = nLodClosestDist;
			obj->lol.furthest = nLodFurthestDist;
		}
	}

	if(rigid_flag) // bonde rigid stuff is done later based on mesh
	{
		calcRigidBody(obj, nDensity, -1, name, true);
	}


	//CopyDAMesh( obj );
}

/*
void StreamlineVertices(object *obj)
{
	int bid_index = 0;
	int vid_index = 0;
	int tid_index = 0;
	int nid_index = 0;

	vertices *v = &(obj->v);

	for(int gid = 0; gid < obj->face_group_count; gid++)
	{
		face_group *fg = obj->face_group_list + gid;

		for(int fid = 0; fid < fg->count; fid++)
		{
			int f_nid = fg->normal[fid];
			if(f_nid > nid_index)
			{
				SwapNormal(obj, f_nid, nid_index);
				nid_index++;
			}

			for(int j = 0; j < 3; j++)
			{
				int bid, vid, tid, v_nid;
			
				bid = fg->batch_chain[3*fid+j];
				vid = v->object_batch_list[bid];
				
				v_nid = v->normal[vid];
				if(v_nid > nid_index)
				{
					SwapNormal(obj, v_nid, nid_index);
					nid_index++;
					assert(bid == fg->batch_chain[3*fid+j]);
					assert(vid == v->object_batch_list[bid]);
				}
				
				if(vid > vid_index)
				{
					SwapVertices(obj, vid, vid_index);
					vid_index++;
					assert(bid == fg->batch_chain[3*fid+j]);
				}

				
				tid = v->texture_batch_list[bid];
				if(tid > tid_index)
				{
					SwapUVVertices(obj, tid, tid_index);
					tid_index++;
					assert(bid == fg->batch_chain[3*fid+j]);
				}
				
				if(bid > bid_index) // move closer to front
				{
					SwapBatchIndecies(obj, bid, bid_index);
					bid_index++;
				}
			}
		}
	}
}
*/
void SortFacesByArea(object *obj)
{
	for(int gid = 0; gid < obj->face_group_count; gid++)
	{
		obj->active_group = gid;
		face_group *fg = obj->face_group_list + gid;
		sortQ(fg->batch_chain, fg->count, 3 * sizeof(int), CompareFacesByArea, &(obj->v), SwapLodFaces, obj);
	}
}

/*
void SwapFaces(void *swap_lib, const int id1, const int id2)
{
	if(id1 != id2)
	{
		object *obj = (object*)swap_lib;
		face_group *fg = obj->face_group_list + obj->active_group; //(face_group*)swap_lib;
		assert(fg->count > 0 && id1 < fg->count && id2 < fg->count);

		Swap32(fg->normal + id1, fg->normal + id2);
		Swap32(fg->D_coefficient + id1, fg->D_coefficient + id2);
		Swap32(fg->property + id1, fg->property + id2);

		Swap32(fg->batch_chain + 3*id1,     fg->batch_chain + 3*id2);
		Swap32(fg->batch_chain + 3*id1 + 1, fg->batch_chain + 3*id2 + 1);
		Swap32(fg->batch_chain + 3*id1 + 2, fg->batch_chain + 3*id2 + 2);

		Swap32(fg->api_node_id + id1, fg->api_node_id + id2);
		Swap32(fg->api_face_id + id1, fg->api_face_id + id2);
		Swap32(fg->api_smg_id + id1, fg->api_smg_id + id2);
	}
}
*/

// largest to smallest
int CompareFacesByArea(const void *pt1, const void *pt2, const void *cmp_lib)
{
  const vertices *v = (vertices*)cmp_lib;
  
  const int *idx_list1 = (int*)pt1;
  const float *p1 = v->object_list + 3 * v->object_batch_list[ idx_list1[0] ];
  const float *p2 = v->object_list + 3 * v->object_batch_list[ idx_list1[1] ];
  const float *p3 = v->object_list + 3 * v->object_batch_list[ idx_list1[2] ];

  float v1[3], v2[3], v3[3];
  
  v1[0] = p2[0]-p1[0]; 
  v1[1] = p2[1]-p1[1];
  v1[2] = p2[2]-p1[2];

  v2[0] = p3[0]-p1[0]; 
  v2[1] = p3[1]-p1[1];
  v2[2] = p3[2]-p1[2];

  Cross(v1, v2, v3);
  const float a1 = Magnitude3_sq(v3);

  const int *idx_list2 = (int*)pt2;
  p1 = v->object_list + 3 * v->object_batch_list[ idx_list2[0] ];
  p2 = v->object_list + 3 * v->object_batch_list[ idx_list2[1] ];
  p3 = v->object_list + 3 * v->object_batch_list[ idx_list2[2] ];

  v1[0] = p2[0]-p1[0]; 
  v1[1] = p2[1]-p1[1];
  v1[2] = p2[2]-p1[2];

  v2[0] = p3[0]-p1[0]; 
  v2[1] = p3[1]-p1[1];
  v2[2] = p3[2]-p1[2];

  Cross(v1, v2, v3);
  const float a2 = Magnitude3_sq(v3);

  if(a1 < a2)
  {
	  return 1;
  }
  else
  if(a1 > a2)
  {
	  return -1;
  }
  
  return 0;
}

void MergeGroupsByMaterial(object *obj)
{
	for(int j = 0; j < obj->face_group_count - 1; j++)
	{
		for(int i = j+1; i < obj->face_group_count; i++)
        {
redo_label:
			const int m_id1 = obj->face_group_list[j].material_id;
			const int m_id2 = obj->face_group_list[i].material_id;

			const cq2Mtl *m1 = obj->ml->cq2Materials[m_id1];
			const cq2Mtl *m2 = obj->ml->cq2Materials[m_id2];

			if( SameMtl(m1, m2) )
			{
				int swap_id = -1;
				int keep_id = -1;
				const cq2Mtl *swap_mtl = NULL;

				/*
				if(m2->diffuse.pointed_to_by_atl == false)
				{
					swap_id = m_id2;
					keep_id = m_id1;
					swap_mtl = m2;
				}else
				if(m1->diffuse.pointed_to_by_atl == false)
				{
					swap_id = m_id1;
					keep_id = m_id2;
					swap_mtl = m1;
				}*/

				if(swap_id >= 0)
				{
					MemSwap(obj->face_group_list + i,
							obj->face_group_list + obj->face_group_count - 1,
						sizeof(face_group));

					MergeGroups(obj, j, obj->face_group_count - 1); // drops group count
					obj->face_group_list[j].material_id = keep_id;

					fprintf(stderr, "Warning: material %s is not used after merging!\n",
						swap_mtl->name);

					SwapMaterial(obj, swap_id, obj->ml->cq2Count - 1);

					for(int bid=0; bid < obj->v.batch_count; bid++)
					{
						if(obj->v.mtl_batch_list[bid] == obj->ml->cq2Count - 1)
						{
							obj->v.mtl_batch_list[bid] = keep_id;
						}
					}

					//FreeMtl(obj->ml->cq2Materials[obj->ml->cq2Count - 1]);
					//Free(obj->ml->cq2Materials[obj->ml->cq2Count - 1]);
					obj->ml->cq2Count--;
					obj->ml->cq2Materials=(cq2Mtl**)Realloc(obj->ml->cq2Materials, obj->ml->cq2Count*sizeof(cq2Mtl*));

					if(i < obj->face_group_count)
						goto redo_label;
				}
			}
		}
	}
}

void MergeGroups(object *obj, const int id1, const int id2)
{
	assert(id2 == obj->face_group_count - 1);

	face_group *fg1 = obj->face_group_list + id1;
	face_group *fg2 = obj->face_group_list + id2;

	assert(fg1->count == fg1->max_count);
	assert(fg2->count == fg2->max_count);

	const int new_f_cnt = fg1->count + fg2->count;

	fg1->batch_chain = (int*)Realloc(fg1->batch_chain, 3 * new_f_cnt * sizeof(int));
	memcpy(fg1->batch_chain + 3 * fg1->count, fg2->batch_chain, 3 * fg2->count * sizeof(int));

	fg1->normal = (int*)Realloc(fg1->normal, new_f_cnt * sizeof(int));
	memcpy(fg1->normal + fg1->count, fg2->normal, fg2->count * sizeof(int));

	fg1->D_coefficient = (float*)Realloc(fg1->D_coefficient, new_f_cnt * sizeof(float));
	memcpy(fg1->D_coefficient + fg1->count, fg2->D_coefficient, fg2->count * sizeof(float));

	fg1->property = (FACE_PROPERTY*)Realloc(fg1->property, new_f_cnt * sizeof(FACE_PROPERTY));
	memcpy(fg1->property + fg1->count, fg2->property, fg2->count * sizeof(FACE_PROPERTY));

	fg1->api_node_id = (int*)Realloc(fg1->api_node_id, new_f_cnt * sizeof(int));
	memcpy(fg1->api_node_id + fg1->count, fg2->api_node_id, fg2->count * sizeof(int));

	fg1->api_face_id = (int*)Realloc(fg1->api_face_id, new_f_cnt * sizeof(int));
	memcpy(fg1->api_face_id + fg1->count, fg2->api_face_id, fg2->count * sizeof(int));

	fg1->api_smg_id = (int*)Realloc(fg1->api_smg_id, new_f_cnt * sizeof(int));
	memcpy(fg1->api_smg_id + fg1->count, fg2->api_smg_id, fg2->count * sizeof(int));

	assert(!fg1->quadric_list && !fg2->quadric_list);
	assert(!fg1->adjacent_face_list && !fg2->adjacent_face_list);

	fg1->count = new_f_cnt;
	fg1->max_count = new_f_cnt;


	FreeFaceGroup(fg2);

	obj->face_group_count--;
	obj->face_group_list = (face_group*)Realloc(obj->face_group_list,
		obj->face_group_count * sizeof(face_group));
}

float Dist3(const float p1[3], const float p2[3])
{
    return (float)sqrt(DistSq3(p1, p2));
}

float DistSq3(const float p1[3], const float p2[3])
{
    float v[3] = {p1[0]-p2[0], p1[1]-p2[1], p1[2]-p2[2]};

    return Dot3(v, v);
}

float GetVertexAngle(const float vertices[9], const int id /* 0-2 */)
{
  assert(id >= 0 && id < 3);

  int vid1, vid2, vid3;
  if(id == 0)
  {
	vid1 = 2;
	vid2 = 0;
	vid3 = 1;
  }
  else
  if(id == 1)
  {
    vid1 = 0;
	vid2 = 1;
	vid3 = 2;
  }
  else
  {
	  vid1 = 1;
	  vid2 = 2;
	  vid3 = 0;
  }

  float vec1[3] = {
	vertices[3*vid1 + 0] - vertices[3*vid2 + 0],
	vertices[3*vid1 + 1] - vertices[3*vid2 + 1],
	vertices[3*vid1 + 2] - vertices[3*vid2 + 2] };
  //Normalize3(vec1);

  float vec2[3] = {
	vertices[3*vid3 + 0] - vertices[3*vid2 + 0],
	vertices[3*vid3 + 1] - vertices[3*vid2 + 1],
	vertices[3*vid3 + 2] - vertices[3*vid2 + 2] };
  //Normalize3(vec2);

  return Acos( Dot3(vec1, vec2) / sqrt( Magnitude3_sq(vec1) * Magnitude3_sq(vec2) ) );
}

float GetNewVertexAngle(const object *obj, int g_id, int f_id, int v_id)
{
	assert(v_id < 3 && v_id >= 0);
	assert(g_id < obj->face_group_count);

	face_group *fg = obj->face_group_list + g_id;
	assert(f_id < fg->count);

	int vb1, vb2, vb3;
	if(v_id == 0)
	{
		vb1 = fg->batch_chain[3*f_id+2];
		vb2 = fg->batch_chain[3*f_id];
		vb3 = fg->batch_chain[3*f_id+1];
	}
	else
	if(v_id == 1)
	{
		vb1 = fg->batch_chain[3*f_id];
		vb2 = fg->batch_chain[3*f_id+1];
		vb3 = fg->batch_chain[3*f_id+2];
	}
	else
	{
		vb1 = fg->batch_chain[3*f_id+1];
		vb2 = fg->batch_chain[3*f_id+2];
		vb3 = fg->batch_chain[3*f_id];
	}

	vb1 = obj->v.object_batch_list[vb1];
	vb2 = obj->v.object_batch_list[vb2];
	vb3 = obj->v.object_batch_list[vb3];
	

	float tmp[9];
	tmp[0] = obj->v.object_list[3*vb1];
	tmp[1] = obj->v.object_list[3*vb1+1];
	tmp[2] = obj->v.object_list[3*vb1+2];

	tmp[3] = obj->v.object_list[3*vb2];
	tmp[4] = obj->v.object_list[3*vb2+1];
	tmp[5] = obj->v.object_list[3*vb2+2];

	tmp[6] = obj->v.object_list[3*vb3];
	tmp[7] = obj->v.object_list[3*vb3+1];
	tmp[8] = obj->v.object_list[3*vb3+2];

	return GetVertexAngle(tmp, 1);

  /*
  const vertices *v = &(obj->v);
  float vec1[3], vec2[3];

  vec1[0] = v->object_list[3*vb1  ] - v->object_list[3*vb2];
  vec1[1] = v->object_list[3*vb1+1] - v->object_list[3*vb2+1];
  vec1[2] = v->object_list[3*vb1+2] - v->object_list[3*vb2+2];
  Normalize3(vec1);
 
  vec2[0] = v->object_list[3*vb3  ] - v->object_list[3*vb2];
  vec2[1] = v->object_list[3*vb3+1] - v->object_list[3*vb2+1];
  vec2[2] = v->object_list[3*vb3+2] - v->object_list[3*vb2+2];
  Normalize3(vec2);

  return Acos(Dot3(vec1, vec2));
  */
}


/*
float GetVertexAngle(int init_vid, int fv_count, const vertices *v, int v_id)
{
  int ve_id1=-1, ve_id2=-1;
  int i;

  // get end points
  for(i=0; i<fv_count; i++)
  {
    if(v->object_chain[init_vid+i] == v_id)
    {
      if(i == 0 ){
        ve_id1 = v->object_chain[init_vid + (fv_count-1)];
        ve_id2 = v->object_chain[init_vid + 1];
      }else
      if(i == (fv_count-1)){
         ve_id1 = v->object_chain[init_vid + (i-1)];
         ve_id2 = v->object_chain[init_vid];
      }
      else
      {
        ve_id1 = v->object_chain[init_vid + (i-1)];
        ve_id2 = v->object_chain[init_vid + (i+1)];
      }
      break;
    }
  }
 
  float tmp[9];
  tmp[0] = v->object_list[3*ve_id1];
  tmp[1] = v->object_list[3*ve_id1+1];
  tmp[2] = v->object_list[3*ve_id1+2];

  tmp[3] = v->object_list[3*v_id];
  tmp[4] = v->object_list[3*v_id+1];
  tmp[5] = v->object_list[3*v_id+2];

  tmp[6] = v->object_list[3*ve_id2];
  tmp[7] = v->object_list[3*ve_id2+1];
  tmp[8] = v->object_list[3*ve_id2+2];

  return GetVertexAngle(tmp, 1);
}
*/

/*
float GetVertexAngle(const faces *f, const vertices *v, int f_id, int v_id)
{
	return GetVertexAngle(f->initial_vertex[f_id], f->vertex_count[f_id], v, v_id);
}
*/

void ReplaceChar(char *name, const char c1, const char c2)
{
	int i = strlen(name) - 1;
	while(i--)
	{
		if(name[i] == c1)
		{
			name[i] = c2;
		}
	}
}

void ReplaceFirstChar(char *name, const char c1, const char c2)
{
	for(unsigned int i = 0; i < strlen(name); i++)
	{
		if(name[i] == c1)
		{
			name[i] = c2;
			return;
		}
	}
}

void ReplaceBeforeEqual(char *name)
{
	const int length = strlen(name);
	for(int i = 0; i < length; i++)
	{
		if(name[i] == '=')
		{
			name[i] = 0;
			i--;
			while(i >= 0 && (name[i] == ' ' || name[i] == '\t'))
			{
				name[i] = 0;
				i--;
			}
			return;
		}
	}
}

void FixName(char *name)
{
    int len = strlen(name);

    for(int i=0; i<len; i++)
	{
        if(name[i]==' ')
            name[i]='_';
    }
}

int StripPrefix(char *name, const char *prefix)
{
  char *pt;
  int length;

  length=strlen(prefix);

  if(!strncmp(name, prefix, length))
  {
    pt = name + length;
    strcpy(name, pt);
    return 1;
  }

  return 0;
}
 
void RemoveLines(Vector **vlist, int *vcount, bool *is_flat)
{
	assert(*vcount >= 3);

	*is_flat = true;

	float d_tol = .001f;
	float normal[3];
	if(-1 != calcNormal((float*)(*vlist), (float*)((*vlist)+1), (float*)((*vlist)+2), normal))
	{
		float D = ( -Dot3( normal, (float*)((*vlist)  ) ) -
			         Dot3( normal, (float*)((*vlist)+1) ) -
				     Dot3( normal, (float*)((*vlist)+2) ) ) * (1.0 / 3.0);
			

		for(int i=3; i < *vcount; i++)
		{
			float tmp_normal[3];
			if(-1 != calcNormal((float*)(*vlist), (float*)((*vlist)+1), (float*)((*vlist)+i), tmp_normal))
			{
				float tmp_D = ( -Dot3( tmp_normal, (float*)((*vlist)  ) ) -
								 Dot3( tmp_normal, (float*)((*vlist)+1) ) -
								 Dot3( tmp_normal, (float*)((*vlist)+i) ) ) * (1.0 / 3.0);


				float dot = Dot3(normal, tmp_normal);
				if( fabs( dot ) < cos(3.0 * D2R) ||
					( dot > 0.0 &&
					  fabs(D - tmp_D) > d_tol ) ||
					( dot < 0.0 &&
					  fabs(D) - fabs(tmp_D) > d_tol )
				  )
				{
					*is_flat = false;
					//return false;
				}
			}
			else // remove middle point on line
			{
				

			}
		}
	}
	else
	{
		// ok for NURBS
		//Winprint("Error: first face has bad normal! Contact Mike S.\n");
	}

	//return true;
}

bool FixIfFlat(Vector **vlist, int *vcount)
{
	assert(*vcount >= 3);

	bool result = false;
	
	bool is_flat;
	RemoveLines(vlist, vcount, &is_flat);
	if( is_flat )
	{
		float scale = _MIN(
					_MIN(Dist3((float*)(*vlist), (float*)((*vlist)+1)),
						  Dist3((float*)(*vlist), (float*)((*vlist)+2)) ),
					Dist3((float*)((*vlist)+1), (float*)((*vlist)+2)) );
		scale /= 100.0f;
	
		float normal[3];
		int n_res = calcNormal((float*)(*vlist), 
							   (float*)((*vlist)+((*vcount) - 1)/2), 
							   (float*)((*vlist)+((*vcount) - 1)), normal);
		int vn_id = 2;
		while(n_res == -1 && vn_id < *vcount)
		{
			n_res = calcNormal((float*)(*vlist), 
							   (float*)((*vlist)+1), 
							   (float*)((*vlist)+vn_id), normal);
			vn_id++;
		}

		if(n_res == -1)
		{
			return result;
		}

		Vector *tmp_vlist = (Vector*)Malloc( 2 * *vcount * sizeof(Vector));
		memcpy(tmp_vlist, *vlist, *vcount * sizeof(Vector));
		memcpy(tmp_vlist + *vcount, (*vlist), *vcount * sizeof(Vector));

		int i;
		float fraction = .5f * scale;
		for(i=0; i < *vcount; i++)
		{
			tmp_vlist[i].x = (*vlist)[i].x + fraction * normal[0] +
				0.00001f * (0.5f - rand()/float(RAND_MAX));
			tmp_vlist[i].y = (*vlist)[i].y + fraction * normal[1] +
				0.00001f * (0.5f - rand()/float(RAND_MAX));
			tmp_vlist[i].z = (*vlist)[i].z + fraction * normal[2] +
				0.00001f * (0.5f - rand()/float(RAND_MAX));
		}

		for(i=*vcount; i < 2 * *vcount; i++)
		{
			tmp_vlist[i].x = (*vlist)[i - *vcount].x - fraction * normal[0] +
				0.00001f * (0.5f - rand()/float(RAND_MAX));
			tmp_vlist[i].y = (*vlist)[i - *vcount].y - fraction * normal[1] +
				0.00001f * (0.5f - rand()/float(RAND_MAX));
			tmp_vlist[i].z = (*vlist)[i - *vcount].z - fraction * normal[2] +
				0.00001f * (0.5f - rand()/float(RAND_MAX));
		}

		(*vcount) *= 2;

		Free(*vlist);
		*vlist = tmp_vlist;

		result = true;
	}

	return result;
}

void calcRigidBody(object *obj, float density, float, const char name[64], bool)
{
#if USE_DA_MESH
  if( obj->type == FIXED_MESH )
  {
	const int vcount = obj->da_mesh.v_cnt;
    Vector *vlist = (Vector*)Malloc(vcount * sizeof(Vector));
	memcpy(vlist, obj->da_mesh.v_list, vcount * sizeof(Vector));

    calcRigidBody(obj->extents, vlist, vcount, density, name);

    Free(vlist);
  }
  else
#endif
  if((obj->type == FIXED_MESH || obj->type == DEF_MESH) && obj->v.object_count > 0)
  {
    const int vcount = obj->v.object_count;
    Vector *vlist = (Vector*)Malloc(vcount * sizeof(Vector));
	memcpy(vlist, obj->v.object_list, vcount * sizeof(Vector));

    calcRigidBody(obj->extents, vlist, vcount, density, name);

    Free(vlist);
  }
  else
  if( (obj->type == FIXED_NURB || obj->type == DEF_NURB) && obj->nurb_count > 0)
  {
	  int vcount = 0;
	  for(nurb *nr = obj->nurb_list; nr < obj->nurb_list + obj->nurb_count; nr++)
	  {
		  vcount += nr->s_point_count * nr->t_point_count;
	  }

	  Vector *vlist = (Vector*)Malloc(vcount * sizeof(Vector));

	  Vector *vp = vlist;

	  for(nr = obj->nurb_list; nr < obj->nurb_list + obj->nurb_count; nr++)
	  {
			for(float *pt = nr->point_list;
			  pt < nr->point_list + 3 * nr->s_point_count * nr->t_point_count;
			  )
			{
				  vp->x = *pt++;
				  vp->y = *pt++;
				  vp->z = *pt++;
				  vp++;
			}
	  }

	  calcRigidBody(obj->extents, vlist, vcount, density, name);

	  Free(vlist);
  }else
  if( (obj->type == FIXED_PATCH || obj->type == DEF_PATCH) && obj->b_mesh.group_cnt > 0)
  {
#if 0 // use vectors as well
	  int vcount = obj->b_mesh.vertex_cnt + obj->b_mesh.vector_cnt;
	  Vector *vlist = (Vector*)Malloc(vcount * sizeof(Vector));
	  memcpy(vlist, obj->b_mesh.vertices, obj->b_mesh.vertex_cnt * sizeof(Vector));
	  memcpy(vlist + obj->b_mesh.vertex_cnt, obj->b_mesh.vectors, obj->b_mesh.vector_cnt * sizeof(Vector));
#else // use vertices only
	  int vcount = obj->b_mesh.vertex_cnt;
	  Vector *vlist = (Vector*)Malloc(vcount * sizeof(Vector));
	  memcpy(vlist, obj->b_mesh.vertices, obj->b_mesh.vertex_cnt * sizeof(Vector));
#endif
	  calcRigidBody(obj->extents, vlist, vcount, density, name);

	  Free(vlist);
  }
  else
  {
    fprintf(stderr, "Warning: %s has NO vertices for RigidBody!\n", name);
  }
}

void calcRigidBody(Extents& extents, const Vector *orig_vlist, const int orig_vcount, float density,
				   const char name[64])
{
  int vcount;
  Vector *vlist;
  if(orig_vcount >= 3)
  {
	  vcount = orig_vcount;
	  vlist = (Vector*)Malloc(vcount * sizeof(Vector));
	  memcpy(vlist, orig_vlist, vcount * sizeof(Vector));

	  FixIfFlat(&vlist, &vcount);
  }
  else
  {
    if(!exporting_deformable)
    {
      Winprint("Warning: %s does not have enough vertices for extents!\n", name);
    }
    return;
  }

  
  bool hull_success = true;
  // compute convex hull
  extents.compute_convex_hull(vcount, vlist);
  if(extents.convex_hull.num_triangles == 0)
  {
	  Winprint("Warning: compute_convex_hull for %s failed!\n", name);
	  hull_success = false;
  }

  TrapFpu(true);

  // make sure we have the minimum convex hull (tetrahedron)
  if(extents.convex_hull.num_edges < 6 ||
     extents.convex_hull.num_vertices < 4 ||
     extents.convex_hull.num_triangles < 4 ) 
  {
  	fprintf(stderr,"Warning: Convex Hull of %s is simpler than a tetrahedron! Contact Mike S.\n", name);
	hull_success = false;
  }

  // last resort work around
  if(!hull_success)
  {
	  extents.convex_hull.volume = 0.0f; // so it won't export
	 
	  // compute old style box & sphere
	  extents.box.compute_bounds(vcount, vlist);
	  extents.compute_mass(density, -1);
	  extents.box.tr.identity(); // since the box is exactly at it's own center of mass
	  
	  extents.compute_sphere(vcount, vlist);
	  extents.sphere.render_radius = extents.sphere.radius;
	  extents.sphere.tr.m.identity();
	  extents.sphere.tr.v = PersistVector(extents.sphere.center.x - extents.mass.center.x,
										  extents.sphere.center.y - extents.mass.center.y,
										  extents.sphere.center.z - extents.mass.center.z);
	  extents.sphere.render_center = extents.mass.center + extents.sphere.tr.v;
	  

	  // assign names
	  strcpy(extents.sphere.name, name);
	  strcat(extents.sphere.name, " Sphere 1");
	  strcpy(extents.box.name, name);
	  strcat(extents.box.name, " Box 1");

	  Free(vlist);
	  TrapFpu(false);
	  return;
  }
  
  // only use vertices of convex hull for the rest
  vcount = extents.convex_hull.num_vertices;
  vlist = (Vector*)Realloc(vlist, vcount * sizeof(Vector));
  for(int i=0; i<vcount; i++)
  {
	vlist[i].x = extents.convex_hull.vertices[i].p.x;
	vlist[i].y = extents.convex_hull.vertices[i].p.y;
	vlist[i].z = extents.convex_hull.vertices[i].p.z;
  }

  // get center of mass & inertia matrix
  POLYHEDRON p;
  LoadPolyhedron(&p, &(extents.convex_hull));
  
  TrapFpu(false); // fix
  compVolumeIntegrals(&p, density);
  TrapFpu(true);

  if( !is_float( p.volume ) )
  {
#ifdef WIN32
  assert(_isnan(p.volume));
#endif
	  fprintf(stderr, "POLYHEDRON volume of convex hull is NaN!\n");
	  p.volume = 0.0f;
	  p.mass = 0.0f;
  }


  // finish convex hull
  extents.convex_hull.volume = p.volume;
  extents.convex_hull.tr.identity();
  extents.convex_hull.tr.v = PersistVector(-p.c_o_m[0], -p.c_o_m[1], -p.c_o_m[2]);

  // set mass mass center inertia
  if(p.mass > 0.0)
  {
	  extents.mass.mass = p.mass;

	  extents.mass.center.x = p.c_o_m[0];
	  extents.mass.center.y = p.c_o_m[1];
	  extents.mass.center.z = p.c_o_m[2];

	  extents.mass.inertia.d[0][0] = p.i_t[0][0];
	  extents.mass.inertia.d[0][1] = p.i_t[0][1];
	  extents.mass.inertia.d[0][2] = p.i_t[0][2];

	  extents.mass.inertia.d[1][0] = p.i_t[1][0];
	  extents.mass.inertia.d[1][1] = p.i_t[1][1];
	  extents.mass.inertia.d[1][2] = p.i_t[1][2];

	  extents.mass.inertia.d[2][0] = p.i_t[2][0];
	  extents.mass.inertia.d[2][1] = p.i_t[2][1];
	  extents.mass.inertia.d[2][2] = p.i_t[2][2];
  }
  else
  {	  
	  // use box instead
	  extents.box.compute_bounds(vcount, vlist);
	  extents.compute_mass(density, -1);
  }

  p.Release();

  // make points relative to center of mass
  for(i=0; i < vcount; i++)
  {
	  vlist[i].x -= extents.mass.center.x;
	  vlist[i].y -= extents.mass.center.y;
	  vlist[i].z -= extents.mass.center.z;
  }

  // compute SPHERE
  extents.compute_sphere(vcount, vlist);
  extents.sphere.render_radius = extents.sphere.radius;
  extents.sphere.tr.m.identity();
  extents.sphere.tr.v = PersistVector(extents.sphere.center.x, extents.sphere.center.y, extents.sphere.center.z);
  extents.sphere.render_center = extents.mass.center + extents.sphere.tr.v;

  if(no_physics)
  {
	  TrapFpu(false);
	  return;
  }

  // compute box
  {
	  // we need a decent number of vertices for the oriented box
	  const int d_vcount = (vcount >= 50) ? vcount : 50;
	  ExtPoint3 *d_vlist = (ExtPoint3*)Malloc(d_vcount*sizeof(ExtPoint3));
	  int i;
	  for(i=0; i < vcount; i++)
	  {
		d_vlist[i].x = vlist[i].x;
		d_vlist[i].y = vlist[i].y; 
		d_vlist[i].z = vlist[i].z; 
	  }
	  // possibly generate some extra points to make the results more accurate
	  assert(vcount >= 4);
	  for(i = vcount; i < d_vcount; i++)
	  {
		int r1 = (rand() * vcount) / (RAND_MAX + 1);
		int r2 = (rand() * vcount) / (RAND_MAX + 1);
		int r3 = (rand() * vcount) / (RAND_MAX + 1);
		//int r4 = (rand() * vcount) / (RAND_MAX + 1);

		float fraction[4] = {(float)rand() / (float)RAND_MAX,
							 (float)rand() / (float)RAND_MAX,
							 (float)rand() / (float)RAND_MAX,
							 (float)rand() / (float)RAND_MAX};
		float sum = fraction[0] + fraction[1] + fraction[2];// + fraction[3];
		sum = 1.0f / sum;
		fraction[0] *= sum;
		fraction[1] *= sum;
		fraction[2] *= sum;
		//fraction[3] *= sum;
	
		d_vlist[i].x = fraction[0] * vlist[r1].x +
					   fraction[1] * vlist[r2].x +
					   fraction[2] * vlist[r3].x;// +
					   //fraction[3] * vlist[r4].x;

		d_vlist[i].y = fraction[0] * vlist[r1].y +
					   fraction[1] * vlist[r2].y +
					   fraction[2] * vlist[r3].y;// +
					   //fraction[3] * vlist[r4].y;

		d_vlist[i].z = fraction[0] * vlist[r1].z +
					   fraction[1] * vlist[r2].z +
					   fraction[2] * vlist[r3].z;// +
					   //fraction[3] * vlist[r4].z;
	  }


	  OBBox3 orient_box = MinimalBox3 (d_vcount, d_vlist);
	  Free(d_vlist);

	  // regular non oriented box  //ExtentBox 
	  extents.box.compute_bounds(vcount, vlist);

	  // see which is better
	  if(.9999 * extents.box.volume < orient_box.volume)
	  {
		  // get eigen rotation
		  Vector axes[3] = { Vector(orient_box.axis[0].x, orient_box.axis[0].y, orient_box.axis[0].z),
							 Vector(orient_box.axis[1].x, orient_box.axis[1].y, orient_box.axis[1].z),
							 Vector(orient_box.axis[2].x, orient_box.axis[2].y, orient_box.axis[2].z) };
							
		  axes[0].normalize();
		  axes[1].normalize();
		  axes[2].normalize();

		  // sanity check
		  assert( fabs( dot_product(axes[0], axes[1]) ) < .0001f);
		  assert( fabs( dot_product(axes[0], axes[2]) ) < .0001f);
		  assert( fabs( dot_product(axes[1], axes[2]) ) < .0001f);

		  extents.box.tr.m.e00 = axes[0].x;
		  extents.box.tr.m.e10 = axes[0].y;
		  extents.box.tr.m.e20 = axes[0].z;

		  extents.box.tr.m.e01 = axes[1].x;
		  extents.box.tr.m.e11 = axes[1].y;
		  extents.box.tr.m.e21 = axes[1].z;

		  extents.box.tr.m.e02 = axes[2].x;
		  extents.box.tr.m.e12 = axes[2].y;
		  extents.box.tr.m.e22 = axes[2].z;

		  extents.box.tr.v.x = extents.box.center.x = orient_box.center.x;
		  extents.box.tr.v.y = extents.box.center.y = orient_box.center.y;
		  extents.box.tr.v.z = extents.box.center.z = orient_box.center.z;
		  
		  extents.box.size.x = orient_box.extent[0];
		  extents.box.size.y = orient_box.extent[1];
		  extents.box.size.z = orient_box.extent[2];

		  extents.box.volume = orient_box.volume;	

		  // rotate vlist to local coord
		  PersistMatrix & ptm = extents.box.tr.m;
		  for(i=0; i < vcount; i++)
		  {
			// subtract off center of mass
			Vector tmp_v(vlist[i].x, vlist[i].y, vlist[i].z);
			
			// inverse rotation
			vlist[i].x = tmp_v.x * ptm.e00 + tmp_v.y * ptm.e10 + tmp_v.z * ptm.e20;
			vlist[i].y = tmp_v.x * ptm.e01 + tmp_v.y * ptm.e11 + tmp_v.z * ptm.e21;
			vlist[i].z = tmp_v.x * ptm.e02 + tmp_v.y * ptm.e12 + tmp_v.z * ptm.e22;
		  }

	  }
	  else
	  {
		  extents.box.tr.identity();
		  extents.box.tr.v.x = extents.box.center.x;
		  extents.box.tr.v.y = extents.box.center.y;
		  extents.box.tr.v.z = extents.box.center.z;

	  	  if(1.0001 * extents.box.volume < orient_box.volume)
		  {
			  fprintf(stderr,
				"Warning: oriented box is bigger than non oriented one.! %.8f %.8f\n",
				extents.box.volume, orient_box.volume);
		  }
	  }
  }

  
  // TUBE & CYLINDER
  extents.compute_cylinder(vcount, vlist);

								// X,Y, or Z alligned
  extents.cylinder.cyl_tr.m = extents.cylinder.cyl_tr.m * extents.box.tr.m;
  Vector cyl_center = .5f * (extents.cylinder.p1 + extents.cylinder.p2);
  extents.cylinder.cyl_tr.v = extents.box.tr.m * PersistVector(cyl_center.x, cyl_center.y, cyl_center.z);
  
								// X,Y, or Z alligned
  extents.cylinder.tub_tr.m = extents.cylinder.tub_tr.m * extents.box.tr.m;
  Vector tub_center = .5f * (extents.cylinder.tub_p1 + extents.cylinder.tub_p2);
  extents.cylinder.tub_tr.v = extents.box.tr.m * PersistVector(tub_center.x, tub_center.y, tub_center.z);


  Free(vlist);
  
  if((extents.convex_hull.volume > 1.001f * extents.sphere.volume ||
	  extents.convex_hull.volume > 1.001f * extents.box.volume ||
	  extents.convex_hull.volume > 1.001f * extents.cylinder.cyl_volume ||
	  extents.convex_hull.volume > 1.001f * extents.cylinder.tub_volume) &&
	 (extents.convex_hull.volume > 0.00001f)
	)
  {
	Winprint("Error: Convex Hull volume is too large!\n"
			 " convex hull=%.3f\n sphere=%.3f\n box=%.3f\n cylinder=%.3f\n tube=%.3f\n",
			 extents.convex_hull.volume, extents.sphere.volume,
			 extents.box.volume, extents.cylinder.cyl_volume,
			 extents.cylinder.tub_volume);
  }

  
#ifdef SGI
  if(extents.mass.mass > 10000.0f)
  {
    fprintf(stderr,"Warning: object has mass=%f, which is greater than 10000.0\n",
      extents.mass.mass);
    fprintf(stderr,"Consider using lower density.\n");
  }
#endif

  // used for IK by brute force
  // extents are not optimal but each child is guaranteed to be entirely inside it's parent
  if(ik_extents)
  {
	  ik_extents = 0; // so we can call calcRigidBody again

	  const float margin = .9f;
	  // find leaf extent

	  ext_type type = SPHERE;
	  float volume = extents.sphere.volume;

	  if( extents.cylinder.tub_volume > 0.0f && extents.cylinder.tub_volume < margin * volume)
	  {
		  type = TUBE;
		  volume = extents.cylinder.tub_volume;
	  }

	  if( extents.box.volume > 0.0f && extents.box.volume < margin * volume)
	  {
		  type = BOX;
		  volume = extents.box.volume;
	  }

	  if( extents.cylinder.cyl_volume > 0.0f && extents.cylinder.cyl_volume < margin * volume)
	  {
		  type = CYLINDER;
		  volume = extents.cylinder.cyl_volume;
	  }

	  if( extents.convex_hull.volume > 0.0f && extents.convex_hull.volume < margin * volume)
	  {
		  type = HULL;
		  volume = extents.convex_hull.volume;
	  }


	  Vector *tmp_vlist = NULL;
	  int tmp_vcount = 0;
	  switch(type)
	  {
	  case SPHERE:
		  extents.cylinder.tub_volume =
		  extents.box.volume =
		  extents.cylinder.cyl_volume =
		  extents.convex_hull.volume = 0.0f;
		  break;

	  case TUBE:
		  // recompute sphere based on tube	
		  GetVertsFromTube(extents, &tmp_vlist, &tmp_vcount);
		  assert(tmp_vlist);
		  
		  calcRigidBody(extents, tmp_vlist, tmp_vcount, density, name);
		  Free(tmp_vlist);
		  
		  extents.box.volume =
		  extents.cylinder.cyl_volume =
		  extents.convex_hull.volume = 0.0f;
		  break;

	  case BOX:
		  // recompute tube based on box
		  // recompute sphere based on box
		  GetVertsFromBox(extents, &tmp_vlist, &tmp_vcount);
		  assert(tmp_vlist);
		  
		  calcRigidBody(extents, tmp_vlist, tmp_vcount, density, name);
		  Free(tmp_vlist);

		  extents.cylinder.cyl_volume =
		  extents.convex_hull.volume = 0.0f;
		  break;

	  case CYLINDER:
		  // recompute box based on cylinder
		  // recompute tube based on cylinder
		  // recompute sphere based on cylinder
		  GetVertsFromCylinder(extents, &tmp_vlist, &tmp_vcount);
		  assert(tmp_vlist);
		  
		  calcRigidBody(extents, tmp_vlist, tmp_vcount, density, name);
		  Free(tmp_vlist);

		  extents.convex_hull.volume = 0.0f;
		  break;

      case HULL:
		  break;

	  default:
		  Winprint("Error: confused extents!\n");
	  }

	  ik_extents = 1;
  }


  // assign names
  if(strlen(name) > (64 - 16))
  {
	  Winprint("Error: extent name \"%s\" is too long!\n", name);
  }
  
  strcpy(extents.sphere.name, name);
  strcat(extents.sphere.name, " Sphere 1");

  strcpy(extents.box.name, name);
  strcat(extents.box.name, " Box 1");

  strcpy(extents.cylinder.cyl_name, name);
  strcat(extents.cylinder.cyl_name, " Cylinder 1");

  strcpy(extents.cylinder.tub_name, name);
  strcat(extents.cylinder.tub_name, " Tube 1");

  strcpy(extents.convex_hull.name, name);
  strcat(extents.convex_hull.name, " Convex mesh 1");

  if(verbose_level>=3)
  {
    printf("done w/ calcRigidBody\n");
  }

  TrapFpu(false);
}

void GetVertsFromBox(Extents & extents, Vector **vlist, int *vcount)
{
	*vcount = 8;

	(*vlist) = (Vector*)Malloc(*vcount * sizeof(Vector));

	(*vlist)[0] = extents.mass.center + 
				  Transform_vector(extents.box.tr, 
								   Vector(extents.box.center.x + extents.box.size.x,
										  extents.box.center.y + extents.box.size.y,
										  extents.box.center.z + extents.box.size.z) );
	(*vlist)[1] = extents.mass.center + 
				  Transform_vector(extents.box.tr, 
								   Vector(extents.box.center.x - extents.box.size.x,
										  extents.box.center.y + extents.box.size.y,
										  extents.box.center.z + extents.box.size.z) );

	(*vlist)[2] = extents.mass.center + 
				  Transform_vector(extents.box.tr, 
								   Vector(extents.box.center.x + extents.box.size.x,
										  extents.box.center.y - extents.box.size.y,
										  extents.box.center.z + extents.box.size.z) );

	(*vlist)[3] = extents.mass.center + 
				  Transform_vector(extents.box.tr, 
								   Vector(extents.box.center.x - extents.box.size.x,
										  extents.box.center.y - extents.box.size.y,
										  extents.box.center.z + extents.box.size.z) );

	(*vlist)[4] = extents.mass.center + 
				  Transform_vector(extents.box.tr, 
								   Vector(extents.box.center.x + extents.box.size.x,
										  extents.box.center.y + extents.box.size.y,
										  extents.box.center.z - extents.box.size.z) );

	(*vlist)[5] = extents.mass.center + 
				  Transform_vector(extents.box.tr, 
								   Vector(extents.box.center.x - extents.box.size.x,
										  extents.box.center.y + extents.box.size.y,
										  extents.box.center.z - extents.box.size.z) );

	(*vlist)[6] = extents.mass.center + 
				  Transform_vector(extents.box.tr, 
								   Vector(extents.box.center.x + extents.box.size.x,
										  extents.box.center.y - extents.box.size.y,
										  extents.box.center.z - extents.box.size.z) );

	(*vlist)[7] = extents.mass.center + 
				  Transform_vector(extents.box.tr, 
								   Vector(extents.box.center.x - extents.box.size.x,
										  extents.box.center.y - extents.box.size.y,
										  extents.box.center.z - extents.box.size.z) );
}

void GetVertsFromCylinder(Extents & extents, Vector **vlist, int *vcount)
{
	*vcount = 64;
	(*vlist) = (Vector*)Malloc(*vcount * sizeof(Vector));

	// sample rims
	for(int i=0; i < *vcount / 2; i++)
	{
		float angle = (2.0*M_PI * i) / (*vcount / 2);
		float c_a = cos(angle);
		float s_a = sin(angle);

		(*vlist)[2*i] = extents.mass.center +
					  Transform_vector(extents.cylinder.cyl_tr,
									   Vector(extents.cylinder.cyl_r * c_a,
											  extents.cylinder.cyl_r * s_a,
											  extents.cylinder.cyl_length / 2) );
		(*vlist)[2*i+1] = extents.mass.center +
					  Transform_vector(extents.cylinder.cyl_tr,
									   Vector(extents.cylinder.cyl_r * c_a,
											  extents.cylinder.cyl_r * s_a,
											  -extents.cylinder.cyl_length / 2) );
	}
}

void GetVertsFromTube(Extents & extents, Vector **vlist, int *vcount)
{
	*vcount = 128;
	(*vlist) = (Vector*)Malloc(*vcount * sizeof(Vector));

	// sample rims
	for(int i=0; i < *vcount / 4; i++)
	{
		float angle = (i * 2.0*M_PI) / (*vcount / 4);
		float c_a = cos(angle);
		float s_a = sin(angle);

		(*vlist)[2*i] = extents.mass.center +
					  Transform_vector(extents.cylinder.tub_tr,
									   Vector(extents.cylinder.tub_r * c_a,
											  extents.cylinder.tub_r * s_a,
											  extents.cylinder.tub_length / 2) );
		(*vlist)[2*i+1] = extents.mass.center +
					  Transform_vector(extents.cylinder.tub_tr,
									   Vector(extents.cylinder.tub_r * c_a,
											  extents.cylinder.tub_r * s_a,
											  -extents.cylinder.tub_length / 2) );
	}

	Vector *vp = (*vlist) + 2 * i;

	// sample caps using a spiral
	for(i=0; i < *vcount / 4; i++)
	{
		float angle = (i * 2.0*M_PI) / (*vcount / 4);
		float c_a = cos(angle);
		float s_a = sin(angle);

		float angle2 = (i * M_PI/2.0) / (*vcount / 4);
		float c_a2 = cos(angle2);
		float s_a2 = sin(angle2);

		vp[2*i] = extents.mass.center +
					  Transform_vector(extents.cylinder.tub_tr,
									   Vector(extents.cylinder.tub_r * c_a * c_a2,
											  extents.cylinder.tub_r * s_a * c_a2,
											  extents.cylinder.tub_length / 2 + extents.cylinder.tub_r * s_a2) );
		vp[2*i+1] = extents.mass.center +
					  Transform_vector(extents.cylinder.tub_tr,
									   Vector(extents.cylinder.tub_r * c_a * c_a2,
											  extents.cylinder.tub_r * s_a * c_a2,
											  -extents.cylinder.tub_length / 2 - extents.cylinder.tub_r * s_a2) );
	}
}

bool IsFlatHull(const CollisionMesh & ch)
{
	const float *normal = (float*)&ch.normals[ch.triangles[0].normal];
	for(int i=1; i < ch.num_triangles; i++)
	{
		if(	fabs( Dot3(normal, (float*)&ch.normals[ch.triangles[i].normal]) ) < cos(2.5 * D2R) ||
			( Dot3(normal, (float*)&ch.normals[ch.triangles[i].normal]) > 0.0 &&
			  fabs(ch.triangle_d[0] - ch.triangle_d[i]) > .0005 ) ||
			( Dot3(normal, (float*)&ch.normals[ch.triangles[i].normal]) < 0.0 &&
			  fabs(ch.triangle_d[0]) - fabs(ch.triangle_d[i]) > .0005 )
		  )
		{
			return false;
		}
	}
	return true;
}

void AdjustHull(CollisionMesh *ch, const int vcount, const Vector *vlist, float scale)
{
	Vector *tmp_vlist = (Vector*)Malloc(vcount * sizeof(Vector));

	do
	{
		float normal[3] = { ch->normals[ch->triangles[0].normal].x,
							ch->normals[ch->triangles[0].normal].y,
							ch->normals[ch->triangles[0].normal].z };
		Normalize3(normal);
		for(int i=0; i < vcount; i++)
		{
			// -.5 to .5
			float fraction = (i%2) ? .5f * scale : -.5f * scale;
			//float fraction = scale * (.5f - (float)rand() / (float)RAND_MAX);
			tmp_vlist[i].x = vlist[i].x + fraction * normal[0];
			tmp_vlist[i].y = vlist[i].y + fraction * normal[1];
			tmp_vlist[i].z = vlist[i].z + fraction * normal[2];
		}

		ComputeConvexHull(*ch, tmp_vlist, vcount); 

		scale *= 1.1f;
	}
	while( IsFlatHull(*ch) == true );

	Free(tmp_vlist);
}

void LoadPolyhedron(POLYHEDRON *p, const CollisionMesh *ch)
{
  
  p->numVerts = ch->num_vertices;
  p->verts = (double*)Malloc(p->numVerts * 3 * sizeof(double));
  for(int i=0;  i<p->numVerts; i++)
  {
	  p->verts[3*i + 0] = ch->vertices[i].p.x;
	  p->verts[3*i + 1] = ch->vertices[i].p.y;
	  p->verts[3*i + 2] = ch->vertices[i].p.z;
  }

  p->numFaces = ch->num_triangles;
  p->faces = (P_FACE*)Malloc(p->numFaces * sizeof(P_FACE));
  for(int j=0; j<p->numFaces; j++)
  {
	  double dx1, dy1, dz1, dx2, dy2, dz2, nx, ny, nz, len;

	  P_FACE *f = p->faces + j;
	  f->poly = p;
	  f->verts[0] = ch->triangles[j].v[0];
	  f->verts[1] = ch->triangles[j].v[1];
	  f->verts[2] = ch->triangles[j].v[2];

	  //compute face normal and offset w from first 3 vertices 
      dx1 = p->verts[3*f->verts[1] + 0] - p->verts[3*f->verts[0] + 0];
      dy1 = p->verts[3*f->verts[1] + 1] - p->verts[3*f->verts[0] + 1];
      dz1 = p->verts[3*f->verts[1] + 2] - p->verts[3*f->verts[0] + 2];
      dx2 = p->verts[3*f->verts[2] + 0] - p->verts[3*f->verts[1] + 0];
      dy2 = p->verts[3*f->verts[2] + 1] - p->verts[3*f->verts[1] + 1];
      dz2 = p->verts[3*f->verts[2] + 2] - p->verts[3*f->verts[1] + 2];
      nx = dy1 * dz2 - dy2 * dz1;
      ny = dz1 * dx2 - dz2 * dx1;
      nz = dx1 * dy2 - dx2 * dy1;

	  len = sqrt(nx * nx + ny * ny + nz * nz);
	  if(len > 0.0)
	  {
		  len = 1.0 / len;
	  }
      //len = 1.0 / sqrt(nx * nx + ny * ny + nz * nz);
      f->norm[0] = nx * len;
      f->norm[1] = ny * len;
      f->norm[2] = nz * len;
      f->w = - f->norm[0] * p->verts[3*f->verts[0] + 0]
             - f->norm[1] * p->verts[3*f->verts[0] + 1]
             - f->norm[2] * p->verts[3*f->verts[0] + 2];
  }

}

int OutOfSphere(float cx, float cy, float cz, float x, float y, float z, float r)
{
  //printf("box=%f sph=%f\n",(cx+x)*(cx+x) + (cy+y)*(cy+y) + (cz+z)*(cz+z),(r*r));

  if( ( (cx+x)*(cx+x) + (cy+y)*(cy+y) + (cz+z)*(cz+z) - (r*r) ) > XYZ_TOLERANCE )
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void ZeroBox(ExtentBox *box)
{
  box->center.x=
  box->center.y=
  box->center.z=0.0f;

  box->size.x=
  box->size.y=
  box->size.z=0.0f;

  box->volume = 0.0f;
}

file_node* CreateRigidBody(const Extents& extents)
{
/*
|---Rigid body [0]
*/
  if(no_physics) return NULL;

  file_node *rigid_node;
  file_node **tmp;

  rigid_node=CreateNode(Rigid_body, D);
  tmp=&(rigid_node->child);

  *tmp=CreateMass(extents.mass);
  tmp=&((*tmp)->sibling);

  *tmp=CreateExtent(extents);
  if(*tmp)
  {
	tmp=&((*tmp)->sibling);
  }

  if(verbose_level>=3){
    printf("done w/ CreateRigidBody\n");
  }

  return rigid_node;
}

file_node* CreateProperties(object *obj)
{
	SynchProperties(obj);

	file_node *node = NULL;

	if(obj->bin_prop_size > 0)
	{
		node = CreateNode("Properties", F);
		node->data_size = obj->bin_prop_size;
		node->data = obj->bin_prop;
/*
FILE *fp = fopen("c:\\export\\p.p", "wb");
fwrite(node->data, sizeof(U8), node->data_size, fp);
fflush(fp);
fclose(fp);
*/
	}

	return node;
}

file_node* CreateMass(const MassData& mass)
{
  file_node *node;
  file_node *tmp;

  node=CreateNode(Mass_properties,D);

  tmp=node->child=CreateNode(Mass,F);
  tmp->data_size=sizeof(mass.mass);   
  tmp->data=(unsigned char*)&(mass.mass);

  if(mass.mass <= 0.0f)
  {
	Winprint("Error: mass is %f! See Mike S.\n", mass.mass);
  }

  tmp->sibling=CreateNode(Center_of_mass,F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(mass.center);
  tmp->data=(unsigned char*)&(mass.center);

  tmp->sibling=CreateNode(Inertia_tensor,F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(mass.inertia);
  tmp->data=(unsigned char*)&(mass.inertia);

  return node;
}

file_node* CreateExtent(const Extents& extents)
{
  file_node *node = NULL;
  const float margin = 0.9f;
  
/*
//for testing def mesh extents in objview
{
  node = CreateNode("Extent tree", D);
  node->child = CreateSphere(extents.sphere);

  file_node *tmp;
  tmp = GetLastSib(node->child->child);
  tmp->sibling = CreateBox(extents.box);

  return node;
}
*/

  if(extents.sphere.volume > 0.0f)
  {
	  file_node *tmp;
	
	  node = CreateNode("Extent tree", D);
	  node->child = CreateSphere(extents.sphere);
	  tmp = GetLastSib(node->child->child);

	  float volume = extents.sphere.volume;

	  if((extents.cylinder.tub_volume < volume * margin) && (extents.cylinder.tub_volume > 0.0f))
	  {
		tmp->sibling = CreateTube(extents.cylinder);
		tmp = GetLastSib(tmp->sibling->child->child);
		volume = extents.cylinder.tub_volume; 
	  }

	  if((extents.box.volume < volume * margin) && (extents.box.volume > 0.0f))
	  {
		tmp->sibling = CreateBox(extents.box);
		tmp = GetLastSib(tmp->sibling->child->child);
		volume = extents.box.volume;
	  }

	  if((extents.cylinder.cyl_volume < volume * margin) && (extents.cylinder.cyl_volume > 0.0f))
	  {
		tmp->sibling = CreateCylinder(extents.cylinder);
		tmp = GetLastSib(tmp->sibling->child->child);
		volume = extents.cylinder.cyl_volume; 
	  }

	  if( (convex_hull_flag) &&
		  (extents.convex_hull.volume < volume * margin) &&
		  (extents.convex_hull.volume > 0.0f) &&
		  (extents.convex_hull.num_edges >= 6) &&
		  (extents.convex_hull.num_vertices >= 4) &&
		  (extents.convex_hull.num_triangles >= 4)
		)
	  {
		  tmp->sibling = CreateConvexHull(extents.convex_hull);
		  volume = extents.convex_hull.volume;
		  //tmp = GetLastSib(tmp->sibling->child->child);	
	  }
  }
  else
  {
    if(!exporting_deformable)
    {
	  Winprint("Warning: bounding sphere volume is %f!\n",
                   "Not exporting extents for this child.\n",
            extents.sphere.volume);
    }
  }

  return node;
}

file_node* CreateSphere(const ExtentSphere& sphere)
{
  file_node *node;
  file_node *tmp;

  node = CreateNode("Sphere 1", D);

  tmp = node->child = CreateNode("Name",F);
  tmp->data_size = strlen(sphere.name) + 1;
  tmp->data = (unsigned char*)(sphere.name);

  tmp->sibling = CreateNode("Transform",F);
  tmp = tmp->sibling;
  tmp->data_size = sizeof(sphere.tr);
  tmp->data = (unsigned char*)&(sphere.tr);

  tmp->sibling = CreateNode("Radius",F);
  tmp = tmp->sibling;
  tmp->data_size = sizeof(sphere.radius);
  tmp->data = (unsigned char*)&(sphere.radius);

  return node;
}

file_node* CreateLodLevels(lod_object *lod_obj, int txt_flag)
{
  file_node *node;
  file_node *tmp;
  char level_name[256]={0};

  tmp=node = CreateNode("Level0", D);
  tmp->child = CreateObject( &(lod_obj->obj_list[0]), txt_flag);
  assert(tmp->child);
  tmp->child->sibling = CreateProperties(&(lod_obj->obj_list[0]));

  for(int i=1; i<lod_obj->count; i++)
  {
    sprintf(level_name,"Level%d",i);
    tmp->sibling = CreateNode(level_name, D);
    tmp->sibling->child = CreateObject(&(lod_obj->obj_list[i]), txt_flag);
    assert(tmp->sibling->child);
	tmp->sibling->child->sibling = CreateProperties(&(lod_obj->obj_list[i]));

    tmp = tmp->sibling;
  }

  return node;
}

file_node* CreateLodObject(lod_object *lod_obj, int txt_flag)
{
  file_node *node = NULL;
  file_node **tmp = NULL;

  if(lod_obj->count < 1)
  {
    return NULL;
  }

  if(lod_obj->count == 1)
  {
    node = CreateObject(&(lod_obj->obj_list[0]), txt_flag);

	tmp = &(node->sibling);
	*tmp = CreateProperties(&(lod_obj->obj_list[0]));
	if(*tmp)
	{ 
		tmp = &((*tmp)->sibling);
	}
  }else
  if(lod_obj->count > 1)
  {
    node = CreateNode(MultiLevel,D);
    node->child = CreateSwitch(lod_obj);
    node->child->sibling = CreateLodLevels(lod_obj, txt_flag);

	tmp = &(node->sibling);
  }

  if( lod_obj->obj_list[0].type == FIXED_MESH ||
	  lod_obj->obj_list[0].type == DEF_MESH ||
	  lod_obj->obj_list[0].type == FIXED_NURB ||
	  lod_obj->obj_list[0].type == DEF_NURB ||
	  lod_obj->obj_list[0].type == FIXED_PATCH ||
	  lod_obj->obj_list[0].type == DEF_PATCH)
  {
      *tmp = CreateHardPoints(&(lod_obj->obj_list[0]));
	  if(*tmp)
	  {
		  tmp = &((*tmp)->sibling);
	  }

#if USE_DA_MESH
	*tmp = CreateMass(lod_obj->obj_list[0].extents.mass);
     tmp = &((*tmp)->sibling);

    *tmp = CreateExtent(lod_obj->obj_list[0].extents);
#else
	*tmp = CreateRigidBody(lod_obj->obj_list[0].extents);
#endif
  }

  if(verbose_level>=2)
  {
    printf("done w/ CreateLodObject\n");
  }

  return node;
}
 
file_node* CreateSwitch(lod_object *lod_obj)
{
  file_node *node;

  node=CreateNode(Switch, F);
  node->data_size=(lod_obj->count-1)*sizeof(*(lod_obj->switch_list));
  node->data=(unsigned char*)lod_obj->switch_list;

  return node;
}

void AdjustUV(float *uv_list, int count)
{
  int i;
  float tmp;

  // round off to reasonable uv precission
  // this prevents very small numbers from screwing things up
  for(i=0; i < count; i++)
  {
    // U
    tmp=fmod(uv_list[2*i], UV_STEP);
    if(tmp<.5*UV_STEP)
	{
      uv_list[2*i]-=tmp;
    }
    else
	{
      uv_list[2*i]+=(UV_STEP-tmp);
    }

    // V
    tmp=fmod(uv_list[2*i+1], UV_STEP);
    if(tmp<.5*UV_STEP)
	{
      uv_list[2*i+1]-=tmp;
    }
    else
	{
      uv_list[2*i+1]+=(UV_STEP-tmp);
    }
  }

#ifdef GAMEGEN
  float u_min, v_min;
  // move to 1st quadrant as close as possible to the origin
  u_min = uv_list[0];
  v_min = uv_list[1];
  for(i=1; i < count; i++)
  {
    if(uv_list[2*i]<u_min)
      u_min=uv_list[2*i];
    if(uv_list[2*i+1]<v_min)
      v_min=uv_list[2*i+1];
  }

  u_min = floor(u_min);
  v_min = floor(v_min);
  for(i=0; i < count; i++)
  {
    uv_list[2*i] -= u_min;
    uv_list[2*i+1] -= v_min;

    // uv_list[2*i]-=(int)(uv_list[2*i]);
    // uv_list[2*i+1]-=(int)(uv_list[2*i+1]);
    // flip
    // uv_list[2*i]=1.0-uv_list[2*i];
    // uv_list[2*i+1]=1.0-uv_list[2*i+1];
  }
#endif
}

void LoadLodObject(lod_object *lod_obj, file_node *root)
{
  lod_obj;
  root;

  Winprint("Error: LoadLodObject() is broken!\n"); return;

  /*
  file_node *node;
  char lod_name[256]={0};
  float *switch_dist;

  InitLodObject(lod_obj, NULL, NULL);

  node=GetWideNode(root, "MultiLevel");
  if(node){ // see if object has lod's
    node=GetWideNodeG(root, "Switch");
    switch_dist=(float*)(node->data);

    lod_obj->obj_list=(object*)Malloc(sizeof(object));
    sprintf(lod_name,"Level%d",lod_obj->count);
    node=GetWideNodeG(root, lod_name); 
    node=node->child; // openFLAME 3D N-mesh
    Load3DBObject( &(lod_obj->obj_list[0]), root);  // not node so we get HP's
    lod_obj->count=1;

    sprintf(lod_name,"Level%d", lod_obj->count);
    while(NULL != (node=GetWideNode(root, lod_name)))
	{
      node=node->child;
      lod_obj->switch_list=(float*)Realloc(lod_obj->switch_list, 
                           lod_obj->count*sizeof(float));
      lod_obj->switch_list[lod_obj->count-1]=*switch_dist;
      switch_dist++;

      lod_obj->obj_list=(object*)Realloc(lod_obj->obj_list, 
                        (lod_obj->count+1)*sizeof(object));
      Load3DBObject( &(lod_obj->obj_list[lod_obj->count]), node);
 
      lod_obj->count++;
      sprintf(lod_name,"Level%d",lod_obj->count);
    }
  }
  else{
    lod_obj->count=1;
    lod_obj->switch_list=NULL;
    lod_obj->obj_list=(object*)Malloc(sizeof(object));
    Load3DBObject( &(lod_obj->obj_list[0]), root);
  }
  */
}

void LoadHP(object *obj, file_node *root)
{
  file_node *hp_root;
  file_node *node;

  hp_root=GetWideNode(root, (char*)HP_ROOT_NAME);
  if(hp_root) return;

  node=GetWideNode(hp_root,(char*)HP_FIXED_NAME);
  if(node){
    obj->hp_fix_count=(node->data_size)/sizeof(PersistHPFixed);
    obj->hp_fix_list=(PersistHPFixed*)Malloc(node->data_size);
    memcpy(obj->hp_fix_list, node->data, node->data_size);
  }

  node=GetWideNode(hp_root,(char*)HP_PRISMATIC_NAME);
  if(node){
    obj->hp_pris_count=(node->data_size)/sizeof(PersistHPPrismatic);
    obj->hp_pris_list=(PersistHPPrismatic*)Malloc(node->data_size);
    memcpy(obj->hp_pris_list, node->data, node->data_size);
  }

  node=GetWideNode(hp_root,(char*)HP_REVOLUTE_NAME);
  if(node){
    obj->hp_rev_count=(node->data_size)/sizeof(PersistHPRevolute);
    obj->hp_rev_list=(PersistHPRevolute*)Malloc(node->data_size);
    memcpy(obj->hp_rev_list, node->data, node->data_size);
  }
}

void LoadRigidBody(Extents *extents, file_node *root)
{
  file_node *rbody_node;
  file_node *mass_node;
  file_node *sphere_node;
  file_node *box_node;
  file_node *node;

  rbody_node=GetWideNode(root, (char*)Rigid_body);
  if(!rbody_node) return;

  // mass
  mass_node=GetWideNodeG(rbody_node, (char*)Mass_properties);
  node=GetWideNodeG(mass_node, (char*)Mass);
  extents->mass.mass=*(float*)(node->data); 
  node=GetWideNodeG(mass_node, (char*)Center_of_mass);
  extents->mass.center=*(Vector*)(node->data);
  node=GetWideNodeG(mass_node, (char*)Inertia_tensor);
  extents->mass.inertia=*(Matrix*)(node->data);

  // bounding sphere
  sphere_node=GetWideNodeG(rbody_node, (char*)"Bounding volume");
  sphere_node=GetWideNodeG(sphere_node, (char*)"Sphere");
  node=GetWideNodeG(sphere_node, (char*)Center);
  extents->sphere.center=*(Vector*)(node->data);
  node=GetWideNodeG(sphere_node, (char*)"Radius");
  extents->sphere.radius=*(float*)(node->data);

  // bounding box (optional);
  box_node=GetWideNodeG(rbody_node, (char*)"Bounding volume");

  box_node=GetWideNode(box_node, (char*)Box);
  if(box_node){
     node=GetWideNodeG(box_node, (char*)Center); 
     extents->box.center=*(Vector*)(node->data);

     node=GetWideNodeG(box_node, (char*)half_x); 
     extents->box.size.x=*(float*)(node->data);
     node=GetWideNodeG(box_node, (char*)half_y); 
     extents->box.size.y=*(float*)(node->data);
     node=GetWideNodeG(box_node, (char*)half_z); 
     extents->box.size.z=*(float*)(node->data);
  }
}

void Load3DBObject(object *obj, file_node *root)
{
  obj;
  root;

  Winprint("Error: Load3DBObject() is out of date!\n"); return;

  /*
  InitObject(obj, NULL, NULL);
  obj->type = FIXED_MESH;

  LoadTextures(obj->tl, root);
  LoadMaterials(&(obj->ml), root);

  LoadVertices(&(obj->v), root);
  LoadNormals(&(obj->n), root);
  //LoadFaces(&(obj->f), root);
    
  LoadHP( obj, root);
  LoadRigidBody(&(obj->extents), root);
  */
}

void LoadVertices(vertices *v, file_node *root)
{
  v;
  root;

  Winprint("Error: LoadVertices() is out of date!\n"); return;

  /*
  file_node *node;
  file_node *tmp;

  node=GetWideNodeG(root, Vertices);

  tmp=GetDeepNodeG(node, Object_vertex_count);
  v->object_count=*(int*)(tmp->data);

  tmp=GetDeepNodeG(node, Object_vertex_list);
  v->object_list=(float*)Malloc(tmp->data_size);
  memcpy(v->object_list, tmp->data, tmp->data_size);

  tmp=GetDeepNodeG(node, Texture_vertex_count);
  v->texture_count=*(int*)(tmp->data);

  tmp=GetDeepNodeG(node, Texture_vertex_list);
  v->texture_list=(float*)Malloc(tmp->data_size);
  memcpy(v->texture_list, tmp->data, tmp->data_size);

  tmp=GetDeepNode(node, Vertex_normal);
  if(tmp){
    v->normal=(int*)Malloc(tmp->data_size);
    memcpy(v->normal, tmp->data, tmp->data_size);
  }

  tmp=GetDeepNode(node, Vertex_D_coefficient);
  if(tmp){
    v->D_coefficient=(float*)Malloc(tmp->data_size);
    memcpy(v->D_coefficient, tmp->data, tmp->data_size);
  }
  */
}
  
void LoadNormals(normals *n, file_node *root)
{
  file_node *node;
  file_node *tmp;

  node=GetWideNodeG(root, Normals);

  tmp=GetDeepNodeG(node, Surface_normal_count);
  n->count=*(int*)(tmp->data);

  tmp=GetDeepNodeG(node, Surface_normal_list);
  n->list=(float*)Malloc(tmp->data_size);
  memcpy(n->list, tmp->data, tmp->data_size);
}

void WriteObject(object *obj, const char *file_name, int txt_flag)
{
  file_node *root;
  file_node **tmp;

  root = CreateNode("\\",D);
  root->child=CreateExporterVersion();
  root->child->sibling = CreateObject(obj, txt_flag);

  tmp = &(root->child->sibling->sibling);

  *tmp = CreateProperties(obj);
  if(*tmp)
  {
	  tmp = &((*tmp)->sibling);
  }

  *tmp = CreateHardPoints(obj);
  if(*tmp)
  {
	  tmp = &((*tmp)->sibling);
  }

#if USE_DA_MESH
  *tmp = CreateMass(obj->extents.mass);
   tmp = &((*tmp)->sibling);

  *tmp = CreateExtent(obj->extents);
#else
  *tmp = CreateRigidBody(obj->extents);
#endif

  WriteUTF(root, file_name);
  FreeTree(root);
}

void WriteLodObject(lod_object *l_obj, int txt_flag)
{
  file_node *root;

  if( (l_obj->count > 0) && 
	  ((l_obj->obj_list[0].face_count > 0) || 
	   (l_obj->obj_list[0].nurb_count > 0) ||
	   (l_obj->obj_list[0].b_mesh.patch_cnt > 0) ||
	   (l_obj->obj_list[0].da_mesh.grp_cnt > 0) ||
	   (l_obj->obj_list[0].type == LIGHT) ||
	   (l_obj->obj_list[0].type == CAMERA)) )
  {
    root=CreateNode("\\",D);
	root->child=CreateExporterVersion();
    root->child->sibling=CreateLodObject(l_obj, txt_flag); // will create a regular object if only 1 lod level

#if USE_DA_MESH
	file_node **tmp = &(root->child->sibling->sibling);
	while(*tmp)
	{
		tmp = &((*tmp)->sibling);
	}
	object *obj = l_obj->obj_list;
	if(obj->ml->cq2Count > 0)
	{
		*tmp=CreateMtlLib(obj->ml);
		tmp=&((*tmp)->sibling);

		if(obj->tl->count > 0 || obj->atl->count > 0)
		{
		  *tmp=CreateTxtLib(obj->tl, obj->atl, NULL);
		  tmp=&((*tmp)->sibling);
		}
	}
#endif

    WriteUTF(root, l_obj->file_name);
    FreeTree(root);
  }
  else
  {
	  Winprint("Error: NOT writting empty %s\n",
		  (l_obj->file_name) ? l_obj->file_name : "object" );
  }
}

file_node* CreateConvexHull(const CollisionMesh& mesh)
{
  file_node *node=NULL;
  file_node *tmp;
  
  if(mesh.num_vertices > 0)
  {

	node = CreateNode("Children", D);

	tmp = node->child = CreateNode("Convex mesh 1", D);

	tmp->child = CreateNode("Name", F);
	tmp = tmp->child;
	tmp->data_size = strlen(mesh.name) + 1;
	tmp->data = (unsigned char*)(mesh.name);

	tmp->sibling = CreateNode("Transform", F);
	tmp = tmp->sibling;
	tmp->data_size = sizeof(mesh.tr);
	tmp->data = (unsigned char*)&(mesh.tr);

	tmp->sibling = CreateNode("Centroid", F);
    tmp=tmp->sibling;
    tmp->data_size=sizeof(mesh.centroid);
    tmp->data=(unsigned char*)&(mesh.centroid);

	tmp->sibling = CreateNode("Vertex count", F);
	tmp=tmp->sibling;
    tmp->data_size=sizeof(mesh.num_vertices);
    tmp->data=(unsigned char*)&(mesh.num_vertices);

    tmp->sibling=CreateNode("Vertex list", F);
    tmp=tmp->sibling;
    tmp->data_size=mesh.num_vertices*sizeof(*mesh.vertices);
    tmp->data=(unsigned char*)(mesh.vertices);

    tmp->sibling=CreateNode("Edge count", F);
    tmp=tmp->sibling;
    tmp->data_size=sizeof(mesh.num_edges);
    tmp->data=(unsigned char*)&(mesh.num_edges);

    tmp->sibling=CreateNode("Edge list", F);
    tmp=tmp->sibling;
    tmp->data_size=mesh.num_edges*sizeof(*mesh.edges);
    tmp->data=(unsigned char*)(mesh.edges);

    tmp->sibling=CreateNode("Face count", F);
    tmp=tmp->sibling;
    tmp->data_size=sizeof(mesh.num_triangles);
    tmp->data=(unsigned char*)&(mesh.num_triangles);

    tmp->sibling=CreateNode("Face list", F);
    tmp=tmp->sibling;
    tmp->data_size=mesh.num_triangles*sizeof(*mesh.triangles);
    tmp->data=(unsigned char*)(mesh.triangles);
    
    tmp->sibling=CreateNode("Normal count", F);
    tmp=tmp->sibling;
    tmp->data_size=sizeof(mesh.num_normals);
    tmp->data=(unsigned char*)&(mesh.num_normals);

    tmp->sibling=CreateNode("Normal list", F);
    tmp=tmp->sibling;
    tmp->data_size=mesh.num_normals*sizeof(*mesh.normals);
    tmp->data=(unsigned char*)(mesh.normals);

	tmp->sibling=CreateNode("Triangle D", F);
    tmp=tmp->sibling;
    tmp->data_size=mesh.num_triangles*sizeof(*mesh.triangle_d);
    tmp->data=(unsigned char*)(mesh.triangle_d);
   
/*

	file_node *verts;
    file_node *edges;
    file_node *tris;
    file_node *norms;

    node=CreateNode("Convex hull", D);

    verts=node->child=CreateNode("Vertices", D);
    tmp=node->child;

    tmp->child=CreateNode("Count", F);
    tmp=tmp->child;
    tmp->data_size=sizeof(mesh.num_vertices);
    tmp->data=(unsigned char*)&(mesh.num_vertices);

    tmp->sibling=CreateNode("List", F);
    tmp=tmp->sibling;
    tmp->data_size=mesh.num_vertices*sizeof(Vector);
    tmp->data=(unsigned char*)(mesh.vertices_p);

    tmp->sibling=CreateNode("Index", F);
    tmp=tmp->sibling;
    tmp->data_size=mesh.num_vertices*sizeof(int);
    tmp->data=(unsigned char*)(mesh.vertices_n);


    edges=verts->sibling=CreateNode("Edges", D);
    tmp=verts->sibling;
    
    tmp->child=CreateNode("Count", F);
    tmp=tmp->child;
    tmp->data_size=sizeof(mesh.num_edges);
    tmp->data=(unsigned char*)&(mesh.num_edges);

    tmp->sibling=CreateNode("Vertex_id", F);
    tmp=tmp->sibling;
    tmp->data_size=2*mesh.num_edges*sizeof(int);
    tmp->data=(unsigned char*)(mesh.edges_v2);

    tmp->sibling=CreateNode("Triangle_id", F);
    tmp=tmp->sibling;
    tmp->data_size=2*mesh.num_edges*sizeof(int);
    tmp->data=(unsigned char*)(mesh.edges_t2);


    tris=edges->sibling=CreateNode("Triangles", D);
    tmp=edges->sibling;

    tmp->child=CreateNode("Count", F);
    tmp=tmp->child;
    tmp->data_size=sizeof(mesh.num_triangles);
    tmp->data=(unsigned char*)&(mesh.num_triangles);

    tmp->sibling=CreateNode("Vertex_id", F);
    tmp=tmp->sibling;
    tmp->data_size=3*mesh.num_triangles*sizeof(int);
    tmp->data=(unsigned char*)(mesh.triangles_v3);
    
    tmp->sibling=CreateNode("Edge_id", F);
    tmp=tmp->sibling;
    tmp->data_size=3*mesh.num_triangles*sizeof(int);
    tmp->data=(unsigned char*)(mesh.triangles_e3);

    tmp->sibling=CreateNode("Normal_id", F);
    tmp=tmp->sibling;
    tmp->data_size=mesh.num_triangles*sizeof(int);
    tmp->data=(unsigned char*)(mesh.triangles_n);

    tmp->sibling=CreateNode("Triangle_D", F);
    tmp=tmp->sibling;
    tmp->data_size=mesh.num_triangles*sizeof(float);
    tmp->data=(unsigned char*)(mesh.triangle_d);

    norms=tris->sibling=CreateNode("Normals", D);
    tmp=tris->sibling;

    tmp->child=CreateNode("Count", F);
    tmp=tmp->child;
    tmp->data_size=sizeof(mesh.num_normals);
    tmp->data=(unsigned char*)&(mesh.num_normals);

    tmp->sibling=CreateNode("List", F);
    tmp=tmp->sibling;
    tmp->data_size=3*mesh.num_normals*sizeof(float);
    tmp->data=(unsigned char*)(mesh.normals);

    norms->sibling=CreateNode("Centroid", F);
    tmp=norms->sibling;
    tmp->data_size=sizeof(Vector);
    tmp->data=(unsigned char*)&(mesh.centroid);
*/
  }

  return node;
}

file_node* CreateCylinder(const Cylinder& cylinder)
{
  file_node *node=NULL;
  file_node *tmp;

  node = CreateNode("Children", D);

  tmp = node->child = CreateNode("Cylinder 1", D); 

  tmp->child=CreateNode("Name",F);
  tmp=tmp->child;
  tmp->data_size=strlen(cylinder.cyl_name) + 1;
  tmp->data=(unsigned char*)(cylinder.cyl_name);

  tmp->sibling=CreateNode("Transform",F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(cylinder.cyl_tr);
  tmp->data=(unsigned char*)&(cylinder.cyl_tr);

  tmp->sibling=CreateNode("Length",F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(cylinder.cyl_length);
  tmp->data=(unsigned char*)&(cylinder.cyl_length);

  tmp->sibling=CreateNode("Radius", F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(cylinder.cyl_r);
  tmp->data=(unsigned char*)&(cylinder.cyl_r);

  return node;
}

file_node* CreateTube(const Cylinder& cylinder)
{
  file_node *node=NULL;
  file_node *tmp;

  node = CreateNode("Children", D);

  tmp = node->child = CreateNode("Tube 1", D); 

  tmp->child=CreateNode("Name",F);
  tmp=tmp->child;
  tmp->data_size=strlen(cylinder.tub_name) + 1;
  tmp->data=(unsigned char*)(cylinder.tub_name);

  tmp->sibling=CreateNode("Transform",F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(cylinder.tub_tr);
  tmp->data=(unsigned char*)&(cylinder.tub_tr);

  tmp->sibling=CreateNode("Length",F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(cylinder.tub_length);
  tmp->data=(unsigned char*)&(cylinder.tub_length);

  tmp->sibling=CreateNode("Radius", F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(cylinder.tub_r);
  tmp->data=(unsigned char*)&(cylinder.tub_r);

  return node;
}

file_node* CreateBox(const ExtentBox& box)
{
  file_node *node = NULL;
  file_node *tmp;

  node = CreateNode("Children", D);

  tmp = node->child = CreateNode("Box 1",D); 

  tmp->child=CreateNode("Name",F);
  tmp=tmp->child;
  tmp->data_size=strlen(box.name) + 1;
  tmp->data=(unsigned char*)(box.name);

  tmp->sibling=CreateNode("Transform",F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(box.tr);
  tmp->data=(unsigned char*)&(box.tr);

  tmp->sibling=CreateNode(half_x,F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(box.size.x);
  tmp->data=(unsigned char*)&(box.size.x);

  tmp->sibling=CreateNode(half_y,F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(box.size.y);
  tmp->data=(unsigned char*)&(box.size.y);

  tmp->sibling=CreateNode(half_z,F);
  tmp=tmp->sibling;
  tmp->data_size=sizeof(box.size.z);
  tmp->data=(unsigned char*)&(box.size.z);

  return node;
}

/* cross product */
void Cross(const float * const V1, const float * const V2, float * const V3)
{
  V3[0] = V1[1]*V2[2] - V1[2]*V2[1];
  V3[1] = V1[2]*V2[0] - V1[0]*V2[2];
  V3[2] = V1[0]*V2[1] - V1[1]*V2[0];
}

void Matrix_to_PersistMatrix(Matrix *m, PersistMatrix *pm)
{
  pm->e00=m->d[0][0];
  pm->e01=m->d[0][1];
  pm->e02=m->d[0][2];

  pm->e10=m->d[1][0];
  pm->e11=m->d[1][1];
  pm->e12=m->d[1][2];

  pm->e20=m->d[2][0];
  pm->e21=m->d[2][1];
  pm->e22=m->d[2][2];
}

void AddFixedHP(file_node *node, const PersistHPFixed & hp)
{
	assert(node && !strcmp(node->name, HP_FIXED_NAME));

	file_node **current = &(node->child);

	while(*current)
	{
		current = &((*current)->sibling);
	}

	file_node *tmp;
	*current = CreateNode(hp.name, D);

	(*current)->child = tmp = CreateNode("Position", F);
	tmp->data_size = sizeof(hp.point);
	tmp->data = (unsigned char*)&(hp.point);

	tmp->sibling = CreateNode("Orientation", F);
	tmp = tmp->sibling;
	tmp->data_size = sizeof(hp.orientation);
	tmp->data = (unsigned char*)&(hp.orientation);
}

void AddPrisHP(file_node *node, const PersistHPPrismatic & hp)
{
	assert(node && !strcmp(node->name, HP_PRISMATIC_NAME));

	file_node **current = &(node->child);

	while(*current)
	{
		current = &((*current)->sibling);
	}

	file_node *tmp;
	*current = CreateNode(hp.spot.name, D);

	(*current)->child = tmp = CreateNode("Position", F);
	tmp->data_size = sizeof(hp.spot.point);
	tmp->data = (unsigned char*)&(hp.spot.point);

	tmp->sibling = CreateNode("Orientation", F);
	tmp = tmp->sibling;
	tmp->data_size = sizeof(hp.spot.orientation);
	tmp->data = (unsigned char*)&(hp.spot.orientation);

	tmp->sibling = CreateNode("Axis", F);
	tmp = tmp->sibling;
	tmp->data_size = sizeof(hp.axis);
	tmp->data = (unsigned char*)&(hp.axis);

	tmp->sibling = CreateNode("Min", F);
	tmp = tmp->sibling;
	tmp->data_size = sizeof(hp.min);
	tmp->data = (unsigned char*)&(hp.min);

	tmp->sibling = CreateNode("Max", F);
	tmp = tmp->sibling;
	tmp->data_size = sizeof(hp.max);
	tmp->data = (unsigned char*)&(hp.max);
}

void AddRevHP(file_node *node, const PersistHPRevolute & hp)
{
	assert(node && !strcmp(node->name, HP_REVOLUTE_NAME));

	file_node **current = &(node->child);

	while(*current)
	{
		current = &((*current)->sibling);
	}

	file_node *tmp;
	*current = CreateNode(hp.spot.name, D);

	(*current)->child = tmp = CreateNode("Position", F);
	tmp->data_size = sizeof(hp.spot.point);
	tmp->data = (unsigned char*)&(hp.spot.point);

	tmp->sibling = CreateNode("Orientation", F);
	tmp = tmp->sibling;
	tmp->data_size = sizeof(hp.spot.orientation);
	tmp->data = (unsigned char*)&(hp.spot.orientation);

	tmp->sibling = CreateNode("Axis", F);
	tmp = tmp->sibling;
	tmp->data_size = sizeof(hp.axis);
	tmp->data = (unsigned char*)&(hp.axis);

	tmp->sibling = CreateNode("Min", F);
	tmp = tmp->sibling;
	tmp->data_size = sizeof(hp.min);
	tmp->data = (unsigned char*)&(hp.min);

	tmp->sibling = CreateNode("Max", F);
	tmp = tmp->sibling;
	tmp->data_size = sizeof(hp.max);
	tmp->data = (unsigned char*)&(hp.max);
}

file_node* CreateHardPoints(object *obj)
{
// new open HP
  file_node *hp_node;
  file_node **next;

  if(obj->hp_fix_count + obj->hp_pris_count + obj->hp_rev_count > 0)
  {
    hp_node=CreateNode((char*)HP_ROOT_NAME, D); // "Hardpoints"
    next=&(hp_node->child);
    if(obj->hp_fix_count > 0)
	{
      *next=CreateNode((char*)HP_FIXED_NAME, D);
	  for(int i=0; i < obj->hp_fix_count; i++)
	  {
		  AddFixedHP(*next, obj->hp_fix_list[i]);
	  }
      next=&((*next)->sibling);  
    }

    if(obj->hp_pris_count > 0)
	{
      *next=CreateNode((char*)HP_PRISMATIC_NAME, D);
	  for(int i=0; i < obj->hp_pris_count; i++)
	  {
		  AddPrisHP(*next, obj->hp_pris_list[i]);
	  }
      next=&((*next)->sibling);
    }

    if(obj->hp_rev_count > 0)
	{
      *next=CreateNode((char*)HP_REVOLUTE_NAME, D);
	  for(int i=0; i < obj->hp_rev_count; i++)
	  {
		  AddRevHP(*next, obj->hp_rev_list[i]);
	  }
      next=&((*next)->sibling);  
    }

    return hp_node;
  }

  return NULL;

/* // old closed format
  file_node *hp_node;
  file_node **next;

  if(obj->hp_fix_count + obj->hp_pris_count + obj->hp_rev_count)
  {
    hp_node=CreateNode((char*)HP_ROOT_NAME, D); // "Hardpoints"
    next=&(hp_node->child);
    if(obj->hp_fix_count)
	{
      *next=CreateNode((char*)HP_FIXED_NAME, F);
      (*next)->data_size=obj->hp_fix_count*sizeof(PersistHPFixed);
      (*next)->data=(unsigned char*)obj->hp_fix_list;
      next=&((*next)->sibling);  
    }
    if(obj->hp_pris_count)
	{
      *next=CreateNode((char*)HP_PRISMATIC_NAME, F);
      (*next)->data_size=obj->hp_pris_count*sizeof(PersistHPPrismatic);
      (*next)->data=(unsigned char*)obj->hp_pris_list;
      next=&((*next)->sibling);  
    }
    if(obj->hp_rev_count)
	{
      *next=CreateNode((char*)HP_REVOLUTE_NAME, F);
      (*next)->data_size=obj->hp_rev_count*sizeof(PersistHPRevolute);
      (*next)->data=(unsigned char*)obj->hp_rev_list;
      next=&((*next)->sibling);  
    }

    return hp_node;
  }

  return NULL;
 */
}

int calcNormal(const float p1[3], const float p2[3], const float p3[3],
               float normal[3])
{
  // get vectors from points
  const float v1[3] = {p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]};  
  const float v2[3] = {p3[0]-p1[0], p3[1]-p1[1], p3[2]-p1[2]};  

  Cross(v1, v2, normal);

  return Normalize3(normal);
}

int calcFaceGroupNormal(const face_group *fg, const vertices *v, int f_id, float normal[3])
{
  int v_id1 = v->object_batch_list[ fg->batch_chain[3*f_id] ];
  int v_id2 = v->object_batch_list[ fg->batch_chain[3*f_id+1] ];
  int v_id3 = v->object_batch_list[ fg->batch_chain[3*f_id+2] ];
  
  return calcNormal(v->object_list + 3*v_id1,
					v->object_list + 3*v_id2,
					v->object_list + 3*v_id3,
					normal);
}

int CheckPoly(poly *p)
{
  assert(p->vertex_count == 3);

  if( GetDupVertexID(p) > 0)
  {
	  return -1;
  }

  const float angle0 = GetVertexAngle(p->vertices, 0);
  const float angle1 = GetVertexAngle(p->vertices, 1);
  const float angle2 = GetVertexAngle(p->vertices, 2);

  if(angle0 <= 0.0f || angle0 >= 180.0f*D2R ||
	 angle1 <= 0.0f || angle1 >= 180.0f*D2R ||
	 angle2 <= 0.0f || angle2 >= 180.0f*D2R ||
	 fabs(angle0 + angle1 + angle2 - 180.0f*D2R) >= .0001 )
  {
	fprintf(stderr,"internal angles DON'T add up; dropping face.\n");
	return -1;
  }

//return 1;

  if(angle1 > angle0 && angle1 > angle2)
  {
	  float tmp_v[3] = {p->vertices[0], p->vertices[1], p->vertices[2]};
	  memmove(&(p->vertices[0]), &(p->vertices[3]), 6 * sizeof(float));
	  p->vertices[6] = tmp_v[0]; p->vertices[7] = tmp_v[1]; p->vertices[8] = tmp_v[2];

	  float tmp_uv[2] = {p->uv[0], p->uv[1]};
	  memmove(&(p->uv[0]), &(p->uv[2]), 4 * sizeof(float));
	  p->uv[4] = tmp_uv[0]; p->uv[5] = tmp_uv[1];

	  float tmp_uv2[2] = {p->uv2[0], p->uv2[1]};
	  memmove(&(p->uv2[0]), &(p->uv2[2]), 4 * sizeof(float));
	  p->uv2[4] = tmp_uv2[0]; p->uv2[5] = tmp_uv2[1];

	  int tmp_c[3] = {p->color[0], p->color[1], p->color[2]};
	  memmove(&(p->color[0]), &(p->color[3]), 6 * sizeof(int));
	  p->color[6] = tmp_c[0]; p->color[7] = tmp_c[1]; p->color[8] = tmp_c[2];

	  int tmp_fl = p->flags[0];
	  p->flags[0] = p->flags[1];
	  p->flags[1] = p->flags[2];
	  p->flags[2] = tmp_fl;

	  float tmp_fr[3] = {p->frozen[0], p->frozen[1], p->frozen[2]};
	  memmove(&(p->frozen[0]), &(p->frozen[3]), 6 * sizeof(float));
	  p->frozen[6] = tmp_fr[0]; p->frozen[7] = tmp_fr[1]; p->frozen[8] = tmp_fr[2];

	  int tmp_ap = p->api_uv_id[0];
	  p->api_uv_id[0] = p->api_uv_id[1];
	  p->api_uv_id[1] = p->api_uv_id[2];
	  p->api_uv_id[2] = tmp_ap;

	  int tmp_ap2 = p->api_uv_id2[0];
	  p->api_uv_id2[0] = p->api_uv_id2[1];
	  p->api_uv_id2[1] = p->api_uv_id2[2];
	  p->api_uv_id2[2] = tmp_ap2;

	  tmp_ap = p->api_xyz_id[0];
	  p->api_xyz_id[0] = p->api_xyz_id[1];
	  p->api_xyz_id[1] = p->api_xyz_id[2];
	  p->api_xyz_id[2] = tmp_ap;
  }
  else 
  if(angle2 > angle0 && angle2 > angle1)
  {
	  float tmp_v[3] = {p->vertices[6], p->vertices[7], p->vertices[8]};
	  memmove(&(p->vertices[3]), &(p->vertices[0]), 6 * sizeof(float));
	  p->vertices[0] = tmp_v[0]; p->vertices[1] = tmp_v[1]; p->vertices[2] = tmp_v[2];

	  float tmp_uv[2] = {p->uv[4], p->uv[5]};
	  memmove(&(p->uv[2]), &(p->uv[0]), 4 * sizeof(float));
	  p->uv[0] = tmp_uv[0]; p->uv[1] = tmp_uv[1];

	  float tmp_uv2[2] = {p->uv2[4], p->uv2[5]};
	  memmove(&(p->uv2[2]), &(p->uv2[0]), 4 * sizeof(float));
	  p->uv2[0] = tmp_uv2[0]; p->uv2[1] = tmp_uv2[1];

	  int tmp_c[3] = {p->color[6], p->color[7], p->color[8]};
	  memmove(&(p->color[3]), &(p->color[0]), 6 * sizeof(int));
	  p->color[0] = tmp_c[0]; p->color[1] = tmp_c[1]; p->color[2] = tmp_c[2];

	  int tmp_fl = p->flags[2];
	  p->flags[2] = p->flags[1];
	  p->flags[1] = p->flags[0];
	  p->flags[0] = tmp_fl;

	  float tmp_fr[3] = {p->frozen[6], p->frozen[7], p->frozen[8]};
	  memmove(&(p->frozen[3]), &(p->frozen[0]), 6 * sizeof(float));
	  p->frozen[0] = tmp_fr[0]; p->frozen[1] = tmp_fr[1]; p->frozen[2] = tmp_fr[2];

	  int tmp_ap = p->api_uv_id[2];
	  p->api_uv_id[2] = p->api_uv_id[1];
	  p->api_uv_id[1] = p->api_uv_id[0];
	  p->api_uv_id[0] = tmp_ap;

	  int tmp_ap2 = p->api_uv_id2[2];
	  p->api_uv_id2[2] = p->api_uv_id2[1];
	  p->api_uv_id2[1] = p->api_uv_id2[0];
	  p->api_uv_id2[0] = tmp_ap2;

	  tmp_ap = p->api_xyz_id[2];
	  p->api_xyz_id[2] = p->api_xyz_id[1];
	  p->api_xyz_id[1] = p->api_xyz_id[0];
	  p->api_xyz_id[0] = tmp_ap;
  }

  return 1; // ok
}

int GetDupVertexID(const poly *p)
{
  int j, i;
 
  for(j=0; j < p->vertex_count - 1; j++)
  {
    for(i=j+1; i<p->vertex_count; i++)
    {
      if(SameXYZ(&(p->vertices[3*j]), &(p->vertices[3*i]), XYZ_TOLERANCE))
      {
        return i;
      }
    }
  }

  return -1;
}

void ScaleObjectDeformable(object *obj, const float scale_factor)
{
  if(scale_factor == 1.0f)
    return;

  if(obj->type == DEF_MESH)
  {
	vertices *v = &(obj->v);

	int index = 0;
	for(int i = 0; i < v->object_count; i++)
	{
		assert(v->first_vertex[i] == index);

		for(int j = 0; j < v->vertex_bone_count[i]; j++)
		{
			v->bone_vertex_list[3*index] *= scale_factor;
			v->bone_vertex_list[3*index+1] *= scale_factor;
			v->bone_vertex_list[3*index+2] *= scale_factor;

			index++;
		}
	}
  }else
  if(obj->type == DEF_PATCH)
  {
	  Bezier_mesh & b_mesh = obj->b_mesh;

	  if( b_mesh.bone_vertex_list_ver )
	  {
		  int index = 0;
		  for(int i = 0; i < b_mesh.vertex_cnt; i++)
		  {
			  for(int j = 0; j < b_mesh.vertex_bone_count_ver[i]; j++)
			  {
				  b_mesh.bone_vertex_list_ver[index] *= scale_factor;
				  index++;
			  }
		  }
	  }

	  if( b_mesh.bone_vertex_list_vec )
	  {
		  int index = 0;
		  for(int i = 0; i < b_mesh.vector_cnt; i++)
		  {
			  for(int j = 0; j < b_mesh.vertex_bone_count_vec[i]; j++)
			  {
				  b_mesh.bone_vertex_list_vec[index] *= scale_factor;
				  index++;
			  }
		  }
	  }
  }
  else
  {
	Winprint("Error: ScaleObjectDeformable() not implemented for type %d\n", obj->type);
  }
}

void ScaleObject(object *obj, const float scale_factor)
{
  if(scale_factor == 1.0f)
    return;

  if(obj->type == FIXED_MESH || obj->type == DEF_MESH)
  {
	  for(int i=0; i<(obj->v.object_count)*3; i++)
	  {
		obj->v.object_list[i]*=scale_factor;
	  }

	  if(obj->face_group_list)
	  {
		for(int g_id=0; g_id<obj->face_group_count; g_id++)
		{
		  const face_group *fg = obj->face_group_list + g_id;

		  for(i=0; i<fg->count; i++)
		  {
			fg->D_coefficient[i] *= scale_factor;
		  }
		}
	  }

	  for(i=0; i<obj->v.uv_bone_count; i++)
	  {
		  obj->v.x_to_u_scale[i] /= scale_factor;
		  obj->v.y_to_v_scale[i] /= scale_factor;
	  }

	  ScaleObjectHP(obj, scale_factor);

	  // causes core dump on SGI
	  //calcRigidBody(obj, DENSITY, obj->extents.mass.mass 
						 //* scale_factor * scale_factor * scale_factor);
  }else
  if(obj->type == FIXED_PATCH || obj->type == DEF_PATCH)
  {
	  Bezier_mesh & b_mesh = obj->b_mesh;

	  if( b_mesh.vertices )
	  {
		  for(int i = 0; i < b_mesh.vertex_cnt; i++)
		  {
			  b_mesh.vertices[i] *= scale_factor;
		  }
	  }

	  if( b_mesh.vectors )
	  {
		  for(int i = 0; i < b_mesh.vector_cnt; i++)
		  {
			  b_mesh.vectors[i] *= scale_factor;
		  }
	  }

	  if( b_mesh.aux )
	  {
		  for(int i = 0; i < b_mesh.aux_cnt; i++)
		  {
			  b_mesh.aux[i] *= scale_factor;
		  }
	  }

	  ScaleObjectHP(obj, scale_factor);
  }
  else
  {
	Winprint("Error: ScaleObject() not implemented for type %d\n", obj->type);
  }

  if(verbose_level>=3){
    printf("done w/ ScaleObject\n");
  }
}

void ScaleObjectHP(object *obj, const float scale_factor)
{
  if(scale_factor == 1.0f) return;

  for(int i=0; i<obj->hp_fix_count; i++)
  {
    obj->hp_fix_list[i].point.x*=scale_factor;
    obj->hp_fix_list[i].point.y*=scale_factor;
    obj->hp_fix_list[i].point.z*=scale_factor;
  }
  for(i=0; i<obj->hp_pris_count; i++)
  {
    obj->hp_pris_list[i].spot.point.x*=scale_factor;
    obj->hp_pris_list[i].spot.point.y*=scale_factor;
    obj->hp_pris_list[i].spot.point.z*=scale_factor;

    obj->hp_pris_list[i].min*=scale_factor;
    obj->hp_pris_list[i].max*=scale_factor;
  }
  for(i=0; i<obj->hp_rev_count; i++)
  {
    obj->hp_rev_list[i].spot.point.x*=scale_factor;
    obj->hp_rev_list[i].spot.point.y*=scale_factor;
    obj->hp_rev_list[i].spot.point.z*=scale_factor;
  }
}

void ScaleLodObject(lod_object *lod_obj, float scale_factor, float density)
{
  if(scale_factor == 1.0f) return;

  for(int i=0; i<lod_obj->count-1; i++)
  {
    lod_obj->switch_list[i]*=scale_factor;
    ScaleObject(&(lod_obj->obj_list[i]), scale_factor);
    calcRigidBody(&(lod_obj->obj_list[i]), density, -1, NULL, true);
  }

  ScaleObject(&(lod_obj->obj_list[lod_obj->count-1]), scale_factor);
  calcRigidBody(&(lod_obj->obj_list[lod_obj->count-1]), density, -1, NULL, true);
  // first switch dist is assumed 0

  if(verbose_level>=3){
    printf("done w/ ScaleLodObject\n");
  }
}

void RotateObjectRoot(object *obj, const Matrix& m)
{
  // should not happen
  if(obj->v.vertex_bone_count == NULL) return;

  int i;
  int j;
  int index;
  float x,y,z;
  
  index=0;
  for(i=0; i<obj->v.object_count; i++)
  {
    if(obj->v.bone_id_list[i] == 0)
    {
      assert(index == obj->v.first_vertex[i]);
      for(j=0; j<obj->v.vertex_bone_count[i]; j++)
      {
         // rotate vertex
         x =  obj->v.bone_vertex_list[3*(index+j)];
         y =  obj->v.bone_vertex_list[3*(index+j)+1];
         z =  obj->v.bone_vertex_list[3*(index+j)+2];

         obj->v.bone_vertex_list[3*(index+j)] =
           x*m.d[0][0] + y*m.d[0][1] + z*m.d[0][2];
         obj->v.bone_vertex_list[3*(index+j)+1] =
           x*m.d[1][0] + y*m.d[1][1] + z*m.d[1][2];
         obj->v.bone_vertex_list[3*(index+j)+2] =
           x*m.d[2][0] + y*m.d[2][1] + z*m.d[2][2];
      
         // rotate normal   
         x =  obj->v.bone_normal_list[3*(index+j)];
         y =  obj->v.bone_normal_list[3*(index+j)+1];
         z =  obj->v.bone_normal_list[3*(index+j)+2];

         obj->v.bone_normal_list[3*(index+j)] =
           x*m.d[0][0] + y*m.d[0][1] + z*m.d[0][2];
         obj->v.bone_normal_list[3*(index+j)+1] =
           x*m.d[1][0] + y*m.d[1][1] + z*m.d[1][2];
         obj->v.bone_normal_list[3*(index+j)+2] =
           x*m.d[2][0] + y*m.d[2][1] + z*m.d[2][2];
      }
    }
    index += obj->v.vertex_bone_count[i];
  }

}

Matrix MatrixTranspose(const Matrix& m)
{
  Matrix mt;

  mt.d[0][0] = m.d[0][0];
  mt.d[0][1] = m.d[1][0];
  mt.d[0][2] = m.d[2][0];

  mt.d[1][0] = m.d[0][1];
  mt.d[1][1] = m.d[1][1];
  mt.d[1][2] = m.d[2][1];

  mt.d[2][0] = m.d[0][2];
  mt.d[2][1] = m.d[1][2];
  mt.d[2][2] = m.d[2][2];

  return mt;
}

void InsertHPFixed(object *obj, const PersistHPFixed& hp_fixed)
{
  obj->hp_fix_count++;
  obj->hp_fix_list=(PersistHPFixed*)Realloc(obj->hp_fix_list,
                    obj->hp_fix_count*sizeof(PersistHPFixed));
  obj->hp_fix_list[obj->hp_fix_count-1]=hp_fixed;
}

void InsertHPPrismatic(object *obj, const PersistHPPrismatic& hp_prismatic)
{
  obj->hp_pris_count++;
  obj->hp_pris_list=(PersistHPPrismatic*)Realloc(obj->hp_pris_list,
                    obj->hp_pris_count*sizeof(PersistHPPrismatic));
  obj->hp_pris_list[obj->hp_pris_count-1]=hp_prismatic;
}

void InsertHPRevolute(object *obj, const PersistHPRevolute& hp_revolute)
{
  obj->hp_rev_count++;
  obj->hp_rev_list=(PersistHPRevolute*)Realloc(obj->hp_rev_list,
                   obj->hp_rev_count*sizeof(PersistHPRevolute));
  obj->hp_rev_list[obj->hp_rev_count-1]=hp_revolute;
}

void InitHPFixed(PersistHPFixed *fixed)
{
  fixed->name[0]=0;
  fixed->point.x = fixed->point.y = fixed->point.z = 0.0f;
  fixed->orientation.identity();
}

int IsRightHanded(const PersistMatrix *pm)
{
  float v1[3], v2[3], v3[3];
  float v4[3];

  v1[0] = pm->e00;
  v1[1] = pm->e10;
  v1[2] = pm->e20;

  v2[0] = pm->e01;
  v2[1] = pm->e11;
  v2[2] = pm->e21;

  v3[0] = pm->e02;
  v3[1] = pm->e12;
  v3[2] = pm->e22;

  Cross(v1, v2, v4);

  float angle = Acos( Dot3(v3, v4) / sqrt(Magnitude3_sq(v3) * Magnitude3_sq(v4)) );

/*
  printf("v1 %.3f %.3f %.3f\n",v1[0], v1[1], v1[2]);
  printf("v2 %.3f %.3f %.3f\n",v2[0], v2[1], v2[2]);
  printf("v3 %.3f %.3f %.3f\n",v3[0], v3[1], v3[2]);
  printf("v4 %.3f %.3f %.3f\n",v4[0], v4[1], v4[2]);
  printf("%f\n", R2D*angle);
*/

  if(angle < D2R * 1.0f)
  {
    return 1;
  }  

  return 0;
}

void SynchProperties(object *obj)
{
	if(obj->prop_hdr.propCount > 0)
	{
		assert(obj->bin_prop == NULL && obj->bin_prop_size == 0);

		PersistPropHeader & prop_hdr = obj->prop_hdr;
		PersistProperty2 *prop_list = obj->prop_list;
		
		// compute offsets
		prop_hdr.propOffset = sizeof(PersistPropHeader);
		prop_hdr.dataOffset = prop_hdr.propOffset + prop_hdr.propCount * sizeof(PersistProperty);

		unsigned int curDataOffset = 0;
		for(unsigned int i = 0; i < prop_hdr.propCount; i++)
		{
			assert(prop_list[i].name && prop_list[i].nameLen == strlen(prop_list[i].name) + 1);
			assert(prop_list[i].dataLen >= prop_list[i].nameLen);
			assert(prop_list[i].data || prop_list[i].nameLen == prop_list[i].dataLen);
			assert(prop_list[i].propType > PT_UNKNOWN && prop_list[i].propType < PT_VOID);
			
			prop_list[i].dataOffset = curDataOffset;
			curDataOffset += prop_list[i].dataLen;
		}

		// write to buffer
		obj->bin_prop_size = sizeof(PersistPropHeader);
		obj->bin_prop_size += prop_hdr.propCount * sizeof(PersistProperty);

		for(i = 0; i < prop_hdr.propCount; i++)
		{
			obj->bin_prop_size += prop_list[i].dataLen;// + prop_list[i].nameLen;
		}

		obj->bin_prop = (unsigned char*)Malloc(obj->bin_prop_size);
		unsigned char *pt = obj->bin_prop;

		memcpy(pt, &(prop_hdr), sizeof(PersistPropHeader));
		pt += sizeof(PersistPropHeader);

		for(i = 0; i < prop_hdr.propCount; i++)
		{
			memcpy(pt, &(prop_list[i]), sizeof(PersistProperty));
			pt += sizeof(PersistProperty);
		}

		for(i = 0; i < prop_hdr.propCount; i++)
		{
			memcpy(pt, prop_list[i].name, strlen( prop_list[i].name ));
			pt += strlen(prop_list[i].name);
			
			*pt = 0;
			pt++;

			memcpy(pt, prop_list[i].data, prop_list[i].dataLen - prop_list[i].nameLen);
			pt += prop_list[i].dataLen - prop_list[i].nameLen;
		}

		assert(pt == obj->bin_prop + obj->bin_prop_size);
	}
}

int Bezier_mesh::AppendAux(const Vector aux9[9])
{
	int id = aux_cnt;
	
	aux_cnt += 9;
	aux = (Vector*)Realloc(aux, aux_cnt * sizeof(Vector));
	memcpy(aux + id, aux9, 9 * sizeof(Vector));

	return id;
}

void Bezier_mesh::InsertPatch( Bezier_patch & patch, const int mtl_id, const int api_id,
							   const Vector aux9[9])
{
	if(patch.type == 3)
	{
		patch.aux_index = AppendAux(aux9);
	}
	else
	{
		patch.aux_index = -1;
	}

	for(int i = 0; i < group_cnt; i++)
	{
		Bezier_patch_group *pg = groups + i;

		if(pg->mtl_id == mtl_id)
		{
			pg->InsertPatch( patch, api_id );
			patch_cnt++;
			return;
		}
	}

	// new group
	group_cnt++;
	groups = (Bezier_patch_group*)Realloc(groups, group_cnt * sizeof(Bezier_patch_group));
	groups[group_cnt - 1].Init();
	groups[group_cnt - 1].mtl_id = mtl_id;
	groups[group_cnt - 1].InsertPatch( patch, api_id );
	patch_cnt++;
}

void Bezier_mesh::CalcEdges(void)
{
	edge_cnt = 0;
	Free(edges);

	for(int gid = 0; gid < group_cnt; gid++)
	{
		Bezier_patch_group & group = groups[gid];
		for(int pid = 0; pid < group.patch_cnt; pid++)
		{
			Bezier_patch & patch = group.patch_list[pid];

			patch.edges[3] = -1; // for tri patches
			int e_count = patch.type;
			for(int i = 0; i < e_count; i++)
			{
				patch.edges[i] = InsertEdge( patch.v[i], patch.v[(i+1) % e_count], gid, pid );
			}
		}
	}
}

int Bezier_mesh::GetEdge(const int v1, const int v2)
{
	for(int eid = 0; eid < edge_cnt; eid++)
	{
		const Bezier_edge & edge = edges[eid];
		if( (edge.v1 == v1 && edge.v2 == v2) ||
			(edge.v1 == v2 && edge.v2 == v1) )
		{
			return eid;
		}
	}

	return -1;
}

int Bezier_mesh::InsertEdge(const int v1, const int v2, const int gid, const int pid )
{
	int eid = GetEdge( v1, v2 );

	if( eid >= 0 )
	{
		Bezier_edge & edge = edges[eid];
		assert(edge.patch1_grp >= 0 && edge.patch1_grp < group_cnt);
		assert(edge.patch1 >= 0 && edge.patch1 < patch_cnt);

		// if this happens edge has more than 2 patches
		if( edge.patch2_grp != -1 || edge.patch2 != -1 )
		{
			Winprint("Error: got an edge w/ more than 2 patches!\n");
		}
		else
		{
			edge.patch2_grp = gid;
			edge.patch2 = pid;
		}
	}
	else
	{
		eid = edge_cnt;
		edge_cnt++;

		edges = (Bezier_edge*)Realloc(edges, edge_cnt * sizeof(*edges));
		edges[eid].v1 = v1;
		edges[eid].v2 = v2;
		edges[eid].patch1_grp = gid;
		edges[eid].patch1 = pid;
		edges[eid].patch2_grp = -1;
		edges[eid].patch2 = -1;
	}

	return eid;
}

void Bezier_patch_group::InsertPatch( const Bezier_patch & patch, const int api_id )
{
	patch_cnt++;
	patch_list = (Bezier_patch*)Realloc(patch_list, patch_cnt * sizeof(Bezier_patch));
	patch_list[patch_cnt - 1] = patch;
	api_id_list = (int*)Realloc(api_id_list, patch_cnt * sizeof(int));
	api_id_list[patch_cnt - 1] = api_id;
}

/*
#ifndef SGI
#include "timer.h"

extern long large_size;
extern long small_size;

extern double large_time;
extern double small_time;

void CopyDAMesh( object *obj )
{
	RPVertex rp_face[3];
	SmallMesh & s_mesh = obj->s_mesh;

	const vertices *v = &(obj->v);
	const normals *n = &(obj->n);

	for( int fg_id = 0; fg_id < obj->face_group_count; fg_id++)
	{
		const face_group *fg = obj->face_group_list + fg_id;

		for(int f_id = 0; f_id < fg->count; f_id++)
		{
			for(int vi = 0; vi < 3; vi++)
			{
				int bid = fg->batch_chain[3*f_id+vi];

				int vid = v->object_batch_list[bid];
				rp_face[vi].pos.x = v->object_list[3*vid];
				rp_face[vi].pos.y = v->object_list[3*vid+1];
				rp_face[vi].pos.z = v->object_list[3*vid+2];

				int uvid = v->texture_batch_list[bid];
				rp_face[vi].uv.u = v->texture_list[2*uvid];
				rp_face[vi].uv.v = v->texture_list[2*uvid+1];

				int nid;
				if( fg->property[f_id] & SMOOTH_SHADED )
				{
					nid = v->normal[vid];
				}
				else
				{
					nid = fg->normal[f_id];
				}

				rp_face[vi].normal.x = n->list[3*nid];
				rp_face[vi].normal.y = n->list[3*nid+1];
				rp_face[vi].normal.z = n->list[3*nid+2];
			}
			
			s_mesh.InsertFace( rp_face );
		}
	}

	PTimer tt; // also time memcpy vs manual int and vertex copy
	
	const long sm_size = s_mesh.GetSize();
	
	LargeMesh lm1;
	lm1.CopyFromSmall( s_mesh ); // malloc mem
	tt.reset();
	lm1.CopyFromSmall( s_mesh );
	small_time += tt.get_elapsed_time();

	const long lg_size = lm1.GetSize();
	assert( lg_size >= sm_size );

	LargeMesh lm2;
	lm2.CopyFromLarge( lm1 ); // malloc mem
	tt.reset();
	lm2.CopyFromLarge( lm1 );
	large_time += tt.get_elapsed_time();

	lm1.Release();
	lm2.Release();

	large_size += lg_size;
	small_size += sm_size;
}
#endif
*/

#ifndef SGI
#pragma warning( default : 4244 ) 
// conversion from 'double' to 'float', possible loss of data
#endif
