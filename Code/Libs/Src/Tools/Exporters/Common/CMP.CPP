#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifndef SGI
#pragma warning( 3 : 4100 ) // unreferenced formal parameter
#pragma warning( 3 : 4189 ) // local variable is initialized but not referenced
#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( error : 4700 )
#pragma warning( 3 : 4706 ) // assignment within conditional expression
#endif

#include "sgi_utf.h"
#include "cmp.h"
#include "persistcompound.h"
#include "matrix.h"

#ifndef SGI
#pragma warning( disable : 4018 ) // signed/unsigned mismatch
#pragma warning( disable : 4514 ) // unreferenced inline function has been removed
#endif

Transform get_general_inverse(SINGLE & w, const Transform & t)
{
	Transform dst(false);

	SINGLE determinant = t.d[0][0] * (t.d[1][1] * t.d[2][2] - t.d[2][1] * t.d[1][2]) -
						 t.d[0][1] * (t.d[1][0] * t.d[2][2] - t.d[2][0] * t.d[1][2]) +
						 t.d[0][2] * (t.d[1][0] * t.d[2][1] - t.d[2][0] * t.d[1][1]);

	if (fabs(determinant) > 1e-5f)
	{
		SINGLE dt = 1.0f / determinant;

		dst.d[0][0] =  (t.d[1][1] * t.d[2][2] - t.d[2][1] * t.d[1][2]) * dt;
		dst.d[0][1] = -(t.d[0][1] * t.d[2][2] - t.d[2][1] * t.d[0][2]) * dt;
		dst.d[0][2] =  (t.d[0][1] * t.d[1][2] - t.d[1][1] * t.d[0][2]) * dt;

		dst.d[1][0] = -(t.d[1][0] * t.d[2][2] - t.d[2][0] * t.d[1][2]) * dt;
		dst.d[1][1] =  (t.d[0][0] * t.d[2][2] - t.d[2][0] * t.d[0][2]) * dt;
		dst.d[1][2] = -(t.d[0][0] * t.d[1][2] - t.d[1][0] * t.d[0][2]) * dt;

		dst.d[2][0] =  (t.d[1][0] * t.d[2][1] - t.d[2][0] * t.d[1][1]) * dt;
		dst.d[2][1] = -(t.d[0][0] * t.d[2][1] - t.d[2][0] * t.d[0][1]) * dt;
		dst.d[2][2] =  (t.d[0][0] * t.d[1][1] - t.d[1][0] * t.d[0][1]) * dt;

		dst.translation.x = -( t.d[1][0] * t.d[2][1] * t.translation.z +
							   t.d[2][0] * t.d[1][2] * t.translation.y +
							   t.d[1][1] * t.d[2][2] * t.translation.x -
							   t.d[1][0] * t.d[2][2] * t.translation.y -
							   t.d[2][0] * t.d[1][1] * t.translation.z -
							   t.d[2][1] * t.d[1][2] * t.translation.x ) * dt;

		dst.translation.y =  ( t.d[0][0] * t.d[2][1] * t.translation.z +
							   t.d[2][0] * t.d[0][2] * t.translation.y +
							   t.d[0][1] * t.d[2][2] * t.translation.x -
							   t.d[0][0] * t.d[2][2] * t.translation.y -
							   t.d[2][0] * t.d[0][1] * t.translation.z -
							   t.d[2][1] * t.d[0][2] * t.translation.x ) * dt;

		dst.translation.z = -( t.d[0][0] * t.d[1][1] * t.translation.z +
							   t.d[1][0] * t.d[0][2] * t.translation.y +
							   t.d[0][1] * t.d[1][2] * t.translation.x -
							   t.d[0][0] * t.d[1][2] * t.translation.y -
							   t.d[1][0] * t.d[0][1] * t.translation.z -
							   t.d[1][1] * t.d[0][2] * t.translation.x ) * dt;

		w = ( t.d[0][0] * t.d[1][1] * t.d[2][2] +
			  t.d[1][0] * t.d[2][1] * t.d[0][2] +
			  t.d[2][0] * t.d[0][1] * t.d[1][2] -
			  t.d[2][0] * t.d[1][1] * t.d[0][2] -
			  t.d[0][0] * t.d[2][1] * t.d[1][2] -
			  t.d[1][0] * t.d[0][1] * t.d[2][2] ) * dt;
	}
	else
	{
		dst.zero(); 
	}

	return dst;
}

void Matrix_mul(Matrix & dst, const Matrix & m1, const Matrix & m2)
{
	for (int i = 0; i < 3; i++)
	{
		dst.d[i][0] = m1.d[i][0] * m2.d[0][0] + m1.d[i][1] * m2.d[1][0] + m1.d[i][2] * m2.d[2][0];
		dst.d[i][1] = m1.d[i][0] * m2.d[0][1] + m1.d[i][1] * m2.d[1][1] + m1.d[i][2] * m2.d[2][1];
		dst.d[i][2] = m1.d[i][0] * m2.d[0][2] + m1.d[i][1] * m2.d[1][2] + m1.d[i][2] * m2.d[2][2];
	}
}

void Matrix_mul_Vector(Vector & dst, const Matrix & m, const Vector & v)
{
	dst.x = m.d[0][0] * v.x + m.d[0][1] * v.y + m.d[0][2] * v.z;
	dst.y = m.d[1][0] * v.x + m.d[1][1] * v.y + m.d[1][2] * v.z;
	dst.z = m.d[2][0] * v.x + m.d[2][1] * v.y + m.d[2][2] * v.z;
}

void Transform_mul(Transform & dst, const Transform & m1, const Transform & m2)
{
	dst.d[0][0] = m1.d[0][0] * m2.d[0][0] +
						m1.d[0][1] * m2.d[1][0] +
						m1.d[0][2] * m2.d[2][0];

	dst.d[0][1] = m1.d[0][0] * m2.d[0][1] +
						m1.d[0][1] * m2.d[1][1] +
						m1.d[0][2] * m2.d[2][1];

	dst.d[0][2] = m1.d[0][0] * m2.d[0][2] +
						m1.d[0][1] * m2.d[1][2] +
						m1.d[0][2] * m2.d[2][2];

	dst.translation.x = m1.d[0][0] * m2.translation.x +
						m1.d[0][1] * m2.translation.y +
						m1.d[0][2] * m2.translation.z +
						m1.translation.x;

	dst.d[1][0] = m1.d[1][0] * m2.d[0][0] +
						m1.d[1][1] * m2.d[1][0] +
						m1.d[1][2] * m2.d[2][0];

	dst.d[1][1] = m1.d[1][0] * m2.d[0][1] +
						m1.d[1][1] * m2.d[1][1] +
						m1.d[1][2] * m2.d[2][1];

	dst.d[1][2] = m1.d[1][0] * m2.d[0][2] +
						m1.d[1][1] * m2.d[1][2] +
						m1.d[1][2] * m2.d[2][2];

	dst.translation.y = m1.d[1][0] * m2.translation.x +
						m1.d[1][1] * m2.translation.y +
						m1.d[1][2] * m2.translation.z +
						m1.translation.y;

	dst.d[2][0] = m1.d[2][0] * m2.d[0][0] +
						m1.d[2][1] * m2.d[1][0] +
						m1.d[2][2] * m2.d[2][0];

	dst.d[2][1] = m1.d[2][0] * m2.d[0][1] +
						m1.d[2][1] * m2.d[1][1] +
						m1.d[2][2] * m2.d[2][1];

	dst.d[2][2] = m1.d[2][0] * m2.d[0][2] +
						m1.d[2][1] * m2.d[1][2] +
						m1.d[2][2] * m2.d[2][2];

	dst.translation.z = m1.d[2][0] * m2.translation.x +
						m1.d[2][1] * m2.translation.y +
						m1.d[2][2] * m2.translation.z +
						m1.translation.z;
}

Transform Transform::get_inverse(void) const
{
	Vector tmp;

	Matrix_mul_Vector(tmp, *(Matrix*)this, -translation);
	return Transform(Matrix::get_transpose(), tmp);

	//return Transform( Matrix::get_transpose(), -(translation * *(Matrix*)this) );
}

void TransformBoneVertices(object *obj, const CompoundObject *c_obj)
{
	assert(obj->v.vertex_bone_count != NULL);

	vertices & v = obj->v;
// TODO: these loops should be reordered
	for(int b_id = 0; b_id < c_obj->part_count; b_id++)
	{
		Transform tr( c_obj->transform_list[b_id] );
		Transform tr_n (tr);
		tr_n.translation = Vector(0, 0, 0);

		for(int v_id = 0; v_id < v.object_count; v_id++)
		{
			const int first = v.first_vertex[v_id];
			for(int id = first; id < first + v.vertex_bone_count[v_id]; id++)
			{
				if(v.bone_id_list[id] == b_id)
				{
					const Vector v1(v.object_list[3*v_id  ],
						  			v.object_list[3*v_id+1],
									v.object_list[3*v_id+2]);
					const Vector v2 ( Transform_vector(tr, v1) );
					v.bone_vertex_list[3*id  ] = v2.x;
					v.bone_vertex_list[3*id+1] = v2.y;
					v.bone_vertex_list[3*id+2] = v2.z;

					const int n_id = v.normal[v_id];
					const Vector n1(obj->n.list[3*n_id],
									obj->n.list[3*n_id+1],
									obj->n.list[3*n_id+2]);
					const Vector n2 ( Transform_vector(tr_n, n1) );
					v.bone_normal_list[3*id  ] = n2.x;
					v.bone_normal_list[3*id+1] = n2.y;
					v.bone_normal_list[3*id+2] = n2.z;
				}
			}
		}
	}
}

void ComputeBoneExtentsBezCV(const Bezier_mesh *b_mesh, CompoundObject *c_obj, const float density)
{

	Vector *vlist = (Vector*)Malloc(b_mesh->vertex_cnt * sizeof(Vector));

	for(int b_id = 0; b_id < c_obj->part_count; b_id++)
	{
		// get all vertices belonging to this bone
		int v_count = 0;
		for(int vid = 0; vid < b_mesh->vertex_cnt; vid++)
		{
			int fv = b_mesh->first_vertex_ver[vid];

			for(int i = 0; i < b_mesh->vertex_bone_count_ver[vid]; i++)
			{
				if( b_mesh->bone_id_list_ver[fv + i] == b_id)
				{
					vlist[v_count] = b_mesh->bone_vertex_list_ver[fv + i];	
					v_count++;
				}
			}
		}

		assert(v_count <= b_mesh->vertex_cnt);

		Extents & ext = c_obj->lod_object_list[b_id].obj_list[0].extents;
		if(v_count > 0) // >=3 ??
		{
			calcRigidBody(ext, vlist, v_count, density,
				c_obj->part_list[b_id].obj_name);
		}
		else
		{
			int parent_id = GetCmpParent(c_obj, b_id);
			if(parent_id >= 0)
			{
				ext.mass = c_obj->lod_object_list[parent_id].obj_list[0].extents.mass;
				ext.mass.mass *= .1f;
				ext.mass.center.x =
				ext.mass.center.y =
				ext.mass.center.z = 0.0f;

				ext.mass.inertia.d[0][0] *= .1f;
				ext.mass.inertia.d[0][1] *= .1f;
				ext.mass.inertia.d[0][2] *= .1f;

				ext.mass.inertia.d[1][0] *= .1f;
				ext.mass.inertia.d[1][1] *= .1f;
				ext.mass.inertia.d[1][2] *= .1f;

				ext.mass.inertia.d[2][0] *= .1f;
				ext.mass.inertia.d[2][1] *= .1f;
				ext.mass.inertia.d[2][2] *= .1f;
			}
			else // this is the root bone
			{
				// uses default
			}
		}
	}

	Free(vlist);
}

void ComputeBoneExtentsNurbCV(const object *obj, CompoundObject *c_obj, const float density)
{
	if(obj->type != DEF_NURB)
	{
		Winprint("Error: can't ComputeBoneExtentsCV() for object type %d.\n", obj->type);
		return;
	}

	int max_v_count = 0;

	for(int nid = 0; nid < obj->nurb_count; nid++)
	{
		const nurb & nrb = obj->nurb_list[nid];

		max_v_count += nrb.s_point_count * nrb.t_point_count;
	}

	Vector *vlist = (Vector*)Malloc(max_v_count * sizeof(Vector));

	for(int b_id = 0; b_id < c_obj->part_count; b_id++)
	{
		// get all vertices belonging to this bone
		int v_count = 0;
		for(int nid = 0; nid < obj->nurb_count; nid++)
		{
			const nurb & nrb = obj->nurb_list[nid];

			for(int s = 0; s < nrb.s_point_count; s++)
			{
				for(int t = 0; t < nrb.t_point_count; t++)
				{
					const int fv = nrb.first_vertex[s*nrb.t_point_count + t];

					for(int i = 0; i < nrb.vertex_bone_count[s*nrb.t_point_count + t]; i++)
					{
						if(nrb.bone_id_list[fv + i] == b_id)
						{
							vlist[v_count] = 
								Vector( nrb.bone_vertex_list[3*(fv + i)],
										nrb.bone_vertex_list[3*(fv + i)+1],
										nrb.bone_vertex_list[3*(fv + i)+2] );
							v_count++;
						}
					}
				}
			}
		}

		assert(v_count <= max_v_count);

		Extents & ext = c_obj->lod_object_list[b_id].obj_list[0].extents;
		if(v_count > 0) // >=3 ??
		{
			calcRigidBody(ext, vlist, v_count, density,
				c_obj->part_list[b_id].obj_name);
		}
		else
		{
			int parent_id = GetCmpParent(c_obj, b_id);
			if(parent_id >= 0)
			{
				ext.mass = c_obj->lod_object_list[parent_id].obj_list[0].extents.mass;
				ext.mass.mass *= .1f;
				ext.mass.center.x =
				ext.mass.center.y =
				ext.mass.center.z = 0.0f;

				ext.mass.inertia.d[0][0] *= .1f;
				ext.mass.inertia.d[0][1] *= .1f;
				ext.mass.inertia.d[0][2] *= .1f;

				ext.mass.inertia.d[1][0] *= .1f;
				ext.mass.inertia.d[1][1] *= .1f;
				ext.mass.inertia.d[1][2] *= .1f;

				ext.mass.inertia.d[2][0] *= .1f;
				ext.mass.inertia.d[2][1] *= .1f;
				ext.mass.inertia.d[2][2] *= .1f;
			}
			else // this is the root bone
			{
				// uses default
			}
		}
	}

	Free(vlist);
}

void ComputeBoneExtents(const object *obj, CompoundObject *c_obj, const float density)
{
	if(obj->type != DEF_MESH)
	{
		Winprint("Error: can't ComputeBoneExtents() for object type %d.\n", obj->type);
		return;
	}

	assert(obj->v.vertex_bone_count);

	const vertices *v = &(obj->v); 

	bool *used_vertex = (bool*)Malloc(v->object_count * sizeof(bool));
	Vector *vlist = (Vector*)Malloc(v->object_count * sizeof(Vector));

	for(int b_id=0; b_id < c_obj->part_count; b_id++)
	{
		//Transform tmp_tr( c_obj->transform_list[b_id] );

		memset(used_vertex, 0, v->object_count * sizeof(bool));

		for(int g_id=0; g_id < obj->face_group_count; g_id++)
		{
			const face_group *fg = obj->face_group_list + g_id;
			for(int f_id=0; f_id < fg->count; f_id++)
			{
				int bt_id1 = fg->batch_chain[3*f_id];
				int bt_id2 = fg->batch_chain[3*f_id+1];
				int bt_id3 = fg->batch_chain[3*f_id+2];

				int vid1 = v->object_batch_list[bt_id1];
				int vid2 = v->object_batch_list[bt_id2];
				int vid3 = v->object_batch_list[bt_id3];

				bool used_face = false;

				int i;
				int first;
				first = v->first_vertex[vid1];
				for(i=0; i < v->vertex_bone_count[vid1]; i++)
				{
					if(v->bone_id_list[first+i] == b_id)
					{
						//sanity check
                        /*
						Vector tmp_v1(v->object_list[3*vid1],
							v->object_list[3*vid1+1],
							v->object_list[3*vid1+2]);

						Vector tmp_v2 = Transform_vector(tmp_tr, tmp_v1);

						Vector tmp_v3(v->bone_vertex_list[3*(first+i)],
							v->bone_vertex_list[3*(first+i)+1],
							v->bone_vertex_list[3*(first+i)+2]);


						Vector tmp_v4(tmp_v2.x - tmp_v3.x,
							tmp_v2.y - tmp_v3.y,
							tmp_v2.z - tmp_v3.z);

						if(fabs(tmp_v4.x) + fabs(tmp_v4.y) + fabs(tmp_v4.z) > .00001)
						{
							printf("Error\n");
						}
                         */

						used_vertex[vid1] = true;
						// mark face if vertex weight is at least 10%
						if(v->bone_weight_list[first+i] > .1f)
						{
							used_face = true;
						}
                        break;
					}
				}

				first = v->first_vertex[vid2];
				for(i=0; i < v->vertex_bone_count[vid2]; i++)
				{
					if(v->bone_id_list[first+i] == b_id)
					{
						used_vertex[vid2] = true;
						// mark face if vertex weight is at least 10%
						if(v->bone_weight_list[first+i] > .1f)
						{
							used_face = true;
						}
                        break;
					}
				}

				first = v->first_vertex[vid3];
				for(i=0; i < v->vertex_bone_count[vid3]; i++)
				{
					if(v->bone_id_list[first+i] == b_id)
					{
						used_vertex[vid3] = true;
						// mark face if vertex weight is at least 10%
						if(v->bone_weight_list[first+i] > .1f)
						{
							used_face = true;
						}
                        break;
					}
				}

                 // we want all vertices if even one of this face belongs
				if(used_face)
				{
					used_vertex[vid1] = true;
					used_vertex[vid2] = true;
					used_vertex[vid3] = true;
				}
			}
		}

	
		//Transform tr( GetWorldTransform(c_obj, b_id) );
		Transform tr( c_obj->transform_list[b_id] );
		//tr = tr.get_inverse();
		
		// add used vertices
		int v_count = 0;
		for(int i=0; i < v->object_count; i++)
		{
			if(used_vertex[i])
			{
				vlist[v_count].x = v->object_list[3*i];
				vlist[v_count].y = v->object_list[3*i+1];
				vlist[v_count].z = v->object_list[3*i+2];

				// transform to local coord
				vlist[v_count] = Transform_vector(tr, vlist[v_count]);

				v_count++;
			}
		}

		Extents & ext = c_obj->lod_object_list[b_id].obj_list[0].extents;
		if(v_count > 0) // >=3 ??
		{
			calcRigidBody(ext, vlist, v_count, density,
				c_obj->part_list[b_id].obj_name);
		}
		else
		{
			int parent_id = GetCmpParent(c_obj, b_id);
			if(parent_id >= 0)
			{
				ext.mass = c_obj->lod_object_list[parent_id].obj_list[0].extents.mass;
				ext.mass.mass *= .1f;
				ext.mass.center.x =
				ext.mass.center.y =
				ext.mass.center.z = 0.0f;

				ext.mass.inertia.d[0][0] *= .1f;
				ext.mass.inertia.d[0][1] *= .1f;
				ext.mass.inertia.d[0][2] *= .1f;

				ext.mass.inertia.d[1][0] *= .1f;
				ext.mass.inertia.d[1][1] *= .1f;
				ext.mass.inertia.d[1][2] *= .1f;

				ext.mass.inertia.d[2][0] *= .1f;
				ext.mass.inertia.d[2][1] *= .1f;
				ext.mass.inertia.d[2][2] *= .1f;
			}
			else // this is the root bone
			{
				// uses default
			}
		}
	}

	Free(used_vertex);
	Free(vlist);
}

// move objects to their center of mass
void CenterMass(CompoundObject *c_obj, object *d_obj)
{
	if(d_obj && d_obj->type != DEF_MESH)
	{
		Winprint("Error: centering object type %d is not yet supported.\n", d_obj->type);
		return;
	}

	for(int p_id=0; p_id < c_obj->part_count; p_id++)
	{
		lod_object & l_obj = c_obj->lod_object_list[p_id];
		object & obj = l_obj.obj_list[0];

		if(obj.type != FIXED_MESH && obj.type != DEF_MESH)
		{
			Winprint("Error: centering object type %d is not yet supported.\n", obj.type);
			continue;
		}

		const Vector c_o_m (obj.extents.mass.center);

		if(c_o_m.x == 0.0f && c_o_m.y == 0.0f && c_o_m.z == 0.0f)
			continue;

		// mesh
		for(int v_id=0; v_id < obj.v.object_count; v_id++)
		{
			obj.v.object_list[3*v_id  ] -= c_o_m.x;
			obj.v.object_list[3*v_id+1] -= c_o_m.y;
			obj.v.object_list[3*v_id+2] -= c_o_m.z;

			obj.v.D_coefficient[v_id] = -Dot3(obj.v.object_list + 3*v_id,
				&(obj.n.list[ 3 * obj.v.normal[v_id] ]));
		}

		for(int g_id=0; g_id < obj.face_group_count; g_id++)
		{
			const face_group & fg = obj.face_group_list[g_id];

			for(int f_id=0; f_id < fg.count; f_id++)
			{
				int v_id1 = obj.v.object_batch_list[ fg.batch_chain[3*f_id] ];
				int v_id2 = obj.v.object_batch_list[ fg.batch_chain[3*f_id+1] ];
				int v_id3 = obj.v.object_batch_list[ fg.batch_chain[3*f_id+2] ];
				int n_id = fg.normal[f_id];
				fg.D_coefficient[f_id] = 
					( -Dot3(obj.v.object_list + 3*v_id1, obj.n.list + 3*n_id) -
					   Dot3(obj.v.object_list + 3*v_id2, obj.n.list + 3*n_id) -
					   Dot3(obj.v.object_list + 3*v_id3, obj.n.list + 3*n_id) ) * (float)(1.0 / 3.0);
			}
		}

		// deformable
		if(d_obj)
		{
			int index = 0;
			for(int v_id=0; v_id < d_obj->v.object_count; v_id++)
			{
				for(int j=0; j < d_obj->v.vertex_bone_count[v_id]; j++)
				{
					assert(d_obj->v.bone_id_list[index] < c_obj->part_count);

					if(d_obj->v.bone_id_list[index] == p_id)
					{
						d_obj->v.bone_vertex_list[3*index  ] -= c_o_m.x;
						d_obj->v.bone_vertex_list[3*index+1] -= c_o_m.y;
						d_obj->v.bone_vertex_list[3*index+2] -= c_o_m.z;
					}
					index++;
				}
			}

			// might as well adjust static mesh too
			if(p_id == 0)
			{
				for(int v_id=0; v_id < d_obj->v.object_count; v_id++)
				{
					d_obj->v.object_list[3*v_id  ] -= c_o_m.x;
					d_obj->v.object_list[3*v_id+1] -= c_o_m.y;
					d_obj->v.object_list[3*v_id+2] -= c_o_m.z;

					d_obj->v.D_coefficient[v_id] = -Dot3(d_obj->v.object_list + 3*v_id,
						&(d_obj->n.list[ 3 * d_obj->v.normal[v_id] ]));
				}

				for(int g_id=0; g_id < d_obj->face_group_count; g_id++)
				{
					const face_group & fg = d_obj->face_group_list[g_id];

					for(int f_id=0; f_id < fg.count; f_id++)
					{
						int v_id1 = d_obj->v.object_batch_list[ fg.batch_chain[3*f_id] ];
						int v_id2 = d_obj->v.object_batch_list[ fg.batch_chain[3*f_id+1] ];
						int v_id3 = d_obj->v.object_batch_list[ fg.batch_chain[3*f_id+2] ];
						int n_id = fg.normal[f_id];
						fg.D_coefficient[f_id] =
							( -Dot3(d_obj->v.object_list + 3*v_id1, d_obj->n.list + 3*n_id) -
							   Dot3(d_obj->v.object_list + 3*v_id2, d_obj->n.list + 3*n_id) -
							   Dot3(d_obj->v.object_list + 3*v_id3, d_obj->n.list + 3*n_id) ) * (float)(1.0 / 3.0);
					}
				}
			}
		}

		// LOD stuff
		int i;
		if(obj.lol.count > 0)
		{
			for(i=0; i < obj.lol.count; i++)
			{
				lod_step & los = obj.lol.step_list[i];

				los.v1 -= c_o_m;
				los.v2 -= c_o_m;
				los.v3 -= c_o_m;

				//los.v1_D += Dot3((float*)&c_o_m, &(obj.n.list[3*los.v1_nid])); // same
				los.v1_D = -Dot3((float*)&(los.v1), &(obj.n.list[3*los.v1_nid]));
				los.v2_D = -Dot3((float*)&(los.v2), &(obj.n.list[3*los.v2_nid]));
				los.v3_D = -Dot3((float*)&(los.v3), &(obj.n.list[3*los.v3_nid]));
			}

			for(i=0; i < obj.lol.face_count; i++)
			{
				obj.lol.high_D_coefficient_list[i] +=
					Dot3(&(obj.n.list[3 * obj.lol.high_normal_list[i] ]), (float*)&c_o_m);

				obj.lol.low_D_coefficient_list[i] +=
					Dot3(&(obj.n.list[3 * obj.lol.low_normal_list[i] ]), (float*)&c_o_m);
			}
		}

		// HP's
		for(i=0; i < obj.hp_fix_count; i++)
		{
			obj.hp_fix_list[i].point.x -= c_o_m.x;
			obj.hp_fix_list[i].point.y -= c_o_m.y;
			obj.hp_fix_list[i].point.z -= c_o_m.z;
		}
		for(i=0; i < obj.hp_pris_count; i++)
		{
			obj.hp_pris_list[i].spot.point.x -= c_o_m.x;
			obj.hp_pris_list[i].spot.point.y -= c_o_m.y;
			obj.hp_pris_list[i].spot.point.z -= c_o_m.z;
		}
		for(i=0; i < obj.hp_rev_count; i++)
		{
			obj.hp_rev_list[i].spot.point.x -= c_o_m.x;
			obj.hp_rev_list[i].spot.point.y -= c_o_m.y;
			obj.hp_rev_list[i].spot.point.z -= c_o_m.z;
		}


		// connections
		assert(c_obj->part_list[p_id].index == p_id);
		const char *name = c_obj->part_list[p_id].obj_name;

		for(i=0; i < c_obj->fix_count; i++)
		{
			if(!strcmp(c_obj->fix_list[i].parent, name) )
			{
				c_obj->fix_list[i].pos.x -= c_o_m.x;
				c_obj->fix_list[i].pos.y -= c_o_m.y;
				c_obj->fix_list[i].pos.z -= c_o_m.z;
			}

			if(!strcmp(c_obj->fix_list[i].child, name) )
			{
				// translate into paren't coord system
				Vector tmp_v;
				Matrix_mul_Vector(tmp_v, c_obj->fix_list[i].orient, c_o_m);
				 
				c_obj->fix_list[i].pos.x += tmp_v.x;
				c_obj->fix_list[i].pos.y += tmp_v.y;
				c_obj->fix_list[i].pos.z += tmp_v.z;
			}
		}

		for(i=0; i<c_obj->pris_count; i++)
		{
			if(!strcmp(c_obj->pris_list[i].parent, name) )
			{
				c_obj->pris_list[i].parent_point.x -= c_o_m.x;
				c_obj->pris_list[i].parent_point.y -= c_o_m.y;
				c_obj->pris_list[i].parent_point.z -= c_o_m.z;
			}

			if(!strcmp(c_obj->pris_list[i].child, name) )
			{
				c_obj->pris_list[i].child_point.x -= c_o_m.x;
				c_obj->pris_list[i].child_point.y -= c_o_m.y;
				c_obj->pris_list[i].child_point.z -= c_o_m.z;
			}
		}

		for(i=0; i<c_obj->rev_count; i++)
		{
			if(!strcmp(c_obj->rev_list[i].parent, name) )
			{
				c_obj->rev_list[i].parent_point.x -= c_o_m.x;
				c_obj->rev_list[i].parent_point.y -= c_o_m.y;
				c_obj->rev_list[i].parent_point.z -= c_o_m.z;
			}

			if(!strcmp(c_obj->rev_list[i].child, name) )
			{
				c_obj->rev_list[i].child_point.x -= c_o_m.x;
				c_obj->rev_list[i].child_point.y -= c_o_m.y;
				c_obj->rev_list[i].child_point.z -= c_o_m.z;
			}
		}

		for(i=0; i<c_obj->sphere_count; i++)
		{
			if(!strcmp(c_obj->sphere_list[i].parent, name) )
			{
				c_obj->sphere_list[i].parent_point.x -= c_o_m.x;
				c_obj->sphere_list[i].parent_point.y -= c_o_m.y;
				c_obj->sphere_list[i].parent_point.z -= c_o_m.z;
			}

			if(!strcmp(c_obj->sphere_list[i].child, name) )
			{
				c_obj->sphere_list[i].child_point.x -= c_o_m.x;
				c_obj->sphere_list[i].child_point.y -= c_o_m.y;
				c_obj->sphere_list[i].child_point.z -= c_o_m.z;
			}
		}

		for(i=0; i < c_obj->trans_count; i++)
		{
			if(!strcmp(c_obj->trans_list[i].parent, name) )
			{
				c_obj->trans_list[i].pos.x -= c_o_m.x;
				c_obj->trans_list[i].pos.y -= c_o_m.y;
				c_obj->trans_list[i].pos.z -= c_o_m.z;
			}

			if(!strcmp(c_obj->trans_list[i].child, name) )
			{
				// translate into paren't coord system
				Vector tmp_v;
				Matrix_mul_Vector(tmp_v, c_obj->trans_list[i].orient, c_o_m);
				 
				c_obj->trans_list[i].pos.x += tmp_v.x;
				c_obj->trans_list[i].pos.y += tmp_v.y;
				c_obj->trans_list[i].pos.z += tmp_v.z;
			}
		}

		for(i=0; i < c_obj->loose_count; i++)
		{
			if(!strcmp(c_obj->loose_list[i].parent, name) )
			{
				c_obj->loose_list[i].pos.x -= c_o_m.x;
				c_obj->loose_list[i].pos.y -= c_o_m.y;
				c_obj->loose_list[i].pos.z -= c_o_m.z;
			}

			if(!strcmp(c_obj->loose_list[i].child, name) )
			{
				// translate into paren't coord system
				Vector tmp_v;
				Matrix_mul_Vector(tmp_v, c_obj->loose_list[i].orient, c_o_m);
				 
				c_obj->loose_list[i].pos.x += tmp_v.x;
				c_obj->loose_list[i].pos.y += tmp_v.y;
				c_obj->loose_list[i].pos.z += tmp_v.z;
			}
		}

		obj.extents.sphere.render_center -= c_o_m;
		// reset center of mass
		obj.extents.mass.center = Vector(0.0f, 0.0f, 0.0f);
	}
}

