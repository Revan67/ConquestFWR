#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define MATERIAL_H_INCLUDE_LOAD_CODE 1

#include "FileSys_Utility.h"
#include "XNURB.h"
#include "NURBFileIO.h"
#include "ITextureLibrary.h"
#include "TSmartPointer.h"
#include "FDump.h"
#include "tempstr.h"
#include "nurbmesh.h"
#include "iigloo/iinurbscurve.h"
#include "iigloo/iigloo_util.h"

#pragma warning( push, 4 )

#ifndef NDEBUG
	#define CHECK(x)		\
	{						\
		int result = (x);	\
		ASSERT (result);	\
	}
#else								
	#define CHECK(x)	(void)(x);
#endif

bool XNURB::read (IFileSystem* parent, ITextureLibrary* txm_lib)
{
	ASSERT(parent);
	ASSERT(txm_lib);

	/*
	TrapFpu(true);

    // text file debug

	FILE *fp = fopen("c:\\export\\planet.txt", "r");

	patch_cnt = 1;
	patch_list = new NURBPatch[patch_cnt];

	NURBPatch & patch = patch_list[0];

	int s_closed;
	int t_closed;
	fscanf(fp, "%d", &s_closed);
	fscanf(fp, "%d", &t_closed);

	fscanf(fp, "%d", &patch.s_order);
	fscanf(fp, "%d", &patch.t_order);

	fscanf(fp, "%d", &patch.s_knot_cnt);
	fscanf(fp, "%d", &patch.t_knot_cnt);
	patch.s_knot_list = new float[patch.s_knot_cnt];
	patch.t_knot_list = new float[patch.t_knot_cnt];

	fscanf(fp, "%d", &patch.s_point_cnt);
	fscanf(fp, "%d", &patch.t_point_cnt);
	patch.point_list = new Vector[patch.s_point_cnt * patch.t_point_cnt];
	patch.weight_list = new float[patch.s_point_cnt * patch.t_point_cnt];

	patch.s_basis_cnt = patch.s_point_cnt - (patch.s_order - 1);
	patch.t_basis_cnt = patch.t_point_cnt - (patch.t_order - 1);
	// computed later by void NURBMESH::GenerateNormalsUVs(XNURB *xnurb)
	patch.s_vertex_cnt = patch.s_basis_cnt * (patch.s_order - 1) + 1;
	patch.t_vertex_cnt = patch.t_basis_cnt * (patch.t_order - 1) + 1;
	patch.normals = new Vector[patch.s_vertex_cnt * patch.t_vertex_cnt];
	patch.vertices = new Vector[patch.s_vertex_cnt * patch.t_vertex_cnt];
	patch.D_coefficient = new float[patch.s_vertex_cnt * patch.t_vertex_cnt];

	material_cnt = 1;
	material_list = new Material[material_cnt];
	patch.mtl_id = 0;

	int knot;
	for ( knot = 0; knot < patch.s_knot_cnt; knot++ )
	{
		fscanf(fp, "%f", &(patch.s_knot_list[knot]));
	}

	for ( knot = 0; knot < patch.t_knot_cnt; knot++ )
	{
		fscanf(fp, "%f", &(patch.t_knot_list[knot]));
	}

	for ( int row = 0; row < patch.s_point_cnt; row++ )
	{
		for ( int column = 0; column < patch.t_point_cnt; column++ )
		{
			int u, v;
			fscanf(fp, "%d %d %f %f %f %f",
				&u, &v, 
				&(patch.point_list[row * patch.t_point_cnt + column].x),
				&(patch.point_list[row * patch.t_point_cnt + column].y),
				&(patch.point_list[row * patch.t_point_cnt + column].z),
				&(patch.weight_list[row * patch.t_point_cnt + column]));
			
			patch.point_list[row * patch.t_point_cnt + column] *= .01f;
		}
	}

	fclose(fp);

	return true;
	*/


	COMPTR<IFileSystem> f1, f2, f3, f4;

	f1 = parent;

	{
		//
		// Read NURB Set
		//
		if (descend_read(f1, "NURB set", f2) == GR_OK)
		{
			read_val(f2, "Count",				&patch_cnt);
			patch_list = new NURBPatch[patch_cnt];

			NURBPatch * patch = patch_list;
			for (int i = 0; i < patch_cnt; i++, patch++)
			{
				memset(patch, 0, sizeof(*patch));

				char patch_name[32];
				sprintf(patch_name, "NURB%d", i);
				if (descend_read(f2, patch_name, f3) == GR_OK)
				{
					read_val(f3, "S order",			&patch->s_order);
					read_val(f3, "T order",			&patch->t_order);
					if(patch->s_order > HIGHEST_ORDER || patch->s_order < LOWEST_ORDER ||
					   patch->t_order > HIGHEST_ORDER || patch->t_order < LOWEST_ORDER)
					{
						char file_name[256];
						f1->GetFileName(file_name, 256);
						char message[256];
						sprintf(message, "In file %s %dx%d order NURBS are not yet supported!\n",
							file_name, patch->s_order, patch->t_order);
						GENERAL_ERROR(message);
						return false;
					}

					read_val(f3, "S knot count",	&patch->s_knot_cnt);
					read_val(f3, "T knot count",	&patch->t_knot_cnt);
					ASSERT_FATAL(patch->s_knot_cnt >= 2 * patch->s_order);
					ASSERT_FATAL(patch->t_knot_cnt >= 2 * patch->t_order);
					
					read_ptr(f3, "S knot list",		&patch->s_knot_list);
					read_ptr(f3, "T knot list",		&patch->t_knot_list);

					read_val(f3, "S point count",	&patch->s_point_cnt);
					read_val(f3, "T point count",	&patch->t_point_cnt);
					ASSERT_FATAL(patch->s_point_cnt >= patch->s_order);
					ASSERT_FATAL(patch->t_point_cnt >= patch->t_order);
					ASSERT_FATAL(patch->s_point_cnt == patch->s_knot_cnt - patch->s_order);
					ASSERT_FATAL(patch->t_point_cnt == patch->t_knot_cnt - patch->t_order);

					read_ptr(f3, "Point list",		&patch->point_list);
#if RATIONAL_SUPPORT
					read_ptr(f3, "Weight list",		&patch->weight_list); // optional
#endif
					if(patch->weight_list)
						rational_count++;
					else
						non_rational_count++;
		
					read_val(f3, "Material",		&patch->mtl_id);

					// read UV's
					read_val(f3, "S UV count",		&patch->u_cnt);
					read_val(f3, "T UV count",		&patch->v_cnt);
					// less if repeated knots
					ASSERT_FATAL(patch->u_cnt <= patch->s_point_cnt - patch->s_order + 2);
					ASSERT_FATAL(patch->v_cnt <= patch->t_point_cnt - patch->t_order + 2);

					read_ptr(f3, "UV list",			&patch->uv_list);
				}
				else
				{
					GENERAL_FATAL ("NURB patches missing!\n");
				}
			}
		}
	
	
		//emaurer:  load any textures that this object defines into the system-
		//wide texture database.  next, load any materials that this object
		//defines and connect them with the already loaded textures.

		//
		// Read optional texture library
		//

		txm_lib->load_library(f1,NULL);

		//
		// Read material library 
		//

		if (descend_read(f1, "Material library", f2) == GR_OK)
		{
			//
			// Get # of materials in this mesh's material list
			// (must be nonzero)
			//

			read_val(f2, "Material count", &material_cnt);

			ASSERT_FATAL(material_cnt);
			material_list = new Material[material_cnt];

			//
			// Walk material directories, creating each material in turn and
			// assigning it to a list entry based on its identifier value
			//

			HANDLE          search;
			WIN32_FIND_DATA found;

			search = f2->FindFirstFile("*.*", &found);

			if (search != INVALID_HANDLE_VALUE)
			{
				do
				{
				//
				// If this is a valid material directory (not "." or ".."), 
				// enter it and get the material's attributes
				//

					if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						continue;
					}

					if ((!strcmp(found.cFileName,".")) || 
						(!strcmp(found.cFileName,"..")))
					{
						continue;
					}
					
					if (descend_read(f2, found.cFileName, f3) == GR_OK)
					{
						int material_index;

						int r = read_val(f3, "Material identifier", &material_index);
						ASSERT_FATAL (r);

						ASSERT_FATAL (material_index < material_cnt);

						Material* cur = material_list + material_index;

						cur->initialize();
						cur->set_name( found.cFileName );
						cur->set_texture_library( txm_lib );
						cur->load_from_filesystem( f3 );
					}
				}
				while (f2->FindNextFile(search, &found));
			}

			f2->FindClose(search);
		}

		if (descend_read(f1, "Sphere", f2) == GR_OK)
		{
			read_val(f2, "Center", &sphere_center);
			read_val(f2, "Radius", &radius);

			ASSERT_FATAL(radius > 0.0f);
		}
		else
		{
			GENERAL_FATAL ("Render bounding Sphere missing in file!\n");
		}

		compute_bounds();
		compute_centroid();
	}
	
	GENERAL_TRACE_2(TEMPSTR("NURBMESH: Created %d patches, %d materials\n",
						patch_cnt,
						material_cnt));
	return true;
}

