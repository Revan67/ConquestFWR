//
// NURB rendering code.
//

#include "NURBMesh.h"
#include "lightman.h"
#include "Engine.h"
#include "ILight.h"
#include "ICamera.h"
#include "rendpipeline.h"
#include "FDUMP.h"
#include "matrix4.h"
#include "TextureCoord.h"


#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( push, 3 )

#define IRP_CALL( call )	\
	if( BATCH ) {			\
		BATCH->call ;		\
	}						\
	else if( PIPE ) {		\
		PIPE->call ;		\
	}

vis_state NURBMESH::render(ICamera * camera, INSTANCE_INDEX index, U32 flags, const Transform *tr,
						   float lod_fraction)
{
	ASSERT(camera);
	//TrapFpu(true);

	vis_state result = VS_UNKNOWN;

	NURBInstance * object;
	inst_map::iterator inst;

	if( (inst = instances.find( index )) == instances.end() ) {
		return result;
	}

	object = inst->second;

	XNURB * xnurb = object->xnurb;

	ASSERT(xnurb);

	xnurb->last_face_cnt = 0;
	xnurb->last_vertex_cnt = 0;

	
	
	Transform world_to_view ( camera->get_inverse_transform() );

	Transform object_to_world(false);
	if(tr != NULL) 
	{
		object_to_world = *tr * ENG->get_transform(index);
	}
	else 
	{
		object_to_world = ENG->get_transform(index);
	}

	// NOTE:
	// zero_D is a hack to keep things simple and fast
	// if things get smaller face visibility checks don't work w/o recalculating D
	// but to do that correctly we would also have to recompute the plane equation/normal
	// for each face.  setting D to 0 gets around this as 0 is scale independent.  note, the normal
	// direction can still be wrong so lighting can be off
	bool zero_D;
	Vector svec;
	if( tr ) 
	{	
		svec = Vector(tr->get_i().magnitude(), tr->get_j().magnitude(), tr->get_k().magnitude());
		zero_D = (svec.x < 1.0 || svec.y < 1.0 || svec.z < 1.0);
	}
	else
	{
		svec = Vector(1, 1, 1);
		zero_D = false;
	}

	float max_scale = Tmax(svec.x, Tmax(svec.y, svec.z));

	//
	// Don't bother with objects outside the view frustum.
	//	
	// sphere center in world transformed to view
	Vector view_pos (world_to_view.rotate_translate( object_to_world * xnurb->sphere_center));

	result = camera->object_visibility(view_pos, xnurb->radius * max_scale);
	if( result == VS_NOT_VISIBLE || result == VS_SUB_PIXEL) 
	{
		return result;
	}

	const Vector cam_pos ( camera->get_position() );
	const Vector cam_pos_in_object_space ( object_to_world.inverse_rotate_translate(cam_pos) );
	

	// Take out any scale that might have been in tr for lighting purposes
	// otherwise normals get scaled and so do dot products
	Transform light_world_to_object(false);
	if( tr ) 
	{
		light_world_to_object = object_to_world;
		light_world_to_object.set_i( light_world_to_object.get_i().normalize() );
		light_world_to_object.set_j( light_world_to_object.get_j().normalize() );
		light_world_to_object.set_k( light_world_to_object.get_k().normalize() );
		light_world_to_object = light_world_to_object.get_inverse(); 
	}
	else 
	{
		light_world_to_object = object_to_world.get_inverse();
	}

	Transform object_to_view ( world_to_view * object_to_world );

	IRP_CALL( set_modelview( object_to_view ) )

	
	// only use view dependent tessellation when object is relatively close (per Talley)
	// unfortunately this causes lots of popping
/*
	const float lod_threshold = 0.0f;
	if(lod_fraction >= lod_threshold)
	{
		Matrix4 proj_m;

		IRP_CALL ( get_projection(proj_m) )

		proj_m = proj_m * Matrix4(object_to_view);

		// to damp jitter and improve cache hits
		if((proj_m - object->view_transform).get_norm() > 0.115f ) // .115f is about 10 degrees of rotation
		{
			object->view_transform = proj_m;
		}
		
		IIFloat	tes_control[1] = {DEFAULT_PIXEL_ERROR};
		if(xnurb->rational_count > 0)
		{
			ActivateEvaluator(RATIONAL);
			ii_result( evaluator->Unlock_State() );
				evaluator->Enable( IIGLOO_VIEW_DEPENDANT_TESSELLATION );
				evaluator->Set_View_Transform( (IIFloat*)&(object->view_transform[0][0]) );
				evaluator->Set_Tessellation_Control( IIGLOO_GEOMETRY_DATA, tes_control );
			ii_result( evaluator->Lock_State() );
		}

		if(xnurb->non_rational_count > 0)
		{
			ActivateEvaluator(NON_RATIONAL);
			ii_result( evaluator->Unlock_State() );
				evaluator->Enable( IIGLOO_VIEW_DEPENDANT_TESSELLATION );
				evaluator->Set_View_Transform( (IIFloat*)&(object->view_transform[0][0]) );
				evaluator->Set_Tessellation_Control( IIGLOO_GEOMETRY_DATA, tes_control );
			ii_result( evaluator->Lock_State() );
		}
	}
	else
*/
	{
		const Vector centroid_in_camera ( object_to_view * xnurb->centroid );
		const float pixel_length_at_object = pixel_error * centroid_in_camera.magnitude() /
			( camera->get_znear() * camera->get_hpc() );

		IIFloat	tes_control[1] = {pixel_length_at_object}; // range .0001 - 10000  IIGLOO_MAGNITUDE_TOLERANCE

		if(xnurb->rational_count > 0)
		{
			ActivateEvaluator(RATIONAL);
			ii_result( evaluator->Unlock_State() );
				evaluator->Disable( IIGLOO_VIEW_DEPENDANT_TESSELLATION );
				evaluator->Set_Tessellation_Control( IIGLOO_GEOMETRY_DATA, tes_control );
			ii_result( evaluator->Lock_State() );
		}

		if(xnurb->non_rational_count > 0)
		{
			ActivateEvaluator(NON_RATIONAL);
			ii_result( evaluator->Unlock_State() );
				evaluator->Disable( IIGLOO_VIEW_DEPENDANT_TESSELLATION );
				evaluator->Set_Tessellation_Control( IIGLOO_GEOMETRY_DATA, tes_control );
			ii_result( evaluator->Lock_State() );
		}
	}
	
	for(int i = 0; i < xnurb->patch_cnt; i++)
	{
		const NURBPatch & patch = xnurb->patch_list[i];

		if(patch.weight_list)
			ActivateEvaluator(RATIONAL);
		else
			ActivateEvaluator(NON_RATIONAL);

		vis_state patch_visibility = CullPatch(patch, cam_pos_in_object_space);

		switch(patch_visibility)
		{
			case VS_NOT_VISIBLE:
				continue;
				break;
			case VS_PARTIALLY_VISIBLE:
				// deffer culling to sub patches
				break;
			case VS_FULLY_VISIBLE:
				// IIGLOO tessellates CW so we cull CCW
				IRP_CALL ( set_render_state( D3DRS_CULLMODE, D3DCULL_NONE ) )		
				break;
			default:
				GENERAL_FATAL("BAD vis_state!\n"); 
				break;
		}

		Material & mat = xnurb->material_list[patch.mtl_id];

		// We don't light each individual vertex after tessellation because a normal at each vertex
		// would also have to be generated.  Secondly we would have to use IIView to cull.
		// This should be more efficient for tesseleations higher than 4x4
		// and less efficient at lower ones.
		const ClampFlags light_clamp = (flags & RF_CLAMP_COLOR) ? CF_COLOR : CF_INTENSITY;
		LightPatch(light_world_to_object, patch, mat, light_clamp);

		// Set common state for all passes
		//
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	0 ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_ALPHAOP,		D3DTOP_MODULATE ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_ALPHAARG1,		D3DTA_TEXTURE ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_ALPHAARG2,		D3DTA_DIFFUSE ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_COLOROP,		D3DTOP_MODULATE ))
		IRP_CALL( set_texture_stage_state( 1, D3DTSS_COLOROP,		D3DTOP_DISABLE ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_COLORARG1,		D3DTA_TEXTURE ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_COLORARG2,		D3DTA_DIFFUSE ))
		IRP_CALL( set_sampler_state( 0, D3DSAMP_MINFILTER,		D3DTEXF_LINEAR ))
		IRP_CALL( set_sampler_state( 0, D3DSAMP_MAGFILTER,		D3DTEXF_LINEAR ))
		IRP_CALL( set_sampler_state( 0, D3DSAMP_MIPFILTER,		D3DTEXF_POINT ))

		IRP_CALL( set_render_state( D3DRS_ZENABLE,			TRUE ))
		IRP_CALL( set_render_state( D3DRS_ZWRITEENABLE,	TRUE ))

		IRP_CALL( set_texture_stage_texture( 0, mat.texture_id ))
		//IRP_CALL( set_render_state( D3DRS_TEXTUREHANDLE, mat.texture_id ))

		IRP_CALL( set_texture_stage_state( 0, D3DTSS_ADDRESSU,
			MAT_GET_ADDR_MODE(mat.texture_flags,0) ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_ADDRESSV,
			MAT_GET_ADDR_MODE(mat.texture_flags,1) ))
		
		bool opaque;
		if (mat.transparency < 255)
		{
			opaque = false;
		}
		else if (mat.texture_id && mat.texture_flags & TF_F_HAS_ALPHA) //mat.texture_alpha_bits)
		{
			opaque = false;
		}
		else
		{
			opaque = true;
		}

		if (!opaque)
		{
			IRP_CALL( set_render_state( D3DRS_ALPHABLENDENABLE, TRUE ) )
			IRP_CALL( set_render_state( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA ) )
			IRP_CALL( set_render_state( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA ) )
		}
		else
		{
			IRP_CALL( set_render_state( D3DRS_ALPHABLENDENABLE, FALSE ) )
		}
		
		SetPatchEvaluatorState(patch);

		IIUInt		row = 0;
		IIUInt		point_step;

		for ( int s_basis = 0; s_basis < patch.s_basis_cnt; s_basis++ )
		{
			patch.s_basis_list[s_basis]->Get_Control_Point_Step( &point_step );
			row += point_step; // point step is 0 for basis 0; 1 for non repeated knots; 
							   // 2 for twice repeated knot; ...

			IIUInt column = 0;

			for ( int t_basis = 0; t_basis < patch.t_basis_cnt; t_basis++ )
			{
				patch.t_basis_list[t_basis]->Get_Control_Point_Step( &point_step );
				column += point_step;

				// cull sub patches
				if(VS_PARTIALLY_VISIBLE == patch_visibility)
				{
					switch(visible_patch[s_basis * patch.t_basis_cnt + t_basis])
					{
						case VS_NOT_VISIBLE:
							continue;
							break;
						case VS_PARTIALLY_VISIBLE:
							IRP_CALL ( set_render_state( D3DRS_CULLMODE, D3DCULL_CCW ) )		
							break;
						case VS_FULLY_VISIBLE:
							// disabling culling should work but sorting problems w/ 16 bit Z show up
							IRP_CALL ( set_render_state( D3DRS_CULLMODE, D3DCULL_CCW ) )
							//IRP_CALL ( set_render_state( D3DRS_CULLMODE, D3DCULL_NONE ) )		
							break;
						default:
							GENERAL_FATAL("BAD vis_state!\n"); 
							break;
					}
				}

				// assign control points to polynomials
				AssignDataToPolynomials(patch, row, column, s_basis, t_basis, UV_ON | LIGHT_ON);

				// assign polynomials to evaluator
				AssignPolynomialsToEvaluator(patch, s_basis, t_basis, UV_ON | LIGHT_ON);

#if USE_CACHE
				ii_result( evaluator->Set_Current_Evaluation( 
					object->iicache_hangle_list[i][s_basis * patch.t_basis_cnt + t_basis]) );

				// light is changing so cache needs updating no matter what
				ii_result( evaluator->Reset_Evaluation_Component( IIGLOO_COLOR_DATA ) );
#endif

				IIBool					done;
				IITessellationType		tessellation_type;
				IIUInt					point_count;
				IIUInt					index_count;
				IIBool					indexed_output;
				IIInt					generated_point_count;
				IIInt					generated_point_offset;

				ii_result( evaluator->Begin_Evaluation( &done ) );

				do
				{
					ii_result( evaluator->Get_Tessellation_Output_Data( &tessellation_type, 
						&point_count, &indexed_output, &index_count, &generated_point_offset,
						&generated_point_count) );

					ASSERT_FATAL((int)point_count <= vertex_pool_len);
					ASSERT_FATAL(indexed_output); // forced by IIGLOO_INDEXED_TESSELLATION_OUTPUT

					render_faces(tessellation_type, point_count, index_count,
						generated_point_offset, generated_point_count,
						xnurb->material_list[patch.mtl_id], xnurb, 0 );

					if( !done )
					{
						ii_result( evaluator->Continue_Evaluation( &done ) );
					}
					else
					{
						break;
					}
				} while ( true );

				ii_result( evaluator->End_Evaluation() );
			}
		}
	}

	return result;
}