file_node* CreateBoneExtents(const CompoundObject *c_obj)
{
	file_node *node = NULL;
	file_node **next_node = &node;

	for(int i = 0; i < c_obj->part_count; i++)
	{
		*next_node = CreateNode(c_obj->part_list[i].file_name, D);
		file_node **next = &((*next_node)->child);
		
#if USE_DA_MESH
		*next = CreateNode( "Bone to root", F );
		(*next)->data_size = sizeof(PersistTransform);
		(*next)->data = (U8*)&(c_obj->transform_list[i]);

		next = &((*next)->sibling);
#endif

		*next = CreateHardPoints(&(c_obj->lod_object_list[i].obj_list[0]));
		if( *next )
		{
			next = &((*next)->sibling);
		}

		*next = CreateRigidBody(c_obj->lod_object_list[i].obj_list[0].extents);
	
		next_node = &((*next_node)->sibling);	
	}

	return node;
}

/*
void ComputeCompoundExtents(CompoundObject *c_obj, float density, float mass, const char *name)
{
    Vector *vlist = NULL;
    int vcount = 0;

    for(int i=0; i<c_obj->part_count; i++)
    {
        const lod_object *lobj = c_obj->lod_object_list + i;
        const object *obj = &(lobj->obj_list[0]);

        if(obj->type == MESH)
        {
            PersistTransform pt = GetWorldTransform(c_obj, i);

            int old_count = vcount;
            vcount += obj->v.object_count;
            vlist = (Vector*)Realloc(vlist, vcount*sizeof(Vector));

            PersistVector *p_vlist = ((PersistVector*)vlist) + old_count;
            PersistVector *p_obj_list = (PersistVector*)(obj->v.object_list);
            for(int v_id=0; v_id < obj->v.object_count; v_id++)
            {
                p_vlist[v_id] = pt * p_obj_list[v_id];
            }
        }
    }

    if(vcount > 0)
    {
        //object *obj = &(c_obj->lod_object_list[0].obj_list[0]);
        calcRigidBody(c_obj->extents, vlist, vcount, mass, density, name, false);
    }

    Free(vlist);
}
*/

PersistTransform GetWorldTransform(const CompoundObject *c_obj, int id)
{
    const char *name = c_obj->part_list[id].obj_name;

    PersistTransform pt = GetTransform(c_obj, name);

    int p_id = GetCmpParent(c_obj, id);
    while(p_id >= 0)
    {
        const char *p_name = c_obj->part_list[p_id].obj_name;
        PersistTransform p_pt = GetTransform(c_obj, p_name);

        //pt = pt * p_pt; //bad
		pt = p_pt * pt;
        p_id = GetCmpParent(c_obj, p_id);
    }
    
    return pt;
}

int GetCmpParent(const CompoundObject *c_obj, int id)
{
    const char *name = c_obj->part_list[id].obj_name;
	assert(name);

    if( !strcmp(name, ROOT_OBJ_NAME) )
    {
        return -1;
    }

    char *p_name = NULL;

    int i;
    for(i=0; i<c_obj->fix_count && !p_name; i++)
    {
        if( !strcmp(c_obj->fix_list[i].child, name) )
        {
            p_name = c_obj->fix_list[i].parent;
        }
    }

    for(i=0; i<c_obj->pris_count && !p_name; i++)
    {
        if( !strcmp(c_obj->pris_list[i].child, name) )
        {
            p_name = c_obj->pris_list[i].parent;
        }
    }

    for(i=0; i<c_obj->rev_count && !p_name; i++)
    {
        if( !strcmp(c_obj->rev_list[i].child, name) )
        {
            p_name = c_obj->rev_list[i].parent;
        }
    }

    for(i=0; i<c_obj->sphere_count && !p_name; i++)
    {
        if( !strcmp(c_obj->sphere_list[i].child, name) )
        {
            p_name = c_obj->sphere_list[i].parent;
        }
    }

    for(i=0; i<c_obj->trans_count && !p_name; i++)
    {
        if( !strcmp(c_obj->trans_list[i].child, name) )
        {
            p_name = c_obj->trans_list[i].parent;
        }
    }

    for(i=0; i<c_obj->loose_count && !p_name; i++)
    {
        if( !strcmp(c_obj->loose_list[i].child, name) )
        {
            p_name = c_obj->loose_list[i].parent;
        }
    }
    

    // get parent index
    if(p_name)
    {
        for(int i=0; i<c_obj->part_count; i++)
        {
            if( !strcmp(p_name, c_obj->part_list[i].obj_name) )
            {
                return i;
            }
        }
    }

    return -1;
}

PersistTransform GetTransform(const CompoundObject *c_obj, const char *name)
{
	assert(name);

    PersistTransform pt;
	pt.identity();
    
    if( !strcmp(name, ROOT_OBJ_NAME) )
    {
        return pt;
    }

    int i;
    for(i=0; i<c_obj->fix_count; i++)
    {
        if( !strcmp(c_obj->fix_list[i].child, name) )
        {
            pt.m = c_obj->fix_list[i].orient;
            pt.v = c_obj->fix_list[i].pos;
            return pt;
        }
    }

    for(i=0; i<c_obj->pris_count; i++)
    {
        if( !strcmp(c_obj->pris_list[i].child, name) )
        {
            pt.m = c_obj->pris_list[i].rel_orientation;
            pt.v = c_obj->pris_list[i].parent_point - (pt.m * c_obj->pris_list[i].child_point);
            return pt;
        }
    }

    for(i=0; i<c_obj->rev_count; i++)
    {
        if( !strcmp(c_obj->rev_list[i].child, name) )
        {
            pt.m = c_obj->rev_list[i].rel_orientation;
            pt.v = c_obj->rev_list[i].parent_point - (pt.m * c_obj->rev_list[i].child_point);
            return pt;
        }
    }

    for(i=0; i<c_obj->sphere_count; i++)
    {
        if( !strcmp(c_obj->sphere_list[i].child, name) )
        {
            pt.m = c_obj->sphere_list[i].rel_orientation;
            pt.v = c_obj->sphere_list[i].parent_point - (pt.m * c_obj->sphere_list[i].child_point);
			//pt.v = PersistVector(0,0,0) - c_obj->sphere_list[i].parent_point;
            return pt;
        }
    }

    for(i=0; i<c_obj->trans_count; i++)
    {
        if( !strcmp(c_obj->trans_list[i].child, name) )
        {
            pt.m = c_obj->trans_list[i].orient;
            pt.v = c_obj->trans_list[i].pos;
            return pt;
        }
    }

    for(i=0; i<c_obj->loose_count; i++)
    {
        if( !strcmp(c_obj->loose_list[i].child, name) )
        {
            pt.m = c_obj->loose_list[i].orient;
            pt.v = c_obj->loose_list[i].pos;
            return pt;
        }
    }
    
    return pt;
}


void InitCompoundObject(CompoundObject *c_obj)
{
  c_obj->file_name[0]=0;

  c_obj->part_count=0;
  c_obj->part_list=NULL;
  c_obj->lod_object_list=NULL;
  c_obj->transform_list=NULL;

//  InitTxtLib(&(c_obj->tl));
//  InitAnimTxtLib(&(c_obj->atl), &(c_obj->tl));
  InitMtlLib(&(c_obj->ml));
  
  c_obj->fix_count=0;
  c_obj->fix_list=NULL;

  c_obj->pris_count=0;
  c_obj->pris_list=NULL;

  c_obj->rev_count=0;
  c_obj->rev_list=NULL;

  c_obj->sphere_count=0;
  c_obj->sphere_list=NULL;

  c_obj->trans_count=0;
  c_obj->trans_list=NULL;

  c_obj->loose_count=0;
  c_obj->loose_list=NULL;

  c_obj->n_channel_count=0;
  c_obj->n_channel_list=NULL;

  c_obj->script_count=0;
  c_obj->script_list=NULL;

  c_obj->root_transform.identity();
  c_obj->scale = 1.0f;

  InitExtents(&(c_obj->extents));
}

void InitNamedPart(NamedPart *part)
{
    part->obj_name[0]=0;
    part->file_name[0]=0;
    part->index=-1;
	part->export = true;
}

void CheckNameLenght(char *name)
{
  if(strlen(name)>PARTNAME_MAX-1) {
    fprintf(stderr, "Warning: truncating object name %s to ",name);
    name[PARTNAME_MAX-1]=0;
    fprintf(stderr, "%s\n",name);
  }
}

file_node* CreateCmpnd(CompoundObject *c_obj, int split_flag, int anim_flag,
            int txt_flag)
{
  file_node *cmpnd_node;
  file_node **next=NULL;
  file_node *tmp;
  char name[256]={0};

  cmpnd_node=CreateNode(COMPOUND_DIR_NAME, D); // "Cmpnd"

  if(c_obj->part_count > 0)
  {
    file_node **tmp2;
    tmp2 = &(cmpnd_node->child);
    if(exporting_deformable)
    {
      *tmp2 = CreateNode("Scale", F);
      (*tmp2)->data_size = sizeof(c_obj->scale);
      (*tmp2)->data = (unsigned char*)&(c_obj->scale);
      tmp2 = &((*tmp2)->sibling);
    }

    *tmp2 = CreateParts(c_obj);

    if(c_obj->fix_count + c_obj->pris_count + c_obj->rev_count + 
       c_obj->sphere_count + c_obj->loose_count + c_obj->trans_count > 0)
    {
      tmp=GetLastSib(cmpnd_node->child);
      next=&(tmp->sibling);

      *next=CreateNode(CONNECTION_DIR_NAME, D); // "Cons"
      next=&((*next)->child);
  
      if(c_obj->fix_count>0)
      {
        *next=CreateFix(c_obj);
        next=&((*next)->sibling);
      }
      if(c_obj->pris_count>0)
      {
        *next=CreatePris(c_obj);
        next=&((*next)->sibling);
      }
      if(c_obj->rev_count>0)
      {
        *next=CreateRev(c_obj);
        next=&((*next)->sibling);
      }
      if(c_obj->sphere_count>0)
      {
        *next=CreateSphere(c_obj);
        next=&((*next)->sibling);
      }
      if(c_obj->loose_count>0)
      {
        *next=CreateLoose(c_obj);
        next=&((*next)->sibling);
      }
      if(c_obj->trans_count>0)
      {
        *next=CreateTrans(c_obj);
        next=&((*next)->sibling);
      }
    }

    if(c_obj->extents.sphere.volume > 0.0)
    {
		Winprint("Error: should no longer be exporting Compound extents!\n");
        tmp = GetLastSib(cmpnd_node->child);
        tmp->sibling = CreateExtent(c_obj->extents);
    }

  }

  tmp=cmpnd_node;

  // animation
  if(anim_flag)
  {
    if(c_obj->n_channel_count>0)
    {
        tmp->sibling=CreateAnimation(c_obj);
        if(tmp->sibling)
          tmp=tmp->sibling;
    }
  }

  // objects
  if(!split_flag)
  {
    if(c_obj->lod_object_list)
    {

      // txm lib

	//if(txt_flag)
	//{
	//	if(c_obj->tl.count > 0)
	//	{
	//		tmp->sibling = CreateTxtLib(&(c_obj->tl), NULL, tmp->sibling); 
	//		tmp = tmp->sibling;
	//	}

/*		if(c_obj->atl.count > 0)
		{
			tmp->sibling = CreateAnimTxtLib(&(c_obj->atl)); 
			tmp = tmp->sibling;
		}*/
	//}

      // meshes, cameras, & lights
      for(int i=0; i<c_obj->part_count; i++)
	  {
        if( (c_obj->lod_object_list[i].count > 0)      && 
            ( (c_obj->lod_object_list[i].obj_list[0].face_count > 0  ) ||
			  (c_obj->lod_object_list[i].obj_list[0].nurb_count > 0) ||
			  (c_obj->lod_object_list[i].obj_list[0].b_mesh.group_cnt > 0) ||
			  (c_obj->lod_object_list[i].obj_list[0].da_mesh.grp_cnt > 0) ||
              (c_obj->lod_object_list[i].obj_list[0].type == LIGHT) ||
              (c_obj->lod_object_list[i].obj_list[0].type == CAMERA) )
           &&
           (c_obj->lod_object_list[i].export_flag))
		{
		  if(!c_obj->lod_object_list[i].file_name || strlen(c_obj->lod_object_list[i].file_name) < 1)
		  {
			  Winprint("Error: object w/o a name!\n");
			  continue;  // this will probably not help anyway
		  }
          strcpy(name, c_obj->lod_object_list[i].file_name);
          StripPath(name);
          tmp->sibling=CreateNode(name, D);
          tmp->sibling->child=CreateLodObject(&(c_obj->lod_object_list[i]), 0);
          tmp=tmp->sibling;
        }
      }
    }
  }

  if(verbose_level>=2){
    printf("done w/ CreateCmpnd\n");
  }

  return cmpnd_node;
}

file_node* CreateSkeletonName(const char *name)
{
    file_node *node;

    node = CreateNode("Skeleton", D);
    node->child = CreateNode("Name", F);
    node->child->data_size = strlen(name) + 1;
    node->child->data = (unsigned char*)name;

    return node;
}

file_node* CreateTrans(CompoundObject *c_obj)
{
  file_node *node;

  node=CreateNode(JT_TRANSLATIONAL_NAME, F); // "Trans"
  node->data_size=c_obj->trans_count*sizeof(*(c_obj->trans_list));
  node->data=(unsigned char*)c_obj->trans_list;

  return node;
}

file_node* CreateLoose(CompoundObject *c_obj)
{
  file_node *node;

  node=CreateNode(JT_LOOSE_NAME, F); // "Loose"
  node->data_size=c_obj->loose_count*sizeof(*(c_obj->loose_list));
  node->data=(unsigned char*)c_obj->loose_list;

  return node;
}

file_node* CreateSphere(CompoundObject *c_obj)
{
/*
  file_node *node;

  file_node * node=CreateNode(JT_SPHERICAL_NAME, D); // "Sphere"
  file_node ** next = &(node->child); 
  for(int i=0; i<c_obj->sphere_count; i++)
  {
	  file_node *tmp = NULL;
	  char name[64] = {0};
	  sprintf(name, "Joint %d", i);
	  
	  
	  *next = CreateNode(name, D);
	  
	  (*next)->child = CreateNode("Parent", F);
	  tmp = (*next)->child;
	  tmp->data_size = strlen(c_obj->sphere_list[i].parent) + 1;
	  tmp->data = 
*/

		  
	  
	
/*
	char parent[PARTNAME_MAX];
	char child[PARTNAME_MAX];

	PersistVector parent_point;
	PersistVector child_point;

	PersistMatrix rel_orientation;

	SINGLE min_about_i;
	SINGLE max_about_i;

	SINGLE min_about_j;
	SINGLE max_about_j;

	SINGLE min_about_k;
	SINGLE max_about_k;
*/

/*
	  next = &((*next)->sibling);


  }
  node->data_size=c_obj->sphere_count*sizeof(*(c_obj->sphere_list));
  node->data=(unsigned char*)c_obj->sphere_list;

  return node;
*/

  file_node *node;

  node=CreateNode(JT_SPHERICAL_NAME, F); // "Sphere"
  node->data_size=c_obj->sphere_count*sizeof(*(c_obj->sphere_list));
  node->data=(unsigned char*)c_obj->sphere_list;

  return node;
}

file_node* CreateRev(CompoundObject *c_obj)
{
  file_node *node;

  node=CreateNode(JT_REVOLUTE_NAME, F); // "Rev"
  node->data_size=c_obj->rev_count*sizeof(*(c_obj->rev_list));
  node->data=(unsigned char*)c_obj->rev_list;

  return node;
}

file_node* CreatePris(CompoundObject *c_obj)
{
  file_node *node;

  node=CreateNode(JT_PRISMATIC_NAME, F); // "Pris"
  node->data_size=c_obj->pris_count*sizeof(*(c_obj->pris_list));
  node->data=(unsigned char*)c_obj->pris_list;

  return node;
}

file_node* CreateFix(CompoundObject *c_obj)
{
  file_node *node;

  node=CreateNode(JT_FIXED_NAME, F); // "Fix"
  node->data_size=c_obj->fix_count*sizeof(*(c_obj->fix_list));
  node->data=(unsigned char*)c_obj->fix_list;

  return node;
}

file_node* CreateParts(CompoundObject *c_obj)
{
  file_node *node;
  file_node **next;
  file_node *tmp;
  char part_name[256]={0};
  int index;

  next=&(node);

  for(index=0; index<c_obj->part_count; index++)
  {
	  if(c_obj->part_list[index].export)
	  {
		// don't prepend "Part_" if Root
		if(!strcmp(ROOT_OBJ_NAME, c_obj->part_list[index].obj_name)){
		  strcpy(part_name,ROOT_OBJ_NAME);
		}
		else{
		  strcpy(part_name, "Part_");
		  strcat(part_name,c_obj->part_list[index].obj_name);
		}

		*next=tmp=CreateNode(part_name,D);

		tmp->child=CreateNode(OBJECT_NAME, F);
		tmp=tmp->child;
		tmp->data_size=(strlen(c_obj->part_list[index].obj_name)+1)*sizeof(char);
		tmp->data=(unsigned char*)c_obj->part_list[index].obj_name;
  
		tmp->sibling=CreateNode(FILE_NAME, F);
		tmp=tmp->sibling;
		tmp->data_size=(strlen(c_obj->part_list[index].file_name)+1)*sizeof(char);
		tmp->data=(unsigned char*)c_obj->part_list[index].file_name;

		tmp->sibling=CreateNode("Index", F);
		tmp=tmp->sibling;
		tmp->data_size=sizeof(c_obj->part_list[index].index);
		tmp->data=(unsigned char*)&(c_obj->part_list[index].index);

		next=&((*next)->sibling);
	  }
  }

  return node;
}

void WriteCmpd(CompoundObject *c_obj, int split_flag, int txt_flag)
{
  file_node *root=NULL;
                              // animated root
  if((c_obj->part_count>1) || (c_obj->n_channel_count>0))
  {
    root=CreateNode("\\",D);
    root->child=CreateExporterVersion();
    root->child->sibling=CreateCmpnd(c_obj, split_flag, 1, txt_flag); // anim_flag
    WriteUTF(root, c_obj->file_name);
    FreeTree(root);

    if((split_flag) && (c_obj->lod_object_list))
    {
      for(int i=0; i<c_obj->part_count; i++){
        if(c_obj->lod_object_list[i].export_flag){
          WriteLodObject(&(c_obj->lod_object_list[i]), txt_flag);
        }
      }
    }
  }
  else
  if(c_obj->part_count > 0)
  {
    if(c_obj->lod_object_list[0].export_flag)
    {
	  char tmp_name[256]; tmp_name[0]=0;
      strcpy(tmp_name, c_obj->lod_object_list[0].file_name);
      strcpy(c_obj->lod_object_list[0].file_name, c_obj->file_name);

	  for(int i = strlen(c_obj->lod_object_list[0].file_name) - 1; i >= 0; i--)
	  {
		  if(c_obj->lod_object_list[0].file_name[i] == '.')
		  {
			  c_obj->lod_object_list[0].file_name[i] = 0;
			  break;
		  }
		  else
		  {
			  c_obj->lod_object_list[0].file_name[i] = 0;
		  }
	  }
      
	  if(c_obj->lod_object_list[0].obj_list[0].type == FIXED_NURB ||
		 c_obj->lod_object_list[0].obj_list[0].type == DEF_NURB )
	  {
	    strcat(c_obj->lod_object_list[0].file_name, ".nrb");
	  }else
	  if(c_obj->lod_object_list[0].obj_list[0].type == FIXED_PATCH ||
		 c_obj->lod_object_list[0].obj_list[0].type == DEF_PATCH )
	  {
		  strcat(c_obj->lod_object_list[0].file_name, ".bez");
	  }
	  else
	  {
		assert(c_obj->lod_object_list[0].obj_list[0].type == FIXED_MESH ||
			   c_obj->lod_object_list[0].obj_list[0].type == DEF_MESH );
		strcat(c_obj->lod_object_list[0].file_name, ".3db");
	  }
      WriteLodObject(&(c_obj->lod_object_list[0]), txt_flag);
      strcpy(c_obj->lod_object_list[0].file_name, tmp_name);
    }
  }

  if(verbose_level>=2){
    printf("done w/ WriteCmpd\n");
  }
}


