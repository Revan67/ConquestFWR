/*
by Didier Badouel
from "Graphics Gems", Academic Press, 1990
Pg 393
*/

// V0, V1, & V2 are CW!!
// alpha from V0 to V1
// beta  from V0 to V2

// ray: r(t) = origin + dir * t;

struct Vector2
{
	float	x, y;

	Vector2(void){}

	Vector2(const float xx, const float yy)
		: x(xx), y(yy) {}

	inline void set(const float xx, const float yy)
	{
		x = xx;
		y = yy;
	}

	inline friend Vector2 operator - (const Vector2 & v1, const Vector2 & v2)
	{
		return Vector2(v1.x - v2.x, v1.y - v2.y);
	}
};

#include "vector.h"

inline float det2x2(const float a, const float b, const float c, const float d)
{
	return (a * d - b * c);
}

int RayPolygon( const Vector & ray_origin, const Vector & ray_dir,
			    const Vector tri[3], 
				Vector * const intersection,
				float * const alpha, float * const beta)

{
	Vector e1 ( tri[1] - tri[0] );
	Vector e2 ( tri[2] - tri[0] );
	Vector normal ( cross_product(e1, e2).normalize() );
	float d = dot_product( normal, tri[0] );

	float p_r_dot = dot_product( normal, ray_dir);

	// ray is parallel to plane so no intersection (maybe edge on but who cares)
	if(p_r_dot == 0.0f)
	{
		return 0;
	}

#ifdef FACE_FRONT_ONLY
	// face normal is is pointing away from ray
	if(p_r_dot > 0.0f)
	{
		return 0;
	}
#endif

	float t = -(d + dot_product( normal, ray_origin)) / p_r_dot;

	*intersection = ray_origin + t * ray_dir;

#ifdef RAY_FRONT_ONLY
	// intersection behind ray origin
	if(t <= 0) 
	{
		return 0;
	}
#endif

	Vector2 tri_2d[3];
	Vector2 inter_2d;
	//int i0, i1, i2;
	if(fabs(normal.x) > fabs(normal.y) && fabs(normal.x) > fabs(normal.z))
	{
		//i0 = 0; i1 = 1; i2 = 2;
		tri_2d[0].set(tri[0].y, tri[0].z);
		tri_2d[1].set(tri[1].y, tri[1].z);
		tri_2d[2].set(tri[2].y, tri[2].z);
		inter_2d.set(intersection->y, intersection->z);
	}
	else
	if(fabs(normal.y) > fabs(normal.x) && fabs(normal.y) > fabs(normal.z))
	{
		//i0 = 1; i1 = 2; i2 = 0;
		tri_2d[0].set(tri[0].x, tri[0].z);
		tri_2d[1].set(tri[1].x, tri[1].z);
		tri_2d[2].set(tri[2].x, tri[2].z);
		inter_2d.set(intersection->x, intersection->z);
	}
	else
	{
		//i0 = 2; i1 = 0; i2 = 1;
		tri_2d[0].set(tri[0].x, tri[0].y);
		tri_2d[1].set(tri[1].x, tri[1].y);
		tri_2d[2].set(tri[2].x, tri[2].y);
		inter_2d.set(intersection->x, intersection->y);
	}

	float u0 = inter_2d.x  - tri_2d[0].x;
	float u1 = tri_2d[1].x - tri_2d[0].x;
	float u2 = tri_2d[2].x - tri_2d[0].x;

	float v0 = inter_2d.y  - tri_2d[0].y;
	float v1 = tri_2d[1].y - tri_2d[0].y;
	float v2 = tri_2d[2].y - tri_2d[0].y;
	
	float denom = det2x2(u1, u2, v1, v2);
	if(denom == 0.0f)
	{
		return -1;
	}

	*alpha = det2x2(u0, u2, v0, v2) / denom;
	*beta  = det2x2(u1, u0, v1, v0) / denom;

	if(*alpha >= 0.0f && *beta >= 0.0f && *alpha + *beta <= 1.0f)
	{
		return 1; // inside
	}
	else
	{
		// *closest = _min(intersection w/ each of the 3 edges)
		// NOT using clamped *alpha & *beta

		return 0; // outside
	}

	// interpolation
	// V = (1 - (alpha + beta))*V0 + alpha*V1 + beta*V2

	// clamping
	// first clamp both alpha & beta individually from 0 to 1
	// if still alpha + beta > 1 then clamp beta (beta = 1 - alpha)
}


/* the value of t is computed.
 * i1 and i2 come from the polygon description.
 * V is the vertex table for the polygon and N the
 * associated normal vectors.
 */
/*
bool RayPolygon()
{
	P[0] = ray.O[0] + ray.D[0]*t;
	P[1] = ray.O[1] + ray.D[1]*t;
	P[2] = ray.O[2] + ray.D[2]*t;

	u0 = P[i1] - V[0][i1];
	v0 = P[i2] - V[0][i2];

	inter = FALSE;
	i = 2;
	do {
		// The polygon is viewed as (n-2) triangles.
		u1 = V[i-1][i1] - V[0][i1]; v1 = V[i-1][i2] - V[0][i2];
		u2 = V[i  ][i1] - V[0][i1]; v2 = V[i  ][i2] - V[0][i2];

		if (u1 == 0)
		{
			beta = u0/u2;
			if ((beta >= 0.)&&(beta <= 1.))
			{
				alpha = (v0 - beta*v2)/v1;
				inter = ((alpha >= 0.)&&(alpha+beta) <= 1.));
			}
		}
		else
		{
			beta = (v0*u1 - u0*v1)/(v2*u1 - u2*v1);
			if ((beta >= 0.)&&(beta <= 1.))
			{
				alpha = (u0 - beta*u2)/u1;
				inter = ((alpha >= 0)&&((alpha+beta) <= 1.));
			}
		}
	} while ((!inter) && (++i < poly.n));

	if (inter)
	{
		// Storing the intersection point.
		ray.P[0] = P[0]; ray.P[1] = P[1]; ray.P[2] = P[2];
		// the normal vector can be interpolated now or later.
		if (poly.interpolate)
		{
			gamma = 1 - (alpha+beta);
			ray.normal[0] = gamma * N[0][0] + alpha * N[i-1][0] +
			 beta * N[i][0];
			ray.normal[1] = gamma * N[0][1] + alpha * N[i-1][1] +
			 beta * N[i][1];
			ray.normal[2] = gamma * N[0][2] + alpha * N[i-1][2] +
			 beta * N[i][2];
		}
	}
	return (inter);
}
*/