void NURBMESH::render_faces(const IITessellationType tessellation_type, const IIUInt point_count, 
			const IIUInt index_count, const IIInt generated_point_offset, 
			const IIInt generated_point_count, const Material & mat,
			XNURB * xnurb, const U32 clip)
{
	IIInt end_generated_point = generated_point_offset + generated_point_count;
	for(int vid = generated_point_offset; vid < end_generated_point; vid++)
	{
		vertex_pool[vid].r = (unsigned char)light_pool[vid].x;
		vertex_pool[vid].g = (unsigned char)light_pool[vid].y;
		vertex_pool[vid].b = (unsigned char)light_pool[vid].z;
		vertex_pool[vid].a = (unsigned char)mat.transparency;
	}

	int face_cnt;
	if(tessellation_type == IIGLOO_TRIANGLE_TESSELLATION)
	{
		face_cnt = index_count / 3;
		ASSERT(3 * face_cnt == (int)index_count);
#if 1
		IRP_CALL( draw_indexed_primitive( D3DPT_TRIANGLELIST, D3DFVF_RPVERTEX,
			vertex_pool, point_count, index_list, index_count, clip) )
#endif
#if 0	// wireframe
		//IRP_CALL ( set_render_state( D3DRS_CULLMODE, D3DCULL_NONE ) )
		IRP_CALL( set_render_state( D3DRS_LIGHTING,		FALSE ))
		IRP_CALL( set_render_state( D3DRS_ZENABLE,			TRUE ))
		IRP_CALL( set_render_state( D3DRS_ZWRITEENABLE,	TRUE ))
		IRP_CALL( set_render_state( D3DRS_ZFUNC, D3DCMP_ALWAYS ))
		IRP_CALL( set_texture_stage_texture( 0, 0 ))
		IRP_CALL( set_render_state( D3DRS_FILLMODE,	D3DFILL_WIREFRAME ))
		IRP_CALL( draw_indexed_primitive( D3DPT_TRIANGLELIST, D3DFVF_RPVERTEX,
			vertex_pool, point_count, index_list, index_count, clip) )
		IRP_CALL( set_render_state( D3DRS_FILLMODE,	D3DFILL_SOLID ))
		IRP_CALL( set_texture_stage_texture( 0, mat.texture_id ))
		IRP_CALL( set_render_state( D3DRS_ZFUNC, D3DCMP_LESS ))
		//IRP_CALL ( set_render_state( D3DRS_CULLMODE, D3DCULL_CCW ) )
#endif
	}
	else
	if(tessellation_type == IIGLOO_TRIANGLE_STRIP_TESSELLATION)
	{
		face_cnt = index_count - 2;
#if 1
		IRP_CALL( draw_indexed_primitive( D3DPT_TRIANGLESTRIP, D3DFVF_RPVERTEX,
			vertex_pool, point_count, index_list, index_count, clip) )
#endif
#if 0	// wireframe
		//IRP_CALL ( set_render_state( D3DRS_CULLMODE, D3DCULL_NONE ) )
		IRP_CALL( set_render_state( D3DRS_LIGHTING,		FALSE ))
		IRP_CALL( set_render_state( D3DRS_ZENABLE,			TRUE ))
		IRP_CALL( set_render_state( D3DRS_ZWRITEENABLE,	TRUE ))
		IRP_CALL( set_render_state( D3DRS_ZFUNC, D3DCMP_ALWAYS ))
		IRP_CALL( set_texture_stage_texture( 0, 0 ))
		IRP_CALL( set_render_state( D3DRS_FILLMODE,	D3DFILL_WIREFRAME ))
		IRP_CALL( draw_indexed_primitive( D3DPT_TRIANGLESTRIP, D3DFVF_RPVERTEX,
			vertex_pool, point_count, index_list, index_count, clip) )
		IRP_CALL( set_render_state( D3DRS_FILLMODE,	D3DFILL_SOLID ))
		IRP_CALL( set_texture_stage_texture( 0, mat.texture_id ))
		IRP_CALL( set_render_state( D3DRS_ZFUNC, D3DCMP_LESS ))
		//IRP_CALL ( set_render_state( D3DRS_CULLMODE, D3DCULL_CCW ) )
#endif
	}
	else
	{
		GENERAL_FATAL("IIGLOO unsupported tessellation type!\n");
	}

	// keep track of rendered faces/vertices
	xnurb->last_face_cnt += face_cnt;
	xnurb->last_vertex_cnt += point_count;
}