void FreeCmpObject(CompoundObject *c_obj)
{
  Free(c_obj->part_list);

  if(c_obj->lod_object_list)
  {
    for(int i=0; i<c_obj->part_count; i++)
	{
      FreeLodObject(&(c_obj->lod_object_list[i]));
    }
    Free(c_obj->lod_object_list);
  }

  Free(c_obj->transform_list);

//  FreeTxtLib(&(c_obj->tl));
//  FreeAnimTxtLib(&(c_obj->atl));
//  FreeMtlLib(&(c_obj->ml));

  Free(c_obj->fix_list);
  Free(c_obj->pris_list);
  Free(c_obj->rev_list);
  Free(c_obj->sphere_list);
  Free(c_obj->trans_list);
  Free(c_obj->loose_list);
  for(int i=0; i<c_obj->script_count; i++)
  {
      Free(c_obj->script_list[i].channel_list);
  }
  Free(c_obj->script_list); // fix
  FreeNamedChannels(c_obj->n_channel_list, c_obj->n_channel_count);
  Free(c_obj->n_channel_list);

  c_obj->extents.convex_hull.free();

  
  if(verbose_level>=2)
  {
    printf("done w/ FreeCmpObject\n");
  }
}
  
void FreeNamedChannels(NamedChannel *list, int count)
{
  int i;

  for(i=0;i<count;i++){
    Free(list[i].channel.data);
  }
  // Free(list);
}

void InitFixed(Fix *fixed)
{
  fixed->parent[0]=0;
  fixed->child[0]=0;
    
  // init translation
  fixed->pos.x=
  fixed->pos.y=
  fixed->pos.z=0.0f;

  // init rotation
  fixed->orient.e01= fixed->orient.e02=
  fixed->orient.e10= fixed->orient.e12=
  fixed->orient.e20= fixed->orient.e21=0.0f;

  fixed->orient.e00=
  fixed->orient.e11=
  fixed->orient.e22=1.0f;
}

void InitPris(Pris *prismatic)
{
  prismatic->parent[0]=0;
  prismatic->child[0]=0;

  prismatic->parent_point.x=
  prismatic->parent_point.y=
  prismatic->parent_point.z=0.0;

  prismatic->child_point.x=
  prismatic->child_point.y=
  prismatic->child_point.z=0.0;

  prismatic->axis.x=
  prismatic->axis.y=
  prismatic->axis.z=(float)(1.0/sqrt(3.0));

  // init orientation
  prismatic->rel_orientation.e01= prismatic->rel_orientation.e02=
  prismatic->rel_orientation.e10= prismatic->rel_orientation.e12=
  prismatic->rel_orientation.e20= prismatic->rel_orientation.e21=0.0;

  prismatic->rel_orientation.e00=
  prismatic->rel_orientation.e11=
  prismatic->rel_orientation.e22=1.0;

  prismatic->min=
  prismatic->max=0.0;
}

void InitSphere(PersistSphere *sphere)
{
  sphere->parent[0]=0;
  sphere->child[0]=0;

  sphere->parent_point.x=
  sphere->parent_point.y=
  sphere->parent_point.z=0.0;

  sphere->child_point.x=
  sphere->child_point.y=
  sphere->child_point.z=0.0;

  // init orientation
  sphere->rel_orientation.e01= sphere->rel_orientation.e02=
  sphere->rel_orientation.e10= sphere->rel_orientation.e12=
  sphere->rel_orientation.e20= sphere->rel_orientation.e21=0.0;

  sphere->rel_orientation.e00=
  sphere->rel_orientation.e11=
  sphere->rel_orientation.e22=1.0;

  sphere->min_about_i=
  sphere->max_about_i=0.0f;
  sphere->min_about_j=
  sphere->max_about_j=0.0f;
  sphere->min_about_k=
  sphere->max_about_k=0.0f;
}

void InitRev(Rev *revolute)
{
  revolute->parent[0]=0;
  revolute->child[0]=0;

  revolute->parent_point.x=
  revolute->parent_point.y=
  revolute->parent_point.z=0.0;

  revolute->child_point.x=
  revolute->child_point.y=
  revolute->child_point.z=0.0;

  revolute->axis.x=
  revolute->axis.y=
  revolute->axis.z=(float)(1.0/sqrt(3.0));

  // init orientation
  revolute->rel_orientation.e01= revolute->rel_orientation.e02=
  revolute->rel_orientation.e10= revolute->rel_orientation.e12=
  revolute->rel_orientation.e20= revolute->rel_orientation.e21=0.0;

  revolute->rel_orientation.e00=
  revolute->rel_orientation.e11=
  revolute->rel_orientation.e22=1.0;

  revolute->min=
  revolute->max=0.0;
}

/*
void LoadCompoundObject(CompoundObject *c_obj, file_node *root)
{
  InitCompoundObject(c_obj);

  LoadCompoundNames(c_obj, root);
  LoadCompoundFix(c_obj, root);
  LoadCompoundPris(c_obj, root);
  LoadCompoundRev(c_obj, root);
  LoadCompoundSphere(c_obj, root);
  printf("WARNING: LoadCompoundTranslational & Loose not yet supported.\n"); 

  LoadNChannels(c_obj, root);
  LoadScripts(c_obj, root);
}
*/

void LoadCompoundNames(CompoundObject *c_obj, file_node *root)
{
  file_node *node;
  file_node *next;

  node=GetWideNode(root, COMPOUND_DIR_NAME);
  if(!node){
    fprintf(stderr,"Error: no %s node in file\n",COMPOUND_DIR_NAME);
    exit(1);
  }

  next=node->child;
  while(next && 
        (!strncmp(next->name, "Root", 4) || !strncmp(next->name, "Part", 4))){
      c_obj->part_count++;
      c_obj->part_list=(NamedPart*)Realloc(c_obj->part_list,
                       c_obj->part_count*sizeof(NamedPart));
    if(next->child){ // old format
      node=GetWideNodeG(next, OBJECT_NAME);
      //node=next->child; 
      strcpy(c_obj->part_list[c_obj->part_count-1].obj_name, (char*)node->data);
  
      node=GetWideNodeG(next, FILE_NAME);
      //node=node->sibling;
      strcpy(c_obj->part_list[c_obj->part_count-1].file_name, (char*)node->data);
  
      node=GetWideNodeG(next, "Index");
      //node=node->sibling;
      c_obj->part_list[c_obj->part_count-1].index=*((int*)node->data);
    }
    else{
      strcpy(c_obj->part_list[c_obj->part_count-1].obj_name,
        (char*)next->data);
      strcpy(c_obj->part_list[c_obj->part_count-1].file_name,
        (char*)(next->data+64));
    }
    next=next->sibling;
  }
}

void LoadNChannels(CompoundObject *c_obj, file_node *root)
{
  file_node *node;
  file_node *tmp;

  node=GetWideNode(root, ANIMATION);

  if(node){
    node=GetWideNodeG(node, CHNL);
    node=node->child;
 
    while(node){
      c_obj->n_channel_count++;
      c_obj->n_channel_list=(NamedChannel*)Realloc(c_obj->n_channel_list,
                            c_obj->n_channel_count*sizeof(NamedChannel));
      InitNamedChannel(&(c_obj->n_channel_list[c_obj->n_channel_count-1]));
      strcpy(c_obj->n_channel_list[c_obj->n_channel_count-1].name, node->name);

      tmp=node->child; // Header
      c_obj->n_channel_list[c_obj->n_channel_count-1].channel.header=
        *(Header*)(tmp->data);

      tmp=tmp->sibling; // Frames 
      c_obj->n_channel_list[c_obj->n_channel_count-1].channel.data=
        (unsigned char*)Malloc(tmp->data_size);
      memcpy(c_obj->n_channel_list[c_obj->n_channel_count-1].channel.data,
        tmp->data, tmp->data_size);

      node=node->sibling;
    }
  }
}

/*
void LoadScripts(CompoundObject *c_obj, file_node *root)
{
  file_node *node;

  node=GetWideNode(root, ANIMATION);

  if(node){
    node=GetWideNodeG(node, SCRIPT);
    node=node->child;
 
    while(node){
      c_obj->script_count++;
      c_obj->script_list=(NamedScript*)Realloc(c_obj->script_list,
                         c_obj->script_count*sizeof(NamedScript));
      InitScript(&(c_obj->script_list[c_obj->script_count-1]));

      if(node->type == D){
        file_node *tmp;
        file_node *map_node;
        map_node = node->child;

        while(map_node){
          c_obj->script_list[c_obj->script_count-1].channel_count++;

          c_obj->script_list[c_obj->script_count-1].channel_list=
            (PersistAnimChannelMapping*)Realloc(
              c_obj->script_list[c_obj->script_count-1].channel_list,
              c_obj->script_list[c_obj->script_count-1].channel_count *
              sizeof(PersistAnimChannelMapping));
 
          tmp = GetDeepNode(map_node, "Parent name");
          if(tmp)
          {
            strcpy(c_obj->script_list[c_obj->script_count-1].channel_list
                  [c_obj->script_list[c_obj->script_count-1].channel_count-1].parent,
                  (char*)tmp->data);
          }
 
          tmp = GetDeepNode(map_node, "Child name");
          if(tmp)
          {
            strcpy(c_obj->script_list[c_obj->script_count-1].channel_list
                   [c_obj->script_list[c_obj->script_count-1].channel_count-1].child,
                   (char*)tmp->data);
          }
  
          tmp = GetDeepNode(map_node, "Channel name");
          if(tmp)
          {
            strcpy(c_obj->script_list[c_obj->script_count-1].channel_list
                   [c_obj->script_list[c_obj->script_count-1].channel_count-1].channel,
                   (char*)tmp->data);
          }
  
          map_node = map_node->sibling;
        }
      }
      else{ // old format

        c_obj->script_list[c_obj->script_count-1].channel_count=
          node->data_size/sizeof(PersistAnimChannelMapping);

        c_obj->script_list[c_obj->script_count-1].channel_list=
          (PersistAnimChannelMapping*)Malloc(node->data_size);

        memcpy(c_obj->script_list[c_obj->script_count-1].channel_list,
          node->data, node->data_size);
      }

      node=node->sibling;
    }
  }
}
*/

void LoadCompoundFix(CompoundObject *c_obj, file_node *root)
{
  file_node *node;

  node=GetWideNode(root, JT_FIXED_NAME);
  if(!node)
    return;

  c_obj->fix_count=node->data_size/sizeof(Fix);
  if(c_obj->fix_count*sizeof(Fix) != node->data_size){
    fprintf(stderr,"Error: wrong size of %s data chunk.\n",JT_FIXED_NAME);
    exit(1);
  }
  c_obj->fix_list=(Fix*)Malloc(c_obj->fix_count*sizeof(Fix));

  memcpy(c_obj->fix_list, node->data, c_obj->fix_count*sizeof(Fix));
}

void LoadCompoundPris(CompoundObject *c_obj, file_node *root)
{
  file_node *node;

  node=GetWideNode(root, JT_PRISMATIC_NAME);
  if(!node)
    return;

  c_obj->pris_count=node->data_size/sizeof(Pris);
  if(c_obj->pris_count*sizeof(Pris) != node->data_size){
    fprintf(stderr,"Error: wrong size of %s data chunk.\n",JT_PRISMATIC_NAME);
    exit(1);
  }
  c_obj->pris_list=(Pris*)Malloc(c_obj->pris_count*sizeof(Pris));

  memcpy(c_obj->pris_list, node->data, c_obj->pris_count*sizeof(Pris));
}

void LoadCompoundRev(CompoundObject *c_obj, file_node *root)
{
  file_node *node;

  node=GetWideNode(root, JT_REVOLUTE_NAME);
  if(!node)
    return;

  c_obj->rev_count=node->data_size/sizeof(Rev);
  if(c_obj->rev_count*sizeof(Rev) != node->data_size){
    fprintf(stderr,"Error: wrong size of %s data chunk.\n",JT_REVOLUTE_NAME);
    exit(1);
  }
  c_obj->rev_list=(Rev*)Malloc(c_obj->rev_count*sizeof(Rev));

  memcpy(c_obj->rev_list, node->data, c_obj->rev_count*sizeof(Rev));
}

void LoadCompoundSphere(CompoundObject *c_obj, file_node *root)
{
  file_node *node;

  node=GetWideNode(root, JT_SPHERICAL_NAME);
  if(!node)
    return;

  c_obj->sphere_count=node->data_size/sizeof(PersistSphere);
  if(c_obj->sphere_count*sizeof(PersistSphere) != node->data_size){
    fprintf(stderr,"Error: wrong size of %s data chunk.\n",JT_SPHERICAL_NAME);
    exit(1);
  }
  c_obj->sphere_list=(PersistSphere*)Malloc(c_obj->sphere_count*sizeof(PersistSphere));

  memcpy(c_obj->sphere_list, node->data, c_obj->sphere_count*sizeof(PersistSphere));
}

void PrintPersistMatrix(PersistMatrix *mtx)
{
  printf("%.5f %.5f %.5f\n",mtx->e00, mtx->e01, mtx->e02);
  printf("%.5f %.5f %.5f\n",mtx->e10, mtx->e11, mtx->e12);
  printf("%.5f %.5f %.5f\n",mtx->e20, mtx->e21, mtx->e22);
} 

void PrintPersistVector(PersistVector *vec)
{
  printf("%.5f %.5f %.5f\n",vec->x, vec->y, vec->z);
}

void PrintPris(Pris *con)
{
  printf("\nPris\n");
  printf("parent=%s child=%s\n",con->parent, con->child);
  printf("parent_point\n");
  PrintPersistVector(&(con->parent_point));
  printf("child_point\n");
  PrintPersistVector(&(con->child_point));
  printf("axis\n");
  PrintPersistVector(&(con->axis));
  printf("rel_orientation\n");
  PrintPersistMatrix(&(con->rel_orientation));
  printf("min=%.5f max=%.5f\n",con->min, con->max);
}

void PrintRev(Rev *con)
{
  printf("\nRev\n");
  printf("parent=%s child=%s\n",con->parent, con->child);
  printf("parent_point\n");
  PrintPersistVector(&(con->parent_point));
  printf("child_point\n");
  PrintPersistVector(&(con->child_point));
  printf("axis\n");
  PrintPersistVector(&(con->axis));
  printf("rel_orientation\n");
  PrintPersistMatrix(&(con->rel_orientation));
  printf("min=%.5f max=%.5f\n",con->min, con->max);
}

void PrintSphere(PersistSphere *con)
{
  printf("\nSphere\n");
  printf("parent=%s child=%s\n",con->parent, con->child);
  printf("parent_point\n");
  PrintPersistVector(&(con->parent_point));
  printf("child_point\n");
  PrintPersistVector(&(con->child_point));
  printf("rel_orientation\n");
  PrintPersistMatrix(&(con->rel_orientation));
  printf("min_i=%.5f max_i=%.5f\n",con->min_about_i, con->max_about_i);
  printf("min_j=%.5f max_j=%.5f\n",con->min_about_j, con->max_about_j);
  printf("min_k=%.5f max_k=%.5f\n",con->min_about_k, con->max_about_k);
}

void PrintFix(Fix *con)
{
  printf("\nFix\n");
  printf("parent=%s child=%s\n",con->parent, con->child);
  printf("pos\n");
  PrintPersistVector(&(con->pos));
  printf("orient\n");
  PrintPersistMatrix(&(con->orient));
}

void PrintScript(NamedScript *script, int id)
{
  int i;

  printf("script id %d:\n",id);
  printf("number of channels %d\n",script->channel_count);

  for(i=0; i<script->channel_count; i++){
    printf("channel index %d:\n",i);
    printf("parent=%s child=%s channel_name=%s\n",
      script->channel_list[i].parent,
      script->channel_list[i].child,
      script->channel_list[i].channel);
  }

  printf("\n");
}

void PrintNChannel(CompoundObject *c_obj, NamedChannel *n_ch, int id)
{
  float *f_data;
  int i;
  int type;

  printf("channel %s   id %d:\n",n_ch->name, id);

  type=GetChannelType(c_obj, id);
  switch(type)
  {
  case REVOLUTE:
    printf("Assigned to a REVOLUTE joint.\n");
    break;
  case PRISMATIC:
    printf("Assigned to a PRISMATIC joint.\n");
    break;
  case SPHERICAL:
    printf("Assigned to a SPHERICAL joint.\n");
    break;
  case TRANSLATIONAL:
    printf("Assigned to a TRANSLATIONAL joint.\n");
    printf("print not yet supported\n"); 
    return;
    // break;
  case LOOSE:
    printf("Assigned to a LOOSE joint.\n");
    break;
  case EVENT:
    break;
  default:
    Winprint("Error: unknown channel type!\n");
  }

  printf("frames=%u capture_rate=%f fps=%f type=%u\n",
    n_ch->channel.header.frames, n_ch->channel.header.capture_rate,
    1.0/n_ch->channel.header.capture_rate, n_ch->channel.header.type);

  f_data=(float*)n_ch->channel.data;
  if(n_ch->channel.header.capture_rate<0){ // key frames
    if(type==REVOLUTE){
      for(i=0; i<n_ch->channel.header.frames; i++){
        printf("time=%f value=%f R   %f Deg \n",*(f_data), *(f_data+1),
          (*(f_data+1))/D2R);
        f_data+=2;
      }
    }else
    if(type==PRISMATIC){
      for(i=0; i<n_ch->channel.header.frames; i++){
        printf("time=%f value=%f\n",*(f_data), *(f_data+1));
        f_data+=2;
      }
    }
    else{
      fprintf(stderr,"Error: don't yet know how to read this channel.\n");
    }
  }
  else{ // periodic
    if(type==REVOLUTE){
      printf("%d value=%f R   %fDeg\n",0, *(f_data), (*(f_data))/D2R);
      f_data++;
      for(i=1; i<n_ch->channel.header.frames; i++){
        printf("%d value=%fR   %fDeg  delta=%fDeg\n",i, *(f_data), (*(f_data))/D2R,
        (*(f_data)-*(f_data-1))/D2R);
        f_data++;
      }
    }else
    if(type==PRISMATIC){
      printf("%d value=%f\n",0, *(f_data));
      f_data++;
      for(i=1; i<n_ch->channel.header.frames; i++){
        printf("%d value=%f delta=%f\n",i, *(f_data), *(f_data)-*(f_data-1));
        f_data++;
      }
    }else
    if(type==SPHERICAL){
      for(i=0; i<n_ch->channel.header.frames; i++){
        printf("%d ",i);
        PrintQuat(f_data);
        f_data+=4;
      }
    }else
    if(type==LOOSE){
      for(i=0; i<n_ch->channel.header.frames; i++){
        printf("%d \n",i);
        printf("trans x=%.3f y=%.3f z=%.3f\n",*(f_data), *(f_data+1), *(f_data+2));
        PrintQuat(f_data+3);
        f_data+=7;
      }
    }
    else{
      fprintf(stderr,"Error: don't know how to read this channel type.\n"); 
    }
  }

  printf("\n");
}

void InitFrame(Frame *frame)
{
  frame->name=NULL;
  frame->parent_name=NULL;

  frame->pris_count=0;
  frame->rev_count=0;

  frame->from.x=frame->from.y=frame->from.z=0.0;
  frame->delta.x=frame->delta.y=frame->delta.z=0.0;
  frame->step=0.0;

  frame->center.x=frame->center.y=frame->center.z=0.0;
  frame->vector_t.x=frame->vector_t.y=frame->vector_t.z=0.0;
  frame->vector_r.x=frame->vector_r.y=frame->vector_r.z=0.0;

  frame->angle=0.0;
  frame->orient.e00=frame->orient.e11=frame->orient.e22=1.0;
  frame->orient.e01=frame->orient.e02=
  frame->orient.e10=frame->orient.e12=
  frame->orient.e20=frame->orient.e21=0.0;

  frame->quat.v.w=1.0f;
  frame->quat.v.x=frame->quat.v.y=frame->quat.v.z=0.0f;

  frame->index=INT_MAX;
}

void InitDofData(DofData *data)
{
  data->type=-1;
  data->name[0]=0;
  data->pivot[0]=data->pivot[1]=data->pivot[2]=0.0;
  data->min_t[0]=data->min_t[1]=data->min_t[2]=0.0;
  data->max_t[0]=data->max_t[1]=data->max_t[2]=0.0;
  data->min_r[0]=data->min_r[1]=data->min_r[2]=0.0;
  data->max_r[0]=data->max_r[1]=data->max_r[2]=0.0;
  data->axis_pt[0]=data->axis_pt[1]=data->axis_pt[2]=0.0;
  data->plane_pt[0]=data->plane_pt[1]=data->plane_pt[2]=0.0;

  // computed from animation data
  data->axis[0]=data->axis[1]=data->axis[2]=0.0;
  data->min_angle=data->max_angle=0.0;
  data->min_step=data->max_step=0.0;
}

void DofData::Dump(FILE *strm) const
{
  fprintf(strm, "DofJoint: %s\n", name);
  fprintf(strm, "Pivot: %.3f %.3f %.3f\n", pivot[0], pivot[1], pivot[2]);

  switch(type)
  {
    case FFIXED:
    {
      fprintf(strm, "Type: FIXED\n");
    }
    break;
    case REVOLUTE:
    {
      fprintf(strm, "Type: REVOLUTE\n");
      fprintf(strm, "Axis: %.3f %.3f %.3f\n", axis[0], axis[1], axis[2]);
      fprintf(strm, "min_angle: %.3f\n", R2D*min_angle);
      fprintf(strm, "max_angle: %.3f\n", R2D*max_angle);
    }
    break;
    case PRISMATIC:
    {
      fprintf(strm, "Type: PRISMATIC\n");
      fprintf(strm, "Axis: %.3f %.3f %.3f\n", axis[0], axis[1], axis[2]);
      fprintf(strm, "min_step: %.3f\n", min_step);
      fprintf(strm, "max_step: %.3f\n", max_step);
    }
    break;
    case TRANSLATIONAL:
    {
      fprintf(strm, "Type: TRANSLATIONAL\n");
      fprintf(strm, "min_t: %.3f %.3f %.3f\n", min_t[0], min_t[1], min_t[2]);
      fprintf(strm, "max_t: %.3f %.3f %.3f\n", max_t[0], max_t[1], max_t[2]);
    }
    break;
    case SPHERICAL:
    {
      fprintf(strm, "Type: SPHERICAL\n");
      fprintf(strm, "min_r: %.3f %.3f %.3f\n", 
        R2D * min_r[0], 
        R2D * min_r[1], 
        R2D * min_r[2]);
      fprintf(strm, "max_r: %.3f %.3f %.3f\n", 
        R2D * max_r[0], 
        R2D * max_r[1], 
        R2D * max_r[2]);
    }
    break;
    default:
    {
      fprintf(strm, "Type: UNKNOWN/UNSUPPORTED %d\n", type);
      fprintf(strm, "Axis: %.3f %.3f %.3f\n", axis[0], axis[1], axis[2]);
      fprintf(strm, "min_step: %.3f\n", min_step);
      fprintf(strm, "max_step: %.3f\n", max_step);
      fprintf(strm, "min_t: %.3f %.3f %.3f\n", min_t[0], min_t[1], min_t[2]);
      fprintf(strm, "max_t: %.3f %.3f %.3f\n", max_t[0], max_t[1], max_t[2]);
      fprintf(strm, "min_angle: %.3f\n", R2D*min_angle);
      fprintf(strm, "max_angle: %.3f\n", R2D*max_angle);
      fprintf(strm, "min_r: %.3f %.3f %.3f\n", 
        R2D * min_r[0], 
        R2D * min_r[1], 
        R2D * min_r[2]);
      fprintf(strm, "max_r: %.3f %.3f %.3f\n", 
        R2D * max_r[0], 
        R2D * max_r[1], 
        R2D * max_r[2]);
    }
  }
}

void InitNamedChannel(NamedChannel *n_channel)
{
  n_channel->name[0]=0;
  n_channel->first_frame=0;
  n_channel->last_frame=0;
  n_channel->data_size=0;
  InitChannel(&(n_channel->channel));
}

void InitChannel(Channel *channel)
{
  InitHeader(&(channel->header));
  channel->data=NULL;
}

void InitHeader(Header *header)
{
  header->frames=0;
  header->capture_rate=0;
  header->type=0;
}

void InitScript(NamedScript *script)
{
  script->channel_count=0;
  script->channel_list=NULL;
  script->name[0] = 0;
}


