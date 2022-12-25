//
// CORNERS.CPP - Code to compute bounding box for the latest Conquest selection scheme. 
// Computes the corners of a bounding slice through the middle of the object, including all
// child objects (hence the code complexity). These corners are in OBJECT SPACE. You can get
// them into world space at any given time using the following formula:
//
// corner_in_world_space = object_position_vector + object_orientation_matrix * corner_in_object_space
//
// CAVEAT: You must have called IEngine::update() AT LEAST ONCE on the object before calling
// ComputeCorners() or the child objects will not be correctly positioned with respect to
// their parent(s).
//
// Here's the only function you need to call:
//
// void ComputeCorners(Vector * corners, INSTANCE_INDEX index);
//
//

//#include "3dmath.h"
//#include "engine.h"
//#include "renderer.h"

//extern IEngine *	ENGINE;
//extern IRenderer *	REND;
//extern IModel *		MODEL;

//namespace corners
//{

static void MakeBox(Vector * verts, const float * bounds)
{
	verts[0].set(bounds[0], bounds[2], bounds[4]);
	verts[1].set(bounds[1], bounds[2], bounds[4]);
	verts[2].set(bounds[0], bounds[3], bounds[4]);
	verts[3].set(bounds[1], bounds[3], bounds[4]);
	verts[4].set(bounds[0], bounds[2], bounds[5]);
	verts[5].set(bounds[1], bounds[2], bounds[5]);
	verts[6].set(bounds[0], bounds[3], bounds[5]);
	verts[7].set(bounds[1], bounds[3], bounds[5]);
}

//

static void ExpandBox(float * dst, const float * src)
{
	if (src[0] > dst[0])
	{
		dst[0] = src[0];
	}
	if (src[1] < dst[1])
	{
		dst[1] = src[1];
	}
	if (src[2] > dst[2])
	{
		dst[2] = src[2];
	}
	if (src[3] < dst[3])
	{
		dst[3] = src[3];
	}
	if (src[4] > dst[4])
	{
		dst[4] = src[4];
	}
	if (src[5] < dst[5])
	{
		dst[5] = src[5];
	}
}

//

static void ExpandBox(float * dst, const Vector & v)
{
	if (v.x > dst[0])
	{
		dst[0] = v.x;
	}
	if (v.x < dst[1])
	{
		dst[1] = v.x;
	}
	if (v.y > dst[2])
	{
		dst[2] = v.y;
	}
	if (v.y < dst[3])
	{
		dst[3] = v.y;
	}
	if (v.z > dst[4])
	{
		dst[4] = v.z;
	}
	if (v.z < dst[5])
	{
		dst[5] = v.z;
	}
}

//
// WARNING: Recursive function.
//
static void ExpandBoundingBox(float * box, INSTANCE_INDEX parent)
{
// Get parent's bounding box.
	SINGLE local_box[6];
	HARCH arch = parent;
	if (REND->get_archetype_bounding_box(arch, LODPERCENT, local_box))
	{
		ExpandBox(box, local_box);
	}

// Get inverse of parent's transform in order to get child boxes 
// into parent's frame.
	Transform Xp = ENGINE->get_transform(parent);
	Transform Xpi = Xp.get_inverse();

	INSTANCE_INDEX c = ENGINE->get_instance_child_next(parent,0,INVALID_INSTANCE_INDEX);
	while (c != INVALID_INSTANCE_INDEX)
	{
	// Get child's bounding box.
		arch = c;
		if (REND->get_archetype_bounding_box(arch, LODPERCENT, local_box))
		{
		// Transform to parent's coordinate system, expand box.
			Vector v[8];
			MakeBox(v, local_box);

			Transform Xc = ENGINE->get_transform(c);
			Transform Xc2p = Xpi.multiply(Xc);

			Vector * vp = v;
			for (int i = 0; i < 8; i++, vp++)
			{
				*vp = Xc2p.rotate_translate(*vp);
				ExpandBox(box, *vp);
			}
		}

	// Recursively expand down.
		ExpandBoundingBox(box, c);

	// Go to next child.
		c = ENGINE->get_instance_child_next(parent, EN_DONT_RECURSE, c);
	}
}

//
static void ComputeCorners(float box[6], INSTANCE_INDEX index)
{
	for (int i = 0; i < 6; i++)
	{
		box[i] = 0.0;
	}

	ExpandBoundingBox(box, index);

// 0, 1 = min, max x
// 2, 3 = min, max y
// 4, 5 = min, max z
	
//	corners[0].set( box[0], 0, box[4]);
//	corners[1].set( box[1], 0, box[4]);
//	corners[2].set( box[0], 0, box[5]);
//	corners[3].set( box[1], 0, box[5]);
}

//}  // end of namespace corners
//