void NURBMESH::AssignDataToPolynomials(const NURBPatch & patch, const IIUInt row, const IIUInt column,
									   const int s_basis, const int t_basis, const int which)
{
	const int s_order = patch.s_order - LOWEST_ORDER;
	const int t_order = patch.t_order - LOWEST_ORDER;

	// LIGHT
	if(which & LIGHT_ON)
	{
		const int light_index =
			s_basis * (patch.s_order - 1) * patch.t_vertex_cnt + (patch.t_order - 1) * t_basis;
		r_polynom[s_order][t_order]->Set_Control_Data( &(light_points[light_index].r),
			patch.t_vertex_cnt * sizeof(f_LightRGB), sizeof(f_LightRGB));
		g_polynom[s_order][t_order]->Set_Control_Data( &(light_points[light_index].g),
			patch.t_vertex_cnt * sizeof(f_LightRGB), sizeof(f_LightRGB));
		b_polynom[s_order][t_order]->Set_Control_Data( &(light_points[light_index].b),
			patch.t_vertex_cnt * sizeof(f_LightRGB), sizeof(f_LightRGB));
	}
}

void NURBMESH::AssignPolynomialsToEvaluator(const NURBPatch & patch, const int s_basis, const int t_basis,
											const int which)
{
	//evaluator->Clear_Polynomials(); // craps out (this should work now, but happens automatically in Unlock_State so it is not necessary here. JWT)
	
	// XYZ
	evaluator->Set_Polynomial( IIGLOO_X, patch.x_polynom[s_basis * patch.t_basis_cnt + t_basis] );
	evaluator->Set_Polynomial( IIGLOO_Y, patch.y_polynom[s_basis * patch.t_basis_cnt + t_basis] );
	evaluator->Set_Polynomial( IIGLOO_Z, patch.z_polynom[s_basis * patch.t_basis_cnt + t_basis] );

#if RATIONAL_SUPPORT
	if(patch.weight_list)
	{
		ii_result( evaluator->Set_Polynomial( IIGLOO_W, patch.w_polynom[s_basis * patch.t_basis_cnt + t_basis]) );
	}
#endif

	// UV
	if((which & UV_ON) && patch.uv_list)
	{
		evaluator->Set_Polynomial( IIGLOO_TEXTURE_U, patch.u_polynom[s_basis * patch.t_basis_cnt + t_basis] );
		evaluator->Set_Polynomial( IIGLOO_TEXTURE_V, patch.v_polynom[s_basis * patch.t_basis_cnt + t_basis] );
	}

	// LIGHT
	if(which & LIGHT_ON)
	{	
		const int s_order = patch.s_order - LOWEST_ORDER;
		const int t_order = patch.t_order - LOWEST_ORDER;

		evaluator->Set_Polynomial( IIGLOO_R, r_polynom[s_order][t_order] );
		evaluator->Set_Polynomial( IIGLOO_G, g_polynom[s_order][t_order] );
		evaluator->Set_Polynomial( IIGLOO_B, b_polynom[s_order][t_order] );
	}
}