bool XNURB::InitBasis(void)
{
	for(int i=0; i < patch_cnt; i++)
	{
		NURBPatch & n_patch = patch_list[i];

		IINURBSCurve	NURBS_curve;

		// create the S parametric basis matrices using the IINURBS lib
		IIInt		s_matrix_count;
		IIFloat*	s_matrices;
		IIUInt*		s_point_steps;

		NURBS_curve.Set_Order( n_patch.s_order );
		NURBS_curve.Set_Knot_Vector( n_patch.s_knot_cnt, n_patch.s_knot_list );
		NURBS_curve.Create_Basis_Matrices( s_matrix_count, s_matrices, s_point_steps );
	
		// these are equal if no knots repeat
		if( s_matrix_count > n_patch.s_point_cnt - (n_patch.s_order - 1) )
		{
			return false;
		}
		n_patch.s_basis_cnt = s_matrix_count;

		
		// create the T parametric basis matrices using the IINURBS lib
		IIInt		t_matrix_count;
		IIFloat*	t_matrices;
		IIUInt*		t_point_steps;

		NURBS_curve.Set_Order( n_patch.t_order );
		NURBS_curve.Set_Knot_Vector( n_patch.t_knot_cnt, n_patch.t_knot_list );
		NURBS_curve.Create_Basis_Matrices( t_matrix_count, t_matrices, t_point_steps );

		// these are equal if no knots repeat
		if( t_matrix_count > n_patch.t_point_cnt - (n_patch.t_order - 1) )
		{
			return false;
		}
		n_patch.t_basis_cnt = t_matrix_count;


		n_patch.s_basis_list = new IIBasis * [n_patch.s_basis_cnt];
		n_patch.t_basis_list = new IIBasis * [n_patch.t_basis_cnt];


		for ( int b = 0; b < n_patch.s_basis_cnt; b++ )
		{
			HRESULT hr = ::CoCreateInstance( CLSID_IIBasis,
								NULL,
								CLSCTX_INPROC_SERVER,
								IID_IIBasis,
								(void**)(n_patch.s_basis_list + b));
			if ( FAILED(hr) )
			{
				return ( false );
			}
			else
			{
				n_patch.s_basis_list[b]->Set_Order(Order_From_Count(n_patch.s_order));
				n_patch.s_basis_list[b]->Set_Control_Point_Step( s_point_steps[b] );
				n_patch.s_basis_list[b]->Set_Coefficients( 
					&s_matrices[b * n_patch.s_order * n_patch.s_order], 0, 0 );
			}
		}

		
		for ( b = 0; b < n_patch.t_basis_cnt; b++ )
		{
			HRESULT hr = ::CoCreateInstance( CLSID_IIBasis,
								NULL,
								CLSCTX_INPROC_SERVER,
								IID_IIBasis,
								(void**)(n_patch.t_basis_list + b) );
			if ( FAILED(hr) )
			{
				return ( false );
			}
			else
			{
				n_patch.t_basis_list[b]->Set_Order(Order_From_Count(n_patch.t_order));
				n_patch.t_basis_list[b]->Set_Control_Point_Step( t_point_steps[b] );
				n_patch.t_basis_list[b]->Set_Coefficients( 
					&t_matrices[b * n_patch.t_order * n_patch.t_order], 0, 0 );
			}
		}

		delete [] s_matrices;
		delete [] s_point_steps;
		delete [] t_matrices;
		delete [] t_point_steps;

		/* might still be needed by copy archetype
		delete [] n_patch.s_knot_list;
		n_patch.s_knot_list = NULL;
		n_patch.s_knot_cnt = 0;
		delete [] n_patch.t_knot_list;
		n_patch.t_knot_list = NULL;
		n_patch.t_knot_cnt = 0;
		*/
	}

	return true;
}

