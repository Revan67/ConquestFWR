//--------------------------------------------------------------------------//
//                                                                          //
//                                 Extent.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/CQExtent.cpp 2     2/24/00 6:53p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include <Extent.h>
#include "CQExtent.h"

static float square(float n)
{
	return n * n;
}

bool RayEllipse(Vector & p ,Vector & normal,const Vector & base, const Vector & dir, const struct Ellipse & ellipse)
{
	bool result;

	Matrix inv = ellipse.R.get_inverse();
	Vector newDir = inv*dir;

	newDir.x *= ellipse.scale.x;
	newDir.y *= ellipse.scale.y;
	newDir.z *= ellipse.scale.z;
	
	Vector dx = base - ellipse.center;
	dx = inv*dx;
	dx.x *= ellipse.scale.x;
	dx.y *= ellipse.scale.y;
	dx.z *= ellipse.scale.z;

	float r_squared = square(ellipse.radius);

	float a = dot_product(newDir, newDir);
	float b = 2 * dot_product(newDir, dx);
	float c = dot_product(dx, dx) - r_squared;
	float disc = square(b) - 4.0 * a * c;
	if (disc >= 0)
	{
		float sqrt_disc = sqrt(disc);
		float one_over_2a = 0.5 * 1.0 / a;
		float t0 = (-b - sqrt_disc) * one_over_2a;
		float t1 = (-b + sqrt_disc) * one_over_2a;

	// Need smaller non-negative root.
		float t;
		if (t0 >= 0)
		{
			if (t1 >= 0)
			{
				t = __min(t0, t1);
			}
			else
			{
				t = t0;
			}
		}
		else if (t1 >= 0)
		{
			t = t1;
		}
		else 
		{
			t = -1;
		}

		if (t >= 0)
		{
			result = true;
			p = base + t * dir;
		}
		else
		{
			result = false;
		}
	}
	else
	{
	// No real roots, no intersection.
		result = false;
	}

	normal = ellipse.R*dx;
	return result;
}

bool RayEllipse(Vector & p, const Vector & base, const Vector & dir, const struct Ellipse & ellipse,Vector & normal)
{
	bool result;

	Matrix inv = ellipse.R.get_inverse();
	Vector newDir = inv*dir;

	newDir.x *= ellipse.scale.x;
	newDir.y *= ellipse.scale.y;
	newDir.z *= ellipse.scale.z;
	
	Vector dx = base - ellipse.center;
	dx = inv*dx;
	dx.x *= ellipse.scale.x;
	dx.y *= ellipse.scale.y;
	dx.z *= ellipse.scale.z;

	float r_squared = square(ellipse.radius);

	float a = dot_product(newDir, newDir);
	float b = 2 * dot_product(newDir, dx);
	float c = dot_product(dx, dx) - r_squared;
	float disc = square(b) - 4.0 * a * c;
	if (disc >= 0)
	{
		float sqrt_disc = sqrt(disc);
		float one_over_2a = 0.5 * 1.0 / a;
		float t0 = (-b - sqrt_disc) * one_over_2a;
		float t1 = (-b + sqrt_disc) * one_over_2a;

	// Need smaller non-negative root.
		float t;
		if (t0 >= 0)
		{
			if (t1 >= 0)
			{
				t = __min(t0, t1);
			}
			else
			{
				t = t0;
			}
		}
		else if (t1 >= 0)
		{
			t = t1;
		}
		else 
		{
			t = -1;
		}

		if (t >= 0)
		{
			result = true;
			p = base + t * dir;
		}
		else
		{
			result = false;
		}
	}
	else
	{
	// No real roots, no intersection.
		result = false;
	}

	normal = ellipse.R*dx;
	return result;
}
//---------------------------------------------------------------------------
//--------------------------End Extent.cpp-----------------------------------
//---------------------------------------------------------------------------