vis_state NURBMESH::CullPatch(const NURBPatch & patch, const Vector & cam_pos_in_object_space)
{
	vis_state result = VS_UNKNOWN;

	int visible_count = 0;
	for(int s = 0; s < patch.s_vertex_cnt; s++)
	{
		for(int t = 0; t < patch.t_vertex_cnt; t++)
		{
			/*
			// cull front side for debug
			const float dot = -dot_product(patch.normals[s * patch.t_vertex_cnt + t],
										   cam_pos_in_object_space);
			visible_vertex[s * patch.t_vertex_cnt + t] =
				(-dot < -patch.D_coefficient[s * patch.t_vertex_cnt + t]);
			*/
			 
			const float dot = -dot_product(patch.normals[s * patch.t_vertex_cnt + t],
										   cam_pos_in_object_space);
			visible_vertex[s * patch.t_vertex_cnt + t] =
				(dot < patch.D_coefficient[s * patch.t_vertex_cnt + t]);
	
			if(visible_vertex[s * patch.t_vertex_cnt + t])
			{
				visible_count++;
			}
		}
	}

	if(visible_count == 0)
	{
		result = VS_NOT_VISIBLE;
	}
	else
	if(visible_count == patch.s_vertex_cnt * patch.t_vertex_cnt)
	{
		result = VS_FULLY_VISIBLE;
	}
	else
	{
		result = VS_PARTIALLY_VISIBLE;
	}


	// mark visible sub patches
	for(s = 0; s < patch.s_basis_cnt; s++)
	{
		for(int t = 0; t < patch.t_basis_cnt; t++)
		{
			int sub_visible_count = 0;
			for(int ps = 0; ps < patch.s_order; ps++)
			{
				int cs = s * (patch.s_order - 1) + ps;
				for(int pt = 0; pt < patch.t_order; pt++)
				{
					int ct = t * (patch.t_order - 1) + pt;

					if(visible_vertex[cs * patch.t_vertex_cnt + ct])
					{
						sub_visible_count++;
					}
				}
			}

			if(sub_visible_count == 0)
			{
				visible_patch[s * patch.t_basis_cnt + t] = VS_NOT_VISIBLE;

			}
			else if(sub_visible_count == patch.s_order * patch.t_order)
			{
				visible_patch[s * patch.t_basis_cnt + t] = VS_FULLY_VISIBLE;
			}
			else
			{
				visible_patch[s * patch.t_basis_cnt + t] = VS_PARTIALLY_VISIBLE;
			}
		}
	}

#if 0	// render normals
	IRP_CALL( set_texture_stage_texture( 0, 0 ))
	IRP_CALL( set_render_state( D3DRS_ZENABLE,			TRUE ))
	IRP_CALL( set_render_state( D3DRS_ZWRITEENABLE,	TRUE ))
	IRP_CALL( set_render_state( D3DRS_ZFUNC, D3DCMP_ALWAYS ))
	
	Vector normal_end;
	const float scale = .15 * patch.vertices[0].magnitude();

	pb.Begin(PB_LINES);
	for(int s_pos = 0; s_pos < patch.s_vertex_cnt; s_pos++)
	{
		for(int t_pos = 0; t_pos < patch.t_vertex_cnt; t_pos++)
		{
			normal_end = patch.vertices[s_pos * patch.t_vertex_cnt + t_pos] +
						 scale * patch.normals[s_pos * patch.t_vertex_cnt + t_pos];
			pb.Color4ub(255, 255, 255, 255);
			pb.Vertex3fv((float*)&(patch.vertices[s_pos * patch.t_vertex_cnt + t_pos].x));
			pb.Color4ub(255, 0, 0, 255);
			pb.Vertex3fv((float*)&(normal_end.x));
		}
	}
	pb.End();

	IRP_CALL( set_render_state( D3DRS_ZFUNC, D3DCMP_LESS ))
#endif

	return result;
}

