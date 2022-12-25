#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <float.h>
#include <stdio.h>

#include "collision.h"
#include "XPrim.h"
#include "Debugprint.h"
#include "VBox.h"
#include "VCyl.h"
#include "Tfuncs.h"
#include "da_heap_utility.h"
#include "dacom.h"
#include "tcomponent.h"
#include "sysconsumerdesc.h"



HINSTANCE	hInstance;	// DLL instance handle
ICOManager *DACOM;		// Handle to component manager

const char *CLSID_Collision = "Collision";

static	float	*buffer1;
static	float	*buffer2;
static	int		buffer_length;

//
/*
  If we completely separate collision from contact, we break CQ and possibly FL. Old-style 
  usage should be preserved.

  usages:
	collide_extent_hierarchies - yes/no, where

	collide_extent_hierarchies - yes/no, which extent pair
	contact_extents - exact contact

  So within collision functions, we need 3 paths:
	1. Check intersection. If intersecting, compute closest points.
	2. Check intersection, report yes/no.
	3. Compute closest points regardless of intersection.

  Path #1 is old style. 
  This is complicated by the fact that some closest points computations
  fail miserably when the extents are intersecting.
*/
//

struct DACOM_NO_VTABLE Collision : public IAggregateComponent,
								   public ICollision
{
protected:
	BEGIN_DACOM_MAP_INBOUND(Collision)
	DACOM_INTERFACE_ENTRY(ICollision)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_ICollision, ICollision)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	END_DACOM_MAP()

public:
	Collision();
	~Collision();

	GENRESULT	init(DACOMDESC * desc)
	{
		return	GR_OK;
	}

//
// ICollision methods.
//
// Give ray origin & direction, return point of intersection with extent.
// Returns true if ray hits extent, false if not.

// CHECKS SINGLE EXTENT ONLY.
	virtual bool COMAPI intersect_ray_with_extent(	
									Vector & point_of_intersection, Vector & normal, 
									const Vector & ray_origin, const Vector & ray_direction, 
									const BaseExtent & object, 
									const Transform & obj_center_of_mass_frame);

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
									bool find_closest = false);

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
										const BaseExtent * root2, const Transform & T2, float epsilon = 1e-3);

//
// Hierarchical version of above. Fills in "x1" and "x2" with leaf-level intersecting extents if any.
//
	virtual bool COMAPI collide_extent_hierarchies(	const BaseExtent *& x1, const BaseExtent *& x2, 
													const BaseExtent * root1, const Transform & T1,
													const BaseExtent * root2, const Transform & T2, float epsilon = 1e-3);

//
// Returns closest points between 2 extents, regardless of intersection.
//
	virtual void COMAPI compute_contact(CollisionData & data, 
										const BaseExtent * x1, const Transform & T1,
										const BaseExtent * x2, const Transform & T2);


//
// OLD-STYLE FUNCTIONS. Compute yes/no and contact info simultaneously.
//
	virtual bool COMAPI collide_extents(CollisionData & data, 
										const BaseExtent * extent1, const Transform & T1,
										const BaseExtent * extent2, const Transform & T2);

	virtual bool COMAPI collide_extent_hierarchies(	CollisionData & data, 
													const BaseExtent * root1, const Transform & T1,
													const BaseExtent * root2, const Transform & T2);

	virtual void COMAPI get_collision_stats(CollisionStats & stats);

	bool recurse_collide_extents(const BaseExtent *& intersect1, const BaseExtent *& intersect2,
							   const BaseExtent * e1, const Transform & T1,
							   const BaseExtent * e2, const Transform & T2, float epsilon);

	bool recurse_collide_extents(CollisionData & data, 
							   const BaseExtent * e1, const Transform & T1,
							   const BaseExtent * e2, const Transform & T2);

	// IAggregateComponent
	virtual GENRESULT	COMAPI Initialize(void)
	{
		return GR_OK;
	};
};

//

Collision::Collision()
{
	buffer1 = NULL;
	buffer2 = NULL;
}

//

Collision::~Collision()
{
	delete[] buffer1;
	buffer1 = NULL;

	delete[] buffer2;
	buffer2 = NULL;
}

//

BOOL	COMAPI	DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	IComponentFactory	*server;
	
	switch(fdwReason)
	{
		//
		// DLL_PROCESS_ATTACH: Create object server component and register it 
		// with DACOM manager
		//
		case DLL_PROCESS_ATTACH:

			hInstance = hinstDLL;

			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);

			server = new DAComponentFactory2<DAComponentAggregate<Collision>, SYSCONSUMERDESC> (CLSID_Collision);

			if(!server)
			{
				break;
			}

			DACOM	=DACOM_Acquire();
			
			//
			// Register at object-renderer priority
			//

			if(DACOM)
			{
				DACOM->RegisterComponent(server, CLSID_Collision, DACOM_NORMAL_PRIORITY);
			}

			server->Release();
			break;

		//
		// DLL_PROCESS_DETACH: Release DACOM manager instance
		//
		case DLL_PROCESS_DETACH:

			if(DACOM)
			{
				DACOM->Release();
				DACOM	=NULL;
			}
			break;
	}
	return	TRUE;
}

//
/*
  If we completely separate collision from contact, we break CQ and possibly FL. Old-style 
  usage should be preserved.

  usages:
	collide_extent_hierarchies - yes/no, where

	collide_extent_hierarchies - yes/no, which extent pair
	contact_extents - exact contact

  So within collision functions, we need 3 paths:
	1. Check intersection. If intersecting, compute closest points.
	2. Check intersection, report yes/no.
	3. Compute closest points regardless of intersection.

  Path #1 is old style. 
  This is complicated by the fact that some closest points computations
  fail miserably when the extents are intersecting.
*/
//

const float DefaultEpsilon = 1e-3;

//

static char Work[256];
int CollisionCase = 0;

//

//
// UTIL functions:
//
inline float clampf(float & f, float min, float max)
{
	f = Tmin(max, Tmax(min, f));
	return f;
}

inline float square(float n)
{
	return n * n;
}

inline bool fequal(float f, float g, float tolerance)
{
	return (fabs(f - g) < tolerance);
}

inline float deg_to_rad(float deg)
{
	return deg * 3.14159f / 180.0f;
}

inline float rad_to_deg(float rad)
{
	return rad * 180.0f / 3.14159f;
}

//

void vclip(VMesh & b1, CollisionMesh * mesh1, 
		   VMesh & b2, CollisionMesh * mesh2, 
		   const Vector & r1, const Matrix & R1,
		   const Vector & r2, const Matrix & R2,
		   Vector &p1, Vector & p2, Vector & n);

//

//DEBUG
void DrawLine(const Vector & v0, const Vector & v1, int r, int g, int b)
{
}

//

bool FindClosestPoints(const CollisionMesh & p, const CollisionMesh & q, const Matrix & Rp, const Vector & rp, const Matrix & Rq, const Vector & rq, Vector & v1, Vector & v2, Vector & normal);

//
// Transform func table: In most cases we want to transform both primitives to 
// some common coordinate system (e.g. world space) to compute the intersection.
// In some cases, however, particularly those involving meshes, it makes more
// sense to transform the primitive in question into the mesh's frame, since it
// avoids transforming mesh vertices. Anyway, the type of transform used for 
// each primitive test is table-driven, as is the intersection test itself.
//

typedef void (*TransformFunc)(XForm & dst1, const XForm & src1, XForm & dst2, const XForm & src2);

void Both2World(XForm & dst1, const XForm & src1, XForm & dst2, const XForm & src2)
{
	dst1 = src1;
	dst2 = src2;
}

//

void First2Second(XForm & dst1, const XForm & src1, XForm & dst2, const XForm & src2)
{
	Matrix R2T = src2.R.get_transpose();
	dst1.x = R2T * (src1.x - src2.x);
	dst1.R = R2T * src1.R;

	dst2.x.zero();
	dst2.R.set_identity();
}

//

void Second2First(XForm & dst1, const XForm & src1, XForm & dst2, const XForm & src2)
{
	dst1.x.zero();
	dst1.R.set_identity();

	Matrix R1T = src1.R.get_transpose();
	dst2.x = R1T * (src2.x - src1.x);
	dst2.R = R1T * src2.R;
}

//
// TABLES OF TRANSFORM AND COLLISION FUNCTIONS ARE SYMMETRIC; no need to store entire table.
//

TransformFunc TFTable[8][8] =
{
	{Both2World,	Both2World,		Both2World,		First2Second,	First2Second,	First2Second,	First2Second,	Both2World},
	{Both2World,	Both2World,		Both2World,		Both2World,		First2Second,	First2Second,	First2Second,	Both2World},
	{Both2World,	Both2World,		Both2World,		First2Second,	First2Second, 	Both2World,		First2Second,	Both2World},
	{Second2First,	Both2World,		Second2First,	Both2World,		First2Second,	Both2World,		First2Second,	Second2First},
	{Second2First,	Second2First,	Second2First,	Second2First,	Second2First,	Both2World,		First2Second,	Second2First},
	{Second2First,	Second2First,	Both2World,		Both2World,		Both2World,		Both2World,		First2Second,	Second2First},
	{Second2First,	Second2First,	Second2First,	Second2First,	Second2First,	Second2First,	First2Second,	Second2First},
	{Both2World,	Both2World,		Both2World,		First2Second,	First2Second,	First2Second,	First2Second,	Both2World}
};

//
// Table-driven primitive intersection tests.
//
typedef bool (*CollisionFunc)(CollisionData &, 
							  const GeometricPrimitive *, const XForm &, 
							  const GeometricPrimitive *, const XForm &,
							  float epsilon, bool compute_contact);

//

bool Wrong(CollisionData &, const GeometricPrimitive *, const XForm &, const GeometricPrimitive *, const XForm &, float, bool)
{
//	DebugPrint("ICollision: Unsupported primitive pair.\n");
	return false;
}