file_node* CreateAnimation(CompoundObject *c_obj)
{
  if(c_obj->n_channel_count > 0 && c_obj->script_count > 0)
  {
    file_node *anim_node;
    file_node *script_node;
    file_node **next;
    int i, j;
    char script_name[256]={0};
    char prefix[256]={0};
  
    anim_node=CreateNode(ANIMATION, D);
    script_node=anim_node->child=CreateNode(SCRIPT, D);
  
    // connect scripts
    next=&(script_node->child);

    file_node **tmp=NULL;
    file_node **sc_node;
    char name[256] = {0};
  
    sc_node = next;
    for(i=0; i<c_obj->script_count; i++)
    {
      // use the name of the first channel w/o the Ch_ and tack on Sc_ instead
      assert(!strncmp("Ch_", c_obj->script_list[i].channel_list[0].channel, 3));
  
      strcpy(prefix, c_obj->script_list[i].channel_list[0].channel+3);
      
#ifdef GAMEGEN
      if(c_obj->script_list[i].channel_count > 1)
      {
          assert(strstr(prefix, "_"));
          StripPostfix(prefix);
      }
	  sprintf(script_name,"Sc_%s", prefix);

#else // MAX

	  if(strlen(c_obj->script_list[i].name) == 0)
	  {
		  Winprint("Error: UNNAMED Script!\n");
	  }
	  strcpy(script_name, c_obj->script_list[i].name);
#endif

      *sc_node = CreateNode(script_name, D);
      tmp = next = &((*sc_node)->child);

      // add root height from floor (only if we have a root animation)
      for(j=0; j<c_obj->script_list[i].channel_count; j++)
      {
          if( (c_obj->script_list[i].channel_list[j].child[0] == 0) &&
              !strcmp(c_obj->script_list[i].channel_list[j].parent, ROOT_OBJ_NAME) )
          {
              *tmp = CreateNode("Root height", F);
              (*tmp)->data_size = sizeof(float);
#ifdef ALIAS
              (*tmp)->data = (unsigned char*)&c_obj->root_transform.v.z;
#else
              (*tmp)->data = (unsigned char*)&c_obj->root_transform.v.y;
#endif
              tmp = next = &((*tmp)->sibling);
              break;
          }
      }

      // list any channels assigned to root
	  int id = 0;
      for(j=0; j<c_obj->script_list[i].channel_count; j++)
	  {
          if( (c_obj->script_list[i].channel_list[j].child[0] == 0) &&
              (c_obj->script_list[i].channel_list[j].parent[0] != 0) )
		  {
              sprintf(name, "Object map %d", id);
              *tmp = CreateNode(name, D);
              next = &((*tmp)->sibling);
              tmp = &((*tmp)->child);
              
               *tmp = CreateNode("Parent name", F);
              (*tmp)->data_size = strlen(c_obj->script_list[i].channel_list[j].parent) + 1;
              (*tmp)->data = (unsigned char*)c_obj->script_list[i].channel_list[j].parent;
              tmp = &((*tmp)->sibling);
  
			  *tmp = CreateChannel(c_obj, c_obj->script_list[i].channel_list[j].channel);
              tmp = &((*tmp)->sibling);
  
              tmp = next;
			  id++;
          }
      }
    
      tmp = next;
      // list any channels assigned to joints
	  id = 0;
      for(j=0; j<c_obj->script_list[i].channel_count; j++)
	  {
          if( (c_obj->script_list[i].channel_list[j].child[0] != 0) &&
              (c_obj->script_list[i].channel_list[j].parent[0] != 0) )
		  {
              sprintf(name, "Joint map %d", id);
              *tmp = CreateNode(name, D);
              next = &((*tmp)->sibling);
              tmp = &((*tmp)->child);
  
              *tmp = CreateNode("Parent name", F);
              (*tmp)->data_size = strlen(c_obj->script_list[i].channel_list[j].parent) + 1;
              (*tmp)->data = (unsigned char*)c_obj->script_list[i].channel_list[j].parent;
              tmp = &((*tmp)->sibling);
  
              *tmp = CreateNode("Child name", F);
              (*tmp)->data_size = strlen(c_obj->script_list[i].channel_list[j].child) + 1;
              (*tmp)->data = (unsigned char*)c_obj->script_list[i].channel_list[j].child;
               tmp = &((*tmp)->sibling);
  
			  *tmp = CreateChannel(c_obj, c_obj->script_list[i].channel_list[j].channel);
              tmp = &((*tmp)->sibling);
  
              tmp = next;
			  id++;
          }
      }

      tmp = next;
      // list non assigned channels (events)
	  id = 0;
      for(j=0; j<c_obj->script_list[i].channel_count; j++)
	  {
          if( (c_obj->script_list[i].channel_list[j].child[0] == 0) &&
              (c_obj->script_list[i].channel_list[j].parent[0] == 0) )
		  {
              sprintf(name, "Event map %d", id);
              *tmp = CreateNode(name, D);
              next = &((*tmp)->sibling);
              tmp = &((*tmp)->child);
  
			  *tmp = CreateChannel(c_obj, c_obj->script_list[i].channel_list[j].channel);
              tmp = &((*tmp)->sibling);
  
              tmp = next;
			  id++;
          }
      }

      sc_node = &((*sc_node)->sibling);
    }

    if(verbose_level>=2){
      printf("done w/ CreateAnimation\n");
    }

    return anim_node;
  }

  return NULL;
}

file_node *CreateChannel( CompoundObject *c_obj, const char *name )
{
	file_node *node = NULL;
	file_node *tmp;

	// find channel
	NamedChannel *n_channel = NULL;
	for(int i = 0; i < c_obj->n_channel_count; i++)
	{
		if(!strcmp(name, c_obj->n_channel_list[i].name))
		{
			n_channel = c_obj->n_channel_list + i;
			break;
		}
	}
	assert(n_channel);
	
      node = CreateNode("Channel", D);

      tmp = node->child = CreateNode("Header", F);
      tmp->data_size = sizeof(n_channel->channel.header);
      tmp->data = (unsigned char*)&(n_channel->channel.header);

      tmp = tmp->sibling = CreateNode("Frames", F);

      if(n_channel->channel.header.type ==
         (PersistDT_VECTOR | PersistDT_QUATERNION))
	  {
        tmp->data_size = n_channel->channel.header.frames *
        (sizeof(PersistVector) + sizeof(PersistQuaternion) );
      }
      else
      if(n_channel->channel.header.type==PersistDT_FLOAT)
	  {
        tmp->data_size = n_channel->channel.header.frames*sizeof(float);
      }
      else
      if(n_channel->channel.header.type==PersistDT_QUATERNION)
	  {
        tmp->data_size = n_channel->channel.header.frames*sizeof(PersistQuaternion);
      }
      else
      if(n_channel->channel.header.type==PersistDT_VECTOR)
	  {
        tmp->data_size = n_channel->channel.header.frames*sizeof(PersistVector);
      }
      else
      if(n_channel->channel.header.type==PersistDT_EVENT)
	  {
          goto event_label; // we can't figure the channel size here since it has arbitrary data type
          // tacked on at the end
      }
      else
	  {
        Winprint("Error: unsupported channel type.\n");
        exit(1);
      }
  
      // increase size for time stamps
      if(n_channel->channel.header.capture_rate < 0)
	  { 
        // non periodic data
        tmp->data_size += n_channel->channel.header.frames*sizeof(float);
      }
  
event_label:
      // event channel data size can't be computed so we kept track of it
      if(n_channel->channel.header.type==PersistDT_EVENT)
	  {
          tmp->data_size = n_channel->data_size;
      }
  
      tmp->data=(n_channel->channel.data);
	
	return node;
}


// old format w/ separate script and channel chunks
/*
file_node* CreateAnimation(CompoundObject *c_obj)
{
  if(c_obj->n_channel_count > 0 && c_obj->script_count > 0)
  {
    file_node *anim_node;
    file_node *chnl_node;
    file_node *script_node;
    file_node **next;
    int i, j;
    char script_name[256]={0};
    char prefix[256]={0};
  
    anim_node=CreateNode(ANIMATION, D);
    chnl_node=anim_node->child=CreateNode(CHNL, D);
    script_node=chnl_node->sibling=CreateNode(SCRIPT, D);
  
    // connect channels
    next=&(chnl_node->child);
    for(i=0; i<c_obj->n_channel_count; i++)
    {
      *next=CreateNode(c_obj->n_channel_list[i].name, D);
      (*next)->child=CreateNode("Header", F);
      (*next)->child->data_size=sizeof(c_obj->n_channel_list[i].channel.header);
      (*next)->child->data=(unsigned char*)
                           &(c_obj->n_channel_list[i].channel.header);
      (*next)->child->sibling=CreateNode("Frames", F);
  
      if(c_obj->n_channel_list[i].channel.header.type ==
         (PersistDT_VECTOR | PersistDT_QUATERNION)){
        (*next)->child->sibling->data_size =
        c_obj->n_channel_list[i].channel.header.frames *
        (sizeof(PersistVector) + sizeof(PersistQuaternion) );
      }
      else
      if(c_obj->n_channel_list[i].channel.header.type==PersistDT_FLOAT){
        (*next)->child->sibling->data_size=
        c_obj->n_channel_list[i].channel.header.frames*sizeof(float);
      }
      else
      if(c_obj->n_channel_list[i].channel.header.type==PersistDT_QUATERNION){
        (*next)->child->sibling->data_size=
        c_obj->n_channel_list[i].channel.header.frames*sizeof(PersistQuaternion);
      }
      else
      if(c_obj->n_channel_list[i].channel.header.type==PersistDT_VECTOR){
        (*next)->child->sibling->data_size=
        c_obj->n_channel_list[i].channel.header.frames*sizeof(PersistVector);
      }
      else
      if(c_obj->n_channel_list[i].channel.header.type==PersistDT_EVENT){
          goto event_label; // we can't figure the channel size here since it has arbitrary data type
          // tacked on at the end
      }
      else{
        Winprint("Error: unsupported channel type.\n");
        exit(1);
      }
  
      // increase size for time stamps
      if(c_obj->n_channel_list[i].channel.header.capture_rate < 0){ 
        // non periodic data
        (*next)->child->sibling->data_size +=
        c_obj->n_channel_list[i].channel.header.frames*sizeof(float);
      }
  
event_label:
      // event channel data size can't be computed so we kept track of it
      if(c_obj->n_channel_list[i].channel.header.type==PersistDT_EVENT){
          (*next)->child->sibling->data_size = c_obj->n_channel_list[i].data_size;
      }
  
      (*next)->child->sibling->data=(c_obj->n_channel_list[i].channel.data);
      next=&((*next)->sibling);
    }

    // connect scripts
    next=&(script_node->child);

    file_node **tmp=NULL;
    file_node **sc_node;
    char name[256] = {0};
  
    sc_node = next;
    for(i=0; i<c_obj->script_count; i++)
    {
      // use the name of the first channel w/o the Ch_ and tack on Sc_ instead
      assert(!strncmp("Ch_", c_obj->script_list[i].channel_list[0].channel, 3));
  
      strcpy(prefix, c_obj->script_list[i].channel_list[0].channel+3);
      
#ifdef GAMEGEN
          if(c_obj->script_list[i].channel_count > 1)
          {
              assert(strstr(prefix, "_"));
              StripPostfix(prefix);
          }
	  sprintf(script_name,"Sc_%s", prefix);

#else // MAX

	  if(strlen(c_obj->script_list[i].name) == 0)
	  {
		  Winprint("Error: UNNAMED Script!\n");
	  }
	  strcpy(script_name, c_obj->script_list[i].name);
#endif

      *sc_node = CreateNode(script_name, D);
      tmp = next = &((*sc_node)->child);

      // add root height from floor (only if we have a root animation)
      for(j=0; j<c_obj->script_list[i].channel_count; j++)
      {
          if( (c_obj->script_list[i].channel_list[j].child[0] == 0) &&
              !strcmp(c_obj->script_list[i].channel_list[j].parent, ROOT_OBJ_NAME) )
          {
              *tmp = CreateNode("Root height", F);
              (*tmp)->data_size = sizeof(float);
#ifdef ALIAS
              (*tmp)->data = (unsigned char*)&c_obj->root_transform.v.z;
#else
              (*tmp)->data = (unsigned char*)&c_obj->root_transform.v.y;
#endif
              tmp = next = &((*tmp)->sibling);
              break;
          }
      }

      // list any channels assigned to root
      for(j=0; j<c_obj->script_list[i].channel_count; j++){
          if( (c_obj->script_list[i].channel_list[j].child[0] == 0) &&
              (c_obj->script_list[i].channel_list[j].parent[0] != 0) ){
  
              sprintf(name, "Object map %d", j);
              *tmp = CreateNode(name, D);
              next = &((*tmp)->sibling);
              tmp = &((*tmp)->child);
              
               *tmp = CreateNode("Parent name", F);
              (*tmp)->data_size = strlen(c_obj->script_list[i].channel_list[j].parent) + 1;
              (*tmp)->data = (unsigned char*)c_obj->script_list[i].channel_list[j].parent;
              tmp = &((*tmp)->sibling);
  
              *tmp = CreateNode("Channel name", F);
              (*tmp)->data_size = strlen(c_obj->script_list[i].channel_list[j].channel) + 1;
              (*tmp)->data = (unsigned char*)c_obj->script_list[i].channel_list[j].channel;
              tmp = &((*tmp)->sibling);
  
              tmp = next;
          }
      }
    
      tmp = next;
      // list any channels assigned to joints
      for(j=0; j<c_obj->script_list[i].channel_count; j++){
          if( (c_obj->script_list[i].channel_list[j].child[0] != 0) &&
              (c_obj->script_list[i].channel_list[j].parent[0] != 0) ){
  
              sprintf(name, "Joint map %d", j);
              *tmp = CreateNode(name, D);
              next = &((*tmp)->sibling);
              tmp = &((*tmp)->child);
  
              *tmp = CreateNode("Parent name", F);
              (*tmp)->data_size = strlen(c_obj->script_list[i].channel_list[j].parent) + 1;
              (*tmp)->data = (unsigned char*)c_obj->script_list[i].channel_list[j].parent;
              tmp = &((*tmp)->sibling);
  
              *tmp = CreateNode("Child name", F);
              (*tmp)->data_size = strlen(c_obj->script_list[i].channel_list[j].child) + 1;
              (*tmp)->data = (unsigned char*)c_obj->script_list[i].channel_list[j].child;
               tmp = &((*tmp)->sibling);
  
              *tmp = CreateNode("Channel name", F);
              (*tmp)->data_size = strlen(c_obj->script_list[i].channel_list[j].channel) + 1;
              (*tmp)->data = (unsigned char*)c_obj->script_list[i].channel_list[j].channel;
              tmp = &((*tmp)->sibling);
  
              tmp = next;
          }
      }

      tmp = next;
      // list non assigned channels (events)
      for(j=0; j<c_obj->script_list[i].channel_count; j++){
          if( (c_obj->script_list[i].channel_list[j].child[0] == 0) &&
              (c_obj->script_list[i].channel_list[j].parent[0] == 0) ){
  
              sprintf(name, "Event map %d", j);
              *tmp = CreateNode(name, D);
              next = &((*tmp)->sibling);
              tmp = &((*tmp)->child);
  
              *tmp = CreateNode("Channel name", F);
              (*tmp)->data_size = strlen(c_obj->script_list[i].channel_list[j].channel) + 1;
              (*tmp)->data = (unsigned char*)c_obj->script_list[i].channel_list[j].channel;
              tmp = &((*tmp)->sibling);
  
              tmp = next;
          }
      }

      sc_node = &((*sc_node)->sibling);
    }

    if(verbose_level>=2){
      printf("done w/ CreateAnimation\n");
    }

    return anim_node;
  }

  return NULL;
}
*/

/* can fail to reset a joint that has been moved before
void RemoveIdentityChannels(CompoundObject *c_obj)
{
	for(int i=0; i<c_obj->n_channel_count; i++)
	{
	  if(IsIdentityChannel(c_obj, i))
	  {
		RemoveChannel(c_obj, i);
		if(i < c_obj->n_channel_count) // remove channel swaps this w/ last
		{
			i--;
		}
	  }
	}
}
*/

/* would mess up looping
void ShortenConstantChannels(CompoundObject *c_obj)
{
	for(int i=0; i<c_obj->n_channel_count; i++)
	{
		ShortenChannel(c_obj, i);

		// TODO check if removed so i-- can back up one
	}
}
*/

/*
void RemoveConstantChannels(CompoundObject *c_obj)
{
  if(remove_constant_channels)
  {
    for(int i=0; i<c_obj->n_channel_count; i++)
    {
      if(IsConstantChannel(c_obj, i))
      {
        RemoveChannel(c_obj, i);
		if(i < c_obj->n_channel_count) // remove channel swaps this w/ last
		{
			i--;
		}
      }
    }
  }
}

bool IsConstantChannel(CompoundObject *c_obj, int id)
{
  //char *ch_name=c_obj->n_channel_list[id].name;

  int type = GetChannelType(c_obj, id);
  int frame_size;
  switch(type)
  {
    case EVENT:
      return false;
    case REVOLUTE:
      frame_size = sizeof(float); 
      break;
    case PRISMATIC:
      frame_size = sizeof(float); 
      break;
    case SPHERICAL:
      frame_size = 4 * sizeof(float); 
      break;
    case TRANSLATIONAL:
      frame_size = 3 * sizeof(float); 
      break;
    case LOOSE:
      frame_size = 7 * sizeof(float); 
      break;
    default:
      Winprint("Error: unknown channel type for %s!\n",
        c_obj->n_channel_list[id].name);
      return false;
   }
  assert(c_obj->n_channel_list[id].channel.header.capture_rate != -1);

   float *first = (float*)c_obj->n_channel_list[id].channel.data;
   for(int j=1; j < c_obj->n_channel_list[id].channel.header.frames; j++)
   {
     float *current =
       (float*)(c_obj->n_channel_list[id].channel.data + j*frame_size);

     for(int chunk=0; chunk < frame_size/sizeof(float); chunk++)
     {
       if( fabs(first[chunk] - current[chunk]) > 0.00001f)
       {
         return false;
       }
     }
   }

   return true;
}
*/

// remove any frames at the end of a channel that don't do anything
/*
void ShortenChannel(CompoundObject *c_obj, int id)
{
  int type = GetChannelType(c_obj, id);
  int frame_size;
  switch(type)
  {
    case EVENT:
        return;
    case REVOLUTE:
      frame_size = sizeof(float); 
      break;
    case PRISMATIC:
      frame_size = sizeof(float); 
      break;
    case SPHERICAL:
      frame_size = 4 * sizeof(float); 
      break;
    case TRANSLATIONAL:
      frame_size = 3 * sizeof(float); 
      break;
    case LOOSE:
      frame_size = 7 * sizeof(float); 
      break;
    default:
      Winprint("Error: unknown channel type for %s!\n",
        c_obj->n_channel_list[id].name);
	  return;
   }
  assert(c_obj->n_channel_list[id].channel.header.capture_rate != -1);

   float *last = (float*)(c_obj->n_channel_list[id].channel.data +
	   (c_obj->n_channel_list[id].channel.header.frames - 1) * frame_size);
   int remove_count = 0;
   for(int j = c_obj->n_channel_list[id].channel.header.frames - 2; j >= 0 ; j--)
   {
     float *current =
       (float*)(c_obj->n_channel_list[id].channel.data + j*frame_size);

	 bool same = true;
	 for(int chunk=0; chunk < frame_size/sizeof(float); chunk++)
	 {
		if( fabs(*(current + chunk) - *(last + chunk)) > 0.00001f)
		{
			same = false;
			break;
		}
	 }

     //if(memcmp(last, current, frame_size))
     if(!same)
	 {
		break;
     }
	 else
	 {
		remove_count++;
	 }
   }

   if((remove_count == c_obj->n_channel_list[id].channel.header.frames - 1) &&
	   FirstFrameIsIdentity(c_obj, id))  // i.e., whole channel is identity
   {
	  RemoveChannel(c_obj, id);
   }
   else
   if(remove_count > 0)
   {
	   c_obj->n_channel_list[id].channel.header.frames -= remove_count;
	   c_obj->n_channel_list[id].channel.data =
		   (unsigned char*)Realloc(c_obj->n_channel_list[id].channel.data,
		   c_obj->n_channel_list[id].channel.header.frames * frame_size);
   }
}
*/

/*
bool IsIdentityChannel(CompoundObject *c_obj, int id)
{
  assert(c_obj->n_channel_list[id].channel.header.capture_rate != -1);
  //char *ch_name=c_obj->n_channel_list[id].name;
  
  const float tolerance = 0.0001f;
  float *current;
  int frame_size;
  int j;
  int type = GetChannelType(c_obj, id);
  switch(type)
  {
    case EVENT:
        return false;
    case REVOLUTE:
    case PRISMATIC:
		frame_size = sizeof(float);
		for(j=0; j < c_obj->n_channel_list[id].channel.header.frames; j++)
		{
			 current = (float*)(c_obj->n_channel_list[id].channel.data + j*frame_size);
			 if( fabs(0.0f - *current) > tolerance )
			 {
			   return false;
			 }
		}
      break;
    case SPHERICAL:
		frame_size = 4 * sizeof(float);
		for(j=0; j < c_obj->n_channel_list[id].channel.header.frames; j++)
		{
			 current = (float*)(c_obj->n_channel_list[id].channel.data + j*frame_size);
			 if( fabs(1.0f - *current) > tolerance )
			 {
			   return false;
			 }
		}
      break;
    case TRANSLATIONAL:
		frame_size = 3 * sizeof(float);
		for(j=0; j < c_obj->n_channel_list[id].channel.header.frames; j++)
		{
			 current = (float*)(c_obj->n_channel_list[id].channel.data + j*frame_size);
			 if( fabs(0.0f - *(current)) > tolerance ||
				 fabs(0.0f - *(current+1)) > tolerance ||
				 fabs(0.0f - *(current+2)) > tolerance)
			 {
			   return false;
			 }
		}
      break;
    case LOOSE:
		frame_size = 7 * sizeof(float);
		for(j=0; j < c_obj->n_channel_list[id].channel.header.frames; j++)
		{
			 current = (float*)(c_obj->n_channel_list[id].channel.data + j*frame_size);
			 if( fabs(0.0f - *(current)) > tolerance ||
				 fabs(0.0f - *(current+1)) > tolerance ||
				 fabs(0.0f - *(current+2)) > tolerance ||
				 fabs(1.0f - *(current+3)) > tolerance )
			 {
			   return false;
		 }
		}
      break;
    default:
      Winprint("Error: unknown channel type for %s!\n",
        c_obj->n_channel_list[id].name);
      return false;
   }

   return true;
}
*/

/*
bool FirstFrameIsIdentity(CompoundObject *c_obj, int id)
{
  assert(c_obj->n_channel_list[id].channel.header.capture_rate != -1);

  const float tolerance = 0.00001f;
  float *current;
  int type = GetChannelType(c_obj, id);
  switch(type)
  {
    case EVENT:
        return false;
    case REVOLUTE:
    case PRISMATIC:
		 current = (float*)c_obj->n_channel_list[id].channel.data;
		 if( fabs(0.0f - *current) > tolerance )
		 {
		   return false;
		 }
      break;
    case SPHERICAL:
		 current = (float*)c_obj->n_channel_list[id].channel.data;
		 if( fabs(1.0f - *current) > tolerance )
		 {
		   return false;
		 }
      break;
    case TRANSLATIONAL:
		 current = (float*)c_obj->n_channel_list[id].channel.data;
		 if( fabs(0.0f - *(current)) > tolerance ||
			 fabs(0.0f - *(current+1)) > tolerance ||
			 fabs(0.0f - *(current+2)) > tolerance)
		 {
		   return false;
		 }
      break;
    case LOOSE:
		 current = (float*)c_obj->n_channel_list[id].channel.data;
		 if( fabs(0.0f - *(current)) > tolerance ||
			 fabs(0.0f - *(current+1)) > tolerance ||
			 fabs(0.0f - *(current+2)) > tolerance ||
			 fabs(1.0f - *(current+3)) > tolerance )
		 {
		   return false;
		 }
      break;
    default:
      Winprint("Error: unknown channel type for %s!\n",
        c_obj->n_channel_list[id].name);
      return false;
   }

   return true;
}
*/

void SubNamedChannel(const NamedChannel & n_channel, NamedChannel *sub_channel,
				const int first, const int last, bool root_flag)
{
	assert(first <= last);

	int n_frames = 1 + (last - first);

	InitNamedChannel(sub_channel);

	sub_channel->first_frame = 0;
	sub_channel->last_frame = last - first;
	sub_channel->channel.header.frames = n_frames;
	sub_channel->channel.header.capture_rate = n_channel.channel.header.capture_rate;
	sub_channel->channel.header.type = n_channel.channel.header.type;
	assert(n_channel.channel.header.capture_rate > 0.0);

	  int frame_size = 0;
	  if(sub_channel->channel.header.type & PersistDT_FLOAT)
	  {
		  frame_size += sizeof(float);
	  }
	  if(sub_channel->channel.header.type & PersistDT_VECTOR)
	  {
		  frame_size += 3 * sizeof(float);
	  }
	  if(sub_channel->channel.header.type & PersistDT_QUATERNION)
	  {
		  frame_size += 4 * sizeof(float);
	  }
	  if(sub_channel->channel.header.type & PersistDT_EVENT)
	  {
		  Winprint("Error: can't handle \"PersistDT_EVENT\" channel subdivision!\n");
	  }
	  assert(frame_size >= 4);

	sub_channel->channel.data =
	  (unsigned char*)Malloc(sub_channel->channel.header.frames * frame_size);

	int offset = first * frame_size;
	const unsigned char *src = n_channel.channel.data + offset;
	unsigned char *dest = sub_channel->channel.data;
	for(int fr_id=0; fr_id < n_frames; fr_id++)
	{
		for(int b_id=0; b_id < frame_size; b_id++)
		{
			*dest = *src;
			dest++;
			src++;
		}
	}

	// make everything relative to first frame of this subsegment
   if(root_flag && !exporting_deformable)
	{
		assert(frame_size == 7 * sizeof(float));
		float *start = (float*)(sub_channel->channel.data);

		QT qt;
		qt.v.w = *(start+3);
		qt.v.x = *(start+4);
		qt.v.y = *(start+5);
		qt.v.z = *(start+6);

		Matrix m;
		qt_to_mat(&m, &qt);

		Transform t(m, Vector(*(start), *(start+1), *(start+2)));
		t = t.get_inverse();

		for(int fr_id=0; fr_id < n_frames; fr_id++)
		{
			float *current = (float*)(sub_channel->channel.data + fr_id * frame_size);

			QT qt;
			qt.v.w = *(current+3);
			qt.v.x = *(current+4);
			qt.v.y = *(current+5);
			qt.v.z = *(current+6);

			Matrix cm;
			qt_to_mat(&cm, &qt);

			Transform ct(cm, Vector(*(current), *(current+1), *(current+2)));

			Transform l_t(false);
			Transform_mul(l_t, t, ct);
			//Transform_mul(l_t, ct, t);

			QT qt2;
			mat_to_qt(&qt2, &l_t);

			*(current  ) = l_t.translation.x;
			*(current+1) = l_t.translation.y;
			*(current+2) = l_t.translation.z;

			*(current+3) = qt2.v.w;
			*(current+4) = qt2.v.x;
			*(current+5) = qt2.v.y;
			*(current+6) = qt2.v.z;

			if(fr_id == 0)
			{
				assert( fabs(*(current  )) < .0001);
				assert( fabs(*(current+1)) < .0001);
				assert( fabs(*(current+2)) < .0001);
				assert( fabs(1.0 - *(current+3)) < .0001);
				assert( fabs(*(current+4)) < .0001);
				assert( fabs(*(current+5)) < .0001);
				assert( fabs(*(current+6)) < .0001);
			}	
		}
	}
}

