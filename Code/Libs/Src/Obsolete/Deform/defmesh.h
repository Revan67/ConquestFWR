//
//
//

#ifndef DEFMESH_H
#define DEFMESH_H

//

#include "3dmath.h"
#include "filesys.h"
#include "engine.h"
#include "mesh.h"

/*

FUTURE WORK:

Archetype
	BoneDescriptors
	BaseMesh
	DeformableMesh extensions to basemesh.

Instance
	Bones (skeleton)
	Transformed vertices & normals.

*/


struct BoneDescriptor
{
	char *	object_name;
	char *	file_name;
	int		index;

	int		num_vertices;
	Vector *vertices;
	Vector *normals;

	static char * mesh_name;

	int		vertex_counter;

	BoneDescriptor(void)
	{
		memset(this, 0, sizeof(*this));
	}

	~BoneDescriptor(void)
	{
		free();
	}

	void free(void)
	{
		delete [] object_name;
		delete [] file_name;
		delete [] vertices;
		delete [] normals;
		if (mesh_name)
		{
			delete [] mesh_name;
			mesh_name = NULL;
		}
		
		memset(this, 0, sizeof(*this));     
	}

	void read(IFileSystem * file)
	{
		U32 bytes_read;

	// Name of object, for use with IModel::is_named().
		DAFILEDESC desc("Object name");
		desc.dwDesiredAccess = GENERIC_READ;
		desc.dwCreationDistribution = OPEN_EXISTING;
		HANDLE h = file->OpenChild(&desc);
		if (h != INVALID_HANDLE_VALUE)
		{
			int size = file->GetFileSize(h, NULL);
			if (size)
			{
				object_name = new char[size];
				file->ReadFile(h, object_name, size, &bytes_read);
			}
			file->CloseHandle(h);
		}

	// Name of file, to be read below.
		desc = "File name";
		h = file->OpenChild(&desc);
		if (h != INVALID_HANDLE_VALUE)
		{
			int size = file->GetFileSize(h, NULL);
			if (size)
			{
				file_name = new char[size];
				file->ReadFile(h, file_name, size, &bytes_read);
			}
			file->CloseHandle(h);
		}

	// Index.
		desc = "Index";
		h = file->OpenChild(&desc);
		if (h != INVALID_HANDLE_VALUE)
		{
			file->ReadFile(h, &index, sizeof(index), &bytes_read);
			file->CloseHandle(h);
		}
	}
};

//

struct BoneArchetype
{
// DEBUG
	char name[64];
// DEBUG

	int				id;

//	INSTANCE_INDEX	instance;

	int				num_vertices;
	Vector *		vertices;
	Vector *		normals;

	BoneArchetype(void)
	{
		memset(this, 0, sizeof(BoneArchetype));
	}

	~BoneArchetype(void)
	{
		delete [] vertices;
		delete [] normals;
	}

	void init(const BoneDescriptor & desc)
	{
	// DEBUG
		strcpy(name, desc.object_name);
	// DEBUG

		id = desc.index;

		num_vertices = desc.num_vertices;
		if (num_vertices)
		{
			vertices = new Vector[num_vertices];
			memcpy(vertices, desc.vertices, sizeof(Vector) * num_vertices);

			normals = new Vector[num_vertices];
			memcpy(normals, desc.normals, sizeof(Vector) * num_vertices);
		}
	}
};

//

struct BoneInstance
{
	const BoneArchetype *	arch;
	INSTANCE_INDEX			instance;
	Vector *				transformed_vertices;
	Vector *				transformed_normals;
	U32						vertex_counter;

	BoneInstance(const BoneArchetype * arch);
	~BoneInstance(void)
	{
		delete [] transformed_vertices;
		delete [] transformed_normals;
	}

};

//


struct DeformableMesh
{

	void render_skeleton(void);
	void render(void);

	bool visible_rect(RECT & rect, struct ICamera * camera);

};

//

#endif