//

#ifndef PATCHGROUP_H
#define PATCHGROUP_H

#include "vector.h"

struct BezierEdge
{
	int v1;			// vertex index 1
	int v2;			// vertex index 2

	int patch1;		// patch index 1
	int patch1_grp; // patch group index 1

	int patch2;
	int patch2_grp;
};

struct BezierPatch
{
	int	type;			// 3 tri; 4 quad;

	int	v[4];			// Can have 3 or 4 vertices
	int	vec[9];			// Can have 6 or 8 vector points // or holds aux indices for D4 tri patch
	int	interior[4];	// Can have 3 or 4 interior vertices
	int aux_index;		// Used for static triangular patches only -- index to Degree 4 control points
	int	tv[4];			// Texture verts 3 or 4
	int edges[4];		// 3 or 4 indices into edges
};

struct PatchGroup
{
	int				patch_cnt;	// number of patches
	BezierPatch	*	patch_list;	// patches

	int				mtl_id;

	PatchGroup(void)
	{
		mtl_id = -1;
		patch_cnt = 0;
		patch_list = NULL;
	}

	~PatchGroup(void)
	{
		delete [] patch_list;
	}
};


//

#endif