/*void RemoveUnusedTextures(CompoundObject *c_obj)
{
	for(int t_id=0; t_id < c_obj->tl.count; t_id++)
	{
		const char *t_name = c_obj->tl.textures[t_id]->name;

		if( !IsUsedTexture(c_obj, t_name) )
		{
			RemoveTexture(&(c_obj->tl), t_id); // c_obj->n_channel_count--;
			if(t_id < c_obj->tl.count) // remove channel swaps this w/ last
			{
				t_id--;
			}
		}
	}
}*/

/*
bool IsUsedTexture(const CompoundObject *c_obj, const char *name)
{
	Winprint("Error: IsUsedTexture is out of date!\n");

	for(int i=0; i<c_obj->part_count; i++)
	{
		const lod_object *l_obj = &(c_obj->lod_object_list[i]);

		for(int lod_id=0; lod_id < l_obj->count; lod_id++)
		{

			const object *obj = &(l_obj->obj_list[lod_id]);

			for(int m_id=0; m_id < obj->ml->count; m_id++)
			{
				if( obj->ml->materials[m_id]->diffuse.texture_name &&
					!strcmp(obj->ml->materials[m_id]->diffuse.texture_name, name) )
				{
					return true;
				}
			}
		}
	}

	return false;
}*/

/*
void RemoveTexture(txt_lib *tl, int id)
{
	FreeTxt(tl->textures[id]);
	Free(tl->textures[id]);

	tl->textures[id] = tl->textures[tl->count - 1];

	tl->count--;
	tl->textures = (txt**)Realloc(tl->textures, tl->count * sizeof(txt*));	
}
*/

void RemoveChannel(CompoundObject *c_obj, int i)
{
  // remove any script that used that channel only
  char *ch_name=c_obj->n_channel_list[i].name; 

  for(int j=0; j<c_obj->script_count; j++)
  {
    if(c_obj->script_list[j].channel_count == 1)
    {
      if(!strcmp(c_obj->script_list[j].channel_list[0].channel, ch_name))
      {
        SwapScripts(c_obj, j, c_obj->script_count - 1);
        c_obj->script_count--;
        Free(c_obj->script_list[c_obj->script_count].channel_list);
        c_obj->script_list = (NamedScript*)Realloc(c_obj->script_list,
          c_obj->script_count * sizeof(NamedScript));
      }
    }
  }

  // remove channel
  SwapChannels(c_obj, i, c_obj->n_channel_count - 1);
  c_obj->n_channel_count--;
  Free(c_obj->n_channel_list[c_obj->n_channel_count].channel.data);
  c_obj->n_channel_list = (NamedChannel*)Realloc(c_obj->n_channel_list,
    c_obj->n_channel_count * sizeof(NamedChannel));
}

void SwapScripts(CompoundObject *c_obj, int id1, int id2)
{
  if(id1 == id2) return;

  MemSwap(c_obj->script_list+id1,
          c_obj->script_list+id2,
          sizeof(NamedScript));
}

void SwapChannels(CompoundObject *c_obj, int id1, int id2)
{
  if(id1 == id2) return;

  MemSwap(c_obj->n_channel_list+id1,
          c_obj->n_channel_list+id2,
          sizeof(NamedChannel));
}

void ExtractKeyFrames(CompoundObject *c_obj)
{
  int i;
  int type;

  for(i=0; i<c_obj->n_channel_count; i++)
  {
    type=GetChannelType(c_obj, i);
    switch(type)
    {
    case REVOLUTE:
      KeyFrameChannel(&(c_obj->n_channel_list[i].channel), REVOLUTE);
      break;
    case PRISMATIC:
      KeyFrameChannel(&(c_obj->n_channel_list[i].channel), PRISMATIC);
      break;
    case SPHERICAL:
      KeyFrameChannel(&(c_obj->n_channel_list[i].channel), SPHERICAL);
      break;
    case TRANSLATIONAL:
      KeyFrameChannel(&(c_obj->n_channel_list[i].channel), TRANSLATIONAL);
      break;
    case LOOSE:
      KeyFrameChannel(&(c_obj->n_channel_list[i].channel), LOOSE);
      break;
    case EVENT:
      break;
    default:
      Winprint("Error: unknown channel type for %s.\n",
        c_obj->n_channel_list[i].name);
    }
  }
}

bool IsLinear1D(const float a1, const float a2, const float a3, const float fraction,
				const float tolerance)
{
	assert(fraction >= 0.0f && fraction <= 1.0f);

	return ( (float)fabs((a1*fraction + a2*(1.0f - fraction) - a3)) < tolerance );
}

bool IsLinear3D(const float * const v1, const float * const v2, const float * const v3,
				const float fraction, const float tolerance)
{
	assert(fraction >= 0.0f && fraction <= 1.0f);

	float v4[3] = {
		v1[0] * fraction + v2[0] * (1.0f-fraction),
		v1[1] * fraction + v2[1] * (1.0f-fraction),
		v1[2] * fraction + v2[2] * (1.0f-fraction)};

	float dx = v3[0] - v4[0];
	float dy = v3[1] - v4[1];
	float dz = v3[2] - v4[2];

	return ( dx*dx + dy*dy + dz*dz < tolerance*tolerance);
}

bool IsLinearQuat(const PersistQuaternion * const q1, const PersistQuaternion * const q2,
				  const PersistQuaternion * const q3, const float fraction, const float tolerance)
{
	PersistQuaternion q4;

	quat_slerp(q4, *q1, *q2, fraction);

	float rot_angle, axis_angle;
	float qt_angle = GetQuatDiffAngles(q4, *q3, &rot_angle, &axis_angle);

	return (qt_angle < tolerance);
	//return (rot_angle < tolerance && axis_angle < tolerance);
}

void KeyFrameChannel(Channel *channel, int type)
{
  if(channel->header.frames <= 2)
    return;

  switch(type)
  {
    case REVOLUTE:
    case PRISMATIC:
	{
		float tolerance;
		if(type == REVOLUTE)
		{
			tolerance = 1.0f * (float)D2R;
		}
		else
		{
			tolerance = 0.0001f;
		}

		int n_keyframes = 1;

		float *data_pt = (float*)channel->data;
		float *key_buffer = (float*)Malloc(channel->header.frames * sizeof(float));
		float *time_buffer = (float*)Malloc(channel->header.frames * sizeof(float));
  
		int lk_id = 0;
		key_buffer[0] = data_pt[0];
		time_buffer[0] = 0.0f;

		for(int i=0; i < channel->header.frames-2; i++)
		{
		  float delta = (float)fabs(data_pt[lk_id] - data_pt[i+1]);
		  float fraction = (float)(i - lk_id + 1) / (float)(i - lk_id + 2);

		  if( !IsLinear1D(data_pt[lk_id], data_pt[i+2], data_pt[i+1], fraction, tolerance) ||
			  (type == REVOLUTE && delta >= M_PI))
		  {
			  lk_id = i+1;
			  key_buffer[n_keyframes] = data_pt[lk_id];
			  time_buffer[n_keyframes] = (lk_id) * channel->header.capture_rate;
			  n_keyframes++;
		  }
		}  

		// last frame is always a key frame
		key_buffer[n_keyframes] = data_pt[channel->header.frames-1];
		time_buffer[n_keyframes] = (channel->header.frames-1) *
								    channel->header.capture_rate;
		n_keyframes++;
  
		if(2*n_keyframes < channel->header.frames)
		{
		  if(verbose_level>=2)
		  {
			for(i=0; i < n_keyframes; i++) // debug only
			{
			  printf("frame#=%d time=%.2f data=%.2f\n",
					 i, time_buffer[i], key_buffer[i]/D2R);
			}
		  }
		  channel->header.capture_rate=-1;
		  channel->header.frames=n_keyframes;
  
		  channel->data=(unsigned char*)Realloc(channel->data,
						2*n_keyframes*sizeof(float)); 
		  data_pt=(float*)channel->data;
		  for(i=0;i<n_keyframes;i++)
		  {
			data_pt[2*i]=time_buffer[i];
			data_pt[2*i+1]=key_buffer[i];
		  }
		}
		Free(key_buffer);
		Free(time_buffer);
		break;
	}
	case SPHERICAL:
	{
		int n_keyframes = 1;
		
		PersistQuaternion *data_pt = (PersistQuaternion*)channel->data;
		PersistQuaternion *key_buffer = (PersistQuaternion*)Malloc(channel->header.frames*
				   sizeof(PersistQuaternion));
		float *time_buffer = (float*)Malloc(channel->header.frames*sizeof(float));
  
		int lk_id = 0;
		key_buffer[0] = data_pt[0];
		time_buffer[0] = 0.0f;

		float tolerance = .01f * (float)D2R;
		for(int i=0; i < channel->header.frames-2; i++)
		{
			float fraction = (float)(i - lk_id + 1) / (float)(i - lk_id + 2);

			float lk_rot_angle, lk_axis_angle;
			if( !IsLinearQuat(&(data_pt[lk_id]), &(data_pt[i+2]), &(data_pt[i+1]), fraction, tolerance) ||
				( GetQuatDiffAngles(data_pt[lk_id], data_pt[i+1],
										&lk_rot_angle, &lk_axis_angle) > 90.0f * D2R) )
			{
				lk_id = i+1;
				key_buffer[n_keyframes] = data_pt[lk_id];
				time_buffer[n_keyframes] = (lk_id) * channel->header.capture_rate;
				n_keyframes++;
			}

			
		  /*
		  quat_slerp(inter_quat, key_buffer[n_keyframes-1], data_pt[i+2], 
					 (float)(i - lk_id + 1)/(float)(i - lk_id + 2));
 
		  float rot_angle, axis_angle;
		  float qt_angle = GetQuatDiffAngles(inter_quat, data_pt[i+1], &rot_angle,
									 &axis_angle);
		  // lk (last key)
		  float lk_rot_angle, lk_axis_angle;
		  float lk_qt_angle = GetQuatDiffAngles(key_buffer[n_keyframes-1], data_pt[i+1],
										&lk_rot_angle, &lk_axis_angle);
      
		  qt_angle = (float)fabs(qt_angle)*R2D;
		  rot_angle = (float)fabs(rot_angle)*R2D;
		  axis_angle = (float)fabs(axis_angle)*R2D;
		  lk_qt_angle = (float)fabs(lk_qt_angle)*R2D;
		  lk_rot_angle = (float)fabs(lk_rot_angle)*R2D;
		  lk_axis_angle = (float)fabs(lk_axis_angle)*R2D;

		  // we only need to check 4D angle (not distance) since all
		  // quats are normalized

		  // 0.01 degree (why does this have to be so tight to work ??)
		  if((qt_angle >= .01f) || (lk_qt_angle >= 90.0f ) )
		  {  
		  //if((rot_angle >= 1.0) || (axis_angle >= 1.0) || 
				 //(lk_qt_angle >= 90.0) || (lk_axis_angle >= 90.0)){
			  // 4D steps larger than 90 deg are not allowed
		  // printf("qt=%f rot=%f axis=%f\n",qt_angle, rot_angle, axis_angle);
			key_buffer[n_keyframes] = data_pt[i+1];
			time_buffer[n_keyframes] = (i+1) * channel->header.capture_rate;
			n_keyframes++;
			lk_id = i+1;
		  }
		  */
		}  

		// last frame is always a key frame
		key_buffer[n_keyframes] = data_pt[channel->header.frames-1];
		time_buffer[n_keyframes] = (channel->header.frames-1) *
								   channel->header.capture_rate;
		n_keyframes++;
  
		// see if we have less data
		if(5*n_keyframes < 4*channel->header.frames)
		{
		  channel->header.capture_rate = -1;
		  channel->header.frames = n_keyframes;
  
		  channel->data = (unsigned char*)Realloc(channel->data,
						  n_keyframes * (sizeof(PersistQuaternion) + sizeof(float))); 

		  for(i=0; i < n_keyframes; i++)
		  {
			*(float*)(channel->data+i*4*5) = time_buffer[i];
			*(PersistQuaternion*)(channel->data+i*4*5+4) = key_buffer[i];
		  }
		}
		Free(key_buffer);
		Free(time_buffer);
		break;
	}
	case TRANSLATIONAL:
	{
      	int n_keyframes = 1;
		
		PersistVector *data_pt = (PersistVector*)channel->data;
		PersistVector *key_buffer = (PersistVector*)Malloc(channel->header.frames*
				   sizeof(PersistVector));
		float *time_buffer = (float*)Malloc(channel->header.frames*sizeof(float));
  
		int lk_id = 0;
		key_buffer[0] = data_pt[0];
		time_buffer[0] = 0.0f;

		float tolerance = 0.0001f;
		for(int i=0; i < channel->header.frames-2; i++)
		{
			float fraction = (float)(i - lk_id + 1) / (float)(i - lk_id + 2);

			if( !IsLinear3D((float*)&(data_pt[lk_id]), (float*)&(data_pt[i+2]),
							(float*)&(data_pt[i+1]), fraction, tolerance) )
			{
				lk_id = i+1;
				key_buffer[n_keyframes] = data_pt[lk_id];
				time_buffer[n_keyframes] = (lk_id) * channel->header.capture_rate;
				n_keyframes++;
			}
		}  

		// last frame is always a key frame
		key_buffer[n_keyframes] = data_pt[channel->header.frames-1];
		time_buffer[n_keyframes] = (channel->header.frames-1) *
								   channel->header.capture_rate;
		n_keyframes++;
  
		// see if we have less data
		if(4*n_keyframes < 3*channel->header.frames)
		{
		  channel->header.capture_rate = -1;
		  channel->header.frames = n_keyframes;
  
		  channel->data = (unsigned char*)Realloc(channel->data,
						  n_keyframes * (sizeof(PersistVector) + sizeof(float))); 

		  for(i=0; i < n_keyframes; i++)
		  {
			*(float*)(channel->data + i*4*4) = time_buffer[i];
			*(PersistVector*)(channel->data + i*4*4 + 4) = key_buffer[i];
		  }
		}
		Free(key_buffer);
		Free(time_buffer);
		break;
	}
	case LOOSE:
	{
		struct VQ {
			PersistVector v;
			PersistQuaternion q;
		};

		int n_keyframes = 1;
		
		VQ *data_pt = (VQ*)channel->data;
		VQ *key_buffer = (VQ*)Malloc(channel->header.frames *
				   (sizeof(PersistQuaternion) + sizeof(PersistVector)));
		float *time_buffer = (float*)Malloc(channel->header.frames*sizeof(float));
  
		int lk_id = 0;
		key_buffer[0] = data_pt[0];
		time_buffer[0] = 0.0f;

		float q_tolerance = 0.01f * (float)D2R;
		float v_tolerance = 0.0001f;
		for(int i=0; i < channel->header.frames-2; i++)
		{
			float fraction = (float)(i - lk_id + 1) / (float)(i - lk_id + 2);

			float lk_rot_angle, lk_axis_angle;
			if( !IsLinearQuat(&(data_pt[lk_id].q), &(data_pt[i+2].q), &(data_pt[i+1].q),
								fraction, q_tolerance) ||
				( GetQuatDiffAngles(data_pt[lk_id].q, data_pt[i+1].q,
										&lk_rot_angle, &lk_axis_angle) > 90.0f * D2R) ||
				!IsLinear3D((float*)&(data_pt[lk_id].v), (float*)&(data_pt[i+2].v),
							(float*)&(data_pt[i+1].v), fraction, v_tolerance) )
			{
				lk_id = i+1;
				key_buffer[n_keyframes] = data_pt[lk_id];
				time_buffer[n_keyframes] = (lk_id) * channel->header.capture_rate;
				n_keyframes++;
			}
		}  

		// last frame is always a key frame
		key_buffer[n_keyframes] = data_pt[channel->header.frames-1];
		time_buffer[n_keyframes] = (channel->header.frames-1) *
								   channel->header.capture_rate;
		n_keyframes++;
  
		// see if we have less data
		if(8*n_keyframes < 7*channel->header.frames)
		{
		  channel->header.capture_rate = -1;
		  channel->header.frames = n_keyframes;
  
		  channel->data = (unsigned char*)Realloc(channel->data,
						  n_keyframes * (sizeof(VQ) + sizeof(float))); 

		  for(i=0; i < n_keyframes; i++)
		  {
			*(float*)(channel->data + i*4*8) = time_buffer[i];
			*(VQ*)(channel->data + i*4*8 + sizeof(float)) = key_buffer[i];
		  }
		}
		Free(key_buffer);
		Free(time_buffer);
		break;
	}
	default:
	{
		Winprint("Error: unknown channel type!\n");
		exit(1);
	}
  }
}

// only works if we start w/ 1 channel per script    
void CombineChannels(CompoundObject *c_obj, int force_flag)
{
  int old_sc_num, new_sc_num;
  int ch_num;
  char prefix1[PersistAnimCHANNEL_NAME_MAX];
  char prefix2[PersistAnimCHANNEL_NAME_MAX];
  NamedScript *scr_list;
  int new_scr_count;
  int combine_flag;
 
  scr_list=NULL;
  new_scr_count=0;
  for(old_sc_num=0; old_sc_num<c_obj->script_count; old_sc_num++)
  {

    strcpy(prefix1, c_obj->script_list[old_sc_num].channel_list[0].channel);
#ifdef SGI
    StripPostfix(prefix1);
#else
	strtok(prefix1, "_");
	strcpy(prefix1, strtok(NULL, "_"));
#endif
    
    combine_flag=0;
    for(new_sc_num=0; 
        (new_sc_num < new_scr_count) && (!combine_flag); 
        new_sc_num++)
    {
      for(ch_num=0; 
          (ch_num<scr_list[new_sc_num].channel_count) && (!combine_flag); 
          ch_num++)
      {

        strcpy(prefix2, scr_list[new_sc_num].channel_list[ch_num].channel);
#ifdef SGI
        StripPostfix(prefix2);
#else
		strtok(prefix2, "_");
		strcpy(prefix2, strtok(NULL, "_"));
#endif

        if( (!strcmp(prefix1, prefix2)) || (force_flag) )
        {
          scr_list[new_sc_num].channel_count++;
          scr_list[new_sc_num].channel_list = (PersistAnimChannelMapping*)
                                   Realloc(scr_list[new_sc_num].channel_list,
                                   scr_list[new_sc_num].channel_count*
                                   sizeof(PersistAnimChannelMapping)); 
          scr_list[new_sc_num].channel_list[scr_list[new_sc_num].channel_count-1] =
            c_obj->script_list[old_sc_num].channel_list[0];
          Free(c_obj->script_list[old_sc_num].channel_list);
          combine_flag=1;
        }
      }
    }

    if( !combine_flag )// append
    {
      new_scr_count++;
      scr_list=(NamedScript*)Realloc(scr_list, new_scr_count*sizeof(NamedScript));
      InitScript(&scr_list[new_scr_count-1]);
      scr_list[new_scr_count-1] = c_obj->script_list[old_sc_num];
    }

  }

  Free(c_obj->script_list);
  c_obj->script_list=scr_list;
  c_obj->script_count=new_scr_count;
}

void SynchronizeScriptTiming(CompoundObject *c_obj)
{
  int sc_num, ch_num;
  char *ch_name;
  int channels;
  int *ch_id_list;

  ch_id_list=NULL;
  for(sc_num=0; sc_num<c_obj->script_count; sc_num++){
    channels=c_obj->script_list[sc_num].channel_count;

    // select scripts w/ more than 1 channel
    if(channels>1){
      ch_id_list=(int*)Realloc(ch_id_list, channels*sizeof(int));

      // create a list of channels to sequence
      for(ch_num=0; ch_num<channels; ch_num++){
        ch_name=c_obj->script_list[sc_num].channel_list[ch_num].channel;
        ch_id_list[ch_num]=GetChId(c_obj, ch_name);
        // printf("passing to SequenceAnim() sc_id=%d ch_id=%d name=%s\n",sc_num, ch_id_list[ch_num], ch_name);
      }

      SequenceAnim(c_obj, ch_id_list, channels);
    }

  }
  Free(ch_id_list);
}

void SequenceAnim(CompoundObject *c_obj, int *ch_id_list, int n_channels)
{
  int i, j;
  int tmp;
  int frame;
  int first_frame;

  // sanity check
  if(n_channels > c_obj->n_channel_count){
    Winprint("Error: bad channel count.\n");
    exit(1);
  }

  // sort by first frame number // bubble sort
  for(j=0; j<n_channels-1; j++){
    for(i=0; i<n_channels-(j+1); i++){
      if(c_obj->n_channel_list[ch_id_list[i]].first_frame
         > c_obj->n_channel_list[ch_id_list[i+1]].first_frame){
        tmp=ch_id_list[i];
        ch_id_list[i]=ch_id_list[i+1];
        ch_id_list[i+1]=tmp;
      }
    }
  }

  if(verbose_level>=3){
    for(i=0; i<n_channels; i++){
      printf("listid=%d chid=%d ff=%d lf=%d\n",i,ch_id_list[i],
        c_obj->n_channel_list[ch_id_list[i]].first_frame,
        c_obj->n_channel_list[ch_id_list[i]].last_frame);
    }
  }

  Channel *ch;
  int frame_delta;
  int type;

  first_frame=c_obj->n_channel_list[ch_id_list[0]].first_frame;
  for(i=1; i<n_channels; i++){

    frame_delta=c_obj->n_channel_list[ch_id_list[i]].first_frame - first_frame;
    if(frame_delta>0){
      ch=&(c_obj->n_channel_list[ch_id_list[i]].channel);
      type=GetChannelType(c_obj, ch_id_list[i]);

      if(ch->header.capture_rate<0.0){ // key frames
       fprintf(stderr,"Error: trying to sequence key frame animation.");
       fprintf(stderr," Not supported.\n");
      }
      else{ // regular frames
         if(type==SPHERICAL){
           PersistQuaternion *data;
           PersistQuaternion pq;
           pq.w=1.0f;
           pq.x=pq.y=pq.z=0.0f;

           data=(PersistQuaternion*)Malloc((frame_delta+ch->header.frames)
                *sizeof(PersistQuaternion)); 
           for(frame=0; frame<frame_delta; frame++){
             data[frame]=pq; // insert dead frames
           }
           memcpy(data+frame_delta, ch->data,
                   ch->header.frames*sizeof(PersistQuaternion));
           ch->header.frames+=frame_delta;
           Free(ch->data);
           ch->data=(unsigned char*)data;
         }else
         if((type==REVOLUTE) || (type==PRISMATIC)) {
           float *data;
           data=(float*)Malloc((frame_delta+ch->header.frames)*sizeof(float)); 
           for(frame=0; frame<frame_delta; frame++){
             // data[frame]=*(float*)ch->data; // repeat 1st frame delta times
             data[frame]=0.0f; // insert dead frames
           }
           memcpy(data+frame_delta, ch->data,
                   ch->header.frames*sizeof(float));
           ch->header.frames+=frame_delta;
           Free(ch->data);
           ch->data=(unsigned char*)data;
         }
         else{
           Winprint("Error: unknown channel type.\n");
           exit(1);
         }
      }
    }

  }
}

/*
mtl* GetMtlIDbyTXT(const void *cmp_ptr, const char *txt_name)
{
  if(!txt_name || !cmp_ptr)
    return NULL;

  const CompoundObject *c_obj = (CompoundObject*)cmp_ptr;

  for(int i=0; i < c_obj->part_count ; i++)
  {
	  lod_object *lobj = c_obj->lod_object_list + i;

	  for(int j=0; j < lobj->count; j++)
	  {
		object *obj = lobj->obj_list + j;

		mtl *result = GetMtlIDbyTXT(&(obj->ml), txt_name);
		if(result) return result;
	 }
  }

  return NULL;
}
*/

int GetChId(CompoundObject *c_obj, char *ch_name)
{
  int i;

  for(i=0; i<c_obj->n_channel_count; i++){
    if(!strcmp(c_obj->n_channel_list[i].name, ch_name)){
      return i; 
    }
  }
  return -1;
}


void GetPrefix2(char *out, const char *in)
{
  char *start;

  strcpy(out, in);
  strtok(out, "_");
  start=strtok(NULL, "_");
  strcpy(out, start);
}

void GetPostfix1(char *out, const char *in)
{
  char *start;

  strcpy(out, in);
  strtok(out, "_");
  start=strtok(NULL, "_");
  strcpy(out, in);
  strcpy(out, start);
}

void StripPostfix(char *buffer)
{
    int length = strlen(buffer);
    int id = length - 1;

    while(buffer[id] != '_')
    {
        buffer[id] = 0;
        id--;
        assert(id >= 0);
    }
    buffer[id] = 0;
}

void InsertSphere(CompoundObject *c_obj, const PersistSphere& sphere)
{
    c_obj->sphere_count++;
    c_obj->sphere_list=(PersistSphere*)Realloc(c_obj->sphere_list, 
                                     c_obj->sphere_count*sizeof(PersistSphere));
    c_obj->sphere_list[c_obj->sphere_count-1]=sphere;
}

void InsertPris(CompoundObject *c_obj, const Pris& prismatic)
{
    c_obj->pris_count++;
    c_obj->pris_list=(Pris*)Realloc(c_obj->pris_list, 
                                        c_obj->pris_count*sizeof(Pris));
    c_obj->pris_list[c_obj->pris_count-1]=prismatic;
}

