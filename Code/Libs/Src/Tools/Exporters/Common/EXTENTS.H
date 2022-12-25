#ifndef EXTENTS_H
#define EXTENTS_H

#include "matrix.h"
#include "vector.h"
#include "persistmath.h"
#include "cmesh.h"
#include "cylinder.h"


#ifndef SGI
#pragma warning( disable : 4244 ) 
// onversion from 'double' to 'float', possible loss of data
#endif
//---------------------------------------------------------------------------

#define MIN_MASS 1e-5f

struct MassData
{
	float	mass;
	Vector	center;
	Matrix	inertia;
};

//---------------------------------------------------------------------------

struct ExtentSphere
{
	char name[64];
	PersistTransform tr;
	Vector center; // relative to com
	Vector render_center; // relative to object
	float radius;
	float render_radius;  // duplicate so byte swaping doesn't get messed up
    float volume;

	void init (void);

	float DistSquared (const Vector &v1, const Vector &v2);

	void compute_bounds (int count, const Vector *list);
};

//---------------------------------------------------------------------------

struct ExtentBox
{
	char name[64];
	PersistTransform tr;
	Vector center;
	Vector size;
    float volume;
	bool adjusted;

	void compute_bounds (int count, const Vector *list);
	void compute_mass (MassData &data, float density=0.001, float set_mass=-1);
};

//---------------------------------------------------------------------------

struct Extents
{
	MassData	mass;

	ExtentSphere	sphere;
	ExtentBox		box;
    CollisionMesh	convex_hull;
    Cylinder		cylinder;

	void compute_sphere (int count, const Vector *list);

	void compute_box (int count, const Vector *list);

	void compute_cylinder (int count, const Vector *list);

	void compute_convex_hull (int count, const Vector *list);

	void compute_mass (float density=0.001, float set_mass=-1);
};

//---------------------------------------------------------------------------

#ifndef SGI
#pragma warning( default : 4244 ) 
// conversion from 'double' to 'float', possible loss of data
#endif

#endif // EXTENTS_H