bool XNURB::InitPolys(const PolynomialEnumData poly_data_user[],
					  const PolynomialEnumData & poly_data_linear)
{
	for(int i = 0; i < patch_cnt; i++)
	{
		NURBPatch & n_patch = patch_list[i];

		const PolynomialEnumData & poly_data = poly_data_user[(n_patch.s_order - LOWEST_ORDER) * NUM_ORDERS + n_patch.t_order - LOWEST_ORDER];

		if ( poly_data.poly_found )
		{
			n_patch.x_polynom = new IIPolynomial* [n_patch.s_basis_cnt * n_patch.t_basis_cnt];
			n_patch.y_polynom = new IIPolynomial* [n_patch.s_basis_cnt * n_patch.t_basis_cnt];
			n_patch.z_polynom = new IIPolynomial* [n_patch.s_basis_cnt * n_patch.t_basis_cnt];
#if RATIONAL_SUPPORT
			if(n_patch.weight_list)
			{
				n_patch.w_polynom = new IIPolynomial* [n_patch.s_basis_cnt * n_patch.t_basis_cnt];
			}
#endif

			for ( int s = 0; s < n_patch.s_basis_cnt; s++ )
			{
				for (int t = 0; t < n_patch.t_basis_cnt; t++ )
				{
					HRESULT hr = ::CoCreateInstance( poly_data.poly_id,
										NULL,
										CLSCTX_INPROC_SERVER,
										IID_IIPolynomial,
										(void**)&(n_patch.x_polynom[s * n_patch.t_basis_cnt + t]) );

					if ( FAILED(hr) )
					{
						return false;
					}

					hr = ::CoCreateInstance( poly_data.poly_id,
									NULL,
									CLSCTX_INPROC_SERVER,
									IID_IIPolynomial,
									(void**)&(n_patch.y_polynom[s * n_patch.t_basis_cnt + t]) );

					if ( FAILED(hr) )
					{
						return false;
					}

					hr = ::CoCreateInstance( poly_data.poly_id,
									NULL,
									CLSCTX_INPROC_SERVER,
									IID_IIPolynomial,
									(void**)&(n_patch.z_polynom[s * n_patch.t_basis_cnt + t]) );

					if ( FAILED(hr) )
					{
						return false;
					}
#if RATIONAL_SUPPORT
					if(n_patch.weight_list)
					{
						hr = ::CoCreateInstance( poly_data.poly_id,
										NULL,
										CLSCTX_INPROC_SERVER,
										IID_IIPolynomial,
										(void**)&(n_patch.w_polynom[s * n_patch.t_basis_cnt + t]) );

						if ( FAILED(hr) )
						{
							return false;
						}
					}
#endif
				}
			}
		}
		else
		{
			return false;
		}

		if(n_patch.uv_list)
		{
			if ( poly_data_linear.poly_found )
			{
				n_patch.u_polynom = new IIPolynomial* [n_patch.s_basis_cnt * n_patch.t_basis_cnt];
				n_patch.v_polynom = new IIPolynomial* [n_patch.s_basis_cnt * n_patch.t_basis_cnt];

				for ( int s = 0; s < n_patch.s_basis_cnt; s++ )
				{
					for (int t = 0; t < n_patch.t_basis_cnt; t++ )
					{

						HRESULT hr = ::CoCreateInstance( poly_data_linear.poly_id,
											NULL,
											CLSCTX_INPROC_SERVER,
											IID_IIPolynomial,
											(void**)&(n_patch.u_polynom[s * n_patch.t_basis_cnt + t]) );

						if ( FAILED(hr) )
						{
							return false;
						}

						hr = ::CoCreateInstance( poly_data_linear.poly_id,
											NULL,
											CLSCTX_INPROC_SERVER,
											IID_IIPolynomial,
											(void**)&(n_patch.v_polynom[s * n_patch.t_basis_cnt + t]) );

						if ( FAILED(hr) )
						{
							return false;
						}
					}
				}
			}
			else
			{
				return false;
			}
		}
	}

	return true;
}