void InsertTrans(CompoundObject *c_obj, const Trans& trans)
{
    c_obj->trans_count++;
    c_obj->trans_list=(Trans*)Realloc(c_obj->trans_list, 
                                        c_obj->trans_count*sizeof(Trans));
    c_obj->trans_list[c_obj->trans_count-1]=trans;
}

void InsertRev(CompoundObject *c_obj, const Rev& revolute)
{
    c_obj->rev_count++;
    c_obj->rev_list=(Rev*)Realloc(c_obj->rev_list, c_obj->rev_count*sizeof(Rev));
    c_obj->rev_list[c_obj->rev_count-1]=revolute;
}

void InsertFixed(CompoundObject *c_obj, const Fix& fixed)
{
    c_obj->fix_count++;
    c_obj->fix_list=(Fix*)Realloc(c_obj->fix_list, c_obj->fix_count*sizeof(Fix));
    c_obj->fix_list[c_obj->fix_count-1]=fixed;
}

void InsertLoose(CompoundObject *c_obj, const Loose& loose)
{
    c_obj->loose_count++;
    c_obj->loose_list=(Loose*)Realloc(c_obj->loose_list, c_obj->loose_count*sizeof(Loose));
    c_obj->loose_list[c_obj->loose_count-1]=loose;
}

int InsertCompoundName(CompoundObject *c_obj, const char *name, 
		const char *f_name, const char *root_name, 
		const char *extension, int obj_flag, PersistTransform *pt, 
		const bool export)
{
  NamedPart new_part;
  InitNamedPart(&new_part);
  new_part.export = export;

  int i;
  char object_name[256] = {0};
  char file_name[256] = {0};

  strcpy(object_name, name);
  strcpy(file_name, f_name);

  if(extension && (strlen(extension) > 0))
  {
    strcat(file_name, extension);
  }

#ifdef GAMEGEN
    strtok(object_name, "#");
    if(strlen(object_name) < strlen(name)){
      StripPrefix(file_name, object_name);
      StripPrefix(file_name, "#");
    }
#endif

  
  CheckNameLenght(object_name);

  // check for duplicate name
  for(i=0; i<c_obj->part_count; i++)
  {
    char tmp_name[256] = {0};
    if(i==0)
    {
        strcpy(tmp_name, root_name);
    }
    else
    {
        strcpy(tmp_name, c_obj->part_list[i].obj_name);
    }

    if(strlen(object_name) != strlen(tmp_name))
    {
        continue;
    }
    else
    {
        int flag = 1;
        int delta = 'A' - 'a';
        for(int l=0; l<strlen(object_name); l++)
        {
            if( (object_name[l] != tmp_name[l]) &&
                (object_name[l]+delta != tmp_name[l]) &&
                (object_name[l]-delta != tmp_name[l]) )
            {

                flag=0;
                break;
            }
        }
        if(flag)
        {
              Winprint("Error: object %s is in database twice!\n"
                       "Possibly due to duplicate names or names longer than %d characters\n"
                       "Note: names are case INSENSITIVE. i.e.,(Box & box are duplicate names)",
                       object_name, PARTNAME_MAX-1);
              exit(1);
        }
    }
  }

  new_part.index=c_obj->part_count;

  // create part
  if(!strcmp(object_name, root_name)){
    strcpy(new_part.obj_name, ROOT_OBJ_NAME);
  }
  else{
    strcpy(new_part.obj_name, object_name);
  }
  strcpy(new_part.file_name, file_name);

  // add part
  c_obj->part_count++;
  c_obj->part_list=(NamedPart*)Realloc(c_obj->part_list,
                   c_obj->part_count*sizeof(NamedPart));
  c_obj->part_list[c_obj->part_count-1]=new_part;


  if(obj_flag >= 1)
  {
	  c_obj->lod_object_list = (lod_object*)Realloc(c_obj->lod_object_list,
		  c_obj->part_count * sizeof(lod_object));
	  InitLodObject(&(c_obj->lod_object_list[c_obj->part_count - 1]), /*&(c_obj->tl), &(c_obj->atl), */&(c_obj->ml));
	  strcpy(c_obj->lod_object_list[c_obj->part_count - 1].file_name, file_name);
	  //c_obj->lod_object_list[i].export_flag // default = 1

	  if(obj_flag >= 2)
	  {
		c_obj->lod_object_list[c_obj->part_count - 1].count = 1;
		c_obj->lod_object_list[c_obj->part_count - 1].obj_list = (object*)Malloc(sizeof(object));
		InitObject(c_obj->lod_object_list[c_obj->part_count - 1].obj_list,
			/*&(c_obj->tl), &(c_obj->atl),*/ &(c_obj->ml));
	  }	
  }

  c_obj->transform_list = (PersistTransform*)Realloc(c_obj->transform_list,
	  c_obj->part_count * sizeof(PersistTransform));

  if(pt)
  {
	  c_obj->transform_list[c_obj->part_count-1] = *pt;
  }
  else
  {
	  c_obj->transform_list[c_obj->part_count-1].identity();
  }

  return(new_part.index);
}
  
void InsertNamedChannel(CompoundObject *c_obj, const NamedChannel& n_channel)
{
    c_obj->n_channel_count++;
    c_obj->n_channel_list=(NamedChannel*)Realloc(c_obj->n_channel_list,
                          c_obj->n_channel_count*sizeof(NamedChannel));
    c_obj->n_channel_list[c_obj->n_channel_count-1]=n_channel;
}

void InsertScript(CompoundObject *c_obj, const NamedScript& script)
{
    c_obj->script_count++;
    c_obj->script_list=(NamedScript*)Realloc(c_obj->script_list,
                       c_obj->script_count*sizeof(NamedScript));
    c_obj->script_list[c_obj->script_count-1]=script;
}

/*
void WordAllignChannel(NamedChannel *n_channel)
{
    if(n_channel->data_size % 4)
    {
        int old_size = n_channel->data_size;
        int pad = (4 - n_channel->data_size%4);
        n_channel->data_size += pad;
    
        n_channel->channel.data = (unsigned char*)Realloc(n_channel->channel.data,
            n_channel->data_size);

        for(int i=0; i<pad; i++)
        {
            n_channel->channel.data[old_size+i] = 0;
        }
    }
}
*/

void InsertChannelEvent(NamedChannel & n_channel, float time,
channel_event type, const char *in_name)
{
  char *buf_name=NULL;
  int buf_length = 0;
  if(in_name)
  {
    buf_length = (strlen(in_name) + 1) * sizeof(char);
    int pad = 0;
    if(buf_length%4)
    {
      pad = (4 - buf_length%4);
      buf_length += pad;
    }

    buf_name=(char*)Malloc(buf_length);
    strcpy(buf_name, in_name);
    for(int j=0; j<pad; j++)
    {
      buf_name[(buf_length-1) - j] = 0;
    }
#ifdef SGI
    SwapByteOrder((unsigned char*)buf_name, (unsigned char*)buf_name,
                   buf_length, in_name);
#endif
  }

  // first frame
  if(n_channel.channel.header.frames == 0)
  {
    n_channel.channel.header.frames = 1;
    if(buf_name)
    {
      n_channel.data_size = sizeof(float) + sizeof(int) + sizeof(int) +
                            buf_length;
    }
    else
    {
                            // start time   // offset    // event type
      n_channel.data_size = sizeof(float) + sizeof(int) + sizeof(int);
    }

    n_channel.channel.data = (unsigned char*)Malloc( n_channel.data_size );
    memset(n_channel.channel.data, 0, n_channel.data_size);

    unsigned char *pt = n_channel.channel.data;
    *(float*)pt = time;
    pt += 4;

    // offset
    *(int*)pt = 0;
    pt += 4;

    *(int*)pt = type;
    pt += 4;

    if(buf_name)
    {
      memcpy(pt, buf_name, buf_length);
      pt += buf_length;
    }
  }
  else
  {
    int new_size =          // time         // offset     // type
      n_channel.data_size + sizeof(float) + sizeof(int) + sizeof(int);
    if(buf_name) new_size += buf_length;
    
    int insert_offset = 0;

    unsigned char* data =
      (unsigned char*)Malloc(new_size);
    memset(data, 0, new_size);


    assert(sizeof(float) == sizeof(int)); // mixing these two
    float *old_frames = (float*)n_channel.channel.data;
    float *new_frames = (float*)data;

    unsigned char *old_data = (n_channel.channel.data +
      n_channel.channel.header.frames * (sizeof(float) + sizeof(int)) );
    unsigned char *new_data = (data +
      (n_channel.channel.header.frames + 1) * (sizeof(float) + sizeof(int)) );
    
    bool inserted = false;
    int current_offset = 0;
    for(int i=0; i < n_channel.channel.header.frames; i++)
    {

      // find correct frame
      if(!inserted && old_frames[0] > time)
      {
        // insert
        new_frames[0] = time;
        ((int*)new_frames)[1] = ((int*)old_frames)[1]; 
        assert(((int*)new_frames)[1] == current_offset);

        *(int*)(new_data + ((int*)new_frames)[1]) = type;

        if(buf_name)
        {
          memcpy(new_data + ((int*)new_frames)[1] + sizeof(int),
                 buf_name, buf_length);
        }

        current_offset += sizeof(int) + buf_length;

                        // type
        insert_offset = sizeof(int);
        if(buf_name) insert_offset += buf_length;

        new_frames += 2;
        inserted = true;
      } 

      // copy old/existing frame
      new_frames[0] = old_frames[0];
      ((int*)new_frames)[1] = ((int*)old_frames)[1] + insert_offset;
      assert(((int*)new_frames)[1] == current_offset);

      // copy from current offset to next offset
      int size;
      if(i == n_channel.channel.header.frames - 1)
      {
        // to the end
        size = (n_channel.channel.data + n_channel.data_size) -
             (old_data + ((int*)old_frames)[1]);
      }
      else
      {
        // to next offset
        size = ((int*)old_frames)[3] - ((int*)old_frames)[1]; 
      }

      memcpy(new_data + ((int*)new_frames)[1],
             old_data + ((int*)old_frames)[1],
             size);
        
      current_offset += size;

      new_frames += 2;
      old_frames += 2;
    }


  // if last frame
  if(!inserted)
  {
    // insert
    new_frames[0] = time;
    ((int*)new_frames)[1] = new_size - (n_channel.channel.header.frames+1)*(sizeof(float)+sizeof(int)) -
        (sizeof(int) + buf_length); 
    assert(((int*)new_frames)[1] == current_offset);

    *(int*)(new_data + ((int*)new_frames)[1]) = type;

    if(buf_name)
    {
      memcpy(new_data + ((int*)new_frames)[1] + sizeof(int),
             buf_name, buf_length);
    }

                    // type
    insert_offset = sizeof(int);
    if(buf_name) insert_offset += buf_length;

    new_frames += 2;
    inserted = true;
  } 

    // copy data
    n_channel.data_size = new_size;
    Free(n_channel.channel.data);
    n_channel.channel.data = data;
    n_channel.channel.header.frames++;
  }

  Free(buf_name);

  assert(!(n_channel.data_size % 4));
}

QT QTMul(const QT& q1, const QT& q2)
{
  QT result;
  // q1q2=(w1*w2 - v1.v2, w1*v2 + w2*v1 + v1xv2);

  result.v.w = q1.v.w*q2.v.w - (q1.v.x*q2.v.x + q1.v.y*q2.v.y + q1.v.z*q2.v.z);

  result.v.x = q1.v.w*q2.v.x + q2.v.w*q1.v.x;
  result.v.y = q1.v.w*q2.v.y + q2.v.w*q1.v.y;
  result.v.z = q1.v.w*q2.v.z + q2.v.w*q1.v.z;

  float tmp[3];
  Cross(&(q1.d[1]), &(q2.d[1]), tmp);

  result.v.x+=tmp[0];
  result.v.y+=tmp[1];
  result.v.z+=tmp[2];

  return result;
}

void RotateCmpd(CompoundObject * /*c_obj*/, const Matrix & /*m*/)
{

#if 0 // done by GetLocalTM
  int i;

  // rotate root's immediate children
  // fixed joints
  for(i=0; i<c_obj->fix_count; i++){
    if(!strcmp(c_obj->fix_list[i].parent, ROOT_OBJ_NAME)){
      RotatePersistVector(&(c_obj->fix_list[i].pos), m);
      PreRotatePersistMatrix(&(c_obj->fix_list[i].orient), m);
    }
  }

  // pris joints
  for(i=0; i<c_obj->pris_count; i++){
    if(!strcmp(c_obj->pris_list[i].parent, ROOT_OBJ_NAME)){
      RotatePersistVector(&(c_obj->pris_list[i].parent_point), m);
      PreRotatePersistMatrix(&(c_obj->pris_list[i].rel_orientation), m);
    }
  }

  // rev joints
  for(i=0; i<c_obj->rev_count; i++){
    if(!strcmp(c_obj->rev_list[i].parent, ROOT_OBJ_NAME)){
      RotatePersistVector(&(c_obj->rev_list[i].parent_point), m);
      PreRotatePersistMatrix(&(c_obj->rev_list[i].rel_orientation), m);
    }
  }

  // spherical joints
  for(i=0; i<c_obj->sphere_count; i++){
    if(!strcmp(c_obj->sphere_list[i].parent, ROOT_OBJ_NAME)){
      RotatePersistVector(&(c_obj->sphere_list[i].parent_point), m);
      PreRotatePersistMatrix(&(c_obj->sphere_list[i].rel_orientation), m);
    }
  }

  // trans joints
  for(i=0; i<c_obj->trans_count; i++){
    if(!strcmp(c_obj->trans_list[i].parent, ROOT_OBJ_NAME)){
      RotatePersistVector(&(c_obj->trans_list[i].pos), m);
      PreRotatePersistMatrix(&(c_obj->trans_list[i].orient), m);
    }
  }

  // loose joints
  for(i=0; i<c_obj->loose_count; i++){
    if(!strcmp(c_obj->loose_list[i].parent, ROOT_OBJ_NAME)){
      RotatePersistVector(&(c_obj->loose_list[i].pos), m);
      PreRotatePersistMatrix(&(c_obj->loose_list[i].orient), m);
    }
  }
#endif

#if 0 // done by ReadRootFrames
  // rotate root frames (rotation already done)
  RotateLooseChannel(c_obj->n_channel_list[0].channel.data,
                    c_obj->n_channel_list[0].channel.header.frames, m);
#endif
 
#if 0 // done in ReadFramesChannel for now

  //Matrix m_inv = MatrixTranspose(m);
  Matrix m_inv = m;
  int j,k;

  // rotate children's channels
  for(i=0; i<c_obj->script_count; i++){
    for(j=0; j<c_obj->script_list[i].channel_count; j++){
      if(!strcmp(c_obj->script_list[i].channel_list[j].parent, ROOT_OBJ_NAME)){

        for(k=0; k<c_obj->n_channel_count; k++){
          if(!strcmp(c_obj->n_channel_list[k].name,
                     c_obj->script_list[i].channel_list[j].channel))
          {
            printf("%s\n",c_obj->script_list[i].channel_list[j].channel);
            if(c_obj->n_channel_list[k].channel.header.type == PersistDT_QUATERNION){

              RotateSphereChannel(c_obj->n_channel_list[k].channel.data,
                                 c_obj->n_channel_list[k].channel.header.frames, m_inv);

            }
            else
            if(c_obj->n_channel_list[k].channel.header.type == PersistDT_VECTOR){
              RotateTransChannel(c_obj->n_channel_list[k].channel.data,
                                 c_obj->n_channel_list[k].channel.header.frames, m_inv);
            }
            else{
              fprintf(stderr,"Warning: Not rotating %d type channel %s.\n",
                c_obj->n_channel_list[k].channel.header.type,
                c_obj->n_channel_list[k].name);
            }
          }
        }
      }
    }
  }
#endif

  // rotate root geometry
  //done by RotateObject
}

void PreRotatePersistMatrix(PersistMatrix *pm, const Matrix& m)
{
  PersistMatrix result;

  result.e00 = m.d[0][0] * pm->e00 + m.d[0][1] * pm->e10 + m.d[0][2] * pm->e20;
  result.e10 = m.d[1][0] * pm->e00 + m.d[1][1] * pm->e10 + m.d[1][2] * pm->e20;
  result.e20 = m.d[2][0] * pm->e00 + m.d[2][1] * pm->e10 + m.d[2][2] * pm->e20;

  result.e01 = m.d[0][0] * pm->e01 + m.d[0][1] * pm->e11 + m.d[0][2] * pm->e21;
  result.e11 = m.d[1][0] * pm->e01 + m.d[1][1] * pm->e11 + m.d[1][2] * pm->e21;
  result.e21 = m.d[2][0] * pm->e01 + m.d[2][1] * pm->e11 + m.d[2][2] * pm->e21;

  result.e02 = m.d[0][0] * pm->e02 + m.d[0][1] * pm->e12 + m.d[0][2] * pm->e22;
  result.e12 = m.d[1][0] * pm->e02 + m.d[1][1] * pm->e12 + m.d[1][2] * pm->e22;
  result.e22 = m.d[2][0] * pm->e02 + m.d[2][1] * pm->e12 + m.d[2][2] * pm->e22;

  *pm = result;
}

void RotatePersistVector( PersistVector *v, const Matrix& m)
{
  float x,y,z;

  x = v->x * m.d[0][0] + v->y * m.d[0][1] + v->z * m.d[0][2];
  y = v->x * m.d[1][0] + v->y * m.d[1][1] + v->z * m.d[1][2];
  z = v->x * m.d[2][0] + v->y * m.d[2][1] + v->z * m.d[2][2];

#if 0
  x = v->x * m.d[0][0] + v->y * m.d[1][0] + v->z * m.d[2][0];
  y = v->x * m.d[0][1] + v->y * m.d[1][1] + v->z * m.d[2][1];
  z = v->x * m.d[0][2] + v->y * m.d[1][2] + v->z * m.d[2][2];
#endif

  v->x = x;
  v->y = y;
  v->z = z;
}

// not used
void RotateTransChannel(unsigned char *data, int n_frames, const Matrix& m)
{
  int i;
  float *f_data;
  float x, y, z;

  f_data = (float*)data; 
  for(i=0; i<n_frames; i++){
    
    x = f_data[0] * m.d[0][0] + f_data[0] * m.d[1][0] + f_data[0] * m.d[2][0];
    y = f_data[1] * m.d[0][1] + f_data[1] * m.d[1][1] + f_data[1] * m.d[2][1];
    z = f_data[2] * m.d[0][2] + f_data[2] * m.d[1][2] + f_data[2] * m.d[2][2];

    f_data[0] = x;
    f_data[2] = y;
    f_data[3] = z;
  
    f_data += 3;
  }
}

// not used (quat to mat mat to quat bug ?? )
void RotateSphereChannel(unsigned char *data, int n_frames, const Matrix& m)
{
  int i;
  float *f_data;
  float x, y, z;
  float magnitude;
  Matrix m_inv;

  m_inv = MatrixTranspose(m);

  //DumpMatrix(m);
  //DumpMatrix(m_inv);
  //DumpMatrix(Matrix_mul_Matrix(m, m_inv));

  f_data = (float*)data; 
  for(i=0; i<n_frames; i++){
  
    printf("o %f %f %f %f\n",
      2.0*Acos(f_data[0])*R2D, f_data[1], f_data[2], f_data[3]);

    QT quat;
    Matrix src_mm;
    Matrix mm;
    Matrix tmp;
  
    // get frame matrix 
    qt_to_mat(&src_mm, (QT*)f_data);

    tmp = Matrix_mul_Matrix(src_mm, m); 
    mm = Matrix_mul_Matrix(tmp, m_inv); 

    mat_to_qt(&quat, &mm);

    printf("n %f %f %f %f\n",
      2.0*Acos(quat.v.w)*R2D, quat.v.x, quat.v.y, quat.v.z);

    //DumpMatrix(tmp);
    //printf("\n");
    DumpMatrix(src_mm);
    printf("\n");
    DumpMatrix(mm);
    printf("\n");

 
#if 0 
    x = f_data[1] * m.d[0][0] + f_data[1] * m.d[1][0] + f_data[1] * m.d[2][0];
    y = f_data[2] * m.d[0][1] + f_data[2] * m.d[1][1] + f_data[2] * m.d[2][1];
    z = f_data[3] * m.d[0][2] + f_data[3] * m.d[1][2] + f_data[3] * m.d[2][2];
#else
    x = f_data[1] * m.d[0][0] + f_data[1] * m.d[0][1] + f_data[1] * m.d[0][2];
    y = f_data[2] * m.d[1][0] + f_data[2] * m.d[1][1] + f_data[2] * m.d[1][2];
    z = f_data[3] * m.d[2][0] + f_data[3] * m.d[2][1] + f_data[3] * m.d[2][2];
#endif

    magnitude = (float)sqrt( f_data[0]*f_data[0] + x*x + y*y + z*z);
    if(magnitude>0.0f){
      magnitude = 1.0f/magnitude;
    }

    f_data[0] = f_data[0]*magnitude;
    f_data[1] = x*magnitude;
    f_data[2] = y*magnitude;
    f_data[3] = z*magnitude;
  
    f_data += 4;
  }
}

// not used
void RotateLooseChannel(unsigned char *data, int n_frames, const Matrix& m)
{
  int i;
  float *f_data;
  float x, y, z;

  f_data = (float*)data; 

  for(i=0; i<n_frames; i++){

    // trans 
#if 1 // correct
    x = f_data[0] * m.d[0][0] + f_data[1] * m.d[0][1] + f_data[2] * m.d[0][2];
    y = f_data[0] * m.d[1][0] + f_data[1] * m.d[1][1] + f_data[2] * m.d[1][2];
    z = f_data[0] * m.d[2][0] + f_data[1] * m.d[2][1] + f_data[2] * m.d[2][2];
#else
    x = f_data[0] * m.d[0][0] + f_data[1] * m.d[1][0] + f_data[2] * m.d[2][0];
    y = f_data[0] * m.d[0][1] + f_data[1] * m.d[1][1] + f_data[2] * m.d[2][1];
    z = f_data[0] * m.d[0][2] + f_data[1] * m.d[1][2] + f_data[2] * m.d[2][2];
#endif

    f_data[0] = x;
    f_data[1] = y;
    f_data[2] = z;

    // rot
    // already transformed by ReadRootFrames since we get all the nice
    // matrix operations for free
   
    f_data += 7;
  }
}

// used by MAX exporter
void RotateTransFrames(Frame *frame_list, int n_frames, const Fix& fixed)
{
  PersistVector tmp;
  
  for(int i=0; i < n_frames; i++)
  {
      tmp.x = frame_list[i].vector_t.x*fixed.orient.e00 +
              frame_list[i].vector_t.y*fixed.orient.e10 +
              frame_list[i].vector_t.z*fixed.orient.e20;
      tmp.y = frame_list[i].vector_t.x*fixed.orient.e01 +
              frame_list[i].vector_t.y*fixed.orient.e11 +
              frame_list[i].vector_t.z*fixed.orient.e21;
      tmp.z = frame_list[i].vector_t.x*fixed.orient.e02 +
              frame_list[i].vector_t.y*fixed.orient.e12 +
              frame_list[i].vector_t.z*fixed.orient.e22;
      frame_list[i].vector_t = tmp;
  }
}

void DumpFrames(Frame *frame_list, int n_frames)
{
  int i;

  for(i=0; i<n_frames; i++){
    printf("T step x,y,z  %.3f  %.3f %.3f %.3f\n",
      frame_list[i].step, frame_list[i].vector_t.x,
      frame_list[i].vector_t.y, frame_list[i].vector_t.z);
    printf("R angl x,y,z  %.3f  %.3f %.3f %.3f\n",
      R2D*frame_list[i].step, frame_list[i].vector_r.x,
      frame_list[i].vector_r.y, frame_list[i].vector_r.z);
  }
}

// subtract off frame 0
void NormalizeFrames(Frame *frame_list, int n_frames)
{
  int i;
  float step, angle;

  step=frame_list[0].step;
  angle=frame_list[0].angle;

  for(i=0; i<n_frames; i++)
  {
    frame_list[i].step-=step;
    frame_list[i].angle-=angle;  // WRONG!! assumes constant axis
  }

  SetQuats(frame_list, n_frames);
}

void SetQuats(Frame *frame_list, int n_frames)
{
  int i;

  for(i=0; i<n_frames; i++){
    float tmp; 

    frame_list[i].quat.v.w=(float)cos(frame_list[i].angle*.5);
    if(frame_list[i].quat.v.w <1.0){  // just for numerical accuracy
      tmp=(float)sin(frame_list[i].angle*.5);
      frame_list[i].quat.v.x=frame_list[i].vector_r.x*tmp;
      frame_list[i].quat.v.y=frame_list[i].vector_r.y*tmp;
      frame_list[i].quat.v.z=frame_list[i].vector_r.z*tmp;
    }
    else{
      frame_list[i].quat.v.x=
      frame_list[i].quat.v.y=
      frame_list[i].quat.v.z=0.0f;
    }
  }
}

