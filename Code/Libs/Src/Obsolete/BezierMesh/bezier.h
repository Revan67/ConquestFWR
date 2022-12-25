#ifndef __BEZIER_H
#define __BEZIER_H

#include "Material.h"
#include "vector.h"
#include "PatchGroup.h"
#include "TextureCoord.h"


struct BezierMesh 
{
	int				patch_cnt;	// total number of patches in all groups
	int				group_cnt;	// number of patch groups
	PatchGroup *	patch_groups;// patch group list

	int				vertex_cnt;	// number of control vertices
	Vector *		vertices;	// control vertices
	
	int				uv_cnt;		// number of uv coordinates
	TexCoord *		uv_list;	// uv coordinates

	int				material_cnt; 
	Material *		material_list;

	Vector			sphere_center;	// bounding sphere center
	SINGLE			radius;

	SINGLE			bounds[6];		// min max x,y,z in object space
	Vector			centroid;		// average of all controll points

	int				edge_cnt;
	BezierEdge *	edges;

	mutable int		last_face_cnt;
	mutable int		last_vertex_cnt;

	void Release(void)
	{
		delete [] patch_groups;
		delete [] vertices;
		delete [] uv_list;
		delete [] material_list;
		delete [] edges;
	}

	BezierMesh(void)
	{
		memset(this, 0, sizeof(*this));
	}

	~BezierMesh()
	{
		Release();
	}

	bool read (struct IFileSystem * parent, struct ITXMLib * txm_lib);
	
	void compute_bounds (void);
	void compute_centroid (void);
	bool expand_bounding_box (float box[6]) const;
	// Fills in array of 8 vertices of bounding box (in object space).
	void get_bounding_box(Vector * verts) const;
	void copy_bezier(const BezierMesh & src);
};

#endif