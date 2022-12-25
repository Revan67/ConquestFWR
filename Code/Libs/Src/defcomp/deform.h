#ifndef DEFORM_H
#define DEFORM_H

#include <stdlib.h>

#include "FileSys.h"
#include "Engine.h"
#include "IAnim.h"
#include "StdDAT.h"
#include "system.h"
#include "FaceProp.h"
#include "FaceGroup.h"
#include "PatchGroup.h"
#include "TextureCoord.h"
#include "matrix4.h"

//

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
	bool	extra:1;

	BoneDescriptor(void)
	{
		memset(this, 0, sizeof(*this));
		index = -1;
	}

	~BoneDescriptor(void)
	{
		free();
	}

	void free(void)
	{
		if (object_name)
		{
			delete [] object_name;
			object_name = NULL;
		}
		if (file_name)
		{
			delete [] file_name;
			file_name = NULL;
		}
		if (vertices)
		{
			delete [] vertices;
			vertices = NULL;
		}
		if (normals)
		{
			delete [] normals;
			normals = NULL;
		}

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

	// Name of object, for use with IEngine::is_named().
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

	int				num_vertices;
	Vector *		vertices;
	Vector *		normals;

	int				num_faces;
	int *			faces;

	bool			extra:1;

	BoneArchetype(void)
	{
		memset(this, 0, sizeof(BoneArchetype));
	}

	~BoneArchetype(void)
	{
		delete [] vertices;
		delete [] normals;
		delete [] faces;
	}

	void init(const BoneDescriptor & desc)
	{
	// DEBUG
		if (desc.object_name)
		{
			strcpy(name, desc.object_name);
		}
	// DEBUG

		id = desc.index;

		num_vertices = desc.num_vertices;
		if (num_vertices)
		{
			vertices = new Vector[num_vertices];
			memcpy(vertices, desc.vertices, sizeof(Vector) * num_vertices);

			if(desc.normals) // patches don't have normals
			{
				normals = new Vector[num_vertices];
				memcpy(normals, desc.normals, sizeof(Vector) * num_vertices);
			}
		}

		extra = desc.extra;
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

	BoneInstance(void){}
	void InitBoneInstance(const BoneArchetype * arch, const bool need_normals); // must be called after constructor
	~BoneInstance(void)
	{
		delete [] transformed_vertices;
		delete [] transformed_normals;
	}
};

//

struct IKJoint
{
	JOINT_INDEX				idx;
	CHANNEL_INSTANCE_INDEX	channel;
	Quaternion				joint_data;		// joint data.
	Matrix					R;				// global orientation
	Vector					r;				// offset in R.
	Vector					p;				// global position.
	Quaternion				qmid;			// midpoint (relative) orientation.

	IKJoint(void) {}
	void init(JOINT_INDEX jnt, CHANNEL_INSTANCE_INDEX chan);
};

//

struct IKScriptLink : public Animation::IVirtualChannel
{
	HANDLE					handle;
	U32						num_joints;
	JOINT_INDEX *			joints;
	Matrix *				Rmid;
	INSTANCE_INDEX			root;
	INSTANCE_INDEX			end_effector;
	CHANNEL_INSTANCE_INDEX *channels;
	Quaternion *			data;
	const Vector &			point;
	const Matrix &			orient;
	U32						flags;
	bool *					locked;
	int						num_locked_joints;
	bool					done:1;
	bool					child_offset:1;
	float					damping_factor;
	int						num_bones_calcd;

	IKScriptLink(const Vector & pt, const Matrix & _orient) : point(pt), orient(_orient), done(false)
	{
		num_bones_calcd	=0;
	}

	~IKScriptLink(void);
	virtual int update(void * dst, U32 channel_idx, const Animation::Target & target, float time);

	void compute_forward_kinematics(Vector & p_eff, Matrix & R_eff, Vector * p_new, Matrix * R_new) const;

	void build_jacobian(void) const;
	bool solve(void);
	void solve_sr(void);
	void solve_sr_child_offset(void);

};

//

struct ScriptLink
{
	bool			ik_script;
	union
	{
		SCRIPT_INST		instance;
		IKScriptLink *	ik;
	};

	float			start_height;

	ScriptLink *	prev;
	ScriptLink *	next;

	ScriptLink(void)
	{
		ik_script = false;
		instance = -1;
	}

	void release(void);
};
	
//

#endif