bool XNURB::AssignBasis(void)
{
	for(int i = 0; i < patch_cnt; i++)
	{
		NURBPatch & n_patch = patch_list[i];

		for ( int s = 0; s < n_patch.s_basis_cnt; s++ )
		{
			for (int t = 0; t < n_patch.t_basis_cnt; t++ )
			{
				n_patch.x_polynom[s * n_patch.t_basis_cnt + t]->Set_S_Basis( n_patch.s_basis_list[s] );
				n_patch.x_polynom[s * n_patch.t_basis_cnt + t]->Set_T_Basis( n_patch.t_basis_list[t] );

				n_patch.y_polynom[s * n_patch.t_basis_cnt + t]->Set_S_Basis( n_patch.s_basis_list[s] );
				n_patch.y_polynom[s * n_patch.t_basis_cnt + t]->Set_T_Basis( n_patch.t_basis_list[t] );

				n_patch.z_polynom[s * n_patch.t_basis_cnt + t]->Set_S_Basis( n_patch.s_basis_list[s] );
				n_patch.z_polynom[s * n_patch.t_basis_cnt + t]->Set_T_Basis( n_patch.t_basis_list[t] );

#if RATIONAL_SUPPORT
				if(n_patch.weight_list)
				{
					n_patch.w_polynom[s * n_patch.t_basis_cnt + t]->Set_S_Basis( n_patch.s_basis_list[s] );
					n_patch.w_polynom[s * n_patch.t_basis_cnt + t]->Set_T_Basis( n_patch.t_basis_list[t] );
				}
#endif
			}
		}
	}

	return true;
}