int CleanFrames(Frame *frame_list, int n_frames)
{
  int i;
  float dot;

  int angle_flag = 0;
  // assign axes to frames w/ 0 degree rotation from next frame
  for(i=0; i<n_frames-1; i++){ // bottom to top
    // rotation
    if(frame_list[i+1].angle == 0.0){
      frame_list[i+1].vector_r.x = frame_list[i].vector_r.x;
      frame_list[i+1].vector_r.y = frame_list[i].vector_r.y;
      frame_list[i+1].vector_r.z = frame_list[i].vector_r.z;
    }

    if(frame_list[i+1].angle != 0.0f) 
    {
        angle_flag = 1;
    }

    // translation
    if(frame_list[i+1].step == 0.0){
      frame_list[i+1].vector_t.x = frame_list[i].vector_t.x;
      frame_list[i+1].vector_t.y = frame_list[i].vector_t.y;
      frame_list[i+1].vector_t.z = frame_list[i].vector_t.z;
    }
  }

  for(i=n_frames-2; i>=0; i--){ // top to bottom
    // rotation
    if(frame_list[i].angle==0.0){
      frame_list[i].vector_r.x=frame_list[i+1].vector_r.x;
      frame_list[i].vector_r.y=frame_list[i+1].vector_r.y;
      frame_list[i].vector_r.z=frame_list[i+1].vector_r.z;
    }

    if(frame_list[i].angle != 0.0f) 
    {
        angle_flag = 1;
    }

    // translation
    if(frame_list[i].step==0.0){
      frame_list[i].vector_t.x=frame_list[i+1].vector_t.x;
      frame_list[i].vector_t.y=frame_list[i+1].vector_t.y;
      frame_list[i].vector_t.z=frame_list[i+1].vector_t.z;
    }
  }

  // added for animations w/ only 1 frame (poses)
  if(frame_list[0].angle != 0.0f)
  {
      angle_flag = 1;
  }


  // if no frame has any angle we make axix the X axis
  if(!angle_flag)
  {
     for(i=0; i<n_frames; i++)
     {
        frame_list[i].vector_r.x = 1.0f;
        frame_list[i].vector_r.y =
        frame_list[i].vector_r.z = 0.0f;
     }
  }

  // normalize to -180/180
  for(i=0; i<n_frames; i++){
    frame_list[i].angle=NormAngle(frame_list[i].angle);
  }

  // check for axis flipping due to non unique quaternion definition
  for(i=0; i<n_frames-1; i++){
    // rotation
    dot=frame_list[i].vector_r.x*frame_list[i+1].vector_r.x+
        frame_list[i].vector_r.y*frame_list[i+1].vector_r.y+
        frame_list[i].vector_r.z*frame_list[i+1].vector_r.z;
    if(dot<0.0){
      frame_list[i+1].vector_r.x=-frame_list[i+1].vector_r.x;
      frame_list[i+1].vector_r.y=-frame_list[i+1].vector_r.y;
      frame_list[i+1].vector_r.z=-frame_list[i+1].vector_r.z;
      frame_list[i+1].angle=-frame_list[i+1].angle; // -180/180
      // frame_list[i+1].angle=(float)(2.0*M_PI-frame_list[i+1].angle); // 0-360
    }
    // translation
    dot=frame_list[i].vector_t.x*frame_list[i+1].vector_t.x+
        frame_list[i].vector_t.y*frame_list[i+1].vector_t.y+
        frame_list[i].vector_t.z*frame_list[i+1].vector_t.z;
    if(dot<0.0){
      frame_list[i+1].vector_t.x=-frame_list[i+1].vector_t.x;
      frame_list[i+1].vector_t.y=-frame_list[i+1].vector_t.y;
      frame_list[i+1].vector_t.z=-frame_list[i+1].vector_t.z;
      frame_list[i+1].step=-frame_list[i+1].step; 
    }
  }

  // set quaternions
  SetQuats(frame_list, n_frames);

  int type;
  float rev_flag=0.0f;
  float pris_flag=0.0f;
  for(i=0; i<n_frames; i++)
  {
    rev_flag+=(float)fabs(frame_list[i].angle);
    pris_flag+=(float)fabs(frame_list[i].step);
  }

  if((pris_flag > 0.0f) && (rev_flag > 0.0f)){
    type=LOOSE;
  }
  else
  if((pris_flag > 0.0f) && (rev_flag == 0.0f)){
    type=PRISMATIC; // this could later become TRANSLATIONAL
  }
  else
  if((pris_flag == 0.0f) && (rev_flag > 0.0f)){
    type=REVOLUTE; // this could later become SPHERICAL
  }
  else{
    type=FFIXED;
  }

  //printf("LOOSE=%d PRISMATIC=%d REVOLUTE=%d FFIXED=%d\n",
          //LOOSE,   PRISMATIC,   REVOLUTE,   FFIXED);
  //printf("pris_flag=%f rev_flag=%f type=%d\n",pris_flag,rev_flag,type);
  //DumpFrames(frame_list, n_frames);
  return type;
}


int IsInRange(float angle, const Vector& axis, const Vector& min, const Vector& max)
{
    Vector norm_axis(axis);
    norm_axis.normalize();

    norm_axis *= angle;
    
    if(norm_axis.x > max.x || norm_axis.x < min.x)
        return 0;

    if(norm_axis.y > max.y || norm_axis.y < min.y)
        return 0;

    if(norm_axis.z > max.z || norm_axis.z < min.z)
        return 0;

    return 1;
}


int AnimAxisMinMax(Frame* frame_list, int n_frames, float *axis, float *min,
                float *max, int type, float min_v[3], float max_v[3])
{
  float magnitude;
  int i;
  float angle;
  int sign, previous_sign=0;
  float last_angle;
  float last_step;
  float vxt, vyt, vzt;

  // compute average axis and min max angle limits
  *min = 0.0f;
  *max = 0.0f;
  axis[0] = 0.0f;
  axis[1] = 0.0f;
  axis[2] = 0.0f;
  min_v[0] = min_v[1] = min_v[2] = 0.0f;
  max_v[0] = max_v[1] = max_v[2] = 0.0f;

  if(type==REVOLUTE){
    if(frame_list[0].angle > 0.0f){
      previous_sign=1;
    }
    else{
      previous_sign=-1;
    }
  }

  vxt=vyt=vzt=0.0f;
  last_angle=0.0f;
  last_step=0.0f;
  float delta_angle; 
  //PersistVector first_axis; 
  //first_axis.x=first_axis.y=first_axis.z=0.0f;
  //PersistVector *last_axis=&first_axis;
  for(i=0; i<n_frames; i++){
    if(type==REVOLUTE){

      delta_angle=(float)fabs(frame_list[i].angle - last_angle);
      if(delta_angle>M_PI){
          delta_angle=(float)(2.0*M_PI-delta_angle);
      }

      axis[0] += ((frame_list[i].vector_r.x /*- last_axis->x*/) * delta_angle);
      axis[1] += ((frame_list[i].vector_r.y /*- last_axis->y*/) * delta_angle);
      axis[2] += ((frame_list[i].vector_r.z /*- last_axis->z*/) * delta_angle);
      //last_axis = &(frame_list[i].vector_r);
      last_angle = frame_list[i].angle;
      
      //axis[0] += frame_list[i].vector_r.x * (float)fabs(frame_list[i].angle);
      //axis[1] += frame_list[i].vector_r.y * (float)fabs(frame_list[i].angle);
      //axis[2] += frame_list[i].vector_r.z * (float)fabs(frame_list[i].angle);

      // distance from 0 degrees
      angle=frame_list[i].angle;
      if(angle>=M_PI/2.0){
        sign=1;
      }else
      if(angle<=-M_PI/2.0){
        sign=-1;
      }
      else{
        sign=0;
      }
      // we are crossing between 180 & -180 so limits have to support this
      // for interpolation purposes
      if( ((sign== 1)&&(previous_sign==-1)) ||
          ((sign==-1)&&(previous_sign== 1))){
        *min=(float)(-2.0*M_PI);
        *max=(float)(2.0*M_PI);
      }
      previous_sign=sign;

      if(angle<*min)
        *min=angle;
      if(angle>*max)
        *max=angle;

      if(frame_list[i].vector_r.x * frame_list[i].angle < min_v[0])
          min_v[0] = frame_list[i].vector_r.x * frame_list[i].angle;
      if(frame_list[i].vector_r.x * frame_list[i].angle > max_v[0])
          max_v[0] = frame_list[i].vector_r.x * frame_list[i].angle;

      if(frame_list[i].vector_r.y * frame_list[i].angle < min_v[1])
          min_v[1] = frame_list[i].vector_r.y * frame_list[i].angle;
      if(frame_list[i].vector_r.y * frame_list[i].angle > max_v[1])
          max_v[1] = frame_list[i].vector_r.y * frame_list[i].angle;

      if(frame_list[i].vector_r.z * frame_list[i].angle < min_v[2])
          min_v[2] = frame_list[i].vector_r.z * frame_list[i].angle;
      if(frame_list[i].vector_r.z * frame_list[i].angle > max_v[2])
          max_v[2] = frame_list[i].vector_r.z * frame_list[i].angle;

    }else
    if(type==PRISMATIC){
        //axis[0]+=frame_list[i].vector_t.x*fabs(frame_list[i].step);
        //axis[1]+=frame_list[i].vector_t.y*fabs(frame_list[i].step);
        //axis[1]+=frame_list[i].vector_t.z*fabs(frame_list[i].step);
      if(frame_list[i].step > last_step){
        axis[0] += (frame_list[i].vector_t.x*frame_list[i].step - vxt*last_step);
        axis[1] += (frame_list[i].vector_t.y*frame_list[i].step - vyt*last_step);
        axis[2] += (frame_list[i].vector_t.z*frame_list[i].step - vzt*last_step);
      }
      else{
        axis[0] += (vxt*last_step - frame_list[i].vector_t.x*frame_list[i].step);
        axis[1] += (vyt*last_step - frame_list[i].vector_t.y*frame_list[i].step);
        axis[2] += (vzt*last_step - frame_list[i].vector_t.z*frame_list[i].step);
      }
      last_step=frame_list[i].step;
      vxt = frame_list[i].vector_t.x;
      vyt = frame_list[i].vector_t.y;
      vzt = frame_list[i].vector_t.z;

      if(frame_list[i].step<*min)
        *min=frame_list[i].step;
      if(frame_list[i].step>*max)
        *max=frame_list[i].step;

      if(frame_list[i].vector_t.x * frame_list[i].step < min_v[0])
          min_v[0] = frame_list[i].vector_t.x * frame_list[i].step;
      if(frame_list[i].vector_t.x * frame_list[i].step > max_v[0])
          max_v[0] = frame_list[i].vector_t.x * frame_list[i].step;

      if(frame_list[i].vector_t.y * frame_list[i].step < min_v[1])
          min_v[1] = frame_list[i].vector_t.y * frame_list[i].step;
      if(frame_list[i].vector_t.y * frame_list[i].step > max_v[1])
          max_v[1] = frame_list[i].vector_t.y * frame_list[i].step;

      if(frame_list[i].vector_t.z * frame_list[i].step < min_v[2])
          min_v[2] = frame_list[i].vector_t.z * frame_list[i].step;
      if(frame_list[i].vector_t.z * frame_list[i].step > max_v[2])
          max_v[2] = frame_list[i].vector_t.z * frame_list[i].step;

    }
  }
  // normalize axis
  magnitude=(float)sqrt(axis[0]*axis[0] +
                        axis[1]*axis[1] +
                        axis[2]*axis[2]);
  if(magnitude > 0.0f)
  {
    axis[0]/=magnitude;
    axis[1]/=magnitude;
    axis[2]/=magnitude;
  }

  float vec[3];
  float thresh_angle;
  float max_thresh_angle=0.0f;
  // decide if we need a spherical joint
  if(type==REVOLUTE){
    for(i=0; i<n_frames; i++){
      if(fabs(frame_list[i].angle) >0.0){ // will have a valid axis
        vec[0]=frame_list[i].vector_r.x;
        vec[1]=frame_list[i].vector_r.y;
        vec[2]=frame_list[i].vector_r.z;
        thresh_angle = GetAngle(axis, vec);
//printf("%d ra=%.3f ta=%.3f\n",i, frame_list[i].angle*R2D, thresh_angle*R2D);
        if( thresh_angle > max_thresh_angle){
            max_thresh_angle = thresh_angle;
        }
        if( thresh_angle > (3.0*D2R) ){  // axis withing 3 degrees
          type=SPHERICAL;
          // printf("found an animated SPHERICAL joint.\n");
          // break;
        }
      }
    }
    if((type == SPHERICAL) && (max_thresh_angle*R2D < 10.0f)){// for our interest only
        //printf("Spherical thresh angle was %.2f deg\n", max_thresh_angle*R2D);
    }
  }
  else
  // decide if we need a 3D translational joint
  if(type==PRISMATIC){
    for(i=0; i<n_frames; i++){
      if(fabs(frame_list[i].step) >0.0){ // will have a valid axis
        vec[0]=frame_list[i].vector_t.x;
        vec[1]=frame_list[i].vector_t.y;
        vec[2]=frame_list[i].vector_t.z;
        thresh_angle = GetAngle(axis, vec);
        if( thresh_angle > max_thresh_angle ){
            max_thresh_angle = thresh_angle;
        }
        if( thresh_angle > (3.0*D2R) ){  // axis withing 3 degree
          type=TRANSLATIONAL;
          //break;
        }
      }
    }
    if((type == TRANSLATIONAL) && (max_thresh_angle*R2D < 10.0f)){
        fprintf(stderr,"Translational thresh angle was %.2f deg\n",max_thresh_angle*R2D);
    }
  }

  return type;
}

float NormAngle(float angle) // -180 to +180
{
   angle = (float)fmod(angle, 2.0*M_PI);
   //angle=((long)(angle*100000.0))/100000.0f;
   angle = (float)floor(angle*100000.0) / 100000.0f;
   
   if(angle>M_PI)
     angle-=(float)(2.0*M_PI); 
   if(angle<-M_PI)
     angle+=(float)(2.0*M_PI);

   return angle;
}

void LoadPrismatic(Pris *prismatic, Fix *fixed, DofData *dof_data, int type)
{
    InitPris(prismatic);

    if(verbose_level>=2){
      printf("Pris parent=%s child=%s\n",fixed->parent, fixed->child);
    }

    strcpy(prismatic->parent, fixed->parent);
    strcpy(prismatic->child, fixed->child);
    prismatic->rel_orientation=fixed->orient;

    if(type==PIVOT){
      prismatic->parent_point.x=dof_data->pivot[0];
      prismatic->parent_point.y=dof_data->pivot[1];
      prismatic->parent_point.z=dof_data->pivot[2];

      float x, y, z;
      x=-fixed->pos.x+dof_data->pivot[0];
      y=-fixed->pos.y+dof_data->pivot[1];
      z=-fixed->pos.z+dof_data->pivot[2];
      prismatic->child_point.x = x*fixed->orient.e00 +
                                 y*fixed->orient.e10 +
                                 z*fixed->orient.e20;
      prismatic->child_point.y = x*fixed->orient.e01 +
                                 y*fixed->orient.e11 +
                                 z*fixed->orient.e21;
      prismatic->child_point.z = x*fixed->orient.e02 +
                                 y*fixed->orient.e12 +
                                 z*fixed->orient.e22;

      prismatic->axis.x=dof_data->axis[0];
      prismatic->axis.y=dof_data->axis[1];
      prismatic->axis.z=dof_data->axis[2];
      prismatic->min=dof_data->min_step;
      prismatic->max=dof_data->max_step;
    }else
    if(type==ANIM){
      prismatic->parent_point.x=fixed->pos.x+dof_data->pivot[0];
      prismatic->parent_point.y=fixed->pos.y+dof_data->pivot[1];
      prismatic->parent_point.z=fixed->pos.z+dof_data->pivot[2];
  
      prismatic->child_point.x=dof_data->pivot[0];
      prismatic->child_point.y=dof_data->pivot[1];
      prismatic->child_point.z=dof_data->pivot[2];

      // axis & min,max already extracted from frame data
      prismatic->axis.x=dof_data->axis[0];
      prismatic->axis.y=dof_data->axis[1];
      prismatic->axis.z=dof_data->axis[2];
      prismatic->min=dof_data->min_step;
      prismatic->max=dof_data->max_step;
    }else
    if(type==MAX_ANIM){
      prismatic->parent_point.x=dof_data->pivot[0];
      prismatic->parent_point.y=dof_data->pivot[1];
      prismatic->parent_point.z=dof_data->pivot[2];

      prismatic->child_point.x=dof_data->pivot[0] - fixed->pos.x;
      prismatic->child_point.y=dof_data->pivot[1] - fixed->pos.y;
      prismatic->child_point.z=dof_data->pivot[2] - fixed->pos.z;

      prismatic->min=dof_data->min_step;
      prismatic->max=dof_data->max_step;
      // axis loaded later
    }else
    if(type==ALIAS_JOINT){
      prismatic->parent_point.x=dof_data->pivot[0];
      prismatic->parent_point.y=dof_data->pivot[1];
      prismatic->parent_point.z=dof_data->pivot[2];

      float x, y, z;
      x=-fixed->pos.x+dof_data->pivot[0];
      y=-fixed->pos.y+dof_data->pivot[1];
      z=-fixed->pos.z+dof_data->pivot[2];
      prismatic->child_point.x = x*fixed->orient.e00 +
                                 y*fixed->orient.e10 +
                                 z*fixed->orient.e20;
      prismatic->child_point.y = x*fixed->orient.e01 +
                                 y*fixed->orient.e11 +
                                 z*fixed->orient.e21;
      prismatic->child_point.z = x*fixed->orient.e02 +
                                 y*fixed->orient.e12 +
                                 z*fixed->orient.e22;

      prismatic->min=dof_data->min_step;
      prismatic->max=dof_data->max_step;

      prismatic->axis.x=dof_data->axis[0];
      prismatic->axis.y=dof_data->axis[1];
      prismatic->axis.z=dof_data->axis[2];
    }

    if(verbose_level>=2){
      printf("joint limits min=%f max=%f\n",prismatic->min/D2R, prismatic->max/D2R);
    }
}

void LoadSphere(PersistSphere *sphere, Fix *fixed, DofData *dof_data, int type)
{
    InitSphere(sphere);

    if(verbose_level>=2){
      printf("Sphere parent=%s child=%s\n",fixed->parent, fixed->child);
    }

    strcpy(sphere->parent, fixed->parent);
    strcpy(sphere->child, fixed->child);
    sphere->rel_orientation=fixed->orient;

    if(type==PIVOT){
      sphere->parent_point.x= dof_data->pivot[0];
      sphere->parent_point.y= dof_data->pivot[1];
      sphere->parent_point.z= dof_data->pivot[2];

      float x, y, z;
      x=-fixed->pos.x+dof_data->pivot[0];
      y=-fixed->pos.y+dof_data->pivot[1];
      z=-fixed->pos.z+dof_data->pivot[2];
      sphere->child_point.x = x*fixed->orient.e00 +
                              y*fixed->orient.e10 +
                              z*fixed->orient.e20;
      sphere->child_point.y = x*fixed->orient.e01 +
                              y*fixed->orient.e11 +
                              z*fixed->orient.e21;
      sphere->child_point.z = x*fixed->orient.e02 +
                              y*fixed->orient.e12 +
                              z*fixed->orient.e22;

      sphere->min_about_i= NormAngle(dof_data->min_r[0]);
      sphere->min_about_j= NormAngle(dof_data->min_r[1]);
      sphere->min_about_k= NormAngle(dof_data->min_r[2]);

      sphere->max_about_i= NormAngle(dof_data->max_r[0]);
      sphere->max_about_j= NormAngle(dof_data->max_r[1]);
      sphere->max_about_k= NormAngle(dof_data->max_r[2]);
    }else
    if(type==ANIM){
      sphere->parent_point.x= fixed->pos.x + dof_data->pivot[0];
      sphere->parent_point.y= fixed->pos.y + dof_data->pivot[1];
      sphere->parent_point.z= fixed->pos.z + dof_data->pivot[2];

      sphere->child_point.x= dof_data->pivot[0];
      sphere->child_point.y= dof_data->pivot[1];
      sphere->child_point.z= dof_data->pivot[2];

      // fix
      sphere->min_about_i= // DON't normalize angle
      sphere->min_about_j=
      sphere->min_about_k=(dof_data->min_angle);

      sphere->max_about_i=
      sphere->max_about_j=
      sphere->max_about_k=(dof_data->max_angle);
    }else
    if(type==MAX_ANIM){
      sphere->parent_point.x=dof_data->pivot[0];
      sphere->parent_point.y=dof_data->pivot[1];
      sphere->parent_point.z=dof_data->pivot[2];

      sphere->child_point.x=dof_data->pivot[0] - fixed->pos.x;
      sphere->child_point.y=dof_data->pivot[1] - fixed->pos.y;
      sphere->child_point.z=dof_data->pivot[2] - fixed->pos.z;
      
      sphere->min_about_i=(dof_data->min_r[0]); // DON't normalize angle
      sphere->min_about_j=(dof_data->min_r[1]);
      sphere->min_about_k=(dof_data->min_r[2]);
    
      sphere->max_about_i=(dof_data->max_r[0]);
      sphere->max_about_j=(dof_data->max_r[1]);
      sphere->max_about_k=(dof_data->max_r[2]);
    }else
    if(type==ALIAS_JOINT){
      sphere->parent_point.x=dof_data->pivot[0];
      sphere->parent_point.y=dof_data->pivot[1];
      sphere->parent_point.z=dof_data->pivot[2];

      float x, y, z;
      x=-fixed->pos.x+dof_data->pivot[0];
      y=-fixed->pos.y+dof_data->pivot[1];
      z=-fixed->pos.z+dof_data->pivot[2];
      sphere->child_point.x = x*fixed->orient.e00 +
                              y*fixed->orient.e10 +
                              z*fixed->orient.e20;
      sphere->child_point.y = x*fixed->orient.e01 +
                              y*fixed->orient.e11 +
                              z*fixed->orient.e21;
      sphere->child_point.z = x*fixed->orient.e02 +
                              y*fixed->orient.e12 +
                              z*fixed->orient.e22;
      
      sphere->min_about_i=(dof_data->min_r[0]); // DON't normalize angle
      sphere->min_about_j=(dof_data->min_r[1]);
      sphere->min_about_k=(dof_data->min_r[2]);
    
      sphere->max_about_i=(dof_data->max_r[0]);
      sphere->max_about_j=(dof_data->max_r[1]);
      sphere->max_about_k=(dof_data->max_r[2]);
    }

    if(verbose_level>=2){
      printf("joint limits min_i=%f max_i=%f\n",
        sphere->min_about_i/D2R, sphere->max_about_i/D2R);
      printf("joint limits min_j=%f max_j=%f\n",
        sphere->min_about_j/D2R, sphere->max_about_j/D2R);
      printf("joint limits min_k=%f max_k=%f\n",
        sphere->min_about_k/D2R, sphere->max_about_k/D2R);
    }
}

void LoadRevolute(Rev *revolute, Fix *fixed, DofData *dof_data, int type)
{
// fixed->pos is always in parent's coord system
// pivot's coord system varies since different API's provide different
// convenient ways of getting the data i.e.,(we do any conversions here)

    InitRev(revolute);

    if(verbose_level>=2){
      printf("Rev parent=%s child=%s\n",fixed->parent, fixed->child);
    }

    strcpy(revolute->parent, fixed->parent);
    strcpy(revolute->child, fixed->child);
    revolute->rel_orientation=fixed->orient;

    if(type==PIVOT){
      // pivot in parent's coord system
      revolute->parent_point.x=dof_data->pivot[0];
      revolute->parent_point.y=dof_data->pivot[1];
      revolute->parent_point.z=dof_data->pivot[2];

      float x, y, z;
      x = dof_data->pivot[0] - fixed->pos.x;
      y = dof_data->pivot[1] - fixed->pos.y;
      z = dof_data->pivot[2] - fixed->pos.z;
      revolute->child_point.x = x*fixed->orient.e00 +
                                y*fixed->orient.e10 +
                                z*fixed->orient.e20;
      revolute->child_point.y = x*fixed->orient.e01 +
                                y*fixed->orient.e11 +
                                z*fixed->orient.e21;
      revolute->child_point.z = x*fixed->orient.e02 +
                                y*fixed->orient.e12 +
                                z*fixed->orient.e22;

      revolute->axis.x=dof_data->axis[0];
      revolute->axis.y=dof_data->axis[1];
      revolute->axis.z=dof_data->axis[2];
      revolute->min=dof_data->min_angle;
      revolute->max=dof_data->max_angle;
    }else
    if(type==ANIM){
      // pivot in child's coord system
      revolute->parent_point.x=fixed->pos.x+dof_data->pivot[0];
      revolute->parent_point.y=fixed->pos.y+dof_data->pivot[1];
      revolute->parent_point.z=fixed->pos.z+dof_data->pivot[2];

      revolute->child_point.x=dof_data->pivot[0];
      revolute->child_point.y=dof_data->pivot[1];
      revolute->child_point.z=dof_data->pivot[2];

      // axis & min,max already extracted from frame data
      revolute->axis.x= dof_data->axis[0];
      revolute->axis.y= dof_data->axis[1];
      revolute->axis.z= dof_data->axis[2];

      revolute->min=(dof_data->min_angle); // DON't normalize angle
      revolute->max=(dof_data->max_angle);
    }else
    if(type==MAX_ANIM){
      // pivot is in parent's coord system
      revolute->parent_point.x=dof_data->pivot[0];
      revolute->parent_point.y=dof_data->pivot[1];
      revolute->parent_point.z=dof_data->pivot[2];

      revolute->child_point.x = dof_data->pivot[0] - fixed->pos.x;
      revolute->child_point.y = dof_data->pivot[1] - fixed->pos.y;
      revolute->child_point.z = dof_data->pivot[2] - fixed->pos.z;

      revolute->min=(dof_data->min_angle); // DON't normalize angle
      revolute->max=(dof_data->max_angle);
      // axis loaded later
    }else
    if(type==ALIAS_JOINT){
      // pivot is in parent's coord system
      revolute->parent_point.x=dof_data->pivot[0];
      revolute->parent_point.y=dof_data->pivot[1];
      revolute->parent_point.z=dof_data->pivot[2];

      float x, y, z;
      x = dof_data->pivot[0] - fixed->pos.x;
      y = dof_data->pivot[1] - fixed->pos.y;
      z = dof_data->pivot[2] - fixed->pos.z;
      revolute->child_point.x = x*fixed->orient.e00 +
                                y*fixed->orient.e10 +
                                z*fixed->orient.e20;
      revolute->child_point.y = x*fixed->orient.e01 +
                                y*fixed->orient.e11 +
                                z*fixed->orient.e21;
      revolute->child_point.z = x*fixed->orient.e02 +
                                y*fixed->orient.e12 +
                                z*fixed->orient.e22;

      revolute->min=(dof_data->min_angle); // DON't normalize angle
      revolute->max=(dof_data->max_angle);

      revolute->axis.x= dof_data->axis[0];
      revolute->axis.y= dof_data->axis[1];
      revolute->axis.z= dof_data->axis[2];
    }

    if(verbose_level>=2){
      printf("joint limits min=%f max=%f\n",revolute->min/D2R, revolute->max/D2R);
    }
}

