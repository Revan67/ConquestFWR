//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "cbsp.h"
#include "cmesh.h"
#include "stddat.h"

//

void AddPlane(BSPNode * root, const Plane & plane)
{
	if (root->front)
	{
		AddPlane(root->front, plane);
		AddPlane(root->back, plane);
	}
	else
	{
		root->front = new BSPNode;
		root->front->plane = plane;
		root->back = new BSPNode;
		root->back->plane = plane;
	}
}

//

void InsertPoint(BSPNode * root, const Vector & p, int n)
{
	if (root->front)
	{
		float d = root->plane.compute_distance(p);
		if (d > 1e-5)
		{
			InsertPoint(root->front, p, n);
		}
		else if (d < -1e-5)
		{
			InsertPoint(root->back, p, n);
		}
		else
		{
			OutputDebugString("ouch\n");
			root->normal = n;
		}
	}
	else
	{
		root->normal = n;
	}
}

//

void FindLeafNodes(BSPNode * root, DynamicArray<BSPNode *> & leaf_nodes, int & num_leaf_nodes)
{
	if (root->front)
	{
		FindLeafNodes(root->front, leaf_nodes, num_leaf_nodes);
		if (root->back)
		{
			FindLeafNodes(root->back, leaf_nodes, num_leaf_nodes);
		}
	}
	else if (root->back)
	{
	 	FindLeafNodes(root->back, leaf_nodes, num_leaf_nodes);
	}
	else
	{
		leaf_nodes[num_leaf_nodes++] = root;
	}
}


//

void BSPTree::build(const CollisionMesh * mesh)
{
	Plane v[6];	// voronoi planes.

	v[0].init(mesh->center, mesh->vertices[0].p, mesh->vertices[3].p);
	v[1].init(mesh->center, mesh->vertices[7].p, mesh->vertices[4].p);
	v[2].init(mesh->center, mesh->vertices[4].p, mesh->vertices[5].p);
	v[3].init(mesh->center, mesh->vertices[0].p, mesh->vertices[1].p);
	v[4].init(mesh->center, mesh->vertices[3].p, mesh->vertices[7].p);
	v[5].init(mesh->center, mesh->vertices[4].p, mesh->vertices[0].p);

//
// Now create huge BSP tree with all these nodes.
//
	root = new BSPNode;
	root->plane = v[0];
	for (int i = 1; i < 6; i++)
	{
		AddPlane(root, v[i]);
	}

	int num_leaf_nodes = 0;
	DynamicArray<BSPNode *> leaf_nodes;
	FindLeafNodes(root, leaf_nodes, num_leaf_nodes);

//
// Traverse leaf nodes, compute normal at each.
//
	for (i = 0; i < num_leaf_nodes; i++)
	{

	}
/*
	Triangle * t = mesh->triangles;
	for (i = 0; i < mesh->num_triangles; i++, t++)
	{
		Vector c = mesh->vertices[t->v[0]].p;
		c += mesh->vertices[t->v[1]].p;
		c += mesh->vertices[t->v[2]].p;
		c /= 3.0;

		InsertPoint(root, c, t->normal);
	}
*/
}

//

int GetN(BSPNode * root, const Vector & p)
{
	int result;
	if (root->front || root->back)
	{
		float d = root->plane.compute_distance(p);
		if (d > 1e-5)
		{
			if (root->front)
			{
				result = GetN(root->front, p);
			}
			else
			{
				result = root->normal;
			}
		}
		else if (d < -1e-5)
		{
			if (root->back)
			{
				result = GetN(root->back, p);
			}
			else
			{
				result = root->normal;
			}
		}
	}
	else
	{
		result = root->normal;
	}
	return result;
}

//

int BSPTree::get_normal(const Vector & p)
{
	int result = GetN(root, p);
	return result;
}


//