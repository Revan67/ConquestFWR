#ifndef SGI
#pragma warning( 3 : 4100 ) // unreferenced formal parameter
#pragma warning( 3 : 4189 ) // local variable is initialized but not referenced
#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( error : 4700 )
#pragma warning( 3 : 4706 ) // assignment within conditional expression
#endif

#include "extents.h"
#include "minsphr.h"
#include "misc.h"

#ifndef SGI
#pragma warning( disable : 4244 ) 
// onversion from 'double' to 'float', possible loss of data
#endif



	void ExtentSphere::init (void)
	{
		center.set(0,0,0);
		radius = 0;
                volume = 0;
	}

	float ExtentSphere::DistSquared (const Vector &v1, const Vector &v2)
	{
		Vector v = v1 - v2;
		return dot_product(v,v);
	}

	void ExtentSphere::compute_bounds (int count, const Vector *list)
	{
		assert( count >= 3 );
		Sphere sphr = MinimalSphere (count, (Point3*)list);

		//int exponent;
		//frexp( sphr.r, &exponent );
		//if( sphr.r != FLT_MAX && is_float(sphr.r) && exponent != 128 && exponent != -1)
		if( sphr.r != FLT_MAX && is_float(sphr.r) )
		{
			// pad to avoid numerical accuracy problems
			radius = 
			render_radius = 1.00001f * sphr.r;
			center.x = sphr.x;
			center.y = sphr.y;
			center.z = sphr.z;
		}
		else // O(n^2)
		{
			fprintf(stderr, "Warning: ExtentSphere using backup method.\n"); 

			const Vector *v1, *v2;
			float max_dist;
			int i, j;

			max_dist = DistSquared(list[0], list[1]);
			v1 = &list[0];
			v2 = &list[1];
			for(j=0; j<count; j++){
				for(i=j; i<count; i++){
					if(DistSquared(list[j], list[i]) > max_dist){
						max_dist = DistSquared(list[j], list[i]);
						v1 = &list[j];
						v2 = &list[i];
					}
				}
			}

			Vector delta = *v2 - *v1;
			center = *v1 + 0.5 * delta;
			radius = 0.5 * delta.magnitude();
			float rad_sq = radius * radius;
			
			const Vector * vp = list;
			for (i = 0; i < count; i++, vp++)
			{
				delta = *vp - center;

				float old_sq = dot_product(delta, delta);
				if (old_sq > rad_sq)
				{
				//
				// Point is outside current bounding sphere, need to update.
				//
					// save last sphere
					Vector min_center = center;
					float min_rad = radius;

					// update
					float old = (float) sqrt(old_sq);
					radius = 0.5 * (radius + old);
					rad_sq = radius * radius;

					float offset = old - radius;
					center = (radius * center + offset * *vp) / old;

					// save new sphere
					Vector max_center = center;
					float max_rad = radius;

					// set min center to opposite side
					min_center = center + (center - min_center);

					// see if we can back up any
					for(j=0; j<15; j++)
					{
						Vector tmp_center = .5f * (min_center + max_center);
						float tmp_rad = .5f * (min_rad + max_rad);
						float tmp_rad_sq = tmp_rad * tmp_rad;

						int min_flag = 0;
						const Vector *tmp_vp = list;

						for(int k=0; k<i; k++, tmp_vp++)
						{
							Vector tmp_delta = *tmp_vp - tmp_center;
							float tmp_old_sq = dot_product(tmp_delta, tmp_delta);

							if(tmp_old_sq > tmp_rad_sq)
							{
								min_center = tmp_center;
								min_rad = tmp_rad;
								min_flag = 1;
								break;
							}
						}

						if(!min_flag)
						{
							max_center = tmp_center;
							max_rad = tmp_rad;
						}

					}

					radius = max_rad;
					rad_sq = radius*radius;
					center = max_center;
				}
			}

			// pad to avoid numerical accuracy problems
			radius *= 1.00001f;
			render_radius = radius;
		}

        volume = (4.0f/3.0f) * M_PI * radius*radius*radius;

		tr.identity();
		tr.v.x = center.x;
		tr.v.y = center.y;
		tr.v.z = center.z;

		//render_center = center;
	}
	


//---------------------------------------------------------------------------

	void ExtentBox::compute_bounds (int count, const Vector *list)
	{
		Vector min, max;

		min = list[0];
		max = list[0];

		for (int i=1; i<count; i++)
		{
			if (list[i].x < min.x) min.x = list[i].x;
			if (list[i].x > max.x) max.x = list[i].x;
			if (list[i].y < min.y) min.y = list[i].y;
			if (list[i].y > max.y) max.y = list[i].y;
			if (list[i].z < min.z) min.z = list[i].z;
			if (list[i].z > max.z) max.z = list[i].z;
		}

		center = (min + max) / 2.0;
		size = (max - min) / 2.0;

                // check & adjust for 0 thickness -ms
				adjusted = false;
                #define T_TOLERANCE .0001
                if(size.x<T_TOLERANCE)
                {
                    size.x = (size.y<size.z) ? size.y/100.0 : size.z/100.0;
					adjusted = true;
                }
                if(size.y<T_TOLERANCE)
                {
                    size.y = (size.x<size.z) ? size.x/100.0 : size.z/100.0;
					adjusted = true;
                }
                if(size.z<T_TOLERANCE)
                {
                    size.z = (size.x<size.y) ? size.x/100.0 : size.y/100.0;
					adjusted = true;
                }
                #undef T_TOLERANCE

		// pad to avoid numerical accuracy problems
		size *= 1.00001f;

        volume = 8.0f * size.x * size.y * size.z;

		tr.identity();
		tr.v.x = center.x;
		tr.v.y = center.y;
		tr.v.z = center.z;
	}

	void ExtentBox::compute_mass (MassData &data, float density/*=0.001*/, float set_mass/*=-1*/)
	{
	//
	// Compute mass properties of bounding block for now.
	//
		float xdim = size.x * 2;
		float ydim = size.y * 2;
		float zdim = size.z * 2;

		if (set_mass >= 0)
		{
			data.mass = set_mass;
		}
		else
		{
			data.mass = density * xdim * ydim * zdim;
			if(data.mass < MIN_MASS)
			{
				data.mass = MIN_MASS;
			}
		}


		float xs = xdim * xdim;
		float ys = ydim * ydim;
		float zs = zdim * zdim;

		float scale = data.mass / 12.0f;		// constant?
		float Ixx = scale * (ys + zs);
		float Iyy = scale * (xs + zs);
		float Izz = scale * (xs + ys);

		data.inertia.zero();
		data.inertia.d[0][0] = Ixx;
		data.inertia.d[1][1] = Iyy;
		data.inertia.d[2][2] = Izz;

		data.center.x = center.x;
		data.center.y = center.y;
		data.center.z = center.z;
	}

//---------------------------------------------------------------------------

	void Extents::compute_sphere (int count, const Vector *list)
	{
		sphere.compute_bounds(count, list);
	}

	void Extents::compute_box (int count, const Vector *list)
	{
		box.compute_bounds(count, list);
	}

	void Extents::compute_cylinder (int count, const Vector *list)
	{
		ComputeCylinder(&cylinder, list, count);
	}

	void Extents::compute_convex_hull (int count, const Vector *list)
	{
		ComputeConvexHull(convex_hull, list, count);
	}

	void Extents::compute_mass (float density/*=0.001*/, float set_mass/*=-1*/)
	{
		box.compute_mass(mass, density, set_mass);
	}

#ifndef SGI
#pragma warning( default : 4244 ) 
// conversion from 'double' to 'float', possible loss of data
#endif