void NURBMESH::LightPatch( const Transform & light_world_to_object, const NURBPatch & patch,
									  const Material & mat, const ClampFlags light_clamp)
{
	LIGHTMAN->light_vertices_strided( &(tmp_lights[0]), sizeof(LightRGB),
									  &(patch.vertices[0]), sizeof(Vector),
									  &(patch.normals[0]), sizeof(Vector),
									  &(light_indices[0]), sizeof(U32),
									  patch.s_vertex_cnt * patch.t_vertex_cnt,
									  &light_world_to_object, light_clamp);

	
	const float er = mat.emission.r;
	const float eg = mat.emission.g;
	const float eb = mat.emission.b;
	const float dr = mat.diffuse.r * (1.0f / 255.0f);
	const float dg = mat.diffuse.g * (1.0f / 255.0f);
	const float db = mat.diffuse.b * (1.0f / 255.0f);
	f_LightRGB *tlp = light_points;

	if(light_clamp == CF_INTENSITY)
	{
		float max_c;
		for(const LightRGB *tl = tmp_lights; tl < tmp_lights + patch.s_vertex_cnt * patch.t_vertex_cnt; tl++, tlp++)
		{
			tlp->r = er + dr * tl->r;
			tlp->g = eg + dg * tl->g;
			tlp->b = eb + db * tl->b;

			if(255.0f < (max_c = Tmax(Tmax(tlp->r, tlp->g), tlp->b)))
			{
				max_c = 255.0f / max_c;
				tlp->r *= max_c;
				tlp->g *= max_c;
				tlp->b *= max_c;
			}
		}
	}else
	if(light_clamp == CF_COLOR)
	{
		for(const LightRGB *tl = tmp_lights; tl < tmp_lights + patch.s_vertex_cnt * patch.t_vertex_cnt; tl++, tlp++)
		{
			tlp->r = Tmin(er + dr * tl->r, 255.0f);
			tlp->g = Tmin(eg + dg * tl->g, 255.0f);
			tlp->b = Tmin(eb + db * tl->b, 255.0f);	
		}
	}
	
	// make all white
	/*
	for(s_pos = 0; s_pos < patch.s_vertex_cnt; s_pos++)
	{
		for(int t_pos = 0; t_pos < patch.t_vertex_cnt; t_pos++)
		{
			light_points[s_pos * patch.t_vertex_cnt + t_pos].r = (rand() * 255 ) / RANDTmax;
			light_points[s_pos * patch.t_vertex_cnt + t_pos].g = 192;
			light_points[s_pos * patch.t_vertex_cnt + t_pos].b = 192;
		}
	}
	*/
}


void NURBMESH::delete_pools( void )
{
	delete [] vertex_pool;
	vertex_pool = NULL;

	delete [] light_pool;
	light_pool = NULL;
}

void NURBMESH::verify_pools(int size)
{
	// Check that caches and pools are large enough.
	if (vertex_pool_len < size)
	{
		delete_pools();

		vertex_pool_len = size;
		vertex_pool = new RPVertex[vertex_pool_len];
		light_pool = new Vector[vertex_pool_len];
	}
}

void NURBMESH::delete_lists( void )
{
	delete [] index_list;
	index_list = NULL;
}

void NURBMESH::verify_lists(int size)
{
	// Check that caches and pools are large enough.
	if (index_list_len < size)
	{
		delete_lists();

		index_list_len = size;

		index_list = new U16[index_list_len];
	}
}

#pragma warning( pop )