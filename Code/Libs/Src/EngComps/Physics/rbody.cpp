//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "fileutil.h"
#include "rbody.h"

//

void RigidBody::load(IFileSystem * file)
{
	IFileSystem * sub = OpenDirectory("Mass properties", file);
	if (sub)
	{
		Vector cm;

		LoadFile("Mass",			&mass,	sizeof(mass),	sub);
		LoadFile("Center of mass",  &cm,	sizeof(cm),		sub);
		LoadFile("Inertia tensor",	&Ibody,	sizeof(Ibody),	sub);

		arm = -cm;

		sub->Release();
	}

	sub = OpenDirectory("Extent tree", file);
	if (sub)
	{
	// NEW FORMAT.
		extents = load_extent_tree(sub);
		sub->Release();
	}
	else
	{
	// TRY OLD FORMAT.
		sub = OpenDirectory("Extent data", file);
		if (sub)
		{
			IFileSystem * ext = OpenDirectory("Bounding volume", sub);

			BaseExtent * obj_extents[2];
			obj_extents[0] = NULL;
			obj_extents[1] = NULL;

			if (ext)
			{
				IFileSystem * f = OpenDirectory("Sphere", ext);
				if (f)
				{
					Sphere sphere;
					Vector center;
					LoadFile("Center", &center, sizeof(Vector), f);
					LoadFile("Radius", &sphere.radius, sizeof(SINGLE), f);

				// Transform sphere center to center-of-mass coordinate system.
					center += arm;

					obj_extents[0] = new SphereExtent(sphere);
					obj_extents[0]->xform.set_identity();
					obj_extents[0]->xform.set_position(center);
					f->Release();
				}

				f = OpenDirectory("Box", ext);
				if (f)
				{
					Box box;
					Vector center;
					LoadFile("Center",	&center,		sizeof(Vector), f);
					LoadFile("half x",	&box.half_x,	sizeof(SINGLE), f);
					LoadFile("half y",	&box.half_y,	sizeof(SINGLE), f);
					LoadFile("half z",	&box.half_z,	sizeof(SINGLE), f);

				// Transform box center to center-of-mass coordinate system.
					center += arm;

					obj_extents[1] = new BoxExtent(box);
					obj_extents[1]->xform.set_identity();
					obj_extents[1]->xform.set_position(center);
					f->Release();
				}

				if (obj_extents[0])
				{
					extents = obj_extents[0];
					if (obj_extents[1])
					{
						extents->child = obj_extents[1];
					}
				}
				else if (obj_extents[1])
				{
					extents = obj_extents[1];
				}

				ext->Release();
			}

			sub->Release();
		}
	}

}

//

void LoadCollisionMesh(CollisionMesh * mesh, IFileSystem * file)
{
	LoadFile("Vertex count",	&mesh->num_vertices,	sizeof(mesh->num_vertices),				file);
	mesh->vertices = new Vertex[mesh->num_vertices];
	LoadFile("Vertex list",		mesh->vertices,			sizeof(Vertex) * mesh->num_vertices,	file);

	LoadFile("Edge count",		&mesh->num_edges,		sizeof(mesh->num_edges),				file);
	mesh->edges = new Edge[mesh->num_edges];
	LoadFile("Edge list",		mesh->edges,			sizeof(Edge) * mesh->num_edges,			file);

	LoadFile("Face count",		&mesh->num_triangles,	sizeof(mesh->num_triangles),			file);
	mesh->triangles = new Triangle[mesh->num_triangles];
	LoadFile("Face list",		mesh->triangles,		sizeof(Triangle) * mesh->num_triangles,	file);

	mesh->triangle_d = new float[mesh->num_triangles];
	LoadFile("Triangle D",		mesh->triangle_d,		sizeof(float) * mesh->num_triangles,	file);

	LoadFile("Normal count",	&mesh->num_normals,		sizeof(mesh->num_normals),				file);
	mesh->normals = new Vector[mesh->num_normals];
	LoadFile("Normal list",		mesh->normals,			sizeof(Vector) * mesh->num_normals,		file);

	LoadFile("Centroid",		&mesh->centroid,		sizeof(Vector),							file);
}