bool XNURB::AssignData(void)
{
	for(int i = 0; i < patch_cnt; i++)
	{
		NURBPatch & n_patch = patch_list[i];

		IIUInt		row = 0;
		IIUInt		point_step;

		for ( int s = 0; s < n_patch.s_basis_cnt; s++ )
		{
			n_patch.s_basis_list[s]->Get_Control_Point_Step( &point_step );
			row += point_step; // point step is 0 for basis 0; 1 for non repeated knots; 
							   // 2 for twice repeated knot; ...
			
			IIUInt column = 0;
			for (int t = 0; t < n_patch.t_basis_cnt; t++ )
			{
				n_patch.t_basis_list[t]->Get_Control_Point_Step( &point_step );
				column += point_step;

				n_patch.x_polynom[s * n_patch.t_basis_cnt + t]->Set_Control_Data
				(
					&(n_patch.point_list[row * n_patch.t_point_cnt + column].x),
					n_patch.t_point_cnt * sizeof(Vector), sizeof(Vector) 
				);

				n_patch.y_polynom[s * n_patch.t_basis_cnt + t]->Set_Control_Data
				(
					&(n_patch.point_list[row * n_patch.t_point_cnt + column].y),
					n_patch.t_point_cnt * sizeof(Vector), sizeof(Vector) 
				);

				n_patch.z_polynom[s * n_patch.t_basis_cnt + t]->Set_Control_Data
				(
					&(n_patch.point_list[row * n_patch.t_point_cnt + column].z),
					n_patch.t_point_cnt * sizeof(Vector), sizeof(Vector) 
				);

#if RATIONAL_SUPPORT
				if(n_patch.weight_list)
				{
					n_patch.w_polynom[s * n_patch.t_basis_cnt + t]->Set_Control_Data
					(
						&(n_patch.weight_list[row * n_patch.t_point_cnt + column]),
						n_patch.t_point_cnt * sizeof(float), sizeof(float) 
					);
				}
#endif
			}
		}
	}

	return true;
}

bool XNURB::AssignUVData(void)
{
	for(int i = 0; i < patch_cnt; i++)
	{
		NURBPatch & n_patch = patch_list[i];

		if(n_patch.uv_list)
		{
			IIUInt		row = 0;
			IIUInt		point_step;

			for ( int s = 0; s < n_patch.s_basis_cnt; s++ )
			{
				n_patch.s_basis_list[s]->Get_Control_Point_Step( &point_step );
				row += point_step; // point step is 0 for basis 0; 1 for non repeated knots; 
								   // 2 for twice repeated knot; ...
				
				IIUInt column = 0;
				for (int t = 0; t < n_patch.t_basis_cnt; t++ )
				{
					n_patch.t_basis_list[t]->Get_Control_Point_Step( &point_step );
					column += point_step;

					n_patch.u_polynom[s * n_patch.t_basis_cnt + t]->Set_Control_Data
					(
						&(n_patch.uv_list[s * n_patch.v_cnt + t].u),
						n_patch.v_cnt * sizeof(Vector2), sizeof(Vector2)
					);

					n_patch.v_polynom[s * n_patch.t_basis_cnt + t]->Set_Control_Data
					(
						&(n_patch.uv_list[s * n_patch.v_cnt + t].v),
						n_patch.v_cnt * sizeof(Vector2), sizeof(Vector2)
					);
				}
			}
		}
	}

	return true;
}

#pragma warning( pop )