#ifndef COLLISION_H
#define COLLISION_H

//

#include "dacom.h"
#include "extent.h"

//

struct CollisionData
{
	const BaseExtent * e1;
	const BaseExtent * e2;

//
// Coefficient of restitution for collision, 0 <= coeff <= 1.
// 0 = inelastic collision, i.e. kinetic energy in direction of impact is lost
//     and objects stick together.
// 1 = perfectly elastic collision, no energy is lost and objects bounce off
//     each other like billiard balls.
// DEFAULTS to 0.5.
//
	float		coeff;

//
// mu is the coefficient of dynamic friction between 2 objects.
//
// Some sample numbers pulled from a freshman physics textbook:
//
// wood on wood:				0.2
// waxed skis on wet snow:		0.1
// ice on ice:					0.03
// lubricated metal on metal:	0.07
// unlubricated steel on steel:	0.3
// rubber on anything:			1
// 
// DEFAULTS to 0.5.
//
	float		mu;

	Vector		contact;		// contact point in world coordinates.
	Vector		normal;			// normal from e2 --> e1 in world coordinates.

	CollisionData(void)
	{
		e1 = e2 = NULL;
		coeff = mu = 0.5;
		contact.zero();
		normal.zero();
	}

	CollisionData(const BaseExtent * x1, const BaseExtent * x2)
	{
		e1 = x1;
		e2 = x2;
		coeff = mu = 0.5;
		contact.zero();
		normal.zero();
	}
};

//

struct CollisionStats
{
	int sphere_sphere;
	int sphere_box;
	int sphere_tube;
	int sphere_cylinder;
	int sphere_convexmesh;

	int box_box;
	int box_tube;
	int box_cylinder;
	int box_convexmesh;

	int tube_tube;
	int tube_cylinder;
	int tube_convexmesh;

	int cylinder_cylinder;
	int cylinder_convexmesh;

	int convexmesh_convexmesh;
};

//
#define IID_ICollision "ICollision"

struct ICollision : public IDAComponent
{
// Give ray origin & direction, return point of intersection with extent.
// Returns true if ray hits extent, false if not.

// CHECKS SINGLE EXTENT ONLY.
	virtual bool COMAPI intersect_ray_with_extent(	
									Vector & point_of_intersection, Vector & normal, 
									const Vector & ray_origin, const Vector & ray_direction, 
									const BaseExtent & object, 
									const Transform & obj_center_of_mass_frame) = 0;

//
// CHECKS EXTENT HIERARCHY rooted at root_extent. 
// If (find_closest == true), will check all leaf extents in the hierarchy to find intersection closest to ray origin.
// Otherwise, returns after finding any intersection.
//
	virtual bool COMAPI intersect_ray_with_extent_hierarchy(
									Vector & point_of_intersection, Vector & normal,
									const Vector & ray_origin, const Vector & ray_direction,
									const BaseExtent & root_extent, 
									const Transform & obj_center_of_mass_frame,
									bool find_closest = false) = 0;

//
// MOTIVATION for separating yes/no collision from contact determination:
//
// In some cases, determining whether or not two extents intersect is much cheaper than figuring
// out exactly how/where they intersect. Also, once an intersection is detected, it's often 
// necessary to step back in time and compute the closest points pre-intersection in order to 
// compute a decent collision response. 
//
// Returns simple yes/no. 
//
	virtual bool COMAPI collide_extents(const BaseExtent * root1, const Transform & T1,
										const BaseExtent * root2, const Transform & T2, float epsilon = 1e-3) = 0;

//
// Hierarchical version of above. Fills in "x1" and "x2" with leaf-level intersecting extents if any.
//
	virtual bool COMAPI collide_extent_hierarchies(	const BaseExtent *& x1, const BaseExtent *& x2, 
													const BaseExtent * root1, const Transform & T1,
													const BaseExtent * root2, const Transform & T2, float epsilon = 1e-3) = 0;

//
// Returns closest points between 2 extents, regardless of intersection.
//
	virtual void COMAPI compute_contact(CollisionData & data, 
										const BaseExtent * x1, const Transform & T1,
										const BaseExtent * x2, const Transform & T2) = 0;


//
// OLD-STYLE FUNCTIONS. Compute yes/no and contact info simultaneously.
//
	virtual bool COMAPI collide_extents(CollisionData & data, 
										const BaseExtent * extent1, const Transform & T1,
										const BaseExtent * extent2, const Transform & T2) = 0;

	virtual bool COMAPI collide_extent_hierarchies(	CollisionData & data, 
													const BaseExtent * root1, const Transform & T1,
													const BaseExtent * root2, const Transform & T2) = 0;

	virtual void COMAPI get_collision_stats(CollisionStats & stats) = 0;
};


//

#endif