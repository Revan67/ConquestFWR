//--------------------------------------------------------------------------//
//                                                                          //
//                                 Extent.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/CQExtent.h 3     2/25/00 6:54p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef CQEXTENT_H
#define CQEXTENT_H

#ifndef COLLISION_H
#include <collision.h>
#endif

struct Ellipse : public GeometricPrimitive
{
	Vector center;
	SINGLE radius;
	Vector scale;
	Matrix R;

	Ellipse()
	{}

	Ellipse(const Vector & _center,const SINGLE _radius,const Vector &_scale,const Matrix &rot)
	{
		center = _center;
		radius = _radius;
		scale = _scale;
		R = rot;
	}

	void transform(GeometricPrimitive * dst, const Vector & trans, const Matrix & rot) const
	{
		Ellipse * ellipse = (Ellipse *) dst;
		ellipse->center = trans + rot * center;
		ellipse->radius = radius;
		ellipse->scale = scale;
		ellipse->R = rot;
	}
};

bool RayEllipse(Vector & p ,Vector & normal,const Vector & base, const Vector & dir, const struct Ellipse & ellipse);
//obsolete - phase out
bool RayEllipse(Vector & p, const Vector & base, const Vector & dir, const struct Ellipse & ellipse,Vector & normal);

#endif
//---------------------------------------------------------------------------
//--------------------------End Extent.h-------------------------------------
//---------------------------------------------------------------------------