//
// RECURSIVE FUNCTION.
//
BaseExtent * RigidBody::load_extent_tree(IFileSystem * file)
{
	BaseExtent * result = NULL;

	WIN32_FIND_DATA found;
	HANDLE search = file->FindFirstFile("*.*", &found);
	if (search != INVALID_HANDLE_VALUE)
	{
		BaseExtent * prev = NULL;

		do
		{
			if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				BaseExtent * extent = NULL;

				IFileSystem * ext = OpenDirectory(found.cFileName, file);
				if (ext)
				{
					Transform extent_xform;
					LoadFile("Transform",	&extent_xform,	sizeof(extent_xform),	ext);

					if (strncmp(found.cFileName, "Plane", strlen("Plane")) == 0)
					{
						Plane plane;
						LoadFile("Normal",	&plane.N,	sizeof(plane.N),	ext);
						LoadFile("D",		&plane.D,	sizeof(plane.D),	ext);

						extent = new PlaneExtent(plane);
					}
					else if (strncmp(found.cFileName, "Sphere", strlen("Sphere")) == 0)
					{
						Sphere sphere;
						LoadFile("Radius",	&sphere.radius,	sizeof(sphere.radius),	ext);
						extent = new SphereExtent(sphere);
					}
					else if (strncmp(found.cFileName, "Cylinder", strlen("Cylinder")) == 0)
					{
						Cylinder cyl;
						LoadFile("Radius",	&cyl.radius,	sizeof(cyl.radius),	ext);
						LoadFile("Length",	&cyl.length,	sizeof(cyl.length),	ext);
						extent = new CylinderExtent(cyl);
					}
					else if (strncmp(found.cFileName, "Box", strlen("Box")) == 0)
					{
						Box box;
						LoadFile("half x",	&box.half_x,	sizeof(box.half_x),	ext);
						LoadFile("half y",	&box.half_y,	sizeof(box.half_y),	ext);
						LoadFile("half z",	&box.half_z,	sizeof(box.half_z),	ext);
						extent = new BoxExtent(box);
					}
					else if (strncmp(found.cFileName, "Convex mesh", strlen("Convex mesh")) == 0)
					{
						CollisionMesh * mesh = new CollisionMesh;
						LoadCollisionMesh(mesh, ext);

						extent = new ConvexMeshExtent(mesh);
					}
					else if (strncmp(found.cFileName, "Mesh", strlen("Mesh")) == 0)
					{
						CollisionMesh * mesh = new CollisionMesh;
						LoadCollisionMesh(mesh, ext);

						extent = new MeshExtent(mesh);
					}
					else if (strncmp(found.cFileName, "Tube", strlen("Tube")) == 0)
					{
						Tube tube;
						LoadFile("Radius",	&tube.radius,	sizeof(tube.radius),	ext);
						LoadFile("Length",	&tube.length,	sizeof(tube.length),	ext);
						extent = new TubeExtent(tube);
					}

					if (extent)
					{
						extent->xform = extent_xform;
						if (!result)
						{
							result = extent;
						}

						extent->name = (char *) LoadFile("Name", NULL, 0, ext);

						IFileSystem * children = OpenDirectory("Children", ext);
						if (children)
						{
							extent->child = load_extent_tree(children);
							children->Release();
						}

						if (prev)
						{
							prev->next = extent;
						}
						prev = extent;
					}
					ext->Release();
				}
			}

		} while (file->FindNextFile(search, &found));

		file->FindClose(search);
	}

	return result;
}

//

void RigidBody::delete_extent_tree(BaseExtent * root)
{
	BaseExtent * child = root->child;
	while (child)
	{
		BaseExtent * next = child->next;
		delete_extent_tree(child);
		child = next;
	}

	delete root;
}

//