#ifdef SUPPORT_LINES
bool LineLine		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool LinePlane		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool LineSphere		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool LineCylinder	 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool LineBox		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool LineMesh		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool LineTrilist	 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
#endif
bool PlaneSphere     (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool PlaneCylinder   (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool PlaneBox        (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool PlaneMesh       (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool PlaneTrilist    (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool SphereSphere    (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool SphereCylinder  (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool SphereBox       (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool SphereMesh      (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool SphereTrilist   (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool CylinderCylinder(CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool CylinderBox     (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool CylinderMesh    (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool CylinderTrilist (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool BoxBox          (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool BoxMesh         (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool BoxTrilist      (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool MeshMesh        (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool MeshTrilist     (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool TrilistTrilist  (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool LineTube		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool PlaneTube		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool SphereTube		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool CylinderTube	 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool BoxTube		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool MeshTube		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool TrilistTube	 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);
bool TubeTube		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact);

// ELLIPSOID STUFF NOT YET IMPLEMENTED.
bool PlaneEllipsoid	   (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact) {return false;}
bool SphereEllipsoid   (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact) {return false;}
bool CylinderEllipsoid (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact) {return false;} 
bool BoxEllipsoid      (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact) {return false;}
bool MeshEllipsoid     (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact) {return false;}
bool TrilistEllipsoid  (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact) {return false;}
bool TubeEllipsoid     (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact) {return false;}
bool EllipsoidEllipsoid(CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact) {return false;}

//

CollisionFunc CFTable[9][9] =
{
// LINES UNSUPPORTED.
	{Wrong,			Wrong,			Wrong,			Wrong,				Wrong,			Wrong,			Wrong,				Wrong,			Wrong},	
	{Wrong,			Wrong,			PlaneSphere,	PlaneCylinder,		PlaneBox,		PlaneMesh,		PlaneTrilist,		PlaneTube,		PlaneEllipsoid},
	{Wrong,			PlaneSphere,	SphereSphere,	SphereCylinder,		SphereBox,		SphereMesh,		SphereTrilist,		SphereTube,		SphereEllipsoid},
	{Wrong,			PlaneCylinder,	SphereCylinder,	CylinderCylinder,	CylinderBox,	CylinderMesh,	CylinderTrilist,	CylinderTube,	CylinderEllipsoid},
	{Wrong,			PlaneBox,		SphereBox,		CylinderBox,		BoxBox,			BoxMesh,		BoxTrilist,			BoxTube,		BoxEllipsoid},
	{Wrong,			PlaneMesh,		SphereMesh,		CylinderMesh,		BoxMesh,		MeshMesh,		MeshTrilist,		MeshTube,		MeshEllipsoid},
	{Wrong,			PlaneTrilist,	SphereTrilist,	CylinderTrilist,	BoxTrilist,		MeshTrilist,	TrilistTrilist,		TrilistTube,	TrilistEllipsoid},
	{Wrong,			PlaneTube,		SphereTube,		CylinderTube,		BoxTube,		MeshTube,		TrilistTube,		TubeTube,		TubeEllipsoid},
	{Wrong,			PlaneEllipsoid,	SphereEllipsoid,CylinderEllipsoid,	BoxEllipsoid,	MeshEllipsoid,	TrilistEllipsoid,	TubeEllipsoid,	EllipsoidEllipsoid}
};

//

CollisionMesh StdBox1, StdBox2, StdSphere, StdCyl1, StdCyl2, StdLine;
VBox * VBox1 = NULL;
VBox * VBox2 = NULL;
VCyl * VCyl1 = NULL;
VCyl * VCyl2 = NULL;

// Buffer for xforming vertices, etc.
int			x_length = 0;
Vector *	xformed = NULL;

int			itemp_size = 0;
int *		itemp = NULL;

bool StdMeshesReady = false;

//

void GenerateCylinder(CollisionMesh * mesh, float len, float radius);

//

void FreeStdMeshes(void)
{
	if (StdMeshesReady)
	{
		StdBox1.free();
		StdBox2.free();
		StdSphere.free();
		StdCyl1.free();
		StdCyl2.free();
		StdLine.free();
		StdMeshesReady = false;

		if (VBox1) delete VBox1;
		if (VBox2) delete VBox2;
		if (xformed) delete [] xformed;
		if (itemp) delete [] itemp;
		if (VCyl1) delete VCyl1;
		if (VCyl2) delete VCyl2;
	}
}

//

//BSPTree BoxBSP;

//


void InitStdMeshes(void)
{
	VBox1 = new VBox(1, 1, 1);
	VBox2 = new VBox(1, 1, 1);

	VCyl1 = new VCyl(1, 1);
	VCyl2 = new VCyl(1, 1);

	StdBox1.centroid.zero();
	StdBox2.centroid.zero();

	StdBox1.num_vertices = 8;
	StdBox1.vertices = new Vertex[8];
	StdBox1.num_triangles = 12;
	StdBox1.triangles = new Triangle[12];

// Make triangles, compute normals and edges.
	StdBox1.vertices[0].p.set( 1,  1,  1);
	StdBox1.vertices[1].p.set( 1, -1,  1);
	StdBox1.vertices[2].p.set(-1, -1,  1);
	StdBox1.vertices[3].p.set(-1,  1,  1);
	StdBox1.vertices[4].p.set( 1,  1, -1);
	StdBox1.vertices[5].p.set( 1, -1, -1);
	StdBox1.vertices[6].p.set(-1, -1, -1);
	StdBox1.vertices[7].p.set(-1,  1, -1);

	StdBox1.triangles[ 0].v[0] = 0;
	StdBox1.triangles[ 0].v[1] = 1;
	StdBox1.triangles[ 0].v[2] = 3;
	StdBox1.triangles[ 1].v[0] = 1;
	StdBox1.triangles[ 1].v[1] = 2;
	StdBox1.triangles[ 1].v[2] = 3;
	StdBox1.triangles[ 2].v[0] = 0;
	StdBox1.triangles[ 2].v[1] = 4;
	StdBox1.triangles[ 2].v[2] = 1;
	StdBox1.triangles[ 3].v[0] = 4;
	StdBox1.triangles[ 3].v[1] = 5;
	StdBox1.triangles[ 3].v[2] = 1;
	StdBox1.triangles[ 4].v[0] = 0;
	StdBox1.triangles[ 4].v[1] = 7;
	StdBox1.triangles[ 4].v[2] = 4;
	StdBox1.triangles[ 5].v[0] = 3;
	StdBox1.triangles[ 5].v[1] = 7;
	StdBox1.triangles[ 5].v[2] = 0;
	StdBox1.triangles[ 6].v[0] = 4;
	StdBox1.triangles[ 6].v[1] = 7;
	StdBox1.triangles[ 6].v[2] = 5;
	StdBox1.triangles[ 7].v[0] = 7;
	StdBox1.triangles[ 7].v[1] = 6;
	StdBox1.triangles[ 7].v[2] = 5;
	StdBox1.triangles[ 8].v[0] = 3;
	StdBox1.triangles[ 8].v[1] = 6;
	StdBox1.triangles[ 8].v[2] = 7;
	StdBox1.triangles[ 9].v[0] = 6;
	StdBox1.triangles[ 9].v[1] = 3;
	StdBox1.triangles[ 9].v[2] = 2;
	StdBox1.triangles[10].v[0] = 1;
	StdBox1.triangles[10].v[1] = 5;
	StdBox1.triangles[10].v[2] = 2;
	StdBox1.triangles[11].v[0] = 2;
	StdBox1.triangles[11].v[1] = 5;
	StdBox1.triangles[11].v[2] = 6;

	StdBox1.compute_edges();
	StdBox1.compute_normals();

// Fix normals, since averaging faces doesn't really work for a box.
	StdBox1.normals[StdBox1.vertices[0].n].set( 1.0f,  1.0f,  1.0f);
	StdBox1.normals[StdBox1.vertices[1].n].set( 1.0f, -1.0f,  1.0f);
	StdBox1.normals[StdBox1.vertices[2].n].set(-1.0f, -1.0f,  1.0f);
	StdBox1.normals[StdBox1.vertices[3].n].set(-1.0f,  1.0f,  1.0f);
	StdBox1.normals[StdBox1.vertices[4].n].set( 1.0f,  1.0f, -1.0f);
	StdBox1.normals[StdBox1.vertices[5].n].set( 1.0f, -1.0f, -1.0f);
	StdBox1.normals[StdBox1.vertices[6].n].set(-1.0f, -1.0f, -1.0f);
	StdBox1.normals[StdBox1.vertices[7].n].set(-1.0f,  1.0f, -1.0f);

	for (int i = 0; i < 8; i++)
	{
		StdBox1.normals[StdBox1.vertices[i].n].normalize();
	}

// COMPUTE BSP
//	BoxBSP.build(&StdBox1);

	StdBox2.num_vertices = 8;
	StdBox2.vertices = new Vertex[8];
	StdBox2.num_triangles = 12;
	StdBox2.triangles = new Triangle[12];

	memcpy(StdBox2.vertices, StdBox1.vertices, sizeof(Vertex) * 8);
	memcpy(StdBox2.triangles, StdBox1.triangles, sizeof(Triangle) * 12);

	StdBox2.num_normals = StdBox1.num_normals;
	StdBox2.normals = new Vector[StdBox2.num_normals];
	memcpy(StdBox2.normals, StdBox1.normals, sizeof(Vector) * StdBox2.num_normals);

	StdBox2.num_edges = StdBox1.num_edges;
	StdBox2.edges = new Edge[StdBox2.num_edges];
	memcpy(StdBox2.edges, StdBox1.edges, sizeof(Edge) * StdBox2.num_edges);

	StdSphere.num_vertices = 1;
	StdSphere.vertices = new Vertex;
	StdSphere.num_normals = 1;
	StdSphere.normals = new Vector;

	GenerateCylinder(&StdCyl1, 1, 1);
	GenerateCylinder(&StdCyl2, 1, 1);

	StdLine.num_vertices = 2;
	StdLine.vertices = new Vertex[2];
	StdLine.vertices[0].n = 0;
	StdLine.vertices[1].n = 1;
	StdLine.num_normals = 2;
	StdLine.normals = new Vector[2];
	StdLine.normals[0].set(1, 0, 0);	// these won't be used, but initialize them
	StdLine.normals[1].set(1, 0, 0);	// to something that won't blow up.

	StdMeshesReady = true;
}

//

static void Transform_to_4x4 (float m[16], const Transform &t)
{
        m[ 0] = t.d[0][0];
        m[ 1] = t.d[1][0];
        m[ 2] = t.d[2][0];
        m[ 3] = 0;

        m[ 4] = t.d[0][1];
        m[ 5] = t.d[1][1];
        m[ 6] = t.d[2][1];
        m[ 7] = 0;

        m[ 8] = t.d[0][2];
        m[ 9] = t.d[1][2];
        m[10] = t.d[2][2];
        m[11] = 0;

        m[12] = t.translation.x;
        m[13] = t.translation.y;
        m[14] = t.translation.z;
        m[15] = 1;
}

//

bool COMAPI Collision::collide_extents(	CollisionData & data, 
												const BaseExtent * extent1, 
												const Transform & T1,
												const BaseExtent * extent2, 
												const Transform & T2)
{
	bool result;

	if (extent1->type >= ET_LINE_SEGMENT && extent1->type < ET_NONE &&
		extent2->type >= ET_LINE_SEGMENT && extent2->type < ET_NONE)
	{
		const GeometricPrimitive * gp1 = extent1->get_primitive();
		const GeometricPrimitive * gp2 = extent2->get_primitive();

	// Take extent's transform into account:
		Transform X1 = T1.multiply(extent1->xform);
		Transform X2 = T2.multiply(extent2->xform);

		const Vector & p1 = X1.translation;
		const Matrix & R1 = X1;

		const Vector & p2 = X2.translation;
		const Matrix & R2 = X2;

		TransformFunc xform = TFTable[extent1->type][extent2->type];
		XForm src1(p1, R1);
		XForm src2(p2, R2);
		XForm dst1, dst2;
		xform(dst1, src1, dst2, src2);

		CollisionFunc func = CFTable[extent1->type][extent2->type];
	// Collision functions assume smaller comes in first.
		if (extent1->type <= extent2->type)
		{
			result = func(data, gp1, dst1, gp2, dst2, DefaultEpsilon, false);
			if (result)
			{
				func(data, gp1, dst1, gp2, dst2, DefaultEpsilon, true);
			}
		}
		else
		{
			result = func(data, gp2, dst2, gp1, dst1, DefaultEpsilon, false);
		// NORMAL ALWAYS POINTS FROM second extent to first, so reverse.
			if (result)
			{
				func(data, gp2, dst2, gp1, dst1, DefaultEpsilon, true);
				data.normal = -data.normal;
			}
		}

		if (result)
		{
			data.e1 = extent1;
			data.e2 = extent2;
			data.coeff = 0.5f;
			data.mu = 0.5f;
		/*
			float nmag = data.normal.magnitude();
			if (fabs(nmag - 1.0) > 1e-5)
			{
//				DebugPrint("ouch\n");
			}
		*/
			if (xform == First2Second)
			{
				data.contact = T2.rotate_translate(data.contact);
				data.normal = T2.rotate(data.normal);
			}
			else if (xform == Second2First)
			{
				data.contact = T1.rotate_translate(data.contact);
				data.normal = T1.rotate(data.normal);
			}
		}
	}
	else
	{
		result = false;
	}

	return result;
}

//

bool Collision::recurse_collide_extents(CollisionData & data, 
											   const BaseExtent * e1, const Transform & T1,
											   const BaseExtent * e2, const Transform & T2)
{
	bool result = false;

	const BaseExtent * x1 = e1;
	const BaseExtent * x2 = e2;

	while (!result && x1)
	{
		while (!result && x2)
		{
			if (collide_extents(data, x1, T1, x2, T2))
			{
			// deal.
				if (x1->is_leaf())
				{
					if (x2->is_leaf())
					{
						result = true;
						data.e1 = x1;
						data.e2 = x2;
						break;
					}
					else
					{
						x2 = x2->child;
					}
				}
				else if (x2->is_leaf())
				{
					x1 = x1->child;
				}
				else
				{
					x1 = x1->child;
					x2 = x2->child;
				}

				result = recurse_collide_extents(data, x1, T1, x2, T2);
			}

			x2 = x2->next;
		}

		x1 = x1->next;
	}

	return result;
}

//

bool COMAPI Collision::collide_extent_hierarchies(	CollisionData & data, 
															const BaseExtent * root1, const Transform & T1,
															const BaseExtent * root2, const Transform & T2)
{
	bool result = false;

	const BaseExtent * ex1 = root1;
	while (ex1)
	{
		const BaseExtent * ex2 = root2;
		while (ex2)
		{
			if (recurse_collide_extents(data, ex1, T1, ex2, T2))
			{
				result = true;
				break;
			}
			ex2 = ex2->next;
		}

		ex1 = ex1->next;
	}

	return result;
}

//
// GEOMETRIC INTERSECTION FUNCTIONS FOLLOW.
//
// All of which are either 1) common-sense geometry, 2) standard ray-tracing 
// lore, 3) stolen from Graphics Gems, or 4) some highly-specialized algorithm
// taken from an obscure academic paper.
//
//

const float TOLERANCE = 1e-5;

//

float MinLineLine (const LineSegment & line0, const LineSegment & line1, float & s, float & t)
{
	const float tolerance = 1e-5;

	Vector b0 = line0.p0;
	Vector m0 = line0.p1 - line0.p0;
	Vector b1 = line1.p0;
	Vector m1 = line1.p1 - line1.p0;

	Vector diff = b0 - b1;

	float A =  dot_product(m0, m0);
	float B = -dot_product(m0, m1);
	float C =  dot_product(m1, m1);
	float D =  dot_product(m0, diff);
	float E = -dot_product(m1, diff);
	float det = A * C - B * B;

	if (fabs(det) >= tolerance)
	{
	// line segments are not parallel
		s = (B*E-C*D)/det;
		t = (B*D-A*E)/det;
		
		if ( s >= 0.0f )
		{
			if ( s <= 1.0f )
			{
				if ( t >= 0.0f )
				{
					if ( t <= 1.0f )  // region 0
					{
						// minimum at two interiors
					}
					else  // region 3
					{
						t = 1.0f;
						s = -(B+D)/A;
						if ( s < 0.0f ) s = 0.0f; else if ( s > 1.0f ) s = 1.0f;
					}
				}
				else  // region 7
				{
					t = 0.0f;
					s = -D/A;
					if ( s < 0.0f ) s = 0.0f; else if ( s > 1.0f ) s = 1.0f;
				}
			}
			else
			{
				if ( t >= 0.0f )
				{
					if ( t <= 1.0f )  // region 1
					{
						s = 1.0f;
						t = -(B+E)/C;
						if ( t < 0.0f ) t = 0.0f; else if ( t > 1.0f ) t = 1.0f;
					}
					else  // region 2
					{
						if ( A+B+D > 0.0f )
						{
							t = 1.0f;
							s = -(B+D)/A;
							if ( s < 0.0f ) s = 0.0f;
						}
						else if ( B+C+E > 0.0f )
						{
							s = 1.0f;
							t = -(B+E)/C;
							if ( t < 0.0f ) t = 0.0f;
						}
						else
						{
							s = 1.0f;
							t = 1.0f;
						}
					}
				}
				else  // region 8
				{
					if ( A+D > 0.0f )
					{
						t = 0.0f;
						s = -D/A;
						if ( s < 0.0f ) s = 0.0f;
					}
					else if ( B+E < 0.0f )
					{
						s = 1.0f;
						t = -(B+E)/C;
						if ( t > 1.0f ) t = 1.0f;
					}
					else
					{
						s = 1.0f;
						t = 0.0f;
					}
				}
			}
		}
		else 
		{
			if ( t >= 0.0f )
			{
				if ( t <= 1.0f )  // region 5
				{
					s = 0.0f;
					t = -E/C;
					if ( t < 0.0f ) t = 0.0f; else if ( t > 1.0f ) t = 1.0f;
				}
				else  // region 4
				{
					if ( B+D < 0.0f )
					{
						t = 1.0f;
						s = -(B+D)/A;
						if ( s > 1.0f ) s = 1.0f;
					}
					else if ( C+E > 0.0f )
					{
						s = 0.0f;
						t = -E/C;
						if ( t < 0.0f ) t = 0.0f;
					}
					else
					{
						s = 0.0f;
						t = 1.0f;
					}
				}
			}
			else   // region 6
			{
				if ( D < 0.0f )
				{
					t = 0.0f;
					s = -D/A;
					if ( s > 1.0f ) s = 1.0f;
				}
				else if ( E < 0.0f )
				{
					s = 0.0f;
					t = -E/C;
					if ( t > 1.0f ) t = 1.0f;
				}
				else
				{
					s = 0.0f;
					t = 0.0f;
				}
			}
		}
	}
	else
	{
		// line segments are parallel
		double t0, t1;
		if ( B > 0.0f )  // t0 > t1
		{
			t0 = -D/A;
			if ( t0 <= 0.0f )
			{
				s = 0.0f;
				t = 0.0f;
			}
			else if ( t0 <= 1.0f )
			{
				s = t0;
				t = 0.0f;
			}
			else
			{
				t1 = -(B+D)/A;
				if ( t1 <= 1.0f )
				{
					s = 1.0f;
					t = (t0-1.0f)/(t0-t1);
				}
				else
				{
					s = 1.0f;
					t = 1.0f;
				}
			}
		}
		else  // t0 < t1
		{
			t1 = -(B+D)/A;
			if ( t1 <= 0.0f )
			{
				s = 0.0f;
				t = 1.0f;
			}
			else if ( t1 <= 1.0f )
			{
				s = t1;
				t = 1.0f;
			}
			else
			{
				t0 = -D/A;
				if ( t0 <= 1.0f )
				{
					s = 1.0f;
					t = (1.0f-t0)/(t1-t0);
				}
				else
				{
					s = 1.0f;
					t = 0.0f;
				}
			}
		}
	}

// I don't like this.
//	return sqrt(fabs(s*(A*s+B*t+2*D)+t*(B*s+C*t+2*E)+dot_product(diff,diff)));

	Vector p0 = b0 + s * m0;
	Vector p1 = b1 + t * m1;
	Vector dp = p0 - p1;
	float dist_squared = dot_product(dp, dp);
	float dist = sqrt(dist_squared);
	return dist;
}

//

void ClosestLineLine(const LineSegment & line0, const LineSegment & line1, float & s, Vector & p0, float & t, Vector & p1)
{
	const float tolerance = 1e-5;

	Vector b0 = line0.p0;
	Vector m0 = line0.p1 - line0.p0;
	Vector b1 = line1.p0;
	Vector m1 = line1.p1 - line1.p0;

	Vector diff = b0 - b1;

	float A =  dot_product(m0, m0);
	float B = -dot_product(m0, m1);
	float C =  dot_product(m1, m1);
	float D =  dot_product(m0, diff);
	float E = -dot_product(m1, diff);
	float det = A * C - B * B;

	if (fabs(det) >= tolerance)
	{
	// line segments are not parallel
		s = (B*E-C*D)/det;
		t = (B*D-A*E)/det;
		
		if ( s >= 0.0f )
		{
			if ( s <= 1.0f )
			{
				if ( t >= 0.0f )
				{
					if ( t <= 1.0f )  // region 0
					{
						// minimum at two interiors
					}
					else  // region 3
					{
						t = 1.0f;
						s = -(B+D)/A;
						if ( s < 0.0f ) s = 0.0f; else if ( s > 1.0f ) s = 1.0f;
					}
				}
				else  // region 7
				{
					t = 0.0f;
					s = -D/A;
					if ( s < 0.0f ) s = 0.0f; else if ( s > 1.0f ) s = 1.0f;
				}
			}
			else
			{
				if ( t >= 0.0f )
				{
					if ( t <= 1.0f )  // region 1
					{
						s = 1.0f;
						t = -(B+E)/C;
						if ( t < 0.0f ) t = 0.0f; else if ( t > 1.0f ) t = 1.0f;
					}
					else  // region 2
					{
						if ( A+B+D > 0.0f )
						{
							t = 1.0f;
							s = -(B+D)/A;
							if ( s < 0.0f ) s = 0.0f;
						}
						else if ( B+C+E > 0.0f )
						{
							s = 1.0f;
							t = -(B+E)/C;
							if ( t < 0.0f ) t = 0.0f;
						}
						else
						{
							s = 1.0f;
							t = 1.0f;
						}
					}
				}
				else  // region 8
				{
					if ( A+D > 0.0f )
					{
						t = 0.0f;
						s = -D/A;
						if ( s < 0.0f ) s = 0.0f;
					}
					else if ( B+E < 0.0f )
					{
						s = 1.0f;
						t = -(B+E)/C;
						if ( t > 1.0f ) t = 1.0f;
					}
					else
					{
						s = 1.0f;
						t = 0.0f;
					}
				}
			}
		}
		else 
		{
			if ( t >= 0.0f )
			{
				if ( t <= 1.0f )  // region 5
				{
					s = 0.0f;
					t = -E/C;
					if ( t < 0.0f ) t = 0.0f; else if ( t > 1.0f ) t = 1.0f;
				}
				else  // region 4
				{
					if ( B+D < 0.0f )
					{
						t = 1.0f;
						s = -(B+D)/A;
						if ( s > 1.0f ) s = 1.0f;
					}
					else if ( C+E > 0.0f )
					{
						s = 0.0f;
						t = -E/C;
						if ( t < 0.0f ) t = 0.0f;
					}
					else
					{
						s = 0.0f;
						t = 1.0f;
					}
				}
			}
			else   // region 6
			{
				if ( D < 0.0f )
				{
					t = 0.0f;
					s = -D/A;
					if ( s > 1.0f ) s = 1.0f;
				}
				else if ( E < 0.0f )
				{
					s = 0.0f;
					t = -E/C;
					if ( t > 1.0f ) t = 1.0f;
				}
				else
				{
					s = 0.0f;
					t = 0.0f;
				}
			}
		}
	}
	else
	{
		// line segments are parallel
		double t0, t1;
		if ( B > 0.0f )  // t0 > t1
		{
			t0 = -D/A;
			if ( t0 <= 0.0f )
			{
				s = 0.0f;
				t = 0.0f;
			}
			else if ( t0 <= 1.0f )
			{
				s = t0;
				t = 0.0f;
			}
			else
			{
				t1 = -(B+D)/A;
				if ( t1 <= 1.0f )
				{
					s = 1.0f;
					t = (t0-1.0f)/(t0-t1);
				}
				else
				{
					s = 1.0f;
					t = 1.0f;
				}
			}
		}
		else  // t0 < t1
		{
			t1 = -(B+D)/A;
			if ( t1 <= 0.0f )
			{
				s = 0.0f;
				t = 1.0f;
			}
			else if ( t1 <= 1.0f )
			{
				s = t1;
				t = 1.0f;
			}
			else
			{
				t0 = -D/A;
				if ( t0 <= 1.0f )
				{
					s = 1.0f;
					t = (1.0f-t0)/(t1-t0);
				}
				else
				{
					s = 1.0f;
					t = 0.0f;
				}
			}
		}
	}

	p0 = b0 + s * m0;
	p1 = b1 + t * m1;
}

//

void ClosestLineLine(Vector & p1, Vector & p2, const Vector & base1, const Vector & dir1, const Vector & base2, const Vector & dir2)
{
	Vector diff = base1 - base2;

	float A = dot_product(dir1, dir1);
	float B = -dot_product(dir1, dir2);
	float C = dot_product(dir2, dir2);
	float D = dot_product(dir1, diff);
	float E = -dot_product(dir2, diff);
	float det = A*C-B*B;

	float s, t;
	if (fabs(det) >= TOLERANCE)
	{
		// line segments are not parallel
		s = (B*E-C*D)/det;
		t = (B*D-A*E)/det;
		
		if ( s >= 0.0f )
		{
			if ( s <= 1.0f )
			{
				if ( t >= 0.0f )
				{
					if ( t <= 1.0f )  // region 0
					{
						// minimum at two interiors
					}
					else  // region 3
					{
						t = 1.0f;
						s = -(B+D)/A;
						if ( s < 0.0f ) s = 0.0f; else if ( s > 1.0f ) s = 1.0f;
					}
				}
				else  // region 7
				{
					t = 0.0f;
					s = -D/A;
					if ( s < 0.0f ) s = 0.0f; else if ( s > 1.0f ) s = 1.0f;
				}
			}
			else
			{
				if ( t >= 0.0f )
				{
					if ( t <= 1.0f )  // region 1
					{
						s = 1.0f;
						t = -(B+E)/C;
						if ( t < 0.0f ) t = 0.0f; else if ( t > 1.0f ) t = 1.0f;
					}
					else  // region 2
					{
						if ( A+B+D > 0 )
						{
							t = 1.0f;
							s = -(B+D)/A;
							if ( s < 0.0f ) s = 0.0f;
						}
						else if ( B+C+E > 0 )
						{
							s = 1.0f;
							t = -(B+E)/C;
							if ( t < 0.0f ) t = 0.0f;
						}
						else
						{
							s = 1.0f;
							t = 1.0f;
						}
					}
				}
				else  // region 8
				{
					if ( A+D > 0 )
					{
						t = 0.0f;
						s = -D/A;
						if ( s < 0.0f ) s = 0.0f;
					}
					else if ( B+E < 0 )
					{
						s = 1.0f;
						t = -(B+E)/C;
						if ( t > 1.0f ) t = 1.0f;
					}
					else
					{
						s = 1.0f;
						t = 0.0f;
					}
				}
			}
		}
		else 
		{
			if ( t >= 0.0f )
			{
				if ( t <= 1.0f )  // region 5
				{
					s = 0.0f;
					t = -E/C;
					if ( t < 0.0f ) t = 0.0f; else if ( t > 1.0f ) t = 1.0f;
				}
				else  // region 4
				{
					if ( B+D < 0 )
					{
						t = 1.0f;
						s = -(B+D)/A;
						if ( s > 1.0f ) s = 1.0f;
					}
					else if ( C+E > 0 )
					{
						s = 0.0f;
						t = -E/C;
						if ( t < 0.0f ) t = 0.0f;
					}
					else
					{
						s = 0.0f;
						t = 1.0f;
					}
				}
			}
			else   // region 6
			{
				if ( D < 0 )
				{
					t = 0.0f;
					s = -D/A;
					if ( s > 1.0f ) s = 1.0f;
				}
				else if ( E < 0 )
				{
					s = 0.0f;
					t = -E/C;
					if ( t > 1.0f ) t = 1.0f;
				}
				else
				{
					s = 0.0f;
					t = 0.0f;
				}
			}
		}
	}
	else
	{
// CHANGE THIS TO USE SUPERIOR OVERLAP CHECKING AS IN CylinderCylinder().

		// line segments are parallel
		double t0, t1;
		if ( B > 0.0f )  // t0 > t1
		{
			t0 = -D/A;
			if ( t0 <= 0.0f )
			{
				s = 0.0f;
				t = 0.0f;
			}
			else if ( t0 <= 1.0f )
			{
				s = t0;
				t = 0.0f;
			}
			else
			{
				t1 = -(B+D)/A;
				if ( t1 <= 1.0f )
				{
					s = 1.0f;
					t = (t0-1.0f)/(t0-t1);
				}
				else
				{
					s = 1.0f;
					t = 1.0f;
				}
			}
		}
		else  // t0 < t1
		{
			t1 = -(B+D)/A;
			if ( t1 <= 0.0f )
			{
				s = 0.0f;
				t = 1.0f;
			}
			else if ( t1 <= 1.0f )
			{
				s = t1;
				t = 1.0f;
			}
			else
			{
				t0 = -D/A;
				if ( t0 <= 1.0f )
				{
					s = 1.0f;
					t = (1.0f-t0)/(t1-t0);
				}
				else
				{
					s = 1.0f;
					t = 0.0f;
				}
			}
		}
	}

	p1 = base1 + s * dir1;
	p2 = base2 + t * dir2;

//	return sqrt(fabs(s*(A*s+B*t+2*D)+t*(B*s+C*t+2*E)+Dot(diff,diff)));
}

//
//
//
#ifdef SUPPORT_LINES
bool LineLine		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	XLineSegment seg1(*((LineSegment *) g1), x1);
	XLineSegment seg2(*((LineSegment *) g2), x2);

	Vector l1 = seg1.p1 - seg1.p0;
	Vector l2 = seg2.p1 - seg2.p0;

	Vector dl = seg1.p0 - seg2.p0;
	float a =  dot_product(l1, l1);
	float b = -dot_product(l1, l2);
	float c =  dot_product(l2, l2);
	float d =  dot_product(l1, dl);
	float e = -dot_product(l2, dl);

	float determinant = a * c - b * b;
	if (fabs(determinant) >= TOLERANCE)
	{
		float one_over_det = 1.0f / determinant;
		float r = (b * e - c * d) * one_over_det;
		float s = (b * d - a * e) * one_over_det;

		if (r >= -epsilon && r <= 1+epsilon && s >= -epsilon && s <= 1+epsilon)
		{
		// Closest points are on segments.
			Vector p1 = seg1.p0 + r * l1;
			Vector p2 = seg2.p0 + s * l2;

			if (compute_contact)
			{
				data.contact = p1;
			// We can't know the correct normal direction without knowing velocities, etc.
				data.normal = cross_product(l1, l2);
				data.normal.normalize();
			}
			else
			{
				Vector dp = p1 - p2;
	    		float dist = dot_product(dp, dp);
				result = (dist <= square(epsilon));
			}
		}
		else
		{
		// Lines intersect beyond segment endpoints.
			if (compute_contact)
			{
				r = Tmax(0, Tmin(r, 1));
				s = Tmax(0, Tmin(s, 1));
				Vector p1 = seg1.p0 + r * l1;
				Vector p2 = seg2.p0 + s * l2;
				data.contact = p1;
			// We can't know the correct normal direction without knowing velocities, etc.
				data.normal = cross_product(l1, l2);
				data.normal.normalize();
			}
			else
			{
				result = false;
			}
		}
	}
	else
	{
	// Lines are parallel, check overlap.
		result = false;
	}

	return result;
}

//

bool LinePlane		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	XLineSegment line(*((LineSegment *) g1), x1);
	XPlane plane(*((Plane *) g2), x2);

	Vector line_dir = line.p1 - line.p0;
	float denom = dot_product(line_dir, plane.N);
	if (fabs(denom) >= TOLERANCE)
	{
		float num = -plane.compute_distance(line.p0);
		float t = num / denom;

		if (compute_contact)
		{
			t = Tmax(0, Tmin(t, 1));
			data.contact = line.p0 + t * line_dir;
			data.normal = plane.N;
		}
		else
		{
			result = (t >= -epsilon && t <= 1 + epsilon);
		}
	}
	else
	{
	// Line is parallel to plane.
		if (compute_contact)
		{
		// use midpoint.
			data.contact = 0.5 * (line.p0 + line.p1);
			data.normal = plane.N;
		}
		else
		{
			float dist = plane.compute_distance(line.p0);
			result = (dist <= epsilon);
		}
	}

	return result;
}

//

bool LineSphere		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	XLineSegment line(*((LineSegment *) g1), x1);
	XSphere sphere(*((Sphere *) g2), x2);

	Vector dir = line.p1 - line.p0;

	float x0 = line.p0.x;
	float y0 = line.p0.y;
	float z0 = line.p0.z;

	float xc = sphere.center.x;
	float yc = sphere.center.y;
	float zc = sphere.center.z;

	float r_squared = square(sphere.radius);

	float a = dot_product(dir, dir);
	float b = 2.0f * (dir.x * (x0 - xc) + dir.y * (y0 - yc) + dir.z * (z0 - zc));
	float c = square(x0 - xc) + square(y0 - yc) + square(z0 - zc) - r_squared;

	float disc = square(b) - 4.0f * a * c;
	if (disc >= 0)
	{
		float sqrt_disc = sqrt(disc);
		float one_over_2a = 0.5 * 1.0f / a;
		float t0 = (-b - sqrt_disc) * one_over_2a;
		float t1 = (-b + sqrt_disc) * one_over_2a;

	// Need smaller non-negative root.
		float t;
		if (t0 >= 0)
		{
			if (t1 >= 0)
			{
				t = Tmin(t0, t1);
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

		if (compute_contact)
		{
			t = Tmax(0, Tmin(t, 1));
			data.contact = line.p0 + t * dir;
			data.normal = data.contact - sphere.center;
			data.normal.normalize();
		}
		else
		{
			result = (t >= -epsilon && t <= 1+epsilon);
			if (!result)
			{
			// See if both endpoints are in the sphere.
				Vector d0 = line.p0 - sphere.center;
				float dd0 = dot_product(d0, d0);
				if (dd0 <= r_squared)
				{
					Vector d1 = line.p1 - sphere.center;
					float dd1 = dot_product(d1, d1);
					if (dd1 <= r_squared)
					{
					// Both endpoints are contained in sphere.
						result = true;
					}
				}
			}
		}
	}
	else
	{
	// No real roots, no intersection with sphere surface.
		if (compute_contact)
		{
			Vector diff = sphere.center - line.p0;
			float t = dot_product(diff, dir) / dot_product(dir, dir);

			data.contact = line.p0 + t * dir;
			data.normal = data.contact - sphere.center;
			data.normal.normalize();
		}
		else
		{
			result = false;
		}
	}

	return result;
}

//

#define		SIDE	0		/* Object surface		*/
#define		BOT	1		/* Bottom end-cap surface	*/
#define		TOP	2		/* Top	  end-cap surface	*/

char * SideName[3] = 
{
	"side",
	"bottom",
	"top"
};

//

bool ClipObj(const Vector & base, const Vector & dir, const Plane & bottom, const Plane & top, float & obj_in, float & obj_out, int & surf_in, int & surf_out)
{
	surf_in = surf_out = SIDE;
	float in  = obj_in;
	float out = obj_out;

// Intersect the ray with the bottom end-cap plane.

	float dc = dot_product(bottom.N, dir);
	float dw = bottom.compute_distance(base);

	if (fabs(dc) < TOLERANCE) 
	{		
	// If parallel to bottom plane.
	    if (dw >= 0) 
		{
			return false;
		}		
	} 
	else 
	{
		float t  = - dw / dc;
		if (dc >= 0.0f) 
		{			    // If far plane
			if (t > in && t < out) 
			{ 
				out = t; 
				surf_out = BOT; 
			}
			if (t < in) 
			{
				return false;
			}
		} 
		else 
		{				    // If near plane
			if  (t > in && t < out)
			{
				in = t; 
				surf_in = BOT; 
			}
			if (t > out) 
			{
				return false;
			}
		}
	}

//	Intersect the ray with the top end-cap plane.

	dc = dot_product(top.N, dir);
	dw = top.compute_distance(base);

	if  (fabs(dc) < TOLERANCE) 
	{		/* If parallel to top plane	*/
		if (dw >= 0) 
		{
			return false;
		}
	} 
	else 
	{
	    float t = - dw / dc;
		if (dc >= 0) 
		{			    /* If far plane	*/
			if  (t > in && t < out) 
			{
				out = t; 
				surf_out = TOP; 
			}
			if (t < in) 
			{
				return false;
			}
		} 
		else 
		{				    // If near plane
			if (t > in && t < out) 
			{ 
				in = t; 
				surf_in  = TOP; 
			}
			if (t > out) 
			{
				return false;
			}
		}
	}

	obj_in	= in;
	obj_out = out;

	return (in < out);
}

//
// Line in cylinder's frame.
//
bool LineCylinder	 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	XLineSegment line(*((LineSegment *) g1), x1);
	XCylinder cyl(*((Cylinder *) g2), x2);

	Vector half_axis = 0.5 * cyl.length * cyl.axis;
	Vector t0_0 = cyl.center - half_axis;
	Vector t0_1 = cyl.center + half_axis;

	Vector t1_0 = line.p0;
	Vector t1_1 = line.p1;

	LineSegment cyl_seg(t0_0, t0_1);
	LineSegment line_seg(t1_0, t1_1);

	Vector p0, p1;
	float s, t;
	ClosestLineLine(cyl_seg, line_seg, s, p0, t, p1);
	Vector dp = p1 - p0;

	float dist_squared = dot_product(dp, dp);
	if ((dist_squared > square(cyl.radius + epsilon)) && !compute_contact)
	{
	// Contact not possible.
		result = false;
	}
	else
	{
		if (s == 0)
		{
			if (dp.z < 0)
			{
			// pointing out of bottom plane, check corner.
				Vector perp = cross_product(cyl.axis, dp);
				float pmag = perp.magnitude();
				if (pmag > TOLERANCE)
				{
					perp /= pmag;
					Vector corner(perp.y * cyl.radius, -perp.x * cyl.radius, 0);

					Vector l0 = t0_0 - corner;
					Vector l1 = t0_0 + corner;
					LineSegment ltemp(l0, l1);
					float u, v;
					float dist = MinLineLine(ltemp, line_seg, u, v);
					if (dist < epsilon || compute_contact)
					{
						result = true;
						if (compute_contact)
						{
							data.contact = l0 + u * (l1 - l0);
							Vector tp = t1_0 + v * (t1_1 - t1_0);
							data.normal = data.contact - tp;
							data.normal.normalize();
						}
					}
					else
					{
						result = false;
					}
				}
				else
				{
					//DebugPrint("parallel\n");
				}
			}
			else
			{
				result = true;
				if (compute_contact)
				{
					Vector cp = t0_0;
					Vector tp = t1_0 + t * (t1_1 - t1_0);
					Vector dp = cp - tp;
					dp.normalize();
					data.contact = tp;// + dp * tube.radius;
					data.normal = dp;
				}
			}
		}
		else if (s == 1)
		{
			if (dp.z > 0)
			{
			// pointing out of top plane.
				Vector perp = cross_product(cyl.axis, dp);
				float pmag = perp.magnitude();
				if (pmag > TOLERANCE)
				{
					perp /= pmag;
					Vector corner(perp.y * cyl.radius, -perp.x * cyl.radius, 0);

					Vector l0 = t0_1 - corner;
					Vector l1 = t0_1 + corner;
					LineSegment ltemp(l0, l1);
					float u, v;
					float dist = MinLineLine(ltemp, line_seg, u, v);
					if (dist < epsilon || compute_contact)
					{
						result = true;
						if (compute_contact)
						{
							data.contact = l0 + u * (l1 - l0);
							Vector tp = t1_0 + v * (t1_1 - t1_0);
							data.normal = data.contact - tp;
							data.normal.normalize();
						}
					}
					else
					{
						result = false;
					}
				}
				else
				{
					//DebugPrint("parallel\n");
				}
			}
			else
			{
				result = true;
				if (compute_contact)
				{
					Vector cp = t0_1;
					Vector tp = t1_0 + t * (t1_1 - t1_0);
					Vector dp = cp - tp;
					dp.normalize();
					data.contact = tp;// + dp * tube.radius;
					data.normal = dp;
				}
			}
		}
		else
		{
			result = true;
			if (compute_contact)
			{
				Vector cp = t0_0 + s * (t0_1 - t0_0);
				Vector tp = t1_0 + t * (t1_1 - t1_0);
				Vector dp = cp - tp;
				dp.normalize();
				data.contact = tp;// + dp * tube.radius;
				data.normal = dp;
			}
		}
	}

	return result;
}
#endif	// SUPPORT_LINES
//

void AdjustBox(CollisionMesh * box, float x, float y, float z)
{
	box->vertices[0].p.set( x,  y,  z);
	box->vertices[1].p.set( x, -y,  z);
	box->vertices[2].p.set(-x, -y,  z);
	box->vertices[3].p.set(-x,  y,  z);
	box->vertices[4].p.set( x,  y, -z);
	box->vertices[5].p.set( x, -y, -z);
	box->vertices[6].p.set(-x, -y, -z);
	box->vertices[7].p.set(-x,  y, -z);
}

#ifdef SUPPORT_LINES
//
// Assumption: line transformed into box's frame.
//
bool LineBox		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result = false;

	XLineSegment line(*((LineSegment *) g1), x1);
	XBox box(*((Box *) g2), x2);

	Vector t0 = line.p0;
	Vector t1 = line.p1;

// Project tube ends onto box axes; if outside all 3 axes, no intersection.
	float xcheck = box.half_x + epsilon;
	if ((t0.x > xcheck && t1.x > xcheck) || (t0.x < -xcheck && t1.x < -xcheck))
	{
		result = false;
	}
	else
	{
		float ycheck = box.half_y + epsilon;
		if ((t0.y > ycheck && t1.y > ycheck) || (t0.y < -ycheck && t1.y < -ycheck))
		{
			result = false;
		}
		else
		{
			float zcheck = box.half_z + epsilon;
			if ((t0.z > zcheck && t1.z > zcheck) || (t0.z < -zcheck && t1.z < -zcheck))
			{
				result = false;
			}
			else
			{
			// Damn, passed trivial rejection, now we have to do some real work.
				if (!StdMeshesReady)
				{
					InitStdMeshes();
				}

				AdjustBox(&StdBox1, box.half_x, box.half_y, box.half_z);
				StdLine.vertices[0].p = t0;
				StdLine.vertices[1].p = t1;
				Vector p0, p1, N;
				FindClosestPoints(StdBox1, StdLine, x1.R, x1.x, x1.R, x1.x, p0, p1, N);

				Vector dp = p0 - p1;

				if (compute_contact)
				{
					float bx = box.half_x * 0.9;
					float by = box.half_y * 0.9;
					float bz = box.half_z * 0.9;
					data.normal.zero();
					const Vector & box_point = p1;
					if (box_point.x < -bx)
					{
						data.normal.x = -1;
					}
					else if (box_point.x > bx)
					{
						data.normal.x = 1;
					}
					if (box_point.y < -by)
					{
						data.normal.y = -1;
					}
					else if (box_point.y > by)
					{
						data.normal.y = 1;
					}
					if (box_point.z < -bz)
					{
						data.normal.z = -1;
					}
					else if (box_point.z > bz)
					{
						data.normal.z = 1;
					}
					data.normal.normalize();

					Vector axis = t1 - t0;
					float amag = axis.magnitude();
					float cos_theta = dot_product(axis / amag, data.normal);

					const float cos_tol = cos(deg_to_rad(89));

					if (fabs(cos_theta) < cos_tol)
					{
					// perpendicular to plane.
						data.contact = t0 + 0.5 * axis - epsilon * N;
					}
					else
					{
						data.contact = p0;
					}
				}
				else
				{
					float p_squared = dot_product(dp, dp);
					result = (p_squared <= square(epsilon));
				}
			}
		}
	}
	
	return result;
}

//

bool LineMesh		 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	XLineSegment line(*((LineSegment *) g1), x1);
	CollisionMesh * mesh = (CollisionMesh *) g2;

	Vector dl = line.p1 - line.p0;
	float llen = dl.magnitude();
	Vector org = line.p0;
	Vector dir = dl / llen;

	float tnear = -FLT_MAX;
	float tfar = llen;
	int front_normal, back_normal;

	result = true;

	Triangle * tri = mesh->triangles;
	float * D = mesh->triangle_d;
	for (int i = 0; i < mesh->num_triangles; i++, tri++, D++)
	{
		Vector * N = mesh->normals + tri->normal;
		float vd = dot_product(dir, *N);
		float vn = dot_product(org, *N) + *D;

		if (fabs(vd) < TOLERANCE)
		{
		// Line is parallel to plane.
			if (vn > 0)
			{
			// Origin is outside.
				result = false;
				break;
			}
		}
		else
		{
			float t = -vn / vd;
			if (vd < 0)
			{
				if (t > tfar)
				{
					result = false;
					break;
				}

				if (t > tnear)
				{
					front_normal = tri->normal;
					tnear = t;
				}
			}
			else
			{
				if (t < tnear)
				{
					result = false;
					break;
				}
				if (t < tfar)
				{
					back_normal = tri->normal;
					tfar = t;
				}
			}
		}
	}


	if (result)
	{
		if (tnear > 0)
		{
			if (compute_contact)
			{
				data.contact = org + dir * tnear;
				data.normal = mesh->normals[front_normal];
			}
		}
		else
		{
			if (tfar > 0 && tfar < llen)
			{
				if (compute_contact)
				{
					data.contact = org + dir * tfar;
					data.normal = mesh->normals[back_normal];
				}
			}
			else
			{
				result = false;
			}
		}
	}

	return result;
}
#endif // SUPPORT_LINES

//

float MinPointTriangle(const Vector & p, const Vector & b, const Vector & e0, const Vector & e1, float & s, float & t)
{
	Vector diff = b - p;
	float A = dot_product(e0, e0);
	float B = dot_product(e0, e1);
	float C = dot_product(e1, e1);
	float D = dot_product(e0, diff);
	float E = dot_product(e1, diff);
	float F = dot_product(diff,diff);
	float det = A*C-B*B;  // ASSERT:  det != 0 for triangles

	s = (B*E-C*D)/det;
	t = (B*D-A*E)/det;

	if ( s+t <= 1.0f )
	{
		if ( s < 0.0f )
		{
			if ( t < 0.0f )  // region 4
			{
				if ( D < 0 )
				{
					t = 0.0f;
					s = -D/A;
					if ( s > 1.0f ) s = 1.0f;
				}
				else if ( E < 0 )
				{
					s = 0.0f;
					t = -E/C;
					if ( t > 1.0f ) t = 1.0f;
				}
				else
				{
					s = 0.0f;
					t = 0.0f;
				}
			}
			else  // region 3
			{
				s = 0.0f;
				t = -E/C;
				if ( t < 0.0f ) t = 0.0f; else if ( t > 1.0f ) t = 1.0f;
			}
		}
		else if ( t < 0.0f )  // region 5
		{
			t = 0.0f;
			s = -D/A;
			if ( s < 0.0f ) s = 0.0f; else if ( s > 1.0f ) s = 1.0f;
		}
		else  // region 0
		{
			// minimum at interior point
		}
	}
	else
	{
		if ( s < 0.0f )  // region 2
		{
			if ( B-C+D-E < 0.0f )
			{
				s = -(B-C+D-E)/(A-2*B+C);
				if ( s < 0.0f ) s = 0.0f; else if ( s > 1.0f ) s = 1.0f;
				t = 1.0f-s;
			}
			else if ( C+E > 0.0f )
			{
				s = 0.0f;
				t = -E/C;
				if ( t < 0.0f ) t = 0.0f; else if ( t > 1.0f ) t = 1.0f;
			}
			else
			{
				s = 0.0f;
				t = 1.0f;
			}
		}
		else if ( t < 0.0f )  // region 6
		{
			if ( A-B+D-E > 0.0f )
			{
				t = (A-B+D-E)/(A-2*B+C);
				if ( t < 0.0f ) t = 0.0f; else if ( t > 1.0f ) t = 1.0f;
				s = 1.0f-t;
			}
			else if ( A+D > 0.0f )
			{
				t = 0.0f;
				s = -D/A;
				if ( s < 0.0f ) s = 0.0f; else if ( s > 1.0f ) s = 1.0f;
			}
			else
			{
				s = 1.0f;
				t = 0.0f;
			}
		}
		else  // region 1
		{
			s = -(B-C+D-E)/(A-2*B+C);
			if ( s < 0.0f ) s = 0.0f; else if ( s > 1.0f ) s = 1.0f;
			t = 1.0f-s;
		}
	}

	return sqrt(fabs(s*(A*s+B*t+2*D)+t*(B*s+C*t+2*E)+F));
}

//

float MinLineTriangle(const LineSegment & line, const Vector & b0, const Vector & e0, const Vector & e1, float & r, float & s, float &t)
{
	Vector lbase = line.p0;
	Vector ldir = line.p1 - line.p0;

	Vector diff = b0 - lbase;
	float A00 = +dot_product(ldir, ldir);
	float A01 = -dot_product(ldir, e0);
	float A02 = -dot_product(ldir, e1);
	float A11 = +dot_product(e0, e0);
	float A12 = +dot_product(e0, e1);
	float A22 = +dot_product(e1, e1);
	float B0  = -dot_product(diff, ldir);
	float B1  = +dot_product(diff, e0);
	float B2  = +dot_product(diff, e1);
	float cof00 = A11 * A22 - A12 * A12;
	float cof01 = A02 * A12 - A01 * A22;
	float cof02 = A01 * A12 - A02 * A11;
	float det = A00 * cof00 + A01 * cof01 + A02 * cof02;

	LineSegment triline;
	float min, min0, r0, s0, t0;

	if (fabs(det) >= TOLERANCE)
	{
		float cof11 = A00 * A22 - A02 * A02;
		float cof12 = A02 * A01 - A00 * A12;
		float cof22 = A00 * A11 - A01 * A01;
		float rhs0 = -B0 / det;
		float rhs1 = -B1 / det;
		float rhs2 = -B2 / det;

		r = cof00 * rhs0 + cof01 * rhs1 + cof02 * rhs2;
		s = cof01 * rhs0 + cof11 * rhs1 + cof12 * rhs2;
		t = cof02 * rhs0 + cof12 * rhs1 + cof22 * rhs2;

		if (r < 0)
		{
			if (s + t <= 1)
			{
				if (s < 0)
				{
					if (t < 0)  // region 4m
					{
					// min on face s=0 or t=0 or r=0
						triline.p0 = b0;
						triline.p1 = b0 + e1;

						min = MinLineLine(line, triline, r, t);
						s = 0.0f;
						triline.p0 = b0;
						triline.p1 = b0 + e0;
						min0 = MinLineLine(line, triline, r0, s0);
						t0 = 0.0f;
						if ( min0 < min )
						{
							min = min0;
							r = r0;
							s = s0;
							t = t0;
						}
						min0 = MinPointTriangle(line.p0, b0, e0, e1, s0, t0);
						r0 = 0.0f;
						if ( min0 < min )
						{
							min = min0;
							r = r0;
							s = s0;
							t = t0;
						}
					}
					else  // region 3m
					{
						// min on face s=0 or r=0
						triline.p0 = b0;
						triline.p1 = b0 + e1;
						min = MinLineLine(line, triline, r, t);
						s = 0.0f;
						min0 = MinPointTriangle(line.p0, b0, e0, e1, s0, t0);
						r0 = 0.0f;
						if ( min0 < min )
						{
							min = min0;
							r = r0;
							s = s0;
							t = t0;
						}
					}
				}
				else if ( t < 0.0f )  // region 5m
				{
					// min on face t=0 or r=0
					triline.p0 = b0;
					triline.p1 = b0 + e0;
					min = MinLineLine(line, triline, r, s);
					t = 0.0f;
					min0 = MinPointTriangle(line.p0, b0, e0, e1, s0, t0);
					r0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else  // region 0m
				{
					// min face on r=0
					min = MinPointTriangle(line.p0, b0, e0, e1, s, t);
					r = 0.0f;
				}
			}
			else
			{
				if ( s < 0.0f )  // region 2m
				{
					// min on face s=0 or s+t=1 or r=0
					triline.p0 = b0;
					triline.p1 = b0 + e1;
					min = MinLineLine(line, triline, r, t);
					s = 0.0f;
					triline.p0 = b0 + e0;
					triline.p1 = b0 + e1;
					min0 = MinLineLine(line, triline, r0, t0);
					s0 = 1.0f-t0;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
					min0 = MinPointTriangle(line.p0, b0, e0, e1, s0, t0);
					r0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else if ( t < 0.0f )  // region 6m
				{
					// min on face t=0 or s+t=1 or r=0
					triline.p0 = b0;
					triline.p1 = b0 + e0;
					min = MinLineLine(line, triline, r, s);
					t = 0.0f;
					triline.p0 = b0 + e0;
					triline.p1 = b0 + e1;
					min0 = MinLineLine(line, triline, r0, t0);
					s0 = 1.0f-t0;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
					min0 = MinPointTriangle(line.p0, b0, e0, e1, s0, t0);
					r0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else  // region 1m
				{
					// min on face s+t=1 or r=0
					triline.p0 = b0 + e0;
					triline.p1 = b0 + e1;
					min = MinLineLine(line, triline, r, t);
					s = 1.0f-t;
					min0 = MinPointTriangle(line.p0, b0, e0, e1, s0, t0);
					r0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
			}
		}
		else if ( r <= 1.0f )
		{
			if ( s+t <= 1.0f )
			{
				if ( s < 0.0f )
				{
					if ( t < 0.0f )  // region 4
					{
						// min on face s=0 or t=0
						triline.p0 = b0;
						triline.p1 = b0 + e1;
						min = MinLineLine(line, triline, r, t);
						s = 0.0f;
						triline.p0 = b0;
						triline.p1 = b0 + e0;
						min0 = MinLineLine(line, triline, r0, s0);
						t0 = 0.0f;
						if ( min0 < min )
						{
							min = min0;
							r = r0;
							s = s0;
							t = t0;
						}
					}
					else  // region 3
					{
						// min on face s=0
						triline.p0 = b0;
						triline.p1 = b0 + e1;
						min = MinLineLine(line, triline, r, t);
						s = 0.0f;
					}
				}
				else if ( t < 0.0f )  // region 5
				{
					// min on face t=0
					triline.p0 = b0;
					triline.p1 = b0 + e0;
					min = MinLineLine(line, triline, r, s);
					t = 0.0f;
				}
				else  // region 0
				{
					// global minimum is interior, done
					Vector pl = lbase + r * ldir;
					Vector pt = b0 + s * e0 + t * e1;
					Vector dp = pl - pt;
					float dsquared = dot_product(dp, dp);
					float dist = sqrt(dsquared);
				min = dist;
				/*
					min = sqrt(fabs(r*(A00*r+A01*s+A02*t+2.0f*B0)
						  +s*(A01*r+A11*s+A12*t+2.0f*B1)
						  +t*(A02*r+A12*s+A22*t+2.0f*B2)
						  +dot_product(diff,diff)));
				*/
				}
			}
			else
			{
				if ( s < 0.0f )  // region 2
				{
					// min on face s=0 or s+t=1
					triline.p0 = b0;
					triline.p1 = b0 + e1;
					min = MinLineLine(line, triline, r, t);
					s = 0.0f;
					triline.p0 = b0 + e0;
					triline.p1 = b0 + e1;
					min0 = MinLineLine(line, triline, r0, t0);
					s0 = 1.0f-t0;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else if ( t < 0.0f )  // region 6
				{
					// min on face t=0 or s+t=1
					triline.p0 = b0;
					triline.p1 = b0 + e0;
					min = MinLineLine(line, triline, r, s);
					t = 0.0f;
					triline.p0 = b0 + e0;
					triline.p1 = b0 + e1;
					min0 = MinLineLine(line, triline, r0, t0);
					s0 = 1.0f-t0;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else  // region 1
				{
					// min on face s+t=1
					triline.p0 = b0 + e0;
					triline.p1 = b0 + e1;
					min = MinLineLine(line, triline, r, t);
					s = 1.0f-t;
				}
			}
		}
		else  // r > 1.0f
		{
			if ( s+t <= 1.0f )
			{
				if ( s < 0.0f )
				{
					if ( t < 0.0f )  // region 4p
					{
						// min on face s=0 or t=0 or r=1
						triline.p0 = b0;
						triline.p1 = b0 + e1;
						min = MinLineLine(line, triline, r, t);
						s = 0.0f;
						triline.p0 = b0;
						triline.p1 = b0 + e0;
						min0 = MinLineLine(line, triline, r0, s0);
						t0 = 0.0f;
						if ( min0 < min )
						{
							min = min0;
							r = r0;
							s = s0;
							t = t0;
						}
						min0 = MinPointTriangle(line.p1, b0, e0, e1, s0, t0);
						r0 = 1.0f;
						if ( min0 < min )
						{
							min = min0;
							r = r0;
							s = s0;
							t = t0;
						}
					}
					else  // region 3p
					{
						// min on face s=0 or r=1
						triline.p0 = b0;
						triline.p1 = b0 + e1;
						min = MinLineLine(line, triline, r, t);
						s = 0.0f;
						min0 = MinPointTriangle(line.p1, b0, e0, e1, s0, t0);
						r0 = 1.0f;
						if ( min0 < min )
						{
							min = min0;
							r = r0;
							s = s0;
							t = t0;
						}
					}
				}
				else if ( t < 0.0f )  // region 5p
				{
					// min on face t=0 or r=1
					triline.p0 = b0;
					triline.p1 = b0 + e0;
					min = MinLineLine(line, triline, r, s);
					t = 0.0f;
					min0 = MinPointTriangle(line.p1, b0, e0, e1, s0, t0);
					r0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else  // region 0p
				{
					// min face on r=1
					min = MinPointTriangle(line.p1, b0, e0, e1, s, t);
					r = 1.0f;
				}
			}
			else
			{
				if ( s < 0.0f )  // region 2p
				{
					// min on face s=0 or s+t=1 or r=1
					triline.p0 = b0;
					triline.p1 = b0 + e1;
					min = MinLineLine(line, triline, r, t);
					s = 0.0f;
					triline.p0 = b0 + e0;
					triline.p1 = b0 + e1;
					min0 = MinLineLine(line, triline, r0, t0);
					s0 = 1.0f-t0;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
					min0 = MinPointTriangle(line.p1, b0, e0, e1, s0, t0);
					r0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else if ( t < 0.0f )  // region 6p
				{
					// min on face t=0 or s+t=1 or r=1
					triline.p0 = b0;
					triline.p1 = b0 + e0;
					min = MinLineLine(line, triline, r, s);
					t = 0.0f;
					triline.p0 = b0 + e0;
					triline.p1 = b0 + e1;
					min0 = MinLineLine(line, triline, r0, t0);
					s0 = 1.0f-t0;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
					min0 = MinPointTriangle(line.p1, b0, e0, e1, s0, t0);
					r0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else  // region 1p
				{
					// min on face s+t=1 or r=1
					triline.p0 = b0 + e0;
					triline.p1 = b0 + e1;
					min = MinLineLine(line, triline, r, t);
					s = 1.0f-t;
					min0 = MinPointTriangle(line.p1, b0, e0, e1, s0, t0);
					r0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
			}
		}
	}
	else
	{
		// line and triangle are parallel
		triline.p0 = b0;
		triline.p1 = b0 + e0;
		min = MinLineLine(line, triline, r, s);
		t = 0.0f;

		triline.p1 = b0 + e1;
		min0 = MinLineLine(line, triline, r0, t0);
		s0 = 0.0f;
		if ( min0 < min )
		{
			min = min0;
			r = r0;
			s = s0;
			t = t0;
		}

		triline.p0 += e0;
		min0 = MinLineLine(line, triline, r0, t0);
		s0 = 1.0f-t0;
		if ( min0 < min )
		{
			min = min0;
			r = r0;
			s = s0;
			t = t0;
		}

		min0 = MinPointTriangle(line.p0, b0, e0, e1, s0, t0);
		r0 = 0.0f;
		if ( min0 < min )
		{
			min = min0;
			r = r0;
			s = s0;
			t = t0;
		}

		min0 = MinPointTriangle(line.p1, b0, e0, e1, s0, t0);
		r0 = 1.0f;
		if ( min0 < min )
		{
			min = min0;
			r = r0;
			s = s0;
			t = t0;
		}
	}

	return min;
}

//

struct TriCollision
{
	Triangle *	tri1;
	Triangle *	tri2;
	Vector		pt;
};

//

static TriCollision tricol[128];

//
#ifdef SUPPORT_LINES
bool LineTrilist	 (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	XLineSegment line(*((LineSegment *) g1), x1);
	CollisionMesh * mesh = (CollisionMesh *) g2;

	Vector lbase = line.p0;
	Vector ldir = line.p1 - line.p0;

	float global_min = FLT_MAX;
	int num_tri_cols = 0;

	const float tolerance = 1e-5;

	Triangle * tri = mesh->triangles;
	for (int i = 0; i < mesh->num_triangles; i++, tri++)
	{
		Vector v0 = mesh->vertices[tri->v[0]].p;
		Vector v1 = mesh->vertices[tri->v[1]].p;
		Vector v2 = mesh->vertices[tri->v[2]].p;

		Vector b0 = v0;
		Vector e0 = v1 - v0;
		Vector e1 = v2 - v0;

		float r, s, t;
		float min = MinLineTriangle(line, b0, e0, e1, r, s, t);

		float dd = fabs(global_min - min);
		if (dd <= tolerance)
		{
			tricol[num_tri_cols].tri1 = tri;
			tricol[num_tri_cols].pt = lbase + r * ldir;
			num_tri_cols++;
		}
		else if (min < global_min)		
		{
			global_min = min;
			tricol[0].tri1 = tri;
			tricol[0].pt = lbase + r * ldir;
			num_tri_cols = 1;
		}
	}

	if (global_min < tolerance)
	{
		result = true;
		if (compute_contact)
		{
			Vector closest(0, 0, 0);
			Vector normal(0, 0, 0);
			for (i = 0; i < num_tri_cols; i++)
			{
				closest += tricol[i].pt;
				normal += mesh->normals[tricol[i].tri1->normal];
			}

			closest /= num_tri_cols;
			normal.normalize();

			data.contact = closest;
			data.normal = normal;
		}
	}
	else
	{
		result = false;
	}

	return result;
}
#endif
//

bool PlaneSphere     (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	XPlane p(*((Plane *) g1), x1);
	XSphere s(*((Sphere *) g2), x2);

	float dist = p.compute_distance(s.center);

	if (compute_contact)
	{
		data.contact = s.center - Tmin(dist, s.radius) * p.N;
		data.normal = -p.N;
	}
	else
	{
		result = (dist <= s.radius + epsilon);
	}

	return result;
}

//
// Sounds simple, doesn't it.
//
bool PlaneCylinder   (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result = false;

// If both cylinder endpoints are more than cylinder.radius away from the plane, no
// collision is possible. This is by far the easiest case involving cylinders.

	XPlane plane(*((Plane *) g1), x1);
	XCylinder cyl(*((Cylinder *) g2), x2);

	float half_len = cyl.length * 0.5;
	Vector axis = cyl.axis * half_len;
	Vector e0 = cyl.center + axis;
	Vector e1 = cyl.center - axis;

	float d0 = dot_product(plane.N, e0) + plane.D;
	float d1 = dot_product(plane.N, e1) + plane.D;

	if (compute_contact)
	{
		float dd = fabs(d0 - d1);
		if (dd < TOLERANCE)
		{
		// endpoints are same distance from plane.
			data.contact = cyl.center - cyl.radius * plane.N;
			data.normal = -plane.N;
		}
		else
		{
		// Compute corners closest to plane.
			Vector perp = cross_product(plane.N, cyl.axis);
			perp.normalize();

		// Rotate perpendicular toward plane.
			Quaternion q(cyl.axis, -3.14159 * 0.5);
			Vector toward = cyl.radius * q.transform(perp);

			if (d0 < d1)
			{
				data.contact = e0 + toward;
			}
			else if (d1 < d0)
			{
				data.contact = e1 + toward;
			}
			else
			{
				data.contact = 0.5 * (e0 + e1) + toward;
			}

			data.normal = -plane.N;
		}
	}
	else
	{
		if (d0 <= cyl.radius + epsilon)
		{
			if (d1 <= cyl.radius + epsilon)
			{
			// Both endpoints are close to the plane. Collision guaranteed.
				result = true;
			}
			else
			{
			// e0 is close. Need to check angle of axis from plane.
				float cos_theta = dot_product(-cyl.axis, plane.N);

			// Check for axis parallel to plane normal, which means e0 is closest point.
				float dc = 1.0f - fabs(cos_theta);
				float cos_epsilon = cos(epsilon);
				if ((dc < cos_epsilon) && (d0 < epsilon))
				{
					result = true;
				}
				else
				{
				// Compute distance from corner to plane. 
				// Avoid costly trig functions in favor of a few multiplies.
					float adj = cyl.radius * cos_theta;						// definition of cos.
					float dcorner_squared = square(cyl.radius+epsilon) - square(adj);	// law of cosines.
					if (d0 < epsilon || square(d0) <= dcorner_squared)
					{
						result = true;
					}
					else
					{
						result = false;
					}
				}
			}
		}
		else if (d1 <= cyl.radius + epsilon)
		{
		// e1 is close. Need to check angle of axis from plane.
			float cos_theta = dot_product(cyl.axis, plane.N);

		// Check for axis parallel to plane normal, which means e0 is closest point.
			float dc = 1.0f - fabs(cos_theta);
			float cos_epsilon = cos(epsilon);
			if ((dc < cos_epsilon) && (d1 < epsilon))
			{
				result = true;
			}
			else
			{
			// Compute distance from corner to plane.
				float adj = cyl.radius * cos_theta;						// definition of cos.
				float dcorner_squared = square(cyl.radius+epsilon) - square(adj);	// law of cosines.
				if (d1 < epsilon || square(d1) <= dcorner_squared)
				{
					result = true;
				}
				else
				{
					result = false;
				}
			}
		}
		else
		{
			result = false;
		}
	}

	return result;
}

//
// There's probably a more clever way of doing this. We check the distance
// of each box vertex from the plane, averaging the positions of equidistant
// close vertices to find the closest point.
//
bool PlaneBox        (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	XPlane plane(*((Plane *) g1), x1);
	XBox box(*((Box *) g2), x2);

	Vector vert[8];
	vert[0].set(-box.half_x, -box.half_y, -box.half_z);
	vert[1].set(-box.half_x, -box.half_y, +box.half_z);
	vert[2].set(-box.half_x, +box.half_y, -box.half_z);
	vert[3].set(-box.half_x, +box.half_y, +box.half_z);
	vert[4].set(+box.half_x, -box.half_y, -box.half_z);
	vert[5].set(+box.half_x, -box.half_y, +box.half_z);
	vert[6].set(+box.half_x, +box.half_y, -box.half_z);
	vert[7].set(+box.half_x, +box.half_y, +box.half_z);

	int min_verts[8];
	int num_min_verts = 0;

	float min = FLT_MAX;
	Vector * v = vert;
	for (int i = 0; i < 8; i++, v++)
	{
		float d = dot_product(plane.N, *v) + plane.D;
		float dd = fabs(d - min);
		if (dd < 1e-3)	// close enough
		{
			min_verts[num_min_verts++] = i;
		}
		else if (d < min)
		{
			min = d;
			min_verts[0] = i;
			num_min_verts = 1;
		}
	}

	if (compute_contact)
	{
		Vector closest;
		if (num_min_verts > 1)
		{
			closest.zero();
			for (int i = 0; i < num_min_verts; i++)
			{
				closest += vert[min_verts[i]];
			}

			closest /= num_min_verts;
		}
		else
		{
			closest = vert[min_verts[0]];
		}

		data.contact = closest;
		data.normal = -plane.N;
	}
	else
	{
		result = (min <= epsilon);
	}

	return result;
}

//
// Do the same as BoxMesh. It's O(n) in the number of mesh vertices.
// Could be more clever: use FindClosestPoints()-type approach to quickly hone in on
// closest feature to plane.
//
bool PlaneMesh       (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	XPlane plane(*((Plane *) g1), x1);
	CollisionMesh * mesh = (CollisionMesh *) g2;

	if (itemp_size < mesh->num_vertices)
	{
		delete [] itemp;
		itemp = new int[mesh->num_vertices];
		itemp_size = mesh->num_vertices;
	}

	int * min_verts = itemp;
	int num_min_verts = 0;

	float min = FLT_MAX;
	Vertex * v = mesh->vertices;
	for (int i = 0; i < mesh->num_vertices; i++, v++)
	{
		float d = dot_product(plane.N, v->p) + plane.D;
		float dd = fabs(d - min);
		if (dd < TOLERANCE)	// close enough
		{
			min_verts[num_min_verts++] = i;
		}
		else if (d < min)
		{
			min = d;
			min_verts[0] = i;
			num_min_verts = 1;
		}
	}

	if (compute_contact)
	{
		Vector closest(0, 0, 0);
		for (int i = 0; i < num_min_verts; i++)
		{
			closest += mesh->vertices[min_verts[i]].p;
		}

		closest /= num_min_verts;

		data.contact = closest;
		data.normal = -plane.N;
	}
	else
	{
		result = (min <= epsilon);
	}

	return result;
}

//

bool PlaneTrilist    (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	XPlane plane(*((Plane *) g1), x1);
	CollisionMesh * mesh = (CollisionMesh *) g2;

	if (itemp_size < mesh->num_vertices)
	{
		delete [] itemp;
		itemp = new int[mesh->num_vertices];
		itemp_size = mesh->num_vertices;
	}

	int * min_verts = itemp;
	int num_min_verts = 0;

	float min = FLT_MAX;
	Vertex * v = mesh->vertices;
	for (int i = 0; i < mesh->num_vertices; i++, v++)
	{
		float d = dot_product(plane.N, v->p) + plane.D;
		float dd = fabs(d - min);
		if (dd < TOLERANCE)	// close enough
		{
			min_verts[num_min_verts++] = i;
		}
		else if (d < min)
		{
			min = d;
			min_verts[0] = i;
			num_min_verts = 1;
		}
	}

	if (compute_contact)
	{
		Vector closest(0, 0, 0);
		for (int i = 0; i < num_min_verts; i++)
		{
			closest += mesh->vertices[min_verts[i]].p;
		}

		closest /= num_min_verts;

		data.contact = closest;
		data.normal = -plane.N;
	}
	else
	{
		result = (min <= epsilon);
	}

	return result;
}

//

bool SphereSphere    (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.sphere_sphere++;

	bool result;

	XSphere s1(*((Sphere *) g1), x1);
	XSphere s2(*((Sphere *) g2), x2);

	Vector dc = s1.center - s2.center;
	float dist_squared = dot_product(dc, dc);

	if (compute_contact)
	{
		result = true;
		if (dist_squared < TOLERANCE)
		{
		// centers coincide, contact point won't be useful.
			dc.zero();
		}
		else
		{
			dc /= sqrt(dist_squared);
		}

		data.contact = s2.center + dc * s2.radius;
	// perturb the normal ever so slightly to avoid perfectly stacking spheres.
		data.normal = dc;
		data.normal.x += -1e-2 + rand() * 2e-2 / RAND_MAX;
		data.normal.y += -1e-2 + rand() * 2e-2 / RAND_MAX;
		data.normal.z += -1e-2 + rand() * 2e-2 / RAND_MAX;
	}
	else
	{
		float rsum_squared = square(s1.radius + s2.radius + epsilon);
		result = (dist_squared <= rsum_squared);
	}

	return result;
}

//
// Assumption: sphere transformed to cylinder frame.
//
bool SphereCylinder  (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.sphere_cylinder++;

	bool result;

	XSphere sphere(*((Sphere *) g1), x1);
	XCylinder cyl(*((Cylinder *) g2), x2);

	float half_len = 0.5 * cyl.length;

	Vector cyl_base(0, 0, -half_len);

// diff = vector from cyl center to sphere center.
	Vector diff = sphere.center;
//	diff.z += half_len;

// Closest point on cylinder axis to sphere center is cyl_base + t * cyl.axis.
	float t = diff.z;

	const float max_len = sphere.radius + half_len + epsilon;
	if ((t < -max_len) || (t > max_len) && !compute_contact)
	{
	// Clear of end.
		result = false;
	}
	else
	{
	// Compute distance from axis.
	// x cases:
	// - between end caps, use distance from axis.
	// - above top: if inside radius, collide with top, otherwise use corner.
	// - below bottom: if inside radius, collide with bottom, otherwise use corner.

	// diff = shortest vector from cylinder axis to sphere center.
		diff.z = 0;
		float dist_squared = square(diff.x) + square(diff.y);

		if (t < -half_len || t > half_len)
		{
		// We already know we're within the sphere's radius of the end cap; see
		// if we're within the cylinder's radius from its axis.
			if (dist_squared <= square(cyl.radius + epsilon) || compute_contact)
			{
			// Colliding with top or bottom.
				result = true;

				if (compute_contact)
				{
					if (t < 0)
					{
						data.contact.set(diff.x, diff.y, -half_len);
						data.normal.set(0, 0, -1);
					}
					else
					{
						data.contact.set(diff.x, diff.y, half_len);
						data.normal.set(0, 0, 1);
					}
				}
			}
			else
			{
			// Possibly colliding with corner.
				Vector corner = diff;
				corner.normalize();
				corner *= cyl.radius;
				corner.z = (t < 0) ? -half_len : half_len;

				Vector delta = sphere.center - corner;
				float dist_squared = dot_product(delta, delta);
				if (dist_squared <= square(sphere.radius + epsilon) || compute_contact)
				{
					result = true;
					if (compute_contact)
					{
						data.contact = corner;
						data.normal = delta;
						data.normal.normalize();
					}
				}
				else
				{
					result = false;
				}
			}
		}
		else
		{
		 	if (compute_contact)
		 	{
		 		Vector axis_pt(0, 0, t);
				diff.normalize();
				data.contact = axis_pt + diff * cyl.radius;
				data.normal = diff;
			}
			else
			{
				result = (dist_squared <= square(cyl.radius + sphere.radius + epsilon));
			}
		}
	}

	return result;
}

//
// Assumption: sphere transformed to box frame.
//
bool SphereBox       (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.sphere_box++;

	bool result;

	XSphere s(*((Sphere *) g1), x1);
	XBox b(*((Box *) g2), x2);

	const Vector & c = s.center;
	Vector p;

	if (c.x < -b.half_x)
	{
		p.x = -b.half_x;
	}
	else if (c.x > b.half_x)
	{
		p.x = b.half_x;
	}
	else
	{
		p.x = c.x;
	}

	if (c.y < -b.half_y)
	{
		p.y = -b.half_y;
	}
	else if (c.y > b.half_y)
	{
		p.y = b.half_y;
	}
	else
	{
		p.y = c.y;
	}

	if (c.z < -b.half_z)
	{
		p.z = -b.half_z;
	}
	else if (c.z > b.half_z)
	{
		p.z = b.half_z;
	}
	else
	{
		p.z = c.z;
	}

	Vector d = c - p;

	if (compute_contact)
	{
		data.contact = p;
		float mag = d.magnitude();
		if (mag > TOLERANCE)
		{
			data.normal = d;
		}
		else
		{
			data.normal = p;
		}
		data.normal.normalize();
	}
	else
	{
		float dd = dot_product(d, d);
		result = (dd <= square(s.radius + epsilon));
	}
	
	return result;
}

//

bool SphereMesh      (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.sphere_convexmesh++;

	bool result;

	XSphere sphere(*((Sphere *) g1), x1);
	CollisionMesh * m = (CollisionMesh *) g2;

	if (!StdMeshesReady)
	{
		InitStdMeshes();
	}

	StdSphere.vertices[0].p.zero();	// untransformed.
	Matrix I;
	I.set_identity();	// sphere's orientation is irrelevant.

	Vector p1, p2, n;
	FindClosestPoints(StdSphere, *m, I, sphere.center, x2.R, x2.x, p1, p2, n);
	Vector closest = x2.x + x2.R * p2;
	Vector dc = closest - sphere.center;
	if (compute_contact)
	{
		data.contact = closest;
		data.normal = n;
	}
	else
	{
		float dist_squared = dot_product(dc, dc);
		float r_squared = square(sphere.radius + epsilon);
		result = (dist_squared <= r_squared);
	}

	return result;
}

//

bool SphereTrilist   (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	XSphere sphere(*((Sphere *) g1), x1);
	CollisionMesh * mesh = (CollisionMesh *) g2;

	float r_squared = square(sphere.radius + epsilon);

	float min = FLT_MAX;
	int num_tri_cols = 0;

// Sphere is in mesh frame. Compute distance from sphere center to each triangle in mesh.

	Triangle * tri = mesh->triangles;
	for (int i = 0; i < mesh->num_triangles; i++, tri++)
	{
		const Vector & b0 = mesh->vertices[tri->v[0]].p;
		const Vector & v1 = mesh->vertices[tri->v[1]].p;
		const Vector & v2 = mesh->vertices[tri->v[2]].p;

	// We might want to precompute these guys for each triangle.
		//Vector b0 = v0;
		Vector e0 = v1 - b0;
		Vector e1 = v2 - b0;

		Vector diff = b0 - sphere.center;

	// We could even precompute some of these dot_products per triangle.
		float a = dot_product(e0, e0);
		float b = dot_product(e0, e1);
		float c = dot_product(e1, e1);
		float d = dot_product(e0, diff);
		float e = dot_product(e1, diff);
		float f = dot_product(diff, diff);
	// This determinant will never be zero for a non-degenerate triangle.
		float det = a * c - b * b;

		float s = (b * e - c * d) / det;
		float t = (b * d - a * e) / det;

		if (s+t <= 1.0f)
		{
			if (s < 0.0f)
			{
				if (t < 0.0f)  // region 4
				{
					if (d < 0)
					{
						t = 0.0f;
						s = -d / a;
						if (s > 1.0f) 
						{
							s = 1.0f;
						}
					}
					else if (e < 0)
					{
						s = 0.0f;
						t = - e / c;
						if (t > 1.0f) 
						{
							t = 1.0f;
						}
					}
					else
					{
						s = 0.0f;
						t = 0.0f;
					}
				}
				else  // region 3
				{
					s = 0.0f;
					t = - e / c;
					if (t < 0.0f)
					{
						t = 0.0f;
					}
					else if (t > 1.0f)
					{
						t = 1.0f;
					}
				}
			}
			else if (t < 0.0f)  // region 5
			{
				t = 0.0f;
				s = -d / a;
				if (s < 0.0f)
				{
					s = 0.0f;
				}
				else if (s > 1.0f)
				{
					s = 1.0f;
				}
			}
			else  // region 0
			{
				// minimum at interior point
			}
		}
		else
		{
			if (s < 0.0f)  // region 2
			{
				if (b - c + d - e < 0.0f)
				{
					s = -(b - c + d - e) / (a - 2 * b + c);
					if (s < 0.0f) 
					{
						s = 0.0f; 
					}
					else if (s > 1.0f)
					{
						s = 1.0f;
					}
					t = 1.0f - s;
				}
				else if (c + e > 0.0f)
				{
					s = 0.0f;
					t = - e / c;
					if (t < 0.0f)
					{
						t = 0.0f;
					}
					else if (t > 1.0f)
					{
						t = 1.0f;
					}
				}
				else
				{
					s = 0.0f;
					t = 1.0f;
				}
			}
			else if (t < 0.0f)  // region 6
			{
				if (a - b + d - e > 0.0f)
				{
					t = (a - b + d - e) / (a - 2 * b + c);
					if (t < 0.0f) 
					{
						t = 0.0f;
					}
					else if (t > 1.0f)
					{
						t = 1.0f;
					}
					s = 1.0f - t;
				}
				else if (a + d > 0.0f)
				{
					t = 0.0f;
					s = -d / a;
					if (s < 0.0f) 
					{
						s = 0.0f;
					}
					else if (s > 1.0f)
					{
						s = 1.0f;
					}
				}
				else
				{
					s = 1.0f;
					t = 0.0f;
				}
			}
			else  // region 1
			{
				s = -(b - c + d - e) / (a - 2 * b + c);
				if (s < 0.0f )
				{
					s = 0.0f; 
				}
				else if (s > 1.0f) 
				{
					s = 1.0f;
				}
				t = 1.0f - s;
			}
		}
// Alternate computation of min distance.
//float dist_squared = fabs(s * (a * s + b * t + 2 * d) + t * (b * s + c * t + 2 * e) + f);

		Vector closest = b0 + s * e0 + t * e1;
		Vector dc = closest - sphere.center;
		float dc_squared = dot_product(dc, dc);
		float dd = fabs(dc_squared - min);
		if (dd <= TOLERANCE)
		{
			tricol[num_tri_cols].tri1 = tri;
			tricol[num_tri_cols].pt = closest;
			num_tri_cols++;
		}
		else if (dc_squared < min)
		{
			min = dc_squared;
			tricol[0].tri1 = tri;
			tricol[0].pt = closest;
			num_tri_cols = 1;
		}
	}

	if (compute_contact)
	{
		data.contact.zero();
		data.normal.zero();
		for (int i = 0; i < num_tri_cols; i++)
		{
			data.contact += tricol[i].pt;
			data.normal += mesh->normals[tricol[i].tri1->normal];
		}

	// average.
		data.contact /= num_tri_cols;
		data.normal.normalize();
	}
	else
	{
		result = (min <= r_squared);
	}

	return result;
}

//

float PointToLineDistanceSquared(Vector & rel, const Vector & pt, const Vector & base, const Vector & dir, bool clamp = false)
{
	Vector diff = pt - base;
	float t = dot_product(dir, diff) / dot_product(dir, dir);
	if (clamp)
	{
		clampf(t, 0, 1);
	}

	diff -= t * dir;
	rel = diff;
	return dot_product(diff, diff);
}

//

bool CylinderContainsPoint(const XCylinder * cyl, const Vector & p)
{
	ASSERT(cyl);

	bool result;

	Vector rel = p - cyl->center;
	Vector axis = cyl->axis;
	float t = dot_product(rel, axis);

	float half_len = 0.5 * cyl->length;
	if (fabs(t) <= half_len)
	{
		Vector axis_pt = cyl->center + t * axis;
		Vector diff = axis_pt - p;
		float dist_squared = dot_product(diff, diff);
		if (dist_squared <= square(cyl->radius))
		{
			result = true;
		}
		else
		{
			result = false;
		}
	}
	else
	{
		result = false;
	}

	return result;
}

//

void AdjustCylinder(CollisionMesh * cyl, float length, float radius)
{
	ASSERT(cyl);

	Vector p0 = cyl->vertices[0].p;
	p0.z = 0;
	float r = p0.magnitude();
	float scale = radius / r;

	Vertex * v = cyl->vertices;
	for (int i = 0; i < 16; i++, v++)
	{
		v->p.z = 0;
		v->p *= scale;
	}

	float half_len = length * 0.5;
	v = cyl->vertices;
	for (i = 0; i < 16; i++, v++)
	{
		v->p.z = half_len;
		(v+16)->p = v->p;
		(v+16)->p.z = -half_len;
	}
}

//
// Ouch.
//
bool CylinderCylinder(CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.cylinder_cylinder++;

	bool result = false;

	XCylinder cyl1(*((Cylinder *) g1), x1);
	XCylinder cyl2(*((Cylinder *) g2), x2);

// Treat 'em like fat lines. Deal with corner cases as in other cylinder/X tests.
	Vector l1 = cyl1.axis * cyl1.length;
	Vector l2 = cyl2.axis * cyl2.length;

	Vector base1 = cyl1.center - l1 * 0.5;
	Vector base2 = cyl2.center - l2 * 0.5;

	Vector diff = base1 - base2;

	float A =  dot_product(l1, l1);
	float B = -dot_product(l1, l2);
	float C =  dot_product(l2, l2);
	float D =  dot_product(l1, diff);
	float E = -dot_product(l2, diff);
	float det = A*C-B*B;

	float s, t;
	if (fabs(det) >= TOLERANCE)
	{
	// line segments are not parallel
		s = (B*E-C*D)/det;
		t = (B*D-A*E)/det;
		
		if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
		{
		// Closest axis points are between endcaps. Relatively easy case.
			Vector p1 = base1 + s * l1;
			Vector p2 = base2 + t * l2;

			Vector dp = p1 - p2;

			if (compute_contact)
			{
				dp.normalize();
				data.contact = p2 + dp * cyl2.radius;
				data.normal = dp;
			}
			else
			{
				float dist_squared = dot_product(dp, dp);
				result = (dist_squared <= square(cyl1.radius + cyl2.radius + epsilon));
			}
		}
		else
		{
		// Fuck it, use FindClosestPoints().
			if (!StdMeshesReady)
			{
				InitStdMeshes();
			}
/*
			AdjustCylinder(&StdCyl1, cyl1.length, cyl1.radius);
			AdjustCylinder(&StdCyl2, cyl2.length, cyl2.radius);
			VCyl1->adjust(cyl1.length, cyl1.radius);
			VCyl2->adjust(cyl2.length, cyl2.radius);

			Vector p1(0, 0, 0), p2(0, 0, 0), normal;
			vclip(*VCyl1, &StdCyl1, *VCyl2, &StdCyl2, x1.x, x1.R, x2.x, x2.R, p1, p2, normal);
			Vector c1 = x1.transform(p1);
			Vector c2 = x2.transform(p2);
*/
/*
glColor3ub(0, 255, 0);
glBegin(GL_LINES);
	glVertex3fv(&c1.x);
	glVertex3fv(&c2.x);
glEnd();
*/

			AdjustCylinder(&StdCyl1, cyl1.length, cyl1.radius);
			AdjustCylinder(&StdCyl2, cyl2.length, cyl2.radius);

			Vector p1, p2, normal;
			FindClosestPoints(StdCyl1, StdCyl2, x1.R, cyl1.center, x2.R, cyl2.center, p1, p2, normal);

			Vector c1 = cyl1.center + x1.R * p1;
			Vector c2 = cyl2.center + x2.R * p2;

			if (compute_contact)
			{
				data.contact = c1;

			// Use endcap normal if at all possible:

				Plane p0(base1, -cyl1.axis);
				float d = p0.compute_distance(c2);
				if (d > 0)
				{
					data.normal = cyl1.axis;
				}
				else
				{
					Vector end1 = cyl1.center + l1 * 0.5;
					p0.init(end1, cyl1.axis);
					d = p0.compute_distance(c2);
					if (d > 0)
					{
						data.normal = -cyl1.axis;
					}
					else
					{
						p0.init(base2, -cyl2.axis);
						d = p0.compute_distance(c1);
						if (d > 0)
						{
							data.normal = -cyl2.axis;
						}
						else
						{
							Vector end2 = cyl2.center + l2 * 0.5;
							p0.init(end2, cyl2.axis);
							d = p0.compute_distance(c1);
							if (d > 0)
							{
								data.normal = cyl2.axis;
							}
							else
							{
								data.normal = normal;
							}
						}
					}
				}
			}
			else
			{
				Vector dc = c1 - c2;
				float dist_squared = dot_product(dc, dc);
				result = (dist_squared <= epsilon);
			}
		}
	}
	else
	{
	// PARALLEL.
		float proj = -E / cyl2.length;
		bool in;
		if (B < 0)
		{
		// Facing same direction.
			in = (proj >= -(cyl1.length+epsilon)) && (proj <= (cyl2.length+epsilon));
		}
		else
		{
		// Facing opposite directions.
			in = (proj >= -epsilon) && (proj <= cyl1.length + cyl2.length + epsilon);
		}

		if (in || compute_contact)
		{
			Vector v0 = base2 + proj * cyl2.axis;
			Vector diff = base1 - v0;
			float dist_squared = dot_product(diff, diff);
			if (dist_squared <= square(cyl1.radius + cyl2.radius + epsilon) || compute_contact)
			{
				result = true;
				if (compute_contact)
				{
					float overlap;
#define BIT_LEFT	0x1
#define BIT_RIGHT	0x2

					unsigned int bits = 0;
					if (B < 0)
					{
					// facing same direction.
						float dp = cyl2.length - proj;
						if (dp <= cyl1.length)
						{
							bits |= BIT_LEFT;
						}
						if (proj < 0)
						{
							bits |= BIT_RIGHT;
						}

						if (bits == BIT_LEFT)
						{
							overlap = proj + 0.5 * dp;
						}
						else if (bits == BIT_RIGHT)
						{
							overlap = 0.5 * (cyl1.length + proj);
						}
						else
						{
						// middle of shorter cylinder.
							if (cyl1.length < cyl2.length)
							{
								overlap = proj + 0.5 * cyl1.length;
							}
							else
							{
								overlap = 0.5 * cyl2.length;
							}
						}
					}
					else
					{
					// facing opposite directions.
						if (proj > cyl2.length)
						{
							bits |= BIT_LEFT;
						}
						if (proj - cyl1.length < 0)
						{
							bits |= BIT_RIGHT;
						}
						if (bits == BIT_LEFT)
						{
							float b1 = proj - cyl1.length;
							overlap = 0.5 * (b1 + cyl2.length);
						}
						else if (bits == BIT_RIGHT)
						{
							overlap = 0.5 * proj;
						}
						else
						{
						// middle of shorter cylinder.
							if (cyl1.length < cyl2.length)
							{
								overlap = proj - 0.5 * cyl1.length;
							}
							else
							{
								overlap = 0.5 * cyl2.length;
							}
						}
					}

					diff.normalize();
					data.contact = base2 + overlap * cyl2.axis + diff * cyl2.radius;
					data.normal = diff;
				}
			}
			else
			{
				result = false;
			}
		}
		else
		{
			result = false;
		}
	}

	return result;
}

//
// ASSUMPTION: Cylinder transformed into box frame.
//
bool CylinderBox     (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.box_cylinder++;

	bool result = false;

	XCylinder cyl(*((Cylinder *) g1), x1);
	XBox box(*((Box *) g2), x2);

	if (compute_contact)
	{
	// Use mesh/mesh with standard cylinder & box meshes.
		if (!StdMeshesReady)
		{
			InitStdMeshes();
		}

		AdjustCylinder(&StdCyl1, cyl.length, cyl.radius);
		AdjustBox(&StdBox1, box.half_x, box.half_y, box.half_z);
  		Vector p1, p2, normal;
  		bool clear = FindClosestPoints(StdCyl1, StdBox1, x1.R, x1.x, x2.R, x2.x, p1, p2, normal);

  		data.contact = x1.transform(p1);
		data.normal = normal;
/*
	    VCyl1->adjust(cyl.length, cyl.radius);
		VBox1->adjust(box.half_x, box.half_y, box.half_z);

		Vector p1(0, 0, 0), p2(0, 0, 0), n;
		vclip(*VCyl1, &StdCyl1, *VBox1, &StdBox1, x1.x, x1.R, x2.x, x2.R, p1, p2, n);
		data.contact = x1.transform(p1);
	// n comes back in cylinder frame.
		data.normal = x1.R * n;
*/
/*
glColor3ub(0, 255, 0);
glBegin(GL_LINES);
	glVertex3fv(&data.contact.x);
	Vector c2 = x2.transform(p2);
	glVertex3fv(&c2.x);
glEnd();
*/
	}
	else
	{
	// Quick rejection: project cylinder onto each of box's axes, rejecting if
	// beyond cylinder's radius.

		Vector half_axis = cyl.axis * cyl.length * 0.5;
		Vector base = cyl.center - half_axis;
		Vector end = cyl.center + half_axis;

		float xcheck = box.half_x + cyl.radius;
		if ((base.x > xcheck && end.x > xcheck) || (base.x < -xcheck && end.x < -xcheck))
		{
			result = false;
		}
		else
		{
			float ycheck = box.half_y + cyl.radius;
			if ((base.y > ycheck && end.y > ycheck) || (base.y < -ycheck && end.y < -ycheck))
			{
				result = false;
			}
			else
			{
				float zcheck = box.half_z + cyl.radius;
				if ((base.z > zcheck && end.z > zcheck) || (base.z < -zcheck && end.z < -zcheck))
				{
					result = false;
				}
				else
				{
				// Damn, didn't get trivially rejected. Now we've got to do some real work.

				// Use mesh/mesh with standard cylinder & box meshes.
					if (!StdMeshesReady)
					{
						InitStdMeshes();
					}

					float cl = cyl.length * 1.1;
					float cr = cyl.radius * 1.1;
					float bx = box.half_x * 1.1;
					float by = box.half_y * 1.1;
					float bz = box.half_z * 1.1;

					AdjustCylinder(&StdCyl1, cl, cr);
					AdjustBox(&StdBox1, bx, by, bz);
/*
					VCyl1->adjust(cl, cr);
					VBox1->adjust(bx, by, bz);

					Vector p1(0, 0, 0), p2(0, 0, 0), n;
					vclip(*VCyl1, &StdCyl1, *VBox1, &StdBox1, x1.x, x1.R, x2.x, x2.R, p1, p2, n);
					Vector c1 = x1.transform(p1) / 1.1;
					Vector c2 = x2.transform(p2) / 1.1;

glColor3ub(0, 255, 0);
glBegin(GL_LINES);
	glVertex3fv(&c1.x);
	glVertex3fv(&c2.x);
glEnd();

					Vector dc = c1 - c2;
					float dsq = dot_product(dc, dc);
					result = (dsq <= square(epsilon));

*/
					Vector p1, p2, normal;
					bool clear = FindClosestPoints(StdCyl1, StdBox1, x1.R, x1.x, x2.R, x2.x, p1, p2, normal);

					Vector c1 = x1.transform(p1);
					Vector dp = c1 - p2;
					float dist_squared = dot_product(dp, dp);
					if (dist_squared <= square(epsilon))
					{
						result = true;
					}
					else
					{
						result = false;
					}

				}
			}
		}
	}

	return result;
}

//

bool CylinderMesh    (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.cylinder_convexmesh++;

	bool result;

	XCylinder cyl(*((Cylinder *) g1), x1);
	CollisionMesh * mesh = (CollisionMesh *) g2;

	if (!StdMeshesReady)
	{
		InitStdMeshes();
	}

	AdjustCylinder(&StdCyl1, cyl.length, cyl.radius);

	Vector p1, p2, n;
	FindClosestPoints(StdCyl1, *mesh, x1.R, x1.x, x2.R, x2.x, p1, p2, n);

	Vector c1 = x1.transform(p1);

	if (compute_contact)
	{
		data.contact = c1;
		data.normal = n;
	}
	else
	{
		Vector c2 = x2.transform(p2);
		Vector dc = c1 - c2;
		float dist_squared = dot_product(dc, dc);
		result = (dist_squared <= epsilon);
	}

	return result;
}

//

bool CylinderTrilist (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	XCylinder cyl(*((Cylinder *) g1), x1);

	if (!StdMeshesReady)
	{
		InitStdMeshes();
	}

	AdjustCylinder(&StdCyl1, cyl.length, cyl.radius);

	return TrilistTrilist(data, &StdCyl1, x1, g2, x2, epsilon, compute_contact);
}

//

void DrawMesh(const CollisionMesh * mesh);

static void GetBoxNormal(Vector & N, const Box & box, const Vector & p)
{
	float x = fabs(p.x);
	float y = fabs(p.y);
	float z = fabs(p.z);

	float dx = fabs(x - box.half_x) / box.half_x;
	float dy = fabs(y - box.half_y) / box.half_y;
	float dz = fabs(z - box.half_z) / box.half_z;

// find closest plane.
	int closest = 0;
	float dxy = fabs(dx - dy);
	float dxz = fabs(dx - dz);
	float dyz = fabs(dy - dz);

	float dmin = dx;
	int face = 0;
	if (dy < dmin)
	{
		face = 1;
		dmin = dy;
	}
	if (dz < dmin)
	{
		face = 2;
		dmin = dz;
	}

	switch (face)
	{
		case 0:
			if (dxy < TOLERANCE || dxz < TOLERANCE)
			{
				N.x = p.x / x;
				N.y = p.y / y;
				N.z = p.z / z;
			}
			else
			{
				N.set(p.x, 0, 0);
			}
			break;
		case 1:
			if (dxy < TOLERANCE || dyz < TOLERANCE)
			{
				N.x = p.x / x;
				N.y = p.y / y;
				N.z = p.z / z;
			}
			else
			{
				N.set(0, p.y, 0);
			}
			break;
		case 2:
			if (dxz < TOLERANCE || dyz < TOLERANCE)
			{
				N.x = p.x / x;
				N.y = p.y / y;
				N.z = p.z / z;
			}
			else
			{
				N.set(0, 0, p.z);
			}
			break;
	}

	N.normalize();
}

//
#if 0
bool GetBoxNormalV(Vector & n, const CollisionMesh & b, const Vector & p)
{
	bool result;

	Plane v[6];	// voronoi planes.

	v[0].init(b.center, b.vertices[0].p, b.vertices[3].p);
	v[1].init(b.center, b.vertices[7].p, b.vertices[4].p);
	v[2].init(b.center, b.vertices[4].p, b.vertices[5].p);
	v[3].init(b.center, b.vertices[0].p, b.vertices[1].p);
	v[4].init(b.center, b.vertices[3].p, b.vertices[7].p);
	v[5].init(b.center, b.vertices[4].p, b.vertices[0].p);

	float d[6];
	for (int i = 0; i < 6; i++)
	{
		d[i] = v[i].compute_distance(p);
	}

	const float tol = 1e-5;
	if (d[0] > tol && d[5] > tol && d[1] > tol && d[4] > tol)
	{
		result = true;
		n.set(0, 1, 0);
	}
	else if (d[0] < -tol && d[5] < -tol && d[1] < -tol && d[4] < -tol)
	{
		result = true;
		n.set(0, -1, 0);
	}
	else if (d[5] < -tol && d[2] > tol && d[4] > tol && d[3] < -tol)
	{
		result = true;
		n.set(1, 0, 0);
	}
	else if (d[5] > tol && d[2] < -tol && d[4] < -tol && d[3] > tol)
	{
		result = true;
		n.set(-1, 0, 0);
	}
	else if (d[3] > tol && d[2] > tol && d[0] < -tol && d[1] > tol)
	{
		result = true;
		n.set(0, 0, 1);
	}
	else if (d[3] < -tol && d[2] < -tol && d[0] > tol && d[1] < -tol)
	{
		result = true;
		n.set(0, 0, -1);
	}
	else
	{
		result = false;
	}

	return result;
}
#endif

//
// Returns a relative measure of how far a point is from the surface of a box.
static float InsideBoxMetric(const Box & box, const Vector & p)
{
	float dx = fabs(fabs(p.x) - box.half_x) / box.half_x;
	float dy = fabs(fabs(p.y) - box.half_y) / box.half_y;
	float dz = fabs(fabs(p.z) - box.half_z) / box.half_z;
	float max = Tmax(dx, Tmax(dy, dz));
	return max;
}

//

static inline bool PointInBox(const Box & box, const Vector & p)
{
	return ((fabs(p.x) <= box.half_x) && (fabs(p.y) <= box.half_y) && (fabs(p.z) <= box.half_z));
}

void HackDrawPoint(const Vector & pt, int r, int g, int b, float radius);
bool RayBox(Vector & p, const Vector & base, const Vector & dir, const Box & box, Vector & n);

/*
inline void CopyVec(BCVector & dst, Vector & src)
{
	dst[0] = src.x;
	dst[1] = src.y;
	dst[2] = src.z;
}
*/
//
#if 0
void ComputeBoxContacts(CollisionMesh & mesh1, const Box & b1, const CollisionMesh & mesh2, const Box & b2, const Vector & r12, const Matrix & R12, const Vector & r21, const Matrix & R21, Vector &p, Vector & n)
{
	float v1 = b1.volume();
	float v2 = b2.volume();

	if (v1 > v2)
	{
		Vector xv[8];
		bool in[8];
	// check b2 against b1.
		Vertex * v2 = mesh2.vertices;
		for (int i = 0; i < 8; i++, v2++)
		{
			xv[i] = r21 + R21 * v2->p;
			in[i] = PointInBox(b1, xv[i]);
		}

		n.zero();
		Vector sect(0, 0, 0);
		int num_intersections = 0;

		Vector N;
		int box_edges[12] = {0, 2, 3, 4, 6, 7, 8, 9, 11, 13, 14, 16};
		for (i = 0; i < 12; i++)
		{
			Edge * e = mesh2.edges + box_edges[i];
			int v0 = e->v[0];
			int v1 = e->v[1];

			if (!in[v0])
			{
				Vector dir = xv[v1] - xv[v0];
				Vector is, N;
				if (RayBox(is, xv[v0], dir, b1, N))
				{
					Vector d = is - xv[v0];
					if (d.magnitude() <= dir.magnitude())
					{
						sect += is;
						n -= N;
						num_intersections++;
					}
				}
			}
			
			if (!in[v1])
			{
				Vector dir = xv[v0] - xv[v1];
				Vector is, N;
				if (RayBox(is, xv[v1], dir, b1, N))
				{
					Vector d = is - xv[v1];
					if (d.magnitude() <= dir.magnitude())
					{
						sect += is;
						n -= N;
						num_intersections++;
					}
				}
			}
		}

		if (num_intersections)
		{
			p = sect / float(num_intersections);
			//n = -n;
			if (n.magnitude() < 0.1)
			{
				n = -N;
			}
			else
			{
				n.normalize();
			}
		}
	}
	else
	{
		Vector xv[8];
		bool in[8];
	// check b2 against b1.
		Vertex * v1 = mesh1.vertices;
		for (int i = 0; i < 8; i++, v1++)
		{
			xv[i] = r12 + R12 * v1->p;
			in[i] = PointInBox(b2, xv[i]);
		}

		n.zero();
		Vector sect(0, 0, 0);
		int num_intersections = 0;

		Vector N;
		int box_edges[12] = {0, 2, 3, 4, 6, 7, 8, 9, 11, 13, 14, 16};
		for (i = 0; i < 12; i++)
		{
			Edge * e = mesh2.edges + box_edges[i];
			int v0 = e->v[0];
			int v1 = e->v[1];

			if (!in[v0])
			{
				Vector dir = xv[v1] - xv[v0];
				Vector is, N;
				if (RayBox(is, xv[v0], dir, b2, N))
				{
					Vector d = is - xv[v0];
					if (d.magnitude() <= dir.magnitude())
					{
						sect += is;
						n += N;
						num_intersections++;
					}
				}
			}
			
			if (!in[v1])
			{
				Vector dir = xv[v0] - xv[v1];
				Vector is, N;
				if (RayBox(is, xv[v1], dir, b2, N))
				{
					Vector d = is - xv[v1];
					if (d.magnitude() <= dir.magnitude())
					{
						sect += is;
						n += N;
						num_intersections++;
					}
				}
			}
		}

		if (num_intersections)
		{
			p = sect / float(num_intersections);
			//n = -n;
			if (n.magnitude() < 0.1)
			{
				n = N;
			}
			else
			{
				n.normalize();
			}
		}
	}
}
#endif

//
// BoxBox() intersection test. This is some damn ugly code. For an explanation,
// see the OBBTree paper from Siggraph '96 from the UNC people.
//
bool BoxBox          (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.box_box++;

	bool result = false;

	XBox b1(*((Box *) g1), x1);
	XBox b2(*((Box *) g2), x2);

	if (compute_contact)
	{
		if (!StdMeshesReady)
		{
			InitStdMeshes();
		}

		VBox1->adjust(b1.half_x, b1.half_y, b1.half_z);
		VBox2->adjust(b2.half_x, b2.half_y, b2.half_z);

		Vector p1, p2, n;
		vclip(*VBox1, &StdBox1, *VBox2, &StdBox2, b1.center, x1.R, b2.center, x2.R, p1, p2, n);
		data.contact = p1;
		data.normal = n;
	}
	else
	{
	// R1 = identity.
	// R2 = b2's orientation relative to b1.

		const Vector & T = b2.center;
		const Matrix & R = x2.R;

		float a[3], b[3];
		a[0] = b1.half_x * 1.1;
		a[1] = b1.half_y * 1.1;
		a[2] = b1.half_z * 1.1;
				 
		b[0] = b2.half_x * 1.1;
		b[1] = b2.half_y * 1.1;
		b[2] = b2.half_z * 1.1;

		Matrix Rabs;
		Rabs.d[0][0] = fabs(R.d[0][0]);
		Rabs.d[0][1] = fabs(R.d[0][1]);
		Rabs.d[0][2] = fabs(R.d[0][2]);
		Rabs.d[1][0] = fabs(R.d[1][0]);
		Rabs.d[1][1] = fabs(R.d[1][1]);
		Rabs.d[1][2] = fabs(R.d[1][2]);
		Rabs.d[2][0] = fabs(R.d[2][0]);
		Rabs.d[2][1] = fabs(R.d[2][1]);
		Rabs.d[2][2] = fabs(R.d[2][2]);

		int r = 1;
		double t = fabs(T.x);
		double s;

	// A1 x A2 = A0
		t = fabs(T.x);
  
		r &= (t <= (a[0] + b[0] * Rabs.d[0][0] + b[1] * Rabs.d[0][1] + b[2] * Rabs.d[0][2]));
		if (!r)
		{
			goto done;
		}
  
	// B1 x B2 = B0
		s = T.x*R.d[0][0] + T.y*R.d[1][0] + T.z*R.d[2][0];
		t = fabs(s);

		r &= (t <= (b[0] + a[0] * Rabs.d[0][0] + a[1] * Rabs.d[1][0] + a[2] * Rabs.d[2][0]));
		if (!r)
		{
			goto done;
		}
    
	// A2 x A0 = A1
		t = fabs(T.y);

		r &= (t <= (a[1] + b[0] * Rabs.d[1][0] + b[1] * Rabs.d[1][1] + b[2] * Rabs.d[1][2]));
		if (!r)
		{
			goto done;
		}

	// A0 x A1 = A2
		t = fabs(T.z);

		r &= (t <= (a[2] + b[0] * Rabs.d[2][0] + b[1] * Rabs.d[2][1] + b[2] * Rabs.d[2][2]));
		if (!r)
		{
			goto done;
		}

	// B2 x B0 = B1
		s = T.x*R.d[0][1] + T.y*R.d[1][1] + T.z*R.d[2][1];
		t = fabs(s);

		r &= (t <= (b[1] + a[0] * Rabs.d[0][1] + a[1] * Rabs.d[1][1] + a[2] * Rabs.d[2][1]));
		if (!r)
		{
			goto done;
		}

	// B0 x B1 = B2
		s = T.x*R.d[0][2] + T.y*R.d[1][2] + T.z*R.d[2][2];
		t = fabs(s);

		r &= (t <= (b[2] + a[0] * Rabs.d[0][2] + a[1] * Rabs.d[1][2] + a[2] * Rabs.d[2][2]));
		if (!r)
		{
			goto done;
		}

	// A0 x B0
		s = T.z * R.d[1][0] - T.y * R.d[2][0];
		t = fabs(s);

		r &= (t <= (a[1] * Rabs.d[2][0] + a[2] * Rabs.d[1][0] + b[1] * Rabs.d[0][2] + b[2] * Rabs.d[0][1]));
		if (!r)
		{
			goto done;
		}

	// A0 x B1
		s = T.z * R.d[1][1] - T.y * R.d[2][1];
		t = fabs(s);

		r &= (t <= (a[1] * Rabs.d[2][1] + a[2] * Rabs.d[1][1] + b[0] * Rabs.d[0][2] + b[2] * Rabs.d[0][0]));
		if (!r)
		{
			goto done;
		}

	// A0 x B2
		s = T.z * R.d[1][2] - T.y * R.d[2][2];
		t = fabs(s);

		r &= (t <= (a[1] * Rabs.d[2][2] + a[2] * Rabs.d[1][2] + b[0] * Rabs.d[0][1] + b[1] * Rabs.d[0][0]));
		if (!r)
		{
			goto done;
		}

	// A1 x B0
		s = T.x * R.d[2][0] - T.z * R.d[0][0];
		t = fabs(s);

		r &= (t <= (a[0] * Rabs.d[2][0] + a[2] * Rabs.d[0][0] + b[1] * Rabs.d[1][2] + b[2] * Rabs.d[1][1]));
		if (!r)
		{
			goto done;
		}

	// A1 x B1
		s = T.x * R.d[2][1] - T.z * R.d[0][1];
		t = fabs(s);

		r &= (t <= (a[0] * Rabs.d[2][1] + a[2] * Rabs.d[0][1] + b[0] * Rabs.d[1][2] + b[2] * Rabs.d[1][0]));
		if (!r)
		{
			goto done;
		}

	// A1 x B2
		s = T.x * R.d[2][2] - T.z * R.d[0][2];
		t = fabs(s);

		r &= (t <= (a[0] * Rabs.d[2][2] + a[2] * Rabs.d[0][2] + b[0] * Rabs.d[1][1] + b[1] * Rabs.d[1][0]));
		if (!r)
		{
			goto done;
		}

	// A2 x B0
		s = T.y * R.d[0][0] - T.x * R.d[1][0];
		t = fabs(s);

		r &= (t <= (a[0] * Rabs.d[1][0] + a[1] * Rabs.d[0][0] + b[1] * Rabs.d[2][2] + b[2] * Rabs.d[2][1]));
		if (!r)
		{
			goto done;
		}

	// A2 x B1
		s = T.y * R.d[0][1] - T.x * R.d[1][1];
		t = fabs(s);

		r &= (t <= (a[0] * Rabs.d[1][1] + a[1] * Rabs.d[0][1] + b[0] * Rabs.d[2][2] + b[2] * Rabs.d[2][0]));
		if (!r)
		{
			goto done;
		}

	// A2 x B2
		s = T.y * R.d[0][2] - T.x * R.d[1][2];
		t = fabs(s);

		r &= (t <= (a[0] * Rabs.d[1][2] + a[1] * Rabs.d[0][2] + b[0] * Rabs.d[2][1] + b[1] * Rabs.d[2][0]));
		if (r)
		{
			result = true;
		}
done:;
	}

	return result;
}

//

bool BoxMesh         (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.box_convexmesh++;

	bool result;

	if (!StdMeshesReady)
	{
		InitStdMeshes();
	}

	XBox box(*((Box *) g1), x1);
	CollisionMesh * mesh = (CollisionMesh *) g2;

	float qx = box.half_x;
	float qy = box.half_y;
	float qz = box.half_z;

	AdjustBox(&StdBox1, qx, qy, qz);

	Vector p1, p2, n;
	FindClosestPoints(StdBox1, *mesh, x1.R, box.center, x2.R, x2.x, p1, p2, n);

	Vector c1 = box.center + x1.R * p1;
	if (compute_contact)
	{
		data.contact = c1;
		data.normal = n;
	}
	else
	{
		Vector c2 = x2.x + x2.R * p2;
		Vector dc = c1 - c2;
		float dist_squared = dot_product(dc, dc);
		result = (dist_squared <= epsilon);
	}

	return result;
}

//

bool BoxTrilist      (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	if (!StdMeshesReady)
	{
		InitStdMeshes();
	}

	XBox box(*((Box *) g1), x1);
	CollisionMesh * mesh = (CollisionMesh *) g2;

	float qx = box.half_x;
	float qy = box.half_y;
	float qz = box.half_z;

	AdjustBox(&StdBox1, qx, qy, qz);

	//StdBox1.center = x1.x;

	return TrilistTrilist(data, &StdBox1, x1, mesh, x2, epsilon, compute_contact);
}

//
// This ain't cheap, but should be reasonable for non-huge meshes.
//
bool MeshMesh        (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.convexmesh_convexmesh++;

	bool result;

	CollisionMesh * mesh1 = (CollisionMesh *) g1;
	CollisionMesh * mesh2 = (CollisionMesh *) g2;
/*
VMesh m1(mesh1);
VMesh m2(mesh2);
Vector p1(0, 0, 0), p2(0, 0, 0), n;
vclip(m1, mesh1, m2, mesh2, x1.x, x1.R, x2.x, x2.R, p1, p2, n);

if (compute_contact)
{
	data.contact = x1.transform(p1);
	data.normal = n;
}
else
{
	Vector c1 = x1.transform(p1);
	Vector c2 = x2.transform(p2);
	Vector dp = c1 - c2;
	float dist_squared = dot_product(dp, dp);
	result = (dist_squared <= square(epsilon));
}
return result;
*/
	Vector p1, p2, n;
	bool clear = FindClosestPoints(*mesh1, *mesh2, x1.R, x1.x, x2.R, x2.x, p1, p2, n);

	Vector c1 = x1.x + x1.R * p1;
	if (compute_contact)
	{
		data.contact = c1;
		data.normal = n;
	}
	else
	{
		Vector c2 = x2.x + x2.R * p2;
		Vector dc = c1 - c2;
		float dist_squared = dot_product(dc, dc);
		result = (dist_squared <= epsilon);
	}

	return result;
}

//

float MinTriangleTriangle(const Vector & t0_b, const Vector & t0_e0, const Vector & t0_e1, 
						  const Vector & t1_b, const Vector & t1_e0, const Vector & t1_e1,
						  float & s, float & t, float & u, float &v)
{
	float s0, t0, u0, v0, min, min0;
	LineSegment line;

// compare edges of tri0 against all of tri1


// b --> e0
	line.p0 = t0_b;
	line.p1 = t0_b + t0_e0;
	min = MinLineTriangle(line, t1_b, t1_e0, t1_e1, s, u, v);
	t = 0.0f;

// b --> e1
	line.p1 = line.p0 + t0_e1;
	min0 = MinLineTriangle(line, t1_b, t1_e0, t1_e1, t0, u0, v0);
	s0 = 0.0f;
	if ( min0 < min )
	{
		min = min0;
		s = s0;
		t = t0;
		u = u0;
		v = v0;
	}

// e0 --> e1
	line.p0 += t0_e0;
	min0 = MinLineTriangle(line, t1_b, t1_e0, t1_e1, t0, u0, v0);  // fix:  replace s0 with t0
	s0 = 1.0f-t0;  // fix:  swap s0 and t0
	if ( min0 < min )
	{
		min = min0;
		s = s0;
		t = t0;
		u = u0;
		v = v0;
	}

// compare edges of tri1 against all of tri0

// b --> e0
	line.p0 = t1_b;
	line.p1 = t1_b + t1_e0;
	min0 = MinLineTriangle(line, t0_b, t0_e0, t0_e1, u0, s0, t0);
	v0 = 0.0f;
	if ( min0 < min )
	{
		min = min0;
		s = s0;
		t = t0;
		u = u0;
		v = v0;
	}

// b --> e1
	line.p1 = line.p0 + t1_e1;
	min0 = MinLineTriangle(line, t0_b, t0_e0, t0_e1, v0, s0, t0);
	u0 = 0.0f;
	if ( min0 < min )
	{
		min = min0;
		s = s0;
		t = t0;
		u = u0;
		v = v0;
	}

// e0 --> e1
	line.p0 += t1_e0;
	min0 = MinLineTriangle(line, t0_b, t0_e0, t0_e1, v0, s0, t0);  // fix: replace u0 with v0
	u0 = 1.0f-v0;  // fix: swap u0 and v0
	if ( min0 < min )
	{
		min = min0;
		s = s0;
		t = t0;
		u = u0;
		v = v0;
	}

	return min;
}

//
// THIS IS EXTREMELY SLOW for big meshes.
//
bool MeshTrilist     (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	return TrilistTrilist(data, g1, x1, g2, x2, epsilon, compute_contact);
}

//
// THIS THE MOST GOD-AWFULLY SLOW FUNCTION YOU CAN EVER POSSIBLY IMAGINE. USE A 
// HIERARCHY OF BOUNDING SPHERES & BOXES a la OBB tree stuff if you really need
// to collide a lot of non-convex triangle lists.
//
// Function is O(NM) where N is num triangles in first mesh, M num triangles in 2nd,
// and the multiplier is large.
//
bool TrilistTrilist  (CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	CollisionMesh * mesh1 = (CollisionMesh *) g1;
	CollisionMesh * mesh2 = (CollisionMesh *) g2;

	if (x_length < mesh1->num_vertices)
	{
		if (xformed)
		{
			delete [] xformed;
		}
		x_length = mesh1->num_vertices;
		xformed = new Vector[x_length];
	}

	Vertex * src = mesh1->vertices;
	Vector * dst = xformed;
	for (int i = 0; i < mesh1->num_vertices; i++, src++, dst++)
	{
		*dst = x1.x + x1.R * src->p;
	}

	float global_min = FLT_MAX;
	int num_tri_cols = 0;

	Triangle * t1 = mesh1->triangles;
	for (i = 0; i < mesh1->num_triangles; i++, t1++)
	{
		const Vector * v0 = xformed + t1->v[0];
		const Vector * v1 = xformed + t1->v[1];
		const Vector * v2 = xformed + t1->v[2];

		Vector t0_b	= *v0;
		Vector t0_e0 = *v1 - *v0;
		Vector t0_e1 = *v2 - *v0;

		Triangle * t2 = mesh2->triangles;
		for (int j = 0; j < mesh2->num_triangles; j++, t2++)
		{
			v0 = &mesh2->vertices[t2->v[0]].p;
			v1 = &mesh2->vertices[t2->v[1]].p;
			v2 = &mesh2->vertices[t2->v[2]].p;

			Vector t1_b	= *v0;
			Vector t1_e0 = *v1 - *v0;
			Vector t1_e1 = *v2 - *v0;

		// intersect t1, t2
			float s, t, u, v;
			float dist = MinTriangleTriangle(t0_b, t0_e0, t0_e1, t1_b, t1_e0, t1_e1, s, t, u, v);
			float dd = fabs(dist - global_min);
			if (dd <= TOLERANCE)
			{
				if (compute_contact)
				{
					tricol[num_tri_cols].tri1 = t1;
					tricol[num_tri_cols].tri2 = t2;
					Vector p0 = t0_b + s * t0_e0 + t * t0_e1;
//			Vector p1 = t1_b + u * t1_e0 + v * t1_e1;
					tricol[num_tri_cols].pt = p0;
					if (num_tri_cols < 128)
					{
						num_tri_cols++;
					}
				}
			}
			else if (dist < global_min)		
			{
				global_min = dist;
				if (compute_contact)
				{
					tricol[num_tri_cols].tri1 = t1;
					tricol[num_tri_cols].tri2 = t2;
					Vector p0 = t0_b + s * t0_e0 + t * t0_e1;
//			Vector p1 = t1_b + u * t1_e0 + v * t1_e1;
					tricol[num_tri_cols].pt = p0;
					num_tri_cols = 1;
				}
			}
		}
	}

	if (compute_contact)
	{
		Vector closest(0, 0, 0);
		Vector normal(0, 0, 0);
		for (int i = 0; i < num_tri_cols; i++)
		{
		HackDrawPoint(tricol[i].pt, 32, 32, 255, 0.5);

			closest += tricol[i].pt;
			normal += mesh2->normals[tricol[i].tri2->normal];
		}

	// average.
		closest /= num_tri_cols;
		normal.normalize();

		data.contact = closest;
		data.normal = normal;
	}
	else
	{
		result = (global_min < epsilon);
	}

	return result;
}

//

bool LineTube		(CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	XLineSegment line(*((LineSegment *) g1), x1);
	XTube tube(*((Tube *) g2), x2);

	float half_len = 0.5 * tube.length;
	Vector half_axis = tube.axis * half_len;
	Vector t0 = tube.center + half_axis;
	Vector t1 = tube.center - half_axis;

	LineSegment tube_seg(t0, t1);

	Vector p0, p1;
	float s, t;
	ClosestLineLine(line, tube_seg, s, p0, t, p1);
	Vector dp = p0 - p1;
	if (compute_contact)
	{
		dp.normalize();
		data.contact = p1 + dp * tube.radius;
		data.normal = dp;
	}
	else
	{
		float dist_squared = dot_product(dp, dp);
		result = (dist_squared <= square(tube.radius + epsilon));
	}

	return result;
}

//

bool PlaneTube		(CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	XPlane plane(*((Plane *) g1), x1);
	XTube tube(*((Tube *) g2), x2);

	Vector half_axis = 0.5 * tube.length * tube.axis;
	Vector t0 = tube.center - half_axis;
	Vector t1 = tube.center + half_axis;

	Vector dir = t1 - t0;
	float denom = dot_product(dir, plane.N);
	if (fabs(denom) >= TOLERANCE)
	{
		float num = -plane.compute_distance(t0);
		float t = num / denom;

		t = Tmax(0.0f, Tmin(1.0f, t));

		Vector p = t0 + t * dir;

		if (compute_contact)
		{
			data.contact = p - plane.N * tube.radius;
			data.normal = -plane.N;
		}
		else
		{
			float dist = plane.compute_distance(p);
			result = (dist <= tube.radius);
		}
	}
	else
	{
	// Tube is parallel to plane.
		if (compute_contact)
		{
			data.contact = tube.center - tube.radius * plane.N;
			data.normal = -plane.N;
		}
		else
		{
			float dist = plane.compute_distance(t0);
			result = (dist <= tube.radius);
		}
	}

	return result;
}

//

bool SphereTube		(CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.sphere_tube++;

	bool result;

	XSphere sphere(*((Sphere *) g1), x1);
	XTube tube(*((Tube *) g2), x2);

	Vector half_axis = 0.5 * tube.length * tube.axis;
	Vector tube_base = tube.center - half_axis;
	Vector diff = sphere.center - tube_base;

	float t = dot_product(tube.axis, diff) / dot_product(tube.axis, tube.axis);

	if ((t < -(sphere.radius + tube.radius)) || (t > (tube.length + tube.radius + sphere.radius)) && !compute_contact)
	{
	// Clear of ends.
		result = false;
	}
	else
	{
		t = Tmax(-tube.length, Tmin(t, tube.length));
		diff -= t * tube.axis;

		if (compute_contact)
		{
			Vector tp = tube_base + t * tube.axis;
			diff.normalize();
			data.contact = tp + diff * tube.radius;
			data.normal = diff;
		}
		else
		{
			float dist_squared = dot_product(diff, diff);
			result = (dist_squared <= square(tube.radius + sphere.radius + epsilon));
		}
	}

	return result;
}

//

bool CylinderTube	(CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.tube_cylinder++;

	bool result;

	XCylinder cyl(*((Cylinder *) g1), x1);
	XTube tube(*((Tube *) g2), x2);

	Vector half_axis = 0.5 * cyl.length * cyl.axis;
	Vector t0_0 = cyl.center - half_axis;
	Vector t0_1 = cyl.center + half_axis;

	half_axis = 0.5 * tube.length * tube.axis;
	Vector t1_0 = tube.center - half_axis;
	Vector t1_1 = tube.center + half_axis;

	LineSegment cyl_seg(t0_0, t0_1);
	LineSegment tube_seg(t1_0, t1_1);

	Vector p0, p1;
	float s, t;
	ClosestLineLine(cyl_seg, tube_seg, s, p0, t, p1);
	Vector dp = p1 - p0;

	float dist_squared = dot_product(dp, dp);
	if ((dist_squared > square(cyl.radius + tube.radius)) && !compute_contact)
	{
	// Contact not possible.
		result = false;
	}
	else
	{
		if (s == 0)
		{
			if (dp.z < 0)
			{
			// pointing out of bottom plane, check corner.
				Vector perp = cross_product(cyl.axis, dp);
				float pmag = perp.magnitude();
				if (pmag > TOLERANCE)
				{
					perp /= pmag;
					Vector corner(perp.y * cyl.radius, -perp.x * cyl.radius, 0);

					Vector l0 = t0_0 - corner;
					Vector l1 = t0_0 + corner;
					LineSegment ltemp(l0, l1);
					float u, v;
					float dist = MinLineLine(ltemp, tube_seg, u, v);
					if (dist <= (tube.radius+epsilon) || compute_contact)
					{
						result = true;
						if (compute_contact)
						{
							data.contact = l0 + u * (l1 - l0);
							Vector tp = t1_0 + v * (t1_1 - t1_0);
							data.normal = data.contact - tp;
							data.normal.normalize();
						}
					}
					else
					{
						result = false;
					}
				}
				else
				{
					//DebugPrint("parallel\n");
				}
			}
			else
			{
				result = true;
				if (compute_contact)
				{
					Vector cp = t0_0;
					Vector tp = t1_0 + t * (t1_1 - t1_0);
					Vector dp = cp - tp;
					dp.normalize();
					data.contact = tp + dp * tube.radius;
					data.normal = dp;
				}
			}
		}
		else if (s == 1)
		{
			if (dp.z > 0)
			{
			// pointing out of top plane.
				Vector perp = cross_product(cyl.axis, dp);
				float pmag = perp.magnitude();
				if (pmag > TOLERANCE)
				{
					perp /= pmag;
					Vector corner(perp.y * cyl.radius, -perp.x * cyl.radius, 0);

					Vector l0 = t0_1 - corner;
					Vector l1 = t0_1 + corner;
					LineSegment ltemp(l0, l1);
					float u, v;
					float dist = MinLineLine(ltemp, tube_seg, u, v);
					if (dist <= (tube.radius+epsilon) || compute_contact)
					{
						result = true;
						if (compute_contact)
						{
							data.contact = l0 + u * (l1 - l0);
							Vector tp = t1_0 + v * (t1_1 - t1_0);
							data.normal = data.contact - tp;
							data.normal.normalize();
						}
					}
					else
					{
						result = false;
					}
				}
				else
				{
					//DebugPrint("parallel\n");
				}
			}
			else
			{
				result = true;
				if (compute_contact)
				{
					Vector cp = t0_1;
					Vector tp = t1_0 + t * (t1_1 - t1_0);
					Vector dp = cp - tp;
					dp.normalize();
					data.contact = tp + dp * tube.radius;
					data.normal = dp;
				}
			}
		}
		else
		{
			result = true;
			if (compute_contact)
			{
				Vector cp = t0_0 + s * (t0_1 - t0_0);
				Vector tp = t1_0 + t * (t1_1 - t1_0);
				Vector dp = cp - tp;
				dp.normalize();
				data.contact = tp + dp * tube.radius;
				data.normal = dp;
			}
		}
	}

	return result;
}

//

struct Rect
{
	Vector b, e0, e1;
};

//

float MinPointRect(const Vector & p, const Rect & rect,	float & s, float & t)
{
	Vector diff = rect.b - p;
	float A = dot_product(rect.e0, rect.e0);
	float B = dot_product(rect.e0, rect.e1);
	float C = dot_product(rect.e1, rect.e1);
	float D = dot_product(rect.e0, diff);
	float E = dot_product(rect.e1, diff);
	float F = dot_product(diff, diff);
	float det = A*C-B*B;  // ASSERT:  det != 0 for rectoids

	s = (B*E-C*D)/det;
	t = (B*D-A*E)/det;

	if ( s < 0.0f )
	{
		if ( t < 0.0f )  // region 6
		{
			if ( D < 0.0f )
			{
				t = 0.0f;
				s = -D/A;
				if ( s > 1.0f ) s = 1.0f;
			}
			else if ( E < 0.0f )
			{
				s = 0.0f;
				t = -E/C;
				if ( t > 1.0f ) t = 1.0f;
			}
			else
			{
				s = 0.0f;
				t = 0.0f;
			}
		}
		else if ( t <= 1.0f )  // region 5
		{
			s = 0.0f;
			t = -E/C;
			if ( t < 0.0f ) t = 0.0f; else if ( t > 1.0f ) t = 1.0f;
		}
		else  // region 4
		{
			if ( B+D < 0.0f )
			{
				t = 1.0f;
				s = -(B+D)/A;
				if ( s > 1.0f ) s = 1.0f;
			}
			else if ( C+E > 0.0f )
			{
				s = 0.0f;
				t = -E/C;
				if ( t < 0.0f ) t = 0.0f;
			}
			else
			{
				s = 0.0f;
				t = 1.0f;
			}
		}
	}
	else if ( s <= 1.0f )
	{
		if ( t < 0.0f )  // region 7
		{
			t = 0.0f;
			s = -D/A;
			if ( s < 0.0f ) s = 0.0f; else if ( s > 1.0f ) s = 1.0f;
		}
		else if ( t <= 1.0f )  // region 0
		{
			// minimum at interior point
		}
		else  // region 3
		{
			t = 1.0f;
			s = -(B+D)/A;
			if ( s < 0.0f ) s = 0.0f; else if ( s > 1.0f ) s = 1.0f;
		}
	}
	else
	{
		if ( t < 0.0f )  // region 8
		{
			if ( A+D > 0.0f )
			{
				t = 0.0f;
				s = -D/A;
				if ( s < 0.0f ) s = 0.0f;
			}
			else if ( B+E < 0.0f )
			{
				s = 1.0f;
				t = -(B+E)/C;
				if ( t > 1.0f ) t = 1.0f;
			}
			else
			{
				s = 1.0f;
				t = 0.0f;
			}
		}
		else if ( t <= 1.0f )  // region 1
		{
			s = 1.0f;
			t = -(B+E)/C;
			if ( t < 0.0f ) t = 0.0f; else if ( t > 1.0f ) t = 1.0f;
		}
		else  // region 2
		{
			if ( A+B+D > 0.0f )
			{
				t = 1.0f;
				s = -(B+D)/A;
				if ( s < 0.0f ) s = 0.0f;
			}
			else if ( B+C+E > 0.0f )
			{
				s = 1.0f;
				t = -(B+E)/C;
				if ( t < 0.0f ) t = 0.0f;
			}
			else
			{
				s = 1.0f;
				t = 1.0f;
			}
		}
	}

	return sqrt(fabs(s*(A*s+B*t+2*D)+t*(B*s+C*t+2*E)+dot_product(diff,diff)));
}

//

float MinLineRect(const LineSegment & line, const Rect & rect, float & r, float & s, float & t)
{
	Vector line_m = line.p1 - line.p0;

	Vector diff = rect.b - line.p0;
	float A00 = +dot_product(line_m, line_m);
	float A01 = -dot_product(line_m, rect.e0);
	float A02 = -dot_product(line_m, rect.e1);
	float A11 = +dot_product(rect.e0, rect.e0);
	float A12 = +dot_product(rect.e0, rect.e1);
	float A22 = +dot_product(rect.e1, rect.e1);
	float B0  = -dot_product(diff, line_m);
	float B1  = +dot_product(diff, rect.e0);
	float B2  = +dot_product(diff, rect.e1);
	float cof00 = A11*A22-A12*A12;
	float cof01 = A02*A12-A01*A22;
	float cof02 = A01*A12-A02*A11;
	float det = A00*cof00+A01*cof01+A02*cof02;

	const float tolerance = 1e-05;
	LineSegment rectline;
	float min, min0, r0, s0, t0;

	if ( fabs(det) >= tolerance )
	{
		float cof11 = A00*A22-A02*A02;
		float cof12 = A02*A01-A00*A12;
		float cof22 = A00*A11-A01*A01;
		float rhs0 = -B0/det;
		float rhs1 = -B1/det;
		float rhs2 = -B2/det;

		r = cof00*rhs0+cof01*rhs1+cof02*rhs2;
		s = cof01*rhs0+cof11*rhs1+cof12*rhs2;
		t = cof02*rhs0+cof12*rhs1+cof22*rhs2;

		if ( r < 0.0f )
		{
			if ( s < 0.0f )
			{
				if ( t < 0.0f )  // region 6m
				{
					// min on face s=0 or t=0 or r=0
					rectline.p0 = rect.b;
					rectline.p1 = rect.b + rect.e1;
					min = MinLineLine(line, rectline, r, t);
					s = 0.0f;
					rectline.p0 = rect.b;
					rectline.p1 = rect.b + rect.e0;
					min0 = MinLineLine(line, rectline, r0, s0);
					t0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
					min0 = MinPointRect(line.p0, rect, s0, t0);
					r0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else if ( t <= 1.0f )  // region 5m
				{
					// min on face s=0 or r=0
					rectline.p0 = rect.b;
					rectline.p1 = rect.b + rect.e1;
					min = MinLineLine(line, rectline, r, t);
					s = 0.0f;
					min0 = MinPointRect(line.p0, rect, s0, t0);
					r0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else  // region 4m
				{
					// min on face s=0 or t=1 or r=0
					rectline.p0 = rect.b;
					rectline.p1 = rect.b + rect.e1;
					min = MinLineLine(line, rectline, r, t);
					s = 0.0f;
					rectline.p0 = rect.b + rect.e1;
					rectline.p1 = rectline.p0 + rect.e0;
					min0 = MinLineLine(line,rectline,r0,s0);
					t0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
					min0 = MinPointRect(line.p0,rect,s0,t0);
					r0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
			}
			else if ( s <= 1.0f )
			{
				if ( t < 0.0f )  // region 7m
				{
					// min on face t=0 or r=0
					rectline.p0 = rect.b;
					rectline.p1 = rectline.p0 + rect.e0;
					min = MinLineLine(line,rectline,r,s);
					t = 0.0f;
					min0 = MinPointRect(line.p0,rect,s0,t0);
					r0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else if ( t <= 1.0f )  // region 0m
				{
					// min on face r=0
					min = MinPointRect(line.p0,rect,s,t);
					r = 0.0f;
				}
				else  // region 3m
				{
					// min on face t=1 or r=0
					rectline.p0 = rect.b + rect.e1;
					rectline.p1 = rectline.p0 + rect.e0;
					min = MinLineLine(line,rectline,r,s);
					t = 1.0f;
					min0 = MinPointRect(line.p0,rect,s0,t0);
					r0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
			}
			else
			{
				if ( t < 0.0f )  // region 8m
				{
					// min on face s=1 or t=0 or r=0
					rectline.p0 = rect.b + rect.e0;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 1.0f;
					rectline.p0 = rect.b;
					rectline.p1 = rectline.p0 + rect.e0;
					min0 = MinLineLine(line,rectline,r0,s0);
					t0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
					min0 = MinPointRect(line.p0,rect,s0,t0);
					r0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else if ( t <= 1.0f )  // region 1m
				{
					// min on face s=1 or r=0
					rectline.p0 = rect.b + rect.e0;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 1.0f;
					min0 = MinPointRect(line.p0,rect,s0,t0);
					r0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else  // region 2m
				{
					// min on face s=1 or t=1 or r=0
					rectline.p0 = rect.b + rect.e0;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 1.0f;
					rectline.p0 = rect.b + rect.e1;
					rectline.p1 = rectline.p0 + rect.e0;
					min0 = MinLineLine(line,rectline,r0,s0);
					t0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
					min0 = MinPointRect(line.p0,rect,s0,t0);
					r0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
			}
		}
		else if ( r <= 1.0f )
		{
			if ( s < 0.0f )
			{
				if ( t < 0.0f )  // region 6
				{
					// min on face s=0 or t=0
					rectline.p0 = rect.b;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 0.0f;
					rectline.p0 = rect.b;
					rectline.p1 = rectline.p0 + rect.e0;
					min0 = MinLineLine(line,rectline,r0,s0);
					t0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else if ( t <= 1.0f )  // region 5
				{
					// min on face s=0
					rectline.p0 = rect.b;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 0.0f;
				}
				else // region 4
				{
					// min on face s=0 or t=1
					rectline.p0 = rect.b;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 0.0f;
					rectline.p0 = rect.b + rect.e1;
					rectline.p1 = rectline.p0 + rect.e0;
					min0 = MinLineLine(line,rectline,r0,s0);
					t0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
			}
			else if ( s <= 1.0f )
			{
				if ( t < 0.0f )  // region 7
				{
					// min on face t=0
					rectline.p0 = rect.b;
					rectline.p1 = rectline.p0 + rect.e0;
					min = MinLineLine(line,rectline,r,s);
					t = 0.0f;
				}
				else if ( t <= 1.0f )  // region 0
				{
					// global minimum is interior
					min = sqrt(fabs(r*(A00*r+A01*s+A02*t+2.0f*B0)
						  +s*(A01*r+A11*s+A12*t+2.0f*B1)
						  +t*(A02*r+A12*s+A22*t+2.0f*B2)
						  +dot_product(diff,diff)));
				}
				else  // region 3
				{
					// min on face t=1
					rectline.p0 = rect.b + rect.e1;
					rectline.p1 = rectline.p0 + rect.e0;
					min = MinLineLine(line,rectline,r,s);
					t = 1.0f;
				}
			}
			else
			{
				if ( t < 0.0f )  // region 8
				{
					// min on face s=1 or t=0
					rectline.p0 = rect.b + rect.e0;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 1.0f;
					rectline.p0 = rect.b;
					rectline.p1 = rectline.p0 + rect.e0;
					min0 = MinLineLine(line,rectline,r0,s0);
					t0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else if ( t <= 1.0f )  // region 1
				{
					// min on face s=1
					rectline.p0 = rect.b + rect.e0;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 1.0f;
				}
				else  // region 2
				{
					// min on face s=1 or t=1
					rectline.p0 = rect.b + rect.e0;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 1.0f;
					rectline.p0 = rect.b + rect.e1;
					rectline.p1 = rectline.p1 + rect.e0;
					min0 = MinLineLine(line,rectline,r0,s0);
					t0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
			}
		}
		else
		{
			if ( s < 0.0f )
			{
				if ( t < 0.0f )  // region 6p
				{
					// min on face s=0 or t=0 or r=0
					rectline.p0 = rect.b;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 0.0f;
					rectline.p1 = rectline.p0 + rect.e0;
					min0 = MinLineLine(line,rectline,r0,s0);
					t0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
					min0 = MinPointRect(line.p1,rect,s0,t0);
					r0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else if ( t <= 1.0f )  // region 5p
				{
					// min on face s=0 or r=0
					rectline.p0 = rect.b;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 0.0f;
					min0 = MinPointRect(line.p0,rect,s0,t0);
					min0 = MinPointRect(line.p1,rect,s0,t0);
					r0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else  // region 4p
				{
					// min on face s=0 or t=1 or r=0
					rectline.p0 = rect.b;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 0.0f;
					rectline.p0 = rect.b + rect.e1;
					rectline.p1 = rectline.p0 + rect.e0;
					min0 = MinLineLine(line,rectline,r0,s0);
					t0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
					min0 = MinPointRect(line.p1,rect,s0,t0);
					r0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
			}
			else if ( s <= 1.0f )
			{
				if ( t < 0.0f )  // region 7p
				{
					// min on face t=0 or r=0
					rectline.p0 = rect.b;
					rectline.p1 = rectline.p0 + rect.e0;
					min = MinLineLine(line,rectline,r,s);
					t = 0.0f;
					min0 = MinPointRect(line.p1,rect,s0,t0);
					r0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else if ( t <= 1.0f )  // region 0p
				{
					// min on face r=0
					min = MinPointRect(line.p1,rect,s,t);
					r = 0.0f;
				}
				else  // region 3p
				{
					// min on face t=1 or r=0
					rectline.p0 = rect.b + rect.e1;
					rectline.p1 = rectline.p0 + rect.e0;
					min = MinLineLine(line,rectline,r,s);
					t = 1.0f;
					min0 = MinPointRect(line.p1,rect,s0,t0);
					r0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
			}
			else
			{
				if ( t < 0.0f )  // region 8p
				{
					// min on face s=1 or t=0 or r=0
					rectline.p0 = rect.b + rect.e0;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 1.0f;
					rectline.p0 = rect.b;
					rectline.p1 = rectline.p0 + rect.e0;
					min0 = MinLineLine(line,rectline,r0,s0);
					t0 = 0.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
					min0 = MinPointRect(line.p1,rect,s0,t0);
					r0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else if ( t <= 1.0f )  // region 1p
				{
					// min on face s=1 or r=0
					rectline.p0 = rect.b + rect.e0;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 1.0f;
					min0 = MinPointRect(line.p1,rect,s0,t0);
					r0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
				else  // region 2p
				{
					// min on face s=1 or t=1 or r=0
					rectline.p0 = rect.b + rect.e0;
					rectline.p1 = rectline.p0 + rect.e1;
					min = MinLineLine(line,rectline,r,t);
					s = 1.0f;
					rectline.p0 = rect.b + rect.e1;
					rectline.p1 = rectline.p0 + rect.e0;
					min0 = MinLineLine(line,rectline,r0,s0);
					t0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
					min0 = MinPointRect(line.p1,rect,s0,t0);
					r0 = 1.0f;
					if ( min0 < min )
					{
						min = min0;
						r = r0;
						s = s0;
						t = t0;
					}
				}
			}
		}
	}
	else
	{
		// line and rectoid are parallel
		rectline.p0 = rect.b;
		rectline.p1 = rectline.p0 + rect.e0;
		min = MinLineLine(line,rectline,r,s);
		t = 0.0f;

		rectline.p1 = rectline.p0 + rect.e1;
		min0 = MinLineLine(line,rectline,r0,t0);
		s0 = 0.0f;
		if ( min0 < min )
		{
			min = min0;
			r = r0;
			s = s0;
			t = t0;
		}

		rectline.p0 = rect.b + rect.e1;
		rectline.p1 = rectline.p0 + rect.e0;
		min0 = MinLineLine(line,rectline,r0,s0);
		t0 = 1.0f;
		if ( min0 < min )
		{
			min = min0;
			r = r0;
			s = s0;
			t = t0;
		}

		rectline.p0 = rect.b + rect.e0;
		rectline.p1 = rectline.p0 + rect.e1;
		min0 = MinLineLine(line,rectline,r0,t0);
		s0 = 1.0f;
		if ( min0 < min )
		{
			min = min0;
			r = r0;
			s = s0;
			t = t0;
		}

		min0 = MinPointRect(line.p0,rect,s0,t0);
		r0 = 0.0f;
		if ( min0 < min )
		{
			min = min0;
			r = r0;
			s = s0;
			t = t0;
		}

		min0 = MinPointRect(line.p1,rect,s0,t0);
		r0 = 1.0f;
		if ( min0 < min )
		{
			min = min0;
			r = r0;
			s = s0;
			t = t0;
		}
	}

	return min;
}


//

bool BoxTube		(CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.box_tube++;

	bool result = false;

	XBox box(*((Box *) g1), x1);
	XTube tube(*((Tube *) g2), x2);

	Vector half_axis = 0.5 * tube.length * tube.axis;
	Vector t0 = tube.center + half_axis;
	Vector t1 = tube.center - half_axis;

	if (compute_contact)
	{
		if (!StdMeshesReady)
		{
			InitStdMeshes();
		}

		AdjustBox(&StdBox1, box.half_x, box.half_y, box.half_z);
		StdLine.vertices[0].p = t0;
		StdLine.vertices[1].p = t1;
		Vector p0, p1, N;
		FindClosestPoints(StdBox1, StdLine, x1.R, x1.x, x1.R, x1.x, p0, p1, N);

		Vector dp = p0 - p1;

	// Separating plane between extents.
		N = dp;
		N.normalize();

		float cos_theta = dot_product(tube.axis, N);

		const float cos_tol = cos(deg_to_rad(89));

		if (fabs(cos_theta) < cos_tol)
		{
		// perpendicular to plane.
			data.contact = tube.center + tube.radius * N;
		}
		else
		{
			data.contact = p0;
		}

		data.normal = N;
	}
	else
	{
	// Project tube ends onto box axes; if outside all 3 axes, no intersection.
		float xcheck = box.half_x + tube.radius;
		if ((t0.x > xcheck && t1.x > xcheck) || (t0.x < -xcheck && t1.x < -xcheck))
		{
			result = false;
		}
		else
		{
			float ycheck = box.half_y + tube.radius;
			if ((t0.y > ycheck && t1.y > ycheck) || (t0.y < -ycheck && t1.y < -ycheck))
			{
				result = false;
			}
			else
			{
				float zcheck = box.half_z + tube.radius;
				if ((t0.z > zcheck && t1.z > zcheck) || (t0.z < -zcheck && t1.z < -zcheck))
				{
					result = false;
				}
				else
				{
				// Damn, passed trivial rejection, now we have to do some real work.
					if (!StdMeshesReady)
					{
						InitStdMeshes();
					}

					AdjustBox(&StdBox1, box.half_x, box.half_y, box.half_z);
					StdLine.vertices[0].p = t0;
					StdLine.vertices[1].p = t1;
					Vector p0, p1, N;
					FindClosestPoints(StdBox1, StdLine, x1.R, x1.x, x1.R, x1.x, p0, p1, N);

					Vector dp = p0 - p1;
					float p_squared = dot_product(dp, dp);
					result = (p_squared <= square(tube.radius + epsilon));
				}
			}
		}
	}
	
	return result;
}

//
// SHOULD USE GJK here instead.
//
bool MeshTube		(CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.tube_convexmesh++;

	bool result;

	if (!StdMeshesReady)
	{
		InitStdMeshes();
	}

	CollisionMesh * mesh = (CollisionMesh *) g1;
	XTube tube(*((Tube *) g2), x2);

	Vector half_axis = 0.5 * tube.length * tube.axis;
	Vector t0 = tube.center - half_axis;
	Vector t1 = tube.center + half_axis;

	StdLine.vertices[0].p = t0;
	StdLine.vertices[1].p = t1;
	Vector v1, v2, n;
	FindClosestPoints(*mesh, StdLine, x1.R, x1.x, x1.R, x1.x, v1, v2, n);

	Vector dv = v1 - v2;
	if (compute_contact)
	{
		data.contact = v1;
		dv.normalize();
		data.normal = dv;
	}
	else
	{
		float dist = dv.magnitude();
		result = (dist <= (tube.radius+epsilon));
	}

	return result;
}

//

bool TrilistTube	(CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

	bool result;

	CollisionMesh * mesh = (CollisionMesh *) g1;
	XTube tube(*((Tube *) g2), x2);

	Vector half_axis = 0.5 * tube.length * tube.axis;
	Vector t0 = tube.center - half_axis;
	Vector t1 = tube.center + half_axis;

	LineSegment tline(t0, t1);

	float min_dist = FLT_MAX;
	float min_r, min_s, min_t;
	int min_i;

	Triangle * tri = mesh->triangles;
	for (int i = 0; i < mesh->num_triangles; i++, tri++)
	{
		const Vector & b = mesh->vertices[tri->v[0]].p;
		Vector e0 = mesh->vertices[tri->v[1]].p - b;
		Vector e1 = mesh->vertices[tri->v[2]].p - b;
		float r, s, t;
		float dist = MinLineTriangle(tline, b, e0, e1, r, s, t);
		if (dist < min_dist)
		{
			min_dist = dist;
			min_i = i;
			min_r = r;
			min_s = s;
			min_t = t;
		}
	}

	if (compute_contact)
	{
		tri = mesh->triangles + min_i;
		const Vector & v0 = mesh->vertices[tri->v[0]].p;
		Vector e0 = mesh->vertices[tri->v[1]].p - v0;
		Vector e1 = mesh->vertices[tri->v[2]].p - v0;
		Vector tri_point = mesh->vertices[tri->v[0]].p + min_s * e0 + min_t * e1;

		data.contact = tri_point;
		data.normal = -mesh->normals[tri->normal];
	}
	else
	{
		result = (min_dist <= (tube.radius + epsilon));
	}

	return result;
}

//

bool TubeTube		(CollisionData & data, const GeometricPrimitive * g1, const XForm & x1, const GeometricPrimitive * g2, const XForm & x2, float epsilon, bool compute_contact)
{
	ASSERT(g1);
	ASSERT(g2);

//	Collision::collision_stats.tube_tube++;

	bool result;

	XTube tube0(*((Tube *) g1), x1);
	XTube tube1(*((Tube *) g2), x2);

	Vector half_axis = 0.5 * tube0.length * tube0.axis;
	Vector t0_0 = tube0.center + half_axis;
	Vector t0_1 = tube0.center - half_axis;

	half_axis = 0.5 * tube1.length * tube1.axis;
	Vector t1_0 = tube1.center + half_axis;
	Vector t1_1 = tube1.center - half_axis;

	LineSegment tube_seg0(t0_0, t0_1);
	LineSegment tube_seg1(t1_0, t1_1);

	Vector p0, p1;
	float s, t;
	ClosestLineLine(tube_seg0, tube_seg1, s, p0, t, p1);
	Vector dp = p0 - p1;

	if (compute_contact)
	{
		dp.normalize();
		data.contact = p1 + dp * tube1.radius;
		data.normal = dp;
	}
	else
	{
		float dist_squared = dot_product(dp, dp);
		result = (dist_squared <= square(tube0.radius + tube1.radius));
	}

	return result;
}

//

//
// Returns simple yes/no.
//
bool COMAPI Collision::collide_extents(const BaseExtent * extent1, const Transform & T1, const BaseExtent * extent2, const Transform & T2, float epsilon)
{
	ASSERT(extent1);
	ASSERT(extent2);

	bool result;

//
// Sphere/sphere check gets special treatment, since ideally every extent hierarchy has an outer
// bounding sphere as its root.
//
	if (extent1->type == ET_SPHERE && extent2->type == ET_SPHERE)
	{
//		collision_stats.sphere_sphere++;

		Vector c1 = T1.translation + T1.get_orientation() * extent1->xform.translation;
		Vector c2 = T2.translation + T2.get_orientation() * extent2->xform.translation;

		Vector dc = c1 - c2;
		float dist_squared = dot_product(dc, dc);

		float r1 = ((const SphereExtent *) extent1)->sphere.radius;
		float r2 = ((const SphereExtent *) extent2)->sphere.radius;

		float rsum_squared = square(r1 + r2 + epsilon);
		result = (dist_squared <= rsum_squared);
	}
	else if (extent1->type >= ET_LINE_SEGMENT && extent1->type < ET_NONE &&
			 extent2->type >= ET_LINE_SEGMENT && extent2->type < ET_NONE)
	{
		const GeometricPrimitive * gp1 = extent1->get_primitive();
		const GeometricPrimitive * gp2 = extent2->get_primitive();

	// Take extent's transform into account:
		Transform X1 = T1.multiply(extent1->xform);
		Transform X2 = T2.multiply(extent2->xform);

		const Vector & p1 = X1.translation;
		const Matrix & R1 = X1;

		const Vector & p2 = X2.translation;
		const Matrix & R2 = X2;

		TransformFunc xform = TFTable[extent1->type][extent2->type];
		XForm src1(p1, R1);
		XForm src2(p2, R2);
		XForm dst1, dst2;
		xform(dst1, src1, dst2, src2);

		CollisionData data;
		CollisionFunc func = CFTable[extent1->type][extent2->type];
	// Collision functions assume smaller comes in first.
		if (extent1->type <= extent2->type)
		{
			result = func(data, gp1, dst1, gp2, dst2, epsilon, false);
		}
		else
		{
			result = func(data, gp2, dst2, gp1, dst1, epsilon, false);
		}
	}
	else
	{
		result = false;
	}

	return result;
}

//

bool Collision::recurse_collide_extents(const BaseExtent *& intersect1, const BaseExtent *& intersect2,
											   const BaseExtent * e1, const Transform & T1,
											   const BaseExtent * e2, const Transform & T2, float epsilon)
{
	bool result = false;

	const BaseExtent * x1 = e1;
	const BaseExtent * x2 = e2;

	while (!result && x1)
	{
		while (!result && x2)
		{
			if (collide_extents(x1, T1, x2, T2, epsilon))
			{
			// deal.
				if (x1->is_leaf())
				{
					if (x2->is_leaf())
					{
						result = true;
						intersect1 = x1;
						intersect2 = x2;
						break;
					}
					else
					{
						x2 = x2->child;
					}
				}
				else if (x2->is_leaf())
				{
					x1 = x1->child;
				}
				else
				{
					x1 = x1->child;
					x2 = x2->child;
				}

				result = recurse_collide_extents(intersect1, intersect2, x1, T1, x2, T2, epsilon);
			}

			x2 = x2->next;
		}

		x1 = x1->next;
	}

	return result;
}


//

bool COMAPI Collision::collide_extent_hierarchies(const BaseExtent *& x1, const BaseExtent *& x2, const BaseExtent * root1, const Transform & T1, const BaseExtent * root2, const Transform & T2, float epsilon)
{
	bool result = false;

	const BaseExtent * ex1 = root1;
	while (ex1)
	{
		const BaseExtent * ex2 = root2;
		while (ex2)
		{
			if (recurse_collide_extents(x1, x2, ex1, T1, ex2, T2, epsilon))
			{
				result = true;
				break;
			}
			ex2 = ex2->next;
		}

		ex1 = ex1->next;
	}

	return result;
}

//

//
// Returns closest points between 2 extents.
//
extern bool GlobalHack;
void COMAPI Collision::compute_contact(CollisionData & data, const BaseExtent * x1, const Transform & T1, const BaseExtent * x2, const Transform & T2)
{
	ASSERT(x1);
	ASSERT(x2);

	if (x1->type >= ET_LINE_SEGMENT && x1->type < ET_NONE &&
		x2->type >= ET_LINE_SEGMENT && x2->type < ET_NONE)
	{
		const GeometricPrimitive * gp1 = x1->get_primitive();
		const GeometricPrimitive * gp2 = x2->get_primitive();

	// Take extent's transform into account:
		Transform X1 = T1.multiply(x1->xform);
		Transform X2 = T2.multiply(x2->xform);

		const Vector & p1 = X1.translation;
		const Matrix & R1 = X1;

		const Vector & p2 = X2.translation;
		const Matrix & R2 = X2;

		TransformFunc xform = TFTable[x1->type][x2->type];
		XForm src1(p1, R1);
		XForm src2(p2, R2);
		XForm dst1, dst2;
		xform(dst1, src1, dst2, src2);

		CollisionFunc func = CFTable[x1->type][x2->type];
	// Collision functions assume smaller comes in first.
		if (x1->type <= x2->type)
		{
			func(data, gp1, dst1, gp2, dst2, DefaultEpsilon, true);
		}
		else
		{
			func(data, gp2, dst2, gp1, dst1, DefaultEpsilon, true);
			data.normal = -data.normal;
		}

		if (xform == First2Second)
		{
			data.contact = p2 + R2 * data.contact;
			data.normal = R2 * data.normal;
		}
		else if (xform == Second2First)
		{
			data.contact = p1 + R1 * data.contact;
			data.normal = R1 * data.normal;
		}


	}
}

//

void COMAPI Collision::get_collision_stats(CollisionStats & stats)
{
//	stats = collision_stats;
//	memset(&collision_stats, 0, sizeof(collision_stats));
}

//
//
//
//
//

static float tolerance = 1e-5f;

//

bool RayLineSegment(float & t, Vector & p, Vector & normal, const Vector & base, const Vector & dir, const LineSegment & line)
{
	return false;
}

//

bool RayPlane(float & t, Vector & p, Vector & normal, const Vector & base, const Vector & dir, const Plane & plane)
{
	bool result;

	float denom = dot_product(dir, plane.N);
	if (fabs(denom) >= tolerance)
	{
		float num = -plane.compute_distance(base);
		t = num / denom;
		if (t >= 0.0f)
		{
			result = true;
			p = base + t * dir;
			normal = plane.N;
		}
		else
		{
		// Ray intersects plane behind origin.
			result = false;
		}
	}
	else
	{
	// Ray is parallel to plane.
		result = false;
	}

	return result;
}

//

bool RaySphere(float & t, Vector & p, Vector & N, const Vector & base, const Vector & dir, const XSphere & sphere)
{
	bool result;

	Vector dx = base - sphere.center;

	float r_squared = square(sphere.radius);

	float a = dot_product(dir, dir);
	float b = 2.0f * dot_product(dir, dx);
	float c = dot_product(dx, dx) - r_squared;
	float disc = square(b) - 4.0f * a * c;
	if (disc >= 0)
	{
		float sqrt_disc = sqrt(disc);
		float one_over_2a = 0.5f * 1.0f / a;
		float t0 = (-b - sqrt_disc) * one_over_2a;
		float t1 = (-b + sqrt_disc) * one_over_2a;

	// Need smaller non-negative root.
		if (t0 >= 0.0f)
		{
			if (t1 >= 0.0f)
			{
				t = Tmin(t0, t1);
			}
			else
			{
				t = t0;
			}
		}
		else if (t1 >= 0.0f)
		{
			t = t1;
		}
		else 
		{
			t = -1;
		}

		if (t >= 0.0f)
		{
			result = true;
			p = base + t * dir;
			N = p - sphere.center;
			N.normalize();
		}
		else
		{
			result = false;
		}
	}
	else
	{
	// No real roots, no intersection with sphere surface.
		result = false;
	}

	return result;
}

//

#define		SIDE	0		/* Object surface		*/
#define		BOT	1		/* Bottom end-cap surface	*/
#define		TOP	2		/* Top	  end-cap surface	*/

//

static bool ClipObj(const Vector & base, const Vector & dir, const Plane & bottom, const Plane & top, float & obj_in, float & obj_out, int & surf_in, int & surf_out)
{
	surf_in = surf_out = SIDE;
	float in  = obj_in;
	float out = obj_out;

// Intersect the ray with the bottom end-cap plane.

	float dc = dot_product(bottom.N, dir);
	float dw = bottom.compute_distance(base);

	if (fabs(dc) < tolerance) 
	{		
	// If parallel to bottom plane.
	    if (dw >= 0) 
		{
			return false;
		}		
	} 
	else 
	{
		float t  = - dw / dc;
		if (dc >= 0.0f) 
		{			    /* If far plane	*/
			if (t > in && t < out) 
			{ 
				out = t; 
				surf_out = BOT; 
			}
			if (t < in) 
			{
				return false;
			}
		} 
		else 
		{				    /* If near plane	*/
			if  (t > in && t < out)
			{
				in = t; 
				surf_in = BOT; 
			}
			if (t > out) 
			{
				return false;
			}
		}
	}

/*	Intersect the ray with the top end-cap plane.			*/

	dc = dot_product(top.N, dir);
	dw = top.compute_distance(base);

	if  (fabs(dc) < tolerance) 
	{		/* If parallel to top plane	*/
		if (dw >= 0.0f) 
		{
			return false;
		}
	} 
	else 
	{
	    float t = - dw / dc;
		if (dc >= 0.0f) 
		{			    /* If far plane	*/
			if  (t > in && t < out) 
			{
				out = t; 
				surf_out = TOP; 
			}
			if (t < in) 
			{
				return false;
			}
		} 
		else 
		{				    /* If near plane	*/
			if (t > in && t < out) 
			{ 
				in = t; 
				surf_in  = TOP; 
			}
			if (t > out) 
			{
				return false;
			}
		}
	}

	obj_in	= in;
	obj_out = out;

	return (in < out);
}


bool RayCylinder(float & t, Vector & p, Vector & N, const Vector & base, const Vector & dir, const XCylinder & cyl)
{
	bool result;

	float llen = dir.magnitude();
	Vector ldir = dir / llen;

	float half_len = 0.5f * cyl.length;

	Vector cbase = cyl.center - half_len * cyl.axis;
	Vector cdir = cyl.axis;

	Vector rel = base - cbase;

// Get vector perpendicular to both line and cylinder axis.
	Vector perp = cross_product(ldir, cdir);
	float pmag = perp.magnitude();

	float t_in, t_out;

// If magnitude == 0, line is parallel to cylinder axis.
	if (pmag < tolerance)
	{
	// line is parallel to cylinder, check distance.
		float d = dot_product(rel, cyl.axis);
		Vector D = rel - d * cyl.axis;
		d = D.magnitude();
		if (d <= cyl.radius)
		{
			result = true;
			t_in = -FLT_MAX;
			t_out = FLT_MAX;
		}
		else
		{
			result = false;
		}
	}
	else
	{
	// Not parallel. Normalize perp.
		perp /= pmag;

	// Compute distance of closest approach.
		float d = fabs(dot_product(rel, perp));
		if (d <= cyl.radius)
		{
		// Closest approach is within cylinder. Now compute vector
		// perpendicular to cylinder axis and perp.

			Vector o = cross_product(rel, cyl.axis);
			t = -dot_product(o, perp) / pmag;

			o = cross_product(perp, cyl.axis);
			o.normalize();
			float s = fabs(sqrt(square(cyl.radius) - square(d)) / dot_product(ldir, o));
			t_in = t - s; 
			t_out = t + s;

			result = true;
		}
		else
		{
			result = false;
		}
	}

	if (result)
	{
	// Compute endcap planes.
		Plane p0, p1;
		Vector half_axis = cyl.axis * half_len;
		Vector e0 = cyl.center - half_axis;
		p0.N = -cyl.axis;
		p0.D = -dot_product(p0.N, e0);

		Vector e1 = cyl.center + half_axis;
		p1.N = cyl.axis;
		p1.D = -dot_product(p1.N, e1);

		int in, out;
		if (ClipObj(base, ldir, p0, p1, t_in, t_out, in, out))
		{
			if (t_in >= 0.0f)
			{
				if (t_out >= 0.0f)
				{
					t = Tmin(t_in, t_out);
				}
				else
				{
					t = t_in;
				}
			}
			else if (t_out >= 0.0f)
			{
				t = t_out;
			}
			else 
			{
				t = -1;
			}

			t /= llen;
			if (t >= 0.0f)
			{
				result = true;
				p = base + t * dir;

			// compute normal:
				Vector dp = p - cbase;
				float d = dot_product(dp, cyl.axis);
				Vector paxis = cbase + d * cyl.axis;
				dp = p - paxis;
				float drad = dp.magnitude();
				if (fabs(drad - cyl.radius) < 1e-3)
				{
					N = dp;
					N.normalize();
				}
				else if (fabs(d) < 1e-3)
				{
					N = -cyl.axis;
				}
				else if (fabs(d - cyl.length) < 1e-3)
				{
					N = cyl.axis;
				}
			}
			else
			{
				result = false;
			}
		}
		else
		{
			result = false;
		}
	}

	return result;
}

//

#define RIGHT	0
#define LEFT	1
#define MIDDLE	2

//

bool RayBox(float & t, Vector & p, const Vector & base, const Vector & dir, const Box & box, Vector & normal)
{
	bool result;

	bool inside = true;
	int quadrant[3];
	int whichPlane;

	float maxT[3];
	float candidatePlane[3];

	float origin[3] = {base.x, base.y, base.z};
	float direction[3] = {dir.x, dir.y, dir.z};
	float minB[3] = {-box.half_x, -box.half_y, -box.half_z};
	float maxB[3] = { box.half_x,  box.half_y,  box.half_z};

	for (int i = 0; i < 3; i++)
	{
		if (origin[i] < minB[i]) 
		{  
			quadrant[i] = LEFT;
			candidatePlane[i] = minB[i];
			inside = FALSE;
		}
		else if (origin[i] > maxB[i]) 
		{
			quadrant[i] = RIGHT;
			candidatePlane[i] = maxB[i];
			inside = FALSE;
		}
		else	
		{
			quadrant[i] = MIDDLE;
		}
	}

// Ray origin inside bounding box
	if (inside)	
	{
		p = base;
		normal.zero();	// normal is meaningless.
		result = true;
	}
	else
	{
	// Calculate T distances to candidate planes
		for (i = 0; i < 3; i++)
		{
			if (quadrant[i] != MIDDLE && fabs(direction[i]) > tolerance)
			{
				maxT[i] = (candidatePlane[i] - origin[i]) / direction[i];
			}
			else
			{
				maxT[i] = -1.0f;
			}
		}

	// Get largest of the maxT's for final choice of intersection
		whichPlane = 0;
		for (i = 1; i < 3; i++)
		{
			if (maxT[whichPlane] < maxT[i])
			{
				whichPlane = i;
			}
		}

	// Check final candidate actually inside box
		if (maxT[whichPlane] < 0)
		{
			result = false;
		}
		else
		{
			t = maxT[whichPlane];

			float coord[3];

			for (i = 0; i < 3; i++)
			{
				if (whichPlane != i) 
				{
					coord[i] = origin[i] + maxT[whichPlane] * direction[i];
					if (coord[i] < minB[i] || coord[i] > maxB[i])
					{
						result = false;
						break;
					}
				}
				else 
				{
					coord[i] = candidatePlane[i];
					result = true;
				}
			}

			if (result)
			{
				p.set(coord[0], coord[1], coord[2]);
				normal.zero();
				switch (whichPlane)
				{
				case 0:
					normal.x = (coord[0] > 0) ? 1.0f : -1.0f;
					break;
				case 1:	 
					normal.y = (coord[1] > 0) ? 1.0f : -1.0f;
					break;
				case 2:
					normal.z = (coord[2] > 0) ? 1.0f : -1.0f;
					break;
				}
			}
		}
	}

	return result;
}

//

bool RayConvexMesh(float & t, Vector & p, Vector & N, const Vector & base, const Vector & dir, const CollisionMesh & mesh)
{
	bool result;

	if (buffer_length < mesh.num_normals)
	{
		if (buffer1)
		{
			delete [] buffer1;
		}
		if (buffer2)
		{
			delete [] buffer2;
		}

		buffer_length = mesh.num_normals;
		buffer1 = new float[mesh.num_normals];
		buffer2 = new float[mesh.num_normals];
	}

	float llen = dir.magnitude();
	Vector d = dir / llen;

	float tnear = -FLT_MAX;
	float tfar = FLT_MAX;
	int front_normal, back_normal;

	result = true;

	Vector * n = mesh.normals;
	for (int i = 0; i < mesh.num_normals; i++, n++)
	{
		buffer1[i] = dot_product(d, *n);
		buffer2[i] = dot_product(base, *n);
	}

	Triangle * tri = mesh.triangles;
	float * D = mesh.triangle_d;
	for (i = 0; i < mesh.num_triangles; i++, tri++, D++)
	{
	// vd = ray_direction dot plane_normal.
		float vd = buffer1[tri->normal];

	// vn = distance of ray_origin from plane.
		float vn = buffer2[tri->normal] + *D;

		if (fabs(vd) < tolerance)
		{
		// Ray is parallel to plane.
			if (vn > 0.0f)
			{
			// Ray is outside plane & parallel to plane, can't intersect object.
				result = false;
				break;
			}
		}
		else
		{
		// Ray isn't parallel to plane.

		// Find distance along ray to intersection with plane.
			t = -vn / vd;

			if (vd < 0.0f)
			{
			// Plane faces away from ray.
				if (t > tfar)
				{
					result = false;
					break;
				}

				if (t > tnear)
				{
					front_normal = tri->normal;
					tnear = t;
				}
			}
			else
			{
			// Plane faces ray.

				if (t < tnear)
				{
					result = false;
					break;
				}

				if (t < tfar)
				{
					back_normal = tri->normal;
					tfar = t;
				}
			}
		}
	}


	if (result)
	{
		if (tnear > 0.0f)
		{
			t = tnear;
			p = base + d * t;
			N = mesh.normals[front_normal];
		}
		else 
		{
			if (tfar > 0.0f && tfar < FLT_MAX)
			{
				t = tfar;
				p = base + d * t;
				N = mesh.normals[back_normal];
			}
			else
			{
				result = false;
			}
		}
	}

	return result;
}

//

bool RayTrilist(float & t, Vector & p, Vector & N, const Vector & base, const Vector & dir, const CollisionMesh & mesh)
{
	bool result=false;

	float tmin = FLT_MAX;

	Triangle * tri = mesh.triangles;
	for (int i = 0; i < mesh.num_triangles; i++, tri++)
	{
		Vector * v0 = &mesh.vertices[tri->v[0]].p;
		Vector * v1 = &mesh.vertices[tri->v[1]].p;
		Vector * v2 = &mesh.vertices[tri->v[2]].p;

		Vector edge1 = *v1 - *v0;
		Vector edge2 = *v2 - *v0;

		Vector pvec = cross_product(dir, edge2);
		float det = dot_product(edge1, pvec);
		if (det < tolerance)
		{
		//	result = false;
		}
		else
		{
			Vector tvec = base - *v0;
			float u = dot_product(tvec, pvec);
			if (u < 0.0f || u > det)
			{
			//	result = false;
			}
			else
			{
				Vector qvec = cross_product(tvec, edge1);
				float v = dot_product(dir, qvec);
				if (v < 0.0f || (u+v) > det)
				{
				//	result = false;
				}
				else
				{
					result = true;
					t = dot_product(edge2, qvec) / det;
					if (t < tmin)
					{
						tmin = t;
						p = base + t * dir;					
						N = mesh.normals[tri->normal];
					}
				}
			}
		}
	}

	t = tmin;

	return result;
}

//

// QUICK HACK, CLEAN UP LATER.
bool RayTube(float & t, Vector & p, Vector & N, const Vector & base, const Vector & dir, const XTube & tube)
{
	bool result = false;

	float	t0 = FLT_MAX, t1 = FLT_MAX, t2 = FLT_MAX;
	Vector	p0, p1, p2;
	Vector	N0, N1, N2;

	XCylinder cyl;
	memcpy(&cyl, &tube, sizeof(XTube));
	bool r0 = RayCylinder(t0, p0, N0, base, dir, cyl);

	Vector half_axis = tube.axis * 0.5f * tube.length;
	Vector te = tube.center - half_axis;
	Sphere s;
	s.radius = tube.radius;
	Transform T; T.set_identity(); T.set_position(te);
	XSphere sphere(s, T);
	bool r1 = RaySphere(t1, p1, N1, base, dir, sphere);

	te = tube.center + half_axis;
	sphere.center = te;
	bool r2 = RaySphere(t2, p2, N2, base, dir, sphere);

	if (r0 || r1 || r2)
	{
		result = true;

		float tmin = Tmin(Tmin(t0, t1), t2);
		if (tmin == t0)
		{
			t = t0;
			p = p0;
			N = N0;
		}
		else if (tmin == t1)
		{
			t = t1;
			p = p1;
			N = N1;
		}
		else if (tmin == t2)
		{
			t = t2;
			p = p2;
			N = N2;
		}
	}

	return result;
}

//

void TransformRayToObjectSpace(Vector & xorg, Vector & xdir, const Vector & org, const Vector & dir, const Transform & T)
{
	const Matrix & R = (Matrix) T;

	//Matrix RT = R.get_transpose();
	xorg = (org - T.translation) * R;		// multiply V * M is the same as M.T * V. 
	xdir = dir * R;
}

//

static bool internal_check_extent(float & t, Vector & point, Vector & normal, 
						   const Vector & origin, const Vector & direction,
						   const BaseExtent * x, const Transform & T)
{
	ASSERT(x);

	bool result = false;

	if (x->type >= ET_LINE_SEGMENT && x->type <= ET_TUBE)
	{
		const GeometricPrimitive * gp = x->get_primitive();

	// Take extent's transform into account:
		Transform X = T.multiply(x->xform);

		switch (x->type)
		{
			case ET_LINE_SEGMENT:
			{
				XLineSegment line(*((LineSegment *) gp), X);
				result = RayLineSegment(t, point, normal, origin, direction, line);
				break;
			}
			case ET_INFINITE_PLANE:
			{
				XPlane plane(*((Plane *) gp), X);
				result = RayPlane(t, point, normal, origin, direction, plane);
				break;
			}
			case ET_SPHERE:
			{
				XSphere sphere(*((Sphere *) gp), X);
				result = RaySphere(t, point, normal, origin, direction, sphere);
				break;
			}
			case ET_CYLINDER:
			{
				XCylinder cyl(*((Cylinder *) gp), X);
				result = RayCylinder(t, point, normal, origin, direction, cyl);
				break;
			}
			case ET_BOX:
			{
				Vector org, dir;
				TransformRayToObjectSpace(org, dir, origin, direction, X);

				result = RayBox(t, point, org, dir, *((Box *) gp), normal);
				if (result)
				{
					point = X.rotate_translate(point);
					normal = X.rotate(normal);
				}
				break;
			}
			case ET_CONVEX_MESH:
			{
				Vector org, dir;
				TransformRayToObjectSpace(org, dir, origin, direction, X);

				result = RayConvexMesh(t, point, normal, org, dir, (CollisionMesh &) *gp);
				if (result)
				{
					point = X.rotate_translate(point);
					normal = X.rotate(normal);
				}
				break;
			}
			case ET_GENERAL_MESH:
			{
				Vector org, dir;
				TransformRayToObjectSpace(org, dir, origin, direction, X);

				result = RayTrilist(t, point, normal, org, dir, (CollisionMesh &) *gp);
				if (result)
				{
					point = X.rotate_translate(point);
					normal = X.rotate(normal);
				}
				break;
			}
			case ET_TUBE:
			{
				XTube tube(*((Tube *) gp), X);
				result = RayTube(t, point, normal, origin, direction, tube);
				break;
			}
		}
	}
	else	
	{
		result = false;
	}

	return result;
}

//

bool COMAPI Collision::intersect_ray_with_extent(Vector & point, Vector & normal, const Vector & origin, const Vector & direction, const BaseExtent & extent, const Transform & T)
{
	float t;
	bool result = internal_check_extent(t, point, normal, origin, direction, &extent, T);
	return result;
}

//

static bool internal_check_hierarchy(float & t, Vector & point, Vector & normal, 
							  const Vector & org, const Vector & dir, 
							  const BaseExtent * root, const Transform & T)
{
	ASSERT(root);
	bool result = false;

	float tmin = FLT_MAX;
	Vector pmin, Nmin;

	const BaseExtent * x = root;
	while (x)
	{
		if (internal_check_extent(t, point, normal, org, dir, x, T))
		{
			bool hit = false;

			if (x->child)
			{
				hit = internal_check_hierarchy(t, point, normal, org, dir, x->child, T);
			}
			else
			{
			// this is a leaf.
				hit = true;
			}

			if (hit && (t < tmin))
			{
				tmin = t;
				pmin = point;
				Nmin = normal;

				result = true;
			}
		}
		
		x = x->next;
	}

	if (result)
	{
		t = tmin;
		point = pmin;
		normal = Nmin;
	}

	return result;
}

//

bool COMAPI Collision::intersect_ray_with_extent_hierarchy(	Vector & point, Vector & normal,
																	const Vector & origin, const Vector & direction, 
																	const BaseExtent & extent, 
																	const Transform & T, bool find_closest)
{
	bool result = false;

	float t;

	if (find_closest)
	{
		result = internal_check_hierarchy(t, point, normal, origin, direction, &extent, T);
	}
	else
	{
		const BaseExtent * x = &extent;

		while (!result && x)
		{
			result = internal_check_extent(t, point, normal, origin, direction, x, T);

			if (result && x->child)
			{
				result = intersect_ray_with_extent_hierarchy(point, normal, origin, direction, *x->child, T, false);
			}

			x = x->next;
		}
	}

	return result;
}

//