void ScaleCompoundObject(CompoundObject *c_obj, const float scale_factor)
{
  c_obj->root_transform.v.x *= scale_factor;
  c_obj->root_transform.v.y *= scale_factor;
  c_obj->root_transform.v.z *= scale_factor;

  if(c_obj->transform_list != NULL)
  {
	for(int i=0; i < c_obj->part_count; i++)
	{
		c_obj->transform_list[i].v.x *= scale_factor;
		c_obj->transform_list[i].v.y *= scale_factor;
		c_obj->transform_list[i].v.z *= scale_factor;
	}
  }

  for(int i=0; i<c_obj->fix_count; i++)
  {
    c_obj->fix_list[i].pos.x*=scale_factor;
    c_obj->fix_list[i].pos.y*=scale_factor;
    c_obj->fix_list[i].pos.z*=scale_factor;
  }

  for(i=0; i<c_obj->pris_count; i++)
  {
    c_obj->pris_list[i].parent_point.x*=scale_factor;
    c_obj->pris_list[i].parent_point.y*=scale_factor;
    c_obj->pris_list[i].parent_point.z*=scale_factor;

    c_obj->pris_list[i].child_point.x*=scale_factor;
    c_obj->pris_list[i].child_point.y*=scale_factor;
    c_obj->pris_list[i].child_point.z*=scale_factor;

    c_obj->pris_list[i].min*=scale_factor;
    c_obj->pris_list[i].max*=scale_factor;
  }

  for(i=0; i<c_obj->rev_count; i++)
  {
    c_obj->rev_list[i].parent_point.x*=scale_factor;
    c_obj->rev_list[i].parent_point.y*=scale_factor;
    c_obj->rev_list[i].parent_point.z*=scale_factor;

    c_obj->rev_list[i].child_point.x*=scale_factor;
    c_obj->rev_list[i].child_point.y*=scale_factor;
    c_obj->rev_list[i].child_point.z*=scale_factor;
  }

  for(i=0; i<c_obj->sphere_count; i++)
  {
	  c_obj->sphere_list[i].parent_point.x *= scale_factor;
	  c_obj->sphere_list[i].parent_point.y *= scale_factor;
	  c_obj->sphere_list[i].parent_point.z *= scale_factor;

	  c_obj->sphere_list[i].child_point.x *= scale_factor;
	  c_obj->sphere_list[i].child_point.y *= scale_factor;
	  c_obj->sphere_list[i].child_point.z *= scale_factor;
  }

  for(i=0; i<c_obj->trans_count; i++)
  {
	  c_obj->trans_list[i].pos.x *= scale_factor;
	  c_obj->trans_list[i].pos.y *= scale_factor;
	  c_obj->trans_list[i].pos.z *= scale_factor;
  }

  for(i=0; i<c_obj->loose_count; i++)
  {
	  c_obj->loose_list[i].pos.x *= scale_factor;
	  c_obj->loose_list[i].pos.y *= scale_factor;
	  c_obj->loose_list[i].pos.z *= scale_factor;
  }

  
  for(i=0; i<c_obj->n_channel_count; i++)
  {
    int type = GetChannelType(c_obj, i);
    switch(type)
    {
    case PRISMATIC:
    case TRANSLATIONAL:
    case LOOSE:
      ScaleChannel(&(c_obj->n_channel_list[i].channel), scale_factor,
                   type);
      break;
    }
  }

}

int GetChannelType(CompoundObject *c_obj, int id)
{
  int j, k, l;
  char *ch_name;
  char *parent, *child;

  if(id >= c_obj->n_channel_count)
    return -1;

  if(c_obj->n_channel_list[id].channel.header.type==(PersistDT_VECTOR | PersistDT_QUATERNION)){
     return LOOSE;
  }else
  if(c_obj->n_channel_list[id].channel.header.type==(PersistDT_QUATERNION)){
     return SPHERICAL;
  }else
  if(c_obj->n_channel_list[id].channel.header.type==(PersistDT_VECTOR)){
     return TRANSLATIONAL;
  }else
  if(c_obj->n_channel_list[id].channel.header.type==(PersistDT_EVENT)){
     return EVENT;
  }


  ch_name=c_obj->n_channel_list[id].name; 

  for(j=0; j<c_obj->script_count; j++){
    for(k=0; k<c_obj->script_list[j].channel_count; k++){

      if(!strcmp(c_obj->script_list[j].channel_list[k].channel, ch_name)){
        parent=c_obj->script_list[j].channel_list[k].parent;
        child=c_obj->script_list[j].channel_list[k].child;
  
        for(l=0; l<c_obj->pris_count; l++){
          if((!strcmp(c_obj->pris_list[l].parent, parent)) &&
             (!strcmp(c_obj->pris_list[l].child, child)) ){
            return PRISMATIC;
          }
        }
 
        for(l=0; l<c_obj->rev_count; l++){
          if((!strcmp(c_obj->rev_list[l].parent, parent)) &&
             (!strcmp(c_obj->rev_list[l].child, child)) ){
            return REVOLUTE;
          }
        }

#if 0 // is checked faster above
        for(l=0; l<c_obj->sphere_count; l++){
          if((!strcmp(c_obj->sphere_list[l].parent, parent)) &&
             (!strcmp(c_obj->sphere_list[l].child, child)) ){
            return SPHERICAL;
          }
        }

        for(l=0; l<c_obj->trans_count; l++){
          if((!strcmp(c_obj->trans_list[l].parent, parent)) &&
             (!strcmp(c_obj->trans_list[l].child, child)) ){
            return TRANSLATIONAL;
          }
        }

        for(l=0; l<c_obj->loose_count; l++){
          if((!strcmp(c_obj->loose_list[l].parent, parent)) &&
             (!strcmp(c_obj->loose_list[l].child, child)) ){
            return LOOSE;
          }
        }
#endif

      }
    }
  }

  fprintf(stderr,"Error: unknown channel type for %s.\n",
          c_obj->n_channel_list[id].name);
  return -1;
}

void ScaleChannel(Channel *ch, float scale_factor, int type)
{
  int i;
  float *data_pt;
  int key_flag;

  if(ch->header.capture_rate < 0)
  {
    key_flag = 1;
  }
  else{
    key_flag = 0;
  }


  data_pt=(float*)ch->data;

  if(type == PRISMATIC)
  {
    if(key_flag)
    { // key frames
      for(i=0; i<ch->header.frames; i++)
      {
        data_pt[2*i+1] *= scale_factor;
      }
    }
    else
    { // frames
      for(i=0; i<ch->header.frames; i++)
      {
        data_pt[i] *= scale_factor;
      }
    }
  }
  else
  if(type == TRANSLATIONAL)
  {
    if(key_flag)
    {
      for(i=0; i<ch->header.frames; i++)
      {
        data_pt[4*i+1] *= scale_factor;
        data_pt[4*i+2] *= scale_factor;
        data_pt[4*i+3] *= scale_factor;
      }
    }
    else
    {
      for(i=0; i<ch->header.frames; i++)
      {
        data_pt[3*i] *= scale_factor;
        data_pt[3*i+1] *= scale_factor;
        data_pt[3*i+2] *= scale_factor;
      }
    }
  }
  else
  if(type == LOOSE)
  {
    if(key_flag)
    {
      for(i=0; i<ch->header.frames; i++)
      {
        data_pt[8*i+1] *= scale_factor;
        data_pt[8*i+2] *= scale_factor;
        data_pt[8*i+3] *= scale_factor;
      }
    }
    else
    {
      for(i=0; i<ch->header.frames; i++)
      {
        data_pt[7*i] *= scale_factor;
        data_pt[7*i+1] *= scale_factor;
        data_pt[7*i+2] *= scale_factor;
      }
    }
  }

}

void mat_to_qt(QT *dst, const Matrix *m)
{
  static int nxt[3]={1,2,0};
  SINGLE trace = m->d[0][0] + m->d[1][1] + m->d[2][2];

    if (trace > 0.0)
    {
        SINGLE st = (SINGLE) sqrt(trace + 1.0);
        dst->v.w = st * 0.5f;
        st = 0.5f / st;

        dst->v.x = (m->d[2][1] - m->d[1][2]) * st;
        dst->v.y = (m->d[0][2] - m->d[2][0]) * st;
        dst->v.z = (m->d[1][0] - m->d[0][1]) * st;
    }
    else
    {
        int i = 0;
        if (m->d[1][1] > m->d[0][0])
        {
            i = 1;
        }
        if (m->d[2][2] > m->d[i][i])
        {
            i = 2;
        }

        int j = nxt[i];
        int k = nxt[j];

        SINGLE st = (SINGLE) sqrt((m->d[i][i] - (m->d[j][j] + m->d[k][k])) + 1.0);

        dst->d[i+1] = st * 0.5f;
        st = 0.5f / st;
        dst->v.w = (m->d[k][j] - m->d[j][k]) * st;
        dst->d[j+1] = (m->d[j][i] + m->d[i][j]) * st;
        dst->d[k+1] = (m->d[k][i] + m->d[i][k]) * st;
    }
}

void qt_to_mat(Matrix *dst, const QT *q)
{
        SINGLE s = 2.0f / (q->v.w * q->v.w + 
                           q->v.x * q->v.x + 
                           q->v.y * q->v.y + 
                           q->v.z * q->v.z);

        SINGLE xs = q->v.x * s;
        SINGLE ys = q->v.y * s;
        SINGLE zs = q->v.z * s;

        SINGLE wx = q->v.w * xs;
        SINGLE wy = q->v.w * ys;
        SINGLE wz = q->v.w * zs;

        SINGLE xx = q->v.x * xs;
        SINGLE xy = q->v.x * ys;
        SINGLE xz = q->v.x * zs;
        SINGLE yy = q->v.y * ys;
        SINGLE yz = q->v.y * zs;
        SINGLE zz = q->v.z * zs;

        dst->d[0][0] = 1.0f - (yy + zz);
        dst->d[1][0] = xy + wz;
        dst->d[2][0] = xz - wy;

        dst->d[0][1] = xy - wz;
        dst->d[1][1] = 1.0f - (xx + zz);
        dst->d[2][1] = yz + wx;

        dst->d[0][2] = xz + wy;
        dst->d[1][2] = yz - wx;
        dst->d[2][2] = 1.0f - (xx + yy);
}


float GetQuatDiffAngles(const PersistQuaternion & qt1, const PersistQuaternion & qt2, 
                   float *rot_angle, float *axis_angle)
{
  QT q1, q2;
  float inv_mag;
  float dot;
  float angle1, angle2;

  q1.v=qt1;
  q2.v=qt2;

  angle1 = 2.0f * (Acos(q1.v.w) - Acos(q2.v.w));
  angle2 = 2.0f * (Acos(q1.v.w) + Acos(q2.v.w));
  *rot_angle = (angle1 <= angle2) ? angle1 : angle2;
    
  angle1 = GetAngle(&(q1.d[1]), &(q2.d[1]));
  q2.d[1]=-q2.d[1];
  q2.d[2]=-q2.d[2];
  q2.d[3]=-q2.d[3];
  angle2 = GetAngle(&(q1.d[1]), &(q2.d[1]));
  *axis_angle = (angle1 <= angle2) ? angle1 : angle2;
      
  // compute 4D arc angle
  q1.v=qt1;
  q2.v=qt2;

  inv_mag=1.0f/(float)sqrt(q1.v.w*q1.v.w + q1.v.x*q1.v.x + q1.v.y*q1.v.y + q1.v.z*q1.v.z); 
  q1.v.w*=inv_mag;
  q1.v.x*=inv_mag;
  q1.v.y*=inv_mag;
  q1.v.z*=inv_mag;

  inv_mag=1.0f/(float)sqrt(q2.v.w*q2.v.w + q2.v.x*q2.v.x + q2.v.y*q2.v.y + q2.v.z*q2.v.z); 
  q2.v.w*=inv_mag;
  q2.v.x*=inv_mag;
  q2.v.y*=inv_mag;
  q2.v.z*=inv_mag;

  dot=q1.v.w*q2.v.w + q1.v.x*q2.v.x + q1.v.y*q2.v.y + q1.v.z*q2.v.z;
  angle1=Acos(dot);

  dot=(-q1.v.w)*q2.v.w + (-q1.v.x)*q2.v.x + (-q1.v.y)*q2.v.y + (-q1.v.z)*q2.v.z;
  angle2=Acos(dot);
 
  return ( (angle1 <= angle2) ? angle1 : angle2 );
}

// Quaternion interpolation.
void quat_slerp(PersistQuaternion & dst, const PersistQuaternion & q1, 
                const PersistQuaternion & q2, SINGLE t)
{
        const float  Q_EPSILON=0.00001f;

        // Make sure we're taking the shorter arc.

    PersistQuaternion sum, dif, qs;

    sum.w = q1.w + q2.w;
    sum.x = q1.x + q2.x;
    sum.y = q1.y + q2.y;
    sum.z = q1.z + q2.z;

    dif.w = q1.w - q2.w;
    dif.x = q1.x - q2.x;
    dif.y = q1.y - q2.y;
    dif.z = q1.z - q2.z;

    SINGLE s1 = dif.w * dif.w + dif.x * dif.x + dif.y * dif.y + dif.z * dif.z;
    SINGLE s2 = sum.w * sum.w + sum.x * sum.x + sum.y * sum.y + sum.z * sum.z;

    if (s1 > s2)
    {
        qs.w = -q2.w;
        qs.x = -q2.x;
        qs.y = -q2.y;
        qs.z = -q2.z;
    }
    else
    {
        qs = q2;
    }

    SINGLE cos_omega = q1.w * qs.w + q1.x * qs.x + q1.y * qs.y + q1.z * qs.z;

// Check for cases where rotations are nearly opposite, which causes numerical
// weirdness.
    if ((1.0 + cos_omega) > Q_EPSILON)
    {
    // Check for case where rotations are very close, which also causes 
    // weirdness. 
        if ((1.0 - cos_omega) > Q_EPSILON)
        {
            SINGLE omega = Acos(cos_omega);
            SINGLE sin_omega = (float)sin(omega);
    
            s1 = (float)sin((1.0 - t) * omega) / sin_omega;
            s2 = (float)sin(t * omega) / sin_omega;
        }
        else
        {
        // Too close, do straight linear interpolation.
            s1 = 1.0f - t;
            s2 = t;
        }

        dst.w = s1 * q1.w + s2 * qs.w;
        dst.x = s1 * q1.x + s2 * qs.x;
        dst.y = s1 * q1.y + s2 * qs.y;
        dst.z = s1 * q1.z + s2 * qs.z;
    }
    else
    {
        dst.w =  q1.z;
        dst.x = -q1.y;
        dst.y =  q1.x;
        dst.z = -q1.w;

        SINGLE half_pi = SINGLE(M_PI / 2.0);

        s1 = (float)sin((1.0 - t) * half_pi);
        s2 = (float)sin(t * half_pi);

        dst.x = s1 * q1.x + s2 * dst.x;
        dst.y = s1 * q1.y + s2 * dst.y;
        dst.z = s1 * q1.z + s2 * dst.z;
    }
}

int GetNodeType(file_node *node)
{
  // sanity check
  if(node==NULL) return -1;

  if(!strcmp(node->name, COMPOUND_DIR_NAME)){
    return CMPND;
  }else
  if(!strcmp(node->name, ANIMATION)){
    return ANIM;
  }else
  if(!strcmp(node->name, openFLAME_3D_N_mesh)){
    return OPENFLAME;
  }else
  if(!strcmp(node->name, MultiLevel)){
    return MULTILEVEL;
  }else
  if(!strcmp(node->name, Rigid_body)){
    return RIGID;
  }else
  if(!strcmp(node->name, Bone)){
    return BONE;
  }
  else{
    return UNKNOWN;
  }
}

void CleanDofData(DofData *data)
{
  const float tolerance = .00001f;
  // open up a joint correctly
  if(fabs(data->min_r[0] - -180.0f*D2R) < tolerance && 
     fabs(data->max_r[0] -  180.0f*D2R) < tolerance)
  {
    data->min_r[0] = (float)(-360.0f*D2R);
    data->max_r[0] = (float)( 360.0f*D2R);
  }
  if(fabs(data->min_r[1] - -180.0f*D2R) < tolerance && 
     fabs(data->max_r[1] -  180.0f*D2R) < tolerance)
  {
    data->min_r[1] = (float)(-360.0f*D2R);
    data->max_r[1] = (float)( 360.0f*D2R);
  }
  if(fabs(data->min_r[1] - -180.0f*D2R) < tolerance && 
     fabs(data->max_r[1] -  180.0f*D2R) < tolerance)
  {
    data->min_r[1] = (float)(-360.0f*D2R);
    data->max_r[1] = (float)( 360.0f*D2R);
  }

  float r_axis[3]={0.0f, 0.0f, 0.0f};
  float t_axis[3]={0.0f, 0.0f, 0.0f};
  float t_delta[3]={0.0f, 0.0f, 0.0f};


  // this is a hack to allow for interpolation across the  180 to -180 boundary
    if(fabs(data->min_r[0] - -360.0*D2R) > tolerance || fabs(data->max_r[0] - 360.0*D2R) > tolerance)
    {
        data->min_r[0]=NormAngle(data->min_r[0]);
        data->max_r[0]=NormAngle(data->max_r[0]);
    }

    if(fabs(data->min_r[1] - -360.0*D2R) > tolerance || fabs(data->max_r[1] - 360.0*D2R) > tolerance)
    {
        data->min_r[1]=NormAngle(data->min_r[1]);
        data->max_r[1]=NormAngle(data->max_r[1]);
    }

    if(fabs(data->min_r[2] - -360.0*D2R) > tolerance || fabs(data->max_r[2] - 360.0*D2R) > tolerance)
    {
        data->min_r[2]=NormAngle(data->min_r[2]);
        data->max_r[2]=NormAngle(data->max_r[2]);
    }

  // see which x,y,z limits are set for translation
  if(fabs(data->min_t[0])+fabs(data->max_t[0])){
    t_axis[0]=1.0f;
    t_delta[0]=data->max_t[0] - data->min_t[0];
    data->min_step=data->min_t[0];
    data->max_step=data->max_t[0];
  }
  if(fabs(data->min_t[1])+fabs(data->max_t[1])){
    t_axis[1]=1.0f;
    t_delta[1]=data->max_t[1] - data->min_t[1];
    data->min_step=data->min_t[1];
    data->max_step=data->max_t[1];
  }
  if(fabs(data->min_t[2])+fabs(data->max_t[2])){
    t_axis[2]=1.0f;
    t_delta[2]=data->max_t[2] - data->min_t[2];
    data->min_step=data->min_t[2];
    data->max_step=data->max_t[2];
  }

  // see which x,y,z limits are set for rotation
  if(fabs(data->min_r[0])+fabs(data->max_r[0])){
    r_axis[0]=1.0f;
    data->min_angle=data->min_r[0];
    data->max_angle=data->max_r[0];
  }
  if(fabs(data->min_r[1])+fabs(data->max_r[1])){
    r_axis[1]=1.0f;
    data->min_angle=data->min_r[1];
    data->max_angle=data->max_r[1];
  }
  if(fabs(data->min_r[2])+fabs(data->max_r[2])){
    r_axis[2]=1.0f;
    data->min_angle=data->min_r[2];
    data->max_angle=data->max_r[2];
  }

  // check for invalid pris joint (we try to patch it if invalid)
  if(t_axis[0]+t_axis[1]+t_axis[2] > 1.0f){
    fprintf(stderr,
      "Warning: invalid DOF node w/ more than 1 prismatic degree of freedom.\n");
    fprintf(stderr,"Using the largest of %.3f %.3f %.3f.\n",
      t_axis[0],t_axis[1],t_axis[2]);
    fprintf(stderr,"%s\n",data->name);

    if((t_delta[0]>=t_delta[1]) && (t_delta[0]>=t_delta[2])){
      t_axis[0]=1.0f;
      t_axis[1]=0.0f;
      t_axis[2]=0.0f;
      data->min_step=data->min_t[0];
      data->max_step=data->max_t[0];
    }else
    if((t_delta[1]>=t_delta[0]) && (t_delta[1]>=t_delta[2])){
      t_axis[0]=0.0f;
      t_axis[1]=1.0f;
      t_axis[2]=0.0f;
      data->min_step=data->min_t[1];
      data->max_step=data->max_t[1];
    }else
    if((t_delta[2]>=t_delta[0]) && (t_delta[2]>=t_delta[1])){
      t_axis[0]=0.0f;
      t_axis[1]=0.0f;
      t_axis[2]=1.0f;
      data->min_step=data->min_t[2];
      data->max_step=data->max_t[2];
    }
  }

  if((t_axis[0]+t_axis[1]+t_axis[2] > 0.0f) && 
     (r_axis[0]+r_axis[1]+r_axis[2] > 0.0f)){
    fprintf(stderr,
      "Warning: invalid DOF node w/ both rotational & prismatic limits.\n");
    fprintf(stderr,"Using rotational only.\n");
    fprintf(stderr,"%s\n",data->name);

    t_axis[0]=t_axis[1]=t_axis[2]=0.0f;
    // data->min_step=data->max_step=0.0f;
  }

  if(r_axis[0]+r_axis[1]+r_axis[2] > 1.0f){
    data->type=SPHERICAL;
    data->min_angle=data->max_angle=0.0f;
  }else
  if(r_axis[0]+r_axis[1]+r_axis[2] > 0.0f){
    data->type=REVOLUTE;
    data->axis[0]=r_axis[0];
    data->axis[1]=r_axis[1];
    data->axis[2]=r_axis[2];
  }else
  if(t_axis[0]+t_axis[1]+t_axis[2] > 0.0f){
    data->type=PRISMATIC;
    data->axis[0]=t_axis[0];
    data->axis[1]=t_axis[1];
    data->axis[2]=t_axis[2];
  }
  else{
    data->type=FFIXED;
  }
}

void PrintQuat(const float quat[])
{
  float mag = (float)sqrt(quat[1]*quat[1] + quat[2]*quat[2] + quat[3]*quat[3]);

  if(mag>0.0f)
  {
    mag=1.0f/mag;
  }
  else
  {
    mag=1.0f;
  }

  printf("w=%.3f x=%.3f y=%.3f z=%.3f angle=%.3fDeg axis= %.3f %.3f %.3f\n",
         quat[0], quat[1], quat[2], quat[3],
         R2D*2.0*Acos(quat[0]), quat[1]*mag, quat[2]*mag, quat[3]*mag);
}

Matrix Matrix_mul_Matrix(const Matrix& m1, const Matrix& m2)
{
  Matrix result;

  result.d[0][0] = 

    m1.d[0][0]*m2.d[0][0] + m1.d[0][1]*m2.d[1][0] + m1.d[0][2]*m2.d[2][0];

  result.d[1][0] = 

    m1.d[0][0]*m2.d[0][1] + m1.d[0][1]*m2.d[1][1] + m1.d[0][2]*m2.d[2][1];

  result.d[2][0] = 

    m1.d[0][0]*m2.d[0][2] + m1.d[0][1]*m2.d[1][2] + m1.d[0][2]*m2.d[2][2];


  result.d[0][1] = 

    m1.d[1][0]*m2.d[0][0] + m1.d[1][1]*m2.d[1][0] + m1.d[1][2]*m2.d[2][0];

  result.d[1][1] = 

    m1.d[1][0]*m2.d[0][1] + m1.d[1][1]*m2.d[1][1] + m1.d[1][2]*m2.d[2][1];

  result.d[2][1] = 

    m1.d[1][0]*m2.d[0][2] + m1.d[1][1]*m2.d[1][2] + m1.d[1][2]*m2.d[2][2];


  result.d[0][2] = 

    m1.d[2][0]*m2.d[0][0] + m1.d[2][1]*m2.d[1][0] + m1.d[2][2]*m2.d[2][0];

  result.d[1][2] = 

    m1.d[2][0]*m2.d[0][1] + m1.d[2][1]*m2.d[1][1] + m1.d[2][2]*m2.d[2][1];

  result.d[2][2] = 

    m1.d[2][0]*m2.d[0][2] + m1.d[2][1]*m2.d[1][2] + m1.d[2][2]*m2.d[2][2];

  return result;
}

void DumpMatrix(const Matrix& m)
{
  printf("%.6f %.6f %.6f\n",m.d[0][0], m.d[0][1], m.d[0][2]);
  printf("%.6f %.6f %.6f\n",m.d[1][0], m.d[1][1], m.d[1][2]);
  printf("%.6f %.6f %.6f\n",m.d[2][0], m.d[2][1], m.d[2][2]);
}

#ifndef SGI
#pragma warning( default : 4018 ) 
// signed/unsigned mismatch
#endif
